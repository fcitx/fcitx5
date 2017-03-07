/*
 * Copyright (C) 2017~2017 by CSSlayer
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
#ifndef _FCITX_INPUTPANEL_H_
#define _FCITX_INPUTPANEL_H_

#include "fcitxcore_export.h"
#include <fcitx/candidatelist.h>
#include <fcitx/element.h>
#include <fcitx/text.h>

namespace fcitx {

class InputPanelPrivate;
class InputContext;

class FCITXCORE_EXPORT InputPanel {
public:
    InputPanel(InputContext *ic);
    virtual ~InputPanel();

    const Text &preedit();
    void setPreedit(const Text &text);

    const Text &auxUp();
    void setAuxUp(const Text &text);

    const Text &auxDown();
    void setAuxDown(const Text &text);

    const Text &clientPreedit();
    void setClientPreedit(const Text &clientPreedit);

    CandidateList *candidateList();
    void setCandidateList(CandidateList *candidate);

    void reset();

private:
    std::unique_ptr<InputPanelPrivate> d_ptr;
    FCITX_DECLARE_PRIVATE(InputPanel);
};
}

#endif // _FCITX_INPUTPANEL_H_
