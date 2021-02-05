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

    class FutureImplBase
    {
    public:
        using FutureExceptionCallback = std::function<void(std::exception)>;

        FutureImplBase()
        = default;

        virtual ~FutureImplBase()
        = default;

        void setException(const std::exception &exception)
        {
            std::unique_lock<std::mutex> lck(mutex_);
            // set value has no effect when the state is ready
            if (state_ == ready) {
                return;
            }
            exception_ = exception;
            hasException_ = true;
            state_ = ready;
            cv_.notify_all();
            if (exceptionCallback_) {
                exceptionCallback_(exception_);
            }
        }

        const std::exception &getException() const
        {
            return exception_;
        }

        inline FutureState getState() const
        {
            return state_;
        }

        inline bool isReady() const
        {
            return state_ == ready;
        }

        inline bool hasException() const
        {
            return hasException_;
        }

        void wait()
        {
            std::unique_lock<std::mutex> lck(mutex_);
            if (!isReady()) {
                cv_.wait(lck);
            }
        }

        void setExceptionCallback(const FutureExceptionCallback &callback)
        {
            std::unique_lock<std::mutex> lck(mutex_);
            exceptionCallback_ = callback;
            if (isReady()) {
                // TODO: what if the state is already ready?
            }
        }

        void applyException(const std::function<void(std::exception)> &func, const std::exception &exception)
        {
            // TODO: how to classify the specific kind of exception?
            func(exception);
            setException(exception);
        }

    protected:
        void setState(FutureState newState)
        {
            state_ = newState;
        }

    protected:
        std::mutex mutex_{};
        std::condition_variable cv_{};

        bool hasException_ = false;
        std::exception exception_{};
        FutureState state_ = FutureState::waiting;
        FutureExceptionCallback exceptionCallback_ = nullptr;
    };

    /**
     * Future implementation class
     */
    template<typename T>
    class FutureImpl : public FutureImplBase, public std::enable_shared_from_this<FutureImpl<T>>
    {
        using FutureImplPtr = std::shared_ptr<FutureImpl<T>>;
        using FutureCompletionCallback = std::function<void(FutureImplPtr)>;
    public:
        FutureImpl()
        = default;

        T getValue() const
        {
            return value_;
        }

        void setValue(T value)
        {
            std::unique_lock<std::mutex> lck(mutex_);
            // set value has no effect when the state is ready
            if (isReady()) {
                return;
            }
            value_ = value;
            setState(ready);
            cv_.notify_all();
            triggerCallback();
        }

        void setCompletionCallback(const FutureCompletionCallback &callback)
        {
            /*
             * set value and set callback could happen in the same time in different thread.
             * we still want to trigger the callback even if the state is ready.
             */
            std::unique_lock<std::mutex> lck(mutex_);
            completionCallback_ = callback;
            if (isReady()) {
                triggerCallback();
            }
        }

        template<typename F, typename R>
        void apply(const F &func, std::shared_ptr<FutureImpl<R>> future)
        {
            try {
                setValue(func(std::move(Future<R>(future))));
            } catch (std::exception &e) {
                setException(e);
            }
        }

    private:

        T value_;
        FutureCompletionCallback completionCallback_ = nullptr;

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
    class FutureImpl<void> : public FutureImplBase, public std::enable_shared_from_this<FutureImpl<void>>
    {
        using FutureImplPtr = std::shared_ptr<FutureImpl<void>>;
        using FutureCompletionCallback = std::function<void(FutureImplPtr)>;
    public:
        FutureImpl()
        = default;

        void setValue()
        {
            std::unique_lock<std::mutex> lck(mutex_);
            if (isReady()) {
                return;
            }
            setState(ready);
            cv_.notify_all();
            triggerCallback();
        }

        void getValue()
        {
        }

        void setCompletionCallback(const FutureCompletionCallback &callback)
        {
            /*
             * set value and set callback could happen in the same time in different thread.
             * we still want to trigger the callback even if the state is ready.
             */
            std::unique_lock<std::mutex> lck(mutex_);
            completionCallback_ = callback;
            if (isReady()) {
                triggerCallback();
            }
        }

        template<typename F, typename R>
        void apply(const F &func, std::shared_ptr<FutureImpl<R>> future)
        {
            try {
                func(std::move(Future<R>(future)));
                setValue();
            } catch (std::exception &e) {
                setException(e);
            }
        } 

    private:
        FutureCompletionCallback completionCallback_ = nullptr;

        void triggerCallback()
        {
            if (completionCallback_) {
                completionCallback_(this->shared_from_this());
            }
        }
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
            return future_->isReady();
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
