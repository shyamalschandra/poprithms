#ifndef POPRITHMS_SCHEDULE_ANNEAL_GRAPH
#define POPRITHMS_SCHEDULE_ANNEAL_GRAPH

#include <array>
#include <map>
#include <set>
#include <tuple>
#include <vector>
#include <poprithms/schedule/anneal/alloc.hpp>
#include <poprithms/schedule/anneal/allocweight.hpp>
#include <poprithms/schedule/anneal/annealusings.hpp>
#include <poprithms/schedule/anneal/op.hpp>
#include <poprithms/schedule/anneal/pathmatrixoptimizations.hpp>
#include <poprithms/schedule/anneal/schedulechange.hpp>
#include <poprithms/schedule/anneal/shiftandcost.hpp>
#include <poprithms/schedule/anneal/trackentry.hpp>
#include <poprithms/schedule/pathmatrix/pathmatrix.hpp>

// Design of the schedule annealing algorithm
// -------------------------------------------
// - store all schedule dependant information in the Graph class, not
//   the Op or the Alloc classes. With this decision, Ops and Allocs will
//   never be updated once the annealing begins
//
// - make the search algorithm for updates as fast as possible, at the expense
//   of the update algorithm. This because (1) finding swaps is easily
//   parallelisable and (2) updates are few and far between, especially at
//   later iterations of the algorithm, so most time is spent searching for
//   swaps

// TODO(T14827) Parallelize the search for energy reducing swaps. Suggestions:
// What the best approach is depends on whether we require the algorithm to be
// deterministic. Assuming that we do, this is what I propose (for the search)
// a vector of indices to process, toProcess, and a nextIndex, initialized to
// 0 Each thread, when ready, gets nextIndex and increments it by 1.
//
// It processes its index, and if there an improvement, requests all searching
// to halt. When all threads are finished their index, take the lowest index
// improves, call it updateIndex. Apply update, and reset nextIndex to
// updateIndex

namespace poprithms {
namespace schedule {
namespace anneal {

// Algorithms give exactly same results, RIPPLE is just much faster
enum class MinSumLivenessAlgo { SIMPLE, RIPPLE };

// The algorithm is initialized with a single run of Kahn's algorithm, the
// tie-breaker does not make much difference to overall performance of the
// algorithm but GREEDY means slightly fewer shifts are required when
// annealing starts
enum class KahnTieBreaker { RANDOM = 0, GREEDY, FIFO, N };
static constexpr auto NKahnTieBreakers =
    static_cast<uint64_t>(KahnTieBreaker::N);
std::ostream &operator<<(std::ostream &, KahnTieBreaker);
KahnTieBreaker kahnTieBreaker(const std::string &);

class Graph {
public:
  // The Graph is grown incrementally with these functions:

  // Create an Alloc
  AllocAddress insertAlloc(AllocWeight w);

  AllocAddress insertAlloc(double w) { return insertAlloc({w, 0}); }

  // Create an Op, return its OpAddress
  OpAddress insertOp(const std::string &dbString);

  // Create multiple Ops, return their OpAddresses
  std::vector<OpAddress> insertOps(const std::vector<std::string> &dbStrings);

  // Register that "aa" must be live when "oa" executes
  void insertOpAlloc(OpAddress oa, AllocAddress aa);

  // Register that "aa" must be live when "oas" execute
  void insertOpAlloc(const std::vector<OpAddress> &oas, AllocAddress aa);

  // Register that "before" must execute before "after"
  void insertConstraint(OpAddress before, OpAddress after);

  // Register one constraint for each element of "css"
  using BeforeAndAfter = std::array<OpAddress, 2>;
  void insertConstraints(const std::vector<BeforeAndAfter> &css);

  // Register that "before" must be executed before "after", and that no Ops
  // can be executed between "before" and "after"
  void insertLink(OpAddress before, OpAddress after);

  // The above methods are combined in some convenience methods:
  template <typename A, typename B>
  OpAddress insertOp(A &&befores, B &&allocs, const std::string &dbString) {
    auto opId = insertOp(dbString);
    for (auto &&x : befores) {
      insertConstraint(x, opId);
    }
    for (auto &&x : allocs) {
      insertOpAlloc(opId, x);
    }
    return opId;
  }

