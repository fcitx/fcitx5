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

/// \addtogroup FcitxCore
/// \{
/// \file
/// \brief Input Method Manager For fcitx.

namespace fcitx {

class AddonManager;
class InputMethodManagerPrivate;
class Instance;
class InputMethodEntry;

/**
 * Class to manage all the input method relation information.
 *
 * It will list all the available input methods from configuration file and
 * addon. The configuration file is located under $XDG_DATA/fcitx5/inputmethod.
 *
 * Additional runtime input method can be reported by input method addon by
 * listInputMethods.
 *
 * @see InputMethodEngine::listInputMethods
 */
class FCITXCORE_EXPORT InputMethodManager : public ConnectableObject {
public:
    InputMethodManager(AddonManager *addonManager_);
    virtual ~InputMethodManager();

    /**
     * Load the input method information from disk.
     *
     * If it does not exist, use the callback to create the default setup.
     */
    void load(const std::function<void(InputMethodManager &)>
                  &buildDefaultGroupCallback = {});

    /// Reset all the group information to initial state.
    void reset(const std::function<void(InputMethodManager &)>
                   &buildDefaultGroupCallback = {});

    /**
     * Load new input method configuration file from disk.
     *
     * It only load "new" input method configuration, and it would not update
     * the loaded data. Should only be used after load is called.
     */
    void refresh();

    /**
     * Save the input method information to disk.
     *
     * Commonly, the storage path will be ~/.config/fcitx5/profile.
     */
    void save();

    /// Return all the names of group by order.
    std::vector<std::string> groups() const;

    /// Return the number of groups.
    int groupCount() const;

    /// Set the name of current group, rest of the group order will be adjusted
    /// accordingly.
    void setCurrentGroup(const std::string &group);

    /// Return the current group.
    const InputMethodGroup &currentGroup() const;

    /**
     * Set default input method for current group.
     *
     * @see InputMethodGroup::setDefaultInputMethod
     */
    void setDefaultInputMethod(const std::string &name);

    /// Return the input methdo group of given name.
    const InputMethodGroup *group(const std::string &name) const;

    /**
     * Update the information of an existing group.
     *
     * The group info will be revalidated and filtered to the existing input
     * methods.
     */
    void setGroup(InputMethodGroup newGroupInfo);

    /// Create a new empty group with given name.
    void addEmptyGroup(const std::string &name);

    /// Remove an existing group by name.
    void removeGroup(const std::string &name);

    /**
     * Update the initial order of groups.
     *
     * This function should be only used in the buildDefaultGroupCallback.
     * Otherwise the group order can be only modified via setCurrentGroup.
     *
     * @param groups the order of groups.
     * @see InputMethodManager::load
     */
    void setGroupOrder(const std::vector<std::string> &groups);

    /// Return a given input method entry by name.
    const InputMethodEntry *entry(const std::string &name) const;

    /**
     * Enumerate all the input method entries.
     *
     * @return return true if the enumeration is done without interruption.
     */
    bool foreachEntries(
        const std::function<bool(const InputMethodEntry &entry)> &callback);

    /**
     * Emit the signal when current group is about to change.
     *
     * @see InputMethodManager::setCurrentGroup
     * @see InputMethodManager::removeGroup
     * @see InputMethodManager::setGroup
     * @see InputMethodManager::load
     * @see InputMethodManager::reset
     */
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
