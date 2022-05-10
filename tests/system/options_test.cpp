//
// Created by AllenChan on 2022/5/10.
//

#include "fries/system/options.hpp"

#include "gtest/gtest.h"

namespace fries {

class options_test : public ::testing::Test {};

enum class test_option {
  option1 = 1 << 0,
  option2 = 1 << 1,
  option3 = 1 << 2,
};

using test_options = system::options<test_option>;

TEST_F(options_test, enum_test) {
  test_options opt(test_option::option1);
  EXPECT_TRUE(opt.has(test_option::option1));
  EXPECT_FALSE(opt.has(test_option::option2));
  EXPECT_FALSE(opt.has(test_option::option3));

  opt |= test_option::option2;
  EXPECT_TRUE(opt.has(test_option::option2));
  opt.exclude(test_option::option1);
  EXPECT_FALSE(opt.has(test_option::option1));

  test_options opt1 = opt | test_option::option1;
  EXPECT_TRUE(opt1.has(test_option::option1));
  EXPECT_TRUE(opt1.has(test_option::option2));

  test_options opt2 = {test_option::option1, test_option::option3};
  EXPECT_TRUE(opt2.has(test_option::option1));
  EXPECT_TRUE(opt2.has(test_option::option3));

  opt |= opt2;
  EXPECT_TRUE(opt.has(test_option::option1));
  EXPECT_TRUE(opt.has(test_option::option2));
  EXPECT_TRUE(opt.has(test_option::option3));

  test_options opt3(test_option::option2);
  test_options opt4 = opt3 | opt2;
  EXPECT_TRUE(opt4.has(test_option::option1));
  EXPECT_TRUE(opt4.has(test_option::option2));
  EXPECT_TRUE(opt4.has(test_option::option3));
}

}  // namespace fries
