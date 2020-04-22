//
// Copyright (C) 2016~2016 by CSSlayer
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
#ifndef _FCITX_INPUTCONTEXTPROPERTY_H_
#define _FCITX_INPUTCONTEXTPROPERTY_H_

#include <memory>
#include <fcitx-utils/macros.h>
#include <fcitx-utils/trackableobject.h>
#include "fcitxcore_export.h"

namespace fcitx {

class FCITXCORE_EXPORT InputContextProperty {
public:
    virtual ~InputContextProperty() {}
    virtual void copyTo(InputContextProperty *){};
    virtual bool needCopy() const { return false; }
};

class InputContext;
class InputContextManager;
class InputContextPropertyFactoryPrivate;

class FCITXCORE_EXPORT InputContextPropertyFactory
    : public fcitx::TrackableObject<InputContextPropertyFactory> {
    friend class InputContextManager;

public:
    InputContextPropertyFactory();
    virtual ~InputContextPropertyFactory();
    virtual InputContextProperty *create(InputContext &) = 0;

    bool registered() const;
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
        : func_(f) {}

    InputContextProperty *create(InputContext &ic) override {
        return func_(ic);
    }

private:
    std::function<Ret *(InputContext &)> func_;
};

template <typename T>
using FactoryFor = LambdaInputContextPropertyFactory<T>;
} // namespace fcitx

#endif // _FCITX_INPUTCONTEXTPROPERTY_H_
