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
    bool isValid() const { return !m_that.expired(); }

    T *get() const { return m_that.expired() ? nullptr : m_rawThat; }

    TrackableObjectReference() : m_rawThat(nullptr) {}

    TrackableObjectReference(const TrackableObjectReference &other)
        : m_that(other.m_that), m_rawThat(other.m_rawThat) {}

    TrackableObjectReference(TrackableObjectReference &&other)
        : m_that(std::move(other.m_that)), m_rawThat(other.m_rawThat) {}

    TrackableObjectReference &operator=(const TrackableObjectReference &other) {
        if (&other == this)
            return *this;
        m_that = other.m_that;
        m_rawThat = other.m_rawThat;
        return *this;
    }

    void unwatch() {
        m_that.reset();
        m_rawThat = nullptr;
    }

private:
    TrackableObjectReference(std::weak_ptr<T *> that, T *rawThat) : m_that(std::move(that)), m_rawThat(rawThat) {}

    std::weak_ptr<T *> m_that;
    T *m_rawThat;
};

template <typename T>
class TrackableObject {
public:
    TrackableObject() : m_self(std::make_unique<std::shared_ptr<T *>>(std::make_shared<T *>(static_cast<T *>(this)))) {}
    TrackableObject(const TrackableObject &) = delete;

    TrackableObjectReference<T> watch() { return TrackableObjectReference<T>(*m_self, static_cast<T *>(this)); }

private:
    std::unique_ptr<std::shared_ptr<T *>> m_self;
};
}

#endif // _FCITX_UTILS_TRACKABLEOBJECT_H_
