#!/bin/sh

function download_file()
{
    if [ "x$3" != "xf" ]; then
        if [ -f $1 ]; then
            return
        fi
    fi
    rm -f $1
    wget $2
}

XKEYSYM=http://cgit.freedesktop.org/xorg/proto/xproto/plain/keysymdef.h
XF86KEYSYM=http://cgit.freedesktop.org/xorg/proto/x11proto/plain/XF86keysym.h
KEYSYMGEN_HEADER=keysymgen.h
KEYNAMETABLE_HEADER=keynametable.h

download_file keysymdef.h $XKEYSYM
download_file XF86keysym.h $XF86KEYSYM

grep '^#define' keysymdef.h | sed 's|^#define XK_\([a-zA-Z_0-9]\+\) \+0x\([0-9A-Fa-f]\+\)\(.*\)$|\1\t0x\2\t\3|g' > keylist
grep '^#define' XF86keysym.h | grep -v 'XF86XK_Calculater\|XF86XK_Q' | sed 's|XF86XK_Clear|XF86XK_WindowClear|g' | sed 's|XF86XK_Select|XF86XK_SelectButton|g' | sed 's|^#define XF86XK_\([a-zA-Z_0-9]\+\)[ \t]\+0x\([0-9A-Fa-f]\+\)\(.*\)$|\1\t0x\2\t\3|g' >> keylist

python3 update-keydata.py
