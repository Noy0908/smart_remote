/*
 * Copyright (c) 2019 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

/*
MODIFIED SAMPLE TO INCLUDE EXTENSIONS ++
*/

#include <zephyr/kernel.h>
#include <zephyr/console/console.h>
#include <string.h>
#include <zephyr/sys/printk.h>
#include <zephyr/types.h>
#include <zephyr/logging/log.h>

#include <zephyr/drivers/gpio.h> 

#include <dk_buttons_and_leds.h>
#include "app_bt_mouse.h"
#include "app_timeslot.h"
#include "app_esb.h"

#define FW_VERSION		"1.0.0"

#define MOV_LED			DK_LED1
#define TIMESLOT_LED	DK_LED2

/* Key used to turn on circle drawing*/
#define KEY_ON_MASK		DK_BTN1_MSK
/* Key used to turn off circle drawing */
#define KEY_OFF_MASK    DK_BTN2_MSK

#define ESB_PKT_SIZE	72

LOG_MODULE_REGISTER(main, CONFIG_ESB_BT_LOG_LEVEL);


/*
 * Get button configuration from the devicetree sw0 alias. This is mandatory.
 */
#define SW0_NODE	DT_ALIAS(sw0)
#if !DT_NODE_HAS_STATUS(SW0_NODE, okay)
#error "Unsupported board: sw0 devicetree alias is not defined"
#endif
static const struct gpio_dt_spec button = GPIO_DT_SPEC_GET_OR(SW0_NODE, gpios,{0});
static struct gpio_callback button_cb_data;


static const uint8_t  dbg_pins[] = {31, 30, 29, 28, 04, 03};

// static const struct device *dbg_port= DEVICE_DT_GET(DT_NODELABEL(gpio0));
static const struct device *button_port= DEVICE_DT_GET(DT_NODELABEL(gpio0));

static const struct gpio_dt_spec leds[] = {
	// GPIO_DT_SPEC_GET(DT_ALIAS(led0), gpios),
	GPIO_DT_SPEC_GET(DT_ALIAS(led1), gpios),
	GPIO_DT_SPEC_GET(DT_ALIAS(led2), gpios),
};

static bool is_bt_connected = false;

static uint8_t esb_tx_buf[ESB_PKT_SIZE] = {0};
static int tx_counter = 0;

/* Callback function signalling that a timeslot is started or stopped */
void on_timeslot_start_stop(timeslot_callback_type_t type)
{

	switch (type) {
		case APP_TS_STARTED:
		// gpio_pin_set(dbg_port, dbg_pins[0], 1);
		// 	dk_set_led_off(TIMESLOT_LED);
			app_esb_resume();

			if(is_bt_connected) {

		// gpio_pin_set(dbg_port, dbg_pins[2], 1);	
				memcpy(esb_tx_buf, (uint8_t*)&tx_counter, 4);
				int err = app_esb_send(esb_tx_buf, ESB_PKT_SIZE);
				if (err < 0) {
					LOG_INF("ESB TX upload failed (err %i)", err);
				}
				else {
					LOG_INF("ESB TX upload %.2x-%.2x", (tx_counter& 0xFF), ((tx_counter >> 8) & 0xFF));
					tx_counter++;
				}
		// gpio_pin_set(dbg_port, dbg_pins[2], 0);	
		
			}

			break;
		case APP_TS_STOPPED:
		// gpio_pin_set(dbg_port, dbg_pins[0], 0);
			// dk_set_led_on(TIMESLOT_LED);
			app_esb_suspend();
			break;
	}
}

void on_esb_callback(app_esb_event_t *event)
{
	switch(event->evt_type) {
		case APP_ESB_EVT_TX_SUCCESS:

		// gpio_pin_set(dbg_port, dbg_pins[3], 1);
			LOG_INF("ESB TX success");
		// gpio_pin_set(dbg_port, dbg_pins[3], 0);
			break;
		case APP_ESB_EVT_TX_FAIL:
			LOG_INF("ESB TX failed");
			break;
		case APP_ESB_EVT_RX:
			LOG_INF("ESB RX: 0x%.2x-0x%.2x-0x%.2x-0x%.2x", event->buf[0], event->buf[1], event->buf[2], event->buf[3]);
			break;
		default:
			LOG_ERR("Unknown APP ESB event!");
			break;
	}
}


void on_bt_callback(app_bt_event_t *event)
{
	switch(event->evt_type) {
		case APP_BT_EVT_CONNECTED:
			is_bt_connected = true;
			LOG_INF("BT CONNECTED");
			break;
		case APP_BT_EVT_DISCONNECTED:
			is_bt_connected = false;
			LOG_INF("BT DISCONNECTED");
			break;
	}
}

void button_changed(uint32_t button_state, uint32_t has_changed)
{
	
	uint32_t buttons = button_state & has_changed;

	
	if ((buttons & KEY_ON_MASK) && is_bt_connected) {
		dk_set_led_on(MOV_LED);
		app_bt_move(true);
	}
	if (buttons & KEY_OFF_MASK) {
		dk_set_led_off(MOV_LED);
		app_bt_move(false);
	}
}


// int dbg_pins_init( void )
// {
	
// 	int err;
	
// 	if (!device_is_ready(dbg_port)) {
// 		LOG_ERR("Could not bind to debug port");
// 		return -ENODEV;
// 	}
	
