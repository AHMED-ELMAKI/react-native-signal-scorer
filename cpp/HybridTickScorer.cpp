#include "HybridTickScorer.hpp"

namespace margelo::nitro::signalscorer {

HybridTickScorer::HybridTickScorer() = default;

void HybridTickScorer::update(double sample) {
  rust_.update(sample);
}

std::vector<double> HybridTickScorer::getSignal() {
  return rust_.getSignal();
}

void HybridTickScorer::rewind() {
  rust_.rewind();
}

} // namespace margelo::nitro::signalscorer
