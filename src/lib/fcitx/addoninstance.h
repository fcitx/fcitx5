/*
 * Copyright (C) 2016~2016 by CSSlayer
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
#ifndef _FCITX_ADDONINSTANCE_H_
#define _FCITX_ADDONINSTANCE_H_

#include <unordered_map>
#include <functional>
#include <type_traits>
#include "fcitxcore_export.h"
#include "fcitx-utils/library.h"

namespace fcitx {

class AddonInstance;
class AddonFunctionAdaptorBase {
public:
    AddonInstance *addon;
    void *wrapCallback;

    AddonFunctionAdaptorBase(AddonInstance *addon_, void *wrapCallback_) : addon(addon_), wrapCallback(wrapCallback_) {}
};

template <typename T>
struct AddonFunctionSignature;

template <typename Signature>
struct AddonFunctionWrapperType;

template <typename Signature>
using AddonFunctionSignatureType = typename AddonFunctionSignature<Signature>::type;

template <typename Ret, typename... Args>
struct AddonFunctionWrapperType<Ret(Args...)> {
    typedef Ret type(AddonInstance *, AddonFunctionAdaptorBase *, Args...);
};

class FCITXCORE_EXPORT AddonInstance {
public:
    template <typename Signature, typename... Args>
    typename std::function<Signature>::result_type callWithSignature(const std::string &name, Args &&... args) {
        auto iter = callbackMap.find(name);
        if (iter == callbackMap.end()) {
            throw std::runtime_error(name.c_str());
        }
        auto adaptor = callbackMap[name];
        auto func = Library::toFunction<typename AddonFunctionWrapperType<Signature>::type>(adaptor->wrapCallback);
        return func(this, adaptor, std::forward<Args>(args)...);
    }
    template <typename MetaSignatureString, typename... Args>
    auto callWithMetaString(Args &&... args) {
        return callWithSignature<AddonFunctionSignatureType<MetaSignatureString>>(MetaSignatureString::data(),
                                                                                  std::forward<Args>(args)...);
    }
    template <typename MetaType, typename... Args>
    auto call(Args &&... args) {
        return callWithSignature<typename MetaType::Signature>(MetaType::Name::data(), std::forward<Args>(args)...);
    }

    virtual ~AddonInstance();

    void registerCallback(const std::string &name, AddonFunctionAdaptorBase *adaptor) { callbackMap[name] = adaptor; }

    std::unordered_map<std::string, AddonFunctionAdaptorBase *> callbackMap;
};

template <typename Class, typename Ret, typename... Args>
class AddonFunctionAdaptor : public AddonFunctionAdaptorBase {
public:
    typedef Ret (Class::*CallbackType)(Args...);
    typedef Ret Signature(Args...);

    AddonFunctionAdaptor(const std::string &name, Class *addon_, CallbackType pCallback_)
        : AddonFunctionAdaptorBase(addon_, reinterpret_cast<void *>(callback)), pCallback(pCallback_) {
        addon_->registerCallback(name, this);
    }

    static Ret callback(AddonInstance *addon_, AddonFunctionAdaptorBase *adaptor_, Args... args) {
        auto adaptor = reinterpret_cast<AddonFunctionAdaptor<Class, Ret, Args...> *>(adaptor_);
        auto addon = reinterpret_cast<Class *>(addon_);
        auto pCallback = adaptor->pCallback;
        return (addon->*pCallback)(std::forward<Args>(args)...);
    }

private:
    CallbackType pCallback;
};

template <typename Class, typename Ret, typename... Args>
AddonFunctionAdaptor<Class, Ret, Args...> MakeAddonFunctionAdaptor(Ret (Class::*pCallback)(Args...));
}

#define FCITX_ADDON_DECLARE_FUNCTION(NAME, FUNCTION, SIGNATURE...)                                                     \
    namespace fcitx {                                                                                                  \
    template <>                                                                                                        \
    struct AddonFunctionSignature<makeMetaString(#NAME "::" #FUNCTION)> {                                              \
        typedef std::remove_reference_t<decltype(std::declval<SIGNATURE>())> type;                                     \
    };                                                                                                                 \
    namespace I##NAME {                                                                                                \
        struct FUNCTION {                                                                                              \
            typedef makeMetaString(#NAME "::" #FUNCTION) Name;                                                         \
            using Signature = AddonFunctionSignatureType<Name>;                                                        \
        };                                                                                                             \
    }                                                                                                                  \
    }

// template<typename Signature>

#define FCITX_ADDON_EXPORT_FUNCTION(CLASS, FUNCTION)                                                                   \
    decltype(MakeAddonFunctionAdaptor(&CLASS::FUNCTION)) FUNCTION##Adaptor{#CLASS "::" #FUNCTION, this,                \
                                                                           &CLASS::FUNCTION};                          \
    static_assert(std::is_same<decltype(::fcitx::MakeAddonFunctionAdaptor(&CLASS::FUNCTION))::Signature,               \
                               ::fcitx::AddonFunctionSignatureType<MSTR(#CLASS "::" #FUNCTION)>>::value,               \
                  "Signature doesn't match");

#define FCITX_ADDON_FACTORY(ClassName)                                                                                 \
    extern "C" {                                                                                                       \
    FCITXCORE_EXPORT                                                                                                   \
    ::fcitx::AddonFactory *fcitx_addon_factory_instance() {                                                            \
        static ClassName factory;                                                                                      \
        return &factory;                                                                                               \
    }                                                                                                                  \
    }

#endif // _FCITX_ADDONINSTANCE_H_
