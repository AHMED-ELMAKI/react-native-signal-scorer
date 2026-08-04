import { signalScorer, scoreSignal } from './src'

const result = signalScorer.score([0.2, 0.5, 0.7, 1.1, 1.3])
console.log('single:', result)

const batch = signalScorer.evaluateBatch([0.2, 0.5, 0.7, 1.1, 1.3], 3)
console.log('batch:', batch)

const viaFn = scoreSignal([1.0, 1.0, 1.0, 1.0])
console.log('scoreSignal:', viaFn)
