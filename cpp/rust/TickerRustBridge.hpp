#pragma once

#include <vector>

namespace margelo::nitro::signalscorer {

/**
 * C++ FFI bridge to the Rust streaming scoring core.
 *
 * Owns an opaque scorer handle and forwards calls to the Rust engine. The
 * Rust core is the single source of truth for scoring; the C++ {@link
 * TickerPulse} engine is used only as a development-time parity reference.
 */
class TickerRustBridge {
 public:
  TickerRustBridge();
  ~TickerRustBridge();

  // Non-copyable because of the owned handle.
  TickerRustBridge(const TickerRustBridge&) = delete;
  TickerRustBridge& operator=(const TickerRustBridge&) = delete;

  /** Feeds one sample into the Rust engine. */
  void update(double sample);

  /** Returns the current scored row: [hue, confidence, score, velocity, spread, level]. */
  std::vector<double> getSignal();

  /** Clears the Rust engine's history. */
  void rewind();

 private:
  void* handle_ = nullptr;
};

} // namespace margelo::nitro::signalscorer
