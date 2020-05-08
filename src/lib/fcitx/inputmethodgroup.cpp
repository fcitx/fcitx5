/*
 * SPDX-FileCopyrightText: 2016-2016 CSSlayer <wengxt@gmail.com>
 *
 * SPDX-License-Identifier: LGPL-2.1-or-later
 *
 */

#include "inputmethodgroup.h"
#include <algorithm>

namespace fcitx {

namespace {
static const std::string emptyString;
}

class InputMethodGroupItemPrivate {
public:
    InputMethodGroupItemPrivate(const std::string &name) : name_(name) {}
    FCITX_INLINE_DEFINE_DEFAULT_DTOR_COPY_AND_MOVE_WITHOUT_SPEC(
        InputMethodGroupItemPrivate);

    std::string name_;
    std::string layout_;
};

class InputMethodGroupPrivate {
public:
    InputMethodGroupPrivate(const std::string &name) : name_(name) {}

    std::string name_;
    std::vector<InputMethodGroupItem> inputMethodList_;
    std::string defaultInputMethod_;
    std::string defaultLayout_;
};

InputMethodGroupItem::InputMethodGroupItem(const std::string &name)
    : d_ptr(std::make_unique<InputMethodGroupItemPrivate>(name)) {}

FCITX_DEFINE_DPTR_COPY_AND_DEFAULT_DTOR_AND_MOVE(InputMethodGroupItem);

const std::string &InputMethodGroupItem::name() const {
    FCITX_D();
    return d->name_;
}

const std::string &InputMethodGroupItem::layout() const {
    FCITX_D();
    return d->layout_;
}

InputMethodGroupItem &
InputMethodGroupItem::setLayout(const std::string &layout) {
    FCITX_D();
    d->layout_ = layout;
    return *this;
}

InputMethodGroup::InputMethodGroup(const std::string &name)
    : d_ptr(std::make_unique<InputMethodGroupPrivate>(name)) {}

FCITX_DEFINE_DPTR_COPY_AND_DEFAULT_DTOR_AND_MOVE(InputMethodGroup);

const std::string &InputMethodGroup::name() const {
    FCITX_D();
    return d->name_;
}

std::vector<InputMethodGroupItem> &InputMethodGroup::inputMethodList() {
    FCITX_D();
    return d->inputMethodList_;
}

const std::vector<InputMethodGroupItem> &
InputMethodGroup::inputMethodList() const {
    FCITX_D();
    return d->inputMethodList_;
}

void InputMethodGroup::setDefaultInputMethod(const std::string &im) {
    FCITX_D();
    if (std::any_of(d->inputMethodList_.begin(), d->inputMethodList_.end(),
                    [&im](const InputMethodGroupItem &item) {
                        return item.name() == im;
                    })) {
        if (d->inputMethodList_.size() > 1 &&
            d->inputMethodList_[0].name() == im) {
            d->defaultInputMethod_ = d->inputMethodList_[1].name();
        } else {
            d->defaultInputMethod_ = im;
        }
    } else {
        if (d->inputMethodList_.size() > 1) {
            d->defaultInputMethod_ = d->inputMethodList_[1].name();
        } else {
            d->defaultInputMethod_ = d->inputMethodList_.empty()
                                         ? ""
                                         : d->inputMethodList_[0].name();
        }
    }
}

const std::string &InputMethodGroup::layoutFor(const std::string &im) {
    FCITX_D();
    auto iter = std::find_if(
        d->inputMethodList_.begin(), d->inputMethodList_.end(),
        [&im](const InputMethodGroupItem &item) { return item.name() == im; });
    if (iter != d->inputMethodList_.end()) {
        return iter->layout();
    }
    return emptyString;
}

const std::string &InputMethodGroup::defaultInputMethod() const {
    FCITX_D();
    return d->defaultInputMethod_;
}

void InputMethodGroup::setDefaultLayout(const std::string &im) {
    FCITX_D();
    d->defaultLayout_ = im;
}

const std::string &InputMethodGroup::defaultLayout() const {
    FCITX_D();
    return d->defaultLayout_;
}
} // namespace fcitx
