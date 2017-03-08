set(FCITX_CORE_SOURCES
    addoninfo.cpp
    instance.cpp
    inputcontextmanager.cpp
    focusgroup.cpp
    inputcontext.cpp
    surroundingtext.cpp
    addonloader.cpp
    addonmanager.cpp
    addoninstance.cpp
    addonfactory.cpp
    inputmethodgroup.cpp
    inputmethodmanager.cpp
    globalconfig.cpp
    text.cpp
    event.cpp
    action.cpp
    menu.cpp
    userinterface.cpp
    userinterfacemanager.cpp
    element.cpp
    statusarea.cpp
    inputpanel.cpp
    candidatelist.cpp
    )

set(FCITX_CORE_HEADERS
    addoninfo.h
    instance.h
    inputcontextmanager.h
    focusgroup.h
    surroundingtext.h
    addonloader.h
    addonmanager.h
    inputmethodmanager.h
    inputmethodgroup.h
    inputmethodengine.h
    inputmethodentry.h
    globalconfig.h
    text.h
    event.h
    action.h
    menu.h
    ${CMAKE_CURRENT_BINARY_DIR}/fcitxcore_export.h
    )


add_library(Fcitx5Core SHARED ${FCITX_CORE_SOURCES})
set_target_properties(Fcitx5Core
  PROPERTIES VERSION 5.0
  SOVERSION 5
  COMPILE_FLAGS "-fvisibility=hidden"
  LINK_FLAGS "-Wl,--no-undefined"
  EXPORT_NAME Core
  )
target_include_directories(Fcitx5Core PUBLIC
    "$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/..;${CMAKE_CURRENT_BINARY_DIR}>"
    "$<INSTALL_INTERFACE:${CMAKE_INSTALL_FULL_INCLUDEDIR}>/Fcitx5/Core")
target_link_libraries(Fcitx5Core PUBLIC Fcitx5::Config Fcitx5::Utils PRIVATE LibUUID::LibUUID)

configure_file(Fcitx5Core.pc.in ${CMAKE_CURRENT_BINARY_DIR}/Fcitx5Core.pc @ONLY)

install(TARGETS Fcitx5Core EXPORT Fcitx5CoreTargets LIBRARY DESTINATION "${CMAKE_INSTALL_LIBDIR}")
install(FILES ${CMAKE_CURRENT_BINARY_DIR}/Fcitx5Core.pc
  DESTINATION "${CMAKE_INSTALL_LIBDATADIR}/pkgconfig")
install(FILES ${FCITX_CORE_HEADERS} DESTINATION "${CMAKE_INSTALL_INCLUDEDIR}/Fcitx5/Core/fcitx")

add_library(Fcitx5::Core ALIAS Fcitx5Core)

generate_export_header(Fcitx5Core BASE_NAME FcitxCore)
ecm_setup_version(PROJECT
                  PACKAGE_VERSION_FILE "${CMAKE_CURRENT_BINARY_DIR}/Fcitx5CoreConfigVersion.cmake"
                  SOVERSION 5)

configure_package_config_file("${CMAKE_CURRENT_SOURCE_DIR}/Fcitx5CoreConfig.cmake.in"
                              "${CMAKE_CURRENT_BINARY_DIR}/Fcitx5CoreConfig.cmake"
                              INSTALL_DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/Fcitx5Core
)

install(EXPORT Fcitx5CoreTargets DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/Fcitx5Core FILE Fcitx5CoreTargets.cmake NAMESPACE Fcitx5::)

install(FILES  "${CMAKE_CURRENT_BINARY_DIR}/Fcitx5CoreConfig.cmake"
               "${CMAKE_CURRENT_BINARY_DIR}/Fcitx5CoreConfigVersion.cmake"
               "Fcitx5Macros.cmake"
               "Fcitx5Download.cmake.in"
        DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/Fcitx5Core
        COMPONENT Devel )
