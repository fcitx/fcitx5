add_library(dbus MODULE dbusmodule.cpp)
target_link_libraries(dbus Fcitx5::Core)

