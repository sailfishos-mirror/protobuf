// Protocol Buffers - Google's data interchange format
// Copyright 2008 Google Inc.  All rights reserved.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd

#include "google/protobuf/micro_string.h"

#include <cstddef>
#include <cstdint>
#include <string>
#include <tuple>
#include <utility>
#include <vector>

#include <gtest/gtest.h>
#include "absl/base/config.h"
#include "absl/log/absl_check.h"
#include "absl/strings/str_cat.h"
#include "absl/strings/str_format.h"
#include "google/protobuf/arena.h"
#include "google/protobuf/arena_align.h"
#include "google/protobuf/arenastring.h"
#include "google/protobuf/port.h"


namespace google {
namespace protobuf {
namespace internal {
namespace {
constexpr size_t kMicroHeapSize = sizeof(uint8_t) * 2;
constexpr size_t kHeapBaseSize = sizeof(char*) + 2 * sizeof(uint32_t);

enum PreviousState { kSSO, kMicroHeap, kOwned, kUnowned, kString, kAlias };

static constexpr auto kUnownedPayload =
    MicroString::MakeUnownedPayload("0123456789");

class MicroStringPrevTest
    : public testing::TestWithParam<std::tuple<bool, PreviousState>> {
 protected:
  MicroStringPrevTest() : str_(MakeFromState(prev_state(), arena())) {}

  ~MicroStringPrevTest() override {
    if (!has_arena()) {
      str_.Destroy();
    }
  }

  static MicroString MakeFromState(PreviousState state, Arena* arena) {
    MicroString str;
    switch (state) {
      case kSSO: {
        std::string input(MicroString::kSSOCapacity, 'x');
        str.Set(input, arena);
        ABSL_CHECK_EQ(str.Get(), input);
        break;
      }
      case kMicroHeap:
        str.Set("Very long string", arena);
        ABSL_CHECK_EQ(str.Get(), "Very long string");
        break;
      case kOwned: {
        std::string very_very_long(MicroString::kMaxMicroHeapCapacity + 1, 'x');
        str.Set(very_very_long, arena);
        ABSL_CHECK_EQ(str.Get(), very_very_long);
        break;
      }
      case kUnowned:
        str.SetUnowned(kUnownedPayload, arena);
        ABSL_CHECK_EQ(str.Get(), "0123456789");
        break;

      case kString:
        str.SetString(std::string("This is a very long string too, which "
                                  "won't use std::string's SSO"),
                      arena);
        ABSL_CHECK_EQ(str.Get(),
                      "This is a very long string too, which "
                      "won't use std::string's SSO");
        break;
      case kAlias:
        str.SetAlias("Another long string, but aliased", arena);
        ABSL_CHECK_EQ(str.Get(), "Another long string, but aliased");
        break;
    }
    return str;
  }

  PreviousState prev_state() { return std::get<PreviousState>(GetParam()); }

  bool has_arena() { return std::get<0>(GetParam()); }
  size_t arena_space_used() { return has_arena() ? arena()->SpaceUsed() : 0; }
  Arena* arena() { return has_arena() ? &arena_ : nullptr; }

  Arena arena_;
  MicroString str_;
};

struct Printer {
  static constexpr absl::string_view kNames[] = {"SSO",     "Micro",  "Owned",
                                                 "Unowned", "String", "Alias"};
  static_assert(kNames[kSSO] == "SSO");
  static_assert(kNames[kMicroHeap] == "Micro");
  static_assert(kNames[kOwned] == "Owned");
  static_assert(kNames[kUnowned] == "Unowned");
  static_assert(kNames[kString] == "String");
  static_assert(kNames[kAlias] == "Alias");
  template <typename T>
  std::string operator()(const T& in) const {
    return absl::StrCat(std::get<0>(in.param) ? "Arena" : "NoArena", "_",
                        kNames[std::get<1>(in.param)]);
  }
};

INSTANTIATE_TEST_SUITE_P(MicroStringTransitionTest, MicroStringPrevTest,
                         testing::Combine(testing::Bool(),
                                          testing::Values(kSSO, kMicroHeap,
                                                          kOwned, kUnowned,
                                                          kString, kAlias)),
                         Printer{});

TEST(MicroStringTest, SSOIsEnabledWhenExpected) {
#if defined(ABSL_IS_LITTLE_ENDIAN)
  constexpr bool kExpectSSO = sizeof(uintptr_t) >= 8;
#else
  constexpr bool kExpectSSO = false;
#endif
  if (kExpectSSO) {
    EXPECT_TRUE(MicroString::kHasSSO);
    EXPECT_EQ(MicroString::kSSOCapacity, sizeof(MicroString) - 1);
  } else {
    EXPECT_FALSE(MicroString::kHasSSO);
    EXPECT_EQ(MicroString::kSSOCapacity, 0);
  }
}

TEST(MicroStringTest, DefaultIsEmpty) {
  MicroString str;
  EXPECT_EQ(str.Get(), "");
}

TEST(MicroStringTest, HasConstexprDefaultConstructor) {
  constexpr MicroString str;
  EXPECT_EQ(str.Get(), "");
}

TEST(MicroStringTest, ConstexprUnownedGlobal) {
  static constexpr auto payload = MicroString::MakeUnownedPayload("0123456789");
  static constexpr MicroString global_instance(payload);

  EXPECT_EQ("0123456789", global_instance.Get());
  EXPECT_EQ(static_cast<const void*>(payload.payload.payload),
            static_cast<const void*>(global_instance.Get().data()));
}

template <typename T>
void TestSSO() {
  Arena arena;
  for (Arena* a : {static_cast<Arena*>(nullptr), &arena}) {
    for (size_t size = 0; size <= T::kSSOCapacity; ++size) {
      const absl::string_view input("ABCDEFGHIJKLMNOPQRSTUVWXYZ", size);
      T str;
      size_t used = arena.SpaceUsed();
      str.Set(input, a);
      EXPECT_EQ(str.Get(), input);
      EXPECT_EQ(used, arena.SpaceUsed());
      EXPECT_EQ(0, str.SpaceUsedExcludingSelfLong());

      // We explicitly don't call Destroy() here. If we allocated heap by
      // mistake it will be detected as a memory leak.
    }
  }
}

TEST(MicroStringTest, SetSSOFromClear) {
  if (!MicroString::kHasSSO) {
    GTEST_SKIP() << "SSO is not active";
  }

  TestSSO<MicroString>();
  TestSSO<MicroStringExtra<8>>();
  TestSSO<MicroStringExtra<16>>();
}

TEST(MicroStringTest, CapacityIsRoundedUp) {
  Arena arena;
  MicroString str;

  str.Set("0123456789", &arena);
  const size_t used = arena.SpaceUsed();
  EXPECT_EQ(str.Capacity(), 16 - kMicroHeapSize);
  str.Set("01234567890123", &arena);
  EXPECT_EQ(used, arena.SpaceUsed());
}

TEST_P(MicroStringPrevTest, SetSSO) {
  if (!MicroString::kHasSSO) {
    GTEST_SKIP() << "SSO is not active";
  }

  const absl::string_view input("ABCD");
  size_t used = arena_space_used();
  size_t self_used = str_.SpaceUsedExcludingSelfLong();
  str_.Set(input, arena());
  EXPECT_EQ(str_.Get(), input);
  // We never use more space than before, regardless of the previous state of
  // the class.
  EXPECT_EQ(used, arena_space_used());
  EXPECT_GE(self_used, str_.SpaceUsedExcludingSelfLong());
}

TEST_P(MicroStringPrevTest, SetMicro) {
  for (size_t size : {str_.kSSOCapacity + 1, size_t{30}}) {
    const absl::string_view input("ABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789", size);
    size_t used = arena_space_used();
    size_t self_used = str_.SpaceUsedExcludingSelfLong();
    const bool will_reuse = str_.Capacity() >= input.size();
    str_.Set(input, arena());
    EXPECT_EQ(str_.Get(), input);

    size_t expected_use =
        will_reuse ? 0 : ArenaAlignDefault::Ceil(kMicroHeapSize + size);
    if (has_arena()) {
      EXPECT_EQ(used + expected_use, arena_space_used());
    }
    EXPECT_EQ(will_reuse ? self_used : expected_use,
              str_.SpaceUsedExcludingSelfLong());
  }
}

TEST_P(MicroStringPrevTest, SetOwned) {
  for (size_t size : {str_.kMaxMicroHeapCapacity + 1, size_t{300}}) {
    const std::string input(size, 'x');
    size_t used = arena_space_used();
    size_t self_used = str_.SpaceUsedExcludingSelfLong();
    const bool will_reuse = str_.Capacity() >= input.size();
    str_.Set(input, arena());
    EXPECT_EQ(str_.Get(), input);

    size_t expected_use =
        will_reuse ? 0 : ArenaAlignDefault::Ceil(kHeapBaseSize + size);
    if (has_arena()) {
      EXPECT_EQ(used + expected_use, arena_space_used());
    }
    if (has_arena()) {
      EXPECT_EQ(will_reuse ? self_used : expected_use,
                str_.SpaceUsedExcludingSelfLong());
    } else {
      // When on heap we don't know how much we round up during allocation.
      EXPECT_LE(will_reuse ? self_used : expected_use,
                str_.SpaceUsedExcludingSelfLong());
    }
  }
}

TEST_P(MicroStringPrevTest, SetAliasSmall) {
  if (!MicroString::kHasSSO) {
    GTEST_SKIP() << "SSO is not active";
  }
  const absl::string_view input("ABC");

  size_t used = arena_space_used();
  size_t self_used = str_.SpaceUsedExcludingSelfLong();
  str_.SetAlias(input, arena());
  auto out = str_.Get();
  if (prev_state() == kAlias) {
    // If we had an alias, we reuse the HeapBase to point to the new alias
    // regardless of size.
    EXPECT_EQ(out.data(), input.data());
  } else {
    // The data will be copied instead, because it is too small to alias.
    EXPECT_NE(out.data(), input.data());
  }
  EXPECT_EQ(out, input);

  if (has_arena()) {
    // We should not need to allocate memory here in any case.
    EXPECT_EQ(used, arena_space_used());
  }
  EXPECT_EQ(self_used, str_.SpaceUsedExcludingSelfLong());
}

TEST_P(MicroStringPrevTest, SetAliasLarge) {
  const absl::string_view input("ABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789");

  size_t used = arena_space_used();
  str_.SetAlias(input, arena());
  auto out = str_.Get();
  // Don't use op==, we want to check it points to the exact same buffer.
  EXPECT_EQ(out.data(), input.data());
  EXPECT_EQ(out.size(), input.size());

  const size_t expected_use = prev_state() == kAlias ? 0 : kHeapBaseSize;
  if (has_arena()) {
    EXPECT_EQ(used + expected_use, arena_space_used());
  }
  EXPECT_EQ(kHeapBaseSize, str_.SpaceUsedExcludingSelfLong());
}

TEST_P(MicroStringPrevTest, SetUnowned) {
  static constexpr auto kUnownedPayload =
      MicroString::MakeUnownedPayload("This one is unowned.");

  size_t used = arena_space_used();
  str_.SetUnowned(kUnownedPayload, arena());
  auto out = str_.Get();
  // Don't use op==, we want to check it points to the exact same buffer.
  EXPECT_EQ(out.data(), kUnownedPayload.payload.payload);
  EXPECT_EQ(out.size(), kUnownedPayload.payload.size);

  // Never uses more memory.
  EXPECT_EQ(used, arena_space_used());
  EXPECT_EQ(0, str_.SpaceUsedExcludingSelfLong());
}

TEST_P(MicroStringPrevTest, SetStringSmall) {
  if (!MicroString::kHasSSO) {
    GTEST_SKIP() << "SSO is not active";
  }

  std::string input(1, 'a');
  size_t used = arena_space_used();
  size_t self_used = str_.SpaceUsedExcludingSelfLong();
  str_.SetString(std::move(input), arena());
  // NOLINTNEXTLINE: We really didn't move from the input, so this is fine.
  EXPECT_EQ(str_.Get(), input);

  // Never uses more space.
  EXPECT_EQ(used, arena_space_used());
  EXPECT_GE(self_used, str_.SpaceUsedExcludingSelfLong());
}

TEST_P(MicroStringPrevTest, SetStringMedium) {
  std::string input(16, 'a');
  size_t used = arena_space_used();
  size_t self_used = str_.SpaceUsedExcludingSelfLong();
  str_.SetString(std::move(input), arena());
  // NOLINTNEXTLINE: We really didn't move from the input, so this is fine.
  EXPECT_EQ(str_.Get(), input);

  const bool will_reuse = !(prev_state() == kSSO || prev_state() == kAlias ||
                            prev_state() == kUnowned);
  const size_t expected_use =
      will_reuse ? 0 : ArenaAlignDefault::Ceil(kMicroHeapSize + input.size());

  // Never uses more space.
  if (has_arena()) {
    EXPECT_EQ(used + expected_use, arena_space_used());
  }
  if (has_arena()) {
    EXPECT_EQ(will_reuse ? self_used : expected_use,
              str_.SpaceUsedExcludingSelfLong());
  } else {
    // When on heap we don't know how much we round up during allocation.
    EXPECT_LE(will_reuse ? self_used : expected_use,
              str_.SpaceUsedExcludingSelfLong());
  }
}

TEST_P(MicroStringPrevTest, SetStringLarge) {
  std::string input(128, 'a');
  std::string copy = input;
  const char* copy_data = copy.data();
  size_t used = arena_space_used();
  str_.SetString(std::move(copy), arena());
  EXPECT_EQ(str_.Get(), input);

  // Verify that the string was moved.
  EXPECT_EQ(copy_data, str_.Get().data());

  const size_t expected_use =
      prev_state() != kString ? kHeapBaseSize + sizeof(std::string) : 0;

  // Never uses more space.
  if (has_arena()) {
    EXPECT_EQ(used + expected_use, arena_space_used());
  }
  EXPECT_EQ(kHeapBaseSize + sizeof(std::string) + input.capacity() + /* \0 */ 1,
            str_.SpaceUsedExcludingSelfLong());
}

TEST_P(MicroStringPrevTest, CopyConstruct) {
  size_t used = arena_space_used();
  MicroString copy(arena(), str_);
  EXPECT_EQ(str_.Get(), copy.Get());

  size_t expected_use;
  switch (prev_state()) {
    case kUnowned:
    case kSSO:
      // These won't use any memory.
      expected_use = 0;
      break;
    case kMicroHeap:
    case kString:
    case kAlias:
      // These all copy as a normal setter
      expected_use =
          ArenaAlignDefault::Ceil(kMicroHeapSize + str_.Get().size());
      break;
    case kOwned:
      expected_use = ArenaAlignDefault::Ceil(kHeapBaseSize + str_.Get().size());
      break;
  }
  if (has_arena()) {
    EXPECT_EQ(used + expected_use, arena_space_used());
  }
  if (has_arena()) {
    EXPECT_EQ(expected_use, copy.SpaceUsedExcludingSelfLong());
  } else {
    // When on heap we don't know how much we round up during allocation.
    EXPECT_LE(expected_use, copy.SpaceUsedExcludingSelfLong());
  }

  if (!has_arena()) copy.Destroy();
}

TEST(MicroStringTest, UnownedIsPropagated) {
  MicroString src(kUnownedPayload);

  ASSERT_EQ(src.Get().data(), kUnownedPayload.payload.payload);

  {
    MicroString str(nullptr, src);
    EXPECT_EQ(str.Get().data(), src.Get().data());
    EXPECT_EQ(0, str.SpaceUsedExcludingSelfLong());
  }
  {
    MicroString str;
    EXPECT_NE(str.Get().data(), src.Get().data());
    str.SetUnowned(kUnownedPayload, nullptr);
    ASSERT_EQ(str.Get().data(), src.Get().data());
    EXPECT_EQ(0, str.SpaceUsedExcludingSelfLong());
  }
}

TEST_P(MicroStringPrevTest, AssignmentViaSet) {
  for (auto state : {kSSO, kMicroHeap, kOwned, kUnowned, kString, kAlias}) {
    MicroString source = MakeFromState(state, arena());

    if (!has_arena()) {
      str_.Destroy();
    }
    str_ = MakeFromState(prev_state(), arena());

    size_t used = arena_space_used();
    SCOPED_TRACE(Printer::kNames[state]);
    SCOPED_TRACE(str_.Get());
    SCOPED_TRACE(source.Get());
    const bool can_fit = str_.Capacity() >= source.Get().size();
    str_.Set(source, arena());
    EXPECT_EQ(str_.Get(), source.Get());
    if (state == prev_state() && state != kAlias) {
      // No new memory should be used if the state is the same. (except kAlias
      // because we need to copy the data and there is no space for it).
      EXPECT_EQ(used, arena_space_used());
    } else if (can_fit) {
      // No new memory should be used if the destination can fit the source.
      EXPECT_EQ(used, arena_space_used());
    } else {
      size_t expected_use;
      switch (state) {
        case kUnowned:
        case kSSO:
          // These won't use any memory.
          expected_use = 0;
          break;
        case kMicroHeap:
        case kString:
        case kAlias:
          // These all copy as a normal setter
          expected_use =
              ArenaAlignDefault::Ceil(kMicroHeapSize + str_.Get().size());
          break;
        case kOwned:
          expected_use =
              ArenaAlignDefault::Ceil(kHeapBaseSize + str_.Get().size());
          break;
      }
      if (has_arena()) {
        EXPECT_EQ(expected_use, arena_space_used() - used);
      }
      if (has_arena()) {
        EXPECT_EQ(expected_use, str_.SpaceUsedExcludingSelfLong());
      } else {
        // When on heap we don't know how much we round up during allocation.
        EXPECT_LE(expected_use, str_.SpaceUsedExcludingSelfLong());
      }
    }

    if (!has_arena()) source.Destroy();
  }
}

TEST(MicroStringExtraTest, SettersWithinSSO) {
  if (!MicroString::kHasSSO || MicroStringExtra<8>::kSSOCapacity != 15) {
    GTEST_SKIP() << "SSO is not active";
  }

  Arena arena;
  size_t used = arena.SpaceUsed();
  size_t expected_use = ArenaAlignDefault::Ceil(kMicroHeapSize + 16);
  MicroStringExtra<8> str15;
  // Setting 15 chars should work fine.
  str15.Set("123456789012345", &arena);
  EXPECT_EQ("123456789012345", str15.Get());
  EXPECT_EQ(used, arena.SpaceUsed());
  EXPECT_EQ(0, str15.SpaceUsedExcludingSelfLong());
  // But 16 should go in the heap.
  str15.Set("1234567890123456", &arena);
  EXPECT_EQ("1234567890123456", str15.Get());
  EXPECT_EQ(used + expected_use, arena.SpaceUsed());
  EXPECT_EQ(expected_use, str15.SpaceUsedExcludingSelfLong());

  used = arena.SpaceUsed();
  expected_use = ArenaAlignDefault::Ceil(kMicroHeapSize + 24);
  // Same but a larger buffer.
  MicroStringExtra<16> str23;
  // Setting 15 chars should work fine.
  str23.Set("12345678901234567890123", &arena);
  EXPECT_EQ("12345678901234567890123", str23.Get());
  EXPECT_EQ(used, arena.SpaceUsed());
  EXPECT_EQ(0, str23.SpaceUsedExcludingSelfLong());
  // But 24 should go in the heap.
  str23.Set("123456789012345678901234", &arena);
  EXPECT_EQ("123456789012345678901234", str23.Get());
  EXPECT_EQ(used + expected_use, arena.SpaceUsed());
  EXPECT_EQ(expected_use, str23.SpaceUsedExcludingSelfLong());
}

TEST(MicroStringExtraTest, CopyConstructWithinSSO) {
  if (!MicroString::kHasSSO) {
    GTEST_SKIP() << "SSO is not active";
  }

  Arena arena;
  const size_t used = arena.SpaceUsed();
  MicroStringExtra<16> sso;
  constexpr absl::string_view kStr10 = "1234567890";
  ABSL_CHECK_GT(kStr10.size(), MicroString::kSSOCapacity);
  ABSL_CHECK_LE(kStr10.size(), MicroStringExtra<16>::kSSOCapacity);
  sso.Set(kStr10, &arena);
  EXPECT_EQ(used, arena.SpaceUsed());
  MicroStringExtra<16> copy(nullptr, sso);
  EXPECT_EQ(kStr10, copy.Get());
  // Should not have used any extra memory.
  EXPECT_EQ(used, arena.SpaceUsed());
  EXPECT_EQ(0, copy.SpaceUsedExcludingSelfLong());
}

// DO NOT SUBMIT: Missing test for Set(MicroStringExtra).

size_t SpaceUsedExcludingSelfLong(const ArenaStringPtr& str) {
  return str.IsDefault() ? 0
                         : sizeof(std::string) +
                               StringSpaceUsedExcludingSelfLong(str.Get());
}

TEST(MicroStringTest, MemoryUsageComparison) {
  Arena arena;
  MicroString micro_str;
  ArenaStringPtr arena_str;
  arena_str.InitDefault();

  const std::string input(200, 'x');

  size_t size_min = 0;
  int64_t micro_str_used = 0;
  int64_t arena_str_used = 0;

  const auto print_range = [&](size_t size_max) {
    int64_t diff = micro_str_used - arena_str_used;
    absl::PrintF(
        "[%3d, %3d] MicroString-ArenaStringPtr=%3d (%s) MicroUsed=%3d "
        "ArenaStringPtrUsed=%3d\n",
        size_min, size_max, diff,
        diff == 0  ? "same "
        : diff < 0 ? "saves"
                   : "regrs",
        micro_str_used, arena_str_used);
  };
  for (size_t i = 1; i < input.size(); ++i) {
    absl::string_view this_input(input.data(), i);
    micro_str.Set(this_input, &arena);
    arena_str.Set(this_input, &arena);

    int64_t this_micro_str_used = micro_str.SpaceUsedExcludingSelfLong();
    int64_t this_arena_str_used = SpaceUsedExcludingSelfLong(arena_str);
    // We expect to always use the same or less memory.
    EXPECT_LE(this_micro_str_used, this_arena_str_used);

    int64_t diff = micro_str_used - arena_str_used;
    int64_t this_diff = this_micro_str_used - this_arena_str_used;

    if (this_diff != diff) {
      print_range(i - 1);
      size_min = i;
      micro_str_used = this_micro_str_used;
      arena_str_used = this_arena_str_used;
    }
  }
  print_range(input.size());
}


}  // namespace
}  // namespace internal
}  // namespace protobuf
}  // namespace google

absl::string_view CodegenMicroStringGet(google::protobuf::internal::MicroString& str) {
  return str.Get();
}
absl::string_view CodegenArenaStringPtrGet(
    google::protobuf::internal::ArenaStringPtr& str) {
  return str.Get();
}
int odr [[maybe_unused]] =
    (google::protobuf::internal::StrongPointer(&CodegenMicroStringGet),
     google::protobuf::internal::StrongPointer(&CodegenArenaStringPtrGet), 0);
