/*
 * SPDX-FileCopyrightText: 2020-2020 CSSlayer <wengxt@gmail.com>
 *
 * SPDX-License-Identifier: LGPL-2.1-or-later
 *
 */
#ifndef _TESTFRONTEND_TESTFRONTEND_H_
#define _TESTFRONTEND_TESTFRONTEND_H_

#include "fcitx/addoninstance.h"
#include "fcitx/instance.h"
#include "testfrontend_public.h"

namespace fcitx {

class TestFrontend : public AddonInstance {
public:
    TestFrontend(Instance *instance);
    ~TestFrontend();

    Instance *instance() { return instance_; }

private:
    ICUUID createInputContext(const std::string &program);
    void destroyInputContext(ICUUID uuid);
    void keyEvent(ICUUID uuid, const Key &key, bool isRelease);
    FCITX_ADDON_EXPORT_FUNCTION(TestFrontend, createInputContext);
    FCITX_ADDON_EXPORT_FUNCTION(TestFrontend, destroyInputContext);
    FCITX_ADDON_EXPORT_FUNCTION(TestFrontend, keyEvent);

    Instance *instance_;
};
} // namespace fcitx

#endif // _TESTFRONTEND_TESTFRONTEND_H_
