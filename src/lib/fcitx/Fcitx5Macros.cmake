set(_Fcitx5Macro_SELF "${CMAKE_CURRENT_LIST_FILE}")
get_filename_component(_Fcitx5Macro_SELF_DIR "${_Fcitx5Macro_SELF}" PATH)

include(WriteBasicConfigVersionFile)

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
        install(FILES ${FEM_HEADERS} DESTINATION "${_MODULE_HEADER_DIR}")
    endif()
endmacro()
