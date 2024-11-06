/*
 * SPDX-FileCopyrightText: 2024 Qijia Liu <liumeo@pku.edu.cn>
 *
 * SPDX-License-Identifier: LGPL-2.1-or-later
 *
 */
#include "fcitx-utils/event.h"
#include "fcitx-utils/log.h"
#include "event_impl.h"

namespace fcitx {

template <typename Interface>
struct StubEventSourceBase : public Interface {
public:
    ~StubEventSourceBase() override {}

    bool isEnabled() const override { return false; }

    void setEnabled(bool enabled) override { FCITX_UNUSED(enabled); }

    bool isOneShot() const override { return false; }

    void setOneShot() override {}
};

struct StubEventSource : public StubEventSourceBase<EventSource> {
    StubEventSource() {}
};

struct StubEventSourceIO : public StubEventSourceBase<EventSourceIO> {
    StubEventSourceIO() {}

    int fd() const override { return 0; }

    void setFd(int fd) override { FCITX_UNUSED(fd); }

    IOEventFlags events() const override { return IOEventFlag::In; }

    void setEvents(IOEventFlags flags) override { FCITX_UNUSED(flags); }

    IOEventFlags revents() const override { return IOEventFlag::In; }
};

struct StubEventSourceTime : public StubEventSourceBase<EventSourceTime> {
    StubEventSourceTime() {}

    uint64_t time() const override { return 0; }

    void setTime(uint64_t time) override { FCITX_UNUSED(time); }

    uint64_t accuracy() const override { return 0; }

    void setAccuracy(uint64_t time) override { FCITX_UNUSED(time); }

    clockid_t clock() const override { return 0; }
};

static std::shared_ptr<EventLoopImpl> eventLoopImpl = nullptr;

void setEventLoopImpl(std::unique_ptr<EventLoopImpl> impl) {
    eventLoopImpl = std::move(impl);
}

class EventLoopPrivate {
public:
    EventLoopPrivate() {
        if (!eventLoopImpl) {
            FCITX_WARN() << "Using stub event loop implementation.";
            eventLoopImpl = std::make_shared<EventLoopImpl>();
        }
        impl_ = eventLoopImpl;
    }
    ~EventLoopPrivate() {}

    std::shared_ptr<EventLoopImpl> impl_;
};

EventLoop::EventLoop() : d_ptr(std::make_unique<EventLoopPrivate>()) {}

EventLoop::~EventLoop() = default;

const char *EventLoop::impl() { return "stub"; }

void *EventLoop::nativeHandle() { return nullptr; }

bool EventLoop::exec() { return true; }

void EventLoop::exit() {}

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

std::unique_ptr<EventSourceIO>
EventLoopImpl::addIOEvent(int fd, IOEventFlags flags, IOCallback callback) {
    FCITX_UNUSED(fd);
    FCITX_UNUSED(flags);
    FCITX_UNUSED(callback);
    return std::make_unique<StubEventSourceIO>();
}

std::unique_ptr<EventSourceTime>
EventLoopImpl::addTimeEvent(clockid_t clock, uint64_t usec, uint64_t accuracy,
                            TimeCallback callback) {
    FCITX_UNUSED(clock);
    FCITX_UNUSED(usec);
    FCITX_UNUSED(accuracy);
    FCITX_UNUSED(callback);
    return std::make_unique<StubEventSourceTime>();
}

std::unique_ptr<EventSource>
EventLoopImpl::addExitEvent(EventCallback callback) {
    FCITX_UNUSED(callback);
    return std::make_unique<StubEventSource>();
}

std::unique_ptr<EventSource>
EventLoopImpl::addDeferEvent(EventCallback callback) {
    FCITX_UNUSED(callback);
    return std::make_unique<StubEventSource>();
}

std::unique_ptr<EventSource>
EventLoopImpl::addPostEvent(EventCallback callback) {
    FCITX_UNUSED(callback);
    return std::make_unique<StubEventSource>();
}
} // namespace fcitx
