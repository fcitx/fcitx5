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
#ifndef _FCITX_SURROUNDINGTEXT_H_
#define _FCITX_SURROUNDINGTEXT_H_

#include <memory>
#include <string>
#include <fcitx-utils/log.h>
#include <fcitx-utils/macros.h>
#include "fcitxcore_export.h"

namespace fcitx {

class SurroundingTextPrivate;

class FCITXCORE_EXPORT SurroundingText {
public:
    SurroundingText();
    virtual ~SurroundingText();

    void invalidate();
    bool isValid() const;
    unsigned int anchor() const;
    unsigned int cursor() const;
    const std::string &text() const;
    std::string selectedText() const;

    void setText(const std::string &text, unsigned int cursor,
                 unsigned int anchor);
    void setCursor(unsigned int cursor, unsigned int anchor);
    void deleteText(int offset, unsigned int size);

private:
    std::unique_ptr<SurroundingTextPrivate> d_ptr;
    FCITX_DECLARE_PRIVATE(SurroundingText);
};

FCITXCORE_EXPORT
LogMessageBuilder &operator<<(LogMessageBuilder &log,
                              const SurroundingText &surroundingText);

} // namespace fcitx

#endif // _FCITX_SURROUNDINGTEXT_H_
