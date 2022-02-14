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

/**
 * Class represents the current state of surrounding text of an input context.
 */
class FCITXCORE_EXPORT SurroundingText {
public:
    SurroundingText();
    virtual ~SurroundingText();

    /// Reset surrounding text to invalid state.
    void invalidate();
    /// Return whether surrounding text is valid.
    bool isValid() const;
    /// offset of anchor in character.
    unsigned int anchor() const;
    /// offset of anchor in character.
    unsigned int cursor() const;
    const std::string &text() const;
    std::string selectedText() const;

    /**
     * Set current of surrounding text.
     *
     * If cursor and anchor are out of range, it will be reset to invalid state.
     *
     * @param text text
     * @param cursor offset of cursor in character.
     * @param anchor offset of anchor in character.
     */
    void setText(const std::string &text, unsigned int cursor,
                 unsigned int anchor);

    /**
     * Set current cursor and anchor of surrounding text.
     *
     * If cursor and anchor are out of range, it will be reset to invalid state.
     * This function is useful to safe some bandwidth.
     *
     * @param cursor offset of cursor in character.
     * @param anchor offset of anchor in character.
     */
    void setCursor(unsigned int cursor, unsigned int anchor);

    /**
     * Delete surrounding text with offset and size.
     *
     * This can be used to update the local state of surrounding text before
     * client send it back.
     * @param offset offset to cursor position.
     * @param size length of text to delete.
     */
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
