add_library(dummyaddon MODULE dummyaddon.cpp)
target_link_libraries(dummyaddon Fcitx5::Core)

add_custom_target(copy-addon DEPENDS unicode.conf.in-fmt quickphrase.conf.in-fmt xim.conf.in-fmt xcb.conf.in-fmt)
add_custom_command(TARGET copy-addon COMMAND ${CMAKE_COMMAND} -E copy ${PROJECT_BINARY_DIR}/src/modules/unicode/unicode.conf ${CMAKE_CURRENT_BINARY_DIR}/unicode.conf)
add_custom_command(TARGET copy-addon COMMAND ${CMAKE_COMMAND} -E copy ${PROJECT_BINARY_DIR}/src/modules/quickphrase/quickphrase.conf ${CMAKE_CURRENT_BINARY_DIR}/quickphrase.conf)
add_custom_command(TARGET copy-addon COMMAND ${CMAKE_COMMAND} -E copy ${PROJECT_BINARY_DIR}/src/frontend/xim/xim.conf ${CMAKE_CURRENT_BINARY_DIR}/xim.conf)
add_custom_command(TARGET copy-addon COMMAND ${CMAKE_COMMAND} -E copy ${PROJECT_BINARY_DIR}/src/modules/xcb/xcb.conf ${CMAKE_CURRENT_BINARY_DIR}/xcb.conf)
add_custom_command(TARGET copy-addon COMMAND ${CMAKE_COMMAND} -E copy ${PROJECT_SOURCE_DIR}/testing/testfrontend/testfrontend.conf ${CMAKE_CURRENT_BINARY_DIR}/testfrontend.conf)
add_custom_command(TARGET copy-addon COMMAND ${CMAKE_COMMAND} -E copy ${PROJECT_SOURCE_DIR}/testing/testim/testim.conf ${CMAKE_CURRENT_BINARY_DIR}/testim.conf)
add_custom_command(TARGET copy-addon COMMAND ${CMAKE_COMMAND} -E copy ${PROJECT_SOURCE_DIR}/testing/testui/testui.conf ${CMAKE_CURRENT_BINARY_DIR}/testui.conf)

