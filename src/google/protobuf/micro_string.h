// Protocol Buffers - Google's data interchange format
// Copyright 2008 Google Inc.  All rights reserved.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd

#ifndef GOOGLE_PROTOBUF_MICRO_STRING_H__
#define GOOGLE_PROTOBUF_MICRO_STRING_H__

#include <cstdint>

#include "absl/base/config.h"
#include "absl/log/absl_check.h"
#include "absl/strings/string_view.h"
#include "google/protobuf/arena.h"

// must be last:
#include "google/protobuf/port_def.inc"

namespace google {
namespace protobuf {
namespace internal {

// The MicroString class holds a `char` buffer in one of several
// representations, each with their own properties:
//  - SSO: When enabled, SSO instances store the bytes inlined in the class.
//         They require no memory allocation.
//  - MicroHeap: Cheapest heap representation. It is two `uint8_t` for capacity
//               and size, then the char buffer.
//  - HeapBase: The following representations use HeapBase as the header,
//              differentiating themselves via the `capacity` field.
//    * kOwned: A `char` array follows the base. Similar to MicroHeap, but with
//              2^32 byte limit, instead of 2^8.
//    * kAlias: The base points into an aliased unowned buffer. The base itself
//              is owned. Used for `SetAlias`.
//              An object copy will make its own copy of the data, as alias
//              lifetime is not guaranteed beyond the original message.
//    * kUnowned: Similar to kAlias, but the base is also unowned. Both the base
//                and the payload are guaranteed immutable and immortal. Used
//                for global strings, like non-empty default values.
//                An object copy will maintain the unonwed status and require no
//                memory allocation.
//    * kString: The object holds a HeapString. The base points into the
//               `std::string` instance. Used for `SetString` to allow taking
//               ownership of `std::string` payloads.
//
class PROTOBUF_EXPORT MicroString {
  struct HeapBase {
    char* payload;
    uint32_t size;
    // One of HeapKind, or the capacity for the owned buffer.
    uint32_t capacity;
  };

 public:
#if defined(ABSL_IS_LITTLE_ENDIAN)
  static constexpr bool kHasSSO = sizeof(uintptr_t) >= 8;
#else
  // For now, disable SSO if not in little endian.
  // We can revisit this later if performance in such platforms is relevant.
  static constexpr bool kHasSSO = false;
#endif

  static constexpr size_t kSSOCapacity = kHasSSO ? sizeof(uintptr_t) - 1 : 0;
  static constexpr size_t kMaxMicroHeapCapacity = 255;

  // Empty SSO string.
  constexpr MicroString() : rep_() {}

  // We keep this constructor inline so that the `memcpy` call can be optimized
  // via constant propagation.
  MicroString(Arena* arena, const MicroString& other,
              size_t sso_capacity = kSSOCapacity)
      : rep_{} {
    if (other.is_sso()) {
      memcpy(&rep_, &other.rep_, sso_capacity + 1);
    } else if (other.is_heap() && other.heap_kind() == kUnowned) {
      rep_ = other.rep_;
    } else {
      Set(other.Get(), arena, sso_capacity);
    }
  }

  // Trivial destructor.
  // The payload must be destroyed via `Destroy()` when not in an arena. If
  // using arenas, no destruction is necessary and calls to `Destroy()` are
  // invalid.
  ~MicroString() = default;

  union UnownedPayload {
    HeapBase payload;
    // We use a union to be able to get an unaligned (+1) pointer for the
    // payload in the constexpr contructor. `for_tag + 1` is equivalent to
    // `reinterpret_cast<uintptr_t>(&payload) | kIsHeapTag`
    char for_tag[1];
  };
  explicit constexpr MicroString(const UnownedPayload& unowned)
      : rep_(const_cast<char*>(unowned.for_tag + kIsHeapTag)) {}

  void Destroy() {
    if (!is_sso()) DestroySlow();
  }

  // Sets the payload to `other`. Copy behavior depends on the kind of payload.
  // We keep this function inline so that the `memcpy` call can be optimized
  // via constant propagation.
  void Set(const MicroString& other, Arena* arena,
           size_t sso_capacity = kSSOCapacity) {
    // If SSO, just memcpy directly.
    if (other.is_sso()) {
      if (arena == nullptr) Destroy();
      memcpy(&rep_, &other.rep_, sso_capacity + 1);
      return;
    }
    // Unowned property gets propagated, even if we have a rep already.
    if (other.is_heap() && other.heap_kind() == kUnowned) {
      if (arena == nullptr) Destroy();
      rep_ = other.rep_;
      return;
    }
    Set(other.Get(), arena, sso_capacity);
  }

  // Sets the payload to `data`. Always copies the data.
  void Set(absl::string_view data, Arena* arena,
           size_t sso_capacity = kSSOCapacity);

  // Sets the payload to `data`. Might copy the data or alias the input buffer.
  void SetAlias(absl::string_view data, Arena* arena,
                size_t sso_capacity = kSSOCapacity);

  // Sets the payload to `str`. Might copy the data or take ownership of `str`.
  void SetString(std::string&& data, Arena* arena,
                 size_t sso_capacity = kSSOCapacity);

  // Set the payload to `unowned`. Will not allocate memory, but might free
  // memory if already set.
  void SetUnowned(const UnownedPayload& unowned, Arena* arena);

  // The capacity for write access of this string.
  // It can be 0 if the payload is not writable.
  size_t Capacity() const;

  size_t SpaceUsedExcludingSelfLong() const;

