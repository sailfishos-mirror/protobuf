#include "google/protobuf/repeated_field_proxy.h"

#include <cstdint>
#include <functional>
#include <limits>
#include <string>
#include <utility>
#include <vector>

#include <gmock/gmock.h>
#include <gtest/gtest.h>
#include "absl/algorithm/container.h"
#include "absl/strings/cord.h"
#include "absl/strings/string_view.h"
#include "google/protobuf/arena.h"
#include "google/protobuf/repeated_field.h"
#include "google/protobuf/repeated_ptr_field.h"
#include "google/protobuf/test_protos/repeated_field_proxy_test.pb.h"
#include "google/protobuf/test_textproto.h"


namespace google {
namespace protobuf {
namespace internal {
namespace {

using ::proto2_unittest::RepeatedFieldProxyTestSimpleMessage;
using ::testing::Address;
using ::testing::ElementsAre;
using ::testing::IsEmpty;

template <typename T>
T StrAs(absl::string_view s) {
  return T(s);
}

// A test-only container for a repeated field that manages construction and
// destruction of the underlying repeated field, and can construct proxies.
//
// This is necessary because RepeatedFieldProxy has no public constructors,
// aside from copy assignment.
template <typename T>
class TestOnlyRepeatedFieldContainer {
  using FieldType = typename internal::RepeatedFieldTraits<T>::type;

 public:
  static TestOnlyRepeatedFieldContainer<T> New(Arena* arena) {
    return TestOnlyRepeatedFieldContainer<T>(Arena::Create<FieldType>(arena),
                                             arena);
  }

  ~TestOnlyRepeatedFieldContainer() {
    if (arena_ == nullptr) {
      delete field_;
    }
  }

  FieldType& operator*() { return *field_; }
  const FieldType& operator*() const { return *field_; }

  FieldType* operator->() { return &*field_; }
  const FieldType* operator->() const { return &*field_; }

  RepeatedFieldProxy<T> MakeProxy() {
    return internal::ConstructRepeatedFieldProxy<T>(*field_, arena_);
  }
  RepeatedFieldProxy<const T> MakeConstProxy() const {
    return internal::ConstructRepeatedFieldProxy<const T>(*field_);
  }

 private:
  TestOnlyRepeatedFieldContainer(FieldType* field, Arena* arena)
      : field_(field), arena_(arena) {}

  FieldType* field_;
  Arena* arena_;
};

class RepeatedFieldProxyTest : public testing::TestWithParam<bool> {
 protected:
  bool UseArena() const { return GetParam(); }
  Arena* arena() { return UseArena() ? &arena_ : nullptr; }

  template <typename T>
  TestOnlyRepeatedFieldContainer<T> MakeRepeatedFieldContainer() {
    return TestOnlyRepeatedFieldContainer<T>::New(arena());
  }

