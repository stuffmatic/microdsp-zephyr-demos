#include <zephyr.h>
#include <nrfx_i2s.h>

nrfx_i2s_buffers_t nrfx_i2s_buffers_1 = {
    .p_rx_buffer = NULL,
    .p_tx_buffer = NULL
};

nrfx_i2s_buffers_t nrfx_i2s_buffers_2 = {
    .p_rx_buffer = NULL,
    .p_tx_buffer = NULL
};

// #define USE_WM8904

#define DESIRED_SAMPLE_RATE 44100
#define ACTUAL_SAMPLE_RATE 41666.7 // 43478.261
#define AUDIO_BUFFER_N_FRAMES 256
#define AUDIO_BUFFER_N_CHANNELS 2
#define BYTES_PER_SAMPLE 2
#define AUDIO_BUFFER_N_SAMPLES (AUDIO_BUFFER_N_FRAMES * AUDIO_BUFFER_N_CHANNELS)
#define AUDIO_BUFFER_BYTE_SIZE (BYTES_PER_SAMPLE * AUDIO_BUFFER_N_SAMPLES)
#define AUDIO_BUFFER_WORD_SIZE (AUDIO_BUFFER_BYTE_SIZE / 4)

static int is_buffer_1 = 1;
static int16_t rx_static_1[AUDIO_BUFFER_N_SAMPLES];
static int16_t tx_static_1[AUDIO_BUFFER_N_SAMPLES];
static int16_t rx_static_2[AUDIO_BUFFER_N_SAMPLES];
static int16_t tx_static_2[AUDIO_BUFFER_N_SAMPLES];
static float scratch_buffer_out[AUDIO_BUFFER_N_SAMPLES];
static float scratch_buffer_in[AUDIO_BUFFER_N_SAMPLES];

// I2S config
// uint8_t lrck_pin     = 32 + 2; // MCU_GPIO_PORTB(2); // 32 + 2; // P1.02
// uint8_t sdout_pin    = 5; // P0.05
// uint8_t sck_pin      = 32 + 3; // MCU_GPIO_PORTB(3); // P1.03

const uint8_t lrck_pin = 11;  // 29; // P0.29 7 aka word select
const uint8_t sdout_pin = 7; // 30; // P0.30 25
const uint8_t sck_pin = 26;  // 31; // aka bitcklock P0.31 26
const uint8_t mck_pin = 6;  // 27; // NRFX_I2S_PIN_NOT_USED;
const uint8_t sdin_pin = 25; // NRFX_I2S_PIN_NOT_USED; // 6; // NRFX_I2S_PIN_NOT_USED;

nrfx_i2s_config_t nrfx_i2s_cfg = {
    .sck_pin = sck_pin,
    .lrck_pin = lrck_pin,
    .mck_pin = mck_pin,
    .sdout_pin = sdout_pin,
    .sdin_pin = sdin_pin,
    .irq_priority = NRFX_I2S_DEFAULT_CONFIG_IRQ_PRIORITY,
    .mode = NRF_I2S_MODE_MASTER,
    .format = NRF_I2S_FORMAT_I2S,
    .alignment = NRF_I2S_ALIGN_LEFT,
    .channels = NRF_I2S_CHANNELS_STEREO,
    #ifdef USE_WM8904
    .sample_width = NRF_I2S_SWIDTH_24BIT,
    .mck_setup = NRF_I2S_MCK_32MDIV15,
    .ratio = NRF_I2S_RATIO_48X,
    #else
    .sample_width = NRF_I2S_SWIDTH_16BIT,
    .mck_setup = 0x50000000UL, // NRF_I2S_MCK_32MDIV3,
    .ratio = NRF_I2S_RATIO_256X,
    #endif
};

void i2s_callback(void* tx_ptr, void* rx_ptr) {
		// TODO: not always int16_t
    int16_t* tx = (int16_t*)tx_ptr;
    int16_t* rx = (int16_t*)rx_ptr;
    const int frame_count = AUDIO_BUFFER_N_FRAMES;
    memset(scratch_buffer_out, 0, AUDIO_BUFFER_N_SAMPLES * sizeof(float));

		// Convert incoming audio from PCM
    for (int i = 0; i < frame_count; i++) {
        scratch_buffer_in[i] = (rx[2 * i] + rx[2 * i + 1]) / 32767.0;
    }

		// Render outgoing audio
		for (int i = 0; i < frame_count; i++) {
			scratch_buffer_out[i] = 0.001 * frame_count;
		}

		// Convert outgoing audio to PCM
    for (int i = 0; i < frame_count; i++)
    {
        const float value = scratch_buffer_out[i];
        int16_t sample = 32767.0 * value;
        tx[2 * i] = sample;
        tx[2 * i + 1] = sample;
    }
}

void nrfx_i2s_data_handler(nrfx_i2s_buffers_t const *p_released, uint32_t status)
{
    if (status == NRFX_I2S_STATUS_NEXT_BUFFERS_NEEDED)
    {
        if (p_released != NULL && p_released->p_tx_buffer != NULL && p_released->p_rx_buffer != NULL) {
            i2s_callback((void*)p_released->p_tx_buffer, (void*)p_released->p_rx_buffer);
        }
        is_buffer_1 = !is_buffer_1;
        nrfx_err_t result = nrfx_i2s_next_buffers_set(is_buffer_1 ? &nrfx_i2s_buffers_1 : &nrfx_i2s_buffers_2);
        __ASSERT(result == NRFX_SUCCESS, "nrfx_i2s_next_buffers_set failed with result %d", result);
    }
    else if (status == NRFX_I2S_STATUS_TRANSFER_STOPPED)
    {
        __ASSERT(0, "status == NRFX_I2S_STATUS_TRANSFER_STOPPED");
    }
}

ISR_DIRECT_DECLARE(i2s_isr_handler)
{
	// data_ready_flag = false;
	nrfx_i2s_irq_handler();
	ISR_DIRECT_PM(); /* PM done after servicing interrupt for best latency
			  */
	return 1;		 /* We should check if scheduling decision should be made */
}

nrfx_err_t i2s_start() {
	nrfx_i2s_buffers_1.p_rx_buffer = (uint32_t*)(&rx_static_1);
	nrfx_i2s_buffers_1.p_tx_buffer = (uint32_t*)(&tx_static_1);
	nrfx_i2s_buffers_2.p_rx_buffer = (uint32_t*)(&rx_static_2);
	nrfx_i2s_buffers_2.p_tx_buffer = (uint32_t*)(&tx_static_2);

  IRQ_DIRECT_CONNECT(NRFX_IRQ_NUMBER_GET(NRF_I2S), 0, i2s_isr_handler, 0);

  nrfx_err_t result = nrfx_i2s_init(&nrfx_i2s_cfg, &nrfx_i2s_data_handler);
	__ASSERT(result == NRFX_SUCCESS, "nrfx_i2s_init failed with result %d", result);
	result = nrfx_i2s_start(&nrfx_i2s_buffers_1, AUDIO_BUFFER_WORD_SIZE, 0);
	__ASSERT(result == NRFX_SUCCESS, "nrfx_i2s_start failed with result %d", result);

  return NRFX_SUCCESS; // TODO: return proper result
}