set(DICT_COMP_SRC
  comp_spell_dict.c
  )

add_executable(comp-spell-dict ${DICT_COMP_SRC})
add_executable(Fcitx5::comp-spell-dict ALIAS comp-spell-dict)
target_link_libraries(comp-spell-dict fcitx-utils)
install(TARGETS comp-spell-dict
  DESTINATION "${FCITX_INSTALL_ADDONDIR}/libexec")

set(SPELL_EN_DICT_VER 20121020)
set(SPELL_EN_DICT "${CMAKE_CURRENT_BINARY_DIR}/en_dict.fscd")
set(SPELL_EN_DICT_SRC "${CMAKE_CURRENT_BINARY_DIR}/en_dict.txt")
set(SPELL_EN_DICT_TAR "en_dict-${SPELL_EN_DICT_VER}.tar.gz")
set(SPELL_EN_DICT_URL
  "https://download.fcitx-im.org/data/en_dict-${SPELL_EN_DICT_VER}.tar.gz")

fcitx5_download(spell-en-download ${SPELL_EN_DICT_URL} ${SPELL_EN_DICT_TAR}
                c44a5d7847925eea9e4d2d04748d442cd28dd9299a0b572ef7d91eac4f5a6ceb)
fcitx5_extract(spell-en-extract "${SPELL_EN_DICT_TAR}" DEPENDS spell-en-download
  OUTPUT ${SPELL_EN_DICT_SRC})

add_custom_command(
  OUTPUT "${SPELL_EN_DICT}"
  DEPENDS "${SPELL_EN_DICT_SRC}" Fcitx5::comp-spell-dict spell-en-extract
  COMMAND Fcitx5::comp-spell-dict --comp-dict
  "${SPELL_EN_DICT_SRC}" "${SPELL_EN_DICT}")
add_custom_target(spell_en_dict ALL DEPENDS "${SPELL_EN_DICT}")
install(FILES "${SPELL_EN_DICT}" DESTINATION "${FCITX_INSTALL_PKGDATADIR}/spell")
