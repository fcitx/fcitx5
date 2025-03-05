/*
 * SPDX-FileCopyrightText: 2022~2022 CSSlayer <wengxt@gmail.com>
 *
 * SPDX-License-Identifier: LGPL-2.1-or-later
 *
 */
#ifndef _FCITX5_FRONTEND_WAYLANDIM_VIRTUALINPUTCONTEXT_H_
#define _FCITX5_FRONTEND_WAYLANDIM_VIRTUALINPUTCONTEXT_H_

#include <memory>
#include <optional>
#include <string>
#include <unordered_map>
#include "fcitx-utils/capabilityflags.h"
#include "fcitx-utils/signals.h"
#include "fcitx/event.h"
#include "fcitx/inputcontext.h"
#include "appmonitor.h"

#ifdef __linux__
#include <linux/input-event-codes.h>
#elif __FreeBSD__
#include <dev/evdev/input-event-codes.h>
#else
#define KEY_LEFTCTRL    29
#define KEY_LEFTSHIFT   42
#define KEY_RIGHTSHIFT  54
#define KEY_LEFTALT     56
#define KEY_CAPSLOCK    58
#define KEY_NUMLOCK     69
#define KEY_RIGHTCTRL   97
#define KEY_RIGHTALT    100
#define KEY_LEFTMETA    125
#define KEY_RIGHTMETA   126
#endif

namespace fcitx {

class VirtualInputContextManager;

class VirtualInputContextGlue : public InputContext {
public:
    using InputContext::InputContext;
    // Qualifier is const to ensure the state is read from ic.
    virtual void commitStringDelegate(const InputContext *ic,
                                      const std::string &text) const = 0;
    virtual void deleteSurroundingTextDelegate(InputContext *ic, int offset,
                                               unsigned int size) const = 0;
    virtual void forwardKeyDelegate(InputContext *ic,
                                    const ForwardKeyEvent &key) const = 0;
    virtual void updatePreeditDelegate(InputContext *ic) const = 0;

    bool realFocus() const {
        if (virtualICManager_) {
            return realFocus_;
        }
        return hasFocus();
    }
    void setRealFocus(bool focus) { realFocus_ = focus; }

    void setVirtualInputContextManager(VirtualInputContextManager *manager) {
        virtualICManager_ = manager;
    }

    InputContext *delegatedInputContext();

    void focusInWrapper();
    void focusOutWrapper();
    void updateSurroundingTextWrapper();
    void setCapabilityFlagsWrapper(CapabilityFlags flags);

    static bool isModifier(const int keycode) {
        int code = keycode - 8;
        return code == KEY_LEFTCTRL ||
            code == KEY_LEFTSHIFT ||
            code == KEY_RIGHTSHIFT ||
            code == KEY_LEFTALT ||
            code == KEY_CAPSLOCK ||
            code == KEY_NUMLOCK ||
            code == KEY_RIGHTCTRL ||
            code == KEY_RIGHTALT ||
            code == KEY_LEFTMETA ||
            code == KEY_RIGHTMETA;
    }

private:
    void commitStringImpl(const std::string &text) override {
        commitStringDelegate(this, text);
    }

    void deleteSurroundingTextImpl(int offset, unsigned int size) override {
        deleteSurroundingTextDelegate(this, offset, size);
    }

    void forwardKeyImpl(const ForwardKeyEvent &key) override {
        forwardKeyDelegate(this, key);
    }
    void updatePreeditImpl() override { updatePreeditDelegate(this); }

    bool realFocus_ = false;
    VirtualInputContextManager *virtualICManager_ = nullptr;
};

class VirtualInputContext : public InputContext {
public:
    VirtualInputContext(InputContextManager &manager,
                        const std::string &program,
                        VirtualInputContextGlue *parent)
        : InputContext(manager, program), parent_(parent) {
        created();

        setFocusGroup(parent->focusGroup());
        setCapabilityFlags(parent->capabilityFlags());
    }

    ~VirtualInputContext() override { destroy(); }

    const char *frontend() const override { return parent_->frontend(); }
    InputContext *parent() const { return parent_; }

protected:
    void commitStringImpl(const std::string &text) override {
        parent_->commitStringDelegate(this, text);
    }
    void deleteSurroundingTextImpl(int offset, unsigned int size) override {
        parent_->deleteSurroundingTextDelegate(this, offset, size);
    }

    void forwardKeyImpl(const ForwardKeyEvent &key) override {
        parent_->forwardKeyDelegate(this, key);
    }

    void updatePreeditImpl() override { parent_->updatePreeditDelegate(this); }

private:
    VirtualInputContextGlue *parent_;
};

class VirtualInputContextManager {

public:
    VirtualInputContextManager(InputContextManager *manager,
                               VirtualInputContextGlue *parent,
                               AppMonitor *app);
    ~VirtualInputContextManager();

    void setRealFocus(bool focus);

    InputContext *focusedVirtualIC();

private:
    void
    appUpdated(const std::unordered_map<std::string, std::string> &appState,
               std::optional<std::string> focus);

    void updateFocus();

    ScopedConnection conn_;
    InputContextManager *manager_;
    VirtualInputContextGlue *parentIC_;
    AppMonitor *app_;
    std::unordered_map<std::string, std::string> lastAppState_;
    std::unordered_map<std::string, std::unique_ptr<InputContext>> managed_;
    std::optional<std::string> focus_;
};

} // namespace fcitx

#endif // _FCITX5_FRONTEND_WAYLANDIM_VIRTUALINPUTCONTEXT_H_
