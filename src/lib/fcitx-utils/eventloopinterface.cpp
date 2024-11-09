/*
 * SPDX-FileCopyrightText: 2015-2024 CSSlayer <wengxt@gmail.com>
 *
 * SPDX-License-Identifier: LGPL-2.1-or-later
 *
 */
#include "eventloopinterface.h"
#include <cstdint>
#include <cstring>
#include <ctime>
#include <stdexcept>

namespace fcitx {

namespace {

constexpr uint64_t USEC_INFINITY = static_cast<uint64_t>(-1);
constexpr uint64_t USEC_PER_SEC = 1000000ULL;
constexpr uint64_t NSEC_PER_USEC = 1000ULL;

} // namespace

// From systemd :)
uint64_t timespec_load(const struct timespec *ts) {
    if (ts->tv_sec == (time_t)-1 && ts->tv_nsec == (long)-1) {
        return USEC_INFINITY;
    }

    if ((uint64_t)ts->tv_sec >
        (UINT64_MAX - (ts->tv_nsec / NSEC_PER_USEC)) / USEC_PER_SEC) {
        return USEC_INFINITY;
    }

    return (uint64_t)ts->tv_sec * USEC_PER_SEC +
           (uint64_t)ts->tv_nsec / NSEC_PER_USEC;
}

uint64_t now(clockid_t clock_id) {
    struct timespec ts;
    clock_gettime(clock_id, &ts);

    return timespec_load(&ts);
}

EventLoopException::EventLoopException(int error)
    : std::runtime_error(std::strerror(error)), errno_(error) {}

void EventSourceTime::setNextInterval(uint64_t time) {
    setTime(now(clock()) + time);
}

EventSource::~EventSource() = default;

EventLoopInterface::~EventLoopInterface() = default;

} // namespace fcitx
