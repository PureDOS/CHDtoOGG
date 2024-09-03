/*
 * Copyright 2024 https://github.com/PureDOS
 *
 * Based on wasm-rt.h from "The WebAssembly Binary Toolkit"
 * distributed under the following license terms:
 *
 * Copyright 2018 WebAssembly Community Group participants
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#ifndef WASM_RT_H_
#define WASM_RT_H_

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

// Not thread-safe due to WASM_RT's static memory
typedef uint32_t (*fnEncodeVorbisFeedSamples)(float* bufL, float* bufR, uint32_t num, void* user_data);
typedef void (*fnEncodeVorbisOutput)(const void* data, uint32_t len, void* user_data);
extern void WasmEncodeVorbis(int quality, fnEncodeVorbisFeedSamples feed, fnEncodeVorbisOutput outpt, void* user_data);

#ifdef __cplusplus
}
#endif

#ifndef WASM_RT_FROM_INVOKER

/** Maximum stack depth before trapping. */
#ifndef WASM_RT_MAX_CALL_STACK_DEPTH
#define WASM_RT_MAX_CALL_STACK_DEPTH 500
#endif

/** Reason a trap occurred. Provide this to `wasm_rt_trap`. */
typedef enum {
  WASM_RT_TRAP_NONE,         /** No error. */
  WASM_RT_TRAP_OOB,          /** Out-of-bounds access in linear memory. */
  WASM_RT_TRAP_INT_OVERFLOW, /** Integer overflow on divide or truncation. */
  WASM_RT_TRAP_DIV_BY_ZERO,  /** Integer divide by zero. */
  WASM_RT_TRAP_INVALID_CONVERSION, /** Conversion from NaN to integer. */
  WASM_RT_TRAP_UNREACHABLE,        /** Unreachable instruction executed. */
  WASM_RT_TRAP_CALL_INDIRECT,      /** Invalid call_indirect, for any reason. */
  WASM_RT_TRAP_EXHAUSTION,         /** Call stack exhausted. */
} wasm_rt_trap_t;

/** Value types. Used to define function signatures. */
typedef enum {
  WASM_RT_I32,
  WASM_RT_I64,
  WASM_RT_F32,
  WASM_RT_F64,
} wasm_rt_type_t;

/** A function type for all `anyfunc` functions in a Table. All functions are
 * stored in this canonical form, but must be cast to their proper signature to
 * call. */
typedef void (*wasm_rt_funcref_t)(void);

/** A single element of a Table. */
typedef struct {
  /** The index as returned from `wasm_rt_register_func_type`. */
  uint32_t func_type;
  /** The function. The embedder must know the actual C signature of the
   * function and cast to it before calling. */
  wasm_rt_funcref_t func;
} wasm_rt_elem_t;

/** A Memory object. */
typedef struct {
  /** The linear memory data, with a byte length of `size`. */
  uint8_t* data;
  /** The current and maximum page count for this Memory object. If there is no
   * maximum, `max_pages` is 0xffffffffu (i.e. UINT32_MAX). */
  uint32_t pages, max_pages;
  /** The current size of the linear memory, in bytes. */
  uint32_t size;
} wasm_rt_memory_t;

/** A Table object. */
typedef struct {
  /** The table element data, with an element count of `size`. */
  wasm_rt_elem_t* data;
  /** The maximum element count of this Table object. If there is no maximum,
   * `max_size` is 0xffffffffu (i.e. UINT32_MAX). */
  uint32_t max_size;
  /** The current element count of the table. */
  uint32_t size;
} wasm_rt_table_t;

typedef uint8_t u8;
typedef int8_t s8;
typedef uint16_t u16;
typedef int16_t s16;
typedef uint32_t u32;
typedef int32_t s32;
typedef uint64_t u64;
typedef int64_t s64;
typedef float f32;
typedef double f64;

#define WASM_RT_ADD_PREFIX(x) _wasm_##x
extern wasm_rt_memory_t *WASM_RT_ADD_PREFIX(Z_memory);
static void WASM_RT_ADD_PREFIX(init)(void);
static void w2c___wasm_call_ctors(void);
static void w2c_EncodeVorbis(u32);
static fnEncodeVorbisFeedSamples _cur_feed;
static fnEncodeVorbisOutput _cur_outpt;
static void* _cur_user_data;
static uint8_t* w2c_mem_data;
static uint32_t wasm_rt_func_counter;

