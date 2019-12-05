//
// Created by gaetz on 05/12/2019.
//

#include <cstdint>
#include <climits>

#ifndef TYPES_H
#define TYPES_H

static_assert(sizeof(int8_t)*CHAR_BIT==8, "int8_t is not 8 bit on this architecture, fix the i8 typedef.");
typedef int8_t i8;

static_assert(sizeof(int16_t)*CHAR_BIT==16, "int16_t is not 32 bit on this architecture, fix the i16 typedef.");
typedef int16_t i16;

static_assert(sizeof(int32_t)*CHAR_BIT==32, "int32_t is not 32 bit on this architecture, fix the i32 typedef.");
typedef int32_t i32;

static_assert(sizeof(int64_t)*CHAR_BIT==64, "int64_t is not 64 bit on this architecture, fix the i64 typedef.");
typedef int64_t i64;

static_assert(sizeof(uint8_t)*CHAR_BIT==8, "uint8_t is not 8 bit on this architecture, fix the u8 typedef.");
typedef uint8_t u8;

static_assert(sizeof(uint16_t)*CHAR_BIT==16, "uint16_t is not 16 bit on this architecture, fix the u16 typedef.");
typedef uint16_t u16;

static_assert(sizeof(uint32_t)*CHAR_BIT==32, "uint32_t is not 32 bit on this architecture, fix the u32 typedef.");
typedef uint32_t u32;

static_assert(sizeof(uint64_t)*CHAR_BIT==64, "uint64_t is not 64 bit on this architecture, fix the u64 typedef.");
typedef uint64_t u64;

static_assert(sizeof(float)*CHAR_BIT==32, "float is not 32 bit on this architecture, fix the f32 typedef.");
typedef float f32;

#endif //TYPES_H
