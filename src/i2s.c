#include "i2s.h"
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

// Floating point mono scratch buffers.
static float scratch_buffer_out[AUDIO_BUFFER_N_FRAMES];
static float scratch_buffer_in[AUDIO_BUFFER_N_FRAMES];

// Use two pairs of rx/tx buffers for double buffering,
// i.e process/render one pair of buffers while the other is
// being received/transfered.
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
            audio_callbacks_t* audio_callbacks = (audio_callbacks_t*)p1;
            if (atomic_test_bit(&dropout_occurred, 0)) {
                audio_callbacks->dropout_cb(audio_callbacks->cb_data);
                atomic_clear_bit(&dropout_occurred, 0);
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
            int32_t *tx = (int32_t *)buffers_to_process->p_tx_buffer;
            int32_t *rx = (int32_t *)buffers_to_process->p_rx_buffer;

            // Convert incoming audio from PCM, discarding right channel
            int32_t rx_min = 0;
            int32_t rx_max = 0;
            float max_sq = 0.0;
            for (int i = 0; i < AUDIO_BUFFER_N_FRAMES; i++) {
                int32_t rx_l = rx[2 * i];
                int32_t rx_r = rx[2 * i + 1];
                if (rx_l < rx_min) {
                    rx_min = rx_l;
                }
                if (rx_r < rx_min) {
                    rx_min = rx_r;
                }
                if (rx_l > rx_max) {
                    rx_max = rx_l;
                }
                if (rx_r > rx_max) {
                    rx_max = rx_r;
                }
                float val_f = rx_l / (float)8388607.0;
                if (val_f * val_f > max_sq) {
                    max_sq = val_f * val_f;
                }
                scratch_buffer_in[i] = val_f;
            }
            // printk("rx_min %d, rx_max %d, max_sq %f\n", rx_min, rx_max, max_sq);

            memset(scratch_buffer_out, 0, AUDIO_BUFFER_N_FRAMES * sizeof(float));
            audio_callbacks->processing_cb(
                audio_callbacks->cb_data,
                AUDIO_BUFFER_N_FRAMES,
                scratch_buffer_out,
                scratch_buffer_in
            );

            // Convert outgoing audio to PCM
            for (int i = 0; i < AUDIO_BUFFER_N_FRAMES; i++) {
                int32_t out_val = scratch_buffer_out[i] * 8388607.0f;
                tx[2 * i] = out_val;
                tx[2 * i + 1] = out_val;
            }

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

nrfx_err_t i2s_start(audio_cfg_t* audio_cfg, audio_callbacks_t* audio_callbacks)
{
    // Start a dedicated, high priority thread for audio processing.
    k_tid_t processing_thread_tid = k_thread_create(
        &processing_thread_data,
        processing_thread_stack_area,
        K_THREAD_STACK_SIZEOF(processing_thread_stack_area),
        processing_thread_entry_point,
        audio_callbacks, NULL, NULL,
        PROCESSING_THREAD_PRIORITY, 0, K_NO_WAIT
    );

    // Init and start I2S
    IRQ_DIRECT_CONNECT(NRFX_IRQ_NUMBER_GET(NRF_I2S0), 0, i2s_isr_handler, 0);

    nrfx_i2s_config_t nrfx_i2s_cfg = {
        .sck_pin = audio_cfg->sck_pin,
        .lrck_pin = audio_cfg->lrck_pin,
        .mck_pin = audio_cfg->mck_pin,
        .sdout_pin = audio_cfg->sdout_pin,
        .sdin_pin = audio_cfg->sdin_pin,
        .mck_setup = audio_cfg->mck_setup,
        .ratio = audio_cfg->ratio,
        .irq_priority = NRFX_I2S_DEFAULT_CONFIG_IRQ_PRIORITY,
        .mode = NRF_I2S_MODE_MASTER,
        .format = NRF_I2S_FORMAT_I2S,
        .alignment = NRF_I2S_ALIGN_LEFT, // only applies to input. output is always left aligned per https://infocenter.nordicsemi.com/index.jsp?topic=%2Fcom.nordic.infocenter.nrf52832.ps.v1.1%2Fi2s.html
        .channels = NRF_I2S_CHANNELS_STEREO,
        .sample_width = NRF_I2S_SWIDTH_24BIT
    };

    nrfx_err_t result = nrfx_i2s_init(&nrfx_i2s_cfg, &nrfx_i2s_data_handler);
    if (result != NRFX_SUCCESS) {
        printk("nrfx_i2s_init failed with result %d", result);
        __ASSERT(result == NRFX_SUCCESS, "nrfx_i2s_init failed with result %d", result);
        return result;
    }

    result = nrfx_i2s_start(&nrfx_i2s_buffers_1, AUDIO_BUFFER_WORD_SIZE, 0);
    if (result != NRFX_SUCCESS) {
        printk("nrfx_i2s_start failed with result %d", result);
        __ASSERT(result == NRFX_SUCCESS, "nrfx_i2s_start failed with result %d", result);
        return result;
    }

    return NRFX_SUCCESS;
}