/*
 * SPDX-FileCopyrightText: 2015-2015 CSSlayer <wengxt@gmail.com>
 *
 * SPDX-License-Identifier: LGPL-2.1-or-later
 *
 */
#ifndef _FCITX_UTILS_EVENT_H_
#define _FCITX_UTILS_EVENT_H_

#include <cstdint>
#include <functional>
#include <memory>
#include <fcitx-utils/eventloopinterface.h>
#include <fcitx-utils/fcitxutils_export.h>
#include <fcitx-utils/flags.h>
#include <fcitx-utils/macros.h>

namespace fcitx {

using EventLoopFactory = std::function<std::unique_ptr<EventLoopInterface>()>;

class EventLoopPrivate;
class FCITXUTILS_EXPORT EventLoop {
public:
    EventLoop();
    EventLoop(std::unique_ptr<EventLoopInterface> impl);
    virtual ~EventLoop();
    bool exec();
    void exit();

    /**
     * Return the default implementation name.
     *
     * This will only return the default implementation name.
     * Do not rely on this value.
     *
     * @see EventLoop::implementation
     */
    FCITXUTILS_DEPRECATED static const char *impl();

    /**
     * Return the name of implementation of event loop.
     * @since 5.1.12
     */
    const char *implementation() const;
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

    /**
     * Add an async event that is safe to be triggered from another thread.
     *
     * To ensure safe usage of this event:
     * 1. Do not change event to disable, if there may be pending call to send()
     * 2. Due to (1), if it is oneshot, ensure you only call send() once.
     * 3. Join all the possible pending thread that may call send(), before
     *    destructing the event.
     * 4. Like other event, the event should be only created/destructed on the
     *    event loop thread.
     *
     * EventDispatcher uses this event internally and provides an easier and
     * safer interface to use.
     *
     * @see EventDispatcher
     *
     * @param callback callback function
     * @return async event source
     * @since 5.1.13
     */
    FCITX_NODISCARD std::unique_ptr<EventSourceAsync>
    addAsyncEvent(EventCallback callback);

    /**
     * Set an external event loop implementation.
     *
     * This is useful if you need to integrate fcitx with another event loop.
     *
     * @since 5.1.12
     */
    static void setEventLoopFactory(EventLoopFactory factory);

private:
    const std::unique_ptr<EventLoopPrivate> d_ptr;
    FCITX_DECLARE_PRIVATE(EventLoop);
};
} // namespace fcitx

#endif // _FCITX_UTILS_EVENT_H_
