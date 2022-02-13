/*
 * SPDX-FileCopyrightText: 2016-2016 CSSlayer <wengxt@gmail.com>
 *
 * SPDX-License-Identifier: LGPL-2.1-or-later
 *
 */

#include "waylandui.h"
#include <algorithm>
#include "fcitx-utils/charutils.h"
#include "fcitx-utils/stringutils.h"
#include "config.h"
#include "display.h"
#include "waylandinputwindow.h"
#include "waylandshmwindow.h"
#include "wl_compositor.h"
#include "wl_seat.h"
#include "wl_shell.h"
#include "wl_shm.h"
#include "zwp_input_panel_v1.h"
#include "zwp_input_popup_surface_v2.h"

#ifdef CAIRO_EGL_FOUND

#include <EGL/egl.h>
#include <EGL/eglext.h>
#include <cairo-gl.h>
#include "waylandeglwindow.h"

#endif

namespace fcitx::classicui {

#ifdef CAIRO_EGL_FOUND

static bool checkEGLExtension(EGLDisplay display, const char *extension) {
    const char *extensions = eglQueryString(display, EGL_EXTENSIONS);
    if (!extensions) {
        return false;
    }
    auto exts = stringutils::split(extensions, FCITX_WHITESPACE);
    return std::find(exts.begin(), exts.end(), std::string(extension)) !=
           exts.end();
}

static inline EGLDisplay getEGLDisplay(EGLenum platform,
                                       wl_display *nativeDisplay,
                                       const EGLint *attribList) {
    if (checkEGLExtension(EGL_NO_DISPLAY, "EGL_EXT_platform_base")) {
        if (checkEGLExtension(EGL_NO_DISPLAY, "EGL_KHR_platform_wayland") ||
            checkEGLExtension(EGL_NO_DISPLAY, "EGL_EXT_platform_wayland") ||
            checkEGLExtension(EGL_NO_DISPLAY, "EGL_MESA_platform_wayland")) {

            static PFNEGLGETPLATFORMDISPLAYEXTPROC eglGetPlatformDisplay =
                nullptr;
            if (!eglGetPlatformDisplay)
                eglGetPlatformDisplay =
                    (PFNEGLGETPLATFORMDISPLAYEXTPROC)eglGetProcAddress(
                        "eglGetPlatformDisplayEXT");

            return eglGetPlatformDisplay(platform, nativeDisplay, attribList);
        }
    }

    return eglGetDisplay((EGLNativeDisplayType)nativeDisplay);
}

#endif

WaylandUI::WaylandUI(ClassicUI *parent, const std::string &name,
                     wl_display *display)
    : parent_(parent), name_(name), display_(static_cast<wayland::Display *>(
                                        wl_display_get_user_data(display))) {
#ifdef CAIRO_EGL_FOUND
    hasEgl_ = initEGL();
#endif
    display_->requestGlobals<wayland::WlCompositor>();
    display_->requestGlobals<wayland::WlShm>();
    display_->requestGlobals<wayland::WlSeat>();
    display_->requestGlobals<wayland::ZwpInputPanelV1>();
    panelConn_ = display_->globalCreated().connect(
        [this](const std::string &name, const std::shared_ptr<void> &) {
            if (name == wayland::ZwpInputPanelV1::interface) {
                if (inputWindow_) {
                    inputWindow_->initPanel();
                }
            } else if (name == wayland::WlCompositor::interface ||
                       name == wayland::WlShm::interface) {
                setupInputWindow();
            } else if (name == wayland::WlSeat::interface) {
                auto seat = display_->getGlobal<wayland::WlSeat>();
                if (seat) {
                    pointer_ = std::make_unique<WaylandPointer>(seat.get());
                }
            }
        });
    panelRemovedConn_ = display_->globalRemoved().connect(
        [this](const std::string &name, const std::shared_ptr<void> &) {
            if (name == wayland::ZwpInputPanelV1::interface) {
                if (inputWindow_) {
                    inputWindow_->resetPanel();
                }
            }
        });
    auto seat = display_->getGlobal<wayland::WlSeat>();
    if (seat) {
        pointer_ = std::make_unique<WaylandPointer>(seat.get());
    }
    display_->sync();
    setupInputWindow();
}

WaylandUI::~WaylandUI() {
#ifdef CAIRO_EGL_FOUND
    if (argbDevice_) {
        cairo_device_destroy(argbDevice_);
    }

    if (eglDisplay_) {
        eglMakeCurrent(eglDisplay_, EGL_NO_SURFACE, EGL_NO_SURFACE,
                       EGL_NO_CONTEXT);
        eglTerminate(eglDisplay_);
        eglReleaseThread();
    }
#endif
}

#ifdef CAIRO_EGL_FOUND
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
                                              EGL_OPENGL_BIT,
                                              EGL_NONE};

    EGLint *context_attribs = nullptr;
    EGLint api = EGL_OPENGL_API;

    eglDisplay_ = getEGLDisplay(EGL_PLATFORM_WAYLAND_KHR, *display_, nullptr);

    if (eglInitialize(eglDisplay_, &major, &minor) != EGL_TRUE) {
        return false;
    }

    if (eglBindAPI(api) != EGL_TRUE) {
        return false;
    }

    if (!eglChooseConfig(eglDisplay_, argb_cfg_attribs, &argbConfig_, 1, &n) ||
        n != 1) {
        return false;
    }

    argbCtx_ = eglCreateContext(eglDisplay_, argbConfig_, EGL_NO_CONTEXT,
                                context_attribs);
    if (!argbCtx_) {
        CLASSICUI_DEBUG() << "EGL Error: " << eglGetError();
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

    return nullptr;
}

