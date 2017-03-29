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
#include "inputcontext.h"
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
    UserInterfaceManagerPrivate() {}

    UserInterface *ui_ = nullptr;
    UserInterface *uiFallback_ = nullptr;

    std::unordered_map<std::string, std::pair<Action *, ScopedConnection>>
        actions_;

    typedef std::list<std::pair<
        InputContext *, std::unordered_set<UserInterfaceComponent, EnumHash>>>
        UIUpdateList;
    UIUpdateList updateList_;
    std::unordered_map<InputContext *, UIUpdateList::iterator> updateIndex_;
};

UserInterfaceManager::UserInterfaceManager()
    : d_ptr(std::make_unique<UserInterfaceManagerPrivate>()) {}

UserInterfaceManager::~UserInterfaceManager() {}

void UserInterfaceManager::load(AddonManager *addonManager,
                                const std::string &uiName) {
    FCITX_D();
    auto names = addonManager->addonNames(AddonCategory::UI);

    if (names.count(uiName)) {
        auto ui = addonManager->addon(uiName, true);
        if (ui) {
            d->ui_ = static_cast<UserInterface *>(ui);
        }
    }

    if (!d->ui_) {
        // FIXME: implement fallback
        for (auto &name : names) {
            auto ui = addonManager->addon(name, true);
            if (ui) {
                d->ui_ = static_cast<UserInterface *>(ui);
                break;
            }
        }
    }
}

bool UserInterfaceManager::registerAction(const std::string &name,
                                          Action *action) {
    FCITX_D();
    if (!action->name().empty() || name.empty()) {
        return false;
    }
    auto iter = d->actions_.find(name);
    if (iter != d->actions_.end()) {
        return false;
    }
    ScopedConnection conn = action->connect<ObjectDestroyed>(
        [this, action](void *) { unregisterAction(action); });
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

void fcitx::UserInterfaceManager::update(
    fcitx::UserInterfaceComponent component,
    fcitx::InputContext *inputContext) {
    FCITX_D();
    auto iter = d->updateIndex_.find(inputContext);
    decltype(d->updateList_)::iterator listIter;
    if (d->updateIndex_.end() == iter) {
        d->updateList_.emplace_back(std::piecewise_construct,
                                    std::forward_as_tuple(inputContext),
                                    std::forward_as_tuple());
        d->updateIndex_[inputContext] = listIter =
            std::prev(d->updateList_.end());
    } else {
        listIter = iter->second;
    }
    listIter->second.insert(component);
}

void fcitx::UserInterfaceManager::expire(fcitx::InputContext *inputContext) {
    FCITX_D();
    auto iter = d->updateIndex_.find(inputContext);
    if (d->updateIndex_.end() != iter) {
        d->updateList_.erase(iter->second);
        d->updateIndex_.erase(iter);
    }
}

void fcitx::UserInterfaceManager::flush() {
    FCITX_D();
    if (!d->ui_) {
        return;
    }
    for (auto &p : d->updateList_) {
        for (auto comp : p.second) {
            d->ui_->update(comp, p.first);
        }
    }
    d->updateIndex_.clear();
    d->updateList_.clear();
}
