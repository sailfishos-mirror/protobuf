
#include <stdint.h>

#include "upb/message/message.h"
#include "upb/mini_table/internal/message.h"
#include "upb/mini_table/message.h"
#include "upb/wire/decode_fast/cardinality.h"
#include "upb/wire/decode_fast/dispatch.h"
#include "upb/wire/decode_fast/field_parsers.h"
#include "upb/wire/internal/decoder.h"

// Must be last.
#include "upb/port/def.inc"

UPB_PRESERVE_NONE const char* _upb_FastDecoder_FallbackToMiniTable(
    struct upb_Decoder* d, const char* ptr, upb_Message* msg, intptr_t table,
    uint64_t hasbits, uint64_t data) {
  (void)data;
  upb_DecodeFast_SetHasbits(msg, hasbits);
  return ptr;
}

UPB_PRESERVE_NONE const char* _upb_FastDecoder_DecodeGeneric(
    struct upb_Decoder* d, const char* ptr, upb_Message* msg, intptr_t table,
    uint64_t hasbits, uint64_t data) {
  const upb_MiniTable* mt = decode_totablep(table);
  if ((mt->UPB_PRIVATE(ext) & kUpb_ExtMode_AllFastFieldsAssigned) &&
      // Tag is XOR'd with expected field prior to dispatch. If tag is 0, this
      // was a known field that hit a handler and needed fallback. If tag is
      // non-zero, this is not a known field, and thanks to AllFieldsAssigned,
      // we know it is safe for fast Extension / Unknown dispatch.
      (uint16_t)data != 0) {
    // Restore the tag to clear XOR'ing.
    data = _upb_FastDecoder_LoadTag(ptr);
    // Fast DecodeUnknown only handles 1- / 2-byte tags, and will loop back to
    // DecodeGeneric otherwise. Check this here to prevent infinite loop.
    if (UPB_LIKELY((data & 0x80) == 0 || (data & 0x8000) == 0)) {
      UPB_MUSTTAIL return _upb_FastDecoder_DecodeUnknown(d, ptr, msg, table,
                                                         hasbits, data);
    }
  }
  upb_DecodeFastNext ret;
  UPB_DECODEFAST_EXIT(kUpb_DecodeFastNext_FallbackToMiniTable, &ret);
  return _upb_FastDecoder_FallbackToMiniTable(d, ptr, msg, table, hasbits,
                                              data);
}
