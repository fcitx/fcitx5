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
#ifndef _FCITX_UTILS_TRACKABLEOBJECT_H_
#define _FCITX_UTILS_TRACKABLEOBJECT_H_

/// \addtogroup FcitxUtils
/// \{
/// \file
/// \brief Utitliy classes for statically tracking the life of a object.

#include <fcitx-utils/handlertable.h>
#include <fcitx-utils/macros.h>
#include <memory>

namespace fcitx {

template <typename T>
class TrackableObject;

/// \brief Utility class provides a weak reference to the object.
///
/// Not thread-safe.
template <typename T>
class TrackableObjectReference final {
    friend class TrackableObject<T>;

public:
    TrackableObjectReference() : rawThat_(nullptr) {}
    FCITX_INLINE_DEFINE_DEFAULT_DTOR_COPY_AND_MOVE(TrackableObjectReference)

    /// \brief Check if the reference is still valid.
    bool isValid() const { return !that_.expired(); }

    /// \brief Get the referenced object. Return nullptr if it is not available.
    T *get() const { return that_.expired() ? nullptr : rawThat_; }

    /// \brief Reset reference to empty state.
    void unwatch() {
        that_.reset();
        rawThat_ = nullptr;
    }

private:
    TrackableObjectReference(std::weak_ptr<bool> that, T *rawThat)
        : that_(std::move(that)), rawThat_(rawThat) {}

    std::weak_ptr<bool> that_;
    T *rawThat_;
};

/// \brief Helper class to be used with TrackableObjectReference
///
/// One need to subclass it to make use of it.
/// \code{.cpp}
/// class Object : TrackableObject<Object> {};
///
/// Object obj; auto ref = obj.watch();
/// \endcode
/// \see TrackableObjectReference
template <typename T>
class TrackableObject {
public:
    TrackableObject()
        : self_(std::make_unique<std::shared_ptr<bool>>(
              std::make_shared<bool>())) {}
    TrackableObject(const TrackableObject &) = delete;
    virtual ~TrackableObject() {}

    TrackableObjectReference<T> watch() {
        return TrackableObjectReference<T>(*self_, static_cast<T *>(this));
    }

private:
    std::unique_ptr<std::shared_ptr<bool>> self_;
};
}

#endif // _FCITX_UTILS_TRACKABLEOBJECT_H_
