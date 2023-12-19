#include <zephyr/kernel.h>
#include <zephyr/audio/dmic.h>
#include <zephyr/logging/log.h>
#include "app_esb.h"
#include "drv_mic.h"
#include "dvi_adpcm.h"
#include <caf/events/click_event.h>
#include "mic_work_event.h"

#define MODULE mic_work
#include <caf/events/module_state_event.h>
#include "app_timeslot.h"

LOG_MODULE_REGISTER(sound_service, LOG_LEVEL_INF);

#define SOUND_STACK_SIZE        2048
#define SOUND_PRIORITY          5


#define ESB_BLOCK_SIZE            (CONFIG_ESB_MAX_PAYLOAD_LENGTH - CONFIG_ESB_MAX_PAYLOAD_LENGTH % (MAX_BLOCK_SIZE/4 + 3))

/* Milliseconds to wait for a block to be read. */
#define READ_TIMEOUT            SYS_FOREVER_MS


static dvi_adpcm_state_t    m_adpcm_state;


static void mic_data_handle(void *, void *, void *)
{
	int err;

    void *buffer;
	uint32_t size;
	static uint8_t esb_tx_buf[CONFIG_ESB_MAX_PAYLOAD_LENGTH] = {5};
	static uint8_t esb_total_size = 0;

    dvi_adpcm_init_state(&m_adpcm_state);

	drv_audio_init();

	err = app_esb_init(APP_ESB_MODE_PTX);
	if (err) {
		LOG_ERR("app_esb init failed (err %d)", err);
		return;
	}
	
	timeslot_init();

    LOG_INF("Sound service start, wait for PCM data......");

    while(1)
    {
	#if 1
        int frame_size;
	    char frame_buf[MAX_BLOCK_SIZE/4 + 3] = {0};

        size = read_audio_data(&buffer, READ_TIMEOUT);
        if(size)
        {
			dvi_adpcm_encode(buffer, size, frame_buf, &frame_size,&m_adpcm_state, true);

			memcpy(&(esb_tx_buf[esb_total_size]), frame_buf, frame_size);
			esb_total_size += frame_size;

			if(esb_total_size >= CONFIG_ESB_MAX_PAYLOAD_LENGTH)
			{
				esb_package_enqueue(esb_tx_buf, CONFIG_ESB_MAX_PAYLOAD_LENGTH);
				memset(esb_tx_buf, 0, CONFIG_ESB_MAX_PAYLOAD_LENGTH);

				esb_total_size = 0;
			}
			else
			{
				if (get_timeslot_status()) 
					pull_packet_from_tx_msgq();
			}
			
            free_audio_memory(buffer);
		}
		else
		{
			LOG_INF("Sound service resume, wait for PCM data......");
			k_sleep(K_MSEC(1000));
		}
		
	#else
		esb_package_enqueue(esb_tx_buf, CONFIG_ESB_MAX_PAYLOAD_LENGTH);
		LOG_INF("Sound service resume, wait for PCM data......");
		k_sleep(K_SECONDS(1));
	#endif
    }
}



K_THREAD_DEFINE(sound_service, SOUND_STACK_SIZE,
                mic_data_handle, NULL, NULL, NULL,
                K_PRIO_PREEMPT(7), K_ESSENTIAL, 0);



// extern void turn_on_off_led(bool onOff);
static volatile bool button_flag = false;
static bool handle_click_event(const struct click_event *event)
{
	if ((event->key_id == CONFIG_DESKTOP_BLE_PEER_CONTROL_BUTTON) &&
	    (event->click == CLICK_SHORT)) 
	{
		button_flag = !button_flag;
		// wake up device and trigger micphone to work
		if(button_flag)
		{
			LOG_INF("Micphone start to work!");
			drv_mic_start();
		}
		else
		{
			LOG_INF("Micphone stop to work!");
			drv_mic_stop();
		}
	}

	return true;
}




static bool app_event_handler(const struct app_event_header *aeh)
{
	if (is_module_state_event(aeh)) 
	{
		const struct module_state_event *event = cast_module_state_event(aeh);

		if (check_state(event, MODULE_ID(main), MODULE_STATE_READY)) {
			static bool initialized;

			__ASSERT_NO_MSG(!initialized);
			module_set_state(MODULE_STATE_READY);
			initialized = true;
		}

		return false;
	}

	if (is_click_event(aeh)) 
	{
		return handle_click_event(cast_click_event(aeh));
	}

	/* If event is unhandled, unsubscribe. */
	__ASSERT_NO_MSG(false);

	return false;
}


APP_EVENT_LISTENER(MODULE, app_event_handler);
APP_EVENT_SUBSCRIBE(MODULE, module_state_event);
APP_EVENT_SUBSCRIBE(MODULE, click_event);
