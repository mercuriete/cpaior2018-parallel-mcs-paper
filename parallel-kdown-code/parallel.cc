/* vim: set sw=4 sts=4 et foldmethod=syntax : */

#include "parallel.hh"

#include <algorithm>
#include <array>
#include <atomic>
#include <chrono>
#include <condition_variable>
#include <functional>
#include <limits>
#include <list>
#include <map>
#include <numeric>
#include <random>
#include <thread>
#include <tuple>
#include <utility>
#include <vector>

#include <iostream>

#include <boost/dynamic_bitset.hpp>

using std::array;
using std::atomic;
using std::condition_variable;
using std::fill;
using std::find_if;
using std::function;
using std::get;
using std::greater;
using std::list;
using std::make_pair;
using std::map;
using std::mutex;
using std::mt19937;
using std::numeric_limits;
using std::pair;
using std::thread;
using std::to_string;
using std::tuple;
using std::uniform_int_distribution;
using std::unique;
using std::unique_lock;
using std::vector;

using std::chrono::duration_cast;
using std::chrono::milliseconds;
using std::chrono::steady_clock;

using std::cerr;
using std::endl;

namespace
{
    /// We'll use an array of unsigned long longs to represent our bits.
    using BitWord = unsigned long long;

    /// Number of bits per word.
    static const constexpr int bits_per_word = sizeof(BitWord) * 8;

    /**
     * A bitset with a fixed maximum size. This only provides the operations
     * we actually use in the bitset algorithms: it's more readable this way
     * than doing all the bit voodoo inline.
     *
     * Indices start at 0.
     */
    template <unsigned words_>
    class FixedBitSet
    {
        private:
            using Bits = array<BitWord, words_>;

            Bits _bits = {{ }};

        public:
            FixedBitSet() = default;

            FixedBitSet(unsigned size)
            {
                assert(size <= words_ * bits_per_word);
            }

            FixedBitSet(const FixedBitSet &) = default;

            FixedBitSet & operator= (const FixedBitSet &) = default;

            /**
             * Set a given bit 'on'.
             */
            auto set(int a) -> void
            {
                // The 1 does have to be of type BitWord. If we just specify a
                // literal, it ends up being an int, and it isn't converted
                // upwards until after the shift is done.
                _bits[a / bits_per_word] |= (BitWord{ 1 } << (a % bits_per_word));
            }

            /**
             * Set a given bit 'off'.
             */
            auto reset(int a) -> void
            {
                _bits[a / bits_per_word] &= ~(BitWord{ 1 } << (a % bits_per_word));
            }

            auto reset() -> void
            {
                for (auto & p : _bits)
                    p = 0;
            }

            /**
             * Is a given bit on?
             */
            auto operator[] (int a) const -> bool
            {
                return _bits[a / bits_per_word] & (BitWord{ 1 } << (a % bits_per_word));
            }

            /**
             * How many bits are on?
             */
            auto count() const -> unsigned
            {
                unsigned result = 0;
                for (auto & p : _bits)
                    result += __builtin_popcountll(p);
                return result;
            }

            /**
             * Are any bits on?
             */
            auto none() const -> bool
            {
                for (auto & p : _bits)
                    if (0 != p)
                        return false;
                return true;
            }

            /**
             * Intersect (bitwise-and) with another set.
             */
            auto operator&= (const FixedBitSet<words_> & other) -> FixedBitSet &
            {
                for (typename Bits::size_type i = 0 ; i < words_ ; ++i)
                    _bits[i] = _bits[i] & other._bits[i];
                return *this;
            }

            auto operator& (const FixedBitSet & other) const -> FixedBitSet
            {
                FixedBitSet result;
                for (typename Bits::size_type i = 0 ; i < words_ ; ++i)
                    result._bits[i] = _bits[i] & other._bits[i];
                return result;
            }

            /**
             * Union (bitwise-or) with another set.
             */
            auto operator|= (const FixedBitSet<words_> & other) -> FixedBitSet &
            {
                for (typename Bits::size_type i = 0 ; i < words_ ; ++i)
                    _bits[i] = _bits[i] | other._bits[i];
                return *this;
            }

            auto operator| (const FixedBitSet & other) const -> FixedBitSet
            {
                FixedBitSet result;
                for (typename Bits::size_type i = 0 ; i < words_ ; ++i)
                    result._bits[i] = _bits[i] | other._bits[i];
                return result;
            }

            static const constexpr unsigned npos = numeric_limits<unsigned>::max();

            /**
             * Return the index of the first set ('on') bit, or -1 if we are
             * empty.
             */
            auto find_first() const -> unsigned
            {
                for (typename Bits::size_type i = 0 ; i < _bits.size() ; ++i) {
                    int b = __builtin_ffsll(_bits[i]);
                    if (0 != b)
                        return i * bits_per_word + b - 1;
                }
                return npos;
            }

