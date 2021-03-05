add_subdirectory(themes)

set(CLASSICUI_WAYLAND_SRCS)
set(CLASSICUI_WAYLAND_LIBS)
if (WAYLAND_FOUND)
    set(CLASSICUI_WAYLAND_SRCS ${CLASSICUI_WAYLAND_SRCS}
        waylandui.cpp waylandwindow.cpp waylandeglwindow.cpp waylandshmwindow.cpp
        waylandpointer.cpp buffer.cpp waylandinputwindow.cpp)
    set(CLASSICUI_WAYLAND_LIBS ${CLASSICUI_WAYLAND_LIBS}
        Fcitx5::Module::Wayland Fcitx5::Module::WaylandIM Wayland::Client Fcitx5::Wayland::Core
        Fcitx5::Wayland::InputMethod Fcitx5::Wayland::InputMethodV2)
else()
endif()

if (ENABLE_X11)
    set(CLASSICUI_WAYLAND_SRCS ${CLASSICUI_WAYLAND_SRCS}
        xcbui.cpp xcbwindow.cpp xcbtraywindow.cpp xcbinputwindow.cpp xcbmenu.cpp)
    set(CLASSICUI_WAYLAND_LIBS ${CLASSICUI_WAYLAND_LIBS} PkgConfig::CairoXCB Fcitx5::Module::XCB
        XCB::AUX XCB::ICCCM XCB::XINERAMA XCB::RANDR XCB::EWMH)
endif()

add_library(classicui MODULE
    classicui.cpp window.cpp theme.cpp inputwindow.cpp  ${CLASSICUI_WAYLAND_SRCS}
    )

if (CAIRO_EGL_FOUND)
set(CAIRO_EGL_LIBRARY PkgConfig::CairoEGL Wayland::Egl EGL::EGL)
else()
set(CAIRO_EGL_LIBRARY)
endif()

target_link_libraries(classicui
    Fcitx5::Core
    PkgConfig::Cairo PkgConfig::Pango
    PkgConfig::GdkPixbuf PkgConfig::GioUnix
    Fcitx5::Module::NotificationItem
    ${CAIRO_EGL_LIBRARY}
    ${CLASSICUI_WAYLAND_LIBS}
    ${FMT_TARGET})
set_target_properties(classicui PROPERTIES PREFIX "")
install(TARGETS classicui DESTINATION "${FCITX_INSTALL_ADDONDIR}")
configure_file(classicui.conf.in.in classicui.conf.in @ONLY)
fcitx5_translate_desktop_file(${CMAKE_CURRENT_BINARY_DIR}/classicui.conf.in classicui.conf)
install(FILES "${CMAKE_CURRENT_BINARY_DIR}/classicui.conf" DESTINATION "${FCITX_INSTALL_PKGDATADIR}/addon")
