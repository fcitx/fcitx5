/*
 * SPDX-FileCopyrightText: 2017-2017 CSSlayer <wengxt@gmail.com>
 *
 * SPDX-License-Identifier: LGPL-2.1-or-later
 *
 */
#ifndef _FCITX_INPUTPANEL_H_
#define _FCITX_INPUTPANEL_H_

#include <functional>
#include <memory>
#include <fcitx-utils/macros.h>
#include <fcitx/candidatelist.h>
#include <fcitx/fcitxcore_export.h>
#include <fcitx/text.h>

/// \addtogroup FcitxCore
/// \{
/// \file
/// \brief Class for input panel in UI.

namespace fcitx {

class InputPanelPrivate;
class InputContext;

using CustomInputPanelCallback = std::function<void(InputContext *)>;

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

    /**
     * An text message that allows the input method to display a message on top
     * of the input panel.
     *
     * When not empty, it should behave as if it is displayed in aux up.
     *
     * If you just need a naive implementation with timeout, consider using
     * Instance::showCustomInputMethodInformation.
     *
     * This is intended to be used by a third party to display some message when
     * engine/temp mode is controlling the input panel.
     *
     * This message should still be considered as temporary and
     * InputPanel::reset will still clear it when the input panel current owner
     * want to reset.
     *
     * @see Instance::showInputMethodInformation
     * @see Instance::showCustomInputMethodInformation
     */
    const Text &overlayMessage() const;

    /**
     * Set an overlay message on the input panel.
     *
     * When set, the input panel will not display any other fields.
     *
     * @param text the message to display.
     */
    void setOverlayMessage(Text text);

    /**
     * Return the current input panel display callback.
     *
     * @see setCustomInputPanelCallback
     * @return current custom ui callback
     * @since 5.0.24
     */
    const CustomInputPanelCallback &customInputPanelCallback() const;

    /**
     * Set a custom callback to display the input panel.
     *
     * When this is set,
     * Instance::updateUserInterface(UserInterfaceComponent::InputPanel) will
     * trigger a call to the callback function instead. The capability flag
     * ClientSideInputPanel will not be respected, but the clientPreedit will
     * still be sent if InputContext::updatePreedit is called.
     *
     * All the UI display batching logic still applies. The actual update that
     * triggers this callback will be called as a deferred event after current
     * event. If you need UI update right away (rare), you can still true as
     * immediate to Instance::updateUserInterface.
     *
     * @param callback callback to display input panel.
     *
     * @since 5.0.24
     */
    void setCustomInputPanelCallback(CustomInputPanelCallback callback);

    const CustomInputPanelCallback &customVirtualKeyboardCallback() const;

    void setCustomVirtualKeyboardCallback(CustomInputPanelCallback callback);

    void reset();

    /// Whether input panel is totally empty.
    bool empty() const;

private:
    std::unique_ptr<InputPanelPrivate> d_ptr;
    FCITX_DECLARE_PRIVATE(InputPanel);
};
} // namespace fcitx

#endif // _FCITX_INPUTPANEL_H_
