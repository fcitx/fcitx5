/*
 * Copyright (C) 2016~2016 by CSSlayer
 * wengxt@gmail.com
 *
 * This library is free software; you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as
 * published by the Free Software Foundation; either version 2.1 of the
 * License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; see the file COPYING. If not,
 * see <http://www.gnu.org/licenses/>.
 */
#ifndef _FCITX_INPUTMETHODGROUP_H_
#define _FCITX_INPUTMETHODGROUP_H_

#include "fcitxcore_export.h"
#include <fcitx-utils/macros.h>
#include <list>
#include <memory>
#include <string>
#include <vector>

namespace fcitx {

class InputMethodGroupPrivate;
class InputMethodGroupItemPrivate;

class FCITXCORE_EXPORT InputMethodGroupItem {
public:
    InputMethodGroupItem(const std::string &name);
    FCITX_DECLARE_VIRTUAL_DTOR_COPY_AND_MOVE(InputMethodGroupItem);

    InputMethodGroupItem &setLayout(const std::string &layout);
    const std::string &name() const;
    const std::string &layout() const;

    std::unique_ptr<InputMethodGroupItemPrivate> d_ptr;
    FCITX_DECLARE_PRIVATE(InputMethodGroupItem);
};

class FCITXCORE_EXPORT InputMethodGroup {
public:
    explicit InputMethodGroup(const std::string &name);
    FCITX_DECLARE_VIRTUAL_DTOR_COPY_AND_MOVE(InputMethodGroup);

    const std::string &name() const;
    void setDefaultLayout(const std::string &layout);
    const std::string &defaultLayout() const;
    std::vector<InputMethodGroupItem> &inputMethodList();
    const std::vector<InputMethodGroupItem> &inputMethodList() const;
    const std::string &defaultInputMethod() const;
    void setDefaultInputMethod(const std::string &im);
    const std::string &layoutFor(const std::string &im);

private:
    std::unique_ptr<InputMethodGroupPrivate> d_ptr;
    FCITX_DECLARE_PRIVATE(InputMethodGroup);
};
}

#endif // _FCITX_INPUTMETHODGROUP_H_
