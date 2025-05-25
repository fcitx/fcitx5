add_executable(fcitx5 main.cpp errorhandler.cpp)
target_link_libraries(fcitx5 Fcitx5::Core LibIntl::LibIntl)
if (ENABLE_KEYBOARD)
    target_link_libraries(fcitx5 keyboard)
endif()
if (EXECINFO_FOUND)
target_link_libraries(fcitx5 Execinfo::Execinfo)
endif()

if(HAVE_NO_DEPRECATED_DECLARATIONS)
    set_source_files_properties(main.cpp PROPERTIES COMPILE_FLAGS "-Wno-deprecated-declarations")
endif()

install(TARGETS fcitx5 DESTINATION "${FCITX_INSTALL_BINDIR}")
