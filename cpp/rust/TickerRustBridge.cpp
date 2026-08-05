#include "TickerRustBridge.hpp"

#include <cstddef>
#include <cstdio>
#include <array>

#if defined(__ANDROID__)
#include <android/log.h>
#endif

// Rust FFI exports (see rust/src/lib.rs). The Rust core is required and is
// the sole scoring engine.
extern "C" {
void* tick_create();
void tick_destroy(void* handle);
void tick_ingest(void* handle, double sample);
int tick_glimpse(void* handle, double* out);
void tick_rewind(void* handle);
}

namespace margelo::nitro::signalscorer {

namespace {

constexpr const char* kLogTag = "SignalTicker";

void logDebug(const char* message) {
#if defined(__ANDROID__)
  __android_log_print(ANDROID_LOG_DEBUG, kLogTag, "%s", message);
#else
  std::fprintf(stderr, "[%s] %s\n", kLogTag, message);
#endif
}

constexpr std::size_t kRowSize = 6;

} // namespace

TickerRustBridge::TickerRustBridge() : handle_(tick_create()) {
  logDebug("TickerRustBridge: created");
}

TickerRustBridge::~TickerRustBridge() {
  if (handle_ != nullptr) {
    tick_destroy(handle_);
    handle_ = nullptr;
  }
}

void TickerRustBridge::update(double sample) {
  if (handle_ != nullptr) {
    tick_ingest(handle_, sample);
  }
}

std::vector<double> TickerRustBridge::getSignal() {
  std::vector<double> row(kRowSize, 0.0);
  if (handle_ != nullptr) {
    tick_glimpse(handle_, row.data());
  }
  return row;
}

void TickerRustBridge::rewind() {
  if (handle_ != nullptr) {
    tick_rewind(handle_);
  }
}

} // namespace margelo::nitro::signalscorer
