/* vim: set sw=4 sts=4 et foldmethod=syntax : */

#include "clique.hh"
#include "bit_graph.hh"
#include "template_voodoo.hh"

#include <algorithm>
#include <limits>
#include <iostream>
#include <atomic>
#include <mutex>
#include <condition_variable>
#include <numeric>
#include <chrono>
#include <thread>

using std::chrono::duration_cast;
using std::chrono::milliseconds;
using std::chrono::steady_clock;

namespace
{
    struct AtomicIncumbent
    {
        std::atomic<unsigned> value;

        AtomicIncumbent()
        {
            value.store(0, std::memory_order_seq_cst);
        }

        bool update(unsigned v)
        {
            while (true) {
                unsigned cur_v = value.load(std::memory_order_seq_cst);
                if (v > cur_v) {
                    if (value.compare_exchange_strong(cur_v, v, std::memory_order_seq_cst))
                        return true;
                }
                else
                    return false;
            }
        }

        bool beaten_by(unsigned v) const
        {
            return v > value.load(std::memory_order_seq_cst);
        }

        unsigned get() const
        {
            return value.load(std::memory_order_relaxed);
        }
    };

    template <typename Item_>
    class Queue
    {
        private:
            const bool _donations_possible;

            std::atomic<bool> _want_producer;

            /* These protect and watch all subsequent private members. */
            std::mutex _mutex;
            std::condition_variable _cond;

            std::list<Item_> _list;
            bool _initial_producer_done;
            std::atomic<bool> _want_donations; /* Readable without holding _mutex */

            /* We keep track of how many consumers are busy, to decide whether
             * donations might be produced or whether consumers can exit. */
            unsigned _number_busy;
            std::atomic<unsigned> _number_idle;

        public:
            /**
             * We need to know how many consumers we will have. We also allow
             * donations to be disabled. */
            Queue(unsigned number_of_dequeuers, bool donations_possible) :
                _donations_possible(donations_possible),
                _want_producer(true),
                _initial_producer_done(false),
                _number_busy(number_of_dequeuers),
                _number_idle(0)
            {
                /* Initially we don't want donations */
                _want_donations.store(false, std::memory_order_seq_cst);
            }

            ~Queue() = default;
            Queue(const Queue &) = delete;
            Queue & operator= (const Queue &) = delete;

            /**
             * Called by the initial producer when producing work.
             *
             * To avoid the queue from becoming huge, we block if there are
             * more than size elements pending.
             */
            void enqueue_blocking(Item_ && item, unsigned size)
            {
                std::unique_lock<std::mutex> guard(_mutex);

                /* Don't let the queue get too full. */
                while (_list.size() > size)
                    _cond.wait(guard);

                _list.push_back(std::move(item));

                /* We're not empty, so we don't want donations. */
                _want_donations.store(false, std::memory_order_seq_cst);

                /* Something may be waiting in dequeue_blocking(). */
                _cond.notify_all();
            }

            /**
             * Called by consumers when donating work.
             *
             * May be called even if donations are not being requested.
             */
            void enqueue(Item_ && item)
            {
                std::unique_lock<std::mutex> guard(_mutex);

                _list.push_back(std::move(item));

                /* We are no longer empty, so we don't want donations */
                _want_donations.store(false, std::memory_order_seq_cst);

                /* Something may be waiting in dequeue_blocking(). */
                _cond.notify_all();
            }

