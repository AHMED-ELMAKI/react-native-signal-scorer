#pragma once

#include <algorithm>
#include <cmath>
#include <cstddef>

namespace margelo::nitro::signalscorer {

/**
 * A single scored snapshot produced by {@link TickerPulse}.
 *
 * `hue` is a numeric label: 0 = flat, 1 = rising, 2 = wild.
 */
struct TickSnapshot {
  double hue = 0.0;
  double confidence = 0.0;
  double score = 0.0;
  double velocity = 0.0;
  double spread = 0.0;
  double level = 0.0;
};

inline double squeeze(double value, double lo, double hi) {
  return std::max(lo, std::min(hi, value));
}

/**
 * A stateful, per-stream scoring engine. Ingest samples one at a time; the
 * engine maintains rolling EMA estimates of level, velocity, and dispersion.
 */
class TickerPulse {
 public:
  TickerPulse() = default;

  /** Feeds one sample into the engine and advances the rolling state. */
  void ingest(double sample) {
    if (samplesSeen_ == 0) {
      latestLevel_ = sample;
      priorLevel_ = sample;
      latestVelocity_ = 0.0;
      latestSpread_ = 0.0;
      samplesSeen_ = 1;
      return;
    }

    priorLevel_ = latestLevel_;
    latestLevel_ = kLevelAlpha * sample + (1.0 - kLevelAlpha) * latestLevel_;

    const double delta = latestLevel_ - priorLevel_;
    latestVelocity_ = kVelocityAlpha * delta + (1.0 - kVelocityAlpha) * latestVelocity_;

    const double error = sample - latestLevel_;
    latestSpread_ = kSpreadAlpha * std::fabs(error) + (1.0 - kSpreadAlpha) * latestSpread_;

    ++samplesSeen_;
  }

  /** Returns the current scored snapshot without disrupting state. */
  TickSnapshot glimpse() const {
    TickSnapshot out;
    if (samplesSeen_ == 0) {
      return out;
    }

    const double scale = std::max(1.0, std::fabs(latestLevel_) + 1.0);
    const double normVelocity = squeeze(latestVelocity_ / scale, -1.0, 1.0);
    const double normSpread = squeeze(latestSpread_ / scale, 0.0, 1.0);

    out.score = squeeze(0.5 + 0.4 * normVelocity - 0.2 * normSpread, 0.0, 1.0);

    if (normSpread > 0.55) {
      out.hue = 2.0; // wild
    } else if (out.score > 0.62) {
      out.hue = 1.0; // rising
    } else {
      out.hue = 0.0; // flat
    }

    out.confidence = squeeze(
        0.5 + 0.3 * std::fabs(normVelocity) + 0.2 * (1.0 - normSpread), 0.0, 1.0);

    out.velocity = latestVelocity_;
    out.spread = latestSpread_;
    out.level = latestLevel_;
    return out;
  }

  /** Clears all observed history. */
  void rewind() {
    samplesSeen_ = 0;
    latestLevel_ = 0.0;
    priorLevel_ = 0.0;
    latestVelocity_ = 0.0;
    latestSpread_ = 0.0;
  }

 private:
  static constexpr double kLevelAlpha = 0.25;
  static constexpr double kVelocityAlpha = 0.5;
  static constexpr double kSpreadAlpha = 0.2;

  std::size_t samplesSeen_ = 0;
  double latestLevel_ = 0.0;
  double priorLevel_ = 0.0;
  double latestVelocity_ = 0.0;
  double latestSpread_ = 0.0;
};

} // namespace margelo::nitro::signalscorer
