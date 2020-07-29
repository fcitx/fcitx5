add_subdirectory(themes)

if (WAYLAND_FOUND)

    set(CLASSICUI_WAYLAND_SRCS 
        waylandui.cpp waylandwindow.cpp waylandeglwindow.cpp waylandshmwindow.cpp
        waylandpointer.cpp buffer.cpp waylandinputwindow.cpp)
    set(CLASSICUI_WAYLAND_LIBS
        Fcitx5::Module::Wayland Fcitx5::Module::WaylandIM Wayland::Client Fcitx5::Wayland::Core
        Fcitx5::Wayland::InputMethod Fcitx5::Wayland::InputMethodV2)
else()
    set(CLASSICUI_WAYLAND_SRCS)
    set(CLASSICUI_WAYLAND_LIBS)
endif()

add_library(classicui MODULE
    classicui.cpp xcbui.cpp xcbwindow.cpp window.cpp theme.cpp
    xcbtraywindow.cpp inputwindow.cpp xcbinputwindow.cpp xcbmenu.cpp
    ${CLASSICUI_WAYLAND_SRCS}
    )

if (CAIRO_EGL_FOUND)
set(CAIRO_EGL_LIBRARY PkgConfig::CairoEGL Wayland::Egl EGL::EGL)
else()
set(CAIRO_EGL_LIBRARY)
endif()

target_link_libraries(classicui
    Fcitx5::Core
    PkgConfig::Cairo PkgConfig::CairoXCB PkgConfig::Pango
    PkgConfig::GdkPixbuf PkgConfig::GioUnix
    Fcitx5::Module::XCB Fcitx5::Module::NotificationItem
    XCB::AUX XCB::ICCCM XCB::XINERAMA XCB::RANDR XCB::EWMH
    ${CAIRO_EGL_LIBRARY}
    ${CLASSICUI_WAYLAND_LIBS}
    ${FMT_TARGET})
set_target_properties(classicui PROPERTIES PREFIX "")
install(TARGETS classicui DESTINATION "${FCITX_INSTALL_ADDONDIR}")
fcitx5_translate_desktop_file(classicui.conf.in classicui.conf)
install(FILES "${CMAKE_CURRENT_BINARY_DIR}/classicui.conf" DESTINATION "${FCITX_INSTALL_PKGDATADIR}/addon")
