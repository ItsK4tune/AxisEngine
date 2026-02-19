

#ifndef AVFILTER_VERSION_H
#define AVFILTER_VERSION_H



#include "libavutil/version.h"

#include "version_major.h"

#define LIBAVFILTER_VERSION_MINOR   4
#define LIBAVFILTER_VERSION_MICRO 100


#define LIBAVFILTER_VERSION_INT AV_VERSION_INT(LIBAVFILTER_VERSION_MAJOR, \
                                               LIBAVFILTER_VERSION_MINOR, \
                                               LIBAVFILTER_VERSION_MICRO)
#define LIBAVFILTER_VERSION     AV_VERSION(LIBAVFILTER_VERSION_MAJOR,   \
                                           LIBAVFILTER_VERSION_MINOR,   \
                                           LIBAVFILTER_VERSION_MICRO)
#define LIBAVFILTER_BUILD       LIBAVFILTER_VERSION_INT

#define LIBAVFILTER_IDENT       "Lavfi" AV_STRINGIFY(LIBAVFILTER_VERSION)

#endif 
