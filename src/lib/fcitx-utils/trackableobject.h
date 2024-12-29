/*
 * SPDX-FileCopyrightText: 2016-2016 CSSlayer <wengxt@gmail.com>
 *
 * SPDX-License-Identifier: LGPL-2.1-or-later
 *
 */
#ifndef _FCITX_UTILS_TRACKABLEOBJECT_H_
#define _FCITX_UTILS_TRACKABLEOBJECT_H_

/// \addtogroup FcitxUtils
/// \{
/// \file
/// \brief Utitliy classes for statically tracking the life of a object.

#include <memory>
#include <type_traits>
#include <utility>
#include <fcitx-utils/handlertable.h>
#include <fcitx-utils/macros.h>

namespace fcitx {

template <typename T>
class TrackableObject;

/// \brief Utility class provides a weak reference to the object.
///
/// Not thread-safe.
template <typename T>
class TrackableObjectReference final {
    friend class TrackableObject<std::remove_const_t<T>>;

public:
    TrackableObjectReference() : rawThat_(nullptr) {}
    FCITX_INLINE_DEFINE_DEFAULT_DTOR_COPY_AND_MOVE(TrackableObjectReference)

    /// \brief Check if the reference is still valid.
    bool isValid() const { return !that_.expired(); }

    /// \brief Check if the reference is not tracking anything.
    bool isNull() const { return rawThat_ == nullptr; }

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
    virtual ~TrackableObject() = default;

    TrackableObjectReference<T> watch() {
        return TrackableObjectReference<T>(*self_, static_cast<T *>(this));
    }

    TrackableObjectReference<const T> watch() const {
        return TrackableObjectReference<const T>(*self_,
                                                 static_cast<const T *>(this));
    }

private:
    std::unique_ptr<std::shared_ptr<bool>> self_;
};
} // namespace fcitx

#endif // _FCITX_UTILS_TRACKABLEOBJECT_H_
