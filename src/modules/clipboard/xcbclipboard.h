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
    using ConvertSelectionFunction = void (XcbClipboardData::*)(
        xcb_atom_t type, const char *data, size_t length);
    std::unique_ptr<HandlerTableEntryBase>
    convertSelection(const char *type, ConvertSelectionFunction fn);
    bool isValidTextType(xcb_atom_t type) const;
    void checkMime(xcb_atom_t type, const char *data, size_t length);
    void checkPassword(xcb_atom_t type, const char *data, size_t length);
    void readData(xcb_atom_t type, const char *data, size_t length);
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

    xcb_atom_t passwordAtom() const { return passwordAtom_; }
    xcb_atom_t utf8StringAtom() const { return utf8StringAtom_; }

private:
    void primaryChanged();
    void clipboardChanged();
    Clipboard *parent_;
    std::string name_;
    AddonInstance *xcb_;
    std::vector<std::unique_ptr<HandlerTableEntryBase>> selectionCallbacks_;
    xcb_atom_t passwordAtom_ = XCB_ATOM_NONE;
    xcb_atom_t utf8StringAtom_ = XCB_ATOM_NONE;

    XcbClipboardData primary_;
    XcbClipboardData clipboard_;
};

} // namespace fcitx

#endif // _FCITX5_MODULES_CLIPBOARD_XCBCLIPBOARD_H_