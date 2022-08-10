/*
 * SPDX-FileCopyrightText: 2022-2022 CSSlayer <wengxt@gmail.com>
 *
 * SPDX-License-Identifier: LGPL-2.1-or-later
 *
 */
#ifndef _FCITX_ADDONINSTANCE_P_H_
#define _FCITX_ADDONINSTANCE_P_H_

#include <string>
#include <unordered_map>
#include "addoninfo.h"
#include "addoninstance.h"

namespace fcitx {

class AddonInstancePrivate {
public:
    std::unordered_map<std::string, AddonFunctionAdaptorBase *> callbackMap_;
    const AddonInfo *addonInfo_ = nullptr;
};

} // namespace fcitx

#endif // _FCITX_ADDONINSTANCE_P_H_
