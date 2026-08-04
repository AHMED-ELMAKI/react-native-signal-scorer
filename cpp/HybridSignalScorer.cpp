#include "HybridSignalScorer.hpp"

#include "ScoringAlgorithm.hpp"

namespace margelo::nitro::signalscorer {

void HybridSignalScorer::loadHybridMethods() {
  HybridSignalScorerSpec::loadHybridMethods();
}

std::vector<double> HybridSignalScorer::evaluate(const std::vector<double>& values) {
  auto result = rust_.evaluate(values);
  if (result.size() >= 6) {
    return result;
  }
  // Rust unavailable -> fallback to the portable C++ engine.
  return computeScoreFlat(values);
}

std::vector<double> HybridSignalScorer::evaluateBatch(const std::vector<double>& flatData, double windowSize) {
  int win = static_cast<int>(windowSize);
  auto result = rust_.evaluateBatch(flatData, win);
  if (!result.empty() || flatData.empty() || win <= 0) {
    return result;
  }
  return computeScoreBatchFlat(flatData, win);
}

void HybridSignalScorer::reset(const std::string& modelName) {
  rust_.reset(modelName);
}

} // namespace margelo::nitro::signalscorer
