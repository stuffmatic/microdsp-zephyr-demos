#include "codec.h"
#include <zephyr/drivers/i2c.h>

#define WM8758B_ADDRESS 0b11010
#define WM8904_ADDRESS 0b11010
#define I2C_NODE DT_NODELABEL(i2c0)
static const struct device *i2c_dev = DEVICE_DT_GET(I2C_NODE);

struct register_write
{
    uint16_t addr;
    uint16_t data;
};

// Writes data to a wmXXXX codec register
int write_reg(uint8_t device_addr, uint16_t reg_addr, uint16_t data)
{
    __ASSERT(reg_addr <= 0x7f, "invalid codec register address"); // 7 bit register address
    __ASSERT(reg_addr <= 0x1ff, "invalid codec register data");   // 9 bit register data
    const uint16_t reg_and_address = (reg_addr << 9) | data;

    uint8_t payload[2] = {
        (reg_and_address >> 8) & 0xff, // msb
        (reg_and_address >> 0) & 0xff  // lsb
    };
    int write_rc = i2c_write(i2c_dev, payload, 2, device_addr);
    printk("write_reg result %d\n", write_rc);
    return write_rc;
}

uint32_t read_reg(uint8_t device_addr, uint16_t reg_addr)
{
    __ASSERT(reg_addr <= 0x7f, "invalid codec register address"); // 7 bit register address
    const uint16_t reg_and_address = (reg_addr << 9);
    uint8_t read_data[2] = {
        (reg_and_address >> 8) & 0xff, // msb
        (reg_and_address >> 0) & 0xff  // lsb
    };
    int read_rc = i2c_read(i2c_dev, read_data, 2, device_addr);
    printk("read_reg result %d\n", read_rc);
    uint32_t result = ((read_data[0] << 8) & 0xff00) | read_data[1];
    return result;
}

void init_wm8758b_codec()
{
    if (device_is_ready(i2c_dev))
    {
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
        write_reg(WM8758B_ADDRESS, 6, 0b100000000); // 0x140 should be the default value

        // TODO: The following PLL values
        // are based on an actual sample rate of 41666.7
        // PLL N (10, no pre scale)
        write_reg(WM8758B_ADDRESS, 36, 0x0b);
        // PLL K (0.66666) 1, 2, 3
        write_reg(WM8758B_ADDRESS, 37, 0b10101010);
        write_reg(WM8758B_ADDRESS, 38, 0b10101010);
        write_reg(WM8758B_ADDRESS, 39, 0b10101011);

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
    }
    else
    {
        // TODO: report error
    }
}

