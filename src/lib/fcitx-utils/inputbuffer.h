/*
 * Copyright (C) 2017~2017 by CSSlayer
 * wengxt@gmail.com
 *
 * This library is free software; you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as
 * published by the Free Software Foundation; either version 2 of the
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
#ifndef _FCITX_UTILS_INPUTBUFFER_H_
#define _FCITX_UTILS_INPUTBUFFER_H_

#include "fcitxutils_export.h"
#include <cstring>
#include <fcitx-utils/flags.h>
#include <fcitx-utils/macros.h>
#include <memory>

namespace fcitx {
class InputBufferPrivate;

enum class InputBufferOption { None = 0, AsciiOnly = 1, FixedCursor = 1 << 1 };

using InputBufferOptions = Flags<InputBufferOption>;

class FCITXUTILS_EXPORT InputBuffer {
public:
    InputBuffer(InputBufferOptions options = InputBufferOption::None);
    virtual ~InputBuffer();

    InputBufferOptions options() const;

    void type(const char *s, size_t length) { typeImpl(s, length); }
    void type(const std::string &s) { type(s.c_str(), s.size()); }
    void type(const char *s) { type(s, std::strlen(s)); }
    void type(uint32_t unicode);

    virtual void erase(size_t from, size_t to);
    virtual void setCursor(size_t cursor);

    size_t maxSize() const;
    void setMaxSize(size_t s);

    const std::string &userInput() const;
    size_t cursor() const;
    size_t cursorByChar() const;
    size_t size() const;
    uint32_t charAt(size_t i) const;
    std::pair<size_t, size_t> rangeAt(size_t i) const;
    size_t sizeAt(size_t i) const;
    bool empty() const { return size() == 0; }
    inline bool del() {
        auto c = cursor();
        if (c < size()) {
            erase(c, c + 1);
            return true;
        }
        return false;
    }
    inline bool backspace() {
        auto c = cursor();
        if (c > 0) {
            erase(c - 1, c);
            return true;
        }
        return false;
    }

    void clear() { erase(0, size()); }
    void shrinkToFit();

protected:
    virtual void typeImpl(const char *s, size_t length);

private:
    std::unique_ptr<InputBufferPrivate> d_ptr;
    FCITX_DECLARE_PRIVATE(InputBuffer);
};
}

#endif // _FCITX_UTILS_INPUTBUFFER_H_
