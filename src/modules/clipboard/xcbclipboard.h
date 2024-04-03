/*
 * SPDX-FileCopyrightText: 2024~2024 CSSlayer <wengxt@gmail.com>
 *
 * SPDX-License-Identifier: LGPL-2.1-or-later
 *
 */
#ifndef _FCITX5_MODULES_CLIPBOARD_XCBCLIPBOARD_H_
#define _FCITX5_MODULES_CLIPBOARD_XCBCLIPBOARD_H_

#include <string>
#include <xcb/xproto.h>
#include "fcitx-utils/handlertable.h"
#include "fcitx/addoninstance.h"

namespace fcitx {

class Clipboard;
class XcbClipboard;

enum class XcbClipboardMode {
    Primary,
    Clipboard,
};

class XcbClipboardData {
public:
    XcbClipboardData(XcbClipboard *xcbClip, XcbClipboardMode mode);

    void request();

private:
    void cleanup();

    const char *modeString() const;

    XcbClipboard *xcbClip_ = nullptr;
    XcbClipboardMode mode_;
    std::unique_ptr<HandlerTableEntryBase> callback_;
};

class XcbClipboard {

public:
    XcbClipboard(Clipboard *clipboard, std::string name);

    void setClipboard(const std::string &str);
    void setPrimary(const std::string &str);

    AddonInstance *xcb() const { return xcb_; }
    const std::string &name() const { return name_; }

private:
    void primaryChanged();
    void clipboardChanged();
    Clipboard *parent_;
    std::string name_;
    AddonInstance *xcb_;
    std::vector<std::unique_ptr<HandlerTableEntryBase>> selectionCallbacks_;
    xcb_atom_t passwordAtom_ = XCB_ATOM_NONE;
    xcb_atom_t utf8StringAtom_ = XCB_ATOM_NONE;

    std::unique_ptr<HandlerTableEntryBase> primaryCallback_;
    std::unique_ptr<HandlerTableEntryBase> clipboardCallback_;
};

} // namespace fcitx

#endif // _FCITX5_MODULES_CLIPBOARD_XCBCLIPBOARD_H_