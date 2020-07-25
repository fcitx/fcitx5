/*
 * SPDX-FileCopyrightText: 2017-2017 Henry Hu
 * henry.hu.sh@gmail.com
 *
 * SPDX-License-Identifier: LGPL-2.1-or-later
 *
 */

#include "event.h"
#include <exception>
#include <functional>
#include <vector>
#include <event2/event.h>
#include "trackableobject.h"

namespace fcitx {

static uint32_t IOEventFlagsToLibEventFlags(IOEventFlags flags) {
    uint32_t result = 0;
    if (flags & IOEventFlag::In) {
        result |= EV_READ;
    }
    if (flags & IOEventFlag::Out) {
        result |= EV_WRITE;
    }
    /*
    if (flags & IOEventFlag::Err) {
        result |= EV_READ | EV_WRITE;
    }
    if (flags & IOEventFlag::Hup) {
        result |= EV_CLOSED;
    }
    */
    if (flags & IOEventFlag::EdgeTrigger) {
        result |= EV_ET;
    }
    return result;
}

static IOEventFlags LibEventFlagsToIOEventFlags(uint32_t flags) {
    return ((flags & EV_READ) ? IOEventFlag::In : IOEventFlags()) |
           ((flags & EV_WRITE) ? IOEventFlag::Out : IOEventFlags()) |
           // ((flags & EV_CLOSED) ? IOEventFlag::Hup : IOEventFlags()) |
           ((flags & EV_ET) ? IOEventFlag::EdgeTrigger : IOEventFlags());
}

static void EventTimeToTimeval(uint64_t usec, clockid_t clock,
                               struct timeval *tv) {
    // When usec = 0, we trigger it immediately.
    if (usec == 0) {
        tv->tv_sec = 0;
        tv->tv_usec = 0;
        return;
    }
    uint64_t curr = now(clock);
    if (curr > usec) {
        tv->tv_sec = 0;
        tv->tv_usec = 0;
    } else {
        usec = usec - curr;
        tv->tv_sec = usec / 1000000;
        tv->tv_usec = usec % 1000000;
    }
}

void IOEventCallback(evutil_socket_t, short, void *);
void TimeEventCallback(evutil_socket_t, short, void *);

enum class LibEventSourceEnableState { Disabled = 0, Oneshot = 1, Enabled = 2 };

template <typename Interface>
struct LibEventSourceBase : public Interface {
public:
    LibEventSourceBase(event_base *eventBase) : eventBase_(eventBase) {}

    ~LibEventSourceBase() = default;

    bool isEnabled() const override {
        return state_ != LibEventSourceEnableState::Disabled;
    }
    void setEnabled(bool enabled) override {
        auto newState = enabled ? LibEventSourceEnableState::Enabled
                                : LibEventSourceEnableState::Disabled;
        setState(newState);
    }

    virtual void resetEvent() = 0;

    void setOneShot() override { setState(LibEventSourceEnableState::Oneshot); }

    bool isOneShot() const override {
        return state_ == LibEventSourceEnableState::Oneshot;
    }

protected:
    event_base *eventBase_; // not owned
    UniqueCPtr<event, event_free> event_;
    LibEventSourceEnableState state_ = LibEventSourceEnableState::Disabled;

private:
    void setState(LibEventSourceEnableState state) {
        if (state_ != state) {
            state_ = state;
            resetEvent();
        }
    }
};

struct LibEventSourceIO final : public LibEventSourceBase<EventSourceIO> {
    LibEventSourceIO(IOCallback _callback, event_base *eventBase, int fd,
                     IOEventFlags flags)
        : LibEventSourceBase(eventBase), fd_(fd), flags_(flags),
          callback_(_callback) {
        setEnabled(true);
    }

    virtual int fd() const override { return fd_; }

    virtual void setFd(int fd) override {
        if (fd_ != fd) {
            fd_ = fd;
            resetEvent();
        }
    }

    virtual IOEventFlags events() const override { return flags_; }

    void setEvents(IOEventFlags flags) override {
        if (flags_ != flags) {
            flags_ = flags;
            resetEvent();
        }
    }