#ifdef _MSC_VER
#define inline __forceinline
#include <intrin.h>
#define __builtin_popcount __popcnt
static inline int __builtin_ctz(uint32_t x) { unsigned long ret; _BitScanForward(&ret, x); return (int)ret; }
static inline int __builtin_clz(uint32_t x) { unsigned long ret; _BitScanReverse(&ret, x); return (int)(31 ^ ret); }
#ifdef _WIN64
#define __builtin_popcountll __popcnt64
static inline int __builtin_ctzll(uint64_t value) { unsigned long ret; _BitScanForward64(&ret, value); return (int)ret; }
static inline int __builtin_clzll(uint64_t value) { unsigned long ret; _BitScanReverse64(&ret, value); return (int)(63 ^ ret); }
#else /* _WIN64 */
#define __builtin_popcountll(x) (__popcnt((x) & 0xFFFFFFFF) + __popcnt((x) >> 32))
static inline int __builtin_ctzll(uint64_t value) { uint32_t msh = (uint32_t)(value >> 32); uint32_t lsh = (uint32_t)(value & 0xFFFFFFFF); if (lsh != 0) return __builtin_ctz(lsh); return 32 + __builtin_ctz(msh); }
static inline int __builtin_clzll(uint64_t value) { uint32_t msh = (uint32_t)(value >> 32); uint32_t lsh = (uint32_t)(value & 0xFFFFFFFF); if (msh != 0) return __builtin_clz(msh); return 32 + __builtin_clz(lsh); }
#endif /* _WIN64 */
#define __builtin_expect(x, y) x
#define __builtin_expect(x, y) x
#define __builtin_memcpy memcpy
#pragma warning(disable:4305) /* warning C4305: '=' : truncation from 'double' to 'float' */
#else
#pragma GCC diagnostic ignored "-Wpedantic"
#pragma GCC diagnostic ignored "-Wparentheses"
#pragma GCC diagnostic ignored "-Wunused-variable"
#pragma GCC diagnostic ignored "-Wunused-function"
#pragma GCC diagnostic ignored "-Wunused-local-typedefs"
#pragma GCC diagnostic ignored "-Wpragmas"
#pragma GCC diagnostic ignored "-Wunknown-warning-option"
#pragma GCC diagnostic ignored "-Wunused-but-set-variable" // only in debug wasm builds
#pragma GCC diagnostic ignored "-Wstringop-overflow" // dlmalloc out of memory forced crash
#endif

/* Use clib math as is */
#include <math.h>
#include <fenv.h>
#define Z_envZ_sinZ_dd static_cast<f64(*)(f64)>(&sin)
#define Z_envZ_cosZ_dd static_cast<f64(*)(f64)>(&cos)
#define Z_envZ_logZ_dd static_cast<f64(*)(f64)>(&log)
#define Z_envZ_expZ_dd static_cast<f64(*)(f64)>(&exp)
#define Z_envZ_atanZ_dd static_cast<f64(*)(f64)>(&atan)
#define Z_envZ_powZ_ddd static_cast<f64(*)(f64,f64)>(&pow)
#define Z_envZ_sqrtZ_dd static_cast<f64(*)(f64)>(&sqrt)
#define Z_envZ_fabsZ_dd static_cast<f64(*)(f64)>(&fabs)
#define Z_envZ_ldexpZ_ddi static_cast<f64(*)(f64,int)>(&ldexp)

/* TRAP(x) as written in the generated .wasm.cpp file isn't C++ conformant (error in GCC/clang, accepted in MSVC).
 * But the math function usage to directly use sin/cos/etc. isn't C conformant. */
//#define WASM_RT_USE_TRAP

#ifndef WASM_RT_USE_TRAP
/* Add `#ifdef WASM_RT_USE_TRAP` and `#endif` around the generated macros in the .wasm.c file */
#define TRAP(x) DONT_USE
#define UNLIKELY DONT_USE
#define MEMCHECK(a,b,c) DONT_USE
#define FUNC_PROLOGUE
#define FUNC_EPILOGUE
#define UNREACHABLE
#define CALL_INDIRECT(table, t, ft, x, ...) ((t)table.data[x].func)(__VA_ARGS__)
#define DIV_S(ut, min, x, y) ((ut)((x) / (y)))
#define REM_S(ut, min, x, y) ((ut)((x) % (y)))
#define DIVREM_U(op, x, y) ((x) op (y))
#define FMAX(x, y) ((x > y) ? x : y)
#define FMIN(x, y) ((x < y) ? x : y)
#define TRUNC_S(ut, st, ft, min, minop, max, x) ((ut)(st)(x))
#else
/** Current call stack depth. */
static uint32_t wasm_rt_call_stack_depth;
static inline void wasm_rt_trap(wasm_rt_trap_t trp) { *(volatile int*)0 |= 0xbad; }
#endif

