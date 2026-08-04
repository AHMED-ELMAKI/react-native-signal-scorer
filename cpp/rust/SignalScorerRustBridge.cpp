#include "SignalScorerRustBridge.hpp"

#include "../ScoringAlgorithm.hpp"

#include <cstddef>
#include <cstdio>

#if defined(__ANDROID__)
#include <android/log.h>
#endif

#if defined(SS_USE_RUST)
extern "C" {
double* ss_evaluate(const double* data, int len, int* out_len);
double* ss_evaluate_batch(const double* data, int len, int window_size, int* out_len);
void ss_reset(const char* registry_name);
void ss_free_data(double* ptr);
}
#endif

namespace margelo::nitro::signalscorer {

namespace {

constexpr const char* kLogTag = "NitroSignalScorer";

void logDebug(const char* message) {
#if defined(__ANDROID__)
  __android_log_print(ANDROID_LOG_DEBUG, kLogTag, "%s", message);
#else
  std::fprintf(stderr, "[%s] %s\n", kLogTag, message);
#endif
}

} // namespace

static std::vector<double> copyFromCArray(double* data, int len) {
  std::vector<double> result;
  if (data == nullptr || len <= 0) return result;
  result.assign(data, data + len);
  return result;
}

std::vector<double> SignalScorerRustBridge::evaluate(const std::vector<double>& values) {
#if defined(SS_USE_RUST)
  logDebug("rust.evaluate(): begin");
  int outLen = 0;
  double* outData = nullptr;
  int len = static_cast<int>(values.size());
  if (!values.empty()) {
    outData = ss_evaluate(values.data(), len, &outLen);
  }
  auto result = copyFromCArray(outData, outLen);
  if (outData != nullptr) {
    ss_free_data(outData);
  }
  logDebug("rust.evaluate(): complete");
  return result;
#else
  (void)values;
  logDebug("rust.evaluate(): SS_USE_RUST disabled, using C++ fallback");
  return {};
#endif
}

std::vector<double> SignalScorerRustBridge::evaluateBatch(const std::vector<double>& flatData, int windowSize) {
#if defined(SS_USE_RUST)
  logDebug("rust.evaluateBatch(): begin");
  int outLen = 0;
  double* outData = nullptr;
  int len = static_cast<int>(flatData.size());
  if (!flatData.empty() && windowSize > 0) {
    outData = ss_evaluate_batch(flatData.data(), len, windowSize, &outLen);
  }
  auto result = copyFromCArray(outData, outLen);
  if (outData != nullptr) {
    ss_free_data(outData);
  }
  logDebug("rust.evaluateBatch(): complete");
  return result;
#else
  (void)flatData;
  (void)windowSize;
  logDebug("rust.evaluateBatch(): SS_USE_RUST disabled, using C++ fallback");
  return {};
#endif
}

void SignalScorerRustBridge::reset(const std::string& modelName) {
#if defined(SS_USE_RUST)
  logDebug("rust.reset(): begin");
  ss_reset(modelName.empty() ? nullptr : modelName.c_str());
  logDebug("rust.reset(): complete");
#else
  (void)modelName;
  logDebug("rust.reset(): SS_USE_RUST disabled");
#endif
}

} // namespace margelo::nitro::signalscorer
