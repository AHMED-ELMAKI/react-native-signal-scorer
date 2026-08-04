import { type HybridObject, NitroModules } from 'react-native-nitro-modules'

/**
 * A single scored signal window.
 *
 * `prediction` is encoded as a numeric code so it can cross the native boundary:
 *   0 = 'steady', 1 = 'accelerating', 2 = 'volatile'
 */
export interface SignalPrediction {
  prediction: 'steady' | 'accelerating' | 'volatile'
  confidence: number
  score: number
  slope: number
  volatility: number
  mean: number
}

export const PREDICTION_CODES = {
  steady: 0,
  accelerating: 1,
  volatile: 2,
} as const

export const PREDICTION_LABELS = ['steady', 'accelerating', 'volatile'] as const

export type PredictionCode = (typeof PREDICTION_CODES)[keyof typeof PREDICTION_CODES]

/**
 * Decodes a flat numeric array returned by the native `evaluate` method
 * back into a structured {@link SignalPrediction}.
 */
export function decodePrediction(flat: Array<number>): SignalPrediction {
  const [code, confidence, score, slope, volatility, mean] = flat
  return {
    prediction: PREDICTION_LABELS[code] ?? 'steady',
    confidence,
    score,
    slope,
    volatility,
    mean,
  }
}

interface SignalScorer
  extends HybridObject<{ ios: 'c++'; android: 'c++' }> {
  /**
   * Evaluates a single window of signal values.
   * Returns a flat array: [predictionCode, confidence, score, slope, volatility, mean]
   */
  evaluate(values: Array<number>): Array<number>

  /**
   * Evaluates a signal in sliding windows of `windowSize`.
   * Returns a flat array of per-window results, each encoded as by `evaluate`.
   */
  evaluateBatch(flatData: Array<number>, windowSize: number): Array<number>

  /**
   * Resets any internal named registry / state.
   */
  reset(modelName?: string): void
}

// Creating the hybrid object may throw when running outside of React Native
// (e.g. in a Node/Jest context). In that case the TS fallback in `src/index.ts`
// transparently takes over so behavior is always preserved.
let signalScorer: SignalScorer | null = null
try {
  signalScorer =
    NitroModules.createHybridObject<SignalScorer>('SignalScorer')
} catch {
  signalScorer = null
}

export { signalScorer }
export type { SignalScorer }
