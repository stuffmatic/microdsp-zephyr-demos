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
    return read_rc;
}

int wm8904_write_reg(uint8_t reg_addr, uint16_t data)
{
    uint8_t write_buf[3] = {
        reg_addr,
        (data & 0xff00) >> 8, // msb
        data & 0xff  // lsb
    };
    uint8_t read_buf[2] = { 0, 0 };
    int write_rc = i2c_write_read(i2c_dev, WM8904_ADDRESS, write_buf, 3, read_buf, 2);
    // printk("write_reg result %d\n", write_rc);
    return write_rc;
}

void wm8904_init()
{
    // https://github.com/avrxml/asf/blob/master/sam/components/audio/codec/wm8904/example/wm8904_example.c
    if (device_is_ready(i2c_dev))
    {
        // Reset the device, making sure CLK_SYS_ENA = 0. From the datasheet:
        // Note that, if it cannot be assured that MCLK is present when accessing
        // the register map, then it is required to set CLK_SYS_ENA = 0 to
        // ensure correct operation.
        wm8904_write_reg(WM8904_SW_RESET_AND_ID, 0);

        uint16_t device_id = 0;
        wm8904_read_reg(WM8904_SW_RESET_AND_ID, &device_id);
        __ASSERT(device_id == 0x8904, "Failed to read wm8904 device id");

        // wm8904_write_register(WM8904_BIAS_CONTROL_0, WM8904_ISEL_HP_BIAS);
        wm8904_write_reg(WM8904_BIAS_CONTROL_0, 0b1000);

        // wm8904_write_register(WM8904_VMID_CONTROL_0, WM8904_VMID_BUF_ENA |
        //     					 WM8904_VMID_RES_FAST | WM8904_VMID_ENA);
        wm8904_write_reg(WM8904_VMID_CONTROL_0, 0b1000111);
        k_msleep(5);
        // wm8904_write_register(WM8904_VMID_CONTROL_0, WM8904_VMID_BUF_ENA |
        //    					 WM8904_VMID_RES_NORMAL | WM8904_VMID_ENA);
        wm8904_write_reg(WM8904_VMID_CONTROL_0, 0b1000011);
        // wm8904_write_register(WM8904_BIAS_CONTROL_0, WM8904_ISEL_HP_BIAS | WM8904_BIAS_ENA);
        wm8904_write_reg(WM8904_BIAS_CONTROL_0, 0b11001);
        // wm8904_write_register(WM8904_POWER_MANAGEMENT_0, WM8904_INL_ENA | WM8904_INR_ENA);
        wm8904_write_reg(WM8904_POWER_MANAGEMENT_0, 0b11);
        // wm8904_write_register(WM8904_POWER_MANAGEMENT_2, WM8904_HPL_PGA_ENA | WM8904_HPR_PGA_ENA);
        wm8904_write_reg(WM8904_POWER_MANAGEMENT_2, 0b11);
        // wm8904_write_register(WM8904_DAC_DIGITAL_1, WM8904_DEEMPH(0));
        wm8904_write_reg(WM8904_DAC_DIGITAL_1, 0b0);
        // wm8904_write_register(WM8904_ANALOGUE_OUT12_ZC, 0x0000);
        wm8904_write_reg(WM8904_ANALOGUE_OUT12_ZC, 0b0);
        // wm8904_write_register(WM8904_CHARGE_PUMP_0, WM8904_CP_ENA);
        wm8904_write_reg(WM8904_CHARGE_PUMP_0, 0b1);
        // wm8904_write_register(WM8904_CLASS_W_0, WM8904_CP_DYN_PWR);
        wm8904_write_reg(WM8904_CLASS_W_0, 0b1);

        // wm8904_write_register(WM8904_FLL_CONTROL_1, 0x0000);
        // wm8904_write_register(WM8904_FLL_CONTROL_1, WM8904_FLL_FRACN_ENA | WM8904_FLL_ENA);
        // wm8904_write_register(WM8904_FLL_CONTROL_2, WM8904_FLL_OUTDIV(7)| WM8904_FLL_FRATIO(4));
        wm8904_write_reg(WM8904_FLL_CONTROL_2, 8 << 8);
        // wm8904_write_register(WM8904_FLL_CONTROL_3, WM8904_FLL_K(0x8000));
        wm8904_write_reg(WM8904_FLL_CONTROL_3, 0x0);
        // wm8904_write_register(WM8904_FLL_CONTROL_4, WM8904_FLL_N(0xBB));
        wm8904_write_reg(WM8904_FLL_CONTROL_4, 32 << 5); //
        // WM8904_FLL_CONTROL_5
        wm8904_write_reg(WM8904_FLL_CONTROL_5, 0b1); // use bitckl as FLL reference
        wm8904_write_reg(WM8904_FLL_CONTROL_1, 0b001); // ENABLE FLL  FRACN_ENA true
        k_msleep(5);

        // wm8904_write_register(WM8904_CLOCK_RATES_0, 0x0000);
        wm8904_write_reg(WM8904_CLOCK_RATES_0, 0b0);
        // wm8904_write_register(WM8904_CLOCK_RATES_1, WM8904_CLK_SYS_RATE(3) | WM8904_SAMPLE_RATE(5));
        wm8904_write_reg(WM8904_CLOCK_RATES_1, 0b101 | (0b11 << 10)); // sysclk = 256fs, fs =~ 44.1 khz
        // wm8904_write_register(WM8904_CLOCK_RATES_2,
        //           		     WM8904_SYSCLK_SRC | WM8904_CLK_SYS_ENA | WM8904_CLK_DSP_ENA);
        // wm8904_write_reg(WM8904_CLOCK_RATES_2, (1 << 1) | (1 << 14)); // sysclk src = pll, disable sysclock
        // wm8904_write_register(WM8904_AUDIO_INTERFACE_1, WM8904_BCLK_DIR | WM8904_AIF_FMT_I2S);
        const int word_len = (0b10 << 2); // 24 bit
        wm8904_write_reg(WM8904_AUDIO_INTERFACE_1, 0b10 | word_len);
        // wm8904_write_register(WM8904_AUDIO_INTERFACE_2, WM8904_BCLK_DIV(8));
        // wm8904_write_reg(WM8904_AUDIO_INTERFACE_2, 0b0100); // <- master only?
        // wm8904_write_register(WM8904_AUDIO_INTERFACE_3, WM8904_LRCLK_DIR | WM8904_LRCLK_RATE(0x20)); // <- master only?
        // wm8904_write_reg(0x1b, xxx);
        // wm8904_write_register(WM8904_POWER_MANAGEMENT_6,
        // 	         			 WM8904_DACL_ENA | WM8904_DACR_ENA |
        // 				WM8904_ADCL_ENA | WM8904_ADCR_ENA);
        wm8904_write_reg(WM8904_POWER_MANAGEMENT_6, 0b1111);
        k_msleep(5);


        // wm8904_write_register(WM8904_ANALOGUE_HP_0,
        // 				WM8904_HPL_ENA | WM8904_HPR_ENA);
        wm8904_write_reg(WM8904_ANALOGUE_HP_0, 0b10001);
        wm8904_write_reg(WM8904_ANALOGUE_LINEOUT_0, 0b10001);
        k_msleep(1);
        // wm8904_write_register(WM8904_ANALOGUE_HP_0,
        // 				WM8904_HPL_ENA_DLY | WM8904_HPL_ENA |
        // 				WM8904_HPR_ENA_DLY | WM8904_HPR_ENA);
        wm8904_write_reg(WM8904_ANALOGUE_HP_0, 0b110011);
        wm8904_write_reg(WM8904_ANALOGUE_LINEOUT_0, 0b110011);

        // wm8904_write_register(WM8904_DC_SERVO_0,
        // WM8904_DCS_ENA_CHAN_3 | WM8904_DCS_ENA_CHAN_2 |
        // WM8904_DCS_ENA_CHAN_1 | WM8904_DCS_ENA_CHAN_0);
        wm8904_write_reg(WM8904_DC_SERVO_0, 0b1111);
        // wm8904_write_register(WM8904_DC_SERVO_1,
        // 			WM8904_DCS_TRIG_STARTUP_3 | WM8904_DCS_TRIG_STARTUP_2 |
        // 		WM8904_DCS_TRIG_STARTUP_1 | WM8904_DCS_TRIG_STARTUP_0);
        wm8904_write_reg(WM8904_DC_SERVO_1, (1 << 4) | (1 << 5) | (1 << 6) | (1 << 7));
        k_msleep(100);
        // wm8904_write_register(WM8904_ANALOGUE_HP_0,
        // 			WM8904_HPL_ENA_OUTP | WM8904_HPL_ENA_DLY | WM8904_HPL_ENA |
        // 		WM8904_HPR_ENA_OUTP | WM8904_HPR_ENA_DLY | WM8904_HPR_ENA);
        wm8904_write_reg(WM8904_ANALOGUE_HP_0, 0b1110111);
        wm8904_write_reg(WM8904_ANALOGUE_LINEOUT_0, 0b1110111);

        // wm8904_write_register(WM8904_ANALOGUE_HP_0,
        // 			WM8904_HPL_RMV_SHORT | WM8904_HPL_ENA_OUTP | WM8904_HPL_ENA_DLY | WM8904_HPL_ENA |
        // 		WM8904_HPR_RMV_SHORT | WM8904_HPR_ENA_OUTP | WM8904_HPR_ENA_DLY | WM8904_HPR_ENA);
        wm8904_write_reg(WM8904_ANALOGUE_HP_0, 0b11111111);
        wm8904_write_reg(WM8904_ANALOGUE_LINEOUT_0, 0b11111111);

        // wm8904_write_register(WM8904_ANALOGUE_OUT1_LEFT, WM8904_HPOUT_VU | WM8904_HPOUTL_VOL(0x39));
        // wm8904_write_register(WM8904_ANALOGUE_OUT1_RIGHT, WM8904_HPOUT_VU | WM8904_HPOUTR_VOL(0x39));
        wm8904_write_reg(WM8904_ANALOGUE_OUT1_LEFT, 0b111001 | (1 << 7));
        wm8904_write_reg(WM8904_ANALOGUE_OUT1_RIGHT, 0b111001 | (1 << 7));
        wm8904_write_reg(WM8904_ANALOGUE_OUT2_LEFT, 0b111001 | (1 << 7));
        wm8904_write_reg(WM8904_ANALOGUE_OUT2_RIGHT, 0b111001 | (1 << 7));

        k_msleep(100);

        // mic test, left channel
        if (true)
        {
            // unmute, full gain
            wm8904_write_reg(WM8904_ANALOGUE_LEFT_INPUT_0, 0b11111);
            wm8904_write_reg(WM8904_ANALOGUE_RIGHT_INPUT_1, 0b11111);
            // set digital volume
            wm8904_write_reg(WM8904_ADC_DIGITAL_VOLUME_LEFT, 0b11000000);
            wm8904_write_reg(WM8904_ADC_DIGITAL_VOLUME_RIGHT, 0b11000000);
            // wm8904_write_reg(WM8904_ANALOGUE_RIGHT_INPUT_0, 0b11111);
            wm8904_write_reg(WM8904_ANALOGUE_LEFT_INPUT_1, 0b0010100);
            wm8904_write_reg(WM8904_ANALOGUE_RIGHT_INPUT_1, 0b0010100);

            // mic bypass
            // wm8904_write_reg(WM8904_ANALOGUE_OUT12_ZC, 0b1111);

            // digital loopback
            // wm8904_write_reg(WM8904_AUDIO_INTERFACE_0, 0b101010000);
        }

        // sysclk src = pll, enable sys clk, enable dsp clk
        // if there's no mclk input, make sure this is the last write.
        wm8904_write_reg(WM8904_CLOCK_RATES_2, (1 << 1) | (1 << 2) | (1 << 14));

    }
}