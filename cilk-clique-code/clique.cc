/* vim: set sw=4 sts=4 et foldmethod=syntax : */

#include "clique.hh"
#include "bit_graph.hh"
#include "template_voodoo.hh"

#include <algorithm>
#include <numeric>
#include <limits>
#include <iostream>
#include <atomic>
#include <mutex>
#include <chrono>
#include <map>

#include <cilk/cilk.h>
#include <cilk/reducer_opadd.h>

using std::chrono::steady_clock;
using std::chrono::duration_cast;
using std::chrono::milliseconds;

namespace
{
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

    struct Incumbent
    {
        std::atomic<unsigned> value{ 0 };

        std::mutex mutex;
        std::vector<unsigned> c;

        std::chrono::time_point<std::chrono::steady_clock> start_time;

        void update(const std::vector<unsigned> & new_c)
        {
            while (true) {
                unsigned current_value = value;
                if (new_c.size() > current_value) {
                    unsigned new_c_size = new_c.size();
                    if (value.compare_exchange_strong(current_value, new_c_size)) {
                        std::unique_lock<std::mutex> lock(mutex);
                        c = new_c;
                        std::cerr << "-- " << (duration_cast<milliseconds>(steady_clock::now() - start_time)).count()
                            << " " << new_c.size() << std::endl;
                        break;
                    }
                }
                else
                    break;
            }
        }
    };

    template <unsigned n_words_>
    struct Clique
    {
        const Association & association;
        const Params & params;

        FixedBitGraph<n_words_> graph;
        std::vector<int> order, invorder;
        Incumbent incumbent;

        cilk::reducer_opadd<unsigned long long> nodes;

        Clique(const std::pair<Association, AssociatedEdges> & g, const Params & q, const VFGraph &) :
            association(g.first),
            params(q),
            order(g.first.size()),
            invorder(g.first.size()),
            nodes(0)
        {
            incumbent.start_time = params.start_time;

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

            for (unsigned i = 0 ; i < order.size() ; ++i)
                invorder[order[i]] = i;

            for (auto & f : g.second)
                graph.add_edge(invorder[f.first], invorder[f.second]);
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

        auto colour_class_order_defer1(
                const FixedBitSet<n_words_> & p,
                std::array<unsigned, n_words_ * bits_per_word> & p_order,
                std::array<unsigned, n_words_ * bits_per_word> & p_bounds) -> void
        {
            FixedBitSet<n_words_> p_left = p; // not coloured yet
            unsigned colour = 0;        // current colour
            unsigned i = 0;             // position in p_bounds

            unsigned d = 0;             // number deferred
            std::array<unsigned, n_words_ * bits_per_word> defer;

            // while we've things left to colour
            while (! p_left.empty()) {
                // next colour
                ++colour;
                // things that can still be given this colour
                FixedBitSet<n_words_> q = p_left;

                // while we can still give something this colour
                unsigned number_with_this_colour = 0;
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
                    ++number_with_this_colour;
                }

                if (1 == number_with_this_colour) {
                    --i;
                    --colour;
                    defer[d++] = p_order[i];
                }
            }

            for (unsigned n = 0 ; n < d ; ++n) {
                ++colour;
                p_order[i] = defer[n];
                p_bounds[i] = colour;
                i++;
            }
        }

        auto colour_class_order_sort(
                const FixedBitSet<n_words_> & p,
                std::array<unsigned, n_words_ * bits_per_word> & p_order,
                std::array<unsigned, n_words_ * bits_per_word> & p_bounds) -> void
        {
            FixedBitSet<n_words_> p_left = p; // not coloured yet
            std::vector<std::vector<unsigned> > colour_classes;

            // while we've things left to colour
            while (! p_left.empty()) {
                // next colour
                colour_classes.push_back({});

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
                    colour_classes.back().push_back(v);
                }
            }

            std::stable_sort(colour_classes.begin(), colour_classes.end(), [] (const auto & a, const auto & b) {
                    return a.size() > b.size();
                    });

            unsigned colour = 0;         // current colour
            unsigned i = 0;              // position in p_bounds

            for (auto & c : colour_classes) {
                ++colour;
                for (auto & v : c) {
                    p_order[i] = v;
                    p_bounds[i] = colour;
                    ++i;
                }
            }
        }

