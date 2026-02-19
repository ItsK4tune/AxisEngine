



#ifndef AVUTIL_UUID_H
#define AVUTIL_UUID_H

#include <stdint.h>
#include <string.h>

#define AV_PRI_UUID                          \
    "%02hhx%02hhx%02hhx%02hhx-%02hhx%02hhx-" \
    "%02hhx%02hhx-%02hhx%02hhx-%02hhx%02hhx%02hhx%02hhx%02hhx%02hhx"

#define AV_PRI_URN_UUID                               \
    "urn:uuid:%02hhx%02hhx%02hhx%02hhx-%02hhx%02hhx-" \
    "%02hhx%02hhx-%02hhx%02hhx-%02hhx%02hhx%02hhx%02hhx%02hhx%02hhx"


#define AV_UUID_ARG(x)                  \
    (x)[ 0], (x)[ 1], (x)[ 2], (x)[ 3], \
    (x)[ 4], (x)[ 5], (x)[ 6], (x)[ 7], \
    (x)[ 8], (x)[ 9], (x)[10], (x)[11], \
    (x)[12], (x)[13], (x)[14], (x)[15]

#define AV_UUID_LEN 16


typedef uint8_t AVUUID[AV_UUID_LEN];


int av_uuid_parse(const char *in, AVUUID uu);


int av_uuid_urn_parse(const char *in, AVUUID uu);


int av_uuid_parse_range(const char *in_start, const char *in_end, AVUUID uu);


void av_uuid_unparse(const AVUUID uu, char *out);


static inline int av_uuid_equal(const AVUUID uu1, const AVUUID uu2)
{
    return memcmp(uu1, uu2, AV_UUID_LEN) == 0;
}


static inline void av_uuid_copy(AVUUID dest, const AVUUID src)
{
    memcpy(dest, src, AV_UUID_LEN);
}


static inline void av_uuid_nil(AVUUID uu)
{
    memset(uu, 0, AV_UUID_LEN);
}

#endif 
