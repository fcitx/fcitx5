/*
 * SPDX-FileCopyrightText: 2016-2016 CSSlayer <wengxt@gmail.com>
 *
 * SPDX-License-Identifier: LGPL-2.1-or-later
 *
 */
#ifndef _FCITX_INPUTCONTEXTMANAGER_H_
#define _FCITX_INPUTCONTEXTMANAGER_H_

#include <memory>
#include <fcitx-config/enum.h>
#include <fcitx-utils/macros.h>
#include <fcitx/fcitxcore_export.h>
#include <fcitx/inputcontext.h>

namespace fcitx {

class InputContextManagerPrivate;
class FocusGroup;
class Instance;
class InputContextProperty;
typedef std::function<bool(FocusGroup *ic)> FocusGroupVisitor;

FCITX_CONFIG_ENUM(PropertyPropagatePolicy, All, Program, No);

class FCITXCORE_EXPORT InputContextManager {
    friend class InputContext;
    friend class FocusGroup;
    friend class Instance;
    friend class InputContextPropertyFactory;

public:
    InputContextManager();
    virtual ~InputContextManager();

    /**
     * Find the input context by UUID.
     *
     * This is useful when you want to pass a token from another process to
     * identify the input context.
     *
     * @param uuid UUID of input context.
     * @return pointer to input context or null if nothing is found.
     *
     * @see InputContext::uuid
     */
    InputContext *findByUUID(ICUUID uuid);

    /**
     * Set the property propgate policy.
     *
     * The policy can be either All, Program or No, to define whether a certain
     * state need to be copied  to another input context.
     *
     * @param policy policy
     *
     * @see GlobalConfig::shareInputState
     */
    void setPropertyPropagatePolicy(PropertyPropagatePolicy policy);

    Instance *instance();

    /**
     * Register a named property for input context.
     *
     * This is used to store the per-input context state.
     *
     * @param name unique name of input context.
     * @param factory factory
     * @return registration successful or not.
     */
    bool registerProperty(const std::string &name,
                          InputContextPropertyFactory *factory);

    bool foreach(const InputContextVisitor &visitor);
    bool foreachFocused(const InputContextVisitor &visitor);
    bool foreachGroup(const FocusGroupVisitor &visitor);

    /**
     * Get the last focused input context. This is useful for certain UI to get
     * the most recently used input context.
     *
     * @return pointer of the last focused input context or null if there is no
     * focus.
     */
    InputContext *lastFocusedInputContext();
    /**
     * Get the last used input context. This is useful for certain UI to get the
     * most recently used input context.
     *
     * Certain UI implementation may cause focus out in the application, this is
     * a way for them to get the input context being used.
     *
     * When PropertyPropagatePolicy is All, if there is no other recently
     * focused input context it will return a dummy input context. It is useful
     * to use this dummy IC to propagate data to other input context, e.g.
     * change current input method.
     *
     * @return fcitx::InputContext*
     */
    InputContext *mostRecentInputContext();

    /**
     * Return a dummy input context registered with this input method manager.
     *
     * The value is useful for a place holder in certain cases, e.g. get some
     * value from action.
     *
     * @return fcitx::InputContext*
     * @since 5.0.24
     */
    InputContext *dummyInputContext() const;

    void setPreeditEnabledByDefault(bool enable);
    bool isPreeditEnabledByDefault() const;

private:
    void finalize();

    void setInstance(Instance *instance);
    void registerInputContext(InputContext &inputContext);
    void unregisterInputContext(InputContext &inputContext);

    void registerFocusGroup(FocusGroup &group);
    void unregisterFocusGroup(FocusGroup &group);
    void unregisterProperty(const std::string &name);

    void notifyFocus(InputContext &inputContext, bool focus);

    InputContextPropertyFactory *factoryForName(const std::string &name);
    void propagateProperty(InputContext &inputContext,
                           const InputContextPropertyFactory *factory);
    InputContextProperty *property(InputContext &inputContext,
                                   const InputContextPropertyFactory *factory);

    std::unique_ptr<InputContextManagerPrivate> d_ptr;
    FCITX_DECLARE_PRIVATE(InputContextManager);
};
} // namespace fcitx

#endif // _FCITX_INPUTCONTEXTMANAGER_H_
