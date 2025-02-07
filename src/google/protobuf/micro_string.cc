// Protocol Buffers - Google's data interchange format
// Copyright 2008 Google Inc.  All rights reserved.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd

#include "google/protobuf/micro_string.h"

#include <algorithm>
#include <cstdint>
#include <cstring>
#include <utility>

#include "absl/log/absl_check.h"
#include "absl/strings/string_view.h"
#include "google/protobuf/arena.h"
#include "google/protobuf/arena_align.h"
#include "google/protobuf/port.h"

// must be last:
#include "google/protobuf/port_def.inc"

namespace google {
namespace protobuf {
namespace internal {

void MicroString::DestroySlow() {
  if (is_micro_heap()) {
    internal::SizedDelete(micro_heap(), MicroHeapSize(micro_heap()->capacity));
    return;
  }

  switch (heap_kind()) {
    case kOwned:
      internal::SizedDelete(heap(), OwnedHeapSize(heap()->capacity));
      break;
    case kString:
      delete heap_string();
      break;
    case kAlias:
      delete heap();
      break;
    case kUnowned:
      // Nothing to destroy.
      break;
  }
}

void MicroString::Set(absl::string_view data, Arena* arena,
                      size_t sso_capacity) {
  // Reuse space if possible.
  if (is_micro_heap()) {
    auto* h = micro_heap();
    if (h->capacity >= data.size()) {
      h->size = data.size();
      memcpy(h->data(), data.data(), data.size());
      return;
    }
    if (arena == nullptr) {
      DestroySlow();
    }
  } else if (is_heap()) {
    switch (heap_kind()) {
      case kOwned: {
        auto* h = heap();
        if (h->capacity >= data.size()) {
          h->size = data.size();
          memcpy(h->payload, data.data(), data.size());
          return;
        }
        break;
      }
      case kString: {
        auto* h = heap_string();
        if (h->str.capacity() >= data.size()) {
          h->size = data.size();
          h->str.assign(data.data(), data.size());
          h->payload = h->str.data();
          return;
        }
        break;
      }
      case kAlias:
      case kUnowned:
        // No capacity to reuse.
        break;
    }
    if (arena == nullptr) {
      DestroySlow();
    }
  }

  if (kHasSSO && data.size() <= sso_capacity) {
    set_sso_size(data.size());
    memcpy(sso_head(), data.data(), data.size());
    return;
  }

  if (!kHasSSO && data.empty()) {
    rep_ = nullptr;
    return;
  }

  size_t capacity = data.size();
  if (capacity <= kMaxMicroHeapCapacity) {
    MicroHeap* h;
    if (arena == nullptr) {
      const internal::SizedPtr alloc = internal::AllocateAtLeast(
          ArenaAlignDefault::Ceil(MicroHeapSize(capacity)));
      // Maybe we rounded up too much.
      capacity = std::min(kMaxMicroHeapCapacity, alloc.n - sizeof(MicroHeap));
      h = reinterpret_cast<MicroHeap*>(alloc.p);
    } else {
      capacity = ArenaAlignDefault::Ceil(capacity + sizeof(MicroHeap)) -
                 sizeof(MicroHeap);
      capacity = std::min(kMaxMicroHeapCapacity, capacity);
      auto* alloc = Arena::CreateArray<char>(arena, MicroHeapSize(capacity));
      h = reinterpret_cast<MicroHeap*>(alloc);
    }

    rep_ = reinterpret_cast<void*>(reinterpret_cast<uintptr_t>(h) +
                                   kIsMicroHeapTag);
    ABSL_DCHECK(is_micro_heap());

    h->size = data.size();
    h->capacity = capacity;
    memcpy(h->data(), data.data(), data.size());

    return;
  }

  HeapBase* h;
  ABSL_DCHECK_GE(capacity, kOwned);
  if (arena == nullptr) {
    const internal::SizedPtr alloc = internal::AllocateAtLeast(
        ArenaAlignDefault::Ceil(OwnedHeapSize(capacity)));
    capacity = alloc.n - sizeof(HeapBase);
    h = reinterpret_cast<HeapBase*>(alloc.p);
  } else {
    capacity = ArenaAlignDefault::Ceil(capacity);
    auto* alloc = Arena::CreateArray<char>(arena, OwnedHeapSize(capacity));
    h = reinterpret_cast<HeapBase*>(alloc);
  }

  rep_ = reinterpret_cast<void*>(reinterpret_cast<uintptr_t>(h) | kIsHeapTag);
  h->size = data.size();
  h->capacity = capacity;
  h->payload = reinterpret_cast<char*>(h + 1);
  memcpy(h->payload, data.data(), data.size());

  ABSL_DCHECK_EQ(+heap_kind(), +kOwned);
}

void MicroString::SetAlias(absl::string_view data, Arena* arena,
                           size_t sso_capacity) {
  // If we already have an alias, reuse the block.
  if (is_heap() && heap_kind() == kAlias) {
    auto* h = heap();
    h->payload = const_cast<char*>(data.data());
    h->size = data.size();
    return;
  }
  // If we can fit in SSO, avoid allocating memory.
  if (data.size() <= sso_capacity) {
    return Set(data, arena, sso_capacity);
  }

  HeapBase* h;
  if (!is_heap() || heap_kind() != kAlias) {
    if (arena == nullptr) {
      Destroy();
    }

    h = Arena::Create<HeapBase>(arena);
    h->capacity = kAlias;
    rep_ = reinterpret_cast<void*>(reinterpret_cast<uintptr_t>(h) | kIsHeapTag);
  } else {
    h = heap();
  }
  h->payload = const_cast<char*>(data.data());
  h->size = data.size();
  ABSL_DCHECK_EQ(+heap_kind(), +kAlias);
}

void MicroString::SetString(std::string&& data, Arena* arena,
                            size_t sso_capacity) {
  if (data.size() <= std::max(sso_capacity, size_t{32})) {
    // Just copy the data. The overhead of the string is not worth it.
    return Set(data, arena);
  }

  HeapString* h;
  if (!is_heap() || heap_kind() != kString) {
    if (arena == nullptr) Destroy();

    h = Arena::Create<HeapString>(arena);
    h->capacity = kString;
    rep_ = reinterpret_cast<void*>(reinterpret_cast<uintptr_t>(h) | kIsHeapTag);
  } else {
    h = heap_string();
  }

  h->str = std::move(data);
  h->payload = h->str.data();
  h->size = h->str.size();

  ABSL_DCHECK_EQ(+heap_kind(), +kString);
}

void MicroString::SetUnowned(const UnownedPayload& unowned, Arena* arena) {
  if (arena == nullptr) Destroy();
  rep_ = reinterpret_cast<void*>(reinterpret_cast<uintptr_t>(&unowned) |
                                 kIsHeapTag);
  ABSL_DCHECK_EQ(+heap_kind(), +kUnowned);
}

size_t MicroString::Capacity() const {
  if (is_sso()) {
    return kSSOCapacity;
  } else if (is_micro_heap()) {
    return micro_heap()->capacity;
  } else {
    switch (heap_kind()) {
      case kOwned:
        return heap()->capacity;
      case kString:
        return heap_string()->str.capacity();
      case kAlias:
      case kUnowned:
        return 0;
    }
  }
  Unreachable();
}

size_t MicroString::SpaceUsedExcludingSelfLong() const {
  if (is_sso()) return 0;
  if (is_micro_heap()) return MicroHeapSize(micro_heap()->capacity);
  switch (heap_kind()) {
    case kOwned:
      return sizeof(HeapBase) + heap()->capacity;
    case kString:
      return sizeof(HeapString) +
             StringSpaceUsedExcludingSelfLong(heap_string()->str);
    case kAlias:
      return sizeof(HeapBase);
    case kUnowned:
      return 0;
  }
  Unreachable();
}

}  // namespace internal
}  // namespace protobuf
}  // namespace google

#include "google/protobuf/port_undef.inc"
