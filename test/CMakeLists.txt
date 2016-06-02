add_subdirectory(addon)

set(FCITX_UTILS_TEST
    teststringutils
    testkey
    testutf8
    testcolor
    testi18nstring
    testevent
    testdbusunixfd
    testdbusmessage
    testdbus
    testlist
    testfs
    testlibrary
    teststandardpath)

set(teststandardpath_ARGS "${CMAKE_CURRENT_SOURCE_DIR}/addon")
set(testdbus_LIBS Pthread::Pthread)

foreach(TESTCASE ${FCITX_UTILS_TEST})
    add_executable(${TESTCASE} ${TESTCASE}.cpp)
    target_link_libraries(${TESTCASE} Fcitx5::Utils Systemd::Systemd ${${TESTCASE}_LIBS})
    add_test(NAME ${TESTCASE}
             COMMAND ${TESTCASE} ${${TESTCASE}_ARGS})
endforeach()

set(FCITX_CONFIG_TEST
    testconfig)

foreach(TESTCASE ${FCITX_CONFIG_TEST})
    add_executable(${TESTCASE} ${TESTCASE}.cpp)
    target_link_libraries(${TESTCASE} Fcitx5::Config)
    add_test(NAME ${TESTCASE}
             COMMAND ${TESTCASE})
endforeach()

set(testaddon_ARGS "${CMAKE_CURRENT_SOURCE_DIR}/addon2" "${CMAKE_CURRENT_BINARY_DIR}/addon")

set(FCITX_CORE_TEST
    testinputcontext
    testsurroundingtext
    testaddon)
foreach(TESTCASE ${FCITX_CORE_TEST})
    add_executable(${TESTCASE} ${TESTCASE}.cpp)
    target_link_libraries(${TESTCASE} Fcitx5::Core)
    add_test(NAME ${TESTCASE}
             COMMAND ${TESTCASE} ${${TESTCASE}_ARGS})
endforeach()

add_dependencies(testaddon dummyaddon)
