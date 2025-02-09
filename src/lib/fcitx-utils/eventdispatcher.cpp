/*
 * SPDX-FileCopyrightText: 2019-2019 CSSlayer <wengxt@gmail.com>
 *
 * SPDX-License-Identifier: LGPL-2.1-or-later
 *
 */
#include "eventdispatcher.h"
#include <cstdint>
#include <functional>
#include <memory>
#include <mutex>
#include <queue>
#include <utility>
#include "event.h"
#include "eventloopinterface.h"
#include "fs.h"
#include "macros.h"
#include "unixfd.h"

namespace fcitx {
class EventDispatcherPrivate {
public:
    void dispatchEvent() {
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

    // Mutex to be used to protect fields below.
    mutable std::mutex mutex_;
    std::queue<std::function<void()>> eventList_;
    std::unique_ptr<EventSourceAsync> asyncEvent_;
    EventLoop *loop_ = nullptr;
};

EventDispatcher::EventDispatcher()
    : d_ptr(std::make_unique<EventDispatcherPrivate>()) {}

EventDispatcher::~EventDispatcher() = default;

void EventDispatcher::attach(EventLoop *event) {
    FCITX_D();
    std::lock_guard<std::mutex> lock(d->mutex_);
    d->asyncEvent_ = event->addAsyncEvent([d](EventSource *) {
        d->dispatchEvent();
        return true;
    });
    d->loop_ = event;
}

void EventDispatcher::detach() {
    FCITX_D();
    std::lock_guard<std::mutex> lock(d->mutex_);
    d->asyncEvent_.reset();
    d->loop_ = nullptr;
}

void EventDispatcher::schedule(std::function<void()> functor) {
    FCITX_D();
    std::lock_guard<std::mutex> lock(d->mutex_);
    // functor can be null and we will still trigger async event.
    if (functor) {
        if (!d->asyncEvent_) {
            return;
        }
        d->eventList_.push(std::move(functor));
    }
    d->asyncEvent_->send();
}

EventLoop *EventDispatcher::eventLoop() const {
    FCITX_D();
    std::lock_guard<std::mutex> lock(d->mutex_);
    return d->loop_;
}

} // namespace fcitx
