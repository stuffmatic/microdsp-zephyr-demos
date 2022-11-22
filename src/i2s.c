#include "i2s.h"
#include <zephyr.h>
#include <zephyr/kernel/thread.h>

////////////////////////////////////////////////////////////
// Audio buffers
////////////////////////////////////////////////////////////
#define AUDIO_BUFFER_N_FRAMES 256
#define AUDIO_BUFFER_N_CHANNELS 2
#define BYTES_PER_SAMPLE 2
#define AUDIO_BUFFER_N_SAMPLES (AUDIO_BUFFER_N_FRAMES * AUDIO_BUFFER_N_CHANNELS)
#define AUDIO_BUFFER_BYTE_SIZE (BYTES_PER_SAMPLE * AUDIO_BUFFER_N_SAMPLES)
#define AUDIO_BUFFER_WORD_SIZE (AUDIO_BUFFER_BYTE_SIZE / 4)

// Floating point scratch buffers.
static float scratch_buffer_out[AUDIO_BUFFER_N_SAMPLES];
static float scratch_buffer_in[AUDIO_BUFFER_N_SAMPLES];

// Use two pairs of rx/tx buffers for double buffering,
// i.e process/render one pair of buffers while the other is
// being transfered.
// __attribute__((aligned(4)))
static int16_t __attribute__((aligned(4))) rx_1[AUDIO_BUFFER_N_SAMPLES];
static int16_t __attribute__((aligned(4))) tx_1[AUDIO_BUFFER_N_SAMPLES];
static int16_t __attribute__((aligned(4))) rx_2[AUDIO_BUFFER_N_SAMPLES];
static int16_t __attribute__((aligned(4))) tx_2[AUDIO_BUFFER_N_SAMPLES];
nrfx_i2s_buffers_t nrfx_i2s_buffers_1 = {
    .p_rx_buffer = (uint32_t *)(&rx_1), .p_tx_buffer = (uint32_t *)(&tx_1)
};
nrfx_i2s_buffers_t nrfx_i2s_buffers_2 = {
    .p_rx_buffer = (uint32_t *)(&rx_2), .p_tx_buffer = (uint32_t *)(&tx_2)
};

////////////////////////////////////////////////////////////
// Audio processing synchronization
////////////////////////////////////////////////////////////
// Flag indicating which buffer pair is currently available for
// processing/rendering.
static int processing_buffers_1 = 0;
// A semaphore used to trigger audio processing on a dedicated thread
// from the I2S interrupt handler.
K_SEM_DEFINE(processing_thread_semaphore, 0, 1);
// Indicates if the current buffer pair is being processed/rendered.
atomic_t processing_in_progress = ATOMIC_INIT(0x00);

////////////////////////////////////////////////////////////
// Audio processing thread
////////////////////////////////////////////////////////////
#define PROCESSING_THREAD_STACK_SIZE 2048
// Processing thread is a top priority cooperative thread
#define PROCESSING_THREAD_PRIORITY -CONFIG_NUM_COOP_PRIORITIES
K_THREAD_STACK_DEFINE(processing_thread_stack_area, PROCESSING_THREAD_STACK_SIZE);
struct k_thread processing_thread_data;

static float oscillator_phase = -1;
static float oscillator_dphase = 2.0 * 600.0 / 44100.0;
static void processing_callback(float *tx, const float *rx) {
    for (int i = 0; i < AUDIO_BUFFER_N_FRAMES; i++) {
        const float a = oscillator_phase < 0 ? 1 : -1;
        const float gain = 0.1;
        tx[2 * i] = gain * (oscillator_phase + a * oscillator_phase * oscillator_phase);
        oscillator_phase += oscillator_dphase;
        if (oscillator_phase > 1) {
            oscillator_phase -= 2;
        }
    }
}

static void process_buffers(int16_t* tx, const int16_t* rx) {
    // Convert incoming audio from PCM
    for (int i = 0; i < AUDIO_BUFFER_N_SAMPLES; i++) {
        scratch_buffer_in[i] = rx[i] / 32767.0;
    }

    memset(scratch_buffer_out, 0, AUDIO_BUFFER_N_SAMPLES * sizeof(float));
    processing_callback(scratch_buffer_out, scratch_buffer_in);

    // Convert outgoing audio to PCM
    for (int i = 0; i < AUDIO_BUFFER_N_SAMPLES; i++) {
        tx[i] = scratch_buffer_out[i] * 32767.0; // TODO: saturated?
    }
}

static void processing_thread_entry_point(void *p1, void *p2, void *p3) {
    while (true) {
        if (k_sem_take(&processing_thread_semaphore, K_FOREVER) == 0) {
            if (!atomic_test_bit(&processing_in_progress, 0)) {
                printk("processing_in_progress is not set");
                __ASSERT(atomic_test_bit(&processing_in_progress, 0), "processing_in_progress is not set");
            }

            nrfx_i2s_buffers_t* buffers_to_process = processing_buffers_1 ? &nrfx_i2s_buffers_1 : &nrfx_i2s_buffers_2;
            // TODO: not always int16_t
            int16_t *tx = (int16_t *)buffers_to_process->p_tx_buffer;
            int16_t *rx = (int16_t *)buffers_to_process->p_rx_buffer;

            process_buffers(tx, rx);

            nrfx_i2s_buffers_t* buffers_to_set = processing_buffers_1 ? &nrfx_i2s_buffers_2 : &nrfx_i2s_buffers_1;
            nrfx_err_t result = nrfx_i2s_next_buffers_set(buffers_to_set);
            if (result != NRFX_SUCCESS) {
                printk("nrfx_i2s_next_buffers_set failed with %d\n", result);
                __ASSERT(result == NRFX_SUCCESS, "nrfx_i2s_next_buffers_set failed with result %d", result);
            }

            processing_buffers_1 = !processing_buffers_1;

            atomic_clear_bit(&processing_in_progress, 0);
        }
    }
}