            /**
             * Called by consumers waiting for work.
             *
             * Blocks until an item is available, and places that item in the
             * parameter. Returns true if a work item was provided, and false
             * otherwise. A false return value means the consumer should exit.
             *
             * We shouldn't return false if the initial producer is still
             * running, or if any other consumer thread is still going (since
             * it may donate work). But we must return false if the initial
             * producer is done, and if all the consumer threads are waiting
             * for something to do.
             */
            bool dequeue_blocking(Item_ & item)
            {
                std::unique_lock<std::mutex> guard(_mutex);
                while (true) {
                    if (! _list.empty()) {
                        /* We have something to do. */
                        item = std::move(*_list.begin());
                        _list.pop_front();

                        if (_initial_producer_done && _list.empty()) {
                            /* We're now empty, and the initial producer isn't
                             * going to give us anything else, so request
                             * donations. */
                            _want_donations.store(true, std::memory_order_seq_cst);
                        }

                        /* Something might be waiting in enqueue_blocking for
                         * space to become available. */
                        _cond.notify_all();

                        return true;
                    }

                    /* We're either about to block or give up, so we are no
                     * longer busy. */
                    --_number_busy;
                    ++_number_idle;

                    if (_initial_producer_done && ((! want_donations()) || (0 == _number_busy))) {
                        /* The list is empty, and nothing new can possibly be
                         * produced, so we're done. Other threads may be
                         * waiting for _number_busy to reach 0, so we need to
                         * wake them up. */
                        _cond.notify_all();

                        return false;
                    }

                    _cond.wait(guard);

                    /* We're potentially busy again (if this is a spurious
                     * wakeup, none of the conditions will be met back around
                     * the loop, and we'll decrement it again. */
                    ++_number_busy;
                    --_number_idle;
                }

                return false;
            }

            /**
             * Must be called when the initial producer is finished.
             */
            void initial_producer_done()
            {
                std::unique_lock<std::mutex> guard(_mutex);

                _initial_producer_done = true;

                /* The list might be empty, if consumers dequeued quickly. In
                 * that case, we now want donations. */
                if (_list.empty())
                    _want_donations.store(true, std::memory_order_seq_cst);

                /* Maybe every consumer is finished and waiting in
                 * dequeue_blocking(). */
                _cond.notify_all();
            }

            /**
             * If a consumer has an opportunity to donate, should it?
             */
            bool want_donations()
            {
                /* No locking. It's ok if we see an 'older' value of
                 * _want_donations. */
                return _donations_possible &&
                    _number_idle.load(std::memory_order_relaxed) > 0 &&
                    _want_donations.load(std::memory_order_relaxed);
            }

            /**
             * Do we want a producer?
             *
             * This will return true exactly once. Use of this is optional.
             */
            bool want_producer()
            {
                bool result = true;
                return _want_producer.compare_exchange_strong(result, false);
            }
    };

    const constexpr int number_of_depths = 5;
    const constexpr int number_of_steal_points = number_of_depths - 1;

    struct Subproblem
    {
        std::vector<int> offsets;
    };

    struct QueueItem
    {
        Subproblem subproblem;
    };

    struct StealPoint
    {
        std::mutex mutex;
        std::condition_variable cv;

        bool is_finished;

        bool has_data;
        std::vector<int> data;

        bool was_stolen;

        StealPoint() :
            is_finished(false),
            has_data(false),
            was_stolen(false)
        {
            mutex.lock();
        }

        void publish(std::vector<int> & s)
        {
            if (is_finished)
                return;

            data = s;
            has_data = true;
            cv.notify_all();
            mutex.unlock();
        }

        bool steal(std::vector<int> & s) __attribute__((noinline))
        {
            std::unique_lock<std::mutex> guard(mutex);

            while ((! has_data) && (! is_finished))
                cv.wait(guard);

            if (! is_finished && has_data) {
                s = data;
                was_stolen = true;
                return true;
            }
            else
                return false;
        }

        bool unpublish_and_keep_going()
        {
            if (is_finished)
                return true;

            mutex.lock();
            has_data = false;
            return ! was_stolen;
        }

        void finished()
        {
            is_finished = true;
            has_data = false;
            cv.notify_all();
            mutex.unlock();
        }
    };

    struct alignas(16) StealPoints
    {
        std::vector<StealPoint> points;

        StealPoints() :
            points{ number_of_steal_points }
        {
        }
    };

    using Association = std::map<std::pair<unsigned, unsigned>, unsigned>;
    using AssociatedEdges = std::vector<std::pair<unsigned, unsigned> >;

