/*
 * SPDX-FileCopyrightText: 2016-2016 CSSlayer <wengxt@gmail.com>
 *
 * SPDX-License-Identifier: LGPL-2.1-or-later
 *
 */

#include "action.h"
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
    auto oldMenu = this->menu();
    if (oldMenu) {
        oldMenu->removeParent(this);
    }
    if (menu) {
        menu->addParent(this);
    }
}

Menu *Action::menu() {
    auto childList = childs();
    if (childList.size()) {
        return static_cast<Menu *>(childList.front());
    }
    return nullptr;
}

const std::string &Action::name() const {
    FCITX_D();
    return d->name_;
}

void Action::update(InputContext *ic) { emit<Update>(ic); }

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
    : Action(), d_ptr(std::make_unique<SimpleActionPrivate>(this)) {}

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

std::string SimpleAction::icon(InputContext *) const {
    FCITX_D();
    return d->icon_;
}

bool SimpleAction::isChecked(InputContext *) const {
    FCITX_D();
    return d->checked_;
}

std::string SimpleAction::shortText(InputContext *) const {
    FCITX_D();
    return d->shortText_;
}

std::string SimpleAction::longText(InputContext *) const {
    FCITX_D();
    return d->longText_;
}

void SimpleAction::activate(InputContext *ic) {
    emit<SimpleAction::Activated>(ic);
}
} // namespace fcitx
