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
#ifndef _FCITX_UTILS_TRACKABLEOBJECT_H_
#define _FCITX_UTILS_TRACKABLEOBJECT_H_

#include <fcitx-utils/handlertable.h>
#include <fcitx-utils/macros.h>
#include <memory>

namespace fcitx {

template <typename T>
class TrackableObject;

template <typename T>
class TrackableObjectReference {
    friend class TrackableObject<T>;

public:
    bool isValid() const { return !that_.expired(); }

    T *get() const { return that_.expired() ? nullptr : rawThat_; }

    TrackableObjectReference() : rawThat_(nullptr) {}

    TrackableObjectReference(const TrackableObjectReference &other)
        : that_(other.that_), rawThat_(other.rawThat_) {}

    TrackableObjectReference(TrackableObjectReference &&other)
        : that_(std::move(other.that_)), rawThat_(other.rawThat_) {}

    TrackableObjectReference &operator=(const TrackableObjectReference &other) {
        if (&other == this)
            return *this;
        that_ = other.that_;
        rawThat_ = other.rawThat_;
        return *this;
    }

    void unwatch() {
        that_.reset();
        rawThat_ = nullptr;
    }

private:
    TrackableObjectReference(std::weak_ptr<T *> that, T *rawThat) : that_(std::move(that)), rawThat_(rawThat) {}

    std::weak_ptr<T *> that_;
    T *rawThat_;
};

template <typename T>
class TrackableObject {
public:
    TrackableObject() : self_(std::make_unique<std::shared_ptr<T *>>(std::make_shared<T *>(static_cast<T *>(this)))) {}
    TrackableObject(const TrackableObject &) = delete;

    TrackableObjectReference<T> watch() { return TrackableObjectReference<T>(*self_, static_cast<T *>(this)); }

private:
    std::unique_ptr<std::shared_ptr<T *>> self_;
};
}

#endif // _FCITX_UTILS_TRACKABLEOBJECT_H_
