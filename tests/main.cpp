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
        this_thread::sleep_for(chrono::seconds(1));
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

    // test set value before then
    auto pv = make_shared<Promise<void>>();
    auto fv = pv->getFuture();

    pv->setValue();

    fv.then([](const Future<void> &future) {
        cout << "void1" << endl;
    }).then([](const Future<void> &future) {
        cout << "void2" << endl;
    }).wait();


    // test exception
    auto pe = make_shared<Promise<int>>();
    auto fe = pe->getFuture();

    auto feFinal = fe.then([](const Future<int> &future) {
        cout << "pe value: " << future.getValue() << endl;
        throw std::exception();
    }).capture([](const std::exception &exception) {
        cout << "pe exception: " << exception.what() << endl;
    });

    thread te([pe]() {
        this_thread::sleep_for(chrono::seconds(1));
//        pe->setException(exception());
        pe->setValue(10);
    });
    te.detach();

    feFinal.wait();
    if (feFinal.hasException()) {
        cout << "e: " << feFinal.getException().what() << endl;
    }

    return 0;
}
