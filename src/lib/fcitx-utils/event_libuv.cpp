/*
 * SPDX-FileCopyrightText: 2017-2017 Henry Hu
 * henry.hu.sh@gmail.com
 *
 * SPDX-License-Identifier: LGPL-2.1-or-later
 *
 */

#include <ctime>
#include <exception>
#include <functional>
#include <memory>
#include <vector>
#include <uv.h>
#include "event.h"
#include "log.h"
#include "trackableobject.h"

#define FCITX_LIBUV_DEBUG() FCITX_LOGC(::fcitx::libuv_logcategory, Debug)

namespace fcitx {

FCITX_DEFINE_LOG_CATEGORY(libuv_logcategory, "libuv");

static int IOEventFlagsToLibUVFlags(IOEventFlags flags) {
    int result = 0;
    if (flags & IOEventFlag::In) {
        result |= UV_READABLE;
    }
    if (flags & IOEventFlag::Out) {
        result |= UV_WRITABLE;
    }
    if (flags & IOEventFlag::Hup) {
        result |= UV_DISCONNECT;
    }
    return result;
}

static IOEventFlags LibUVFlagsToIOEventFlags(int flags) {
    return ((flags & UV_READABLE) ? IOEventFlag::In : IOEventFlags()) |
           ((flags & UV_WRITABLE) ? IOEventFlag::Out : IOEventFlags()) |
           ((flags & UV_DISCONNECT) ? IOEventFlag::Hup : IOEventFlags());
}

void IOEventCallback(uv_poll_t *handle, int status, int events);
void TimeEventCallback(uv_timer_t *handle);
void PostEventCallback(uv_prepare_t *handle);

enum class LibUVSourceEnableState { Disabled = 0, Oneshot = 1, Enabled = 2 };

struct UVLoop {
    UVLoop() { uv_loop_init(&loop_); }

    ~UVLoop();

    operator uv_loop_t *() { return &loop_; }

    uv_loop_t loop_;
};

struct LibUVSourceBase {
public:
    LibUVSourceBase(std::shared_ptr<UVLoop> loop) : loop_(loop) {}

    virtual ~LibUVSourceBase() { cleanup(); };
    void cleanup() {
        if (!handle_) {
            return;
        }
        auto handle = handle_;
        handle_->data = nullptr;
        handle_ = nullptr;
        uv_close(handle, [](uv_handle_t *handle) { free(handle); });
    }

    virtual void init(uv_loop_t *loop) = 0;

    void resetEvent() {
        cleanup();
        if (state_ == LibUVSourceEnableState::Disabled) {
            return;
        }
        auto loop = loop_.lock();
        if (!loop) {
            return;
        }
        init(*loop);
    }

protected:
    void setState(LibUVSourceEnableState state) {
        if (state_ != state) {
            state_ = state;
            resetEvent();
        }
    }

    std::weak_ptr<UVLoop> loop_;
    uv_handle_t *handle_ = nullptr;
    LibUVSourceEnableState state_ = LibUVSourceEnableState::Disabled;
};

UVLoop::~UVLoop() {
    // Close and detach all handle.
    uv_walk(
        &loop_,
        [](uv_handle_t *handle, void *) {
            if (handle && !uv_is_closing(handle)) {
                if (handle->data) {
                    static_cast<LibUVSourceBase *>(handle->data)->cleanup();
                }
            }
        },
        nullptr);
    int r = uv_loop_close(&loop_);
    FCITX_DEBUG() << "UVLoop close: " << r;
    if (r == 0) {
        return;
    }
    do {
        r = uv_run(&loop_, UV_RUN_ONCE);
    } while (r != 0);
    // Now we're safe.
    r = uv_loop_close(&loop_);
    FCITX_DEBUG() << "UVLoop close r2: " << r;
}

template <typename Interface, typename HandleType>
struct LibUVSource : public Interface, public LibUVSourceBase {
public:
    LibUVSource(std::shared_ptr<UVLoop> loop)
        : LibUVSourceBase(std::move(loop)) {}

    bool isEnabled() const override {
        return state_ != LibUVSourceEnableState::Disabled;
    }
    void setEnabled(bool enabled) override {
        auto newState = enabled ? LibUVSourceEnableState::Enabled
                                : LibUVSourceEnableState::Disabled;
        setState(newState);
    }

    void setOneShot() override { setState(LibUVSourceEnableState::Oneshot); }

    bool isOneShot() const override {
        return state_ == LibUVSourceEnableState::Oneshot;
    }

