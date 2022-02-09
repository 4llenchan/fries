//
// Created by AllenChan on 2022/2/10.
//
#include "fries/system/switcher.hpp"

#include <gtest/gtest.h>

using namespace fries::system;

class switcher_test : public ::testing::Test {};

class custom_type : public switchable {
public:
    explicit custom_type(int value) : value_(value) {}

    int get_value() const { return value_; }

    size_t get_hash() const override { return std::hash<int>()(value_); }

private:
    int value_;
};

ENABLE_SWITCH(custom_type)

TEST_F(switcher_test, custom_type) {
    custom_type ct1(1);
    custom_type ct2(2);
    custom_type ct3(3);

    custom_type t(3);

    select(t)
        .found(ct1, []() { EXPECT_TRUE(false); })
        .found(ct2, []() { EXPECT_TRUE(false); })
        .found(ct3, []() { EXPECT_TRUE(true); })
        .others([]() { EXPECT_TRUE(false); });

    select(t)
        .found(ct1, []() { EXPECT_TRUE(false); })
        .found(ct2, []() { EXPECT_TRUE(false); })
        .found(ct3, [](const custom_type& ct) { EXPECT_EQ(ct.get_value(), 3); })
        .others([](const custom_type& ct) { EXPECT_TRUE(false); });
}

TEST_F(switcher_test, string_type) {
    using std::string;
    string s1 = "a";
    string s2 = "b";

    string t = "b";

    select(t)
        .found(s1, []() { EXPECT_TRUE(false); })
        .found("d", []() { EXPECT_TRUE(false); })
        .found(s2, []() { EXPECT_TRUE(true); })
        .others([]() { EXPECT_TRUE(false); });
}

TEST_F(switcher_test, integers) {
    int t = 10;
    select(t).found(5, []() { EXPECT_TRUE(false); }).others([](int target) {
        EXPECT_EQ(target, 10);
        EXPECT_TRUE(true);
    });
}