    IOEventFlags revents() const override {
        IOEventFlags revents;

        if (flags_ & IOEventFlag::In) {
            if (event_pending(event_.get(), EV_READ, nullptr)) {
                revents |= IOEventFlag::In;
            }
        }
        if (flags_ & IOEventFlag::Out) {
            if (event_pending(event_.get(), EV_WRITE, nullptr)) {
                revents |= IOEventFlag::Out;
            }
        }

        return revents;
    }

    void resetEvent() override {
        // event_del if event_ is not null, so we can use event_assign later.
        if (event_) {
            event_del(event_.get());
        }
        if (!isEnabled()) {
            return;
        }
        short flags = IOEventFlagsToLibEventFlags(flags_);
        if (state_ != LibEventSourceEnableState::Oneshot) {
            flags |= EV_PERSIST;
        }
        // flags |= EV_CLOSED;
        if (!event_) {
            event_.reset(
                event_new(eventBase_, fd_, flags, IOEventCallback, this));
            if (!event_) {
                throw EventLoopException(ENOMEM);
            }
        } else {
            event_assign(event_.get(), eventBase_, fd_, flags, IOEventCallback,
                         this);
        }
        event_add(event_.get(), nullptr);
    }

    int fd_;
    IOEventFlags flags_;
    IOCallback callback_;
};

struct LibEventSourceTime final : public LibEventSourceBase<EventSourceTime>,
                                  public TrackableObject<LibEventSourceTime> {
    LibEventSourceTime(TimeCallback _callback, event_base *eventBase,
                       uint64_t time, clockid_t clockid, uint64_t accuracy)
        : LibEventSourceBase(eventBase), time_(time), clock_(clockid),
          accuracy_(accuracy), callback_(std::move(_callback)) {
        setOneShot();
    }

    virtual uint64_t time() const override { return time_; }

    virtual void setTime(uint64_t time) override {
        time_ = time;
        resetEvent();
    }

    virtual uint64_t accuracy() const override { return accuracy_; }

    virtual void setAccuracy(uint64_t time) override { accuracy_ = time; }

    void setClock(clockid_t clockid) {
        clock_ = clockid;
        resetEvent();
    }

    virtual clockid_t clock() const override { return clock_; }

    virtual void resetEvent() override {
        if (event_) {
            event_del(event_.get());
        }
        if (!isEnabled()) {
            return;
        }
        if (!event_) {
            event_.reset(
                event_new(eventBase_, -1, EV_TIMEOUT, TimeEventCallback, this));
            if (!event_) {
                throw EventLoopException(ENOMEM);
            }
        }
        struct timeval tv;
        EventTimeToTimeval(time_, clock_, &tv);
        event_add(event_.get(), &tv);
    }

    uint64_t time_;
    clockid_t clock_;
    uint64_t accuracy_;
    TimeCallback callback_;
};

struct LibEventSourceExit final : public EventSource,
                                  public TrackableObject<LibEventSourceExit> {
    LibEventSourceExit(EventCallback _callback)
        : callback_(std::move(_callback)) {}

    bool isOneShot() const override {
        return state_ == LibEventSourceEnableState::Oneshot;
    }
    bool isEnabled() const override {
        return state_ != LibEventSourceEnableState::Disabled;
    }
    void setEnabled(bool enabled) override {
        state_ = enabled ? LibEventSourceEnableState::Enabled
                         : LibEventSourceEnableState::Disabled;
    }

    void setOneShot() override { state_ = LibEventSourceEnableState::Oneshot; }

    LibEventSourceEnableState state_ = LibEventSourceEnableState::Oneshot;
    EventCallback callback_;
};

class EventLoopPrivate {
public:
    EventLoopPrivate() {
        event_config *config = event_config_new();
        if (config == nullptr) {
            throw std::runtime_error("Create event_config failed.");
        }
        event_config_require_features(config, EV_FEATURE_ET);

        event_ = event_base_new_with_config(config);
        if (event_ == nullptr) {
            throw std::runtime_error("Create event_base failed.");
        }
        event_config_free(config);
    }