EGLSurface WaylandUI::createEGLSurface(wl_egl_window *window,
                                       const EGLint *attrib_list) {
    static PFNEGLCREATEPLATFORMWINDOWSURFACEEXTPROC create_platform_window =
        nullptr;

    if (!create_platform_window) {
        create_platform_window =
            (PFNEGLCREATEPLATFORMWINDOWSURFACEEXTPROC)getEGLProcAddress(
                "eglCreatePlatformWindowSurfaceEXT");
    }

    if (create_platform_window) {
        return create_platform_window(eglDisplay_, argbConfig_, window,
                                      attrib_list);
    }

    return eglCreateWindowSurface(eglDisplay_, argbConfig_,
                                  (EGLNativeWindowType)window, attrib_list);
}

void WaylandUI::destroyEGLSurface(EGLSurface surface) {
    eglDestroySurface(eglDisplay_, surface);
}

cairo_surface_t *WaylandUI::createEGLCairoSurface(EGLSurface surface, int width,
                                                  int height) {
    return cairo_gl_surface_create_for_egl(argbDevice_, surface, width, height);
}
#endif

void WaylandUI::update(UserInterfaceComponent component,
                       InputContext *inputContext) {
    if (inputWindow_ && component == UserInterfaceComponent::InputPanel) {
        inputWindow_->update(inputContext);
    }
}

void WaylandUI::suspend() { inputWindow_.reset(); }

void WaylandUI::resume() { setupInputWindow(); }

void WaylandUI::setupInputWindow() {
    if (parent_->suspended() || inputWindow_) {
        return;
    }

    // Unable to draw window.
    if (!hasEgl_ && !display_->getGlobal<wayland::WlShm>()) {
        return;
    }
    // Unable to create surface.
    if (!display_->getGlobal<wayland::WlCompositor>()) {
        return;
    }
    inputWindow_ = std::make_unique<WaylandInputWindow>(this);
    inputWindow_->initPanel();
}

std::unique_ptr<WaylandWindow> WaylandUI::newWindow() {
#ifdef CAIRO_EGL_FOUND
    if (hasEgl_) {
        return std::make_unique<WaylandEGLWindow>(this);
    } else
#endif
    {
        return std::make_unique<WaylandShmWindow>(this);
    }
}
} // namespace fcitx::classicui
