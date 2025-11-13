#ifndef GOOGLE_PROTOBUF_EDITIONS_TEST_UTILS_H__
#define GOOGLE_PROTOBUF_EDITIONS_TEST_UTILS_H__

#include "google/protobuf/descriptor.pb.h"

namespace google {
namespace protobuf {
namespace compiler {

const FeatureSetDefaults::FeatureSetEditionDefault* FindEditionDefault(
    const FeatureSetDefaults& defaults, Edition edition);

}  // namespace compiler
}  // namespace protobuf
}  // namespace google

#endif  // GOOGLE_PROTOBUF_EDITIONS_TEST_UTILS_H__
