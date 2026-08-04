import {
  PREDICTION_CODES,
  type SignalPrediction,
} from './SignalScorer.nitro'

/**
 * Pure TypeScript implementation of the signal scoring algorithm.
 *
 * This mirrors the exact math used by the C++ fallback engine and the Rust
 * core so that results are identical regardless of which layer is active.
 */
export function clamp(value: number, min: number, max: number): number {
  return Math.min(max, Math.max(min, value))
}

export function scoreSignalValues(values: number[]): SignalPrediction {
  if (values.length === 0) {
    return {
      prediction: 'steady',
      confidence: 0,
      score: 0,
      slope: 0,
      volatility: 0,
      mean: 0,
    }
  }

  const mean = values.reduce((sum, value) => sum + value, 0) / values.length
  const first = values[0]
  const last = values[values.length - 1]
  const slope = (last - first) / Math.max(1, values.length - 1)
  const deviations = values.map((value) => value - mean)
  const variance =
    deviations.reduce((sum, value) => sum + value * value, 0) / values.length
  const volatility = Math.sqrt(variance)

  const normalizedSlope = clamp(slope / Math.max(1, Math.abs(mean) + 1), -1, 1)
  const normalizedVolatility = clamp(
    volatility / Math.max(1, Math.abs(mean) + 1),
    0,
    1,
  )
  const score = clamp(
    0.5 + 0.35 * normalizedSlope + 0.15 * normalizedVolatility,
    0,
    1,
  )

  let prediction: SignalPrediction['prediction'] = 'steady'
  if (score > 0.7) {
    prediction = 'accelerating'
  } else if (normalizedVolatility > 0.45) {
    prediction = 'volatile'
  }

  const confidence = clamp(
    0.55 + 0.2 * Math.abs(normalizedSlope) + 0.25 * normalizedVolatility,
    0,
    1,
  )

  return {
    prediction,
    confidence,
    score,
    slope,
    volatility,
    mean,
  }
}

export function scoreSignalBatch(flatData: number[], windowSize: number): SignalPrediction[] {
  if (windowSize <= 0) return []
  const results: SignalPrediction[] = []
  for (let i = 0; i + windowSize <= flatData.length; i++) {
    results.push(scoreSignalValues(flatData.slice(i, i + windowSize)))
  }
  return results
}

export function encodePrediction(p: SignalPrediction): number[] {
  return [
    PREDICTION_CODES[p.prediction],
    p.confidence,
    p.score,
    p.slope,
    p.volatility,
    p.mean,
  ]
}
