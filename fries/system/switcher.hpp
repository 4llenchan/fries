//
// Created by AllenChan on 2022/2/10.
//

#ifndef FRIES_SYSTEM_SWITCHER_H_
#define FRIES_SYSTEM_SWITCHER_H_

#include <functional>
#include <map>
#include <string>
#include <unordered_map>

namespace fries {
namespace system {

class switchable {
 public:
  virtual size_t get_hash() const = 0;

  bool operator==(const switchable& other) const { return get_hash() == other.get_hash(); }
};

#define ENABLE_SWITCH(TypeName)                                         \
  namespace std {                                                       \
  template <>                                                           \
  struct hash<TypeName> {                                               \
    size_t operator()(const TypeName& k) const { return k.get_hash(); } \
  };                                                                    \
  }  // namespace std

template <typename Type>
class switcher {
 public:
  using hit = std::function<void(void)>;
  using hit_with_target = std::function<void(const Type&)>;

  explicit switcher(const Type& target) : target_(target) {}

  switcher& found(const Type& _case, const hit& h) {
    reflections_[_case] = h;
    return *this;
  }

  switcher& found(const Type& _case, const hit_with_target& h) {
    reflections_[_case] = [this, &h]() { h(target_); };
    return *this;
  }

  void others(const hit& h) {
    others_ = h;
    done();
  }

  void others(const hit_with_target& h) {
    others_ = [this, &h]() { h(target_); };
    done();
  }

  void done() {
    auto it = reflections_.find(target_);
    if (it != reflections_.end()) {
      auto h = it->second;
      h();
    } else if (others_ != nullptr) {
      others_();
    }
  }

 private:
  const Type& target_;
  hit others_;
  std::unordered_map<Type, hit> reflections_;
};

template <typename ExpressionType>
switcher<ExpressionType> select(const ExpressionType& expression) {
  return switcher<ExpressionType>(expression);
}
}  // namespace system
}  // namespace fries

#endif  // FRIES_SYSTEM_SWITCHER_H_