  absl::string_view Get() const {
    if (!kHasSSO && rep_ == nullptr) {
      return absl::string_view();
    } else if (is_micro_heap()) {
      auto* h = micro_heap();
      return absl::string_view(h->data(), h->size);
    } else if (is_sso()) {
      return absl::string_view(sso_head(), sso_size());
    } else {
      auto* h = heap();
      return absl::string_view(h->payload, h->size);
    }
  }

  static constexpr UnownedPayload MakeUnownedPayload(absl::string_view data) {
    return UnownedPayload{HeapBase{const_cast<char*>(data.data()),
                                   static_cast<uint32_t>(data.size()),
                                   kUnowned}};
  }

 protected:
  struct HeapString : HeapBase {
    std::string str;
  };

  static constexpr uintptr_t kIsHeapTag = 0x1;
  static_assert(sizeof(UnownedPayload::for_tag) == kIsHeapTag);
  static constexpr uintptr_t kIsMicroHeapTag = 0x2;
  static_assert((kIsHeapTag & kIsMicroHeapTag) == 0);
  static constexpr int kTagShift = 2;

  enum HeapKind {
    // The buffer is unowned, but the heap payload is owned.
    kAlias,
    // The whole payload is unowned.
    kUnowned,
    // The payload is a HeapString payload.
    kString,
    // An owned HeapBase+chars payload.
    kOwned
  };
  HeapKind heap_kind() const {
    ABSL_DCHECK(is_heap());
    size_t cap = heap()->capacity;
    return cap >= kOwned ? kOwned : static_cast<HeapKind>(cap);
  }

  struct MicroHeap {
    uint8_t size;
    uint8_t capacity;
    char* data() { return reinterpret_cast<char*>(this + 1); }
  };
  // Micro-optimization: by using kIsMicroHeapTag as 2, the MicroHeap `rep_`
  // pointer (with the tag) is already pointing into the data buffer.
  static_assert(sizeof(MicroHeap) == kIsMicroHeapTag);
  MicroHeap* micro_heap() const {
    ABSL_DCHECK(is_micro_heap());
    // NOTE: We use `-` instead of `&` so that the arithmetic gets folded into
    // offsets after the cast.
    return reinterpret_cast<MicroHeap*>(reinterpret_cast<uintptr_t>(rep_) -
                                        kIsMicroHeapTag);
  }
  static size_t MicroHeapSize(size_t capacity) {
    return sizeof(MicroHeap) + capacity;
  }
  static size_t OwnedHeapSize(size_t capacity) {
    return sizeof(HeapBase) + capacity;
  }

  HeapBase* heap() const {
    // NOTE: We use `-` instead of `&` so that the arithmetic gets folded into
    // offsets after the cast.
    return reinterpret_cast<HeapBase*>(reinterpret_cast<uintptr_t>(rep_) -
                                       kIsHeapTag);
  }
  HeapString* heap_string() const {
    ABSL_DCHECK_EQ(+kString, +heap_kind());
    return static_cast<HeapString*>(heap());
  }

  bool is_micro_heap() const {
    return (reinterpret_cast<uintptr_t>(rep_) & kIsMicroHeapTag) ==
           kIsMicroHeapTag;
  }
  bool is_heap() const {
    return (reinterpret_cast<uintptr_t>(rep_) & kIsHeapTag) == kIsHeapTag;
  }
  bool is_sso() const { return kHasSSO && !is_micro_heap() && !is_heap(); }
  size_t sso_size() const {
    ABSL_DCHECK(is_sso());
    return static_cast<uint8_t>(reinterpret_cast<uintptr_t>(rep_)) >> kTagShift;
  }
  void set_sso_size(size_t size) {
    ABSL_DCHECK(kHasSSO);
    rep_ = reinterpret_cast<void*>(size << kTagShift);
    ABSL_DCHECK(is_sso());
  }
  char* sso_head() {
    ABSL_DCHECK(is_sso());
    return reinterpret_cast<char*>(&rep_) + 1;
  }
  const char* sso_head() const {
    ABSL_DCHECK(is_sso());
    return reinterpret_cast<const char*>(&rep_) + 1;
  }

  void DestroySlow();

  void* rep_;
};

template <size_t N>
class MicroStringExtraImpl : private MicroString {
 public:
  static constexpr size_t kSSOCapacity =
      kHasSSO ? N + MicroString::kSSOCapacity : 0;

  constexpr MicroStringExtraImpl() = default;
  MicroStringExtraImpl(Arena* arena, const MicroStringExtraImpl& other)
      : MicroString(arena, other, kSSOCapacity) {}

  using MicroString::Get;
  void Set(const MicroStringExtraImpl& other, Arena* arena) {
    MicroString::Set(other, arena, kSSOCapacity);
  }
  void Set(absl::string_view data, Arena* arena) {
    MicroString::Set(data, arena, kSSOCapacity);
  }
  void SetAlias(absl::string_view data, Arena* arena) {
    MicroString::SetAlias(data, arena, kSSOCapacity);
  }
  void SetString(std::string&& str, Arena* arena) {
    MicroString::SetString(std::move(str), arena, kSSOCapacity);
  }

  using MicroString::Destroy;

  size_t Capacity() const {
    return is_sso() ? kSSOCapacity : MicroString::Capacity();
  }

  using MicroString::SpaceUsedExcludingSelfLong;

 private:
  char extra_buffer_[N];
};

template <size_t N>
using MicroStringExtra =
    std::conditional_t<N == 0 || !MicroString::kHasSSO, MicroString,
                       MicroStringExtraImpl<N>>;

}  // namespace internal
}  // namespace protobuf
}  // namespace google

#endif  // GOOGLE_PROTOBUF_MICRO_STRING_H__

#include "google/protobuf/port_undef.inc"
