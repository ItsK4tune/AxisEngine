



#ifndef AVUTIL_SHA_H
#define AVUTIL_SHA_H

#include <stddef.h>
#include <stdint.h>

#include "attributes.h"



extern const int av_sha_size;

struct AVSHA;


struct AVSHA *av_sha_alloc(void);


int av_sha_init(struct AVSHA* context, int bits);


void av_sha_update(struct AVSHA *ctx, const uint8_t *data, size_t len);


void av_sha_final(struct AVSHA* context, uint8_t *digest);



#endif 
