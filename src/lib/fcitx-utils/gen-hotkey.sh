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

XKEYSYM=https://raw.githubusercontent.com/xkbcommon/libxkbcommon/refs/heads/master/include/xkbcommon/xkbcommon-keysyms.h
KEYSYMGEN_HEADER=keysymgen.h

download_file xkbcommon-keysyms.h $XKEYSYM

grep '^#define.*XKB_KEY_' xkbcommon-keysyms.h | sed 's|^#define XKB_KEY_\([a-zA-Z_0-9]\+\) \+0x\([0-9A-Fa-f]\+\)\(.*\)$|\1\t0x\2\t\3|g' | grep -v '^NoSymbol' > keylist
sed -i 's|^XF86Q\t|Compaq_Q\t|g' keylist
sed -i 's|^XF86Break\t|MediaBreak\t|g' keylist
sed -i 's|^XF86Clear\t|WindowClear\t|g' keylist
sed -i 's|^XF86Select\t|SelectButton\t|g' keylist
sed -i 's|^XF86||g' keylist

python3 update-keydata.py
