/*
 * SPDX-FileCopyrightText: 2019-2019 CSSlayer <wengxt@gmail.com>
 *
 * SPDX-License-Identifier: LGPL-2.1-or-later
 *
 */
#include "eventdispatcher.h"
#include <fcntl.h>
#include <unistd.h>
#include <mutex>
#include <queue>
#include <stdexcept>
#include "event.h"
#include "misc_p.h"
#include "unixfd.h"

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
    if (safePipe(selfpipe)) {
        throw std::runtime_error("Failed to create pipe");
    }
    d->fd_[0].give(selfpipe[0]);
    d->fd_[1].give(selfpipe[1]);
}

EventDispatcher::~EventDispatcher() {}

void EventDispatcher::attach(EventLoop *event) {
    FCITX_D();
    std::lock_guard<std::mutex> lock(d->mutex_);
    d->ioEvent_ = event->addIOEvent(d->fd_[0].fd(), IOEventFlag::In,
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
