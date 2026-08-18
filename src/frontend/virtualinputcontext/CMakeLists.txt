set(VIRTUAL_INPUT_CONTEXT_SOURCES
    virtualinputcontext.cpp
    appmonitor.cpp
)

add_library(virtualinputcontext STATIC ${VIRTUAL_INPUT_CONTEXT_SOURCES})
set_target_properties(virtualinputcontext PROPERTIES
  POSITION_INDEPENDENT_CODE ON
  )
target_include_directories(virtualinputcontext PUBLIC
    "$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>")
target_link_libraries(virtualinputcontext Fcitx5::Core)
