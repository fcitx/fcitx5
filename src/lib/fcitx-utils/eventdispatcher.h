//
// Copyright (C) 2015~2019 by CSSlayer
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

#ifndef _FCITX_UTILS_EVENTDISPATCHER_H_
#define _FCITX_UTILS_EVENTDISPATCHER_H_

#include <functional>
#include <memory>
#include <fcitx-utils/macros.h>
#include "fcitxutils_export.h"

namespace fcitx {

class EventLoop;
class EventDispatcherPrivate;
/**
 * A thread safe class to post event to a certain EventLoop.
 *
 */
class FCITXUTILS_EXPORT EventDispatcher {
public:
    /**
     * Construct a new event dispatcher. May throw exception if it fails to
     * create underlying file descriptor.
     */
    EventDispatcher();
    virtual ~EventDispatcher();

    /**
     * Attach EventDispatcher to an EventLoop. Must be called in the same thread
     * of EventLoop.
     *
     * @param event event loop to attach to.
     */
    void attach(EventLoop *event);

    /**
     * Detach event dispatcher from event loop, must be called from the same
     * thread from event loop.
     */
    void detach();
    /**
     * A thread-safe function to schedule a functor to be call from event loop.
     *
     * @param functor functor to be called.
     */
    void schedule(std::function<void()> functor);

private:
    const std::unique_ptr<EventDispatcherPrivate> d_ptr;
    FCITX_DECLARE_PRIVATE(EventDispatcher);
};

} // namespace fcitx
#endif // _FCITX_UTILS_EVENTDISPATCHER_H_
