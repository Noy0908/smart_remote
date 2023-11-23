#ifndef __ESB_HANDLE_H__
#define __ESB_HANDLE_H__

#include <stdint.h>


int esb_initialize(void);

void esb_buffer_handle(void);

void free_esb_slab_memory(void *buffer);

#endif
