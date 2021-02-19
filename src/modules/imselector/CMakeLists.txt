add_library(imselector MODULE imselector.cpp)
target_link_libraries(imselector Fcitx5::Core)
set_target_properties(imselector PROPERTIES PREFIX "")
install(TARGETS imselector DESTINATION "${FCITX_INSTALL_ADDONDIR}")
fcitx5_translate_desktop_file(imselector.conf.in imselector.conf)
install(FILES "${CMAKE_CURRENT_BINARY_DIR}/imselector.conf" DESTINATION "${FCITX_INSTALL_PKGDATADIR}/addon")

