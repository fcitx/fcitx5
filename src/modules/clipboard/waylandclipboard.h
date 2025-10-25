/*
 * SPDX-FileCopyrightText: 2021~2021 CSSlayer <wengxt@gmail.com>
 *
 * SPDX-License-Identifier: LGPL-2.1-or-later
 *
 */
#ifndef _FCITX5_MODULES_CLIPBOARD_WAYLANDCLIPBOARD_H_
#define _FCITX5_MODULES_CLIPBOARD_WAYLANDCLIPBOARD_H_

#include <cstdint>
#include <functional>
#include <list>
#include <memory>
#include <string>
#include <thread>
#include <unordered_map>
#include <unordered_set>
#include <vector>
#include <wayland-client-core.h>
#include "fcitx-utils/event.h"
#include "fcitx-utils/eventdispatcher.h"
#include "fcitx-utils/eventloopinterface.h"
#include "fcitx-utils/signals.h"
#include "fcitx-utils/trackableobject.h"
#include "fcitx-utils/unixfd.h"
#include "display.h"
#include "ext_data_control_device_v1.h"
#include "ext_data_control_manager_v1.h"
#include "ext_data_control_offer_v1.h"
#include "zwlr_data_control_device_v1.h"
#include "zwlr_data_control_manager_v1.h"
#include "zwlr_data_control_offer_v1.h"

namespace fcitx {

// DataDevice receives primary/selection by DataOffer. It also starts
// DataReaderThread that will read data from file descriptor.
// Upon receive DataOffer, DataReaderThread::addTask will be used to
// initiate a reading task and call the callback if it suceeds.

using DataOfferDataCallback =
    std::function<void(const std::vector<char> &data)>;
using DataOfferCallback =
    std::function<void(const std::vector<char> &data, bool password)>;

class DataOfferInterface : public TrackableObject<DataOfferInterface> {
public:
    virtual ~DataOfferInterface();
};

template <typename DataControlOffer>
class DataOffer;

struct DataOfferTask {
    DataOfferTask() = default;
    DataOfferTask(const DataOfferTask &) = delete;
    DataOfferTask(DataOfferTask &&) = delete;

    DataOfferTask &operator=(const DataOfferTask &) = delete;
    DataOfferTask &operator=(DataOfferTask &&) = delete;

    uint64_t id_ = 0;
    TrackableObjectReference<DataOfferInterface> offer_;
    DataOfferDataCallback callback_;
    std::shared_ptr<UnixFD> fd_;
    std::vector<char> data_;
    std::unique_ptr<EventSourceIO> ioEvent_;
    std::unique_ptr<EventSource> timeEvent_;
};

class DataReaderThread {
public:
    DataReaderThread(EventDispatcher &dispatcherToMain)
        : dispatcherToMain_(dispatcherToMain) {}

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

    uint64_t addTask(DataOfferInterface *offer, std::shared_ptr<UnixFD> fd,
                     DataOfferDataCallback callback);
    void removeTask(uint64_t token);

private:
    // Function that run on reader thread
    void realRun();
    void addTaskOnWorker(uint64_t id,
                         TrackableObjectReference<DataOfferInterface> offer,
                         std::shared_ptr<UnixFD> fd,
                         DataOfferDataCallback callback);
    void handleTaskIO(DataOfferTask *task, IOEventFlags flags);
    void handleTaskTimeout(DataOfferTask *task);
    // End of function that run on reader thread

    EventDispatcher &dispatcherToMain_;
    std::unique_ptr<std::thread> thread_;
    uint64_t nextId_ = 1;

    // Value only read/write by the reader thread.
    EventDispatcher dispatcherToWorker_;
    std::unordered_map<uint64_t, DataOfferTask> tasks_;
};

template <typename DataControlOffer>
class DataOffer : public DataOfferInterface {
public:
    DataOffer(DataControlOffer *offer, bool ignorePassword);
    ~DataOffer() override;

    void receiveData(DataReaderThread &thread, DataOfferCallback callback);

private:
    void receiveDataForMime(const std::string &mime,
                            DataOfferDataCallback callback);
    void receiveRealData(DataOfferDataCallback callback);

    std::list<ScopedConnection> conns_;
    std::unordered_set<std::string> mimeTypes_;
    std::unique_ptr<DataControlOffer> offer_;
    bool ignorePassword_ = true;
    bool isPassword_ = false;
    UnixFD fd_;
    DataReaderThread *thread_ = nullptr;
    uint64_t taskId_ = 0;
};

class WaylandClipboard;
class Clipboard;

class DataDeviceInterface {
public:
    virtual ~DataDeviceInterface();
};

template <typename DataControlDevice>
struct DataControlDeviceTraits;

template <>
struct DataControlDeviceTraits<wayland::ExtDataControlDeviceV1> {
    using DataControlOfferType = wayland::ExtDataControlOfferV1;
};

template <>
struct DataControlDeviceTraits<wayland::ZwlrDataControlDeviceV1> {
    using DataControlOfferType = wayland::ZwlrDataControlOfferV1;
};

template <typename DataControlDevice>
class DataDevice : public DataDeviceInterface {
    using DataControlOfferType = typename DataControlDeviceTraits<
        DataControlDevice>::DataControlOfferType;
    using DataOfferType = DataOffer<DataControlOfferType>;

public:
    DataDevice(WaylandClipboard *clipboard, DataControlDevice *device);

private:
    WaylandClipboard *clipboard_;
    std::unique_ptr<DataControlDevice> device_;
    DataReaderThread thread_;
    std::unique_ptr<DataOfferType> primaryOffer_;
    std::unique_ptr<DataOfferType> clipboardOffer_;
    std::list<ScopedConnection> conns_;
};

class WaylandClipboard {

public:
    WaylandClipboard(Clipboard *clipboard, std::string name,
                     wl_display *display);

    void setClipboard(const std::string &str, bool password);
    void setPrimary(const std::string &str, bool password);
    auto display() const { return display_; }
    auto parent() const { return parent_; }

private:
    void refreshSeat();
    Clipboard *parent_;
    std::string name_;
    wayland::Display *display_;
    ScopedConnection globalConn_;
    ScopedConnection globalRemoveConn_;
    std::shared_ptr<wayland::ExtDataControlManagerV1> ext_manager_;
    std::shared_ptr<wayland::ZwlrDataControlManagerV1> wlr_manager_;
    std::unordered_map<wayland::WlSeat *, std::unique_ptr<DataDeviceInterface>>
        deviceMap_;
};

} // namespace fcitx

#endif // _FCITX5_MODULES_CLIPBOARD_WAYLANDCLIPBOARD_H_
