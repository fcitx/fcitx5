/*
 * SPDX-FileCopyrightText: 2024~2024 CSSlayer <wengxt@gmail.com>
 *
 * SPDX-License-Identifier: LGPL-2.1-or-later
 *
 */
#include "xcbclipboard.h"
#include <ctime>
#include <string_view>
#include <xcb/xproto.h>
#include "clipboard.h"

namespace fcitx {

XcbClipboardData::XcbClipboardData(XcbClipboard *xcbClip, XcbClipboardMode mode)
    : xcbClip_(xcbClip), mode_(mode) {}

void XcbClipboardData::request() {
    //
    reset();
    callback_ = xcbClip_->xcb()->call<IXCBModule::convertSelection>(
        xcbClip_->name(), modeString(), "TARGETS",
        [this](xcb_atom_t type, const char *data, size_t length) {
            checkMime(type, data, length);
        });
}

void XcbClipboardData::reset() {
    callback_.reset();
    password_ = false;
}

const char *XcbClipboardData::modeString() const {
    if (mode_ == XcbClipboardMode::Primary) {
        return "PRIMARY";
    }
    return "CLIPBOARD";
}

std::unique_ptr<HandlerTableEntryBase> XcbClipboardData::convertSelection(
    const char *type, XcbClipboardData::ConvertSelectionFunction fn) {
    return xcbClip_->xcb()->call<IXCBModule::convertSelection>(
        xcbClip_->name(), modeString(), type,
        [this, fn](xcb_atom_t type, const char *data, size_t length) {
            (this->*fn)(type, data, length);
        });
}

bool XcbClipboardData::isValidTextType(xcb_atom_t type) const {
    if (type == XCB_ATOM_STRING) {
        return true;
    }
    auto utf8Atom = xcbClip_->utf8StringAtom();
    return utf8Atom != XCB_ATOM_NONE && type == utf8Atom;
}

void XcbClipboardData::checkMime(xcb_atom_t type, const char *data,
                                 size_t length) {
    if (type != XCB_ATOM_ATOM) {
        reset();
        return;
    }

    const auto *atoms = reinterpret_cast<const xcb_atom_t *>(data);
    bool maybePassword = false;
    bool isText = false;
    size_t size = length / sizeof(xcb_atom_t);
    for (size_t i = 0; i < size; i++) {
        if (xcbClip_->passwordAtom() != XCB_ATOM_NONE &&
            atoms[i] == xcbClip_->passwordAtom()) {
            maybePassword = true;
        } else if (isValidTextType(atoms[i])) {
            isText = true;
        }
    }

    if (!isText) {
        reset();
        return;
    }

    if (maybePassword) {
        callback_ = convertSelection(PASSWORD_MIME_TYPE,
                                     &XcbClipboardData::checkPassword);
    } else {
        callback_ = convertSelection("", &XcbClipboardData::readData);
    }
}

void XcbClipboardData::checkPassword(xcb_atom_t /*type*/, const char *data,
                                     size_t length) {
    std::string_view str(data, length);
    if (str == "secret") {
        if (*xcbClip_->parent()->config().ignorePasswordFromPasswordManager) {
            FCITX_CLIPBOARD_DEBUG()
                << "XCB display:" << xcbClip_->name() << " " << modeString()
                << " contains password, ignore.";
            reset();
            return;
        }
        password_ = true;
    }
    callback_ = convertSelection("", &XcbClipboardData::readData);
}

void XcbClipboardData::readData(xcb_atom_t type, const char *data,
                                size_t length) {

    switch (mode_) {
    case XcbClipboardMode::Primary:
        if (!data || !isValidTextType(type)) {
            xcbClip_->setPrimary("", /*password=*/false);
        } else {
            std::string str(data, length);
            xcbClip_->setPrimary(str, /*password=*/password_);
        }
        break;
    case XcbClipboardMode::Clipboard:
        if (isValidTextType(type) && data) {
            std::string str(data, length);
            xcbClip_->setClipboard(str, /*password=*/password_);
        }
        break;
    }
    reset();
}

XcbClipboard::XcbClipboard(Clipboard *clipboard, std::string name)
    : parent_(clipboard), name_(std::move(name)), xcb_(clipboard->xcb()),
      primary_(this, XcbClipboardMode::Primary),
      clipboard_(this, XcbClipboardMode::Clipboard) {

    // Ensure that atom exists. See:
    // https://github.com/fcitx/fcitx5/issues/610 PRIMARY /
    // CLIPBOARD is not guaranteed to exist if fcitx5 is
    // launched at an very early stage. We should try to create
    // atom ourselves.
    xcb_->call<IXCBModule::atom>(name_, "PRIMARY", false);
    xcb_->call<IXCBModule::atom>(name_, "CLIPBOARD", false);
    xcb_->call<IXCBModule::atom>(name_, "TARGETS", false);
    passwordAtom_ =
        xcb_->call<IXCBModule::atom>(name_, PASSWORD_MIME_TYPE, false);
    utf8StringAtom_ = xcb_->call<IXCBModule::atom>(name_, "UTF8_STRING", false);
    selectionCallbacks_.emplace_back(xcb_->call<IXCBModule::addSelection>(
        name_, "PRIMARY", [this](xcb_atom_t) { primaryChanged(); }));
    selectionCallbacks_.emplace_back(xcb_->call<IXCBModule::addSelection>(
        name_, "CLIPBOARD", [this](xcb_atom_t) { clipboardChanged(); }));
    primaryChanged();
    clipboardChanged();
}

void XcbClipboard::primaryChanged() { primary_.request(); }

void XcbClipboard::clipboardChanged() { clipboard_.request(); }

void XcbClipboard::setClipboard(const std::string &str, bool password) {
    parent_->setClipboardV2(name_, str, password);
}

void XcbClipboard::setPrimary(const std::string &str, bool password) {
    parent_->setPrimaryV2(name_, str, password);
}

} // namespace fcitx