        auto expand(
                const std::vector<unsigned> & c,
                const FixedBitSet<n_words_> & p
                ) -> void
        {
            ++nodes;

            struct ArrayWrapper
            {
                std::array<unsigned, n_words_ * bits_per_word> * _array;

                ArrayWrapper() :
                    _array(new std::array<unsigned, n_words_ * bits_per_word>{{}})
                {
                }

                ~ArrayWrapper()
                {
                    delete _array;
                }

                operator std::array<unsigned, n_words_ * bits_per_word> & ()
                {
                    return *_array;
                }

                unsigned operator[] (unsigned p)
                {
                    return (*_array)[p];
                }
            };

            using ColourArrayType = typename std::conditional<n_words_ >= 16, ArrayWrapper, std::array<unsigned, n_words_ * bits_per_word> >::type;

            // initial colouring
            ColourArrayType p_order;
            ColourArrayType p_bounds;

            switch (params.how_much_sorting) {
                case Params::no_sorting:
                    colour_class_order(p, p_order, p_bounds);
                    break;

                case Params::defer1:
                    colour_class_order_defer1(p, p_order, p_bounds);
                    break;

                case Params::full_sort:
                    colour_class_order_sort(p, p_order, p_bounds);
                    break;
            }

            if (params.parallel_for) {
                cilk_for (int n = p.popcount() - 1 ; n >= 0 ; --n) {
                    // bound, timeout or early exit?
                    if (! (c.size() + p_bounds[n] <= incumbent.value || (params.decide > 0 && incumbent.value >= params.decide) || params.abort->load())) {
                        auto v = p_order[n];

                        auto new_c = c;

                        // consider taking v
                        new_c.push_back(v);

                        // filter p to contain vertices adjacent to v
                        FixedBitSet<n_words_> new_p = p;
                        graph.intersect_with_row(v, new_p);

                        for (int x = p.popcount() - 1 ; x > n ; --x)
                            new_p.unset(p_order[x]);

                        if (! new_p.empty())
                            expand(new_c, new_p);
                        else
                            incumbent.update(new_c);
                    }
                }
            }
            else {
                auto shrinking_p = p;

                // for each v in p... (v comes later)
                for (int n = p.popcount() - 1 ; n >= 0 ; --n) {
                    // bound, timeout or early exit?
                    if (c.size() + p_bounds[n] <= incumbent.value || (params.decide > 0 && incumbent.value >= params.decide) || params.abort->load())
                        return;

                    auto v = p_order[n];

                    if (params.deep && c.size() < 2) {
                        auto new_c = c;

                        // consider taking v
                        new_c.push_back(v);

                        // filter p to contain vertices adjacent to v
                        FixedBitSet<n_words_> new_p = shrinking_p;
                        graph.intersect_with_row(v, new_p);

                        if (! new_p.empty())
                            expand(new_c, new_p);
                        else
                            incumbent.update(new_c);
                    }
                    else {
                        cilk_spawn [this, &c, shrinking_p, v] {
                            auto new_c = c;

                            // consider taking v
                            new_c.push_back(v);

                            // filter p to contain vertices adjacent to v
                            FixedBitSet<n_words_> new_p = shrinking_p;
                            graph.intersect_with_row(v, new_p);

                            if (! new_p.empty())
                                expand(new_c, new_p);
                            else
                                incumbent.update(new_c);
                        }();
                    }

                    // now consider not taking v
                    shrinking_p.unset(v);
                }
            }
        }

        auto run() -> Result
        {
            std::vector<unsigned> c;
            c.reserve(graph.size());

            FixedBitSet<n_words_> p;
            p.set_up_to(graph.size());

            incumbent.value = params.prime;

            // go!
            expand(c, p);

            Result result;
            result.nodes = nodes.get_value();
            for (auto & v : incumbent.c)
                result.isomorphism.insert(unproduct(association, order[v]));

            return result;
        }
    };

    template <template <unsigned> class SGI_>
    struct Apply
    {
        template <unsigned size_, typename> using Type = SGI_<size_>;
    };
}

auto clique_subgraph_isomorphism(const std::pair<VFGraph, VFGraph> & graphs, const Params & params) -> Result
{
    auto product = modular_product(graphs.first, graphs.second);

    return select_graph_size<Apply<Clique>::template Type, Result>(AllGraphSizes(), product, params, graphs.first);
}

