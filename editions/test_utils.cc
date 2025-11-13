#include "editions/test_utils.h"

#include "google/protobuf/descriptor.pb.h"

namespace google {
namespace protobuf {
namespace compiler {

const FeatureSetDefaults::FeatureSetEditionDefault* FindEditionDefault(
    const FeatureSetDefaults& defaults, Edition edition) {
  for (const auto& edition_default : defaults.defaults()) {
    if (edition_default.edition() == edition) {
      return &edition_default;
    }
  }
  return nullptr;
}

}  // namespace compiler
}  // namespace protobuf
}  // namespace google
