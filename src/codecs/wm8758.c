#include <zephyr/drivers/i2c.h>
#include "wm8758.h"

#ifdef CONFIG_SOC_SERIES_NRF53X
#define I2C_NODE DT_NODELABEL(i2c1)
#else
#define I2C_NODE DT_NODELABEL(i2c0)
#endif

static const struct device *i2c_dev = DEVICE_DT_GET(I2C_NODE);

struct register_write
{
    uint16_t addr;
    uint16_t data;
};

int wm8758_write_reg(uint16_t reg_addr, uint16_t data)
{
    __ASSERT(reg_addr <= 0x7f, "invalid codec register address"); // 7 bit register address
    __ASSERT(reg_addr <= 0x1ff, "invalid codec register data");   // 9 bit register data
    const uint16_t reg_and_address = (reg_addr << 9) | data;

    uint8_t payload[2] = {
        (reg_and_address & 0xff) >> 8, // msb
        (reg_and_address >> 0) & 0xff  // lsb
    };
    int write_rc = i2c_write(i2c_dev, payload, 2, WM8758B_ADDRESS);
    printk("write_reg result %d\n", write_rc);
    return write_rc;
}

void init_wm8758()
{
    if (device_is_ready(i2c_dev))
    {
        // Reset command
        wm8758_write_reg(WM8758B_SW_RESET, 0);
        // biascut
        wm8758_write_reg(WM8758B_BIAS_CTRL, (1 << 8));

        // Set
        // PLL enable (1 << 5)
        // BIASEN "The analogue amplifiers will not operate unless BIASEN is enabled." p 76
        // The VMIDSEL and BIASEN bits must be set to enable analogue output midrail voltage and for normal DAC operation. p 24
        // (1 << 0) sets vmidsel to 01 as in the recommended startup sequence
        const int vmidsel = 1 << 0;
        const int bufioen = 1 << 2;
        const int biasen = 1 << 3;
        const int pllen = 1 << 5;
        wm8758_write_reg(WM8758B_POWER_MANAGEMENT_1, pllen | biasen | bufioen | vmidsel);

        // Enable OUT1 left/right
        const int adc_enable = (1 << 1) | (1 << 0);
        const int input_pga_enable = (1 << 3) | (1 << 2);
        const int boost_enable = (1 << 5) | (1 << 4);
        const int out1_enable = (1 << 8) | (1 << 7);
        wm8758_write_reg(WM8758B_POWER_MANAGEMENT_2, out1_enable | boost_enable | input_pga_enable | adc_enable);

        // Enable DAC left/right and left/right mixer
        const int dac_enable = (1 << 1) | (1 << 0);
        const int mixer_enable = (1 << 3) | (1 << 2);
        wm8758_write_reg(WM8758B_POWER_MANAGEMENT_3, mixer_enable | dac_enable);

        // 16 bit i2s format
        wm8758_write_reg(WM8758B_AUDIO_INTERFACE, 0x10);

        // companding
        // write_reg(i2c_num, 5, 0x0);

        // CLKSEL (use PLL)
        wm8758_write_reg(WM8758B_CLOCK_GEN_CTRL, 0b100000000); // 0x140 should be the default value

        // TODO: The following PLL values
        // are based on an actual sample rate of 41666.7
        // PLL N (10, no pre scale)
        wm8758_write_reg(WM8758B_PLL_N, 0x0b);
        // PLL K (0.66666) 1, 2, 3
        wm8758_write_reg(WM8758B_PLL_K_1, 0b10101010);
        wm8758_write_reg(WM8758B_PLL_K_2, 0b10101010);
        wm8758_write_reg(WM8758B_PLL_K_3, 0b10101011);

        // Set l/r dac volume
        // wm8758_write_reg(WM8758B_LEFT_DAC_DIGITAL_VOL, 0x1ff);
        // wm8758_write_reg(WM8758B_RIGHT_DAC_DIGITAL_VOL, 0x1ff);

        // ALC
        // write_reg(i2c_num, WM8758B_ALC_CTRL_1, 0x0);
        // write_reg(i2c_num, WM8758B_ALC_CTRL_2, 0x0);
        // write_reg(i2c_num, WM8758B_ALC_CTRL_3, 0x0);

        // mic bypass path (for debugging mic input)
        // write_reg(i2c_num, WM8758B_BEEP_CTRL, (1 << 8) | (1 << 7));

        // l/r adc digital volume
        wm8758_write_reg(WM8758B_LEFT_ADC_DIGITAL_VOL, 0x1ff);
        wm8758_write_reg(WM8758B_RIGHT_ADC_DIGITAL_VOL, 0x1ff);

        // input PGA volume
        // write_reg(i2c_num, WM8758B_INPUT_CTRL, 0x22); // loud metallic static if 0
        // write_reg(i2c_num, WM8758B_LEFT_INP_PGA_GAIN_CTRL, 0x11f); // gain adjustment audible in the bypass path too
        // write_reg(i2c_num, WM8758B_RIGHT_INP_PGA_GAIN_CTRL, 0x11f);
        wm8758_write_reg(WM8758B_INPUT_CTRL, 0x12); // loud metallic static if 0
        wm8758_write_reg(WM8758B_LEFT_INP_PGA_GAIN_CTRL, 0x3f);
        wm8758_write_reg(WM8758B_RIGHT_INP_PGA_GAIN_CTRL, 0x3f);

        // input boost ctrl. does not seem audible in the bypass path
        const int pga_boost_enable = 1;
        wm8758_write_reg(WM8758B_LEFT_ADC_BOOST_CTRL, (pga_boost_enable << 8));
        wm8758_write_reg(WM8758B_RIGHT_ADC_BOOST_CTRL, (pga_boost_enable << 8));

        // LOUT1VOL/ROUT1VOL
        wm8758_write_reg(WM8758B_LOUT1, (1 << 8) | 0x39);
        wm8758_write_reg(WM8758B_ROUT1, (1 << 8) | 0x39);

        // mic bypass path (for debugging mic input)
        // wm8758_write_reg(WM8758B_BEEP_CTRL, (1 << 8) | (1 << 7));
    }
    else
    {
        // TODO: report error
    }
}
