include(CheckCCompilerFlag)
check_c_compiler_flag("-std=c99" C99_SUPPORTED)
if (NOT C99_SUPPORTED)
    message(FATAL_ERROR "C99 is required to compile Fcitx.")
endif()

include(CheckCXXCompilerFlag)

check_cxx_compiler_flag("-std=c++14" CXX14_SUPPORTED)

if(NOT CXX14_SUPPORTED)
    message(FATAL_ERROR "need c++ 14 compatible compiler to compile")
endif()

set(CMAKE_C_FLAGS "-Wall -Wextra -std=c99 -fvisibility=hidden ${CMAKE_C_FLAGS}")
set(CMAKE_CXX_FLAGS "-Wall -Wextra -std=c++14 -fvisibility=hidden ${CMAKE_CXX_FLAGS}")
set(CMAKE_SHARED_LINKER_FLAGS "-Wl,--no-undefined -Wl,--as-needed ${CMAKE_SHARED_LINKER_FLAGS}")
set(CMAKE_MODULE_LINKER_FLAGS "-Wl,--as-needed ${CMAKE_MODULE_LINKER_FLAGS}")


if(ENABLE_COVERAGE)
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -fprofile-arcs -ftest-coverage")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fprofile-arcs -ftest-coverage")
    set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -lgcov")
endif()
