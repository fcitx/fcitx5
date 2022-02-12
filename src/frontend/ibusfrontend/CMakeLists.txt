add_library(ibusfrontend MODULE ibusfrontend.cpp)
target_link_libraries(ibusfrontend Fcitx5::Core Fcitx5::Module::DBus ${FMT_TARGET})
install(TARGETS ibusfrontend DESTINATION "${FCITX_INSTALL_ADDONDIR}")
configure_file(ibusfrontend.conf.in.in ibusfrontend.conf.in @ONLY)
fcitx5_translate_desktop_file(${CMAKE_CURRENT_BINARY_DIR}/ibusfrontend.conf.in ibusfrontend.conf)
install(FILES "${CMAKE_CURRENT_BINARY_DIR}/ibusfrontend.conf" DESTINATION "${FCITX_INSTALL_PKGDATADIR}/addon"
        COMPONENT config)

if (ENABLE_LIBUUID)
target_link_libraries(ibusfrontend LibUUID::LibUUID)
endif()
