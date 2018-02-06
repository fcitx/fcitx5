//
// Copyright (C) 2017~2017 by CSSlayer
// wengxt@gmail.com
//
// This library is free software; you can redistribute it and/or modify
// it under the terms of the GNU Lesser General Public License as
// published by the Free Software Foundation; either version 2.1 of the
// License, or (at your option) any later version.
//
// This library is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
// Lesser General Public License for more details.
//
// You should have received a copy of the GNU Lesser General Public
// License along with this library; see the file COPYING. If not,
// see <http://www.gnu.org/licenses/>.
//
#include "fcitx-utils/log.h"
#include "fcitx/icontheme.h"

using namespace fcitx;

int main() {
    IconTheme theme("breeze");
    FCITX_INFO() << theme.name().match();
    FCITX_INFO() << theme.comment().match();

    for (auto &inheritTheme : theme.inherits()) {
        FCITX_INFO() << inheritTheme.name().match();
    }
#if 0
    for (auto &directory : theme.directories()) {
        FCITX_INFO() << directory.name();
        FCITX_INFO() << directory.size();
        FCITX_INFO() << directory.scale();
        FCITX_INFO() << directory.context();
        FCITX_INFO() << to_string(directory.type());
        FCITX_INFO() << directory.maxSize();
        FCITX_INFO() << directory.minSize();
        FCITX_INFO() << directory.threshold();
    }
#endif

    FCITX_INFO() << theme.findIcon("fcitx-pinyin", 32, 1);

    return 0;
}
