#!/bin/sh

# sleep for a little while to avoid duplicate startup
sleep 2

# Test whether fcitx5 is running correctly with dbus...
fcitx5-remote > /dev/null 2>&1

if [ $? = "1" ]; then
    echo "Fcitx5 seems is not running"
    fcitx5
else
    echo "Fcitx5 is running correctly."
fi
