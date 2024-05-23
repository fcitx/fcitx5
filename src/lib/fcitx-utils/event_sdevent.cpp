/*
 * SPDX-FileCopyrightText: 2015-2015 CSSlayer <wengxt@gmail.com>
 *
 * SPDX-License-Identifier: LGPL-2.1-or-later
 *
 */

#include <cstdint>
#include <cstdlib>
#include <exception>
#include <functional>
#include <memory>
#include <mutex>
#include <stdexcept>
#include <utility>
#include <sys/epoll.h>

#if defined(__COVERITY__) && !defined(__INCLUDE_LEVEL__)
#define __INCLUDE_LEVEL__ 2
#endif
#include <systemd/sd-event.h>
#include "event.h"
#include "log.h"
#include "macros.h"
#include "stringutils.h"

namespace fcitx {

namespace {
uint32_t IOEventFlagsToEpollFlags(IOEventFlags flags) {
    uint32_t result = 0;
    if (flags & IOEventFlag::In) {
        result |= EPOLLIN;
    }
    if (flags & IOEventFlag::Out) {
        result |= EPOLLOUT;
    }
    if (flags & IOEventFlag::Err) {
        result |= EPOLLERR;
    }
    if (flags & IOEventFlag::Hup) {
        result |= EPOLLHUP;
    }
    if (flags & IOEventFlag::EdgeTrigger) {
        result |= EPOLLET;
    }
    return result;
}

IOEventFlags EpollFlagsToIOEventFlags(uint32_t flags) {
    return ((flags & EPOLLIN) ? IOEventFlag::In : IOEventFlags()) |
           ((flags & EPOLLOUT) ? IOEventFlag::Out : IOEventFlags()) |
           ((flags & EPOLLERR) ? IOEventFlag::Err : IOEventFlags()) |
           ((flags & EPOLLHUP) ? IOEventFlag::Hup : IOEventFlags()) |
           ((flags & EPOLLET) ? IOEventFlag::EdgeTrigger : IOEventFlags());
}

} // namespace

template <typename Interface>
struct SDEventSourceBase : public Interface {
public:
    ~SDEventSourceBase() override {
        if (eventSource_) {
            sd_event_source_set_enabled(eventSource_, SD_EVENT_OFF);
            sd_event_source_set_userdata(eventSource_, nullptr);
            sd_event_source_unref(eventSource_);
        }
    }

    void setEventSource(sd_event_source *event) { eventSource_ = event; }

    bool isEnabled() const override {
        int result = 0;
        if (int err = sd_event_source_get_enabled(eventSource_, &result);
            err < 0) {
            throw EventLoopException(err);
        }
        return result != SD_EVENT_OFF;
    }

    void setEnabled(bool enabled) override {
        sd_event_source_set_enabled(eventSource_,
                                    enabled ? SD_EVENT_ON : SD_EVENT_OFF);
    }

    bool isOneShot() const override {
        int result = 0;
        if (int err = sd_event_source_get_enabled(eventSource_, &result);
            err < 0) {
            throw EventLoopException(err);
        }
        return result == SD_EVENT_ONESHOT;
    }

    void setOneShot() override {
        sd_event_source_set_enabled(eventSource_, SD_EVENT_ONESHOT);
    }

protected:
    sd_event_source *eventSource_;
};

struct SDEventSource : public SDEventSourceBase<EventSource> {
    SDEventSource(EventCallback _callback)
        : callback_(std::make_shared<EventCallback>(std::move(_callback))) {}

    std::shared_ptr<EventCallback> callback_;
};

struct SDEventSourceIO : public SDEventSourceBase<EventSourceIO> {
    SDEventSourceIO(IOCallback _callback)
        : callback_(std::make_shared<IOCallback>(std::move(_callback))) {}

    int fd() const override {
        int ret = sd_event_source_get_io_fd(eventSource_);
        if (ret < 0) {
            throw EventLoopException(ret);
        }
        return ret;
    }

    void setFd(int fd) override {
        int ret = sd_event_source_set_io_fd(eventSource_, fd);
        if (ret < 0) {
            throw EventLoopException(ret);
        }
    }

    IOEventFlags events() const override {
        uint32_t events;
        int ret = sd_event_source_get_io_events(eventSource_, &events);
        if (ret < 0) {
            throw EventLoopException(ret);
        }
        return EpollFlagsToIOEventFlags(events);
    }

    void setEvents(IOEventFlags flags) override {
        int ret = sd_event_source_set_io_events(
            eventSource_, IOEventFlagsToEpollFlags(flags));
        if (ret < 0) {
            throw EventLoopException(ret);
        }
    }

    IOEventFlags revents() const override {
        uint32_t revents;
        int ret = sd_event_source_get_io_revents(eventSource_, &revents);
        if (ret < 0) {
            throw EventLoopException(ret);
        }
        return EpollFlagsToIOEventFlags(revents);
    }

    std::shared_ptr<IOCallback> callback_;
};

struct SDEventSourceTime : public SDEventSourceBase<EventSourceTime> {
    SDEventSourceTime(TimeCallback _callback)
        : callback_(std::make_shared<TimeCallback>(std::move(_callback))) {}

    uint64_t time() const override {
        uint64_t time;
        int err = sd_event_source_get_time(eventSource_, &time);
        if (err < 0) {
            throw EventLoopException(err);
        }
        return time;
    }

    void setTime(uint64_t time) override {
        int ret = sd_event_source_set_time(eventSource_, time);
        if (ret < 0) {
            throw EventLoopException(ret);
        }
    }

