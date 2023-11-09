/* Copyright (c) 2007 Nordic Semiconductor. All Rights Reserved.
 *
 * The information contained herein is confidential property of 
 * Nordic Semiconductor. The use, copying, transfer or disclosure 
 * of such information is prohibited except by express written
 * agreement with Nordic Semiconductor.
 */ 

/** @file
 * Header file for mouse circle data generator used in the 
 * Wireless Desktop Protocol demo.
 *
 * @author Lasse Olsen
 *
 */

#ifndef _CIRLE_TEST_H
#define _CIRLE_TEST_H 

#include <stdint.h>

#define CORD_SIZE 25

typedef enum
{
  X_POS,
  Y_POS
} circle_test_pos;

/**
Function for generating relative mouse positions drawing a circle.

@param ret_data returns the relative movements. X movement is returned in *(ret_data)
and Y movement is returned in *(ret_data+1). The drawn cicle will consist of 100
points.  
*/
void circle_test_get(int8_t *ret_data);

#endif
