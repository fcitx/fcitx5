file(GLOB PO_FILES RELATIVE ${CMAKE_CURRENT_SOURCE_DIR} *.po)
gettext_create_translations(fcitx.pot ALL ${PO_FILES})
