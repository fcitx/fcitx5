cmake_minimum_required(VERSION 3.0.0)
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

#######################################################################
# Options
#######################################################################
option(ENABLE_TEST "Build Test" On)
option(ENABLE_COVERAGE "Build the project with gcov support (Need ENABLE_TEST=On)" Off)

#######################################################################
# Find packages
#######################################################################
find_package(Systemd REQUIRED)

# directory needed by bsd
if(NOT CMAKE_INSTALL_LIBDATADIR)
  set(CMAKE_INSTALL_LIBDATADIR "${CMAKE_INSTALL_LIBDIR}" CACHE PATH "libdata root (LIBDIR)")
  set(CMAKE_INSTALL_LIBDATADIR "${CMAKE_INSTALL_LIBDIR}")
  mark_as_advanced(CMAKE_INSTALL_LIBDATADIR)
endif()

include(FcitxCompilerSettings)

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
