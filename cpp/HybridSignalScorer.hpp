#pragma once

#include <memory>
#include <string>
#include <vector>

#include <NitroModules/HybridObject.hpp>

#include "HybridSignalScorerSpec.hpp"
#include "rust/SignalScorerRustBridge.hpp"

namespace margelo::nitro::signalscorer {

class HybridSignalScorer : public HybridSignalScorerSpec {
 public:
  HybridSignalScorer() : HybridObject(TAG) {}
  ~HybridSignalScorer() override = default;

  std::vector<double> evaluate(const std::vector<double>& values) override;
  std::vector<double> evaluateBatch(const std::vector<double>& flatData, double windowSize) override;
  void reset(const std::string& modelName) override;

  void loadHybridMethods() override;

 private:
  SignalScorerRustBridge rust_;
};

} // namespace margelo::nitro::signalscorer
