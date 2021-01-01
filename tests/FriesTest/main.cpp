#include <iostream>
#include <memory>
#include <fries/futures/Future.hpp>

int main()
{
    using namespace fries;
    auto p = std::make_shared<Promise<int>>();
    auto f = p->getFuture();
    p->setValue(20);
    std::cout << f.getValue() << std::endl;
    return 0;
}
