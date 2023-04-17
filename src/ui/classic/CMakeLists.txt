if (ENABLE_X11 OR WAYLAND_FOUND)

add_subdirectory(themes)

set(CLASSICUI_SRCS)
set(CLASSICUI_LIBS)
if (WAYLAND_FOUND)
    set(CLASSICUI_SRCS ${CLASSICUI_SRCS}
        waylandui.cpp waylandwindow.cpp waylandshmwindow.cpp
        waylandpointer.cpp buffer.cpp waylandinputwindow.cpp)
    set(CLASSICUI_LIBS ${CLASSICUI_LIBS}
        Fcitx5::Module::Wayland Fcitx5::Module::WaylandIM Wayland::Client Fcitx5::Wayland::Core
        Fcitx5::Wayland::InputMethod Fcitx5::Wayland::InputMethodV2
        Fcitx5::Wayland::Blur
        Fcitx5::Wayland::FractionalScale Fcitx5::Wayland::Viewporter)
else()
endif()

if (ENABLE_X11)
    set(CLASSICUI_SRCS ${CLASSICUI_SRCS}
        xcbui.cpp xcbwindow.cpp xcbtraywindow.cpp xcbinputwindow.cpp xcbmenu.cpp)
    set(CLASSICUI_LIBS ${CLASSICUI_LIBS} PkgConfig::CairoXCB Fcitx5::Module::XCB
        XCB::AUX XCB::ICCCM XCB::XINERAMA XCB::RANDR XCB::EWMH)
endif()

if (ENABLE_DBUS)
    set(CLASSICUI_LIBS ${CLASSICUI_LIBS} Fcitx5::Module::DBus)
endif()

add_library(classicui MODULE
    classicui.cpp window.cpp theme.cpp inputwindow.cpp plasmathemewatchdog.cpp ${CLASSICUI_SRCS}
    )

target_link_libraries(classicui
    Fcitx5::Core
    PkgConfig::Cairo PkgConfig::Pango
    PkgConfig::GdkPixbuf PkgConfig::GioUnix
    Fcitx5::Module::NotificationItem
    ${CLASSICUI_LIBS}
    ${FMT_TARGET})

install(TARGETS classicui DESTINATION "${FCITX_INSTALL_ADDONDIR}")
configure_file(classicui.conf.in.in classicui.conf.in @ONLY)
fcitx5_translate_desktop_file(${CMAKE_CURRENT_BINARY_DIR}/classicui.conf.in classicui.conf)
install(FILES "${CMAKE_CURRENT_BINARY_DIR}/classicui.conf" DESTINATION "${FCITX_INSTALL_PKGDATADIR}/addon"
        COMPONENT config)

endif()

fcitx5_export_module(ClassicUI TARGET classicui BUILD_INCLUDE_DIRECTORIES "${CMAKE_CURRENT_SOURCE_DIR}" HEADERS classicui_public.h)
