# Try to find Libkvm functionality
# Once done this will define
#
#  LIBKVM_FOUND - system has Libkvm
#  LIBKVM_INCLUDE_DIR - Libkvm include directory
#  LIBKVM_LIBRARIES - Libraries needed to use Libkvm
#

find_path(LIBKVM_INCLUDE_DIR kvm.h)
find_library(LIBKVM_LIBRARIES NAMES kvm)

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(Libkvm  DEFAULT_MSG  LIBKVM_INCLUDE_DIR  LIBKVM_LIBRARIES)

mark_as_advanced(LIBKVM_INCLUDE_DIR  LIBKVM_LIBRARIES)