  OpAddress insertOp(std::initializer_list<OpAddress> befores,
                     std::initializer_list<AllocAddress> allocs,
                     const std::string &dbString) {
    return insertOp(std::vector<OpAddress>(befores),
                    std::vector<AllocAddress>(allocs),
                    dbString);
  }

  // A new Graph can be generated by merging groups of Ops in this
  // Graph into single Ops. The method below does this. The returned tuple
  // consists of (1) the reduced Graph, containing merged Ops and (2) a
  // mapping from the Ops in the reduced (child) Graph to Ops in this
  // (the parent) Graph.
  using ParentGraphOps = std::vector<std::vector<OpAddress>>;
  using OpMerged       = std::tuple<Graph, ParentGraphOps>;
  OpMerged getMerged(std::vector<std::vector<OpAddress>> chains) const;

  // Merges all chains formed of Ops with Links
  // Recall : linked Ops are guarenteed to be scheduled contiguously.
  OpMerged getLinkMerged() const;

  // Merges all chains formed of tightly paired Ops
  // Recall : two Ops are said to be tightly paired if one is the unique
  // output of the other, which in turn is the unique input of the first.
  OpMerged getTightMerged() const;

  static Graph fromSerializationString(const std::string &);
  void appendSerialization(std::ostream &) const;
  std::string getSerializationString() const;

  void append(std::ostream &ost) const;

  const std::vector<Op> &getOps() const { return allOps; }
  const Op &getOp(OpAddress address) const { return allOps[address]; }
  uint64_t nOps() const { return allOps.size(); }
  int nOps_i32() const { return static_cast<int>(nOps()); }

  const std::vector<Alloc> &getAllocs() const { return allAllocs; }
  const Alloc &getAlloc(AllocAddress address) const {
    return allAllocs[address];
  }
  uint64_t nAllocs() const { return getAllocs().size(); }
  std::string getLivenessString() const;

  // to be called once, when growing of Graph is complete
  void
  initialize(KahnTieBreaker              = KahnTieBreaker::GREEDY,
             uint32_t kahnSeed           = defaultKahnSeed(),
             PathMatrixOptimizations pmo = PathMatrixOptimizations::allOff());

  void initialize(const std::map<std::string, std::string> &);

  // Should be called once after the final call to a growing member. Sorts
  // certain Op member ids to accelerate the annealing algorithm
  void finalize();
  bool isSchedulable() const;

  // verify that all graph connections are sensible
  void assertCorrectness() const;

  static bool defaultDebug() { return false; }
  static uint32_t defaultMinSumLivenessSeed() { return 1; }
  static double defaultPStayPut() { return 10.0; }
  static double defaultPHigherFallRate() { return 2.0; }
  static double defaultPClimb() { return 1.0; }
  static bool defaultLogging() { return true; }
  static bool defaultFilterSusceptible() { return true; }
  static double defaultTimeLimitSeconds() { return 1e9; }
  static int64_t defaultSwapLimitCount() { return static_cast<int64_t>(1e9); }

  static KahnTieBreaker defaultKahnTieBreaker() {
    return KahnTieBreaker::GREEDY;
  }
  static uint32_t defaultKahnSeed() { return 1; }

  // All Ops which thus far do not have any input dependencies
  std::vector<OpAddress> getInputOps() const;

  // definition of a "round":
  // one iteration through all Ops to search for,
  // and possibly apply, improvements
  //
  // After each round with at least 1 improvement, the
  // algorithm runs again with the same nToShift
  //
  // Arguments are
  // algo : RIPPLE (recommended) or SIMPLE (slow) : identical scheduling but
  // RIPPLE uses techniques to make it fast
  //
  // debug : compares "algo" above to SIMPLE to confirm agreement, and checks
  // state of graph edges at each iteration. This makes execution slow
  //
  // seed : the algorithm randomly shuffles op indices in each round
  //
  // filterSusceptible : in each round which follows a round with at least one
  // shift, only considershifts of ranges if at least one Op in the
  // range has a constraint to an Op which moved in the previous round.
  // Changing this boolean value may change the final local minimum found
  //
  // logging : log the choice between a,b,c at each round