            auto find_next(unsigned start_after) const -> unsigned
            {
                unsigned start = start_after + 1;

                auto word = _bits[start / bits_per_word];
                word &= ~((BitWord(1) << (start % bits_per_word)) - 1);
                int b = __builtin_ffsll(word);
                if (0 != b)
                    return start / bits_per_word * bits_per_word + b - 1;

                for (typename Bits::size_type i = start / bits_per_word + 1; i < _bits.size() ; ++i) {
                    int b = __builtin_ffsll(_bits[i]);
                    if (0 != b)
                        return i * bits_per_word + b - 1;
                }

                return npos;
            }

            auto operator== (const FixedBitSet<words_> & other) const -> bool
            {
                if (_bits.size() != other._bits.size())
                    return false;

                for (typename Bits::size_type i = 0 ; i < _bits.size() ; ++i)
                    if (_bits[i] != other._bits[i])
                        return false;

                return true;
            }

            auto operator~ () const -> FixedBitSet
            {
                FixedBitSet result = *this;
                for (auto & p : result._bits)
                    p = ~p;
                return result;
            }
    };

    const constexpr int split_levels = 100;

    struct Position
    {
        array<unsigned, split_levels + 1> values;
        unsigned depth;

        Position()
        {
            fill(values.begin(), values.end(), 0);
            depth = 0;
        }

        bool operator< (const Position & other) const
        {
            if (depth < other.depth)
                return true;
            else if (depth > other.depth)
                return false;

            for (unsigned p = 0 ; p < split_levels + 1 ; ++p)
                if (values.at(p) < other.values.at(p))
                    return true;
                else if (values.at(p) > other.values.at(p))
                    return false;

            return false;
        }

        void add(unsigned d, unsigned v)
        {
            depth = d;
            if (d <= split_levels)
                values[d] = v;
        }
    };

    struct HelpMe
    {
        struct Task
        {
            const function<void (unsigned long long &)> * func;
            int pending;
        };

        mutex general_mutex;
        condition_variable cv;
        map<Position, Task> tasks;
        atomic<bool> finish;

        vector<thread> threads;

        list<milliseconds> times;
        list<unsigned long long> nodes;

        HelpMe(int n_threads) :
            finish(false)
        {
            for (int t = 0 ; t < n_threads ; ++t)
                threads.emplace_back([this, n_threads, t] {
                        milliseconds total_work_time = milliseconds::zero();
                        unsigned long long this_thread_nodes = 0;
                        while (! finish.load()) {
                            unique_lock<mutex> guard(general_mutex);
                            bool did_something = false;
                            for (auto task = tasks.begin() ; task != tasks.end() ; ++task) {
                                if (task->second.func) {
                                    auto f = task->second.func;
                                    ++task->second.pending;
                                    guard.unlock();

                                    auto start_work_time = steady_clock::now(); // local start time

                                    (*f)(this_thread_nodes);

                                    auto work_time = duration_cast<milliseconds>(steady_clock::now() - start_work_time);
                                    total_work_time += work_time;

                                    guard.lock();
                                    task->second.func = nullptr;
                                    if (0 == --task->second.pending)
                                        cv.notify_all();

                                    did_something = true;
                                    break;
                                }
                            }

                            if ((! did_something) && (! finish.load()))
                                cv.wait(guard);
                        }

                        unique_lock<mutex> guard(general_mutex);
                        times.push_back(total_work_time);
                        nodes.push_back(this_thread_nodes);
                        });
        }

        auto kill_workers() -> void
        {
            {
                unique_lock<mutex> guard(general_mutex);
                finish.store(true);
                cv.notify_all();
            }

            for (auto & t : threads)
                t.join();

            threads.clear();

            if (! times.empty()) {
                cerr << "-- worker thread times";
                for (auto & t : times)
                    cerr << " " << t.count();
                cerr << endl;
                times.clear();
            }
        }

        ~HelpMe()
        {
            kill_workers();
        }

        HelpMe(const HelpMe &) = delete;

        void get_help_with(const Position & position, const function<void (unsigned long long &)> & func, unsigned long long & my_nodes)
        {
            map<Position, HelpMe::Task>::iterator task;
            {
                unique_lock<mutex> guard(general_mutex);
                auto r = tasks.emplace(position, HelpMe::Task{ &func, 0 });
                assert(r.second);
                task = r.first;
                cv.notify_all();
            }

            func(my_nodes);

            {
                unique_lock<mutex> guard(general_mutex);
                while (0 != task->second.pending)
                    cv.wait(guard);
                tasks.erase(task);
            }
        }

