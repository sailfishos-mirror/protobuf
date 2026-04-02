// Protocol Buffers - Google's data interchange format
// Copyright 2026 Google LLC.  All rights reserved.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd

#include <string>

#include "google/protobuf/descriptor.pb.h"
#include <gmock/gmock.h>
#include <gtest/gtest.h>
#include "absl/status/status.h"
#include "google/protobuf/descriptor.h"
#include "google/protobuf/json/json.h"
#include "google/protobuf/json_enumvalue_options.pb.h"
#include "google/protobuf/json_enumvalue_options_unittest.pb.h"

#ifndef ASSERT_OK
#define ASSERT_OK(x) ASSERT_TRUE(x.ok())
#endif  // ASSERT_OK
#ifndef EXPECT_OK
#define EXPECT_OK(x) EXPECT_TRUE(x.ok())
#endif  // EXPECT_OK

namespace json_enumvalue_options_test {
namespace {

using json_options_unittest::Armor;
using json_options_unittest::Knight;

TEST(JsonEnumvalueOptionsTest, GreatHelmDescriptorAccess) {
  auto desc = google::protobuf::GetEnumDescriptor<Armor>();
  auto great_helm_desc = desc->FindValueByName("ARMOR_GREAT_HELM");
  EXPECT_EQ(great_helm_desc->options().GetExtension(pb::enumvalue::json).name(),
            "gr8 helm");
}

// Gorget does not have json_name set, so it defaults to ARMOR_GORGET.
TEST(JsonEnumvalueOptionsTest, GorgetDefaultSerialization) {
  Knight msg;
  msg.set_armor(Armor::ARMOR_GORGET);
  EXPECT_EQ(msg.armor(), Armor::ARMOR_GORGET);

  std::string json_res{};
  absl::Status status = google::protobuf::json::MessageToJsonString(msg, &json_res);
  EXPECT_OK(status);
  EXPECT_EQ(json_res, R"json({"armor":"ARMOR_GORGET"})json");
}

TEST(JsonEnumvalueOptionsTest, GreatHelmSerialization) {
  Knight msg;
  msg.set_armor(Armor::ARMOR_GREAT_HELM);
  EXPECT_EQ(msg.armor(), Armor::ARMOR_GREAT_HELM);

  std::string json_res{};
  absl::Status status = google::protobuf::json::MessageToJsonString(msg, &json_res);
  EXPECT_OK(status);
  EXPECT_EQ(json_res, R"json({"armor":"gr8 helm"})json");
}

TEST(JsonEnumvalueOptionsTest, GreatHelmIntOverride) {
  Knight msg;
  msg.set_armor(Armor::ARMOR_GREAT_HELM);
  std::string json_res{};
  absl::Status status = google::protobuf::json::MessageToJsonString(
      msg, &json_res,
      google::protobuf::json::PrintOptions{.always_print_enums_as_ints = true});
  EXPECT_OK(status);
  EXPECT_EQ(json_res, R"json({"armor":1})json");
}

}  // namespace
}  // namespace json_enumvalue_options_test
