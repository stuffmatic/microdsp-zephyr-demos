#include "i2s_nrfx.h"
#include <zephyr.h>
#include <zephyr/kernel/thread.h>

////////////////////////////////////////////////////////////
// Audio buffers
////////////////////////////////////////////////////////////
#define AUDIO_BUFFER_N_FRAMES 256
#define AUDIO_BUFFER_N_CHANNELS 2
#define BYTES_PER_SAMPLE 4 // 24 bit samples are transfered in 32 bit words
#define AUDIO_BUFFER_N_SAMPLES (AUDIO_BUFFER_N_FRAMES * AUDIO_BUFFER_N_CHANNELS)
#define AUDIO_BUFFER_BYTE_SIZE (BYTES_PER_SAMPLE * AUDIO_BUFFER_N_SAMPLES)
#define AUDIO_BUFFER_WORD_SIZE (AUDIO_BUFFER_BYTE_SIZE / 4)

// Floating point scratch buffers.
static float scratch_buffer_out[AUDIO_BUFFER_N_SAMPLES];
static float scratch_buffer_in[AUDIO_BUFFER_N_SAMPLES];

// Use two pairs of rx/tx buffers for double buffering,
// i.e process/render one pair of buffers while the other is
// being transfered.
static int32_t __attribute__((aligned(4))) rx_1[AUDIO_BUFFER_N_SAMPLES];
static int32_t __attribute__((aligned(4))) tx_1[AUDIO_BUFFER_N_SAMPLES];
static int32_t __attribute__((aligned(4))) rx_2[AUDIO_BUFFER_N_SAMPLES];
static int32_t __attribute__((aligned(4))) tx_2[AUDIO_BUFFER_N_SAMPLES];
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
// Indicates if a dropout occurred, i.e that audio buffers were
// not processed in time for the next buffer swap.
atomic_t dropout_occurred = ATOMIC_INIT(0x00);

////////////////////////////////////////////////////////////
// Audio processing thread
////////////////////////////////////////////////////////////
#define PROCESSING_THREAD_STACK_SIZE 2048
// Processing thread is a top priority cooperative thread
#define PROCESSING_THREAD_PRIORITY -CONFIG_NUM_COOP_PRIORITIES
K_THREAD_STACK_DEFINE(processing_thread_stack_area, PROCESSING_THREAD_STACK_SIZE);
struct k_thread processing_thread_data;

uint32_t processing_sem_give_time = 0;

static void processing_thread_entry_point(void *p1, void *p2, void *p3) {
    while (true) {
        if (k_sem_take(&processing_thread_semaphore, K_FOREVER) == 0) {
            audio_processing_options_t* processing_options = (audio_processing_options_t*)p1;
            if (atomic_test_bit(&dropout_occurred, 0)) {
                processing_options->dropout_cb(processing_options);
            }
            uint32_t processing_sem_take_time = k_cycle_get_32();
            uint32_t cycles_spent = processing_sem_take_time - processing_sem_give_time;
            uint32_t ns_spent = k_cyc_to_ns_ceil32(cycles_spent);
            // printk("processing thread start took %d ns\n", ns_spent);

            if (!atomic_test_bit(&processing_in_progress, 0)) {
                printk("processing_in_progress is not set");
                __ASSERT(atomic_test_bit(&processing_in_progress, 0), "processing_in_progress is not set");
            }

            nrfx_i2s_buffers_t* buffers_to_process = processing_buffers_1 ? &nrfx_i2s_buffers_1 : &nrfx_i2s_buffers_2;
            // TODO: not always int32_t
            int32_t *tx = (int32_t *)buffers_to_process->p_tx_buffer;
            int32_t *rx = (int32_t *)buffers_to_process->p_rx_buffer;

            // Convert incoming audio from PCM
            for (int i = 0; i < AUDIO_BUFFER_N_SAMPLES; i++) {
                scratch_buffer_in[i] = rx[i] / 8388607.0;
            }

            memset(scratch_buffer_out, 0, AUDIO_BUFFER_N_SAMPLES * sizeof(float));
            processing_options->processing_cb(
                processing_options->processing_cb_data,
                AUDIO_BUFFER_N_FRAMES,
                AUDIO_BUFFER_N_CHANNELS,
                scratch_buffer_out,
                scratch_buffer_in
            );

            // Convert outgoing audio to PCM
            for (int i = 0; i < AUDIO_BUFFER_N_SAMPLES; i++) {
                tx[i] = scratch_buffer_out[i] * 8388607.0; // TODO: saturated?
            }

            nrfx_i2s_buffers_t* buffers_to_set = processing_buffers_1 ? &nrfx_i2s_buffers_2 : &nrfx_i2s_buffers_1;
            nrfx_err_t result = nrfx_i2s_next_buffers_set(buffers_to_process);
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
const uint8_t mck_pin = NRFX_I2S_PIN_NOT_USED; // 31;   // 27; // NRFX_I2S_PIN_NOT_USED;
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
    .sample_width = NRF_I2S_SWIDTH_24BIT,
    .mck_setup = NRF_I2S_MCK_32MDIV15,
    .ratio = NRF_I2S_RATIO_48X,
};

ISR_DIRECT_DECLARE(i2s_isr_handler)
{
    nrfx_i2s_irq_handler();
    // PM done after servicing interrupt for best latency
    ISR_DIRECT_PM();
    // We should check if scheduling decision should be made
    return 1;
}

void nrfx_i2s_data_handler(nrfx_i2s_buffers_t const *p_released, uint32_t status)
{
    if (status == NRFX_I2S_STATUS_NEXT_BUFFERS_NEEDED) {
        if (atomic_test_bit(&processing_in_progress, 0) == false) {
            atomic_set_bit(&processing_in_progress, 0);
            processing_sem_give_time = k_cycle_get_32();
            k_sem_give(&processing_thread_semaphore);
        } else {
            // Missed deadline.
            atomic_set_bit(&dropout_occurred, 0);
        }
    }
    else if (status == NRFX_I2S_STATUS_TRANSFER_STOPPED) {
        //
    }
}

nrfx_err_t i2s_start(audio_processing_options_t* processing_options)
{
    // Start a dedicated, high priority thread for audio processing.
    k_tid_t processing_thread_tid = k_thread_create(
        &processing_thread_data,
        processing_thread_stack_area,
        K_THREAD_STACK_SIZEOF(processing_thread_stack_area),
        processing_thread_entry_point,
        processing_options, NULL, NULL,
        PROCESSING_THREAD_PRIORITY, 0, K_NO_WAIT
    );

    IRQ_DIRECT_CONNECT(NRFX_IRQ_NUMBER_GET(NRF_I2S), 0, i2s_isr_handler, 0);

    // Init and start I2S
    nrfx_err_t result = nrfx_i2s_init(&nrfx_i2s_cfg, &nrfx_i2s_data_handler);
    if (result != NRFX_SUCCESS) {
        printk("nrfx_i2s_init failed with result %d", result);
        __ASSERT(result == NRFX_SUCCESS, "nrfx_i2s_init failed with result %d", result);
    }

    result = nrfx_i2s_start(&nrfx_i2s_buffers_1, AUDIO_BUFFER_WORD_SIZE, 0);
    if (result != NRFX_SUCCESS) {
        printk("nrfx_i2s_start failed with result %d", result);
        __ASSERT(result == NRFX_SUCCESS, "nrfx_i2s_start failed with result %d", result);
    }

    return NRFX_SUCCESS; // TODO: return proper result
}