        void get_help_with_loop(const function<void ()> & func)
        {
            Position position;
            function<void (unsigned long long &)> wrapped_func = [&] (const unsigned long long &) { func(); };
            unsigned long long ignore_nodes = 0;

            get_help_with(position, wrapped_func, ignore_nodes);
        }
    };

    template <typename Bitset_>
    struct SIP
    {
        struct Domain
        {
            unsigned v;
            bool fixed;
            Bitset_ values;
        };

        using Domains = vector<Domain>;

        struct Assignments
        {
            vector<tuple<unsigned, unsigned, bool> > trail;

            auto push_branch(unsigned a, unsigned b) -> void
            {
                trail.emplace_back(a, b, true);
            }

            auto push_implication(unsigned a, unsigned b) -> void
            {
                if (trail.end() == find_if(trail.begin(), trail.end(), [&] (const auto & x) {
                            return get<0>(x) == a && get<1>(x) == b;
                            }))
                    trail.emplace_back(a, b, false);
            }

            auto pop() -> void
            {
                while ((! trail.empty()) && (! get<2>(trail.back())))
                    trail.pop_back();

                if (! trail.empty())
                    trail.pop_back();
            }

            auto store_to(map<int, int> & m, unsigned wildcard_start) -> void
            {
                m.clear();
                for (auto & t : trail) {
                    if (get<1>(t) >= wildcard_start)
                        m.emplace(get<0>(t), -1);
                    else
                        m.emplace(get<0>(t), get<1>(t));
                }
            }
        };

        const Params & params;
        unsigned domain_size;

        Result result;
        mutex result_mutex;

        list<pair<vector<Bitset_>, vector<Bitset_> > > adjacency_constraints;
        vector<unsigned> pattern_degrees, target_degrees;

        Domains initial_domains;

        unsigned wildcard_start;
        Bitset_ all_wildcards;

        atomic<bool> solution_found;

        mutable HelpMe help_me;

