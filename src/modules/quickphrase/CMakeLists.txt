add_library(quickphrase MODULE quickphrase.cpp)
target_link_libraries(quickphrase Fcitx5::Core Fcitx5::Module::Spell)
set_target_properties(quickphrase PROPERTIES PREFIX "")
install(TARGETS quickphrase DESTINATION "${FCITX_INSTALL_ADDONDIR}")
fcitx5_translate_conf_file(quickphrase.conf.in quickphrase.conf)
install(FILES "${CMAKE_CURRENT_BINARY_DIR}/quickphrase.conf" DESTINATION "${FCITX_INSTALL_PKGDATADIR}/addon")
fcitx5_export_module(QuickPhrase TARGET quickphrase BUILD_INCLUDE_DIRECTORIES "${CMAKE_CURRENT_SOURCE_DIR}" HEADERS quickphrase_public.h INSTALL)

install(DIRECTORY quickphrase.d DESTINATION "${FCITX_INSTALL_PKGDATADIR}/data"
  FILES_MATCHING PATTERN "*.mb")

