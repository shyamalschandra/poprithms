// Copyright (c) 2020 Graphcore Ltd. All rights reserved.
#include <algorithm>
#include <array>
#include <iostream>
#include <numeric>
#include <random>
#include <string>
#include <tuple>
#include <vector>

#include <poprithms/schedule/anneal/error.hpp>
#include <poprithms/schedule/anneal/graph.hpp>
#include <poprithms/schedule/anneal/opalloc.hpp>
#include <testutil/schedule/anneal/annealcommandlineoptions.hpp>
#include <testutil/schedule/anneal/randomgraph.hpp>

// N Ops,
// [1....E] producers for each Op randomly from D most previous
// each Op creates 1 new alloc, used allocs of all producers
// allocs have size in [10, 20)
//

int main(int argc, char **argv) {

  // N 40 E 5 D 20 graphSeed 1012 seed 114 : final sum is 5260
  // N 40 E 5 D 20 graphSeed 1012 seed 115 : final sum is 5242
  //
  // interestingly, for many different seeds, the final sum is always either
  // 5260 or 5242.

  using namespace poprithms::schedule::anneal;

  auto opts = AnnealCommandLineOptions().getCommandLineOptionsMap(
      argc,
      argv,
      {"N", "E", "D", "graphSeed"},
      {"Number of Ops",
       "Number of producers per Op",
       "range depth in past from which to select producers, randomly",
       "random source for selecting producers"});

  auto N         = std::stoi(opts.at("N"));
  auto E         = std::stoi(opts.at("E"));
  auto D         = std::stoi(opts.at("D"));
  auto graphSeed = std::stoi(opts.at("graphSeed"));

  auto g = getRandomGraph(N, E, D, graphSeed);
  g.initialize(KahnTieBreaker::RANDOM, 1015);
  g.minSumLivenessAnneal(
      AnnealCommandLineOptions().getAlgoCommandLineOptionsMap(opts));

  // nothing specific to test, we'll verify the sum liveness;
  std::vector<std::vector<ScheduleIndex>> allocToSched(g.nAllocs());
  for (ScheduleIndex i = 0; i < g.nOps_i32(); ++i) {
    OpAddress a = g.scheduleToOp(i);
    for (AllocAddress a : g.getOp(a).getAllocs()) {
      allocToSched[a].push_back(i);
    }
  }
  AllocWeight s{0};
  for (const auto &alloc : g.getAllocs()) {
    auto allocAddress = alloc.getAddress();
    if (!allocToSched[allocAddress].empty()) {
      auto cnt = (allocToSched[allocAddress].back() -
                  allocToSched[allocAddress][0] + 1);
      s += alloc.getWeight() * cnt;
    }
  }

  std::cout << g.getLivenessString() << std::endl;

  if (s != g.getSumLiveness()) {
    std::cout << s << " != " << g.getSumLiveness() << std::endl;
    throw poprithms::schedule::anneal::error(
        "Computed sum of final liveness incorrect in random example test");
  }

  if (g.getSerializationString() !=
          Graph::fromSerializationString(g.getSerializationString())
              .getSerializationString() ||
      g != Graph::fromSerializationString(g.getSerializationString())) {
    std::ostringstream oss;
    oss << "g.serialization != g.serialization(fromSerial(g.serialization)). "
        << "This suggests a problem with Graph serialization. ";
    oss << "The serialization of G is " << g.getSerializationString();
    throw error(oss.str());
  }
  return 0;
}
