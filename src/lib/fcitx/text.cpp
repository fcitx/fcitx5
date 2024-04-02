/*
 * SPDX-FileCopyrightText: 2016-2016 CSSlayer <wengxt@gmail.com>
 *
 * SPDX-License-Identifier: LGPL-2.1-or-later
 *
 */

#include "text.h"
#include <iterator>
#include <stdexcept>
#include <tuple>
#include <vector>
#include "fcitx-utils/stringutils.h"
#include "fcitx-utils/utf8.h"

namespace fcitx {

class TextPrivate {
public:
    TextPrivate() = default;
    FCITX_INLINE_DEFINE_DEFAULT_DTOR_AND_COPY(TextPrivate)

    std::vector<std::tuple<std::string, TextFormatFlags>> texts_;
    int cursor_ = -1;
};

Text::Text() : d_ptr(std::make_unique<TextPrivate>()) {}

Text::Text(std::string text, TextFormatFlags flag) : Text() {
    append(std::move(text), flag);
}

FCITX_DEFINE_DPTR_COPY_AND_DEFAULT_DTOR_AND_MOVE(Text)

void Text::clear() {
    FCITX_D();
    d->texts_.clear();
    setCursor();
}

int Text::cursor() const {
    FCITX_D();
    return d->cursor_;
}

void Text::setCursor(int pos) {
    FCITX_D();
    d->cursor_ = pos;
}

void Text::append(std::string str, TextFormatFlags flag) {
    FCITX_D();
    if (!utf8::validate(str)) {
        throw std::invalid_argument("Invalid utf8 string");
    }
    d->texts_.emplace_back(std::move(str), flag);
}

void Text::append(Text text) {
    FCITX_D();
    std::copy(std::make_move_iterator(text.d_ptr->texts_.begin()),
              std::make_move_iterator(text.d_ptr->texts_.end()),
              std::back_inserter(d->texts_));
}

const std::string &Text::stringAt(int idx) const {
    FCITX_D();
    return std::get<std::string>(d->texts_[idx]);
}

TextFormatFlags Text::formatAt(int idx) const {
    FCITX_D();
    return std::get<TextFormatFlags>(d->texts_[idx]);
}

size_t Text::size() const {
    FCITX_D();
    return d->texts_.size();
}

bool Text::empty() const {
    FCITX_D();
    return d->texts_.empty();
}

std::string Text::toString() const {
    FCITX_D();
    std::string result;
    for (const auto &p : d->texts_) {
        result += std::get<std::string>(p);
    }

    return result;
}

size_t Text::textLength() const {
    FCITX_D();
    size_t length = 0;
    for (const auto &p : d->texts_) {
        length += std::get<std::string>(p).size();
    }

    return length;
}

std::string Text::toStringForCommit() const {
    FCITX_D();
    std::string result;
    for (const auto &p : d->texts_) {
        if (!(std::get<TextFormatFlags>(p) & TextFormatFlag::DontCommit)) {
            result += std::get<std::string>(p);
        }
    }

    return result;
}

std::ostream &operator<<(std::ostream &os, const Text &text) {
    os << "Text(";
    for (size_t i = 0; i < text.size(); i++) {
        os << "<" << text.stringAt(i) << ", flag=" << text.formatAt(i) << ">";
        if (i + 1 != text.size()) {
            os << ", ";
        }
    }
    os << ", cursor=" << text.cursor() << ")";
    return os;
}

std::vector<Text> Text::splitByLine() const {
    FCITX_D();
    std::vector<Text> texts;
    // Put first line.
    texts.emplace_back();
    for (const auto &p : d->texts_) {
        if (std::get<std::string>(p).empty()) {
            continue;
        }
        auto lines = stringutils::split(std::get<std::string>(p), "\n",
                                        stringutils::SplitBehavior::KeepEmpty);
        auto flag = std::get<TextFormatFlags>(p);
        texts.back().append(lines[0], flag);
        for (size_t i = 1; i < lines.size(); i++) {
            texts.emplace_back();
            texts.back().append(lines[i], flag);
        }
    }

    return texts;
}

Text Text::normalize() const {
    FCITX_D();

    Text normalized;
    std::string curStr;
    TextFormatFlags curFormat;
    for (const auto &[str, format] : d->texts_) {
        if (str.empty()) {
            continue;
        }
        if (curFormat == format) {
            curStr.append(str);
        } else {
            if (!curStr.empty()) {
                normalized.append(std::move(curStr), curFormat);
            }
            curStr = str;
            curFormat = format;
        }
    }
    if (!curStr.empty()) {
        normalized.append(std::move(curStr), curFormat);
    }
    normalized.setCursor(cursor());
    return normalized;
}

} // namespace fcitx