#define WM8904_SW_RESET_AND_ID 0x00
#define WM8904_BIAS_CONTROL_0 0x04
#define WM8904_VMID_CONTROL_0 0x05
#define WM8904_MIC_BIAS_CONTROL_0 0x06
#define WM8904_MIC_BIAS_CONTROL_1 0x07
#define WM8904_ANALOGUE_ADC_0 0x0A
#define WM8904_POWER_MANAGEMENT_0 0x0C
#define WM8904_POWER_MANAGEMENT_2 0x0E
#define WM8904_POWER_MANAGEMENT_3 0x0F
#define WM8904_POWER_MANAGEMENT_6 0x12
#define WM8904_CLOCK_RATES_0 0x14
#define WM8904_CLOCK_RATES_1 0x15
#define WM8904_CLOCK_RATES_2 0x16
#define WM8904_AUDIO_INTERFACE_0 0x18
#define WM8904_AUDIO_INTERFACE_1 0x19
#define WM8904_AUDIO_INTERFACE_2 0x1A
#define WM8904_AUDIO_INTERFACE_3 0x1B
#define WM8904_DAC_DIGITAL_VOLUME_LEFT 0x1E
#define WM8904_DAC_DIGITAL_VOLUME_RIGHT 0x1F
#define WM8904_DAC_DIGITAL_0 0x20
#define WM8904_DAC_DIGITAL_1 0x21
#define WM8904_ADC_DIGITAL_VOLUME_LEFT 0x24
#define WM8904_ADC_DIGITAL_VOLUME_RIGHT 0x25
#define WM8904_ADC_DIGITAL_0 0x26
#define WM8904_DIGITAL_MICROPHONE_0 0x27
#define WM8904_DRC_0 0x28
#define WM8904_DRC_1 0x29
#define WM8904_DRC_2 0x2A
#define WM8904_DRC_3 0x2B
#define WM8904_ANALOGUE_LEFT_INPUT_0 0x2C
#define WM8904_ANALOGUE_RIGHT_INPUT_0 0x2D
#define WM8904_ANALOGUE_LEFT_INPUT_1 0x2E
#define WM8904_ANALOGUE_RIGHT_INPUT_1 0x2F
#define WM8904_ANALOGUE_OUT1_LEFT 0x39
#define WM8904_ANALOGUE_OUT1_RIGHT 0x3A
#define WM8904_ANALOGUE_OUT2_LEFT 0x3B
#define WM8904_ANALOGUE_OUT2_RIGHT 0x3C
#define WM8904_ANALOGUE_OUT12_ZC 0x3D
#define WM8904_DC_SERVO_0 0x43
#define WM8904_DC_SERVO_1 0x44
#define WM8904_DC_SERVO_2 0x45
#define WM8904_DC_SERVO_4 0x47
#define WM8904_DC_SERVO_5 0x48
#define WM8904_DC_SERVO_6 0x49
#define WM8904_DC_SERVO_7 0x4A
#define WM8904_DC_SERVO_8 0x4B
#define WM8904_DC_SERVO_9 0x4C
#define WM8904_DC_SERVO_READBACK_0 0x4D
#define WM8904_ANALOGUE_HP_0 0x5A
#define WM8904_ANALOGUE_LINEOUT_0 0x5E
#define WM8904_CHARGE_PUMP_0 0x62
#define WM8904_CLASS_W_0 0x68
#define WM8904_WRITE_SEQUENCER_0 0x6C
#define WM8904_WRITE_SEQUENCER_1 0x6D
#define WM8904_WRITE_SEQUENCER_2 0x6E
#define WM8904_WRITE_SEQUENCER_3 0x6F
#define WM8904_WRITE_SEQUENCER_4 0x70
#define WM8904_FLL_CONTROL_1 0x74
#define WM8904_FLL_CONTROL_2 0x75
#define WM8904_FLL_CONTROL_3 0x76
#define WM8904_FLL_CONTROL_4 0x77
#define WM8904_FLL_CONTROL_5 0x78
#define WM8904_GPIO_CONTROL_1 0x79
#define WM8904_GPIO_CONTROL_2 0x7A
#define WM8904_GPIO_CONTROL_3 0x7B
#define WM8904_GPIO_CONTROL_4 0x7C
#define WM8904_DIGITAL_PULLS 0x7E
#define WM8904_INTERRUPT_STATUS 0x7F
#define WM8904_INTERRUPT_STATUS_MASK 0x80
#define WM8904_INTERRUPT_POLARITY 0x81
#define WM8904_INTERRUPT_DEBOUNCE 0x82
#define WM8904_EQ1 0x86
#define WM8904_EQ2 0x87
#define WM8904_EQ3 0x88
#define WM8904_EQ4 0x89
#define WM8904_EQ5 0x8A
#define WM8904_EQ6 0x8B
#define WM8904_EQ7 0x8C
#define WM8904_EQ8 0x8D
#define WM8904_EQ9 0x8E
#define WM8904_EQ10 0x8F
#define WM8904_EQ11 0x90
#define WM8904_EQ12 0x91
#define WM8904_EQ13 0x92
#define WM8904_EQ14 0x93
#define WM8904_EQ15 0x94
#define WM8904_EQ16 0x95
#define WM8904_EQ17 0x96
#define WM8904_EQ18 0x97
#define WM8904_EQ19 0x98
#define WM8904_EQ20 0x99
#define WM8904_EQ21 0x9A
#define WM8904_EQ22 0x9B
#define WM8904_EQ23 0x9C
#define WM8904_EQ24 0x9D
#define WM8904_CONTROL_INTERFACE_TEST_1 0xA1
#define WM8904_ADC_TEST_0 0xC6
#define WM8904_FLL_NCO_TEST_0 0xF7
#define WM8904_FLL_NCO_TEST_1 0xF8

