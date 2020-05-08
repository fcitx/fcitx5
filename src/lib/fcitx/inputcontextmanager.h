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
#include <fcitx/inputcontext.h>
#include <fcitx/inputcontextproperty.h>
#include "fcitxcore_export.h"

namespace fcitx {

class InputContextManagerPrivate;
class FocusGroup;
class Instance;
class InputContextProperty;
typedef std::function<bool(FocusGroup *ic)> FocusGroupVisitor;

FCITX_CONFIG_ENUM(PropertyPropagatePolicy, All, Program, None);

class FCITXCORE_EXPORT InputContextManager {
    friend class InputContext;
    friend class FocusGroup;
    friend class Instance;
    friend class InputContextPropertyFactory;

public:
    InputContextManager();
    virtual ~InputContextManager();

    InputContext *findByUUID(ICUUID uuid);

    void setPropertyPropagatePolicy(PropertyPropagatePolicy policy);

    Instance *instance();

    bool registerProperty(const std::string &name,
                          InputContextPropertyFactory *factory);

    bool foreach(const InputContextVisitor &visitor);
    bool foreachFocused(const InputContextVisitor &visitor);
    bool foreachGroup(const FocusGroupVisitor &visitor);

    InputContext *lastFocusedInputContext();
    InputContext *mostRecentInputContext();

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
