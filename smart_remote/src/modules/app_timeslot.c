#include <zephyr/kernel.h>
#include "app_timeslot.h"
#include <zephyr/irq.h>
#include <zephyr/sys/ring_buffer.h>
#include <hal/nrf_timer.h>
#include <mpsl_timeslot.h>
#include <mpsl.h>
#include <zephyr/logging/log.h>
#include "app_esb.h"

LOG_MODULE_REGISTER(timeslot, LOG_LEVEL_INF);

#define TIMESLOT_REQUEST_TIMEOUT_US  1000000
#define TIMESLOT_LENGTH_US           3000//10000
#define TIMESLOT_EXT_LENGTH_US       1000//10000
#define TIMESLOT_EXT_MARGIN_MARGIN	 100//1000
#define TIMESLOT_REQ_EARLIEST_MARGIN 20
#define TIMER_EXPIRY_US_EARLY 		 (TIMESLOT_LENGTH_US - MPSL_TIMESLOT_EXTENSION_MARGIN_MIN_US - TIMESLOT_EXT_MARGIN_MARGIN)
#define TIMER_EXPIRY_REQ			 (TIMESLOT_LENGTH_US - MPSL_TIMESLOT_EXTENSION_MARGIN_MIN_US - TIMESLOT_REQ_EARLIEST_MARGIN)

#define MPSL_THREAD_PRIO             CONFIG_MPSL_THREAD_COOP_PRIO
#define STACKSIZE                    1024
#define THREAD_PRIORITY              K_LOWEST_APPLICATION_THREAD_PRIO



static volatile bool m_in_timeslot = false;

void RADIO_IRQHandler(void);

// Requests and callbacks to be run serialized from an SWI interrupt
enum mpsl_timeslot_call {
	REQ_OPEN_SESSION,
	REQ_MAKE_REQUEST,
	REQ_CLOSE_SESSION,
};

// Timeslot request
static mpsl_timeslot_request_t timeslot_request_earliest = {
	.request_type = MPSL_TIMESLOT_REQ_TYPE_EARLIEST,
	.params.earliest.hfclk = MPSL_TIMESLOT_HFCLK_CFG_NO_GUARANTEE,
	.params.earliest.priority = MPSL_TIMESLOT_PRIORITY_NORMAL,
	.params.earliest.length_us = TIMESLOT_LENGTH_US,
	.params.earliest.timeout_us = TIMESLOT_REQUEST_TIMEOUT_US
};

static mpsl_timeslot_signal_return_param_t signal_callback_return_param;

static void set_timeslot_active_status(bool active);

/* Message queue for requesting MPSL API calls to non-preemptible thread */
K_MSGQ_DEFINE(mpsl_api_msgq, sizeof(enum mpsl_timeslot_call), 10, 4);

static void schedule_request(enum mpsl_timeslot_call call)
{
	int err;
	enum mpsl_timeslot_call api_call = call;
	err = k_msgq_put(&mpsl_api_msgq, &api_call, K_NO_WAIT);
	if (err) {
		LOG_ERR("Message sent error: %d", err);
		k_oops();
	}
}


