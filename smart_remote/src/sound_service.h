#ifndef __SOUND_SERVICE_H__
#define __SOUND_SERVICE_H__

#include "drv_mic.h"


#define ESB_BLOCK_SIZE            (CONFIG_ESB_MAX_PAYLOAD_LENGTH - CONFIG_ESB_MAX_PAYLOAD_LENGTH % (MAX_BLOCK_SIZE/4 + 3))
// #define ADPCM_BLOCK_SIZE            MAX_BLOCK_SIZE
#define ADPCM_BLOCK_COUNT           20


void free_adpcm_memory(void *buffer);


#endif



