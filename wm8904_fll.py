##########################
# nRF clock config
##########################
NRF_I2S_MCK_32MDIV = 15
NRF_I2S_RATIO = 48
n_channels = 2 # stereo
bits_per_sample = 24

mclk = 32_000_000 / NRF_I2S_MCK_32MDIV # master clock frequency
fs = mclk / NRF_I2S_RATIO # sample rate
sclk = fs * n_channels * bits_per_sample # bit clock frequency
print("nRF clock config")
print("    NRF_I2S_MCK_32MDIV_" + str(NRF_I2S_MCK_32MDIV))
print("    NRF_I2S_RATIO_" + str(NRF_I2S_RATIO))
print("    MCLK " + str(mclk) + " Hz")
print("    SCLK " + str(sclk) + " Hz")
print("    fs   " + str(fs) + " Hz")
print("")

##########################
# WM8904 FLL confg
##########################
print("WM8904 FLL config")

sysclk = 256 * fs # 256fs is required if both ADC and DAC is enabled
f_ref = sclk
FLL_RATIO = 1 # recommended for FREF > 1MHz

#
NK = 42.666666666666666
FLL_OUT_DIV = 8 #  4 <= FLL_OUT_DIV <= 64

f_vco = f_ref * NK * FLL_RATIO
f_out = f_vco / FLL_OUT_DIV

print("    sysclk (256fs) " + str(sysclk) + " Hz" )
print("    f_ref          " + str(f_ref) + " Hz")
print("    f_vco          " + str(f_vco) + " Hz")
print("    f_out          " + str(f_out) + " Hz")
print("    N.K            " + str(NK))
print("    FLL_OUT_DIV    " + str(FLL_OUT_DIV))


if sysclk < 3_000_000:
  print("    ❗️ Invalid sysclk")

if f_vco < 90_000_000 or f_vco > 100_000_000:
  print("    ❗️ Invalid fVco")