/*
 * Copyright (C) 2016~2016 by CSSlayer
 * wengxt@gmail.com
 *
 * This library is free software; you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as
 * published by the Free Software Foundation; either version 2 of the
 * License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; see the file COPYING. If not,
 * see <http://www.gnu.org/licenses/>.
 */

#include "userinterfacemanager.h"
#include "action.h"
#include "userinterface.h"

namespace fcitx {

struct UserInterfaceComponentHash {
    template <typename T>
    std::underlying_type_t<T> operator()(T t) const {
        return static_cast<std::underlying_type_t<T>>(t);
    }
};

class UserInterfaceManagerPrivate {
public:
    UserInterfaceManagerPrivate() {
        UserInterfaceComponent comps[] = {UserInterfaceComponent::InputPanel, UserInterfaceComponent::StatusArea};
        for (auto comp : comps) {
            dirty_[comp] = false;
        }

        statusArea_.connect<StatusArea::Update>([]() {});
    }

    UserInterface *ui_ = nullptr;
    UserInterface *uiFallback_ = nullptr;
    StatusArea statusArea_;

    std::unordered_map<std::string, std::pair<Action *, ScopedConnection>> actions_;

    std::unordered_map<UserInterfaceComponent, bool, UserInterfaceComponentHash> dirty_;
};

UserInterfaceManager::UserInterfaceManager() : d_ptr(std::make_unique<UserInterfaceManagerPrivate>()) {}

UserInterfaceManager::~UserInterfaceManager() {}

void UserInterfaceManager::load(AddonManager *addonManager) {
    FCITX_D();
    auto names = addonManager->addonNames(AddonCategory::UI);

    // FIXME: implement fallback
    for (auto &name : names) {
        auto ui = addonManager->addon(name);
        if (ui) {
            d->ui_ = static_cast<UserInterface *>(ui);
            break;
        }
    }
}

StatusArea &UserInterfaceManager::statusArea() {
    FCITX_D();
    return d->statusArea_;
}

void UserInterfaceManager::update() {
    FCITX_D();
    for (auto &p : d->dirty_) {
        if (p.second) {
            d->ui_->update(p.first);
        }
    }
}

bool UserInterfaceManager::registerAction(const std::string &name, Action *action) {
    FCITX_D();
    if (!action->name().empty() || name.empty()) {
        return false;
    }
    auto iter = d->actions_.find(name);
    if (iter != d->actions_.end()) {
        return false;
    }
    ScopedConnection conn = action->connect<ObjectDestroyed>([this, action](void *) { unregisterAction(action); });
    d->actions_.emplace(name, std::make_pair(action, std::move(conn)));
    action->setName(name);
    return true;
}

void UserInterfaceManager::unregisterAction(Action *action) {
    FCITX_D();
    auto iter = d->actions_.find(action->name());
    if (iter == d->actions_.end()) {
        return;
    }
    if (std::get<0>(iter->second) != action) {
        return;
    }
    d->actions_.erase(iter);
    action->setName(std::string());
}

Action *UserInterfaceManager::lookupAction(const std::string &name) {
    FCITX_D();
    auto iter = d->actions_.find(name);
    if (iter == d->actions_.end()) {
        return nullptr;
    }
    return std::get<0>(iter->second);
}
}
