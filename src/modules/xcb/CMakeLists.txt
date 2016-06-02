add_library(xcb MODULE xcbmodule.cpp)
target_link_libraries(xcb Fcitx5::Core XCB::XCB XCB::AUX XCB::XKB XKBCommon::XKBCommon XKBCommon::X11)
