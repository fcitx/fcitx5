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

/// \addtogroup FcitxCore
/// \{
/// \file
/// \brief Class for input panel in UI.

namespace fcitx {

class InputPanelPrivate;
class InputContext;

/**
 * Input Panel is usually a floating window that is display at the cursor of
 * input.
 *
 * But it can also be a embedded fixed window. The actual representation is
 * implementation-defined. In certain cases, all the information input panel is
 * forwarded to client and will be drawn by client.
 *
 * A common input panel is shown as
 *
 * | Aux Up   | Preedit          |
 * |----------|------------------|
 * | Aux down | Candidate 1, 2.. |
 * Or
 * | Aux Up | Preedit            |
 * |--------|--------------------|
 * | Aux down                    ||
 * | Candidate 1                 ||
 * | Candidate 2                 ||
 * | ...                         ||
 * | Candidate n                 ||
 */
class FCITXCORE_EXPORT InputPanel {
public:
    /// Construct a Input Panel associated with given input context.
    InputPanel(InputContext *ic);
    virtual ~InputPanel();

    const Text &preedit() const;
    void setPreedit(const Text &text);

    const Text &auxUp() const;
    void setAuxUp(const Text &text);

    const Text &auxDown() const;
    void setAuxDown(const Text &text);

    /// The preedit text embedded in client window.
    const Text &clientPreedit() const;
    void setClientPreedit(const Text &clientPreedit);

    std::shared_ptr<CandidateList> candidateList() const;
    void setCandidateList(std::unique_ptr<CandidateList> candidate);

    void reset();

    /// Whether input panel is totally empty.
    bool empty() const;

private:
    std::unique_ptr<InputPanelPrivate> d_ptr;
    FCITX_DECLARE_PRIVATE(InputPanel);
};
} // namespace fcitx

#endif // _FCITX_INPUTPANEL_H_
