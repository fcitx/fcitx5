/*
 * SPDX-FileCopyrightText: 2016-2016 CSSlayer <wengxt@gmail.com>
 *
 * SPDX-License-Identifier: LGPL-2.1-or-later
 *
 */
#ifndef _FCITX_SURROUNDINGTEXT_H_
#define _FCITX_SURROUNDINGTEXT_H_

#include <memory>
#include <string>
#include <fcitx-utils/log.h>
#include <fcitx-utils/macros.h>
#include "fcitxcore_export.h"

/// \addtogroup FcitxCore
/// \{
/// \file
/// \brief Local cache for surrounding text

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
