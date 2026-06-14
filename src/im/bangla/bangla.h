/*
 * SPDX-License-Identifier: LGPL-2.1-or-later
 */
#ifndef FCITX_BANGLA_BANGLA_H
#define FCITX_BANGLA_BANGLA_H

#include <fcitx/addonfactory.h>
#include <fcitx/addonmanager.h>
#include <fcitx/inputcontext.h>
#include <fcitx/inputmethodengine.h>
#include <fcitx/instance.h>
#include <fcitx/text.h>
#include <fcitx-utils/misc.h>

#include <string>
#include <unordered_map>

#include "avro_parser.h"

namespace fcitx {

class BanglaEngine : public InputMethodEngine {
public:
    explicit BanglaEngine(Instance *instance) { FCITX_UNUSED(instance); }

    void keyEvent(const InputMethodEntry &entry, KeyEvent &keyEvent) override;
    void activate(const InputMethodEntry &entry, InputContextEvent &event) override;
    void reset(const InputMethodEntry &entry, InputContextEvent &event) override;

private:
    static bool isProbhatEntry(const InputMethodEntry &entry);
    static bool isAvroInputChar(char ch);
    void handleProbhatKeyEvent(KeyEvent &keyEvent);
    void updatePreedit(InputContext *ic);
    void clearBuffer(InputContext *ic);
    void clearPreedit(InputContext *ic);
    std::string resolveCommit() const;
    void commitBuffer(InputContext *ic);
    void loadUserDict();

    AvroParser parser_;
    std::string buffer_;
    std::unordered_map<std::string, std::string> userDict_;
};

class BanglaFactory : public AddonFactory {
public:
    AddonInstance *create(AddonManager *manager) override {
        return new BanglaEngine(manager->instance());
    }
};

} // namespace fcitx

#endif
