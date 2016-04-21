set(FCITX_UTILS_TEST
    testkey
    testutf8
    testcolor
    testi18nstring
    testevent
    testdbusunixfd
    testdbusmessage
    testdbus)

foreach(TESTCASE ${FCITX_UTILS_TEST})
    add_executable(${TESTCASE} ${TESTCASE}.cpp)
    target_link_libraries(${TESTCASE} Fcitx5::Utils Systemd::Systemd)
    add_test(NAME ${TESTCASE}
             COMMAND ${TESTCASE})
endforeach()

set(FCITX_CONFIG_TEST
    testconfig)

foreach(TESTCASE ${FCITX_CONFIG_TEST})
    add_executable(${TESTCASE} ${TESTCASE}.cpp)
    target_link_libraries(${TESTCASE} Fcitx5::Config)
    add_test(NAME ${TESTCASE}
             COMMAND ${TESTCASE})
endforeach()
