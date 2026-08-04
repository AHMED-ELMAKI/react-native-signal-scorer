# react-native-signal-scorer

A React Native Nitro module for scoring time-series signals with a Rust-backed core and a C++ fallback engine.

## What it does

This package exposes a small native API for evaluating numeric signal arrays and returning:

- average value (`mean`)
- trend direction (`slope`)
- volatility (standard deviation)
- a composite `score` in `[0, 1]`
- a `confidence` in `[0, 1]`
- a simple label: `steady`, `accelerating`, or `volatile`

It also supports **batch evaluation** over sliding windows, and can run multiple named "registries" to keep independent scoring contexts.

## Why this is different

 It focuses on signal analysis, trend scoring, and time-series heuristics instead of decision-tree inference. The architecture is borrowed from the Nitro module template (TypeScript spec → C++ HybridObject → Rust FFI), but the algorithm and domain are entirely its own.

## Architecture

```
TypeScript (spec) -> C++ HybridObject -> C++ RustBridge (FFI) -> Rust (scoring)
                                        -> C++ fallback engine (when Rust is unavailable)
```

- `src/` – TypeScript entrypoints and API surface (with a pure-TS fallback)
- `cpp/` – C++ HybridObject + portable scoring algorithm
- `cpp/rust/` – C++ FFI bridge to the Rust core (gated by `SS_USE_RUST`)
- `rust/` – Rust scoring engine
- `android/` and `ios/` – native integration folders
- `nitrogen/` – generated glue code

## Getting started

```bash
npm install
npm run typecheck
```

To (re)generate the Nitro glue:

```bash
npm run specs
```

## API

```ts
import { signalScorer, scoreSignal } from 'react-native-signal-scorer'

const result = signalScorer.score([0.2, 0.5, 0.7, 1.1, 1.3])
// {
//   prediction: 'accelerating',
//   confidence: 0.8,
//   score: 0.85,
//   slope: 0.275,
//   volatility: 0.4,
//   mean: 0.76,
// }

const batch = signalScorer.evaluateBatch([0.2, 0.5, 0.7, 1.1, 1.3], 3)
// Array<SignalPrediction> for windows of size 3
```

## Next steps

- wire the Rust layer into the build (already scaffolded via `build-rust-android.sh`)
- add platform-specific native glue (already scaffolded)
- expose a React hook for use in apps
