/*
 * SPDX-FileCopyrightText: 2024-2024 CSSlayer <wengxt@gmail.com>
 *
 * SPDX-License-Identifier: LGPL-2.1-or-later
 *
 */
#ifndef _FCITX_UTILS_EVENTLOOPINTERFACE_H_
#define _FCITX_UTILS_EVENTLOOPINTERFACE_H_

#include <cstdint>
#include <functional>
#include <memory>
#include <stdexcept>
#include <fcitx-utils/fcitxutils_export.h>
#include <fcitx-utils/flags.h>
#include <fcitx-utils/macros.h>

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

/**
 * A thread-safe event source can be triggered from other threads.
 *
 * @since 5.1.13
 */
struct FCITXUTILS_EXPORT EventSourceAsync : public EventSource {
    /**
     * Trigger the event from other thread.
     *
     * The callback is guranteed to be called send() if it is enabled.
     * Multiple call to send() may only trigger the callback once.
     */
    virtual void send() = 0;
};

using IOCallback =
    std::function<bool(EventSourceIO *, int fd, IOEventFlags flags)>;
using TimeCallback = std::function<bool(EventSourceTime *, uint64_t usec)>;
using EventCallback = std::function<bool(EventSource *)>;

FCITXUTILS_EXPORT uint64_t now(clockid_t clock);

/**
 * @brief Abstract Event Loop
 *
 * @since 5.1.12
 */
class FCITXUTILS_EXPORT EventLoopInterface {
public:
    virtual ~EventLoopInterface();

    /**
     * @brief Execute event loop
     *
     * @return true if event loop is exited successfully.
     */
    virtual bool exec() = 0;

    /**
     * @brief Quit event loop
     *
     * Request event loop to quit, pending event may still be executed before
     * quit. Also execute exit event right before exiting.
     *
     * @see EventLoopInterface::addExitEvent
     */
    virtual void exit() = 0;

    /**
     * @brief Return a static implementation name of event loop
     *
     * Fcitx right now supports sd-event and libuv as implementation.
     * The library being used is decided at build time.
     *
     * @return Name of event loop implementation
     */
    virtual const char *implementation() const = 0;

    /**
     * @brief Return the internal native handle to the event loop.
     *
     * This can be useful if you want to use the underlying API against
     * event loop.
     *
     * For libuv, it will be uv_loop_t*.
     * For sd-event, it will be sd_event*.
     *
     * @return internal implementation
     * @see implementation
     */
    virtual void *nativeHandle() = 0;

    FCITX_NODISCARD std::unique_ptr<EventSourceIO> virtual addIOEvent(
        int fd, IOEventFlags flags, IOCallback callback) = 0;
    FCITX_NODISCARD std::unique_ptr<EventSourceTime> virtual addTimeEvent(
        clockid_t clock, uint64_t usec, uint64_t accuracy,
        TimeCallback callback) = 0;
    FCITX_NODISCARD virtual std::unique_ptr<EventSource>
    addExitEvent(EventCallback callback) = 0;
    FCITX_NODISCARD virtual std::unique_ptr<EventSource>
    addDeferEvent(EventCallback callback) = 0;
    FCITX_NODISCARD virtual std::unique_ptr<EventSource>
    addPostEvent(EventCallback callback) = 0;
};

class FCITXUTILS_EXPORT EventLoopInterfaceV2 : public EventLoopInterface {
public:
    FCITX_NODISCARD virtual std::unique_ptr<EventSourceAsync>
    addAsyncEvent(EventCallback callback) = 0;
};

} // namespace fcitx

#endif // _FCITX_UTILS_EVENTLOOPINTERFACE_H_
