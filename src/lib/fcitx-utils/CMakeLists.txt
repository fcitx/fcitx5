if (NOT TARGET Systemd::Systemd)
  set(FCITX_UTILS_SOURCES
      dbus/libdbus/bus.cpp
      dbus/libdbus/message.cpp
      dbus/libdbus/objectvtable_libdbus.cpp
      dbus/libdbus/servicenamecache.cpp)
else()
  set(FCITX_UTILS_SOURCES
      dbus/sdbus/bus.cpp
      dbus/sdbus/message.cpp
      dbus/sdbus/objectvtablewrapper.c
      dbus/sdbus/objectvtable_sdbus.cpp)
endif()

if (NOT TARGET Systemd::Systemd)
  set(FCITX_UTILS_SOURCES
    ${FCITX_UTILS_SOURCES}
    event_libevent.cpp)
else()
  set(FCITX_UTILS_SOURCES
    ${FCITX_UTILS_SOURCES}
    event_sdevent.cpp)
endif()

set(FCITX_UTILS_SOURCES
    ${FCITX_UTILS_SOURCES}
    dbus/servicewatcher.cpp
    dbus/matchrule.cpp
    dbus/variant.cpp
    dbus/objectvtable.cpp
    stringutils.cpp
    testing.cpp
    key.cpp
    cutf8.cpp
    color.cpp
    i18nstring.cpp
    event_common.cpp
    eventdispatcher.cpp
    library.cpp
    fs.cpp
    standardpath.cpp
    unixfd.cpp
    utf8.cpp
    connectableobject.cpp
    element.cpp
    i18n.cpp
    inputbuffer.cpp
    log.cpp
    misc.cpp
    )

set(FCITX_UTILS_HEADERS
    macros.h
    stringutils.h
    stringutils_details.h
    key.h
    color.h
    i18nstring.h
    event.h
    eventdispatcher.h
    library.h
    cutf8.h
    fs.h
    standardpath.h
    tuplehelpers.h
    metastring.h
    unixfd.h
    trackableobject.h
    connectableobject.h
    flags.h
    keysym.h
    keysymgen.h
    signals.h
    signals_details.h
    handlertable.h
    handlertable_details.h
    intrusivelist.h
    misc.h
    utf8.h
    cutf8.h
    element.h
    rect.h
    charutils.h
    i18n.h
    capabilityflags.h
    textformatflags.h
    inputbuffer.h
    log.h
    testing.h
    ${CMAKE_CURRENT_BINARY_DIR}/fcitxutils_export.h
    )

set(FCITX_UTILS_DBUS_HEADERS
    dbus/message.h
    dbus/objectvtable.h
    dbus/bus.h
    dbus/message_details.h
    dbus/servicewatcher.h
    dbus/matchrule.h
    )

add_library(Fcitx5Utils SHARED ${FCITX_UTILS_SOURCES})
set_target_properties(Fcitx5Utils
  PROPERTIES VERSION 5.0
  SOVERSION 2
  EXPORT_NAME Utils
  )
target_include_directories(Fcitx5Utils PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/..>
    $<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}>
    $<INSTALL_INTERFACE:${CMAKE_INSTALL_FULL_INCLUDEDIR}/Fcitx5/Utils>)
target_link_libraries(Fcitx5Utils PRIVATE DL::DL LibIntl::LibIntl Pthread::Pthread ${FMT_TARGET})

if (NOT TARGET Systemd::Systemd)
target_link_libraries(Fcitx5Utils PRIVATE PkgConfig::DBus PkgConfig::LibEvent)
else()
target_link_libraries(Fcitx5Utils PRIVATE Systemd::Systemd)
endif()

configure_file(Fcitx5Utils.pc.in ${CMAKE_CURRENT_BINARY_DIR}/Fcitx5Utils.pc @ONLY)

install(TARGETS Fcitx5Utils EXPORT Fcitx5UtilsTargets LIBRARY DESTINATION "${CMAKE_INSTALL_LIBDIR}")
install(FILES ${CMAKE_CURRENT_BINARY_DIR}/Fcitx5Utils.pc
  DESTINATION "${CMAKE_INSTALL_LIBDIR}/pkgconfig")
install(FILES ${FCITX_UTILS_HEADERS} DESTINATION "${CMAKE_INSTALL_INCLUDEDIR}/Fcitx5/Utils/fcitx-utils")
install(FILES ${FCITX_UTILS_DBUS_HEADERS} DESTINATION "${CMAKE_INSTALL_INCLUDEDIR}/Fcitx5/Utils/fcitx-utils/dbus")

add_library(Fcitx5::Utils ALIAS Fcitx5Utils)

ecm_setup_version(PROJECT
                  PACKAGE_VERSION_FILE "${CMAKE_CURRENT_BINARY_DIR}/Fcitx5UtilsConfigVersion.cmake"
                  SOVERSION 2)

configure_package_config_file("${CMAKE_CURRENT_SOURCE_DIR}/Fcitx5UtilsConfig.cmake.in"
                              "${CMAKE_CURRENT_BINARY_DIR}/Fcitx5UtilsConfig.cmake"
                              INSTALL_DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/Fcitx5Utils
)

generate_export_header(Fcitx5Utils BASE_NAME FcitxUtils)

install(EXPORT Fcitx5UtilsTargets DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/Fcitx5Utils FILE Fcitx5UtilsTargets.cmake NAMESPACE Fcitx5::)

install(FILES  "${CMAKE_CURRENT_BINARY_DIR}/Fcitx5UtilsConfig.cmake"
               "${CMAKE_CURRENT_BINARY_DIR}/Fcitx5UtilsConfigVersion.cmake"
               Fcitx5Macros.cmake
               Fcitx5Download.cmake.in
               Fcitx5ModuleTemplate.cmake.in
               "${CMAKE_SOURCE_DIR}/cmake/Fcitx5CompilerSettings.cmake"
        DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/Fcitx5Utils
        COMPONENT Devel )
