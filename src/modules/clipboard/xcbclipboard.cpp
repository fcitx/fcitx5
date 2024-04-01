/*
 * SPDX-FileCopyrightText: 2024~2024 CSSlayer <wengxt@gmail.com>
 *
 * SPDX-License-Identifier: LGPL-2.1-or-later
 *
 */
#include "xcbclipboard.h"
#include "clipboard.h"

namespace fcitx {

XcbClipboard::XcbClipboard(Clipboard *clipboard, std::string name)
    : parent_(clipboard), name_(std::move(name)), xcb_(clipboard->xcb()) {

    // Ensure that atom exists. See:
    // https://github.com/fcitx/fcitx5/issues/610 PRIMARY /
    // CLIPBOARD is not guaranteed to exist if fcitx5 is
    // launched at an very early stage. We should try to create
    // atom ourselves.
    xcb_->call<IXCBModule::atom>(name_, "PRIMARY", false);
    xcb_->call<IXCBModule::atom>(name_, "CLIPBOARD", false);
    xcb_->call<IXCBModule::atom>(name_, "TARGETS", false);
    utf8StringAtom_ = xcb_->call<IXCBModule::atom>(name_, "UTF8_STRING", false);
    selectionCallbacks_.emplace_back(xcb_->call<IXCBModule::addSelection>(
        name_, "PRIMARY", [this](xcb_atom_t) { primaryChanged(); }));
    selectionCallbacks_.emplace_back(xcb_->call<IXCBModule::addSelection>(
        name_, "CLIPBOARD", [this](xcb_atom_t) { clipboardChanged(); }));
    primaryChanged();
    clipboardChanged();
}

void XcbClipboard::primaryChanged() {
    primaryCallback_ = xcb_->call<IXCBModule::convertSelection>(
        name_, "PRIMARY", "",
        [this](xcb_atom_t type, const char *data, size_t length) {
            if (type != XCB_ATOM_STRING && type != utf8StringAtom_) {
                return;
            }
            if (!data) {
                parent_->setPrimary(name_, "");
            } else {
                std::string str(data, length);
                parent_->setPrimary(name_, str);
            }
            primaryCallback_.reset();
        });
}

void XcbClipboard::clipboardChanged() {
    clipboardCallback_ = xcb_->call<IXCBModule::convertSelection>(
        name_, "CLIPBOARD", "",
        [this](xcb_atom_t type, const char *data, size_t length) {
            if (type != XCB_ATOM_STRING && type != utf8StringAtom_) {
                return;
            }
            if (!data || !length) {
                return;
            }
            std::string str(data, length);
            parent_->setClipboard(name_, str);
            clipboardCallback_.reset();
        });
}

void XcbClipboard::setClipboard(const std::string &str) {
    parent_->setClipboard(name_, str);
}

void XcbClipboard::setPrimary(const std::string &str) {
    parent_->setPrimary(name_, str);
}

} // namespace fcitx