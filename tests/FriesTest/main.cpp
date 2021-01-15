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

    thread th([p]() {
        this_thread::sleep_for(chrono::seconds(2));
        p->setValue(20);
    });
    th.detach();

    auto finalFuture = f.then([](const Future<int> &future) {
        cout << "1. " << future.getValue() << endl;
        return string("hello");
    }).then([](const Future<string> &future) {
        cout << "2. " << future.getValue() << endl;
        return 10;
    }).then([](const Future<int> &future) {
        cout << "3. " << future.getValue() << endl;
        return nullptr;
    });

    finalFuture.wait();

    return 0;
}
