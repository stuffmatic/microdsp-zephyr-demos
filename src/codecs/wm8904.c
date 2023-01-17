#include <zephyr/drivers/i2c.h>
#include "wm8904.h"

#ifdef CONFIG_SOC_SERIES_NRF53X
#define I2C_NODE DT_NODELABEL(i2c1)
#else
#define I2C_NODE DT_NODELABEL(i2c0)
#endif
static const struct device *i2c_dev = DEVICE_DT_GET(I2C_NODE);

int wm8904_read_reg(uint8_t reg_addr, uint16_t* result)
{
    uint8_t write_buf[1] = { reg_addr };
    uint8_t read_buf[2] = { 0, 0 };
    int read_rc = i2c_write_read(i2c_dev, WM8904_ADDRESS, write_buf, 1, read_buf, 2);
    if (read_rc == 0) {
        *result = ((read_buf[0] << 8) & 0xff00) | read_buf[1];
    }
    __ASSERT(read_rc == 0, "wm8904_read_reg failed");
    return read_rc;
}

int wm8904_write_reg(uint8_t reg_addr, uint16_t data)
{
    uint8_t write_buf[3] = {
        reg_addr,
        (data & 0xff00) >> 8, /* msb */
        data & 0xff  /* lsb */
    };
    uint8_t read_buf[2] = { 0, 0 };
    int write_rc = i2c_write_read(i2c_dev, WM8904_ADDRESS, write_buf, 3, read_buf, 2);
    /* printk("write_reg result %d\n", write_rc); */
    __ASSERT(write_rc == 0, "wm8904_write_reg failed");
    return write_rc;
}

void wm8904_init()
{
    if (device_is_ready(i2c_dev))
    {
        /* Reset the device, making sure CLK_SYS_ENA = 0. From the datasheet:
           Note that, if it cannot be assured that MCLK is present when accessing
           the register map, then it is required to set CLK_SYS_ENA = 0 to
           ensure correct operation. */
        wm8904_write_reg(WM8904_SW_RESET_AND_ID, 0);

        uint16_t device_id = 0;
        wm8904_read_reg(WM8904_SW_RESET_AND_ID, &device_id);
        __ASSERT(device_id == 0x8904, "Failed to read wm8904 device id");

        wm8904_write_reg(WM8904_BIAS_CONTROL_0, 0b1000);
        wm8904_write_reg(WM8904_VMID_CONTROL_0, 0b1000111);
        k_msleep(5);
        wm8904_write_reg(WM8904_VMID_CONTROL_0, 0b1000011);
        wm8904_write_reg(WM8904_BIAS_CONTROL_0, 0b01001);
        wm8904_write_reg(WM8904_POWER_MANAGEMENT_0, 0b11);
        wm8904_write_reg(WM8904_POWER_MANAGEMENT_2, 0b11);
        wm8904_write_reg(WM8904_DAC_DIGITAL_1, 0b0);
        wm8904_write_reg(WM8904_ANALOGUE_OUT12_ZC, 0b0);
        wm8904_write_reg(WM8904_CHARGE_PUMP_0, 0b1);
        wm8904_write_reg(WM8904_CLASS_W_0, 0b1);

        /* NOTE: Bit pattern does NOT correspond to FLL_RATIO number */
        wm8904_write_reg(WM8904_FLL_CONTROL_2, (8 - 1) << 8);
        wm8904_write_reg(WM8904_FLL_CONTROL_3, 0xaaab);
        wm8904_write_reg(WM8904_FLL_CONTROL_4, 42 << 5); //

        wm8904_write_reg(WM8904_FLL_CONTROL_5, 0b1); /* use bitckl as FLL reference */
        wm8904_write_reg(WM8904_FLL_CONTROL_1, 0b101); /* ENABLE FLL  FRACN_ENA true */
        k_msleep(5);

        wm8904_write_reg(WM8904_CLOCK_RATES_0, 0b0);
        wm8904_write_reg(WM8904_CLOCK_RATES_1, 0b101 | (0b11 << 10)); /* sysclk = 256fs, fs =~ 44.1 khz */

        wm8904_write_reg(WM8904_AUDIO_INTERFACE_1, 0b1010); /* 24 bit i2s */
        wm8904_write_reg(WM8904_POWER_MANAGEMENT_6, 0b1111);
        k_msleep(5);

        wm8904_write_reg(WM8904_ANALOGUE_HP_0, 0b10001);
        wm8904_write_reg(WM8904_ANALOGUE_LINEOUT_0, 0b10001);
        k_msleep(1);

        wm8904_write_reg(WM8904_ANALOGUE_HP_0, 0b110011);
        wm8904_write_reg(WM8904_ANALOGUE_LINEOUT_0, 0b110011);

        wm8904_write_reg(WM8904_DC_SERVO_0, 0b1111);
        wm8904_write_reg(WM8904_DC_SERVO_1, (1 << 4) | (1 << 5) | (1 << 6) | (1 << 7));
        k_msleep(100);

        wm8904_write_reg(WM8904_ANALOGUE_HP_0, 0b1110111);
        wm8904_write_reg(WM8904_ANALOGUE_LINEOUT_0, 0b1110111);

        wm8904_write_reg(WM8904_ANALOGUE_HP_0, 0b11111111);
        wm8904_write_reg(WM8904_ANALOGUE_LINEOUT_0, 0b11111111);

        wm8904_write_reg(WM8904_ANALOGUE_OUT1_LEFT, 0b111001 | (1 << 7));
        wm8904_write_reg(WM8904_ANALOGUE_OUT1_RIGHT, 0b111001 | (1 << 7));
        wm8904_write_reg(WM8904_ANALOGUE_OUT2_LEFT, 0b111001 | (1 << 7));
        wm8904_write_reg(WM8904_ANALOGUE_OUT2_RIGHT, 0b111001 | (1 << 7));

        k_msleep(100);

        /* unmute, +19.2 dB gain */
        wm8904_write_reg(WM8904_ANALOGUE_LEFT_INPUT_0, 0b11100);
        wm8904_write_reg(WM8904_ANALOGUE_RIGHT_INPUT_0, 0b11100);
        /* set digital volume */
        wm8904_write_reg(WM8904_ADC_DIGITAL_VOLUME_LEFT,  0b11000000);
        wm8904_write_reg(WM8904_ADC_DIGITAL_VOLUME_RIGHT, 0b11000000);

        wm8904_write_reg(WM8904_ANALOGUE_LEFT_INPUT_1, 0b0010000);
        wm8904_write_reg(WM8904_ANALOGUE_RIGHT_INPUT_1, 0b0010000);

        /* mic bypass */
        /* wm8904_write_reg(WM8904_ANALOGUE_OUT12_ZC, 0b1111); */

        /* digital loopback */
        /* wm8904_write_reg(WM8904_AUDIO_INTERFACE_0, 0b101010000); */

        /* sysclk src = pll, enable sys clk, enable dsp clk
           if there's no mclk input, make sure this is the last write. */
        wm8904_write_reg(WM8904_CLOCK_RATES_2, (1 << 1) | (1 << 2) | (1 << 14));
    }
}