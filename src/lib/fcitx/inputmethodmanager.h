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
#ifndef _FCITX_INPUTMETHODMANAGER_H_
#define _FCITX_INPUTMETHODMANAGER_H_

#include <fcitx-utils/macros.h>
#include <fcitx/inputmethodgroup.h>
#include <memory>
#include <string>
#include <vector>

namespace fcitx {

class AddonManager;
class InputMethodManagerPrivate;
class Instance;
class InputMethodEntry;

class InputMethodManager {
public:
    InputMethodManager(AddonManager *addonManager_);
    virtual ~InputMethodManager();

    void load();

    void save();
    void setInstance(Instance *instance);

    const std::vector<InputMethodGroup> &groups();
    int groupCount() const;
    void setCurrentGroup(const std::string &group);
    const InputMethodGroup &currentGroup() const;
    void setGroupOrder(const std::vector<std::string> &groups);

    const InputMethodEntry *entry(const std::string &name) const;

private:
    void loadConfig();
    void buildDefaultGroup();

    std::unique_ptr<InputMethodManagerPrivate> d_ptr;
    FCITX_DECLARE_PRIVATE(InputMethodManager);
};
}

#endif // _FCITX_INPUTMETHODMANAGER_H_
