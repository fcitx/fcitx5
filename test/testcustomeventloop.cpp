/*
 * SPDX-FileCopyrightText: 2015-2015 CSSlayer <wengxt@gmail.com>
 *
 * SPDX-License-Identifier: LGPL-2.1-or-later
 *
 */

#include <poll.h>
#include <unistd.h>
#include <algorithm>
#include <cassert>
#include <cstdint>
#include <ctime>
#include <limits>
#include <memory>
#include <utility>
#include <vector>
#include "fcitx-utils/event.h"
#include "fcitx-utils/eventdispatcher.h"
#include "fcitx-utils/eventloopinterface.h"
#include "fcitx-utils/intrusivelist.h"
#include "fcitx-utils/macros.h"
#include "fcitx-utils/trackableobject.h"
#include "eventlooptests.h"

using namespace fcitx;

// Following is also an example of poll() based event loop
// It only serves purpose for testing.
enum class PollSourceEnableState { Disabled = 0, Oneshot = 1, Enabled = 2 };

class PollEventLoop;

template <typename T, typename C>
class PollEventBase : public T, public TrackableObject<PollEventBase<T, C>> {
public:
    using Ref = TrackableObjectReference<PollEventBase<T, C>>;

    PollEventBase(PollEventLoop *loop, PollSourceEnableState state, C callback);

    ~PollEventBase();

    bool isEnabled() const override {
        return state_ != PollSourceEnableState::Disabled;
    }
    void setEnabled(bool enabled) override {
        auto newState = enabled ? PollSourceEnableState::Enabled
                                : PollSourceEnableState::Disabled;
        setState(newState);
    }

    void setOneShot() override { setState(PollSourceEnableState::Oneshot); }

    bool isOneShot() const override {
        return state_ == PollSourceEnableState::Oneshot;
    }

    template <typename... Args>
    auto trigger(Args &&...args) {
        auto ref = this->watch();
        if (isOneShot()) {
            setEnabled(false);
        }
        auto result = callback_(this, std::forward<Args>(args)...);
        if (ref.isValid()) {
            if (!result) {
                setEnabled(false);
            }
        }
        return result;
    }

private:
    void setState(PollSourceEnableState state) { state_ = state; }
    PollSourceEnableState state_;
    C callback_;
    TrackableObjectReference<PollEventLoop> loop_;
};

class PollEventSourceIO : public PollEventBase<EventSourceIO, IOCallback>,
                          public IntrusiveListNode {
public:
    PollEventSourceIO(PollEventLoop *loop, int fd, IOEventFlags flags,
                      IOCallback callback)
        : PollEventBase(loop, PollSourceEnableState::Enabled,
                        std::move(callback)),
          fd_(fd), flags_(flags) {}

    int fd() const override { return fd_; }

    void setFd(int fd) override { fd_ = fd; }

    IOEventFlags events() const override { return flags_; }

    void setEvents(IOEventFlags flags) override { flags_ = flags; }

    IOEventFlags revents() const override { return revents_; }

private:
    int fd_;
    IOEventFlags flags_;
    IOEventFlags revents_;
};

class PollEventSourceTime : public PollEventBase<EventSourceTime, TimeCallback>,
                            public IntrusiveListNode {
public:
    PollEventSourceTime(PollEventLoop *loop, clockid_t clock, uint64_t time,
                        uint64_t accuracy, TimeCallback callback)
        : PollEventBase(loop, PollSourceEnableState::Oneshot,
                        std::move(callback)),
          clock_(clock), time_(time), accuracy_(accuracy) {}

    uint64_t time() const override { return time_; }
    void setTime(uint64_t time) override { time_ = time; }
    uint64_t accuracy() const override { return accuracy_; }
    void setAccuracy(uint64_t accuracy) override { accuracy_ = accuracy; }
    FCITX_NODISCARD clockid_t clock() const override { return clock_; }

private:
    clockid_t clock_;
    uint64_t time_, accuracy_;
};

class PollEventSource : public PollEventBase<EventSource, EventCallback>,
                        public IntrusiveListNode {
public:
    PollEventSource(PollEventLoop *loop, PollSourceEnableState state,
                    EventCallback callback)
        : PollEventBase(loop, state, std::move(callback)) {}
};

short IOEventFlagsToPoll(IOEventFlags flags) {
    short result = 0;
    if (flags.test(IOEventFlag::In)) {
        result |= POLLIN;
    }
    if (flags.test(IOEventFlag::Out)) {
        result |= POLLOUT;
    }
    return result;
}

IOEventFlags PollToIOEventFlags(short revent) {
    IOEventFlags result;
    if (revent & POLLIN) {
        result |= IOEventFlag::In;
    }
    if (revent & POLLOUT) {
        result |= IOEventFlag::Out;
    }
    if (revent & POLLERR) {
        result |= IOEventFlag::Err;
    }
    if (revent & POLLHUP) {
        result |= IOEventFlag::Hup;
    }
    return result;
}

