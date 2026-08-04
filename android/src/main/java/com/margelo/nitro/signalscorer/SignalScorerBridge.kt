// Adapted from react-native-random-forest (https://github.com/tony-div/react-native-random-forest)
// by Tony George. See README "Attribution" section.
package com.margelo.nitro.signalscorer

import android.util.Log
import com.facebook.proguard.annotations.DoNotStrip

@DoNotStrip
object SignalScorerBridge {
    private const val TAG = "NitroSignalScorer"

    init {
        System.loadLibrary("NitroSignalScorer")
    }

    @JvmStatic
    @DoNotStrip
    fun evaluate(values: DoubleArray): DoubleArray? {
        return nativeEvaluate(values)
    }

    @JvmStatic
    @DoNotStrip
    fun evaluateBatch(flatData: DoubleArray, windowSize: Int): DoubleArray? {
        return nativeEvaluateBatch(flatData, windowSize)
    }

    @JvmStatic
    @DoNotStrip
    fun reset() {
        nativeReset()
    }

    private external fun nativeEvaluate(values: DoubleArray): DoubleArray?
    private external fun nativeEvaluateBatch(flatData: DoubleArray, windowSize: Int): DoubleArray?
    private external fun nativeReset()
}
