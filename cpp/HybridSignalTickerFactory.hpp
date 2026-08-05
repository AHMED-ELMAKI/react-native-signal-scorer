#pragma once

#include <memory>

#include <NitroModules/HybridObject.hpp>

#include "HybridTickScorerFactorySpec.hpp"
#include "HybridTickScorer.hpp"

namespace margelo::nitro::signalscorer {

class HybridSignalTickerFactory : public HybridTickScorerFactorySpec {
 public:
  HybridSignalTickerFactory() : HybridObject(TAG) {}
  ~HybridSignalTickerFactory() override = default;

  std::shared_ptr<HybridTickScorerSpec> create() override;
};

} // namespace margelo::nitro::signalscorer
