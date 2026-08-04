// Adapted from react-native-random-forest (https://github.com/tony-div/react-native-random-forest)
// by Tony George. See README "Attribution" section.
#pragma once

#include <string>
#include <vector>

namespace margelo::nitro::signalscorer {

/**
 * C++ FFI bridge to the Rust signal-scoring core.
 *
 * All methods fall back to a portable C++ implementation when the Rust
 * static library is not linked (i.e. `SS_USE_RUST` is not defined), so the
 * package remains fully functional even without pre-built Rust binaries.
 */
class SignalScorerRustBridge {
 public:
  std::vector<double> evaluate(const std::vector<double>& values);
  std::vector<double> evaluateBatch(const std::vector<double>& flatData, int windowSize);
  void reset(const std::string& modelName);
};

} // namespace margelo::nitro::signalscorer
