/*
 * SPDX-FileCopyrightText: 2017-2017 CSSlayer <wengxt@gmail.com>
 *
 * SPDX-License-Identifier: LGPL-2.1-or-later
 *
 */
#include <fcitx-utils/log.h>
#include <fcitx-utils/metastring.h>

int main() {
    int a = 0;
    fcitx::Log::setLogRule("*=5");

    FCITX_INFO() << (a = 1);
    FCITX_ASSERT(a == 1);

    fcitx::Log::setLogRule("*=4");
    FCITX_DEBUG() << (a = 2);
    FCITX_ASSERT(a == 1);

    std::vector<int> vec{1, 2, 3};
    FCITX_INFO() << vec;

    std::unordered_map<int, int> map{{1, 1}, {2, 3}};
    FCITX_INFO() << map;

    FCITX_INFO() << std::make_tuple(1, 3, "a", false);

    return 0;
}
