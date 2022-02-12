add_library(unicode MODULE unicode.cpp charselectdata.cpp)
target_link_libraries(unicode Fcitx5::Core Fcitx5::Module::Clipboard ${FMT_TARGET})
install(TARGETS unicode DESTINATION "${FCITX_INSTALL_ADDONDIR}")
configure_file(unicode.conf.in.in unicode.conf.in @ONLY)
fcitx5_translate_desktop_file(${CMAKE_CURRENT_BINARY_DIR}/unicode.conf.in unicode.conf)
install(FILES "${CMAKE_CURRENT_BINARY_DIR}/unicode.conf" DESTINATION "${FCITX_INSTALL_PKGDATADIR}/addon"
        COMPONENT config)
fcitx5_export_module(Unicode TARGET unicode BUILD_INCLUDE_DIRECTORIES "${CMAKE_CURRENT_SOURCE_DIR}" HEADERS unicode_public.h INSTALL)

install(FILES charselectdata DESTINATION "${FCITX_INSTALL_PKGDATADIR}/unicode")

