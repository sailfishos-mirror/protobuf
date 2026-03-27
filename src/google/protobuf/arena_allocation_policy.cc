#include "google/protobuf/arena_allocation_policy.h"

#include <atomic>
#include <cstddef>

// Must be included last.
#include "google/protobuf/port_def.inc"

namespace google {
namespace protobuf {
namespace internal {

PROTOBUF_CONSTINIT std::atomic<size_t> g_experimental_default_max_block_size(
    AllocationPolicy::kCurrentDefaultMaxBlockSize);

void SetExperimentalMaxBlockSize(size_t value) {
  g_experimental_default_max_block_size.store(value, std::memory_order_relaxed);
}

}  // namespace internal
}  // namespace protobuf
}  // namespace google

#include "google/protobuf/port_undef.inc"
