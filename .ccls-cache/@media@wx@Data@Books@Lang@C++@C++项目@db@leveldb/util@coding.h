// Copyright (c) 2011 The LevelDB Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file. See the AUTHORS file for names of contributors.
//
// Endian-neutral encoding:
// * Fixed-length numbers are encoded with least-significant byte first
// * In addition we support variable length "varint" encoding
// * Strings are encoded prefixed by their length in varint format

#ifndef STORAGE_LEVELDB_UTIL_CODING_H_
#define STORAGE_LEVELDB_UTIL_CODING_H_

#include <cstdint>
#include <cstring>
#include <string>

#include "leveldb/slice.h"
#include "port/port.h"

namespace leveldb {

/* 
1.varint是一种紧凑的表示数字的方法，它用一个或多个字节来表示一个数字，值越小的数字使用越少的字节数。
采用Varint，对于很小的int32类型的数字，则可以用1个字节来表示。
大的数字则可能需要5个字节来表示。

2.varint中的每个字节的最高位（bit）有特殊含义，如果该位为1，表示后续的字节也是这个数字的一部分，如果该位为0，则结束。其他的7位（bit）都表示数字。
7位能表示的最大数是127，因此小于128的数字都可以用一个字节表示。
大于等于128的数字，比如说300，会用两个字节在内存中表示为：
原始二进制：100101100
编码后二进制： 10 10101100
低              高
10101100 00000010
*/

//编码
// Standard Put... routines append to a string
void PutFixed32(std::string* dst, uint32_t value);
void PutFixed64(std::string* dst, uint64_t value);
void PutVarint32(std::string* dst, uint32_t value);
void PutVarint64(std::string* dst, uint64_t value);
void PutLengthPrefixedSlice(std::string* dst, const Slice& value); //dst： 长度(编码后) + value

//解码
// Standard Get... routines parse a value from the beginning of a Slice
// and advance the slice past the parsed value.
bool GetVarint32(Slice* input, uint32_t* value);
bool GetVarint64(Slice* input, uint64_t* value);
bool GetLengthPrefixedSlice(Slice* input, Slice* result);

//解码变长整形最基础函数。
// Pointer-based variants of GetVarint...  These either store a value
// in *v and return a pointer just past the parsed value, or return
// nullptr on error.  These routines only look at bytes in the range
// [p..limit-1]
const char* GetVarint32Ptr(const char* p, const char* limit, uint32_t* v);
const char* GetVarint64Ptr(const char* p, const char* limit, uint64_t* v);

//获取编码后长度
// Returns the length of the varint32 or varint64 encoding of "v"
int VarintLength(uint64_t v);

// 编码变长整形的最基础函数。
// Lower-level versions of Put... that write directly into a character buffer
// and return a pointer just past the last byte written.
// REQUIRES: dst has enough space for the value being written
char* EncodeVarint32(char* dst, uint32_t value);
char* EncodeVarint64(char* dst, uint64_t value);

// TODO(costan): Remove port::kLittleEndian and the fast paths based on
//               std::memcpy when clang learns to optimize the generic code, as
//               described in https://bugs.llvm.org/show_bug.cgi?id=41761
//
// The platform-independent code in DecodeFixed{32,64}() gets optimized to mov
// on x86 and ldr on ARM64, by both clang and gcc. However, only gcc optimizes
// the platform-independent code in EncodeFixed{32,64}() to mov / str.

// Lower-level versions of Put... that write directly into a character buffer
// REQUIRES: dst has enough space for the value being written

inline void EncodeFixed32(char* dst, uint32_t value) {
  uint8_t* const buffer = reinterpret_cast<uint8_t*>(dst);

  if (port::kLittleEndian) {
    // Fast path for little-endian CPUs. All major compilers optimize this to a
    // single mov (x86_64) / str (ARM) instruction.
    std::memcpy(buffer, &value, sizeof(uint32_t));
    return;
  }

  // Platform-independent code.
  // Currently, only gcc optimizes this to a single mov / str instruction.
  buffer[0] = static_cast<uint8_t>(value);
  buffer[1] = static_cast<uint8_t>(value >> 8);
  buffer[2] = static_cast<uint8_t>(value >> 16);
  buffer[3] = static_cast<uint8_t>(value >> 24);
}

inline void EncodeFixed64(char* dst, uint64_t value) {
  uint8_t* const buffer = reinterpret_cast<uint8_t*>(dst);

  if (port::kLittleEndian) {
    // Fast path for little-endian CPUs. All major compilers optimize this to a
    // single mov (x86_64) / str (ARM) instruction.
    std::memcpy(buffer, &value, sizeof(uint64_t));
    return;
  }

  // Platform-independent code.
  // Currently, only gcc optimizes this to a single mov / str instruction.
  buffer[0] = static_cast<uint8_t>(value);
  buffer[1] = static_cast<uint8_t>(value >> 8);
  buffer[2] = static_cast<uint8_t>(value >> 16);
  buffer[3] = static_cast<uint8_t>(value >> 24);
  buffer[4] = static_cast<uint8_t>(value >> 32);
  buffer[5] = static_cast<uint8_t>(value >> 40);
  buffer[6] = static_cast<uint8_t>(value >> 48);
  buffer[7] = static_cast<uint8_t>(value >> 56);
}

// Lower-level versions of Get... that read directly from a character buffer
// without any bounds checking.

inline uint32_t DecodeFixed32(const char* ptr) {
  const uint8_t* const buffer = reinterpret_cast<const uint8_t*>(ptr);

  if (port::kLittleEndian) {
    // Fast path for little-endian CPUs. All major compilers optimize this to a
    // single mov (x86_64) / ldr (ARM) instruction.
    uint32_t result;
    std::memcpy(&result, buffer, sizeof(uint32_t));
    return result;
  }

  // Platform-independent code.
  // Clang and gcc optimize this to a single mov / ldr instruction.
  return (static_cast<uint32_t>(buffer[0])) |
         (static_cast<uint32_t>(buffer[1]) << 8) |
         (static_cast<uint32_t>(buffer[2]) << 16) |
         (static_cast<uint32_t>(buffer[3]) << 24);
}

inline uint64_t DecodeFixed64(const char* ptr) {
  const uint8_t* const buffer = reinterpret_cast<const uint8_t*>(ptr);

  if (port::kLittleEndian) {
    // Fast path for little-endian CPUs. All major compilers optimize this to a
    // single mov (x86_64) / ldr (ARM) instruction.
    uint64_t result;
    std::memcpy(&result, buffer, sizeof(uint64_t));
    return result;
  }

  // Platform-independent code.
  // Clang and gcc optimize this to a single mov / ldr instruction.
  return (static_cast<uint64_t>(buffer[0])) |
         (static_cast<uint64_t>(buffer[1]) << 8) |
         (static_cast<uint64_t>(buffer[2]) << 16) |
         (static_cast<uint64_t>(buffer[3]) << 24) |
         (static_cast<uint64_t>(buffer[4]) << 32) |
         (static_cast<uint64_t>(buffer[5]) << 40) |
         (static_cast<uint64_t>(buffer[6]) << 48) |
         (static_cast<uint64_t>(buffer[7]) << 56);
}

// Internal routine for use by fallback path of GetVarint32Ptr
const char* GetVarint32PtrFallback(const char* p, const char* limit,
                                   uint32_t* value);
inline const char* GetVarint32Ptr(const char* p, const char* limit,
                                  uint32_t* value) {
  if (p < limit) {
    uint32_t result = *(reinterpret_cast<const uint8_t*>(p));
    if ((result & 128) == 0) {  //字节最高 bit 为0，说明编码结束。
      *value = result;
      return p + 1;
    }
  }
  return GetVarint32PtrFallback(p, limit, value);
}

}  // namespace leveldb

#endif  // STORAGE_LEVELDB_UTIL_CODING_H_
