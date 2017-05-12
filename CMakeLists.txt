cmake_minimum_required(VERSION 3.6.0)
set(FCITX_VERSION 4.99.0)
project(fcitx VERSION ${FCITX_VERSION})

find_package(ECM REQUIRED 1.0.0)
set(CMAKE_MODULE_PATH ${ECM_MODULE_PATH} "${CMAKE_CURRENT_SOURCE_DIR}/cmake" ${CMAKE_MODULE_PATH})

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

#######################################################################
# Options
#######################################################################
option(ENABLE_TEST "Build Test" On)
option(ENABLE_COVERAGE "Build the project with gcov support (Need ENABLE_TEST=On)" Off)
set(DEFAULT_XKB_RULES "evdev" CACHE STRING "Xkb rules name")

#######################################################################
# Find packages
#######################################################################
find_package(Systemd REQUIRED)
find_package(DL REQUIRED)
find_package(LibIntl REQUIRED)
find_package(LibUUID REQUIRED)
find_package(Pthread REQUIRED)
find_package(Execinfo)
find_package(XCB COMPONENTS XCB AUX XKB XFIXES ICCCM XINERAMA RANDR)
find_package(XKBCommon COMPONENTS XKBCommon X11)
find_package(XCBImdkit)
find_package(IsoCodes)
find_package(Expat)
find_package(XKeyboardConfig)
pkg_check_modules(Cairo IMPORTED_TARGET cairo)
pkg_check_modules(CairoXCB IMPORTED_TARGET cairo-xcb)
pkg_check_modules(CairoEGL IMPORTED_TARGET cairo-egl)
pkg_check_modules(Pango IMPORTED_TARGET pango pangocairo)
find_package(Wayland COMPONENTS Client Egl)
find_package(WaylandScanner)
find_package(WaylandProtocols)
find_package(EGL)

option(ENABLE_ENCHANT "Enable enchant for word predication" On)
option(ENABLE_PRESAGE "Enable presage for word predication" Off)

if(ENABLE_ENCHANT)
  find_package(Enchant REQUIRED)
endif()

if(ENABLE_PRESAGE)
  find_package(Presage REQUIRED)
endif()

set(DEFAULT_XKB_RULES_FILES "${XKEYBOARDCONFIG_XKBBASE}/rules/${DEFAULT_XKB_RULES}.xml")
if (NOT EXISTS "${DEFAULT_XKB_RULES_FILES}")
message(FATAL_ERROR "Could not find default xkb rules file: ${DEFAULT_XKB_RULES_FILES}")
endif ()

# directory needed by bsd
if(NOT CMAKE_INSTALL_LIBDATADIR)
  set(CMAKE_INSTALL_LIBDATADIR "${CMAKE_INSTALL_LIBDIR}" CACHE PATH "libdata root (LIBDIR)")
endif()

get_filename_component(CMAKE_INSTALL_FULL_LIBDATADIR "${CMAKE_INSTALL_LIBDATADIR}" ABSOLUTE)
mark_as_advanced(CMAKE_INSTALL_LIBDATADIR)
mark_as_advanced(CMAKE_INSTALL_FULL_LIBDATADIR)

include(FcitxCompilerSettings)

set(FCITX_INSTALL_PREFIX ${CMAKE_INSTALL_PREFIX})
set(FCITX_INSTALL_INCLUDEDIR ${CMAKE_INSTALL_FULL_INCLUDEDIR})
set(FCITX_INSTALL_LIBDIR ${CMAKE_INSTALL_FULL_LIBDIR})
set(FCITX_INSTALL_LIBDATADIR ${CMAKE_INSTALL_FULL_LIBDATADIR})
set(FCITX_INSTALL_DATADIR ${CMAKE_INSTALL_FULL_DATADIR})
set(FCITX_INSTALL_PKGDATADIR "${CMAKE_INSTALL_FULL_DATADIR}/fcitx5")
set(FCITX_INSTALL_BINDIR ${CMAKE_INSTALL_FULL_BINDIR})
set(FCITX_INSTALL_LOCALEDIR ${CMAKE_INSTALL_FULL_LOCALEDIR})
set(FCITX_INSTALL_ADDONDIR "${CMAKE_INSTALL_FULL_LIBDIR}/fcitx5")
set(FCITX_INSTALL_CMAKECONFIG_DIR "${CMAKE_INSTALL_FULL_LIBDIR}/cmake")
set(FCITX_INSTALL_MODULE_HEADER_DIR "${CMAKE_INSTALL_FULL_INCLUDEDIR}/Fcitx5/Module")
# TODO mac/win
set(FCITX_LIBRARY_SUFFIX ".so")

configure_file(${CMAKE_CURRENT_SOURCE_DIR}/config.h.in ${CMAKE_CURRENT_BINARY_DIR}/config.h)
include_directories(${CMAKE_CURRENT_BINARY_DIR})

add_subdirectory(src)

#######################################################################
# Test features
#######################################################################
if (ENABLE_TEST)
    enable_testing()
    add_subdirectory(test)

    if (ENABLE_COVERAGE)
        add_custom_target(coverage
                        COMMAND "${CMAKE_CTEST_COMMAND}"
                        COMMAND lcov --capture --directory ./ --output-file coverage.info
                        COMMAND genhtml coverage.info --output-directory "coverage_pages"
                        WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR})
    endif()
endif ()

feature_summary(WHAT ALL FATAL_ON_MISSING_REQUIRED_PACKAGES)
