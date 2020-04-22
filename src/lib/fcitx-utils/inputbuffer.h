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
#ifndef _FCITX_UTILS_INPUTBUFFER_H_
#define _FCITX_UTILS_INPUTBUFFER_H_

#include <cstring>
#include <memory>
#include <string>
#include <fcitx-utils/flags.h>
#include <fcitx-utils/macros.h>
#include "fcitxutils_export.h"

/// \addtogroup FcitxUtils
/// \{
/// \file
/// \brief Generic InputBuffer to be used to handle user's preedit.

namespace fcitx {
class InputBufferPrivate;

enum class InputBufferOption {
    /// No option.
    None = 0,
    /// The input buffer is ascii character only, non ascii char will raise
    /// exception.
    AsciiOnly = 1,
    /// Whether the input buffer only supports cursor at the end of buffer.
    FixedCursor = 1 << 1
};

using InputBufferOptions = Flags<InputBufferOption>;

/// A string buffer that come with convinient functions to handle use input.
class FCITXUTILS_EXPORT InputBuffer {
public:
    /// Create a input buffer with options.
    /// \see InputBufferOption
    InputBuffer(InputBufferOptions options = InputBufferOption::None);
    virtual ~InputBuffer();

    /// Get the buffer option.
    InputBufferOptions options() const;

    /// Type a C-String with length into buffer.
    bool type(const char *s, size_t length) { return typeImpl(s, length); }
    /// Type an std::stirng to buffer.
    bool type(const std::string &s) { return type(s.c_str(), s.size()); }
    /// Type a C-String to buffer.
    bool type(const char *s) { return type(s, std::strlen(s)); }
    /// Type a ucs4 character to buffer.
    bool type(uint32_t unicode);

    /// Erase a range of character.
    virtual void erase(size_t from, size_t to);
    /// Set cursor position, by character.
    virtual void setCursor(size_t cursor);

    /// Get the max size of the buffer.
    size_t maxSize() const;

    /// Set max size of the buffer.
    void setMaxSize(size_t s);

    /// Utf8 string in the buffer.
    const std::string &userInput() const;

    /// Cursor position by utf8 character.
    size_t cursor() const;

    /// Cursor position by char (byte).
    size_t cursorByChar() const;

    /// Size of buffer, by number of utf8 character.
    size_t size() const;

    /// UCS-4 char in the buffer. Will raise exception if i is out of range.
    uint32_t charAt(size_t i) const;

    /// Byte range for character at position i.
    std::pair<size_t, size_t> rangeAt(size_t i) const;

    /// Byte size at position i.
    size_t sizeAt(size_t i) const;

    /// Whether buffer is empty.
    bool empty() const { return size() == 0; }

    /// Helper function to implement "delete" key.
    inline bool del() {
        auto c = cursor();
        if (c < size()) {
            erase(c, c + 1);
            return true;
        }
        return false;
    }

    /// Helper function to implement "backspace" key.
    inline bool backspace() {
        auto c = cursor();
        if (c > 0) {
            erase(c - 1, c);
            return true;
        }
        return false;
    }

    /// Clear all buffer.
    void clear() { erase(0, size()); }

    /// Save memory by call shrink to fit to internal buffer.
    void shrinkToFit();

protected:
    /// Type a certain length of utf8 character to the buffer. [s, s+length]
    /// need to be valid utf8 string.
    virtual bool typeImpl(const char *s, size_t length);

private:
    std::unique_ptr<InputBufferPrivate> d_ptr;
    FCITX_DECLARE_PRIVATE(InputBuffer);
};
} // namespace fcitx

#endif // _FCITX_UTILS_INPUTBUFFER_H_
