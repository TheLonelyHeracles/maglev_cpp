#pragma once

#include <cassert>

#include <type_traits>
#include <utility>

#include "maglev/util/type_traits.h"

namespace maglev {


template <typename NodeGroupBaseType>
class WeightedNodeGroupWrapper : public NodeGroupBaseType {
  using base_t = NodeGroupBaseType;

public:
  using node_t = typename base_t::node_t;

  static_assert(is_weighted_t<node_t>::value, "node should be wrapped by WeightedNodeWrapper");
  using weighted_t = typename std::enable_if<is_weighted_t<node_t>::value, typename node_t::weighted_t>::type;

public:

  /// limit max_weight by max_weight <= avg_weight * max_avg_rate_limit, 0 means no limit
  template <typename ...Args>
  WeightedNodeGroupWrapper(double max_avg_rate_limit = 0, Args&& ...args) :
      max_avg_rate_limit_(max_avg_rate_limit), base_t(std::forward<Args>(args)...) {}

  virtual void ready_go() override {
    base_t::ready_go();
    init_weight();
  }

  void set_max_avg_rate_limit(double r) { max_avg_rate_limit_ = r; }

  double max_avg_rate_limit() const { return max_avg_rate_limit_; }

  int weight_sum() const { return weight_sum_; }

  double avg_weight() const { return avg_weight_; }

  // limited max weight, which is really used in maglev
  int max_weight() const { return max_weight_; }

  int real_max_weight() const { return real_max_weight_; }

  bool max_weight_limited() const { return max_weight() < real_max_weight(); }

  void init_weight() {
    if (base_t::empty()) return;

    weight_sum_ = 0;
    real_max_weight_ = 0;
    for (const auto& i : *this) {
      int w = i->weight();
      weight_sum_ += w;
      if (w > real_max_weight_) {
        real_max_weight_ = w;
      }
    }
    avg_weight_ = double(weight_sum_) / double(base_t::size());
    max_weight_ = real_max_weight_;
    if (max_avg_rate_limit_ > 0) {
      int limit = int(max_avg_rate_limit_ * double(weight_sum_) / double(base_t::size()));
      if (max_weight_ > limit) {
        max_weight_ = limit;
      }
    }
    assert(real_max_weight_ > 0);
    assert(max_weight_ > 0);
    assert(weight_sum_ > 0);
    assert(avg_weight_ > 0);
  }

private:
  // user defined
  double max_avg_rate_limit_ = 0;
  // internal params
  int weight_sum_ = 0;
  int max_weight_ = 0;
  int real_max_weight_ = 0;
  double avg_weight_ = 0;
};


template <typename Char, typename Traits, typename NodeGroupBaseType>
std::basic_ostream<Char, Traits>& operator<<(std::basic_ostream<Char, Traits>& os,
                                             const WeightedNodeGroupWrapper<NodeGroupBaseType>& wng) {
  os << "{" << static_cast<const NodeGroupBaseType&>(wng)
     << ",sz:" << wng.size()
     << ",w_sum:" << wng.weight_sum()
     << ",avg_w:" << wng.avg_weight()
     << ",max_w:" << wng.max_weight();
  if (wng.max_weight_limited()) {
    os << "<" << wng.real_max_weight();
  } else {
    os << "=";
  }
  os << ",w_lmt:" << wng.max_avg_rate_limit() << "}";
  return os;
}


}  // namespace maglev
