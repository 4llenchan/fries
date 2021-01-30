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

    // forward definitions
    template<typename T>
    class Promise;

    template<typename T>
    class Future;

    /**
     * Future implementation class
     */
    template<typename T>
    class FutureImpl : public std::enable_shared_from_this<FutureImpl<T>>
    {
    public:
        using FutureImplPtr = std::shared_ptr<FutureImpl<T>>;
        using FutureCompletionCallback = std::function<void(FutureImplPtr)>;
        using FutureExceptionCallback = std::function<void(std::exception)>;

        FutureImpl()
                : hasException_(false), state_(waiting), completionCallback_(nullptr), exceptionCallback_(nullptr)
        {
        }

        T getValue() const
        {
            return value_;
        }

        void setValue(T value)
        {
            std::unique_lock<std::mutex> lck(mutex);
            // set value has no effect when the state is ready
            if (state_ == ready) {
                return;
            }
            value_ = value;
            state_ = ready;
            cv.notify_all();
            triggerCallback();
        }

        void setException(const std::exception &exception)
        {
            std::unique_lock<std::mutex> lck(mutex);
            // set value has no effect when the state is ready
            if (state_ == ready) {
                return;
            }
            exception_ = exception;
            hasException_ = true;
            state_ = ready;
            cv.notify_all();
            if (exceptionCallback_) {
                exceptionCallback_(exception_);
            }
        }

        const std::exception &getException() const
        {
            return exception_;
        }

        FutureState getState() const
        {
            return state_;
        }

        bool hasException() const
        {
            return hasException_;
        }

        void wait()
        {
            std::unique_lock<std::mutex> lck(mutex);
            if (state_ != ready) {
                cv.wait(lck);
            }
        }

        void setCompletionCallback(const FutureCompletionCallback &callback)
        {
            /*
             * set value and set callback could happen in the same time in different thread.
             * we still want to trigger the callback even if the state is ready.
             */
            std::unique_lock<std::mutex> lck(mutex);
            completionCallback_ = callback;
            if (state_ == ready) {
                triggerCallback();
            }
        }

        void setExceptionCallback(const FutureExceptionCallback &callback)
        {
            std::unique_lock<std::mutex> lck(mutex);
            exceptionCallback_ = callback;
            if (state_ == ready) {
                // TODO:
            }
        }

        template<typename F, typename R>
        void apply(const F &func, std::shared_ptr<FutureImpl<R>> future)
        {
            setValue(func(std::move(Future<R>(future))));
        }

        void applyException(const std::function<void(std::exception)> &func, const std::exception &exception)
        {
            // TODO: how to classify the specific kind of exception?
            func(exception);
            setException(exception);
        }

    public:
        std::mutex mutex{};
        std::condition_variable cv{};

    private:
        T value_;
        bool hasException_;
        std::exception exception_;
        FutureState state_;
        FutureCompletionCallback completionCallback_;
        FutureExceptionCallback exceptionCallback_;

        void triggerCallback()
        {
            if (completionCallback_) {
                completionCallback_(this->shared_from_this());
            }
        }
    };

    /**
     * Future implementation class for void type
     */
    template<>
    class FutureImpl<void> : public std::enable_shared_from_this<FutureImpl<void>>
    {
    public:
        using FutureImplPtr = std::shared_ptr<FutureImpl<void>>;
        using FutureCompletionCallback = std::function<void(FutureImplPtr)>;
        using FutureExceptionCallback = std::function<void(std::exception)>;

        FutureImpl()
                : hasException_(false), state_(waiting), completionCallback_(nullptr), exceptionCallback_(nullptr)
        {
        }

        void setValue()
        {
            std::unique_lock<std::mutex> lck(mutex);
            state_ = ready;
            cv.notify_all();
            triggerCallback();
        }

        void getValue()
        {
        }

        FutureState getState() const
        {
            return state_;
        }

        void setException(const std::exception &exception)
        {
            std::unique_lock<std::mutex> lck(mutex);
            // set value has no effect when the state is ready
            if (state_ == ready) {
                return;
            }
            exception_ = exception;
            hasException_ = true;
            state_ = ready;
            cv.notify_all();
            if (exceptionCallback_) {
                exceptionCallback_(exception_);
            }
        }

        const std::exception &getException() const
        {
            return exception_;
        }

        bool hasException() const
        {
            return hasException_;
        }

        void wait()
        {
            std::unique_lock<std::mutex> lck(mutex);
            if (state_ != ready) {
                cv.wait(lck);
            }
        }

        void setCompletionCallback(const FutureCompletionCallback &callback)
        {
            /*
             * set value and set callback could happen in the same time in different thread.
             * we still want to trigger the callback even if the state is ready.
             */
            std::unique_lock<std::mutex> lck(mutex);
            completionCallback_ = callback;
            if (state_ == ready) {
                triggerCallback();
            }
        }

        void setExceptionCallback(const FutureExceptionCallback &callback)
        {
            std::unique_lock<std::mutex> lck(mutex);
            exceptionCallback_ = callback;
            if (state_ == ready) {
                // TODO:
            }
        }

        template<typename F, typename R>
        void apply(const F &func, std::shared_ptr<FutureImpl<R>> future)
        {
            func(std::move(Future<R>(future)));
            setValue();
        }

        void applyException(const std::function<void(std::exception)> &func, const std::exception &exception)
        {
            // TODO: how to classify the specific kind of exception?
            func(exception);
            setException(exception);
        }

    public:
        std::mutex mutex{};
        std::condition_variable cv{};

    private:
        bool hasException_;
        std::exception exception_;
        FutureState state_;
        FutureCompletionCallback completionCallback_;
        FutureExceptionCallback exceptionCallback_;

        void triggerCallback()
        {
            if (completionCallback_) {
                completionCallback_(this->shared_from_this());
            }
        }
    };

    /**
     * Future interface class for void type
     */
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

        bool hasException() const
        {
            return future_->hasException();
        }

        const std::exception &getException() const
        {
            return future_->getException();
        }

        template<typename F>
        Future<typename std::result_of<F(Future<void>)>::type> then(const F &func)
        {
            using NextType = typename std::result_of<F(Future<void>)>::type;
            auto nextFutureImpl = std::make_shared<FutureImpl<NextType>>();
            // create a callable object to fulfill next future
            future_->setCompletionCallback([func, nextFutureImpl](FutureImplPtr futureImpl) {
                nextFutureImpl->apply(func, std::move(futureImpl));
            });
            future_->setExceptionCallback([nextFutureImpl](const std::exception &exception) {
                nextFutureImpl->setException(exception);
            });
            return std::move(Future<NextType>(nextFutureImpl));
        }

        Future<void> capture(const std::function<void(const std::exception &exception)> &func)
        {
            auto nextFutureImpl = std::make_shared<FutureImpl<void>>();
            future_->setCompletionCallback([nextFutureImpl](const FutureImplPtr &futureImpl) {
                nextFutureImpl->setValue();
            });
            future_->setExceptionCallback([func, nextFutureImpl](const std::exception &exception) {
                nextFutureImpl->applyException(func, exception);
            });
            return std::move(Future<void>(nextFutureImpl));
        }

    private:
        FutureImplPtr future_;
    };

    /**
     * Future interface class
     */
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

        bool hasException() const
        {
            return future_->hasException();
        }

        const std::exception &getException() const
        {
            return future_->getException();
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
            future_->setCompletionCallback([func, nextFutureImpl](FutureImplPtr futureImpl) {
                nextFutureImpl->apply(func, std::move(futureImpl));
            });
            future_->setExceptionCallback([nextFutureImpl](const std::exception &exception) {
                nextFutureImpl->setException(exception);
            });
            return std::move(Future<NextType>(nextFutureImpl));
        }

        Future<void> capture(const std::function<void(const std::exception &exception)> &func)
        {
            auto nextFutureImpl = std::make_shared<FutureImpl<void>>();
            future_->setCompletionCallback([nextFutureImpl](FutureImplPtr futureImpl) {
                nextFutureImpl->setValue();
            });
            future_->setExceptionCallback([func, nextFutureImpl](const std::exception &exception) {
                nextFutureImpl->applyException(func, exception);
            });
            return std::move(Future<void>(nextFutureImpl));
        }

    private:
        FutureImplPtr future_;
    };

    /**
     * Promise interface class for void type
     */
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

        void setException(const std::exception &exception)
        {
            future_->setException(exception);
        }

    private:
        using FutureImplPtr = std::shared_ptr<FutureImpl<void>>;
        FutureImplPtr future_;
    };

    /**
     * Promise interface class
     */
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

        void setException(const std::exception &exception)
        {
            future_->setException(exception);
        }

    private:
        using FutureImplPtr = std::shared_ptr<FutureImpl<T>>;
        FutureImplPtr future_;
    };

}

#endif /* FRIES_FUTURE_H */
