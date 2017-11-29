add_executable(fcitx5-remote remote.cpp)
target_link_libraries(fcitx5-remote Fcitx5::Utils)

install(TARGETS fcitx5-remote DESTINATION "${FCITX_INSTALL_BINDIR}")
