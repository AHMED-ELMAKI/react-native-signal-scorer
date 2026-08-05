import { createTicker, scoreSamples } from './src'

const ticker = createTicker()
for (const sample of [0.2, 0.5, 0.7, 1.1, 1.3]) {
  ticker.update(sample)
}
console.log('streaming signal:', ticker.getSignal())

const batchSignal = scoreSamples([0.2, 0.5, 0.7, 1.1, 1.3])
console.log('scoreSamples:', batchSignal)

