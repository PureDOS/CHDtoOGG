/*
Copyright (c) 2024 https://github.com/PureDOS

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are met:

1. Redistributions of source code must retain the above copyright notice, this
   list of conditions and the following disclaimer.

2. Redistributions in binary form must reproduce the above copyright notice,
   this list of conditions and the following disclaimer in the documentation
   and/or other materials provided with the distribution.

3. Neither the name of the copyright holder nor the names of its
   contributors may be used to endorse or promote products derived from
   this software without specific prior written permission.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE
FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <string>
#include <vector>

#define WASM_RT_FROM_INVOKER
#include "EncodeVorbis.wasm-rt.h"

typedef unsigned char Bit8u;
typedef unsigned short Bit16u;
typedef signed short Bit16s;
typedef unsigned int Bit32u;
typedef signed int Bit32s;
#if defined(_MSC_VER)
typedef unsigned __int64 Bit64u;
#else
typedef unsigned long long Bit64u;
#endif

// Use 64-bit fseek and ftell
#if defined(_MSC_VER) && _MSC_VER >= 1400 // VC2005 and up have a special 64-bit fseek
#define fseek_wrap(fp, offset, whence) _fseeki64(fp, (__int64)offset, whence)
#define ftell_wrap(fp) _ftelli64(fp)
#elif defined(HAVE_64BIT_OFFSETS) || (defined(_POSIX_C_SOURCE) && (_POSIX_C_SOURCE - 0) >= 200112) || (defined(__POSIX_VISIBLE) && __POSIX_VISIBLE >= 200112) || (defined(_POSIX_VERSION) && _POSIX_VERSION >= 200112) || __USE_LARGEFILE || (defined(_FILE_OFFSET_BITS) && _FILE_OFFSET_BITS == 64)
#define fseek_wrap(fp, offset, whence) fseeko(fp, (off_t)offset, whence)
#define ftell_wrap(fp) ftello(fp)
#else
#define fseek_wrap(fp, offset, whence) fseek(fp, (long)offset, whence)
#define ftell_wrap(fp) ftell(fp)
#endif

#define CHD_READ_BE32(p) ((Bit32u)((((const Bit8u *)(p))[0] << 24) | (((const Bit8u *)(p))[1] << 16) | (((const Bit8u *)(p))[2] << 8) | ((const Bit8u *)(p))[3]))
#define CHD_READ_BE64(p) ((Bit64u)((((Bit64u)((const Bit8u *)(p))[0] << 56) | ((Bit64u)((const Bit8u *)(p))[1] << 48) | ((Bit64u)((const Bit8u *)(p))[2] << 40) | ((Bit64u)((const Bit8u *)(p))[3] << 32) | ((Bit64u)((const Bit8u *)(p))[4] << 24) | ((Bit64u)((const Bit8u *)(p))[5] << 16) | ((Bit64u)((const Bit8u *)(p))[6] << 8) | (Bit64u)((const Bit8u *)(p))[7])))

static Bit32u CRC32(const void *data, size_t data_size)
{
	static const Bit32u tbl[256] = { 0,0x77073096,0xEE0E612C,0x990951BA,0x76DC419,0x706AF48F,0xE963A535,0x9E6495A3,0xEDB8832,0x79DCB8A4,0xE0D5E91E,0x97D2D988,0x9B64C2B,0x7EB17CBD,0xE7B82D07,0x90BF1D91,0x1DB71064,0x6AB020F2,0xF3B97148,0x84BE41DE,0x1ADAD47D,0x6DDDE4EB,0xF4D4B551,0x83D385C7,0x136C9856,0x646BA8C0,0xFD62F97A,0x8A65C9EC,0x14015C4F,0x63066CD9,0xFA0F3D63,0x8D080DF5,0x3B6E20C8,0x4C69105E,0xD56041E4,0xA2677172,0x3C03E4D1,0x4B04D447,0xD20D85FD,0xA50AB56B,0x35B5A8FA,0x42B2986C,0xDBBBC9D6,0xACBCF940,0x32D86CE3,0x45DF5C75,0xDCD60DCF,0xABD13D59,0x26D930AC,0x51DE003A,0xC8D75180,0xBFD06116,0x21B4F4B5,0x56B3C423,0xCFBA9599,0xB8BDA50F,0x2802B89E,0x5F058808,0xC60CD9B2,0xB10BE924,0x2F6F7C87,0x58684C11,0xC1611DAB,0xB6662D3D,0x76DC4190,0x1DB7106,0x98D220BC,0xEFD5102A,0x71B18589,0x6B6B51F,0x9FBFE4A5,0xE8B8D433,0x7807C9A2,0xF00F934,0x9609A88E,0xE10E9818,0x7F6A0DBB,0x86D3D2D,0x91646C97,0xE6635C01,0x6B6B51F4,0x1C6C6162,0x856530D8,0xF262004E,0x6C0695ED,0x1B01A57B,0x8208F4C1,0xF50FC457,0x65B0D9C6,0x12B7E950,0x8BBEB8EA,0xFCB9887C,0x62DD1DDF,0x15DA2D49,0x8CD37CF3,0xFBD44C65,0x4DB26158,0x3AB551CE,0xA3BC0074,0xD4BB30E2,0x4ADFA541,0x3DD895D7,0xA4D1C46D,0xD3D6F4FB,0x4369E96A,0x346ED9FC,0xAD678846,0xDA60B8D0,0x44042D73,0x33031DE5,0xAA0A4C5F,0xDD0D7CC9,0x5005713C,0x270241AA,0xBE0B1010,0xC90C2086,0x5768B525,0x206F85B3,0xB966D409,0xCE61E49F,0x5EDEF90E,0x29D9C998,0xB0D09822,0xC7D7A8B4,0x59B33D17,0x2EB40D81,0xB7BD5C3B,0xC0BA6CAD,
		0xEDB88320,0x9ABFB3B6,0x3B6E20C,0x74B1D29A,0xEAD54739,0x9DD277AF,0x4DB2615,0x73DC1683,0xE3630B12,0x94643B84,0xD6D6A3E,0x7A6A5AA8,0xE40ECF0B,0x9309FF9D,0xA00AE27,0x7D079EB1,0xF00F9344,0x8708A3D2,0x1E01F268,0x6906C2FE,0xF762575D,0x806567CB,0x196C3671,0x6E6B06E7,0xFED41B76,0x89D32BE0,0x10DA7A5A,0x67DD4ACC,0xF9B9DF6F,0x8EBEEFF9,0x17B7BE43,0x60B08ED5,0xD6D6A3E8,0xA1D1937E,0x38D8C2C4,0x4FDFF252,0xD1BB67F1,0xA6BC5767,0x3FB506DD,0x48B2364B,0xD80D2BDA,0xAF0A1B4C,0x36034AF6,0x41047A60,0xDF60EFC3,0xA867DF55,0x316E8EEF,0x4669BE79,0xCB61B38C,0xBC66831A,0x256FD2A0,0x5268E236,0xCC0C7795,0xBB0B4703,0x220216B9,0x5505262F,0xC5BA3BBE,0xB2BD0B28,0x2BB45A92,0x5CB36A04,0xC2D7FFA7,0xB5D0CF31,0x2CD99E8B,0x5BDEAE1D,0x9B64C2B0,0xEC63F226,0x756AA39C,0x26D930A,0x9C0906A9,0xEB0E363F,0x72076785,0x5005713,0x95BF4A82,0xE2B87A14,0x7BB12BAE,0xCB61B38,0x92D28E9B,0xE5D5BE0D,0x7CDCEFB7,0xBDBDF21,0x86D3D2D4,0xF1D4E242,0x68DDB3F8,0x1FDA836E,0x81BE16CD,0xF6B9265B,0x6FB077E1,0x18B74777,0x88085AE6,0xFF0F6A70,0x66063BCA,0x11010B5C,0x8F659EFF,0xF862AE69,0x616BFFD3,0x166CCF45,0xA00AE278,0xD70DD2EE,0x4E048354,0x3903B3C2,0xA7672661,0xD06016F7,0x4969474D,0x3E6E77DB,0xAED16A4A,0xD9D65ADC,0x40DF0B66,0x37D83BF0,0xA9BCAE53,0xDEBB9EC5,0x47B2CF7F,0x30B5FFE9,0xBDBDF21C,0xCABAC28A,0x53B39330,0x24B4A3A6,0xBAD03605,0xCDD70693,0x54DE5729,0x23D967BF,0xB3667A2E,0xC4614AB8,0x5D681B02,0x2A6F2B94,0xB40BBE37,0xC30C8EA1,0x5A05DF1B,0x2D02EF8D };
	Bit32u crc = (Bit32u)~(Bit32u)0;
	Bit8u* p = (Bit8u*)data, *pEnd = p + data_size;
	for (; data_size & 3; data_size--) crc = (crc >> 8) ^ tbl[(crc ^ *(p++)) & 0xFF];
	for (; p != pEnd; p+=4)
	{
		crc = (crc >> 8) ^ tbl[(crc ^ p[0]) & 0xFF];
		crc = (crc >> 8) ^ tbl[(crc ^ p[1]) & 0xFF];
		crc = (crc >> 8) ^ tbl[(crc ^ p[2]) & 0xFF];
		crc = (crc >> 8) ^ tbl[(crc ^ p[3]) & 0xFF];
	}
	return ~crc;
}

static void FastMD5(const void* data, size_t data_size, Bit8u res[16])
{
	// BASED ON MD5 (public domain)
	// By Galen Guyer - https://github.com/galenguyer/md5
	struct MD5_CTX
	{
		Bit32u A, B, C, D;
		const void* Body(const void *data, size_t size)
		{
			const Bit8u *ptr = (const Bit8u*)data;
			Bit32u a = A, b = B, c = C, d = D;
			do
			{
				Bit32u saved_a = a, saved_b = b, saved_c = c, saved_d = d;
				#define STEP(f, a, b, c, d, x, t, s) (a) += f((b), (c), (d)) + (x) + (t); (a) = (((a) << (s)) | (((a) & 0xffffffff) >> (32 - (s)))); (a) += (b);
				#if defined(__i386__) || _M_IX86 || defined(__x86_64__) || _M_AMD64 || defined(__vax__)
				#define SET(n) (*(Bit32u *)&ptr[(n) * 4])
				#define GET(n) SET(n)
				#else
				Bit32u block[16];
				#define SET(n) (block[(n)] = (Bit32u)ptr[(n) * 4] | ((Bit32u)ptr[(n) * 4 + 1] << 8) | ((Bit32u)ptr[(n) * 4 + 2] << 16) | ((Bit32u)ptr[(n) * 4 + 3] << 24))
				#define GET(n) (block[(n)])
				#endif
				#define F(x, y, z) ((z) ^ ((x) & ((y) ^ (z))))
				#define G(x, y, z) ((y) ^ ((z) & ((x) ^ (y))))
				#define H(x, y, z) (((x) ^ (y)) ^ (z))
				#define J(x, y, z) ((x) ^ ((y) ^ (z)))
				#define I(x, y, z) ((y) ^ ((x) | ~(z)))
				STEP(F, a, b, c, d, SET( 0), 0xd76aa478,  7) STEP(F, d, a, b, c, SET( 1), 0xe8c7b756, 12) STEP(F, c, d, a, b, SET( 2), 0x242070db, 17) STEP(F, b, c, d, a, SET( 3), 0xc1bdceee, 22)
				STEP(F, a, b, c, d, SET( 4), 0xf57c0faf,  7) STEP(F, d, a, b, c, SET( 5), 0x4787c62a, 12) STEP(F, c, d, a, b, SET( 6), 0xa8304613, 17) STEP(F, b, c, d, a, SET( 7), 0xfd469501, 22)
				STEP(F, a, b, c, d, SET( 8), 0x698098d8,  7) STEP(F, d, a, b, c, SET( 9), 0x8b44f7af, 12) STEP(F, c, d, a, b, SET(10), 0xffff5bb1, 17) STEP(F, b, c, d, a, SET(11), 0x895cd7be, 22)
				STEP(F, a, b, c, d, SET(12), 0x6b901122,  7) STEP(F, d, a, b, c, SET(13), 0xfd987193, 12) STEP(F, c, d, a, b, SET(14), 0xa679438e, 17) STEP(F, b, c, d, a, SET(15), 0x49b40821, 22)
				STEP(G, a, b, c, d, GET( 1), 0xf61e2562,  5) STEP(G, d, a, b, c, GET( 6), 0xc040b340,  9) STEP(G, c, d, a, b, GET(11), 0x265e5a51, 14) STEP(G, b, c, d, a, GET( 0), 0xe9b6c7aa, 20)
				STEP(G, a, b, c, d, GET( 5), 0xd62f105d,  5) STEP(G, d, a, b, c, GET(10), 0x02441453,  9) STEP(G, c, d, a, b, GET(15), 0xd8a1e681, 14) STEP(G, b, c, d, a, GET( 4), 0xe7d3fbc8, 20)
				STEP(G, a, b, c, d, GET( 9), 0x21e1cde6,  5) STEP(G, d, a, b, c, GET(14), 0xc33707d6,  9) STEP(G, c, d, a, b, GET( 3), 0xf4d50d87, 14) STEP(G, b, c, d, a, GET( 8), 0x455a14ed, 20)
				STEP(G, a, b, c, d, GET(13), 0xa9e3e905,  5) STEP(G, d, a, b, c, GET( 2), 0xfcefa3f8,  9) STEP(G, c, d, a, b, GET( 7), 0x676f02d9, 14) STEP(G, b, c, d, a, GET(12), 0x8d2a4c8a, 20)
				STEP(H, a, b, c, d, GET( 5), 0xfffa3942,  4) STEP(J, d, a, b, c, GET( 8), 0x8771f681, 11) STEP(H, c, d, a, b, GET(11), 0x6d9d6122, 16) STEP(J, b, c, d, a, GET(14), 0xfde5380c, 23)
				STEP(H, a, b, c, d, GET( 1), 0xa4beea44,  4) STEP(J, d, a, b, c, GET( 4), 0x4bdecfa9, 11) STEP(H, c, d, a, b, GET( 7), 0xf6bb4b60, 16) STEP(J, b, c, d, a, GET(10), 0xbebfbc70, 23)
				STEP(H, a, b, c, d, GET(13), 0x289b7ec6,  4) STEP(J, d, a, b, c, GET( 0), 0xeaa127fa, 11) STEP(H, c, d, a, b, GET( 3), 0xd4ef3085, 16) STEP(J, b, c, d, a, GET( 6), 0x04881d05, 23)
				STEP(H, a, b, c, d, GET( 9), 0xd9d4d039,  4) STEP(J, d, a, b, c, GET(12), 0xe6db99e5, 11) STEP(H, c, d, a, b, GET(15), 0x1fa27cf8, 16) STEP(J, b, c, d, a, GET( 2), 0xc4ac5665, 23)
				STEP(I, a, b, c, d, GET( 0), 0xf4292244,  6) STEP(I, d, a, b, c, GET( 7), 0x432aff97, 10) STEP(I, c, d, a, b, GET(14), 0xab9423a7, 15) STEP(I, b, c, d, a, GET( 5), 0xfc93a039, 21)
				STEP(I, a, b, c, d, GET(12), 0x655b59c3,  6) STEP(I, d, a, b, c, GET( 3), 0x8f0ccc92, 10) STEP(I, c, d, a, b, GET(10), 0xffeff47d, 15) STEP(I, b, c, d, a, GET( 1), 0x85845dd1, 21)
				STEP(I, a, b, c, d, GET( 8), 0x6fa87e4f,  6) STEP(I, d, a, b, c, GET(15), 0xfe2ce6e0, 10) STEP(I, c, d, a, b, GET( 6), 0xa3014314, 15) STEP(I, b, c, d, a, GET(13), 0x4e0811a1, 21)
				STEP(I, a, b, c, d, GET( 4), 0xf7537e82,  6) STEP(I, d, a, b, c, GET(11), 0xbd3af235, 10) STEP(I, c, d, a, b, GET( 2), 0x2ad7d2bb, 15) STEP(I, b, c, d, a, GET( 9), 0xeb86d391, 21)
				#undef F
				#undef G
				#undef H
				#undef J
				#undef I
				#undef GET
				#undef SET
				#undef STEP
				a += saved_a; b += saved_b; c += saved_c; d += saved_d; ptr += 64;
			} while (size -= 64);
			A = a; B = b; C = c; D = d;
			return ptr;
		}
	} ctx = { 0x67452301, 0xefcdab89, 0x98badcfe, 0x10325476 };
	size_t ctx_lo = (data_size & 0x1fffffff) << 3, ctx_hi = data_size >> 29;
	if (data_size >= 64)
	{
		data = ctx.Body(data, data_size & ~(unsigned long)63);
		data_size &= 63;
	}
	Bit8u ctx_buffer[64];
	memcpy(ctx_buffer, data, data_size);
	ctx_buffer[data_size++] = 0x80;
	size_t available = 64 - data_size;
	if (available < 8)
	{
		memset(&ctx_buffer[data_size], 0, available);
		ctx.Body(ctx_buffer, 64);
		data_size = 0;
		available = 64;
	}
	memset(&ctx_buffer[data_size], 0, available - 8);
	#define OUT(dst, src) (dst)[0] = (Bit8u)(src); (dst)[1] = (Bit8u)((src) >> 8); (dst)[2] = (Bit8u)((src) >> 16); (dst)[3] = (Bit8u)((src) >> 24);
	OUT(&ctx_buffer[56], ctx_lo)
	OUT(&ctx_buffer[60], ctx_hi)
	ctx.Body(ctx_buffer, 64);
	OUT(&res[0], ctx.A)
	OUT(&res[4], ctx.B)
	OUT(&res[8], ctx.C)
	OUT(&res[12], ctx.D)
	#undef OUT
}

static void SHA1(const Bit8u* data, size_t data_size, Bit8u res[20])
{
	// BASED ON SHA-1 in C (public domain)
	// By Steve Reid - https://github.com/clibs/sha1
	struct SHA1_CTX
	{
		static void Transform(Bit32u* state, const Bit8u* buffer)
		{
			Bit32u block[16]; memcpy(block, buffer, 64); // Non destructive (can have input buffer be const)
			//Bit32u* block = buffer; // Destructive (buffer will be modified in place)
			Bit32u a = state[0], b = state[1], c = state[2], d = state[3], e = state[4];
			#define SHA1ROL(value, bits) (((value) << (bits)) | ((value) >> (32 - (bits))))
			#ifdef WORDS_BIGENDIAN
			#define SHA1BLK0(i) block[i]
			#else
			#define SHA1BLK0(i) (block[i] = (SHA1ROL(block[i],24)&0xFF00FF00)|(SHA1ROL(block[i],8)&0x00FF00FF))
			#endif
			#define SHA1BLK(i) (block[i&15] = SHA1ROL(block[(i+13)&15]^block[(i+8)&15]^block[(i+2)&15]^block[i&15],1))
			#define SHA1R0(v,w,x,y,z,i) z+=((w&(x^y))^y)+SHA1BLK0(i)+0x5A827999+SHA1ROL(v,5);w=SHA1ROL(w,30);
			#define SHA1R1(v,w,x,y,z,i) z+=((w&(x^y))^y)+SHA1BLK(i)+0x5A827999+SHA1ROL(v,5);w=SHA1ROL(w,30);
			#define SHA1R2(v,w,x,y,z,i) z+=(w^x^y)+SHA1BLK(i)+0x6ED9EBA1+SHA1ROL(v,5);w=SHA1ROL(w,30);
			#define SHA1R3(v,w,x,y,z,i) z+=(((w|x)&y)|(w&x))+SHA1BLK(i)+0x8F1BBCDC+SHA1ROL(v,5);w=SHA1ROL(w,30);
			#define SHA1R4(v,w,x,y,z,i) z+=(w^x^y)+SHA1BLK(i)+0xCA62C1D6+SHA1ROL(v,5);w=SHA1ROL(w,30);
			SHA1R0(a,b,c,d,e, 0); SHA1R0(e,a,b,c,d, 1); SHA1R0(d,e,a,b,c, 2); SHA1R0(c,d,e,a,b, 3);
			SHA1R0(b,c,d,e,a, 4); SHA1R0(a,b,c,d,e, 5); SHA1R0(e,a,b,c,d, 6); SHA1R0(d,e,a,b,c, 7);
			SHA1R0(c,d,e,a,b, 8); SHA1R0(b,c,d,e,a, 9); SHA1R0(a,b,c,d,e,10); SHA1R0(e,a,b,c,d,11);
			SHA1R0(d,e,a,b,c,12); SHA1R0(c,d,e,a,b,13); SHA1R0(b,c,d,e,a,14); SHA1R0(a,b,c,d,e,15);
			SHA1R1(e,a,b,c,d,16); SHA1R1(d,e,a,b,c,17); SHA1R1(c,d,e,a,b,18); SHA1R1(b,c,d,e,a,19);
			SHA1R2(a,b,c,d,e,20); SHA1R2(e,a,b,c,d,21); SHA1R2(d,e,a,b,c,22); SHA1R2(c,d,e,a,b,23);
			SHA1R2(b,c,d,e,a,24); SHA1R2(a,b,c,d,e,25); SHA1R2(e,a,b,c,d,26); SHA1R2(d,e,a,b,c,27);
			SHA1R2(c,d,e,a,b,28); SHA1R2(b,c,d,e,a,29); SHA1R2(a,b,c,d,e,30); SHA1R2(e,a,b,c,d,31);
			SHA1R2(d,e,a,b,c,32); SHA1R2(c,d,e,a,b,33); SHA1R2(b,c,d,e,a,34); SHA1R2(a,b,c,d,e,35);
			SHA1R2(e,a,b,c,d,36); SHA1R2(d,e,a,b,c,37); SHA1R2(c,d,e,a,b,38); SHA1R2(b,c,d,e,a,39);
			SHA1R3(a,b,c,d,e,40); SHA1R3(e,a,b,c,d,41); SHA1R3(d,e,a,b,c,42); SHA1R3(c,d,e,a,b,43);
			SHA1R3(b,c,d,e,a,44); SHA1R3(a,b,c,d,e,45); SHA1R3(e,a,b,c,d,46); SHA1R3(d,e,a,b,c,47);
			SHA1R3(c,d,e,a,b,48); SHA1R3(b,c,d,e,a,49); SHA1R3(a,b,c,d,e,50); SHA1R3(e,a,b,c,d,51);
			SHA1R3(d,e,a,b,c,52); SHA1R3(c,d,e,a,b,53); SHA1R3(b,c,d,e,a,54); SHA1R3(a,b,c,d,e,55);
			SHA1R3(e,a,b,c,d,56); SHA1R3(d,e,a,b,c,57); SHA1R3(c,d,e,a,b,58); SHA1R3(b,c,d,e,a,59);
			SHA1R4(a,b,c,d,e,60); SHA1R4(e,a,b,c,d,61); SHA1R4(d,e,a,b,c,62); SHA1R4(c,d,e,a,b,63);
			SHA1R4(b,c,d,e,a,64); SHA1R4(a,b,c,d,e,65); SHA1R4(e,a,b,c,d,66); SHA1R4(d,e,a,b,c,67);
			SHA1R4(c,d,e,a,b,68); SHA1R4(b,c,d,e,a,69); SHA1R4(a,b,c,d,e,70); SHA1R4(e,a,b,c,d,71);
			SHA1R4(d,e,a,b,c,72); SHA1R4(c,d,e,a,b,73); SHA1R4(b,c,d,e,a,74); SHA1R4(a,b,c,d,e,75);
			SHA1R4(e,a,b,c,d,76); SHA1R4(d,e,a,b,c,77); SHA1R4(c,d,e,a,b,78); SHA1R4(b,c,d,e,a,79);
			state[0] += a; state[1] += b; state[2] += c; state[3] += d; state[4] += e;
		}
		void Process(const Bit8u* data, size_t len)
		{
			size_t i; Bit32u j = count[0];
			if ((count[0] += (Bit32u)(len << 3)) < j) count[1]++;
			count[1] += (Bit32u)(len>>29);
			j = (j >> 3) & 63;
			if ((j + len) > 63)
			{
				memcpy(&buffer[j], data, (i = 64-j));
				Transform(state, buffer);
				for (; i + 63 < len; i += 64) Transform(state, &data[i]);
				j = 0;
			}
			else i = 0;
			memcpy(&buffer[j], &data[i], len - i);
		}
		Bit32u count[2], state[5];
		Bit8u buffer[64];
	} ctx;
	ctx.count[0] = ctx.count[1] = 0;
	ctx.state[0] = 0x67452301;
	ctx.state[1] = 0xEFCDAB89;
	ctx.state[2] = 0x98BADCFE;
	ctx.state[3] = 0x10325476;
	ctx.state[4] = 0xC3D2E1F0;
	ctx.Process(data, data_size);
	Bit8u finalcount[8];
	for (unsigned i = 0; i < 8; i++)  finalcount[i] = (Bit8u)((ctx.count[(i >= 4 ? 0 : 1)] >> ((3-(i & 3)) * 8) ) & 255);
	Bit8u c = 0200;
	ctx.Process(&c, 1);
	while ((ctx.count[0] & 504) != 448) { c = 0000; ctx.Process(&c, 1); }
	ctx.Process(finalcount, 8);
	for (unsigned j = 0; j < 20; j++) res[j] = (Bit8u)((ctx.state[j>>2] >> ((3-(j & 3)) * 8) ) & 255);
}

int main(int argc, const char** argv)
{
	// Very simple test if the ogg encoding produces the expected bits
	struct TestEncode
	{
		enum { TEST_LEN = 5000, TEST_EXPECT_CRC = 0x79d89c91 };
		float buf[TEST_LEN], *bufp; Bit32u crc;
		static uint32_t FeedSamples(float* bufL, float* bufR, uint32_t num, TestEncode* self)
		{
			uint32_t remain = (uint32_t)(self->buf + TEST_LEN - self->bufp);
			if (remain < num) num = remain;
			memcpy(bufL, self->bufp, num*4); memcpy(bufR, self->bufp, num*4);
			self->bufp += num;
			return num;
		}
		static void OggOutput(const void* data, uint32_t len, TestEncode* self) { self->crc ^= CRC32(data, len); }
	} *testenc = (TestEncode*)malloc(sizeof(TestEncode));

	for (float *bufp = testenc->buf, *bufpend = bufp + TestEncode::TEST_LEN, seed = 0; bufp != bufpend; bufp++)
		*bufp = (seed += (seed > 1 ? -1 : 0.000188019f*(bufpend-bufp)));
	testenc->bufp = testenc->buf;
	testenc->crc = 0;
	WasmEncodeVorbis(5, (fnEncodeVorbisFeedSamples)TestEncode::FeedSamples, (fnEncodeVorbisOutput)TestEncode::OggOutput, testenc);
	Bit32u testrescrc = testenc->crc;
	free(testenc);
	if (testrescrc != TestEncode::TEST_EXPECT_CRC)
	{
		fprintf(stderr, "This system failed to produce the expected encoding results, please report this as a bug at https://github.com/PureDOS/CHDtoOGG\n\n");
		fprintf(stderr, "Expected result: 0x%08x - Test result: 0x%08x\n\n", TestEncode::TEST_EXPECT_CRC, testrescrc);
		return 1;
	}

	// Parse commandline arguments
	const char *inPathCHD = NULL, *outPathCUE = NULL, *qualityStr = NULL, *noData = NULL, *showXML = NULL;
	for (int i = 1; i < argc; i++)
	{
		if ((argv[i][0] != '-' && argv[i][0] != '/') || !argv[i][1] || argv[i][2]) goto argerr;
		switch (argv[i][1])
		{
			case 'i': if (inPathCHD  || ++i == argc) goto argerr; inPathCHD  = argv[i]; continue;
			case 'o': if (outPathCUE || ++i == argc) goto argerr; outPathCUE = argv[i]; continue;
			case 'q': if (qualityStr || ++i == argc) goto argerr; qualityStr = argv[i]; continue;
			case 'n': if (noData ) goto argerr; noData  = argv[i]; continue;
			case 'x': if (showXML) goto argerr; showXML = argv[i]; continue;
		}
		argerr: fprintf(stderr, "Unknown command line option '%s'.\n\n", argv[i]); goto help;
	}
	if (!inPathCHD || !*inPathCHD || !outPathCUE || !*outPathCUE)
	{
		help:
		fprintf(stderr, "%s v%s - Command line options:\n"
			"  -i <PATH>  : Path to input CHD file (required)\n"
			"  -o <PATH>  : Path to output CUE file (required)\n"
			"  -q <LEVEL> : Quality level 0 to 10, defaults to 8\n"
			"  -n         : Output an empty data track\n"
			"  -x         : Print XML DAT meta data\n"
			"\n", "CHDtoOGG", "1.1");
		return 1;
	}
	int qualityRaw = (qualityStr ? atoi(qualityStr) : 8);
	int quality = (qualityRaw < 0 ? 0 : qualityRaw > 10 ? 10 : qualityRaw);

	enum { CHD_V5_HEADER_SIZE = 124, CHD_V5_UNCOMPMAPENTRYBYTES = 4, CD_MAX_SECTOR_DATA = 2352, CD_MAX_SUBCODE_DATA = 96, CD_FRAME_SIZE = CD_MAX_SECTOR_DATA + CD_MAX_SUBCODE_DATA };
	enum { METADATA_HEADER_SIZE = 16, CDROM_TRACK_METADATA_TAG = 1128813650, CDROM_TRACK_METADATA2_TAG = 1128813618, CD_TRACK_PADDING = 4 };

	// Read CHD header and check signature
	Bit32u* chd_hunkmap = NULL;
	Bit8u rawheader[CHD_V5_HEADER_SIZE];
	const char* chd_errstr = NULL;
	FILE* fCHD = fopen(inPathCHD, "rb");
	if (!fCHD || !fread(rawheader, CHD_V5_HEADER_SIZE, 1, fCHD) || memcmp(rawheader, "MComprHD", 8))
	{
		chderr:
		fprintf(stderr, (chd_errstr ? chd_errstr : "Error: Invalid/unsupported CHD file '%s'\n\n"), inPathCHD);
		if (chd_hunkmap) free(chd_hunkmap);
		goto help;
	}

	// Check supported version, flags and compression
	Bit32u hdr_length = CHD_READ_BE32(&rawheader[8]);
	Bit32u hdr_version = CHD_READ_BE32(&rawheader[12]);
	if (hdr_version != 5 || hdr_length != CHD_V5_HEADER_SIZE) goto chderr; // only ver 5 is supported
	if (CHD_READ_BE32(&rawheader[16])) { chd_errstr = "Error: Compressed CHD file '%s' is not supported. CHD file needs to be made with `chdman createcd -c none`."; goto chderr; }

	// Make sure it's a CD image
	Bit32u unitsize = CHD_READ_BE32(&rawheader[60]);
	int chd_hunkbytes = (int)CHD_READ_BE32(&rawheader[56]);
	if (unitsize != CD_FRAME_SIZE || (chd_hunkbytes % CD_FRAME_SIZE) || !chd_hunkbytes) goto chderr; // not CD sector size

	// Read file offsets for hunk mapping and track meta data
	fseek_wrap(fCHD, 0, SEEK_END);
	Bit64u chd_size = (Bit64u)ftell_wrap(fCHD);
	Bit64u logicalbytes = CHD_READ_BE64(&rawheader[32]);
	Bit64u mapoffset = CHD_READ_BE64(&rawheader[40]);
	Bit64u metaoffset = CHD_READ_BE64(&rawheader[48]);
	if (mapoffset < CHD_V5_HEADER_SIZE || mapoffset >= chd_size || metaoffset < CHD_V5_HEADER_SIZE || metaoffset >= chd_size || !logicalbytes) goto chderr;

	// Read hunk mapping and convert to file offsets
	Bit32u hunkcount = (Bit32u)((logicalbytes + chd_hunkbytes - 1) / chd_hunkbytes);
	if (chd_size < mapoffset + hunkcount * CHD_V5_UNCOMPMAPENTRYBYTES) goto chderr;
	chd_hunkmap = (Bit32u*)malloc(hunkcount * CHD_V5_UNCOMPMAPENTRYBYTES);
	fseek_wrap(fCHD, mapoffset, SEEK_SET);
	if (!fread(chd_hunkmap, hunkcount * CHD_V5_UNCOMPMAPENTRYBYTES, 1, fCHD)) goto chderr;
	for (Bit32u j = 0; j != hunkcount; j++)
	{
		chd_hunkmap[j] = CHD_READ_BE32(&chd_hunkmap[j]) * chd_hunkbytes;
		if (chd_size < chd_hunkmap[j] + chd_hunkbytes) goto chderr;
	}

	FILE* fCUE = fopen(outPathCUE, "wb");
	if (!fCUE)
	{
		fprintf(stderr, "Error: Unable to write output CUE file '%s'\n\n", outPathCUE);
		free(chd_hunkmap);
		goto help;
	}

	std::vector< std::vector<char> > cueTracks, xmlTracks;
	std::string pathTrack = outPathCUE;
	const char *cueLastFS = strrchr(outPathCUE, '/'), *cueLastBS = strrchr(outPathCUE, '\\'), *cueLastS = (cueLastFS > cueLastBS ? cueLastFS : cueLastBS);
	size_t pathTrackBaseLen = (pathTrack.size() - 4), pathDirLen = (size_t)((cueLastS ? (cueLastS + 1) : outPathCUE) - outPathCUE);

	// Read track meta data
	Bit32u track_frame = 0;
	for (Bit64u metaentry_offset = metaoffset, metaentry_next; metaentry_offset != 0; metaentry_offset = metaentry_next)
	{
		char mt_type[32], mt_subtype[32];
		if (chd_size < metaentry_offset + METADATA_HEADER_SIZE) goto chderr;
		Bit8u raw_meta_header[METADATA_HEADER_SIZE];
		fseek_wrap(fCHD, metaentry_offset, SEEK_SET);
		if (!fread(raw_meta_header, METADATA_HEADER_SIZE, 1, fCHD)) goto chderr;
		Bit32u metaentry_metatag = CHD_READ_BE32(&raw_meta_header[0]);
		Bit32u metaentry_length = (CHD_READ_BE32(&raw_meta_header[4]) & 0x00ffffff);
		metaentry_next = CHD_READ_BE64(&raw_meta_header[8]);
		if (metaentry_metatag != CDROM_TRACK_METADATA_TAG && metaentry_metatag != CDROM_TRACK_METADATA2_TAG) continue;
		if (chd_size < (size_t)(metaentry_offset + METADATA_HEADER_SIZE) + metaentry_length) goto chderr;

		int mt_track_no = 0, mt_frames = 0, mt_pregap = 0;
		if (fscanf(fCHD,
			(metaentry_metatag == CDROM_TRACK_METADATA2_TAG ? "TRACK:%d TYPE:%30s SUBTYPE:%30s FRAMES:%d PREGAP:%d" : "TRACK:%d TYPE:%30s SUBTYPE:%30s FRAMES:%d"),
			&mt_track_no, mt_type, mt_subtype, &mt_frames, &mt_pregap) < 4) continue;
		if (mt_pregap > mt_frames) { chd_errstr = "Error: Track pregap is larger than total track frame count\n"; goto chderr; }

		// In CHD files tracks are padded to a to a 4-sector boundary.
		track_frame += ((CD_TRACK_PADDING - (track_frame % CD_TRACK_PADDING)) % CD_TRACK_PADDING);

		const bool isAudio = !strcmp(mt_type, "AUDIO");
		pathTrack.resize(pathTrackBaseLen);
		pathTrack.append(" (Track ");
		if (mt_track_no > 99) pathTrack += (char)('0' + (mt_track_no/100)%10);
		if (mt_track_no > 9) pathTrack += (char)('0' + (mt_track_no/10)%10);
		pathTrack += (char)('0' + (mt_track_no%10));
		pathTrack.append(isAudio ? ").ogg" : ").bin");
		FILE* fOut = fopen(pathTrack.c_str(), "wb");
		fprintf(stderr, "%s track %d %s ...\n", (isAudio ? "Compressing" : "Writing"), mt_track_no, pathTrack.c_str());
		if (!fOut) { chd_errstr = "Error: Unable to write track file\n"; goto chderr; }

		// Read track data and calculate hashes (CHD sectorSize is always 2448, data_size is based on chdman source, except MODE2_FORM2 is treated same as MODE2_FORM1 because sector size 2324 is unsupported in BIN/CUE)
		const bool ds2048 = !strcmp(mt_type, "MODE1") || !strcmp(mt_type, "MODE2_FORM1") || !strcmp(mt_type, "MODE2_FORM2");
		const bool ds2336 = !strcmp(mt_type, "MODE2") || !strcmp(mt_type, "MODE2_FORM_MIX");
		const size_t data_size = (ds2048 ? 2048 : ds2336 ? 2336 : CD_MAX_SECTOR_DATA);
		const size_t track_size = (size_t)mt_frames * data_size, pregap_size = (size_t)mt_pregap * data_size;
		Bit8u* track_data = (Bit8u*)malloc(track_size), *track_out = track_data;
		for (Bit32u track_frame_end = track_frame + mt_frames; track_frame != track_frame_end; track_frame++, track_out += data_size)
		{
			size_t p = track_frame * CD_FRAME_SIZE, hunk = (p / chd_hunkbytes), hunk_ofs = (p % chd_hunkbytes), hunk_pos = chd_hunkmap[hunk];
			if (!hunk_pos) { memset(track_out, 0, data_size); continue; }
			fseek_wrap(fCHD, hunk_pos + hunk_ofs, SEEK_SET);
			if (!fread(track_out, data_size, 1, fCHD)) { free(track_data); chd_errstr = "Error: Failed to read from source file '%s'\n"; goto chderr; }
		}

		if (cueTracks.size() < (size_t)mt_track_no) { cueTracks.resize((size_t)mt_track_no); xmlTracks.resize((size_t)mt_track_no); }
		std::vector<char> &cueTrack = cueTracks[mt_track_no-1], &xmlTrack = xmlTracks[mt_track_no-1];
		
		struct Encode
		{
			size_t wavpcmlen, wavpcmpos, romcap, romlen;
			Bit8u *wavpcm, *rombuf;

			static uint32_t FeedSamples(float* bufL, float* bufR, uint32_t num, Encode* self)
			{
				uint32_t remain = (uint32_t)((self->wavpcmlen - self->wavpcmpos) / 4);
				if (remain < num) num = remain;
				signed char* pcm = (signed char*)(self->wavpcm + self->wavpcmpos);
				for (uint32_t i = 0; i != num; i++, pcm += 4)
				{
					bufL[i] = ((pcm[1] << 8) | (0x00ff & (int)pcm[0])) / 32768.f;
					bufR[i] = ((pcm[3] << 8) | (0x00ff & (int)pcm[2])) / 32768.f;
				}
				if (!self->wavpcmpos && self->wavpcmlen >= 1024*1024) { fprintf(stderr, "  Progress: 0%%"); fflush(stderr); }
				self->wavpcmpos += num * 4;
				if ((self->wavpcmpos / (1024*1024)) != ((self->wavpcmpos - (num * 4)) / (1024*1024))) { fprintf(stderr, " .. %u%%", (uint32_t)(((uint64_t)self->wavpcmpos * 100 + 50) / self->wavpcmlen)); fflush(stderr); }
				if (self->wavpcmpos == self->wavpcmlen && self->wavpcmlen >= 1024*1024 && num) fprintf(stderr, "\n");
				return num;
			}
			static void OggOutput(const void* data, uint32_t len, Encode* self)
			{
				while (self->romlen + len > self->romcap) self->rombuf = (Bit8u*)realloc(self->rombuf, (self->romcap += 1024*1024));
				memcpy(self->rombuf + self->romlen, data, len);
				self->romlen += len;
			}
		} enc = {0};

		//Function to load data into out with 56448 bytes allocated (stored compressed in 2919 bytes)
		extern void GetEmptyDataTrackBin(Bit8u*);
		static Bit8u emptyDataTrackBin[24 * CD_MAX_SECTOR_DATA];

		Bit32u in_zeros = 0, out_zeros = 0;
		if (isAudio)
		{
			// CHD audio endian swap
			for (Bit8u *p = track_data, *pEnd = p + track_size, tmp; p != pEnd; p += 2)
				{ tmp = p[0]; p[0] = p[1]; p[1] = tmp; }
			// Additional info for audio tracks
			for (; in_zeros != track_size && track_data[in_zeros] == 0; in_zeros++) {}
			if (in_zeros != track_size) for (; out_zeros != track_size && track_data[track_size - 1 - out_zeros] == 0; out_zeros++) {}
			if (pregap_size > in_zeros) { fprintf(stderr, "  Warning: Pregap for track %d contains audio data which will get omitted in exported OGG\n", mt_track_no); fflush(stderr); }

			enc.wavpcm = track_data + pregap_size;
			enc.wavpcmlen = track_size - pregap_size;
			WasmEncodeVorbis(quality, (fnEncodeVorbisFeedSamples)Encode::FeedSamples, (fnEncodeVorbisOutput)Encode::OggOutput, &enc);
		}
		else if (noData)
		{
			if (!emptyDataTrackBin[1]) GetEmptyDataTrackBin(emptyDataTrackBin);
			enc.rombuf = emptyDataTrackBin;
			enc.romlen = sizeof(emptyDataTrackBin);
		}
		else
		{
			enc.rombuf = track_data;
			enc.romlen = track_size;
		}
		fwrite(enc.rombuf, enc.romlen, 1, fOut);
		fclose(fOut);

		cueTrack.resize(160 + (pathTrack.size() - pathDirLen));
		char *pcue = &cueTrack[0], binTrackType[16];
		sprintf(binTrackType, (isAudio ? "AUDIO" : "MODE%c/%04d"), (noData ? '1' : mt_type[4]), (noData ? 2352 : (int)data_size)); //noData is MODE1/2352
		pcue += sprintf(pcue, "FILE \"%s\" %s\r\n", (pathTrack.c_str() + pathDirLen), (isAudio ? "MP3" : "BINARY"));
		pcue += sprintf(pcue, "  TRACK %02d %s\r\n", mt_track_no, binTrackType);
		if (!mt_pregap || (noData && !isAudio))
		{
			// Data or audio track without pregap
			pcue += sprintf(pcue, "    INDEX 01 00:00:00\r\n");
		}
		else if (isAudio)
		{
			// We exclude the pregap data from the OGG encode and use the PREGAP tag to indicate that it has been omitted.
			// Alternative would be to include the pregap data and use a pair of INDEX 00 and INDEX 01 tags but it is not well supported by existing emulators.
			pcue += sprintf(pcue, "    PREGAP %02d:%02d:%02d\r\n", (mt_pregap/(60*75))%60, (mt_pregap/75)%60, mt_pregap%75);
			pcue += sprintf(pcue, "    INDEX 01 00:00:00\r\n");
		}
		else
		{
			// Data track with pregap use a pair of INDEX 00 and INDEX 01 tags
			pcue += sprintf(pcue, "    INDEX 00 00:00:00\r\n");
			pcue += sprintf(pcue, "    INDEX 01 %02d:%02d:%02d\r\n", (mt_pregap/(60*75))%60, (mt_pregap/75)%60, mt_pregap%75);
		}

		if (showXML)
		{
			fprintf(stderr, "  Calculating checksum...\n");
			Bit32u romcrc32 = CRC32(enc.rombuf, enc.romlen);
			Bit8u rommd5[16], romsha1[20];
			FastMD5(enc.rombuf, enc.romlen, rommd5);
			SHA1(enc.rombuf, enc.romlen, romsha1);

			xmlTrack.resize(540 + (pathTrack.size() - pathDirLen));
			char* pxml = &xmlTrack[0];
			pxml += sprintf(pxml, "\t\t<rom name=\"%s\" size=\"%u\" crc=\"%08x\" md5=\"", (pathTrack.c_str() + pathDirLen), (unsigned)enc.romlen, romcrc32);
			for (int rommd5i = 0; rommd5i != 16; rommd5i++) pxml += sprintf(pxml, "%02x", rommd5[rommd5i]);
			pxml += sprintf(pxml, "\" sha1=\"");
			for (int romsha1i = 0; romsha1i != 20; romsha1i++) pxml += sprintf(pxml, "%02x", romsha1[romsha1i]);
			pxml += sprintf(pxml, (isAudio ? "\">\n" : "\"/>\n"));

			Bit32u srccrc32; Bit8u srcmd5[16], srcsha1[20];
			if (track_data != enc.rombuf) { srccrc32 = CRC32(track_data, (size_t)track_size); FastMD5(track_data, (size_t)track_size, srcmd5); SHA1(track_data, (size_t)track_size, srcsha1); }
			else { srccrc32 = romcrc32; memcpy(srcmd5, rommd5, sizeof(srcmd5)); memcpy(srcsha1, romsha1, sizeof(srcsha1)); }

			pxml += sprintf(pxml, "\t\t\t<source frames=\"%d\" pregap=\"%d\" duration=\"%02d:%02d:%02d\" size=\"%u\" crc=\"%08x\" md5=\"", mt_frames, mt_pregap, ((mt_frames/75/60)%100), (mt_frames/75)%60, mt_frames%75, (Bit32u)track_size, srccrc32);
			for (int srcmd5i = 0; srcmd5i != 16; srcmd5i++) pxml += sprintf(pxml, "%02x", srcmd5[srcmd5i]);
			pxml += sprintf(pxml, "\" sha1=\"");
			for (int srcsha1i = 0; srcsha1i != 20; srcsha1i++) pxml += sprintf(pxml, "%02x", srcsha1[srcsha1i]);
			if (isAudio) pxml += sprintf(pxml, "\" in_zeros=\"%u\" out_zeros=\"%u\" trimmed_crc=\"%08x\" quality=\"%d\"", in_zeros, out_zeros, CRC32(track_data + in_zeros, (size_t)(track_size - in_zeros - out_zeros)), quality);
			if (isAudio && pregap_size > in_zeros) pxml += sprintf(pxml, " non_silence_pregap=\"1\"");
			pxml += sprintf(pxml, "/>\n\t\t</rom>\n", in_zeros, out_zeros, CRC32(track_data + in_zeros, (size_t)(track_size - in_zeros - out_zeros)), quality);
		}
		if (enc.rombuf != track_data && enc.rombuf != emptyDataTrackBin) free(enc.rombuf);
		free(track_data);
		fprintf(stderr, "  Finished processing track %d!\n", mt_track_no);
	}
	free(chd_hunkmap);
	chd_hunkmap = NULL;

	if (showXML)
	{
		fprintf(stderr, "\nPrinting XML elements to standard output ...\n---------------------------------------------------------------------------\n");
		for (size_t itrk = 0; itrk != cueTracks.size(); itrk++)
			if (xmlTracks[itrk].size()) printf("%s", &xmlTracks[itrk][0]);
		fprintf(stderr, "---------------------------------------------------------------------------\nDone!\n");
	}

	fprintf(stderr, "\nFinished processing all tracks, writing CUE file %s ...\n", outPathCUE);
	for (size_t itrk = 0; itrk != cueTracks.size(); itrk++)
	{
		if (!cueTracks[itrk].size()) { fprintf(stderr, "Error: CHD misses track %u (but has track %u)\n\n", (unsigned)(itrk + 1), (unsigned)(itrk + 2)); goto chderr; }
		fwrite(&cueTracks[itrk][0], strlen(&cueTracks[itrk][0]), 1, fCUE);
	}
	fprintf(stderr, "Done!\n");
	return 0;
}

//Function to load data into out with 56448 bytes allocated (stored compressed in 2919 bytes)
void GetEmptyDataTrackBin(Bit8u* out)
{
	static const unsigned char emptyDataBinComp[] = "\337\0\377p\0\0\0\2\0\1\200\0\0\0\377\1\21\377\2\"\377\3\63\377\4D\377\5U\377\6f\377{\7wv\305\23h+x\3\367\0\275\365\b\17\64R5\270}xY\365\336\0\364\be4\227&\320Vx\257\277A\b\271\2-\27.\33\261H\337\262D\370\325e\0\302\0\346\337\0C\b\355\2E<Su3\357+%b\371\t\220\0\301\0\240\22\311/\1\t/\377\n?\377\vP\377\fa\377\rr\377\36\16\203\377\17\224\377\17\377xTv\370N\211/\275\365\t/5\374\232\25\322\211/\364{\t/5\250\354\355\234\t/\f\f\34\375\340u\31\313\365\242\371.e\357\274\302\331\t/\5\364R\241\321\367\b\315]N\371.\220H\301\240\330\331/\2\t/\377\17\377\377\17\377\377\17\377\377\17\377\377\36\16\203\377\17\224\377\17\377x\347\330H\340\211/\275\367\t/54u\330=\211/\365{\t/5\323\255\220\335\t/\f\236>\375C\371\70\254\345k\357\377\312\367ee\302\257\t/\5\326\336|\373\34\234\4\66\306\357\377=\220\350\220\301\255\331/\3\t/\377\17\377\377\17\377\377\7\17\377\377\17\377\377\16\203\377\17\224\377\17\377xv\275\330\257\205\211/\2\t/5\232\332u\222^\211/\1\t/5\354g\255\27\t/\f\377\277\65\215\227\220/\242\215}\357\377\257e\331\302v\t/\5g\376\260\216\270\247\342N\352\357\377\372\255\220\330\301u\331/\4\t/\377\1\17\377\377\17\377\377\17\377\377\17\377\377\16\203\377\17\224\377\17\377x\202\353\205*\r\211/\363\t/5\233\222\267~\210\331\365\367\t/5\31\27T\277\32\t/\f(&\367\205\334`\337\3\303\357\377\211e\312\302C\177\t/\5\2+\v\27\23\350\32\276\324\357\377z\220=\301G\331/\200\5\t/\377\17\377\377\17\377\377\17\377\377\17\377\377\16\203\377\17\224\377z\17\377x\23\340\272h\211/\6\t/5\365\65=\323\270\211/\3\t/5&\357\335i\320\t/\f\t-9\353\367t\343D%\357\377\354ev\337\302\232\t/\5\263E\371\263(\357\16b\370\357\377\352\220u\301\240\237\331/\6\t/\377\17\377\377\17\377\377\17\377\377\17\377\377\36\16\203\377\17\224\377\17\377x\240N\n\306\211/\275\4\t/5\375\322\36W\211/\2{\t/5]\234\24\221\t/\f\233\17\375\232gU\204T\354\357\377C\367e\257\302\354\t/\5\221\311$\373~\274\307\tp\357\377G\220\350\255\301\352\331/\7\t/\377\17\377\377\17\377\377\7\17\377\377\17\377\377\16\203\377\17\224\377\17\377x1+\232\257"
		"\243\211/\361\t/5S}\263\370^\211/\366\t/5bV)[\t/\f\377\272\4T\t\375\7\23\n}\357\377&e\23\302\65\t/\5 \376\247\326\332\207!q\\\357\377\372\327\220\345\301\62\331/\b\t/\377\1\17\377\377\17\377\377\17\377\377\17\377\377\16\203\377\17\224\377\17\377xK\353?\355g\211/\373\t/5\335A\327*\251\211/\363\t/5\226~\307\277\316\t/\f\30u\300\330\25#\337\315\216\357\377\17e\211\302\206\177\t/\5\365\22\241N\17\254[\276\360\357\377\364\220z\301\216\331/\200\t\t/\377\17\377\377\17\377\377\17\377\377\17\377\377\16\203\377\17\224\377z\17\377x\332Z}\2\211/\16\t/5\365s\356\207\6\211/\7\t/5\251\357\264\372\4\t/\f9~\16\266\367\275\240\212h\357\377je5\337\302_\t/\5D|S\352\64\357J#\334\357\377d\220\62\301\240V\331/\20\t/\377\17\377\377\17\377\377\17\377\377\17\377\377\36\16\203\377\17\224\377\17\377x\331Jb\262\211/\275\353\t/5v\336\246\313\211/\373{\t/5\257\224\304y\t/\f\266\354\375\3\276 a\225\63\357\377\36\367e\17\302\21\t/\5\324^|\373?\222\306:\247\357\377\365\220\350\364\301\1\331/\21\t/\377\17\377\377\17\377\377\7\17\377\377\17\377\377\16\203\377\17\224\377\17\377xH/\362\257\327\211/\36\t/5\330q\vd^\211/\17\t/5\220^\371\263\t/\f\377\227\347\315\320\210\342\322\325}\357\377{e\263\302\310\t/\5e\376\60\216\233\251 B\213\360\27\364\220\274\301\331\331/\22\t/\377\17\377\377\3\17\377\377\17\377\377\17\377\377\16\203\377\17\224\377\17\377x\373\201\327By\211/\34\t/5\20\236\306o\210\177\365\16\t/5\353\37\204\362\177\t/\f\5\305n\\\251\205\302\276\34\357\377\324ej\302\276\t/\5\377G\274SV=\351)\3}\357\377\310\220d\301\254\331/\23\0\t/\377\17\377\377\17\377\377\17\377\377\17\377\377\16\203\377\17\224\377\17\377x\355j\344\322\210]\367\351\t/5\276\353\61k$\211/\372\t/5\324\325\337\271\70\t/\f$\316\240\62\1\357\6\205\372\357\377\261e\326\302\277g\t/\5\366\322\241\362\6\17\337Q/\357\377X\220,\301t@\331/\24\t/\377\17\377\377\17\377\377\17\377\377\17\377\377\16\203\377=\17\224\377\17\377x\236\334 \224\211/\30z\t/5\277y`\241\211/\f\t/5\367!\245@5\t/\f\263\335\332\373 MI$\264\357\377\227e\357\305\302R\t/\5\223I$]\367\262\5\5\21\357\377\217\220"
		"\311\320\301F\331/\25\t/\377\17\377\377\17\377\377\17\377\377\17\17\377\377\16\203\377\17\224\377\17\377x\17\271\260\361^\211/\355\t/5\21\326\315\16\211/\275\370\t/5\36o}\377\t/\f\222\376\326\24N\345\312cR\357\377\373\362ey\302\213\t/\5\"'\375\326\371\211\343}=\357\377\37\367\220\201\301\236\331/\26\1\1\377CD001\1\0 \"\0\0-o\377\30\260\6\17\377\b\31\250\1 \3\373\0\b\b\0\nO\377\n\23[\217\377\25\60\311\0\27O\377\27\0\277\b/\377\b\0|\b\34\4\364+\r$\2`? \0\0\377\1\21\377T\2\":0\320\0$\0\20\17\0\360\20\f\241\377\17\17\377\377\16\302\377\17\323\377\17\377\71\204\201\25\67\177o\377\32\355\365\357\36\227\242\377\201\302\71\65\256\304\252\321\337\321b\20\2b\321\62\62x\327ex\220\0\2\20\2\2\231\357\377\343\3\30\335\231)\357\3\377\3\242\324\242\354\222\224\231\377\357\231\3\357*\231*\357\276\177 \20qq\350}$ \0\377\254\64\323\n\377\363\376\7\377\334\212\231\376\34\17\263\320\367\230\341\341S\20\2S\341\"\365\"XDX\220\0#\20\2#\377\253\337\320\67 \355\253\33\377\337\67\67\221\345\221\326\261\377\260\253\337\253\67\337\22\253\357\22\337^ \20\347\300\356Z\277\64 \0\253<\313\323\276\35\377\27\331\347\230;\311\37\3\377{d<\222t{T}\377\346o\343>r\365q\204\377\1\370\237T\327\360\254:\377A\275\213\345\60\245gu\377\fU\347c\371\335\254\220\377\315\71\331\323\200)\fm\377Z\220\272{\1>\b\221\377Em\246a\205OL#\377O\231\203An\213\34'\377g\251\340\376\5\335\370v\377\63k\5\1\312\326\226\311\373\345\270\17\317\322\311/\27\1\200\377Y/\17\377\377\17\377\377\17\377\377\17\377\377\17\377\377\v\32\377=\f+\377\r<o\243\221\270\225}\301\367\377\32\365\252\211x  \325\275\365\r\324-\370\256\325\242~\27\365\377\r\364U\312<\20\20\344\273\364\16*-[?my\333\0A\177\16w\2w\342\62\226\16\63K\277Gn\223\272\0\2\303\37q\377\306L\216-\313t$\346\337\0C\16\253\2\317wD\245\233\357\252\20xn\307O\0\326\26\377v\265`\344\205\"94\334\63\22\311/\30\1 \16\356\377\17\377\377\3\17\377\377\17\377\377\17\377\377\16~\377\17\217\377\17\377v\222\314\337\224=\211/\20\365`\201\221\375\215\227\205b\201\313\17\377+\253\337I\241x\263\0\365\b\364\60\377\316\306\310\305\314\61"
		"\316\353{\17\377+9\205\65z\t/\f\335B\375V?\323\222X\357O\377\204\377\0_\31\202^J|q\375\20\377\22\n\236\227\t/\5I\376\177e\264M\241ajy-\377\230\4\25Mr\205\20\210\367lR\322\217\331/\31\1\1\320\0\27\37\377\1\17\377\377\17\377\377\17\377\377\17\377\377\17\17\377\377\16\212\377\17\233\377\17\377p\322\225\340\17|\211/\345\365\365\0\32(\342\17\377-\367k\242=\21\211/\374\364\364\327\0\r\37\377\364\17\377-\271\67\335\277\36\t/\f>X\330\244\317d\337)\230o\377\272\0]\0l\377\0\203\0\20eV\302N\177\t/\5\336W7 y\330\220\277\257o\377O\0\251\0w\0\376{\0\26\220\253\301W\331/\200 \t(\377\17\377\377\17\377\377\17\377\377\17\377\377\16|\377\17\215\377{\17\377x\376\241\177\251\211/\313\365{\17\377\64\37\376\201\346\211/\353\364{\17\377\64\341_\376O\t/\fG\237\375\66\351\222\334\343\252\357\377<\367e\36\302\"\t/\5\70\66I\367\256sm\2\372\"\367\220\365\333\301\2\331/!\1\71\60\27\0\0\t7\377\17\377\377\17\377\377\17\377\377\17\377\377\16\213\377\17\234\377\17\377p\356\327\34V\210\331\367>\365\71\60\336\32\0\t7-d$\372\67\211/\333\37\364\71\60\r\0\t7-\263\70\337\254\321\t/\f.\274vDP\357\337\b'\177\377\272\0\233\0\377\204H\245}e\316\302\373\177\t/\5xZ[o\230*\273\277\37\177\377O\0u\0\210\275\375\262\267\220\320\301\332\331/\"\0\t'\377\17\377\377\17\377\377\17\377\377\17\377\377\16{\377\17\214\377\17\377x\366\334j_b\211/<\365\17\377\64\366y\276\341\246\211/\36\364\17\377\64\367\245\324\276\304\t/\f\364\266[\373\v\33\70\264\205\357\377\366e\357{\302\215\t/\5\253\324f\307\367\334B\21Q\357\377\312\220e\337\301\257\331/#\1\"\0\27wO\377\27\0\b/\377\b\0|\377\b\34\4,\25$\2\0\270\0\31L\1\1\0\0!\17\ts\377\17\377\377\3\17\377\377\17\377\377\17\377\377\16\307\377\17\330\377\17\377\64\261\0\337\21\251\211/\311\365<\0\32wO\377\32\0\373/\377\373\0\243\376\373\343\363\303\355\70\367\30\370g\30\373\365\0\0!\17\31s\316\0\63\276\346\211/\352\364\36\0\rO\377\357\r\0\363/\377\363\0\337\363\371\377\367\357\370\34\31\237\31L\364\317\364\0\0!\17\31s\177\0\"O\177\177\377\231\0\263\0U\0>\377\360\0P\0\70\0n\240\377^PT\360L-{\231\377\276;:P\212\304G\247\377`"
		"\316*\307dZ\30\302\377\274M\203\356M|\215R\377\200\7\313\346\0`\0Z\177\20.)\352\0a\0\304\0\377\270U\252\244\240\361u\332\377\265(\24\324\246\6\315\63\377=Z\234\317\336\311\220\262\377!0cB\261\347\fe\374uP\307\f\340\22";
	const unsigned char *in = emptyDataBinComp;
	for (unsigned char bits = 0, code = 0, *out_end = out + 56448; out != out_end; code <<= 1, bits--)
	{
		if (bits == 0) { code = *in++; bits = 8; }
		if ((code & 0x80) != 0) { *out++ = *in++; continue; }
		int RLE1 = *in++, RLE = (RLE1<<8|*in++), RLESize = ((RLE >> 12) == 0 ? (*in++ + 0x12) : (RLE >> 12) + 2), RLEOffset = ((RLE & 0xFFF) + 1);
		while (RLESize > RLEOffset) { memcpy(out, out - RLEOffset, RLEOffset); out += RLEOffset; RLESize -= RLEOffset; RLEOffset <<= 1; }
		memcpy(out, out - RLEOffset, RLESize); out += RLESize;
	}
}

#if 0 // To use EncodeVorbis.c directly without the wasm recompile (warning: can break determinism of outcome)
extern "C" { extern void EncodeVorbis(int); uint32_t EncodeVorbisFeedSamples(float **buffer, uint32_t num); void EncodeVorbisOutput(const void* data, uint32_t len); };
static fnEncodeVorbisFeedSamples _cur_feed; static fnEncodeVorbisOutput _cur_outpt; static void* _cur_user_data;
uint32_t EncodeVorbisFeedSamples(float **buffer, uint32_t num) { return _cur_feed(buffer[0], buffer[1], num, _cur_user_data); }
void EncodeVorbisOutput(const void* data, uint32_t len) { _cur_outpt(data, len, _cur_user_data); }
void WasmEncodeVorbis(int q, fnEncodeVorbisFeedSamples feed, fnEncodeVorbisOutput outpt, void* user_data) { _cur_feed = feed; _cur_outpt = outpt; _cur_user_data = user_data; EncodeVorbis(q); }
#endif

#if 0 // Run the webassembly module via WASM3 interpreter (deterministic but very slow)
#ifdef _MSC_VER
#pragma comment(linker, "/STACK:4194304") // WASM3 can go into a very deep stack
#endif
#include "wasm3.h"
extern "C" { extern void EncodeVorbis(int); uint32_t EncodeVorbisFeedSamples(float **buffer, uint32_t num); void EncodeVorbisOutput(const void* data, uint32_t len); };
extern "C" { extern M3Result ResizeMemory(IM3Runtime io_runtime, uint32_t i_numPages); };
void WasmEncodeVorbis(int q, fnEncodeVorbisFeedSamples feed, fnEncodeVorbisOutput outpt, void* user_data)
{
	static fnEncodeVorbisFeedSamples _cur_feed; static fnEncodeVorbisOutput _cur_outpt; static void* _cur_user_data;
	_cur_feed = feed;
	_cur_outpt = outpt;
	_cur_user_data = user_data;

	FILE* fwasm = fopen(WASM3_MODULE_PATH, "rb");
	fseek(fwasm, 0, SEEK_END);
	size_t wsize = ftell(fwasm);
	fseek(fwasm, 0, SEEK_SET);
	Bit8u* wasm = (Bit8u*)malloc(wsize);
	fread(wasm, wsize, 1, fwasm);
	fclose(fwasm);

	#define M3ASSERT(cond) (void)((cond) ? ((int)0) : *(volatile int*)0 |= 0xbad|fprintf(stderr, "FAILED ASSERT (%s)\n", #cond))
	struct Local
	{
		static const void* wasmsbrk(IM3Runtime runtime, IM3ImportContext _ctx, uint64_t * _sp, void * _mem)
		{
			uint32_t req = *(uint32_t*)&_sp[1], incr = (req + 15) & ~15;
			uint32_t memSize = m3_GetMemorySize(runtime);
			static uint32_t lastHeapEnd;
			if (!lastHeapEnd || lastHeapEnd > memSize) lastHeapEnd = memSize;
			uint32_t oldPages = (lastHeapEnd + 65535) / 65536, newHeapEnd = lastHeapEnd + incr, newPages = (newHeapEnd + 65535) / 65536;
			if (oldPages < newPages) ResizeMemory(runtime, newPages);
			_sp[0] = lastHeapEnd;
			lastHeapEnd = newHeapEnd;
			return NULL;
		}
		static const void* wasmexit(IM3Runtime runtime, IM3ImportContext _ctx, uint64_t * _sp, void * _mem) { M3ASSERT(0); return "Exit called"; }
		static const void* wasmlog  (IM3Runtime runtime, IM3ImportContext _ctx, uint64_t * sp, void* mem) { *(double*)&sp[0] = log(*(double*)&sp[1]);                   return NULL; }
		static const void* wasmcos  (IM3Runtime runtime, IM3ImportContext _ctx, uint64_t * sp, void* mem) { *(double*)&sp[0] = cos(*(double*)&sp[1]);                   return NULL; }
		static const void* wasmsin  (IM3Runtime runtime, IM3ImportContext _ctx, uint64_t * sp, void* mem) { *(double*)&sp[0] = sin(*(double*)&sp[1]);                   return NULL; }
		static const void* wasmexp  (IM3Runtime runtime, IM3ImportContext _ctx, uint64_t * sp, void* mem) { *(double*)&sp[0] = exp(*(double*)&sp[1]);                   return NULL; }
		static const void* wasmatan (IM3Runtime runtime, IM3ImportContext _ctx, uint64_t * sp, void* mem) { *(double*)&sp[0] = atan(*(double*)&sp[1]);                  return NULL; }
		static const void* wasmpow  (IM3Runtime runtime, IM3ImportContext _ctx, uint64_t * sp, void* mem) { *(double*)&sp[0] = pow(*(double*)&sp[1], *(double*)&sp[2]); return NULL; }
		static const void* wasmldexp(IM3Runtime runtime, IM3ImportContext _ctx, uint64_t * sp, void* mem) { *(double*)&sp[0] = ldexp(*(double*)&sp[1], *(int*)&sp[2]);  return NULL; }
		static const void* wasmmemcpy(IM3Runtime runtime, IM3ImportContext _ctx, uint64_t * sp, void* mem) //(uint32_t dest, uint32_t src, uint32_t count)
		{
			memcpy(m3_GetMemory(runtime,0,0) + *(uint32_t*)&sp[1], m3_GetMemory(runtime,0,0) + *(uint32_t*)&sp[2], *(uint32_t*)&sp[3]);
			*(uint32_t*)&sp[0] = *(uint32_t*)&sp[1];
			return NULL;
		}
		static const void* wasmmemmove(IM3Runtime runtime, IM3ImportContext _ctx, uint64_t * sp, void* mem) //(uint32_t dest, uint32_t src, uint32_t count)
		{
			memmove(m3_GetMemory(runtime,0,0) + *(uint32_t*)&sp[1], m3_GetMemory(runtime,0,0) + *(uint32_t*)&sp[2], *(uint32_t*)&sp[3]);
			*(uint32_t*)&sp[0] = *(uint32_t*)&sp[1];
			return NULL;
		}
		static const void* wasmmemset(IM3Runtime runtime, IM3ImportContext _ctx, uint64_t * sp, void* mem) //(uint32_t dest, int ch, uint32_t count)
		{
			memset(m3_GetMemory(runtime,0,0) + *(uint32_t*)&sp[1], *(int*)&sp[2], *(uint32_t*)&sp[3]);
			*(uint32_t*)&sp[0] = *(uint32_t*)&sp[1];
			return NULL;
		}
		static const void* wasmEncodeVorbisOutput(IM3Runtime runtime, IM3ImportContext _ctx, uint64_t * sp, void* mem) //(uint32_t ptrData, uint32_t len)
		{
			_cur_outpt(m3_GetMemory(runtime,0,0) + *(uint32_t*)&sp[0], *(uint32_t*)&sp[1], _cur_user_data);
			return NULL;
		}
		static const void* wasmEncodeVorbisFeedSamples(IM3Runtime runtime, IM3ImportContext _ctx, uint64_t * sp, void* mem) //(uint32_t ptrBufferArr, uint32_t num)
		{
			uint32_t* ptrBuffer = (uint32_t*)(m3_GetMemory(runtime,0,0) + *(uint32_t*)&sp[1]);
			float* bufL = (float*)(m3_GetMemory(runtime,0,0) + ptrBuffer[0]);
			float* bufR = (float*)(m3_GetMemory(runtime,0,0) + ptrBuffer[1]);
			*(uint32_t*)&sp[0] = _cur_feed(bufL, bufR, *(uint32_t*)&sp[2], _cur_user_data);
			return NULL;
		}
	};
	M3Result err;
	IM3Environment env = m3_NewEnvironment(); M3ASSERT(env);
	IM3Runtime runtime = m3_NewRuntime(env, 1024*1024, NULL); M3ASSERT(runtime);
	IM3Module module; err = m3_ParseModule(env, &module, wasm, (uint32_t)wsize); M3ASSERT(!err);
	err = m3_LoadModule(runtime, module); M3ASSERT(!err);
	IM3Function f;
	err = m3_LinkRawFunction(module, "env", "exit",                    "v(i)",   &Local::wasmexit                   ); M3ASSERT(!err);
	err = m3_LinkRawFunction(module, "env", "sbrk",                    "i(i)",   &Local::wasmsbrk                   ); M3ASSERT(!err);
	err = m3_LinkRawFunction(module, "env", "memcpy",                  "i(iii)", &Local::wasmmemcpy                 ); M3ASSERT(!err);
	err = m3_LinkRawFunction(module, "env", "memset",                  "i(iii)", &Local::wasmmemset                 ); M3ASSERT(!err);
	err = m3_LinkRawFunction(module, "env", "memmove",                 "i(iii)", &Local::wasmmemmove                ); M3ASSERT(!err);
	err = m3_LinkRawFunction(module, "env", "log",                     "F(F)",   &Local::wasmlog                    ); M3ASSERT(!err);
	err = m3_LinkRawFunction(module, "env", "cos",                     "F(F)",   &Local::wasmcos                    ); M3ASSERT(!err);
	err = m3_LinkRawFunction(module, "env", "sin",                     "F(F)",   &Local::wasmsin                    ); M3ASSERT(!err);
	err = m3_LinkRawFunction(module, "env", "exp",                     "F(F)",   &Local::wasmexp                    ); M3ASSERT(!err);
	err = m3_LinkRawFunction(module, "env", "atan",                    "F(F)",   &Local::wasmatan                   ); M3ASSERT(!err);
	err = m3_LinkRawFunction(module, "env", "pow",                     "F(FF)",  &Local::wasmpow                    ); M3ASSERT(!err);
	err = m3_LinkRawFunction(module, "env", "ldexp",                   "F(Fi)",  &Local::wasmldexp                  ); M3ASSERT(!err);
	err = m3_LinkRawFunction(module, "env", "EncodeVorbisOutput",      "v(ii)",  &Local::wasmEncodeVorbisOutput     ); M3ASSERT(!err);
	err = m3_LinkRawFunction(module, "env", "EncodeVorbisFeedSamples", "i(ii)",  &Local::wasmEncodeVorbisFeedSamples); M3ASSERT(!err);
	err = m3_FindFunction(&f, runtime, "__wasm_call_ctors"); M3ASSERT(!err);
	err = m3_CallV(f); M3ASSERT(!err);
	err = m3_FindFunction(&f, runtime, "EncodeVorbis"); M3ASSERT(!err);
	err = m3_CallV(f, q); M3ASSERT(!err);
}
#endif
