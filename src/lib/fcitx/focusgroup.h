/*
 * SPDX-FileCopyrightText: 2016-2016 CSSlayer <wengxt@gmail.com>
 *
 * SPDX-License-Identifier: LGPL-2.1-or-later
 *
 */
#ifndef _FCITX_FOCUSGROUP_H_
#define _FCITX_FOCUSGROUP_H_

#include <cstddef>
#include <memory>
#include <string>
#include <fcitx-utils/macros.h>
#include <fcitx/fcitxcore_export.h>
#include <fcitx/inputcontext.h>
#include <fcitx/inputpanel.h>

namespace fcitx {

class InputContextManager;
class FocusGroupPrivate;
class InputContext;

class FCITXCORE_EXPORT FocusGroup {
    friend class InputContextManagerPrivate;
    friend class InputContext;

public:
    FocusGroup(const std::string &display, InputContextManager &manager);
    FocusGroup(const FocusGroup &) = delete;
    virtual ~FocusGroup();

    void setFocusedInputContext(InputContext *ic);
    InputContext *focusedInputContext() const;
    bool foreach(const InputContextVisitor &visitor);

    const std::string &display() const;
    size_t size() const;

protected:
    void addInputContext(InputContext *ic);
    void removeInputContext(InputContext *ic);

private:
    std::unique_ptr<FocusGroupPrivate> d_ptr;
    FCITX_DECLARE_PRIVATE(FocusGroup);
};
} // namespace fcitx

#endif // _FCITX_FOCUSGROUP_H_
