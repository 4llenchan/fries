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
    }).then([](const Future<void> &future) {
        cout << "4. void" << endl;
    });

    cout << "ready state before wait:" << finalFuture.isReady() << endl;

    finalFuture.wait();

    cout << "ready state after wait:" << finalFuture.isReady() << endl;


    auto pv = make_shared<Promise<void>>();
    auto fv = pv->getFuture();

    thread thv([pv]() {
        this_thread::sleep_for(chrono::seconds(2));
        pv->setValue();
    });
    thv.detach();

    fv.then([](const Future<void> &future) {
        cout << "void" << endl;
    }).wait();

    return 0;
}
