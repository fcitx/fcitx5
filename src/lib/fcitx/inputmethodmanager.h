/*
 * SPDX-FileCopyrightText: 2016-2016 CSSlayer <wengxt@gmail.com>
 *
 * SPDX-License-Identifier: LGPL-2.1-or-later
 *
 */
#ifndef _FCITX_INPUTMETHODMANAGER_H_
#define _FCITX_INPUTMETHODMANAGER_H_

#include <functional>
#include <memory>
#include <string>
#include <vector>
#include <fcitx-utils/connectableobject.h>
#include <fcitx-utils/macros.h>
#include <fcitx/inputmethodgroup.h>
#include "fcitxcore_export.h"

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
    void load(const std::function<void(InputMethodGroup &)>
                  &buildDefaultGroupCallback);
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

    FCITX_DECLARE_SIGNAL(InputMethodManager, CurrentGroupAboutToChange,
                         void(const std::string &group));
    FCITX_DECLARE_SIGNAL(InputMethodManager, CurrentGroupChanged,
                         void(const std::string &group));

private:
    std::unique_ptr<InputMethodManagerPrivate> d_ptr;
    FCITX_DECLARE_PRIVATE(InputMethodManager);
};
} // namespace fcitx

#endif // _FCITX_INPUTMETHODMANAGER_H_