        SIP(const Params & k, const Graph & pattern, const Graph & target) :
            params(k),
            domain_size(target.size() + params.except),
            pattern_degrees(pattern.size()),
            target_degrees(domain_size),
            initial_domains(pattern.size()),
            wildcard_start(target.size()),
            all_wildcards(domain_size),
            help_me(params.threads - 1)
        {
            for (unsigned v = wildcard_start ; v != domain_size ; ++v)
                all_wildcards.set(v);

            // build up adjacency bitsets
            add_adjacency_constraints(pattern, target);

            {
                atomic<unsigned> shared_p{ 0 };
                auto loop_func = [&] () {
                    for (unsigned p = shared_p++ ; p < pattern.size() ; p = shared_p++) {
                        pattern_degrees[p] = pattern.degree(p);
                    }
                };
                help_me.get_help_with_loop(loop_func);
            }

            {
                atomic<unsigned> shared_t{ 0 };
                auto loop_func = [&] () {
                    for (unsigned t = shared_t++ ; t < target.size() ; t = shared_t++) {
                        target_degrees[t] = target.degree(t);
                    }
                };
                help_me.get_help_with_loop(loop_func);
            }

            if (params.except >= 1)
                for (unsigned v = 0 ; v < params.except ; ++v)
                    target_degrees.at(wildcard_start + v) = params.high_wildcards ? target.size() + 1 : 0;

            vector<vector<vector<unsigned> > > p_nds(params.cnds ? adjacency_constraints.size() * adjacency_constraints.size() : adjacency_constraints.size());
            vector<vector<vector<unsigned> > > t_nds(params.cnds ? adjacency_constraints.size() * adjacency_constraints.size() : adjacency_constraints.size());

            if (params.cnds) {
                for (unsigned p = 0 ; p < pattern.size() ; ++p) {
                    unsigned cn = 0;
                    for (auto & c : adjacency_constraints) {
                        for (auto & d : adjacency_constraints) {
                            p_nds[cn].resize(pattern.size());
                            for (unsigned q = 0 ; q < pattern.size() ; ++q)
                                if (c.first[p][q])
                                    p_nds[cn][p].push_back(d.first[q].count());
                            sort(p_nds[cn][p].begin(), p_nds[cn][p].end(), greater<unsigned>());
                            ++cn;
                        }
                    }
                }

                for (unsigned t = 0 ; t < target.size() ; ++t) {
                    unsigned cn = 0;
                    for (auto & c : adjacency_constraints) {
                        for (auto & d : adjacency_constraints) {
                            t_nds[cn].resize(target.size());
                            for (unsigned q = 0 ; q < target.size() ; ++q)
                                if (c.second[t][q])
                                    t_nds[cn][t].push_back(d.second[q].count());
                            sort(t_nds[cn][t].begin(), t_nds[cn][t].end(), greater<unsigned>());
                            ++cn;
                        }
                    }
                }
            }
            else if (params.nds) {
                for (unsigned p = 0 ; p < pattern.size() ; ++p)
                    for (unsigned cn = 0 ; cn < adjacency_constraints.size() ; ++cn)
                        p_nds[cn].resize(pattern.size());

                atomic<unsigned> shared_p{ 0 };
                auto loop_func = [&] () {
                    for (unsigned p = shared_p++ ; p < pattern.size() ; p = shared_p++) {
                        unsigned cn = 0;
                        for (auto & c : adjacency_constraints) {
                            for (unsigned q = 0 ; q < pattern.size() ; ++q)
                                if (c.first[p][q])
                                    p_nds[cn][p].push_back(c.first[q].count());
                            sort(p_nds[cn][p].begin(), p_nds[cn][p].end(), greater<unsigned>());
                            ++cn;
                        }
                    }
                };

                help_me.get_help_with_loop(loop_func);
            }

            // build up initial domains
            Bitset_ initial_domains_union = Bitset_(domain_size);
            for (unsigned q = 0 ; q < domain_size ; ++q)
                initial_domains_union.set(q);

            while (true) {
                if (params.nds) {
                    t_nds = vector<vector<vector<unsigned> > >(adjacency_constraints.size());

                    for (unsigned t = 0 ; t < target.size() ; ++t)
                        for (unsigned cn = 0 ; cn < adjacency_constraints.size() ; ++cn)
                            t_nds[cn].resize(target.size());

                    atomic<unsigned> shared_t{ 0 };
                    auto loop_func = [&] () {
                        for (unsigned t = shared_t++ ; t < target.size() ; t = shared_t++) {
                            unsigned cn = 0;
                            for (auto & c : adjacency_constraints) {
                                for (unsigned q = 0 ; q < target.size() ; ++q)
                                    if (c.second[t][q])
                                        t_nds[cn][t].push_back((c.second[q] & initial_domains_union).count());
                                sort(t_nds[cn][t].begin(), t_nds[cn][t].end(), greater<unsigned>());
                                ++cn;
                            }
                        }
                    };

                    help_me.get_help_with_loop(loop_func);
                }

                atomic<unsigned> shared_p{ 0 };
                auto loop_func = [&] () {
                    for (unsigned p = shared_p++ ; p < pattern.size() ; p = shared_p++) {
                        initial_domains[p].v = p;
                        initial_domains[p].values = Bitset_(domain_size);
                        initial_domains[p].fixed = false;

                        // decide initial domain values
                        for (unsigned t = 0 ; t < target.size() ; ++t) {
                            if (! initial_domains_union[t])
                                continue;

                            bool ok = true;

                            for (auto & c : adjacency_constraints) {
                                // check loops
                                if (c.first[p][p] && ! c.second[t][t])
                                    ok = false;

                                auto c_second_t = c.second[t] & initial_domains_union;

                                // check degree
                                if (ok && params.degree && 0 == params.except && ! (c.first[p].count() <= c_second_t.count()))
                                    ok = false;

                                // check except-degree
                                if (ok && params.degree && params.except >= 1 && ! (c.first[p].count() <= c_second_t.count() + params.except))
                                    ok = false;

                                if (! ok)
                                    break;
                            }

                            // neighbourhood degree sequences
                            if (params.nds) {
                                for (unsigned cn = 0 ; cn < 1 && ok ; ++cn) {
                                    for (unsigned i = params.except ; i < p_nds[cn][p].size() ; ++i) {
                                        if (t_nds[cn][t][i - params.except] + params.except < p_nds[cn][p][i]) {
                                            ok = false;
                                            break;
                                        }
                                    }
                                }
                            }

                            if (ok)
                                initial_domains[p].values.set(t);
                        }

                        // wildcard in domain?
                        if (params.except >= 1)
                            for (unsigned v = wildcard_start ; v != domain_size ; ++v)
                                initial_domains[p].values.set(v);
                    }
                };

                help_me.get_help_with_loop(loop_func);

                if (! params.ilf)
                    break;

                auto old_card = initial_domains_union.count();
                initial_domains_union = Bitset_(domain_size);
                for (unsigned p = 0 ; p < pattern.size() ; ++p)
                    initial_domains_union |= initial_domains[p].values;
                if (initial_domains_union.count() == old_card)
                    break;
            }

            if (params.expensive_stats) {
                Bitset_ initial_domains_union = Bitset_(domain_size);
                for (unsigned p = 0 ; p < pattern.size() ; ++p)
                    initial_domains_union |= initial_domains[p].values;

                result.stats.emplace("UA", to_string(initial_domains_union.count()));
                result.stats.emplace("UP", to_string(domain_size));

                vector<pair<Domain *, unsigned> > assignments;

                for (auto & d : initial_domains)
                    for (auto v = d.values.find_first() ; v != Bitset_::npos && v < wildcard_start ; v = d.values.find_next(v))
                        assignments.emplace_back(&d, v);

                unsigned long long pairs_seen = 0, pairs_disallowed = 0;
                if (assignments.size() >= 2) {
                    uniform_int_distribution<typename decltype(assignments)::size_type>
                        all_dist(0, assignments.size() - 1), all_but_first_dist(1, assignments.size() - 1);
                    mt19937 rand(666);
                    for (unsigned n = 0 ; n < 1000000 ; ++n) {
                        swap(assignments[0], assignments[all_dist(rand)]);
                        swap(assignments[1], assignments[all_but_first_dist(rand)]);
                        auto & a = assignments[0], & b = assignments[1];
                        if (a.first->v != b.first->v && a.second != b.second) {
                            ++pairs_seen;
                            bool disallowed = false;
                            for (auto & c : adjacency_constraints)
                                if (c.first[a.first->v][b.first->v] && ! c.second[a.second][b.second])
                                    disallowed = true;

                            if (disallowed)
                                ++pairs_disallowed;
                        }
                    }
                }

                result.stats.emplace("PS", to_string(pairs_seen));
                result.stats.emplace("PD", to_string(pairs_disallowed));
            }
        }

