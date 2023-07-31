add_subdirectory(classic)

if (ENABLE_DBUS)
    add_subdirectory(kimpanel)
    add_subdirectory(virtualkeyboard)
endif()
