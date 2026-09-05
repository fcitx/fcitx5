/*
 * SPDX-FileCopyrightText: 2026 CSSlayer <wengxt@gmail.com>
 *
 * SPDX-License-Identifier: LGPL-2.1-or-later
 *
 */
#ifndef _FCITX_UTILS_DBUS_COROUTINE_H_
#define _FCITX_UTILS_DBUS_COROUTINE_H_

#include <cstdint>
#include <memory>
#include <stdexcept>
#include <string>
#include <utility>
#include <coroutine>
#include <fcitx-utils/coroutine.h>
#include <fcitx-utils/dbus/bus.h>
#include <fcitx-utils/dbus/message.h>
#include <fcitx-utils/misc.h>
#include "fcitx-utils/dbus/objectvtable.h"
#include "fcitx-utils/trackableobject.h"
#include "message.h"

namespace fcitx::dbus {

class AsyncCall : public TrackableObject<AsyncCall> {

public:
    AsyncCall(Message &&message, uint64_t usec = 0)
        : message_(std::move(message)), usec_(usec) {}

    bool await_ready() const noexcept { return false; }

    void await_suspend(std::coroutine_handle<> continuation) {
        continuation_ = continuation;
        slot_ = message_.callAsync(usec_, [that = watch()](Message &reply) {
            if (auto *thatLocked = that.get()) {
                thatLocked->reply_ = std::move(reply);
                auto continuation =
                    std::exchange(thatLocked->continuation_, {});
                continuation.resume();
            }
            return true;
        });
        if (!slot_) {
            throw std::runtime_error("Failed to start asynchronous DBus call");
        }
    }

    Message await_resume() { return std::move(reply_); }

protected:
    Message message_;
    uint64_t usec_;
    std::unique_ptr<Slot> slot_;
    std::coroutine_handle<> continuation_{nullptr};
    Message reply_;
};

template <typename... ReturnTypes>
class AsyncReturn : protected AsyncCall {
public:
    using AsyncCall::AsyncCall;

    using AsyncCall::await_ready;
    using AsyncCall::await_suspend;
    auto await_resume() {
        if (reply_.isError()) {
            throw MethodCallError(reply_.errorName().c_str(),
                                  reply_.errorMessage().c_str());
        }
        if (reply_.signature() !=
            DBusSignatureTraits<ReturnTypes...>::signature::str()) {
            throw MethodReturnTypeMismatch();
        }

        MetaStringToDBusTupleType<
            typename DBusSignatureTraits<ReturnTypes...>::signature>
            ret;
        reply_ >> ret;
        return ret;
    }
};

} // namespace fcitx::dbus

#endif // _FCITX_UTILS_DBUS_COROUTINE_H_
