/*
 * SPDX-FileCopyrightText: 2024-2024 CSSlayer <wengxt@gmail.com>
 *
 * SPDX-License-Identifier: LGPL-2.1-or-later
 *
 */
#include "candidateaction.h"
#include <memory>
#include <string>
#include "fcitx-utils/macros.h"

namespace fcitx {

class CandidateActionPrivate {
public:
    CandidateActionPrivate() = default;
    FCITX_INLINE_DEFINE_DEFAULT_DTOR_COPY_AND_MOVE_WITHOUT_SPEC(
        CandidateActionPrivate);

    int id_ = 0;
    std::string text_;
    bool isSeparator_ = false;
    std::string icon_;
    bool isCheckable_ = false;
    bool isChecked_ = false;
};

CandidateAction::CandidateAction()
    : d_ptr(std::make_unique<CandidateActionPrivate>()) {}

FCITX_DEFINE_DPTR_COPY_AND_DEFAULT_DTOR_AND_MOVE(CandidateAction);

FCITX_DEFINE_PROPERTY_PRIVATE(CandidateAction, int, id, setId);
FCITX_DEFINE_PROPERTY_PRIVATE(CandidateAction, std::string, text, setText);
FCITX_DEFINE_PROPERTY_PRIVATE(CandidateAction, bool, isSeparator, setSeparator);
FCITX_DEFINE_PROPERTY_PRIVATE(CandidateAction, std::string, icon, setIcon);
FCITX_DEFINE_PROPERTY_PRIVATE(CandidateAction, bool, isCheckable, setCheckable);
FCITX_DEFINE_PROPERTY_PRIVATE(CandidateAction, bool, isChecked, setChecked);

} // namespace fcitx
