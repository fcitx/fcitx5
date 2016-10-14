/*
 * Copyright (C) 2015~2015 by CSSlayer
 * wengxt@gmail.com
 *
 * This library is free software; you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as
 * published by the Free Software Foundation; either version 2 of the
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
#ifndef _FCITX_UTILS_EVENT_H_
#define _FCITX_UTILS_EVENT_H_

#include "fcitxutils_export.h"
#include <cstring>
#include <fcitx-utils/flags.h>
#include <fcitx-utils/macros.h>
#include <functional>
#include <memory>
#include <time.h>

namespace fcitx {

enum class IOEventFlag {
    In = (1 << 0),
    Out = (1 << 1),
    Err = (1 << 2),
    Hup = (1 << 3),
    EdgeTrigger = (1 << 4),
};

typedef Flags<IOEventFlag> IOEventFlags;

class FCITXUTILS_EXPORT EventLoopException : public std::runtime_error {
public:
    EventLoopException(int error);

    int error() const { return errno_; }

private:
    int errno_;
};

struct FCITXUTILS_EXPORT EventSource {
    virtual ~EventSource();
    virtual bool isEnabled() const = 0;
    virtual void setEnabled(bool enabled) = 0;
    virtual bool isOneShot() const = 0;
    virtual void setOneShot() = 0;
};

struct FCITXUTILS_EXPORT EventSourceIO : public EventSource {
    virtual int fd() const = 0;
    virtual void setFd(int fd) = 0;
    virtual IOEventFlags events() const = 0;
    virtual void setEvents(IOEventFlags flags) = 0;
    virtual IOEventFlags revents() const = 0;
};

struct FCITXUTILS_EXPORT EventSourceTime : public EventSource {
    virtual void setNextInterval(uint64_t time);
    virtual uint64_t time() const = 0;
    virtual void setTime(uint64_t time) = 0;
    virtual uint64_t accuracy() const = 0;
    virtual void setAccuracy(uint64_t accuracy) = 0;
    virtual clockid_t clock() const = 0;
};

typedef std::function<bool(EventSource *, int fd, IOEventFlags flags)> IOCallback;
typedef std::function<bool(EventSource *, uint64_t usec)> TimeCallback;

FCITXUTILS_EXPORT uint64_t now(clockid_t clock);

class EventLoopPrivate;
class FCITXUTILS_EXPORT EventLoop {
public:
    EventLoop();
    virtual ~EventLoop();
    bool exec();
    void quit();

    const char *impl();
    void *nativeHandle();

    EventSourceIO *addIOEvent(int fd, IOEventFlags flags, IOCallback callback);
    EventSourceTime *addTimeEvent(clockid_t clock, uint64_t usec, uint64_t accuracy, TimeCallback callback);

private:
    std::unique_ptr<EventLoopPrivate> d_ptr;
    FCITX_DECLARE_PRIVATE(EventLoop);
};
}

#endif // _FCITX_UTILS_EVENT_H_
