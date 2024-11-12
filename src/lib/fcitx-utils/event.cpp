
/*
 * SPDX-FileCopyrightText: 2015-2015 CSSlayer <wengxt@gmail.com>
 *
 * SPDX-License-Identifier: LGPL-2.1-or-later
 *
 */

#include "event.h"
#include <cstdint>
#include <cstring>
#include <ctime>
#include <memory>
#include <utility>
#include "event_p.h"
#include "eventloopinterface.h"
#include "macros.h"

namespace fcitx {

class EventLoopPrivate {
public:
    EventLoopPrivate(std::unique_ptr<EventLoopInterface> impl)
        : impl_(std::move(impl)) {}

    std::unique_ptr<EventLoopInterface> impl_;
};

EventLoop::EventLoop() : EventLoop(createDefaultEventLoop()) {}

EventLoop::EventLoop(std::unique_ptr<EventLoopInterface> impl)
    : d_ptr(std::make_unique<EventLoopPrivate>(std::move(impl))) {}

EventLoop::~EventLoop() = default;

const char *EventLoop::impl() { return defaultEventLoopImplementation(); }

const char *EventLoop::implementation() const {
    FCITX_D();
    return d->impl_->implementation();
}

void *EventLoop::nativeHandle() {
    FCITX_D();
    return d->impl_->nativeHandle();
}

bool EventLoop::exec() {
    FCITX_D();
    return d->impl_->exec();
}

void EventLoop::exit() {
    FCITX_D();
    return d->impl_->exit();
}

std::unique_ptr<EventSourceIO> EventLoop::addIOEvent(int fd, IOEventFlags flags,
                                                     IOCallback callback) {
    FCITX_D();
    return d->impl_->addIOEvent(fd, flags, std::move(callback));
}

std::unique_ptr<EventSourceTime>
EventLoop::addTimeEvent(clockid_t clock, uint64_t usec, uint64_t accuracy,
                        TimeCallback callback) {
    FCITX_D();
    return d->impl_->addTimeEvent(clock, usec, accuracy, std::move(callback));
}

std::unique_ptr<EventSource> EventLoop::addExitEvent(EventCallback callback) {
    FCITX_D();
    return d->impl_->addExitEvent(std::move(callback));
}

std::unique_ptr<EventSource> EventLoop::addDeferEvent(EventCallback callback) {
    FCITX_D();
    return d->impl_->addDeferEvent(std::move(callback));
}

std::unique_ptr<EventSource> EventLoop::addPostEvent(EventCallback callback) {
    FCITX_D();
    return d->impl_->addPostEvent(std::move(callback));
}

} // namespace fcitx
