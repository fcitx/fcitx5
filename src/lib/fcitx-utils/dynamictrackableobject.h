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
#ifndef _FCITX_UTILS_DYNAMICTRACKABLEOBJECT_H_
#define _FCITX_UTILS_DYNAMICTRACKABLEOBJECT_H_

#include <fcitx-utils/signals.h>
#include <memory>

namespace fcitx {

template <typename T>
class DynamicTrackableObject {
    typedef std::function<void(T *)> callback_type;

public:
    virtual ~DynamicTrackableObject() { destroy(); }

    Signal<void(T *)> destroyed;

protected:
    // permit user to notify the destroy event earlier, when the object is not
    // fully destroyed.
    void destroy() {
        if (!m_destroyed) {
            m_destroyed = true;
            destroyed(static_cast<T *>(this));
            destroyed.disconnectAll();
        }
    }

private:
    bool m_destroyed = false;
};
};

#endif // _FCITX_UTILS_DYNAMICTRACKABLEOBJECT_H_
