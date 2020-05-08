/*
 * SPDX-FileCopyrightText: 2016-2016 CSSlayer <wengxt@gmail.com>
 *
 * SPDX-License-Identifier: LGPL-2.1-or-later
 *
 */
#ifndef _FCITX_ADDONFACTORY_H_
#define _FCITX_ADDONFACTORY_H_

#include <string>
#include <fcitx/addoninstance.h>
#include "fcitxcore_export.h"

namespace fcitx {

class AddonManager;

class FCITXCORE_EXPORT AddonFactory {
public:
    virtual ~AddonFactory();
    virtual AddonInstance *create(AddonManager *manager) = 0;
};
} // namespace fcitx

#endif // _FCITX_ADDONFACTORY_H_
