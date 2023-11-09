/* Copyright (c) 2007 Nordic Semiconductor. All Rights Reserved.
 *
 * The information contained herein is confidential property of 
 * Nordic Semiconductor. The use, copying, transfer or disclosure 
 * of such information is prohibited except by express written
 * agreement with Nordic Semiconductor.
 */ 

/** @file
 * Source code for generating mouse circle data for the 
 * Wireless Desktop Protocol demo.
 *
 * @author Lasse Olsen
 *
 */

#include "circle_test.h"

static int8_t circle_cord[] = 
{
  #include "circle_cord_size250.h"
};

void circle_test_get(int8_t *ret_data)
{
  static uint8_t rad=0, cord_index=0;

  // Uses the same data set for every radian of the circle, only altering the sign
  switch(rad)
  {
    case 0:
      ret_data[X_POS] = -circle_cord[CORD_SIZE-1-cord_index];
      ret_data[Y_POS] = circle_cord[cord_index];
      cord_index++;
      break;
    case 1:
      ret_data[X_POS] = -circle_cord[CORD_SIZE-1-cord_index];
      ret_data[Y_POS] = -circle_cord[cord_index];       
      cord_index--;
      break;
    case 2:
       ret_data[X_POS] = circle_cord[CORD_SIZE-1-cord_index];
      ret_data[Y_POS] = -circle_cord[cord_index];     
      cord_index++;
      break;
    case 3:
      ret_data[X_POS] = circle_cord[CORD_SIZE-1-cord_index];
      ret_data[Y_POS] = circle_cord[cord_index];     
      cord_index--;
      break;
    }

  if(!(cord_index < CORD_SIZE))
  {
    rad = (rad + 1) % 4;
    
    if(rad % 2)
    {
      cord_index = (CORD_SIZE -1); 
    }
    else
    {
      cord_index = 0;
    } 
  }        
}
