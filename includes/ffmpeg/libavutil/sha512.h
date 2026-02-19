



#ifndef AVUTIL_SHA512_H
#define AVUTIL_SHA512_H

#include <stddef.h>
#include <stdint.h>

#include "attributes.h"



extern const int av_sha512_size;

struct AVSHA512;


struct AVSHA512 *av_sha512_alloc(void);


int av_sha512_init(struct AVSHA512* context, int bits);


void av_sha512_update(struct AVSHA512* context, const uint8_t* data, size_t len);


void av_sha512_final(struct AVSHA512* context, uint8_t *digest);



#endif 