        auto add_complement_constraints(const Graph & pattern, const Graph & target) -> auto
        {
            auto & d1 = *adjacency_constraints.insert(
                    adjacency_constraints.end(), make_pair(vector<Bitset_>(), vector<Bitset_>()));
            build_d1_adjacency(pattern, false, d1.first, true);
            build_d1_adjacency(target, true, d1.second, true);

            return d1;
        }

        auto add_adjacency_constraints(const Graph & pattern, const Graph & target) -> void
        {
            auto & d1 = *adjacency_constraints.insert(
                    adjacency_constraints.end(), make_pair(vector<Bitset_>(), vector<Bitset_>()));
            build_d1_adjacency(pattern, false, d1.first, false);
            build_d1_adjacency(target, true, d1.second, false);

            if (params.d2graphs) {
                auto & d21 = *adjacency_constraints.insert(
                        adjacency_constraints.end(), make_pair(vector<Bitset_>(), vector<Bitset_>()));
                auto & d22 = *adjacency_constraints.insert(
                        adjacency_constraints.end(), make_pair(vector<Bitset_>(), vector<Bitset_>()));
                auto & d23 = *adjacency_constraints.insert(
                        adjacency_constraints.end(), make_pair(vector<Bitset_>(), vector<Bitset_>()));

                build_d2_adjacency(pattern.size(), d1.first, false, d21.first, d22.first, d23.first);
                build_d2_adjacency(target.size(), d1.second, true, d21.second, d22.second, d23.second);
            }

            if (params.induced) {
                auto d1c = add_complement_constraints(pattern, target);

                if (params.d2cgraphs) {
                    auto & d21c = *adjacency_constraints.insert(
                            adjacency_constraints.end(), make_pair(vector<Bitset_>(), vector<Bitset_>()));
                    auto & d22c = *adjacency_constraints.insert(
                            adjacency_constraints.end(), make_pair(vector<Bitset_>(), vector<Bitset_>()));
                    auto & d23c = *adjacency_constraints.insert(
                            adjacency_constraints.end(), make_pair(vector<Bitset_>(), vector<Bitset_>()));

                    build_d2_adjacency(pattern.size(), d1c.first, false, d21c.first, d22c.first, d23c.first);
                    build_d2_adjacency(target.size(), d1c.second, true, d21c.second, d22c.second, d23c.second);
                }
            }
        }

        auto build_d1_adjacency(const Graph & graph, bool is_target, vector<Bitset_> & adj, bool complement) const -> void
        {
            adj.resize(graph.size());

            atomic<unsigned> shared_t{ 0 };
            auto loop_func = [&] () {
                for (unsigned t = shared_t++ ; t < graph.size() ; t = shared_t++) {
                    adj[t] = Bitset_(is_target ? domain_size : graph.size());
                    for (unsigned u = 0 ; u < graph.size() ; ++u)
                        if (graph.adjacent(t, u) != complement)
                            adj[t].set(u);
                }
            };
            help_me.get_help_with_loop(loop_func);
        }

