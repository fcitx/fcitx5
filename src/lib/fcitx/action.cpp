/*
 * SPDX-FileCopyrightText: 2016-2016 CSSlayer <wengxt@gmail.com>
 *
 * SPDX-License-Identifier: LGPL-2.1-or-later
 *
 */

#include "action.h"
#include <memory>
#include <string>
#include "fcitx-utils/connectableobject.h"
#include "fcitx-utils/key.h"
#include "fcitx-utils/macros.h"
#include "menu.h"
#include "userinterfacemanager.h"

namespace fcitx {

class ActionPrivate : QPtrHolder<Action> {
public:
    ActionPrivate(Action *q) : QPtrHolder<Action>(q) {}
    std::string name_;
    int id_ = 0;
    bool checkable_ = false;
    bool separator_ = false;
    KeyList hotkey_;
    FCITX_DEFINE_SIGNAL_PRIVATE(Action, Update);
};

Action::Action() : d_ptr(std::make_unique<ActionPrivate>(this)) {}

Action::~Action() { destroy(); }

bool Action::isSeparator() const {
    FCITX_D();
    return d->separator_;
}

Action &Action::setSeparator(bool separator) {
    FCITX_D();
    d->separator_ = separator;
    return *this;
}

bool Action::registerAction(const std::string &name,
                            UserInterfaceManager *manager) {
    return manager->registerAction(name, this);
}

void Action::setName(const std::string &name) {
    FCITX_D();
    d->name_ = name;
}

int Action::id() {
    FCITX_D();
    return d->id_;
}

void Action::setId(int id) {
    FCITX_D();
    d->id_ = id;
}

Action &Action::setCheckable(bool checkable) {
    FCITX_D();
    d->checkable_ = checkable;
    return *this;
}

bool Action::isCheckable() const {
    FCITX_D();
    return d->checkable_;
}

void Action::setMenu(Menu *menu) {
    auto *oldMenu = this->menu();
    if (oldMenu) {
        oldMenu->removeParent(this);
    }
    if (menu) {
        menu->addParent(this);
    }
}

Menu *Action::menu() {
    auto childList = childs();
    if (!childList.empty()) {
        return static_cast<Menu *>(childList.front());
    }
    return nullptr;
}

const std::string &Action::name() const {
    FCITX_D();
    return d->name_;
}

void Action::update(InputContext *ic) { emit<Update>(ic); }

const KeyList &Action::hotkey() const {
    FCITX_D();
    return d->hotkey_;
}

void Action::setHotkey(const KeyList &hotkey) {
    FCITX_D();
    d->hotkey_ = hotkey;
}

class SimpleActionPrivate : public QPtrHolder<Action> {
public:
    SimpleActionPrivate(SimpleAction *q) : QPtrHolder(q) {}
    FCITX_DEFINE_SIGNAL_PRIVATE(SimpleAction, Activated);
    std::string longText_;
    std::string shortText_;
    std::string icon_;
    bool checked_ = false;
};

SimpleAction::SimpleAction()
    : d_ptr(std::make_unique<SimpleActionPrivate>(this)) {}

FCITX_DEFINE_DEFAULT_DTOR(SimpleAction);

void SimpleAction::setIcon(const std::string &icon) {
    FCITX_D();
    d->icon_ = icon;
}

void SimpleAction::setChecked(bool checked) {
    FCITX_D();
    d->checked_ = checked;
}

void SimpleAction::setShortText(const std::string &text) {
    FCITX_D();
    d->shortText_ = text;
}

void SimpleAction::setLongText(const std::string &text) {
    FCITX_D();
    d->longText_ = text;
}

std::string SimpleAction::icon(InputContext * /*unused*/) const {
    FCITX_D();
    return d->icon_;
}

bool SimpleAction::isChecked(InputContext * /*unused*/) const {
    FCITX_D();
    return d->checked_;
}

std::string SimpleAction::shortText(InputContext * /*unused*/) const {
    FCITX_D();
    return d->shortText_;
}

std::string SimpleAction::longText(InputContext * /*unused*/) const {
    FCITX_D();
    return d->longText_;
}

void SimpleAction::activate(InputContext *ic) {
    emit<SimpleAction::Activated>(ic);
}
} // namespace fcitx
