//
// Copyright (C) 2020~2020 by CSSlayer
// wengxt@gmail.com
//
// This library is free software; you can redistribute it and/or modify
// it under the terms of the GNU Lesser General Public License as
// published by the Free Software Foundation; either version 2 of the
// License, or (at your option) any later version.
//
// This library is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
// Lesser General Public License for more details.
//
// You should have received a copy of the GNU Lesser General Public
// License along with this library; see the file COPYING. If not,
// see <http://www.gnu.org/licenses/>.
//
#ifndef _FCITX5_MODULES_XCB_XCBEVENTREADER_H_
#define _FCITX5_MODULES_XCB_XCBEVENTREADER_H_

#include <mutex>
#include <thread>
#include <fcitx-utils/event.h>
#include <fcitx-utils/eventdispatcher.h>
#include <xcb/xcb.h>
#include "xcb_public.h"

namespace fcitx {

class XCBConnection;

class XCBEventReader {
public:
    XCBEventReader(XCBConnection *conn);
    ~XCBEventReader();

    auto events() {
        std::lock_guard<std::mutex> lock(mutex_);
        std::list<XCBReply<xcb_generic_event_t>> events;
        using std::swap;
        swap(events_, events);
        return events;
    }
    void wakeUp();

private:
    static void runThread(XCBEventReader *self) { self->run(); }
    void run();
    bool onIOEvent(IOEventFlags flags);
    XCBConnection *conn_;
    EventDispatcher dispatcherToMain_;
    EventDispatcher dispatcherToWorker_;
    bool hadError_ = false;
    std::unique_ptr<EventSource> deferEvent_;
    std::unique_ptr<EventSource> wakeEvent_;
    std::unique_ptr<std::thread> thread_;
    std::unique_ptr<EventLoop> event_;
    std::mutex mutex_;
    std::list<XCBReply<xcb_generic_event_t>> events_;
};

} // namespace fcitx

#endif // _FCITX5_MODULES_XCB_XCBEVENTREADER_H_
