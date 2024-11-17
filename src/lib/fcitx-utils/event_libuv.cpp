/*
 * SPDX-FileCopyrightText: 2017-2017 Henry Hu
 * henry.hu.sh@gmail.com
 *
 * SPDX-License-Identifier: LGPL-2.1-or-later
 *
 */

#include "event_libuv.h"
#include <cstdint>
#include <cstdlib>
#include <ctime>
#include <exception>
#include <functional>
#include <memory>
#include <utility>
#include <vector>
#include <uv.h>
#include "event_p.h"
#include "eventloopinterface.h"
#include "log.h"
#include "trackableobject.h"

#define FCITX_LIBUV_DEBUG() FCITX_LOGC(::fcitx::libuv_logcategory, Debug)

namespace fcitx {

std::unique_ptr<EventLoopInterface> createDefaultEventLoop() {
    return std::make_unique<EventLoopLibUV>();
}

const char *defaultEventLoopImplementation() { return "libuv"; }

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

bool LibUVSourceTime::setup(uv_loop_t *loop, uv_timer_t *timer) {
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
bool LibUVSourcePost::setup(uv_loop_t *loop, uv_prepare_t *prepare) {
    if (int err = uv_prepare_init(loop, prepare); err < 0) {
        FCITX_LIBUV_DEBUG() << "Failed to init prepare with error: " << err;
        return false;
    }
    if (int err = uv_prepare_start(prepare, &PostEventCallback); err < 0) {
        FCITX_LIBUV_DEBUG() << "Failed to start prepare with error: " << err;
        return false;
    }
    return true;
}
bool LibUVSourceIO::setup(uv_loop_t *loop, uv_poll_t *poll) {
    if (int err = uv_poll_init(loop, poll, fd_); err < 0) {
        FCITX_LIBUV_DEBUG()
            << "Failed to init poll for fd: " << fd_ << " with error: " << err;
        return false;
    }
    const auto flags = IOEventFlagsToLibUVFlags(flags_);
    if (int err = uv_poll_start(poll, flags, &IOEventCallback); err < 0) {
        FCITX_LIBUV_DEBUG() << "Failed to start poll with error: " << err;
        return false;
    }
    return true;
}

EventLoopLibUV::EventLoopLibUV() : loop_(std::make_shared<UVLoop>()) {}

const char *EventLoopLibUV::implementation() const { return "libuv"; }

void *EventLoopLibUV::nativeHandle() {
    return static_cast<uv_loop_t *>(*loop_);
}

bool EventLoopLibUV::exec() {
    int r = uv_run(*loop_, UV_RUN_DEFAULT);
    for (auto iter = exitEvents_.begin(); iter != exitEvents_.end();) {
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
            iter = exitEvents_.erase(iter);
        } else {
            ++iter;
        }
    }
    return r >= 0;
}

void EventLoopLibUV::exit() { uv_stop(*loop_); }

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

std::unique_ptr<EventSourceIO>
EventLoopLibUV::addIOEvent(int fd, IOEventFlags flags, IOCallback callback) {
    auto source =
        std::make_unique<LibUVSourceIO>(std::move(callback), loop_, fd, flags);
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
EventLoopLibUV::addTimeEvent(clockid_t clock, uint64_t usec, uint64_t accuracy,
                             TimeCallback callback) {
    auto source = std::make_unique<LibUVSourceTime>(std::move(callback), loop_,
                                                    usec, clock, accuracy);
    return source;
}

std::unique_ptr<EventSource>
EventLoopLibUV::addExitEvent(EventCallback callback) {
    auto source = std::make_unique<LibUVSourceExit>(std::move(callback));
    exitEvents_.push_back(source->watch());
    return source;
}

std::unique_ptr<EventSource>
EventLoopLibUV::addDeferEvent(EventCallback callback) {
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
        auto ret = (*callback)(source);
        if (sourceRef.isValid()) {
            if (!ret) {
                source->setEnabled(false);
            }
        }
    } catch (const std::exception &e) {
        // some abnormal things threw{
        FCITX_FATAL() << e.what();
    }
}

std::unique_ptr<EventSource>
EventLoopLibUV::addPostEvent(EventCallback callback) {
    auto source = std::make_unique<LibUVSourcePost>(std::move(callback), loop_);
    return source;
}
} // namespace fcitx
