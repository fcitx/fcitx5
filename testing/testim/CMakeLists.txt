add_fcitx5_addon(testim testim.cpp)

target_link_libraries(testim
    Fcitx5::Core)
install(TARGETS testim DESTINATION "${FCITX_INSTALL_ADDONDIR}")
install(FILES testim.conf DESTINATION "${FCITX_INSTALL_PKGDATADIR}/testing/addon")

fcitx5_export_module(TestIM TARGET testim BUILD_INCLUDE_DIRECTORIES "${CMAKE_CURRENT_SOURCE_DIR}" HEADERS testim_public.h INSTALL)
