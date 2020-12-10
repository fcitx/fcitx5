# Try to find LibKVM functionality
# Once done this will define
#
#  LIBKVM_FOUND - system has LibKVM
#  LIBKVM_INCLUDE_DIR - LibKVM include directory
#  LIBKVM_LIBRARIES - Libraries needed to use LibKVM
#

find_path(LIBKVM_INCLUDE_DIR kvm.h)
find_library(LIBKVM_LIBRARIES NAMES kvm)

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(LibKVM
    REQUIRED_VARS LIBKVM_INCLUDE_DIR  LIBKVM_LIBRARIES
)

mark_as_advanced(LIBKVM_INCLUDE_DIR  LIBKVM_LIBRARIES)

if(LIBKVM_FOUND AND NOT TARGET LibKVM::LibKVM)
    add_library(LibKVM::LibKVM UNKNOWN IMPORTED)
    set_target_properties(LibKVM::LibKVM PROPERTIES
        IMPORTED_LOCATION "${LIBKVM_LIBRARIES}"
        INTERFACE_INCLUDE_DIRECTORIES "${LIBKVM_INCLUDE_DIR}"
    )
endif()
