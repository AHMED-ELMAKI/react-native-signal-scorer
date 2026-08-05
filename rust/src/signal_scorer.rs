//! Stateful streaming signal scorer.
//!
//! This module implements a live, incremental scoring engine. Unlike a
//! batch-only design, it ingests samples one at a time and maintains a rolling
//! estimate of the signal's level, velocity, and dispersion. A lightweight
//! exponential moving average (EMA) keeps the state bounded in memory while
//! still reacting to recent behavior. Each instance is fully independent, so
//! callers can track many streams concurrently.

/// A single scored snapshot produced by [`TickerScorer::glimpse`].
#[derive(Debug, Clone, Copy)]
pub struct TickSnapshot {
    /// Trend label code: 0 = flat, 1 = rising, 2 = wild.
    pub hue: f64,
    /// Confidence in [`0.0, 1.0`].
    pub confidence: f64,
    /// Composite score in [`0.0, 1.0`].
    pub score: f64,
    /// Smoothed per-sample velocity.
    pub velocity: f64,
    /// Smoothed dispersion (standard deviation-like).
    pub spread: f64,
    /// Smoothed level (mean-like).
    pub level: f64,
}

impl TickSnapshot {
    /// Flattens the snapshot into a fixed 6-element row for FFI crossing.
    pub fn to_row(&self) -> [f64; 6] {
        [
            self.hue,
            self.confidence,
            self.score,
            self.velocity,
            self.spread,
            self.level,
        ]
    }
}

/// Lightweight clipping helper.
#[inline]
fn squeeze(value: f64, lo: f64, hi: f64) -> f64 {
    if value < lo {
        lo
    } else if value > hi {
        hi
    } else {
        value
    }
}

/// A stateful, per-stream scoring engine.
#[derive(Debug)]
pub struct TickerScorer {
    /// Smoothing factor for the level estimate.
    level_alpha: f64,
    /// Smoothing factor for the velocity estimate.
    velocity_alpha: f64,
    /// Smoothing factor for the dispersion estimate.
    spread_alpha: f64,
    /// Number of samples seen so far.
    samples_seen: u64,
    /// Current smoothed level.
    latest_level: f64,
    /// Previous smoothed level (for velocity).
    prior_level: f64,
    /// Current smoothed velocity.
    latest_velocity: f64,
    /// Current smoothed dispersion.
    latest_spread: f64,
}

impl Default for TickerScorer {
    fn default() -> Self {
        Self::new()
    }
}

impl TickerScorer {
    /// Creates a fresh scorer with no observed history.
    pub fn new() -> Self {
        Self {
            level_alpha: 0.25,
            velocity_alpha: 0.5,
            spread_alpha: 0.2,
            samples_seen: 0,
            latest_level: 0.0,
            prior_level: 0.0,
            latest_velocity: 0.0,
            latest_spread: 0.0,
        }
    }

    /// Feeds a single sample into the engine and advances the rolling state.
    pub fn ingest(&mut self, sample: f64) {
        if self.samples_seen == 0 {
            // Seed the filters on the very first sample.
            self.latest_level = sample;
            self.prior_level = sample;
            self.latest_velocity = 0.0;
            self.latest_spread = 0.0;
            self.samples_seen = 1;
            return;
        }

        let level_alpha = self.level_alpha;
        let velocity_alpha = self.velocity_alpha;
        let spread_alpha = self.spread_alpha;

        // Update the level estimate.
        self.prior_level = self.latest_level;
        self.latest_level = level_alpha * sample + (1.0 - level_alpha) * self.latest_level;

        // Update the velocity estimate from the level delta.
        let delta = self.latest_level - self.prior_level;
        self.latest_velocity =
            velocity_alpha * delta + (1.0 - velocity_alpha) * self.latest_velocity;

        // Update the dispersion estimate from the recent error.
        let error = sample - self.latest_level;
        self.latest_spread = spread_alpha * error.abs() + (1.0 - spread_alpha) * self.latest_spread;

        self.samples_seen += 1;
    }

    /// Returns the current scored snapshot without disrupting state.
    pub fn glimpse(&self) -> TickSnapshot {
        if self.samples_seen == 0 {
            return TickSnapshot {
                hue: 0.0,
                confidence: 0.0,
                score: 0.0,
                velocity: 0.0,
                spread: 0.0,
                level: 0.0,
            };
        }

        // Normalize velocity/dispersion against the current level magnitude.
        let scale = (self.latest_level.abs() + 1.0).max(1.0);
        let norm_velocity = squeeze(self.latest_velocity / scale, -1.0, 1.0);
        let norm_spread = squeeze(self.latest_spread / scale, 0.0, 1.0);

        // Composite score: favors upward momentum, penalizes chaotic spread.
        let score = squeeze(
            0.5 + 0.4 * norm_velocity - 0.2 * norm_spread,
            0.0,
            1.0,
        );

        // Label: 0 = flat, 1 = rising, 2 = wild.
        let hue = if norm_spread > 0.55 {
            2.0
        } else if score > 0.62 {
            1.0
        } else {
            0.0
        };

        let confidence = squeeze(
            0.5 + 0.3 * norm_velocity.abs() + 0.2 * (1.0 - norm_spread),
            0.0,
            1.0,
        );

        TickSnapshot {
            hue,
            confidence,
            score,
            velocity: self.latest_velocity,
            spread: self.latest_spread,
            level: self.latest_level,
        }
    }

    /// Clears all observed history, returning the engine to its initial state.
    pub fn rewind(&mut self) {
        self.samples_seen = 0;
        self.latest_level = 0.0;
        self.prior_level = 0.0;
        self.latest_velocity = 0.0;
        self.latest_spread = 0.0;
    }
}

#[cfg(test)]
mod tests {
    use super::*;

    #[test]
    fn empty_scorer_returns_zero_snapshot() {
        let scorer = TickerScorer::new();
        let snap = scorer.glimpse();
        assert_eq!(snap.score, 0.0);
        assert_eq!(snap.hue, 0.0);
    }

    #[test]
    fn rising_signal_marks_rising_hue() {
        let mut scorer = TickerScorer::new();
        for i in 0..40 {
            scorer.ingest(i as f64);
        }
        let snap = scorer.glimpse();
        assert!(snap.velocity > 0.0);
    }

    #[test]
    fn rewind_clears_state() {
        let mut scorer = TickerScorer::new();
        scorer.ingest(10.0);
        scorer.ingest(12.0);
        scorer.rewind();
        let snap = scorer.glimpse();
        assert_eq!(snap.score, 0.0);
    }
}
