/*
 * SPDX-FileCopyrightText: 2026 CSSlayer <wengxt@gmail.com>
 *
 * SPDX-License-Identifier: LGPL-2.1-or-later
 *
 */
#ifndef _FCITX_UTILS_COROUTINE_H_
#define _FCITX_UTILS_COROUTINE_H_

#include <exception>
#include <memory>
#include <stdexcept>
#include <type_traits>
#include <utility>
#include <variant>
#include <coroutine>
#include <fcitx-utils/eventdispatcher.h>
#include "fcitx-utils/misc.h"
#include "fcitx-utils/trackableobject.h"

namespace fcitx {

template <typename T>
    requires(!std::is_reference_v<T>)
class Coroutine;

namespace detail {

struct CoroutinePromiseBase {
    struct FinalAwaiter {
        auto await_ready() const noexcept -> bool { return detached_; }

        template <typename promise_type>
        auto
        await_suspend(std::coroutine_handle<promise_type> coroutine) noexcept
            -> std::coroutine_handle<> {
            auto continuation = coroutine.promise().continuation_;
            return continuation ? continuation : std::noop_coroutine();
        }

        auto await_resume() noexcept -> void {
            // no-op
        }

        bool detached_ = false;
    };

    std::suspend_always initial_suspend() noexcept { return {}; }
    FinalAwaiter final_suspend() noexcept { return {detached_}; }
    void continuation(std::coroutine_handle<> continuation) noexcept {
        continuation_ = continuation;
    }
    void detach() noexcept { detached_ = true; }

private:
    std::coroutine_handle<> continuation_;
    bool detached_ = false;
};

template <typename T>
    requires(!std::is_reference_v<T>)
struct CoroutinePromise : public CoroutinePromiseBase {
    using storage_type = std::remove_cvref_t<T>;

    Coroutine<T> get_return_object() noexcept;

    void return_value(T value) { result_ = std::move(value); }

    void unhandled_exception() { result_ = std::current_exception(); }

    auto result() & -> const T & {
        auto &result = get_result();
        return static_cast<const T &>(result);
    }

    auto result() const & -> const T & {
        auto &result = get_result();
        return static_cast<const T &>(result);
    }

    auto result() && -> T && {
        auto &result = get_result();
        return static_cast<T &&>(result);
    }

private:
    storage_type &get_result() {
        if (std::holds_alternative<storage_type>(result_)) {
            return std::get<storage_type>(result_);
        }
        if (std::holds_alternative<std::exception_ptr>(result_)) {
            std::rethrow_exception(std::get<std::exception_ptr>(result_));
        }
        throw std::runtime_error{"The return value was never set, did you "
                                 "execute the coroutine?"};
    }

    std::variant<std::monostate, storage_type, std::exception_ptr> result_;
};

template <>
struct CoroutinePromise<void> : public CoroutinePromiseBase {
    struct PlaceHolder {};

    Coroutine<void> get_return_object() noexcept;

    void return_void() noexcept { result_ = PlaceHolder{}; }

    void unhandled_exception() { result_ = std::current_exception(); }

    auto result() -> void {
        if (std::holds_alternative<PlaceHolder>(result_)) {
            return;
        }
        if (std::holds_alternative<std::exception_ptr>(result_)) {
            std::rethrow_exception(std::get<std::exception_ptr>(result_));
        }
        throw std::runtime_error{"The return value was never set, did you "
                                 "execute the coroutine?"};
    }

private:
    std::variant<std::monostate, PlaceHolder, std::exception_ptr> result_;
};

template <typename T>
    requires(!std::is_reference_v<T>)
class CoroutineBase {
    class CoroutinePrivateToken {
        CoroutinePrivateToken() = default;
        friend class CoroutineBase;
    };

public:
    using promise_type = detail::CoroutinePromise<T>;
    using handle_type = std::coroutine_handle<promise_type>;
    using value_type = T;

    explicit CoroutineBase(handle_type handle) noexcept : handle_(handle) {}

    ~CoroutineBase() {
        if (handle_) {
            handle_.destroy();
        }
    }

    bool done() const noexcept { return !handle_ || handle_.done(); }

    bool resume() const {
        if (!handle_) {
            return false;
        }

        if (!handle_.done()) {
            handle_.resume();
        }
        return !handle_.done();
    }

    auto destroy() -> bool {
        if (handle_) {
            handle_.destroy();
            handle_ = {};
            return true;
        }

        return false;
    }

    handle_type detach_handle() && noexcept {
        auto handle = std::move(*this).extract_handle({});
        if (handle) {
            handle.promise().detach();
        }
        return handle;
    }

    auto result() -> decltype(auto) {
        if constexpr (std::is_void_v<T>) {
            handle_.promise().result();
        } else {
            return handle_.promise().result();
        }
    }

