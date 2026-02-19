



#ifndef AVUTIL_MD5_H
#define AVUTIL_MD5_H

#include <stddef.h>
#include <stdint.h>

#include "attributes.h"



extern const int av_md5_size;

struct AVMD5;


struct AVMD5 *av_md5_alloc(void);


void av_md5_init(struct AVMD5 *ctx);


void av_md5_update(struct AVMD5 *ctx, const uint8_t *src, size_t len);


void av_md5_final(struct AVMD5 *ctx, uint8_t *dst);


void av_md5_sum(uint8_t *dst, const uint8_t *src, size_t len);



#endif 
