



#ifndef AVUTIL_RIPEMD_H
#define AVUTIL_RIPEMD_H

#include <stddef.h>
#include <stdint.h>

#include "attributes.h"



extern const int av_ripemd_size;

struct AVRIPEMD;


struct AVRIPEMD *av_ripemd_alloc(void);


int av_ripemd_init(struct AVRIPEMD* context, int bits);


void av_ripemd_update(struct AVRIPEMD* context, const uint8_t* data, size_t len);


void av_ripemd_final(struct AVRIPEMD* context, uint8_t *digest);



#endif 