class PollEventLoop : public EventLoopInterface,
                      public TrackableObject<PollEventLoop> {
    struct Recheck {};

public:
    bool exec() override {
        exit_ = false;
        std::vector<pollfd> fds;
        auto ref = this->watch();

        std::vector<PollEventSourceTime::Ref> timeView;
        std::vector<PollEventSourceIO::Ref> ioView;
        std::vector<PollEventSource::Ref> postView;

        auto handleTimeout = [this, &timeView, ref]() {
            timeView.clear();
            timeView.reserve(timeEvents_.size());
            for (PollEventSourceTime &time : timeEvents_) {
                if (time.isEnabled()) {
                    timeView.push_back(time.watch());
                }
            }
            for (auto &timeRef : timeView) {
                if (!ref.isValid()) {
                    break;
                }
                auto *time = timeRef.get();
                if (!time || !time->isEnabled()) {
                    continue;
                }
                auto current = now(time->clock());
                if (time->time() <= current) {
                    time->trigger(current);
                }
            }
        };

        auto collectTimeout = [this, &timeView, ref]() {
            uint64_t timeout = std::numeric_limits<uint64_t>::max();
            timeView.clear();
            timeView.reserve(timeEvents_.size());
            for (PollEventSourceTime &time : timeEvents_) {
                if (!time.isEnabled()) {
                    continue;
                }
                auto current = now(time.clock());
                uint64_t diff =
                    (time.time() > current) ? (time.time() - current) : 0;
                timeout = std::min(timeout, diff);
            }
            return timeout;
        };

        auto handleIO = [this, &ioView, &fds, &ref](uint64_t timeout) {
            timeout = timeout / 1000 + ((timeout % 1000) ? 1 : 0);
            int pollTimeout = -1;
            if (timeout < std::numeric_limits<int>::max()) {
                pollTimeout = timeout;
            }
            fds.clear();
            fds.reserve(ioEvents_.size());
            ioView.clear();
            ioView.reserve(ioEvents_.size());
            for (PollEventSourceIO &event : ioEvents_) {
                if (event.isEnabled()) {
                    auto pollevent = IOEventFlagsToPoll(event.events());
                    if (pollevent) {
                        ioView.push_back(event.watch());
                        fds.push_back(pollfd{.fd = event.fd(),
                                             .events = pollevent,
                                             .revents = 0});
                    }
                }
            }

            assert(ioView.size() == fds.size());

            auto r = poll(fds.data(), fds.size(), pollTimeout);
            if (r < 0) {
                return false;
            }
            if (r > 0) {
                for (size_t i = 0; i < fds.size(); i++) {
                    auto &ioRef = ioView[i];
                    if (!ref.isValid()) {
                        break;
                    }
                    auto *io = ioRef.get();
                    if (!io) {
                        continue;
                    }
                    if (!io->isEnabled()) {
                        continue;
                    }
                    if (fds[i].revents) {
                        io->trigger(io->fd(),
                                    PollToIOEventFlags(fds[i].revents));
                        break;
                    }
                }
            }
            return true;
        };

        auto handlePost = [this, &postView,
                           ref](IntrusiveList<PollEventSource> &events) {
            postView.clear();
            postView.reserve(postEvents_.size());
            for (PollEventSource &event : events) {
                postView.push_back(event.watch());
            }
            for (auto &postRef : postView) {
                if (!ref.isValid()) {
                    break;
                }
                auto *post = postRef.get();
                if (!post) {
                    continue;
                }
                if (!post->isEnabled()) {
                    continue;
                }
                post->trigger();
            }
        };

        while (ref.isValid() && !exit_) {
            handleTimeout();
            if (!ref.isValid() || exit_) {
                break;
            }

            handlePost(postEvents_);
            if (!ref.isValid() || exit_) {
                break;
            }
            auto timeout = collectTimeout();
            if (!handleIO(timeout)) {
                break;
            }
        }

        if (ref.isValid()) {
            handlePost(exitEvents_);
        }

        return true;
    }
    void exit() override { exit_ = true; }

    const char *implementation() const override { return "poll"; }

    void *nativeHandle() override { return nullptr; }

    std::unique_ptr<EventSourceIO> addIOEvent(int fd, IOEventFlags flags,
                                              IOCallback callback) override {
        auto event = std::make_unique<PollEventSourceIO>(this, fd, flags,
                                                         std::move(callback));
        ioEvents_.push_back(*event);
        return event;
    }
    std::unique_ptr<EventSourceTime>
    addTimeEvent(clockid_t clock, uint64_t usec, uint64_t accuracy,
                 TimeCallback callback) override {
        auto event = std::make_unique<PollEventSourceTime>(
            this, clock, usec, accuracy, std::move(callback));
        timeEvents_.push_back(*event);
        return event;
    }
    std::unique_ptr<EventSource>
    addDeferEvent(EventCallback callback) override {
        return addTimeEvent(
            CLOCK_MONOTONIC, 0, 0,
            [callback = std::move(callback)](EventSourceTime *event,
                                             uint64_t /*usec*/) -> bool {
                return callback(event);
            });
    }
    std::unique_ptr<EventSource> addPostEvent(EventCallback callback) override {
        auto event = std::make_unique<PollEventSource>(
            this, PollSourceEnableState::Enabled, std::move(callback));
        postEvents_.push_back(*event);
        return event;
    }
    std::unique_ptr<EventSource> addExitEvent(EventCallback callback) override {
        auto event = std::make_unique<PollEventSource>(
            this, PollSourceEnableState::Enabled, std::move(callback));
        exitEvents_.push_back(*event);
        return event;
    }

private:
    IntrusiveList<PollEventSourceIO> ioEvents_;
    IntrusiveList<PollEventSourceTime> timeEvents_;
    IntrusiveList<PollEventSource> postEvents_;
    IntrusiveList<PollEventSource> exitEvents_;
    bool exit_ = false;
};

template <typename T, typename C>
PollEventBase<T, C>::PollEventBase(PollEventLoop *loop,
                                   PollSourceEnableState state, C callback)
    : state_(state), callback_(std::move(callback)), loop_(loop->watch()) {}

template <typename T, typename C>
PollEventBase<T, C>::~PollEventBase() {}

std::unique_ptr<PollEventLoop> pollEventLoopFactory() {
    return std::make_unique<PollEventLoop>();
}

int main() {
    EventLoop::setEventLoopFactory(pollEventLoopFactory);
    runAllEventLoopTests();
    return 0;
}
