#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zephyr/usb/usb_device.h>
#include <zephyr/usb/class/usb_audio.h>
#include "usb_audio_app.h"
#include "esb_handle.h"
#include "drv_flash.h"

LOG_MODULE_DECLARE(smart_dongle, CONFIG_ESB_PRX_APP_LOG_LEVEL);


#define USB_AUDIO_STACK_SIZE        2048
#define USB_AUDIO_PRIORITY          3

// K_SEM_DEFINE(esb_sem, 0, 1);


NET_BUF_POOL_FIXED_DEFINE(pool_out, CONFIG_FIFO_FRAME_SPLIT_NUM, USB_FRAME_SIZE_STEREO, 8, net_buf_destroy);

static const struct device *const mic_dev = DEVICE_DT_GET_ONE(usb_audio_mic);

extern struct k_msgq esb_queue;


extern int leds_toggle(void);




/*
* single channel transform to dual channels.
*
* @param[in] src_audio 	Pointer to the single channel source pcm stream.
*			 frames	   	Length of the stram buffer
*			 dst_audio	Pointer to the target dual channel pcm stream
*/
static void mono_to_stereo(int16_t* src_audio, int frames, int16_t* dst_audio) 
{
    for (int i = 0; i < frames; i++) 
    {
        dst_audio[2 * i] = src_audio[i];
        dst_audio[2 * i + 1] = src_audio[i];
    }
}


static void data_write(const struct device *dev)
{
	// LOG_INF("data were requested from the device and may be send to the Host!");
	static uint32_t timeCount = 0;
	if(0 == (timeCount++ % 50))
		leds_toggle();

    int ret = 0;
    void *frame_buffer = NULL;
    size_t data_out_size = 0;
     
    struct net_buf *buf_out;

	buf_out = net_buf_alloc(&pool_out, K_NO_WAIT);
	if (!buf_out) 
	{
		LOG_ERR("Failed to allocate data buffer");
		return;
	}

	if(buf_out->size != 64)
		LOG_INF("netbuf_size = %d\t", buf_out->size);

    if(k_msgq_get(&esb_queue, &frame_buffer, K_NO_WAIT) != 0)
    {
        // LOG_WRN("USB audio TX underrun");
		net_buf_unref(buf_out);
		return;
    }
    
   
    // LOG_HEXDUMP_INF(frame_buffer, 8, "Receive audio queue");
	mono_to_stereo((int16_t*) frame_buffer, MAX_BLOCK_SIZE/2, (int16_t*)buf_out->data);
    
	data_out_size =  buf_out->size;
     /** free the memory slab */
    free_esb_slab_memory(frame_buffer);	

	
#if 1
	 /** USB audio driver handle the pcm stream*/
	if (data_out_size == usb_audio_get_in_frame_size(dev)) 
    {
		ret = usb_audio_send(dev, buf_out, data_out_size);
		if (ret) {
			LOG_WRN("USB TX failed, ret: %d", ret);
			net_buf_unref(buf_out);
		}
		// else
		// {	
		// 	LOG_INF("usb audio send %d bytes succeed!\t", data_out_size);
		// }
	} 
    else 
    {
		LOG_WRN("Wrong size write: %d", data_out_size);
	}
#else
	// net_buf_unref(buf_out);
#endif
}

static void feature_update(const struct device *dev,
			   const struct usb_audio_fu_evt *evt)
{
	LOG_DBG("Control selector %d for channel %d updated",
		evt->cs, evt->channel);
	switch (evt->cs) {
	case USB_AUDIO_FU_MUTE_CONTROL:
	default:
		break;
	}
}



static const struct usb_audio_ops mic_ops = {
	.data_request_cb = data_write,
	.feature_update_cb = feature_update,
};


// void usb_audio_init(void)
// {
// 	int ret;

// 	if (!device_is_ready(mic_dev)) {
// 		LOG_ERR("Device USB Microphone is not ready");
// 		return;
// 	}
// 	LOG_INF("Found USB Microphone Device");

// 	usb_audio_register(mic_dev, &mic_ops);

// 	ret = usb_enable(NULL);
// 	if (ret != 0) {
// 		LOG_ERR("Failed to enable USB");
// 		return;
// 	}

// 	LOG_INF("USB enabled");
// 	LOG_INF("mic_frame_size = %d\t ", usb_audio_get_in_frame_size(mic_dev));
// }



static void esb_audio_data_handle(void *, void *, void *)
{
	int ret;
	static uint32_t total_size = 0;

	// soc_flash_init();

	if (!device_is_ready(mic_dev)) {
		LOG_ERR("Device USB Microphone is not ready");
		return;
	}
	LOG_INF("Found USB Microphone Device");

	usb_audio_register(mic_dev, &mic_ops);

	ret = usb_enable(NULL);
	if (ret != 0) {
		LOG_ERR("Failed to enable USB");
		return;
	}

	LOG_INF("USB enabled");
	LOG_INF("mic_frame_size = %d\t ", usb_audio_get_in_frame_size(mic_dev));

#if 0
    while(1)
    {
        void *frame_buffer = NULL;
		uint8_t buf_out[MAX_BLOCK_SIZE * 2] = {0};
		// size_t data_out_size = 0;

		k_msgq_get(&esb_queue, &frame_buffer, K_FOREVER);
		LOG_INF("received ADPCM decompress stream");
		mono_to_stereo((int16_t*) frame_buffer, MAX_BLOCK_SIZE/2, (int16_t*)buf_out);	
		// if(0 == soc_flash_write(TEST_PARTITION_OFFSET + total_size, buf_out, MAX_BLOCK_SIZE*2))
		// {
		// 	total_size += MAX_BLOCK_SIZE*2;
		// }

        //free the memory slab
		free_esb_slab_memory(frame_buffer);	
    }
#endif
}




K_THREAD_DEFINE(esb_audio_service, USB_AUDIO_STACK_SIZE,
                esb_audio_data_handle, NULL, NULL, NULL,
                K_PRIO_PREEMPT(USB_AUDIO_PRIORITY), 0, 0);




