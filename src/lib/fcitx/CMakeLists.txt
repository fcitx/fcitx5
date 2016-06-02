set(FCITX_CORE_SOURCES
    addoninfo.cpp
    instance.cpp
    inputcontextmanager.cpp
    focusgroup.cpp
    inputcontext.cpp
    surroundingtext.cpp
    addonloader.cpp
    addonmanager.cpp
    addonfactory.cpp
    )

set(FCITX_CORE_HEADERS
    addoninfo.h
    instance.h
    inputcontextmanager.h
    focusgroup.h
    surroundingtext.h
    addonloader.h
    addonmanager.h
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
    "$<INSTALL_INTERFACE:${CMAKE_INSTALL_FULL_INCLUDEDIR}>/Fcitx5/FcitxCore")
target_link_libraries(Fcitx5Core PUBLIC Fcitx5::Config Fcitx5::Utils PRIVATE LibUUID::LibUUID)

configure_file(Fcitx5Core.pc.in ${CMAKE_CURRENT_BINARY_DIR}/Fcitx5Core.pc @ONLY)

install(TARGETS Fcitx5Core EXPORT FcitxCoreTargets LIBRARY DESTINATION "${CMAKE_INSTALL_LIBDIR}")
install(FILES ${CMAKE_CURRENT_BINARY_DIR}/Fcitx5Core.pc
  DESTINATION "${CMAKE_INSTALL_LIBDATADIR}/pkgconfig")
install(FILES ${FCITX_CORE_HEADERS} DESTINATION "${CMAKE_INSTALL_INCLUDEDIR}/Fcitx5/FcitxCore/fcitx")

add_library(Fcitx5::Core ALIAS Fcitx5Core)

generate_export_header(Fcitx5Core BASE_NAME FcitxCore)
ecm_setup_version(PROJECT
                  PACKAGE_VERSION_FILE "${CMAKE_CURRENT_BINARY_DIR}/FcitxCoreConfigVersion.cmake"
                  SOVERSION 5)

configure_package_config_file("${CMAKE_CURRENT_SOURCE_DIR}/FcitxCoreConfig.cmake.in"
                              "${CMAKE_CURRENT_BINARY_DIR}/FcitxCoreConfig.cmake"
                              INSTALL_DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/FcitxCore
)

install(EXPORT FcitxCoreTargets DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/FcitxCore FILE FcitxCoreTargets.cmake NAMESPACE Fcitx5::)

install(FILES  "${CMAKE_CURRENT_BINARY_DIR}/FcitxCoreConfig.cmake"
               "${CMAKE_CURRENT_BINARY_DIR}/FcitxCoreConfigVersion.cmake"
        DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/FcitxCore
        COMPONENT Devel )
