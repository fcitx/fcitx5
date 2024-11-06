/*
 * SPDX-FileCopyrightText: 2024 Qijia Liu <liumeo@pku.edu.cn>
 *
 * SPDX-License-Identifier: LGPL-2.1-or-later
 *
 */
#ifndef _FCITX_UTILS_EVENT_IMPL_H_
#define _FCITX_UTILS_EVENT_IMPL_H_

#include <fcitx-utils/event.h>
namespace fcitx {
class FCITXUTILS_EXPORT EventLoopImpl {
public:
    EventLoopImpl() = default;
    virtual ~EventLoopImpl() = default;
    virtual std::unique_ptr<EventSourceIO>
    addIOEvent(int fd, IOEventFlags flags, IOCallback callback);
    virtual std::unique_ptr<EventSourceTime>
    addTimeEvent(clockid_t clock, uint64_t usec, uint64_t accuracy,
                 TimeCallback callback);
    virtual std::unique_ptr<EventSource> addExitEvent(EventCallback callback);
    virtual std::unique_ptr<EventSource> addDeferEvent(EventCallback callback);
    virtual std::unique_ptr<EventSource> addPostEvent(EventCallback callback);
};

FCITXUTILS_EXPORT void setEventLoopImpl(std::unique_ptr<EventLoopImpl> factory);
} // namespace fcitx

#endif // _FCITX_UTILS_EVENT_H_
