/*
 * SPDX-FileCopyrightText: 2026-2026 CSSlayer <wengxt@gmail.com>
 *
 * SPDX-License-Identifier: LGPL-2.1-or-later
 *
 */

#ifndef _FCITX_MODULES_QUICKPHRASE_QUICKPHRASETEMPMODE_H_
#define _FCITX_MODULES_QUICKPHRASE_QUICKPHRASETEMPMODE_H_

#include <string>
#include <string_view>
#include "fcitx-utils/inputbuffer.h"
#include "fcitx-utils/key.h"
#include "fcitx/event.h"
#include "fcitx/inputcontext.h"
#include "fcitx/tempmode.h"
#include "quickphrase_public.h"

namespace fcitx {

class QuickPhrase;

class QuickPhraseState {
public:
    QuickPhraseState();

    void reset(InputContext *inputContext);

    InputBuffer buffer_;
    std::string originalBuffer_;
    QuickPhraseRestoreCallback restoreCallback_;
    bool typed_ = false;
    std::string text_;
    std::string prefix_;
    std::string str_;
    std::string alt_;
    Key key_;
};

class QuickPhraseTempMode : public SimpleTempMode<QuickPhraseState> {
public:
    using Base = SimpleTempMode<QuickPhraseState>;

    explicit QuickPhraseTempMode(QuickPhrase *quickPhrase);

    bool triggerTempMode(const KeyEvent &keyEvent) override;
    bool keyEvent(const KeyEvent &keyEvent) override;
    bool invokeAction(InvokeActionEvent &event) override;
    void reset(InputContext *inputContext) override;
    std::string_view name() const override;

    void trigger(InputContext *inputContext, const std::string &text,
                 const std::string &prefix, const std::string &str,
                 const std::string &alt, const Key &key);
    void setBuffer(InputContext *inputContext, const std::string &text);
    void setBufferWithRestoreCallback(InputContext *inputContext,
                                      const std::string &text,
                                      const std::string &original,
                                      QuickPhraseRestoreCallback callback);
    void updateUI(InputContext *inputContext);

private:
    void setSelectionKeys(QuickPhraseAction action);

    QuickPhrase *quickPhrase_;
    KeyList selectionKeys_;
    KeyStates selectionModifier_;
};

} // namespace fcitx

#endif // _FCITX_MODULES_QUICKPHRASE_QUICKPHRASETEMPMODE_H_