void init_wm8904_codec()
{
    // https://github.com/avrxml/asf/blob/master/sam/components/audio/codec/wm8904/example/wm8904_example.c
    if (device_is_ready(i2c_dev))
    {
        // Reset the device, making sure CLK_SYS_ENA = 0. From the datasheet:
        // Note that, if it cannot be assured that MCLK is present when accessing
        // the register map, then it is required to set CLK_SYS_ENA = 0 to
        // ensure correct operation.
        write_reg(WM8904_ADDRESS, WM8904_SW_RESET_AND_ID, 0);

        // Read device id to make sure the device is present
        uint32_t device_id = read_reg(WM8904_ADDRESS, WM8904_SW_RESET_AND_ID);
        __ASSERT(device_id == 0x8904, "Failed to read wm8904 device id");
        k_msleep(1);

        // Perform start-up sequence
        // 0.
        int ISEL = 0b10;
        write_reg(WM8904_ADDRESS, WM8904_BIAS_CONTROL_0, ISEL << 2);
        k_msleep(100);

        // 1.
        int VMID_BUF_ENA = 1;
        int VMID_RES = 0b11;
        int VMID_ENA = 1;
        write_reg(WM8904_ADDRESS, WM8904_VMID_CONTROL_0, (VMID_ENA << 0) | (VMID_RES << 1) | (VMID_BUF_ENA << 6));
        k_msleep(5);

        // 2.
        VMID_RES = 0b01;
        write_reg(WM8904_ADDRESS, WM8904_VMID_CONTROL_0, (VMID_ENA << 0) | (VMID_RES << 1) | (VMID_BUF_ENA << 6));
        k_msleep(1);

        // 3.
        write_reg(WM8904_ADDRESS, WM8904_BIAS_CONTROL_0, (ISEL << 2) | (1 << 0));
        k_msleep(1);

        // 4.
        write_reg(WM8904_ADDRESS, WM8904_POWER_MANAGEMENT_2, (1 << 1) | (1 << 0));
        k_msleep(1);

        // 5.
        write_reg(WM8904_ADDRESS, WM8904_POWER_MANAGEMENT_3, (1 << 1) | (1 << 0));
        k_msleep(1);

        // 6.
        write_reg(WM8904_ADDRESS, WM8904_CLOCK_RATES_2, (1 << 1));
        k_msleep(1);

        // 7.
        write_reg(WM8904_ADDRESS, WM8904_POWER_MANAGEMENT_6, (1 << 3) | (1 << 2));
        k_msleep(1);

        // 10.
        write_reg(WM8904_ADDRESS, WM8904_CHARGE_PUMP_0, (1 << 0));
        k_msleep(1);

        // 12.
        write_reg(WM8904_ADDRESS, WM8904_ANALOGUE_HP_0, (1 << 4) | (1 << 0));
        k_msleep(1);

        // 13.
        write_reg(WM8904_ADDRESS, WM8904_ANALOGUE_LINEOUT_0, (1 << 4) | (1 << 0));
        k_msleep(1);

        // 14.
        write_reg(WM8904_ADDRESS, WM8904_ANALOGUE_HP_0, (1 << 5) | (1 << 4) | (1 << 1) | (1 << 0));
        k_msleep(1);

        // 15.
        write_reg(WM8904_ADDRESS, WM8904_ANALOGUE_LINEOUT_0, (1 << 5) | (1 << 4) | (1 << 1) | (1 << 0));
        k_msleep(1);

        // 16.
        write_reg(WM8904_ADDRESS, WM8904_DC_SERVO_0, (1 << 3) | (1 << 2) | (1 << 1) | (1 << 0));
        k_msleep(1);

        // 17.
        write_reg(WM8904_ADDRESS, WM8904_DC_SERVO_1, (1 << 7) | (1 << 6) | (1 << 5) | (1 << 4));
        k_msleep(1);

        // 19.
        write_reg(WM8904_ADDRESS, WM8904_ANALOGUE_HP_0, (1 << 6) | (1 << 5) | (1 << 4) | (1 << 2) | (1 << 1) | (1 << 0));
        k_msleep(1);

        // 20.
        write_reg(WM8904_ADDRESS, WM8904_ANALOGUE_LINEOUT_0, (1 << 6) | (1 << 5) | (1 << 4) | (1 << 2) | (1 << 1) | (1 << 0));
        k_msleep(1);

        // 21.
        write_reg(WM8904_ADDRESS, WM8904_ANALOGUE_HP_0, (1 << 7) | (1 << 6) | (1 << 5) | (1 << 4) | (1 << 3) | (1 << 2) | (1 << 1) | (1 << 0));
        k_msleep(1);

        // 22.
        write_reg(WM8904_ADDRESS, WM8904_ANALOGUE_LINEOUT_0, (1 << 7) | (1 << 6) | (1 << 5) | (1 << 4) | (1 << 3) | (1 << 2) | (1 << 1) | (1 << 0));
        k_msleep(1);

        // Pll config
        {
            write_reg(WM8904_ADDRESS, WM8904_FLL_CONTROL_1, 0b001); // ENABLE FLL, FRACN_ENA false
            write_reg(WM8904_ADDRESS, WM8904_FLL_CONTROL_2, (9 << 8));
            write_reg(WM8904_ADDRESS, WM8904_FLL_CONTROL_3, 0x0);
            write_reg(WM8904_ADDRESS, WM8904_FLL_CONTROL_4, (72 << 5));
            write_reg(WM8904_ADDRESS, WM8904_FLL_CONTROL_5, 0b01); // use bitclock as FLL reference

            k_msleep(5);

            // wm8904_write_register(WM8904_CLOCK_RATES_1, WM8904_CLK_SYS_RATE(3) | WM8904_SAMPLE_RATE(5));
            // write_reg(WM8904_ADDRESS, WM8904_CLOCK_RATES_1, 0b101 | (0b11 << 10)); // sysclk = 256fs, fs =~ 44.1 khz
            // wm8904_write_register(WM8904_CLOCK_RATES_0, 0x0000);
            // write_reg(WM8904_ADDRESS, WM8904_CLOCK_RATES_0, 0b0);
            // wm8904_write_register(WM8904_CLOCK_RATES_2,
            //           		     WM8904_SYSCLK_SRC | WM8904_CLK_SYS_ENA | WM8904_CLK_DSP_ENA);
            // write_reg(WM8904_ADDRESS, WM8904_CLOCK_RATES_2, (1 << 1) | (1 << 14)); // sysclk src = pll, disable sysclock
            // wm8904_write_register(WM8904_AUDIO_INTERFACE_1, WM8904_BCLK_DIR | WM8904_AIF_FMT_I2S);
            const int word_len = (0b00 << 2); // 16 bit
            write_reg(WM8904_ADDRESS, WM8904_AUDIO_INTERFACE_1, 0b10 | word_len);
        }

        // enable sysclk and set sysclk src to to fll
        write_reg(WM8904_ADDRESS, WM8904_CLOCK_RATES_2, (1 << 1) | (1 << 2) | (1 << 14));
        k_msleep(1);
    }
}

