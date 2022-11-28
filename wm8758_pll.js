const channelCount = 2
const bitDepth = 16
const NRF_I2S_MCK_32MDIV = 8
const NRF_I2S_RATIO = 96

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

// MCLKDIV
/*
000 = divide by 1
001 = divide by 1.5
010 = divide by 2
011 = divide by 3
100 = divide by 4
101 = divide by 6
110 = divide by 8
111 = divide by 12 */

// 5 < PLLN < 13

MCLKDIV = 1
SYSCLK = 256 * fs
pllFrac = SYSCLK * MCLKDIV * 4 / masterClock
PLLN = Math.floor(pllFrac)
PLLK = Math.round(Math.pow(2, 24) * (pllFrac - PLLN))
f2 = pllFrac * masterClock
fPllOut = f2 / 4 // fixed divide by 4
SYSCLK_REF = fPllOut / MCLKDIV
const pllkBitString = (PLLK >>> 0).toString(2)
const pllkParts = [pllkBitString.substring(0, 8), pllkBitString.substring(8, 16), pllkBitString.substring(16, 24)]
console.log({ f2, PLLN, PLLK: pllkParts, SYSCLK, SYSCLK_REF, pllFrac, MCLKDIV })

