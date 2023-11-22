#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <esb.h>
#include "dvi_adpcm.h"
#include "usb_audio.h"


LOG_MODULE_DECLARE(smart_dongle, CONFIG_ESB_PRX_APP_LOG_LEVEL);


#define USB_AUDIO_STACK_SIZE        2048
#define USB_AUDIO_PRIORITY          3



#define MAX_SAMPLE_RATE     16000
#define SAMPLE_BIT_WIDTH    16
#define BYTES_PER_SAMPLE    sizeof(int16_t)

/* Size of a block for 10 ms of audio data. */
#define BLOCK_SIZE(_sample_rate, _number_of_channels) \
	(BYTES_PER_SAMPLE * (_sample_rate / 100) * _number_of_channels)

#define MAX_BLOCK_SIZE              BLOCK_SIZE(MAX_SAMPLE_RATE, 1)
// #define ESB_BLOCK_SIZE              (MAX_BLOCK_SIZE/4 + 3)

#define ESB_BLOCK_SIZE              MAX_BLOCK_SIZE
#define ESB_BLOCK_COUNT             30

/* Driver will allocate blocks from this slab to save adpcm data into them.
 * Application, after getting a given block then push it to the esb send message queue,
 * needs to free that block.
 */

K_MEM_SLAB_DEFINE(esb_slab, ESB_BLOCK_SIZE, ESB_BLOCK_COUNT, 4);

K_MSGQ_DEFINE(esb_queue, 4, ESB_BLOCK_COUNT, 4);

K_SEM_DEFINE(esb_sem, 0, 1);

static dvi_adpcm_state_t    m_adpcm_state;

/** this pointer variable used for transport the message queue to ESB thread.*/
void *block_ptr = NULL;

static struct esb_payload rx_payload;

static void esb_audio_data_handle(void *, void *, void *)
{
	int err = 0;
    void *buffer;
	uint32_t size;
    // int ret = 0;

    dvi_adpcm_init_state(&m_adpcm_state);

    LOG_INF("Sound service start, wait for PCM data......");

    while(1)
    {
        uint8_t *frame_buffer = NULL;
        int frame_size = 0;

        k_sem_take(&esb_sem, K_FOREVER);

        if (esb_read_rx_payload(&rx_payload) == 0) 
        {
			LOG_INF("Packet received, len %d : "
				"0x%02x, 0x%02x, 0x%02x, 0x%02x ",				
				rx_payload.length, rx_payload.data[0],
				rx_payload.data[1], rx_payload.data[2],
				rx_payload.data[3] );

            // if(k_mem_slab_alloc(&esb_slab, (void **) &block_ptr, K_MSEC(100)) == 0)
            // {
            //     // memcpy(payload->data, rx_fifo.payload[rx_fifo.front]->data, payload->length);
            //     dvi_adpcm_decode(rx_payload.data, rx_payload.length, block_ptr, ESB_BLOCK_SIZE, &m_adpcm_state);
            //    /** send the PCM data to USB audio driver*/
            //     err = k_msgq_put(&esb_queue, &block_ptr, K_MSEC(100));
			// 	if (err) {
			// 		LOG_ERR("Message sent error: %d", err);
			// 	}
            // }
            // else 
			// {
			// 	LOG_ERR("Memory allocation time-out");
			// }	
		} 
        else 
        {
			LOG_ERR("Error while reading esb rx packet");
		}

		// k_msgq_get(&esb_queue, &frame_buffer, K_FOREVER);
    }
}





K_THREAD_DEFINE(esb_audio_service, USB_AUDIO_STACK_SIZE,
                esb_audio_data_handle, NULL, NULL, NULL,
                K_PRIO_PREEMPT(USB_AUDIO_PRIORITY), 0, 0);