        auto build_d2_adjacency(
                const unsigned graph_size,
                const vector<Bitset_> & d1_adj,
                bool is_target,
                vector<Bitset_> & adj1,
                vector<Bitset_> & adj2,
                vector<Bitset_> & adj3) const -> void
        {
            adj1.resize(graph_size);
            adj2.resize(graph_size);
            adj3.resize(graph_size);

            vector<vector<unsigned> > counts(graph_size, vector<unsigned>(graph_size, 0));

            {
                atomic<unsigned> shared_t{ 0 };
                auto loop_func = [&] () {
                    for (unsigned t = shared_t++ ; t < graph_size ; t = shared_t++) {
                        adj1[t] = Bitset_(is_target ? domain_size : graph_size);
                        adj2[t] = Bitset_(is_target ? domain_size : graph_size);
                        adj3[t] = Bitset_(is_target ? domain_size : graph_size);
                        for (auto u = d1_adj[t].find_first() ; u != Bitset_::npos ; u = d1_adj[t].find_next(u))
                            if (t != u)
                                for (auto v = d1_adj[u].find_first() ; v != Bitset_::npos ; v = d1_adj[u].find_next(v))
                                    if (u != v && t != v)
                                        ++counts[t][v];
                    }
                };
                help_me.get_help_with_loop(loop_func);
            }

            {
                atomic<unsigned> shared_t{ 0 };
                auto loop_func = [&] () {
                    for (unsigned t = shared_t++ ; t < graph_size ; t = shared_t++) {
                        for (unsigned u = 0 ; u < graph_size ; ++u) {
                            if (counts[t][u] >= 3 + (is_target ? 0 : params.except))
                                adj3[t].set(u);
                            if (counts[t][u] >= 2 + (is_target ? 0 : params.except))
                                adj2[t].set(u);
                            if (counts[t][u] >= 1 + (is_target ? 0 : params.except))
                                adj1[t].set(u);
                        }
                    }
                };
                help_me.get_help_with_loop(loop_func);
            }
        }

        auto select_branch_domain(Domains & domains) -> typename Domains::iterator
        {
            auto best = domains.end();

            for (auto d = domains.begin() ; d != domains.end() ; ++d) {
                if (d->fixed)
                    continue;

                if (best == domains.end())
                    best = d;
                else {
                    int best_c = best->values.count();
                    int d_c = d->values.count();

                    if (d_c < best_c)
                        best = d;
                    else if (d_c == best_c) {
                        if (pattern_degrees[d->v] > pattern_degrees[best->v])
                            best = d;
                        else if (pattern_degrees[d->v] == pattern_degrees[best->v] && d->v < best->v)
                            best = d;
                    }
                }
            }

            return best;
        }

        auto select_unit_domain(Domains & domains) -> typename Domains::iterator
        {
            return find_if(domains.begin(), domains.end(), [&] (const auto & a) {
                    if (! a.fixed) {
                        auto c = a.values.count();
                        return 1 == c || (c > 1 && a.values.find_first() >= wildcard_start);
                    }
                    else
                        return false;
                    });
        }

        auto unit_propagate(Domains & domains, Assignments & assignments) -> bool
        {
            while (! domains.empty()) {
                auto unit_domain_iter = select_unit_domain(domains);

                if (unit_domain_iter == domains.end()) {
                    if (! cheap_all_different(domains))
                        return false;
                    unit_domain_iter = select_unit_domain(domains);
                    if (unit_domain_iter == domains.end())
                        break;
                }

                auto unit_domain_v = unit_domain_iter->v;
                auto unit_domain_value = unit_domain_iter->values.find_first();
                unit_domain_iter->fixed = true;

                assignments.push_implication(unit_domain_v, unit_domain_value);

                for (auto & d : domains) {
                    if (d.fixed)
                        continue;

                    // injectivity
                    d.values.reset(unit_domain_value);

                    // adjacency
                    if (unit_domain_value < wildcard_start)
                        for (auto & c : adjacency_constraints)
                            if (c.first[unit_domain_v][d.v])
                                d.values &= (c.second[unit_domain_value] | all_wildcards);

                    if (d.values.none())
                        return false;
                }
            }

            return true;
        }

        auto cheap_all_different(Domains & domains) -> bool
        {
            // pick domains smallest first, with tiebreaking
            vector<pair<int, int> > domains_order;
            domains_order.resize(domains.size());
            for (unsigned d = 0 ; d < domains.size() ; ++d) {
                domains_order[d].first = d;
                domains_order[d].second = domains[d].values.count();
            }

            sort(domains_order.begin(), domains_order.begin() + domains.size(),
                    [&] (const pair<int, int> & a, const pair<int, int> & b) {
                        return a.second < b.second || (a.second == b.second && a.first < b.first);
                    });

            // counting all-different
            Bitset_ domains_so_far = Bitset_(domain_size), hall = Bitset_(domain_size);
            unsigned neighbours_so_far = 0;

            for (int i = 0, i_end = domains.size() ; i != i_end ; ++i) {
                auto & d = domains.at(domains_order.at(i).first);

                d.values &= ~hall;

                if (d.values.none())
                    return false;

                domains_so_far |= d.values;
                ++neighbours_so_far;

                unsigned domains_so_far_popcount = domains_so_far.count();
                if (domains_so_far_popcount < neighbours_so_far)
                    return false;
                else if (domains_so_far_popcount == neighbours_so_far) {
                    neighbours_so_far = 0;
                    hall |= domains_so_far;
                    domains_so_far.reset();
                }
            }

            return true;
        }

