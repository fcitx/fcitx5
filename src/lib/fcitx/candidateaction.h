/*
 * SPDX-FileCopyrightText: 2024-2024 CSSlayer <wengxt@gmail.com>
 *
 * SPDX-License-Identifier: LGPL-2.1-or-later
 *
 */
#ifndef _FCITX_CANDIDATEACTION_H_
#define _FCITX_CANDIDATEACTION_H_

#include <memory>
#include <string>
#include <fcitx-utils/macros.h>
#include <fcitx/fcitxcore_export.h>

namespace fcitx {

class CandidateActionPrivate;

class FCITXCORE_EXPORT CandidateAction {
public:
    CandidateAction();
    virtual ~CandidateAction();
    FCITX_DECLARE_COPY_AND_MOVE(CandidateAction);

    FCITX_DECLARE_PROPERTY(int, id, setId);
    FCITX_DECLARE_PROPERTY(std::string, text, setText);
    FCITX_DECLARE_PROPERTY(bool, isSeparator, setSeparator);
    FCITX_DECLARE_PROPERTY(std::string, icon, setIcon);
    FCITX_DECLARE_PROPERTY(bool, isCheckable, setCheckable);
    FCITX_DECLARE_PROPERTY(bool, isChecked, setChecked);

private:
    FCITX_DECLARE_PRIVATE(CandidateAction);
    std::unique_ptr<CandidateActionPrivate> d_ptr;
};

} // namespace fcitx

#endif // _FCITX_CANDIDATEACTION_H_
