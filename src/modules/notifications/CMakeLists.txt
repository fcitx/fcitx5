add_library(notifications MODULE notifications.cpp)
target_link_libraries(notifications Fcitx5::Core Fcitx5::Module::DBus)
set_target_properties(notifications PROPERTIES PREFIX "")
install(TARGETS notifications DESTINATION "${FCITX_INSTALL_ADDONDIR}")
fcitx5_translate_conf_file(notifications.conf.in notifications.conf)
install(FILES "${CMAKE_CURRENT_BINARY_DIR}/notifications.conf" DESTINATION "${FCITX_INSTALL_PKGDATADIR}/addon")
fcitx5_export_module(Notifications TARGET notifications BUILD_INCLUDE_DIRECTORIES "${CMAKE_CURRENT_SOURCE_DIR}" HEADERS notifications_public.h INSTALL)