    auto modular_product(const VFGraph & g1, const VFGraph & g2) -> std::pair<Association, AssociatedEdges>
    {
        unsigned next_vertex = 0;
        Association association;
        AssociatedEdges edges;

        for (unsigned l = 0 ; l < std::min(g1.vertices_by_label.size(), g2.vertices_by_label.size()) ; ++l)
            for (unsigned v1 : g1.vertices_by_label[l])
                for (unsigned v2 : g2.vertices_by_label[l])
                    for (unsigned m = 0 ; m < std::min(g1.vertices_by_label.size(), g2.vertices_by_label.size()) ; ++m)
                        for (unsigned w1 : g1.vertices_by_label[m])
                            for (unsigned w2 : g2.vertices_by_label[m])
                                if (v1 != w1 && v2 != w2
                                        && (g1.edges[v1][v1] == g2.edges[v2][v2])
                                        && (g1.edges[w1][w1] == g2.edges[w2][w2])
                                        && (g1.edges[v1][w1] == g2.edges[v2][w2])
                                        && (g1.edges[w1][v1] == g2.edges[w2][v2])) {

                                    if (! association.count({v1, v2}))
                                        association.insert({{v1, v2}, next_vertex++});
                                    if (! association.count({w1, w2}))
                                        association.insert({{w1, w2}, next_vertex++});

                                    edges.push_back({association.find({v1, v2})->second, association.find({w1, w2})->second});
                                }

        return { association, edges };
    }

    auto unproduct(const Association & association, unsigned v) -> std::pair<unsigned, unsigned>
    {
        for (auto & a : association)
            if (a.second == v)
                return a.first;

        throw "oops";
    }


    template <unsigned n_words_>
    struct TCliqueConnectedMCS
    {

        const Association & association;
        const Params & params;

        FixedBitGraph<n_words_> graph;
        std::vector<int> order, invorder;
        AtomicIncumbent best_anywhere;

        std::atomic<unsigned long long> nodes;

        std::vector<FixedBitSet<n_words_> > vertices_adjacent_to_by_g1;

        TCliqueConnectedMCS(const std::pair<Association, AssociatedEdges> & g, const Params & q, const VFGraph & g1) :
            association(g.first),
            params(q),
            order(g.first.size()),
            invorder(g.first.size()),
            nodes(0),
            vertices_adjacent_to_by_g1(g1.size)
        {
            best_anywhere.value = params.prime;

            // populate our order with every vertex initially
            std::iota(order.begin(), order.end(), 0);

            // pre-calculate degrees
            std::vector<int> degrees;
            degrees.resize(g.first.size());
            for (auto & f : g.second) {
                ++degrees[f.first];
                ++degrees[f.second];
            }

            // sort on degree
            std::sort(order.begin(), order.end(),
                    [&] (int a, int b) { return true ^ (degrees[a] < degrees[b] || (degrees[a] == degrees[b] && a > b)); });

            // re-encode graph as a bit graph
            graph.resize(g.first.size());

            for (unsigned i = 0 ; i < order.size() ; ++i)
                invorder[order[i]] = i;

            for (auto & f : g.second)
                graph.add_edge(invorder[f.first], invorder[f.second]);

            if (params.connected) {
                for (unsigned i = 0 ; i < g1.size ; ++i) {
                    for (int j = 0 ; j < graph.size() ; ++j)
                        if (0 != g1.edges.at(i).at(unproduct(association, order[j]).first))
                            vertices_adjacent_to_by_g1.at(i).set(j);
                }
            }
        }

