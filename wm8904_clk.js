// nrf clock config
const channelCount = 2
const bitDepth = 24
const NRF_I2S_MCK_32MDIV = 15
const NRF_I2S_RATIO = 48

const masterClock = 32_000_000 / NRF_I2S_MCK_32MDIV
const masterClockDiv = NRF_I2S_RATIO
const fs = masterClock / masterClockDiv
const bitClock = fs * channelCount * bitDepth

console.log("nrf clock config:")
console.log({
  NRF_I2S_MCK_32MDIV,
  NRF_I2S_RATIO,
  channelCount,
  bitDepth,
  fs,
  bitClock,
  masterClock
})
console.log()

// wm8904 clock config
// Use PLL to derive SYSCLK from bit clock.
// SYSCLK = 256fs (required if ADC is enabled)
const sysclk = 256 * fs
const fRef = bitClock
// F_out = F_vco / FLL_OUT_DIV
// F_vco = F_ref * N.K * FLL_RATIO
// 90MHz <= F_vco <= 100MHz

const FLL_RATIO = 1 // 000 (i.e 1) recommended for FREF > 1MHz
const NK = 42.6666666666666
const FLL_OUT_DIV = 8 // 4,5,6...64
const fVco = fRef * NK * FLL_RATIO
const fOut = fVco / FLL_OUT_DIV

// sysclk = fRef * NK / FLL_OUT_DIV, 90MHz <= fRef * NK <= 100MHz

console.log("wm8904 clock config:")
console.log({
  sysclk,
  FLL_OUT_DIV,
  NK,
  fVco,
  fOut
})

if (fVco < 90_000_000 || fVco > 100_000_000) {
  console.error("❗️ Invalid fVco")
}