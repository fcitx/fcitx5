/*
 * SPDX-FileCopyrightText: 2016-2016 CSSlayer <wengxt@gmail.com>
 *
 * SPDX-License-Identifier: LGPL-2.1-or-later
 *
 */
#ifndef _FCITX_ADDONFACTORY_H_
#define _FCITX_ADDONFACTORY_H_

#include <fcitx/addoninstance.h>
#include <fcitx/fcitxcore_export.h>

/// \addtogroup FcitxCore
/// \{
/// \file
/// \brief Addon Factory class

namespace fcitx {

class AddonManager;

/// Base class for addon factory.
class FCITXCORE_EXPORT AddonFactory {
public:
    virtual ~AddonFactory();
    /**
     * Create a addon instance for given addon manager.
     *
     * This function is called by AddonManager
     *
     * @return a created addon instance.
     *
     * @see AddonManager
     */
    virtual AddonInstance *create(AddonManager *manager) = 0;
};
} // namespace fcitx

#endif // _FCITX_ADDONFACTORY_H_
