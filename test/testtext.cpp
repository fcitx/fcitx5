/*
 * SPDX-FileCopyrightText: 2021~2021 CSSlayer <wengxt@gmail.com>
 *
 * SPDX-License-Identifier: LGPL-2.1-or-later
 *
 */
#include <fcitx-utils/log.h>
#include <fcitx/text.h>

int main() {
    using namespace fcitx;
    Text text("ABC");

    FCITX_ASSERT(text.toString() == "ABC");
    FCITX_ASSERT(text.toStringForCommit() == "ABC");

    text.append("DEF", TextFormatFlag::DontCommit);
    text.append("GHI", TextFormatFlag::HighLight);

    FCITX_ASSERT(text.toString() == "ABCDEFGHI");
    FCITX_ASSERT(text.toStringForCommit() == "ABCGHI");
    FCITX_ASSERT(text.formatAt(2) == TextFormatFlag::HighLight);

    text.append("X\n", TextFormatFlag::HighLight);
    text.append("Y\n", TextFormatFlag::HighLight);
    text.append("\nZ\n1", TextFormatFlag::HighLight);

    auto lines = text.splitByLine();

    FCITX_ASSERT(lines.size() == 5);
    FCITX_ASSERT(lines[0].toStringForCommit() == "ABCGHIX");
    FCITX_ASSERT(lines[1].toStringForCommit() == "Y");
    FCITX_ASSERT(lines[2].toStringForCommit() == "");
    FCITX_ASSERT(lines[3].toStringForCommit() == "Z");
    FCITX_ASSERT(lines[4].toStringForCommit() == "1");

    return 0;
}
