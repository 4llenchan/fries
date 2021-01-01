//
//  Future.hpp
//
//  Created by AllenChan on 2020/12/5.
//

#ifndef FRIES_FUTURE_H
#define FRIES_FUTURE_H

#include <memory>

namespace fries
{
template<typename T>
class FutureBase : public std::enable_shared_from_this<FutureBase<T>>
{
public:
    T getValue() const
    {
        return value_;
    }

    void setValue(T value)
    {
        value_ = value;
    }

private:
    T value_;
};

template <typename T>
class Promise;

template <typename T>
class Future
{
    friend class Promise<T>;

public:
    T getValue()
    {
        return future_->getValue();
    }
private:
    using FutureBasePtr = std::shared_ptr<FutureBase<T>>;
    FutureBasePtr future_;
    explicit Future(FutureBasePtr futureBase)
    {
        future_ = futureBase;
    }
};

template <typename T>
class Promise
{
public:
    Promise() : future_(std::make_shared<FutureBase<T>>())
    {
    }

    Future<T> getFuture()
    {
        return Future<T>(future_);
    }

    void setValue(T value)
    {
        future_->setValue(value);
    }
private:
    using FutureBasePtr = std::shared_ptr<FutureBase<T>>;
    FutureBasePtr future_;
};

}

#endif /* FRIES_FUTURE_H */
