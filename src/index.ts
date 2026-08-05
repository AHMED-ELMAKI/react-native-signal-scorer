import {
  decodeRow,
  tickerFactory as nativeFactory,
  type TickScorer,
  type TickSignal,
} from './specs/SignalScorer.nitro'
import { TickEngine, encodeRow } from './specs/fallback'

export type {
  TickSignal,
  TickScorer,
  HueCode,
} from './specs/SignalScorer.nitro'
export { HUE_LABELS, decodeRow } from './specs/SignalScorer.nitro'
export { encodeRow, squeeze } from './specs/fallback'

/**
 * A created streaming scorer that dispatches to the native engine when
 * available, otherwise falls back to the pure-TS {@link TickEngine}.
 */
export interface PublicTicker {
  update(sample: number): void
  getSignal(): TickSignal
  rewind(): void
}

/**
 * Creates a new, independent streaming scorer.
 *
 * Uses the native Nitro HybridObject when present; otherwise falls back to a
 * pure-TypeScript engine so behavior is preserved in any environment.
 */
export function createTicker(): PublicTicker {
  if (nativeFactory) {
    try {
      const native: TickScorer = nativeFactory.create()
      return {
        update(sample: number): void {
          native.update(sample)
        },
        getSignal(): TickSignal {
          return decodeRow(native.getSignal())
        },
        rewind(): void {
          native.rewind()
        },
      }
    } catch {
      // fall through to TS fallback
    }
  }

  const engine = new TickEngine()
  return {
    update(sample: number): void {
      engine.ingest(sample)
    },
    getSignal(): TickSignal {
      return engine.glance()
    },
    rewind(): void {
      engine.rewind()
    },
  }
}

/**
 * Convenience helper: scores a finite array of samples by feeding them through
 * a fresh streaming scorer and returning the final snapshot.
 */
export function scoreSamples(samples: number[]): TickSignal {
  const ticker = createTicker()
  for (const s of samples) {
    ticker.update(s)
  }
  return ticker.getSignal()
}

/**
 * Convenience helper: scores samples and returns the snapshot encoding as a
 * flat 6-element row, useful for advanced/profiling use.
 */
export function scoreSamplesRow(samples: number[]): number[] {
  return encodeRow(scoreSamples(samples))
}
