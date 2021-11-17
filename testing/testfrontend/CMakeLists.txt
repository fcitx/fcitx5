add_library(testfrontend MODULE testfrontend.cpp)

target_link_libraries(testfrontend
    Fcitx5::Core)
install(TARGETS testfrontend DESTINATION "${FCITX_INSTALL_ADDONDIR}")
install(FILES testfrontend.conf DESTINATION "${FCITX_INSTALL_PKGDATADIR}/testing/addon")

fcitx5_export_module(TestFrontend TARGET testfrontend BUILD_INCLUDE_DIRECTORIES "${CMAKE_CURRENT_SOURCE_DIR}" HEADERS testfrontend_public.h INSTALL)
