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

#include <algorithm>
#include "inputmethodgroup.h"

namespace fcitx {

class InputMethodGroupItemPrivate {
public:
    InputMethodGroupItemPrivate(const std::string &name_) : name(name_) {}

    std::string name;
    std::string layout;
};

class InputMethodGroupPrivate {
public:
    InputMethodGroupPrivate(const std::string &name_) : name(name_) {}

    std::string name;
    std::vector<InputMethodGroupItem> inputMethodList;
    std::string defaultInputMethod;
};

InputMethodGroupItem::InputMethodGroupItem(const std::string &name)
    : d_ptr(std::make_unique<InputMethodGroupItemPrivate>(name)) {}

InputMethodGroupItem::InputMethodGroupItem(InputMethodGroupItem &&other) noexcept : d_ptr(std::move(other.d_ptr)) {}

InputMethodGroupItem::~InputMethodGroupItem() {}

const std::string &InputMethodGroupItem::name() const {
    FCITX_D();
    return d->name;
}

const std::string &InputMethodGroupItem::layout() const {
    FCITX_D();
    return d->layout;
}

InputMethodGroupItem &InputMethodGroupItem::setLayout(const std::string &layout) {
    FCITX_D();
    d->layout = layout;
    return *this;
}

InputMethodGroup::InputMethodGroup(const std::string &name) : d_ptr(std::make_unique<InputMethodGroupPrivate>(name)) {}

InputMethodGroup::InputMethodGroup(InputMethodGroup &&other) noexcept : d_ptr(std::move(other.d_ptr)) {}

InputMethodGroup::~InputMethodGroup() {}

const std::string &InputMethodGroup::name() const {
    FCITX_D();
    return d->name;
}

std::vector<InputMethodGroupItem> &InputMethodGroup::inputMethodList() {
    FCITX_D();
    return d->inputMethodList;
}

const std::vector<InputMethodGroupItem> &InputMethodGroup::inputMethodList() const {
    FCITX_D();
    return d->inputMethodList;
}

void InputMethodGroup::setDefaultInputMethod(const std::string &im) {
    FCITX_D();
    if (std::any_of(d->inputMethodList.begin(), d->inputMethodList.end(),
                    [&im](const InputMethodGroupItem &item) { return item.name() == im; })) {
        d->defaultInputMethod = im;
    } else {
        if (d->inputMethodList.size() >= 2) {
            d->defaultInputMethod = d->inputMethodList[1].name();
        } else {
            d->defaultInputMethod = d->defaultInputMethod.empty() ? "" : d->inputMethodList[0].name();
        }
    }
}

const std::string &InputMethodGroup::defaultInputMethod() const {
    FCITX_D();
    return d->defaultInputMethod;
}

void InputMethodGroup::setDefaultLayout(const std::string &im) {
    FCITX_D();
    d->defaultInputMethod = im;
}

const std::string &InputMethodGroup::defaultLayout() const {
    FCITX_D();
    return d->defaultInputMethod;
}
}
