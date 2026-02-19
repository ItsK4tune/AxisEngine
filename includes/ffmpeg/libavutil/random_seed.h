

#ifndef AVUTIL_RANDOM_SEED_H
#define AVUTIL_RANDOM_SEED_H

#include <stddef.h>
#include <stdint.h>



uint32_t av_get_random_seed(void);


int av_random_bytes(uint8_t *buf, size_t len);



#endif 
