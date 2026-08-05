#pragma once

#include <memory>
#include <vector>

#include <NitroModules/HybridObject.hpp>

#include "HybridTickScorerSpec.hpp"
#include "rust/TickerRustBridge.hpp"

namespace margelo::nitro::signalscorer {

class HybridTickScorer : public HybridTickScorerSpec {
 public:
  HybridTickScorer();
  ~HybridTickScorer() override = default;

  void update(double sample) override;
  std::vector<double> getSignal() override;
  void rewind() override;

 private:
  TickerRustBridge rust_;
};

} // namespace margelo::nitro::signalscorer
