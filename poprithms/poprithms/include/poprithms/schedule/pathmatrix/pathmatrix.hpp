#ifndef POPRITHMS_SCHEDULE_EDGEMAP_EDGEMAP_HPP
#define POPRITHMS_SCHEDULE_EDGEMAP_EDGEMAP_HPP

#include <array>
#include <bitset>
#include <vector>

namespace poprithms {
namespace schedule {
namespace pathmatrix {

// The number of bits stored per std::bitset,
// see bitset_performance_0.cpp for an run to choose this value.
// std::bitset is a good data-structure here, as it has a very fast
// count() method, probably using x86's popcnt
static constexpr uint64_t BitSetSize = 512;

using BitSet  = std::bitset<BitSetSize>;
using OpId    = uint64_t;
using SchedId = uint64_t;
using Edges   = std::vector<std::vector<OpId>>;

// A class for compactly storing all dependencies between Nodes (Ops) in a
// DAG. Queries for implicit topological constraints between any 2 Ops are
// performed in O(1) time. Memory consumption is O(nOps^2) and object
// construction time is O(nOps * nEdges). The implemenation of the class is
// careful to keep the constants in these complexities low.
class PathMatrix {

public:
  PathMatrix(const Edges &forwardEdges);

  // Returns true if there exist no schedules with "to" before "from"
  bool constrained(OpId from, OpId to) const {
    auto index = to * nBitSetsPerOp + from / BitSetSize;
    auto shift = from % BitSetSize;
    return fwdEdgeSet[index][shift];
  }

  // Returns true iff there exists
  // at least 1 schedule with a before b, and
  // at least 1 schedule with b before a.
  bool unconstrained(OpId a, OpId b) const {
    return !constrained(a, b) && !constrained(b, a);
  }

  // The lowest SchedId that "a" has over all schedules
  SchedId earliest(OpId a) const { return nFwdBefore[a]; }

  // The highest SchedId that "a" has over all schedules
  SchedId latest(OpId a) const { return nOps_u64() - nBwdBefore[a] - 1; }

  uint64_t nOps_u64() const { return nOps; }
  uint64_t nOps_i64() const { return static_cast<int64_t>(nOps); }

  // The set of forward edges passed to the constructor which are redundant.
  // That is, all edges which if removed would not change the total number of
  // schedules
  const auto &getFwdRedundant() const { return fwdRedundant; }

  // The same edges as getFwdRedundant(), but reversed
  const auto &getBwdRedundant() const { return bwdRedundant; }

  static uint64_t getNBitSetsPerOp(uint64_t nOps) {
    return nOps / BitSetSize + (nOps % BitSetSize != 0);
  }

private:
  uint64_t nOps;
  uint64_t nBitSetsPerOp;
  uint64_t nBitSets;

  Edges fwd;
  std::vector<BitSet> fwdEdgeSet;
  std::vector<uint64_t> nFwdBefore;
  std::vector<std::array<OpId, 2>> fwdRedundant;

  Edges bwd;
  std::vector<BitSet> bwdEdgeSet;
  std::vector<uint64_t> nBwdBefore;
  std::vector<std::array<OpId, 2>> bwdRedundant;

  // Diagram:
  //          from
  //
  //        **** ****
  //        **** ****
  //  to    **** ****
  //        **** ****
  //        **** ****
  //        **** ****
  //        **** ****
  //        **** ****
  //
  // An PathMatrix is O(nOps^2) in memory. Each of fwdEdgeSet and bwdEdgeSet
  // store nOps^2 + O(1) bits, recording forward and backard constraints,
  // respectively.
  //
  // In the diagram, BitSetSize is 4 and nOps is 8. Each * in the diagram s a
  // constraint between 2 Ops, and will either be on or off.
  //
  // The heavy lifting of the algorithm is in doing bitwise addition of 2
  // rows, and summation over columns. It is for this reason that std::bitset
  // is used in the direction it is.
  //
  // Note that bwdEdgeSet is the transpose of fwdEdgeSet, and so is not
  // required to be stored. However, certain operations are significantly
  // faster using the transposed layout, and so it is kept.
  //
  //
  // Example:
  //
  //       X0
  //      / \
  //     X1  X2
  //      \ /
  //       X3
  //         \
  //          X4
  //
  //  has fwdEdgeSet:
  //
  //       from
  //       01234
  //     0 00000
  //     1 10000
  //  to 2 10000
  //     3 11100
  //     4 11110
  //
};

} // namespace pathmatrix
} // namespace schedule
} // namespace poprithms

#endif