/* Using the generated load/store functions can be much slower than direct memory access (depending on compiler/platform). */
//#define WASM_RT_USE_GENLOADSTORE

#ifndef WASM_RT_USE_GENLOADSTORE
#if defined(__i386__) || _M_IX86 || defined(__x86_64__) || _M_AMD64 || defined(__aarch64__) || _M_ARM64 || (defined(__arm__) && (__ARM_ARCH >= 7 || defined(__ARM_ARCH_7A__) || defined(__thumb2__))) || (defined(__mips__) && defined(__MIPSEL__))
#define WASN_RT_UNALIGNED_MEMORY
#endif
#ifdef WASN_RT_UNALIGNED_MEMORY
#define i32_load16_u(mem, addr) (u32)*(u16*)(w2c_mem_data+(addr))
#define i32_load(mem, addr)          *(u32*)(w2c_mem_data+(addr))
#define i64_load(mem, addr)          *(u64*)(w2c_mem_data+(addr))
#define f32_load(mem, addr)          *(f32*)(w2c_mem_data+(addr))
#define f64_load(mem, addr)          *(f64*)(w2c_mem_data+(addr))
#define i32_store(mem, addr, val)    *(u32*)(w2c_mem_data+(addr)) = (val)
#define i64_store(mem, addr, val)    *(u64*)(w2c_mem_data+(addr)) = (val)
#define f32_store(mem, addr, val)    *(f32*)(w2c_mem_data+(addr)) = (val)
#define f64_store(mem, addr, val)    *(f64*)(w2c_mem_data+(addr)) = (val)
#define i32_store16(mem, addr, val)  *(u16*)(w2c_mem_data+(addr)) = (u16)(val)
#else
static inline u32 i32_load(wasm_rt_memory_t* mem, u64 addr)     { u32 val; memcpy(&val, w2c_mem_data+addr, 4); return val; }
static inline u32 i32_load16_u(wasm_rt_memory_t* mem, u64 addr) { u16 val; memcpy(&val, w2c_mem_data+addr, 2); return val; }
static inline u64 i64_load(wasm_rt_memory_t* mem, u64 addr)     { u64 val; memcpy(&val, w2c_mem_data+addr, 8); return val; }
static inline f32 f32_load(wasm_rt_memory_t* mem, u64 addr)     { f32 val; memcpy(&val, w2c_mem_data+addr, 4); return val; }
static inline f64 f64_load(wasm_rt_memory_t* mem, u64 addr)     { f64 val; memcpy(&val, w2c_mem_data+addr, 8); return val; }
#define WASM_RT_STATIC_ASSERT(cond) typedef char _SA[(cond)?1:-1]
#define i32_store(mem, addr, val)    { WASM_RT_STATIC_ASSERT(sizeof(val) == 4); memcpy(w2c_mem_data+(addr), &(val), 4); }
#define i64_store(mem, addr, val)    { WASM_RT_STATIC_ASSERT(sizeof(val) == 8); memcpy(w2c_mem_data+(addr), &(val), 8); }
#define f32_store(mem, addr, val)    { WASM_RT_STATIC_ASSERT(sizeof(val) == 4); memcpy(w2c_mem_data+(addr), &(val), 4); }
#define f64_store(mem, addr, val)    { WASM_RT_STATIC_ASSERT(sizeof(val) == 8); memcpy(w2c_mem_data+(addr), &(val), 8); }
#define i32_store16(mem, addr, val)  { u16 u16val = (u16)val; memcpy(w2c_mem_data+(addr), &u16val, 2); }
#endif
#define i32_load8_u(mem, addr)       w2c_mem_data[addr]
#define i32_load8_s(mem, addr)       (u32)(s32)(s8)w2c_mem_data[addr]
#define i32_store8(mem, addr, val)   w2c_mem_data[addr] = (u8)(val)
#define i64_store8(mem, addr, val)   w2c_mem_data[addr] = (u8)(val)
#define LOAD_DATA(mem, addr, src, len) memcpy(w2c_mem_data + (addr), src, len)
#endif

