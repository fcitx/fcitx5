ecm_add_wayland_client_protocol(EXT_FOREIGN_TOPLEVEL_LIST_PROTOCOL_SRCS
    PROTOCOL ${WAYLAND_PROTOCOLS_PKGDATADIR}/staging/ext-foreign-toplevel-list/ext-foreign-toplevel-list-v1.xml
    BASENAME ext-foreign-toplevel-list-v1)

set(FCITX_WAYLAND_EXT_FOREIGN_TOPLEVEL_LIST_PROTOCOL_SOURCES
    ext_foreign_toplevel_list_v1.cpp
    ext_foreign_toplevel_handle_v1.cpp
)

add_library(Fcitx5WaylandExtForeignToplevelList STATIC ${FCITX_WAYLAND_EXT_FOREIGN_TOPLEVEL_LIST_PROTOCOL_SOURCES} ${EXT_FOREIGN_TOPLEVEL_LIST_PROTOCOL_SRCS})
set_target_properties(Fcitx5WaylandExtForeignToplevelList PROPERTIES
  POSITION_INDEPENDENT_CODE ON
  )
target_include_directories(Fcitx5WaylandExtForeignToplevelList PUBLIC
    "$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR};${CMAKE_CURRENT_SOURCE_DIR}/..;${CMAKE_CURRENT_BINARY_DIR}>")
target_link_libraries(Fcitx5WaylandExtForeignToplevelList Wayland::Client Fcitx5::Utils Fcitx5::Wayland::Core)

add_library(Fcitx5::Wayland::ExtForeignToplevelList ALIAS Fcitx5WaylandExtForeignToplevelList)

