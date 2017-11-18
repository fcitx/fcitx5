/*
 * Copyright (C) 2016~2016 by CSSlayer
 * wengxt@gmail.com
 *
 * This library is free software; you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as
 * published by the Free Software Foundation; either version 2.1 of the
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
#ifndef _FCITX_ADDONINSTANCE_H_
#define _FCITX_ADDONINSTANCE_H_

#include "fcitxcore_export.h"
#include <fcitx-config/configuration.h>
#include <fcitx-utils/library.h>
#include <fcitx-utils/metastring.h>
#include <functional>
#include <memory>
#include <type_traits>
#include <unordered_map>

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

class FCITXCORE_EXPORT AddonInstance {
public:
    AddonInstance();
    virtual ~AddonInstance();
    virtual void reloadConfig() {}
    virtual void save() {}
    virtual const Configuration *getConfig() const { return nullptr; }
    virtual void setConfig(const RawConfig &) {}

    template <typename Signature, typename... Args>
    typename std::function<Signature>::result_type
    callWithSignature(const std::string &name, Args &&... args) {
        auto adaptor = findCall(name);
        auto erasureAdaptor =
            static_cast<AddonFunctionAdaptorErasure<Signature> *>(adaptor);
        return erasureAdaptor->callback(std::forward<Args>(args)...);
    }
    template <typename MetaSignatureString, typename... Args>
    auto callWithMetaString(Args &&... args) {
        return callWithSignature<
            AddonFunctionSignatureType<MetaSignatureString>>(
            MetaSignatureString::data(), std::forward<Args>(args)...);
    }
    template <typename MetaType, typename... Args>
    auto call(Args &&... args) {
        return callWithSignature<typename MetaType::Signature>(
            MetaType::Name::data(), std::forward<Args>(args)...);
    }

    void registerCallback(const std::string &name,
                          AddonFunctionAdaptorBase *adaptor);

private:
    AddonFunctionAdaptorBase *findCall(const std::string &name);
    std::unique_ptr<AddonInstancePrivate> d_ptr;
    FCITX_DECLARE_PRIVATE(AddonInstance);
};

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
}

#define FCITX_ADDON_DECLARE_FUNCTION(NAME, FUNCTION, SIGNATURE...)             \
    namespace fcitx {                                                          \
    template <>                                                                \
    struct AddonFunctionSignature<fcitxMakeMetaString(#NAME "::" #FUNCTION)> { \
        typedef std::remove_reference_t<decltype(std::declval<SIGNATURE>())>   \
            type;                                                              \
    };                                                                         \
    namespace I##NAME {                                                        \
        struct FUNCTION {                                                      \
            typedef fcitxMakeMetaString(#NAME "::" #FUNCTION) Name;            \
            using Signature = AddonFunctionSignatureType<Name>;                \
        };                                                                     \
    }                                                                          \
    }

#define FCITX_ADDON_EXPORT_FUNCTION(CLASS, FUNCTION)                           \
    decltype(::fcitx::MakeAddonFunctionAdaptor(                                \
        &CLASS::FUNCTION)) FUNCTION##Adaptor{#CLASS "::" #FUNCTION, this,      \
                                             &CLASS::FUNCTION};                \
    static_assert(                                                             \
        std::is_same<decltype(::fcitx::MakeAddonFunctionAdaptor(               \
                         &CLASS::FUNCTION))::Signature,                        \
                     ::fcitx::AddonFunctionSignatureType<fcitxMakeMetaString(  \
                         #CLASS "::" #FUNCTION)>>::value,                      \
        "Signature doesn't match");

#define FCITX_ADDON_FACTORY(ClassName)                                         \
    extern "C" {                                                               \
    FCITXCORE_EXPORT                                                           \
    ::fcitx::AddonFactory *fcitx_addon_factory_instance() {                    \
        static ClassName factory;                                              \
        return &factory;                                                       \
    }                                                                          \
    }

#define FCITX_ADDON_DEPENDENCY_LOADER(NAME, ADDONMANAGER)                      \
    auto NAME() {                                                              \
        if (_##NAME##FirstCall_) {                                             \
            _##NAME##_ = (ADDONMANAGER).addon(#NAME, true);                    \
            _##NAME##FirstCall_ = false;                                       \
        }                                                                      \
        return _##NAME##_;                                                     \
    }                                                                          \
    bool _##NAME##FirstCall_ = true;                                           \
    ::fcitx::AddonInstance *_##NAME##_ = nullptr;

#endif // _FCITX_ADDONINSTANCE_H_
