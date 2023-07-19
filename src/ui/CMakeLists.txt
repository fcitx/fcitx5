add_subdirectory(classic)

if (ENABLE_DBUS)
    add_subdirectory(kimpanel)
endif()

add_subdirectory(virtualkeyboard)
