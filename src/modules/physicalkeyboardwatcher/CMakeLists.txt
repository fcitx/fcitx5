if (ENABLE_DBUS)
    add_library(physicalkeyboardwatcher MODULE physicalkeyboardwatcher.cpp)
    target_link_libraries(physicalkeyboardwatcher Fcitx5::Core Fcitx5::Module::DBus)
    install(TARGETS physicalkeyboardwatcher DESTINATION "${FCITX_INSTALL_ADDONDIR}")
    install(FILES "${CMAKE_CURRENT_BINARY_DIR}/physicalkeyboardwatcher.conf" DESTINATION "${FCITX_INSTALL_PKGDATADIR}/addon"
            COMPONENT config)
endif()

configure_file(physicalkeyboardwatcher.conf.in.in physicalkeyboardwatcher.conf.in @ONLY)
fcitx5_translate_desktop_file(${CMAKE_CURRENT_BINARY_DIR}/physicalkeyboardwatcher.conf.in physicalkeyboardwatcher.conf)