    handle_type extract_handle(CoroutinePrivateToken /*token*/) && noexcept {
        return std::exchange(handle_, {});
    }

protected:
    static CoroutinePrivateToken token() noexcept { return {}; }

    handle_type handle_{};
};

} // namespace detail

/**
 * Basic coroutine class.
 *
 * @code
 * Coroutine<int> myCoroutine() {
 *     co_return 42;
 * }
 * @endcode
 *
 * Normally, you would use `co_await` to wait for the result of the coroutine.
 *
 * @tparam T The return type of the coroutine.
 * @note The coroutine must be executed before the result is accessed.
 * @note The coroutine must not be copied or moved.
 */
template <typename T>
    requires(!std::is_reference_v<T>)
class [[nodiscard]] Coroutine : public detail::CoroutineBase<T> {
public:
    using base_type = detail::CoroutineBase<T>;
    using promise_type = detail::CoroutinePromise<T>;
    using handle_type = std::coroutine_handle<promise_type>;
    using value_type = T;

    class Awaiter {
    public:
        explicit Awaiter(Coroutine<T> &co) noexcept
            : handle_(std::move(co).extract_handle(base_type::token())) {}
        ~Awaiter() {
            if (handle_) {
                handle_.destroy();
            }
        }

        bool await_ready() const noexcept { return !handle_ || handle_.done(); }

        std::coroutine_handle<>
        await_suspend(std::coroutine_handle<> await) noexcept {
            handle_.promise().continuation(await);
            return handle_;
        }

        auto await_resume() {
            ScopeExit scope{[&]() { std::exchange(handle_, {}).destroy(); }};
            if constexpr (std::is_void_v<T>) {
                handle_.promise().result();
            } else {
                return std::move(handle_.promise()).result();
            }
        }

    private:
        handle_type handle_{};
    };

    using base_type::base_type;

    Coroutine(Coroutine &&other) = delete;
    Coroutine &operator=(Coroutine &&other) = delete;
    Coroutine(const Coroutine &) = delete;
    Coroutine &operator=(const Coroutine &) = delete;

    Awaiter operator co_await() && noexcept { return Awaiter{*this}; }
};

/**
 * CoroutineTask is a move-only coroutine wrapper.
 *
 * It can be used to send a coroutine to an EventDispatcher, which will run the
 * coroutine in the event loop.
 *
 * If you want to cancel the coroutine, you can destroy the CoroutineTask
 * object.
 */
template <typename T>
    requires(!std::is_reference_v<T>)
class CoroutineTask : public detail::CoroutineBase<T>,
                      public TrackableObject<CoroutineTask<T>> {
public:
    using base_type = detail::CoroutineBase<T>;
    using CoroutineType = Coroutine<T>;
    using handle_type = typename CoroutineType::handle_type;
    using value_type = typename CoroutineType::value_type;

    CoroutineTask(CoroutineType &&co) noexcept
        : base_type(std::move(co).extract_handle(base_type::token())) {}

    CoroutineTask(CoroutineTask &&other) noexcept
        : base_type(std::move(other).extract_handle(base_type::token())) {};
    CoroutineTask &operator=(CoroutineTask &&other) noexcept {
        if (this == &other) {
            return *this;
        }
        base_type::destroy();
        base_type::handle_ =
            std::move(other).extract_handle(base_type::token());
        return *this;
    }
    CoroutineTask(const CoroutineTask &) = delete;
    CoroutineTask &operator=(const CoroutineTask &) = delete;
};

template <typename T>
CoroutineTask(Coroutine<T>) -> CoroutineTask<T>;

namespace detail {

template <typename T>
    requires(!std::is_reference_v<T>)
inline Coroutine<T> CoroutinePromise<T>::get_return_object() noexcept {
    return Coroutine<T>{
        std::coroutine_handle<CoroutinePromise<T>>::from_promise(*this)};
}

inline Coroutine<void> CoroutinePromise<void>::get_return_object() noexcept {
    return Coroutine<void>{
        std::coroutine_handle<CoroutinePromise<void>>::from_promise(*this)};
}

} // namespace detail

template <typename CoroutineType>
void sendToEventDispatcherDetached(EventDispatcher *dispatcher,
                                   CoroutineType &&task) {
    // TODO: get rid of shared_ptr once we could use move_only_function.
    dispatcher->schedule(
        [taskPtr = std::make_shared<
             CoroutineTask<typename CoroutineType::value_type>>(
             std::forward<CoroutineType>(task))]() mutable {
            if (auto handle = std::move(*taskPtr).detach_handle()) {
                handle.resume();
            }
        });
}
template <typename T>
void sendToEventDispatcher(EventDispatcher *dispatcher,
                           CoroutineTask<T> &task) {
    dispatcher->scheduleWithContext(task.watch(), [&task]() { task.resume(); });
}

} // namespace fcitx

#endif // _FCITX_UTILS_COROUTINE_H_
