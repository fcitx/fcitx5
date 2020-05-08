configure_file(testdir.h.in ${CMAKE_CURRENT_BINARY_DIR}/testdir.h @ONLY)
include_directories(${CMAKE_CURRENT_BINARY_DIR})

add_subdirectory(addon)

set(FCITX_UTILS_TEST
    testflags
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
    testunixfd
    testmetastring
    testhandlertable
    testsignals
    testinputbuffer
    testlog
    testeventdispatcher)

set(FCITX_UTILS_DBUS_TEST
    testdbusmessage
    testdbus
    testservicewatcher)

set(testdbus_LIBS Pthread::Pthread)
set(testeventdispatcher_LIBS Pthread::Pthread)

add_executable(DBusWrapper IMPORTED)
set_target_properties(DBusWrapper PROPERTIES IMPORTED_LOCATION "${CMAKE_CURRENT_SOURCE_DIR}/dbus_wrapper.sh")

foreach(TESTCASE ${FCITX_UTILS_DBUS_TEST})
    add_executable(${TESTCASE} ${TESTCASE}.cpp)
    target_link_libraries(${TESTCASE} Fcitx5::Utils ${${TESTCASE}_LIBS})
    add_test(NAME ${TESTCASE}
             COMMAND DBusWrapper "${CMAKE_CURRENT_BINARY_DIR}/${TESTCASE}" ${${TESTCASE}_ARGS})
endforeach()

foreach(TESTCASE ${FCITX_UTILS_TEST})
    add_executable(${TESTCASE} ${TESTCASE}.cpp)
    target_link_libraries(${TESTCASE} Fcitx5::Utils ${${TESTCASE}_LIBS})
    add_test(NAME ${TESTCASE}
             COMMAND ${TESTCASE} ${${TESTCASE}_ARGS})
endforeach()
set_target_properties(testlibrary PROPERTIES LINK_FLAGS "-rdynamic")

set(FCITX_CONFIG_TEST
    testconfig
    testconfigdbus)

foreach(TESTCASE ${FCITX_CONFIG_TEST})
    add_executable(${TESTCASE} ${TESTCASE}.cpp)
    target_link_libraries(${TESTCASE} Fcitx5::Config)
    add_test(NAME ${TESTCASE}
             COMMAND ${TESTCASE})
endforeach()

set(FCITX_CORE_TEST
    testinputcontext
    testsurroundingtext
    testaddon
    testmenu
    testuserinterfacemanager
    testelement
    testcandidatelist
    testicontheme)
foreach(TESTCASE ${FCITX_CORE_TEST})
    add_executable(${TESTCASE} ${TESTCASE}.cpp)
    target_link_libraries(${TESTCASE} Fcitx5::Core)
    add_test(NAME ${TESTCASE}
             COMMAND ${TESTCASE} ${${TESTCASE}_ARGS})
endforeach()

add_dependencies(testaddon dummyaddon)

add_executable(testxkbrules testxkbrules.cpp ../src/im/keyboard/xkbrules.cpp ../src/im/keyboard/xmlparser.cpp)
target_compile_definitions(testxkbrules PRIVATE "-D_TEST_XKBRULES")
target_include_directories(testxkbrules PRIVATE ../src)
target_link_libraries(testxkbrules Fcitx5::Core Expat::Expat)
add_test(NAME testxkbrules COMMAND testxkbrules)

add_executable(testisocodes testisocodes.cpp ../src/im/keyboard/isocodes.cpp)
target_compile_definitions(testisocodes PRIVATE "-D_TEST_XKBRULES")
target_include_directories(testisocodes PRIVATE ../src)
target_link_libraries(testisocodes Fcitx5::Core PkgConfig::JsonC)
add_test(NAME testisocodes COMMAND testisocodes)

add_executable(testemoji testemoji.cpp)
target_link_libraries(testemoji Fcitx5::Core Fcitx5::Module::Emoji)
add_dependencies(testemoji emoji emoji.conf.in-fmt)
add_test(NAME testemoji COMMAND testemoji)
