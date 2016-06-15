/*
 * Copyright (C) 2016~2016 by CSSlayer
 * wengxt@gmail.com
 *
 * This library is free software; you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as
 * published by the Free Software Foundation; either version 2 of the
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

#include <memory>
#include <fcitx-utils/macros.h>
#include <vector>
#include <string>

namespace fcitx {

class InputMethodGroupPrivate;
class InputMethodGroupItemPrivate;

class InputMethodGroupItem {
public:
    InputMethodGroupItem(const std::string &name);
    InputMethodGroupItem(InputMethodGroupItem &&other) noexcept;
    virtual ~InputMethodGroupItem();

    InputMethodGroupItem &setLayout(const std::string &layout);
    const std::string &name() const;
    const std::string &layout() const;

    std::unique_ptr<InputMethodGroupItemPrivate> d_ptr;
    FCITX_DECLARE_PRIVATE(InputMethodGroupItem);
};

class InputMethodGroup {
public:
    InputMethodGroup(const std::string &name);
    InputMethodGroup(InputMethodGroup &&other) noexcept;
    virtual ~InputMethodGroup();

    const std::string &name() const;
    void setDefaultLayout(const std::string &layout);
    const std::string &defaultLayout() const;
    std::vector<InputMethodGroupItem> &inputMethodList();
    const std::vector<InputMethodGroupItem> &inputMethodList() const;
    const std::string &defaultInputMethod() const;
    void setDefaultInputMethod(const std::string &im);

private:
    std::unique_ptr<InputMethodGroupPrivate> d_ptr;
    FCITX_DECLARE_PRIVATE(InputMethodGroup);
};
}

#endif // _FCITX_INPUTMETHODGROUP_H_
