cmake_minimum_required(VERSION 3.6.0)
project(fcitx VERSION 5.0.19)
set(FCITX_VERSION ${PROJECT_VERSION})

find_package(ECM REQUIRED 1.0.0)
set(CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake" ${ECM_MODULE_PATH} ${CMAKE_MODULE_PATH})

#######################################################################
# CMake macros
#######################################################################
include(FeatureSummary)
include(GNUInstallDirs)
include(ECMSetupVersion)
include(GenerateExportHeader)
include(WriteBasicConfigVersionFile)
include(ECMUninstallTarget)
include(src/lib/fcitx-utils/Fcitx5Macros.cmake)
include(CheckSymbolExists)

#######################################################################
# Options
#######################################################################
option(ENABLE_TEST "Build Test" On)
option(ENABLE_COVERAGE "Build the project with gcov support (Need ENABLE_TEST=On)" Off)
set(GCOV_TOOL "gcov" CACHE STRING "Path to gcov tool used by coverage.")
set(DEFAULT_XKB_RULES "evdev" CACHE STRING "Xkb rules name")
option(ENABLE_ENCHANT "Enable enchant for word predication" On)
option(ENABLE_X11 "Enable X11 support" On)
option(ENABLE_WAYLAND "Enable wayland support" On)
option(ENABLE_DBUS "Enable DBus" On)
option(ENABLE_DOC "Build doxygen" Off)
option(ENABLE_SERVER "Build a fcitx as server, disable this option if you want to use fcitx as an embedded library." On)
option(ENABLE_KEYBOARD "Enable key event translation with XKB and build keyboard engine" On)
option(USE_SYSTEMD "Use systemd for event loop and dbus, will fallback to libevent/libdbus if not found." On)
option(ENABLE_XDGAUTOSTART "Enable xdg autostart desktop file installation" On)
option(USE_FLATPAK_ICON "Use flatpak icon name for desktop files" Off)
set(CLDR_DIR "" CACHE STRING "Unicode CLDR (Common Locale Data Repository) directory")
option(ENABLE_EMOJI "Enable emoji loading for CLDR" On)
option(ENABLE_LIBUUID "Use libuuid for uuid generation" On)
set(NO_PREEDIT_APPS "gvim.*,wps.*,wpp.*,et.*" CACHE STRING "Disable preedit for follwing app by default.")

if (CLDR_DIR STREQUAL "")
    if (IS_DIRECTORY "${CMAKE_INSTALL_FULL_DATADIR}/unicode/cldr")
        set(CLDR_DIR "${CMAKE_INSTALL_FULL_DATADIR}/unicode/cldr")
    else(IS_DIRECTORY "/usr/share/unicode/cldr")
        set(CLDR_DIR "/usr/share/unicode/cldr")
    endif()
endif()

set(EMOJI_FOUND FALSE)
if (ENABLE_EMOJI)
    if (CLDR_DIR STREQUAL "" OR NOT IS_DIRECTORY "${CLDR_DIR}/common/annotations/")
        message(WARNING "Could not find Unicode CLDR Annotation directory")
    else()
        set(EMOJI_FOUND TRUE)
    endif()
endif()

if ((ENABLE_WAYLAND OR ENABLE_X11) AND NOT ENABLE_KEYBOARD)
    message(FATAL_ERROR "X11 and Wayland require ENABLE_KEYBOARD to be set to ON.")
endif ()


#######################################################################
# Find packages
#######################################################################

find_package(PkgConfig REQUIRED)

if (USE_SYSTEMD)
find_package(Systemd)
endif ()

if (USE_FLATPAK_ICON)
    set(FCITX_ICON_NAME "org.fcitx.Fcitx5")
else()
    set(FCITX_ICON_NAME "fcitx")
endif()

if (NOT TARGET Systemd::Systemd)
    if (ENABLE_DBUS)
        pkg_check_modules(DBus REQUIRED IMPORTED_TARGET "dbus-1")
        pkg_get_variable(DBUS_SYSTEM_BUS_DEFAULT_ADDRESS "dbus-1" "system_bus_default_address")
    endif()

    if (NOT (TARGET libevent::core OR TARGET PkgConfig::LibEvent))
        find_package(Libevent)
    endif()
    if (TARGET libevent::core)
        set(LIBEVENT_TARGET libevent::core)
    else()
        pkg_check_modules(LibEvent REQUIRED IMPORTED_TARGET "libevent_core")
        set(LIBEVENT_TARGET PkgConfig::LibEvent)
    endif()
endif()

if(${CMAKE_SYSTEM_NAME} MATCHES "BSD|DragonFly")
    find_package(LibKVM REQUIRED)
endif()
find_package(DL REQUIRED)

if (NOT TARGET LibIntl::LibIntl)
    find_package(LibIntl REQUIRED)
endif()

if (ENABLE_LIBUUID)
    find_package(LibUUID REQUIRED)
endif()

find_package(Pthread REQUIRED)
find_package(Execinfo)
find_package(Gettext REQUIRED)

if (NOT (TARGET fmt::fmt OR TARGET fmt::fmt-header-only))
    find_package(fmt REQUIRED)
endif()

if (TARGET fmt::fmt-header-only)
  set(FMT_TARGET fmt::fmt-header-only)
else()
  set(FMT_TARGET fmt::fmt)
endif ()

set(REQUIRED_XKBCOMMON_COMPONENTS XKBCommon)
if (ENABLE_X11)
    set(REQUIRED_XKBCOMMON_COMPONENTS ${REQUIRED_XKBCOMMON_COMPONENTS} X11)
    find_package(XCB REQUIRED COMPONENTS XCB AUX XKB XFIXES ICCCM XINERAMA RANDR EWMH KEYSYMS)
    find_package(XCBImdkit 1.0.3 REQUIRED)
    pkg_check_modules(CairoXCB REQUIRED IMPORTED_TARGET cairo-xcb)
    pkg_check_modules(XkbFile REQUIRED IMPORTED_TARGET "xkbfile")
endif()

if (EMOJI_FOUND OR ENABLE_KEYBOARD)
    find_package(Expat REQUIRED)
endif()

if (ENABLE_KEYBOARD)
    find_package(XKBCommon REQUIRED COMPONENTS ${REQUIRED_XKBCOMMON_COMPONENTS})
    find_package(IsoCodes REQUIRED)
    find_package(XKeyboardConfig REQUIRED)
    pkg_check_modules(JsonC REQUIRED IMPORTED_TARGET "json-c")

    set(DEFAULT_XKB_RULES_FILES "${XKEYBOARDCONFIG_XKBBASE}/rules/${DEFAULT_XKB_RULES}.xml")
    if (NOT EXISTS "${DEFAULT_XKB_RULES_FILES}")
    message(FATAL_ERROR "Could not find default xkb rules file: ${DEFAULT_XKB_RULES_FILES}")
    endif()
endif()

# Library for classicui painting
if (ENABLE_X11 OR ENABLE_WAYLAND)
    pkg_check_modules(Cairo IMPORTED_TARGET cairo)
    pkg_check_modules(Pango IMPORTED_TARGET pango pangocairo)
    pkg_check_modules(GdkPixbuf IMPORTED_TARGET gdk-pixbuf-2.0)
    pkg_check_modules(GioUnix IMPORTED_TARGET gio-unix-2.0)
endif()

if (ENABLE_WAYLAND)
    set(REQUIRED_WAYLAND_COMPONENTS Client)
    find_package(Wayland REQUIRED COMPONENTS ${REQUIRED_WAYLAND_COMPONENTS})
    find_package(WaylandScanner REQUIRED)
    find_package(WaylandProtocols REQUIRED)
    pkg_check_modules(Gio IMPORTED_TARGET gio-2.0)
endif()

if(ENABLE_ENCHANT)
  pkg_check_modules(Enchant IMPORTED_TARGET "enchant-2")
  if(NOT TARGET PkgConfig::Enchant)
    pkg_check_modules(Enchant IMPORTED_TARGET "enchant" REQUIRED)
  endif()
endif()

if (TARGET Wayland::Client AND ENABLE_WAYLAND)
    set(WAYLAND_FOUND TRUE)
else()
    set(WAYLAND_FOUND FALSE)
endif()

# directory needed by bsd
if(NOT CMAKE_INSTALL_LIBDATADIR)
  set(CMAKE_INSTALL_LIBDATADIR "${CMAKE_INSTALL_LIBDIR}" CACHE PATH "libdata root (LIBDIR)")
endif()

get_filename_component(CMAKE_INSTALL_FULL_LIBDATADIR "${CMAKE_INSTALL_LIBDATADIR}" ABSOLUTE)
mark_as_advanced(CMAKE_INSTALL_LIBDATADIR)
mark_as_advanced(CMAKE_INSTALL_FULL_LIBDATADIR)

include(Fcitx5CompilerSettings)

set(FCITX_INSTALL_PREFIX ${CMAKE_INSTALL_PREFIX})
set(FCITX_INSTALL_INCLUDEDIR ${CMAKE_INSTALL_FULL_INCLUDEDIR})
set(FCITX_INSTALL_LIBDIR ${CMAKE_INSTALL_FULL_LIBDIR})
set(FCITX_INSTALL_LIBDATADIR ${CMAKE_INSTALL_FULL_LIBDATADIR})
set(FCITX_INSTALL_LIBEXECDIR ${CMAKE_INSTALL_FULL_LIBEXECDIR})
set(FCITX_INSTALL_DATADIR ${CMAKE_INSTALL_FULL_DATADIR})
set(FCITX_INSTALL_PKGDATADIR "${CMAKE_INSTALL_FULL_DATADIR}/fcitx5")
set(FCITX_INSTALL_BINDIR ${CMAKE_INSTALL_FULL_BINDIR})
set(FCITX_INSTALL_LOCALEDIR ${CMAKE_INSTALL_FULL_LOCALEDIR})
set(FCITX_INSTALL_ADDONDIR "${CMAKE_INSTALL_FULL_LIBDIR}/fcitx5")
set(FCITX_INSTALL_CMAKECONFIG_DIR "${CMAKE_INSTALL_FULL_LIBDIR}/cmake")
set(FCITX_INSTALL_MODULE_HEADER_DIR "${CMAKE_INSTALL_FULL_INCLUDEDIR}/Fcitx5/Module/fcitx-module")
# TODO mac/win
set(FCITX_LIBRARY_SUFFIX ".so")

check_function_exists(pipe2 HAVE_PIPE2)

configure_file(${CMAKE_CURRENT_SOURCE_DIR}/config.h.in ${CMAKE_CURRENT_BINARY_DIR}/config.h)
include_directories(${CMAKE_CURRENT_BINARY_DIR})

add_subdirectory(src)
add_subdirectory(data)
add_subdirectory(po)
add_subdirectory(testing)

#######################################################################
# Test features
#######################################################################
if (ENABLE_TEST)
    enable_testing()
    add_subdirectory(test)

    if (ENABLE_COVERAGE)
        add_custom_target(coverage
                        COMMAND "${CMAKE_CTEST_COMMAND}"
                        COMMAND lcov --gcov-tool "${GCOV_TOOL}" --no-external --capture --directory ./ -b "${CMAKE_CURRENT_SOURCE_DIR}" --output-file coverage.info
                        COMMAND genhtml coverage.info --output-directory "coverage_pages"
                        WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR})
    endif()
endif ()

if (ENABLE_DOC)
  find_package(Doxygen REQUIRED)
  file(READ "${CMAKE_CURRENT_SOURCE_DIR}/.codedocs" FCITX_DOXYGEN_CONFIGURATION)
  configure_file(
    ${CMAKE_CURRENT_SOURCE_DIR}/Doxyfile.in
    ${CMAKE_CURRENT_BINARY_DIR}/Doxyfile
    @ONLY)
  add_custom_target(doc
      COMMAND ${DOXYGEN_EXECUTABLE} ${CMAKE_CURRENT_BINARY_DIR}/Doxyfile
      WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
      COMMENT "Generating API documentation with Doxygen"
      VERBATIM)
endif()

feature_summary(WHAT ALL FATAL_ON_MISSING_REQUIRED_PACKAGES)
