add_executable(fcitx main.cpp errorhandler.cpp)
target_link_libraries(fcitx Fcitx5::Core keyboard)
install(TARGETS fcitx DESTINATION "${FCITX_INSTALL_BINDIR}")
