add_library(dummyaddon MODULE dummyaddon.cpp)
target_link_libraries(dummyaddon Fcitx5::Core)

set(COPY_ADDON_DEPENDS unicode.conf.in-fmt quickphrase.conf.in-fmt xcb.conf.in-fmt spell.conf.in-fmt)

if (ENABLE_KEYBOARD)
    list(APPEND COPY_ADDON_DEPENDS keyboard.conf.in-fmt)
endif()

if (ENABLE_X11)
    list(APPEND COPY_ADDON_DEPENDS xim.conf.in-fmt)
endif()

add_custom_target(copy-addon DEPENDS ${COPY_ADDON_DEPENDS})
add_custom_command(TARGET copy-addon COMMAND ${CMAKE_COMMAND} -E copy ${PROJECT_BINARY_DIR}/src/modules/unicode/unicode.conf ${CMAKE_CURRENT_BINARY_DIR}/unicode.conf)
add_custom_command(TARGET copy-addon COMMAND ${CMAKE_COMMAND} -E copy ${PROJECT_BINARY_DIR}/src/modules/quickphrase/quickphrase.conf ${CMAKE_CURRENT_BINARY_DIR}/quickphrase.conf)
add_custom_command(TARGET copy-addon COMMAND ${CMAKE_COMMAND} -E copy ${PROJECT_BINARY_DIR}/src/modules/xcb/xcb.conf ${CMAKE_CURRENT_BINARY_DIR}/xcb.conf)
add_custom_command(TARGET copy-addon COMMAND ${CMAKE_COMMAND} -E copy ${PROJECT_BINARY_DIR}/src/modules/spell/spell.conf ${CMAKE_CURRENT_BINARY_DIR}/spell.conf)
add_custom_command(TARGET copy-addon COMMAND ${CMAKE_COMMAND} -E copy ${PROJECT_SOURCE_DIR}/testing/testfrontend/testfrontend.conf ${CMAKE_CURRENT_BINARY_DIR}/testfrontend.conf)
add_custom_command(TARGET copy-addon COMMAND ${CMAKE_COMMAND} -E copy ${PROJECT_SOURCE_DIR}/testing/testim/testim.conf ${CMAKE_CURRENT_BINARY_DIR}/testim.conf)
add_custom_command(TARGET copy-addon COMMAND ${CMAKE_COMMAND} -E copy ${PROJECT_SOURCE_DIR}/testing/testui/testui.conf ${CMAKE_CURRENT_BINARY_DIR}/testui.conf)

if (ENABLE_KEYBOARD)
    add_custom_command(TARGET copy-addon COMMAND ${CMAKE_COMMAND} -E copy ${PROJECT_BINARY_DIR}/src/im/keyboard/keyboard.conf ${CMAKE_CURRENT_BINARY_DIR}/keyboard.conf)
endif()

if (ENABLE_X11)
    add_custom_command(TARGET copy-addon COMMAND ${CMAKE_COMMAND} -E copy ${PROJECT_BINARY_DIR}/src/frontend/xim/xim.conf ${CMAKE_CURRENT_BINARY_DIR}/xim.conf)
endif()