        auto run() -> Result
        {
            Result global_result;
            std::mutex global_result_mutex;

            /* work queues */
            std::vector<std::unique_ptr<Queue<QueueItem> > > queues;
            for (unsigned depth = 0 ; depth < number_of_depths ; ++depth)
                queues.push_back(std::unique_ptr<Queue<QueueItem> >{ new Queue<QueueItem>{ params.n_threads, false } });

            /* initial job */
            queues[0]->enqueue(QueueItem{ Subproblem{ std::vector<int>{} } });
            if (queues[0]->want_producer())
                queues[0]->initial_producer_done();

            /* threads and steal points */
            std::list<std::thread> threads;
            std::vector<StealPoints> thread_steal_points(params.n_threads);

            // initial colouring
            std::array<unsigned, n_words_ * bits_per_word> initial_p_order;
            std::array<unsigned, n_words_ * bits_per_word> initial_colours;
            {
                FixedBitSet<n_words_> initial_p;
                initial_p.set_up_to(graph.size());
                colour_class_order(initial_p, initial_p_order, initial_colours);
            }

            /* workers */
            for (unsigned i = 0 ; i < params.n_threads ; ++i) {
                threads.push_back(std::thread([&, i] {
                            auto start_time = steady_clock::now(); // local start time
                            auto overall_time = duration_cast<milliseconds>(steady_clock::now() - start_time);

                            Result local_result; // local result

                            for (unsigned depth = 0 ; depth < number_of_depths ; ++depth) {
                                if (queues[depth]->want_producer()) {
                                    /* steal */
                                    for (unsigned j = 0 ; j < params.n_threads ; ++j) {
                                        if (j == i)
                                            continue;

                                        std::vector<int> stole;
                                        if (thread_steal_points.at(j).points.at(depth - 1).steal(stole)) {
                                            stole.pop_back();
                                            for (auto & s : stole)
                                                --s;
                                            while (++stole.back() < graph.size())
                                                queues[depth]->enqueue(QueueItem{ Subproblem{ stole } });
                                        }
                                    }
                                    queues[depth]->initial_producer_done();
                                }

                                while (true) {
                                    // get some work to do
                                    QueueItem args;
                                    if (! queues[depth]->dequeue_blocking(args))
                                        break;

                                    std::vector<unsigned> c;
                                    c.reserve(graph.size());

                                    FixedBitSet<n_words_> p; // local potential additions
                                    p.set_up_to(graph.size());

                                    std::vector<int> position;
                                    position.reserve(graph.size());
                                    position.push_back(0);

                                    // do some work
                                    if (params.connected) {
                                        FixedBitSet<n_words_> a;
                                        expand_connected(c, p, a, initial_p_order, initial_colours, position, local_result,
                                                &args.subproblem, &thread_steal_points.at(i));
                                    }
                                    else
                                        expand(c, p, initial_p_order, initial_colours, position, local_result,
                                                &args.subproblem, &thread_steal_points.at(i));

                                    // record the last time we finished doing useful stuff
                                    overall_time = duration_cast<milliseconds>(steady_clock::now() - start_time);
                                }

                                if (depth < number_of_steal_points)
                                    thread_steal_points.at(i).points.at(depth).finished();
                            }

                            // merge results
                            {
                                std::unique_lock<std::mutex> guard(global_result_mutex);
                                global_result.merge(local_result);
                                global_result.times.push_back(overall_time);
                            }
                            }));
            }

            // wait until they're done, and clean up threads
            for (auto & t : threads)
                t.join();

            return global_result;
        }

        auto colour_class_order(
                const FixedBitSet<n_words_> & p,
                std::array<unsigned, n_words_ * bits_per_word> & p_order,
                std::array<unsigned, n_words_ * bits_per_word> & p_bounds) -> void
        {
            FixedBitSet<n_words_> p_left = p; // not coloured yet
            unsigned colour = 0;         // current colour
            unsigned i = 0;              // position in p_bounds

            // while we've things left to colour
            while (! p_left.empty()) {
                // next colour
                ++colour;
                // things that can still be given this colour
                FixedBitSet<n_words_> q = p_left;

                // while we can still give something this colour
                while (! q.empty()) {
                    // first thing we can colour
                    int v = q.first_set_bit();
                    p_left.unset(v);
                    q.unset(v);

                    // can't give anything adjacent to this the same colour
                    graph.intersect_with_row_complement(v, q);

                    // record in result
                    p_bounds[i] = colour;
                    p_order[i] = v;
                    ++i;
                }
            }
        }

