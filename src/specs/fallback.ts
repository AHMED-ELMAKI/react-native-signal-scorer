import {
  HUE_LABELS,
  type HueCode,
  type TickSignal,
} from './SignalScorer.nitro'

/**
 * Pure TypeScript implementation of the stateful streaming scorer.
 *
 * Mirrors the exact EMA math used by the Rust core so results are identical
 * whether the native layer or this fallback is active.
 */
export function squeeze(value: number, lo: number, hi: number): number {
  return Math.min(hi, Math.max(lo, value))
}

/**
 * A stateful, per-stream scoring engine implemented in TypeScript.
 */
export class TickEngine {
  private levelAlpha = 0.25
  private velocityAlpha = 0.5
  private spreadAlpha = 0.2
  private samplesSeen = 0
  private latestLevel = 0
  private priorLevel = 0
  private latestVelocity = 0
  private latestSpread = 0

  /** Feeds a single sample into the engine and advances the rolling state. */
  ingest(sample: number): void {
    if (this.samplesSeen === 0) {
      this.latestLevel = sample
      this.priorLevel = sample
      this.latestVelocity = 0
      this.latestSpread = 0
      this.samplesSeen = 1
      return
    }

    this.priorLevel = this.latestLevel
    this.latestLevel =
      this.levelAlpha * sample + (1 - this.levelAlpha) * this.latestLevel

    const delta = this.latestLevel - this.priorLevel
    this.latestVelocity =
      this.velocityAlpha * delta + (1 - this.velocityAlpha) * this.latestVelocity

    const error = sample - this.latestLevel
    this.latestSpread =
      this.spreadAlpha * Math.abs(error) + (1 - this.spreadAlpha) * this.latestSpread

    this.samplesSeen += 1
  }

  /** Returns the current scored snapshot without disrupting state. */
  glance(): TickSignal {
    if (this.samplesSeen === 0) {
      return {
        hue: 'flat',
        confidence: 0,
        score: 0,
        velocity: 0,
        spread: 0,
        level: 0,
      }
    }

    const scale = Math.max(1, Math.abs(this.latestLevel) + 1)
    const normVelocity = squeeze(this.latestVelocity / scale, -1, 1)
    const normSpread = squeeze(this.latestSpread / scale, 0, 1)

    const score = squeeze(0.5 + 0.4 * normVelocity - 0.2 * normSpread, 0, 1)

    let hue: HueCode = 'flat'
    const code = normSpread > 0.55 ? 2 : score > 0.62 ? 1 : 0
    hue = HUE_LABELS[code] ?? 'flat'

    const confidence = squeeze(
      0.5 + 0.3 * Math.abs(normVelocity) + 0.2 * (1 - normSpread),
      0,
      1,
    )

    return {
      hue,
      confidence,
      score,
      velocity: this.latestVelocity,
      spread: this.latestSpread,
      level: this.latestLevel,
    }
  }

  /** Clears all observed history, returning the engine to its initial state. */
  rewind(): void {
    this.samplesSeen = 0
    this.latestLevel = 0
    this.priorLevel = 0
    this.latestVelocity = 0
    this.latestSpread = 0
  }
}

/** Encodes a {@link TickSignal} into a flat 6-element row. */
export function encodeRow(t: TickSignal): number[] {
  return [
    HUE_LABELS.indexOf(t.hue),
    t.confidence,
    t.score,
    t.velocity,
    t.spread,
    t.level,
  ]
}
