#include <iostream>
#include <memory>
#include <thread>
#include <fries/futures/Future.hpp>

int main()
{
    using namespace fries;
    using namespace std;
    auto p = std::make_shared<Promise<int>>();
    auto f = p->getFuture();

    thread th([p](){
        this_thread::sleep_for(chrono::seconds(2));
        p->setValue(20);
    });
    th.detach();

    f.wait();

    std::cout << f.isReady() << std::endl;
    std::cout << f.getValue() << std::endl;

    return 0;
}