  void
  minSumLivenessAnneal(MinSumLivenessAlgo algo = MinSumLivenessAlgo::RIPPLE,
                       bool debug              = defaultDebug(),
                       uint32_t seed           = defaultMinSumLivenessSeed(),
                       bool filterSusceptible  = defaultFilterSusceptible(),
                       bool logging            = defaultLogging(),
                       double timeLimitSeconds = defaultTimeLimitSeconds(),
                       int64_t swapLimitCount  = defaultSwapLimitCount());

  // Deprecated API
  // timeline:
  // From 2 April 2020 using this API may result in an error being thrown.
  // pStayPut, pHigherFallRate, pClimb are no longer supported.
  void minSumLivenessAnneal(MinSumLivenessAlgo algo,
                            bool debug,
                            uint32_t seed,
                            double pStayPut,
                            double pHigherFallRate,
                            double pClimb,
                            bool logging,
                            double timeLimitSeconds,
                            int64_t swapLimitCount);

  void minSumLivenessAnneal(const std::map<std::string, std::string> &);

  AllocWeight getMaxLiveness() const;

  AllocWeight getSumLiveness() const;

  AllocWeight scheduleToLiveness(ScheduleIndex i) const {
    return schToLiveness[i];
  }
  OpAddress scheduleToOp(ScheduleIndex i) const {
    return schToOp[static_cast<uint64_t>(i)];
  }
  ScheduleIndex opToSchedule(OpAddress a) const { return opToSch[a]; }

  // sorted schedule indices at which alloc is used
  const std::vector<ScheduleIndex> &allocToSchedule(AllocAddress a) const {
    return allocToSch[a];
  }
  ScheduleIndex allocToFirstSchedule(AllocAddress a) const {
    return allocToSch[a][0];
  }
  ScheduleIndex allocToFinalSchedule(AllocAddress a) const {
    return allocToSch[a].back();
  }

  // the allocs required by the op at a schedule index
  const std::vector<AllocAddress> &scheduleToAllocs(ScheduleIndex i) const {
    return schToAllocs[i];
  }

  // schedule indices of an ops inputs, sorted
  const std::vector<ScheduleIndex> &opToInSchedule(OpAddress a) const {
    return opToInSch[a];
  }

  // schedule indices of an ops output, sorted
  const std::vector<ScheduleIndex> &opToOutSchedule(OpAddress a) const {
    return opToOutSch[a];
  }

  int getNCanFwd(ScheduleIndex i) const {
    return nCanFwd[static_cast<uint64_t>(i)];
  }
  int getNCanBwd(ScheduleIndex i) const {
    return nCanBwd[static_cast<uint64_t>(i)];
  }

  const std::vector<OpAddress> &getScheduleToOp() const { return schToOp; }

  // The following are convenience functions:

  // Ops in "bins" must execute in increasing bin index. For example, if a \in
  // bins[0] and b \in bins[1], then a must execute before b
  void insertBinConstraints(const std::vector<std::vector<OpAddress>> &bins,
                            const std::string &opPrefix);

  // pairs a,b \in "pairs" should be executed as close to each other as
  // possible, with "gravitational force" w
  void insertAttractions(const std::vector<std::array<OpAddress, 2>> &pairs,
                         AllocWeight w);

  bool operator==(const Graph &rhs) const {
    return allOps == rhs.allOps && allAllocs == rhs.allAllocs;
  }
  bool operator!=(const Graph &rhs) const { return !operator==(rhs); }

  // A pair of Ops (a,b) is defined to be a "tight pair" if
  // 1) b is the only output of a,
  // 2) a is the only input of b.
  // Let C(a) be the set of all Ops c s.t. there is no implicit constraint
  // between a and c. It is easy to see that (a,b) is tight implies C(a) =
  // C(b), but C(a) = C(b) does not imply (a,b) is tight.
  std::vector<std::array<OpAddress, 2>> getTightPairs() const;

