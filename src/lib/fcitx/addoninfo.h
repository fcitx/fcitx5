/*
 * SPDX-FileCopyrightText: 2015-2015 CSSlayer <wengxt@gmail.com>
 *
 * SPDX-License-Identifier: LGPL-2.1-or-later
 *
 */
#ifndef _FCITX_ADDON_H_
#define _FCITX_ADDON_H_

#include <memory>
#include <vector>
#include <fcitx-config/enum.h>
#include <fcitx-utils/i18nstring.h>
#include <fcitx-utils/macros.h>
#include "fcitxcore_export.h"

namespace fcitx {

class AddonInfoPrivate;

FCITX_CONFIG_ENUM(AddonCategory, InputMethod, Frontend, Loader, Module, UI)

enum class OverrideEnabled { NotSet, Enabled, Disabled };

class FCITXCORE_EXPORT AddonInfo {
public:
    AddonInfo(const std::string &name);
    virtual ~AddonInfo();

    bool isValid() const;
    const std::string &uniqueName() const;
    const I18NString &name() const;
    const I18NString &comment() const;
    const std::string &type() const;
    AddonCategory category() const;
    const std::string &library() const;
    const std::vector<std::string> &dependencies() const;
    const std::vector<std::string> &optionalDependencies() const;
    bool onDemand() const;
    int uiPriority() const;
    bool isEnabled() const;
    bool isDefaultEnabled() const;
    void setOverrideEnabled(OverrideEnabled overrideEnabled);
    bool isConfigurable() const;

    void load(const RawConfig &config);

private:
    std::unique_ptr<AddonInfoPrivate> d_ptr;
    FCITX_DECLARE_PRIVATE(AddonInfo);
};
} // namespace fcitx

#endif // _FCITX_ADDON_H_
