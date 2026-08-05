import { type HybridObject, NitroModules } from 'react-native-nitro-modules'

/**
 * A scored snapshot produced by a streaming scorer.
 *
 * `hue` is a numeric label for the signal's current character:
 *   0 = flat, 1 = rising, 2 = wild
 */
export interface TickSignal {
  hue: 'flat' | 'rising' | 'wild'
  confidence: number
  score: number
  velocity: number
  spread: number
  level: number
}

export const HUE_LABELS = ['flat', 'rising', 'wild'] as const
export type HueCode = (typeof HUE_LABELS)[number]

/**
 * Decodes a flat numeric row returned by the native `glimpse` method back
 * into a structured {@link TickSignal}.
 */
export function decodeRow(row: Array<number>): TickSignal {
  const [code, confidence, score, velocity, spread, level] = row
  return {
    hue: HUE_LABELS[code] ?? 'flat',
    confidence,
    score,
    velocity,
    spread,
    level,
  }
}

/**
 * A stateful, streaming signal scorer.
 *
 * Feed samples one at a time with `update`, then read the current scored
 * snapshot with `getSignal`. Each instance tracks its own rolling state.
 */
interface TickScorer
  extends HybridObject<{ ios: 'c++'; android: 'c++' }> {
  /**
   * Feeds a single sample into the engine and advances the rolling state.
   */
  update(sample: number): void

  /**
   * Returns the current scored snapshot as a flat row:
   * [hueCode, confidence, score, velocity, spread, level]
   */
  getSignal(): Array<number>

  /**
   * Clears all observed history, returning the engine to its initial state.
   */
  rewind(): void
}

/**
 * Factory object that creates new, independent streaming scorers.
 */
interface TickScorerFactory
  extends HybridObject<{ ios: 'c++'; android: 'c++' }> {
  /**
   * Creates a new, independent streaming scorer.
   */
  create(): TickScorer
}

// Creating the hybrid object may throw outside of React Native (e.g. in a
// Node/Jest context). In that case the TS fallback in `src/index.ts`
// transparently takes over so behavior is always preserved.
let factory: TickScorerFactory | null = null
try {
  factory = NitroModules.createHybridObject<TickScorerFactory>('SignalTicker')
} catch {
  factory = null
}

export { factory as tickerFactory }
export type { TickScorer, TickScorerFactory }
