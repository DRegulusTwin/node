// Copyright 2020 the V8 project authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef V8_COMMON_EXTERNAL_POINTER_INL_H_
#define V8_COMMON_EXTERNAL_POINTER_INL_H_

#include "include/v8-internal.h"
#include "src/common/external-pointer.h"
#include "src/execution/isolate.h"

namespace v8 {
namespace internal {

V8_INLINE Address DecodeExternalPointer(PtrComprCageBase isolate_root,
                                        ExternalPointer_t encoded_pointer,
                                        ExternalPointerTag tag) {
  STATIC_ASSERT(kExternalPointerSize == kSystemPointerSize);
#ifdef V8_HEAP_SANDBOX

  // TODO(syg): V8_HEAP_SANDBOX doesn't work with pointer cage
#ifdef V8_COMPRESS_POINTERS_IN_SHARED_CAGE
#error "V8_HEAP_SANDBOX requires per-Isolate pointer compression cage"
#endif

  uint32_t index = static_cast<uint32_t>(encoded_pointer);
  const Isolate* isolate = Isolate::FromRootAddress(isolate_root.address());
  return isolate->external_pointer_table().get(index) ^ tag;
#else
  return encoded_pointer;
#endif
}

V8_INLINE void InitExternalPointerField(Address field_address,
                                        Isolate* isolate) {
#ifdef V8_HEAP_SANDBOX
  static_assert(kExternalPointerSize == kSystemPointerSize,
                "Review the code below, once kExternalPointerSize is 4-byte "
                "the address of the field will always be aligned");
  ExternalPointer_t index = isolate->external_pointer_table().allocate();
  base::WriteUnalignedValue<ExternalPointer_t>(field_address, index);
#else
  // Nothing to do.
#endif  // V8_HEAP_SANDBOX
}

V8_INLINE void InitExternalPointerField(Address field_address, Isolate* isolate,
                                        Address value, ExternalPointerTag tag) {
#ifdef V8_HEAP_SANDBOX
  ExternalPointer_t index = isolate->external_pointer_table().allocate();
  isolate->external_pointer_table().set(static_cast<uint32_t>(index),
                                        value ^ tag);
  static_assert(kExternalPointerSize == kSystemPointerSize,
                "Review the code below, once kExternalPointerSize is 4-byte "
                "the address of the field will always be aligned");
  base::WriteUnalignedValue<ExternalPointer_t>(field_address, index);
#else
  // Pointer compression causes types larger than kTaggedSize to be unaligned.
  constexpr bool v8_pointer_compression_unaligned =
      kExternalPointerSize > kTaggedSize;
  ExternalPointer_t encoded_value = static_cast<ExternalPointer_t>(value);
  if (v8_pointer_compression_unaligned) {
    base::WriteUnalignedValue<ExternalPointer_t>(field_address, encoded_value);
  } else {
    base::Memory<ExternalPointer_t>(field_address) = encoded_value;
  }
#endif  // V8_HEAP_SANDBOX
}

V8_INLINE Address ReadExternalPointerField(Address field_address,
                                           PtrComprCageBase cage_base,
                                           ExternalPointerTag tag) {
  // Pointer compression causes types larger than kTaggedSize to be unaligned.
  constexpr bool v8_pointer_compression_unaligned =
      kExternalPointerSize > kTaggedSize;
  ExternalPointer_t encoded_value;
  if (v8_pointer_compression_unaligned) {
    encoded_value = base::ReadUnalignedValue<ExternalPointer_t>(field_address);
  } else {
    encoded_value = base::Memory<ExternalPointer_t>(field_address);
  }
  return DecodeExternalPointer(cage_base, encoded_value, tag);
}

V8_INLINE void WriteExternalPointerField(Address field_address,
                                         Isolate* isolate, Address value,
                                         ExternalPointerTag tag) {
#ifdef V8_HEAP_SANDBOX
  static_assert(kExternalPointerSize == kSystemPointerSize,
                "Review the code below, once kExternalPointerSize is 4-byte "
                "the address of the field will always be aligned");

  ExternalPointer_t index =
      base::ReadUnalignedValue<ExternalPointer_t>(field_address);
  isolate->external_pointer_table().set(static_cast<uint32_t>(index),
                                        value ^ tag);
#else
  // Pointer compression causes types larger than kTaggedSize to be unaligned.
  constexpr bool v8_pointer_compression_unaligned =
      kExternalPointerSize > kTaggedSize;
  ExternalPointer_t encoded_value = static_cast<ExternalPointer_t>(value);
  if (v8_pointer_compression_unaligned) {
    base::WriteUnalignedValue<ExternalPointer_t>(field_address, encoded_value);
  } else {
    base::Memory<ExternalPointer_t>(field_address) = encoded_value;
  }
#endif  // V8_HEAP_SANDBOX
}

}  // namespace internal
}  // namespace v8

#endif  // V8_COMMON_EXTERNAL_POINTER_INL_H_
