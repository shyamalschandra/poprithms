#include <poprithms/schedule/anneal/allocweight.hpp>
#include <poprithms/schedule/anneal/error.hpp>

int main() {

  using namespace poprithms::schedule::anneal;

  auto wLargeNeg = AllocWeight(-.001, -2);
  auto wNegOne   = AllocWeight::negativeOne();
  auto wSmallNeg = AllocWeight(-10.0, +2);
  auto wZero     = AllocWeight::zero();
  auto wSmallPos = AllocWeight(100.0, +2);
  auto wLargePos = AllocWeight(.0001, -2);
  auto wMax      = AllocWeight::numericMaxLimit();

  if (wSmallPos == wLargePos) {
    throw error("Error with AllocWeight's operator==");
  }

  if (wZero != wZero) {
    throw error("Error with AllocWeight's operator!=");
  }

  if (!(wLargeNeg < wNegOne && wNegOne < wZero && wZero < wSmallPos &&
        wSmallPos < wLargePos && wLargePos < wMax)) {
    throw error("Error with AllocWeight's operator<");
  }

  if (!(wMax <= wMax) || wMax < wZero) {
    throw error("Error with AllocWeight's operator<=");
  }

  if (wMax.get(+1) != std::numeric_limits<double>::max()) {
    throw error("Error with AllocWeight::get(.)");
  }

  if (wMax.getL1() != std::numeric_limits<double>::infinity()) {
    throw error("Error with AllocWeight::getL1()");
    //
  }

  if (wMax.getAbsolute() != wMax) {
    throw error("Error with AllocWeight::getAbs()");
  }

  return 0;
}