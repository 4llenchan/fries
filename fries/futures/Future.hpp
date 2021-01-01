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

namespace fries
{
enum FutureState
{
    waiting,
    ready,
};

template<typename T>
class FutureImpl : public std::enable_shared_from_this<FutureImpl<T>>
{
public:
    FutureImpl() : state_(waiting)
    {
    }

    T getValue() const
    {
        return value_;
    }

    void setValue(T value)
    {
        value_ = value;
    }

    FutureState getState() const
    {
        return state_;
    }

    void markFinish()
    {
        state_ = ready;
        cv.notify_all();
    }

    void wait()
    {
        std::unique_lock<std::mutex> lck(mutex);
        if (state_ != ready)
        {
            cv.wait(lck);
        }
    }

public:
    std::mutex mutex{};
    std::condition_variable cv{};

private:
    T value_;
    FutureState state_;
};

template <typename T>
class Promise;

template <typename T>
class Future
{
    friend class Promise<T>;

public:
    bool isReady() const
    {
        return future_->getState() == FutureState::ready;
    }

    void wait()
    {
        future_->wait();
    }

    T getValue()
    {
        return future_->getValue();
    }
private:
    using FutureImplPtr = std::shared_ptr<FutureImpl<T>>;
    FutureImplPtr future_;
    explicit Future(FutureImplPtr futureImpl)
    {
        future_ = futureImpl;
    }
};

template <typename T>
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
        std::unique_lock<std::mutex> lck(future_->mutex);
        future_->setValue(std::move(value));
        future_->markFinish();
    }
private:
    using FutureImplPtr = std::shared_ptr<FutureImpl<T>>;
    FutureImplPtr future_;
};

}

#endif /* FRIES_FUTURE_H */
