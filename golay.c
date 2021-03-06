// -*- Mode: C; c-basic-offset: 8; -*-
//
// Copyright (c) 2012 Andrew Tridgell, All Rights Reserved
//
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions
// are met:
//
//  o Redistributions of source code must retain the above copyright
//    notice, this list of conditions and the following disclaimer.
//  o Redistributions in binary form must reproduce the above copyright
//    notice, this list of conditions and the following disclaimer in
//    the documentation and/or other materials provided with the distribution.
//
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
// "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
// LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS
// FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE
// COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT,
// INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
// (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
// SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
// HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT,
// STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
// ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED
// OF THE POSSIBILITY OF SUCH DAMAGE.
//

///
/// @file	golay23.c
///
/// golay 23/12 error correction encoding and decoding
///

#include <stdint.h>
#include "golay23.h"

// intermediate arrays for encodeing/decoding. Using these
// 符号化/復号のための中間配列。これらを使用することで、
// saves some interal memory that would otherwise be needed
// for pointers
// ポインターに必要な内部メモリを節約することができる。
static uint8_t g3[3], g6[6];

// encode 3 bytes data into 6 bytes of coded data
// 3バイトのデータを6バイトの符号化データに符号化する。
// input is in g3[], output in g6[]
// 入力はg3[]、出力はg6[]
static void 
golay_encode24(void)
{
	uint16_t v;
	uint16_t syn;

	v = g3[0] | ((uint16_t)g3[1] & 0x0F) << 8;
	syn = golay23_encode[v];
	g6[0] = syn & 0xFF;
	g6[1] = (g3[0] & 0x1F) << 3 | syn >> 8;
	g6[2] = (g3[0] & 0xE0) >> 5 | (g3[1] & 0x0F) << 3;

	v = g3[2] | ((uint16_t)g3[1] & 0xF0) << 4;
	syn = golay23_encode[v];
	g6[3] = syn & 0xFF;
	g6[4] = (g3[2] & 0x1F) << 3 | syn >> 8;
	g6[5] = (g3[2] & 0xE0) >> 5 | (g3[1] & 0xF0) >> 1;
}

// encode n bytes of data into 2n coded bytes. n must be a multiple 3
// nバイトのデータを2nバイトに符号化する。nは3の倍数でなければならない。
// encoding takes about 6 microseconds per input byte
// 符号化には1バイトあたり約6マイクロ秒かかる。
void 
golay_encode(uint8_t n, uint8_t * in, uint8_t * out)
{
	while (n >= 3) {
		g3[0] = in[0]; g3[1] = in[1]; g3[2] = in[2];
		golay_encode24();
		out[0] = g6[0]; out[1] = g6[1]; out[2] = g6[2]; 
		out[3] = g6[3]; out[4] = g6[4]; out[5] = g6[5]; 
		in += 3;
		out += 6;
		n -= 3;
	}
}

// decode 6 bytes of coded data into 3 bytes of original data
// 6バイトの符号化されたデータを3バイトのオリジナルデータに復号する。
// input is in g6[], output in g3[]
// 入力はg6[]、出力はg3[]
// returns the number of words corrected (0, 1 or 2)
// 訂正された符号語の和を返す。
static uint8_t 
golay_decode24(void)
{
	uint16_t v;
	uint16_t syn;
	uint16_t e;
	uint8_t errcount = 0;

	v = (g6[2] & 0x7F) << 5 | (g6[1] & 0xF8) >> 3;
	syn = golay23_encode[v];
	syn ^= g6[0] | (g6[1] & 0x07) << 8;
	e = golay23_decode[syn];
	if (e) {
		errcount++;
		v ^= e;
	}
	g3[0] = v & 0xFF;
	g3[1] = v >> 8;

	v = (g6[5] & 0x7F) << 5 | (g6[4] & 0xF8) >> 3;
	syn = golay23_encode[v];
	syn ^= g6[3] | (g6[4] & 0x07) << 8;
	e = golay23_decode[syn];
	if (e) {
		errcount++;
		v ^= e;
	}
	g3[1] |= (v >> 4) & 0xF0;
	g3[2] = v & 0xFF;

	return errcount;
}

// decode n bytes of coded data into n/2 bytes of original data
// nバイトの符号化データをn/2バイトの元のデータに復号する。
// n must be a multiple of 6
// nは6の倍数でなければならない。
// decoding takes about 4 microseconds per input byte
// 復号に要する時間は、入力1バイトあたり約4マイクロ秒。
// the number of 12 bit words that required correction is returned
// 訂正が必要な12ビットワードの数が返される。
uint8_t 
golay_decode(uint8_t n, uint8_t * in, uint8_t * out)
{
	uint8_t errcount = 0;
	while (n >= 6) {
		g6[0] = in[0]; g6[1] = in[1]; g6[2] = in[2];
		g6[3] = in[3]; g6[4] = in[4]; g6[5] = in[5];
		errcount += golay_decode24();
		out[0] = g3[0]; out[1] = g3[1]; out[2] = g3[2];
		in += 6;
		out += 3;
		n -= 6;
	}
	return errcount;
}
