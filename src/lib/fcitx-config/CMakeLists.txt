set(FCITX_CONFIG_SOURCES
    rawconfig.cpp
    option.cpp
    configuration.cpp
    marshallfunction.cpp
    iniparser.cpp
    )

set(FCITX_CONFIG_HEADERS
    rawconfig.h
    option.h
    configuration.h
    marshallfunction.h
    iniparser.h
    ${CMAKE_CURRENT_BINARY_DIR}/fcitxconfig_export.h
    )


add_library(Fcitx5Config SHARED ${FCITX_CONFIG_SOURCES})
set_target_properties(Fcitx5Config
  PROPERTIES VERSION 5.0
  SOVERSION 5
  COMPILE_FLAGS "-fvisibility=hidden"
  LINK_FLAGS "-Wl,--no-undefined"
  EXPORT_NAME Config
  )
target_include_directories(Fcitx5Config PUBLIC
    "$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/..;${CMAKE_CURRENT_BINARY_DIR}>"
    "$<INSTALL_INTERFACE:${CMAKE_INSTALL_FULL_INCLUDEDIR}>/Fcitx5/Config")
target_link_libraries(Fcitx5Config PUBLIC Fcitx5::Utils)

configure_file(Fcitx5Config.pc.in ${CMAKE_CURRENT_BINARY_DIR}/Fcitx5Config.pc @ONLY)

install(TARGETS Fcitx5Config EXPORT Fcitx5ConfigTargets LIBRARY DESTINATION "${CMAKE_INSTALL_LIBDIR}")
install(FILES ${CMAKE_CURRENT_BINARY_DIR}/Fcitx5Config.pc
  DESTINATION "${CMAKE_INSTALL_LIBDATADIR}/pkgconfig")
install(FILES ${FCITX_CONFIG_HEADERS} DESTINATION "${CMAKE_INSTALL_INCLUDEDIR}/Fcitx5/Config/fcitx-config")

add_library(Fcitx5::Config ALIAS Fcitx5Config)

generate_export_header(Fcitx5Config BASE_NAME FcitxConfig)
ecm_setup_version(PROJECT
                  PACKAGE_VERSION_FILE "${CMAKE_CURRENT_BINARY_DIR}/Fcitx5ConfigConfigVersion.cmake"
                  SOVERSION 5)

configure_package_config_file("${CMAKE_CURRENT_SOURCE_DIR}/Fcitx5ConfigConfig.cmake.in"
                              "${CMAKE_CURRENT_BINARY_DIR}/Fcitx5ConfigConfig.cmake"
                              INSTALL_DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/Fcitx5Config
)

install(EXPORT Fcitx5ConfigTargets DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/Fcitx5Config FILE Fcitx5ConfigTargets.cmake NAMESPACE Fcitx5::)

install(FILES  "${CMAKE_CURRENT_BINARY_DIR}/Fcitx5ConfigConfig.cmake"
               "${CMAKE_CURRENT_BINARY_DIR}/Fcitx5ConfigConfigVersion.cmake"
        DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/Fcitx5Config
        COMPONENT Devel )
