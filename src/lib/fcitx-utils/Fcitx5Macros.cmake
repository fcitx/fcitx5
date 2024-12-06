set(_Fcitx5Macro_SELF "${CMAKE_CURRENT_LIST_FILE}")
get_filename_component(_Fcitx5Macro_SELF_DIR "${_Fcitx5Macro_SELF}" PATH)

option(BUILD_SHARED_FCITX_ADDON "Build addon as shared library" On)

if (BUILD_SHARED_FCITX_ADDON)
    set(FCITX_ADDON_CMAKE_LIBRARY_TYPE MODULE)
    set(FCITX_ADDON_TYPE "SharedLibrary")
else()
    set(FCITX_ADDON_CMAKE_LIBRARY_TYPE STATIC)
    set(FCITX_ADDON_TYPE "StaticLibrary")
endif()

include(WriteBasicConfigVersionFile)

function(fcitx5_import_addons target)
  set(options)
  set(one_value_args REGISTRY_VARNAME)
  set(multi_value_args ADDONS)
  cmake_parse_arguments(FCITX5_IMPORT
    "${options}" "${one_value_args}" "${multi_value_args}" ${ARGN})

  foreach(addon IN LISTS FCITX5_IMPORT_ADDONS)
      set(filename "${CMAKE_CURRENT_BINARY_DIR}/${target}-${addon}-import-addon.cpp")
      file(WRITE "${filename}" "
#include <fcitx/addonloader.h>
extern fcitx::StaticAddonRegistry ${FCITX5_IMPORT_REGISTRY_VARNAME};
FCITX_IMPORT_ADDON_FACTORY(${FCITX5_IMPORT_REGISTRY_VARNAME}, ${addon});
")
      target_sources(${target} PRIVATE ${filename})
      target_link_libraries(${target} ${addon})
  endforeach()

endfunction()

function(fcitx5_get_addon_targets OUT)
    cmake_parse_arguments(ARG "" "" "" ${ARGN})
    set(dirs ${ARG_UNPARSED_ARGUMENTS})
    set(_addon_targets)
    foreach(dir IN LISTS dirs)
        get_property(targets DIRECTORY "${dir}" PROPERTY BUILDSYSTEM_TARGETS)
        foreach(target IN LISTS targets)
            get_target_property(is_fcitx_addon ${target} FCITX_ADDON)
            if (is_fcitx_addon)
                list(APPEND _addon_targets ${target})
            endif()
        endforeach()

        get_property(subdirs DIRECTORY "${dir}" PROPERTY SUBDIRECTORIES)

        fcitx5_get_addon_targets(_subdir_addon_targets ${subdirs})
        list(APPEND _addon_targets ${_subdir_addon_targets})
    endforeach()

    set(${OUT} ${_addon_targets} PARENT_SCOPE)
endfunction()

function(fcitx5_download tgt_name url output sha256sum)
  get_filename_component(output "${output}" ABSOLUTE)
  set(FCITX5_DOWNLOAD_URL "${url}")
  set(FCITX5_DOWNLOAD_DEST "${output}")
  set(FCITX5_DOWNLOAD_SHA256 "${sha256sum}")
  configure_file("${_Fcitx5Macro_SELF_DIR}/Fcitx5Download.cmake.in"
                 "${CMAKE_CURRENT_BINARY_DIR}/${tgt_name}-download.cmake"
                 @ONLY)
  add_custom_target("${tgt_name}" ALL
      COMMAND "${CMAKE_COMMAND}" -P "${CMAKE_CURRENT_BINARY_DIR}/${tgt_name}-download.cmake")
  # This is the rule to create the target file, it is depending of the target
  # that does the real download so any files/targets that is depending on this
  # file will be run after the download finished.
  #
  # Since this rule doesn't have any command or file dependencies, cmake
  # won't notice any change in the rule and therefore it won't remove the
  # target file (and therefore triggers an unwilling redownload) if the real
  # rule (which is in the target defined above) has changed.
  #
  # This behavior is designed to be friendly for a build from cache with all
  # necessary files already downloaded so that a change in the
  # build options/url/checksum will not cause cmake to remove the target file
  # if it has already been updated correctly.
  add_custom_command(OUTPUT "${output}" DEPENDS "${tgt_name}")
endfunction()

function(fcitx5_extract tgt_name ifile)
  set(options)
  set(one_value_args)
  set(multi_value_args OUTPUT DEPENDS)
  cmake_parse_arguments(FCITX5_EXTRACT
    "${options}" "${one_value_args}" "${multi_value_args}" ${ARGN})
  set(STAMP_FILE "${CMAKE_CURRENT_BINARY_DIR}/.${tgt_name}.stamp")
  get_filename_component(ifile "${ifile}" ABSOLUTE)
  add_custom_command(OUTPUT "${STAMP_FILE}"
    COMMAND "${CMAKE_COMMAND}" -E tar x "${ifile}"
    COMMAND "${CMAKE_COMMAND}" -E touch "${STAMP_FILE}"
    COMMAND "${CMAKE_COMMAND}" -E touch_nocreate ${FCITX5_EXTRACT_OUTPUT}
    DEPENDS ${FCITX5_EXTRACT_DEPENDS} "${ifile}")
  add_custom_target("${tgt_name}" ALL DEPENDS "${STAMP_FILE}")
  add_custom_command(OUTPUT ${FCITX5_EXTRACT_OUTPUT}
    DEPENDS "${tgt_name}")
endfunction()


set(_Fcitx5ModuleMacros_SELF "${CMAKE_CURRENT_LIST_FILE}")
get_filename_component(_Fcitx5ModuleMacros_SELF_DIR "${_Fcitx5ModuleMacros_SELF}" PATH)

macro(fcitx5_export_module EXPORTNAME)
    set(options INSTALL)
    set(one_value_args BUILD_INCLUDE_DIRECTORIES TARGET INCLUDE_INSTALL_DIR LIB_INSTALL_DIR
                       COMPATIBILITY VERSION)
    set(multi_value_args HEADERS)
    cmake_parse_arguments(FEM
        "${options}" "${one_value_args}" "${multi_value_args}" ${ARGN})
    if (FEM_INCLUDE_INSTALL_DIR)
        set(_MODULE_HEADER_DIR "${FEM_INCLUDE_INSTALL_DIR}")
    else()
        set(_MODULE_HEADER_DIR "${FCITX_INSTALL_MODULE_HEADER_DIR}/${FEM_TARGET}")
    endif()
    if (NOT FEM_LIB_INSTALL_DIR)
        set(FEM_LIB_INSTALL_DIR "${FCITX_INSTALL_LIBDIR}")
    endif()
    set(FEM_EXPORTNAME "${EXPORTNAME}")
    add_library(${FEM_TARGET}-interface INTERFACE)
    add_library(Fcitx5::Module::${EXPORTNAME} ALIAS ${FEM_TARGET}-interface)
    set_target_properties(${FEM_TARGET}-interface PROPERTIES INTERFACE_INCLUDE_DIRECTORIES "${FEM_BUILD_INCLUDE_DIRECTORIES}")
    configure_file(
        "${_Fcitx5Macro_SELF_DIR}/Fcitx5ModuleTemplate.cmake.in"
        "${CMAKE_CURRENT_BINARY_DIR}/Fcitx5Module${EXPORTNAME}.cmake" @ONLY)
    configure_file(
        "${_Fcitx5Macro_SELF_DIR}/Fcitx5ModuleTemplate.cmake.in"
        "${CMAKE_CURRENT_BINARY_DIR}/Fcitx5Module${EXPORTNAME}Config.cmake" @ONLY)

    if (NOT FEM_VERSION)
        set(FEM_VERSION ${PROJECT_VERSION})
    endif()
    write_basic_package_version_file("${CMAKE_CURRENT_BINARY_DIR}/Fcitx5Module${EXPORTNAME}ConfigVersion.cmake" VERSION "${FEM_VERSION}" COMPATIBILITY AnyNewerVersion)
    if(FEM_INSTALL)
        install(FILES "${CMAKE_CURRENT_BINARY_DIR}/Fcitx5Module${EXPORTNAME}Config.cmake"
                      "${CMAKE_CURRENT_BINARY_DIR}/Fcitx5Module${EXPORTNAME}ConfigVersion.cmake" DESTINATION "${FEM_LIB_INSTALL_DIR}/cmake/Fcitx5Module${EXPORTNAME}")
        install(FILES ${FEM_HEADERS} DESTINATION "${_MODULE_HEADER_DIR}" COMPONENT header)
    endif()
endmacro()

function(_fcitx5_get_unique_target_name _name _unique_name)
  set(propertyName "_FCITX5_UNIQUE_COUNTER_${_name}")
  get_property(currentCounter GLOBAL PROPERTY "${propertyName}")
  if(NOT currentCounter)
    set(${_unique_name} "${_name}" PARENT_SCOPE)
    set(currentCounter 1)
  else()
    set(${_unique_name} "${_name}${currentCounter}" PARENT_SCOPE)
    math(EXPR currentCounter "${currentCounter} + 1")
  endif()
  set_property(GLOBAL PROPERTY ${propertyName} ${currentCounter} )
endfunction()

function(fcitx5_translate_desktop_file SRC DEST)
  set(options XML)
  set(one_value_args PO_DIRECTORY)
  set(multi_value_args KEYWORDS)
  cmake_parse_arguments(FCITX5_TRANSLATE
    "${options}" "${one_value_args}" "${multi_value_args}" ${ARGN})

  if (NOT IS_ABSOLUTE ${SRC})
    set(SRC "${CMAKE_CURRENT_SOURCE_DIR}/${SRC}")
  endif()
  if (NOT IS_ABSOLUTE ${DEST})
    set(DEST "${CMAKE_CURRENT_BINARY_DIR}/${DEST}")
  endif()
  get_filename_component(SRC_BASE ${SRC} NAME)

  set(KEYWORD_ARGS)
  if (NOT FCITX5_TRANSLATE_PO_DIRECTORY)
    set(FCITX5_TRANSLATE_PO_DIRECTORY "${PROJECT_SOURCE_DIR}/po")
  endif()
  file(GLOB PO_FILES "${FCITX5_TRANSLATE_PO_DIRECTORY}/*.po")

  if (FCITX5_TRANSLATE_KEYWORDS)
    list(APPEND KEYWORD_ARGS "--keyword=")
    foreach(KEYWORD IN LISTS FCITX5_TRANSLATE_KEYWORDS)
      list(APPEND KEYWORD_ARGS "--keyword=${KEYWORD}")
    endforeach()
  endif()

  if (FCITX5_TRANSLATE_XML)
    set(TYPE_ARG "--xml")
  else()
    set(TYPE_ARG "--desktop")
  endif()

  add_custom_command(OUTPUT "${DEST}"
    COMMAND "${GETTEXT_MSGFMT_EXECUTABLE}" "${TYPE_ARG}" -d ${FCITX5_TRANSLATE_PO_DIRECTORY}
            ${KEYWORD_ARGS} --template "${SRC}" -o "${DEST}"
    DEPENDS "${SRC}" ${PO_FILES})
  _fcitx5_get_unique_target_name("${SRC_BASE}-fmt" uniqueTargetName)
  add_custom_target("${uniqueTargetName}" ALL DEPENDS "${DEST}")

  if (TARGET generate-desktop-file)
    add_dependencies(generate-desktop-file "${uniqueTargetName}")
  endif()
endfunction()

# Gettext function are not good for our use case.
# GETTEXT_CREATE_TRANSLATIONS will call msgmerge which may update po file
function(fcitx5_install_translation domain)
  file(GLOB PO_FILES RELATIVE "${CMAKE_CURRENT_SOURCE_DIR}" *.po)
  set(MO_FILES)
  foreach(PO_FILE IN LISTS PO_FILES)
    get_filename_component(PO_LANG ${PO_FILE} NAME_WE)
    get_filename_component(ABS_PO_FILE ${PO_FILE} ABSOLUTE)
    set(MO_FILE ${CMAKE_CURRENT_BINARY_DIR}/${domain}-${PO_LANG}.mo)

    add_custom_command(
        OUTPUT ${MO_FILE}
        COMMAND ${GETTEXT_MSGFMT_EXECUTABLE} -o ${MO_FILE} ${ABS_PO_FILE}
        DEPENDS ${ABS_PO_FILE}
    )

    install(FILES ${MO_FILE} RENAME ${domain}.mo DESTINATION ${FCITX_INSTALL_LOCALEDIR}/${PO_LANG}/LC_MESSAGES
            COMPONENT translation)
    set(MO_FILES ${MO_FILES} ${MO_FILE})
  endforeach ()
  add_custom_target("${domain}-translation" ALL DEPENDS ${MO_FILES})
  if (TARGET translation-file)
    add_dependencies(translation-file "${domain}-translation")
  endif()

endfunction()

function(fcitx5_add_i18n_definition)
  set(options)
  set(one_value_args LOCALE_INSTALL_DIR)
  set(multi_value_args TARGETS)
  cmake_parse_arguments(FCITX5_AID
    "${options}" "${one_value_args}" "${multi_value_args}" ${ARGN})

  if (FCITX5_AID_LOCALE_INSTALL_DIR)
    set(_LOCALE_DIR "${FCITX5_AID_LOCALE_INSTALL_DIR}")
  else()
    set(_LOCALE_DIR "${FCITX_INSTALL_LOCALEDIR}")
  endif()
  string(CONFIGURE ${_LOCALE_DIR} LOCALE_DIR ESCAPE_QUOTES)

  if(FCITX5_AID_TARGETS)
    message(${FCITX5_AID_TARGETS})
    foreach(TARGET_NAME IN LISTS FCITX5_AID_TARGETS)
        target_compile_definitions(${TARGET_NAME} PRIVATE -DFCITX_INSTALL_LOCALEDIR=\"${LOCALE_DIR}\")
    endforeach()
  else()
    add_definitions(-DFCITX_INSTALL_LOCALEDIR=\"${LOCALE_DIR}\")
  endif()
endfunction()


if (NOT TARGET generate-desktop-file)
    add_custom_target(generate-desktop-file)
endif()

if (NOT TARGET translation-file)
    add_custom_target(translation-file)
endif()

function(add_fcitx5_addon target_name)
  cmake_parse_arguments(ARG "" "" "" ${ARGN})
  set(srcs ${ARG_UNPARSED_ARGUMENTS})

  add_library(${target_name} ${FCITX_ADDON_CMAKE_LIBRARY_TYPE} ${srcs} )
  set_target_properties(${target_name} PROPERTIES FCITX_ADDON TRUE)
endfunction()
