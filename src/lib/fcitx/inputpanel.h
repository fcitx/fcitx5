/*
 * SPDX-FileCopyrightText: 2017-2017 CSSlayer <wengxt@gmail.com>
 *
 * SPDX-License-Identifier: LGPL-2.1-or-later
 *
 */
#ifndef _FCITX_INPUTPANEL_H_
#define _FCITX_INPUTPANEL_H_

#include <fcitx-utils/element.h>
#include <fcitx/candidatelist.h>
#include <fcitx/text.h>
#include "fcitxcore_export.h"

namespace fcitx {

class InputPanelPrivate;
class InputContext;

class FCITXCORE_EXPORT InputPanel {
public:
    InputPanel(InputContext *ic);
    virtual ~InputPanel();

    const Text &preedit() const;
    void setPreedit(const Text &text);

    const Text &auxUp() const;
    void setAuxUp(const Text &text);

    const Text &auxDown() const;
    void setAuxDown(const Text &text);

    const Text &clientPreedit() const;
    void setClientPreedit(const Text &clientPreedit);

    std::shared_ptr<CandidateList> candidateList() const;
    void setCandidateList(std::unique_ptr<CandidateList> candidate);

    void reset();
    bool empty() const;

private:
    std::unique_ptr<InputPanelPrivate> d_ptr;
    FCITX_DECLARE_PRIVATE(InputPanel);
};
} // namespace fcitx

#endif // _FCITX_INPUTPANEL_H_
