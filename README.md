# react-native-signal-scorer

A high-performance React Native Nitro module for real-time time-series signal scoring, powered by a stateful Rust streaming core and a C++ reference fallback.

## What it does

This package provides a stateful, sample-by-sample streaming signal scoring engine. As sample data arrives (e.g. from IMU sensors, touch streams, or metrics), feed samples sequentially into the engine to maintain rolling estimates of:

- **Level**: Exponentially smoothed signal baseline
- **Velocity**: Smoothed rate of change
- **Spread**: Smoothed signal dispersion / error deviation
- **Composite Score**: Normalized heuristic score in `[0, 1]`
- **Confidence**: Estimation confidence in `[0, 1]`
- **Hue**: Trend classification label (`'flat'`, `'rising'`, or `'wild'`)

Each scorer instance tracks its own independent rolling state, making it ideal for tracking multiple concurrent signal channels.

## Architecture

```
TypeScript (spec) -> Nitro HybridObject (C++) -> C++ Rust Bridge (FFI) -> Rust TickerScorer (stateful core)
```

- **`src/`** – TypeScript entrypoints, Nitro spec interface, and pure-TS `TickEngine` fallback.
- **`cpp/`** – Nitro `HybridObject` C++ factory & instances, plus `TickerPulse.hpp` C++ reference engine.
- **`cpp/rust/`** – C++ FFI bridge wrapping the handle-based Rust API (`tick_create`, `tick_ingest`, `tick_glimpse`, `tick_rewind`, `tick_destroy`).
- **`rust/`** – Pure Rust `TickerScorer` engine compiled via `cargo-ndk` directly into native static libraries (`libsignal_ticker_rust.a`).
- **`android/`** – Gradle & CMake build integration configured for source compilation via `cargo-ndk`.
- **`nitrogen/`** – Nitro generated JSI bindings.

## Getting Started

```bash
npm install
npm run typecheck
```

To (re)generate Nitro glue bindings:

```bash
npm run specs
```

## API Usage

### Streaming Scorer (`createTicker`)

```ts
import { createTicker } from 'react-native-signal-scorer'

// Create an independent streaming scorer
const ticker = createTicker()

// Feed samples sequentially as they arrive
ticker.update(0.2)
ticker.update(0.5)
ticker.update(0.9)

// Inspect current rolling snapshot without disrupting state
const snapshot = ticker.getSignal()
// {
//   hue: 'rising',
//   confidence: 0.78,
//   score: 0.82,
//   velocity: 0.35,
//   spread: 0.08,
//   level: 0.53
// }

// Rewind / clear state back to pristine initial state
ticker.rewind()
```

### Batch Helper (`scoreSamples`)

```ts
import { scoreSamples } from 'react-native-signal-scorer'

// Scores an entire array of samples by feeding them into a fresh streaming ticker
const finalSnapshot = scoreSamples([0.1, 0.3, 0.6, 1.0, 1.4])
```

## Build System

The Android build uses `cargo-ndk` directly to build Rust binaries for all supported ABIs (`arm64-v8a`, `armeabi-v7a`, `x86`, `x86_64`) during standard Gradle compilation, ensuring pure source-built native libraries without relying on prebuilt binary artifacts or Kotlin/JNI wrapper bridges.