  // Starting from "a" and proceeding through Op outputs, find a chain of
  // tightly pairs Op. The returned vector may be the singleton, {a}, if it is
  // not tightly coupled to an output.
  std::vector<OpAddress> tightChainFrom(OpAddress a) const;

private:
  // Return true if there are no linked Ops which would be disconnected by a
  // shift of Ops
  bool isLinkPreserving(ScheduleIndex start0,
                        ScheduleIndex start1,
                        int nToShift) const;

  template <typename T>
  ScheduleIndex getExtremaIndexWithNonUniqueSolution() const;

  void kahn(KahnTieBreaker, uint32_t kahnSeed);
  void removeConstraint(OpAddress before, OpAddress after);

  void confirmShiftAndCost(ScheduleIndex start0,
                           int nToShift,
                           const ShiftAndCost &shiftAndCost,
                           MinSumLivenessAlgo algo) const;

  // The first external consumer of an Op in the range [start, nToShift)
  ScheduleIndex getFirstConsumer(ScheduleIndex start, int nToShift) const;

  // The last external producer of an Op in the range [start, nToShift)
  ScheduleIndex getLastProducer(ScheduleIndex start, int nToShift) const;

  void applyChange(const ScheduleChange &);

  // Note, ALL vectors are maintained sorted in Graph and Op

  // unchanged after initialization: never updated
  std::vector<Op> allOps;
  std::vector<Alloc> allAllocs;

  // updated EVERY time the schedule changes
  std::vector<OpAddress> schToOp;
  std::vector<ScheduleIndex> opToSch;
  std::vector<std::vector<ScheduleIndex>> allocToSch;
  std::vector<std::vector<AllocAddress>> schToAllocs;
  std::vector<std::vector<ScheduleIndex>> opToInSch;
  std::vector<std::vector<ScheduleIndex>> opToOutSch;
  std::vector<int> nCanFwd;
  std::vector<int> nCanBwd;
  std::vector<bool> susceptible;

  // not updated every time the schedule changes
  std::vector<AllocWeight> schToLiveness;

  std::vector<AllocWeight> getRippleCosts(ScheduleIndex start0,
                                          int nToShift,
                                          int sign,
                                          int nCostsToCompute,
                                          int dirOffset) const;

  std::vector<AllocWeight>
  getFwdRippleCosts(ScheduleIndex start, int nToShift, int firstExtCon) const;

  std::vector<AllocWeight> getBwdRippleCosts(ScheduleIndex start0,
                                             int nToShift,
                                             int lastExtProd) const;

  ShiftAndCost getBestShiftRippleAlgo(const ScheduleIndex start,
                                      const int nToShift) const;

  ShiftAndCost getBestShiftSimpleAlgo(const ScheduleIndex start,
                                      const int nToShift) const;

  AllocWeight getShiftCost(ScheduleIndex start0,
                           ScheduleIndex start1,
                           int nToShift,
                           const Alloc &) const;

  std::vector<AllocAddress> getAllocAddresses(ScheduleIndex start,
                                              ScheduleIndex end) const;

  std::vector<AllocWeight> getDeltaLiveness() const;

  void setSchToLiveness();

  void setOpToInSch(OpAddress);

  void setOpToOutSch(OpAddress);

  void setAllocToSch(AllocAddress);

  void setCanCan(int nToShift);

  void updateCanCan(int oldNToShift, int newNToShift);

  void updateNCanFwds(int nToShift,
                      int x0,
                      int o1,
                      const std::vector<OpAddress> &producersTouched);

  void updateNCanBwds(int nToShift,
                      int x0,
                      int o1,
                      const std::vector<OpAddress> &consumersTouched);

  // Susceptible Ops are Ops in (out) the range [rangeStart, rangeEnd] which
  // have a dependency outside the range
  void updateSusceptible(ScheduleIndex rangeStart, ScheduleIndex rangeEnd);

  // TODO(T14827) for multithreading, need one of these scratchpads per thread
  mutable std::vector<TrackEntry> rippleScratch;

  bool isFinalized{false};
  bool isInitialized{false};

  // The unique OpAddresses of Ops which have a forward link
  std::vector<OpAddress> opsWithFwdLinks;

public:
  // return all Ops which have the same ins as a. "a" is an element of the
  // returned vector
  std::vector<OpAddress> getIdenticalIns(OpAddress a) const;

