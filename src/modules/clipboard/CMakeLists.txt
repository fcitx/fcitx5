add_library(clipboard MODULE clipboard.cpp)
target_link_libraries(clipboard Fcitx5::Core Fcitx5::Module::XCB)
set_target_properties(clipboard PROPERTIES PREFIX "")
install(TARGETS clipboard DESTINATION "${FCITX_INSTALL_ADDONDIR}")
fcitx5_translate_desktop_file(clipboard.conf.in clipboard.conf)
install(FILES "${CMAKE_CURRENT_BINARY_DIR}/clipboard.conf" DESTINATION "${FCITX_INSTALL_PKGDATADIR}/addon")
fcitx5_export_module(Clipboard TARGET clipboard BUILD_INCLUDE_DIRECTORIES "${CMAKE_CURRENT_SOURCE_DIR}" HEADERS clipboard_public.h INSTALL)

