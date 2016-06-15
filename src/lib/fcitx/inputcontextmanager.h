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
#ifndef _FCITX_INPUTCONTEXTMANAGER_H_
#define _FCITX_INPUTCONTEXTMANAGER_H_

#include <memory>
#include <fcitx-utils/macros.h>
#include "fcitxcore_export.h"
#include "inputcontext.h"
#include <fcitx-config/enum.h>

namespace fcitx {

class InputContextManagerPrivate;
class FocusGroup;
class Instance;
class InputContextProperty;
typedef std::function<InputContextProperty *(InputContext &)> InputContextPropertyFactory;

FCITX_CONFIG_ENUM(PropertyPropagatePolicy, All, Program, None);

class FCITXCORE_EXPORT InputContextManager {
    friend class InputContext;
    friend class FocusGroup;
    friend class Instance;

public:
    InputContextManager();
    virtual ~InputContextManager();

    FocusGroup &globalFocusGroup();

    InputContext *findByUUID(ICUUID uuid);

    int registerProperty(InputContextPropertyFactory factory);
    void unregisterProperty(int idx);

    void setPropertyPropagatePolicy(PropertyPropagatePolicy policy);

private:
    Instance *instance();
    void setInstance(Instance *instance);
    void registerInputContext(InputContext &inputContext);
    void unregisterInputContext(InputContext &inputContext);

    void registerFocusGroup(FocusGroup &group);
    void unregisterFocusGroup(FocusGroup &group);

    void propagateProperty(InputContext &inputContext, int idx);

    void focusOutNonGlobal();

    std::unique_ptr<InputContextManagerPrivate> d_ptr;
    FCITX_DECLARE_PRIVATE(InputContextManager);
};
}

#endif // _FCITX_INPUTCONTEXTMANAGER_H_
