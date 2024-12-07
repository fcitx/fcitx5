add_definitions("-DFCITX_GETTEXT_DOMAIN=\"fcitx5\"")

add_subdirectory(lib)
add_subdirectory(modules)
add_subdirectory(frontend)
add_subdirectory(im)
add_subdirectory(ui)


if (ENABLE_SERVER)
    add_subdirectory(server)

    if (NOT BUILD_SHARED_FCITX_ADDON)
        fcitx5_get_addon_targets(ADDON_TARGETS modules frontend im ui)

        fcitx5_import_addons(fcitx5
            REGISTRY_VARNAME getStaticAddon
            ADDONS ${ADDON_TARGETS}
        )
    endif()
endif()

add_subdirectory(tools)
