/*
 * SPDX-FileCopyrightText: 2016-2016 CSSlayer <wengxt@gmail.com>
 *
 * SPDX-License-Identifier: LGPL-2.1-or-later
 *
 */
#ifndef _DUMMYADDON_PUBLIC_H_
#define _DUMMYADDON_PUBLIC_H_

#include "fcitx-utils/metastring.h"
#include "fcitx/addoninstance.h"

class Data {
public:
    Data() {}
    Data(const Data &) : copy_(true) {}

    bool isCopy() const { return copy_; }

private:
    const bool copy_ = false;
};

FCITX_ADDON_DECLARE_FUNCTION(DummyAddon, addOne, int(int));
FCITX_ADDON_DECLARE_FUNCTION(DummyAddon, testCopy, const Data &());

#endif // _DUMMYADDON_PUBLIC_H_
