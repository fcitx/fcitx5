/*
 * SPDX-FileCopyrightText: 2016-2016 CSSlayer <wengxt@gmail.com>
 *
 * SPDX-License-Identifier: LGPL-2.1-or-later
 *
 */

#include "surroundingtext.h"
#include "fcitx-utils/utf8.h"

namespace fcitx {

class SurroundingTextPrivate {
public:
    SurroundingTextPrivate() {}

    unsigned int anchor_ = 0, cursor_ = 0;
    std::string text_;
    size_t utf8Length_ = 0;

    bool valid_ = false;
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
    d->utf8Length_ = 0;
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
    auto length = utf8::lengthValidated(text);
    if (length == utf8::INVALID_LENGTH || length < cursor || length < anchor) {
        invalidate();
        return;
    }

    d->valid_ = true;
    d->text_ = text;
    d->cursor_ = cursor;
    d->anchor_ = anchor;
    d->utf8Length_ = length;
}

void SurroundingText::setCursor(unsigned int cursor, unsigned int anchor) {
    FCITX_D();
    if (d->utf8Length_ < cursor || d->utf8Length_ < anchor) {
        invalidate();
        return;
    }
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
        d->utf8Length_ = utf8::lengthValidated(d->text_);
        if (d->utf8Length_ == utf8::INVALID_LENGTH) {
            invalidate();
        }
    } else {
        d->text_.clear();
        d->cursor_ = 0;
        d->utf8Length_ = 0;
    }
    d->anchor_ = d->cursor_;
}

LogMessageBuilder &operator<<(LogMessageBuilder &log,
                              const SurroundingText &surroundingText) {
    log << "SurroundingText(text=";
    log << surroundingText.text();
    log << ",anchor=" << surroundingText.anchor();
    log << ",cursor=" << surroundingText.cursor();
    log << ")";
    return log;
}

} // namespace fcitx
