/*
 * SPDX-FileCopyrightText: 2023-2023 CSSlayer <wengxt@gmail.com>
 *
 * SPDX-License-Identifier: LGPL-2.1-or-later
 *
 */
#ifndef _FCITX_MODULES_WAYLAND_WAYLANDEVENTREADER_H_
#define _FCITX_MODULES_WAYLAND_WAYLANDEVENTREADER_H_

#include <condition_variable>
#include <mutex>
#include <thread>
#include "fcitx-utils/event.h"
#include "fcitx-utils/eventdispatcher.h"
#include "fcitx-utils/trackableobject.h"
#include "display.h"

namespace fcitx {

class WaylandConnection;
class WaylandModule;

class WaylandEventReader : public TrackableObject<WaylandEventReader> {
public:
    WaylandEventReader(WaylandConnection *conn);
    ~WaylandEventReader();

private:
    static void runThread(WaylandEventReader *self) { self->run(); }
    void run();
    bool onIOEvent(IOEventFlags flags);
    void dispatch();
    void quit();

    WaylandModule *module_;
    WaylandConnection *conn_;
    wayland::Display &display_;
    EventDispatcher &dispatcherToMain_;
    EventDispatcher dispatcherToWorker_;
    std::unique_ptr<EventSource> postEvent_;

    // Protected by mutex_;
    bool quitting_ = false;
    bool isReading_ = false;

    std::unique_ptr<std::thread> thread_;
    std::mutex mutex_;
    std::condition_variable condition_;
};
} // namespace fcitx

#endif