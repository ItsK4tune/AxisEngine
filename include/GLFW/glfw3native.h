

#ifndef _glfw3_native_h_
#define _glfw3_native_h_

#ifdef __cplusplus
extern "C" {
#endif










#if !defined(GLFW_NATIVE_INCLUDE_NONE)

 #if defined(GLFW_EXPOSE_NATIVE_WIN32) || defined(GLFW_EXPOSE_NATIVE_WGL)
  
  #if defined(GLFW_APIENTRY_DEFINED)
   #undef APIENTRY
   #undef GLFW_APIENTRY_DEFINED
  #endif
  #include <windows.h>
 #endif

 #if defined(GLFW_EXPOSE_NATIVE_COCOA) || defined(GLFW_EXPOSE_NATIVE_NSGL)
  #if defined(__OBJC__)
   #import <Cocoa/Cocoa.h>
  #else
   #include <ApplicationServices/ApplicationServices.h>
   #include <objc/objc.h>
  #endif
 #endif

 #if defined(GLFW_EXPOSE_NATIVE_X11) || defined(GLFW_EXPOSE_NATIVE_GLX)
  #include <X11/Xlib.h>
  #include <X11/extensions/Xrandr.h>
 #endif

 #if defined(GLFW_EXPOSE_NATIVE_WAYLAND)
  #include <wayland-client.h>
 #endif

 #if defined(GLFW_EXPOSE_NATIVE_WGL)
  
 #endif
 #if defined(GLFW_EXPOSE_NATIVE_NSGL)
  
 #endif
 #if defined(GLFW_EXPOSE_NATIVE_GLX)
  
  #if defined(GLFW_GLAPIENTRY_DEFINED)
   #undef GLAPIENTRY
   #undef GLFW_GLAPIENTRY_DEFINED
  #endif
  #include <GL/glx.h>
 #endif
 #if defined(GLFW_EXPOSE_NATIVE_EGL)
  #include <EGL/egl.h>
 #endif
 #if defined(GLFW_EXPOSE_NATIVE_OSMESA)
  
  #if defined(GLFW_GLAPIENTRY_DEFINED)
   #undef GLAPIENTRY
   #undef GLFW_GLAPIENTRY_DEFINED
  #endif
  #include <GL/osmesa.h>
 #endif

#endif 




#if defined(GLFW_EXPOSE_NATIVE_WIN32)

GLFWAPI const char* glfwGetWin32Adapter(GLFWmonitor* monitor);


GLFWAPI const char* glfwGetWin32Monitor(GLFWmonitor* monitor);


GLFWAPI HWND glfwGetWin32Window(GLFWwindow* window);
#endif

#if defined(GLFW_EXPOSE_NATIVE_WGL)

GLFWAPI HGLRC glfwGetWGLContext(GLFWwindow* window);
#endif

#if defined(GLFW_EXPOSE_NATIVE_COCOA)

GLFWAPI CGDirectDisplayID glfwGetCocoaMonitor(GLFWmonitor* monitor);


GLFWAPI id glfwGetCocoaWindow(GLFWwindow* window);


GLFWAPI id glfwGetCocoaView(GLFWwindow* window);
#endif

#if defined(GLFW_EXPOSE_NATIVE_NSGL)

GLFWAPI id glfwGetNSGLContext(GLFWwindow* window);
#endif

#if defined(GLFW_EXPOSE_NATIVE_X11)

GLFWAPI Display* glfwGetX11Display(void);


GLFWAPI RRCrtc glfwGetX11Adapter(GLFWmonitor* monitor);


GLFWAPI RROutput glfwGetX11Monitor(GLFWmonitor* monitor);


GLFWAPI Window glfwGetX11Window(GLFWwindow* window);


GLFWAPI void glfwSetX11SelectionString(const char* string);


GLFWAPI const char* glfwGetX11SelectionString(void);
#endif

#if defined(GLFW_EXPOSE_NATIVE_GLX)

GLFWAPI GLXContext glfwGetGLXContext(GLFWwindow* window);


GLFWAPI GLXWindow glfwGetGLXWindow(GLFWwindow* window);
#endif

#if defined(GLFW_EXPOSE_NATIVE_WAYLAND)

GLFWAPI struct wl_display* glfwGetWaylandDisplay(void);


GLFWAPI struct wl_output* glfwGetWaylandMonitor(GLFWmonitor* monitor);


GLFWAPI struct wl_surface* glfwGetWaylandWindow(GLFWwindow* window);
#endif

#if defined(GLFW_EXPOSE_NATIVE_EGL)

GLFWAPI EGLDisplay glfwGetEGLDisplay(void);


GLFWAPI EGLContext glfwGetEGLContext(GLFWwindow* window);


GLFWAPI EGLSurface glfwGetEGLSurface(GLFWwindow* window);
#endif

#if defined(GLFW_EXPOSE_NATIVE_OSMESA)

GLFWAPI int glfwGetOSMesaColorBuffer(GLFWwindow* window, int* width, int* height, int* format, void** buffer);


GLFWAPI int glfwGetOSMesaDepthBuffer(GLFWwindow* window, int* width, int* height, int* bytesPerValue, void** buffer);


GLFWAPI OSMesaContext glfwGetOSMesaContext(GLFWwindow* window);
#endif

#ifdef __cplusplus
}
#endif

#endif 

