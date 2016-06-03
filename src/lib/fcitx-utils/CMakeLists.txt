set(FCITX_UTILS_SOURCES
    stringutils.cpp
    key.cpp
    cutf8.cpp
    color.cpp
    i18nstring.cpp
    event_common.cpp
    event_sdevent.cpp
    dbus.cpp
    dbus-message.cpp
    library.cpp
    fs.cpp
    standardpath.cpp
    dbus-object-vtable-wrapper.c
    dbus-object-vtable.cpp
    )

set(FCITX_UTILS_HEADERS
    macros.h
    stringutils.h
    key.h
    color.h
    i18nstring.h
    event.h
    dbus.h
    library.h
    cutf8.h
    dbus-message.h
    fs.h
    standardpath.h
    tuplehelpers.h
    metastring.h
    )

add_library(Fcitx5Utils SHARED ${FCITX_UTILS_SOURCES})
set_target_properties(Fcitx5Utils
  PROPERTIES VERSION 1.0
  SOVERSION 1
  COMPILE_FLAGS "-fvisibility=hidden"
  LINK_FLAGS "-Wl,--no-undefined"
  EXPORT_NAME Utils
  )
target_include_directories(Fcitx5Utils PUBLIC
    "$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/..;${CMAKE_CURRENT_BINARY_DIR}>"
    "$<INSTALL_INTERFACE:${CMAKE_INSTALL_FULL_INCLUDEDIR}>/Fcitx5/FcitxUtils")
target_link_libraries(Fcitx5Utils PRIVATE Systemd::Systemd DL::DL)

configure_file(Fcitx5Utils.pc.in ${CMAKE_CURRENT_BINARY_DIR}/Fcitx5Utils.pc @ONLY)

install(TARGETS Fcitx5Utils EXPORT FcitxUtilsTargets LIBRARY DESTINATION "${CMAKE_INSTALL_LIBDIR}")
install(FILES ${CMAKE_CURRENT_BINARY_DIR}/Fcitx5Utils.pc
  DESTINATION "${CMAKE_INSTALL_LIBDIR}/pkgconfig")
install(FILES ${FCITX_UTILS_HEADERS} DESTINATION "${CMAKE_INSTALL_INCLUDEDIR}/Fcitx5/FcitxUtils/fcitx-utils")

add_library(Fcitx5::Utils ALIAS Fcitx5Utils)

ecm_setup_version(PROJECT
                  PACKAGE_VERSION_FILE "${CMAKE_CURRENT_BINARY_DIR}/FcitxUtilsConfigVersion.cmake"
                  SOVERSION 1)

configure_package_config_file("${CMAKE_CURRENT_SOURCE_DIR}/FcitxUtilsConfig.cmake.in"
                              "${CMAKE_CURRENT_BINARY_DIR}/FcitxUtilsConfig.cmake"
                              INSTALL_DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/FcitxUtils
)

generate_export_header(Fcitx5Utils BASE_NAME FcitxUtils)

install(EXPORT FcitxUtilsTargets DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/FcitxUtils FILE FcitxUtilsTargets.cmake NAMESPACE Fcitx5::)

install(FILES  "${CMAKE_CURRENT_BINARY_DIR}/FcitxUtilsConfig.cmake"
               "${CMAKE_CURRENT_BINARY_DIR}/FcitxUtilsConfigVersion.cmake"
        DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/FcitxUtils
        COMPONENT Devel )