static mpsl_timeslot_signal_return_param_t *mpsl_timeslot_callback(mpsl_timeslot_session_id_t session_id, uint32_t signal_type)
{
	(void) session_id; // unused parameter
	static bool timeslot_extension_failed;
	mpsl_timeslot_signal_return_param_t *p_ret_val = NULL;
	switch (signal_type) {
		case MPSL_TIMESLOT_SIGNAL_START:
			signal_callback_return_param.callback_action = MPSL_TIMESLOT_SIGNAL_ACTION_NONE;
			p_ret_val = &signal_callback_return_param;

			timeslot_extension_failed = false;

			// Reset the radio to make sure no configuration remains from BLE
			NVIC_ClearPendingIRQ(RADIO_IRQn);
			NRF_RADIO->POWER = RADIO_POWER_POWER_Disabled << RADIO_POWER_POWER_Pos;
			NRF_RADIO->POWER = RADIO_POWER_POWER_Enabled << RADIO_POWER_POWER_Pos;
			NVIC_ClearPendingIRQ(RADIO_IRQn);

			nrf_timer_bit_width_set(NRF_TIMER0, NRF_TIMER_BIT_WIDTH_32);
			
			nrf_timer_cc_set(NRF_TIMER0, NRF_TIMER_CC_CHANNEL0, TIMER_EXPIRY_US_EARLY);
			nrf_timer_int_enable(NRF_TIMER0, NRF_TIMER_INT_COMPARE0_MASK);

			nrf_timer_cc_set(NRF_TIMER0, NRF_TIMER_CC_CHANNEL1, TIMER_EXPIRY_REQ);
			nrf_timer_int_enable(NRF_TIMER0, NRF_TIMER_INT_COMPARE1_MASK);

			set_timeslot_active_status(true);

			break;

		case MPSL_TIMESLOT_SIGNAL_TIMER0:
			if(nrf_timer_event_check(NRF_TIMER0, NRF_TIMER_EVENT_COMPARE0)) {
				nrf_timer_int_disable(NRF_TIMER0, NRF_TIMER_INT_COMPARE0_MASK);
				nrf_timer_event_clear(NRF_TIMER0, NRF_TIMER_EVENT_COMPARE0);

				signal_callback_return_param.callback_action = MPSL_TIMESLOT_SIGNAL_ACTION_EXTEND;
				// signal_callback_return_param.params.extend.length_us = TIMESLOT_LENGTH_US;	
				signal_callback_return_param.params.extend.length_us = TIMESLOT_EXT_LENGTH_US;
			}
			else if(nrf_timer_event_check(NRF_TIMER0, NRF_TIMER_EVENT_COMPARE1)) {
				nrf_timer_int_disable(NRF_TIMER0, NRF_TIMER_INT_COMPARE1_MASK);
				nrf_timer_event_clear(NRF_TIMER0, NRF_TIMER_EVENT_COMPARE1);

				if(timeslot_extension_failed) {
					signal_callback_return_param.callback_action = MPSL_TIMESLOT_SIGNAL_ACTION_REQUEST;
					signal_callback_return_param.params.request.p_next = &timeslot_request_earliest;
				} else {
					signal_callback_return_param.callback_action = MPSL_TIMESLOT_SIGNAL_ACTION_NONE;
				}
			}
			p_ret_val = &signal_callback_return_param;
			break;

		case MPSL_TIMESLOT_SIGNAL_EXTEND_SUCCEEDED:
			signal_callback_return_param.callback_action = MPSL_TIMESLOT_SIGNAL_ACTION_NONE;

			// Set next trigger time to be the current + Timer expiry early
			uint32_t current_cc = nrf_timer_cc_get(NRF_TIMER0, NRF_TIMER_CC_CHANNEL0);
			nrf_timer_bit_width_set(NRF_TIMER0, NRF_TIMER_BIT_WIDTH_32);
			// nrf_timer_cc_set(NRF_TIMER0, NRF_TIMER_CC_CHANNEL0, current_cc + TIMESLOT_LENGTH_US);
			nrf_timer_cc_set(NRF_TIMER0, NRF_TIMER_CC_CHANNEL0, current_cc + TIMESLOT_EXT_LENGTH_US);
			nrf_timer_int_enable(NRF_TIMER0, NRF_TIMER_INT_COMPARE0_MASK);

			current_cc = nrf_timer_cc_get(NRF_TIMER0, NRF_TIMER_CC_CHANNEL1);
			nrf_timer_bit_width_set(NRF_TIMER0, NRF_TIMER_BIT_WIDTH_32);
			// nrf_timer_cc_set(NRF_TIMER0, NRF_TIMER_CC_CHANNEL1, current_cc + TIMESLOT_LENGTH_US);
			nrf_timer_cc_set(NRF_TIMER0, NRF_TIMER_CC_CHANNEL1, current_cc + TIMESLOT_EXT_LENGTH_US);
			nrf_timer_int_enable(NRF_TIMER0, NRF_TIMER_INT_COMPARE1_MASK);

			p_ret_val = &signal_callback_return_param;

			break;

		case MPSL_TIMESLOT_SIGNAL_EXTEND_FAILED:
			// LOG_WRN("request timeslot extended failed!");
			signal_callback_return_param.callback_action = MPSL_TIMESLOT_SIGNAL_ACTION_NONE;
			timeslot_extension_failed = true;
			p_ret_val = &signal_callback_return_param;
			set_timeslot_active_status(false);

			break;

		case MPSL_TIMESLOT_SIGNAL_RADIO:
			signal_callback_return_param.callback_action = MPSL_TIMESLOT_SIGNAL_ACTION_NONE;
			p_ret_val = &signal_callback_return_param;

			// We have to manually call the RADIO IRQ handler when the RADIO signal occurs
			if(m_in_timeslot) RADIO_IRQHandler();
			else {
				NVIC_ClearPendingIRQ(RADIO_IRQn);
				NVIC_DisableIRQ(RADIO_IRQn);
			}
			break;

		case MPSL_TIMESLOT_SIGNAL_OVERSTAYED:
			LOG_WRN("something overstayed!");
			signal_callback_return_param.callback_action = MPSL_TIMESLOT_SIGNAL_ACTION_END;
			p_ret_val = &signal_callback_return_param;
			set_timeslot_active_status(false);

			break;

		case MPSL_TIMESLOT_SIGNAL_CANCELLED:
			LOG_DBG("something cancelled!");
			signal_callback_return_param.callback_action = MPSL_TIMESLOT_SIGNAL_ACTION_NONE;
			p_ret_val = &signal_callback_return_param;
			set_timeslot_active_status(false);
			
			// In this case returning SIGNAL_ACTION_REQUEST causes hardfault. We have to request a new timeslot instead, from thread context. 
			schedule_request(REQ_MAKE_REQUEST);
			break;

		case MPSL_TIMESLOT_SIGNAL_BLOCKED:
			LOG_INF("something blocked!");
			signal_callback_return_param.callback_action = MPSL_TIMESLOT_SIGNAL_ACTION_NONE;
			p_ret_val = &signal_callback_return_param;
			set_timeslot_active_status(false);

			// Request a new timeslot in this case
			schedule_request(REQ_MAKE_REQUEST);
			break;

		case MPSL_TIMESLOT_SIGNAL_INVALID_RETURN:
			LOG_WRN("something gave invalid return");
			signal_callback_return_param.callback_action = MPSL_TIMESLOT_SIGNAL_ACTION_END;
			p_ret_val = &signal_callback_return_param;
			set_timeslot_active_status(false);

			break;

		case MPSL_TIMESLOT_SIGNAL_SESSION_IDLE:
			LOG_INF("idle");

			signal_callback_return_param.callback_action = MPSL_TIMESLOT_SIGNAL_ACTION_NONE;
			p_ret_val = &signal_callback_return_param;
			set_timeslot_active_status(false);

			break;

		case MPSL_TIMESLOT_SIGNAL_SESSION_CLOSED:
			LOG_INF("Session closed");

			signal_callback_return_param.callback_action = MPSL_TIMESLOT_SIGNAL_ACTION_NONE;
			p_ret_val = &signal_callback_return_param;
			set_timeslot_active_status(false);

			break;

		default:
			LOG_ERR("unexpected signal: %u", signal_type);
			k_oops();
			break;
	}
	
	return p_ret_val;
}