        auto connected_colour_class_order(
                const FixedBitSet<n_words_> & p,
                const FixedBitSet<n_words_> & a,
                std::array<unsigned, n_words_ * bits_per_word> & p_order,
                std::array<unsigned, n_words_ * bits_per_word> & p_bounds) -> void
        {
            unsigned colour = 0;         // current colour
            unsigned i = 0;              // position in p_bounds

            FixedBitSet<n_words_> p_left = p; // not coloured yet
            p_left.intersect_with_complement(a);

            // while we've things left to colour
            while (! p_left.empty()) {
                // next colour
                ++colour;
                // things that can still be given this colour
                FixedBitSet<n_words_> q = p_left;

                // while we can still give something this colour
                while (! q.empty()) {
                    // first thing we can colour
                    int v = q.first_set_bit();
                    p_left.unset(v);
                    q.unset(v);

                    // can't give anything adjacent to this the same colour
                    graph.intersect_with_row_complement(v, q);

                    // record in result
                    p_bounds[i] = colour;
                    p_order[i] = v;
                    ++i;
                }
            }

            p_left = p;
            p_left.intersect_with(a);

            // while we've things left to colour
            while (! p_left.empty()) {
                // next colour
                ++colour;
                // things that can still be given this colour
                FixedBitSet<n_words_> q = p_left;

                // while we can still give something this colour
                while (! q.empty()) {
                    // first thing we can colour
                    int v = q.first_set_bit();
                    p_left.unset(v);
                    q.unset(v);

                    // can't give anything adjacent to this the same colour
                    graph.intersect_with_row_complement(v, q);

                    // record in result
                    p_bounds[i] = colour;
                    p_order[i] = v;
                    ++i;
                }
            }
        }

        auto expand(
                std::vector<unsigned> & c,
                FixedBitSet<n_words_> & p,
                const std::array<unsigned, n_words_ * bits_per_word> & p_order,
                const std::array<unsigned, n_words_ * bits_per_word> & colours,
                std::vector<int> & position,
                Result & local_result,
                Subproblem * const subproblem,
                StealPoints * const steal_points
                ) -> void
        {
            ++local_result.nodes;

            int skip = 0, stop = std::numeric_limits<int>::max();
            bool keep_going = true;

            if (subproblem && c.size() < subproblem->offsets.size()) {
                skip = subproblem->offsets.at(c.size());
                keep_going = false;
            }

            // for each v in p... (v comes later)
            for (int n = p.popcount() - 1 ; n >= 0 ; --n) {
                ++position.back();

                // bound, timeout or early exit?
                unsigned best_anywhere_value = best_anywhere.get();
                if (c.size() + colours[n] <= best_anywhere_value || params.abort->load())
                    return;

                auto v = p_order[n];

                if (skip > 0) {
                    --skip;
                    p.unset(v);
                }
                else {
                    // consider taking v
                    c.push_back(v);

                    // filter p to contain vertices adjacent to v
                    FixedBitSet<n_words_> new_p = p;
                    graph.intersect_with_row(v, new_p);

                    if (new_p.empty()) {
                        if (best_anywhere.update(c.size())) {
                            local_result.isomorphism.clear();
                            for (auto & v : c)
                                local_result.isomorphism.insert(unproduct(association, order[v]));
                        }
                    }
                    else {
                        position.push_back(0);
                        std::array<unsigned, n_words_ * bits_per_word> new_p_order;
                        std::array<unsigned, n_words_ * bits_per_word> new_colours;
                        colour_class_order(new_p, new_p_order, new_colours);

                        if (steal_points && c.size() < number_of_steal_points)
                            steal_points->points.at(c.size() - 1).publish(position);

                        expand(c, new_p, new_p_order, new_colours, position, local_result,
                                subproblem && c.size() < subproblem->offsets.size() ? subproblem : nullptr,
                                steal_points && c.size() < number_of_steal_points ? steal_points : nullptr);

                        if (steal_points && c.size() < number_of_steal_points)
                            keep_going = steal_points->points.at(c.size() - 1).unpublish_and_keep_going() && keep_going;

                        position.pop_back();
                    }

                    // now consider not taking v
                    c.pop_back();
                    p.unset(v);

                    keep_going = keep_going && (--stop > 0);

                    if (! keep_going)
                        break;
                }
            }
        }

