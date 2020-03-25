//
// Copyright (C) 2020~2020 by CSSlayer
// wengxt@gmail.com
//
// This library is free software; you can redistribute it and/or modify
// it under the terms of the GNU Lesser General Public License as
// published by the Free Software Foundation; either version 2 of the
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
