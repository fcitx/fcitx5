//
// Copyright (C) 2016~2016 by CSSlayer
// wengxt@gmail.com
//
// This library is free software; you can redistribute it and/or modify
// it under the terms of the GNU Lesser General Public License as
// published by the Free Software Foundation; either version 2.1 of the
// License, or (at your option) any later version.
//
// This library is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
// Lesser General Public License for more details.
//
// You should have received a copy of the GNU Lesser General Public
// License along with this library; see the file COPYING. If not,
// see <http://www.gnu.org/licenses/>.
//
#ifndef _FCITX_INPUTMETHODMANAGER_H_
#define _FCITX_INPUTMETHODMANAGER_H_

#include "fcitxcore_export.h"
#include <fcitx-utils/connectableobject.h>
#include <fcitx-utils/macros.h>
#include <fcitx/inputmethodgroup.h>
#include <functional>
#include <memory>
#include <string>
#include <vector>

namespace fcitx {

class AddonManager;
class InputMethodManagerPrivate;
class Instance;
class InputMethodEntry;

class FCITXCORE_EXPORT InputMethodManager : public ConnectableObject {
public:
    InputMethodManager(AddonManager *addonManager_);
    virtual ~InputMethodManager();

    void load();
    void save();

    std::vector<std::string> groups() const;
    int groupCount() const;
    void setCurrentGroup(const std::string &group);
    const InputMethodGroup &currentGroup() const;
    InputMethodGroup &currentGroup();
    const InputMethodGroup *group(const std::string &name) const;
    void setGroup(InputMethodGroup newGroup);
    void addEmptyGroup(const std::string &name);
    void removeGroup(const std::string &name);

    const InputMethodEntry *entry(const std::string &name) const;
    bool foreachEntries(
        const std::function<bool(const InputMethodEntry &entry)> callback);

    FCITX_DECLARE_SIGNAL(InputMethodManager, CurrentGroupAboutToBeChanged,
                         void(const std::string &group));
    FCITX_DECLARE_SIGNAL(InputMethodManager, CurrentGroupChanged,
                         void(const std::string &group));

private:
    void loadConfig();
    void buildDefaultGroup();
    void setGroupOrder(const std::vector<std::string> &groups);

    std::unique_ptr<InputMethodManagerPrivate> d_ptr;
    FCITX_DECLARE_PRIVATE(InputMethodManager);
};
} // namespace fcitx

#endif // _FCITX_INPUTMETHODMANAGER_H_