static inline u32 Z_envZ_memcpyZ_iiii(u32 dest, u32 src, u32 count)
{
	memcpy(w2c_mem_data + dest, w2c_mem_data + src, count);
	return dest;
}
static inline u32 Z_envZ_memmoveZ_iiii(u32 dest, u32 src, u32 count)
{
	memmove(w2c_mem_data + dest, w2c_mem_data + src, count);
	return dest;
}
static inline u32 Z_envZ_memsetZ_iiii(u32 dest, u32 ch, u32 count)
{
	memset(w2c_mem_data + dest, (int)ch, count);
	return dest;
}

static inline void Z_envZ_exitZ_vi(uint32_t code) { *(volatile int*)0 |= 0xbad; }

#include <stdlib.h>
static inline uint32_t Z_envZ_sbrkZ_ii(uint32_t increment)
{
	uint32_t oldPages = WASM_RT_ADD_PREFIX(Z_memory)->pages, oldSize = WASM_RT_ADD_PREFIX(Z_memory)->size, newSize = oldSize + ((increment + 15) & ~15), newPages = (newSize + 65535) / 65536;
	if (newPages > oldPages)
	{
		WASM_RT_ADD_PREFIX(Z_memory)->pages = newPages;
		WASM_RT_ADD_PREFIX(Z_memory)->data = w2c_mem_data = (uint8_t*)realloc(w2c_mem_data, newPages * 65536);
		memset(w2c_mem_data + oldPages * 65536, 0, (newPages - oldPages) * 65536);
	}
	WASM_RT_ADD_PREFIX(Z_memory)->size = newSize;
	return oldSize;
}

static inline void wasm_rt_allocate_memory(wasm_rt_memory_t* mem, uint32_t initial_pages, uint32_t max_pages)
{
	mem->size = initial_pages * 65536;
	if (mem->data == NULL)
	{
		mem->max_pages = max_pages;
		mem->pages = initial_pages + 12; // add extra pages needed by vorbis encoding to avoid reallocation in sbrk
		mem->data = w2c_mem_data = (uint8_t*)malloc(mem->pages * 65536);
	}
	memset(w2c_mem_data, 0, mem->pages * 65536);
}

static inline uint32_t wasm_rt_register_func_type(uint32_t params, uint32_t results, ...)
{
	return wasm_rt_func_counter++;
}

static inline void wasm_rt_allocate_table(wasm_rt_table_t* tbl, uint32_t elements, uint32_t max_elements)
{
	if (tbl->data) return;
	tbl->data = (wasm_rt_elem_t*)malloc(max_elements * sizeof(wasm_rt_elem_t));
	tbl->size = elements;
	tbl->max_size = max_elements;
}

static inline uint32_t Z_envZ_EncodeVorbisFeedSamplesZ_iii(uint32_t ptrBufferArr, uint32_t num)
{
	uint32_t* ptrBuffer = (uint32_t*)(w2c_mem_data + ptrBufferArr);
	float* bufL = (float*)(w2c_mem_data + ptrBuffer[0]);
	float* bufR = (float*)(w2c_mem_data + ptrBuffer[1]);
	return _cur_feed(bufL, bufR, num, _cur_user_data);
}

static inline void Z_envZ_EncodeVorbisOutputZ_vii(uint32_t ptrData, uint32_t len)
{
	_cur_outpt(w2c_mem_data + ptrData, len, _cur_user_data);
}

void WasmEncodeVorbis(int quality, fnEncodeVorbisFeedSamples feed, fnEncodeVorbisOutput outpt, void* user_data)
{
	int olddir = fegetround();
	fesetround(FE_TONEAREST);
	_cur_feed = feed;
	_cur_outpt = outpt;
	_cur_user_data = user_data;
	wasm_rt_func_counter = 0;
	WASM_RT_ADD_PREFIX(init)();
	w2c___wasm_call_ctors();
	w2c_EncodeVorbis((u32)quality);
	fesetround(olddir);
}

#endif /* WASM_RT_FROM_INVOKER */

#endif /* WASM_RT_H_ */
