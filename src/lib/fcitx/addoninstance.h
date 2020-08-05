/*
 * SPDX-FileCopyrightText: 2016-2016 CSSlayer <wengxt@gmail.com>
 *
 * SPDX-License-Identifier: LGPL-2.1-or-later
 *
 */
#ifndef _FCITX_ADDONINSTANCE_H_
#define _FCITX_ADDONINSTANCE_H_

#include <functional>
#include <memory>
#include <type_traits>
#include <unordered_map>
#include <fcitx-config/configuration.h>
#include <fcitx-utils/library.h>
#include <fcitx-utils/metastring.h>
#include <fcitx/addoninstance_details.h>
#include "fcitxcore_export.h"

/// \addtogroup FcitxCore
/// \{
/// \file
/// \brief Addon For fcitx.

namespace fcitx {

/// \brief Base class for any addon in fcitx.
/// To implement addon in fcitx, you will need to create a sub class for this
/// class.
///
/// To make an SharedLibrary Addon, you will also need to use
/// FCITX_ADDON_FACTORY to export the factory for addon.
///
///  An addon can export several function to be invoked by other addons.
/// When you need to do so, you will need some extra command in your
/// CMakeLists.txt, and using FCITX_ADDON_DECLARE_FUNCTION and
/// FCITX_ADDON_EXPORT_FUNCTION.
/// \code{.unparsed}
/// fcitx5_export_module(XCB
///                      TARGET xcb
///                      BUILD_INCLUDE_DIRECTORIES "${CMAKE_CURRENT_SOURCE_DIR}"
///                      HEADERS xcb_public.h INSTALL)
/// \endcode
///
/// First you will need to create a header file with exported addon function.
/// E.g. dummyaddon_public.h
/// \code{.cpp}
/// FCITX_ADDON_DECLARE_FUNCTION(DummyAddon, addOne, int(int));
/// \endcode
/// This file declares a function addOne for DummyAddon, with function signature
/// int(int).
///
/// Then, when you implement the class, add using the macro
/// FCITX_ADDON_EXPORT_FUNCTION to the addon class.
/// \code{.cpp}
/// class DummyAddon : public fcitx::AddonInstance {
/// public:
///     int addOne(int a) { return a + 1; }
///
///     FCITX_ADDON_EXPORT_FUNCTION(DummyAddon, addOne);
/// };
/// \endcode
/// This macro will register the function and check the signature against the
/// actual function to make sure they have the same signature.
///
/// In order to invoke the function in other code, you will need to first obtain
/// the pointer to the addon via AddonManager. Then invoke it by
/// \code{.cpp}
/// addon->call<fcitx::IDummyAddon::addOne>(7);
/// \endcode
class FCITXCORE_EXPORT AddonInstance {
public:
    AddonInstance();
    virtual ~AddonInstance();

    /// Reload configuration from disk.
    virtual void reloadConfig() {}

    /// Save any relevant data. Usually, it will be invoked when fcitx exits.
    virtual void save() {}

    /// Get the configuration.
    virtual const Configuration *getConfig() const { return nullptr; }

    /// Set configuration from Raw Config.
    virtual void setConfig(const RawConfig &) {}
    virtual const Configuration *getSubConfig(const std::string &) const {
        return nullptr;
    }
    virtual void setSubConfig(const std::string &, const RawConfig &) {}

    template <typename Signature, typename... Args>
    typename std::function<Signature>::result_type
    callWithSignature(const std::string &name, Args &&... args) {
        auto *adaptor = findCall(name);
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

    /// Call an exported function for this addon.
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
} // namespace fcitx

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

/// A convenient macro to obtain the addon pointer of another addon.
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
