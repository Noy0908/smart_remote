#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include "usb_audio.h"
#include "esb_handle.h"

LOG_MODULE_DECLARE(smart_dongle, CONFIG_ESB_PRX_APP_LOG_LEVEL);


#define USB_AUDIO_STACK_SIZE        2048
#define USB_AUDIO_PRIORITY          3

// K_SEM_DEFINE(esb_sem, 0, 1);


extern struct k_msgq esb_queue;


static void esb_audio_data_handle(void *, void *, void *)
{
	int err = 0;

    LOG_INF("adpcm decoder thread start......");

    while(1)
    {
        int16_t *frame_buffer = NULL;

		k_msgq_get(&esb_queue, &frame_buffer, K_FOREVER);

        /** USB audio driver handle the pcm stream*/
        LOG_HEXDUMP_INF(frame_buffer, 8, "Receive audio queue");



        //free the memory slab
		free_esb_slab_memory(frame_buffer);	
    }
}





K_THREAD_DEFINE(esb_audio_service, USB_AUDIO_STACK_SIZE,
                esb_audio_data_handle, NULL, NULL, NULL,
                K_PRIO_PREEMPT(USB_AUDIO_PRIORITY), 0, 0);




