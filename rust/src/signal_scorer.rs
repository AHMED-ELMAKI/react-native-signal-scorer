//! Pure signal scoring engine.
//!
//! Mirrors the portable C++ algorithm (cpp/ScoringAlgorithm.hpp) so results are
//! identical regardless of which layer is active. The registry keeps independent
//! scoring contexts (named), analogous to the random-forest package's named models.

/// A single scored window.
pub struct ScoredWindow {
    pub code: f64,       // 0=steady, 1=accelerating, 2=volatile
    pub confidence: f64, // [0,1]
    pub score: f64,      // [0,1]
    pub slope: f64,
    pub volatility: f64,
    pub mean: f64,
}

impl ScoredWindow {
    pub fn flatten(&self) -> Vec<f64> {
        vec![
            self.code,
            self.confidence,
            self.score,
            self.slope,
            self.volatility,
            self.mean,
        ]
    }
}

fn clamp(value: f64, min: f64, max: f64) -> f64 {
    if value < min {
        min
    } else if value > max {
        max
    } else {
        value
    }
}

/// Scores a single window of values. Returns the default (all zeros/steady)
/// window for an empty input, matching the C++/TS behavior.
pub fn score_window(values: &[f64]) -> ScoredWindow {
    if values.is_empty() {
        return ScoredWindow {
            code: 0.0,
            confidence: 0.0,
            score: 0.0,
            slope: 0.0,
            volatility: 0.0,
            mean: 0.0,
        };
    }

    let n = values.len() as f64;
    let mean = values.iter().sum::<f64>() / n;
    let first = values[0];
    let last = values[values.len() - 1];
    let slope = (last - first) / (n - 1.0).max(1.0);

    let variance = values
        .iter()
        .map(|v| {
            let d = v - mean;
            d * d
        })
        .sum::<f64>()
        / n;
    let volatility = variance.sqrt();

    // Matches the reference: Math.max(1, |mean| + 1)
    let abs_mean = (mean.abs() + 1.0).max(1.0);
    let normalized_slope = clamp(slope / abs_mean, -1.0, 1.0);
    let normalized_volatility = clamp(volatility / abs_mean, 0.0, 1.0);
    let score = clamp(0.5 + 0.35 * normalized_slope + 0.15 * normalized_volatility, 0.0, 1.0);

    let code = if score > 0.7 {
        1.0 // accelerating
    } else if normalized_volatility > 0.45 {
        2.0 // volatile
    } else {
        0.0 // steady
    };

    let confidence = clamp(
        0.55 + 0.2 * normalized_slope.abs() + 0.25 * normalized_volatility,
        0.0,
        1.0,
    );

    ScoredWindow {
        code,
        confidence,
        score,
        slope,
        volatility,
        mean,
    }
}

/// Scores a signal in sliding windows of `window_size`.
pub fn score_batch(flat_data: &[f64], window_size: usize) -> Vec<f64> {
    if window_size == 0 || flat_data.len() < window_size {
        return Vec::new();
    }
    let mut out = Vec::with_capacity((flat_data.len() - window_size + 1) * 6);
    for w in flat_data.windows(window_size) {
        out.extend(score_window(w).flatten());
    }
    out
}
