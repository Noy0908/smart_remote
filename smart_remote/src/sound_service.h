#ifndef __SOUND_SERVICE_H__
#define __SOUND_SERVICE_H__

#include "drv_mic.h"


#define ADPCM_BLOCK_SIZE            (MAX_BLOCK_SIZE/4 + 3)
#define ADPCM_BLOCK_COUNT           60


void free_adpcm_memory(void *buffer);


#endif



