#include "editions/test_utils.h"

#include "google/protobuf/descriptor.pb.h"
#include <gtest/gtest.h>
#include "google/protobuf/test_textproto.h"
#include "google/protobuf/unittest_features.pb.h"

namespace google {
namespace protobuf {
namespace compiler {
namespace {

TEST(TestUtilsTest, FindEditionDefault) {
  FeatureSetDefaults defaults = ParseTextOrDie(R"pb(
    defaults {
      edition: EDITION_PROTO3
      overridable_features {
        [pb.test] { file_feature: VALUE2 }
      }
    }
    defaults {
      edition: EDITION_2023
      overridable_features {
        [pb.test] { file_feature: VALUE3 }
      }
    }
  )pb");

  const auto* edition_defaults = FindEditionDefault(defaults, EDITION_2023);
  ASSERT_NE(edition_defaults, nullptr);
  EXPECT_EQ(edition_defaults->edition(), EDITION_2023);
  EXPECT_EQ(edition_defaults->overridable_features()
                .GetExtension(pb::test)
                .file_feature(),
            pb::EnumFeature::VALUE3);
}

TEST(TestUtilsTest, FindEditionDefaultNull) {
  FeatureSetDefaults defaults = ParseTextOrDie(R"pb(
    defaults {
      edition: EDITION_PROTO3
      overridable_features {
        [pb.test] { file_feature: VALUE2 }
      }
    }
    defaults {
      edition: EDITION_2023
      overridable_features {
        [pb.test] { file_feature: VALUE3 }
      }
    }
  )pb");

  EXPECT_EQ(FindEditionDefault(defaults, EDITION_99999_TEST_ONLY), nullptr);
}

}  // namespace
}  // namespace compiler
}  // namespace protobuf
}  // namespace google