void init_wm8904_codec_old()
{
    // https://github.com/avrxml/asf/blob/master/sam/components/audio/codec/wm8904/example/wm8904_example.c
    if (device_is_ready(i2c_dev))
    {
        // Reset the device, making sure CLK_SYS_ENA = 0. From the datasheet:
        // Note that, if it cannot be assured that MCLK is present when accessing
        // the register map, then it is required to set CLK_SYS_ENA = 0 to
        // ensure correct operation.
        write_reg(WM8904_ADDRESS, WM8904_SW_RESET_AND_ID, 0);

        uint32_t device_id = read_reg(WM8904_ADDRESS, WM8904_SW_RESET_AND_ID);
        __ASSERT(device_id == 0x8904, "Failed to read wm8904 device id");

        // wm8904_write_register(WM8904_BIAS_CONTROL_0, WM8904_ISEL_HP_BIAS);
        write_reg(WM8904_ADDRESS, WM8904_BIAS_CONTROL_0, 0b1000);

        // wm8904_write_register(WM8904_VMID_CONTROL_0, WM8904_VMID_BUF_ENA |
        //     					 WM8904_VMID_RES_FAST | WM8904_VMID_ENA);
        write_reg(WM8904_ADDRESS, WM8904_VMID_CONTROL_0, 0b1000111);
        uint32_t test_read = read_reg(WM8904_ADDRESS, WM8904_VMID_CONTROL_0);
        __ASSERT(test_read == 0b1000, "read/write mismatch");
        k_msleep(5);
        // wm8904_write_register(WM8904_VMID_CONTROL_0, WM8904_VMID_BUF_ENA |
        //    					 WM8904_VMID_RES_NORMAL | WM8904_VMID_ENA);
        write_reg(WM8904_ADDRESS, WM8904_VMID_CONTROL_0, 0b1000011);
        // wm8904_write_register(WM8904_BIAS_CONTROL_0, WM8904_ISEL_HP_BIAS | WM8904_BIAS_ENA);
        write_reg(WM8904_ADDRESS, WM8904_BIAS_CONTROL_0, 0b11001);
        // wm8904_write_register(WM8904_POWER_MANAGEMENT_0, WM8904_INL_ENA | WM8904_INR_ENA);
        write_reg(WM8904_ADDRESS, WM8904_POWER_MANAGEMENT_0, 0b11);
        // wm8904_write_register(WM8904_POWER_MANAGEMENT_2, WM8904_HPL_PGA_ENA | WM8904_HPR_PGA_ENA);
        write_reg(WM8904_ADDRESS, WM8904_POWER_MANAGEMENT_2, 0b11);
        // wm8904_write_register(WM8904_DAC_DIGITAL_1, WM8904_DEEMPH(0));
        write_reg(WM8904_ADDRESS, WM8904_DAC_DIGITAL_1, 0b0);
        // wm8904_write_register(WM8904_ANALOGUE_OUT12_ZC, 0x0000);
        write_reg(WM8904_ADDRESS, WM8904_ANALOGUE_OUT12_ZC, 0b0);
        // wm8904_write_register(WM8904_CHARGE_PUMP_0, WM8904_CP_ENA);
        write_reg(WM8904_ADDRESS, WM8904_CHARGE_PUMP_0, 0b1);
        // wm8904_write_register(WM8904_CLASS_W_0, WM8904_CP_DYN_PWR);
        write_reg(WM8904_ADDRESS, WM8904_CLASS_W_0, 0b1);

        // wm8904_write_register(WM8904_FLL_CONTROL_1, 0x0000);
        // write_reg(WM8904_ADDRESS, WM8904_FLL_CONTROL_1, 0b0);
        // wm8904_write_register(WM8904_FLL_CONTROL_2, WM8904_FLL_OUTDIV(7)| WM8904_FLL_FRATIO(4));
        write_reg(WM8904_ADDRESS, WM8904_FLL_CONTROL_2, (9 << 8));
        // wm8904_write_register(WM8904_FLL_CONTROL_3, WM8904_FLL_K(0x8000));
        write_reg(WM8904_ADDRESS, WM8904_FLL_CONTROL_3, 0x0);
        // wm8904_write_register(WM8904_FLL_CONTROL_4, WM8904_FLL_N(0xBB));
        write_reg(WM8904_ADDRESS, WM8904_FLL_CONTROL_4, (72 << 5));
        // WM8904_FLL_CONTROL_5
        write_reg(WM8904_ADDRESS, WM8904_FLL_CONTROL_5, 0b1 | (0b100)); // use bitclock as FLL reference
        // wm8904_write_register(WM8904_FLL_CONTROL_1, WM8904_FLL_FRACN_ENA | WM8904_FLL_ENA);
        write_reg(WM8904_ADDRESS, WM8904_FLL_CONTROL_1, 0b001); // ENABLE FLL  FRACN_ENA false
        k_msleep(5);

        // wm8904_write_register(WM8904_CLOCK_RATES_1, WM8904_CLK_SYS_RATE(3) | WM8904_SAMPLE_RATE(5));
        write_reg(WM8904_ADDRESS, WM8904_CLOCK_RATES_1, 0b101 | (0b11 << 10)); // sysclk = 256fs, fs =~ 44.1 khz
        // wm8904_write_register(WM8904_CLOCK_RATES_0, 0x0000);
        // write_reg(WM8904_ADDRESS, WM8904_CLOCK_RATES_0, 0b0);
        // wm8904_write_register(WM8904_CLOCK_RATES_2,
        //           		     WM8904_SYSCLK_SRC | WM8904_CLK_SYS_ENA | WM8904_CLK_DSP_ENA);
        // write_reg(WM8904_ADDRESS, WM8904_CLOCK_RATES_2, (1 << 1) | (1 << 14)); // sysclk src = pll, disable sysclock
        // wm8904_write_register(WM8904_AUDIO_INTERFACE_1, WM8904_BCLK_DIR | WM8904_AIF_FMT_I2S);
        const int word_len = (0b00 << 2); // 16 bit
        write_reg(WM8904_ADDRESS, WM8904_AUDIO_INTERFACE_1, 0b10 | word_len);
        // wm8904_write_register(WM8904_AUDIO_INTERFACE_2, WM8904_BCLK_DIV(8));
        // write_reg(WM8904_ADDRESS, WM8904_AUDIO_INTERFACE_2, 0b0100); // <- master only?
        // wm8904_write_register(WM8904_AUDIO_INTERFACE_3, WM8904_LRCLK_DIR | WM8904_LRCLK_RATE(0x20)); // <- master only?
        // write_reg(WM8904_ADDRESS, 0x1b, xxx);
        // wm8904_write_register(WM8904_POWER_MANAGEMENT_6,
        // 	         			 WM8904_DACL_ENA | WM8904_DACR_ENA |
        // 				WM8904_ADCL_ENA | WM8904_ADCR_ENA);
        write_reg(WM8904_ADDRESS, WM8904_POWER_MANAGEMENT_6, 0b1111);
        k_msleep(5);

        // wm8904_write_register(WM8904_ANALOGUE_LEFT_INPUT_0, WM8904_LIN_VOL(0x10));
        // wm8904_write_register(WM8904_ANALOGUE_RIGHT_INPUT_0, WM8904_RIN_VOL(0x10));

        // wm8904_write_register(WM8904_ANALOGUE_HP_0,
        // 				WM8904_HPL_ENA | WM8904_HPR_ENA);
        write_reg(WM8904_ADDRESS, WM8904_ANALOGUE_HP_0, 0b10001);
        write_reg(WM8904_ADDRESS, WM8904_ANALOGUE_LINEOUT_0, 0b10001);
        k_msleep(1);
        // wm8904_write_register(WM8904_ANALOGUE_HP_0,
        // 				WM8904_HPL_ENA_DLY | WM8904_HPL_ENA |
        // 				WM8904_HPR_ENA_DLY | WM8904_HPR_ENA);
        write_reg(WM8904_ADDRESS, WM8904_ANALOGUE_HP_0, 0b110011);
        write_reg(WM8904_ADDRESS, WM8904_ANALOGUE_LINEOUT_0, 0b110011);

        // wm8904_write_register(WM8904_DC_SERVO_0,
        // WM8904_DCS_ENA_CHAN_3 | WM8904_DCS_ENA_CHAN_2 |
        // WM8904_DCS_ENA_CHAN_1 | WM8904_DCS_ENA_CHAN_0);
        write_reg(WM8904_ADDRESS, WM8904_DC_SERVO_0, 0b1111);
        // wm8904_write_register(WM8904_DC_SERVO_1,
        // 			WM8904_DCS_TRIG_STARTUP_3 | WM8904_DCS_TRIG_STARTUP_2 |
        // 		WM8904_DCS_TRIG_STARTUP_1 | WM8904_DCS_TRIG_STARTUP_0);
        write_reg(WM8904_ADDRESS, WM8904_DC_SERVO_1, (1 << 4) | (1 << 5) | (1 << 6) | (1 << 7));
        k_msleep(100);
        // wm8904_write_register(WM8904_ANALOGUE_HP_0,
        // 			WM8904_HPL_ENA_OUTP | WM8904_HPL_ENA_DLY | WM8904_HPL_ENA |
        // 		WM8904_HPR_ENA_OUTP | WM8904_HPR_ENA_DLY | WM8904_HPR_ENA);
        write_reg(WM8904_ADDRESS, WM8904_ANALOGUE_HP_0, 0b1110111);
        write_reg(WM8904_ADDRESS, WM8904_ANALOGUE_LINEOUT_0, 0b1110111);

        // wm8904_write_register(WM8904_ANALOGUE_HP_0,
        // 			WM8904_HPL_RMV_SHORT | WM8904_HPL_ENA_OUTP | WM8904_HPL_ENA_DLY | WM8904_HPL_ENA |
        // 		WM8904_HPR_RMV_SHORT | WM8904_HPR_ENA_OUTP | WM8904_HPR_ENA_DLY | WM8904_HPR_ENA);
        write_reg(WM8904_ADDRESS, WM8904_ANALOGUE_HP_0, 0b11111111);
        write_reg(WM8904_ADDRESS, WM8904_ANALOGUE_LINEOUT_0, 0b11111111);

        // wm8904_write_register(WM8904_ANALOGUE_OUT1_LEFT, WM8904_HPOUT_VU | WM8904_HPOUTL_VOL(0x39));
        // wm8904_write_register(WM8904_ANALOGUE_OUT1_RIGHT, WM8904_HPOUT_VU | WM8904_HPOUTR_VOL(0x39));
        write_reg(WM8904_ADDRESS, WM8904_ANALOGUE_OUT1_LEFT, 0b111001 | (1 << 7));
        write_reg(WM8904_ADDRESS, WM8904_ANALOGUE_OUT1_RIGHT, 0b111001 | (1 << 7));
        write_reg(WM8904_ADDRESS, WM8904_ANALOGUE_OUT2_LEFT, 0b111001 | (1 << 7));
        write_reg(WM8904_ADDRESS, WM8904_ANALOGUE_OUT2_RIGHT, 0b111001 | (1 << 7));

        k_msleep(100);

        write_reg(WM8904_ADDRESS, WM8904_CLOCK_RATES_2, (1 << 1) | (1 << 2) | (1 << 14)); // sysclk src = pll, ENABLE CLK_SYS_ENA
    }
}