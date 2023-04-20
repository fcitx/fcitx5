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
    testeventdispatcher
    testrect
    testfallbackuuid
    testsemver)

set(FCITX_UTILS_DBUS_TEST
    testdbusmessage
    testdbus
    testservicewatcher)

set(testdbus_LIBS Pthread::Pthread)
set(testeventdispatcher_LIBS Pthread::Pthread)

find_program(XVFB_BIN Xvfb)

add_executable(XvfbWrapper IMPORTED)
set_target_properties(XvfbWrapper PROPERTIES IMPORTED_LOCATION "${CMAKE_CURRENT_SOURCE_DIR}/xvfb_wrapper.sh")

if (ENABLE_DBUS)
    find_program(DBUS_DAEMON_BIN dbus-daemon)
    add_executable(DBusWrapper IMPORTED)
    set_target_properties(DBusWrapper PROPERTIES IMPORTED_LOCATION "${CMAKE_CURRENT_SOURCE_DIR}/dbus_wrapper.sh")

    foreach(TESTCASE ${FCITX_UTILS_DBUS_TEST})
        add_executable(${TESTCASE} ${TESTCASE}.cpp)
        target_link_libraries(${TESTCASE} Fcitx5::Utils ${${TESTCASE}_LIBS})

        if (DBUS_DAEMON_BIN)
            add_test(NAME ${TESTCASE}
                    COMMAND DBusWrapper "${DBUS_DAEMON_BIN}" "${CMAKE_CURRENT_BINARY_DIR}/${TESTCASE}" ${${TESTCASE}_ARGS})
        endif()
    endforeach()
endif()

foreach(TESTCASE ${FCITX_UTILS_TEST})
    add_executable(${TESTCASE} ${TESTCASE}.cpp)
    target_link_libraries(${TESTCASE} Fcitx5::Utils ${${TESTCASE}_LIBS})
    add_test(NAME ${TESTCASE}
             COMMAND ${TESTCASE} ${${TESTCASE}_ARGS})
endforeach()
set_target_properties(testlibrary PROPERTIES LINK_FLAGS "-rdynamic")

set(FCITX_CONFIG_TEST
    testconfig)

if (ENABLE_DBUS)
    list(APPEND FCITX_CONFIG_TEST testconfigdbus)
endif()

foreach(TESTCASE ${FCITX_CONFIG_TEST})
    add_executable(${TESTCASE} ${TESTCASE}.cpp)
    target_link_libraries(${TESTCASE} Fcitx5::Config)
    add_test(NAME ${TESTCASE}
             COMMAND ${TESTCASE})
endforeach()

set(testinputcontext_LIBS Fcitx5::Module::TestFrontend Fcitx5::Module::TestIM)

set(FCITX_CORE_TEST
    testinputcontext
    testsurroundingtext
    testaddon
    testmenu
    testuserinterfacemanager
    testelement
    testcandidatelist
    testicontheme
    testtext
    testinstance)

foreach(TESTCASE ${FCITX_CORE_TEST})
    add_executable(${TESTCASE} ${TESTCASE}.cpp)
    target_link_libraries(${TESTCASE} Fcitx5::Core ${${TESTCASE}_LIBS})
    add_test(NAME ${TESTCASE}
             COMMAND ${TESTCASE} ${${TESTCASE}_ARGS})
endforeach()

add_dependencies(testaddon dummyaddon)

if (ENABLE_KEYBOARD)
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

    add_executable(testcompose testcompose.cpp)
    target_link_libraries(testcompose Fcitx5::Core Fcitx5::Module::TestFrontend XKBCommon::XKBCommon keyboard)
    add_test(NAME testcompose COMMAND testcompose)
endif()

if (TARGET emoji)
add_executable(testemoji testemoji.cpp)
target_link_libraries(testemoji Fcitx5::Core Fcitx5::Module::Emoji)
add_dependencies(testemoji emoji emoji.conf.in-fmt)
add_test(NAME testemoji COMMAND testemoji)
endif()

add_executable(testunicode testunicode.cpp)
target_link_libraries(testunicode Fcitx5::Core Fcitx5::Module::TestFrontend Fcitx5::Module::TestIM)
add_dependencies(testunicode copy-addon unicode testui testfrontend testim)
add_test(NAME testunicode COMMAND testunicode)


if (ENABLE_KEYBOARD)
    add_executable(testspell testspell.cpp)
    target_link_libraries(testspell Fcitx5::Core Fcitx5::Module::TestFrontend keyboard)
    add_dependencies(testspell copy-addon spell testui testfrontend spell_en_dict)
    add_test(NAME testspell COMMAND testspell)
endif()

add_executable(testquickphrase testquickphrase.cpp)
target_link_libraries(testquickphrase Fcitx5::Core Fcitx5::Module::QuickPhrase Fcitx5::Module::TestFrontend Fcitx5::Module::TestIM)
add_dependencies(testquickphrase copy-addon quickphrase testui testfrontend testim)
add_test(NAME testquickphrase COMMAND testquickphrase)

if (ENABLE_X11)
add_executable(testxim testxim.cpp)
target_link_libraries(testxim Fcitx5::Core Fcitx5::Module::TestIM Pthread::Pthread XCB::XCB XCB::AUX XCBImdkit::XCBImdkit)
add_dependencies(testxim copy-addon xim testui testfrontend testim)

# FIXME: Test may fail on some system
#if (XVFB_BIN)
#    add_test(NAME testxim COMMAND XvfbWrapper "${XVFB_BIN}" "${CMAKE_CURRENT_BINARY_DIR}/testxim")
#endif()
endif()