const uint8_t lrck_pin = 29; // 29; // P0.29 7 aka word select
const uint8_t sdout_pin = 30; // 30; // P0.30 25
const uint8_t sck_pin = 4;  // 31; // aka bitcklock P0.31 26
const uint8_t mck_pin = 31;   // 27; // NRFX_I2S_PIN_NOT_USED;
const uint8_t sdin_pin = 28; // NRFX_I2S_PIN_NOT_USED; // 6; // NRFX_I2S_PIN_NOT_USED;

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

ISR_DIRECT_DECLARE(i2s_isr_handler)
{
    nrfx_i2s_irq_handler();
    ISR_DIRECT_PM(); /* PM done after servicing interrupt for best latency
                      */
    return 1;        /* We should check if scheduling decision should be made */
}

void nrfx_i2s_data_handler(nrfx_i2s_buffers_t const *p_released, uint32_t status)
{
    if (status == NRFX_I2S_STATUS_NEXT_BUFFERS_NEEDED) {
        nrfx_i2s_buffers_t* buffers_to_process = processing_buffers_1 ? &nrfx_i2s_buffers_1 : &nrfx_i2s_buffers_2;
        // TODO: not always int16_t
        int16_t *tx = (int16_t *)buffers_to_process->p_tx_buffer;
        int16_t *rx = (int16_t *)buffers_to_process->p_rx_buffer;
        process_buffers(tx, rx);

        // nrfx_i2s_buffers_t* buffers_to_set = processing_buffers_1 ? &nrfx_i2s_buffers_2 : &nrfx_i2s_buffers_1;
        nrfx_err_t result = nrfx_i2s_next_buffers_set(buffers_to_process);
        if (result != NRFX_SUCCESS) {
            printk("nrfx_i2s_next_buffers_set failed with %d\n", result);
            __ASSERT(result == NRFX_SUCCESS, "nrfx_i2s_next_buffers_set failed with result %d", result);
        }
        processing_buffers_1 = !processing_buffers_1;
        /* if (atomic_test_bit(&processing_in_progress, 0) == false) {
            atomic_set_bit(&processing_in_progress, 0);
            k_sem_give(&processing_thread_semaphore);
        } else {
            // Missed deadline.
        } */
    }
    else if (status == NRFX_I2S_STATUS_TRANSFER_STOPPED) {
        printk("status == NRFX_I2S_STATUS_TRANSFER_STOPPED");
        // __ASSERT(0, "status == NRFX_I2S_STATUS_TRANSFER_STOPPED");
    }
}

// TODO: pass rendering/processing context (ptr) + callback and pass that
// to the processing thread.
nrfx_err_t i2s_start()
{
    // Start a dedicated, high priority thread for audio processing/rendering.
    k_tid_t processing_thread_tid = k_thread_create(
        &processing_thread_data,
        processing_thread_stack_area,
        K_THREAD_STACK_SIZEOF(processing_thread_stack_area),
        processing_thread_entry_point,
        NULL, NULL, NULL,
        PROCESSING_THREAD_PRIORITY, 0, K_NO_WAIT
    );



    // Init and start I2S
    nrfx_err_t result = nrfx_i2s_init(&nrfx_i2s_cfg, &nrfx_i2s_data_handler);
    if (result != NRFX_SUCCESS) {
        printk("nrfx_i2s_init failed with result %d", result);
        __ASSERT(result == NRFX_SUCCESS, "nrfx_i2s_init failed with result %d", result);
    }

    if ((((uint32_t)nrfx_i2s_buffers_1.p_rx_buffer) & 0x3u) != 0u) {
        printk("rx 1 not aligned");
    }
    if ((((uint32_t)nrfx_i2s_buffers_1.p_tx_buffer) & 0x3u) != 0u) {
        printk("tx 1 not aligned");
    }
    if ((((uint32_t)nrfx_i2s_buffers_2.p_rx_buffer) & 0x3u) != 0u) {
        printk("rx 2 not aligned");
    }
    if ((((uint32_t)nrfx_i2s_buffers_2.p_tx_buffer) & 0x3u) != 0u) {
        printk("tx 2 not aligned");
    }

    result = nrfx_i2s_start(&nrfx_i2s_buffers_1, AUDIO_BUFFER_WORD_SIZE, 0);
    if (result != NRFX_SUCCESS) {
        printk("nrfx_i2s_start failed with result %d", result);
        __ASSERT(result == NRFX_SUCCESS, "nrfx_i2s_start failed with result %d", result);
    }

    IRQ_DIRECT_CONNECT(NRFX_IRQ_NUMBER_GET(NRF_I2S), 0, i2s_isr_handler, 0);


    return NRFX_SUCCESS; // TODO: return proper result
}