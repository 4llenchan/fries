//
// Created by AllenChan on 2021/11/6.
//
#include "fries/futures/future.hpp"

#include <gtest/gtest.h>

#include <thread>
#include <vector>

using namespace std;
using fries::future;
using fries::promise;

class future_test : public ::testing::Test {};

TEST_F(future_test, general) {
    auto p = make_shared<promise<int>>();
    auto f = p->get_future();

    thread th([p]() {
        this_thread::sleep_for(chrono::seconds(1));
        p->set_value(20);
    });
    th.detach();

    vector<int> seq;
    auto final_future = f.then([&seq](const future<int> &future) {
                             EXPECT_EQ(future.get_value(), 20);
                             seq.emplace_back(1);
                             return string{"hello"};
                         })
                            .then([&seq](const future<string> &future) {
                                EXPECT_EQ(future.get_value(), "hello");
                                seq.emplace_back(2);
                                return 10;
                            })
                            .then([&seq](const future<int> &future) {
                                EXPECT_EQ(future.get_value(), 10);
                                seq.emplace_back(3);
                            })
                            .then([&seq](const future<void> &future) { seq.emplace_back(4); });
    final_future.wait();
    EXPECT_TRUE(final_future.is_ready());

    vector<int> expect{1, 2, 3, 4};
    EXPECT_EQ(seq, expect);
}

TEST_F(future_test, set_before_then) {
    // test set value before then
    auto p = make_shared<promise<void>>();
    auto f = p->get_future();

    p->set_value();

    f.then([](const future<void> &future) {}).then([](const future<void> &future) {}).wait();

    EXPECT_TRUE(true);
}

TEST_F(future_test, exception) {
    auto p = make_shared<promise<int>>();
    auto f = p->get_future();

    auto final = f.then([](const future<int> &future) {
                      EXPECT_EQ(future.get_value(), 10);
                      throw std::out_of_range("");
                  })
                     .then([](const future<void> &future) { EXPECT_TRUE(false); })
                     .capture([](const std::exception &exception) {
                         EXPECT_TRUE(true);
                         exception.what();
                     });

    thread te([p]() {
        this_thread::sleep_for(chrono::seconds(1));
        p->set_value(10);
    });
    te.detach();

    final.wait();
    EXPECT_TRUE(final.has_exception());
}