void timeslot_init(void)
{

	schedule_request(REQ_OPEN_SESSION);

	schedule_request(REQ_MAKE_REQUEST);
}


static void set_timeslot_active_status(bool active)
{
	if (active) 
	{
		if (!m_in_timeslot)
		{
			m_in_timeslot = true;
			app_esb_resume();
		}
		// pull_packet_from_tx_msgq();
	} 
	else 
	{
		if (m_in_timeslot) 
		{
			m_in_timeslot = false;
			app_esb_suspend();
		}
	}
}

bool get_timeslot_status(void)
{
	return m_in_timeslot;
}

/* To ensure thread safe operation, call all MPSL APIs from a non-preemptible
 * thread.
 */
static void mpsl_nonpreemptible_thread(void)
{
	int err;
	enum mpsl_timeslot_call api_call = 0;

	/* Initialize to invalid session id */
	mpsl_timeslot_session_id_t session_id = 0xFFu;

	while (1) {
		if (k_msgq_get(&mpsl_api_msgq, &api_call, K_FOREVER) == 0) {
			switch (api_call) {
				case REQ_OPEN_SESSION:
					err = mpsl_timeslot_session_open(mpsl_timeslot_callback, &session_id);
					if (err) {
						LOG_ERR("Timeslot session open error: %d", err);
						k_oops();
					}
					break;
				case REQ_MAKE_REQUEST:
					err = mpsl_timeslot_request(session_id, &timeslot_request_earliest);
					if (err) {
						LOG_ERR("Timeslot request error: %d", err);
						k_oops();
					}
					break;
				case REQ_CLOSE_SESSION:
					err = mpsl_timeslot_session_close(session_id);
					if (err) {
						LOG_ERR("Timeslot session close error: %d", err);
						k_oops();
					}
					break;
				default:
					LOG_ERR("Wrong timeslot API call");
					k_oops();
					break;
			}
		}
	}
}


K_THREAD_DEFINE(mpsl_nonpreemptible_thread_id, STACKSIZE,
		mpsl_nonpreemptible_thread, NULL, NULL, NULL,
		K_PRIO_COOP(MPSL_THREAD_PRIO), 0, 0);