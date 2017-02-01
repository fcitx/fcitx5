set(WAYLAND_IM_PROTOCOL_SRCS)

ecm_add_wayland_client_protocol(WAYLAND_IM_PROTOCOL_SRCS
    PROTOCOL ${WAYLAND_PROTOCOLS_PKGDATADIR}/unstable/input-method/input-method-unstable-v1.xml
    BASENAME input-method-unstable-v1)

add_library(classicui MODULE classicui.cpp xcbui.cpp xcbwindow.cpp window.cpp theme.cpp waylandui.cpp waylandwindow.cpp waylandoutput.cpp ${WAYLAND_IM_PROTOCOL_SRCS})
target_include_directories(classicui PRIVATE ${CMAKE_CURRENT_BINARY_DIR})
target_link_libraries(classicui Fcitx5::Core Cairo::Cairo Cairo::XCB Cairo::EGL Fcitx5::Module::XCB Fcitx5::Module::Wayland XCB::AUX Wayland::Client EGL::EGL Fcitx5::Wayland::Core)
set_target_properties(classicui PROPERTIES PREFIX "")
install(TARGETS classicui DESTINATION "${FCITX_INSTALL_ADDONDIR}")
install(FILES classicui.conf DESTINATION "${FCITX_INSTALL_PKGDATADIR}/addon")
