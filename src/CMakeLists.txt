add_definitions("-DFCITX_GETTEXT_DOMAIN=\"fcitx5\"")

add_subdirectory(lib)
add_subdirectory(modules)
add_subdirectory(frontend)

if (ENABLE_SERVER)
add_subdirectory(server)
endif()

add_subdirectory(im)
add_subdirectory(ui)
add_subdirectory(tools)