        auto solve_nopar(
                Domains & domains,
                Assignments & assignments,
                unsigned long long & active_nodes) -> void
        {
            if (*params.abort || solution_found)
                return;

            ++active_nodes;

            auto branch_domain = select_branch_domain(domains);

            if (domains.end() == branch_domain) {
                unique_lock<mutex> guard(result_mutex);
                assignments.store_to(result.isomorphism, wildcard_start);
                solution_found = true;
                return;
            }

            vector<unsigned> branch_values;
            for (auto branch_value = branch_domain->values.find_first() ;
                    branch_value != Bitset_::npos ;
                    branch_value = branch_domain->values.find_next(branch_value))
                branch_values.push_back(branch_value);

            sort(branch_values.begin(), branch_values.end(), [&] (const auto & a, const auto & b) {
                    return target_degrees.at(a) < target_degrees.at(b) || (target_degrees.at(a) == target_degrees.at(b) && a < b);
                    });

            bool already_did_a_wildcard = false;

            for (auto & branch_value : branch_values) {
                if (*params.abort || solution_found)
                    return;

                if (already_did_a_wildcard && branch_value >= wildcard_start)
                    continue;

                if (branch_value >= wildcard_start)
                    already_did_a_wildcard = true;

                assignments.push_branch(branch_domain->v, branch_value);

                Domains new_domains;
                new_domains.reserve(domains.size());
                for (auto & d : domains) {
                    if (d.fixed)
                        continue;

                    if (d.v == branch_domain->v) {
                        Bitset_ just_branch_value = d.values;
                        just_branch_value.reset();
                        just_branch_value.set(branch_value);
                        new_domains.emplace_back(Domain{ unsigned(d.v), false, just_branch_value });
                    }
                    else
                        new_domains.emplace_back(Domain{ unsigned(d.v), false, d.values });
                }

                if (unit_propagate(new_domains, assignments))
                    solve_nopar(new_domains, assignments, active_nodes);

                // restore assignments
                assignments.pop();
            }

            return;
        }

        auto solve(
                Domains & domains,
                Assignments & assignments,
                const Position & position,
                unsigned depth,
                unsigned long long & my_nodes) -> void
        {
            if (*params.abort || solution_found)
                return;

            ++my_nodes;

            auto branch_domain = select_branch_domain(domains);

            if (domains.end() == branch_domain) {
                unique_lock<mutex> guard(result_mutex);
                assignments.store_to(result.isomorphism, wildcard_start);
                solution_found = true;
                return;
            }

            vector<unsigned> branch_values;
            for (auto branch_value = branch_domain->values.find_first() ;
                    branch_value != Bitset_::npos ;
                    branch_value = branch_domain->values.find_next(branch_value))
                branch_values.push_back(branch_value);

            sort(branch_values.begin(), branch_values.end(), [&] (const auto & a, const auto & b) {
                    return target_degrees.at(a) < target_degrees.at(b) || (target_degrees.at(a) == target_degrees.at(b) && a < b);
                    });

            bool already_did_a_wildcard = false;

            vector<function<void (unsigned long long &)> > funcs;
            funcs.reserve(branch_values.size());

            int i = 0;
            for (auto & branch_value : branch_values) {
                if (*params.abort || solution_found)
                    return;

                if (already_did_a_wildcard && branch_value >= wildcard_start)
                    continue;

                if (branch_value >= wildcard_start)
                    already_did_a_wildcard = true;

                ++i;
                funcs.emplace_back([&domains, &position, branch_domain, branch_value, assignments, depth, i, this] (unsigned long long & active_nodes) mutable {
                    assignments.push_branch(branch_domain->v, branch_value);

                    Domains new_domains;
                    new_domains.reserve(domains.size());
                    for (auto & d : domains) {
                        if (d.fixed)
                            continue;

                        if (d.v == branch_domain->v) {
                            Bitset_ just_branch_value = d.values;
                            just_branch_value.reset();
                            just_branch_value.set(branch_value);
                            new_domains.emplace_back(Domain{ unsigned(d.v), false, just_branch_value });
                        }
                        else
                            new_domains.emplace_back(Domain{ unsigned(d.v), false, d.values });
                    }

                    if (unit_propagate(new_domains, assignments)) {
                        auto new_position = position;
                        new_position.add(depth + 1, i + 1);
                        if (depth > split_levels)
                            solve_nopar(new_domains, assignments, active_nodes);
                        else
                            solve(new_domains, assignments, new_position, depth + 1, active_nodes);
                    }
                });
            }

            if (! funcs.empty()) {
                atomic<int> shared_b{ 0 };
                auto this_thread_function = [&] (unsigned long long & active_nodes) {
                    for (int b = shared_b++, b_end = funcs.size() ; b < b_end /* not != */ ; b = shared_b++) {
                        if (*params.abort || solution_found)
                            return;

                        (*next(funcs.begin(), b))(active_nodes);
                    }
                };

                if (depth <= split_levels)
                    help_me.get_help_with(position, this_thread_function, my_nodes);
                else
                    this_thread_function(my_nodes);
            }
        }

