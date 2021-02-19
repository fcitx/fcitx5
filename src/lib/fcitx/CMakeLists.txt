set(FCITX_CORE_SOURCES
    addoninfo.cpp
    instance.cpp
    inputcontextmanager.cpp
    inputcontextproperty.cpp
    focusgroup.cpp
    inputcontext.cpp
    surroundingtext.cpp
    addonloader.cpp
    addonmanager.cpp
    addoninstance.cpp
    addonfactory.cpp
    inputmethodgroup.cpp
    inputmethodmanager.cpp
    inputmethodentry.cpp
    globalconfig.cpp
    text.cpp
    event.cpp
    action.cpp
    menu.cpp
    userinterface.cpp
    userinterfacemanager.cpp
    statusarea.cpp
    inputpanel.cpp
    candidatelist.cpp
    icontheme.cpp
    )

set(FCITX_CORE_HEADERS
    addoninfo.h
    addoninstance.h
    addoninstance_details.h
    addonfactory.h
    instance.h
    inputcontextmanager.h
    inputcontext.h
    inputcontextproperty.h
    inputpanel.h
    candidatelist.h
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
    userinterface.h
    userinterfacemanager.h
    icontheme.h
    statusarea.h
    ${CMAKE_CURRENT_BINARY_DIR}/fcitxcore_export.h
    )

ecm_setup_version(PROJECT
                  VARIABLE_PREFIX Fcitx5Core
                  PACKAGE_VERSION_FILE "${CMAKE_CURRENT_BINARY_DIR}/Fcitx5CoreConfigVersion.cmake"
                  SOVERSION 7)

add_library(Fcitx5Core SHARED ${FCITX_CORE_SOURCES})
set_target_properties(Fcitx5Core
  PROPERTIES VERSION ${Fcitx5Core_VERSION}
  SOVERSION ${Fcitx5Core_SOVERSION}
  EXPORT_NAME Core
  )
target_include_directories(Fcitx5Core PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/..>
    $<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}>
    $<INSTALL_INTERFACE:${CMAKE_INSTALL_FULL_INCLUDEDIR}/Fcitx5/Core>)
target_link_libraries(Fcitx5Core PUBLIC Fcitx5::Config Fcitx5::Utils PRIVATE LibUUID::LibUUID LibIntl::LibIntl ${FMT_TARGET})
if (ENABLE_KEYBOARD)
    target_link_libraries(Fcitx5Core PRIVATE XKBCommon::XKBCommon)
endif()

configure_file(Fcitx5Core.pc.in ${CMAKE_CURRENT_BINARY_DIR}/Fcitx5Core.pc @ONLY)

install(TARGETS Fcitx5Core EXPORT Fcitx5CoreTargets LIBRARY DESTINATION "${CMAKE_INSTALL_LIBDIR}")
install(FILES ${CMAKE_CURRENT_BINARY_DIR}/Fcitx5Core.pc
  DESTINATION "${CMAKE_INSTALL_LIBDATADIR}/pkgconfig")
install(FILES ${FCITX_CORE_HEADERS} DESTINATION "${CMAKE_INSTALL_INCLUDEDIR}/Fcitx5/Core/fcitx")

add_library(Fcitx5::Core ALIAS Fcitx5Core)

generate_export_header(Fcitx5Core BASE_NAME FcitxCore)

configure_package_config_file("${CMAKE_CURRENT_SOURCE_DIR}/Fcitx5CoreConfig.cmake.in"
                              "${CMAKE_CURRENT_BINARY_DIR}/Fcitx5CoreConfig.cmake"
                              INSTALL_DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/Fcitx5Core
)

install(EXPORT Fcitx5CoreTargets DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/Fcitx5Core FILE Fcitx5CoreTargets.cmake NAMESPACE Fcitx5::)

install(FILES  "${CMAKE_CURRENT_BINARY_DIR}/Fcitx5CoreConfig.cmake"
               "${CMAKE_CURRENT_BINARY_DIR}/Fcitx5CoreConfigVersion.cmake"
        DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/Fcitx5Core
        COMPONENT Devel )