    inline HandleType *handle() {
        return reinterpret_cast<HandleType *>(handle_);
    }

    void init(uv_loop_t *loop) override {
        handle_ = static_cast<uv_handle_t *>(calloc(1, sizeof(HandleType)));
        handle_->data = static_cast<LibUVSourceBase *>(this);
        if (!setup(loop, handle())) {
            free(handle_);
            handle_ = nullptr;
        }
    }

    virtual bool setup(uv_loop_t *loop, HandleType *handle) = 0;
};

struct LibUVSourceIO final : public LibUVSource<EventSourceIO, uv_poll_t>,
                             public TrackableObject<LibUVSourceIO> {
    LibUVSourceIO(IOCallback _callback, std::shared_ptr<UVLoop> loop, int fd,
                  IOEventFlags flags)
        : LibUVSource(loop), fd_(fd), flags_(flags),
          callback_(std::make_shared<IOCallback>(std::move(_callback))) {
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

    IOEventFlags revents() const override { return revents_; }

    bool setup(uv_loop_t *loop, uv_poll_t *poll) override {
        if (int err = uv_poll_init(loop, poll, fd_); err < 0) {
            FCITX_LIBUV_DEBUG() << "Failed to init poll for fd: " << fd_
                                << " with error: " << err;
            return false;
        }
        const auto flags = IOEventFlagsToLibUVFlags(flags_);
        if (int err = uv_poll_start(poll, flags, &IOEventCallback); err < 0) {
            FCITX_LIBUV_DEBUG() << "Failed to start poll with error: " << err;
            return false;
        }
        return true;
    }

    int fd_;
    IOEventFlags flags_;
    IOEventFlags revents_;
    std::shared_ptr<IOCallback> callback_;
};

struct LibUVSourceTime final : public LibUVSource<EventSourceTime, uv_timer_t>,
                               public TrackableObject<LibUVSourceTime> {
    LibUVSourceTime(TimeCallback _callback, std::shared_ptr<UVLoop> loop,
                    uint64_t time, clockid_t clockid, uint64_t accuracy)
        : LibUVSource(std::move(loop)), time_(time), clock_(clockid),
          accuracy_(accuracy),
          callback_(std::make_shared<TimeCallback>(std::move(_callback))) {
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

    bool setup(uv_loop_t *loop, uv_timer_t *timer) override {
        if (int err = uv_timer_init(loop, timer); err < 0) {
            FCITX_LIBUV_DEBUG() << "Failed to init timer with error: " << err;
            return false;
        }
        auto curr = now(clock_);
        uint64_t timeout = time_ > curr ? (time_ - curr) : 0;
        // libuv is milliseconds
        timeout /= 1000;
        if (int err = uv_timer_start(timer, &TimeEventCallback, timeout, 0);
            err < 0) {
            FCITX_LIBUV_DEBUG() << "Failed to start timer with error: " << err;
            return false;
        }
        return true;
    }

    uint64_t time_;
    clockid_t clock_;
    uint64_t accuracy_;
    std::shared_ptr<TimeCallback> callback_;
};

struct LibUVSourcePost final : public LibUVSource<EventSource, uv_prepare_t>,
                               public TrackableObject<LibUVSourcePost> {
    LibUVSourcePost(EventCallback callback, std::shared_ptr<UVLoop> loop)
        : LibUVSource(std::move(loop)),
          callback_(std::make_shared<EventCallback>(std::move(callback))) {
        setEnabled(true);
    }

    bool setup(uv_loop_t *loop, uv_prepare_t *prepare) override {
        if (int err = uv_prepare_init(loop, prepare); err < 0) {
            FCITX_LIBUV_DEBUG() << "Failed to init prepare with error: " << err;
            return false;
        }
        if (int err = uv_prepare_start(prepare, &PostEventCallback); err < 0) {
            FCITX_LIBUV_DEBUG()
                << "Failed to start prepare with error: " << err;
            return false;
        }
        return true;
    }

    std::shared_ptr<EventCallback> callback_;
};

struct LibUVSourceExit final : public EventSource,
                               public TrackableObject<LibUVSourceExit> {
    LibUVSourceExit(EventCallback _callback)
        : callback_(std::move(_callback)) {}

    bool isOneShot() const override {
        return state_ == LibUVSourceEnableState::Oneshot;
    }
    bool isEnabled() const override {
        return state_ != LibUVSourceEnableState::Disabled;
    }
    void setEnabled(bool enabled) override {
        state_ = enabled ? LibUVSourceEnableState::Enabled
                         : LibUVSourceEnableState::Disabled;
    }

    void setOneShot() override { state_ = LibUVSourceEnableState::Oneshot; }

    LibUVSourceEnableState state_ = LibUVSourceEnableState::Oneshot;
    EventCallback callback_;
};

class EventLoopPrivate {
public:
    EventLoopPrivate() : loop_(std::make_shared<UVLoop>()) {}

    std::shared_ptr<UVLoop> loop_;
    std::vector<TrackableObjectReference<LibUVSourceExit>> exitEvents_;
};

EventLoop::EventLoop() : d_ptr(std::make_unique<EventLoopPrivate>()) {}

EventLoop::~EventLoop() {}

const char *EventLoop::impl() { return "libuv"; }

void *EventLoop::nativeHandle() {
    FCITX_D();
    return static_cast<uv_loop_t *>(*d->loop_);
}

bool EventLoop::exec() {
    FCITX_D();
    int r = uv_run(*d->loop_, UV_RUN_DEFAULT);
    for (auto iter = d->exitEvents_.begin(); iter != d->exitEvents_.end();) {
        if (auto *event = iter->get()) {
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
    uv_stop(*d->loop_);
}

void IOEventCallback(uv_poll_t *handle, int status, int events) {
    auto *source = static_cast<LibUVSourceIO *>(
        static_cast<LibUVSourceBase *>(handle->data));
    auto sourceRef = source->watch();
    try {
        if (source->isOneShot()) {
            source->setEnabled(false);
        }
        auto flags = LibUVFlagsToIOEventFlags(events);
        if (status < 0) {
            flags |= IOEventFlag::Err;
        }
        auto callback = source->callback_;
        bool ret = (*callback)(source, source->fd(), flags);
        if (sourceRef.isValid()) {
            if (!ret) {
                source->setEnabled(false);
            }
        }
    } catch (const std::exception &e) {
        // some abnormal things threw
        FCITX_FATAL() << e.what();
    }
}

std::unique_ptr<EventSourceIO> EventLoop::addIOEvent(int fd, IOEventFlags flags,
                                                     IOCallback callback) {
    FCITX_D();
    auto source = std::make_unique<LibUVSourceIO>(std::move(callback), d->loop_,
                                                  fd, flags);
    return source;
}

void TimeEventCallback(uv_timer_t *handle) {
    auto *source = static_cast<LibUVSourceTime *>(
        static_cast<LibUVSourceBase *>(handle->data));

    try {
        auto sourceRef = source->watch();
        if (source->isOneShot()) {
            source->setEnabled(false);
        }
        auto callback = source->callback_;
        bool ret = (*callback)(source, source->time());
        if (sourceRef.isValid()) {
            if (!ret) {
                source->setEnabled(false);
            }
            if (source->isEnabled()) {
                source->resetEvent();
            }
        }
    } catch (const std::exception &e) {
        // some abnormal things threw
        FCITX_FATAL() << e.what();
    }
}

std::unique_ptr<EventSourceTime>
EventLoop::addTimeEvent(clockid_t clock, uint64_t usec, uint64_t accuracy,
                        TimeCallback callback) {
    FCITX_D();
    auto source = std::make_unique<LibUVSourceTime>(
        std::move(callback), d->loop_, usec, clock, accuracy);
    return source;
}

std::unique_ptr<EventSource> EventLoop::addExitEvent(EventCallback callback) {
    FCITX_D();
    auto source = std::make_unique<LibUVSourceExit>(std::move(callback));
    d->exitEvents_.push_back(source->watch());
    return source;
}

std::unique_ptr<EventSource> EventLoop::addDeferEvent(EventCallback callback) {
    return addTimeEvent(
        CLOCK_MONOTONIC, 0, 0,
        [callback = std::move(callback)](EventSourceTime *source, uint64_t) {
            return callback(source);
        });
}

void PostEventCallback(uv_prepare_t *handle) {
    auto *source = static_cast<LibUVSourcePost *>(
        static_cast<LibUVSourceBase *>(handle->data));

    try {
        auto sourceRef = source->watch();
        if (source->isOneShot()) {
            source->setEnabled(false);
        }
        auto callback = source->callback_;
        (*callback)(source);
    } catch (const std::exception &e) {
        // some abnormal things threw{
        FCITX_FATAL() << e.what();
    }
}

std::unique_ptr<EventSource> EventLoop::addPostEvent(EventCallback callback) {
    FCITX_D();
    auto source =
        std::make_unique<LibUVSourcePost>(std::move(callback), d->loop_);
    return source;
}

} // namespace fcitx
