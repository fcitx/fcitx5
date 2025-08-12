#!/bin/sh

export XDG_DATA_DIRS=/usr/share:/usr/local/share:${XDG_DATA_DIRS}
export FCITX_ADDON_DIRS=/usr/lib/fcitx5:/usr/local/lib/fcitx5:@FCITX_INSTALL_ADDONDIR@:${FCITX_ADDON_DIRS}
export PATH=/usr/bin:/usr/local/bin:${PATH}

exec @FCITX_INSTALL_BINDIR@/fcitx5 "$@"