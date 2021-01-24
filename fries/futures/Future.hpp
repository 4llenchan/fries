//
//  Future.hpp
//
//  Created by AllenChan on 2020/12/5.
//

#ifndef FRIES_FUTURE_H
#define FRIES_FUTURE_H

#include <memory>
#include <mutex>
#include <condition_variable>
#include <utility>

namespace fries
{
    enum FutureState
    {
        waiting,
        ready,
    };

    template<typename T>
    class Promise;

    template<typename T>
    class Future;

    template<typename T>
    class FutureImpl : public std::enable_shared_from_this<FutureImpl<T>>
    {
    public:
        using FutureImplPtr = std::shared_ptr<FutureImpl<T>>;
        using FutureCompletionCallback = std::function<void(FutureImplPtr)>;

        FutureImpl() : state_(waiting), callback_(nullptr)
        {
        }

        T getValue() const
        {
            return value_;
        }

        void setValue(T value)
        {
            std::unique_lock<std::mutex> lck(mutex);
            value_ = value;
            state_ = ready;
            cv.notify_all();
            if (callback_) {
                callback_(this->shared_from_this());
            }
        }

        FutureState getState() const
        {
            return state_;
        }

        void wait()
        {
            std::unique_lock<std::mutex> lck(mutex);
            if (state_ != ready) {
                cv.wait(lck);
            }
        }

        void setCallback(const FutureCompletionCallback &callback)
        {
            callback_ = callback;
        }

        template<typename F, typename R>
        void apply(const F &func, std::shared_ptr<FutureImpl<R>> future)
        {
            setValue(func(std::move(Future<R>(future))));
        }

    public:
        std::mutex mutex{};
        std::condition_variable cv{};

    private:
        T value_;
        FutureState state_;
        FutureCompletionCallback callback_;
    };


    // implementation for void type
    template<>
    class FutureImpl<void> : public std::enable_shared_from_this<FutureImpl<void>>
    {
    public:
        using FutureImplPtr = std::shared_ptr<FutureImpl<void>>;
        using FutureCompletionCallback = std::function<void(FutureImplPtr)>;

        FutureImpl() : state_(waiting), callback_(nullptr)
        {
        }

        void setValue()
        {
            std::unique_lock<std::mutex> lck(mutex);
            state_ = ready;
            cv.notify_all();
            if (callback_) {
                callback_(this->shared_from_this());
            }
        }

        void getValue()
        {
        }

        FutureState getState() const
        {
            return state_;
        }

        void wait()
        {
            std::unique_lock<std::mutex> lck(mutex);
            if (state_ != ready) {
                cv.wait(lck);
            }
        }

        void setCallback(const FutureCompletionCallback &callback)
        {
            callback_ = callback;
        }

        template<typename F, typename R>
        void apply(const F &func, std::shared_ptr<FutureImpl<R>> future)
        {
            func(std::move(Future<R>(future)));
            setValue();
        }

    public:
        std::mutex mutex{};
        std::condition_variable cv{};

    private:
        FutureState state_;
        FutureCompletionCallback callback_;
    };

    // implementation for void type
    template<>
    class Future<void>
    {
        friend class Promise<void>;

        using FutureImplPtr = std::shared_ptr<FutureImpl<void>>;

    public:
        Future()
        = default;

        Future(Future<void> &future)
        {
            future_ = future.future_;
        }

        Future(Future<void> &&future) noexcept
        {
            future_ = future.future_;
            future.future_.reset();
        }

        explicit Future(FutureImplPtr futureImpl)
        {
            future_ = std::move(futureImpl);
        }

        bool isReady() const
        {
            return future_->getState() == FutureState::ready;
        }

        void wait() const
        {
            future_->wait();
        }

        void getValue() const
        {
            return future_->getValue();
        }

        template<typename F>
        Future<typename std::result_of<F(Future<void>)>::type> then(const F &func)
        {
            using NextType = typename std::result_of<F(Future<void>)>::type;
            auto nextFutureImpl = std::make_shared<FutureImpl<NextType>>();
            // create a callable object to fulfill next future
            future_->setCallback([func, nextFutureImpl](FutureImplPtr futureImpl) {
                nextFutureImpl->apply(func, std::move(futureImpl));
            });
            return std::move(Future<NextType>(nextFutureImpl));
        }

    private:
        FutureImplPtr future_;
    };

    template<typename T>
    class Future
    {
        friend class Promise<T>;

        using FutureImplPtr = std::shared_ptr<FutureImpl<T>>;

    public:
        Future()
        = default;

        Future(Future<T> &future)
        {
            future_ = future.future_;
        }

        Future(Future<T> &&future) noexcept
        {
            future_ = future.future_;
            future.future_.reset();
        }

        explicit Future(FutureImplPtr futureImpl)
        {
            future_ = futureImpl;
        }

        bool isReady() const
        {
            return future_->getState() == FutureState::ready;
        }

        void wait() const
        {
            future_->wait();
        }

        T getValue() const
        {
            return future_->getValue();
        }

        template<typename F>
        Future<typename std::result_of<F(Future<T>)>::type> then(const F &func)
        {
            using NextType = typename std::result_of<F(Future<T>)>::type;
            auto nextFutureImpl = std::make_shared<FutureImpl<NextType>>();
            // create a callable object to fulfill next future
            future_->setCallback([func, nextFutureImpl](FutureImplPtr futureImpl) {
                nextFutureImpl->apply(func, std::move(futureImpl));
            });
            return std::move(Future<NextType>(nextFutureImpl));
        }

    private:
        FutureImplPtr future_;
    };

    // implementation for void type
    template<>
    class Promise<void>
    {
    public:
        Promise() : future_(std::make_shared<FutureImpl<void>>())
        {
        }

        Future<void> getFuture()
        {
            return Future<void>(future_);
        }

        void setValue()
        {
            future_->setValue();
        }

    private:
        using FutureImplPtr = std::shared_ptr<FutureImpl<void>>;
        FutureImplPtr future_;
    };

    template<typename T>
    class Promise
    {
    public:
        Promise() : future_(std::make_shared<FutureImpl<T>>())
        {
        }

        Future<T> getFuture()
        {
            return Future<T>(future_);
        }

        void setValue(T value)
        {
            future_->setValue(std::move(value));
        }

    private:
        using FutureImplPtr = std::shared_ptr<FutureImpl<T>>;
        FutureImplPtr future_;
    };

}

#endif /* FRIES_FUTURE_H */
