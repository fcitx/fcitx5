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
#include "inputbuffer.h"
#include "fcitx-utils/utf8.h"
#include <exception>
#include <functional>
#include <numeric>
#include <stdexcept>
#include <vector>

namespace fcitx {

class InputBufferPrivate {
public:
    InputBufferPrivate(InputBufferOptions options) : options_(options) {}

    // make sure acc_[i] is valid, i \in [0, size()]
    // acc_[i] = sum(j \in 0..i-1 | sz_[j])
    void ensureAccTill(size_t i) const {
        if (accDirty_ <= i) {
            if (accDirty_ == 0) {
                // acc_[0] is always 0
                accDirty_++;
            }
            for (auto iter = std::next(sz_.begin(), accDirty_ - 1),
                      e = std::next(sz_.begin(), i);
                 iter < e; iter++) {
                acc_[accDirty_] = acc_[accDirty_ - 1] + *iter;
                accDirty_++;
            }
        }
    }

    inline bool isAsciiOnly() const {
        return options_.test(InputBufferOption::AsciiOnly);
    }

    inline bool isFixedCursor() const {
        return options_.test(InputBufferOption::FixedCursor);
    }

    const InputBufferOptions options_;
    std::string input_;
    size_t cursor_ = 0;
    std::vector<size_t> sz_; // utf8 lengthindex helper
    size_t maxSize_ = 0;
    mutable std::vector<size_t> acc_ = {0};
    mutable size_t accDirty_ = 0;
};

InputBuffer::InputBuffer(InputBufferOptions options)
    : d_ptr(std::make_unique<InputBufferPrivate>(options)) {}

InputBuffer::~InputBuffer() = default;

InputBufferOptions InputBuffer::options() const {
    FCITX_D();
    return d->options_;
}

bool InputBuffer::type(uint32_t c) { return type(fcitx::utf8::UCS4ToUTF8(c)); }

const std::string &InputBuffer::userInput() const {
    FCITX_D();
    return d->input_;
}

bool InputBuffer::typeImpl(const char *s, size_t length) {
    FCITX_D();
    auto utf8Length = fcitx::utf8::lengthValidated(s, s + length);
    if (utf8Length == fcitx::utf8::INVALID_LENGTH) {
        throw std::invalid_argument("Invalid UTF-8 string");
    }
    if (d->isAsciiOnly() && utf8Length != length) {
        throw std::invalid_argument(
            "ascii only buffer only accept ascii only string");
    }
    if (d->maxSize_ && (utf8Length + size() > d->maxSize_)) {
        return false;
    }
    d->input_.insert(std::next(d->input_.begin(), cursorByChar()), s,
                     s + length);
    if (!d->isAsciiOnly()) {
        auto iter = s;
        auto func = [&iter]() {
            auto next = fcitx::utf8::nextChar(iter);
            auto diff = std::distance(iter, next);
            iter = next;
            return diff;
        };

        auto pos = d->cursor_;
        while (iter < s + length) {
            d->sz_.insert(std::next(d->sz_.begin(), pos), func());
            pos++;
        }
        d->acc_.resize(d->sz_.size() + 1);
        auto newDirty = d->cursor_ > 0 ? d->cursor_ - 1 : 0;
        if (d->accDirty_ > newDirty) {
            d->accDirty_ = newDirty;
        }
    }
    d->cursor_ += utf8Length;
    return true;
}

size_t InputBuffer::cursorByChar() const {
    FCITX_D();
    if (d->isAsciiOnly()) {
        return d->cursor_;
    }
    if (d->cursor_ == size()) {
        return d->input_.size();
    }
    d->ensureAccTill(d->cursor_);
    return d->acc_[d->cursor_];
}

size_t InputBuffer::cursor() const {
    FCITX_D();
    return d->cursor_;
}

size_t InputBuffer::size() const {
    FCITX_D();
    return d->isAsciiOnly() ? d->input_.size() : d->sz_.size();
}

void InputBuffer::setCursor(size_t cursor) {
    FCITX_D();
    if (d->isFixedCursor()) {
        if (cursor != size()) {
            throw std::out_of_range(
                "only valid position of cursor is size() for fixed cursor");
        }
        return;
    }

    if (d->cursor_ > size()) {
        throw std::out_of_range("cursor position out of range");
    }
    d->cursor_ = cursor;
}

void InputBuffer::setMaxSize(size_t s) {
    FCITX_D();
    d->maxSize_ = s;
}

size_t InputBuffer::maxSize() const {
    FCITX_D();
    return d->maxSize_;
}

void InputBuffer::erase(size_t from, size_t to) {
    FCITX_D();
    if (from < to && to <= size()) {
        if (d->isFixedCursor() && to != size()) {
            return;
        }

        size_t fromByChar, lengthByChar;
        if (d->isAsciiOnly()) {
            fromByChar = from;
            lengthByChar = to - from;
        } else {
            d->ensureAccTill(to);
            fromByChar = d->acc_[from];
            lengthByChar = d->acc_[to] - fromByChar;
            d->sz_.erase(std::next(d->sz_.begin(), from),
                         std::next(d->sz_.begin(), to));
            d->accDirty_ = from;
            d->acc_.resize(d->sz_.size() + 1);
        }
        if (d->cursor_ > from) {
            if (d->cursor_ <= to) {
                d->cursor_ = from;
            } else {
                d->cursor_ -= to - from;
            }
        }
        d->input_.erase(fromByChar, lengthByChar);
    }
}

std::pair<size_t, size_t> InputBuffer::rangeAt(size_t i) const {
    FCITX_D();
    if (i >= size()) {
        throw std::out_of_range("out of range");
    }
    if (d->isAsciiOnly()) {
        return {i, i + 1};
    } else {
        d->ensureAccTill(i);
        return {d->acc_[i], d->acc_[i] + d->sz_[i]};
    }
}

uint32_t InputBuffer::charAt(size_t i) const {
    FCITX_D();
    if (i >= size()) {
        throw std::out_of_range("out of range");
    }
    if (d->isAsciiOnly()) {
        return d->input_[i];
    } else {
        d->ensureAccTill(i);
        return utf8::getChar(d->input_.begin() + d->acc_[i],
                             d->input_.begin() + d->sz_[i]);
    }
}

size_t InputBuffer::sizeAt(size_t i) const {
    FCITX_D();
    if (d->isAsciiOnly()) {
        return 1;
    } else {
        return d->sz_[i];
    }
}

void InputBuffer::shrinkToFit() {
    FCITX_D();
    d->input_.shrink_to_fit();
    d->sz_.shrink_to_fit();
    d->acc_.shrink_to_fit();
}
} // namespace fcitx
