add_fcitx5_addon(bangla bangla.cpp avro_parser.cpp)

target_link_libraries(bangla Fcitx5::Core)

configure_file(bangla-addon.conf.in.in bangla.conf.in @ONLY)
fcitx5_translate_desktop_file(${CMAKE_CURRENT_BINARY_DIR}/bangla.conf.in bangla-addon.conf)
fcitx5_translate_desktop_file(avro.conf.in avro-im.conf)
fcitx5_translate_desktop_file(probhat.conf.in probhat.conf)

install(TARGETS bangla DESTINATION "${FCITX_INSTALL_ADDONDIR}")
install(FILES "${CMAKE_CURRENT_BINARY_DIR}/bangla-addon.conf" RENAME bangla.conf
        DESTINATION "${FCITX_INSTALL_PKGDATADIR}/addon" COMPONENT config)
install(FILES "${CMAKE_CURRENT_BINARY_DIR}/avro-im.conf" RENAME avro.conf
        DESTINATION "${FCITX_INSTALL_PKGDATADIR}/inputmethod" COMPONENT config)
install(FILES "${CMAKE_CURRENT_BINARY_DIR}/probhat.conf"
        DESTINATION "${FCITX_INSTALL_PKGDATADIR}/inputmethod" COMPONENT config)
install(FILES "${CMAKE_SOURCE_DIR}/data/bangla/user.dict.txt"
        DESTINATION "${FCITX_INSTALL_PKGDATADIR}/bangla" COMPONENT config)
