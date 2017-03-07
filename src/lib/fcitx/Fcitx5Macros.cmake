set(_Fcitx5Macro_SELF "${CMAKE_CURRENT_LIST_FILE}")
get_filename_component(_Fcitx5Macro_SELF_DIR "${_Fcitx5Macro_SELF}" PATH)

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
