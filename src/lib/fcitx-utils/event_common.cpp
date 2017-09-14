
/*
 * Copyright (C) 2015~2015 by CSSlayer
 * wengxt@gmail.com
 *
 * This library is free software; you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as
 * published by the Free Software Foundation; either version 2.1 of the
 * License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; see the file COPYING. If not,
 * see <http://www.gnu.org/licenses/>.
 */

#include "event.h"
#include <cstring>

#define USEC_INFINITY ((uint64_t)-1)
#define USEC_PER_SEC ((uint64_t)1000000ULL)
#define NSEC_PER_USEC ((uint64_t)1000ULL)

namespace fcitx {

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

EventSource::~EventSource() {}
}
