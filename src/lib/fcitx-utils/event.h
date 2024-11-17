/*
 * SPDX-FileCopyrightText: 2015-2015 CSSlayer <wengxt@gmail.com>
 *
 * SPDX-License-Identifier: LGPL-2.1-or-later
 *
 */
#ifndef _FCITX_UTILS_EVENT_H_
#define _FCITX_UTILS_EVENT_H_

#include <cstdint>
#include <memory>
#include <fcitx-utils/eventloopinterface.h>
#include <fcitx-utils/flags.h>
#include <fcitx-utils/macros.h>
#include "fcitxutils_export.h"

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
