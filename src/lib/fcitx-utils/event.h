/*
 * SPDX-FileCopyrightText: 2015-2015 CSSlayer <wengxt@gmail.com>
 *
 * SPDX-License-Identifier: LGPL-2.1-or-later
 *
 */
#ifndef _FCITX_UTILS_EVENT_H_
#define _FCITX_UTILS_EVENT_H_

#include <cstring>
#include <ctime>
#include <functional>
#include <memory>
#include <stdexcept>
#include <fcitx-utils/flags.h>
#include <fcitx-utils/macros.h>
#include "fcitxutils_export.h"

namespace fcitx {

enum class IOEventFlag {
    In = (1 << 0),
    Out = (1 << 1),
    Err = (1 << 2),
    Hup = (1 << 3),
    EdgeTrigger = (1 << 4),
};

using IOEventFlags = Flags<IOEventFlag>;

class FCITXUTILS_EXPORT EventLoopException : public std::runtime_error {
public:
    EventLoopException(int error);

    FCITX_NODISCARD int error() const { return errno_; }

private:
    int errno_;
};

struct FCITXUTILS_EXPORT EventSource {
    virtual ~EventSource();
    FCITX_NODISCARD virtual bool isEnabled() const = 0;
    virtual void setEnabled(bool enabled) = 0;
    FCITX_NODISCARD virtual bool isOneShot() const = 0;
    virtual void setOneShot() = 0;
};

struct FCITXUTILS_EXPORT EventSourceIO : public EventSource {
    FCITX_NODISCARD virtual int fd() const = 0;
    virtual void setFd(int fd) = 0;
    FCITX_NODISCARD virtual IOEventFlags events() const = 0;
    virtual void setEvents(IOEventFlags flags) = 0;
    FCITX_NODISCARD virtual IOEventFlags revents() const = 0;
};

struct FCITXUTILS_EXPORT EventSourceTime : public EventSource {
    virtual void setNextInterval(uint64_t time);
    FCITX_NODISCARD virtual uint64_t time() const = 0;
    virtual void setTime(uint64_t time) = 0;
    FCITX_NODISCARD virtual uint64_t accuracy() const = 0;
    virtual void setAccuracy(uint64_t accuracy) = 0;
    FCITX_NODISCARD virtual clockid_t clock() const = 0;
};

using IOCallback =
    std::function<bool(EventSourceIO *, int fd, IOEventFlags flags)>;
using TimeCallback = std::function<bool(EventSourceTime *, uint64_t usec)>;
using EventCallback = std::function<bool(EventSource *)>;

FCITXUTILS_EXPORT uint64_t now(clockid_t clock);

class EventLoopPrivate;
class FCITXUTILS_EXPORT EventLoop {
public:
    EventLoop();
    virtual ~EventLoop();
    bool exec();
    void exit();

    static const char *impl();
    void *nativeHandle();

    FCITX_NODISCARD std::unique_ptr<EventSourceIO>
    addIOEvent(int fd, IOEventFlags flags, IOCallback callback);
    FCITX_NODISCARD std::unique_ptr<EventSourceTime>
    addTimeEvent(clockid_t clock, uint64_t usec, uint64_t accuracy,
                 TimeCallback callback);
    FCITX_NODISCARD std::unique_ptr<EventSource>
    addExitEvent(EventCallback callback);
    FCITX_NODISCARD std::unique_ptr<EventSource>
    addDeferEvent(EventCallback callback);
    FCITX_NODISCARD std::unique_ptr<EventSource>
    addPostEvent(EventCallback callback);

private:
    const std::unique_ptr<EventLoopPrivate> d_ptr;
    FCITX_DECLARE_PRIVATE(EventLoop);
};
} // namespace fcitx

#endif // _FCITX_UTILS_EVENT_H_
