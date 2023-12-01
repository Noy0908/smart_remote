/*
 * Copyright (c) 2018 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */
#include <zephyr/device.h>
#include <zephyr/devicetree.h>
#include <zephyr/drivers/clock_control.h>
#include <zephyr/drivers/clock_control/nrf_clock_control.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/irq.h>
#include <zephyr/logging/log.h>
#include <nrf.h>
#include <esb.h>
#include <zephyr/kernel.h>
#include <zephyr/types.h>
#include "esb_handle.h"
#include "dvi_adpcm.h"
#include "usb_audio_app.h"
#include <zephyr/usb/usb_device.h>
#include <zephyr/usb/class/usb_audio.h>


LOG_MODULE_REGISTER(smart_dongle, CONFIG_ESB_PRX_APP_LOG_LEVEL);

#define FW_VERSION				"1.2.1"

dvi_adpcm_state_t    m_adpcm_state;

static const struct gpio_dt_spec leds[] = {
	GPIO_DT_SPEC_GET(DT_ALIAS(led0), gpios),
	GPIO_DT_SPEC_GET(DT_ALIAS(led1), gpios),
	GPIO_DT_SPEC_GET(DT_ALIAS(led2), gpios),
	GPIO_DT_SPEC_GET(DT_ALIAS(led3), gpios),
};

BUILD_ASSERT(DT_SAME_NODE(DT_GPIO_CTLR(DT_ALIAS(led0), gpios),
			  DT_GPIO_CTLR(DT_ALIAS(led1), gpios)) &&
	     DT_SAME_NODE(DT_GPIO_CTLR(DT_ALIAS(led0), gpios),
			  DT_GPIO_CTLR(DT_ALIAS(led2), gpios)) &&
	     DT_SAME_NODE(DT_GPIO_CTLR(DT_ALIAS(led0), gpios),
			  DT_GPIO_CTLR(DT_ALIAS(led3), gpios)),
	     "All LEDs must be on the same port");


static struct esb_payload tx_payload = ESB_CREATE_PAYLOAD(0,
	0x10, 0x11, 0x12, 0x13, 0x14, 0x15, 0x16, 0x17);


static int leds_init(void)
{
	if (!device_is_ready(leds[0].port)) {
		LOG_ERR("LEDs port not ready");
		return -ENODEV;
	}

	for (size_t i = 0; i < ARRAY_SIZE(leds); i++) {
		int err = gpio_pin_configure_dt(&leds[i], GPIO_OUTPUT);

		if (err) {
			LOG_ERR("Unable to configure LED%u, err %d.", i, err);
			return err;
		}

		gpio_pin_set(leds[0].port, leds[i].pin, 0);
	}

	return 0;
}


int leds_toggle(void)
{
	// for (size_t i = 0; i < ARRAY_SIZE(leds); i++) 
	{
		gpio_pin_toggle(leds[0].port, leds[0].pin);
	}

	return 0;
}





int clocks_start(void)
{
	int err;
	int res;
	struct onoff_manager *clk_mgr;
	struct onoff_client clk_cli;

	clk_mgr = z_nrf_clock_control_get_onoff(CLOCK_CONTROL_NRF_SUBSYS_HF);
	if (!clk_mgr) {
		LOG_ERR("Unable to get the Clock manager");
		return -ENXIO;
	}

	sys_notify_init_spinwait(&clk_cli.notify);

	err = onoff_request(clk_mgr, &clk_cli);
	if (err < 0) {
		LOG_ERR("Clock request failed: %d", err);
		return err;
	}

	do {
		err = sys_notify_fetch_result(&clk_cli.notify, &res);
		if (!err && res) {
			LOG_ERR("Clock could not be started: %d", res);
			return res;
		}
	} while (err);

	LOG_DBG("HF clock started, fw_version is %s", FW_VERSION);
	return 0;
}


// extern struct k_sem esb_sem;

int main(void)
{
	int err;

	LOG_INF("Enhanced ShockBurst prx sample");

	err = clocks_start();
	if (err) {
		return err;
	}

	err = leds_init();
	if (err) {
		return err;
	}

	dvi_adpcm_init_state(&m_adpcm_state);

	// usb_audio_init();

	err = esb_initialize();
	if (err) {
		LOG_ERR("ESB initialization failed, err %d", err);
		return err;
	}

	LOG_INF("Initialization complete");

	err = esb_write_payload(&tx_payload);
	if (err) {
		LOG_ERR("Write payload, err %d", err);
		return err;
	}

	LOG_INF("Setting up for packet receiption");

	err = esb_start_rx();
	if (err) {
		LOG_ERR("RX setup failed, err %d", err);
		return err;
	}

	// while(1)
	// {
	// 	k_sem_take(&esb_sem, K_FOREVER);
	// 	esb_buffer_handle();
	// }

	/* return to idle thread */
	return 0;
}
