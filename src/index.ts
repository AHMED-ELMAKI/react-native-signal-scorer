import {
  type SignalPrediction,
  decodePrediction,
  signalScorer as nativeSignalScorer,
} from './specs/SignalScorer.nitro'
import {
  encodePrediction,
  scoreSignalBatch,
  scoreSignalValues,
} from './specs/fallback'

export type {
  SignalPrediction,
  SignalScorer,
  PredictionCode,
} from './specs/SignalScorer.nitro'
export { PREDICTION_CODES, PREDICTION_LABELS, decodePrediction } from './specs/SignalScorer.nitro'

/**
 * Public API surface for the signal scorer.
 *
 * `evaluate`/`score` return a structured {@link SignalPrediction} — matching the
 * reference behavior exactly. Internally the native layer works on flat arrays,
 * and a pure-TS fallback guarantees identical results in any environment.
 */
export interface PublicSignalScorer {
  evaluate(values: number[]): SignalPrediction
  score(values: number[]): SignalPrediction
  evaluateBatch(values: number[], windowSize: number): SignalPrediction[]
  reset(modelName?: string): void
}

const native = nativeSignalScorer

function evaluateObject(values: number[]): SignalPrediction {
  if (native) {
    try {
      return decodePrediction(native.evaluate(values))
    } catch {
      // fall through to TS fallback
    }
  }
  return scoreSignalValues(values)
}

function evaluateBatchObject(values: number[], windowSize: number): SignalPrediction[] {
  if (native) {
    try {
      const flat = native.evaluateBatch(values, windowSize)
      const results: SignalPrediction[] = []
      for (let i = 0; i < flat.length; i += 6) {
        results.push(decodePrediction(flat.slice(i, i + 6)))
      }
      return results
    } catch {
      // fall through to TS fallback
    }
  }
  return scoreSignalBatch(values, windowSize)
}

export const signalScorer: PublicSignalScorer = {
  evaluate: evaluateObject,
  score: evaluateObject,
  evaluateBatch: evaluateBatchObject,
  reset(modelName?: string): void {
    if (native) {
      try {
        native.reset(modelName)
      } catch {
        // ignore
      }
    }
  },
}

export function scoreSignal(values: number[]): SignalPrediction {
  return evaluateObject(values)
}

export function evaluateBatch(values: number[], windowSize: number): SignalPrediction[] {
  return evaluateBatchObject(values, windowSize)
}

// Access to the raw flat-array encoding, useful for advanced/profiling use.
export { encodePrediction } from './specs/fallback'
export { scoreSignalValues as scoreSignalValuesTS } from './specs/fallback'
