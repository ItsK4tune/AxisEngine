



#ifndef AVUTIL_SPHERICAL_H
#define AVUTIL_SPHERICAL_H

#include <stddef.h>
#include <stdint.h>




enum AVSphericalProjection {
    
    AV_SPHERICAL_EQUIRECTANGULAR,

    
    AV_SPHERICAL_CUBEMAP,

    
    AV_SPHERICAL_EQUIRECTANGULAR_TILE,

    
    AV_SPHERICAL_HALF_EQUIRECTANGULAR,

    
    AV_SPHERICAL_RECTILINEAR,

    
    AV_SPHERICAL_FISHEYE,

    
    AV_SPHERICAL_PARAMETRIC_IMMERSIVE,
};


typedef struct AVSphericalMapping {
    
    enum AVSphericalProjection projection;

    
    int32_t yaw;   
    int32_t pitch; 
    int32_t roll;  
    

    
    uint32_t bound_left;   
    uint32_t bound_top;    
    uint32_t bound_right;  
    uint32_t bound_bottom; 
    

    
    uint32_t padding;
} AVSphericalMapping;


AVSphericalMapping *av_spherical_alloc(size_t *size);


void av_spherical_tile_bounds(const AVSphericalMapping *map,
                              size_t width, size_t height,
                              size_t *left, size_t *top,
                              size_t *right, size_t *bottom);


const char *av_spherical_projection_name(enum AVSphericalProjection projection);


int av_spherical_from_name(const char *name);


#endif 