// 	for (size_t i = 0; i < ARRAY_SIZE(dbg_pins); i++) {
// 		err = gpio_pin_configure(dbg_port, dbg_pins[i], GPIO_OUTPUT);
// 		if (err){
// 					LOG_ERR("Unable to configure dbg%u, err %d", i, err);
// 					dbg_port = NULL;
// 					return err;
// 				}
                      		
// 		gpio_pin_set(dbg_port, dbg_pins[i], 0); 
// 	}

// 	return 0;

// }



static int leds_init(void)
{
	int err;

	if (!device_is_ready(leds[0].port)) {
		LOG_ERR("LEDs port not ready");
		return -ENODEV;
	}


	for (size_t i = 0; i < ARRAY_SIZE(leds); i++) {
		err = gpio_pin_configure_dt(&leds[i], GPIO_OUTPUT);
		if (err) {
			LOG_ERR("Cannot configure LED gpio");
			return err;
		}
		gpio_pin_set(leds[0].port, leds[i].pin, 0);
	}

	return 0;
}

static volatile bool button_flag = false;
static void button_pressed(const struct device *dev, struct gpio_callback *cb, uint32_t pins)
{
	/** As the leds controlled by IIC protocal, so it can not work in inerrupt callback*/
	// for (size_t i = 0; i < ARRAY_SIZE(leds); i++) 
	// {	
	// 	gpio_pin_toggle(leds[0].port, leds[i].pin);
	// }
	button_flag = !button_flag;
	if (button_flag && is_bt_connected) {
		app_bt_move(true);
	}

	if (!button_flag) {
		app_bt_move(false);
	}

	LOG_INF("Button pressed at %ld	 button_flag=%d\n", k_cycle_get_32(),button_flag);
}


static int buttons_init( void )
{
	int err;
	
	if (!device_is_ready(button.port)) {
		LOG_ERR("Could not bind to debug port");
		return -ENODEV;
	}

	err = gpio_pin_configure_dt(&button, GPIO_INPUT);
	if (err != 0) {
		LOG_ERR("Error %d: failed to configure %s pin %d\n",
		       err, button.port->name, button.pin);
		return;
	}

	err = gpio_pin_interrupt_configure_dt(&button, GPIO_INT_EDGE_FALLING);
	if (err != 0) {
		LOG_ERR("Error %d: failed to configure interrupt on %s pin %d\n",
			err, button.port->name, button.pin);
		return;
	}

	gpio_init_callback(&button_cb_data, button_pressed, BIT(button.pin));
	gpio_add_callback(button.port, &button_cb_data);
	LOG_INF("Set up button at %s pin %d\r\n", button.port->name, button.pin);

	return 0;
}

int main(void)
{
	int err;

	// err= dbg_pins_init();
	// if (err) {
	// 	LOG_ERR("Debug pins failed (err %d)", err);
	// 	return;
	// }

	err = leds_init();
	if (err) {
		LOG_ERR("LEDs init failed (err %d)", err);
		return;
	}
	
	err = buttons_init();
	if (err) {
		LOG_ERR("Cannot init buttons (err: %d)\n", err);
	}

	LOG_INF("ESB BLE Multiprotocol Example, version is %s!\r\n",FW_VERSION);
#if 1
	err = app_bt_init(on_bt_callback);
	if (err) {
		LOG_ERR("app_bt init failed (err %d)", err);
		return;
	}

	err = app_esb_init(APP_ESB_MODE_PTX, on_esb_callback);
	if (err) {
		LOG_ERR("app_esb init failed (err %d)", err);
		return;
	}
	
	timeslot_init(on_timeslot_start_stop);
#endif
	//uint8_t esb_tx_buf[32] = {0};
	//int tx_counter = 0;
	while (1) {
/*
	gpio_pin_set(dbg_port, dbg_pins[2], 1);	
		memcpy(esb_tx_buf, (uint8_t*)&tx_counter, 4);
		err = app_esb_send(esb_tx_buf, 32);
		if (err < 0) {
			LOG_INF("ESB TX upload failed (err %i)", err);
		}
		else {
			LOG_INF("ESB TX upload %.2x-%.2x", (tx_counter& 0xFF), ((tx_counter >> 8) & 0xFF));
			tx_counter++;
		}
	gpio_pin_set(dbg_port, dbg_pins[2], 0);		

		for (size_t i = 0; i < ARRAY_SIZE(leds); i++) 
		{	
			//gpio_pin_set(leds[0].port, leds[i].pin, 0);
			gpio_pin_toggle(leds[0].port, leds[i].pin);
		}
*/
		k_sleep(K_MSEC(10));
		if(button_flag)
		{
			for (size_t i = 0; i < ARRAY_SIZE(leds); i++) 
			{	
				gpio_pin_set(leds[0].port, leds[i].pin, 1);
				//gpio_pin_toggle(leds[0].port, leds[i].pin);
			}
		}
		else
		{
			for (size_t i = 0; i < ARRAY_SIZE(leds); i++) 
			{	
				gpio_pin_set(leds[0].port, leds[i].pin, 0);
				//gpio_pin_toggle(leds[0].port, leds[i].pin);
			}
		}
		
		// LOG_INF("ESB BLE Multiprotocol Example is running!\r\n");
	}

	return 0;
}