    uint64_t accuracy() const override {
        uint64_t time;
        int err = sd_event_source_get_time_accuracy(eventSource_, &time);
        if (err < 0) {
            throw EventLoopException(err);
        }
        return time;
    }

    void setAccuracy(uint64_t time) override {
        int ret = sd_event_source_set_time_accuracy(eventSource_, time);
        if (ret < 0) {
            throw EventLoopException(ret);
        }
    }

    clockid_t clock() const override {
        clockid_t clock;
        int err = sd_event_source_get_time_clock(eventSource_, &clock);
        if (err < 0) {
            throw EventLoopException(err);
        }
        return clock;
    }

    std::shared_ptr<TimeCallback> callback_;
};

class EventLoopPrivate {
public:
    EventLoopPrivate() {
        if (int rc = sd_event_new(&event_); rc < 0) {
            throw std::runtime_error(stringutils::concat(
                "Create sd_event failed. error code: ", rc));
        }
    }

    ~EventLoopPrivate() { sd_event_unref(event_); }

    std::mutex mutex_;
    sd_event *event_ = nullptr;
};

EventLoop::EventLoop() : d_ptr(std::make_unique<EventLoopPrivate>()) {}

EventLoop::~EventLoop() = default;

const char *EventLoop::impl() { return "sd-event"; }

void *EventLoop::nativeHandle() {
    FCITX_D();
    return d->event_;
}

bool EventLoop::exec() {
    FCITX_D();
    int r = sd_event_loop(d->event_);
    return r >= 0;
}

void EventLoop::exit() {
    FCITX_D();
    sd_event_exit(d->event_, 0);
}

int IOEventCallback(sd_event_source * /*unused*/, int fd, uint32_t revents,
                    void *userdata) {
    auto *source = static_cast<SDEventSourceIO *>(userdata);
    if (!source) {
        return 0;
    }
    try {
        auto callback = source->callback_;
        auto result =
            (*callback)(source, fd, EpollFlagsToIOEventFlags(revents));
        return result ? 0 : -1;
    } catch (const std::exception &e) {
        FCITX_FATAL() << e.what();
    }
    return -1;
}

std::unique_ptr<EventSourceIO> EventLoop::addIOEvent(int fd, IOEventFlags flags,
                                                     IOCallback callback) {
    FCITX_D();
    auto source = std::make_unique<SDEventSourceIO>(std::move(callback));
    sd_event_source *sdEventSource;
    if (int err = sd_event_add_io(d->event_, &sdEventSource, fd,
                                  IOEventFlagsToEpollFlags(flags),
                                  IOEventCallback, source.get());
        err < 0) {
        throw EventLoopException(err);
    }
    source->setEventSource(sdEventSource);
    return source;
}

int TimeEventCallback(sd_event_source * /*unused*/, uint64_t usec,
                      void *userdata) {
    auto *source = static_cast<SDEventSourceTime *>(userdata);
    if (!source) {
        return 0;
    }
    try {
        auto callback = source->callback_;
        auto result = (*callback)(source, usec);
        return result ? 0 : -1;
    } catch (const std::exception &e) {
        // some abnormal things threw
        FCITX_ERROR() << e.what();
        abort();
    }
    return -1;
}

std::unique_ptr<EventSourceTime>
EventLoop::addTimeEvent(clockid_t clock, uint64_t usec, uint64_t accuracy,
                        TimeCallback callback) {
    FCITX_D();
    auto source = std::make_unique<SDEventSourceTime>(std::move(callback));
    sd_event_source *sdEventSource;
    if (int err = sd_event_add_time(d->event_, &sdEventSource, clock, usec,
                                    accuracy, TimeEventCallback, source.get());
        err < 0) {
        throw EventLoopException(err);
    }
    source->setEventSource(sdEventSource);
    return source;
}

int StaticEventCallback(sd_event_source * /*unused*/, void *userdata) {
    auto *source = static_cast<SDEventSource *>(userdata);
    if (!source) {
        return 0;
    }
    try {
        auto callback = source->callback_;
        auto result = (*callback)(source);
        return result ? 0 : -1;
    } catch (const std::exception &e) {
        // some abnormal things threw
        FCITX_ERROR() << e.what();
        abort();
    }
    return -1;
}

std::unique_ptr<EventSource> EventLoop::addExitEvent(EventCallback callback) {
    FCITX_D();
    auto source = std::make_unique<SDEventSource>(std::move(callback));
    sd_event_source *sdEventSource;
    if (int err = sd_event_add_exit(d->event_, &sdEventSource,
                                    StaticEventCallback, source.get());
        err < 0) {
        throw EventLoopException(err);
    }
    source->setEventSource(sdEventSource);
    return source;
}

std::unique_ptr<EventSource> EventLoop::addDeferEvent(EventCallback callback) {
    FCITX_D();
    auto source = std::make_unique<SDEventSource>(std::move(callback));
    sd_event_source *sdEventSource;
    if (int err = sd_event_add_defer(d->event_, &sdEventSource,
                                     StaticEventCallback, source.get());
        err < 0) {
        throw EventLoopException(err);
    }
    source->setEventSource(sdEventSource);
    return source;
}

std::unique_ptr<EventSource> EventLoop::addPostEvent(EventCallback callback) {
    FCITX_D();
    auto source = std::make_unique<SDEventSource>(std::move(callback));
    sd_event_source *sdEventSource;
    if (int err = sd_event_add_post(d->event_, &sdEventSource,
                                    StaticEventCallback, source.get());
        err < 0) {
        throw EventLoopException(err);
    }
    source->setEventSource(sdEventSource);
    return source;
}
} // namespace fcitx
