/* vim: set sw=4 sts=4 et foldmethod=syntax : */

#ifndef CODE_GUARD_PARALLEL_HH
#define CODE_GUARD_PARALLEL_HH 1

#include "graph.hh"
#include "params.hh"
#include "result.hh"

auto cilk_subgraph_isomorphism(const std::pair<Graph, Graph> & graphs, const Params & params) -> Result;

auto cilk_ix_subgraph_isomorphism(const std::pair<Graph, Graph> & graphs, const Params & params) -> Result;

#endif
