set(WAYLAND_IM_PROTOCOL_SRCS)

ecm_add_wayland_client_protocol(WAYLAND_IM_PROTOCOL_SRCS
    PROTOCOL ${WAYLAND_PROTOCOLS_PKGDATADIR}/unstable/text-input/text-input-unstable-v1.xml
    BASENAME text-input-unstable-v1)

ecm_add_wayland_client_protocol(WAYLAND_IM_PROTOCOL_SRCS
    PROTOCOL ${WAYLAND_PROTOCOLS_PKGDATADIR}/unstable/text-input/text-input-unstable-v3.xml
    BASENAME text-input-unstable-v3)

add_library(waylandim MODULE waylandim.cpp waylandimserver.cpp waylandimserverv2.cpp plasmaappmonitor.cpp virtualinputcontext.cpp  wlrappmonitor.cpp appmonitor.cpp ${WAYLAND_IM_PROTOCOL_SRCS})
target_include_directories(waylandim PRIVATE ${CMAKE_CURRENT_BINARY_DIR})
target_link_libraries(waylandim Fcitx5::Core Wayland::Client XKBCommon::XKBCommon Fcitx5::Wayland::PlasmaWindowManagement Fcitx5::Wayland::WLRForeignToplevelManagement Fcitx5::Module::Wayland Fcitx5::Wayland::Core Fcitx5::Wayland::InputMethod Fcitx5::Wayland::InputMethodV2)
install(TARGETS waylandim DESTINATION "${FCITX_INSTALL_ADDONDIR}")
configure_file(${CMAKE_CURRENT_SOURCE_DIR}/waylandim.conf.in.in ${CMAKE_CURRENT_BINARY_DIR}/waylandim.conf.in @ONLY)
fcitx5_translate_desktop_file(${CMAKE_CURRENT_BINARY_DIR}/waylandim.conf.in waylandim.conf)
install(FILES "${CMAKE_CURRENT_BINARY_DIR}/waylandim.conf" DESTINATION "${FCITX_INSTALL_PKGDATADIR}/addon"
        COMPONENT config)

fcitx5_export_module(WaylandIM TARGET waylandim BUILD_INCLUDE_DIRECTORIES "${CMAKE_CURRENT_SOURCE_DIR}" HEADERS waylandim_public.h)
