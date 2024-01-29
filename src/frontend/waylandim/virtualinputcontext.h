/*
 * SPDX-FileCopyrightText: 2022~2022 CSSlayer <wengxt@gmail.com>
 *
 * SPDX-License-Identifier: LGPL-2.1-or-later
 *
 */
#ifndef _FCITX5_FRONTEND_WAYLANDIM_VIRTUALINPUTCONTEXT_H_
#define _FCITX5_FRONTEND_WAYLANDIM_VIRTUALINPUTCONTEXT_H_

#include <optional>
#include <fcitx/inputcontext.h>
#include "fcitx-utils/capabilityflags.h"
#include "fcitx-utils/signals.h"
#include "appmonitor.h"

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

private:
    void commitStringImpl(const std::string &text) override {
        return commitStringDelegate(this, text);
    }

    void deleteSurroundingTextImpl(int offset, unsigned int size) override {
        return deleteSurroundingTextDelegate(this, offset, size);
    }

    void forwardKeyImpl(const ForwardKeyEvent &key) override {
        return forwardKeyDelegate(this, key);
    }
    void updatePreeditImpl() override { return updatePreeditDelegate(this); }

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
