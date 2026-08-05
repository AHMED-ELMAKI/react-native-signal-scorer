#include "HybridSignalTickerFactory.hpp"

namespace margelo::nitro::signalscorer {

std::shared_ptr<HybridTickScorerSpec> HybridSignalTickerFactory::create() {
  return std::make_shared<HybridTickScorer>();
}

} // namespace margelo::nitro::signalscorer
