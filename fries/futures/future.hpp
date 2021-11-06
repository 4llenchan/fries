//
//  future.hpp
//
//  Created by AllenChan on 2020/12/5.
//

#ifndef FRIES_FUTURE_H_
#define FRIES_FUTURE_H_

#include <condition_variable>
#include <functional>
#include <memory>
#include <mutex>
#include <utility>

namespace fries {
enum future_state {
    waiting,
    ready,
};

// forward definitions
template <typename T> class promise;

template <typename T> class future;

class future_impl_base {
public:
    using future_exception_callback = std::function<void(std::exception)>;

    future_impl_base() = default;

    virtual ~future_impl_base() = default;

    void set_exception(const std::exception &exception) {
        std::lock_guard<std::mutex> lck(mutex_);
        // set value has no effect when the state is ready
        if (state_ == ready) {
            return;
        }
        exception_ = exception;
        has_exception_ = true;
        state_ = ready;
        cv_.notify_all();
        if (exception_callback_) {
            exception_callback_(exception_);
        }
    }

    const std::exception &get_exception() const { return exception_; }

    inline future_state get_state() const { return state_; }

    inline bool is_ready() const { return state_ == ready; }

    inline bool has_exception() const { return has_exception_; }

    void wait() {
        std::unique_lock<std::mutex> lck(mutex_);
        if (!is_ready()) {
            cv_.wait(lck);
        }
    }

    void set_exception_callback(const future_exception_callback &callback) {
        std::lock_guard<std::mutex> lck(mutex_);
        exception_callback_ = callback;
        if (is_ready()) {
            // TODO: what if the state is already ready?
        }
    }

    void apply_exception(const std::function<void(std::exception)> &func,
                         const std::exception &exception) {
        // TODO: how to classify the specific kind of exception?
        func(exception);
        set_exception(exception);
    }

protected:
    void set_state(future_state new_state) { state_ = new_state; }

protected:
    std::mutex mutex_{};
    std::condition_variable cv_{};

