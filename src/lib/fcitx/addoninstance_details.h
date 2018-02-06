//
// Copyright (C) 2017~2017 by CSSlayer
// wengxt@gmail.com
//
// This library is free software; you can redistribute it and/or modify
// it under the terms of the GNU Lesser General Public License as
// published by the Free Software Foundation; either version 2.1 of the
// License, or (at your option) any later version.
//
// This library is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
// Lesser General Public License for more details.
//
// You should have received a copy of the GNU Lesser General Public
// License along with this library; see the file COPYING. If not,
// see <http://www.gnu.org/licenses/>.
//
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

template <typename Class, typename Ret, typename... Args>
class AddonFunctionAdaptor : public AddonFunctionAdaptorErasure<Ret(Args...)> {
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
AddonFunctionAdaptor<Class, Ret, Args...>
    MakeAddonFunctionAdaptor(Ret (Class::*pCallback)(Args...));

} // namespace fcitx

#endif // _FCITX_ADDONINSTANCE_DETAILS_H_