        auto record_domain_sizes_in_stats(const Domains & domains)
        {
            auto wildcards = Bitset_(domain_size);
            for (unsigned v = wildcard_start ; v != domain_size ; ++v)
                wildcards.set(v);

            for (auto & d : domains)
                result.stats.emplace("IDS" + to_string(d.v), to_string((d.values & ~wildcards).count()));
        }

        auto run()
        {
            Assignments assignments;
            solution_found = false;

            // eliminate isolated vertices?

            if (unit_propagate(initial_domains, assignments)) {
                if (params.expensive_stats)
                    record_domain_sizes_in_stats(initial_domains);
                Position position;
                unsigned long long main_thread_nodes = 0;
                solve(initial_domains, assignments, position, 0, main_thread_nodes);
                help_me.kill_workers();
                result.nodes += main_thread_nodes;
                for (auto & n : help_me.nodes)
                    result.nodes += n;
            }
            else if (params.expensive_stats)
                record_domain_sizes_in_stats(initial_domains);
        }
    };
}

auto parallel_subgraph_isomorphism(const pair<Graph, Graph> & graphs, const Params & params) -> Result
{
    if (graphs.second.size() + params.except <= 63) {
        SIP<FixedBitSet<64 / sizeof(unsigned long long)> > sip(params, graphs.first, graphs.second);
        sip.run();
        return sip.result;
    }
    else if (graphs.second.size() + params.except <= 127) {
        SIP<FixedBitSet<128 / sizeof(unsigned long long)> > sip(params, graphs.first, graphs.second);
        sip.run();
        return sip.result;
    }
    else if (graphs.second.size() + params.except <= 255) {
        SIP<FixedBitSet<256 / sizeof(unsigned long long)> > sip(params, graphs.first, graphs.second);
        sip.run();
        return sip.result;
    }
    else if (graphs.second.size() + params.except <= 447) {
        SIP<FixedBitSet<448 / sizeof(unsigned long long)> > sip(params, graphs.first, graphs.second);
        sip.run();
        return sip.result;
    }
    else if (graphs.second.size() + params.except <= 511) {
        SIP<FixedBitSet<512 / sizeof(unsigned long long)> > sip(params, graphs.first, graphs.second);
        sip.run();
        return sip.result;
    }
    else {
        SIP<boost::dynamic_bitset<> > sip(params, graphs.first, graphs.second);
        sip.run();
        return sip.result;
    }
}

auto parallel_ix_subgraph_isomorphism(const pair<Graph, Graph> & graphs, const Params & params) -> Result
{
    auto modified_params = params;
    Result modified_result;

    while (! *modified_params.abort) {
        auto start_time = steady_clock::now();

        SIP<boost::dynamic_bitset<> > sip(modified_params, graphs.first, graphs.second);

        sip.run();

        auto pass_time = duration_cast<milliseconds>(steady_clock::now() - start_time);
        modified_result.times.push_back(pass_time);

        modified_result.nodes += sip.result.nodes;
        if (! sip.result.isomorphism.empty()) {
            modified_result.isomorphism = sip.result.isomorphism;
            modified_result.stats.emplace("EXCEPT", to_string(modified_params.except));
            modified_result.stats.emplace("SIZE", to_string(graphs.first.size() - modified_params.except));
            return modified_result;
        }
        else {
            cerr << "-- " << pass_time.count() << " <" << graphs.first.size() - modified_params.except << endl;
            modified_result.stats.emplace("FAIL" + to_string(modified_params.except), to_string(pass_time.count()));
        }

        if (++modified_params.except >= graphs.first.size())
            break;
    }

    return modified_result;
}

