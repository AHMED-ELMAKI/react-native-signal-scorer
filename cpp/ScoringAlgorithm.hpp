#pragma once

#include <algorithm>
#include <cmath>
#include <cstddef>
#include <vector>

namespace margelo::nitro::signalscorer {

/**
 * Portable implementation of the signal scoring algorithm.
 *
 * This is the single source of truth for the scoring math, shared by the
 * C++ fallback engine and available to the hybrid object. The Rust core
 * mirrors the same math via FFI so results are identical across layers.
 */
struct ScoredWindow {
  double code;       // 0=steady, 1=accelerating, 2=volatile
  double confidence; // in [0,1]
  double score;      // in [0,1]
  double slope;
  double volatility;
  double mean;
};

inline double clampValue(double value, double min, double max) {
  return std::max(min, std::min(max, value));
}

inline ScoredWindow computeScore(const std::vector<double>& values) {
  ScoredWindow out{0, 0, 0, 0, 0, 0};

  if (values.empty()) {
    return out;
  }

  const size_t n = values.size();
  double sum = 0;
  for (double v : values) {
    sum += v;
  }
  const double mean = sum / static_cast<double>(n);

  const double first = values.front();
  const double last = values.back();
  const double slope = (last - first) / std::max(1.0, static_cast<double>(n - 1));

  double variance = 0;
  for (double v : values) {
    const double d = v - mean;
    variance += d * d;
  }
  variance /= static_cast<double>(n);
  const double volatility = std::sqrt(variance);

  const double absMean = std::max(1.0, std::abs(mean) + 1.0);
  const double normalizedSlope = clampValue(slope / absMean, -1.0, 1.0);
  const double normalizedVolatility = clampValue(volatility / absMean, 0.0, 1.0);
  const double score = clampValue(0.5 + 0.35 * normalizedSlope + 0.15 * normalizedVolatility, 0.0, 1.0);

  double code = 0; // steady
  if (score > 0.7) {
    code = 1; // accelerating
  } else if (normalizedVolatility > 0.45) {
    code = 2; // volatile
  }

  const double confidence = clampValue(
      0.55 + 0.2 * std::abs(normalizedSlope) + 0.25 * normalizedVolatility, 0.0, 1.0);

  out.code = code;
  out.confidence = confidence;
  out.score = score;
  out.slope = slope;
  out.volatility = volatility;
  out.mean = mean;
  return out;
}

inline std::vector<double> computeScoreFlat(const std::vector<double>& values) {
  ScoredWindow w = computeScore(values);
  return {w.code, w.confidence, w.score, w.slope, w.volatility, w.mean};
}

inline std::vector<double> computeScoreBatchFlat(const std::vector<double>& flatData, int windowSize) {
  std::vector<double> out;
  if (windowSize <= 0) return out;
  const size_t n = flatData.size();
  const size_t win = static_cast<size_t>(windowSize);
  if (n < win) return out;

  for (size_t i = 0; i + win <= n; i++) {
    std::vector<double> window(flatData.begin() + static_cast<std::ptrdiff_t>(i),
                               flatData.begin() + static_cast<std::ptrdiff_t>(i + win));
    ScoredWindow w = computeScore(window);
    out.insert(out.end(), {w.code, w.confidence, w.score, w.slope, w.volatility, w.mean});
  }
  return out;
}

} // namespace margelo::nitro::signalscorer
