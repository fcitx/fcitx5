add_subdirectory(addon)

set(FCITX_UTILS_TEST
    teststringutils
    testkey
    testutf8
    testcolor
    testi18nstring
    testevent
    testlist
    testfs
    testlibrary
    teststandardpath
    testmetastring)

set(FCITX_UTILS_DBUS_TEST
    testdbusunixfd
    testdbusmessage
    testdbus)

set(teststandardpath_ARGS "${CMAKE_CURRENT_SOURCE_DIR}/addon")
set(testdbus_LIBS Pthread::Pthread)

add_executable(DBusWrapper IMPORTED)
set_target_properties(DBusWrapper PROPERTIES IMPORTED_LOCATION "${CMAKE_CURRENT_SOURCE_DIR}/dbus_wrapper.sh")

foreach(TESTCASE ${FCITX_UTILS_DBUS_TEST})
    add_executable(${TESTCASE} ${TESTCASE}.cpp)
    target_link_libraries(${TESTCASE} Fcitx5::Utils Systemd::Systemd ${${TESTCASE}_LIBS})
    add_test(NAME ${TESTCASE}
             COMMAND DBusWrapper "${CMAKE_CURRENT_BINARY_DIR}/${TESTCASE}" ${${TESTCASE}_ARGS})
endforeach()

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
