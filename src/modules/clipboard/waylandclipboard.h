/*
 * SPDX-FileCopyrightText: 2021~2021 CSSlayer <wengxt@gmail.com>
 *
 * SPDX-License-Identifier: LGPL-2.1-or-later
 *
 */
#ifndef _FCITX5_MODULES_CLIPBOARD_WAYLANDCLIPBOARD_H_
#define _FCITX5_MODULES_CLIPBOARD_WAYLANDCLIPBOARD_H_

#include <functional>
#include <thread>
#include <fcitx-utils/event.h>
#include <fcitx-utils/eventdispatcher.h>
#include <fcitx-utils/signals.h>
#include <fcitx-utils/unixfd.h>
#include "display.h"
#include "zwlr_data_control_device_v1.h"
#include "zwlr_data_control_manager_v1.h"

namespace fcitx {

// DataDevice receives primary/selection by DataOffer. It also starts
// DataReaderThread that will read data from file descriptor.
// Upon receive DataOffer, DataReaderThread::addTask will be used to
// initiate a reading task and call the callback if it suceeds.

using DataOfferDataCallback =
    std::function<void(const std::vector<char> &data)>;
using DataOfferCallback =
    std::function<void(const std::vector<char> &data, bool password)>;

struct DataOfferTask {
    DataOfferDataCallback callback_;
    std::shared_ptr<UnixFD> fd_;
    std::vector<char> data_;
    std::unique_ptr<EventSourceIO> ioEvent_;
    std::unique_ptr<EventSource> timeEvent_;
};

class DataReaderThread {
public:
    DataReaderThread(EventLoop *main) { dispatcherToMain_.attach(main); }

    ~DataReaderThread() {
        if (thread_ && thread_->joinable()) {
            dispatcherToWorker_.schedule([this]() {
                if (auto *loop = dispatcherToWorker_.eventLoop()) {
                    loop->exit();
                }
            });
            thread_->join();
        }
    }

    void start() {
        thread_ = std::make_unique<std::thread>(&DataReaderThread::run, this);
    }

    static void run(DataReaderThread *self) { self->realRun(); }

    uint64_t addTask(std::shared_ptr<UnixFD> fd,
                     DataOfferDataCallback callback);
    void removeTask(uint64_t token);

private:
    void realRun();

    EventDispatcher dispatcherToMain_;
    EventDispatcher dispatcherToWorker_;
    std::unique_ptr<std::thread> thread_;
    // Value only handled by the reader thread.
    uint64_t nextId_ = 1;
    std::unordered_map<uint64_t, std::unique_ptr<DataOfferTask>> *tasks_ =
        nullptr;
};

class DataOffer {
public:
    DataOffer(wayland::ZwlrDataControlOfferV1 *offer, bool ignorePassword);
    ~DataOffer();

    void receiveData(DataReaderThread &thread, DataOfferCallback callback);

private:
    void receiveDataForMime(const std::string &mime,
                            DataOfferDataCallback callback);
    void receiveRealData(DataOfferDataCallback callback);

    std::list<ScopedConnection> conns_;
    std::unordered_set<std::string> mimeTypes_;
    std::unique_ptr<wayland::ZwlrDataControlOfferV1> offer_;
    bool ignorePassword_ = true;
    bool isPassword_ = false;
    UnixFD fd_;
    DataReaderThread *thread_ = nullptr;
    uint64_t taskId_ = 0;
};

class WaylandClipboard;
class Clipboard;

class DataDevice {
public:
    DataDevice(WaylandClipboard *clipboard,
               wayland::ZwlrDataControlDeviceV1 *device);

private:
    WaylandClipboard *clipboard_;
    std::unique_ptr<wayland::ZwlrDataControlDeviceV1> device_;
    DataReaderThread thread_;
    std::unique_ptr<DataOffer> primaryOffer_;
    std::unique_ptr<DataOffer> clipboardOffer_;
    std::list<ScopedConnection> conns_;
};

class WaylandClipboard {

public:
    WaylandClipboard(Clipboard *clipboard, std::string name,
                     wl_display *display);

    void setClipboard(const std::string &str, bool password);
    void setPrimary(const std::string &str, bool password);
    EventLoop *eventLoop();
    auto display() const { return display_; }
    auto parent() const { return parent_; }

private:
    void refreshSeat();
    Clipboard *parent_;
    std::string name_;
    wayland::Display *display_;
    ScopedConnection globalConn_;
    ScopedConnection globalRemoveConn_;
    std::shared_ptr<wayland::ZwlrDataControlManagerV1> manager_;
    std::unordered_map<wayland::WlSeat *, std::unique_ptr<DataDevice>>
        deviceMap_;
};

} // namespace fcitx

#endif // _FCITX5_MODULES_CLIPBOARD_WAYLANDCLIPBOARD_H_
