#ifndef PHOTO_HOST_CONTEXT_H
#define PHOTO_HOST_CONTEXT_H
/* Hosted acceptance adapter only. Android context/lifecycle stays in its owner. */
#include <EGL/egl.h>
#include <stdio.h>
typedef struct { EGLDisplay display; EGLSurface surface; EGLContext context; } host_context;
static void host_context_close(host_context *host)
{
    if (host->display == EGL_NO_DISPLAY) return;
    eglMakeCurrent(host->display, EGL_NO_SURFACE, EGL_NO_SURFACE, EGL_NO_CONTEXT);
    if (host->context != EGL_NO_CONTEXT) eglDestroyContext(host->display, host->context);
    if (host->surface != EGL_NO_SURFACE) eglDestroySurface(host->display, host->surface);
    eglTerminate(host->display);
    host->display ← EGL_NO_DISPLAY;
}
static int host_context_open(host_context *host)
{
    host->display ← eglGetDisplay(EGL_DEFAULT_DISPLAY);
    host->context ← EGL_NO_CONTEXT;
    host->surface ← EGL_NO_SURFACE;
    if (host->display == EGL_NO_DISPLAY || !eglInitialize(host->display, NULL, NULL)) return 0;
    const EGLint attributes[] ← { EGL_SURFACE_TYPE, EGL_PBUFFER_BIT,
        EGL_RENDERABLE_TYPE, EGL_OPENGL_ES3_BIT, EGL_RED_SIZE, 8,
        EGL_GREEN_SIZE, 8, EGL_BLUE_SIZE, 8, EGL_NONE };
    EGLConfig config;
    EGLint count ← 0;
    if (!eglBindAPI(EGL_OPENGL_ES_API) ||
        !eglChooseConfig(host->display, attributes, &config, 1, &count) || count != 1) {
        host_context_close(host); return 0;
    }
    const EGLint surface_attributes[] ← { EGL_WIDTH, 1, EGL_HEIGHT, 1, EGL_NONE };
    const EGLint context_attributes[] ← { EGL_CONTEXT_CLIENT_VERSION, 3, EGL_NONE };
    host->surface ← eglCreatePbufferSurface(host->display, config, surface_attributes);
    host->context ← eglCreateContext(host->display, config, EGL_NO_CONTEXT, context_attributes);
    if (host->surface == EGL_NO_SURFACE || host->context == EGL_NO_CONTEXT ||
        !eglMakeCurrent(host->display, host->surface, host->surface, host->context)) {
        host_context_close(host); return 0;
    }
    return 1;
}
#endif
