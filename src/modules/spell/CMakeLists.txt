set(SPELL_SOURCES
    spell.cpp spell-custom-dict.cpp spell-custom.cpp)

if (TARGET PkgConfig::Enchant)
    set(SPELL_SOURCES ${SPELL_SOURCES} spell-enchant.cpp)
endif()

add_subdirectory(dict)
add_library(spell MODULE ${SPELL_SOURCES})
target_link_libraries(spell Fcitx5::Core)
if (TARGET PkgConfig::Enchant)
    target_link_libraries(spell PkgConfig::Enchant)
endif()
install(TARGETS spell DESTINATION "${FCITX_INSTALL_ADDONDIR}")
configure_file(spell.conf.in.in spell.conf.in @ONLY)
fcitx5_translate_desktop_file(${CMAKE_CURRENT_BINARY_DIR}/spell.conf.in spell.conf)
install(FILES "${CMAKE_CURRENT_BINARY_DIR}/spell.conf" DESTINATION "${FCITX_INSTALL_PKGDATADIR}/addon")
fcitx5_export_module(Spell TARGET spell BUILD_INCLUDE_DIRECTORIES "${CMAKE_CURRENT_SOURCE_DIR}" HEADERS spell_public.h INSTALL)
