add_library(classicui MODULE classicui.cpp xcbui.cpp xcbwindow.cpp window.cpp theme.cpp waylandui.cpp waylandwindow.cpp waylandeglwindow.cpp waylandshmwindow.cpp)

target_link_libraries(classicui
    Fcitx5::Core
    Cairo::Cairo Cairo::XCB Cairo::EGL Pango::Pango Pango::Cairo
    Fcitx5::Module::XCB Fcitx5::Module::Wayland
    XCB::AUX Wayland::Client EGL::EGL Wayland::Egl
    Fcitx5::Wayland::Core)
set_target_properties(classicui PROPERTIES PREFIX "")
install(TARGETS classicui DESTINATION "${FCITX_INSTALL_ADDONDIR}")
install(FILES classicui.conf DESTINATION "${FCITX_INSTALL_PKGDATADIR}/addon")
