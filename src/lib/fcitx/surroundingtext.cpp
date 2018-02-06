//
// Copyright (C) 2016~2016 by CSSlayer
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

#include "surroundingtext.h"
#include "fcitx-utils/utf8.h"

namespace fcitx {

class SurroundingTextPrivate {
public:
    SurroundingTextPrivate() : anchor_(0), cursor_(0), valid_(false) {}

    unsigned int anchor_, cursor_;
    std::string text_;

    bool valid_;
};

SurroundingText::SurroundingText()
    : d_ptr(std::make_unique<SurroundingTextPrivate>()) {}

SurroundingText::~SurroundingText() {}

bool SurroundingText::isValid() const {
    FCITX_D();
    return d->valid_;
}

void SurroundingText::invalidate() {
    FCITX_D();
    d->valid_ = false;
    d->anchor_ = 0;
    d->cursor_ = 0;
    d->text_ = std::string();
}

const std::string &SurroundingText::text() const {
    FCITX_D();
    return d->text_;
}

unsigned int SurroundingText::anchor() const {
    FCITX_D();
    return d->anchor_;
}

unsigned int SurroundingText::cursor() const {
    FCITX_D();
    return d->cursor_;
}

std::string SurroundingText::selectedText() const {
    FCITX_D();
    auto start = std::min(anchor(), cursor());
    auto end = std::max(anchor(), cursor());
    auto len = end - start;
    if (len == 0) {
        return {};
    }

    auto startIter = utf8::nextNChar(d->text_.begin(), start);
    auto endIter = utf8::nextNChar(startIter, len);
    return std::string(startIter, endIter);
}

void SurroundingText::setText(const std::string &text, unsigned int cursor,
                              unsigned int anchor) {
    FCITX_D();
    d->valid_ = true;
    d->text_ = text;
    d->cursor_ = cursor;
    d->anchor_ = anchor;
}

void SurroundingText::setCursor(unsigned int cursor, unsigned int anchor) {
    FCITX_D();
    d->cursor_ = cursor;
    d->anchor_ = anchor;
}

void SurroundingText::deleteText(int offset, unsigned int size) {
    FCITX_D();
    if (!d->valid_) {
        return;
    }

    /*
     * do the real deletion here, and client might update it, but input method
     * itself
     * would expect a up to date value after this call.
     *
     * Make their life easier.
     */
    int cursor_pos = d->cursor_ + offset;
    size_t len = utf8::length(d->text_);
    if (cursor_pos >= 0 && len >= size + cursor_pos) {
        auto start = utf8::nextNChar(d->text_.begin(), cursor_pos);
        auto end = utf8::nextNChar(start, size);
        d->text_.erase(start, end);
        d->cursor_ = cursor_pos;
    } else {
        d->text_.clear();
        d->cursor_ = 0;
    }
    d->anchor_ = d->cursor_;
}
} // namespace fcitx
