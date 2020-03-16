//
// Copyright (C) 2019~2019 by CSSlayer
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
#include "eventdispatcher.h"
#include "event.h"
#include "unixfd.h"
#include <fcntl.h>
#include <mutex>
#include <queue>
#include <unistd.h>

namespace fcitx {
class EventDispatcherPrivate {
public:
    void dispatchEvent() {
        uint8_t dummy;
        while (fs::safeRead(fd_[0].fd(), &dummy, sizeof(dummy)) > 0) {
        }
        std::queue<std::function<void()>> eventList;
        {
            std::lock_guard<std::mutex> lock(mutex_);
            using std::swap;
            std::swap(eventList, eventList_);
        }
        while (!eventList.empty()) {
            auto functor = std::move(eventList.front());
            eventList.pop();
            functor();
        }
    }

    // Mutex to be used to protect eventList_.
    std::mutex mutex_;
    std::queue<std::function<void()>> eventList_;
    std::unique_ptr<EventSourceIO> ioEvent_;
    UnixFD fd_[2];
};

EventDispatcher::EventDispatcher()
    : d_ptr(std::make_unique<EventDispatcherPrivate>()) {
    FCITX_D();
    int selfpipe[2];
    if (pipe2(selfpipe, O_CLOEXEC | O_NONBLOCK) < 0) {
        throw std::runtime_error("Failed to create pipe");
    }
    d->fd_[0].give(selfpipe[0]);
    d->fd_[1].give(selfpipe[1]);
}

EventDispatcher::~EventDispatcher() {}

void EventDispatcher::attach(EventLoop *loop) {
    FCITX_D();
    std::lock_guard<std::mutex> lock(d->mutex_);
    d->ioEvent_ = loop->addIOEvent(d->fd_[0].fd(), IOEventFlag::In,
                                   [d](EventSource *, int, IOEventFlags) {
                                       d->dispatchEvent();
                                       return true;
                                   });
}

void EventDispatcher::detach() {
    FCITX_D();
    std::lock_guard<std::mutex> lock(d->mutex_);
    d->ioEvent_.reset();
}

void EventDispatcher::schedule(std::function<void()> functor) {
    FCITX_D();
    {
        std::lock_guard<std::mutex> lock(d->mutex_);
        if (!d->ioEvent_) {
            return;
        }
        d->eventList_.push(std::move(functor));
    }
    uint8_t dummy = 0;
    fs::safeWrite(d->fd_[1].fd(), &dummy, 1);
}

} // namespace fcitx
