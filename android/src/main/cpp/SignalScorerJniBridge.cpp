// Adapted from react-native-random-forest (https://github.com/tony-div/react-native-random-forest)
// by Tony George. See README "Attribution" section.
#include <jni.h>
#include <android/log.h>
#include <cstddef>
#include <cstdio>

#include "../../../cpp/ScoringAlgorithm.hpp"

#if defined(SS_USE_RUST)
extern "C" {
double* ss_evaluate(const double* data, int len, int* out_len);
double* ss_evaluate_batch(const double* data, int len, int window_size, int* out_len);
void ss_reset(const char* registry_name);
void ss_free_data(double* ptr);
}
#endif

namespace {

constexpr const char* kLogTag = "NitroSignalScorer";

void logDebug(const char* message) {
  __android_log_print(ANDROID_LOG_DEBUG, kLogTag, "%s", message);
}

} // namespace

extern "C" {

JNIEXPORT jdoubleArray JNICALL
Java_com_margelo_nitro_signalscorer_SignalScorerBridge_nativeEvaluate(
    JNIEnv* env, jclass /*clazz*/, jdoubleArray values) {
#if defined(SS_USE_RUST)
  jsize len = env->GetArrayLength(values);
  if (len == 0) return nullptr;

  jdouble* data = env->GetDoubleArrayElements(values, nullptr);
  if (data == nullptr) return nullptr;

  int outLen = 0;
  double* outData = ss_evaluate(data, len, &outLen);

  env->ReleaseDoubleArrayElements(values, data, JNI_ABORT);

  if (outData == nullptr || outLen <= 0) return nullptr;

  jdoubleArray result = env->NewDoubleArray(outLen);
  if (result != nullptr) {
    env->SetDoubleArrayRegion(result, 0, outLen, outData);
  }
  ss_free_data(outData);
  return result;
#else
  (void)env; (void)values;
  logDebug("nativeEvaluate(): SS_USE_RUST disabled, using C++ fallback");
  jsize len = env->GetArrayLength(values);
  if (len == 0) return nullptr;
  jdouble* data = env->GetDoubleArrayElements(values, nullptr);
  if (data == nullptr) return nullptr;
  std::vector<double> vec(data, data + len);
  env->ReleaseDoubleArrayElements(values, data, JNI_ABORT);
  auto flat = margelo::nitro::signalscorer::computeScoreFlat(vec);
  jsize outLen = static_cast<jsize>(flat.size());
  jdoubleArray result = env->NewDoubleArray(outLen);
  if (result != nullptr) {
    env->SetDoubleArrayRegion(result, 0, outLen, flat.data());
  }
  return result;
#endif
}

JNIEXPORT jdoubleArray JNICALL
Java_com_margelo_nitro_signalscorer_SignalScorerBridge_nativeEvaluateBatch(
    JNIEnv* env, jclass /*clazz*/, jdoubleArray flatData, jint windowSize) {
#if defined(SS_USE_RUST)
  jsize len = env->GetArrayLength(flatData);
  if (len == 0 || windowSize <= 0) return nullptr;

  jdouble* data = env->GetDoubleArrayElements(flatData, nullptr);
  if (data == nullptr) return nullptr;

  int outLen = 0;
  double* outData = ss_evaluate_batch(data, len, windowSize, &outLen);

  env->ReleaseDoubleArrayElements(flatData, data, JNI_ABORT);

  if (outData == nullptr || outLen <= 0) return nullptr;

  jdoubleArray result = env->NewDoubleArray(outLen);
  if (result != nullptr) {
    env->SetDoubleArrayRegion(result, 0, outLen, outData);
  }
  ss_free_data(outData);
  return result;
#else
  (void)env; (void)flatData; (void)windowSize;
  logDebug("nativeEvaluateBatch(): SS_USE_RUST disabled, using C++ fallback");
  jsize len = env->GetArrayLength(flatData);
  if (len == 0 || windowSize <= 0) return nullptr;
  jdouble* data = env->GetDoubleArrayElements(flatData, nullptr);
  if (data == nullptr) return nullptr;
  std::vector<double> vec(data, data + len);
  env->ReleaseDoubleArrayElements(flatData, data, JNI_ABORT);
  auto flat = margelo::nitro::signalscorer::computeScoreBatchFlat(vec, windowSize);
  jsize outLen = static_cast<jsize>(flat.size());
  jdoubleArray result = env->NewDoubleArray(outLen);
  if (result != nullptr) {
    env->SetDoubleArrayRegion(result, 0, outLen, flat.data());
  }
  return result;
#endif
}

JNIEXPORT void JNICALL
Java_com_margelo_nitro_signalscorer_SignalScorerBridge_nativeReset(
    JNIEnv* env, jclass /*clazz*/) {
#if defined(SS_USE_RUST)
  ss_reset(nullptr);
#else
  (void)env;
  logDebug("nativeReset(): SS_USE_RUST disabled");
#endif
}

} // extern "C"
