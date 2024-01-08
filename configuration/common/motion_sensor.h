/*
 * Copyright (c) 2019 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#ifndef _MOTION_SENSOR_H_
#define _MOTION_SENSOR_H_

enum motion_sensor_option {
	MOTION_SENSOR_OPTION_CPI,
	MOTION_SENSOR_OPTION_SLEEP_ENABLE,
	MOTION_SENSOR_OPTION_SLEEP1_TIMEOUT,
	MOTION_SENSOR_OPTION_SLEEP2_TIMEOUT,
	MOTION_SENSOR_OPTION_SLEEP3_TIMEOUT,
	MOTION_SENSOR_OPTION_SLEEP1_SAMPLE_TIME,
	MOTION_SENSOR_OPTION_SLEEP2_SAMPLE_TIME,
	MOTION_SENSOR_OPTION_SLEEP3_SAMPLE_TIME,
	MOTION_SENSOR_OPTION_COUNT,
};

#if CONFIG_DESKTOP_MOTION_SENSOR_PMW3360_ENABLE

 #include <sensor/pmw3360.h>

 #define MOTION_SENSOR_COMPATIBLE pixart_pmw3360

 static const int motion_sensor_option_attr[MOTION_SENSOR_OPTION_COUNT] = {
	[MOTION_SENSOR_OPTION_CPI] = PMW3360_ATTR_CPI,
	[MOTION_SENSOR_OPTION_SLEEP_ENABLE] = PMW3360_ATTR_REST_ENABLE,
	[MOTION_SENSOR_OPTION_SLEEP1_TIMEOUT] = PMW3360_ATTR_RUN_DOWNSHIFT_TIME,
	[MOTION_SENSOR_OPTION_SLEEP2_TIMEOUT] = PMW3360_ATTR_REST1_DOWNSHIFT_TIME,
	[MOTION_SENSOR_OPTION_SLEEP3_TIMEOUT] = PMW3360_ATTR_REST2_DOWNSHIFT_TIME,
	[MOTION_SENSOR_OPTION_SLEEP1_SAMPLE_TIME] = PMW3360_ATTR_REST1_SAMPLE_TIME,
	[MOTION_SENSOR_OPTION_SLEEP2_SAMPLE_TIME] = PMW3360_ATTR_REST2_SAMPLE_TIME,
	[MOTION_SENSOR_OPTION_SLEEP3_SAMPLE_TIME] = PMW3360_ATTR_REST3_SAMPLE_TIME,
 };

#elif CONFIG_DESKTOP_MOTION_SENSOR_PAW3212_ENABLE

 #include <sensor/paw3212.h>

 #define MOTION_SENSOR_COMPATIBLE pixart_paw3212

 static const int motion_sensor_option_attr[MOTION_SENSOR_OPTION_COUNT] = {
	[MOTION_SENSOR_OPTION_CPI] = PAW3212_ATTR_CPI,
	[MOTION_SENSOR_OPTION_SLEEP_ENABLE] = PAW3212_ATTR_SLEEP_ENABLE,
	[MOTION_SENSOR_OPTION_SLEEP1_TIMEOUT] = PAW3212_ATTR_SLEEP1_TIMEOUT,
	[MOTION_SENSOR_OPTION_SLEEP2_TIMEOUT] = PAW3212_ATTR_SLEEP2_TIMEOUT,
	[MOTION_SENSOR_OPTION_SLEEP3_TIMEOUT] = PAW3212_ATTR_SLEEP3_TIMEOUT,
	[MOTION_SENSOR_OPTION_SLEEP1_SAMPLE_TIME] = PAW3212_ATTR_SLEEP1_SAMPLE_TIME,
	[MOTION_SENSOR_OPTION_SLEEP2_SAMPLE_TIME] = PAW3212_ATTR_SLEEP2_SAMPLE_TIME,
	[MOTION_SENSOR_OPTION_SLEEP3_SAMPLE_TIME] = PAW3212_ATTR_SLEEP3_SAMPLE_TIME,
 };

 #elif CONFIG_DESKTOP_MOTION_SENSOR_MPU9250_ENABLE

 #define MOTION_SENSOR_COMPATIBLE invensense_mpu9250

 static const int motion_sensor_option_attr[MOTION_SENSOR_OPTION_COUNT] = {
	0
 };

#else

 #error "Sensor not supported"

#endif

#endif /* _MOTION_SENSOR_H_ */
