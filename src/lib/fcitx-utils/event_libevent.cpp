#include <event2/event.h>
#include <exception>
#include <functional>
#include <vector>

#if defined(__COVERITY__) && !defined(__INCLUDE_LEVEL__)
#define __INCLUDE_LEVEL__ 2
#endif
#include "event.h"

#ifndef __unused
#define __unused __attribute__((unused))
#endif

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

template <typename Interface>
struct LibEventSourceBase : public Interface {
public:
    LibEventSourceBase(event_base *eventBase)
        : eventBase_(eventBase), event_(nullptr), enabled_(false),
          oneShot_(false) {}

    ~LibEventSourceBase() {
        if (event_) {
            setEnabled(false);
        }
    }

    virtual bool isEnabled() const override { return enabled_; }

    virtual void setEnabled(bool enabled) override {
        if (enabled_ == enabled)
            return;

        int ret;
        if (enabled) {
            ret = enableEvent(false);
        } else {
            ret = event_del(event_);
            event_free(event_);
            event_ = nullptr;
        }
        if (ret < 0) {
            throw EventLoopException(1);
        }
        enabled_ = enabled;
    }

    void reset() {
        if (enabled_) {
            setEnabled(false);
            setEnabled(true);
        }
    }

    virtual void setOneShot() override {
        oneShot_ = true;
        enableEvent(true);
    }

    virtual bool isOneShot() const override { return oneShot_; }

    virtual int enableEvent(bool oneShot) = 0;

protected:
    event_base *eventBase_; // not owned
    event *event_;
    bool enabled_;
    bool oneShot_;
};

struct LibEventSourceIO : public LibEventSourceBase<EventSourceIO> {
    LibEventSourceIO(IOCallback _callback, event_base *eventBase, int fd,
                     IOEventFlags flags)
        : LibEventSourceBase(eventBase), fd_(fd), flags_(flags),
          callback_(_callback) {}

    virtual int fd() const override { return fd_; }

    virtual void setFd(int fd) override {
        fd_ = fd;
        reset();
    }

    virtual IOEventFlags events() const override { return flags_; }

    virtual void setEvents(IOEventFlags flags) override {
        flags_ = flags;
        reset();
    }

    virtual IOEventFlags revents() const override {
        IOEventFlags revents;

        if (flags_ & IOEventFlag::In) {
            if (event_pending(event_, EV_READ, nullptr)) {
                revents |= IOEventFlag::In;
            }
        }
        if (flags_ & IOEventFlag::Out) {
            if (event_pending(event_, EV_WRITE, nullptr)) {
                revents |= IOEventFlag::Out;
            }
        }

        return revents;
    }

    virtual int enableEvent(bool oneShot) override {
        short flags = IOEventFlagsToLibEventFlags(flags_);
        if (!oneShot) {
            flags |= EV_PERSIST;
        }
        // flags |= EV_CLOSED;
        event *event = event_new(eventBase_, fd_, flags, IOEventCallback, this);
        if (event == nullptr) {
            throw EventLoopException(ENOMEM);
        }
        event_ = event;
        return event_add(event, nullptr);
    }

    int fd_;
    IOEventFlags flags_;
    IOCallback callback_;
};

struct LibEventSourceTime : public LibEventSourceBase<EventSourceTime> {
    LibEventSourceTime(TimeCallback _callback, event_base *eventBase,
                       uint64_t time, clockid_t clockid, uint64_t accuracy)
        : LibEventSourceBase(eventBase), time_(time), clock_(clockid),
          accuracy_(accuracy), callback_(_callback) {}

    virtual uint64_t time() const override { return time_; }

    virtual void setTime(uint64_t time) override {
        time_ = time;
        reset();
    }

    virtual uint64_t accuracy() const override { return accuracy_; }

    virtual void setAccuracy(uint64_t time) override { accuracy_ = time; }

    void setClock(clockid_t clockid) { clock_ = clockid; }

    virtual clockid_t clock() const override { return clock_; }

    virtual int enableEvent(bool oneShot __unused) override {
        // Because time_ is absolute time, I have no idea what a ``repeative
        // time event'' should be. Thus, ignore ``oneShot'' for now.
        event *event = event_new(eventBase_, -1, 0, TimeEventCallback, this);
        if (event == nullptr) {
            throw EventLoopException(ENOMEM);
        }
        event_ = event;
        struct timeval tv;
        EventTimeToTimeval(time_, clock_, &tv);
        return event_add(event, &tv);
    }

    uint64_t time_;
    clockid_t clock_;
    uint64_t accuracy_;
    TimeCallback callback_;
};

struct LibEventSourceExit : public LibEventSourceBase<EventSource> {
    LibEventSourceExit(EventCallback _callback, event_base *eventBase)
        : LibEventSourceBase(eventBase), callback_(_callback) {}

    virtual int enableEvent(bool oneShot __unused) override { return 0; }

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
    std::vector<LibEventSourceExit *> exitEvents_;
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
    event *dummy = event_new(d->event_, -1, EV_PERSIST,
                             [](evutil_socket_t, short, void *) {}, nullptr);
    struct timeval tv;
    tv.tv_sec = 1000000000;
    tv.tv_usec = 0;
    event_add(dummy, &tv);
    int r = event_base_loop(d->event_, 0);
#endif
    for (auto exitEvent : d->exitEvents_) {
        try {
            exitEvent->callback_(exitEvent);
        } catch (const std::exception &e) {
            // some abnormal things threw
            abort();
        }
    }
    return r >= 0;
}

void EventLoop::quit() {
    FCITX_D();
    event_base_loopexit(d->event_, nullptr);
}

void IOEventCallback(evutil_socket_t fd, short events, void *arg) {
    auto source = static_cast<LibEventSourceIO *>(arg);
    try {
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
    source->setEnabled(true);
    return source;
}

void TimeEventCallback(evutil_socket_t fd __unused, short events __unused,
                       void *arg) {

    auto source = static_cast<LibEventSourceTime *>(arg);

    try {
        source->callback_(source, source->time());
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
    source->setEnabled(true);
    return source;
}

std::unique_ptr<EventSource> EventLoop::addExitEvent(EventCallback callback) {
    FCITX_D();
    auto source = std::make_unique<LibEventSourceExit>(callback, d->event_);
    d->exitEvents_.push_back(source.get());
    return source;
}

bool DeferEventCallback(EventSourceTime *source, uint64_t usec __unused,
                        EventCallback callback) {
    return callback(source);
}

std::unique_ptr<EventSource> EventLoop::addDeferEvent(EventCallback callback) {
    return addTimeEvent(CLOCK_MONOTONIC, 0, 1,
                        std::bind(DeferEventCallback, std::placeholders::_1,
                                  std::placeholders::_2, callback));
}
}
