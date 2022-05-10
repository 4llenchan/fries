//
// Created by AllenChan on 2022/5/10.
//

#ifndef FRIES_SYSTEM_OPTIONS_HPP_
#define FRIES_SYSTEM_OPTIONS_HPP_

#include <cstdint>
#include <type_traits>
#include <utility>

namespace fries {
namespace system {
template <typename OptionType, typename Enable = void>
class options {};

/// \brief Template class to enable options for an enum.
template <typename option_type>
class options<option_type, typename std::enable_if<std::is_enum<option_type>::value>::type> {
 public:
  explicit options(option_type option) : options_(cast_(option)) {}

  options(const std::initializer_list<option_type>& options) : options_(0) {
    for (auto option : options) {
      options_ |= cast_(option);
    }
  }

  options& operator|(const options<option_type>& other) {
    if (this != &other) {
      options_ |= other.options_;
    }
    return *this;
  }

  options& operator|=(const options<option_type>& other) {
    if (this != &other) {
      options_ |= other.options_;
    }
    return *this;
  }

  options& operator|(option_type option) {
    options_ |= cast_(option);
    return *this;
  }

  options& operator|=(option_type option) {
    options_ |= cast_(option);
    return *this;
  }

  inline bool has(option_type option) const { return (options_ & cast_(option)) > 0; }

  inline void exclude(option_type option) { options_ &= ~cast_(option); }

 private:
  using value_type = uint64_t;
  static inline value_type cast_(option_type option) { return static_cast<value_type>(option); }

  value_type options_;
};
}  // namespace system
}  // namespace fries

#endif  // FRIES_SYSTEM_OPTIONS_HPP_
