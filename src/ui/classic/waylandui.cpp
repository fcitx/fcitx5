/*
 * Copyright (C) 2016~2016 by CSSlayer
 * wengxt@gmail.com
 *
 * This library is free software; you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as
 * published by the Free Software Foundation; either version 2 of the
 * License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; see the file COPYING. If not,
 * see <http://www.gnu.org/licenses/>.
 */

#include "waylandui.h"
#include "display.h"
#include "fcitx-utils/charutils.h"
#include "fcitx-utils/stringutils.h"
#include "xcbui.h"
#include <EGL/egl.h>
#include <EGL/eglext.h>
#include <algorithm>
#include <cairo/cairo-gl.h>

namespace fcitx {
namespace classicui {

static bool checkEGLExtension(EGLDisplay display, const char *extension) {
    const char *extensions = eglQueryString(display, EGL_EXTENSIONS);
    if (!extensions) {
        return false;
    }
    auto exts = stringutils::split(extensions, FCITX_WHITESPACE);
    return std::find(exts.begin(), exts.end(), std::string(extension)) != exts.end();
}

static inline EGLDisplay getEGLDisplay(EGLenum platform, wl_display *nativeDisplay, const EGLint *attribList) {
    if (checkEGLExtension(EGL_NO_DISPLAY, "EGL_EXT_platform_base")) {
        if (checkEGLExtension(EGL_NO_DISPLAY, "EGL_KHR_platform_wayland") ||
            checkEGLExtension(EGL_NO_DISPLAY, "EGL_EXT_platform_wayland") ||
            checkEGLExtension(EGL_NO_DISPLAY, "EGL_MESA_platform_wayland")) {

            static PFNEGLGETPLATFORMDISPLAYEXTPROC eglGetPlatformDisplay = nullptr;
            if (!eglGetPlatformDisplay)
                eglGetPlatformDisplay = (PFNEGLGETPLATFORMDISPLAYEXTPROC)eglGetProcAddress("eglGetPlatformDisplayEXT");

            return eglGetPlatformDisplay(platform, nativeDisplay, attribList);
        }
    }

    return eglGetDisplay((EGLNativeDisplayType)nativeDisplay);
}

WaylandUI::WaylandUI(ClassicUI *parent, const std::string &name, wl_display *display)
    : parent_(parent), name_(name), display_(static_cast<wayland::Display *>(wl_display_get_user_data(display))) {
    hasEgl_ = initEGL();
}

WaylandUI::~WaylandUI() {
    if (argbDevice_) {
        cairo_device_destroy(argbDevice_);
    }

    if (eglDisplay_) {
        eglMakeCurrent(eglDisplay_, EGL_NO_SURFACE, EGL_NO_SURFACE, EGL_NO_CONTEXT);
        eglTerminate(eglDisplay_);
        eglReleaseThread();
    }
}

bool WaylandUI::initEGL() {
    EGLint major, minor;
    EGLint n;

    static const EGLint argb_cfg_attribs[] = {EGL_SURFACE_TYPE,
                                              EGL_WINDOW_BIT,
                                              EGL_RED_SIZE,
                                              1,
                                              EGL_GREEN_SIZE,
                                              1,
                                              EGL_BLUE_SIZE,
                                              1,
                                              EGL_ALPHA_SIZE,
                                              1,
                                              EGL_DEPTH_SIZE,
                                              1,
                                              EGL_RENDERABLE_TYPE,
                                              EGL_OPENGL_ES2_BIT,
                                              EGL_NONE};

    static const EGLint context_attribs[] = {EGL_CONTEXT_CLIENT_VERSION, 2, EGL_NONE};
    EGLint api = EGL_OPENGL_ES_API;

    eglDisplay_ = getEGLDisplay(EGL_PLATFORM_WAYLAND_KHR, *display_, NULL);

    if (!eglInitialize(eglDisplay_, &major, &minor)) {
        return false;
    }

    if (!eglBindAPI(api)) {
        return false;
    }

    if (!eglChooseConfig(eglDisplay_, argb_cfg_attribs, &argbConfig_, 1, &n) || n != 1) {
        return false;
    }

    argbCtx_ = eglCreateContext(eglDisplay_, &argbConfig_, EGL_NO_CONTEXT, context_attribs);
    if (!argbCtx_) {
        return false;
    }

    argbDevice_ = cairo_egl_device_create(eglDisplay_, argbCtx_);
    if (cairo_device_status(argbDevice_) != CAIRO_STATUS_SUCCESS) {
        return false;
    }

    return true;
}

static inline void *getEGLProcAddress(const char *address) {
    if ((checkEGLExtension(EGL_NO_DISPLAY, "EGL_EXT_platform_wayland") ||
         checkEGLExtension(EGL_NO_DISPLAY, "EGL_KHR_platform_wayland"))) {
        return (void *)eglGetProcAddress(address);
    }

    return NULL;
}

EGLSurface WaylandUI::createEGLSurface(wl_egl_window *window, const EGLint *attrib_list) {
    static PFNEGLCREATEPLATFORMWINDOWSURFACEEXTPROC create_platform_window = NULL;

    if (!create_platform_window) {
        create_platform_window =
            (PFNEGLCREATEPLATFORMWINDOWSURFACEEXTPROC)getEGLProcAddress("eglCreatePlatformWindowSurfaceEXT");
    }

    if (create_platform_window)
        return create_platform_window(eglDisplay_, argbConfig_, window, attrib_list);

    return eglCreateWindowSurface(eglDisplay_, argbConfig_, (EGLNativeWindowType)window, attrib_list);
}

void WaylandUI::destroyEGLSurface(EGLSurface surface) { eglDestroySurface(eglDisplay_, surface); }

cairo_surface_t *WaylandUI::createEGLCairoSurface(EGLSurface surface, int width, int height) {
    return cairo_gl_surface_create_for_egl(argbDevice_, surface, width, height);
}
}
}
