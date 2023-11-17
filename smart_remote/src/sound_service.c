#include <zephyr/kernel.h>
#include <zephyr/audio/dmic.h>
#include <zephyr/logging/log.h>
#include "sound_service.h"
#include "drv_mic.h"
#include "dvi_adpcm.h"
#include "mic_work_event.h"

LOG_MODULE_REGISTER(sound_service, LOG_LEVEL_INF);

#define SOUND_STACK_SIZE        2048
#define SOUND_PRIORITY          5

/* Milliseconds to wait for a block to be read. */
#define READ_TIMEOUT            1000


// K_MSGQ_DEFINE(sound_queue, 4, BLOCK_COUNT, 4);


static dvi_adpcm_state_t    m_adpcm_state;

static void mic_data_handle(void *, void *, void *)
{
    void *buffer;
	uint32_t size;
    // int ret = 0;

    dvi_adpcm_init_state(&m_adpcm_state);

    LOG_INF("Sound service start, wait for PCM data......");

	/** suspend the thread until we received a start event*/
	k_thread_suspend(k_current_get());

    while(1)
    {
        int frame_size;
	    uint8_t frame_buf[MAX_BLOCK_SIZE/4 + 3] = {0};

        size = read_audio_data(&buffer, READ_TIMEOUT);
        if(size)
        {
            dvi_adpcm_encode(buffer, size, frame_buf, &frame_size,&m_adpcm_state, true);
            LOG_INF("ADPCM buffer got %u bytes", frame_size);
	        LOG_HEXDUMP_INF(frame_buf,frame_size,"ADPCM data");

            free_audio_memory(buffer);

            // send_frame_by_esb();
        }

		// LOG_INF("Sound service start, wait for PCM data......");
		// k_sleep(K_MSEC(1000));
    }
}


K_THREAD_DEFINE(sound_service, SOUND_STACK_SIZE,
                mic_data_handle, NULL, NULL, NULL,
                K_PRIO_PREEMPT(7), 0, 0);


extern void turn_on_off_led(bool onOff);
static bool mic_work_event_handler(const struct app_event_header *aeh)
{
	if (is_mic_work_event(aeh)) 
	{
		struct mic_work_event *event = cast_mic_work_event(aeh);
		if (event->type == MIC_STATUS_START) 
		{
            LOG_INF("Micphone start to work!");
			drv_mic_start();

			k_thread_resume(sound_service);
			
			turn_on_off_led(true);
			// test_pdm_transfer(BLOCK_COUNT);
		}
		else if(event->type == MIC_STATUS_STOP)
		{
            LOG_INF("Micphone stop to work!");
			drv_mic_stop();

			k_thread_suspend(sound_service);

            turn_on_off_led(false);
		}

		return true;
	}
	else
	{
		return false;
	}
}


APP_EVENT_LISTENER(mic_work, mic_work_event_handler);
APP_EVENT_SUBSCRIBE(mic_work, mic_work_event);