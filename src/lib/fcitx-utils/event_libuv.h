/*
 * SPDX-FileCopyrightText: 2020~2020 CSSlayer <wengxt@gmail.com>
 *
 * SPDX-License-Identifier: LGPL-2.1-or-later
 *
 */
#ifndef _FCITX_UTILS_EVENT_LIBUV_H_
#define _FCITX_UTILS_EVENT_LIBUV_H_

#include <cstdint>
#include <cstdlib>
#include <memory>
#include <utility>
#include <vector>
#include <fcitx-utils/eventloopinterface.h>
#include <fcitx-utils/log.h>
#include <fcitx-utils/macros.h>
#include <fcitx-utils/trackableobject.h>
#include <uv.h>

namespace fcitx {

struct UVLoop {
    UVLoop() { uv_loop_init(&loop_); }

    ~UVLoop();

    operator uv_loop_t *() { return &loop_; }

    uv_loop_t loop_;
};

enum class LibUVSourceEnableState { Disabled = 0, Oneshot = 1, Enabled = 2 };

struct LibUVSourceBase {
public:
    LibUVSourceBase(const std::shared_ptr<UVLoop> &loop) : loop_(loop) {}

    virtual ~LibUVSourceBase() { cleanup(); };
    void cleanup() {
        if (!handle_) {
            return;
        }
        auto *handle = handle_;
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
        : LibUVSource(std::move(loop)), fd_(fd), flags_(flags),
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

    bool setup(uv_loop_t *loop, uv_poll_t *poll) override;

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

    uint64_t time() const override { return time_; }

    void setTime(uint64_t time) override {
        time_ = time;
        resetEvent();
    }

    uint64_t accuracy() const override { return accuracy_; }

    void setAccuracy(uint64_t time) override { accuracy_ = time; }

    void setClock(clockid_t clockid) {
        clock_ = clockid;
        resetEvent();
    }

    clockid_t clock() const override { return clock_; }

    bool setup(uv_loop_t *loop, uv_timer_t *timer) override;

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

    bool setup(uv_loop_t *loop, uv_prepare_t *prepare) override;

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

struct LibUVSourceAsync final
    : public LibUVSource<EventSourceAsync, uv_async_t>,
      public TrackableObject<LibUVSourceAsync> {
    LibUVSourceAsync(EventCallback callback, std::shared_ptr<UVLoop> loop)
        : LibUVSource(std::move(loop)),
          callback_(std::make_shared<EventCallback>(std::move(callback))) {
        setEnabled(true);
    }

    bool setup(uv_loop_t *loop, uv_async_t *async) override;

    void send() override;

    std::shared_ptr<EventCallback> callback_;
};

class EventLoopLibUV : public EventLoopInterfaceV2 {
public:
    EventLoopLibUV();
    bool exec() override;
    void exit() override;
    const char *implementation() const override;
    void *nativeHandle() override;

    FCITX_NODISCARD std::unique_ptr<EventSourceIO>
    addIOEvent(int fd, IOEventFlags flags, IOCallback callback) override;
    FCITX_NODISCARD std::unique_ptr<EventSourceTime>
    addTimeEvent(clockid_t clock, uint64_t usec, uint64_t accuracy,
                 TimeCallback callback) override;
    FCITX_NODISCARD std::unique_ptr<EventSource>
    addExitEvent(EventCallback callback) override;
    FCITX_NODISCARD std::unique_ptr<EventSource>
    addDeferEvent(EventCallback callback) override;
    FCITX_NODISCARD std::unique_ptr<EventSource>
    addPostEvent(EventCallback callback) override;
    FCITX_NODISCARD std::unique_ptr<EventSourceAsync>
    addAsyncEvent(EventCallback callback) override;

private:
    std::shared_ptr<UVLoop> loop_;
    std::vector<TrackableObjectReference<LibUVSourceExit>> exitEvents_;
};

} // namespace fcitx

#endif // _FCITX_UTILS_EVENT_LIBUV_H_
