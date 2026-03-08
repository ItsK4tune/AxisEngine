

#ifndef AVUTIL_HWCONTEXT_D3D12VA_H
#define AVUTIL_HWCONTEXT_D3D12VA_H


#include <stdint.h>
#include <initguid.h>
#include <d3d12.h>
#include <d3d12sdklayers.h>
#include <d3d12video.h>


typedef struct AVD3D12VADeviceContext {
    
    ID3D12Device *device;

    
    ID3D12VideoDevice *video_device;

    
    void (*lock)(void *lock_ctx);
    void (*unlock)(void *lock_ctx);
    void *lock_ctx;
} AVD3D12VADeviceContext;


typedef struct AVD3D12VASyncContext {
    
    ID3D12Fence *fence;

    
    HANDLE event;

    
    uint64_t fence_value;
} AVD3D12VASyncContext;


typedef struct AVD3D12VAFrame {
    
    ID3D12Resource *Texture;

    
    AVD3D12VASyncContext sync_ctx;
} AVD3D12VAFrame;


typedef struct AVD3D12VAFramesContext {
    
    DXGI_FORMAT format;

    
    D3D12_RESOURCE_FLAGS flags;
} AVD3D12VAFramesContext;

#endif 