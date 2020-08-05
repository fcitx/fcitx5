/*
 * SPDX-FileCopyrightText: 2017-2017 CSSlayer <wengxt@gmail.com>
 *
 * SPDX-License-Identifier: LGPL-2.1-or-later
 *
 */
#ifndef _FCITX_ADDONINSTANCE_DETAILS_H_
#define _FCITX_ADDONINSTANCE_DETAILS_H_

namespace fcitx {

class AddonInstance;
class AddonInstancePrivate;
class AddonFunctionAdaptorBase {
public:
    virtual ~AddonFunctionAdaptorBase() = default;
};

template <typename Sig>
class AddonFunctionAdaptorErasure;

template <typename Ret, typename... Args>
class AddonFunctionAdaptorErasure<Ret(Args...)>
    : public AddonFunctionAdaptorBase {
public:
    virtual Ret callback(Args... args) = 0;
};

template <typename T>
struct AddonFunctionSignature;

template <typename Signature>
using AddonFunctionSignatureType =
    typename AddonFunctionSignature<Signature>::type;

template <typename CallbackType>
class AddonFunctionAdaptor;

template <typename Class, typename Ret, typename... Args>
class AddonFunctionAdaptor<Ret (Class::*)(Args...)>
    : public AddonFunctionAdaptorErasure<Ret(Args...)> {
public:
    typedef Ret (Class::*CallbackType)(Args...);
    typedef Ret Signature(Args...);

    AddonFunctionAdaptor(const std::string &name, Class *addon,
                         CallbackType pCallback)
        : AddonFunctionAdaptorErasure<Ret(Args...)>(), addon_(addon),
          pCallback_(pCallback) {
        addon->registerCallback(name, this);
    }

    Ret callback(Args... args) override {
        return (addon_->*pCallback_)(args...);
    }

private:
    Class *addon_;
    CallbackType pCallback_;
};

template <typename Class, typename Ret, typename... Args>
class AddonFunctionAdaptor<Ret (Class::*)(Args...) const>
    : public AddonFunctionAdaptorErasure<Ret(Args...)> {
public:
    typedef Ret (Class::*CallbackType)(Args...) const;
    typedef Ret Signature(Args...);

    AddonFunctionAdaptor(const std::string &name, Class *addon,
                         CallbackType pCallback)
        : AddonFunctionAdaptorErasure<Ret(Args...)>(), addon_(addon),
          pCallback_(pCallback) {
        addon->registerCallback(name, this);
    }

    Ret callback(Args... args) override {
        return (addon_->*pCallback_)(args...);
    }

private:
    Class *addon_;
    CallbackType pCallback_;
};

template <typename CallbackType>
AddonFunctionAdaptor<CallbackType>
MakeAddonFunctionAdaptor(CallbackType pCallback);

} // namespace fcitx

#endif // _FCITX_ADDONINSTANCE_DETAILS_H_
