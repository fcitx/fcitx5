set(SPELL_SOURCES
    spell.cpp spell-custom-dict.cpp spell-custom.cpp)

if (TARGET PkgConfig::Enchant)
    set(SPELL_SOURCES ${SPELL_SOURCES} spell-enchant.cpp)
endif()

add_library(spell MODULE ${SPELL_SOURCES})
target_link_libraries(spell Fcitx5::Core)
if (TARGET PkgConfig::Enchant)
    target_link_libraries(spell PkgConfig::Enchant)
endif()
install(TARGETS spell DESTINATION "${FCITX_INSTALL_ADDONDIR}")
configure_file(spell.conf.in.in spell.conf.in @ONLY)
fcitx5_translate_desktop_file(${CMAKE_CURRENT_BINARY_DIR}/spell.conf.in spell.conf)
install(FILES "${CMAKE_CURRENT_BINARY_DIR}/spell.conf" DESTINATION "${FCITX_INSTALL_PKGDATADIR}/addon"
        COMPONENT config)
fcitx5_export_module(Spell TARGET spell BUILD_INCLUDE_DIRECTORIES "${CMAKE_CURRENT_SOURCE_DIR}" HEADERS spell_public.h INSTALL)

 set(DICT_COMP_SRC
  comp_spell_dict.cpp
  )

add_executable(comp-spell-dict ${DICT_COMP_SRC})
add_executable(Fcitx5::comp-spell-dict ALIAS comp-spell-dict)
target_link_libraries(comp-spell-dict Fcitx5::Utils)
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
