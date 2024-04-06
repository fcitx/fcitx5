/*
 * SPDX-FileCopyrightText: 2021~2021 CSSlayer <wengxt@gmail.com>
 *
 * SPDX-License-Identifier: LGPL-2.1-or-later
 *
 */
#include <fcitx-utils/log.h>
#include <fcitx-utils/textformatflags.h>
#include <fcitx/text.h>
using namespace fcitx;

void test_basic() {
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

    text = Text("ABC");
    Text another = Text("DEF", TextFormatFlag::Bold);
    another.append("GHI", TextFormatFlag::Italic);
    text.append(another);
    FCITX_ASSERT(text.toString() == "ABCDEFGHI");
    FCITX_ASSERT(text.formatAt(1) == TextFormatFlag::Bold);
    FCITX_ASSERT(text.formatAt(2) == TextFormatFlag::Italic);
}

void test_normalize() {
    Text text;
    text.append("ABC", TextFormatFlag::Underline);
    text.append("", TextFormatFlag::HighLight);
    text.append("DEF", TextFormatFlag::Underline);
    text.append("HIJ", TextFormatFlag::HighLight);

    auto normalized = text.normalize();
    FCITX_ASSERT(normalized.size() == 2) << normalized;
    FCITX_ASSERT(normalized.stringAt(0) == "ABCDEF");
    FCITX_ASSERT(normalized.formatAt(0) == TextFormatFlag::Underline);
    FCITX_ASSERT(normalized.stringAt(1) == "HIJ");
    FCITX_ASSERT(normalized.formatAt(1) == TextFormatFlag::HighLight);

    Text empty;
    empty.append("", TextFormatFlag::Underline);
    empty.append("", TextFormatFlag::Underline);
    empty.append("", TextFormatFlag::Underline);

    auto normalizedEmpty = empty.normalize();
    FCITX_ASSERT(normalizedEmpty.empty()) << normalizedEmpty;
}

int main() {
    test_basic();
    test_normalize();
    return 0;
}
