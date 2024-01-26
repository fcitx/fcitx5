/*
 * SPDX-FileCopyrightText: 2016-2016 CSSlayer <wengxt@gmail.com>
 *
 * SPDX-License-Identifier: LGPL-2.1-or-later
 *
 */
#ifndef _FCITX_INPUTCONTEXTPROPERTY_H_
#define _FCITX_INPUTCONTEXTPROPERTY_H_

#include <memory>
#include <fcitx-utils/macros.h>
#include <fcitx-utils/trackableobject.h>
#include "fcitxcore_export.h"

/// \addtogroup FcitxCore
/// \{
/// \file
/// \brief Input Context Property for Fcitx.

namespace fcitx {

/**
 * This is a class that designed to store state that is specific to certain
 * input context.
 *
 * The state may be copied to other input context, depending on the value
 * GlobalConfig::shareInputState. Also in reality, you don't need to copy
 * everything. For example, sub input mode can be a good candidate to copy over
 * the value, like Japanese input method is under hiragana mode or katakana
 * mode. The current input buffer usually need not to be shared between
 * different input context. In Fcitx 5, there can be more than one input context
 * has active focus (Rare for one-seat setup, but it is possible), so it is
 * important to consider that and not to use a global state in the input method
 * engine.
 *
 */
class FCITXCORE_EXPORT InputContextProperty {
public:
    virtual ~InputContextProperty() {}
    /**
     * copy state to another property.
     *
     * Default implemenation is empty.
     * This is triggered by InputContext::updateProperty.
     *
     * @see InputContext::updateProperty
     */
    virtual void copyTo(InputContextProperty *) {}
    /// Quick check if there's need to copy over the state.
    virtual bool needCopy() const { return false; }
};

class InputContext;
class InputContextPropertyFactoryPrivate;

/**
 * Factory class for input context property.
 *
 * Factory can be only registered with one InputContextManager.
 * The factory will automatically unregister itself upon destruction.
 * The factory need to unregister before the destruction of InputContextManager.
 *
 * @see InputContextManager::registerProperty
 */
class FCITXCORE_EXPORT InputContextPropertyFactory
    : public fcitx::TrackableObject<InputContextPropertyFactory> {
    friend class InputContextManager;

public:
    InputContextPropertyFactory();
    virtual ~InputContextPropertyFactory();
    virtual InputContextProperty *create(InputContext &) = 0;

    /// Return whether the factory is already registered with an
    /// InputContextManager.
    bool registered() const;
    /// Unregister the factory from current InputContextManager.
    void unregister();

private:
    std::unique_ptr<InputContextPropertyFactoryPrivate> d_ptr;
    FCITX_DECLARE_PRIVATE(InputContextPropertyFactory);
};

template <typename T>
class SimpleInputContextPropertyFactory : public InputContextPropertyFactory {
public:
    typedef T PropertyType;
    InputContextProperty *create(InputContext &) override { return new T; }
};

template <typename Ret>
class LambdaInputContextPropertyFactory : public InputContextPropertyFactory {
public:
    typedef Ret PropertyType;
    LambdaInputContextPropertyFactory(std::function<Ret *(InputContext &)> f)
        : func_(std::move(f)) {}

    InputContextProperty *create(InputContext &ic) override {
        return func_(ic);
    }

private:
    std::function<Ret *(InputContext &)> func_;
};

/**
 * Convinient short type alias for creating a LambdaInputContextPropertyFactory.
 *
 * Example usage:
 * Define it as a field of AddonInstance.
 * @code
 * FactoryFor<MyProperty> factory_;
 * @endcode
 *
 * Register the property to InputContextManager in constructor of AddonInstance.
 * @code
 * instance_->inputContextManager().registerProperty("propertyName",
 *                                                   &factory_);
 * @endcode
 * The name of property need to be unique.
 *
 * Get the property from input context within the addon.
 * @code
 * auto *state = inputContext->propertyFor(&factory_);
 * @endcode
 * The returned type will be casted automatically. Get property with factory is
 * faster than get property with name.
 *
 * Accessing the property outside the addon.
 * @code
 * InputContextProperty *state = inputContext->property("propertyName");
 * @endcode
 * You may need to cast the type to access any private data within it.
 */
template <typename T>
using FactoryFor = LambdaInputContextPropertyFactory<T>;
} // namespace fcitx

#endif // _FCITX_INPUTCONTEXTPROPERTY_H_