 private:
  Arena arena_;
};

TEST_P(RepeatedFieldProxyTest, RepeatedInt32) {
  auto field = MakeRepeatedFieldContainer<int32_t>();
  RepeatedFieldProxy<int32_t> proxy = field.MakeProxy();
  proxy.push_back(1);
  proxy.push_back(2);
  proxy.push_back(3);
  EXPECT_THAT(proxy, ElementsAre(1, 2, 3));
  EXPECT_THAT(*field, ElementsAre(1, 2, 3));

  proxy.set(1, 4);
  EXPECT_THAT(proxy, ElementsAre(1, 4, 3));
  EXPECT_THAT(*field, ElementsAre(1, 4, 3));
}

TEST_P(RepeatedFieldProxyTest, ConstRepeatedInt32) {
  auto field = MakeRepeatedFieldContainer<int32_t>();
  field->Add(1);
  field->Add(2);
  field->Add(3);

  {
    RepeatedFieldProxy<const int32_t> proxy = field.MakeConstProxy();
    EXPECT_THAT(proxy, ElementsAre(1, 2, 3));
  }

  {
    RepeatedFieldProxy<const int32_t> proxy = field.MakeProxy();
    EXPECT_THAT(proxy, ElementsAre(1, 2, 3));
  }
}

TEST_P(RepeatedFieldProxyTest, RepeatedUint32) {
  auto field = MakeRepeatedFieldContainer<uint32_t>();
  RepeatedFieldProxy<uint32_t> proxy = field.MakeProxy();
  proxy.push_back(1);
  EXPECT_THAT(proxy, ElementsAre(1));
}

TEST_P(RepeatedFieldProxyTest, RepeatedInt64) {
  auto field = MakeRepeatedFieldContainer<int64_t>();
  RepeatedFieldProxy<int64_t> proxy = field.MakeProxy();
  proxy.push_back(std::numeric_limits<int64_t>::min());
  EXPECT_THAT(proxy, ElementsAre(std::numeric_limits<int64_t>::min()));
}

TEST_P(RepeatedFieldProxyTest, RepeatedUint64) {
  auto field = MakeRepeatedFieldContainer<uint64_t>();
  RepeatedFieldProxy<uint64_t> proxy = field.MakeProxy();
  proxy.push_back(std::numeric_limits<uint64_t>::max());
  EXPECT_THAT(proxy, ElementsAre(std::numeric_limits<uint64_t>::max()));
}

TEST_P(RepeatedFieldProxyTest, RepeatedFloat) {
  auto field = MakeRepeatedFieldContainer<float>();
  RepeatedFieldProxy<float> proxy = field.MakeProxy();
  proxy.push_back(1.5);
  EXPECT_THAT(proxy, ElementsAre(1.5));
}

TEST_P(RepeatedFieldProxyTest, RepeatedDouble) {
  auto field = MakeRepeatedFieldContainer<double>();
  RepeatedFieldProxy<double> proxy = field.MakeProxy();
  proxy.push_back(std::numeric_limits<double>::max());
  EXPECT_THAT(proxy, ElementsAre(std::numeric_limits<double>::max()));
}

TEST_P(RepeatedFieldProxyTest, RepeatedString) {
  auto field = MakeRepeatedFieldContainer<std::string>();
  RepeatedFieldProxy<std::string> proxy = field.MakeProxy();
  proxy.push_back("one");
  proxy.push_back("two");
  proxy.push_back("three");
  EXPECT_THAT(proxy, ElementsAre("one", "two", "three"));
  EXPECT_THAT(*field, ElementsAre("one", "two", "three"));

  proxy[1] = "four";
  EXPECT_THAT(proxy, ElementsAre("one", "four", "three"));
  EXPECT_THAT(*field, ElementsAre("one", "four", "three"));
}

TEST_P(RepeatedFieldProxyTest, ConstRepeatedString) {
  auto field = MakeRepeatedFieldContainer<std::string>();
  field->Add("one");
  field->Add("two");
  field->Add("three");

  {
    RepeatedFieldProxy<const std::string> proxy = field.MakeConstProxy();
    EXPECT_THAT(proxy, ElementsAre("one", "two", "three"));
  }

  {
    RepeatedFieldProxy<const std::string> proxy = field.MakeProxy();
    EXPECT_THAT(proxy, ElementsAre("one", "two", "three"));
  }
}

TEST_P(RepeatedFieldProxyTest, RepeatedMessage) {
  auto field =
      MakeRepeatedFieldContainer<RepeatedFieldProxyTestSimpleMessage>();
  RepeatedFieldProxy<RepeatedFieldProxyTestSimpleMessage> proxy =
      field.MakeProxy();
  proxy.emplace_back().set_value(1);

  RepeatedFieldProxyTestSimpleMessage msg;
  msg.set_value(2);
  proxy.push_back(std::move(msg));
  EXPECT_THAT(proxy, ElementsAre(EqualsProto(R"pb(value: 1)pb"),
                                 EqualsProto(R"pb(value: 2)pb")));
  EXPECT_THAT(*field, ElementsAre(EqualsProto(R"pb(value: 1)pb"),
                                  EqualsProto(R"pb(value: 2)pb")));
}

TEST_P(RepeatedFieldProxyTest, ConstRepeatedMessage) {
  auto field =
      MakeRepeatedFieldContainer<RepeatedFieldProxyTestSimpleMessage>();
  field->Add()->set_value(1);
  field->Add()->set_value(2);

  {
    RepeatedFieldProxy<const RepeatedFieldProxyTestSimpleMessage> proxy =
        field.MakeConstProxy();
    EXPECT_THAT(proxy, ElementsAre(EqualsProto(R"pb(value: 1)pb"),
                                   EqualsProto(R"pb(value: 2)pb")));
  }

  {
    RepeatedFieldProxy<const RepeatedFieldProxyTestSimpleMessage> proxy =
        field.MakeProxy();
    EXPECT_THAT(proxy, ElementsAre(EqualsProto(R"pb(value: 1)pb"),
                                   EqualsProto(R"pb(value: 2)pb")));
  }
}

TEST_P(RepeatedFieldProxyTest, Empty) {
  auto field =
      MakeRepeatedFieldContainer<RepeatedFieldProxyTestSimpleMessage>();
  RepeatedFieldProxy<RepeatedFieldProxyTestSimpleMessage> proxy =
      field.MakeProxy();
  EXPECT_TRUE(proxy.empty());
  proxy.emplace_back();
  EXPECT_FALSE(proxy.empty());
}

TEST_P(RepeatedFieldProxyTest, ConstEmpty) {
  auto field =
      MakeRepeatedFieldContainer<RepeatedFieldProxyTestSimpleMessage>();

  {
    RepeatedFieldProxy<const RepeatedFieldProxyTestSimpleMessage> proxy =
        field.MakeConstProxy();
    EXPECT_TRUE(proxy.empty());
  }

  field->Add();
  {
    RepeatedFieldProxy<const RepeatedFieldProxyTestSimpleMessage> proxy =
        field.MakeConstProxy();
    EXPECT_FALSE(proxy.empty());
  }
}

TEST_P(RepeatedFieldProxyTest, Size) {
  auto field =
      MakeRepeatedFieldContainer<RepeatedFieldProxyTestSimpleMessage>();
  RepeatedFieldProxy<RepeatedFieldProxyTestSimpleMessage> proxy =
      field.MakeProxy();
  EXPECT_EQ(proxy.size(), 0);

  proxy.emplace_back();
  EXPECT_EQ(proxy.size(), 1);

  proxy.emplace_back();
  proxy.emplace_back();
  EXPECT_EQ(proxy.size(), 3);
}

TEST_P(RepeatedFieldProxyTest, ConstSize) {
  auto field =
      MakeRepeatedFieldContainer<RepeatedFieldProxyTestSimpleMessage>();

  {
    RepeatedFieldProxy<RepeatedFieldProxyTestSimpleMessage> proxy =
        field.MakeProxy();
    EXPECT_EQ(proxy.size(), 0);
  }

  field->Add();
  {
    RepeatedFieldProxy<const RepeatedFieldProxyTestSimpleMessage> proxy =
        field.MakeConstProxy();
    EXPECT_EQ(proxy.size(), 1);
  }
}

TEST_P(RepeatedFieldProxyTest, ArrayIndexing) {
  auto field =
      MakeRepeatedFieldContainer<RepeatedFieldProxyTestSimpleMessage>();
  field->Add()->set_value(1);
  field->Add()->set_value(2);
  field->Add()->set_value(3);

  {
    auto proxy = field.MakeProxy();
    EXPECT_THAT(proxy[0], EqualsProto(R"pb(value: 1)pb"));
    EXPECT_THAT(proxy[1], EqualsProto(R"pb(value: 2)pb"));
    EXPECT_THAT(proxy[2], EqualsProto(R"pb(value: 3)pb"));
  }

  {
    auto proxy = field.MakeConstProxy();
    EXPECT_THAT(proxy[0], EqualsProto(R"pb(value: 1)pb"));
    EXPECT_THAT(proxy[1], EqualsProto(R"pb(value: 2)pb"));
    EXPECT_THAT(proxy[2], EqualsProto(R"pb(value: 3)pb"));
  }
}

TEST_P(RepeatedFieldProxyTest, MutateElementPrimitive) {
  auto field = MakeRepeatedFieldContainer<int32_t>();
  field->Add(1);
  field->Add(2);
  field->Add(3);

  {
    auto proxy = field.MakeProxy();
    proxy.set(0, 4);

    EXPECT_EQ(proxy[0], 4);
    EXPECT_EQ(proxy[1], 2);
    EXPECT_EQ(proxy[2], 3);
  }
}

TEST_P(RepeatedFieldProxyTest, MutateElementString) {
  auto field = MakeRepeatedFieldContainer<std::string>();
  field->Add("1");
  field->Add("2");
  field->Add("3");
  field->Add("4");

  {
    auto proxy = field.MakeProxy();
    proxy[0] = "5";
    proxy[1] = StrAs<std::string>("6");
    const char* c_str = "7";
    proxy[2] = c_str;
    proxy[3] = StrAs<absl::string_view>("8");

    EXPECT_EQ(proxy[0], "5");
    EXPECT_EQ(proxy[1], "6");
    EXPECT_EQ(proxy[2], "7");
    EXPECT_EQ(proxy[3], "8");

    proxy.set(0, "9");
    proxy.set(1, std::string("10"));
    const char* c_str2 = "11";
    proxy.set(2, c_str2);
    proxy.set(3, absl::string_view("12"));

    EXPECT_EQ(proxy[0], "9");
    EXPECT_EQ(proxy[1], "10");
    EXPECT_EQ(proxy[2], "11");
    EXPECT_EQ(proxy[3], "12");
  }
}

TEST_P(RepeatedFieldProxyTest, MutateElementStringView) {
  auto field = MakeRepeatedFieldContainer<absl::string_view>();
  field->Add("1");
  field->Add("2");
  field->Add("3");
  field->Add("4");

  {
    auto proxy = field.MakeProxy();

    proxy.set(0, "5");
    proxy.set(1, StrAs<std::string>("6"));
    const char* c_str = "7";
    proxy.set(2, c_str);
    proxy.set(3, StrAs<absl::string_view>("8"));

    EXPECT_EQ(proxy[0], "5");
    EXPECT_EQ(proxy[1], "6");
    EXPECT_EQ(proxy[2], "7");
    EXPECT_EQ(proxy[3], "8");

    // A type which is implicitly convertible to both `std::string` and
    // `absl::string_view` to test ambiguity resolution.
    struct StringLike {
      std::string value;

      operator std::string() && { return std::move(value); }
      operator absl::string_view() const { return value; }
    };

    StringLike string_like = {"long string that will be heap allocated"};
    const char* string_ptr = string_like.value.c_str();

    proxy.set(0, std::move(string_like));
    EXPECT_EQ(proxy[0], "long string that will be heap allocated");
    // With the ranked overloads, the `std::string&&` overload should be chosen,
    // and string_like should have been moved.
    EXPECT_EQ(string_like.value, "");
    EXPECT_EQ(string_ptr, proxy[0].data());

    std::string str_val = "9";
    std::reference_wrapper<std::string> string_ref = str_val;
    proxy.set(1, string_ref);
    proxy.set(2, std::ref("10"));
    proxy.set(3, std::cref("11"));
    EXPECT_EQ(proxy[1], "9");
    EXPECT_EQ(proxy[2], "10");
    EXPECT_EQ(proxy[3], "11");
  }
}

TEST_P(RepeatedFieldProxyTest, MutateElementCord) {
  auto field = MakeRepeatedFieldContainer<absl::Cord>();
  field->Add(absl::Cord("1"));
  field->Add(absl::Cord("2"));
  field->Add(absl::Cord("3"));
  field->Add(absl::Cord("4"));

  {
    auto proxy = field.MakeProxy();

    proxy[0] = "5";
    proxy[1] = std::string("6");
    const char* c_str = "7";
    proxy[2] = c_str;
    proxy[3] = absl::string_view("8");

    EXPECT_EQ(proxy[0], "5");
    EXPECT_EQ(proxy[1], "6");
    EXPECT_EQ(proxy[2], "7");
    EXPECT_EQ(proxy[3], "8");

    proxy.set(0, "9");
    proxy.set(1, StrAs<std::string>("10"));
    const char* c_str2 = "11";
    proxy.set(2, c_str2);
    proxy.set(3, StrAs<absl::string_view>("12"));

    EXPECT_EQ(proxy[0], "9");
    EXPECT_EQ(proxy[1], "10");
    EXPECT_EQ(proxy[2], "11");
    EXPECT_EQ(proxy[3], "12");

    auto cord = absl::Cord("13");
    proxy.set(0, std::move(cord));
    EXPECT_EQ(proxy[0], "13");
    EXPECT_EQ(cord, "");
  }
}

TEST_P(RepeatedFieldProxyTest, MutateElementMessage) {
  auto field =
      MakeRepeatedFieldContainer<RepeatedFieldProxyTestSimpleMessage>();
  field->Add()->set_value(1);
  field->Add()->set_value(2);
  field->Add()->set_value(3);

  {
    auto proxy = field.MakeProxy();
    proxy[2].set_value(4);

    EXPECT_THAT(proxy[0], EqualsProto(R"pb(value: 1)pb"));
    EXPECT_THAT(proxy[1], EqualsProto(R"pb(value: 2)pb"));
    EXPECT_THAT(proxy[2], EqualsProto(R"pb(value: 4)pb"));

    RepeatedFieldProxyTestSimpleMessage msg;
    msg.set_value(5);
    proxy.set(0, RepeatedFieldProxyTestSimpleMessage(msg));
    // `msg` is copied into the element, so it should be unchanged.
    EXPECT_TRUE(msg.has_value());

    RepeatedFieldProxyTestSimpleMessage msg2;
    msg2.set_value(6);
    proxy.set(1, std::move(msg2));

    EXPECT_THAT(proxy[0], EqualsProto(R"pb(value: 5)pb"));
    EXPECT_THAT(proxy[1], EqualsProto(R"pb(value: 6)pb"));
    EXPECT_THAT(proxy[2], EqualsProto(R"pb(value: 4)pb"));
  }
}

TEST_P(RepeatedFieldProxyTest, PushBack) {
  auto field = MakeRepeatedFieldContainer<int32_t>();
  auto proxy = field.MakeProxy();
  proxy.push_back(1);
  proxy.push_back(2);
  proxy.push_back(3);

  EXPECT_THAT(*field, ElementsAre(1, 2, 3));
}

TEST_P(RepeatedFieldProxyTest, EmplaceBack) {
  auto field = MakeRepeatedFieldContainer<int32_t>();
  auto proxy = field.MakeProxy();
  proxy.emplace_back(1);
  proxy.emplace_back(2);
  proxy.emplace_back(3);

  EXPECT_THAT(*field, ElementsAre(1, 2, 3));
}

TEST_P(RepeatedFieldProxyTest, Iterators) {
  auto field = MakeRepeatedFieldContainer<uint32_t>();
  RepeatedFieldProxy<uint32_t> proxy = field.MakeProxy();
  proxy.push_back(1);
  proxy.push_back(2);
  proxy.push_back(3);

  auto it = proxy.begin();
  EXPECT_EQ(*it, 1);
  EXPECT_EQ(*(++it), 2);
  EXPECT_EQ(*(++it), 3);
  EXPECT_EQ(++it, proxy.end());

  // Post-increment
  it = proxy.begin();
  EXPECT_EQ(*(it++), 1);

  // Pre-decrement
  it = proxy.end();
  EXPECT_EQ(*(--it), 3);

  // Post-decrement
  EXPECT_EQ(*(it--), 3);
  EXPECT_EQ(*it, 2);

  // Equality
  EXPECT_TRUE(proxy.begin() == proxy.begin());
  EXPECT_TRUE(proxy.end() == proxy.end());
  EXPECT_FALSE(proxy.begin() == proxy.end());

  // Inequality
  EXPECT_FALSE(proxy.begin() != proxy.begin());
  EXPECT_FALSE(proxy.end() != proxy.end());
  EXPECT_TRUE(proxy.begin() != proxy.end());

  // Random access
  it = proxy.begin();
  EXPECT_EQ(*(it + 0), 1);
  EXPECT_EQ(*(it + 1), 2);
  EXPECT_EQ(*(it + 2), 3);

  // Add assign
  it += 2;
  EXPECT_EQ(*(it - 1), 2);

  // Subtract assign
  it -= 1;
  EXPECT_EQ(*it, 2);

  // Indexing
  EXPECT_EQ(it[1], 3);

  // Reverse iterators
  auto rit = proxy.rbegin();
  EXPECT_EQ(*rit, 3);
  EXPECT_EQ(*(++rit), 2);
  EXPECT_EQ(*(++rit), 1);
  EXPECT_EQ(++rit, proxy.rend());
}

TEST_P(RepeatedFieldProxyTest, IteratorMutation) {
  auto field = MakeRepeatedFieldContainer<uint32_t>();
  RepeatedFieldProxy<uint32_t> proxy = field.MakeProxy();
  proxy.push_back(1);
  proxy.push_back(2);
  proxy.push_back(3);

  auto it = proxy.begin();
  *it = 4;
  *(++it) = 5;
  EXPECT_THAT(proxy, ElementsAre(4, 5, 3));
  EXPECT_THAT(*field, ElementsAre(4, 5, 3));

  auto rit = proxy.rbegin();
  *rit = 6;
  *(++rit) = 7;
  EXPECT_THAT(proxy, ElementsAre(4, 7, 6));
  EXPECT_THAT(*field, ElementsAre(4, 7, 6));
}

TEST_P(RepeatedFieldProxyTest, ConstIterators) {
  auto field = MakeRepeatedFieldContainer<uint32_t>();
  field->Add(1);
  field->Add(2);
  field->Add(3);

  RepeatedFieldProxy<const uint32_t> proxy = field.MakeConstProxy();
  auto it = proxy.cbegin();
  EXPECT_EQ(*it, 1);
  EXPECT_EQ(*(++it), 2);
  EXPECT_EQ(*(++it), 3);
  EXPECT_EQ(++it, proxy.cend());

  auto rit = proxy.rbegin();
  EXPECT_EQ(*rit, 3);
  EXPECT_EQ(*(++rit), 2);
  EXPECT_EQ(*(++rit), 1);
  EXPECT_EQ(++rit, proxy.rend());
}

TEST_P(RepeatedFieldProxyTest, PopBack) {
  auto field =
      MakeRepeatedFieldContainer<RepeatedFieldProxyTestSimpleMessage>();
  field->Add()->set_value(1);
  field->Add()->set_value(2);

  RepeatedFieldProxy<RepeatedFieldProxyTestSimpleMessage> proxy =
      field.MakeProxy();
  proxy.pop_back();

  EXPECT_THAT(proxy, ElementsAre(EqualsProto(R"pb(value: 1)pb")));
  EXPECT_THAT(*field, ElementsAre(EqualsProto(R"pb(value: 1)pb")));
}

TEST_P(RepeatedFieldProxyTest, Clear) {
  auto field =
      MakeRepeatedFieldContainer<RepeatedFieldProxyTestSimpleMessage>();
  field->Add()->set_value(1);
  field->Add()->set_value(2);

  RepeatedFieldProxy<RepeatedFieldProxyTestSimpleMessage> proxy =
      field.MakeProxy();
  proxy.clear();

  EXPECT_THAT(proxy, IsEmpty());
  EXPECT_THAT(*field, IsEmpty());
}

TEST_P(RepeatedFieldProxyTest, Erase) {
  auto field =
      MakeRepeatedFieldContainer<RepeatedFieldProxyTestSimpleMessage>();
  field->Add()->set_value(1);
  field->Add()->set_value(2);
  field->Add()->set_value(3);

  RepeatedFieldProxy<RepeatedFieldProxyTestSimpleMessage> proxy =
      field.MakeProxy();
  proxy.erase(absl::c_find_if(
      proxy, [](const RepeatedFieldProxyTestSimpleMessage& msg) {
        return msg.value() == 2;
      }));

  EXPECT_THAT(proxy, ElementsAre(EqualsProto(R"pb(value: 1)pb"),
                                 EqualsProto(R"pb(value: 3)pb")));
  EXPECT_THAT(*field, ElementsAre(EqualsProto(R"pb(value: 1)pb"),
                                  EqualsProto(R"pb(value: 3)pb")));
}

TEST_P(RepeatedFieldProxyTest, EraseRange) {
  auto field =
      MakeRepeatedFieldContainer<RepeatedFieldProxyTestSimpleMessage>();
  field->Add()->set_value(1);
  field->Add()->set_value(2);
  field->Add()->set_value(3);
  field->Add()->set_value(4);

  RepeatedFieldProxy<RepeatedFieldProxyTestSimpleMessage> proxy =
      field.MakeProxy();
  auto it = absl::c_find_if(proxy,
                            [](const RepeatedFieldProxyTestSimpleMessage& msg) {
                              return msg.value() == 2;
                            });
  proxy.erase(it, it + 2);

  EXPECT_THAT(proxy, ElementsAre(EqualsProto(R"pb(value: 1)pb"),
                                 EqualsProto(R"pb(value: 4)pb")));
  EXPECT_THAT(*field, ElementsAre(EqualsProto(R"pb(value: 1)pb"),
                                  EqualsProto(R"pb(value: 4)pb")));
}

TEST_P(RepeatedFieldProxyTest, Proto2EraseInt) {
  auto field = MakeRepeatedFieldContainer<int32_t>();
  field->Add(1);
  field->Add(2);
  field->Add(3);
  field->Add(4);

  auto proxy = field.MakeProxy();
  google::protobuf::erase(proxy, 2);
  EXPECT_THAT(proxy, ElementsAre(1, 3, 4));
  EXPECT_THAT(*field, ElementsAre(1, 3, 4));
}

TEST_P(RepeatedFieldProxyTest, Proto2EraseIfInt) {
  auto field = MakeRepeatedFieldContainer<int32_t>();
  field->Add(1);
  field->Add(2);
  field->Add(3);
  field->Add(4);

  auto proxy = field.MakeProxy();
  google::protobuf::erase_if(proxy, [](int32_t x) { return x % 2 == 0; });
  EXPECT_THAT(proxy, ElementsAre(1, 3));
  EXPECT_THAT(*field, ElementsAre(1, 3));
}

TEST_P(RepeatedFieldProxyTest, Proto2EraseString) {
  auto field = MakeRepeatedFieldContainer<std::string>();
  field->Add("1");
  field->Add("2");
  field->Add("3");
  field->Add("4");

  auto proxy = field.MakeProxy();
  google::protobuf::erase(proxy, "3");
  EXPECT_THAT(proxy, ElementsAre("1", "2", "4"));
  EXPECT_THAT(*field, ElementsAre("1", "2", "4"));
}

TEST_P(RepeatedFieldProxyTest, Proto2EraseIfString) {
  auto field = MakeRepeatedFieldContainer<std::string>();
  field->Add("1");
  field->Add("2");
  field->Add("3");
  field->Add("4");

  auto proxy = field.MakeProxy();
  google::protobuf::erase_if(proxy,
                   [](absl::string_view s) { return s == "2" || s == "4"; });
  EXPECT_THAT(proxy, ElementsAre("1", "3"));
  EXPECT_THAT(*field, ElementsAre("1", "3"));
}

TEST_P(RepeatedFieldProxyTest, Proto2EraseIfMessage) {
  auto field =
      MakeRepeatedFieldContainer<RepeatedFieldProxyTestSimpleMessage>();
  field->Add()->set_value(1);
  field->Add()->set_value(2);
  field->Add()->set_value(3);
  field->Add()->set_value(4);

  auto proxy = field.MakeProxy();
  google::protobuf::erase_if(proxy, [](const auto& msg) { return msg.value() % 2 == 0; });
  EXPECT_THAT(proxy, ElementsAre(EqualsProto(R"pb(value: 1)pb"),
                                 EqualsProto(R"pb(value: 3)pb")));
  EXPECT_THAT(*field, ElementsAre(EqualsProto(R"pb(value: 1)pb"),
                                  EqualsProto(R"pb(value: 3)pb")));
}

TEST_P(RepeatedFieldProxyTest, CopyAssignIntegral) {
  auto field1 = MakeRepeatedFieldContainer<int32_t>();
  field1->Add(1);

  auto field2 = MakeRepeatedFieldContainer<int32_t>();
  field2->Add(2);
  field2->Add(3);

  auto proxy1 = field1.MakeProxy();
  auto proxy2 = field2.MakeProxy();
  proxy1.assign(proxy2);

  EXPECT_THAT(proxy1, ElementsAre(2, 3));
  EXPECT_THAT(*field1, ElementsAre(2, 3));

  EXPECT_THAT(proxy2, ElementsAre(2, 3));
  EXPECT_THAT(*field2, ElementsAre(2, 3));
}

TEST_P(RepeatedFieldProxyTest, CopyAssignMessage) {
  auto field1 =
      MakeRepeatedFieldContainer<RepeatedFieldProxyTestSimpleMessage>();
  field1->Add()->set_value(1);

  auto field2 =
      MakeRepeatedFieldContainer<RepeatedFieldProxyTestSimpleMessage>();
  field2->Add()->set_value(2);
  field2->Add()->set_value(3);

  auto proxy1 = field1.MakeProxy();
  auto proxy2 = field2.MakeProxy();
  proxy1.assign(proxy2);

  EXPECT_THAT(proxy1, ElementsAre(EqualsProto(R"pb(value: 2)pb"),
                                  EqualsProto(R"pb(value: 3)pb")));
  EXPECT_THAT(*field1, ElementsAre(EqualsProto(R"pb(value: 2)pb"),
                                   EqualsProto(R"pb(value: 3)pb")));

  EXPECT_THAT(proxy2, ElementsAre(EqualsProto(R"pb(value: 2)pb"),
                                  EqualsProto(R"pb(value: 3)pb")));
  EXPECT_THAT(*field2, ElementsAre(EqualsProto(R"pb(value: 2)pb"),
                                   EqualsProto(R"pb(value: 3)pb")));
}

TEST_P(RepeatedFieldProxyTest, AssignIteratorRange) {
  auto field =
      MakeRepeatedFieldContainer<RepeatedFieldProxyTestSimpleMessage>();
  field->Add()->set_value(1);

  std::vector<RepeatedFieldProxyTestSimpleMessage> msgs(2);
  msgs[0].set_value(2);
  msgs[1].set_value(3);

  RepeatedFieldProxy<RepeatedFieldProxyTestSimpleMessage> proxy =
      field.MakeProxy();
  proxy.assign(msgs.begin(), msgs.end());

  EXPECT_THAT(proxy, ElementsAre(EqualsProto(R"pb(value: 2)pb"),
                                 EqualsProto(R"pb(value: 3)pb")));
  EXPECT_THAT(*field, ElementsAre(EqualsProto(R"pb(value: 2)pb"),
                                  EqualsProto(R"pb(value: 3)pb")));
}

TEST_P(RepeatedFieldProxyTest, AssignInitializerList) {
  auto field =
      MakeRepeatedFieldContainer<RepeatedFieldProxyTestSimpleMessage>();
  field->Add()->set_value(1);

  RepeatedFieldProxy<RepeatedFieldProxyTestSimpleMessage> proxy =
      field.MakeProxy();
  RepeatedFieldProxyTestSimpleMessage msg1;
  msg1.set_value(2);
  RepeatedFieldProxyTestSimpleMessage msg2;
  msg2.set_value(3);
  proxy.assign({msg1, msg2});

  EXPECT_THAT(proxy, ElementsAre(EqualsProto(R"pb(value: 2)pb"),
                                 EqualsProto(R"pb(value: 3)pb")));
  EXPECT_THAT(*field, ElementsAre(EqualsProto(R"pb(value: 2)pb"),
                                  EqualsProto(R"pb(value: 3)pb")));
}

TEST_P(RepeatedFieldProxyTest, MoveAssignIntegral) {
  auto field1 = MakeRepeatedFieldContainer<int32_t>();
  field1->Add(1);

  auto field2 = MakeRepeatedFieldContainer<int32_t>();
  field2->Add(2);
  field2->Add(3);

  auto proxy1 = field1.MakeProxy();
  auto proxy2 = field2.MakeProxy();
  proxy1.move_assign(proxy2);

  EXPECT_THAT(proxy1, ElementsAre(2, 3));
  EXPECT_THAT(*field1, ElementsAre(2, 3));

  EXPECT_THAT(proxy2, IsEmpty());
  EXPECT_THAT(*field2, IsEmpty());
}

TEST_P(RepeatedFieldProxyTest, MoveAssignMessage) {
  auto field1 =
      MakeRepeatedFieldContainer<RepeatedFieldProxyTestSimpleMessage>();
  field1->Add()->set_value(1);

  auto field2 =
      MakeRepeatedFieldContainer<RepeatedFieldProxyTestSimpleMessage>();
  field2->Add()->set_value(2);
  field2->Add()->set_value(3);

  auto proxy1 = field1.MakeProxy();
  auto proxy2 = field2.MakeProxy();
  proxy1.move_assign(proxy2);

  EXPECT_THAT(proxy1, ElementsAre(EqualsProto(R"pb(value: 2)pb"),
                                  EqualsProto(R"pb(value: 3)pb")));
  EXPECT_THAT(*field1, ElementsAre(EqualsProto(R"pb(value: 2)pb"),
                                   EqualsProto(R"pb(value: 3)pb")));

  EXPECT_THAT(proxy2, IsEmpty());
  EXPECT_THAT(*field2, IsEmpty());
}

TEST_P(RepeatedFieldProxyTest, Reserve) {
  auto field = MakeRepeatedFieldContainer<uint32_t>();

  RepeatedFieldProxy<uint32_t> proxy = field.MakeProxy();
  proxy.reserve(10);

  uint64_t space_used_before = 0;
  if (arena() != nullptr) {
    space_used_before = arena()->SpaceUsed();
  }

  for (int i = 0; i < 10; ++i) {
    proxy.emplace_back(i);
  }

  EXPECT_THAT(proxy, ElementsAre(0, 1, 2, 3, 4, 5, 6, 7, 8, 9));
  EXPECT_THAT(*field, ElementsAre(0, 1, 2, 3, 4, 5, 6, 7, 8, 9));

  if (arena() != nullptr) {
    // In the arena case, we verify that no additional memory was allocated
    // after the initial reserve().
    EXPECT_EQ(space_used_before, arena()->SpaceUsed());
  }
}

TEST_P(RepeatedFieldProxyTest, Swap) {
  auto field1 =
      MakeRepeatedFieldContainer<RepeatedFieldProxyTestSimpleMessage>();
  field1->Add()->set_value(1);

  auto field2 =
      MakeRepeatedFieldContainer<RepeatedFieldProxyTestSimpleMessage>();
  field2->Add()->set_value(2);
  field2->Add()->set_value(3);

  RepeatedFieldProxy<RepeatedFieldProxyTestSimpleMessage> proxy1 =
      field1.MakeProxy();
  RepeatedFieldProxy<RepeatedFieldProxyTestSimpleMessage> proxy2 =
      field2.MakeProxy();
  proxy1.swap(proxy2);

  EXPECT_THAT(proxy1, ElementsAre(EqualsProto(R"pb(value: 2)pb"),
                                  EqualsProto(R"pb(value: 3)pb")));
  EXPECT_THAT(proxy2, ElementsAre(EqualsProto(R"pb(value: 1)pb")));
}

TEST_P(RepeatedFieldProxyTest, ResizeInt) {
  auto field = MakeRepeatedFieldContainer<int>();
  field->Add(1);
  field->Add(2);

  RepeatedFieldProxy<int> proxy = field.MakeProxy();
  proxy.resize(4);

  EXPECT_THAT(proxy, ElementsAre(1, 2, 0, 0));
  EXPECT_THAT(*field, ElementsAre(1, 2, 0, 0));

  proxy.resize(1);
  EXPECT_THAT(proxy, ElementsAre(1));
  EXPECT_THAT(*field, ElementsAre(1));
}

TEST_P(RepeatedFieldProxyTest, ResizeIntWithValue) {
  auto field = MakeRepeatedFieldContainer<int>();
  field->Add(1);

  RepeatedFieldProxy<int> proxy = field.MakeProxy();
  proxy.resize(3, 10);

  EXPECT_THAT(proxy, ElementsAre(1, 10, 10));
  EXPECT_THAT(*field, ElementsAre(1, 10, 10));

  proxy.resize(2, 20);
  EXPECT_THAT(proxy, ElementsAre(1, 10));
  EXPECT_THAT(*field, ElementsAre(1, 10));
}

TEST_P(RepeatedFieldProxyTest, ResizeSubMessage) {
  auto field =
      MakeRepeatedFieldContainer<RepeatedFieldProxyTestSimpleMessage>();
  field->Add()->set_value(1);
  field->Add()->set_value(2);

  RepeatedFieldProxy<RepeatedFieldProxyTestSimpleMessage> proxy =
      field.MakeProxy();
  proxy.resize(4);

  EXPECT_THAT(
      proxy, ElementsAre(EqualsProto(R"pb(value: 1)pb"),
                         EqualsProto(R"pb(value: 2)pb"), EqualsProto(R"pb()pb"),
                         EqualsProto(R"pb()pb")));
  EXPECT_THAT(*field,
              ElementsAre(EqualsProto(R"pb(value: 1)pb"),
                          EqualsProto(R"pb(value: 2)pb"),
                          EqualsProto(R"pb()pb"), EqualsProto(R"pb()pb")));

  proxy.resize(1);
  EXPECT_THAT(proxy, ElementsAre(EqualsProto(R"pb(value: 1)pb")));
  EXPECT_THAT(*field, ElementsAre(EqualsProto(R"pb(value: 1)pb")));
}

TEST_P(RepeatedFieldProxyTest, ResizeSubMessageWithValue) {
  auto field =
      MakeRepeatedFieldContainer<RepeatedFieldProxyTestSimpleMessage>();
  field->Add()->set_value(1);

  RepeatedFieldProxy<RepeatedFieldProxyTestSimpleMessage> proxy =
      field.MakeProxy();
  RepeatedFieldProxyTestSimpleMessage msg;
  msg.set_value(10);
  proxy.resize(3, msg);

  EXPECT_THAT(proxy, ElementsAre(EqualsProto(R"pb(value: 1)pb"),
                                 EqualsProto(R"pb(value: 10)pb"),
                                 EqualsProto(R"pb(value: 10)pb")));
  EXPECT_THAT(*field, ElementsAre(EqualsProto(R"pb(value: 1)pb"),
                                  EqualsProto(R"pb(value: 10)pb"),
                                  EqualsProto(R"pb(value: 10)pb")));

  RepeatedFieldProxyTestSimpleMessage msg2;
  msg2.set_value(20);
  proxy.resize(2, msg2);
  EXPECT_THAT(proxy, ElementsAre(EqualsProto(R"pb(value: 1)pb"),
                                 EqualsProto(R"pb(value: 10)pb")));
  EXPECT_THAT(*field, ElementsAre(EqualsProto(R"pb(value: 1)pb"),
                                  EqualsProto(R"pb(value: 10)pb")));
}

TEST_P(RepeatedFieldProxyTest, Rebind) {
  auto field1 = MakeRepeatedFieldContainer<int32_t>();
  field1->Add(1);

  auto field2 = MakeRepeatedFieldContainer<int32_t>();
  field2->Add(2);

  RepeatedFieldProxy<int32_t> proxy = field1.MakeProxy();
  proxy = field2.MakeProxy();

  // Proxy should be rebound to field2 without having modified either field.
  EXPECT_THAT(proxy, ElementsAre(2));
  EXPECT_THAT(*field1, ElementsAre(1));
  EXPECT_THAT(*field2, ElementsAre(2));
}

TEST_P(RepeatedFieldProxyTest, ExplicitConversionToLegacyRepeatedField) {
  auto field = MakeRepeatedFieldContainer<int>();
  field->Add(1);

  RepeatedFieldProxy<int> proxy = field.MakeProxy();
  // Make an explicit conversion to RepeatedField. This will perform a deep
  // copy.
  RepeatedField<int> field2 = RepeatedField<int>(proxy);
  EXPECT_THAT(field2, ElementsAre(1));

  // Verify that field2 is a copy.
  proxy.clear();
  EXPECT_THAT(field2, ElementsAre(1));
}

TEST_P(RepeatedFieldProxyTest, ExplicitConversionToLegacyRepeatedPtrField) {
  auto field =
      MakeRepeatedFieldContainer<RepeatedFieldProxyTestSimpleMessage>();
  field->Add()->set_value(1);

  RepeatedFieldProxy<RepeatedFieldProxyTestSimpleMessage> proxy =
      field.MakeProxy();
  // Make an explicit conversion to RepeatedPtrField. This will perform a deep
  // copy.
  RepeatedPtrField<RepeatedFieldProxyTestSimpleMessage> field2 =
      RepeatedPtrField<RepeatedFieldProxyTestSimpleMessage>(proxy);
  EXPECT_THAT(field2, ElementsAre(EqualsProto(R"pb(value: 1)pb")));

  // Verify that field2 is a copy.
  proxy.clear();
  EXPECT_THAT(field2, ElementsAre(EqualsProto(R"pb(value: 1)pb")));
}

TEST_P(RepeatedFieldProxyTest, SortInt) {
  auto field = MakeRepeatedFieldContainer<int32_t>();
  field->Add(4);
  field->Add(1);
  field->Add(3);
  field->Add(2);

  auto proxy = field.MakeProxy();
  google::protobuf::c_sort(proxy);
  EXPECT_THAT(proxy, ElementsAre(1, 2, 3, 4));
  EXPECT_THAT(*field, ElementsAre(1, 2, 3, 4));

  google::protobuf::c_sort(proxy, std::greater<int32_t>());
  EXPECT_THAT(proxy, ElementsAre(4, 3, 2, 1));
  EXPECT_THAT(*field, ElementsAre(4, 3, 2, 1));
}

TEST_P(RepeatedFieldProxyTest, SortString) {
  auto field = MakeRepeatedFieldContainer<std::string>();
  field->Add("d");
  field->Add("a");
  field->Add("c");
  field->Add("b");

  auto proxy = field.MakeProxy();
  google::protobuf::c_sort(proxy);
  EXPECT_THAT(proxy, ElementsAre("a", "b", "c", "d"));
  EXPECT_THAT(*field, ElementsAre("a", "b", "c", "d"));

  google::protobuf::c_sort(proxy, std::greater<std::string>());
  EXPECT_THAT(proxy, ElementsAre("d", "c", "b", "a"));
  EXPECT_THAT(*field, ElementsAre("d", "c", "b", "a"));
}

TEST_P(RepeatedFieldProxyTest, SortMessage) {
  auto field =
      MakeRepeatedFieldContainer<RepeatedFieldProxyTestSimpleMessage>();
  field->Add()->set_value(4);
  field->Add()->set_value(1);
  field->Add()->set_value(3);
  field->Add()->set_value(2);

  auto proxy = field.MakeProxy();
  google::protobuf::c_sort(proxy, [](const auto& a, const auto& b) {
    return a.value() < b.value();
  });
  EXPECT_THAT(proxy, ElementsAre(EqualsProto(R"pb(value: 1)pb"),
                                 EqualsProto(R"pb(value: 2)pb"),
                                 EqualsProto(R"pb(value: 3)pb"),
                                 EqualsProto(R"pb(value: 4)pb")));
  EXPECT_THAT(*field, ElementsAre(EqualsProto(R"pb(value: 1)pb"),
                                  EqualsProto(R"pb(value: 2)pb"),
                                  EqualsProto(R"pb(value: 3)pb"),
                                  EqualsProto(R"pb(value: 4)pb")));
}

TEST_P(RepeatedFieldProxyTest, StableSortMessage) {
  auto field =
      MakeRepeatedFieldContainer<RepeatedFieldProxyTestSimpleMessage>();
  auto* msg1 = field->Add();
  msg1->set_value(2);
  auto* msg2 = field->Add();
  msg2->set_value(1);
  auto* msg3 = field->Add();
  msg3->set_value(1);
  auto* msg4 = field->Add();
  msg4->set_value(2);

  auto proxy = field.MakeProxy();
  google::protobuf::c_stable_sort(proxy, [](const auto& a, const auto& b) {
    return a.value() < b.value();
  });
  EXPECT_THAT(proxy, ElementsAre(EqualsProto(R"pb(value: 1)pb"),
                                 EqualsProto(R"pb(value: 1)pb"),
                                 EqualsProto(R"pb(value: 2)pb"),
                                 EqualsProto(R"pb(value: 2)pb")));
  // Stable sort should preserve the relative order of elements that compare
  // equal.
  EXPECT_THAT(proxy, ElementsAre(Address(msg2), Address(msg3), Address(msg1),
                                 Address(msg4)));
  EXPECT_THAT(*field, ElementsAre(EqualsProto(R"pb(value: 1)pb"),
                                  EqualsProto(R"pb(value: 1)pb"),
                                  EqualsProto(R"pb(value: 2)pb"),
                                  EqualsProto(R"pb(value: 2)pb")));
}

INSTANTIATE_TEST_SUITE_P(RepeatedFieldProxyTest, RepeatedFieldProxyTest,
                         testing::Bool(),
                         [](const testing::TestParamInfo<bool>& info) {
                           return info.param ? "WithArena" : "WithoutArena";
                         });

}  // namespace
}  // namespace internal
}  // namespace protobuf
}  // namespace google