    ~EventLoopPrivate() { event_base_free(event_); }

    event_base *event_;
    std::vector<TrackableObjectReference<LibEventSourceExit>> exitEvents_;
};

EventLoop::EventLoop() : d_ptr(std::make_unique<EventLoopPrivate>()) {}

EventLoop::~EventLoop() {}

const char *EventLoop::impl() { return "libevent"; }

void *EventLoop::nativeHandle() {
    FCITX_D();
    return d->event_;
}

bool EventLoop::exec() {
    FCITX_D();
#ifdef EVLOOP_NO_EXIT_ON_EMPTY
    int r = event_base_loop(d->event_, EVLOOP_NO_EXIT_ON_EMPTY);
#else
    UniqueCPtr<event, event_free> dummy(event_new(
        d->event_, -1, EV_PERSIST, [](evutil_socket_t, short, void *) {},
        nullptr));
    struct timeval tv;
    tv.tv_sec = 1000000000;
    tv.tv_usec = 0;
    event_add(dummy.get(), &tv);
    int r = event_base_loop(d->event_, 0);
#endif
    for (auto iter = d->exitEvents_.begin(); iter != d->exitEvents_.end();) {
        if (auto event = iter->get()) {
            if (event->isEnabled()) {
                try {
                    if (event->isOneShot()) {
                        event->setEnabled(false);
                    }
                    event->callback_(event);
                } catch (const std::exception &e) {
                    // some abnormal things threw
                    abort();
                }
            }
        }
        if (!iter->isValid()) {
            iter = d->exitEvents_.erase(iter);
        } else {
            ++iter;
        }
    }
    return r >= 0;
}

void EventLoop::exit() {
    FCITX_D();
    event_base_loopexit(d->event_, nullptr);
}

void IOEventCallback(evutil_socket_t fd, short events, void *arg) {
    auto source = static_cast<LibEventSourceIO *>(arg);
    try {
        if (source->isOneShot()) {
            source->setEnabled(false);
        }
        source->callback_(source, fd, LibEventFlagsToIOEventFlags(events));
    } catch (const std::exception &e) {
        // some abnormal things threw
        abort();
    }
}

std::unique_ptr<EventSourceIO> EventLoop::addIOEvent(int fd, IOEventFlags flags,
                                                     IOCallback callback) {
    FCITX_D();
    auto source =
        std::make_unique<LibEventSourceIO>(callback, d->event_, fd, flags);
    return source;
}

void TimeEventCallback(evutil_socket_t, short, void *arg) {

    auto source = static_cast<LibEventSourceTime *>(arg);

    try {
        auto sourceRef = source->watch();
        if (source->isOneShot()) {
            source->setEnabled(false);
        }
        source->callback_(source, source->time());
        if (sourceRef.isValid() && source->isEnabled()) {
            source->resetEvent();
        }
    } catch (const std::exception &e) {
        // some abnormal things threw
        abort();
    }
}

std::unique_ptr<EventSourceTime>
EventLoop::addTimeEvent(clockid_t clock, uint64_t usec, uint64_t accuracy,
                        TimeCallback callback) {
    FCITX_D();
    auto source = std::make_unique<LibEventSourceTime>(callback, d->event_,
                                                       usec, clock, accuracy);
    return source;
}

std::unique_ptr<EventSource> EventLoop::addExitEvent(EventCallback callback) {
    FCITX_D();
    auto source = std::make_unique<LibEventSourceExit>(callback);
    d->exitEvents_.push_back(source->watch());
    return source;
}

bool DeferEventCallback(EventSourceTime *source, uint64_t,
                        EventCallback callback) {
    return callback(source);
}

std::unique_ptr<EventSource> EventLoop::addDeferEvent(EventCallback callback) {
    return addTimeEvent(CLOCK_MONOTONIC, 0, 0,
                        std::bind(DeferEventCallback, std::placeholders::_1,
                                  std::placeholders::_2, callback));
}
} // namespace fcitx
