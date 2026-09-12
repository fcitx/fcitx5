/*
 * SPDX-FileCopyrightText: 2026 CSSlayer <wengxt@gmail.com>
 *
 * SPDX-License-Identifier: LGPL-2.1-or-later
 *
 */
#ifndef _FCITX_UTILS_AWAITER_H_
#define _FCITX_UTILS_AWAITER_H_

#include <cstdint>
#include <memory>
#include <coroutine>
#include <fcitx-utils/event.h>
#include <fcitx-utils/eventloopinterface.h>

namespace fcitx {

/**
 * @brief Await an event loop time event.
 *
 * The pending time event is cancelled if the awaiter is destroyed before it
 * fires. It must only be used from the same thread as the event loop.
 *
 * @since 5.1.23
 */
class TimeAwaiter {
public:
    /**
     * Create an awaiter for a monotonic-clock relative delay.
     *
     * @param eventLoop event loop on which to create the time event.
     * @param offset delay in microseconds.
     * @param accuracy requested timer accuracy in microseconds.
     * @return an awaiter that resumes after the delay.
     */
    static TimeAwaiter after(EventLoop &eventLoop, uint64_t offset,
                             uint64_t accuracy = 0) {
        return {eventLoop, CLOCK_MONOTONIC, now(CLOCK_MONOTONIC) + offset,
                accuracy};
    }

    /**
     * Create an awaiter for an absolute time.
     *
     * @param eventLoop event loop on which to create the time event.
     * @param clock clock used for @p time.
     * @param time absolute time in microseconds.
     * @param accuracy requested timer accuracy in microseconds.
     * @return an awaiter that resumes at the requested time.
     */
    static TimeAwaiter at(EventLoop &eventLoop, clockid_t clock, uint64_t time,
                          uint64_t accuracy = 0) {
        return {eventLoop, clock, time, accuracy};
    }

    bool await_ready() const noexcept { return false; }

    void await_suspend(std::coroutine_handle<> continuation) {
        source_ = eventLoop_.addTimeEvent(
            clock_, time_, accuracy_,
            [this, continuation](EventSourceTime *, uint64_t time) {
                firedTime_ = time;
                continuation.resume();
                return true;
            });
    }

    /**
     * @return the time at which the event fired, in microseconds.
     */
    uint64_t await_resume() const noexcept { return firedTime_; }

private:
    TimeAwaiter(EventLoop &eventLoop, clockid_t clock, uint64_t time,
                uint64_t accuracy)
        : eventLoop_(eventLoop), clock_(clock), time_(time),
          accuracy_(accuracy) {}
    EventLoop &eventLoop_;
    clockid_t clock_;
    uint64_t time_;
    uint64_t accuracy_;
    uint64_t firedTime_ = 0;
    std::unique_ptr<EventSourceTime> source_;
};

} // namespace fcitx

#endif // _FCITX_UTILS_AWAITER_H_
