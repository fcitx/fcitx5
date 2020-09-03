/*
 * SPDX-FileCopyrightText: 2016-2016 CSSlayer <wengxt@gmail.com>
 *
 * SPDX-License-Identifier: LGPL-2.1-or-later
 *
 */
#ifndef _FCITX_INPUTMETHODGROUP_H_
#define _FCITX_INPUTMETHODGROUP_H_

#include <list>
#include <memory>
#include <string>
#include <vector>
#include <fcitx-utils/log.h>
#include <fcitx-utils/macros.h>
#include "fcitxcore_export.h"

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

LogMessageBuilder &operator<<(LogMessageBuilder &builder,
                              const InputMethodGroupItem &groupItem);

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
    const std::string &layoutFor(const std::string &im) const;

private:
    std::unique_ptr<InputMethodGroupPrivate> d_ptr;
    FCITX_DECLARE_PRIVATE(InputMethodGroup);
};
} // namespace fcitx

#endif // _FCITX_INPUTMETHODGROUP_H_
