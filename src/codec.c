#include "codec.h"
#include <zephyr/drivers/i2c.h>

#define WM8758B_ADDRESS 0b11010
#define WM8904_ADDRESS  0b110100
#define I2C_NODE DT_NODELABEL(i2c0)
static const struct device* i2c_dev = DEVICE_DT_GET(I2C_NODE);

struct register_write {
  uint16_t addr;
  uint16_t data;
};

// Writes data to a wmXXXX codec register
int write_reg(uint8_t device_addr, uint16_t reg_addr, uint16_t data) {
  __ASSERT(reg_addr <= 0x7f, "invalid codec register address"); // 7 bit register address
  __ASSERT(reg_addr <= 0x1ff, "invalid codec register data"); // 9 bit register data
  const uint16_t reg_and_address = (reg_addr << 9) | data;

  uint8_t payload[2] = {
      (reg_and_address >> 8) & 0xff, // msb
      (reg_and_address >> 0) & 0xff  // lsb
  };
  int write_rc = i2c_write(i2c_dev, payload, 2, device_addr);
  printk("write_reg result %d\n", write_rc);
  return write_rc;
}

void init_wm8758b_codec() {
  if (device_is_ready(i2c_dev)) {
    // Reset command
    write_reg(WM8758B_ADDRESS, 0, 0);
    // biascut
    write_reg(WM8758B_ADDRESS, 61, (1 << 8));

    // Set
    // PLL enable (1 << 5)
    // BIASEN "The analogue amplifiers will not operate unless BIASEN is enabled." p 76
    // The VMIDSEL and BIASEN bits must be set to enable analogue output midrail voltage and for normal DAC operation. p 24
    // (1 << 0) sets vmidsel to 01 as in the recommended startup sequence
    const int vmidsel = 1 << 0;
    const int bufioen = 1 << 2;
    const int biasen = 1 << 3;
    const int pllen = 1 << 5;
    write_reg(WM8758B_ADDRESS, 1, pllen | biasen | bufioen | vmidsel);

    // Enable OUT1 left/right
    const int adc_enable = (1 << 1) | (1 << 0);
    const int input_pga_enable = (1 << 3) | (1 << 2);
    const int boost_enable = (1 << 5) | (1 << 4);
    const int out1_enable = (1 << 8) | (1 << 7);
    write_reg(WM8758B_ADDRESS, 2, out1_enable | boost_enable | input_pga_enable | adc_enable);

    // Enable DAC left/right and left/right mixer
    const int dac_enable = (1 << 1) | (1 << 0);
    const int mixer_enable = (1 << 3) | (1 << 2);
    write_reg(WM8758B_ADDRESS, 3, mixer_enable | dac_enable);

    // 16 bit i2s format
    write_reg(WM8758B_ADDRESS, 4, 0x10);

    // companding
    // write_reg(i2c_num, 5, 0x0);

    // CLKSEL (use PLL)
    write_reg(WM8758B_ADDRESS, 6, 0x140); // 0x140 should be the default value

    // TODO: The following PLL values
    // are based on an actual sample rate of 41666.7
    // PLL N (8, no pre scale)
    write_reg(WM8758B_ADDRESS, 36, 0x08);
    // PLL K 1, 2, 3
    write_reg(WM8758B_ADDRESS, 37, 0x0);
    write_reg(WM8758B_ADDRESS, 38, 0x0);
    write_reg(WM8758B_ADDRESS, 39, 0x0);

    // Set l/r dac volume
    // write_reg(WM8758B_ADDRESS, 11, 0x1ff);
    // write_reg(WM8758B_ADDRESS, 12, 0x1ff);

    // ALC
    // write_reg(i2c_num, 32, 0x0);
    // write_reg(i2c_num, 33, 0x0);
    // write_reg(i2c_num, 34, 0x0);

    // mic bypass path (for debugging mic input)
    // write_reg(i2c_num, 43, (1 << 8) | (1 << 7));

    // l/r adc digital volume
    write_reg(WM8758B_ADDRESS, 15, 0x1ff);
    write_reg(WM8758B_ADDRESS, 16, 0x1ff);

    // input PGA volume
    // write_reg(i2c_num, 44, 0x22); // loud metallic static if 0
    // write_reg(i2c_num, 45, 0x11f); // gain adjustment audible in the bypass path too
    // write_reg(i2c_num, 46, 0x11f);
    write_reg(WM8758B_ADDRESS, 44, 0x12); // loud metallic static if 0
    write_reg(WM8758B_ADDRESS, 45, 0x3f);
    write_reg(WM8758B_ADDRESS, 46, 0x3f);

    // input boost ctrl. does not seem audible in the bypass path
    const int pga_boost_enable = 1;
    write_reg(WM8758B_ADDRESS, 47, (pga_boost_enable << 8));
    write_reg(WM8758B_ADDRESS, 48, (pga_boost_enable << 8));

    // LOUT1VOL/ROUT1VOL
    write_reg(WM8758B_ADDRESS, 52, (1 << 8) | 0x39);
    write_reg(WM8758B_ADDRESS, 53, (1 << 8) | 0x39);

    // mic bypass path (for debugging mic input)
    // write_reg(WM8758B_ADDRESS, 43, (1 << 8) | (1 << 7));
  } else {
    // TODO: report error
  }
}

void init_wm8904_codec() {

}