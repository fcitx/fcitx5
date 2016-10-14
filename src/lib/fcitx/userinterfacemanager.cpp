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

namespace fcitx {

class UserInterfaceManagerPrivate {
public:
    UserInterfaceManagerPrivate(AddonManager *manager) : addonManager_(manager) {}

    AddonManager *addonManager_;
};

UserInterfaceManager::UserInterfaceManager(AddonManager *manager)
    : d_ptr(std::make_unique<UserInterfaceManagerPrivate>(manager)) {}

UserInterfaceManager::~UserInterfaceManager() {}

void UserInterfaceManager::init() {
    FCITX_D();
    auto names = d->addonManager_->addonNames(AddonCategory::UI);
    for (auto &name : names) {
    }
}
}