    bool has_exception_ = false;
    std::exception exception_;
    future_state state_ = future_state::waiting;
    future_exception_callback exception_callback_ = nullptr;
};

/**
 * Future implementation class
 */
template <typename T>
class future_impl : public future_impl_base,
                    public std::enable_shared_from_this<future_impl<T>> {
    using future_impl_ptr = std::shared_ptr<future_impl<T>>;
    using future_completion_callback = std::function<void(future_impl_ptr)>;

public:
    future_impl() = default;

    T get_value() const { return value_; }

    void set_value(T value) {
        std::lock_guard<std::mutex> lck(mutex_);
        // set value has no effect when the state is ready
        if (is_ready()) {
            return;
        }
        value_ = value;
        set_state(ready);
        cv_.notify_all();
        trigger_callback();
    }

    void set_completion_callback(const future_completion_callback &callback) {
        /*
         * set value and set callback could happen in the same time in different
         * thread. we still want to trigger the callback even if the state is
         * ready.
         */
        std::lock_guard<std::mutex> lck(mutex_);
        completion_callback_ = callback;
        if (is_ready()) {
            trigger_callback();
        }
    }

    template <typename F, typename R>
    void apply(const F &func, std::shared_ptr<future_impl<R>> future_impl) {
        try {
            set_value(func(std::move(future<R>(future_impl))));
        } catch (std::exception &e) {
            set_exception(e);
        }
    }

private:
    T value_;
    future_completion_callback completion_callback_ = nullptr;

    void trigger_callback() {
        if (completion_callback_) {
            completion_callback_(this->shared_from_this());
        }
    }
};

/**
 * Future implementation class for void type
 */
template <>
class future_impl<void>
    : public future_impl_base,
      public std::enable_shared_from_this<future_impl<void>> {
    using future_impl_ptr = std::shared_ptr<future_impl<void>>;
    using future_completion_callback = std::function<void(future_impl_ptr)>;

public:
    future_impl() = default;

    void set_value() {
        std::lock_guard<std::mutex> lck(mutex_);
        if (is_ready()) {
            return;
        }
        set_state(ready);
        cv_.notify_all();
        trigger_callback();
    }

    void get_value() {}

    void set_completion_callback(const future_completion_callback &callback) {
        /*
         * set value and set callback could happen in the same time in different
         * thread. we still want to trigger the callback even if the state is
         * ready.
         */
        std::lock_guard<std::mutex> lck(mutex_);
        completion_callback_ = callback;
        if (is_ready()) {
            trigger_callback();
        }
    }

    template <typename F, typename R>
    void apply(const F &func, std::shared_ptr<future_impl<R>> future_impl) {
        try {
            func(std::move(future<R>(future_impl)));
            set_value();
        } catch (std::exception &e) {
            set_exception(e);
        }
    }

private:
    future_completion_callback completion_callback_ = nullptr;

    void trigger_callback() {
        if (completion_callback_) {
            completion_callback_(shared_from_this());
        }
    }
};

/**
 * Future interface class
 */
template <typename T> class future {
    friend class promise<T>;

    using future_impl_ptr = std::shared_ptr<future_impl<T>>;

public:
    future() = default;

    future(future<T> &other) : impl_(other.impl_) {}

    future(future<T> &&other) noexcept : impl_(std::move(other.impl_)) {}

    explicit future(future_impl_ptr future_impl)
        : impl_(std::move(future_impl)) {}

    bool is_ready() const { return impl_->is_ready(); }

    bool has_exception() const { return impl_->has_exception(); }

    const std::exception &get_exception() const {
        return impl_->get_exception();
    }

    void wait() const { impl_->wait(); }

    T get_value() const { return impl_->get_value(); }

    template <typename F>
    future<typename std::result_of<F(future<T>)>::type> then(const F &func) {
        using next_type = typename std::result_of<F(future<T>)>::type;
        auto next_future_impl = std::make_shared<future_impl<next_type>>();
        // create a callable object to fulfill next future
        impl_->set_completion_callback(
            [func, next_future_impl](future_impl_ptr impl) {
                next_future_impl->apply(func, std::move(impl));
            });
        impl_->set_exception_callback(
            [next_future_impl](const std::exception &exception) {
                next_future_impl->set_exception(exception);
            });
        return std::move(future<next_type>(next_future_impl));
    }

    future<void>
    capture(const std::function<void(const std::exception &exception)> &func) {
        auto next_future_impl = std::make_shared<future_impl<void>>();
        impl_->set_completion_callback(
            [next_future_impl](future_impl_ptr impl) {
                next_future_impl->set_value();
            });
        impl_->set_exception_callback(
            [func, next_future_impl](const std::exception &exception) {
                next_future_impl->apply_exception(func, exception);
            });
        return future<void>(next_future_impl);
    }

private:
    future_impl_ptr impl_;
};

/**
 * Promise interface class for void type
 */
template <> class promise<void> {
public:
    promise() : impl_(std::make_shared<future_impl<void>>()) {}

    future<void> get_future() { return future<void>(impl_); }

    void set_value() { impl_->set_value(); }

    void set_exception(const std::exception &exception) {
        impl_->set_exception(exception);
    }

private:
    using future_impl_ptr = std::shared_ptr<future_impl<void>>;
    future_impl_ptr impl_;
};

/**
 * Promise interface class
 */
template <typename T> class promise {
public:
    promise() : impl_(std::make_shared<future_impl<T>>()) {}

    future<T> get_future() { return future<T>(impl_); }

    void set_value(T value) { impl_->set_value(std::move(value)); }

    void set_exception(const std::exception &exception) {
        impl_->set_exception(exception);
    }

private:
    using future_impl_ptr = std::shared_ptr<future_impl<T>>;
    future_impl_ptr impl_;
};

} // namespace fries

#endif /* FRIES_FUTURE_H_ */
