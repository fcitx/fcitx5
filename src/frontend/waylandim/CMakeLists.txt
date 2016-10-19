set(WAYLAND_IM_PROTOCOL_SRCS)

ecm_add_wayland_client_protocol(WAYLAND_IM_PROTOCOL_SRCS
    PROTOCOL ${WAYLAND_PROTOCOLS_PKGDATADIR}/unstable/input-method/input-method-unstable-v1.xml
    BASENAME input-method-unstable-v1)

ecm_add_wayland_client_protocol(WAYLAND_IM_PROTOCOL_SRCS
    PROTOCOL ${WAYLAND_PROTOCOLS_PKGDATADIR}/unstable/text-input/text-input-unstable-v1.xml
    BASENAME text-input-unstable-v1)

add_library(waylandim MODULE waylandim.cpp ${WAYLAND_IM_PROTOCOL_SRCS})
target_include_directories(waylandim PRIVATE ${CMAKE_CURRENT_BINARY_DIR})
target_link_libraries(waylandim Fcitx5::Core Wayland::Client XKBCommon::XKBCommon)
set_target_properties(waylandim PROPERTIES PREFIX "")
install(TARGETS waylandim DESTINATION "${FCITX_INSTALL_ADDONDIR}")
install(FILES waylandim.conf DESTINATION "${FCITX_INSTALL_PKGDATADIR}/addon")

