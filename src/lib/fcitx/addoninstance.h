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
    AddonInstance* addon;
    void *wrapCallback;

    AddonFunctionAdaptorBase(AddonInstance *addon_, void *wrapCallback_) :
        addon(addon_), wrapCallback(wrapCallback_) {
    }
};

class FCITXCORE_EXPORT AddonInstance {
public:
    template <typename Signature, typename ...Args>
    std::result_of_t<std::add_pointer_t<Signature>(Args...)> call(const std::string &name, Args&&... args) {
        auto adaptor = callbackMap[name];
        auto func = Library::toFunction<std::result_of_t<std::add_pointer_t<Signature>(Args...)>(AddonInstance*, AddonFunctionAdaptorBase*, Args&&...)>(adaptor->wrapCallback);
        return func(this, adaptor, std::forward<Args>(args)...);
    }

    virtual ~AddonInstance();

    void registerCallback(const std::string &name, AddonFunctionAdaptorBase* adaptor) {
        callbackMap[name] = adaptor;
    }

    std::unordered_map<std::string, AddonFunctionAdaptorBase*> callbackMap;
};

template<typename Class, typename Ret, typename...Args>
class AddonFunctionAdaptor : public AddonFunctionAdaptorBase {
public:
    typedef Ret (Class::*CallbackType)(Args...);

    AddonFunctionAdaptor(const std::string &name, Class* addon_, CallbackType pCallback_) :
        AddonFunctionAdaptorBase(addon_, reinterpret_cast<void*>(callback)), pCallback(pCallback_) {
        addon_->registerCallback(name, this);
    }

    static Ret callback(AddonInstance *addon_, AddonFunctionAdaptorBase* adaptor_, Args&&... args) {
        auto adaptor = reinterpret_cast<AddonFunctionAdaptor<Class, Ret, Args...>*>(adaptor_);
        auto addon = reinterpret_cast<Class*>(addon_);
        auto pCallback = adaptor->pCallback;
        return (addon->*pCallback)(args...);
    }
private:
    CallbackType pCallback;
};

template<typename Class, typename Ret, typename...Args>
AddonFunctionAdaptor<Class, Ret, Args...> MakeAddonFunctionAdaptor(Ret (Class::*pCallback)(Args...));

}

#define FCITX_ADDON_EXPORT_FUNCTION(NAME, FUNCTION) \
    decltype(MakeAddonFunctionAdaptor(&FUNCTION)) NAME##Adaptor{#NAME, this, &FUNCTION}


#define FCITX_ADDON_FACTORY(ClassName)                                        \
    extern "C" {                                                               \
    FCITXCORE_EXPORT                                                           \
    ::fcitx::AddonFactory *fcitx_addon_factory_instance() {                    \
        static ClassName factory;                                              \
        return &factory;                                                       \
    }                                                                          \
    }

#endif // _FCITX_ADDONINSTANCE_H_