        auto expand_connected(
                std::vector<unsigned> & c,
                FixedBitSet<n_words_> & p,
                const FixedBitSet<n_words_> & a,
                const std::array<unsigned, n_words_ * bits_per_word> & p_order,
                const std::array<unsigned, n_words_ * bits_per_word> & colours,
                std::vector<int> & position,
                Result & local_result,
                Subproblem * const subproblem,
                StealPoints * const steal_points
                ) -> void
        {
            ++local_result.nodes;

            int skip = 0, stop = std::numeric_limits<int>::max();
            bool keep_going = true;

            if (subproblem && c.size() < subproblem->offsets.size()) {
                skip = subproblem->offsets.at(c.size());
                keep_going = false;
            }

            // for each v in p... (v comes later)
            for (int n = p.popcount() - 1 ; n >= 0 ; --n) {
                ++position.back();

                // bound, timeout or early exit?
                unsigned best_anywhere_value = best_anywhere.get();
                if (c.size() + colours[n] <= best_anywhere_value || params.abort->load())
                    return;

                auto v = p_order[n];

                if (! c.empty() && ! a.test(v))
                    break;

                if (skip > 0) {
                    --skip;
                    p.unset(v);
                }
                else {
                    // consider taking v
                    c.push_back(v);

                    // filter p to contain vertices adjacent to v
                    FixedBitSet<n_words_> new_p = p;
                    graph.intersect_with_row(v, new_p);

                    if (best_anywhere.update(c.size())) {
                        local_result.isomorphism.clear();
                        for (auto & v : c)
                            local_result.isomorphism.insert(unproduct(association, order[v]));
                    }

                    if (! new_p.empty()) {
                        position.push_back(0);

                        FixedBitSet<n_words_> new_a = a;
                        new_a.union_with(vertices_adjacent_to_by_g1.at(unproduct(association, order[v]).first));

                        std::array<unsigned, n_words_ * bits_per_word> new_p_order;
                        std::array<unsigned, n_words_ * bits_per_word> new_colours;

                        connected_colour_class_order(new_p, new_a, new_p_order, new_colours);

                        if (steal_points && c.size() < number_of_steal_points)
                            steal_points->points.at(c.size() - 1).publish(position);

                        expand_connected(c, new_p, new_a, new_p_order, new_colours, position, local_result,
                                subproblem && c.size() < subproblem->offsets.size() ? subproblem : nullptr,
                                steal_points && c.size() < number_of_steal_points ? steal_points : nullptr);

                        if (steal_points && c.size() < number_of_steal_points)
                            keep_going = steal_points->points.at(c.size() - 1).unpublish_and_keep_going() && keep_going;

                        position.pop_back();
                    }

                    // now consider not taking v
                    c.pop_back();
                    p.unset(v);

                    keep_going = keep_going && (--stop > 0);

                    if (! keep_going)
                        break;
                }
            }
        }
    };

    template <template <unsigned> class SGI_>
    struct Apply
    {
        template <unsigned n_words_, typename> using Type = SGI_<n_words_>;
    };
}

auto clique_subgraph_isomorphism(const std::pair<VFGraph, VFGraph> & graphs, const Params & params) -> Result
{
    auto product = modular_product(graphs.first, graphs.second);

    return select_graph_size<Apply<TCliqueConnectedMCS>::template Type, Result>(AllGraphSizes(), product, params, graphs.first);
}

