add_subdirectory(xcb)

add_subdirectory(dbus)

if (WAYLAND_FOUND)
add_subdirectory(wayland)
endif()

add_subdirectory(spell)
add_subdirectory(notifications)
add_subdirectory(notificationitem)
add_subdirectory(quickphrase)
add_subdirectory(unicode)
add_subdirectory(clipboard)
add_subdirectory(emoji)
add_subdirectory(imselector)

write_basic_package_version_file("${CMAKE_CURRENT_BINARY_DIR}/Fcitx5ModuleConfigVersion.cmake" VERSION @FCITX_VERSION@ COMPATIBILITY AnyNewerVersion)

configure_file("${CMAKE_CURRENT_SOURCE_DIR}/Fcitx5ModuleConfig.cmake.in"
               "${CMAKE_CURRENT_BINARY_DIR}/Fcitx5ModuleConfig.cmake" @ONLY)

install(FILES "${CMAKE_CURRENT_BINARY_DIR}/Fcitx5ModuleConfigVersion.cmake"
              "${CMAKE_CURRENT_BINARY_DIR}/Fcitx5ModuleConfig.cmake"
        DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/Fcitx5Module
        COMPONENT Devel )

configure_file(Fcitx5Module.pc.in ${CMAKE_CURRENT_BINARY_DIR}/Fcitx5Module.pc @ONLY)
install(FILES ${CMAKE_CURRENT_BINARY_DIR}/Fcitx5Module.pc
  DESTINATION "${CMAKE_INSTALL_LIBDATADIR}/pkgconfig")