  std::vector<std::vector<OpAddress>> getForwardEdges() const;

  // Combine all linked Ops form a sets of isolated chains
  std::vector<std::vector<OpAddress>> getLinkChains() const;

  // Combine all tight Op pairs to form sets of isolated chains
  std::vector<std::vector<OpAddress>> getTightChains() const;

  bool hasAtLeastOneLink() const { return !opsWithFwdLinks.empty(); }

  // insert a proxy Op, which is constrained to be scheduled very early, and
  // 1 Alloc, which must be live for "proxy" and Op "a". This attracts "a"
  // towards the beginning of the schedule. The Allocs' AllocWeights, which
  // determines the force of attraction of "a" to the beginning of the
  // schedule, determined by "relativeLexico" and "stepSize".
  template <typename T>
  void insertStartAttractors(const std::vector<OpAddress> &opAddresses,
                             const std::vector<T> &priorities,
                             int relativeLexico,
                             double stepSize = 1.0) {

    // For each Op "a" \in opAddresses, the size of the attracting Alloc is
    // determined by the corresponding priority in "priorities"
    insertStartAttractorsAssert0(opAddresses.size(), priorities.size());

    // All Ops which have no dependencies and can legally be executed first
    auto inputs = getInputOps();

    // sort and unique-ify the priorities
    std::vector<T> unipris(priorities);
    std::sort(unipris.begin(), unipris.end());
    auto last = std::unique(unipris.begin(), unipris.end());
    unipris.erase(last, unipris.cend());

    // If all the priorities are the same, then return - giving all Ops the
    // same level attraction to the start is equivalent to giving them all no
    // attraction to the start.
    if (unipris.size() <= 1) {
      return;
    }

    // Give each unique T a corresponding AllocWeight:
    std::map<T, AllocWeight> ws;
    for (uint64_t i = 0; i < unipris.size(); ++i) {
      ws.insert(
          {unipris[i],
           AllocWeight(stepSize * static_cast<double>(i), relativeLexico)});
    }

    std::vector<OpAddress> attractors;

    for (uint64_t i = 0UL; i < opAddresses.size(); ++i) {
      auto opAddress = opAddresses[i];
      auto pri       = priorities[i];
      auto w         = ws.at(pri);

      if (w != AllocWeight(0)) {
        auto allocAddress = insertAlloc(w);

        std::string attractorStr{"priorityAttractor_" +
                                 getOp(opAddress).getDebugString() + "_" +
                                 toString(w)};

        auto attractor = insertOp({}, {allocAddress}, attractorStr);

        insertOpAlloc(opAddress, allocAddress);
        attractors.push_back(attractor);
      }
    }

    // force attractors to be in a fixed order at the start of the schedule
    for (auto x = std::next(attractors.cbegin()); x != attractors.cend();
         std::advance(x, 1)) {
      insertConstraint(*std::prev(x), *x);
    }
    for (auto x : inputs) {
      insertConstraint(*attractors.crbegin(), x);
    }
  }

private:
  // Insert constraints and links which can be proven to satisfy at least one
  // globally minimizing schedule. These constraints accelerate the annealing
  // algorithm by reducing its search space.
  void applyPathMatrixOptimizations(const PathMatrixOptimizations &);

  pathmatrix::PathMatrix pathMatrix{{}};
  // The lowest change in liveness across all schedules, for each Op
  std::vector<AllocWeight> lowerBoundChange;
  // The highest change in liveness across all schedules, for each Op
  std::vector<AllocWeight> upperBoundChange;

  void initializePathMatrix();
  bool linkTightDrops();
  bool linkCloseTightPairs();
  bool constrainWeightSeparatedGroups();
  bool constrainParallelChains();
  bool slideLinks();
  void insertStartAttractorsAssert0(uint64_t, uint64_t) const;
};

std::ostream &operator<<(std::ostream &ost, const Graph &x);

std::ostream &operator<<(std::ostream &ost, const ShiftAndCost &x);

std::ostream &operator<<(std::ostream &ost, const ScheduleChange &x);

} // namespace anneal
} // namespace schedule
} // namespace poprithms

#endif
