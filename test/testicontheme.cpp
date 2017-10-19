/*
* Copyright (C) 2017~2017 by CSSlayer
* wengxt@gmail.com
*
* This library is free software; you can redistribute it and/or modify
* it under the terms of the GNU Lesser General Public License as
* published by the Free Software Foundation; either version 2.1 of the
* License, or (at your option) any later version.
*
* This library is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
* Lesser General Public License for more details.
*
* You should have received a copy of the GNU Lesser General Public
* License along with this library; see the file COPYING. If not,
* see <http://www.gnu.org/licenses/>.
*/
#include "fcitx-utils/log.h"
#include "fcitx/icontheme.h"

using namespace fcitx;

int main() {
    IconTheme theme("breeze");
    FCITX_LOG(Info) << theme.name().match();
    FCITX_LOG(Info) << theme.comment().match();

    for (auto &inheritTheme : theme.inherits()) {
        FCITX_LOG(Info) << inheritTheme.name().match();
    }
#if 0
    for (auto &directory : theme.directories()) {
        FCITX_LOG(Info) << directory.name();
        FCITX_LOG(Info) << directory.size();
        FCITX_LOG(Info) << directory.scale();
        FCITX_LOG(Info) << directory.context();
        FCITX_LOG(Info) << to_string(directory.type());
        FCITX_LOG(Info) << directory.maxSize();
        FCITX_LOG(Info) << directory.minSize();
        FCITX_LOG(Info) << directory.threshold();
    }
#endif

    FCITX_LOG(Info) << theme.findIcon("fcitx-pinyin", 32, 1);

    return 0;
}
