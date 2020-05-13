/*
===========================================================================

Doom 3 GPL Source Code
Copyright (C) 1999-2011 id Software LLC, a ZeniMax Media company.

This file is part of the Doom 3 GPL Source Code ("Doom 3 Source Code").

Doom 3 Source Code is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

Doom 3 Source Code is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with Doom 3 Source Code.  If not, see <http://www.gnu.org/licenses/>.

In addition, the Doom 3 Source Code is also subject to certain additional terms. You should have received a copy of these additional terms immediately following the terms and conditions of the GNU General Public License which accompanied the Doom 3 Source Code.  If not, please request a copy in writing from id Software at the address below.

If you have questions concerning this license or the applicable additional terms, you may contact in writing id Software LLC, c/o ZeniMax Media Inc., Suite 120, Rockville, Maryland 20850 USA.

===========================================================================
*/

#include "sys/platform.h"

#include "idlib/math/Simd_SSE.h"

//===============================================================
//                                                        M
//  SSE implementation of idSIMDProcessor                MrE
//                                                        E
//===============================================================

#define DRAWVERT_SIZE				60
#define DRAWVERT_XYZ_OFFSET			(0*4)
#define DRAWVERT_ST_OFFSET			(3*4)
#define DRAWVERT_NORMAL_OFFSET		(5*4)
#define DRAWVERT_TANGENT0_OFFSET	(8*4)
#define DRAWVERT_TANGENT1_OFFSET	(11*4)
#define DRAWVERT_COLOR_OFFSET		(14*4)

#if defined(__GNUC__) && defined(__SSE__)

#include <xmmintrin.h>

#define SHUFFLEPS( x, y, z, w )		(( (x) & 3 ) << 6 | ( (y) & 3 ) << 4 | ( (z) & 3 ) << 2 | ( (w) & 3 ))
#define R_SHUFFLEPS( x, y, z, w )	(( (w) & 3 ) << 6 | ( (z) & 3 ) << 4 | ( (y) & 3 ) << 2 | ( (x) & 3 ))

/*
============
idSIMD_SSE::GetName
============
*/
const char * idSIMD_SSE::GetName( void ) const {
	return "MMX & SSE";
}

/*
============
idSIMD_SSE::Dot

  dst[i] = constant.Normal() * src[i].xyz + constant[3];
============
*/
void VPCALL idSIMD_SSE::Dot( float *dst, const idPlane &constant, const idDrawVert *src, const int count ) {
	// 0,  1,  2
	// 3,  4,  5
	// 6,  7,  8
	// 9, 10, 11

	/*
		mov			eax, count
		mov			edi, constant
		mov			edx, eax
		mov			esi, src
		mov			ecx, dst
	*/
	__m128 xmm0, xmm1, xmm2, xmm3, xmm4, xmm5, xmm6, xmm7;	// Declare 8 xmm registers.
	int count_l4 = count;                                   // count_l4 = eax
	int count_l1 = count;                                   // count_l1 = edx
	char *constant_p = (char *)&constant;                   // constant_p = edi
	char *src_p = (char *) src;                             // src_p = esi
	char *dst_p = (char *) dst;                             // dst_p = ecx

	assert( sizeof( idDrawVert ) == DRAWVERT_SIZE );
	assert( ptrdiff_t(&src->xyz) - ptrdiff_t(src) == DRAWVERT_XYZ_OFFSET );

	/*
		and			eax, ~3
		movss		xmm4, [edi+0]
		shufps		xmm4, xmm4, R_SHUFFLEPS( 0, 0, 0, 0 )
		movss		xmm5, [edi+4]
		shufps		xmm5, xmm5, R_SHUFFLEPS( 0, 0, 0, 0 )
		movss		xmm6, [edi+8]
		shufps		xmm6, xmm6, R_SHUFFLEPS( 0, 0, 0, 0 )
		movss		xmm7, [edi+12]
		shufps		xmm7, xmm7, R_SHUFFLEPS( 0, 0, 0, 0 )
	*/
	count_l4 = count_l4 & ~3;
	xmm4 = _mm_load_ss((float *) (constant_p));
	xmm4 = _mm_shuffle_ps(xmm4, xmm4, R_SHUFFLEPS( 0, 0, 0, 0 ));
	xmm5 = _mm_load_ss((float *) (constant_p + 4));
	xmm5 = _mm_shuffle_ps(xmm5, xmm5, R_SHUFFLEPS( 0, 0, 0, 0 ));
	xmm6 = _mm_load_ss((float *) (constant_p + 8));
	xmm6 = _mm_shuffle_ps(xmm6, xmm6, R_SHUFFLEPS( 0, 0, 0, 0 ));
	xmm7 = _mm_load_ss((float *) (constant_p + 12));
	xmm7 = _mm_shuffle_ps(xmm7, xmm7, R_SHUFFLEPS( 0, 0, 0, 0 ));

	/*
		jz			startVert1
	*/
	if(count_l4 != 0) {
	/*
		imul		eax, DRAWVERT_SIZE
		add			esi, eax
		neg			eax
	*/
		count_l4 = count_l4 * DRAWVERT_SIZE;
		src_p = src_p + count_l4;
		count_l4 = -count_l4;
	/*
	loopVert4:
	*/
		do {
	/*
		movss		xmm0, [esi+eax+1*DRAWVERT_SIZE+DRAWVERT_XYZ_OFFSET+0]	//  3,  X,  X,  X
		movss		xmm2, [esi+eax+0*DRAWVERT_SIZE+DRAWVERT_XYZ_OFFSET+8]	//  2,  X,  X,  X
		movhps		xmm0, [esi+eax+0*DRAWVERT_SIZE+DRAWVERT_XYZ_OFFSET+0]	//  3,  X,  0,  1
		movaps		xmm1, xmm0												//  3,  X,  0,  1
	*/
			xmm0 = _mm_load_ss((float *) (src_p+count_l4+1*DRAWVERT_SIZE+DRAWVERT_XYZ_OFFSET+0));        // 3,  X,  X,  X
			xmm2 = _mm_load_ss((float *) (src_p+count_l4+0*DRAWVERT_SIZE+DRAWVERT_XYZ_OFFSET+8));        // 2,  X,  X,  X
			xmm0 = _mm_loadh_pi(xmm0, (__m64 *) (src_p+count_l4+0*DRAWVERT_SIZE+DRAWVERT_XYZ_OFFSET+0)); // 3,  X,  0,  1
			xmm1 = xmm0;							                                                    // 3,  X,  0,  1

	/*
		movlps		xmm1, [esi+eax+1*DRAWVERT_SIZE+DRAWVERT_XYZ_OFFSET+4]	//  4,  5,  0,  1
		shufps		xmm2, xmm1, R_SHUFFLEPS( 0, 1, 0, 1 )					//  2,  X,  4,  5
	*/
			xmm1 = _mm_loadl_pi(xmm1, (__m64 *) (src_p+count_l4+1*DRAWVERT_SIZE+DRAWVERT_XYZ_OFFSET+4)); // 4,  5,  0,  1
			xmm2 = _mm_shuffle_ps(xmm2, xmm1, R_SHUFFLEPS( 0, 1, 0, 1 ));                               // 2,  X,  4,  5

	/*
		movss		xmm3, [esi+eax+3*DRAWVERT_SIZE+DRAWVERT_XYZ_OFFSET+0]	//  9,  X,  X,  X
		movhps		xmm3, [esi+eax+2*DRAWVERT_SIZE+DRAWVERT_XYZ_OFFSET+0]	//  9,  X,  6,  7
		shufps		xmm0, xmm3, R_SHUFFLEPS( 2, 0, 2, 0 )					//  0,  3,  6,  9
	*/
			xmm3 = _mm_load_ss((float *) (src_p+count_l4+3*DRAWVERT_SIZE+DRAWVERT_XYZ_OFFSET+0));        // 9,  X,  X,  X
			xmm3 = _mm_loadh_pi(xmm3, (__m64 *) (src_p+count_l4+2*DRAWVERT_SIZE+DRAWVERT_XYZ_OFFSET+0)); // 9,  X,  6,  7
			xmm0 = _mm_shuffle_ps(xmm0, xmm3, R_SHUFFLEPS( 2, 0, 2, 0 ));                               // 0,  3,  6,  9
	/*
		movlps		xmm3, [esi+eax+3*DRAWVERT_SIZE+DRAWVERT_XYZ_OFFSET+4]	// 10, 11,  6,  7
		shufps		xmm1, xmm3, R_SHUFFLEPS( 3, 0, 3, 0 )					//  1,  4,  7, 10
	*/
			xmm3 = _mm_loadl_pi(xmm3, (__m64 *)(src_p+count_l4+3*DRAWVERT_SIZE+DRAWVERT_XYZ_OFFSET+4));  // 10, 11, 6,  7
			xmm1 = _mm_shuffle_ps(xmm1, xmm3, R_SHUFFLEPS( 3, 0, 3, 0 ));                               // 1,  4,  7,  10
	/*
		movhps		xmm3, [esi+eax+2*DRAWVERT_SIZE+DRAWVERT_XYZ_OFFSET+8]	// 10, 11,  8,  X
		shufps		xmm2, xmm3, R_SHUFFLEPS( 0, 3, 2, 1 )					//  2,  5,  8, 11
	*/
			xmm3 = _mm_loadh_pi(xmm3, (__m64 *)(src_p+count_l4+2*DRAWVERT_SIZE+DRAWVERT_XYZ_OFFSET+8));  // 10, 11, 8,  X
			xmm2 = _mm_shuffle_ps(xmm2, xmm3, R_SHUFFLEPS( 0, 3, 2, 1 ));                               // 2,  5,  8,  11

	/*
		add			ecx, 16
		add			eax, 4*DRAWVERT_SIZE
	*/
			dst_p = dst_p + 16;
			count_l4 = count_l4 + 4*DRAWVERT_SIZE;

	/*
		mulps		xmm0, xmm4
		mulps		xmm1, xmm5
		mulps		xmm2, xmm6
		addps		xmm0, xmm7
		addps		xmm0, xmm1
		addps		xmm0, xmm2
	*/
			xmm0 = _mm_mul_ps(xmm0, xmm4);
			xmm1 = _mm_mul_ps(xmm1, xmm5);
			xmm2 = _mm_mul_ps(xmm2, xmm6);
			xmm0 = _mm_add_ps(xmm0, xmm7);
			xmm0 = _mm_add_ps(xmm0, xmm1);
			xmm0 = _mm_add_ps(xmm0, xmm2);

	/*
		movlps		[ecx-16+0], xmm0
		movhps		[ecx-16+8], xmm0
		jl			loopVert4
	*/
			_mm_storel_pi((__m64 *) (dst_p-16+0), xmm0);
			_mm_storeh_pi((__m64 *) (dst_p-16+8), xmm0);
		} while(count_l4 < 0);
	}

	/*
	startVert1:
		and			edx, 3
		jz			done
	*/
	count_l1 = count_l1 & 3;
	if(count_l1 != 0) {
	/*
		loopVert1:
		movss		xmm0, [esi+eax+DRAWVERT_XYZ_OFFSET+0]
		movss		xmm1, [esi+eax+DRAWVERT_XYZ_OFFSET+4]
		movss		xmm2, [esi+eax+DRAWVERT_XYZ_OFFSET+8]
		mulss		xmm0, xmm4
		mulss		xmm1, xmm5
		mulss		xmm2, xmm6
		addss		xmm0, xmm7
		add			ecx, 4
		addss		xmm0, xmm1
		add			eax, DRAWVERT_SIZE
		addss		xmm0, xmm2
		dec			edx
		movss		[ecx-4], xmm0
		jnz			loopVert1
	*/
		do {
			xmm0 = _mm_load_ss((float *) (src_p+count_l4+DRAWVERT_XYZ_OFFSET+0));
			xmm1 = _mm_load_ss((float *) (src_p+count_l4+DRAWVERT_XYZ_OFFSET+4));
			xmm2 = _mm_load_ss((float *) (src_p+count_l4+DRAWVERT_XYZ_OFFSET+8));
			xmm0 = _mm_mul_ss(xmm0, xmm4);
			xmm1 = _mm_mul_ss(xmm1, xmm5);
			xmm2 = _mm_mul_ss(xmm2, xmm6);
			xmm0 = _mm_add_ss(xmm0, xmm7);
			dst_p = dst_p + 4;
			xmm0 = _mm_add_ss(xmm0, xmm1);
			count_l4 = count_l4 + DRAWVERT_SIZE;
			xmm0 = _mm_add_ss(xmm0, xmm2);
			count_l1 = count_l1 - 1;
			_mm_store_ss((float *) (dst_p-4), xmm0);
		} while( count_l1 != 0);
	}
	/*
		done:
	*/
}

/*
============
idSIMD_SSE::MinMax
============
*/
void VPCALL idSIMD_SSE::MinMax( idVec3 &min, idVec3 &max, const idDrawVert *src, const int *indexes, const int count ) {

	assert( sizeof( idDrawVert ) == DRAWVERT_SIZE );
	assert( ptrdiff_t(&src->xyz) - ptrdiff_t(src) == DRAWVERT_XYZ_OFFSET );

	__m128 xmm0, xmm1, xmm2, xmm3, xmm4, xmm5, xmm6, xmm7;
	char *indexes_p;
	char *src_p;
	int count_l;
	int edx;
	char *min_p;
	char *max_p;

	/*
		movss		xmm0, idMath::INFINITY
		xorps		xmm1, xmm1
		shufps		xmm0, xmm0, R_SHUFFLEPS( 0, 0, 0, 0 )
		subps		xmm1, xmm0
		movaps		xmm2, xmm0
		movaps		xmm3, xmm1
	*/
		xmm0 = _mm_load_ss(&idMath::INFINITY);
		// To satisfy the compiler use xmm0 instead.
		xmm1 = _mm_xor_ps(xmm0, xmm0);
		xmm0 = _mm_shuffle_ps(xmm0, xmm0, R_SHUFFLEPS( 0, 0, 0, 0 ));
		xmm1 = _mm_sub_ps(xmm1, xmm0);
		xmm2 = xmm0;
		xmm3 = xmm1;

	/*
		mov			edi, indexes
		mov			esi, src
		mov			eax, count
		and			eax, ~3
		jz			done4
	*/
		indexes_p = (char *) indexes;
		src_p = (char *) src;
		count_l = count;
		count_l = count_l & ~3;
		if(count_l != 0) {
	/*
		shl			eax, 2
		add			edi, eax
		neg			eax
	*/
			count_l = count_l << 2;
			indexes_p = indexes_p + count_l;
			count_l = -count_l;
	/*
	loop4:
//		prefetchnta	[edi+128]
//		prefetchnta	[esi+4*DRAWVERT_SIZE+DRAWVERT_XYZ_OFFSET]
	*/
		do {
	/*
		mov			edx, [edi+eax+0]
		imul		edx, DRAWVERT_SIZE
		movss		xmm4, [esi+edx+DRAWVERT_XYZ_OFFSET+8]
		movhps		xmm4, [esi+edx+DRAWVERT_XYZ_OFFSET+0]
		minps		xmm0, xmm4
		maxps		xmm1, xmm4
	*/
			edx = *((int*)(indexes_p+count_l+0));
			edx = edx * DRAWVERT_SIZE;
			xmm4 = _mm_load_ss((float *) (src_p+edx+DRAWVERT_XYZ_OFFSET+8));
			xmm4 = _mm_loadh_pi(xmm4, (__m64 *) (src_p+edx+DRAWVERT_XYZ_OFFSET+0) );
			xmm0 = _mm_min_ps(xmm0, xmm4);
			xmm1 = _mm_max_ps(xmm1, xmm4);

	/*
		mov			edx, [edi+eax+4]
		imul		edx, DRAWVERT_SIZE
		movss		xmm5, [esi+edx+DRAWVERT_XYZ_OFFSET+0]
		movhps		xmm5, [esi+edx+DRAWVERT_XYZ_OFFSET+4]
		minps		xmm2, xmm5
		maxps		xmm3, xmm5
	*/
			edx = *((int*)(indexes_p+count_l+4));
			edx = edx * DRAWVERT_SIZE;
			xmm5 = _mm_load_ss((float *) (src_p+edx+DRAWVERT_XYZ_OFFSET+0));
			xmm5 = _mm_loadh_pi(xmm5, (__m64 *) (src_p+edx+DRAWVERT_XYZ_OFFSET+4) );
			xmm2 = _mm_min_ps(xmm2, xmm5);
			xmm3 = _mm_max_ps(xmm3, xmm5);

	/*
		mov			edx, [edi+eax+8]
		imul		edx, DRAWVERT_SIZE
		movss		xmm6, [esi+edx+DRAWVERT_XYZ_OFFSET+8]
		movhps		xmm6, [esi+edx+DRAWVERT_XYZ_OFFSET+0]
		minps		xmm0, xmm6
		maxps		xmm1, xmm6
	*/
			edx = *((int*)(indexes_p+count_l+8));
			edx = edx * DRAWVERT_SIZE;
			xmm6 = _mm_load_ss((float *) (src_p+edx+DRAWVERT_XYZ_OFFSET+8));
			xmm6 = _mm_loadh_pi(xmm6, (__m64 *) (src_p+edx+DRAWVERT_XYZ_OFFSET+0) );
			xmm0 = _mm_min_ps(xmm0, xmm6);
			xmm1 = _mm_max_ps(xmm1, xmm6);

	/*
		mov			edx, [edi+eax+12]
		imul		edx, DRAWVERT_SIZE
		movss		xmm7, [esi+edx+DRAWVERT_XYZ_OFFSET+0]
		movhps		xmm7, [esi+edx+DRAWVERT_XYZ_OFFSET+4]
		minps		xmm2, xmm7
		maxps		xmm3, xmm7
	*/
			edx = *((int*)(indexes_p+count_l+12));
			edx = edx * DRAWVERT_SIZE;
			xmm7 = _mm_load_ss((float *) (src_p+edx+DRAWVERT_XYZ_OFFSET+0));
			xmm7 = _mm_loadh_pi(xmm7, (__m64 *) (src_p+edx+DRAWVERT_XYZ_OFFSET+4) );
			xmm2 = _mm_min_ps(xmm2, xmm7);
			xmm3 = _mm_max_ps(xmm3, xmm7);

	/*
		add			eax, 4*4
		jl			loop4
	*/
			count_l = count_l + 4*4;
		} while (count_l < 0);
	}
	/*
	done4:
		mov			eax, count
		and			eax, 3
		jz			done1
	*/
	count_l = count;
	count_l = count_l & 3;
	if(count_l != 0) {
	/*
		shl			eax, 2
		add			edi, eax
		neg			eax
	*/
		count_l = count_l << 2;
		indexes_p = indexes_p + count_l;
		count_l = -count_l;
	/*
	loop1:
	*/
		do{
	/*
		mov			edx, [edi+eax+0]
		imul		edx, DRAWVERT_SIZE;
		movss		xmm4, [esi+edx+DRAWVERT_XYZ_OFFSET+8]
		movhps		xmm4, [esi+edx+DRAWVERT_XYZ_OFFSET+0]
		minps		xmm0, xmm4
		maxps		xmm1, xmm4
	*/
			edx = *((int*)(indexes_p+count_l+0));
			edx = edx * DRAWVERT_SIZE;
			xmm4 = _mm_load_ss((float *) (src_p+edx+DRAWVERT_XYZ_OFFSET+8));
			xmm4 = _mm_loadh_pi(xmm4, (__m64 *) (src_p+edx+DRAWVERT_XYZ_OFFSET+0) );
			xmm0 = _mm_min_ps(xmm0, xmm4);
			xmm1 = _mm_max_ps(xmm1, xmm4);

	/*
		add			eax, 4
		jl			loop1
	*/
			count_l = count_l + 4;
		} while (count_l < 0);

	}

	/*
	done1:
		shufps		xmm2, xmm2, R_SHUFFLEPS( 3, 1, 0, 2 )
		shufps		xmm3, xmm3, R_SHUFFLEPS( 3, 1, 0, 2 )
		minps		xmm0, xmm2
		maxps		xmm1, xmm3
		mov			esi, min
		movhps		[esi], xmm0
		movss		[esi+8], xmm0
		mov			edi, max
		movhps		[edi], xmm1
		movss		[edi+8], xmm1
	*/
	xmm2 = _mm_shuffle_ps(xmm2, xmm2, R_SHUFFLEPS( 3, 1, 0, 2 ));
	xmm3 = _mm_shuffle_ps(xmm3, xmm3, R_SHUFFLEPS( 3, 1, 0, 2 ));
	xmm0 = _mm_min_ps(xmm0, xmm2);
	xmm1 = _mm_max_ps(xmm1, xmm3);
	min_p = (char *) &min;
	_mm_storeh_pi((__m64 *)(min_p), xmm0);
	_mm_store_ss((float *)(min_p+8), xmm0);
	max_p = (char *) &max;
	_mm_storeh_pi((__m64 *)(max_p), xmm1);
	_mm_store_ss((float *)(max_p+8), xmm1);
}

/*
============
idSIMD_SSE::Dot

  dst[i] = constant * src[i].Normal() + src[i][3];
============
*/
void VPCALL idSIMD_SSE::Dot( float *dst, const idVec3 &constant, const idPlane *src, const int count ) {
	int count_l4;
	int count_l1;
	char *constant_p;
	char *src_p;
	char *dst_p;
	__m128 xmm0, xmm1, xmm2, xmm3, xmm4, xmm5, xmm6, xmm7;

	/*
		mov			eax, count
		mov			edi, constant
		mov			edx, eax
		mov			esi, src
		mov			ecx, dst
		and			eax, ~3
	*/
	count_l4 = count;
	constant_p = (char *) &constant;
	count_l1 = count_l4;
	src_p = (char *) src;
	dst_p = (char *) dst;
	count_l4 = count_l4 & ~3;

	/*
		movss		xmm5, [edi+0]
		shufps		xmm5, xmm5, R_SHUFFLEPS( 0, 0, 0, 0 )
		movss		xmm6, [edi+4]
		shufps		xmm6, xmm6, R_SHUFFLEPS( 0, 0, 0, 0 )
		movss		xmm7, [edi+8]
		shufps		xmm7, xmm7, R_SHUFFLEPS( 0, 0, 0, 0 )
	*/
	xmm5 = _mm_load_ss((float *) (constant_p+0));
	xmm5 = _mm_shuffle_ps(xmm5, xmm5, R_SHUFFLEPS( 0, 0, 0, 0 ));
	xmm6 = _mm_load_ss((float *) (constant_p+4));
	xmm6 = _mm_shuffle_ps(xmm6, xmm6, R_SHUFFLEPS( 0, 0, 0, 0 ));
	xmm7 = _mm_load_ss((float *) (constant_p+8));
	xmm7 = _mm_shuffle_ps(xmm7, xmm7, R_SHUFFLEPS( 0, 0, 0, 0 ));

	/*
		jz			startVert1
	*/
	if (count_l4 != 0) {
	/*
		imul		eax, 16
		add			esi, eax
		neg			eax
	*/
		count_l4 = count_l4 * 16;
		src_p = src_p + count_l4;
		count_l4 = -count_l4;
	/*
	loopVert4:
	*/
		do {
	/*
		movlps		xmm1, [esi+eax+ 0]
		movlps		xmm3, [esi+eax+ 8]
		movhps		xmm1, [esi+eax+16]
		movhps		xmm3, [esi+eax+24]
		movlps		xmm2, [esi+eax+32]
		movlps		xmm4, [esi+eax+40]
		movhps		xmm2, [esi+eax+48]
		movhps		xmm4, [esi+eax+56]
		movaps		xmm0, xmm1
		shufps		xmm0, xmm2, R_SHUFFLEPS( 0, 2, 0, 2 )
		shufps		xmm1, xmm2, R_SHUFFLEPS( 1, 3, 1, 3 )
		movaps		xmm2, xmm3
		shufps		xmm2, xmm4, R_SHUFFLEPS( 0, 2, 0, 2 )
		shufps		xmm3, xmm4, R_SHUFFLEPS( 1, 3, 1, 3 )
	*/
			xmm1 = _mm_loadl_pi(xmm1, (__m64 *)(src_p+count_l4+ 0));
			xmm3 = _mm_loadl_pi(xmm3, (__m64 *)(src_p+count_l4+ 8));
			xmm1 = _mm_loadh_pi(xmm1, (__m64 *)(src_p+count_l4+16));
			xmm3 = _mm_loadh_pi(xmm3, (__m64 *)(src_p+count_l4+24));
			xmm2 = _mm_loadl_pi(xmm2, (__m64 *)(src_p+count_l4+32));
			xmm4 = _mm_loadl_pi(xmm4, (__m64 *)(src_p+count_l4+40));
			xmm2 = _mm_loadh_pi(xmm2, (__m64 *)(src_p+count_l4+48));
			xmm4 = _mm_loadh_pi(xmm4, (__m64 *)(src_p+count_l4+56));

			xmm0 = xmm1;
			xmm0 = _mm_shuffle_ps(xmm0, xmm2, R_SHUFFLEPS( 0, 2, 0, 2 ));
			xmm1 = _mm_shuffle_ps(xmm1, xmm2, R_SHUFFLEPS( 1, 3, 1, 3 ));
			xmm2 = xmm3;
			xmm2 = _mm_shuffle_ps(xmm2, xmm4, R_SHUFFLEPS( 0, 2, 0, 2 ));
			xmm3 = _mm_shuffle_ps(xmm3, xmm4, R_SHUFFLEPS( 1, 3, 1, 3 ));

	/*
		add			ecx, 16
		add			eax, 4*16
	*/
			dst_p = dst_p + 16;
			count_l4 = count_l4 + 4*16;

	/*
		mulps		xmm0, xmm5
		mulps		xmm1, xmm6
		mulps		xmm2, xmm7
		addps		xmm0, xmm3
		addps		xmm0, xmm1
		addps		xmm0, xmm2
	*/
			xmm0 = _mm_mul_ps(xmm0, xmm5);
			xmm1 = _mm_mul_ps(xmm1, xmm6);
			xmm2 = _mm_mul_ps(xmm2, xmm7);
			xmm0 = _mm_add_ps(xmm0, xmm3);
			xmm0 = _mm_add_ps(xmm0, xmm1);
			xmm0 = _mm_add_ps(xmm0, xmm2);

	/*
		movlps		[ecx-16+0], xmm0
		movhps		[ecx-16+8], xmm0
		jl			loopVert4
	*/
			_mm_storel_pi((__m64 *) (dst_p-16+0), xmm0);
			_mm_storeh_pi((__m64 *) (dst_p-16+8), xmm0);
		} while (count_l4 < 0);
	}

	/*
	startVert1:
		and			edx, 3
		jz			done
	*/
	count_l1 = count_l1 & 3;

	if(count_l1 != 0) {
	/*
	loopVert1:
	*/
		do {
	/*
		movss		xmm0, [esi+eax+0]
		movss		xmm1, [esi+eax+4]
		movss		xmm2, [esi+eax+8]
		mulss		xmm0, xmm5
		mulss		xmm1, xmm6
		mulss		xmm2, xmm7
		addss		xmm0, [esi+eax+12]
		add			ecx, 4
		addss		xmm0, xmm1
		add			eax, 16
		addss		xmm0, xmm2
		dec			edx
		movss		[ecx-4], xmm0
		jnz			loopVert1
	*/
			xmm0 = _mm_load_ss((float *) (src_p+count_l4+ 0));
			xmm1 = _mm_load_ss((float *) (src_p+count_l4+ 4));
			xmm2 = _mm_load_ss((float *) (src_p+count_l4+ 8));
			xmm3 = _mm_load_ss((float *) (src_p+count_l4+12));

			xmm0 = _mm_mul_ss(xmm0, xmm5);
			xmm1 = _mm_mul_ss(xmm1, xmm6);
			xmm2 = _mm_mul_ss(xmm2, xmm7);

			xmm0 = _mm_add_ss(xmm0, xmm3);
			dst_p = dst_p + 4;
			xmm0 = _mm_add_ss(xmm0, xmm1);
			count_l4 = count_l4 + 16;
			xmm0 = _mm_add_ss(xmm0, xmm2);
			count_l1 = count_l1 - 1;
			_mm_store_ss((float *) (dst_p-4), xmm0);
		} while (count_l1 != 0);
	}
	/*
	done:
	*/
}

#elif defined(_MSC_VER) && defined(_M_IX86)

#include <xmmintrin.h>

#include "idlib/math/Vector.h"
#include "idlib/math/Matrix.h"
#include "idlib/math/Quat.h"
#include "idlib/math/Plane.h"

#define SHUFFLEPS( x, y, z, w )		(( (x) & 3 ) << 6 | ( (y) & 3 ) << 4 | ( (z) & 3 ) << 2 | ( (w) & 3 ))
#define R_SHUFFLEPS( x, y, z, w )	(( (w) & 3 ) << 6 | ( (z) & 3 ) << 4 | ( (y) & 3 ) << 2 | ( (x) & 3 ))

// transpose a 4x4 matrix loaded into 4 xmm registers (reg4 is temporary)
#define TRANSPOSE_4x4( reg0, reg1, reg2, reg3, reg4 )											\
	__asm	movaps		reg4, reg2								/* reg4 =  8,  9, 10, 11 */		\
	__asm	unpcklps	reg2, reg3								/* reg2 =  8, 12,  9, 13 */		\
	__asm	unpckhps	reg4, reg3								/* reg4 = 10, 14, 11, 15 */		\
	__asm	movaps		reg3, reg0								/* reg3 =  0,  1,  2,  3 */		\
	__asm	unpcklps	reg0, reg1								/* reg0 =  0,  4,  1,  5 */		\
	__asm	unpckhps	reg3, reg1								/* reg3 =  2,  6,  3,  7 */		\
	__asm	movaps		reg1, reg0								/* reg1 =  0,  4,  1,  5 */		\
	__asm	shufps		reg0, reg2, R_SHUFFLEPS( 0, 1, 0, 1 )	/* reg0 =  0,  4,  8, 12 */		\
	__asm	shufps		reg1, reg2, R_SHUFFLEPS( 2, 3, 2, 3 )	/* reg1 =  1,  5,  9, 13 */		\
	__asm	movaps		reg2, reg3								/* reg2 =  2,  6,  3,  7 */		\
	__asm	shufps		reg2, reg4, R_SHUFFLEPS( 0, 1, 0, 1 )	/* reg2 =  2,  6, 10, 14 */		\
	__asm	shufps		reg3, reg4, R_SHUFFLEPS( 2, 3, 2, 3 )	/* reg3 =  3,  7, 11, 15 */

// transpose a 4x4 matrix from memory into 4 xmm registers (reg4 is temporary)
#define TRANPOSE_4x4_FROM_MEMORY( address, reg0, reg1, reg2, reg3, reg4 )						\
	__asm	movlps		reg1, [address+ 0]						/* reg1 =  0,  1,  X,  X */		\
	__asm	movlps		reg3, [address+ 8]						/* reg3 =  2,  3,  X,  X */		\
	__asm	movhps		reg1, [address+16]						/* reg1 =  0,  1,  4,  5 */		\
	__asm	movhps		reg3, [address+24]						/* reg3 =  2,  3,  6,  7 */		\
	__asm	movlps		reg2, [address+32]						/* reg2 =  8,  9,  X,  X */		\
	__asm	movlps		reg4, [address+40]						/* reg4 = 10, 11,  X,  X */		\
	__asm	movhps		reg2, [address+48]						/* reg2 =  8,  9, 12, 13 */		\
	__asm	movhps		reg4, [address+56]						/* reg4 = 10, 11, 14, 15 */		\
	__asm	movaps		reg0, reg1								/* reg0 =  0,  1,  4,  5 */		\
	__asm	shufps		reg0, reg2, R_SHUFFLEPS( 0, 2, 0, 2 )	/* reg0 =  0,  4,  8, 12 */		\
	__asm	shufps		reg1, reg2, R_SHUFFLEPS( 1, 3, 1, 3 )	/* reg1 =  1,  5,  9, 13 */		\
	__asm	movaps		reg2, reg3								/* reg2 =  2,  3,  6,  7 */		\
	__asm	shufps		reg2, reg4, R_SHUFFLEPS( 0, 2, 0, 2 )	/* reg2 =  2,  6, 10, 14 */		\
	__asm	shufps		reg3, reg4, R_SHUFFLEPS( 1, 3, 1, 3 )	/* reg3 =  3,  7, 11, 15 */

// transpose a 4x4 matrix to memory from 4 xmm registers (reg4 is temporary)
#define TRANPOSE_4x4_TO_MEMORY( address, reg0, reg1, reg2, reg3, reg4 )							\
	__asm	movaps		reg4, reg0								/* reg4 =  0,  4,  8, 12 */		\
	__asm	unpcklps	reg0, reg1								/* reg0 =  0,  1,  4,  5 */		\
	__asm	unpckhps	reg4, reg1								/* reg4 =  8,  9, 12, 13 */		\
	__asm	movaps		reg1, reg2								/* reg1 =  2,  6, 10, 14 */		\
	__asm	unpcklps	reg2, reg3								/* reg2 =  2,  3,  6,  7 */		\
	__asm	unpckhps	reg1, reg3								/* reg1 = 10, 11, 14, 15 */		\
	__asm	movlps		[address+ 0], reg0						/* mem0 =  0,  1,  X,  X */		\
	__asm	movlps		[address+ 8], reg2						/* mem0 =  0,  1,  2,  3 */		\
	__asm	movhps		[address+16], reg0						/* mem1 =  4,  5,  X,  X */		\
	__asm	movhps		[address+24], reg2						/* mem1 =  4,  5,  6,  7 */		\
	__asm	movlps		[address+32], reg4						/* mem2 =  8,  9,  X,  X */		\
	__asm	movlps		[address+40], reg1						/* mem2 =  8,  9, 10, 11 */		\
	__asm	movhps		[address+48], reg4						/* mem3 = 12, 13,  X,  X */		\
	__asm	movhps		[address+56], reg1						/* mem3 = 12, 13, 14, 15 */

// transpose a 4x3 matrix loaded into 3 xmm registers (reg3 is temporary)
#define TRANSPOSE_4x3( reg0, reg1, reg2, reg3 )													\
	__asm	movaps		reg3, reg2								/* reg3 =  8,  9, 10, 11 */		\
	__asm	shufps		reg3, reg1, R_SHUFFLEPS( 2, 3, 0, 1 )	/* reg3 = 10, 11,  4,  5 */		\
	__asm	shufps		reg2, reg0, R_SHUFFLEPS( 0, 1, 2, 3 )	/* reg2 =  8,  9,  2,  3 */		\
	__asm	shufps		reg1, reg0, R_SHUFFLEPS( 2, 3, 0, 1 )	/* reg1 =  6,  7,  0,  1 */		\
	__asm	movaps		reg0, reg1								/* reg0 =  6,  7,  0,  1 */		\
	__asm	shufps		reg0, reg2, R_SHUFFLEPS( 2, 0, 3, 1 )	/* reg0 =  0,  6,  3,  9 */		\
	__asm	shufps		reg1, reg3, R_SHUFFLEPS( 3, 1, 2, 0 )	/* reg1 =  1,  7,  4, 10 */		\
	__asm	shufps		reg2, reg3, R_SHUFFLEPS( 2, 0, 3, 1 )	/* reg2 =  2,  8,  5, 11 */

// transpose a 4x3 matrix from memory into 3 xmm registers (reg3 is temporary)
#define TRANSPOSE_4x3_FROM_MEMORY( address, reg0, reg1, reg2, reg3 )							\
	__asm	movlps		reg1, [address+ 0]						/* reg1 =  0,  1,  X,  X */		\
	__asm	movlps		reg2, [address+ 8]						/* reg2 =  2,  3,  X,  X */		\
	__asm	movlps		reg3, [address+16]						/* reg3 =  4,  5,  X,  X */		\
	__asm	movhps		reg1, [address+24]						/* reg1 =  0,  1,  6,  7 */		\
	__asm	movhps		reg2, [address+32]						/* reg2 =  2,  3,  8,  9 */		\
	__asm	movhps		reg3, [address+40]						/* reg3 =  4,  5, 10, 11 */		\
	__asm	movaps		reg0, reg1								/* reg0 =  0,  1,  6,  7 */		\
	__asm	shufps		reg0, reg2, R_SHUFFLEPS( 0, 2, 1, 3 )	/* reg0 =  0,  6,  3,  9 */		\
	__asm	shufps		reg1, reg3, R_SHUFFLEPS( 1, 3, 0, 2 )	/* reg1 =  1,  7,  4, 10 */		\
	__asm	shufps		reg2, reg3, R_SHUFFLEPS( 0, 2, 1, 3 )	/* reg2 =  2,  8,  5, 11 */

// transpose a 4x3 matrix to memory from 3 xmm registers (reg3 is temporary)
#define TRANSPOSE_4x3_TO_MEMORY( address, reg0, reg1, reg2, reg3 )								\
	__asm	movhlps		reg3, reg0								/* reg3 =  3,  9,  X,  X */		\
	__asm	unpcklps	reg0, reg1								/* reg0 =  0,  1,  6,  7 */		\
	__asm	unpckhps	reg1, reg2								/* reg1 =  4,  5, 10, 11 */		\
	__asm	unpcklps	reg2, reg3								/* reg2 =  2,  3,  8,  9 */		\
	__asm	movlps		[address+ 0], reg0						/* mem0 =  0,  1,  X,  X */		\
	__asm	movlps		[address+ 8], reg2						/* mem0 =  0,  1,  2,  3 */		\
	__asm	movlps		[address+16], reg1						/* mem1 =  4,  5,  X,  X */		\
	__asm	movhps		[address+24], reg0						/* mem1 =  4,  5,  6,  7 */		\
	__asm	movhps		[address+32], reg2						/* mem2 =  8,  9,  X,  X */		\
	__asm	movhps		[address+40], reg1						/* mem2 =  8,  9, 10, 11 */


// with alignment
#define KFLOATINITS(   SRC0, COUNT, PRE, POST )				KFLOATINITDSS( SRC0,SRC0,SRC0,COUNT,PRE,POST )
#define KFLOATINITD(   DST, COUNT, PRE, POST )				KFLOATINITDSS( DST,DST,DST,COUNT,PRE,POST )
#define KFLOATINITDS(  DST, SRC0, COUNT, PRE, POST )		KFLOATINITDSS( DST,SRC0,SRC0,COUNT,PRE,POST )

#define KFLOATINITDSS( DST, SRC0, SRC1, COUNT, PRE, POST )\
	__asm	mov		ecx,DST								\
	__asm	shr		ecx,2								\
	__asm	mov		ebx,COUNT							\
	__asm	neg		ecx									\
	__asm	mov		edx,SRC0							\
	__asm	and		ecx,3								\
	__asm	mov		esi,SRC1							\
	__asm	sub		ebx,ecx								\
	__asm	jge		noUnderFlow							\
	__asm	xor		ebx,ebx								\
	__asm	mov		ecx,COUNT							\
	__asm	noUnderFlow:								\
	__asm	mov		PRE,ecx								\
	__asm	mov		eax,ebx								\
	__asm	mov		edi,DST								\
	__asm	and		eax,8-1								\
	__asm	mov		POST,eax							\
	__asm	and		ebx,0xfffffff8						\
	__asm	jle		done								\
	__asm	shl		ebx,2								\
	__asm	lea		ecx,[ecx*4+ebx]						\
	__asm	neg		ebx									\
	__asm	add		edx,ecx								\
	__asm	add		esi,ecx								\
	__asm	add		edi,ecx								\
	__asm	mov		eax,edx								\
	__asm	or		eax,esi

// without alignment (pre==0)
#define KFLOATINITS_NA(   SRC0, COUNT, PRE, POST )				KFLOATINITDSS_NA( SRC0,SRC0,SRC0,COUNT,PRE,POST )
#define KFLOATINITD_NA(   DST, COUNT, PRE, POST )				KFLOATINITDSS_NA( DST,DST,DST,COUNT,PRE,POST )
#define KFLOATINITDS_NA(  DST, SRC0, COUNT, PRE, POST )			KFLOATINITDSS_NA( DST,SRC0,SRC0,COUNT,PRE,POST )
#define KFLOATINITDSS_NA( DST, SRC0, SRC1, COUNT, PRE, POST )\
	__asm	mov		eax,COUNT							\
	__asm	mov		PRE,0								\
	__asm	and		eax,8-1								\
	__asm	mov		ebx,COUNT							\
	__asm	mov		POST,eax							\
	__asm	and		ebx,0xfffffff8						\
	__asm	je		done								\
	__asm	shl		ebx,2								\
	__asm	mov		edx,SRC0							\
	__asm	mov		esi,SRC1							\
	__asm	mov		edi,DST								\
	__asm	add		edx,ebx								\
	__asm	add		esi,ebx								\
	__asm	add		edi,ebx								\
	__asm	mov		eax,edx								\
	__asm	or		eax,esi								\
	__asm	or		eax,edi								\
	__asm	neg		ebx									\

/*
	when OPER is called:
	edx = s0
	esi	= s1
	edi	= d
	ebx	= index*4

	xmm0 & xmm1	must not be trashed
*/
#define KMOVDS1( DST, SRC0 )							\
	__asm	movss	xmm2,SRC0							\
	__asm	movss	DST,xmm2
#define KMOVDS4( DST, SRC0 )							\
	__asm	movups	xmm2,SRC0							\
	__asm	movups	DST,xmm2
#define KMINDS1( DST, SRC0 )							\
	__asm	movss	xmm2,SRC0							\
	__asm	minss	DST,xmm2
#define KMAXDS1( DST, SRC0 )							\
	__asm	movss	xmm2,SRC0							\
	__asm	maxss	DST,xmm2

// general ALU operation
#define KALUDSS1( OP, DST, SRC0, SRC1 )					\
	__asm	movss	xmm2,SRC0							\
	__asm	OP##ss	xmm2,SRC1							\
	__asm	movss	DST,xmm2
#define KALUDSS4( OP, DST, SRC0, SRC1 )					\
	__asm	movups	xmm2,SRC0							\
	__asm	movups	xmm3,SRC1							\
	__asm	OP##ps	xmm2,xmm3							\
	__asm	movups	DST,xmm2

#define KADDDSS1( DST, SRC0, SRC1 )		KALUDSS1( add, DST,SRC0,SRC1 )
#define KADDDSS4( DST, SRC0, SRC1 )		KALUDSS4( add, DST,SRC0,SRC1 )
#define KSUBDSS1( DST, SRC0, SRC1 )		KALUDSS1( sub, DST,SRC0,SRC1 )
#define KSUBDSS4( DST, SRC0, SRC1 )		KALUDSS4( sub, DST,SRC0,SRC1 )
#define KMULDSS1( DST, SRC0, SRC1 )		KALUDSS1( mul, DST,SRC0,SRC1 )
#define KMULDSS4( DST, SRC0, SRC1 )		KALUDSS4( mul, DST,SRC0,SRC1 )

#define KDIVDSS1( DST, SRC0, SRC1 )						\
	__asm	movss	xmm2,SRC1							\
	__asm	rcpss	xmm3,xmm2							\
	__asm	mulss	xmm2,xmm3							\
	__asm	mulss	xmm2,xmm3							\
	__asm	addss	xmm3,xmm3							\
	__asm	subss	xmm3,xmm2							\
	__asm	mulss	xmm3,SRC0							\
	__asm	movss	DST,xmm3
#define KDIVDSS4( DST, SRC0, SRC1 )						\
	__asm	movups	xmm2,SRC1							\
	__asm	rcpps	xmm3,xmm2							\
	__asm	mulps	xmm2,xmm3							\
	__asm	mulps	xmm2,xmm3							\
	__asm	addps	xmm3,xmm3							\
	__asm	subps	xmm3,xmm2							\
	__asm	movups	xmm2,SRC0							\
	__asm	mulps	xmm3,xmm2							\
	__asm	movups	DST,xmm3
#define	KF2IDS1( SRC0 )									\
	__asm	movss		xmm2,SRC0						\
	__asm	cvttps2pi	mm2,xmm2						\
	__asm	movd		[edi+ebx],mm2
#define	KF2IDS4( SRC0 )									\
	__asm	movups		xmm2,SRC0						\
	__asm	cvttps2pi	mm2,xmm2						\
	__asm	movq		[edi+ebx+0],mm2					\
	__asm	shufps		xmm2,xmm2,SHUFFLEPS(1,0,3,2)	\
	__asm	cvttps2pi	mm2,xmm2						\
	__asm	movq		[edi+ebx+8],mm2
#define	KISQRTDS1( DST,SRC0 )							\
	__asm	movss	xmm2,SRC0							\
	__asm	rsqrtss	xmm3,xmm2							\
	__asm	mulss	xmm2,xmm3							\
	__asm	mulss	xmm2,xmm3							\
	__asm	subss	xmm2,xmm1							\
	__asm	mulss	xmm3,xmm0							\
	__asm	mulss	xmm3,xmm2							\
	__asm	movss	DST,xmm3
#define	KISQRTDS4( DST,SRC0 )							\
	__asm	movups	xmm2,SRC0							\
	__asm	rsqrtps	xmm3,xmm2							\
	__asm	mulps	xmm2,xmm3							\
	__asm	mulps	xmm2,xmm3							\
	__asm	subps	xmm2,xmm1							\
	__asm	mulps	xmm3,xmm0							\
	__asm	mulps	xmm3,xmm2							\
	__asm	movups	DST,xmm3

// this is used in vector4 implementation to shift constant V4
#define KANDREGDSV( DST, SRC0, VALUE )					\
	__asm	mov		DST,SRC0							\
	__asm	and		DST,VALUE

// this is used in vector4 code to operate with float arrays as sources
#define KEXPANDFLOAT( DST, SRC )						\
	__asm	movss	DST,SRC								\
	__asm	shufps  DST,DST,0

#define	KADDDS1( DST,SRC )		KADDDSS1( DST,DST,SRC )
#define	KADDDS4( DST,SRC )		KADDDSS4( DST,DST,SRC )
#define	KSUBDS1( DST,SRC )		KSUBDSS1( DST,DST,SRC )
#define	KSUBDS4( DST,SRC )		KSUBDSS4( DST,DST,SRC )
#define	KMULDS1( DST,SRC )		KMULDSS1( DST,DST,SRC )
#define	KMULDS4( DST,SRC )		KMULDSS4( DST,DST,SRC )
#define	KDIVDS1( DST,SRC )		KDIVDSS1( DST,DST,SRC )
#define	KDIVDS4( DST,SRC )		KDIVDSS4( DST,DST,SRC )

// handles pre & post leftovers
#define	KFLOATOPER( OPER, OPER4, COUNT )				\
	__asm		mov		ecx,pre							\
	__asm		mov		ebx,COUNT						\
	__asm		cmp		ebx,ecx							\
	__asm		cmovl	ecx,COUNT						\
	__asm		test	ecx,ecx							\
	__asm		je		preDone							\
	__asm		xor		ebx,ebx							\
	__asm	lpPre:										\
				OPER									\
	__asm		add		ebx,4							\
	__asm		dec		ecx								\
	__asm		jg		lpPre							\
	__asm	preDone:									\
	__asm		mov		ecx,post						\
	__asm		mov		ebx,COUNT						\
	__asm		sub		ebx,ecx							\
	__asm		shl		ebx,2							\
	__asm		cmp		ecx,4							\
	__asm		jl		post4Done						\
				OPER4									\
	__asm		sub		ecx,4							\
	__asm		add		ebx,4*4							\
	__asm	post4Done:									\
	__asm		test	ecx,ecx							\
	__asm		je		postDone						\
	__asm	lpPost:										\
				OPER									\
	__asm		add		ebx,4							\
	__asm		dec		ecx								\
	__asm		jg		lpPost							\
	__asm	postDone:

// operate on a constant and a float array
#define KFLOAT_CA( ALUOP, DST, SRC, CONSTANT, COUNT )	\
	int	pre,post;										\
	__asm		movss	xmm0,CONSTANT					\
	__asm		shufps	xmm0,xmm0,0						\
			KFLOATINITDS( DST, SRC, COUNT, pre, post )	\
	__asm		and		eax,15							\
	__asm		jne		lpNA							\
	__asm		jmp		lpA								\
	__asm		align	16								\
	__asm	lpA:										\
	__asm		prefetchnta	[edx+ebx+64]				\
	__asm		movaps	xmm1,xmm0						\
	__asm		movaps	xmm2,xmm0						\
	__asm		ALUOP##ps	xmm1,[edx+ebx]				\
	__asm		ALUOP##ps	xmm2,[edx+ebx+16]			\
	__asm		movaps	[edi+ebx],xmm1					\
	__asm		movaps	[edi+ebx+16],xmm2				\
	__asm		add		ebx,16*2						\
	__asm		jl		lpA								\
	__asm		jmp		done							\
	__asm		align	16								\
	__asm	lpNA:										\
	__asm		prefetchnta	[edx+ebx+64]				\
	__asm		movaps	xmm1,xmm0						\
	__asm		movaps	xmm2,xmm0						\
	__asm		movups	xmm3,[edx+ebx]					\
	__asm		movups	xmm4,[edx+ebx+16]				\
	__asm		ALUOP##ps	xmm1,xmm3					\
	__asm		ALUOP##ps	xmm2,xmm4					\
	__asm		movaps	[edi+ebx],xmm1					\
	__asm		movaps	[edi+ebx+16],xmm2				\
	__asm		add		ebx,16*2						\
	__asm		jl		lpNA							\
	__asm	done:										\
	__asm		mov		edx,SRC							\
	__asm		mov		edi,DST							\
	__asm		KFLOATOPER( KALUDSS1( ALUOP, [edi+ebx],xmm0,[edx+ebx] ),	\
	__asm					KALUDSS4( ALUOP, [edi+ebx],xmm0,[edx+ebx] ), COUNT )

// operate on two float arrays
#define KFLOAT_AA( ALUOP, DST, SRC0, SRC1, COUNT )		\
	int	pre,post;										\
	KFLOATINITDSS( DST, SRC0, SRC1, COUNT, pre, post )	\
	__asm		and		eax,15							\
	__asm		jne		lpNA							\
	__asm		jmp		lpA								\
	__asm		align	16								\
	__asm	lpA:										\
	__asm		movaps	xmm1,[edx+ebx]					\
	__asm		movaps	xmm2,[edx+ebx+16]				\
	__asm		ALUOP##ps	xmm1,[esi+ebx]				\
	__asm		ALUOP##ps	xmm2,[esi+ebx+16]			\
	__asm		prefetchnta	[edx+ebx+64]				\
	__asm		prefetchnta	[esi+ebx+64]				\
	__asm		movaps	[edi+ebx],xmm1					\
	__asm		movaps	[edi+ebx+16],xmm2				\
	__asm		add		ebx,16*2						\
	__asm		jl		lpA								\
	__asm		jmp		done							\
	__asm		align	16								\
	__asm	lpNA:										\
	__asm		movups	xmm1,[edx+ebx]					\
	__asm		movups	xmm2,[edx+ebx+16]				\
	__asm		movups	xmm3,[esi+ebx]					\
	__asm		movups	xmm4,[esi+ebx+16]				\
	__asm		prefetchnta	[edx+ebx+64]				\
	__asm		prefetchnta	[esi+ebx+64]				\
	__asm		ALUOP##ps	xmm1,xmm3					\
	__asm		ALUOP##ps	xmm2,xmm4					\
	__asm		movaps	[edi+ebx],xmm1					\
	__asm		movaps	[edi+ebx+16],xmm2				\
	__asm		add		ebx,16*2						\
	__asm		jl		lpNA							\
	__asm	done:										\
	__asm		mov		edx,SRC0						\
	__asm		mov		esi,SRC1						\
	__asm		mov		edi,DST							\
	KFLOATOPER( KALUDSS1( ALUOP, [edi+ebx],[edx+ebx],[esi+ebx] ),		\
				KALUDSS4( ALUOP, [edi+ebx],[edx+ebx],[esi+ebx] ), COUNT )


#define DRAWVERT_SIZE				60

#define JOINTQUAT_SIZE				(7*4)
#define JOINTMAT_SIZE				(4*3*4)
#define JOINTWEIGHT_SIZE			(4*4)


#define ALIGN4_INIT1( X, INIT )				ALIGN16( static X[4] ) = { INIT, INIT, INIT, INIT }
#define ALIGN4_INIT4( X, I0, I1, I2, I3 )	ALIGN16( static X[4] ) = { I0, I1, I2, I3 }
#define ALIGN8_INIT1( X, INIT )				ALIGN16( static X[8] ) = { INIT, INIT, INIT, INIT, INIT, INIT, INIT, INIT }

ALIGN8_INIT1( unsigned short SIMD_W_zero, 0 );
ALIGN8_INIT1( unsigned short SIMD_W_maxShort, 1<<15 );

ALIGN4_INIT1( unsigned int SIMD_DW_mat2quatShuffle0, (3<<0)|(2<<8)|(1<<16)|(0<<24) );
ALIGN4_INIT1( unsigned int SIMD_DW_mat2quatShuffle1, (0<<0)|(1<<8)|(2<<16)|(3<<24) );
ALIGN4_INIT1( unsigned int SIMD_DW_mat2quatShuffle2, (1<<0)|(0<<8)|(3<<16)|(2<<24) );
ALIGN4_INIT1( unsigned int SIMD_DW_mat2quatShuffle3, (2<<0)|(3<<8)|(0<<16)|(1<<24) );

ALIGN4_INIT4( unsigned int SIMD_SP_singleSignBitMask, (unsigned int) ( 1 << 31 ), 0, 0, 0 );
ALIGN4_INIT1( unsigned int SIMD_SP_signBitMask, (unsigned int) ( 1 << 31 ) );
ALIGN4_INIT1( unsigned int SIMD_SP_absMask, (unsigned int) ~( 1 << 31 ) );
ALIGN4_INIT1( unsigned int SIMD_SP_infinityMask, (unsigned int) ~( 1 << 23 ) );
ALIGN4_INIT1( unsigned int SIMD_SP_not, 0xFFFFFFFF );

ALIGN4_INIT1( float SIMD_SP_zero, 0.0f );
ALIGN4_INIT1( float SIMD_SP_half, 0.5f );
ALIGN4_INIT1( float SIMD_SP_one, 1.0f );
ALIGN4_INIT1( float SIMD_SP_two, 2.0f );
ALIGN4_INIT1( float SIMD_SP_three, 3.0f );
ALIGN4_INIT1( float SIMD_SP_four, 4.0f );
ALIGN4_INIT1( float SIMD_SP_maxShort, (1<<15) );
ALIGN4_INIT1( float SIMD_SP_tiny, 1e-10f );
ALIGN4_INIT1( float SIMD_SP_PI, idMath::PI );
ALIGN4_INIT1( float SIMD_SP_halfPI, idMath::HALF_PI );
ALIGN4_INIT1( float SIMD_SP_twoPI, idMath::TWO_PI );
ALIGN4_INIT1( float SIMD_SP_oneOverTwoPI, 1.0f / idMath::TWO_PI );
ALIGN4_INIT1( float SIMD_SP_infinity, idMath::INFINITY );
ALIGN4_INIT4( float SIMD_SP_lastOne, 0.0f, 0.0f, 0.0f, 1.0f );

ALIGN4_INIT1( float SIMD_SP_rsqrt_c0,  3.0f );
ALIGN4_INIT1( float SIMD_SP_rsqrt_c1, -0.5f );
ALIGN4_INIT1( float SIMD_SP_mat2quat_rsqrt_c1, -0.5f*0.5f );

ALIGN4_INIT1( float SIMD_SP_sin_c0, -2.39e-08f );
ALIGN4_INIT1( float SIMD_SP_sin_c1,  2.7526e-06f );
ALIGN4_INIT1( float SIMD_SP_sin_c2, -1.98409e-04f );
ALIGN4_INIT1( float SIMD_SP_sin_c3,  8.3333315e-03f );
ALIGN4_INIT1( float SIMD_SP_sin_c4, -1.666666664e-01f );

ALIGN4_INIT1( float SIMD_SP_cos_c0, -2.605e-07f );
ALIGN4_INIT1( float SIMD_SP_cos_c1,  2.47609e-05f );
ALIGN4_INIT1( float SIMD_SP_cos_c2, -1.3888397e-03f );
ALIGN4_INIT1( float SIMD_SP_cos_c3,  4.16666418e-02f );
ALIGN4_INIT1( float SIMD_SP_cos_c4, -4.999999963e-01f );

ALIGN4_INIT1( float SIMD_SP_atan_c0,  0.0028662257f );
ALIGN4_INIT1( float SIMD_SP_atan_c1, -0.0161657367f );
ALIGN4_INIT1( float SIMD_SP_atan_c2,  0.0429096138f );
ALIGN4_INIT1( float SIMD_SP_atan_c3, -0.0752896400f );
ALIGN4_INIT1( float SIMD_SP_atan_c4,  0.1065626393f );
ALIGN4_INIT1( float SIMD_SP_atan_c5, -0.1420889944f );
ALIGN4_INIT1( float SIMD_SP_atan_c6,  0.1999355085f );
ALIGN4_INIT1( float SIMD_SP_atan_c7, -0.3333314528f );

/*
============
SSE_InvSqrt
============
*/
float SSE_InvSqrt( float x ) {
	float y;

	__asm {
		movss		xmm0, x
		rsqrtss		xmm1, xmm0
		mulss		xmm0, xmm1
		mulss		xmm0, xmm1
		subss		xmm0, SIMD_SP_rsqrt_c0
		mulss		xmm1, SIMD_SP_rsqrt_c1
		mulss		xmm0, xmm1
		movss		y, xmm0
	}
	return y;
}

/*
============
SSE_InvSqrt4
============
*/
void SSE_InvSqrt4( float x[4] ) {
	__asm {
		mov			edi, x
		movaps		xmm0, [edi]
		rsqrtps		xmm1, xmm0
		mulps		xmm0, xmm1
		mulps		xmm0, xmm1
		subps		xmm0, SIMD_SP_rsqrt_c0
		mulps		xmm1, SIMD_SP_rsqrt_c1
		mulps		xmm0, xmm1
		movaps		[edi], xmm0
	}
}

/*
============
SSE_SinZeroHalfPI

  The angle must be between zero and half PI.
============
*/
float SSE_SinZeroHalfPI( float a ) {
#if 1

	float t;

	assert( a >= 0.0f && a <= idMath::HALF_PI );

	__asm {
		movss		xmm0, a
		movss		xmm1, xmm0
		mulss		xmm1, xmm1
		movss		xmm2, SIMD_SP_sin_c0
		mulss		xmm2, xmm1
		addss		xmm2, SIMD_SP_sin_c1
		mulss		xmm2, xmm1
		addss		xmm2, SIMD_SP_sin_c2
		mulss		xmm2, xmm1
		addss		xmm2, SIMD_SP_sin_c3
		mulss		xmm2, xmm1
		addss		xmm2, SIMD_SP_sin_c4
		mulss		xmm2, xmm1
		addss		xmm2, SIMD_SP_one
		mulss		xmm2, xmm0
		movss		t, xmm2
	}

	return t;

#else

	float s, t;

	assert( a >= 0.0f && a <= idMath::HALF_PI );

	s = a * a;
	t = -2.39e-08f;
	t *= s;
	t += 2.7526e-06f;
	t *= s;
	t += -1.98409e-04f;
	t *= s;
	t += 8.3333315e-03f;
	t *= s;
	t += -1.666666664e-01f;
	t *= s;
	t += 1.0f;
	t *= a;

	return t;

#endif
}

/*
============
SSE_Sin4ZeroHalfPI

  The angle must be between zero and half PI.
============
*/
void SSE_Sin4ZeroHalfPI( float a[4], float s[4] ) {
	__asm {
		mov			edi, a
		mov			esi, s
		movaps		xmm0, [edi]
		movaps		xmm1, xmm0
		mulps		xmm1, xmm1
		movaps		xmm2, SIMD_SP_sin_c0
		mulps		xmm2, xmm1
		addps		xmm2, SIMD_SP_sin_c1
		mulps		xmm2, xmm1
		addps		xmm2, SIMD_SP_sin_c2
		mulps		xmm2, xmm1
		addps		xmm2, SIMD_SP_sin_c3
		mulps		xmm2, xmm1
		addps		xmm2, SIMD_SP_sin_c4
		mulps		xmm2, xmm1
		addps		xmm2, SIMD_SP_one
		mulps		xmm2, xmm0
		movaps		[esi], xmm2
	}
}

/*
============
SSE_Sin
============
*/
float SSE_Sin( float a ) {
#if 1

	float t;

	__asm {
		movss		xmm1, a
		movss		xmm2, xmm1
		movss		xmm3, xmm1
		mulss		xmm2, SIMD_SP_oneOverTwoPI
		cvttss2si	ecx, xmm2
		cmpltss		xmm3, SIMD_SP_zero
		andps		xmm3, SIMD_SP_one
		cvtsi2ss	xmm2, ecx
		subss		xmm2, xmm3
		mulss		xmm2, SIMD_SP_twoPI
		subss		xmm1, xmm2

		movss		xmm0, SIMD_SP_PI			// xmm0 = PI
		subss		xmm0, xmm1					// xmm0 = PI - a
		movss		xmm1, xmm0					// xmm1 = PI - a
		andps		xmm1, SIMD_SP_signBitMask	// xmm1 = signbit( PI - a )
		movss		xmm2, xmm0					// xmm2 = PI - a
		xorps		xmm2, xmm1					// xmm2 = fabs( PI - a )
		cmpnltss	xmm2, SIMD_SP_halfPI		// xmm2 = ( fabs( PI - a ) >= idMath::HALF_PI ) ? 0xFFFFFFFF : 0x00000000
		movss		xmm3, SIMD_SP_PI			// xmm3 = PI
		xorps		xmm3, xmm1					// xmm3 = PI ^ signbit( PI - a )
		andps		xmm3, xmm2					// xmm3 = ( fabs( PI - a ) >= idMath::HALF_PI ) ? ( PI ^ signbit( PI - a ) ) : 0.0f
		andps		xmm2, SIMD_SP_signBitMask	// xmm2 = ( fabs( PI - a ) >= idMath::HALF_PI ) ? SIMD_SP_signBitMask : 0.0f
		xorps		xmm0, xmm2
		addps		xmm0, xmm3

		movss		xmm1, xmm0
		mulss		xmm1, xmm1
		movss		xmm2, SIMD_SP_sin_c0
		mulss		xmm2, xmm1
		addss		xmm2, SIMD_SP_sin_c1
		mulss		xmm2, xmm1
		addss		xmm2, SIMD_SP_sin_c2
		mulss		xmm2, xmm1
		addss		xmm2, SIMD_SP_sin_c3
		mulss		xmm2, xmm1
		addss		xmm2, SIMD_SP_sin_c4
		mulss		xmm2, xmm1
		addss		xmm2, SIMD_SP_one
		mulss		xmm2, xmm0
		movss		t, xmm2
	}

	return t;

#else

	float s, t;

	if ( ( a < 0.0f ) || ( a >= idMath::TWO_PI ) ) {
		a -= floorf( a / idMath::TWO_PI ) * idMath::TWO_PI;
	}

	a = idMath::PI - a;
	if ( fabs( a ) >= idMath::HALF_PI ) {
		a = ( ( a < 0.0f ) ? -idMath::PI : idMath::PI ) - a;
	}

	s = a * a;
	t = -2.39e-08f;
	t *= s;
	t += 2.7526e-06f;
	t *= s;
	t += -1.98409e-04f;
	t *= s;
	t += 8.3333315e-03f;
	t *= s;
	t += -1.666666664e-01f;
	t *= s;
	t += 1.0f;
	t *= a;

	return t;

#endif
}

/*
============
SSE_Sin4
============
*/
void SSE_Sin4( float a[4], float s[4] ) {
	__asm {
		mov			edi, a
		mov			esi, s
		movaps		xmm1, [edi]
		movaps		xmm2, xmm1
		mulps		xmm2, SIMD_SP_oneOverTwoPI
		movhlps		xmm3, xmm2
		cvttss2si	ecx, xmm2
		cvtsi2ss	xmm2, ecx
		cvttss2si	edx, xmm3
		cvtsi2ss	xmm3, edx
		shufps		xmm2, xmm2, R_SHUFFLEPS( 1, 0, 0, 0 )
		shufps		xmm3, xmm3, R_SHUFFLEPS( 1, 0, 0, 0 )
		cvttss2si	ecx, xmm2
		cvtsi2ss	xmm2, ecx
		cvttss2si	edx, xmm3
		cvtsi2ss	xmm3, edx
		shufps		xmm2, xmm3, R_SHUFFLEPS( 1, 0, 1, 0 )
		movaps		xmm3, xmm1
		cmpltps		xmm3, SIMD_SP_zero
		andps		xmm3, SIMD_SP_one
		subps		xmm2, xmm3
		mulps		xmm2, SIMD_SP_twoPI
		subps		xmm1, xmm2

		movaps		xmm0, SIMD_SP_PI			// xmm0 = PI
		subps		xmm0, xmm1					// xmm0 = PI - a
		movaps		xmm1, xmm0					// xmm1 = PI - a
		andps		xmm1, SIMD_SP_signBitMask	// xmm1 = signbit( PI - a )
		movaps		xmm2, xmm0					// xmm2 = PI - a
		xorps		xmm2, xmm1					// xmm2 = fabs( PI - a )
		cmpnltps	xmm2, SIMD_SP_halfPI		// xmm2 = ( fabs( PI - a ) >= idMath::HALF_PI ) ? 0xFFFFFFFF : 0x00000000
		movaps		xmm3, SIMD_SP_PI			// xmm3 = PI
		xorps		xmm3, xmm1					// xmm3 = PI ^ signbit( PI - a )
		andps		xmm3, xmm2					// xmm3 = ( fabs( PI - a ) >= idMath::HALF_PI ) ? ( PI ^ signbit( PI - a ) ) : 0.0f
		andps		xmm2, SIMD_SP_signBitMask	// xmm2 = ( fabs( PI - a ) >= idMath::HALF_PI ) ? SIMD_SP_signBitMask : 0.0f
		xorps		xmm0, xmm2
		addps		xmm0, xmm3

		movaps		xmm1, xmm0
		mulps		xmm1, xmm1
		movaps		xmm2, SIMD_SP_sin_c0
		mulps		xmm2, xmm1
		addps		xmm2, SIMD_SP_sin_c1
		mulps		xmm2, xmm1
		addps		xmm2, SIMD_SP_sin_c2
		mulps		xmm2, xmm1
		addps		xmm2, SIMD_SP_sin_c3
		mulps		xmm2, xmm1
		addps		xmm2, SIMD_SP_sin_c4
		mulps		xmm2, xmm1
		addps		xmm2, SIMD_SP_one
		mulps		xmm2, xmm0
		movaps		[esi], xmm2
	}
}

/*
============
SSE_CosZeroHalfPI

  The angle must be between zero and half PI.
============
*/
float SSE_CosZeroHalfPI( float a ) {
#if 1

	float t;

	assert( a >= 0.0f && a <= idMath::HALF_PI );

	__asm {
		movss		xmm0, a
		mulss		xmm0, xmm0
		movss		xmm1, SIMD_SP_cos_c0
		mulss		xmm1, xmm0
		addss		xmm1, SIMD_SP_cos_c1
		mulss		xmm1, xmm0
		addss		xmm1, SIMD_SP_cos_c2
		mulss		xmm1, xmm0
		addss		xmm1, SIMD_SP_cos_c3
		mulss		xmm1, xmm0
		addss		xmm1, SIMD_SP_cos_c4
		mulss		xmm1, xmm0
		addss		xmm1, SIMD_SP_one
		movss		t, xmm1
	}

	return t;

#else

	float s, t;

	assert( a >= 0.0f && a <= idMath::HALF_PI );

	s = a * a;
	t = -2.605e-07f;
	t *= s;
	t += 2.47609e-05f;
	t *= s;
	t += -1.3888397e-03f;
	t *= s;
	t += 4.16666418e-02f;
	t *= s;
	t += -4.999999963e-01f;
	t *= s;
	t += 1.0f;

	return t;

#endif
}

/*
============
SSE_Cos4ZeroHalfPI

  The angle must be between zero and half PI.
============
*/
void SSE_Cos4ZeroHalfPI( float a[4], float c[4] ) {
	__asm {
		mov			edi, a
		mov			esi, c
		movaps		xmm0, [edi]
		mulps		xmm0, xmm0
		movaps		xmm1, SIMD_SP_cos_c0
		mulps		xmm1, xmm0
		addps		xmm1, SIMD_SP_cos_c1
		mulps		xmm1, xmm0
		addps		xmm1, SIMD_SP_cos_c2
		mulps		xmm1, xmm0
		addps		xmm1, SIMD_SP_cos_c3
		mulps		xmm1, xmm0
		addps		xmm1, SIMD_SP_cos_c4
		mulps		xmm1, xmm0
		addps		xmm1, SIMD_SP_one
		movaps		[esi], xmm2
	}
}

/*
============
SSE_Cos
============
*/
float SSE_Cos( float a ) {
#if 1

	float t;

	__asm {
		movss		xmm1, a
		movss		xmm2, xmm1
		movss		xmm3, xmm1
		mulss		xmm2, SIMD_SP_oneOverTwoPI
		cvttss2si	ecx, xmm2
		cmpltss		xmm3, SIMD_SP_zero
		andps		xmm3, SIMD_SP_one
		cvtsi2ss	xmm2, ecx
		subss		xmm2, xmm3
		mulss		xmm2, SIMD_SP_twoPI
		subss		xmm1, xmm2

		movss		xmm0, SIMD_SP_PI			// xmm0 = PI
		subss		xmm0, xmm1					// xmm0 = PI - a
		movss		xmm1, xmm0					// xmm1 = PI - a
		andps		xmm1, SIMD_SP_signBitMask	// xmm1 = signbit( PI - a )
		movss		xmm2, xmm0					// xmm2 = PI - a
		xorps		xmm2, xmm1					// xmm2 = fabs( PI - a )
		cmpnltss	xmm2, SIMD_SP_halfPI		// xmm2 = ( fabs( PI - a ) >= idMath::HALF_PI ) ? 0xFFFFFFFF : 0x00000000
		movss		xmm3, SIMD_SP_PI			// xmm3 = PI
		xorps		xmm3, xmm1					// xmm3 = PI ^ signbit( PI - a )
		andps		xmm3, xmm2					// xmm3 = ( fabs( PI - a ) >= idMath::HALF_PI ) ? ( PI ^ signbit( PI - a ) ) : 0.0f
		andps		xmm2, SIMD_SP_signBitMask	// xmm2 = ( fabs( PI - a ) >= idMath::HALF_PI ) ? SIMD_SP_signBitMask : 0.0f
		xorps		xmm0, xmm2
		addps		xmm0, xmm3

		mulss		xmm0, xmm0
		movss		xmm1, SIMD_SP_cos_c0
		mulss		xmm1, xmm0
		addss		xmm1, SIMD_SP_cos_c1
		mulss		xmm1, xmm0
		addss		xmm1, SIMD_SP_cos_c2
		mulss		xmm1, xmm0
		addss		xmm1, SIMD_SP_cos_c3
		mulss		xmm1, xmm0
		addss		xmm1, SIMD_SP_cos_c4
		mulss		xmm1, xmm0
		addss		xmm1, SIMD_SP_one
		xorps		xmm2, SIMD_SP_signBitMask
		xorps		xmm1, xmm2
		movss		t, xmm1
	}

	return t;

#else

	float s, t;

	if ( ( a < 0.0f ) || ( a >= idMath::TWO_PI ) ) {
		a -= floorf( a / idMath::TWO_PI ) * idMath::TWO_PI;
	}

	a = idMath::PI - a;
	if ( fabs( a ) >= idMath::HALF_PI ) {
		a = ( ( a < 0.0f ) ? -idMath::PI : idMath::PI ) - a;
		d = 1.0f;
	} else {
		d = -1.0f;
	}

	s = a * a;
	t = -2.605e-07f;
	t *= s;
	t += 2.47609e-05f;
	t *= s;
	t += -1.3888397e-03f;
	t *= s;
	t += 4.16666418e-02f;
	t *= s;
	t += -4.999999963e-01f;
	t *= s;
	t += 1.0f;
	t *= d;

	return t;

#endif
}

/*
============
SSE_Cos4
============
*/
void SSE_Cos4( float a[4], float c[4] ) {
	__asm {
		mov			edi, a
		mov			esi, c
		movaps		xmm1, [edi]
		movaps		xmm2, xmm1
		mulps		xmm2, SIMD_SP_oneOverTwoPI
		movhlps		xmm3, xmm2
		cvttss2si	ecx, xmm2
		cvtsi2ss	xmm2, ecx
		cvttss2si	edx, xmm3
		cvtsi2ss	xmm3, edx
		shufps		xmm2, xmm2, R_SHUFFLEPS( 1, 0, 0, 0 )
		shufps		xmm3, xmm3, R_SHUFFLEPS( 1, 0, 0, 0 )
		cvttss2si	ecx, xmm2
		cvtsi2ss	xmm2, ecx
		cvttss2si	edx, xmm3
		cvtsi2ss	xmm3, edx
		shufps		xmm2, xmm3, R_SHUFFLEPS( 1, 0, 1, 0 )
		movaps		xmm3, xmm1
		cmpltps		xmm3, SIMD_SP_zero
		andps		xmm3, SIMD_SP_one
		subps		xmm2, xmm3
		mulps		xmm2, SIMD_SP_twoPI
		subps		xmm1, xmm2

		movaps		xmm0, SIMD_SP_PI			// xmm0 = PI
		subps		xmm0, xmm1					// xmm0 = PI - a
		movaps		xmm1, xmm0					// xmm1 = PI - a
		andps		xmm1, SIMD_SP_signBitMask	// xmm1 = signbit( PI - a )
		movaps		xmm2, xmm0					// xmm2 = PI - a
		xorps		xmm2, xmm1					// xmm2 = fabs( PI - a )
		cmpnltps	xmm2, SIMD_SP_halfPI		// xmm2 = ( fabs( PI - a ) >= idMath::HALF_PI ) ? 0xFFFFFFFF : 0x00000000
		movaps		xmm3, SIMD_SP_PI			// xmm3 = PI
		xorps		xmm3, xmm1					// xmm3 = PI ^ signbit( PI - a )
		andps		xmm3, xmm2					// xmm3 = ( fabs( PI - a ) >= idMath::HALF_PI ) ? ( PI ^ signbit( PI - a ) ) : 0.0f
		andps		xmm2, SIMD_SP_signBitMask	// xmm2 = ( fabs( PI - a ) >= idMath::HALF_PI ) ? SIMD_SP_signBitMask : 0.0f
		xorps		xmm0, xmm2
		addps		xmm0, xmm3

		mulps		xmm0, xmm0
		movaps		xmm1, SIMD_SP_cos_c0
		mulps		xmm1, xmm0
		addps		xmm1, SIMD_SP_cos_c1
		mulps		xmm1, xmm0
		addps		xmm1, SIMD_SP_cos_c2
		mulps		xmm1, xmm0
		addps		xmm1, SIMD_SP_cos_c3
		mulps		xmm1, xmm0
		addps		xmm1, SIMD_SP_cos_c4
		mulps		xmm1, xmm0
		addps		xmm1, SIMD_SP_one
		xorps		xmm2, SIMD_SP_signBitMask
		xorps		xmm1, xmm2
		movaps		[esi], xmm1
	}
}

/*
============
SSE_SinCos
============
*/
void SSE_SinCos( float a, float &s, float &c ) {
	__asm {
		mov			edi, s
		mov			esi, c
		movss		xmm1, a
		movss		xmm2, xmm1
		movss		xmm3, xmm1
		mulss		xmm2, SIMD_SP_oneOverTwoPI
		cvttss2si	ecx, xmm2
		cmpltss		xmm3, SIMD_SP_zero
		andps		xmm3, SIMD_SP_one
		cvtsi2ss	xmm2, ecx
		subss		xmm2, xmm3
		mulss		xmm2, SIMD_SP_twoPI
		subss		xmm1, xmm2

		movss		xmm0, SIMD_SP_PI			// xmm0 = PI
		subss		xmm0, xmm1					// xmm0 = PI - a
		movss		xmm1, xmm0					// xmm1 = PI - a
		andps		xmm1, SIMD_SP_signBitMask	// xmm1 = signbit( PI - a )
		movss		xmm2, xmm0					// xmm2 = PI - a
		xorps		xmm2, xmm1					// xmm2 = fabs( PI - a )
		cmpnltss	xmm2, SIMD_SP_halfPI		// xmm2 = ( fabs( PI - a ) >= idMath::HALF_PI ) ? 0xFFFFFFFF : 0x00000000
		movss		xmm3, SIMD_SP_PI			// xmm3 = PI
		xorps		xmm3, xmm1					// xmm3 = PI ^ signbit( PI - a )
		andps		xmm3, xmm2					// xmm3 = ( fabs( PI - a ) >= idMath::HALF_PI ) ? ( PI ^ signbit( PI - a ) ) : 0.0f
		andps		xmm2, SIMD_SP_signBitMask	// xmm2 = ( fabs( PI - a ) >= idMath::HALF_PI ) ? SIMD_SP_signBitMask : 0.0f
		xorps		xmm0, xmm2
		addps		xmm0, xmm3

		movss		xmm1, xmm0
		mulss		xmm1, xmm1
		movss		xmm3, SIMD_SP_sin_c0
		movss		xmm4, SIMD_SP_cos_c0
		mulss		xmm3, xmm1
		mulss		xmm4, xmm1
		addss		xmm3, SIMD_SP_sin_c1
		addss		xmm4, SIMD_SP_cos_c1
		mulss		xmm3, xmm1
		mulss		xmm4, xmm1
		addss		xmm3, SIMD_SP_sin_c2
		addss		xmm4, SIMD_SP_cos_c2
		mulss		xmm3, xmm1
		mulss		xmm4, xmm1
		addss		xmm3, SIMD_SP_sin_c3
		addss		xmm4, SIMD_SP_cos_c3
		mulss		xmm3, xmm1
		mulss		xmm4, xmm1
		addss		xmm3, SIMD_SP_sin_c4
		addss		xmm4, SIMD_SP_cos_c4
		mulss		xmm3, xmm1
		mulss		xmm4, xmm1
		addss		xmm3, SIMD_SP_one
		addss		xmm4, SIMD_SP_one
		mulss		xmm3, xmm0
		xorps		xmm2, SIMD_SP_signBitMask
		xorps		xmm4, xmm2
		movss		[edi], xmm2
		movss		[esi], xmm3
	}
}

/*
============
SSE_SinCos4
============
*/
void SSE_SinCos4( float a[4], float s[4], float c[4] ) {
	__asm {
		mov			eax, a
		mov			edi, s
		mov			esi, c
		movaps		xmm1, [eax]
		movaps		xmm2, xmm1
		mulps		xmm2, SIMD_SP_oneOverTwoPI
		movhlps		xmm3, xmm2
		cvttss2si	ecx, xmm2
		cvtsi2ss	xmm2, ecx
		cvttss2si	edx, xmm3
		cvtsi2ss	xmm3, edx
		shufps		xmm2, xmm2, R_SHUFFLEPS( 1, 0, 0, 0 )
		shufps		xmm3, xmm3, R_SHUFFLEPS( 1, 0, 0, 0 )
		cvttss2si	ecx, xmm2
		cvtsi2ss	xmm2, ecx
		cvttss2si	edx, xmm3
		cvtsi2ss	xmm3, edx
		shufps		xmm2, xmm3, R_SHUFFLEPS( 1, 0, 1, 0 )
		movaps		xmm3, xmm1
		cmpltps		xmm3, SIMD_SP_zero
		andps		xmm3, SIMD_SP_one
		subps		xmm2, xmm3
		mulps		xmm2, SIMD_SP_twoPI
		subps		xmm1, xmm2

		movaps		xmm0, SIMD_SP_PI			// xmm0 = PI
		subps		xmm0, xmm1					// xmm0 = PI - a
		movaps		xmm1, xmm0					// xmm1 = PI - a
		andps		xmm1, SIMD_SP_signBitMask	// xmm1 = signbit( PI - a )
		movaps		xmm2, xmm0					// xmm2 = PI - a
		xorps		xmm2, xmm1					// xmm2 = fabs( PI - a )
		cmpnltps	xmm2, SIMD_SP_halfPI		// xmm2 = ( fabs( PI - a ) >= idMath::HALF_PI ) ? 0xFFFFFFFF : 0x00000000
		movaps		xmm3, SIMD_SP_PI			// xmm3 = PI
		xorps		xmm3, xmm1					// xmm3 = PI ^ signbit( PI - a )
		andps		xmm3, xmm2					// xmm3 = ( fabs( PI - a ) >= idMath::HALF_PI ) ? ( PI ^ signbit( PI - a ) ) : 0.0f
		andps		xmm2, SIMD_SP_signBitMask	// xmm2 = ( fabs( PI - a ) >= idMath::HALF_PI ) ? SIMD_SP_signBitMask : 0.0f
		xorps		xmm0, xmm2
		addps		xmm0, xmm3

		movaps		xmm0, [eax]
		movaps		xmm1, xmm0
		mulps		xmm1, xmm1
		movaps		xmm3, SIMD_SP_sin_c0
		movaps		xmm4, SIMD_SP_cos_c0
		mulps		xmm3, xmm1
		mulps		xmm4, xmm1
		addps		xmm3, SIMD_SP_sin_c1
		addps		xmm4, SIMD_SP_cos_c1
		mulps		xmm3, xmm1
		mulps		xmm4, xmm1
		addps		xmm3, SIMD_SP_sin_c2
		addps		xmm4, SIMD_SP_cos_c2
		mulps		xmm3, xmm1
		mulps		xmm4, xmm1
		addps		xmm3, SIMD_SP_sin_c3
		addps		xmm4, SIMD_SP_cos_c3
		mulps		xmm3, xmm1
		mulps		xmm4, xmm1
		addps		xmm3, SIMD_SP_sin_c4
		addps		xmm4, SIMD_SP_cos_c4
		mulps		xmm3, xmm1
		mulps		xmm4, xmm1
		addps		xmm3, SIMD_SP_one
		addps		xmm4, SIMD_SP_one
		mulps		xmm3, xmm0
		xorps		xmm2, SIMD_SP_signBitMask
		xorps		xmm4, xmm2
		movaps		[edi], xmm3
		movaps		[esi], xmm4
	}
}

/*
============
SSE_ATanPositive

  Both 'x' and 'y' must be positive.
============
*/
float SSE_ATanPositive( float y, float x ) {
#if 1

	float t;

	assert( y >= 0.0f && x >= 0.0f );

	__asm {
		movss		xmm0, x
		movss		xmm3, xmm0
		movss		xmm1, y
		minss		xmm0, xmm1
		maxss		xmm1, xmm3
		cmpeqss		xmm3, xmm0
		rcpss		xmm2, xmm1
		mulss		xmm1, xmm2
		mulss		xmm1, xmm2
		addss		xmm2, xmm2
		subss		xmm2, xmm1				// xmm2 = 1 / y or 1 / x
		mulss		xmm0, xmm2				// xmm0 = x / y or y / x
		movss		xmm1, xmm3
		andps		xmm1, SIMD_SP_signBitMask
		xorps		xmm0, xmm1				// xmm0 = -x / y or y / x
		andps		xmm3, SIMD_SP_halfPI	// xmm3 = HALF_PI or 0.0f
		movss		xmm1, xmm0
		mulss		xmm1, xmm1				// xmm1 = s
		movss		xmm2, SIMD_SP_atan_c0
		mulss		xmm2, xmm1
		addss		xmm2, SIMD_SP_atan_c1
		mulss		xmm2, xmm1
		addss		xmm2, SIMD_SP_atan_c2
		mulss		xmm2, xmm1
		addss		xmm2, SIMD_SP_atan_c3
		mulss		xmm2, xmm1
		addss		xmm2, SIMD_SP_atan_c4
		mulss		xmm2, xmm1
		addss		xmm2, SIMD_SP_atan_c5
		mulss		xmm2, xmm1
		addss		xmm2, SIMD_SP_atan_c6
		mulss		xmm2, xmm1
		addss		xmm2, SIMD_SP_atan_c7
		mulss		xmm2, xmm1
		addss		xmm2, SIMD_SP_one
		mulss		xmm2, xmm0
		addss		xmm2, xmm3
		movss		t, xmm2
	}

	return t;

#else

	float a, d, s, t;

	assert( y >= 0.0f && x >= 0.0f );

	if ( y > x ) {
		a = -x / y;
		d = idMath::HALF_PI;
	} else {
		a = y / x;
		d = 0.0f;
	}
	s = a * a;
	t = 0.0028662257f;
	t *= s;
	t += -0.0161657367f;
	t *= s;
	t += 0.0429096138f;
	t *= s;
	t += -0.0752896400f;
	t *= s;
	t += 0.1065626393f;
	t *= s;
	t += -0.1420889944f;
	t *= s;
	t += 0.1999355085f;
	t *= s;
	t += -0.3333314528f;
	t *= s;
	t += 1.0f;
	t *= a;
	t += d;

	return t;

#endif
}

/*
============
SSE_ATan4Positive

  Both 'x' and 'y' must be positive.
============
*/
void SSE_ATan4Positive( float y[4], float x[4], float at[4] ) {
	__asm {
		mov			esi, x
		mov			edi, y
		mov			edx, at
		movaps		xmm0, [esi]
		movaps		xmm3, xmm0
		movaps		xmm1, [edi]
		minps		xmm0, xmm1
		maxps		xmm1, xmm3
		cmpeqps		xmm3, xmm0
		rcpps		xmm2, xmm1
		mulps		xmm1, xmm2
		mulps		xmm1, xmm2
		addps		xmm2, xmm2
		subps		xmm2, xmm1				// xmm2 = 1 / y or 1 / x
		mulps		xmm0, xmm2				// xmm0 = x / y or y / x
		movaps		xmm1, xmm3
		andps		xmm1, SIMD_SP_signBitMask
		xorps		xmm0, xmm1				// xmm0 = -x / y or y / x
		andps		xmm3, SIMD_SP_halfPI	// xmm3 = HALF_PI or 0.0f
		movaps		xmm1, xmm0
		mulps		xmm1, xmm1				// xmm1 = s
		movaps		xmm2, SIMD_SP_atan_c0
		mulps		xmm2, xmm1
		addps		xmm2, SIMD_SP_atan_c1
		mulps		xmm2, xmm1
		addps		xmm2, SIMD_SP_atan_c2
		mulps		xmm2, xmm1
		addps		xmm2, SIMD_SP_atan_c3
		mulps		xmm2, xmm1
		addps		xmm2, SIMD_SP_atan_c4
		mulps		xmm2, xmm1
		addps		xmm2, SIMD_SP_atan_c5
		mulps		xmm2, xmm1
		addps		xmm2, SIMD_SP_atan_c6
		mulps		xmm2, xmm1
		addps		xmm2, SIMD_SP_atan_c7
		mulps		xmm2, xmm1
		addps		xmm2, SIMD_SP_one
		mulps		xmm2, xmm0
		addps		xmm2, xmm3
		movaps		[edx], xmm2
	}
}

/*
============
SSE_ATan
============
*/
float SSE_ATan( float y, float x ) {
#if 1

	float t;

	__asm {
		movss		xmm0, x
		movss		xmm3, xmm0
		movss		xmm4, xmm0
		andps		xmm0, SIMD_SP_absMask
		movss		xmm1, y
		xorps		xmm4, xmm1
		andps		xmm1, SIMD_SP_absMask
		andps		xmm4, SIMD_SP_signBitMask
		minss		xmm0, xmm1
		maxss		xmm1, xmm3
		cmpeqss		xmm3, xmm0
		rcpss		xmm2, xmm1
		mulss		xmm1, xmm2
		mulss		xmm1, xmm2
		addss		xmm2, xmm2
		subss		xmm2, xmm1				// xmm2 = 1 / y or 1 / x
		mulss		xmm0, xmm2				// xmm0 = x / y or y / x
		xorps		xmm0, xmm4
		movss		xmm1, xmm3
		andps		xmm1, SIMD_SP_signBitMask
		xorps		xmm0, xmm1				// xmm0 = -x / y or y / x
		orps		xmm4, SIMD_SP_halfPI	// xmm4 = +/- HALF_PI
		andps		xmm3, xmm4				// xmm3 = +/- HALF_PI or 0.0f
		movss		xmm1, xmm0
		mulss		xmm1, xmm1				// xmm1 = s
		movss		xmm2, SIMD_SP_atan_c0
		mulss		xmm2, xmm1
		addss		xmm2, SIMD_SP_atan_c1
		mulss		xmm2, xmm1
		addss		xmm2, SIMD_SP_atan_c2
		mulss		xmm2, xmm1
		addss		xmm2, SIMD_SP_atan_c3
		mulss		xmm2, xmm1
		addss		xmm2, SIMD_SP_atan_c4
		mulss		xmm2, xmm1
		addss		xmm2, SIMD_SP_atan_c5
		mulss		xmm2, xmm1
		addss		xmm2, SIMD_SP_atan_c6
		mulss		xmm2, xmm1
		addss		xmm2, SIMD_SP_atan_c7
		mulss		xmm2, xmm1
		addss		xmm2, SIMD_SP_one
		mulss		xmm2, xmm0
		addss		xmm2, xmm3
		movss		t, xmm2
	}

	return t;

#else

	float a, d, s, t;

	if ( fabs( y ) > fabs( x ) ) {
		a = -x / y;
		d = idMath::HALF_PI;
		*((unsigned int *)&d) ^= ( *((unsigned int *)&x) ^ *((unsigned int *)&y) ) & (1<<31);
	} else {
		a = y / x;
		d = 0.0f;
	}

	s = a * a;
	t = 0.0028662257f;
	t *= s;
	t += -0.0161657367f;
	t *= s;
	t += 0.0429096138f;
	t *= s;
	t += -0.0752896400f;
	t *= s;
	t += 0.1065626393f;
	t *= s;
	t += -0.1420889944f;
	t *= s;
	t += 0.1999355085f;
	t *= s;
	t += -0.3333314528f;
	t *= s;
	t += 1.0f;
	t *= a;
	t += d;

	return t;

#endif
}

/*
============
SSE_ATan4
============
*/
void SSE_ATan4( float y[4], float x[4], float at[4] ) {
	__asm {
		mov			esi, x
		mov			edi, y
		mov			edx, at
		movaps		xmm0, [esi]
		movaps		xmm3, xmm0
		movaps		xmm4, xmm0
		andps		xmm0, SIMD_SP_absMask
		movaps		xmm1, [edi]
		xorps		xmm4, xmm1
		andps		xmm1, SIMD_SP_absMask
		andps		xmm4, SIMD_SP_signBitMask
		minps		xmm0, xmm1
		maxps		xmm1, xmm3
		cmpeqps		xmm3, xmm0
		rcpps		xmm2, xmm1
		mulps		xmm1, xmm2
		mulps		xmm1, xmm2
		addps		xmm2, xmm2
		subps		xmm2, xmm1				// xmm2 = 1 / y or 1 / x
		mulps		xmm0, xmm2				// xmm0 = x / y or y / x
		xorps		xmm0, xmm4
		movaps		xmm1, xmm3
		andps		xmm1, SIMD_SP_signBitMask
		xorps		xmm0, xmm1				// xmm0 = -x / y or y / x
		orps		xmm4, SIMD_SP_halfPI	// xmm4 = +/- HALF_PI
		andps		xmm3, xmm4				// xmm3 = +/- HALF_PI or 0.0f
		movaps		xmm1, xmm0
		mulps		xmm1, xmm1				// xmm1 = s
		movaps		xmm2, SIMD_SP_atan_c0
		mulps		xmm2, xmm1
		addps		xmm2, SIMD_SP_atan_c1
		mulps		xmm2, xmm1
		addps		xmm2, SIMD_SP_atan_c2
		mulps		xmm2, xmm1
		addps		xmm2, SIMD_SP_atan_c3
		mulps		xmm2, xmm1
		addps		xmm2, SIMD_SP_atan_c4
		mulps		xmm2, xmm1
		addps		xmm2, SIMD_SP_atan_c5
		mulps		xmm2, xmm1
		addps		xmm2, SIMD_SP_atan_c6
		mulps		xmm2, xmm1
		addps		xmm2, SIMD_SP_atan_c7
		mulps		xmm2, xmm1
		addps		xmm2, SIMD_SP_one
		mulps		xmm2, xmm0
		addps		xmm2, xmm3
		movaps		[edx], xmm2
	}
}

/*
============
SSE_TestTrigonometry
============
*/
void SSE_TestTrigonometry( void ) {
	int i;
	float a, s1, s2, c1, c2;

	for ( i = 0; i < 100; i++ ) {
		a = i * idMath::HALF_PI / 100.0f;

		s1 = sin( a );
		s2 = SSE_SinZeroHalfPI( a );

		if ( fabs( s1 - s2 ) > 1e-7f ) {
			assert( 0 );
		}

		c1 = cos( a );
		c2 = SSE_CosZeroHalfPI( a );

		if ( fabs( c1 - c2 ) > 1e-7f ) {
			assert( 0 );
		}
	}

	for ( i = -200; i < 200; i++ ) {
		a = i * idMath::TWO_PI / 100.0f;

		s1 = sin( a );
		s2 = SSE_Sin( a );

		if ( fabs( s1 - s2 ) > 1e-6f ) {
			assert( 0 );
		}

		c1 = cos( a );
		c2 = SSE_Cos( a );

		if ( fabs( c1 - c2 ) > 1e-6f ) {
			assert( 0 );
		}

		SSE_SinCos( a, s2, c2 );
		if ( fabs( s1 - s2 ) > 1e-6f || fabs( c1 - c2 ) > 1e-6f ) {
			assert( 0 );
		}
	}
}

/*
============
idSIMD_SSE::GetName
============
*/
const char * idSIMD_SSE::GetName( void ) const {
	return "MMX & SSE";
}

/*
============
idSIMD_SSE::Add

  dst[i] = constant + src[i];
============
*/
void VPCALL idSIMD_SSE::Add( float *dst, const float constant, const float *src, const int count ) {
	KFLOAT_CA( add, dst, src, constant, count )
}

/*
============
idSIMD_SSE::Add

  dst[i] = src0[i] + src1[i];
============
*/
void VPCALL idSIMD_SSE::Add( float *dst, const float *src0, const float *src1, const int count ) {
	KFLOAT_AA( add, dst, src0, src1, count )
}

/*
============
idSIMD_SSE::Sub

  dst[i] = constant - src[i];
============
*/
void VPCALL idSIMD_SSE::Sub( float *dst, const float constant, const float *src, const int count ) {
	KFLOAT_CA( sub, dst, src, constant, count )
}

/*
============
idSIMD_SSE::Sub

  dst[i] = src0[i] - src1[i];
============
*/
void VPCALL idSIMD_SSE::Sub( float *dst, const float *src0, const float *src1, const int count ) {
	KFLOAT_AA( sub, dst, src0, src1, count )
}

/*
============
idSIMD_SSE::Mul

  dst[i] = constant * src[i];
============
*/
void VPCALL idSIMD_SSE::Mul( float *dst, const float constant, const float *src, const int count ) {
	KFLOAT_CA( mul, dst, src, constant, count )
}

/*
============
idSIMD_SSE::Mul

  dst[i] = src0[i] * src1[i];
============
*/
void VPCALL idSIMD_SSE::Mul( float *dst, const float *src0, const float *src1, const int count ) {
	KFLOAT_AA( mul, dst, src0, src1, count )
}

/*
============
idSIMD_SSE::Div

  dst[i] = constant / src[i];
============
*/
void VPCALL idSIMD_SSE::Div( float *dst, const float constant, const float *src, const int count ) {
	int pre, post;

	//	1 / x = 2 * rcpps(x) - (x * rcpps(x) * rcpps(x));
	__asm
	{
		movss	xmm1,constant
		shufps	xmm1,xmm1,0

		KFLOATINITDS( dst, src, count, pre, post )
		and		eax,15
		jne		lpNA
		jmp		lpA
		align	16
lpA:
		movaps	xmm2,[edx+ebx]
		movaps	xmm3,[edx+ebx+16]
		rcpps	xmm4,xmm2
		rcpps	xmm5,xmm3
		prefetchnta	[edx+ebx+64]
		mulps	xmm2,xmm4
		mulps	xmm2,xmm4
		mulps	xmm3,xmm5
		mulps	xmm3,xmm5
		addps	xmm4,xmm4
		addps	xmm5,xmm5
		subps	xmm4,xmm2
		subps	xmm5,xmm3
		mulps	xmm4,xmm1
		mulps	xmm5,xmm1
		movaps	[edi+ebx],xmm4
		movaps	[edi+ebx+16],xmm5
		add		ebx,16*2
		jl		lpA
		jmp		done
		align	16
lpNA:
		movups	xmm2,[edx+ebx]
		movups	xmm3,[edx+ebx+16]
		rcpps	xmm4,xmm2
		rcpps	xmm5,xmm3
		prefetchnta	[edx+ebx+64]
		mulps	xmm2,xmm4
		mulps	xmm2,xmm4
		mulps	xmm3,xmm5
		mulps	xmm3,xmm5
		addps	xmm4,xmm4
		addps	xmm5,xmm5
		subps	xmm4,xmm2
		subps	xmm5,xmm3
		mulps	xmm4,xmm1
		mulps	xmm5,xmm1
		movaps	[edi+ebx],xmm4
		movaps	[edi+ebx+16],xmm5
		add		ebx,16*2
		jl		lpNA
done:
		mov		edx,src
		mov		edi,dst
		KFLOATOPER( KDIVDSS1( [edi+ebx],xmm1,[edx+ebx] ),
					KDIVDSS4( [edi+ebx],xmm1,[edx+ebx] ), count )
	}
}

/*
============
idSIMD_SSE::Div

  dst[i] = src0[i] / src1[i];
============
*/
void VPCALL idSIMD_SSE::Div( float *dst, const float *src0, const float *src1, const int count ) {
	int		pre,post;

	//	1 / x = 2 * rcpps(x) - (x * rcpps(x) * rcpps(x));
	__asm
	{
		KFLOATINITDSS( dst, src0, src1, count, pre, post )
		and		eax,15
		jne		lpNA
		jmp		lpA
		align	16
lpA:
		movaps	xmm2,[esi+ebx]
		movaps	xmm3,[esi+ebx+16]
		rcpps	xmm4,xmm2
		rcpps	xmm5,xmm3
		prefetchnta	[esi+ebx+64]
		mulps	xmm2,xmm4
		mulps	xmm2,xmm4
		mulps	xmm3,xmm5
		mulps	xmm3,xmm5
		addps	xmm4,xmm4
		addps	xmm5,xmm5
		subps	xmm4,xmm2
		subps	xmm5,xmm3
		mulps	xmm4,[edx+ebx]
		mulps	xmm5,[edx+ebx+16]
		movaps	[edi+ebx],xmm4
		movaps	[edi+ebx+16],xmm5
		add		ebx,16*2
		jl		lpA
		jmp		done
		align	16
lpNA:
		movups	xmm2,[esi+ebx]
		movups	xmm3,[esi+ebx+16]
		rcpps	xmm4,xmm2
		rcpps	xmm5,xmm3
		prefetchnta	[esi+ebx+64]
		mulps	xmm2,xmm4
		mulps	xmm2,xmm4
		mulps	xmm3,xmm5
		mulps	xmm3,xmm5
		addps	xmm4,xmm4
		addps	xmm5,xmm5
		subps	xmm4,xmm2
		subps	xmm5,xmm3
		movups	xmm2,[edx+ebx]
		movups	xmm3,[edx+ebx+16]
		mulps	xmm4,xmm2
		mulps	xmm5,xmm3
		movaps	[edi+ebx],xmm4
		movaps	[edi+ebx+16],xmm5
		add		ebx,16*2
		jl		lpNA
done:
		mov		edx,src0
		mov		esi,src1
		mov		edi,dst
		KFLOATOPER( KDIVDSS1( [edi+ebx],[edx+ebx],[esi+ebx] ),
					KDIVDSS4( [edi+ebx],[edx+ebx],[esi+ebx] ), count )
	}
}
/*
============
Simd_MulAdd

 assumes count >= 7
============
*/
static void Simd_MulAdd( float *dst, const float constant, const float *src, const int count ) {
	__asm	mov			esi, dst
	__asm	mov			edi, src
	__asm	mov			eax, count
	__asm	shl			eax, 2
	__asm	mov			ecx, esi
	__asm	mov			edx, eax
	__asm	or			ecx, edi
	__asm	fld			constant
	__asm	and			ecx, 15
	__asm	jz			SimdMulAdd16
	__asm	and			ecx, 3
	__asm	jnz			SimdMulAdd8
	__asm	mov			ecx, esi
	__asm	xor			ecx, edi
	__asm	and			ecx, 15
	__asm	jnz			MulAdd8
	__asm	mov			ecx, esi
	__asm	and			ecx, 15
	__asm	neg			ecx
	__asm	add			ecx, 16
	__asm	sub			eax, ecx
	__asm	add			edi, ecx
	__asm	add			esi, ecx
	__asm	neg			ecx
	__asm	mov			edx, eax
	__asm loopPreMulAdd16:
	__asm	fld			st
	__asm	fmul		dword ptr [edi+ecx]
	__asm	fadd		dword ptr [esi+ecx]
	__asm	fstp		dword ptr [esi+ecx]
	__asm	add			ecx, 4
	__asm	jl			loopPreMulAdd16
	__asm SimdMulAdd16:
	__asm	and			eax, ~15
	__asm	movss		xmm1, constant
	__asm	shufps		xmm1, xmm1, 0x00
	__asm	add			esi, eax
	__asm	add			edi, eax
	__asm	neg			eax
	__asm	align		16
	__asm loopMulAdd16:
	__asm	movaps		xmm0, [edi+eax]
	__asm	mulps		xmm0, xmm1
	__asm	addps		xmm0, [esi+eax]
	__asm	movaps		[esi+eax], xmm0
	__asm	add			eax, 16
	__asm	jl			loopMulAdd16
	__asm	jmp			postMulAdd
	__asm MulAdd8:
	__asm	mov			ecx, esi
	__asm	and			ecx, 7
	__asm	jz			SimdMulAdd8
	__asm	sub			eax, ecx
	__asm	add			esi, ecx
	__asm	add			edi, ecx
	__asm	neg			ecx
	__asm	mov			edx, eax
	__asm loopPreMulAdd8:
	__asm	fld			st
	__asm	fmul		dword ptr [edi+ecx]
	__asm	fadd		dword ptr [esi+ecx]
	__asm	fstp		dword ptr [esi+ecx]
	__asm	add			ecx, 4
	__asm	jl			loopPreMulAdd8
	__asm SimdMulAdd8:
	__asm	and			eax, ~15
	__asm	movss		xmm1, constant
	__asm	shufps		xmm1, xmm1, 0x00
	__asm	add			esi, eax
	__asm	add			edi, eax
	__asm	neg			eax
	__asm	align		16
	__asm loopMulAdd8:
	__asm	movlps		xmm0, [edi+eax]
	__asm	movhps		xmm0, [edi+eax+8]
	__asm	mulps		xmm0, xmm1
	__asm	movlps		xmm2, [esi+eax]
	__asm	movhps		xmm2, [esi+eax+8]
	__asm	addps		xmm0, xmm2
	__asm	movlps		[esi+eax], xmm0
	__asm	movhps		[esi+eax+8], xmm0
	__asm	add			eax, 16
	__asm	jl			loopMulAdd8
	__asm	jmp			postMulAdd
	__asm postMulAdd:
	__asm	and			edx, 15
	__asm	jz			MulAddDone
	__asm	add			esi, edx
	__asm	add			edi, edx
	__asm	neg			edx
	__asm loopPostMulAdd:
	__asm	fld			st
	__asm	fmul		dword ptr [edi+edx]
	__asm	fadd		dword ptr [esi+edx]
	__asm	fstp		dword ptr [esi+edx]
	__asm	add			edx, 4
	__asm	jl			loopPostMulAdd
	__asm MulAddDone:
	__asm	fstp		st
}

#define MULADD_FEW( OPER )																				\
switch( count ) {																						\
	case 0:																								\
		return;																							\
	case 1:																								\
		dst[0] OPER c * src[0];																			\
		return;																							\
	case 2:																								\
		dst[0] OPER c * src[0]; dst[1] OPER c * src[1];													\
		return;																							\
	case 3:																								\
		dst[0] OPER c * src[0]; dst[1] OPER c * src[1]; dst[2] OPER c * src[2];							\
		return;																							\
	case 4:																								\
		dst[0] OPER c * src[0]; dst[1] OPER c * src[1]; dst[2] OPER c * src[2]; dst[3] OPER c * src[3];	\
		return;																							\
	case 5:																								\
		dst[0] OPER c * src[0]; dst[1] OPER c * src[1]; dst[2] OPER c * src[2]; dst[3] OPER c * src[3];	\
		dst[4] OPER c * src[4];																			\
		return;																							\
	case 6:																								\
		dst[0] OPER c * src[0]; dst[1] OPER c * src[1]; dst[2] OPER c * src[2]; dst[3] OPER c * src[3];	\
		dst[4] OPER c * src[4]; dst[5] OPER c * src[5];													\
		return;																							\
	case 7:																								\
		dst[0] OPER c * src[0]; dst[1] OPER c * src[1]; dst[2] OPER c * src[2]; dst[3] OPER c * src[3];	\
		dst[4] OPER c * src[4]; dst[5] OPER c * src[5]; dst[6] OPER c * src[6];							\
		return;																							\
	case 8:																								\
		dst[0] OPER c * src[0]; dst[1] OPER c * src[1]; dst[2] OPER c * src[2]; dst[3] OPER c * src[3];	\
		dst[4] OPER c * src[4]; dst[5] OPER c * src[5]; dst[6] OPER c * src[6]; dst[7] OPER c * src[7];	\
		return;																							\
	case 9:																								\
		dst[0] OPER c * src[0]; dst[1] OPER c * src[1]; dst[2] OPER c * src[2]; dst[3] OPER c * src[3];	\
		dst[4] OPER c * src[4]; dst[5] OPER c * src[5]; dst[6] OPER c * src[6]; dst[7] OPER c * src[7];	\
		dst[8] OPER c * src[8];																			\
		return;																							\
	case 10:																							\
		dst[0] OPER c * src[0]; dst[1] OPER c * src[1]; dst[2] OPER c * src[2]; dst[3] OPER c * src[3];	\
		dst[4] OPER c * src[4]; dst[5] OPER c * src[5]; dst[6] OPER c * src[6]; dst[7] OPER c * src[7];	\
		dst[8] OPER c * src[8]; dst[9] OPER c * src[9];													\
		return;																							\
	case 11:																							\
		dst[0] OPER c * src[0]; dst[1] OPER c * src[1]; dst[2] OPER c * src[2]; dst[3] OPER c * src[3];	\
		dst[4] OPER c * src[4]; dst[5] OPER c * src[5]; dst[6] OPER c * src[6]; dst[7] OPER c * src[7];	\
		dst[8] OPER c * src[8]; dst[9] OPER c * src[9]; dst[10] OPER c * src[10];						\
		return;																							\
}

/*
============
idSIMD_SSE::MulAdd

  dst[i] += constant * src[i];
============
*/
void VPCALL idSIMD_SSE::MulAdd( float *dst, const float constant, const float *src, const int count ) {
	float c = constant;
	MULADD_FEW( += )
	Simd_MulAdd( dst, constant, src, count );
}

/*
============
idSIMD_SSE::MulAdd

  dst[i] += src0[i] * src1[i];
============
*/
void VPCALL idSIMD_SSE::MulAdd( float *dst, const float *src0, const float *src1, const int count ) {
	for ( int i = 0; i < count; i++ ) {
		dst[i] += src0[i] + src1[i];
	}
}

/*
============
idSIMD_SSE::MulSub

  dst[i] -= constant * src[i];
============
*/
void VPCALL idSIMD_SSE::MulSub( float *dst, const float constant, const float *src, const int count ) {
	float c = constant;
	MULADD_FEW( -= )
	Simd_MulAdd( dst, -constant, src, count );
}

/*
============
idSIMD_SSE::MulSub

  dst[i] -= src0[i] * src1[i];
============
*/
void VPCALL idSIMD_SSE::MulSub( float *dst, const float *src0, const float *src1, const int count ) {
	for ( int i = 0; i < count; i++ ) {
		dst[i] -= src0[i] + src1[i];
	}
}

/*
============
idSIMD_SSE::Dot

  dst[i] = constant * src[i];
============
*/
void VPCALL idSIMD_SSE::Dot( float *dst, const idVec3 &constant, const idVec3 *src, const int count ) {
	__asm
	{
		mov			eax, count
		mov			edi, constant
		mov			edx, eax
		mov			esi, src
		mov			ecx, dst
		and			eax, ~3

		movss		xmm4, [edi+0]
		shufps		xmm4, xmm4, R_SHUFFLEPS( 0, 0, 0, 0 )
		movss		xmm5, [edi+4]
		shufps		xmm5, xmm5, R_SHUFFLEPS( 0, 0, 0, 0 )
		movss		xmm6, [edi+8]
		shufps		xmm6, xmm6, R_SHUFFLEPS( 0, 0, 0, 0 )

		jz			done4
		imul		eax, 12
		add			esi, eax
		neg			eax

	loop4:
		movlps		xmm1, [esi+eax+ 0]
		movlps		xmm2, [esi+eax+ 8]
		movlps		xmm3, [esi+eax+16]
		movhps		xmm1, [esi+eax+24]
		movhps		xmm2, [esi+eax+32]
		movhps		xmm3, [esi+eax+40]
		movaps		xmm0, xmm1
		shufps		xmm0, xmm2, R_SHUFFLEPS( 0, 2, 1, 3 )
		shufps		xmm1, xmm3, R_SHUFFLEPS( 1, 3, 0, 2 )
		shufps		xmm2, xmm3, R_SHUFFLEPS( 0, 2, 1, 3 )
		add			ecx, 16
		add			eax, 4*12
		mulps		xmm0, xmm4
		mulps		xmm1, xmm5
		mulps		xmm2, xmm6
		addps		xmm0, xmm1
		addps		xmm0, xmm2
		shufps		xmm0, xmm0, R_SHUFFLEPS( 0, 2, 1, 3 )
		movlps		[ecx-16+0], xmm0
		movhps		[ecx-16+8], xmm0
		jl			loop4

	done4:
		and			edx, 3
		jz			done1

	loop1:
		movss		xmm0, [esi+eax+0]
		movss		xmm1, [esi+eax+4]
		movss		xmm2, [esi+eax+8]
		mulss		xmm0, xmm4
		mulss		xmm1, xmm5
		mulss		xmm2, xmm6
		add			ecx, 4
		addss		xmm0, xmm1
		add			eax, 12
		addss		xmm0, xmm2
		dec			edx
		movss		[ecx-4], xmm0
		jnz			loop1

	done1:
	}
}

/*
============
idSIMD_SSE::Dot

  dst[i] = constant * src[i].Normal() + src[i][3];
============
*/
void VPCALL idSIMD_SSE::Dot( float *dst, const idVec3 &constant, const idPlane *src, const int count ) {
	__asm {
		mov			eax, count
		mov			edi, constant
		mov			edx, eax
		mov			esi, src
		mov			ecx, dst
		and			eax, ~3

		movss		xmm5, [edi+0]
		shufps		xmm5, xmm5, R_SHUFFLEPS( 0, 0, 0, 0 )
		movss		xmm6, [edi+4]
		shufps		xmm6, xmm6, R_SHUFFLEPS( 0, 0, 0, 0 )
		movss		xmm7, [edi+8]
		shufps		xmm7, xmm7, R_SHUFFLEPS( 0, 0, 0, 0 )

		jz			startVert1
		imul		eax, 16
		add			esi, eax
		neg			eax

	loopVert4:

		movlps		xmm1, [esi+eax+ 0]
		movlps		xmm3, [esi+eax+ 8]
		movhps		xmm1, [esi+eax+16]
		movhps		xmm3, [esi+eax+24]
		movlps		xmm2, [esi+eax+32]
		movlps		xmm4, [esi+eax+40]
		movhps		xmm2, [esi+eax+48]
		movhps		xmm4, [esi+eax+56]
		movaps		xmm0, xmm1
		shufps		xmm0, xmm2, R_SHUFFLEPS( 0, 2, 0, 2 )
		shufps		xmm1, xmm2, R_SHUFFLEPS( 1, 3, 1, 3 )
		movaps		xmm2, xmm3
		shufps		xmm2, xmm4, R_SHUFFLEPS( 0, 2, 0, 2 )
		shufps		xmm3, xmm4, R_SHUFFLEPS( 1, 3, 1, 3 )

		add			ecx, 16
		add			eax, 4*16

		mulps		xmm0, xmm5
		mulps		xmm1, xmm6
		mulps		xmm2, xmm7
		addps		xmm0, xmm3
		addps		xmm0, xmm1
		addps		xmm0, xmm2

		movlps		[ecx-16+0], xmm0
		movhps		[ecx-16+8], xmm0
		jl			loopVert4

	startVert1:
		and			edx, 3
		jz			done

	loopVert1:
		movss		xmm0, [esi+eax+0]
		movss		xmm1, [esi+eax+4]
		movss		xmm2, [esi+eax+8]
		mulss		xmm0, xmm5
		mulss		xmm1, xmm6
		mulss		xmm2, xmm7
		addss		xmm0, [esi+eax+12]
		add			ecx, 4
		addss		xmm0, xmm1
		add			eax, 16
		addss		xmm0, xmm2
		dec			edx
		movss		[ecx-4], xmm0
		jnz			loopVert1

	done:
	}
}


/*
============
idSIMD_SSE::Dot

  dst[i] = constant.Normal() * src[i] + constant[3];
============
*/
void VPCALL idSIMD_SSE::Dot( float *dst, const idPlane &constant, const idVec3 *src, const int count ) {
	__asm
	{
		mov			eax, count
		mov			edi, constant
		mov			edx, eax
		mov			esi, src
		mov			ecx, dst
		and			eax, ~3

		movss		xmm4, [edi+0]
		shufps		xmm4, xmm4, R_SHUFFLEPS( 0, 0, 0, 0 )
		movss		xmm5, [edi+4]
		shufps		xmm5, xmm5, R_SHUFFLEPS( 0, 0, 0, 0 )
		movss		xmm6, [edi+8]
		shufps		xmm6, xmm6, R_SHUFFLEPS( 0, 0, 0, 0 )
		movss		xmm7, [edi+12]
		shufps		xmm7, xmm7, R_SHUFFLEPS( 0, 0, 0, 0 )

		jz			done4
		imul		eax, 12
		add			esi, eax
		neg			eax

	loop4:
		movlps		xmm1, [esi+eax+ 0]
		movlps		xmm2, [esi+eax+ 8]
		movlps		xmm3, [esi+eax+16]
		movhps		xmm1, [esi+eax+24]
		movhps		xmm2, [esi+eax+32]
		movhps		xmm3, [esi+eax+40]
		movaps		xmm0, xmm1
		shufps		xmm0, xmm2, R_SHUFFLEPS( 0, 2, 1, 3 )
		shufps		xmm1, xmm3, R_SHUFFLEPS( 1, 3, 0, 2 )
		shufps		xmm2, xmm3, R_SHUFFLEPS( 0, 2, 1, 3 )

		add			ecx, 16
		add			eax, 4*12

		mulps		xmm0, xmm4
		mulps		xmm1, xmm5
		mulps		xmm2, xmm6
		addps		xmm0, xmm7
		addps		xmm0, xmm1
		addps		xmm0, xmm2
		shufps		xmm0, xmm0, R_SHUFFLEPS( 0, 2, 1, 3 )

		movlps		[ecx-16+0], xmm0
		movhps		[ecx-16+8], xmm0
		jl			loop4

	done4:
		and			edx, 3
		jz			done1

	loop1:
		movss		xmm0, [esi+eax+0]
		movss		xmm1, [esi+eax+4]
		movss		xmm2, [esi+eax+8]
		mulss		xmm0, xmm4
		mulss		xmm1, xmm5
		mulss		xmm2, xmm6
		addss		xmm0, xmm7
		add			ecx, 4
		addss		xmm0, xmm1
		add			eax, 12
		addss		xmm0, xmm2
		dec			edx
		movss		[ecx-4], xmm0
		jnz			loop1

	done1:
	}
}

/*
============
idSIMD_SSE::Dot

  dst[i] = constant.Normal() * src[i].Normal() + constant[3] * src[i][3];
============
*/
void VPCALL idSIMD_SSE::Dot( float *dst, const idPlane &constant, const idPlane *src, const int count ) {

#define SINGLE_OP(SRC, DEST)							\
	__asm	movlps		xmm0,[SRC]						\
	__asm	movlps		xmm1,[SRC+8]					\
	__asm	mulps		xmm0,xmm4						\
	__asm	mulps		xmm1,xmm5						\
	__asm	addps		xmm0,xmm1						\
	__asm	movaps		xmm1,xmm0						\
	__asm	shufps		xmm1,xmm1,SHUFFLEPS(1,1,1,1)	\
	__asm	addss		xmm0,xmm1						\
	__asm	movss		[DEST],xmm0						\
	__asm	add			SRC,16							\
	__asm	add			DEST,4

#define DUAL_OP(SRC, DEST)								\
	__asm	movlps		xmm0,[SRC]						\
	__asm	movlps		xmm1,[SRC+8]					\
	__asm	movhps		xmm0,[SRC+16]					\
	__asm	movhps		xmm1,[SRC+24]					\
	__asm	mulps		xmm0,xmm4						\
	__asm	mulps		xmm1,xmm5						\
	__asm	addps		xmm0,xmm1						\
	__asm	shufps		xmm1,xmm0,SHUFFLEPS(2,0,1,0)	\
	__asm	shufps		xmm0,xmm0,SHUFFLEPS(3,1,2,0)	\
	__asm	addps		xmm0,xmm1						\
	__asm	movhps		[DEST],xmm0						\
	__asm	add			SRC,32							\
	__asm	add			DEST,8

	__asm {
		mov			edx, dst
		mov			eax, src
		mov			ebx, constant
		mov			ecx, count

		movlps		xmm4, [ebx]
		shufps		xmm4, xmm4, SHUFFLEPS(1,0,1,0)
		movlps		xmm5, [ebx+8]
		shufps		xmm5, xmm5, SHUFFLEPS(1,0,1,0)

		xorps		xmm0, xmm0
		xorps		xmm1, xmm1

	_lpAlignDest:
		test		edx, 0x0f
		jz			_destAligned
		SINGLE_OP(eax,edx)
		dec			ecx
		jnz			_lpAlignDest
		jmp			_vpExit

	_destAligned:
		push		ecx

		cmp			ecx, 4
		jl			_post

		and			ecx, ~3
		shl			ecx, 2
		lea			eax, [eax+ecx*4]
		add			edx, ecx
		neg			ecx

		movlps		xmm0, [eax+ecx*4]
		movhps		xmm0, [eax+ecx*4+16]
		movlps		xmm2, [eax+ecx*4+32]
		movhps		xmm2, [eax+ecx*4+48]
		jmp			_lpStart

		align	16
	_lp:
		prefetchnta	[eax+ecx*4+128]
		addps		xmm1, xmm0
		movlps		xmm0, [eax+ecx*4]
		movhps		xmm0, [eax+ecx*4+16]
		movlps		xmm2, [eax+ecx*4+32]
		movhps		xmm2, [eax+ecx*4+48]
		movaps		[edx+ecx-16],xmm1
	_lpStart:
		movlps		xmm1, [eax+ecx*4+8]
		movhps		xmm1, [eax+ecx*4+24]
		movlps		xmm3, [eax+ecx*4+40]
		movhps		xmm3, [eax+ecx*4+56]
		add			ecx, 16
		mulps		xmm1, xmm5
		mulps		xmm2, xmm4
		mulps		xmm3, xmm5
		addps		xmm2, xmm3						// y3+w3 x3+z3 y2+w2 x2+z2
		mulps		xmm0, xmm4
		addps		xmm0, xmm1						// y1+w1 x1+z1 y0+w0 x0+z0
		movaps		xmm1, xmm0
		shufps		xmm0, xmm2, SHUFFLEPS(2,0,2,0)	// x3+z3 x2+z2 x1+z1 x0+z0
		shufps		xmm1, xmm2, SHUFFLEPS(3,1,3,1)	// y3+w3 y2+w2 y1+w1 y0+w0
		js			_lp
		addps		xmm1, xmm0
		movaps		[edx+ecx-16], xmm1
	_post:
		pop			ecx
		and			ecx, 0x3
		cmp			ecx, 2
		jl			_post1
		DUAL_OP(eax,edx)
		sub			ecx, 2
	_post1:
		cmp			ecx, 1
		jne			_vpExit
		SINGLE_OP(eax,edx)
	_vpExit:
	}

#undef DUAL_OP
#undef SINGLE_OP

}


/*
============
idSIMD_SSE::Dot

  dst[i] = src0[i] * src1[i];
============
*/
void VPCALL idSIMD_SSE::Dot( float *dst, const idVec3 *src0, const idVec3 *src1, const int count ) {
	__asm
	{
		mov			eax, count
		mov			edi, src0
		mov			edx, eax
		mov			esi, src1
		mov			ecx, dst
		and			eax, ~3

		jz			done4
		imul		eax, 12
		add			edi, eax
		add			esi, eax
		neg			eax

	loop4:
		movlps		xmm0, [esi+eax]						// 0, 1, X, X
		movlps		xmm3, [edi+eax]						// 0, 1, X, X
		movlps		xmm1, [esi+eax+8]					// 2, 3, X, X
		movlps		xmm4, [edi+eax+8]					// 2, 3, X, X
		movhps		xmm0, [esi+eax+24]					// 0, 1, 6, 7
		movhps		xmm3, [edi+eax+24]					// 0, 1, 6, 7
		movhps		xmm1, [esi+eax+32]					// 2, 3, 8, 9
		movhps		xmm4, [edi+eax+32]					// 2, 3, 8, 9
		movlps		xmm2, [esi+eax+16]					// 4, 5, X, X
		movlps		xmm5, [edi+eax+16]					// 4, 5, X, X
		movhps		xmm2, [esi+eax+40]					// 4, 5, 10, 11
		movhps		xmm5, [edi+eax+40]					// 4, 5, 10, 11

		add			ecx, 16
		add			eax, 48

		mulps		xmm0, xmm3
		mulps		xmm1, xmm4
		mulps		xmm2, xmm5
		movaps		xmm7, xmm0
		shufps		xmm7, xmm1, R_SHUFFLEPS( 0, 2, 1, 3 )	// 0, 6, 3, 9
		shufps		xmm0, xmm2, R_SHUFFLEPS( 1, 3, 0, 2 )	// 1, 7, 4, 10
		shufps		xmm1, xmm2, R_SHUFFLEPS( 0, 2, 1, 3 )	// 2, 8, 5, 11
		addps		xmm7, xmm0
		addps		xmm7, xmm1
		shufps		xmm7, xmm7, R_SHUFFLEPS( 0, 2, 1, 3 )

		movlps		[ecx-16+0], xmm7
		movhps		[ecx-16+8], xmm7
		jl			loop4

	done4:
		and			edx, 3
		jz			done1

	loop1:
		movss		xmm0, [esi+eax+0]
		movss		xmm3, [edi+eax+0]
		movss		xmm1, [esi+eax+4]
		movss		xmm4, [edi+eax+4]
		movss		xmm2, [esi+eax+8]
		movss		xmm5, [edi+eax+8]
		mulss		xmm0, xmm3
		mulss		xmm1, xmm4
		mulss		xmm2, xmm5
		add			ecx, 4
		addss		xmm0, xmm1
		add			eax, 12
		addss		xmm0, xmm2
		dec			edx
		movss		[ecx-4], xmm0
		jnz			loop1

	done1:
	}
}

/*
============
idSIMD_SSE::Dot

  dot = src1[0] * src2[0] + src1[1] * src2[1] + src1[2] * src2[2] + ...
============
*/
void VPCALL idSIMD_SSE::Dot( float &dot, const float *src1, const float *src2, const int count ) {
	switch( count ) {
		case 0:
			dot = 0.0f;
			return;
		case 1:
			dot = src1[0] * src2[0];
			return;
		case 2:
			dot = src1[0] * src2[0] + src1[1] * src2[1];
			return;
		case 3:
			dot = src1[0] * src2[0] + src1[1] * src2[1] + src1[2] * src2[2];
			return;
		default:
			__asm {
				mov			ecx, src1
				mov			edx, src2
				mov			eax, ecx
				or			eax, edx
				and			eax, 15
				jz			alignedDot
				// unaligned
				mov			eax, count
				shr			eax, 2
				shl			eax, 4
				add			ecx, eax
				add			edx, eax
				neg			eax
				movups		xmm0, [ecx+eax]
				movups		xmm1, [edx+eax]
				mulps		xmm0, xmm1
				add			eax, 16
				jz			doneDot
			loopUnalignedDot:
				movups		xmm1, [ecx+eax]
				movups		xmm2, [edx+eax]
				mulps		xmm1, xmm2
				addps		xmm0, xmm1
				add			eax, 16
				jl			loopUnalignedDot
				jmp			doneDot
				// aligned
			alignedDot:
				mov			eax, count
				shr			eax, 2
				shl			eax, 4
				add			ecx, eax
				add			edx, eax
				neg			eax
				movaps		xmm0, [ecx+eax]
				movaps		xmm1, [edx+eax]
				mulps		xmm0, xmm1
				add			eax, 16
				jz			doneDot
			loopAlignedDot:
				movaps		xmm1, [ecx+eax]
				movaps		xmm2, [edx+eax]
				mulps		xmm1, xmm2
				addps		xmm0, xmm1
				add			eax, 16
				jl			loopAlignedDot
			doneDot:
			}
			switch( count & 3 ) {
				case 1:
					__asm {
						movss	xmm1, [ecx]
						movss	xmm2, [edx]
						mulss	xmm1, xmm2
						addss	xmm0, xmm1
					}
					break;
				case 2:
					__asm {
						xorps	xmm2, xmm2
						movlps	xmm1, [ecx]
						movlps	xmm2, [edx]
						mulps	xmm1, xmm2
						addps	xmm0, xmm1
					}
					break;
				case 3:
					__asm {
						movss	xmm1, [ecx]
						movhps	xmm1, [ecx+4]
						movss	xmm2, [edx]
						movhps	xmm2, [edx+4]
						mulps	xmm1, xmm2
						addps	xmm0, xmm1
					}
					break;
			}
			__asm {
				movhlps		xmm1, xmm0
				addps		xmm0, xmm1
				movaps		xmm1, xmm0
				shufps		xmm1, xmm1, R_SHUFFLEPS( 1, 0, 0, 0 )
				addss		xmm0, xmm1
				mov			eax, dot
				movss		[eax], xmm0
			}
			return;
	}
}

//
//	cmpeqps		==		Equal
//	cmpneqps	!=		Not Equal
//	cmpltps		<		Less Than
//  cmpnltps	>=		Not Less Than
//	cmpnleps	>		Not Less Or Equal
//
#define FLIP	not al
#define NOFLIP

#define COMPARECONSTANT( DST, SRC0, CONSTANT, COUNT, CMP, CMPSIMD, DOFLIP )				\
	int i, cnt, pre, post;																\
	float *aligned;																		\
																						\
	/* if the float array is not aligned on a 4 byte boundary */						\
	if ( ((int) SRC0) & 3 ) {															\
		/* unaligned memory access */													\
		pre = 0;																		\
		cnt = COUNT >> 2;																\
		post = COUNT - (cnt<<2);														\
		__asm	mov			edx, cnt													\
		__asm	test		edx, edx													\
		__asm	je			doneCmp														\
		__asm	push		ebx															\
		__asm	neg			edx															\
		__asm	mov			esi, SRC0													\
		__asm	prefetchnta	[esi+64]													\
		__asm	movss		xmm1, CONSTANT												\
		__asm	shufps		xmm1, xmm1, R_SHUFFLEPS( 0, 0, 0, 0 )						\
		__asm	mov			edi, DST													\
		__asm	mov			ecx, 0x01010101												\
		__asm loopNA:																	\
		__asm	movups		xmm0, [esi]													\
		__asm	prefetchnta	[esi+128]													\
		__asm	CMPSIMD		xmm0, xmm1													\
		__asm	movmskps	eax, xmm0													\
		__asm	DOFLIP																	\
		__asm	mov			ah, al														\
		__asm	shr			ah, 1														\
		__asm	mov			bx, ax														\
		__asm	shl			ebx, 14														\
		__asm	mov			bx, ax														\
		__asm	and			ebx, ecx													\
		__asm	mov			dword ptr [edi], ebx										\
		__asm	add			esi, 16														\
		__asm	add			edi, 4														\
		__asm	inc			edx															\
		__asm	jl			loopNA														\
		__asm	pop			ebx															\
	}																					\
	else {																				\
		/* aligned memory access */														\
		aligned = (float *) ((((int) SRC0) + 15) & ~15);								\
		if ( (int)aligned > ((int)src0) + COUNT ) {										\
			pre = COUNT;																\
			post = 0;																	\
		}																				\
		else {																			\
			pre = aligned - SRC0;														\
			cnt = (COUNT - pre) >> 2;													\
			post = COUNT - pre - (cnt<<2);												\
			__asm	mov			edx, cnt												\
			__asm	test		edx, edx												\
			__asm	je			doneCmp													\
			__asm	push		ebx														\
			__asm	neg			edx														\
			__asm	mov			esi, aligned											\
			__asm	prefetchnta	[esi+64]												\
			__asm	movss		xmm1, CONSTANT											\
			__asm	shufps		xmm1, xmm1, R_SHUFFLEPS( 0, 0, 0, 0 )					\
			__asm	mov			edi, DST												\
			__asm	add			edi, pre												\
			__asm	mov			ecx, 0x01010101											\
			__asm loopA:																\
			__asm	movaps		xmm0, [esi]												\
			__asm	prefetchnta	[esi+128]												\
			__asm	CMPSIMD		xmm0, xmm1												\
			__asm	movmskps	eax, xmm0												\
			__asm	DOFLIP																\
			__asm	mov			ah, al													\
			__asm	shr			ah, 1													\
			__asm	mov			bx, ax													\
			__asm	shl			ebx, 14													\
			__asm	mov			bx, ax													\
			__asm	and			ebx, ecx												\
			__asm	mov			dword ptr [edi], ebx									\
			__asm	add			esi, 16													\
			__asm	add			edi, 4													\
			__asm	inc			edx														\
			__asm	jl			loopA													\
			__asm	pop			ebx														\
		}																				\
	}																					\
	doneCmp:																			\
	double c = constant;																\
	for ( i = 0; i < pre; i++ ) {														\
		dst[i] = src0[i] CMP c;															\
	}																					\
	for ( i = count - post; i < count; i++ ) {											\
		dst[i] = src0[i] CMP c;															\
	}

#define COMPAREBITCONSTANT( DST, BITNUM, SRC0, CONSTANT, COUNT, CMP, CMPSIMD, DOFLIP )	\
	int i, cnt, pre, post;																\
	float *aligned;																		\
																						\
	/* if the float array is not aligned on a 4 byte boundary */						\
	if ( ((int) SRC0) & 3 ) {															\
		/* unaligned memory access */													\
		pre = 0;																		\
		cnt = COUNT >> 2;																\
		post = COUNT - (cnt<<2);														\
		__asm	mov			edx, cnt													\
		__asm	test		edx, edx													\
		__asm	je			doneCmp														\
		__asm	push		ebx															\
		__asm	neg			edx															\
		__asm	mov			esi, SRC0													\
		__asm	prefetchnta	[esi+64]													\
		__asm	movss		xmm1, CONSTANT												\
		__asm	shufps		xmm1, xmm1, R_SHUFFLEPS( 0, 0, 0, 0 )						\
		__asm	mov			edi, DST													\
		__asm	mov			cl, bitNum													\
		__asm loopNA:																	\
		__asm	movups		xmm0, [esi]													\
		__asm	prefetchnta	[esi+128]													\
		__asm	CMPSIMD		xmm0, xmm1													\
		__asm	movmskps	eax, xmm0													\
		__asm	DOFLIP																	\
		__asm	mov			ah, al														\
		__asm	shr			ah, 1														\
		__asm	mov			bx, ax														\
		__asm	shl			ebx, 14														\
		__asm	mov			bx, ax														\
		__asm	and			ebx, 0x01010101												\
		__asm	shl			ebx, cl														\
		__asm	or			ebx, dword ptr [edi]										\
		__asm	mov			dword ptr [edi], ebx										\
		__asm	add			esi, 16														\
		__asm	add			edi, 4														\
		__asm	inc			edx															\
		__asm	jl			loopNA														\
		__asm	pop			ebx															\
	}																					\
	else {																				\
		/* aligned memory access */														\
		aligned = (float *) ((((int) SRC0) + 15) & ~15);								\
		if ( (int)aligned > ((int)src0) + COUNT ) {										\
			pre = COUNT;																\
			post = 0;																	\
		}																				\
		else {																			\
			pre = aligned - SRC0;														\
			cnt = (COUNT - pre) >> 2;													\
			post = COUNT - pre - (cnt<<2);												\
			__asm	mov			edx, cnt												\
			__asm	test		edx, edx												\
			__asm	je			doneCmp													\
			__asm	push		ebx														\
			__asm	neg			edx														\
			__asm	mov			esi, aligned											\
			__asm	prefetchnta	[esi+64]												\
			__asm	movss		xmm1, CONSTANT											\
			__asm	shufps		xmm1, xmm1, R_SHUFFLEPS( 0, 0, 0, 0 )					\
			__asm	mov			edi, DST												\
			__asm	add			edi, pre												\
			__asm	mov			cl, bitNum												\
			__asm loopA:																\
			__asm	movaps		xmm0, [esi]												\
			__asm	prefetchnta	[esi+128]												\
			__asm	CMPSIMD		xmm0, xmm1												\
			__asm	movmskps	eax, xmm0												\
			__asm	DOFLIP																\
			__asm	mov			ah, al													\
			__asm	shr			ah, 1													\
			__asm	mov			bx, ax													\
			__asm	shl			ebx, 14													\
			__asm	mov			bx, ax													\
			__asm	and			ebx, 0x01010101											\
			__asm	shl			ebx, cl													\
			__asm	or			ebx, dword ptr [edi]									\
			__asm	mov			dword ptr [edi], ebx									\
			__asm	add			esi, 16													\
			__asm	add			edi, 4													\
			__asm	inc			edx														\
			__asm	jl			loopA													\
			__asm	pop			ebx														\
		}																				\
	}																					\
	doneCmp:																			\
	float c = constant;																	\
	for ( i = 0; i < pre; i++ ) {														\
		dst[i] |= ( src0[i] CMP c ) << BITNUM;											\
	}																					\
	for ( i = count - post; i < count; i++ ) {											\
		dst[i] |= ( src0[i] CMP c ) << BITNUM;											\
	}

/*
============
idSIMD_SSE::CmpGT

  dst[i] = src0[i] > constant;
============
*/
void VPCALL idSIMD_SSE::CmpGT( byte *dst, const float *src0, const float constant, const int count ) {
	COMPARECONSTANT( dst, src0, constant, count, >, cmpnleps, NOFLIP )
}

/*
============
idSIMD_SSE::CmpGT

  dst[i] |= ( src0[i] > constant ) << bitNum;
============
*/
void VPCALL idSIMD_SSE::CmpGT( byte *dst, const byte bitNum, const float *src0, const float constant, const int count ) {
	COMPAREBITCONSTANT( dst, bitNum, src0, constant, count, >, cmpnleps, NOFLIP )
}

/*
============
idSIMD_SSE::CmpGE

  dst[i] = src0[i] >= constant;
============
*/
void VPCALL idSIMD_SSE::CmpGE( byte *dst, const float *src0, const float constant, const int count ) {
	COMPARECONSTANT( dst, src0, constant, count, >=, cmpnltps, NOFLIP )
}

/*
============
idSIMD_SSE::CmpGE

  dst[i] |= ( src0[i] >= constant ) << bitNum;
============
*/
void VPCALL idSIMD_SSE::CmpGE( byte *dst, const byte bitNum, const float *src0, const float constant, const int count ) {
	COMPAREBITCONSTANT( dst, bitNum, src0, constant, count, >=, cmpnltps, NOFLIP )
}

/*
============
idSIMD_SSE::CmpLT

  dst[i] = src0[i] < constant;
============
*/
void VPCALL idSIMD_SSE::CmpLT( byte *dst, const float *src0, const float constant, const int count ) {
	COMPARECONSTANT( dst, src0, constant, count, <, cmpltps, NOFLIP )
}

/*
============
idSIMD_SSE::CmpLT

  dst[i] |= ( src0[i] < constant ) << bitNum;
============
*/
void VPCALL idSIMD_SSE::CmpLT( byte *dst, const byte bitNum, const float *src0, const float constant, const int count ) {
	COMPAREBITCONSTANT( dst, bitNum, src0, constant, count, <, cmpltps, NOFLIP )
}

/*
============
idSIMD_SSE::CmpLE

  dst[i] = src0[i] <= constant;
============
*/
void VPCALL idSIMD_SSE::CmpLE( byte *dst, const float *src0, const float constant, const int count ) {
	COMPARECONSTANT( dst, src0, constant, count, <=, cmpnleps, FLIP )
}

/*
============
idSIMD_SSE::CmpLE

  dst[i] |= ( src0[i] <= constant ) << bitNum;
============
*/
void VPCALL idSIMD_SSE::CmpLE( byte *dst, const byte bitNum, const float *src0, const float constant, const int count ) {
	COMPAREBITCONSTANT( dst, bitNum, src0, constant, count, <=, cmpnleps, FLIP )
}

/*
============
idSIMD_SSE::MinMax
============
*/
void VPCALL idSIMD_SSE::MinMax( float &min, float &max, const float *src, const int count ) {
	int i, pre, post;

	min = idMath::INFINITY; max = -idMath::INFINITY;

	__asm
	{
		push		ebx
		mov			eax, min
		mov			ebx, max
		movss		xmm0, [eax]
		movss		xmm1, [ebx]
		shufps		xmm0, xmm0, 0
		shufps		xmm1, xmm1, 0

		KFLOATINITS( src, count, pre, post )
		and			eax, 15
		jz			lpA
		jmp			lpNA
		align		16
lpNA:
		movups		xmm2, [edx+ebx]
		movups		xmm3, [edx+ebx+16]
		minps		xmm0, xmm2
		maxps		xmm1, xmm2
		prefetchnta	[edx+ebx+64]
		minps		xmm0, xmm3
		maxps		xmm1, xmm3
		add			ebx, 16*2
		jl			lpNA
		jmp			done2
lpA:
		movaps		xmm2, [edx+ebx]
		movaps		xmm3, [edx+ebx+16]
		minps		xmm0, xmm2
		maxps		xmm1, xmm2
		prefetchnta	[edx+ebx+64]
		minps		xmm0, xmm3
		maxps		xmm1, xmm3
		add			ebx, 16*2
		jl			lpA
		jmp			done2
		align		16
done2:
		movaps		xmm2, xmm0
		movaps		xmm3, xmm1
		shufps		xmm2, xmm2, R_SHUFFLEPS( 1, 2, 3, 0 )
		shufps		xmm3, xmm3, R_SHUFFLEPS( 1, 2, 3, 0 )
		minss		xmm0, xmm2
		maxss		xmm1, xmm3
		shufps		xmm2, xmm2, R_SHUFFLEPS( 1, 2, 3, 0 )
		shufps		xmm3, xmm3, R_SHUFFLEPS( 1, 2, 3, 0 )
		minss		xmm0, xmm2
		maxss		xmm1, xmm3
		shufps		xmm2, xmm2, R_SHUFFLEPS( 1, 2, 3, 0 )
		shufps		xmm3, xmm3, R_SHUFFLEPS( 1, 2, 3, 0 )
		minss		xmm0, xmm2
		maxss		xmm1, xmm3
		mov			eax, min
		mov			ebx, max
		movss		[eax], xmm0
		movss		[ebx], xmm1
done:
		pop			ebx
	}

	for ( i = 0; i < pre; i++ ) {
		float tmp = src[i];
		if ( tmp > max ) {
			max = tmp;
		}
		if ( tmp < min ) {
			min = tmp;
		}
	}
	for ( i = count - post; i < count; i++ ) {
		float tmp = src[i];
		if ( tmp > max ) {
			max = tmp;
		}
		if ( tmp < min ) {
			min = tmp;
		}
	}
}

/*
============
idSIMD_SSE::MinMax
============
*/
void VPCALL idSIMD_SSE::MinMax( idVec2 &min, idVec2 &max, const idVec2 *src, const int count ) {
	__asm {
		mov			eax, count
		test		eax, eax
		movss		xmm0, idMath::INFINITY
		xorps		xmm1, xmm1
		shufps		xmm0, xmm0, R_SHUFFLEPS( 0, 0, 0, 0 )
		subps		xmm1, xmm0
		jz			done
		mov			ecx, eax
		and			ecx, 1
		mov			esi, src
		jz			startLoop
		movlps		xmm2, [esi]
		shufps		xmm2, xmm2, R_SHUFFLEPS( 0, 1, 0, 1 )
		dec			eax
		add			esi, 2*4
		minps		xmm0, xmm2
		maxps		xmm1, xmm2
	startLoop:
		imul		eax, 2*4
		add			esi, eax
		neg			eax
	loopVert:
		movlps		xmm2, [esi+eax]
		movhps		xmm2, [esi+eax+8]
		add			eax, 4*4
		minps		xmm0, xmm2
		maxps		xmm1, xmm2
		jl			loopVert
	done:
		movaps		xmm2, xmm0
		shufps		xmm2, xmm2, R_SHUFFLEPS( 2, 3, 0, 1 )
		minps		xmm0, xmm2
		mov			esi, min
		movlps		[esi], xmm0
		movaps		xmm3, xmm1
		shufps		xmm3, xmm3, R_SHUFFLEPS( 2, 3, 0, 1 )
		maxps		xmm1, xmm3
		mov			edi, max
		movlps		[edi], xmm1
	}
}

/*
============
idSIMD_SSE::MinMax
============
*/
void VPCALL idSIMD_SSE::MinMax( idVec3 &min, idVec3 &max, const idVec3 *src, const int count ) {
	__asm {

		movss		xmm0, idMath::INFINITY
		xorps		xmm1, xmm1
		shufps		xmm0, xmm0, R_SHUFFLEPS( 0, 0, 0, 0 )
		subps		xmm1, xmm0
		movaps		xmm2, xmm0
		movaps		xmm3, xmm1

		mov			esi, src
		mov			eax, count
		and			eax, ~3
		jz			done4
		imul		eax, 12
		add			esi, eax
		neg			eax

	loop4:
//		prefetchnta	[esi+4*12]

		movss		xmm4, [esi+eax+0*12+8]
		movhps		xmm4, [esi+eax+0*12+0]
		minps		xmm0, xmm4
		maxps		xmm1, xmm4

		movss		xmm5, [esi+eax+1*12+0]
		movhps		xmm5, [esi+eax+1*12+4]
		minps		xmm2, xmm5
		maxps		xmm3, xmm5

		movss		xmm6, [esi+eax+2*12+8]
		movhps		xmm6, [esi+eax+2*12+0]
		minps		xmm0, xmm6
		maxps		xmm1, xmm6

		movss		xmm7, [esi+eax+3*12+0]
		movhps		xmm7, [esi+eax+3*12+4]
		minps		xmm2, xmm7
		maxps		xmm3, xmm7

		add			eax, 4*12
		jl			loop4

	done4:
		mov			eax, count
		and			eax, 3
		jz			done1
		imul		eax, 12
		add			esi, eax
		neg			eax

	loop1:
		movss		xmm4, [esi+eax+0*12+8]
		movhps		xmm4, [esi+eax+0*12+0]
		minps		xmm0, xmm4
		maxps		xmm1, xmm4

		add			eax, 12
		jl			loop1

	done1:
		shufps		xmm2, xmm2, R_SHUFFLEPS( 3, 1, 0, 2 )
		shufps		xmm3, xmm3, R_SHUFFLEPS( 3, 1, 0, 2 )
		minps		xmm0, xmm2
		maxps		xmm1, xmm3
		mov			esi, min
		movhps		[esi], xmm0
		movss		[esi+8], xmm0
		mov			edi, max
		movhps		[edi], xmm1
		movss		[edi+8], xmm1
	}
}


/*
============
idSIMD_SSE::Clamp
============
*/
void VPCALL idSIMD_SSE::Clamp( float *dst, const float *src, const float min, const float max, const int count ) {
	int	i, pre, post;

	__asm
	{
		movss	xmm0,min
		movss	xmm1,max
		shufps	xmm0,xmm0,0
		shufps	xmm1,xmm1,0

		KFLOATINITDS( dst, src, count, pre, post )
		and		eax,15
		jne		lpNA
		jmp		lpA
		align	16
lpA:
		movaps	xmm2,[edx+ebx]
		movaps	xmm3,[edx+ebx+16]
		maxps	xmm2,xmm0
		maxps	xmm3,xmm0
		prefetchnta	[edx+ebx+64]
		minps	xmm2,xmm1
		minps	xmm3,xmm1
		movaps	[edi+ebx],xmm2
		movaps	[edi+ebx+16],xmm3
		add		ebx,16*2
		jl		lpA
		jmp		done

		align	16
lpNA:
		movups	xmm2,[edx+ebx]
		movups	xmm3,[edx+ebx+16]
		maxps	xmm2,xmm0
		maxps	xmm3,xmm0
		prefetchnta	[edx+ebx+64]
		minps	xmm2,xmm1
		minps	xmm3,xmm1
		movaps	[edi+ebx],xmm2
		movaps	[edi+ebx+16],xmm3
		add		ebx,16*2
		jl		lpNA
done:
	}

	for ( i = 0; i < pre; i++ ) {
		if ( src[i] < min )
			dst[i] = min;
		else if ( src[i] > max )
			dst[i] = max;
		else
			dst[i] = src[i];
	}

	for( i = count - post; i < count; i++ ) {
		if ( src[i] < min )
			dst[i] = min;
		else if ( src[i] > max )
			dst[i] = max;
		else
			dst[i] = src[i];
	}
}

/*
============
idSIMD_SSE::ClampMin
============
*/
void VPCALL idSIMD_SSE::ClampMin( float *dst, const float *src, const float min, const int count ) {
	int	i, pre, post;

	__asm
	{
		movss	xmm0,min
		shufps	xmm0,xmm0,0

		KFLOATINITDS( dst, src, count, pre, post )
		and		eax,15
		jne		lpNA
		jmp		lpA
		align	16
lpA:
		movaps	xmm2,[edx+ebx]
		movaps	xmm3,[edx+ebx+16]
		maxps	xmm2,xmm0
		prefetchnta	[edx+ebx+64]
		maxps	xmm3,xmm0
		movaps	[edi+ebx],xmm2
		movaps	[edi+ebx+16],xmm3
		add		ebx,16*2
		jl		lpA
		jmp		done

		align	16
lpNA:
		movups	xmm2,[edx+ebx]
		movups	xmm3,[edx+ebx+16]
		maxps	xmm2,xmm0
		prefetchnta	[edx+ebx+64]
		maxps	xmm3,xmm0
		movaps	[edi+ebx],xmm2
		movaps	[edi+ebx+16],xmm3
		add		ebx,16*2
		jl		lpNA
done:
	}

	for( i = 0; i < pre; i++ ) {
		if ( src[i] < min )
			dst[i] = min;
		else
			dst[i] = src[i];
	}
	for( i = count - post; i < count; i++ ) {
		if ( src[i] < min )
			dst[i] = min;
		else
			dst[i] = src[i];
	}
}

/*
============
idSIMD_SSE::ClampMax
============
*/
void VPCALL idSIMD_SSE::ClampMax( float *dst, const float *src, const float max, const int count ) {
	int	i, pre, post;

	__asm
	{
		movss	xmm1,max
		shufps	xmm1,xmm1,0

		KFLOATINITDS( dst, src, count, pre, post )
		and		eax,15
		jne		lpNA
		jmp		lpA
		align	16
lpA:
		movaps	xmm2,[edx+ebx]
		movaps	xmm3,[edx+ebx+16]
		minps	xmm2,xmm1
		prefetchnta	[edx+ebx+64]
		minps	xmm3,xmm1
		movaps	[edi+ebx],xmm2
		movaps	[edi+ebx+16],xmm3
		add		ebx,16*2
		jl		lpA
		jmp		done

		align	16
lpNA:
		movups	xmm2,[edx+ebx]
		movups	xmm3,[edx+ebx+16]
		minps	xmm2,xmm1
		prefetchnta	[edx+ebx+64]
		minps	xmm3,xmm1
		movaps	[edi+ebx],xmm2
		movaps	[edi+ebx+16],xmm3
		add		ebx,16*2
		jl		lpNA
done:
	}

	for( i = 0; i < pre; i++ ) {
		if ( src[i] > max )
			dst[i] = max;
		else
			dst[i] = src[i];
	}

	for( i = count - post; i < count; i++ ) {
		if ( src[i] > max )
			dst[i] = max;
		else
			dst[i] = src[i];
	}
}

/*
============
idSIMD_SSE::Zero16
============
*/
void VPCALL idSIMD_SSE::Zero16( float *dst, const int count ) {
	__asm {
		mov		edx, dst
		mov		eax, count
		add		eax, 3
		shr		eax, 2
		jz		doneZero16
		shl		eax, 4
		add		edx, eax
		neg		eax
		xorps	xmm0, xmm0
	loopZero16:
		movaps	[edx+eax], xmm0
		add		eax, 16
		jl		loopZero16
	doneZero16:
	}
}

/*
============
idSIMD_SSE::Negate16
============
*/
void VPCALL idSIMD_SSE::Negate16( float *dst, const int count ) {
	__asm {
		mov		edx, dst
		mov		eax, count
		add		eax, 3
		shr		eax, 2
		jz		doneNegate16
		shl		eax, 4
		add		edx, eax
		neg		eax
		movss	xmm0, SIMD_SP_signBitMask
		shufps	xmm0, xmm0, R_SHUFFLEPS( 0, 0, 0, 0 )
	loopNegate16:
		movaps	xmm1, [edx+eax]
		xorps	xmm1, xmm0
		movaps	[edx+eax], xmm1
		add		eax, 16
		jl		loopNegate16
	doneNegate16:
	}
}

/*
============
idSIMD_SSE::Copy16
============
*/
void VPCALL idSIMD_SSE::Copy16( float *dst, const float *src, const int count ) {
	__asm {
		mov		ecx, src
		mov		edx, dst
		mov		eax, count
		add		eax, 3
		shr		eax, 2
		jz		doneCopy16
		shl		eax, 4
		add		ecx, eax
		add		edx, eax
		neg		eax
	loopCopy16:
		movaps	xmm0, [ecx+eax]
		movaps	[edx+eax], xmm0
		add		eax, 16
		jl		loopCopy16
	doneCopy16:
	}
}

/*
============
idSIMD_SSE::Add16
============
*/
void VPCALL idSIMD_SSE::Add16( float *dst, const float *src1, const float *src2, const int count ) {
	__asm {
		mov		ecx, src1
		mov		edx, src2
		mov		esi, dst
		mov		eax, count
		add		eax, 3
		shr		eax, 2
		jz		doneAdd16
		shl		eax, 4
		add		esi, eax
		add		ecx, eax
		add		edx, eax
		neg		eax
	loopAdd16:
		movaps	xmm0, [ecx+eax]
		addps	xmm0, [edx+eax]
		movaps	[esi+eax], xmm0
		add		eax, 16
		jl		loopAdd16
	doneAdd16:
	}
}

/*
============
idSIMD_SSE::Sub16
============
*/
void VPCALL idSIMD_SSE::Sub16( float *dst, const float *src1, const float *src2, const int count ) {
	__asm {
		mov		ecx, src1
		mov		edx, src2
		mov		esi, dst
		mov		eax, count
		add		eax, 3
		shr		eax, 2
		jz		doneSub16
		shl		eax, 4
		add		esi, eax
		add		ecx, eax
		add		edx, eax
		neg		eax
	loopSub16:
		movaps	xmm0, [ecx+eax]
		subps	xmm0, [edx+eax]
		movaps	[esi+eax], xmm0
		add		eax, 16
		jl		loopSub16
	doneSub16:
	}
}

/*
============
idSIMD_SSE::Mul16
============
*/
void VPCALL idSIMD_SSE::Mul16( float *dst, const float *src1, const float constant, const int count ) {
	__asm {
		mov		ecx, dst
		mov		edx, src1
		mov		eax, count
		add		eax, 3
		shr		eax, 2
		jz		doneMulScalar16
		movss	xmm1, constant
		shl		eax, 4
		add		ecx, eax
		add		edx, eax
		neg		eax
		shufps	xmm1, xmm1, 0x00
	loopMulScalar16:
		movaps	xmm0, [edx+eax]
		mulps	xmm0, xmm1
		movaps	[ecx+eax], xmm0
		add		eax, 16
		jl		loopMulScalar16
	doneMulScalar16:
	}
}

/*
============
idSIMD_SSE::AddAssign16
============
*/
void VPCALL idSIMD_SSE::AddAssign16( float *dst, const float *src, const int count ) {
	__asm {
		mov		ecx, dst
		mov		edx, src
		mov		eax, count
		add		eax, 3
		shr		eax, 2
		jz		doneAddAssign16
		shl		eax, 4
		add		ecx, eax
		add		edx, eax
		neg		eax
	loopAddAssign16:
		movaps	xmm0, [ecx+eax]
		addps	xmm0, [edx+eax]
		movaps	[ecx+eax], xmm0
		add		eax, 16
		jl		loopAddAssign16
	doneAddAssign16:
	}
}

/*
============
idSIMD_SSE::SubAssign16
============
*/
void VPCALL idSIMD_SSE::SubAssign16( float *dst, const float *src, const int count ) {
	__asm {
		mov		ecx, dst
		mov		edx, src
		mov		eax, count
		add		eax, 3
		shr		eax, 2
		jz		doneSubAssign16
		shl		eax, 4
		add		ecx, eax
		add		edx, eax
		neg		eax
	loopSubAssign16:
		movaps	xmm0, [ecx+eax]
		subps	xmm0, [edx+eax]
		movaps	[ecx+eax], xmm0
		add		eax, 16
		jl		loopSubAssign16
	doneSubAssign16:
	}
}

/*
============
idSIMD_SSE::MulAssign16
============
*/
void VPCALL idSIMD_SSE::MulAssign16( float *dst, const float constant, const int count ) {
	__asm {
		mov		ecx, dst
		mov		eax, count
		add		eax, 3
		shr		eax, 2
		jz		doneMulAssign16
		movss	xmm1, constant
		shl		eax, 4
		add		ecx, eax
		neg		eax
		shufps	xmm1, xmm1, 0x00
	loopMulAssign16:
		movaps	xmm0, [ecx+eax]
		mulps	xmm0, xmm1
		movaps	[ecx+eax], xmm0
		add		eax, 16
		jl		loopMulAssign16
	doneMulAssign16:
	}
}

/*
============
idSIMD_SSE::MatX_MultiplyVecX

	optimizes the following matrix multiplications:

	NxN * Nx1
	Nx6 * 6x1
	6xN * Nx1

	with N in the range [1-6]
============
*/
void VPCALL idSIMD_SSE::MatX_MultiplyVecX( idVecX &dst, const idMatX &mat, const idVecX &vec ) {
#define STORE1( offset, reg1, reg2 )		\
	__asm movss		[eax+offset], reg1
#define STORE2LO( offset, reg1, reg2 )		\
	__asm movlps	[eax+offset], reg1
#define STORE2HI( offset, reg1, reg2 )		\
	__asm movhps	[eax+offset], reg1
#define STORE4( offset, reg1, reg2 )		\
	__asm movlps	[eax+offset], reg1		\
	__asm movhps	[eax+offset+8], reg1
#define STOREC		=

	int numRows;
	const float *mPtr, *vPtr;
	float *dstPtr;

	assert( vec.GetSize() >= mat.GetNumColumns() );
	assert( dst.GetSize() >= mat.GetNumRows() );

	mPtr = mat.ToFloatPtr();
	vPtr = vec.ToFloatPtr();
	dstPtr = dst.ToFloatPtr();
	numRows = mat.GetNumRows();
	switch( mat.GetNumColumns() ) {
		case 1: {
			switch( numRows ) {
				case 1: {		// 1x1 * 1x1
					__asm {
						mov			esi, vPtr
						mov			edi, mPtr
						mov			eax, dstPtr
						movss		xmm0, [esi]
						mulss		xmm0, [edi]
						STORE1( 0, xmm0, xmm1 )
					}
					return;
				}
				case 6: {		// 6x1 * 1x1
					__asm {
						mov			esi, vPtr
						mov			edi, mPtr
						mov			eax, dstPtr
						movss		xmm0, [esi]
						shufps		xmm0, xmm0, R_SHUFFLEPS( 0, 0, 0, 0 )
						movaps		xmm1, xmm0
						mulps		xmm0, [edi]
						mulps		xmm1, [edi+16]
						STORE4( 0, xmm0, xmm2 )
						STORE2LO( 16, xmm1, xmm2 )
					}
					return;
				}
				default: {
					for ( int i = 0; i < numRows; i++ ) {
						dstPtr[i] STOREC mPtr[0] * vPtr[0];
						mPtr++;
					}
					return;
				}
			}
			break;
		}
		case 2: {
			switch( numRows ) {
				case 2: {		// 2x2 * 2x1
					__asm {
						mov			esi, vPtr
						mov			edi, mPtr
						mov			eax, dstPtr
						movss		xmm0, [esi]
						movss		xmm1, [esi+4]
						movss		xmm2, [edi]
						mulss		xmm2, xmm0
						movss		xmm3, [edi+4]
						mulss		xmm3, xmm1
						addss		xmm2, xmm3
						STORE1( 0, xmm2, xmm4 )
						mulss		xmm0, [edi+8]
						mulss		xmm1, [edi+8+4]
						addss		xmm0, xmm1
						STORE1( 4, xmm0, xmm4 )
					}
					return;
				}
				case 6: {		// 6x2 * 2x1
					__asm {
						mov			esi, vPtr
						mov			edi, mPtr
						mov			eax, dstPtr
						movlps		xmm7, [esi]
						shufps		xmm7, xmm7, R_SHUFFLEPS( 0, 1, 0, 1 )
						movaps		xmm0, [edi]
						mulps		xmm0, xmm7
						movaps		xmm1, [edi+16]
						mulps		xmm1, xmm7
						movaps		xmm2, xmm0
						shufps		xmm0, xmm1, R_SHUFFLEPS( 0, 2, 0, 2 )
						shufps		xmm2, xmm1, R_SHUFFLEPS( 1, 3, 1, 3 )
						movaps		xmm3, [edi+32]
						addps		xmm0, xmm2
						mulps		xmm3, xmm7
						STORE4( 0, xmm0, xmm4 )
						shufps		xmm3, xmm3, R_SHUFFLEPS( 0, 2, 1, 3 )
						movhlps		xmm1, xmm3
						addps		xmm3, xmm1
						STORE2LO( 16, xmm3, xmm4 )
					}
					return;
				}
				default: {
					for ( int i = 0; i < numRows; i++ ) {
						dstPtr[i] STOREC mPtr[0] * vPtr[0] + mPtr[1] * vPtr[1];
						mPtr += 2;
					}
					return;
				}
			}
			break;
		}
		case 3: {
			switch( numRows ) {
				case 3: {		// 3x3 * 3x1
					__asm {
						mov			esi, vPtr
						mov			edi, mPtr
						mov			eax, dstPtr
						movss		xmm0, [esi]
						movss		xmm4, [edi]
						mulss		xmm4, xmm0
						movss		xmm1, [esi+4]
						movss		xmm5, [edi+4]
						mulss		xmm5, xmm1
						addss		xmm4, xmm5
						movss		xmm2, [esi+8]
						movss		xmm6, [edi+8]
						mulss		xmm6, xmm2
						addss		xmm4, xmm6
						movss		xmm3, [edi+12]
						mulss		xmm3, xmm0
						STORE1( 0, xmm4, xmm7 );
						movss		xmm5, [edi+12+4]
						mulss		xmm5, xmm1
						addss		xmm3, xmm5
						movss		xmm6, [edi+12+8]
						mulss		xmm6, xmm2
						addss		xmm3, xmm6
						mulss		xmm0, [edi+24]
						mulss		xmm1, [edi+24+4]
						STORE1( 4, xmm3, xmm7 );
						addss		xmm0, xmm1
						mulss		xmm2, [edi+24+8]
						addss		xmm0, xmm2
						STORE1( 8, xmm0, xmm7 );
					}
					return;
				}
				case 6: {		// 6x3 * 3x1
					__asm {
						mov			esi, vPtr
						mov			edi, mPtr
						mov			eax, dstPtr
						movss		xmm5, [esi]
						shufps		xmm5, xmm5, R_SHUFFLEPS( 0, 0, 0, 0 )
						movss		xmm6, [esi+4]
						shufps		xmm6, xmm6, R_SHUFFLEPS( 0, 0, 0, 0 )
						movss		xmm7, [esi+8]
						shufps		xmm7, xmm7, R_SHUFFLEPS( 0, 0, 0, 0 )
						movaps		xmm0, [edi]								// xmm0 = 0, 1, 2, 3
						movlps		xmm1, [edi+4*4]
						shufps		xmm1, xmm0, R_SHUFFLEPS( 0, 1, 1, 2 )	// xmm1 = 4, 5, 1, 2
						movlps		xmm2, [edi+6*4]
						movhps		xmm2, [edi+8*4]							// xmm2 = 6, 7, 8, 9
						shufps		xmm0, xmm2, R_SHUFFLEPS( 0, 3, 0, 3 )	// xmm0 = 0, 3, 6, 9
						mulps		xmm0, xmm5
						movlps		xmm3, [edi+10*4]
						shufps		xmm2, xmm3, R_SHUFFLEPS( 1, 2, 0, 1 )	// xmm2 = 7, 8, 10, 11
						movaps		xmm3, xmm1
						shufps		xmm1, xmm2, R_SHUFFLEPS( 2, 0, 0, 2 )	// xmm1 = 1, 4, 7, 10
						mulps		xmm1, xmm6
						shufps		xmm3, xmm2, R_SHUFFLEPS( 3, 1, 1, 3 )	// xmm3 = 2, 5, 8, 11
						mulps		xmm3, xmm7
						addps		xmm0, xmm1
						addps		xmm0, xmm3
						STORE4( 0, xmm0, xmm4 )
						movss		xmm1, [edi+12*4]
						mulss		xmm1, xmm5
						movss		xmm2, [edi+13*4]
						mulss		xmm2, xmm6
						movss		xmm3, [edi+14*4]
						mulss		xmm3, xmm7
						addss		xmm1, xmm2
						addss		xmm1, xmm3
						STORE1( 16, xmm1, xmm4 )
						mulss		xmm5, [edi+15*4]
						mulss		xmm6, [edi+16*4]
						mulss		xmm7, [edi+17*4]
						addss		xmm5, xmm6
						addss		xmm5, xmm7
						STORE1( 20, xmm5, xmm4 )
					}
					return;
				}
				default: {
					for ( int i = 0; i < numRows; i++ ) {
						dstPtr[i] STOREC mPtr[0] * vPtr[0] + mPtr[1] * vPtr[1] + mPtr[2] * vPtr[2];
						mPtr += 3;
					}
					return;
				}
			}
			break;
		}
		case 4: {
			switch( numRows ) {
				case 4: {		// 4x4 * 4x1
					__asm {
						mov			esi, vPtr
						mov			edi, mPtr
						mov			eax, dstPtr
						movlps		xmm6, qword ptr [esi ]
						movlps		xmm0, qword ptr [edi ]
						shufps		xmm6, xmm6, R_SHUFFLEPS( 0, 1, 0, 1 )
						movhps		xmm0, qword ptr [edi+16]
						mulps		xmm0, xmm6
						movlps		xmm7, qword ptr [esi+ 8]
						movlps		xmm2, qword ptr [edi+ 8]
						shufps		xmm7, xmm7, R_SHUFFLEPS( 0, 1, 0, 1 )
						movhps		xmm2, qword ptr [edi+24]
						mulps		xmm2, xmm7
						movlps		xmm1, qword ptr [edi+32]
						movhps		xmm1, qword ptr [edi+48]
						mulps		xmm1, xmm6
						movlps		xmm3, qword ptr [edi+40]
						addps		xmm0, xmm2
						movhps		xmm3, qword ptr [edi+56]
						mulps		xmm3, xmm7
						movaps		xmm4, xmm0
						addps		xmm1, xmm3
						shufps		xmm4, xmm1, R_SHUFFLEPS( 0, 2, 0, 2 )
						shufps		xmm0, xmm1, R_SHUFFLEPS( 1, 3, 1, 3 )
						addps		xmm0, xmm4
						STORE4( 0, xmm0, xmm2 )
					}
					return;
				}
				case 6: {		// 6x4 * 4x1
					__asm {
						mov			esi, vPtr
						mov			edi, mPtr
						mov			eax, dstPtr
						movlps		xmm6, qword ptr [esi+ 0]
						movlps		xmm0, qword ptr [edi+ 0]
						shufps		xmm6, xmm6, R_SHUFFLEPS( 0, 1, 0, 1 )
						movhps		xmm0, qword ptr [edi+16]
						mulps		xmm0, xmm6
						movlps		xmm7, qword ptr [esi+ 8]
						movlps		xmm2, qword ptr [edi+ 8]
						shufps		xmm7, xmm7, R_SHUFFLEPS( 0, 1, 0, 1 )
						movhps		xmm2, qword ptr [edi+24]
						mulps		xmm2, xmm7
						movlps		xmm1, qword ptr [edi+32]
						movhps		xmm1, qword ptr [edi+48]
						mulps		xmm1, xmm6
						movlps		xmm3, qword ptr [edi+40]
						addps		xmm0, xmm2
						movhps		xmm3, qword ptr [edi+56]
						mulps		xmm3, xmm7
						movaps		xmm4, xmm0
						addps		xmm1, xmm3
						shufps		xmm4, xmm1, R_SHUFFLEPS( 0, 2, 0, 2 )
						shufps		xmm0, xmm1, R_SHUFFLEPS( 1, 3, 1, 3 )
						addps		xmm0, xmm4
						movlps		xmm1, qword ptr [edi+64]
						movhps		xmm1, qword ptr [edi+80]
						STORE4( 0, xmm0, xmm4 )
						mulps		xmm1, xmm6
						movlps		xmm2, qword ptr [edi+72]
						movhps		xmm2, qword ptr [edi+88]
						mulps		xmm2, xmm7
						addps		xmm1, xmm2
						shufps		xmm1, xmm1, R_SHUFFLEPS( 0, 2, 1, 3 )
						movhlps		xmm3, xmm1
						addps		xmm1, xmm3
						STORE2LO( 16, xmm1, xmm4 )
					}
					return;
				}
				default: {
					for ( int i = 0; i < numRows; i++ ) {
						dstPtr[i] STOREC mPtr[0] * vPtr[0] + mPtr[1] * vPtr[1] + mPtr[2] * vPtr[2] + mPtr[3] * vPtr[3];
						mPtr += 4;
					}
					return;
				}
			}
			break;
		}
		case 5: {
			switch( numRows ) {
				case 5: {		// 5x5 * 5x1
					__asm {
						mov			esi, vPtr
						mov			edi, mPtr
						mov			eax, dstPtr
						movss		xmm0, [edi+5*4]							// xmm0 =  5,  X,  X,  X
						movhps		xmm0, [edi+0*4]							// xmm0 =  5,  X,  0,  1
						movss		xmm5, [edi+15*4]						// xmm4 = 15,  X,  X,  X
						movhps		xmm5, [edi+10*4]						// xmm5 = 15,  X, 10, 11
						movaps		xmm1, xmm0								// xmm1 =  5,  X,  0,  1
						shufps		xmm0, xmm5, R_SHUFFLEPS( 2, 0, 2, 0 )	// xmm0 =  0,  5, 10, 15
						movlps		xmm1, [edi+6*4]							// xmm1 =  6,  7,  0,  1
						movlps		xmm5, [edi+16*4]						// xmm5 = 16, 17, 10, 11
						movaps		xmm2, xmm1								// xmm2 =  6,  7,  0,  1
						shufps		xmm1, xmm5, R_SHUFFLEPS( 3, 0, 3, 0 )	// xmm1 =  1,  6, 11, 16
						movhps		xmm2, [edi+2*4]							// xmm2 =  6,  7,  2,  3
						movhps		xmm5, [edi+12*4]						// xmm5 = 16, 17, 12, 13
						movaps		xmm3, xmm2								// xmm3 =  6,  7,  2,  3
						shufps		xmm2, xmm5, R_SHUFFLEPS( 2, 1, 2, 1 )	// xmm2 =  2,  7, 12, 17
						movlps		xmm3, [edi+8*4]							// xmm3 =  8,  9,  2,  3
						movlps		xmm5, [edi+18*4]						// xmm5 = 18, 19, 12, 13
						movss		xmm4, [edi+4*4]							// xmm4 =  4,  X,  X,  X
						movlhps		xmm4, xmm3								// xmm4 =  4,  X,  8,  9
						shufps		xmm3, xmm5, R_SHUFFLEPS( 3, 0, 3, 0 )	// xmm3 =  3,  8, 13, 18
						movhps		xmm5, [edi+14*4]						// xmm6 = 18, 19, 14, 15
						shufps		xmm4, xmm5, R_SHUFFLEPS( 0, 3, 2, 1 )	// xmm4 =  4,  9, 14, 19
						movss		xmm7, [esi+0*4]
						shufps		xmm7, xmm7, 0
						mulps		xmm0, xmm7
						movss		xmm5, [esi+1*4]
						shufps		xmm5, xmm5, 0
						mulps		xmm1, xmm5
						addps		xmm0, xmm1
						movss		xmm6, [esi+2*4]
						shufps		xmm6, xmm6, 0
						mulps		xmm2, xmm6
						addps		xmm0, xmm2
						movss		xmm1, [esi+3*4]
						shufps		xmm1, xmm1, 0
						mulps		xmm3, xmm1
						addps		xmm0, xmm3
						movss		xmm2, [esi+4*4]
						shufps		xmm2, xmm2, 0
						mulps		xmm4, xmm2
						addps		xmm0, xmm4
						mulss		xmm7, [edi+20*4]
						mulss		xmm5, [edi+21*4]
						addps		xmm7, xmm5
						mulss		xmm6, [edi+22*4]
						addps		xmm7, xmm6
						mulss		xmm1, [edi+23*4]
						addps		xmm7, xmm1
						mulss		xmm2, [edi+24*4]
						addps		xmm7, xmm2
						STORE4( 0, xmm0, xmm3 )
						STORE1( 16, xmm7, xmm4 )
					}
					return;
				}
				case 6: {		// 6x5 * 5x1
					__asm {
						mov			esi, vPtr
						mov			edi, mPtr
						mov			eax, dstPtr
						movlps		xmm6, [esi]
						shufps		xmm6, xmm6, R_SHUFFLEPS( 0, 1, 0, 1 )
						movlps		xmm7, [esi+8]
						shufps		xmm7, xmm7, R_SHUFFLEPS( 0, 1, 0, 1 )
						movlps		xmm0, [edi]
						movhps		xmm3, [edi+8]
						movaps		xmm1, [edi+16]
						movlps		xmm2, [edi+32]
						shufps		xmm0, xmm1, R_SHUFFLEPS( 0, 1, 1, 2 )	// xmm0 = 0, 1, 5, 6
						shufps		xmm1, xmm2, R_SHUFFLEPS( 0, 3, 0, 1 )	// xmm1 = 4, 7, 8, 9
						shufps		xmm3, xmm1, R_SHUFFLEPS( 2, 3, 1, 2 )	// xmm3 = 2, 3, 7, 8
						mulps		xmm0, xmm6
						mulps		xmm3, xmm7
						movlps		xmm2, [edi+40]
						addps		xmm0, xmm3								// xmm0 + xmm1
						movhps		xmm5, [edi+40+8]
						movlps		xmm3, [edi+40+16]
						movhps		xmm3, [edi+40+24]
						movlps		xmm4, [edi+40+32]
						shufps		xmm2, xmm3, R_SHUFFLEPS( 0, 1, 1, 2 )	// xmm2 = 10, 11, 15, 16
						shufps		xmm3, xmm4, R_SHUFFLEPS( 0, 3, 0, 1 )	// xmm3 = 14, 17, 18, 19
						shufps		xmm5, xmm3, R_SHUFFLEPS( 2, 3, 1, 2 )	// xmm5 = 12, 13, 17, 18
						mulps		xmm2, xmm6
						mulps		xmm5, xmm7
						addps		xmm2, xmm5								// xmm2 + xmm3
						movss		xmm5, [esi+16]
						shufps		xmm5, xmm5, R_SHUFFLEPS( 0, 0, 0, 0 )
						movaps		xmm4, xmm0
						shufps		xmm0, xmm2, R_SHUFFLEPS( 0, 2, 0, 2 )
						shufps		xmm4, xmm2, R_SHUFFLEPS( 1, 3, 1, 3 )
						shufps		xmm1, xmm3, R_SHUFFLEPS( 0, 3, 0, 3 )
						addps		xmm0, xmm4
						mulps		xmm1, xmm5
						addps		xmm0, xmm1
						STORE4( 0, xmm0, xmm2 )
						movlps		xmm4, [edi+80]
						movhps		xmm3, [edi+80+8]
						movaps		xmm1, [edi+80+16]
						movlps		xmm2, [edi+80+32]
						shufps		xmm4, xmm1, R_SHUFFLEPS( 0, 1, 1, 2 )	// xmm4 = 20, 21, 25, 26
						shufps		xmm1, xmm2, R_SHUFFLEPS( 0, 3, 0, 1 )	// xmm1 = 24, 27, 28, 29
						shufps		xmm3, xmm1, R_SHUFFLEPS( 2, 3, 1, 2 )	// xmm3 = 22, 23, 27, 28
						mulps		xmm4, xmm6
						mulps		xmm3, xmm7
						mulps		xmm1, xmm5
						addps		xmm4, xmm3								// xmm4 + xmm1
						shufps		xmm1, xmm4, R_SHUFFLEPS( 0, 3, 0, 2 )
						shufps		xmm4, xmm4, R_SHUFFLEPS( 1, 3, 0, 0 )
						addps		xmm4, xmm1
						shufps		xmm1, xmm1, R_SHUFFLEPS( 2, 3, 0, 1 )
						addps		xmm4, xmm1
						STORE2LO( 16, xmm4, xmm2 )
					}
					return;
				}
				default: {
					for ( int i = 0; i < numRows; i++ ) {
						dstPtr[i] STOREC mPtr[0] * vPtr[0] + mPtr[1] * vPtr[1] + mPtr[2] * vPtr[2] + mPtr[3] * vPtr[3] + mPtr[4] * vPtr[4];
						mPtr += 5;
					}
					return;
				}
			}
			break;
		}
		case 6: {
			switch( numRows ) {
				case 1: {		// 1x6 * 6x1
					__asm {
						mov			esi, vPtr
						mov			edi, mPtr
						mov			eax, dstPtr
						movss		xmm0, [esi]
						mulss		xmm0, [edi]
						movss		xmm1, [esi+4]
						mulss		xmm1, [edi+4]
						movss		xmm2, [esi+8]
						addss		xmm0, xmm1
						mulss		xmm2, [edi+8]
						movss		xmm3, [esi+12]
						addss		xmm0, xmm2
						mulss		xmm3, [edi+12]
						movss		xmm4, [esi+16]
						addss		xmm0, xmm3
						mulss		xmm4, [edi+16]
						movss		xmm5, [esi+20]
						addss		xmm0, xmm4
						mulss		xmm5, [edi+20]
						movss		xmm6, [esi+24]
						addss		xmm0, xmm5
						mulss		xmm6, [edi+24]
						addss		xmm0, xmm6
						STORE1( 0, xmm0, xmm7 )
					}
					return;
				}
				case 2: {		// 2x6 * 6x1
					__asm {
						mov			esi, vPtr
						mov			edi, mPtr
						mov			eax, dstPtr
						// load idVecX
						movlps		xmm4, [esi]
						movhps		xmm4, [esi+8]
						movlps		xmm5, [esi+16]
						movlhps		xmm5, xmm4
						movhlps		xmm6, xmm4
						movlhps		xmm6, xmm5
						// row 0 and 1
						movaps		xmm0, [edi]
						movaps		xmm1, [edi+16]
						movaps		xmm2, [edi+32]
						mulps		xmm0, xmm4
						mulps		xmm1, xmm5
						mulps		xmm2, xmm6
						movhlps		xmm3, xmm0
						movlhps		xmm3, xmm2
						addps		xmm1, xmm3
						shufps		xmm0, xmm2, R_SHUFFLEPS( 0, 1, 2, 3 )
						addps		xmm1, xmm0
						shufps		xmm1, xmm1, R_SHUFFLEPS( 0, 2, 1, 3 )
						movhlps		xmm0, xmm1
						addps		xmm0, xmm1
						STORE2LO( 0, xmm0, xmm3 )
					}
					return;
				}
				case 3: {		// 3x6 * 6x1
					__asm {
						mov			esi, vPtr
						mov			edi, mPtr
						mov			eax, dstPtr
						// load idVecX
						movlps		xmm4, [esi]
						movhps		xmm4, [esi+8]
						movlps		xmm5, [esi+16]
						movlhps		xmm5, xmm4
						movhlps		xmm6, xmm4
						movlhps		xmm6, xmm5
						// row 0 and 1
						movaps		xmm0, [edi]
						movaps		xmm1, [edi+16]
						movaps		xmm2, [edi+32]
						mulps		xmm0, xmm4
						mulps		xmm1, xmm5
						mulps		xmm2, xmm6
						movhlps		xmm3, xmm0
						movlhps		xmm3, xmm2
						addps		xmm1, xmm3
						shufps		xmm0, xmm2, R_SHUFFLEPS( 0, 1, 2, 3 )
						addps		xmm1, xmm0
						shufps		xmm1, xmm1, R_SHUFFLEPS( 0, 2, 1, 3 )
						movhlps		xmm0, xmm1
						addps		xmm0, xmm1
						STORE2LO( 0, xmm0, xmm3 )
						// row 2
						movaps		xmm0, [edi+48]
						movaps		xmm1, [edi+48+16]
						mulps		xmm0, xmm4
						mulps		xmm1, xmm5
						addps		xmm0, xmm1
						movhlps		xmm1, xmm0
						addps		xmm0, xmm1
						movaps		xmm1, xmm0
						shufps		xmm1, xmm1, R_SHUFFLEPS( 1, 0, 0, 0 )
						addss		xmm0, xmm1
						STORE1( 8, xmm0, xmm3 )
					}
					return;
				}
				case 4: {		// 4x6 * 6x1
					__asm {
						mov			esi, vPtr
						mov			edi, mPtr
						mov			eax, dstPtr
						// load idVecX
						movlps		xmm4, [esi]
						movhps		xmm4, [esi+8]
						movlps		xmm5, [esi+16]
						movlhps		xmm5, xmm4
						movhlps		xmm6, xmm4
						movlhps		xmm6, xmm5
						// row 0 and 1
						movaps		xmm0, [edi]
						movaps		xmm1, [edi+16]
						movaps		xmm2, [edi+32]
						mulps		xmm0, xmm4
						mulps		xmm1, xmm5
						mulps		xmm2, xmm6
						movhlps		xmm7, xmm0
						movlhps		xmm7, xmm2
						addps		xmm7, xmm1
						shufps		xmm0, xmm2, R_SHUFFLEPS( 0, 1, 2, 3 )
						addps		xmm7, xmm0
						// row 2 and 3
						movaps		xmm0, [edi+48]
						movaps		xmm1, [edi+48+16]
						movaps		xmm2, [edi+48+32]
						mulps		xmm0, xmm4
						mulps		xmm1, xmm5
						mulps		xmm2, xmm6
						movhlps		xmm3, xmm0
						movlhps		xmm3, xmm2
						addps		xmm1, xmm3
						shufps		xmm0, xmm2, R_SHUFFLEPS( 0, 1, 2, 3 )
						addps		xmm1, xmm0
						// last 4 additions for the first 4 rows and store result
						movaps		xmm0, xmm7
						shufps		xmm7, xmm1, R_SHUFFLEPS( 0, 2, 0, 2 )
						shufps		xmm0, xmm1, R_SHUFFLEPS( 1, 3, 1, 3 )
						addps		xmm0, xmm7
						STORE4( 0, xmm0, xmm4 )
					}
					return;
				}
				case 5: {		// 5x6 * 6x1
					__asm {
						mov			esi, vPtr
						mov			edi, mPtr
						mov			eax, dstPtr
						// load idVecX
						movlps		xmm4, [esi]
						movhps		xmm4, [esi+8]
						movlps		xmm5, [esi+16]
						movlhps		xmm5, xmm4
						movhlps		xmm6, xmm4
						movlhps		xmm6, xmm5
						// row 0 and 1
						movaps		xmm0, [edi]
						movaps		xmm1, [edi+16]
						movaps		xmm2, [edi+32]
						mulps		xmm0, xmm4
						mulps		xmm1, xmm5
						mulps		xmm2, xmm6
						movhlps		xmm7, xmm0
						movlhps		xmm7, xmm2
						addps		xmm7, xmm1
						shufps		xmm0, xmm2, R_SHUFFLEPS( 0, 1, 2, 3 )
						addps		xmm7, xmm0
						// row 2 and 3
						movaps		xmm0, [edi+48]
						movaps		xmm1, [edi+48+16]
						movaps		xmm2, [edi+48+32]
						mulps		xmm0, xmm4
						mulps		xmm1, xmm5
						mulps		xmm2, xmm6
						movhlps		xmm3, xmm0
						movlhps		xmm3, xmm2
						addps		xmm1, xmm3
						shufps		xmm0, xmm2, R_SHUFFLEPS( 0, 1, 2, 3 )
						addps		xmm1, xmm0
						// last 4 additions for the first 4 rows and store result
						movaps		xmm0, xmm7
						shufps		xmm7, xmm1, R_SHUFFLEPS( 0, 2, 0, 2 )
						shufps		xmm0, xmm1, R_SHUFFLEPS( 1, 3, 1, 3 )
						addps		xmm0, xmm7
						STORE4( 0, xmm0, xmm3 )
						// row 5
						movaps		xmm0, [edi+96]
						movaps		xmm1, [edi+96+16]
						mulps		xmm0, xmm4
						mulps		xmm1, xmm5
						addps		xmm0, xmm1
						movhlps		xmm1, xmm0
						addps		xmm0, xmm1
						movaps		xmm1, xmm0
						shufps		xmm1, xmm1, 0x01
						addss		xmm0, xmm1
						STORE1( 16, xmm0, xmm3 )
					}
					return;
				}
				case 6: {		// 6x6 * 6x1
					__asm {
						mov			esi, vPtr
						mov			edi, mPtr
						mov			eax, dstPtr
						movlps		xmm7, qword ptr [esi]
						movlps		xmm6, qword ptr [esi+8]
						shufps		xmm7, xmm7, 0x44
						shufps		xmm6, xmm6, 0x44
						movlps		xmm0, qword ptr [edi    ]
						movhps		xmm0, qword ptr [edi+ 24]
						mulps		xmm0, xmm7
						movlps		xmm3, qword ptr [edi+  8]
						movhps		xmm3, qword ptr [edi+ 32]
						mulps		xmm3, xmm6
						movlps		xmm1, qword ptr [edi+ 48]
						movhps		xmm1, qword ptr [edi+ 72]
						mulps		xmm1, xmm7
						movlps		xmm2, qword ptr [edi+ 96]
						movhps		xmm2, qword ptr [edi+120]
						mulps		xmm2, xmm7
						movlps		xmm4, qword ptr [edi+ 56]
						movhps		xmm4, qword ptr [edi+ 80]
						movlps		xmm5, qword ptr [edi+104]
						movhps		xmm5, qword ptr [edi+128]
						mulps		xmm4, xmm6
						movlps		xmm7, qword ptr [esi+16]
						addps		xmm0, xmm3
						shufps		xmm7, xmm7, 0x44
						mulps		xmm5, xmm6
						addps		xmm1, xmm4
						movlps		xmm3, qword ptr [edi+ 16]
						movhps		xmm3, qword ptr [edi+ 40]
						addps		xmm2, xmm5
						movlps		xmm4, qword ptr [edi+ 64]
						movhps		xmm4, qword ptr [edi+ 88]
						mulps		xmm3, xmm7
						movlps		xmm5, qword ptr [edi+112]
						movhps		xmm5, qword ptr [edi+136]
						addps		xmm0, xmm3
						mulps		xmm4, xmm7
						mulps		xmm5, xmm7
						addps		xmm1, xmm4
						addps		xmm2, xmm5
						movaps		xmm6, xmm0
						shufps		xmm0, xmm1, 0x88
						shufps		xmm6, xmm1, 0xDD
						movaps		xmm7, xmm2
						shufps		xmm7, xmm2, 0x88
						shufps		xmm2, xmm2, 0xDD
						addps		xmm0, xmm6
						addps		xmm2, xmm7
						STORE4( 0, xmm0, xmm3 )
						STORE2LO( 16, xmm2, xmm4 )
					}
					return;
				}
				default: {
					for ( int i = 0; i < numRows; i++ ) {
						dstPtr[i] STOREC mPtr[0] * vPtr[0] + mPtr[1] * vPtr[1] + mPtr[2] * vPtr[2] +
									mPtr[3] * vPtr[3] + mPtr[4] * vPtr[4] + mPtr[5] * vPtr[5];
						mPtr += 6;
					}
					return;
				}
			}
			break;
		}
		default: {
			int numColumns = mat.GetNumColumns();
			for ( int i = 0; i < numRows; i++ ) {
				float sum = mPtr[0] * vPtr[0];
				for ( int j = 1; j < numColumns; j++ ) {
					sum += mPtr[j] * vPtr[j];
				}
				dstPtr[i] STOREC sum;
				mPtr += numColumns;
			}
			break;
		}
	}

#undef STOREC
#undef STORE4
#undef STORE2HI
#undef STORE2LO
#undef STORE1
}

/*
============
idSIMD_SSE::MatX_MultiplyAddVecX

	optimizes the following matrix multiplications:

	NxN * Nx1
	Nx6 * 6x1
	6xN * Nx1

	with N in the range [1-6]
============
*/
void VPCALL idSIMD_SSE::MatX_MultiplyAddVecX( idVecX &dst, const idMatX &mat, const idVecX &vec ) {
#define STORE1( offset, reg1, reg2 )		\
	__asm movss		reg2, [eax+offset]		\
	__asm addss		reg2, reg1				\
	__asm movss		[eax+offset], reg2
#define STORE2LO( offset, reg1, reg2 )		\
	__asm movlps	reg2, [eax+offset]		\
	__asm addps		reg2, reg1				\
	__asm movlps	[eax+offset], reg2
#define STORE2HI( offset, reg1, reg2 )		\
	__asm movhps	reg2, [eax+offset]		\
	__asm addps		reg2, reg1				\
	__asm movhps	[eax+offset], reg2
#define STORE4( offset, reg1, reg2 )		\
	__asm movlps	reg2, [eax+offset]		\
	__asm movhps	reg2, [eax+offset+8]	\
	__asm addps		reg2, reg1				\
	__asm movlps	[eax+offset], reg2		\
	__asm movhps	[eax+offset+8], reg2
#define STOREC		+=

	int numRows;
	const float *mPtr, *vPtr;
	float *dstPtr;

	assert( vec.GetSize() >= mat.GetNumColumns() );
	assert( dst.GetSize() >= mat.GetNumRows() );

	mPtr = mat.ToFloatPtr();
	vPtr = vec.ToFloatPtr();
	dstPtr = dst.ToFloatPtr();
	numRows = mat.GetNumRows();
	switch( mat.GetNumColumns() ) {
		case 1: {
			switch( numRows ) {
				case 1: {		// 1x1 * 1x1
					__asm {
						mov			esi, vPtr
						mov			edi, mPtr
						mov			eax, dstPtr
						movss		xmm0, [esi]
						mulss		xmm0, [edi]
						STORE1( 0, xmm0, xmm1 )
					}
					return;
				}
				case 6: {		// 6x1 * 1x1
					__asm {
						mov			esi, vPtr
						mov			edi, mPtr
						mov			eax, dstPtr
						movss		xmm0, [esi]
						shufps		xmm0, xmm0, R_SHUFFLEPS( 0, 0, 0, 0 )
						movaps		xmm1, xmm0
						mulps		xmm0, [edi]
						mulps		xmm1, [edi+16]
						STORE4( 0, xmm0, xmm2 )
						STORE2LO( 16, xmm1, xmm2 )
					}
					return;
				}
				default: {
					for ( int i = 0; i < numRows; i++ ) {
						dstPtr[i] STOREC mPtr[0] * vPtr[0];
						mPtr++;
					}
					return;
				}
			}
			break;
		}
		case 2: {
			switch( numRows ) {
				case 2: {		// 2x2 * 2x1
					__asm {
						mov			esi, vPtr
						mov			edi, mPtr
						mov			eax, dstPtr
						movss		xmm0, [esi]
						movss		xmm1, [esi+4]
						movss		xmm2, [edi]
						mulss		xmm2, xmm0
						movss		xmm3, [edi+4]
						mulss		xmm3, xmm1
						addss		xmm2, xmm3
						STORE1( 0, xmm2, xmm4 )
						mulss		xmm0, [edi+8]
						mulss		xmm1, [edi+8+4]
						addss		xmm0, xmm1
						STORE1( 4, xmm0, xmm4 )
					}
					return;
				}
				case 6: {		// 6x2 * 2x1
					__asm {
						mov			esi, vPtr
						mov			edi, mPtr
						mov			eax, dstPtr
						movlps		xmm7, [esi]
						shufps		xmm7, xmm7, R_SHUFFLEPS( 0, 1, 0, 1 )
						movaps		xmm0, [edi]
						mulps		xmm0, xmm7
						movaps		xmm1, [edi+16]
						mulps		xmm1, xmm7
						movaps		xmm2, xmm0
						shufps		xmm0, xmm1, R_SHUFFLEPS( 0, 2, 0, 2 )
						shufps		xmm2, xmm1, R_SHUFFLEPS( 1, 3, 1, 3 )
						movaps		xmm3, [edi+32]
						addps		xmm0, xmm2
						mulps		xmm3, xmm7
						STORE4( 0, xmm0, xmm4 )
						shufps		xmm3, xmm3, R_SHUFFLEPS( 0, 2, 1, 3 )
						movhlps		xmm1, xmm3
						addps		xmm3, xmm1
						STORE2LO( 16, xmm3, xmm4 )
					}
					return;
				}
				default: {
					for ( int i = 0; i < numRows; i++ ) {
						dstPtr[i] STOREC mPtr[0] * vPtr[0] + mPtr[1] * vPtr[1];
						mPtr += 2;
					}
					return;
				}
			}
			break;
		}
		case 3: {
			switch( numRows ) {
				case 3: {		// 3x3 * 3x1
					__asm {
						mov			esi, vPtr
						mov			edi, mPtr
						mov			eax, dstPtr
						movss		xmm0, [esi]
						movss		xmm4, [edi]
						mulss		xmm4, xmm0
						movss		xmm1, [esi+4]
						movss		xmm5, [edi+4]
						mulss		xmm5, xmm1
						addss		xmm4, xmm5
						movss		xmm2, [esi+8]
						movss		xmm6, [edi+8]
						mulss		xmm6, xmm2
						addss		xmm4, xmm6
						movss		xmm3, [edi+12]
						mulss		xmm3, xmm0
						STORE1( 0, xmm4, xmm7 );
						movss		xmm5, [edi+12+4]
						mulss		xmm5, xmm1
						addss		xmm3, xmm5
						movss		xmm6, [edi+12+8]
						mulss		xmm6, xmm2
						addss		xmm3, xmm6
						mulss		xmm0, [edi+24]
						mulss		xmm1, [edi+24+4]
						STORE1( 4, xmm3, xmm7 );
						addss		xmm0, xmm1
						mulss		xmm2, [edi+24+8]
						addss		xmm0, xmm2
						STORE1( 8, xmm0, xmm7 );
					}
					return;
				}
				case 6: {		// 6x3 * 3x1
					__asm {
						mov			esi, vPtr
						mov			edi, mPtr
						mov			eax, dstPtr
						movss		xmm5, [esi]
						shufps		xmm5, xmm5, R_SHUFFLEPS( 0, 0, 0, 0 )
						movss		xmm6, [esi+4]
						shufps		xmm6, xmm6, R_SHUFFLEPS( 0, 0, 0, 0 )
						movss		xmm7, [esi+8]
						shufps		xmm7, xmm7, R_SHUFFLEPS( 0, 0, 0, 0 )
						movaps		xmm0, [edi]								// xmm0 = 0, 1, 2, 3
						movlps		xmm1, [edi+4*4]
						shufps		xmm1, xmm0, R_SHUFFLEPS( 0, 1, 1, 2 )	// xmm1 = 4, 5, 1, 2
						movlps		xmm2, [edi+6*4]
						movhps		xmm2, [edi+8*4]							// xmm2 = 6, 7, 8, 9
						shufps		xmm0, xmm2, R_SHUFFLEPS( 0, 3, 0, 3 )	// xmm0 = 0, 3, 6, 9
						mulps		xmm0, xmm5
						movlps		xmm3, [edi+10*4]
						shufps		xmm2, xmm3, R_SHUFFLEPS( 1, 2, 0, 1 )	// xmm2 = 7, 8, 10, 11
						movaps		xmm3, xmm1
						shufps		xmm1, xmm2, R_SHUFFLEPS( 2, 0, 0, 2 )	// xmm1 = 1, 4, 7, 10
						mulps		xmm1, xmm6
						shufps		xmm3, xmm2, R_SHUFFLEPS( 3, 1, 1, 3 )	// xmm3 = 2, 5, 8, 11
						mulps		xmm3, xmm7
						addps		xmm0, xmm1
						addps		xmm0, xmm3
						STORE4( 0, xmm0, xmm4 )
						movss		xmm1, [edi+12*4]
						mulss		xmm1, xmm5
						movss		xmm2, [edi+13*4]
						mulss		xmm2, xmm6
						movss		xmm3, [edi+14*4]
						mulss		xmm3, xmm7
						addss		xmm1, xmm2
						addss		xmm1, xmm3
						STORE1( 16, xmm1, xmm4 )
						mulss		xmm5, [edi+15*4]
						mulss		xmm6, [edi+16*4]
						mulss		xmm7, [edi+17*4]
						addss		xmm5, xmm6
						addss		xmm5, xmm7
						STORE1( 20, xmm5, xmm4 )
					}
					return;
				}
				default: {
					for ( int i = 0; i < numRows; i++ ) {
						dstPtr[i] STOREC mPtr[0] * vPtr[0] + mPtr[1] * vPtr[1] + mPtr[2] * vPtr[2];
						mPtr += 3;
					}
					return;
				}
			}
			break;
		}
		case 4: {
			switch( numRows ) {
				case 4: {		// 4x4 * 4x1
					__asm {
						mov			esi, vPtr
						mov			edi, mPtr
						mov			eax, dstPtr
						movlps		xmm6, qword ptr [esi ]
						movlps		xmm0, qword ptr [edi ]
						shufps		xmm6, xmm6, R_SHUFFLEPS( 0, 1, 0, 1 )
						movhps		xmm0, qword ptr [edi+16]
						mulps		xmm0, xmm6
						movlps		xmm7, qword ptr [esi+ 8]
						movlps		xmm2, qword ptr [edi+ 8]
						shufps		xmm7, xmm7, R_SHUFFLEPS( 0, 1, 0, 1 )
						movhps		xmm2, qword ptr [edi+24]
						mulps		xmm2, xmm7
						movlps		xmm1, qword ptr [edi+32]
						movhps		xmm1, qword ptr [edi+48]
						mulps		xmm1, xmm6
						movlps		xmm3, qword ptr [edi+40]
						addps		xmm0, xmm2
						movhps		xmm3, qword ptr [edi+56]
						mulps		xmm3, xmm7
						movaps		xmm4, xmm0
						addps		xmm1, xmm3
						shufps		xmm4, xmm1, R_SHUFFLEPS( 0, 2, 0, 2 )
						shufps		xmm0, xmm1, R_SHUFFLEPS( 1, 3, 1, 3 )
						addps		xmm0, xmm4
						STORE4( 0, xmm0, xmm2 )
					}
					return;
				}
				case 6: {		// 6x4 * 4x1
					__asm {
						mov			esi, vPtr
						mov			edi, mPtr
						mov			eax, dstPtr
						movlps		xmm6, qword ptr [esi+ 0]
						movlps		xmm0, qword ptr [edi+ 0]
						shufps		xmm6, xmm6, R_SHUFFLEPS( 0, 1, 0, 1 )
						movhps		xmm0, qword ptr [edi+16]
						mulps		xmm0, xmm6
						movlps		xmm7, qword ptr [esi+ 8]
						movlps		xmm2, qword ptr [edi+ 8]
						shufps		xmm7, xmm7, R_SHUFFLEPS( 0, 1, 0, 1 )
						movhps		xmm2, qword ptr [edi+24]
						mulps		xmm2, xmm7
						movlps		xmm1, qword ptr [edi+32]
						movhps		xmm1, qword ptr [edi+48]
						mulps		xmm1, xmm6
						movlps		xmm3, qword ptr [edi+40]
						addps		xmm0, xmm2
						movhps		xmm3, qword ptr [edi+56]
						mulps		xmm3, xmm7
						movaps		xmm4, xmm0
						addps		xmm1, xmm3
						shufps		xmm4, xmm1, R_SHUFFLEPS( 0, 2, 0, 2 )
						shufps		xmm0, xmm1, R_SHUFFLEPS( 1, 3, 1, 3 )
						addps		xmm0, xmm4
						movlps		xmm1, qword ptr [edi+64]
						movhps		xmm1, qword ptr [edi+80]
						STORE4( 0, xmm0, xmm4 )
						mulps		xmm1, xmm6
						movlps		xmm2, qword ptr [edi+72]
						movhps		xmm2, qword ptr [edi+88]
						mulps		xmm2, xmm7
						addps		xmm1, xmm2
						shufps		xmm1, xmm1, R_SHUFFLEPS( 0, 2, 1, 3 )
						movhlps		xmm3, xmm1
						addps		xmm1, xmm3
						STORE2LO( 16, xmm1, xmm4 )
					}
					return;
				}
				default: {
					for ( int i = 0; i < numRows; i++ ) {
						dstPtr[i] STOREC mPtr[0] * vPtr[0] + mPtr[1] * vPtr[1] + mPtr[2] * vPtr[2] + mPtr[3] * vPtr[3];
						mPtr += 4;
					}
					return;
				}
			}
			break;
		}
		case 5: {
			switch( numRows ) {
				case 5: {		// 5x5 * 5x1
					__asm {
						mov			esi, vPtr
						mov			edi, mPtr
						mov			eax, dstPtr
						movss		xmm0, [edi+5*4]							// xmm0 =  5,  X,  X,  X
						movhps		xmm0, [edi+0*4]							// xmm0 =  5,  X,  0,  1
						movss		xmm5, [edi+15*4]						// xmm4 = 15,  X,  X,  X
						movhps		xmm5, [edi+10*4]						// xmm5 = 15,  X, 10, 11
						movaps		xmm1, xmm0								// xmm1 =  5,  X,  0,  1
						shufps		xmm0, xmm5, R_SHUFFLEPS( 2, 0, 2, 0 )	// xmm0 =  0,  5, 10, 15
						movlps		xmm1, [edi+6*4]							// xmm1 =  6,  7,  0,  1
						movlps		xmm5, [edi+16*4]						// xmm5 = 16, 17, 10, 11
						movaps		xmm2, xmm1								// xmm2 =  6,  7,  0,  1
						shufps		xmm1, xmm5, R_SHUFFLEPS( 3, 0, 3, 0 )	// xmm1 =  1,  6, 11, 16
						movhps		xmm2, [edi+2*4]							// xmm2 =  6,  7,  2,  3
						movhps		xmm5, [edi+12*4]						// xmm5 = 16, 17, 12, 13
						movaps		xmm3, xmm2								// xmm3 =  6,  7,  2,  3
						shufps		xmm2, xmm5, R_SHUFFLEPS( 2, 1, 2, 1 )	// xmm2 =  2,  7, 12, 17
						movlps		xmm3, [edi+8*4]							// xmm3 =  8,  9,  2,  3
						movlps		xmm5, [edi+18*4]						// xmm5 = 18, 19, 12, 13
						movss		xmm4, [edi+4*4]							// xmm4 =  4,  X,  X,  X
						movlhps		xmm4, xmm3								// xmm4 =  4,  X,  8,  9
						shufps		xmm3, xmm5, R_SHUFFLEPS( 3, 0, 3, 0 )	// xmm3 =  3,  8, 13, 18
						movhps		xmm5, [edi+14*4]						// xmm6 = 18, 19, 14, 15
						shufps		xmm4, xmm5, R_SHUFFLEPS( 0, 3, 2, 1 )	// xmm4 =  4,  9, 14, 19
						movss		xmm7, [esi+0*4]
						shufps		xmm7, xmm7, 0
						mulps		xmm0, xmm7
						movss		xmm5, [esi+1*4]
						shufps		xmm5, xmm5, 0
						mulps		xmm1, xmm5
						addps		xmm0, xmm1
						movss		xmm6, [esi+2*4]
						shufps		xmm6, xmm6, 0
						mulps		xmm2, xmm6
						addps		xmm0, xmm2
						movss		xmm1, [esi+3*4]
						shufps		xmm1, xmm1, 0
						mulps		xmm3, xmm1
						addps		xmm0, xmm3
						movss		xmm2, [esi+4*4]
						shufps		xmm2, xmm2, 0
						mulps		xmm4, xmm2
						addps		xmm0, xmm4
						mulss		xmm7, [edi+20*4]
						mulss		xmm5, [edi+21*4]
						addps		xmm7, xmm5
						mulss		xmm6, [edi+22*4]
						addps		xmm7, xmm6
						mulss		xmm1, [edi+23*4]
						addps		xmm7, xmm1
						mulss		xmm2, [edi+24*4]
						addps		xmm7, xmm2
						STORE4( 0, xmm0, xmm3 )
						STORE1( 16, xmm7, xmm4 )
					}
					return;
				}
				case 6: {		// 6x5 * 5x1
					__asm {
						mov			esi, vPtr
						mov			edi, mPtr
						mov			eax, dstPtr
						movlps		xmm6, [esi]
						shufps		xmm6, xmm6, R_SHUFFLEPS( 0, 1, 0, 1 )
						movlps		xmm7, [esi+8]
						shufps		xmm7, xmm7, R_SHUFFLEPS( 0, 1, 0, 1 )
						movlps		xmm0, [edi]
						movhps		xmm3, [edi+8]
						movaps		xmm1, [edi+16]
						movlps		xmm2, [edi+32]
						shufps		xmm0, xmm1, R_SHUFFLEPS( 0, 1, 1, 2 )	// xmm0 = 0, 1, 5, 6
						shufps		xmm1, xmm2, R_SHUFFLEPS( 0, 3, 0, 1 )	// xmm1 = 4, 7, 8, 9
						shufps		xmm3, xmm1, R_SHUFFLEPS( 2, 3, 1, 2 )	// xmm3 = 2, 3, 7, 8
						mulps		xmm0, xmm6
						mulps		xmm3, xmm7
						movlps		xmm2, [edi+40]
						addps		xmm0, xmm3								// xmm0 + xmm1
						movhps		xmm5, [edi+40+8]
						movlps		xmm3, [edi+40+16]
						movhps		xmm3, [edi+40+24]
						movlps		xmm4, [edi+40+32]
						shufps		xmm2, xmm3, R_SHUFFLEPS( 0, 1, 1, 2 )	// xmm2 = 10, 11, 15, 16
						shufps		xmm3, xmm4, R_SHUFFLEPS( 0, 3, 0, 1 )	// xmm3 = 14, 17, 18, 19
						shufps		xmm5, xmm3, R_SHUFFLEPS( 2, 3, 1, 2 )	// xmm5 = 12, 13, 17, 18
						mulps		xmm2, xmm6
						mulps		xmm5, xmm7
						addps		xmm2, xmm5								// xmm2 + xmm3
						movss		xmm5, [esi+16]
						shufps		xmm5, xmm5, R_SHUFFLEPS( 0, 0, 0, 0 )
						movaps		xmm4, xmm0
						shufps		xmm0, xmm2, R_SHUFFLEPS( 0, 2, 0, 2 )
						shufps		xmm4, xmm2, R_SHUFFLEPS( 1, 3, 1, 3 )
						shufps		xmm1, xmm3, R_SHUFFLEPS( 0, 3, 0, 3 )
						addps		xmm0, xmm4
						mulps		xmm1, xmm5
						addps		xmm0, xmm1
						STORE4( 0, xmm0, xmm2 )
						movlps		xmm4, [edi+80]
						movhps		xmm3, [edi+80+8]
						movaps		xmm1, [edi+80+16]
						movlps		xmm2, [edi+80+32]
						shufps		xmm4, xmm1, R_SHUFFLEPS( 0, 1, 1, 2 )	// xmm4 = 20, 21, 25, 26
						shufps		xmm1, xmm2, R_SHUFFLEPS( 0, 3, 0, 1 )	// xmm1 = 24, 27, 28, 29
						shufps		xmm3, xmm1, R_SHUFFLEPS( 2, 3, 1, 2 )	// xmm3 = 22, 23, 27, 28
						mulps		xmm4, xmm6
						mulps		xmm3, xmm7
						mulps		xmm1, xmm5
						addps		xmm4, xmm3								// xmm4 + xmm1
						shufps		xmm1, xmm4, R_SHUFFLEPS( 0, 3, 0, 2 )
						shufps		xmm4, xmm4, R_SHUFFLEPS( 1, 3, 0, 0 )
						addps		xmm4, xmm1
						shufps		xmm1, xmm1, R_SHUFFLEPS( 2, 3, 0, 1 )
						addps		xmm4, xmm1
						STORE2LO( 16, xmm4, xmm2 )
					}
					return;
				}
				default: {
					for ( int i = 0; i < numRows; i++ ) {
						dstPtr[i] STOREC mPtr[0] * vPtr[0] + mPtr[1] * vPtr[1] + mPtr[2] * vPtr[2] + mPtr[3] * vPtr[3] + mPtr[4] * vPtr[4];
						mPtr += 5;
					}
					return;
				}
			}
			break;
		}
		case 6: {
			switch( numRows ) {
				case 1: {		// 1x6 * 6x1
					__asm {
						mov			esi, vPtr
						mov			edi, mPtr
						mov			eax, dstPtr
						movss		xmm0, [esi]
						mulss		xmm0, [edi]
						movss		xmm1, [esi+4]
						mulss		xmm1, [edi+4]
						movss		xmm2, [esi+8]
						addss		xmm0, xmm1
						mulss		xmm2, [edi+8]
						movss		xmm3, [esi+12]
						addss		xmm0, xmm2
						mulss		xmm3, [edi+12]
						movss		xmm4, [esi+16]
						addss		xmm0, xmm3
						mulss		xmm4, [edi+16]
						movss		xmm5, [esi+20]
						addss		xmm0, xmm4
						mulss		xmm5, [edi+20]
						movss		xmm6, [esi+24]
						addss		xmm0, xmm5
						mulss		xmm6, [edi+24]
						addss		xmm0, xmm6
						STORE1( 0, xmm0, xmm7 )
					}
					return;
				}
				case 2: {		// 2x6 * 6x1
					__asm {
						mov			esi, vPtr
						mov			edi, mPtr
						mov			eax, dstPtr
						// load idVecX
						movlps		xmm4, [esi]
						movhps		xmm4, [esi+8]
						movlps		xmm5, [esi+16]
						movlhps		xmm5, xmm4
						movhlps		xmm6, xmm4
						movlhps		xmm6, xmm5
						// row 0 and 1
						movaps		xmm0, [edi]
						movaps		xmm1, [edi+16]
						movaps		xmm2, [edi+32]
						mulps		xmm0, xmm4
						mulps		xmm1, xmm5
						mulps		xmm2, xmm6
						movhlps		xmm3, xmm0
						movlhps		xmm3, xmm2
						addps		xmm1, xmm3
						shufps		xmm0, xmm2, R_SHUFFLEPS( 0, 1, 2, 3 )
						addps		xmm1, xmm0
						shufps		xmm1, xmm1, R_SHUFFLEPS( 0, 2, 1, 3 )
						movhlps		xmm0, xmm1
						addps		xmm0, xmm1
						STORE2LO( 0, xmm0, xmm3 )
					}
					return;
				}
				case 3: {		// 3x6 * 6x1
					__asm {
						mov			esi, vPtr
						mov			edi, mPtr
						mov			eax, dstPtr
						// load idVecX
						movlps		xmm4, [esi]
						movhps		xmm4, [esi+8]
						movlps		xmm5, [esi+16]
						movlhps		xmm5, xmm4
						movhlps		xmm6, xmm4
						movlhps		xmm6, xmm5
						// row 0 and 1
						movaps		xmm0, [edi]
						movaps		xmm1, [edi+16]
						movaps		xmm2, [edi+32]
						mulps		xmm0, xmm4
						mulps		xmm1, xmm5
						mulps		xmm2, xmm6
						movhlps		xmm3, xmm0
						movlhps		xmm3, xmm2
						addps		xmm1, xmm3
						shufps		xmm0, xmm2, R_SHUFFLEPS( 0, 1, 2, 3 )
						addps		xmm1, xmm0
						shufps		xmm1, xmm1, R_SHUFFLEPS( 0, 2, 1, 3 )
						movhlps		xmm0, xmm1
						addps		xmm0, xmm1
						STORE2LO( 0, xmm0, xmm3 )
						// row 2
						movaps		xmm0, [edi+48]
						movaps		xmm1, [edi+48+16]
						mulps		xmm0, xmm4
						mulps		xmm1, xmm5
						addps		xmm0, xmm1
						movhlps		xmm1, xmm0
						addps		xmm0, xmm1
						movaps		xmm1, xmm0
						shufps		xmm1, xmm1, R_SHUFFLEPS( 1, 0, 0, 0 )
						addss		xmm0, xmm1
						STORE1( 8, xmm0, xmm3 )
					}
					return;
				}
				case 4: {		// 4x6 * 6x1
					__asm {
						mov			esi, vPtr
						mov			edi, mPtr
						mov			eax, dstPtr
						// load idVecX
						movlps		xmm4, [esi]
						movhps		xmm4, [esi+8]
						movlps		xmm5, [esi+16]
						movlhps		xmm5, xmm4
						movhlps		xmm6, xmm4
						movlhps		xmm6, xmm5
						// row 0 and 1
						movaps		xmm0, [edi]
						movaps		xmm1, [edi+16]
						movaps		xmm2, [edi+32]
						mulps		xmm0, xmm4
						mulps		xmm1, xmm5
						mulps		xmm2, xmm6
						movhlps		xmm7, xmm0
						movlhps		xmm7, xmm2
						addps		xmm7, xmm1
						shufps		xmm0, xmm2, R_SHUFFLEPS( 0, 1, 2, 3 )
						addps		xmm7, xmm0
						// row 2 and 3
						movaps		xmm0, [edi+48]
						movaps		xmm1, [edi+48+16]
						movaps		xmm2, [edi+48+32]
						mulps		xmm0, xmm4
						mulps		xmm1, xmm5
						mulps		xmm2, xmm6
						movhlps		xmm3, xmm0
						movlhps		xmm3, xmm2
						addps		xmm1, xmm3
						shufps		xmm0, xmm2, R_SHUFFLEPS( 0, 1, 2, 3 )
						addps		xmm1, xmm0
						// last 4 additions for the first 4 rows and store result
						movaps		xmm0, xmm7
						shufps		xmm7, xmm1, R_SHUFFLEPS( 0, 2, 0, 2 )
						shufps		xmm0, xmm1, R_SHUFFLEPS( 1, 3, 1, 3 )
						addps		xmm0, xmm7
						STORE4( 0, xmm0, xmm4 )
					}
					return;
				}
				case 5: {		// 5x6 * 6x1
					__asm {
						mov			esi, vPtr
						mov			edi, mPtr
						mov			eax, dstPtr
						// load idVecX
						movlps		xmm4, [esi]
						movhps		xmm4, [esi+8]
						movlps		xmm5, [esi+16]
						movlhps		xmm5, xmm4
						movhlps		xmm6, xmm4
						movlhps		xmm6, xmm5
						// row 0 and 1
						movaps		xmm0, [edi]
						movaps		xmm1, [edi+16]
						movaps		xmm2, [edi+32]
						mulps		xmm0, xmm4
						mulps		xmm1, xmm5
						mulps		xmm2, xmm6
						movhlps		xmm7, xmm0
						movlhps		xmm7, xmm2
						addps		xmm7, xmm1
						shufps		xmm0, xmm2, R_SHUFFLEPS( 0, 1, 2, 3 )
						addps		xmm7, xmm0
						// row 2 and 3
						movaps		xmm0, [edi+48]
						movaps		xmm1, [edi+48+16]
						movaps		xmm2, [edi+48+32]
						mulps		xmm0, xmm4
						mulps		xmm1, xmm5
						mulps		xmm2, xmm6
						movhlps		xmm3, xmm0
						movlhps		xmm3, xmm2
						addps		xmm1, xmm3
						shufps		xmm0, xmm2, R_SHUFFLEPS( 0, 1, 2, 3 )
						addps		xmm1, xmm0
						// last 4 additions for the first 4 rows and store result
						movaps		xmm0, xmm7
						shufps		xmm7, xmm1, R_SHUFFLEPS( 0, 2, 0, 2 )
						shufps		xmm0, xmm1, R_SHUFFLEPS( 1, 3, 1, 3 )
						addps		xmm0, xmm7
						STORE4( 0, xmm0, xmm3 )
						// row 5
						movaps		xmm0, [edi+96]
						movaps		xmm1, [edi+96+16]
						mulps		xmm0, xmm4
						mulps		xmm1, xmm5
						addps		xmm0, xmm1
						movhlps		xmm1, xmm0
						addps		xmm0, xmm1
						movaps		xmm1, xmm0
						shufps		xmm1, xmm1, 0x01
						addss		xmm0, xmm1
						STORE1( 16, xmm0, xmm3 )
					}
					return;
				}
				case 6: {		// 6x6 * 6x1
					__asm {
						mov			esi, vPtr
						mov			edi, mPtr
						mov			eax, dstPtr
						movlps		xmm7, qword ptr [esi]
						movlps		xmm6, qword ptr [esi+8]
						shufps		xmm7, xmm7, 0x44
						shufps		xmm6, xmm6, 0x44
						movlps		xmm0, qword ptr [edi    ]
						movhps		xmm0, qword ptr [edi+ 24]
						mulps		xmm0, xmm7
						movlps		xmm3, qword ptr [edi+  8]
						movhps		xmm3, qword ptr [edi+ 32]
						mulps		xmm3, xmm6
						movlps		xmm1, qword ptr [edi+ 48]
						movhps		xmm1, qword ptr [edi+ 72]
						mulps		xmm1, xmm7
						movlps		xmm2, qword ptr [edi+ 96]
						movhps		xmm2, qword ptr [edi+120]
						mulps		xmm2, xmm7
						movlps		xmm4, qword ptr [edi+ 56]
						movhps		xmm4, qword ptr [edi+ 80]
						movlps		xmm5, qword ptr [edi+104]
						movhps		xmm5, qword ptr [edi+128]
						mulps		xmm4, xmm6
						movlps		xmm7, qword ptr [esi+16]
						addps		xmm0, xmm3
						shufps		xmm7, xmm7, 0x44
						mulps		xmm5, xmm6
						addps		xmm1, xmm4
						movlps		xmm3, qword ptr [edi+ 16]
						movhps		xmm3, qword ptr [edi+ 40]
						addps		xmm2, xmm5
						movlps		xmm4, qword ptr [edi+ 64]
						movhps		xmm4, qword ptr [edi+ 88]
						mulps		xmm3, xmm7
						movlps		xmm5, qword ptr [edi+112]
						movhps		xmm5, qword ptr [edi+136]
						addps		xmm0, xmm3
						mulps		xmm4, xmm7
						mulps		xmm5, xmm7
						addps		xmm1, xmm4
						addps		xmm2, xmm5
						movaps		xmm6, xmm0
						shufps		xmm0, xmm1, 0x88
						shufps		xmm6, xmm1, 0xDD
						movaps		xmm7, xmm2
						shufps		xmm7, xmm2, 0x88
						shufps		xmm2, xmm2, 0xDD
						addps		xmm0, xmm6
						addps		xmm2, xmm7
						STORE4( 0, xmm0, xmm3 )
						STORE2LO( 16, xmm2, xmm4 )
					}
					return;
				}
				default: {
					for ( int i = 0; i < numRows; i++ ) {
						dstPtr[i] STOREC mPtr[0] * vPtr[0] + mPtr[1] * vPtr[1] + mPtr[2] * vPtr[2] +
									mPtr[3] * vPtr[3] + mPtr[4] * vPtr[4] + mPtr[5] * vPtr[5];
						mPtr += 6;
					}
					return;
				}
			}
			break;
		}
		default: {
			int numColumns = mat.GetNumColumns();
			for ( int i = 0; i < numRows; i++ ) {
				float sum = mPtr[0] * vPtr[0];
				for ( int j = 1; j < numColumns; j++ ) {
					sum += mPtr[j] * vPtr[j];
				}
				dstPtr[i] STOREC sum;
				mPtr += numColumns;
			}
			break;
		}
	}

#undef STOREC
#undef STORE4
#undef STORE2HI
#undef STORE2LO
#undef STORE1
}

/*
============
idSIMD_SSE::MatX_MultiplySubVecX

	optimizes the following matrix multiplications:

	NxN * Nx1
	Nx6 * 6x1
	6xN * Nx1

	with N in the range [1-6]
============
*/
void VPCALL idSIMD_SSE::MatX_MultiplySubVecX( idVecX &dst, const idMatX &mat, const idVecX &vec ) {
#define STORE1( offset, reg1, reg2 )		\
	__asm movss		reg2, [eax+offset]		\
	__asm subss		reg2, reg1				\
	__asm movss		[eax+offset], reg2
#define STORE2LO( offset, reg1, reg2 )		\
	__asm movlps	reg2, [eax+offset]		\
	__asm subps		reg2, reg1				\
	__asm movlps	[eax+offset], reg2
#define STORE2HI( offset, reg1, reg2 )		\
	__asm movhps	reg2, [eax+offset]		\
	__asm subps		reg2, reg1				\
	__asm movhps	[eax+offset], reg2
#define STORE4( offset, reg1, reg2 )		\
	__asm movlps	reg2, [eax+offset]		\
	__asm movhps	reg2, [eax+offset+8]	\
	__asm subps		reg2, reg1				\
	__asm movlps	[eax+offset], reg2		\
	__asm movhps	[eax+offset+8], reg2
#define STOREC		-=

	int numRows;
	const float *mPtr, *vPtr;
	float *dstPtr;

	assert( vec.GetSize() >= mat.GetNumColumns() );
	assert( dst.GetSize() >= mat.GetNumRows() );

	mPtr = mat.ToFloatPtr();
	vPtr = vec.ToFloatPtr();
	dstPtr = dst.ToFloatPtr();
	numRows = mat.GetNumRows();
	switch( mat.GetNumColumns() ) {
		case 1: {
			switch( numRows ) {
				case 1: {		// 1x1 * 1x1
					__asm {
						mov			esi, vPtr
						mov			edi, mPtr
						mov			eax, dstPtr
						movss		xmm0, [esi]
						mulss		xmm0, [edi]
						STORE1( 0, xmm0, xmm1 )
					}
					return;
				}
				case 6: {		// 6x1 * 1x1
					__asm {
						mov			esi, vPtr
						mov			edi, mPtr
						mov			eax, dstPtr
						movss		xmm0, [esi]
						shufps		xmm0, xmm0, R_SHUFFLEPS( 0, 0, 0, 0 )
						movaps		xmm1, xmm0
						mulps		xmm0, [edi]
						mulps		xmm1, [edi+16]
						STORE4( 0, xmm0, xmm2 )
						STORE2LO( 16, xmm1, xmm2 )
					}
					return;
				}
				default: {
					for ( int i = 0; i < numRows; i++ ) {
						dstPtr[i] STOREC mPtr[0] * vPtr[0];
						mPtr++;
					}
					return;
				}
			}
			break;
		}
		case 2: {
			switch( numRows ) {
				case 2: {		// 2x2 * 2x1
					__asm {
						mov			esi, vPtr
						mov			edi, mPtr
						mov			eax, dstPtr
						movss		xmm0, [esi]
						movss		xmm1, [esi+4]
						movss		xmm2, [edi]
						mulss		xmm2, xmm0
						movss		xmm3, [edi+4]
						mulss		xmm3, xmm1
						addss		xmm2, xmm3
						STORE1( 0, xmm2, xmm4 )
						mulss		xmm0, [edi+8]
						mulss		xmm1, [edi+8+4]
						addss		xmm0, xmm1
						STORE1( 4, xmm0, xmm4 )
					}
					return;
				}
				case 6: {		// 6x2 * 2x1
					__asm {
						mov			esi, vPtr
						mov			edi, mPtr
						mov			eax, dstPtr
						movlps		xmm7, [esi]
						shufps		xmm7, xmm7, R_SHUFFLEPS( 0, 1, 0, 1 )
						movaps		xmm0, [edi]
						mulps		xmm0, xmm7
						movaps		xmm1, [edi+16]
						mulps		xmm1, xmm7
						movaps		xmm2, xmm0
						shufps		xmm0, xmm1, R_SHUFFLEPS( 0, 2, 0, 2 )
						shufps		xmm2, xmm1, R_SHUFFLEPS( 1, 3, 1, 3 )
						movaps		xmm3, [edi+32]
						addps		xmm0, xmm2
						mulps		xmm3, xmm7
						STORE4( 0, xmm0, xmm4 )
						shufps		xmm3, xmm3, R_SHUFFLEPS( 0, 2, 1, 3 )
						movhlps		xmm1, xmm3
						addps		xmm3, xmm1
						STORE2LO( 16, xmm3, xmm4 )
					}
					return;
				}
				default: {
					for ( int i = 0; i < numRows; i++ ) {
						dstPtr[i] STOREC mPtr[0] * vPtr[0] + mPtr[1] * vPtr[1];
						mPtr += 2;
					}
					return;
				}
			}
			break;
		}
		case 3: {
			switch( numRows ) {
				case 3: {		// 3x3 * 3x1
					__asm {
						mov			esi, vPtr
						mov			edi, mPtr
						mov			eax, dstPtr
						movss		xmm0, [esi]
						movss		xmm4, [edi]
						mulss		xmm4, xmm0
						movss		xmm1, [esi+4]
						movss		xmm5, [edi+4]
						mulss		xmm5, xmm1
						addss		xmm4, xmm5
						movss		xmm2, [esi+8]
						movss		xmm6, [edi+8]
						mulss		xmm6, xmm2
						addss		xmm4, xmm6
						movss		xmm3, [edi+12]
						mulss		xmm3, xmm0
						STORE1( 0, xmm4, xmm7 );
						movss		xmm5, [edi+12+4]
						mulss		xmm5, xmm1
						addss		xmm3, xmm5
						movss		xmm6, [edi+12+8]
						mulss		xmm6, xmm2
						addss		xmm3, xmm6
						mulss		xmm0, [edi+24]
						mulss		xmm1, [edi+24+4]
						STORE1( 4, xmm3, xmm7 );
						addss		xmm0, xmm1
						mulss		xmm2, [edi+24+8]
						addss		xmm0, xmm2
						STORE1( 8, xmm0, xmm7 );
					}
					return;
				}
				case 6: {		// 6x3 * 3x1
					__asm {
						mov			esi, vPtr
						mov			edi, mPtr
						mov			eax, dstPtr
						movss		xmm5, [esi]
						shufps		xmm5, xmm5, R_SHUFFLEPS( 0, 0, 0, 0 )
						movss		xmm6, [esi+4]
						shufps		xmm6, xmm6, R_SHUFFLEPS( 0, 0, 0, 0 )
						movss		xmm7, [esi+8]
						shufps		xmm7, xmm7, R_SHUFFLEPS( 0, 0, 0, 0 )
						movaps		xmm0, [edi]								// xmm0 = 0, 1, 2, 3
						movlps		xmm1, [edi+4*4]
						shufps		xmm1, xmm0, R_SHUFFLEPS( 0, 1, 1, 2 )	// xmm1 = 4, 5, 1, 2
						movlps		xmm2, [edi+6*4]
						movhps		xmm2, [edi+8*4]							// xmm2 = 6, 7, 8, 9
						shufps		xmm0, xmm2, R_SHUFFLEPS( 0, 3, 0, 3 )	// xmm0 = 0, 3, 6, 9
						mulps		xmm0, xmm5
						movlps		xmm3, [edi+10*4]
						shufps		xmm2, xmm3, R_SHUFFLEPS( 1, 2, 0, 1 )	// xmm2 = 7, 8, 10, 11
						movaps		xmm3, xmm1
						shufps		xmm1, xmm2, R_SHUFFLEPS( 2, 0, 0, 2 )	// xmm1 = 1, 4, 7, 10
						mulps		xmm1, xmm6
						shufps		xmm3, xmm2, R_SHUFFLEPS( 3, 1, 1, 3 )	// xmm3 = 2, 5, 8, 11
						mulps		xmm3, xmm7
						addps		xmm0, xmm1
						addps		xmm0, xmm3
						STORE4( 0, xmm0, xmm4 )
						movss		xmm1, [edi+12*4]
						mulss		xmm1, xmm5
						movss		xmm2, [edi+13*4]
						mulss		xmm2, xmm6
						movss		xmm3, [edi+14*4]
						mulss		xmm3, xmm7
						addss		xmm1, xmm2
						addss		xmm1, xmm3
						STORE1( 16, xmm1, xmm4 )
						mulss		xmm5, [edi+15*4]
						mulss		xmm6, [edi+16*4]
						mulss		xmm7, [edi+17*4]
						addss		xmm5, xmm6
						addss		xmm5, xmm7
						STORE1( 20, xmm5, xmm4 )
					}
					return;
				}
				default: {
					for ( int i = 0; i < numRows; i++ ) {
						dstPtr[i] STOREC mPtr[0] * vPtr[0] + mPtr[1] * vPtr[1] + mPtr[2] * vPtr[2];
						mPtr += 3;
					}
					return;
				}
			}
			break;
		}
		case 4: {
			switch( numRows ) {
				case 4: {		// 4x4 * 4x1
					__asm {
						mov			esi, vPtr
						mov			edi, mPtr
						mov			eax, dstPtr
						movlps		xmm6, qword ptr [esi ]
						movlps		xmm0, qword ptr [edi ]
						shufps		xmm6, xmm6, R_SHUFFLEPS( 0, 1, 0, 1 )
						movhps		xmm0, qword ptr [edi+16]
						mulps		xmm0, xmm6
						movlps		xmm7, qword ptr [esi+ 8]
						movlps		xmm2, qword ptr [edi+ 8]
						shufps		xmm7, xmm7, R_SHUFFLEPS( 0, 1, 0, 1 )
						movhps		xmm2, qword ptr [edi+24]
						mulps		xmm2, xmm7
						movlps		xmm1, qword ptr [edi+32]
						movhps		xmm1, qword ptr [edi+48]
						mulps		xmm1, xmm6
						movlps		xmm3, qword ptr [edi+40]
						addps		xmm0, xmm2
						movhps		xmm3, qword ptr [edi+56]
						mulps		xmm3, xmm7
						movaps		xmm4, xmm0
						addps		xmm1, xmm3
						shufps		xmm4, xmm1, R_SHUFFLEPS( 0, 2, 0, 2 )
						shufps		xmm0, xmm1, R_SHUFFLEPS( 1, 3, 1, 3 )
						addps		xmm0, xmm4
						STORE4( 0, xmm0, xmm2 )
					}
					return;
				}
				case 6: {		// 6x4 * 4x1
					__asm {
						mov			esi, vPtr
						mov			edi, mPtr
						mov			eax, dstPtr
						movlps		xmm6, qword ptr [esi+ 0]
						movlps		xmm0, qword ptr [edi+ 0]
						shufps		xmm6, xmm6, R_SHUFFLEPS( 0, 1, 0, 1 )
						movhps		xmm0, qword ptr [edi+16]
						mulps		xmm0, xmm6
						movlps		xmm7, qword ptr [esi+ 8]
						movlps		xmm2, qword ptr [edi+ 8]
						shufps		xmm7, xmm7, R_SHUFFLEPS( 0, 1, 0, 1 )
						movhps		xmm2, qword ptr [edi+24]
						mulps		xmm2, xmm7
						movlps		xmm1, qword ptr [edi+32]
						movhps		xmm1, qword ptr [edi+48]
						mulps		xmm1, xmm6
						movlps		xmm3, qword ptr [edi+40]
						addps		xmm0, xmm2
						movhps		xmm3, qword ptr [edi+56]
						mulps		xmm3, xmm7
						movaps		xmm4, xmm0
						addps		xmm1, xmm3
						shufps		xmm4, xmm1, R_SHUFFLEPS( 0, 2, 0, 2 )
						shufps		xmm0, xmm1, R_SHUFFLEPS( 1, 3, 1, 3 )
						addps		xmm0, xmm4
						movlps		xmm1, qword ptr [edi+64]
						movhps		xmm1, qword ptr [edi+80]
						STORE4( 0, xmm0, xmm4 )
						mulps		xmm1, xmm6
						movlps		xmm2, qword ptr [edi+72]
						movhps		xmm2, qword ptr [edi+88]
						mulps		xmm2, xmm7
						addps		xmm1, xmm2
						shufps		xmm1, xmm1, R_SHUFFLEPS( 0, 2, 1, 3 )
						movhlps		xmm3, xmm1
						addps		xmm1, xmm3
						STORE2LO( 16, xmm1, xmm4 )
					}
					return;
				}
				default: {
					for ( int i = 0; i < numRows; i++ ) {
						dstPtr[i] STOREC mPtr[0] * vPtr[0] + mPtr[1] * vPtr[1] + mPtr[2] * vPtr[2] + mPtr[3] * vPtr[3];
						mPtr += 4;
					}
					return;
				}
			}
			break;
		}
		case 5: {
			switch( numRows ) {
				case 5: {		// 5x5 * 5x1
					__asm {
						mov			esi, vPtr
						mov			edi, mPtr
						mov			eax, dstPtr
						movss		xmm0, [edi+5*4]							// xmm0 =  5,  X,  X,  X
						movhps		xmm0, [edi+0*4]							// xmm0 =  5,  X,  0,  1
						movss		xmm5, [edi+15*4]						// xmm4 = 15,  X,  X,  X
						movhps		xmm5, [edi+10*4]						// xmm5 = 15,  X, 10, 11
						movaps		xmm1, xmm0								// xmm1 =  5,  X,  0,  1
						shufps		xmm0, xmm5, R_SHUFFLEPS( 2, 0, 2, 0 )	// xmm0 =  0,  5, 10, 15
						movlps		xmm1, [edi+6*4]							// xmm1 =  6,  7,  0,  1
						movlps		xmm5, [edi+16*4]						// xmm5 = 16, 17, 10, 11
						movaps		xmm2, xmm1								// xmm2 =  6,  7,  0,  1
						shufps		xmm1, xmm5, R_SHUFFLEPS( 3, 0, 3, 0 )	// xmm1 =  1,  6, 11, 16
						movhps		xmm2, [edi+2*4]							// xmm2 =  6,  7,  2,  3
						movhps		xmm5, [edi+12*4]						// xmm5 = 16, 17, 12, 13
						movaps		xmm3, xmm2								// xmm3 =  6,  7,  2,  3
						shufps		xmm2, xmm5, R_SHUFFLEPS( 2, 1, 2, 1 )	// xmm2 =  2,  7, 12, 17
						movlps		xmm3, [edi+8*4]							// xmm3 =  8,  9,  2,  3
						movlps		xmm5, [edi+18*4]						// xmm5 = 18, 19, 12, 13
						movss		xmm4, [edi+4*4]							// xmm4 =  4,  X,  X,  X
						movlhps		xmm4, xmm3								// xmm4 =  4,  X,  8,  9
						shufps		xmm3, xmm5, R_SHUFFLEPS( 3, 0, 3, 0 )	// xmm3 =  3,  8, 13, 18
						movhps		xmm5, [edi+14*4]						// xmm6 = 18, 19, 14, 15
						shufps		xmm4, xmm5, R_SHUFFLEPS( 0, 3, 2, 1 )	// xmm4 =  4,  9, 14, 19
						movss		xmm7, [esi+0*4]
						shufps		xmm7, xmm7, 0
						mulps		xmm0, xmm7
						movss		xmm5, [esi+1*4]
						shufps		xmm5, xmm5, 0
						mulps		xmm1, xmm5
						addps		xmm0, xmm1
						movss		xmm6, [esi+2*4]
						shufps		xmm6, xmm6, 0
						mulps		xmm2, xmm6
						addps		xmm0, xmm2
						movss		xmm1, [esi+3*4]
						shufps		xmm1, xmm1, 0
						mulps		xmm3, xmm1
						addps		xmm0, xmm3
						movss		xmm2, [esi+4*4]
						shufps		xmm2, xmm2, 0
						mulps		xmm4, xmm2
						addps		xmm0, xmm4
						mulss		xmm7, [edi+20*4]
						mulss		xmm5, [edi+21*4]
						addps		xmm7, xmm5
						mulss		xmm6, [edi+22*4]
						addps		xmm7, xmm6
						mulss		xmm1, [edi+23*4]
						addps		xmm7, xmm1
						mulss		xmm2, [edi+24*4]
						addps		xmm7, xmm2
						STORE4( 0, xmm0, xmm3 )
						STORE1( 16, xmm7, xmm4 )
					}
					return;
				}
				case 6: {		// 6x5 * 5x1
					__asm {
						mov			esi, vPtr
						mov			edi, mPtr
						mov			eax, dstPtr
						movlps		xmm6, [esi]
						shufps		xmm6, xmm6, R_SHUFFLEPS( 0, 1, 0, 1 )
						movlps		xmm7, [esi+8]
						shufps		xmm7, xmm7, R_SHUFFLEPS( 0, 1, 0, 1 )
						movlps		xmm0, [edi]
						movhps		xmm3, [edi+8]
						movaps		xmm1, [edi+16]
						movlps		xmm2, [edi+32]
						shufps		xmm0, xmm1, R_SHUFFLEPS( 0, 1, 1, 2 )	// xmm0 = 0, 1, 5, 6
						shufps		xmm1, xmm2, R_SHUFFLEPS( 0, 3, 0, 1 )	// xmm1 = 4, 7, 8, 9
						shufps		xmm3, xmm1, R_SHUFFLEPS( 2, 3, 1, 2 )	// xmm3 = 2, 3, 7, 8
						mulps		xmm0, xmm6
						mulps		xmm3, xmm7
						movlps		xmm2, [edi+40]
						addps		xmm0, xmm3								// xmm0 + xmm1
						movhps		xmm5, [edi+40+8]
						movlps		xmm3, [edi+40+16]
						movhps		xmm3, [edi+40+24]
						movlps		xmm4, [edi+40+32]
						shufps		xmm2, xmm3, R_SHUFFLEPS( 0, 1, 1, 2 )	// xmm2 = 10, 11, 15, 16
						shufps		xmm3, xmm4, R_SHUFFLEPS( 0, 3, 0, 1 )	// xmm3 = 14, 17, 18, 19
						shufps		xmm5, xmm3, R_SHUFFLEPS( 2, 3, 1, 2 )	// xmm5 = 12, 13, 17, 18
						mulps		xmm2, xmm6
						mulps		xmm5, xmm7
						addps		xmm2, xmm5								// xmm2 + xmm3
						movss		xmm5, [esi+16]
						shufps		xmm5, xmm5, R_SHUFFLEPS( 0, 0, 0, 0 )
						movaps		xmm4, xmm0
						shufps		xmm0, xmm2, R_SHUFFLEPS( 0, 2, 0, 2 )
						shufps		xmm4, xmm2, R_SHUFFLEPS( 1, 3, 1, 3 )
						shufps		xmm1, xmm3, R_SHUFFLEPS( 0, 3, 0, 3 )
						addps		xmm0, xmm4
						mulps		xmm1, xmm5
						addps		xmm0, xmm1
						STORE4( 0, xmm0, xmm2 )
						movlps		xmm4, [edi+80]
						movhps		xmm3, [edi+80+8]
						movaps		xmm1, [edi+80+16]
						movlps		xmm2, [edi+80+32]
						shufps		xmm4, xmm1, R_SHUFFLEPS( 0, 1, 1, 2 )	// xmm4 = 20, 21, 25, 26
						shufps		xmm1, xmm2, R_SHUFFLEPS( 0, 3, 0, 1 )	// xmm1 = 24, 27, 28, 29
						shufps		xmm3, xmm1, R_SHUFFLEPS( 2, 3, 1, 2 )	// xmm3 = 22, 23, 27, 28
						mulps		xmm4, xmm6
						mulps		xmm3, xmm7
						mulps		xmm1, xmm5
						addps		xmm4, xmm3								// xmm4 + xmm1
						shufps		xmm1, xmm4, R_SHUFFLEPS( 0, 3, 0, 2 )
						shufps		xmm4, xmm4, R_SHUFFLEPS( 1, 3, 0, 0 )
						addps		xmm4, xmm1
						shufps		xmm1, xmm1, R_SHUFFLEPS( 2, 3, 0, 1 )
						addps		xmm4, xmm1
						STORE2LO( 16, xmm4, xmm2 )
					}
					return;
				}
				default: {
					for ( int i = 0; i < numRows; i++ ) {
						dstPtr[i] STOREC mPtr[0] * vPtr[0] + mPtr[1] * vPtr[1] + mPtr[2] * vPtr[2] + mPtr[3] * vPtr[3] + mPtr[4] * vPtr[4];
						mPtr += 5;
					}
					return;
				}
			}
			break;
		}
		case 6: {
			switch( numRows ) {
				case 1: {		// 1x6 * 6x1
					__asm {
						mov			esi, vPtr
						mov			edi, mPtr
						mov			eax, dstPtr
						movss		xmm0, [esi]
						mulss		xmm0, [edi]
						movss		xmm1, [esi+4]
						mulss		xmm1, [edi+4]
						movss		xmm2, [esi+8]
						addss		xmm0, xmm1
						mulss		xmm2, [edi+8]
						movss		xmm3, [esi+12]
						addss		xmm0, xmm2
						mulss		xmm3, [edi+12]
						movss		xmm4, [esi+16]
						addss		xmm0, xmm3
						mulss		xmm4, [edi+16]
						movss		xmm5, [esi+20]
						addss		xmm0, xmm4
						mulss		xmm5, [edi+20]
						movss		xmm6, [esi+24]
						addss		xmm0, xmm5
						mulss		xmm6, [edi+24]
						addss		xmm0, xmm6
						STORE1( 0, xmm0, xmm7 )
					}
					return;
				}
				case 2: {		// 2x6 * 6x1
					__asm {
						mov			esi, vPtr
						mov			edi, mPtr
						mov			eax, dstPtr
						// load idVecX
						movlps		xmm4, [esi]
						movhps		xmm4, [esi+8]
						movlps		xmm5, [esi+16]
						movlhps		xmm5, xmm4
						movhlps		xmm6, xmm4
						movlhps		xmm6, xmm5
						// row 0 and 1
						movaps		xmm0, [edi]
						movaps		xmm1, [edi+16]
						movaps		xmm2, [edi+32]
						mulps		xmm0, xmm4
						mulps		xmm1, xmm5
						mulps		xmm2, xmm6
						movhlps		xmm3, xmm0
						movlhps		xmm3, xmm2
						addps		xmm1, xmm3
						shufps		xmm0, xmm2, R_SHUFFLEPS( 0, 1, 2, 3 )
						addps		xmm1, xmm0
						shufps		xmm1, xmm1, R_SHUFFLEPS( 0, 2, 1, 3 )
						movhlps		xmm0, xmm1
						addps		xmm0, xmm1
						STORE2LO( 0, xmm0, xmm3 )
					}
					return;
				}
				case 3: {		// 3x6 * 6x1
					__asm {
						mov			esi, vPtr
						mov			edi, mPtr
						mov			eax, dstPtr
						// load idVecX
						movlps		xmm4, [esi]
						movhps		xmm4, [esi+8]
						movlps		xmm5, [esi+16]
						movlhps		xmm5, xmm4
						movhlps		xmm6, xmm4
						movlhps		xmm6, xmm5
						// row 0 and 1
						movaps		xmm0, [edi]
						movaps		xmm1, [edi+16]
						movaps		xmm2, [edi+32]
						mulps		xmm0, xmm4
						mulps		xmm1, xmm5
						mulps		xmm2, xmm6
						movhlps		xmm3, xmm0
						movlhps		xmm3, xmm2
						addps		xmm1, xmm3
						shufps		xmm0, xmm2, R_SHUFFLEPS( 0, 1, 2, 3 )
						addps		xmm1, xmm0
						shufps		xmm1, xmm1, R_SHUFFLEPS( 0, 2, 1, 3 )
						movhlps		xmm0, xmm1
						addps		xmm0, xmm1
						STORE2LO( 0, xmm0, xmm3 )
						// row 2
						movaps		xmm0, [edi+48]
						movaps		xmm1, [edi+48+16]
						mulps		xmm0, xmm4
						mulps		xmm1, xmm5
						addps		xmm0, xmm1
						movhlps		xmm1, xmm0
						addps		xmm0, xmm1
						movaps		xmm1, xmm0
						shufps		xmm1, xmm1, R_SHUFFLEPS( 1, 0, 0, 0 )
						addss		xmm0, xmm1
						STORE1( 8, xmm0, xmm3 )
					}
					return;
				}
				case 4: {		// 4x6 * 6x1
					__asm {
						mov			esi, vPtr
						mov			edi, mPtr
						mov			eax, dstPtr
						// load idVecX
						movlps		xmm4, [esi]
						movhps		xmm4, [esi+8]
						movlps		xmm5, [esi+16]
						movlhps		xmm5, xmm4
						movhlps		xmm6, xmm4
						movlhps		xmm6, xmm5
						// row 0 and 1
						movaps		xmm0, [edi]
						movaps		xmm1, [edi+16]
						movaps		xmm2, [edi+32]
						mulps		xmm0, xmm4
						mulps		xmm1, xmm5
						mulps		xmm2, xmm6
						movhlps		xmm7, xmm0
						movlhps		xmm7, xmm2
						addps		xmm7, xmm1
						shufps		xmm0, xmm2, R_SHUFFLEPS( 0, 1, 2, 3 )
						addps		xmm7, xmm0
						// row 2 and 3
						movaps		xmm0, [edi+48]
						movaps		xmm1, [edi+48+16]
						movaps		xmm2, [edi+48+32]
						mulps		xmm0, xmm4
						mulps		xmm1, xmm5
						mulps		xmm2, xmm6
						movhlps		xmm3, xmm0
						movlhps		xmm3, xmm2
						addps		xmm1, xmm3
						shufps		xmm0, xmm2, R_SHUFFLEPS( 0, 1, 2, 3 )
						addps		xmm1, xmm0
						// last 4 additions for the first 4 rows and store result
						movaps		xmm0, xmm7
						shufps		xmm7, xmm1, R_SHUFFLEPS( 0, 2, 0, 2 )
						shufps		xmm0, xmm1, R_SHUFFLEPS( 1, 3, 1, 3 )
						addps		xmm0, xmm7
						STORE4( 0, xmm0, xmm4 )
					}
					return;
				}
				case 5: {		// 5x6 * 6x1
					__asm {
						mov			esi, vPtr
						mov			edi, mPtr
						mov			eax, dstPtr
						// load idVecX
						movlps		xmm4, [esi]
						movhps		xmm4, [esi+8]
						movlps		xmm5, [esi+16]
						movlhps		xmm5, xmm4
						movhlps		xmm6, xmm4
						movlhps		xmm6, xmm5
						// row 0 and 1
						movaps		xmm0, [edi]
						movaps		xmm1, [edi+16]
						movaps		xmm2, [edi+32]
						mulps		xmm0, xmm4
						mulps		xmm1, xmm5
						mulps		xmm2, xmm6
						movhlps		xmm7, xmm0
						movlhps		xmm7, xmm2
						addps		xmm7, xmm1
						shufps		xmm0, xmm2, R_SHUFFLEPS( 0, 1, 2, 3 )
						addps		xmm7, xmm0
						// row 2 and 3
						movaps		xmm0, [edi+48]
						movaps		xmm1, [edi+48+16]
						movaps		xmm2, [edi+48+32]
						mulps		xmm0, xmm4
						mulps		xmm1, xmm5
						mulps		xmm2, xmm6
						movhlps		xmm3, xmm0
						movlhps		xmm3, xmm2
						addps		xmm1, xmm3
						shufps		xmm0, xmm2, R_SHUFFLEPS( 0, 1, 2, 3 )
						addps		xmm1, xmm0
						// last 4 additions for the first 4 rows and store result
						movaps		xmm0, xmm7
						shufps		xmm7, xmm1, R_SHUFFLEPS( 0, 2, 0, 2 )
						shufps		xmm0, xmm1, R_SHUFFLEPS( 1, 3, 1, 3 )
						addps		xmm0, xmm7
						STORE4( 0, xmm0, xmm3 )
						// row 5
						movaps		xmm0, [edi+96]
						movaps		xmm1, [edi+96+16]
						mulps		xmm0, xmm4
						mulps		xmm1, xmm5
						addps		xmm0, xmm1
						movhlps		xmm1, xmm0
						addps		xmm0, xmm1
						movaps		xmm1, xmm0
						shufps		xmm1, xmm1, 0x01
						addss		xmm0, xmm1
						STORE1( 16, xmm0, xmm3 )
					}
					return;
				}
				case 6: {		// 6x6 * 6x1
					__asm {
						mov			esi, vPtr
						mov			edi, mPtr
						mov			eax, dstPtr
						movlps		xmm7, qword ptr [esi]
						movlps		xmm6, qword ptr [esi+8]
						shufps		xmm7, xmm7, 0x44
						shufps		xmm6, xmm6, 0x44
						movlps		xmm0, qword ptr [edi    ]
						movhps		xmm0, qword ptr [edi+ 24]
						mulps		xmm0, xmm7
						movlps		xmm3, qword ptr [edi+  8]
						movhps		xmm3, qword ptr [edi+ 32]
						mulps		xmm3, xmm6
						movlps		xmm1, qword ptr [edi+ 48]
						movhps		xmm1, qword ptr [edi+ 72]
						mulps		xmm1, xmm7
						movlps		xmm2, qword ptr [edi+ 96]
						movhps		xmm2, qword ptr [edi+120]
						mulps		xmm2, xmm7
						movlps		xmm4, qword ptr [edi+ 56]
						movhps		xmm4, qword ptr [edi+ 80]
						movlps		xmm5, qword ptr [edi+104]
						movhps		xmm5, qword ptr [edi+128]
						mulps		xmm4, xmm6
						movlps		xmm7, qword ptr [esi+16]
						addps		xmm0, xmm3
						shufps		xmm7, xmm7, 0x44
						mulps		xmm5, xmm6
						addps		xmm1, xmm4
						movlps		xmm3, qword ptr [edi+ 16]
						movhps		xmm3, qword ptr [edi+ 40]
						addps		xmm2, xmm5
						movlps		xmm4, qword ptr [edi+ 64]
						movhps		xmm4, qword ptr [edi+ 88]
						mulps		xmm3, xmm7
						movlps		xmm5, qword ptr [edi+112]
						movhps		xmm5, qword ptr [edi+136]
						addps		xmm0, xmm3
						mulps		xmm4, xmm7
						mulps		xmm5, xmm7
						addps		xmm1, xmm4
						addps		xmm2, xmm5
						movaps		xmm6, xmm0
						shufps		xmm0, xmm1, 0x88
						shufps		xmm6, xmm1, 0xDD
						movaps		xmm7, xmm2
						shufps		xmm7, xmm2, 0x88
						shufps		xmm2, xmm2, 0xDD
						addps		xmm0, xmm6
						addps		xmm2, xmm7
						STORE4( 0, xmm0, xmm3 )
						STORE2LO( 16, xmm2, xmm4 )
					}
					return;
				}
				default: {
					for ( int i = 0; i < numRows; i++ ) {
						dstPtr[i] STOREC mPtr[0] * vPtr[0] + mPtr[1] * vPtr[1] + mPtr[2] * vPtr[2] +
									mPtr[3] * vPtr[3] + mPtr[4] * vPtr[4] + mPtr[5] * vPtr[5];
						mPtr += 6;
					}
					return;
				}
			}
			break;
		}
		default: {
			int numColumns = mat.GetNumColumns();
			for ( int i = 0; i < numRows; i++ ) {
				float sum = mPtr[0] * vPtr[0];
				for ( int j = 1; j < numColumns; j++ ) {
					sum += mPtr[j] * vPtr[j];
				}
				dstPtr[i] STOREC sum;
				mPtr += numColumns;
			}
			break;
		}
	}

#undef STOREC
#undef STORE4
#undef STORE2HI
#undef STORE2LO
#undef STORE1
}

/*
============
idSIMD_SSE::MatX_TransposeMultiplyVecX

	optimizes the following matrix multiplications:

	Nx6 * Nx1
	6xN * 6x1

	with N in the range [1-6]
============
*/
void VPCALL idSIMD_SSE::MatX_TransposeMultiplyVecX( idVecX &dst, const idMatX &mat, const idVecX &vec ) {
#define STORE1( offset, reg1, reg2 )		\
	__asm movss		[eax+offset], reg1
#define STORE2LO( offset, reg1, reg2 )		\
	__asm movlps	[eax+offset], reg1
#define STORE2HI( offset, reg1, reg2 )		\
	__asm movhps	[eax+offset], reg1
#define STORE4( offset, reg1, reg2 )		\
	__asm movlps	[eax+offset], reg1		\
	__asm movhps	[eax+offset+8], reg1
#define STOREC		=

	int numColumns;
	const float *mPtr, *vPtr;
	float *dstPtr;

	assert( vec.GetSize() >= mat.GetNumRows() );
	assert( dst.GetSize() >= mat.GetNumColumns() );

	mPtr = mat.ToFloatPtr();
	vPtr = vec.ToFloatPtr();
	dstPtr = dst.ToFloatPtr();
	numColumns = mat.GetNumColumns();
	switch( mat.GetNumRows() ) {
		case 1:
			switch( numColumns ) {
				case 6: {		// 1x6 * 1x1
					__asm {
						mov			esi, vPtr
						mov			edi, mPtr
						mov			eax, dstPtr
						movss		xmm0, [esi]
						shufps		xmm0, xmm0, R_SHUFFLEPS( 0, 0, 0, 0 )
						movaps		xmm1, xmm0
						mulps		xmm0, [edi]
						mulps		xmm1, [edi+16]
						STORE4( 0, xmm0, xmm2 )
						STORE2LO( 16, xmm1, xmm3 )
					}
					return;
				}
				default: {
					for ( int i = 0; i < numColumns; i++ ) {
						dstPtr[i] STOREC *(mPtr) * vPtr[0];
						mPtr++;
					}
					return;
				}
			}
			break;
		case 2:
			switch( numColumns ) {
				case 6: {		// 2x6 * 2x1
					__asm {
						mov			esi, vPtr
						mov			edi, mPtr
						mov			eax, dstPtr
						movlps		xmm0, [esi]
						movaps		xmm1, xmm0
						shufps		xmm0, xmm0, R_SHUFFLEPS( 0, 0, 0, 0 )
						shufps		xmm1, xmm1, R_SHUFFLEPS( 1, 1, 1, 1 )
						movaps		xmm2, [edi]
						mulps		xmm2, xmm0
						movlps		xmm3, [edi+24]
						movhps		xmm3, [edi+32]
						mulps		xmm3, xmm1
						addps		xmm2, xmm3
						shufps		xmm0, xmm1, R_SHUFFLEPS( 0, 0, 0, 0 )
						movlps		xmm4, [edi+16]
						movhps		xmm4, [edi+40]
						mulps		xmm4, xmm0
						movhlps		xmm3, xmm4
						addps		xmm3, xmm4
						STORE4( 0, xmm2, xmm5 )
						STORE2LO( 16, xmm3, xmm6 )
					}
					return;
				}
				default: {
					for ( int i = 0; i < numColumns; i++ ) {
						dstPtr[i] STOREC *(mPtr) * vPtr[0] + *(mPtr+numColumns) * vPtr[1];
						mPtr++;
					}
					return;
				}
			}
			break;
		case 3:
			switch( numColumns ) {
				case 6: {		// 3x6 * 3x1
					__asm {
						mov			esi, vPtr
						mov			edi, mPtr
						mov			eax, dstPtr
						movlps		xmm0, [esi+0*4]
						movss		xmm1, [esi+2*4]
						movlps		xmm3, [edi+(0*6+0)*4]
						movhps		xmm3, [edi+(0*6+2)*4]
						movaps		xmm4, xmm0
						shufps		xmm4, xmm4, R_SHUFFLEPS( 0, 0, 0, 0 )
						mulps		xmm3, xmm4
						movlps		xmm5, [edi+(1*6+0)*4]
						movhps		xmm5, [edi+(1*6+2)*4]
						movaps		xmm6, xmm0
						shufps		xmm6, xmm6, R_SHUFFLEPS( 1, 1, 1, 1 )
						mulps		xmm5, xmm6
						addps		xmm3, xmm5
						movlps		xmm4, [edi+(2*6+0)*4]
						movhps		xmm4, [edi+(2*6+2)*4]
						shufps		xmm1, xmm1, R_SHUFFLEPS( 0, 0, 0, 0 )
						mulps		xmm4, xmm1
						addps		xmm3, xmm4
						STORE4( 0, xmm3, xmm7 )
						shufps		xmm0, xmm0, R_SHUFFLEPS( 0, 0, 1, 1 )
						movlps		xmm3, [edi+(0*6+4)*4]
						movhps		xmm3, [edi+(1*6+4)*4]
						mulps		xmm3, xmm0
						movhlps		xmm4, xmm3
						addps		xmm3, xmm4
						movlps		xmm5, [edi+(2*6+4)*4]
						mulps		xmm5, xmm1
						addps		xmm3, xmm5
						STORE2LO( 16, xmm3, xmm7 )
					}
					return;
				}
				default: {
					for ( int i = 0; i < numColumns; i++ ) {
						dstPtr[i] STOREC *(mPtr) * vPtr[0] + *(mPtr+numColumns) * vPtr[1] + *(mPtr+2*numColumns) * vPtr[2];
						mPtr++;
					}
					return;
				}
			}
			break;
		case 4:
			switch( numColumns ) {
				case 6: {		// 4x6 * 4x1
					__asm {
						mov			esi, vPtr
						mov			edi, mPtr
						mov			eax, dstPtr
						movlps		xmm0, [esi+0*4]
						movlps		xmm1, [esi+2*4]
						movaps		xmm3, xmm0
						shufps		xmm3, xmm3, R_SHUFFLEPS( 0, 0, 0, 0 )
						mulps		xmm3, [edi+(0*6+0)*4]
						movlps		xmm5, [edi+(1*6+0)*4]
						movhps		xmm5, [edi+(1*6+2)*4]
						movaps		xmm6, xmm0
						shufps		xmm6, xmm6, R_SHUFFLEPS( 1, 1, 1, 1 )
						mulps		xmm5, xmm6
						addps		xmm3, xmm5
						movlps		xmm4, [edi+(2*6+0)*4]
						movhps		xmm4, [edi+(2*6+2)*4]
						movaps		xmm6, xmm1
						shufps		xmm6, xmm6, R_SHUFFLEPS( 0, 0, 0, 0 )
						mulps		xmm4, xmm6
						addps		xmm3, xmm4
						movlps		xmm5, [edi+(3*6+0)*4]
						movhps		xmm5, [edi+(3*6+2)*4]
						movaps		xmm6, xmm1
						shufps		xmm6, xmm6, R_SHUFFLEPS( 1, 1, 1, 1 )
						mulps		xmm5, xmm6
						addps		xmm3, xmm5
						STORE4( 0, xmm3, xmm7 )
						shufps		xmm0, xmm0, R_SHUFFLEPS( 0, 0, 1, 1 )
						shufps		xmm1, xmm1, R_SHUFFLEPS( 0, 0, 1, 1 )
						movlps		xmm3, [edi+(0*6+4)*4]
						movhps		xmm3, [edi+(1*6+4)*4]
						mulps		xmm3, xmm0
						movlps		xmm4, [edi+(2*6+4)*4]
						movhps		xmm4, [edi+(3*6+4)*4]
						mulps		xmm4, xmm1
						addps		xmm3, xmm4
						movhlps		xmm4, xmm3
						addps		xmm3, xmm4
						STORE2LO( 16, xmm3, xmm7 )
					}
					return;
				}
				default: {
					for ( int i = 0; i < numColumns; i++ ) {
						dstPtr[i] STOREC *(mPtr) * vPtr[0] + *(mPtr+numColumns) * vPtr[1] + *(mPtr+2*numColumns) * vPtr[2] +
								*(mPtr+3*numColumns) * vPtr[3];
						mPtr++;
					}
					return;
				}
			}
			break;
		case 5:
			switch( numColumns ) {
				case 6: {		// 5x6 * 5x1
					__asm {
						mov			esi, vPtr
						mov			edi, mPtr
						mov			eax, dstPtr
						movlps		xmm0, [esi+0*4]
						movlps		xmm1, [esi+2*4]
						movss		xmm2, [esi+4*4]
						movaps		xmm3, xmm0
						shufps		xmm3, xmm3, R_SHUFFLEPS( 0, 0, 0, 0 )
						mulps		xmm3, [edi+(0*6+0)*4]
						movlps		xmm5, [edi+(1*6+0)*4]
						movhps		xmm5, [edi+(1*6+2)*4]
						movaps		xmm6, xmm0
						shufps		xmm6, xmm6, R_SHUFFLEPS( 1, 1, 1, 1 )
						mulps		xmm5, xmm6
						addps		xmm3, xmm5
						movaps		xmm6, xmm1
						shufps		xmm6, xmm6, R_SHUFFLEPS( 0, 0, 0, 0 )
						mulps		xmm6, [edi+(2*6+0)*4]
						addps		xmm3, xmm6
						movlps		xmm5, [edi+(3*6+0)*4]
						movhps		xmm5, [edi+(3*6+2)*4]
						movaps		xmm6, xmm1
						shufps		xmm6, xmm6, R_SHUFFLEPS( 1, 1, 1, 1 )
						mulps		xmm5, xmm6
						addps		xmm3, xmm5
						shufps		xmm2, xmm2, R_SHUFFLEPS( 0, 0, 0, 0 )
						movaps		xmm4, xmm2
						mulps		xmm4, [edi+(4*6+0)*4]
						addps		xmm3, xmm4
						STORE4( 0, xmm3, xmm7 )
						shufps		xmm0, xmm0, R_SHUFFLEPS( 0, 0, 1, 1 )
						shufps		xmm1, xmm1, R_SHUFFLEPS( 0, 0, 1, 1 )
						movlps		xmm3, [edi+(0*6+4)*4]
						movhps		xmm3, [edi+(1*6+4)*4]
						mulps		xmm3, xmm0
						movlps		xmm4, [edi+(2*6+4)*4]
						movhps		xmm4, [edi+(3*6+4)*4]
						mulps		xmm4, xmm1
						addps		xmm3, xmm4
						movhlps		xmm4, xmm3
						addps		xmm3, xmm4
						movlps		xmm5, [edi+(4*6+4)*4]
						mulps		xmm5, xmm2
						addps		xmm3, xmm5
						STORE2LO( 16, xmm3, xmm7 )
					}
					return;
				}
				default: {
					for ( int i = 0; i < numColumns; i++ ) {
						dstPtr[i] STOREC *(mPtr) * vPtr[0] + *(mPtr+numColumns) * vPtr[1] + *(mPtr+2*numColumns) * vPtr[2] +
								*(mPtr+3*numColumns) * vPtr[3] + *(mPtr+4*numColumns) * vPtr[4];
						mPtr++;
					}
					return;
				}
			}
			break;
		case 6:
			switch( numColumns ) {
				case 1: {		// 6x1 * 6x1
					__asm {
						mov			esi, vPtr
						mov			edi, mPtr
						mov			eax, dstPtr
						movlps		xmm0, [esi]
						movhps		xmm0, [esi+8]
						movlps		xmm1, [esi+16]
						mulps		xmm0, [edi]
						mulps		xmm1, [edi+16]
						shufps		xmm1, xmm0, R_SHUFFLEPS( 0, 1, 3, 2 )
						addps		xmm0, xmm1
						movhlps		xmm2, xmm0
						addss		xmm2, xmm0
						shufps		xmm0, xmm0, R_SHUFFLEPS( 1, 0, 0, 0 )
						addss		xmm2, xmm0
						STORE1( 0, xmm2, xmm3 )
					}
					return;
				}
				case 2: {		// 6x2 * 6x1
					__asm {
						mov			esi, vPtr
						mov			edi, mPtr
						mov			eax, dstPtr
						movlps		xmm0, [esi+0*4]
						shufps		xmm0, xmm0, R_SHUFFLEPS( 0, 0, 1, 1 )
						movaps		xmm6, [edi+0*4]
						mulps		xmm6, xmm0
						movlps		xmm1, [esi+2*4]
						shufps		xmm1, xmm1, R_SHUFFLEPS( 0, 0, 1, 1 )
						movaps		xmm7, [edi+4*4]
						mulps		xmm7, xmm1
						addps		xmm6, xmm7
						movlps		xmm2, [esi+4*4]
						shufps		xmm2, xmm2, R_SHUFFLEPS( 0, 0, 1, 1 )
						movaps		xmm7, [edi+8*4]
						mulps		xmm7, xmm2
						addps		xmm6, xmm7
						movhlps		xmm3, xmm6
						addps		xmm3, xmm6
						STORE2LO( 0, xmm3, xmm7 )
					}
					return;
				}
				case 3: {		// 6x3 * 6x1
					__asm {
						mov			esi, vPtr
						mov			edi, mPtr
						mov			eax, dstPtr
						movss		xmm0, [edi+(0*3+2)*4]
						movhps		xmm0, [edi+(0*3+0)*4]
						shufps		xmm0, xmm0, R_SHUFFLEPS( 2, 1, 3, 0 )
						movss		xmm6, [esi+0*4]
						shufps		xmm6, xmm6, R_SHUFFLEPS( 0, 0, 0, 0 )
						mulps		xmm6, xmm0
						movss		xmm1, [edi+(1*3+0)*4]
						movhps		xmm1, [edi+(1*3+1)*4]
						movss		xmm7, [esi+1*4]
						shufps		xmm7, xmm7, R_SHUFFLEPS( 0, 0, 0, 0 )
						mulps		xmm7, xmm1
						addps		xmm6, xmm7
						movss		xmm2, [edi+(2*3+2)*4]
						movhps		xmm2, [edi+(2*3+0)*4]
						shufps		xmm2, xmm2, R_SHUFFLEPS( 2, 1, 3, 0 )
						movss		xmm7, [esi+2*4]
						shufps		xmm7, xmm7, R_SHUFFLEPS( 0, 0, 0, 0 )
						mulps		xmm7, xmm2
						addps		xmm6, xmm7
						movss		xmm3, [edi+(3*3+0)*4]
						movhps		xmm3, [edi+(3*3+1)*4]
						movss		xmm7, [esi+3*4]
						shufps		xmm7, xmm7, R_SHUFFLEPS( 0, 0, 0, 0 )
						mulps		xmm7, xmm3
						addps		xmm6, xmm7
						movss		xmm4, [edi+(4*3+2)*4]
						movhps		xmm4, [edi+(4*3+0)*4]
						shufps		xmm4, xmm4, R_SHUFFLEPS( 2, 1, 3, 0 )
						movss		xmm7, [esi+4*4]
						shufps		xmm7, xmm7, R_SHUFFLEPS( 0, 0, 0, 0 )
						mulps		xmm7, xmm4
						addps		xmm6, xmm7
						movss		xmm5, [edi+(5*3+0)*4]
						movhps		xmm5, [edi+(5*3+1)*4]
						movss		xmm7, [esi+5*4]
						shufps		xmm7, xmm7, R_SHUFFLEPS( 0, 0, 0, 0 )
						mulps		xmm7, xmm5
						addps		xmm6, xmm7
						STORE1( 0, xmm6, xmm7 )
						STORE2HI( 4, xmm6, xmm7 )
					}
					return;
				}
				case 4: {		// 6x4 * 6x1
					__asm {
						mov			esi, vPtr
						mov			edi, mPtr
						mov			eax, dstPtr
						movlps		xmm3, [edi+(0*4+0)*4]
						movhps		xmm3, [edi+(0*4+2)*4]
						movss		xmm4, [esi+0*4]
						shufps		xmm4, xmm4, R_SHUFFLEPS( 0, 0, 0, 0 )
						mulps		xmm3, xmm4
						movlps		xmm5, [edi+(1*4+0)*4]
						movhps		xmm5, [edi+(1*4+2)*4]
						movss		xmm6, [esi+1*4]
						shufps		xmm6, xmm6, R_SHUFFLEPS( 0, 0, 0, 0 )
						mulps		xmm5, xmm6
						addps		xmm3, xmm5
						movlps		xmm4, [edi+(2*4+0)*4]
						movhps		xmm4, [edi+(2*4+2)*4]
						movss		xmm6, [esi+2*4]
						shufps		xmm6, xmm6, R_SHUFFLEPS( 0, 0, 0, 0 )
						mulps		xmm4, xmm6
						addps		xmm3, xmm4
						movlps		xmm5, [edi+(3*4+0)*4]
						movhps		xmm5, [edi+(3*4+2)*4]
						movss		xmm6, [esi+3*4]
						shufps		xmm6, xmm6, R_SHUFFLEPS( 0, 0, 0, 0 )
						mulps		xmm5, xmm6
						addps		xmm3, xmm5
						movlps		xmm4, [edi+(4*4+0)*4]
						movhps		xmm4, [edi+(4*4+2)*4]
						movss		xmm6, [esi+4*4]
						shufps		xmm6, xmm6, R_SHUFFLEPS( 0, 0, 0, 0 )
						mulps		xmm4, xmm6
						addps		xmm3, xmm4
						movlps		xmm5, [edi+(5*4+0)*4]
						movhps		xmm5, [edi+(5*4+2)*4]
						movss		xmm6, [esi+5*4]
						shufps		xmm6, xmm6, R_SHUFFLEPS( 0, 0, 0, 0 )
						mulps		xmm5, xmm6
						addps		xmm3, xmm5
						STORE4( 0, xmm3, xmm7 )
					}
					return;
				}
				case 5: {		// 6x5 * 6x1
					__asm {
						mov			esi, vPtr
						mov			edi, mPtr
						mov			eax, dstPtr
						movlps		xmm6, [edi+(0*5+0)*4]
						movhps		xmm6, [edi+(0*5+2)*4]
						movss		xmm0, [esi+0*4]
						shufps		xmm0, xmm0, R_SHUFFLEPS( 0, 0, 0, 0 )
						mulps		xmm6, xmm0
						movlps		xmm7, [edi+(1*5+0)*4]
						movhps		xmm7, [edi+(1*5+2)*4]
						movss		xmm1, [esi+1*4]
						shufps		xmm1, xmm1, R_SHUFFLEPS( 0, 0, 0, 0 )
						mulps		xmm7, xmm1
						addps		xmm6, xmm7
						movlps		xmm7, [edi+(2*5+0)*4]
						movhps		xmm7, [edi+(2*5+2)*4]
						movss		xmm2, [esi+2*4]
						shufps		xmm2, xmm2, R_SHUFFLEPS( 0, 0, 0, 0 )
						mulps		xmm7, xmm2
						addps		xmm6, xmm7
						movlps		xmm7, [edi+(3*5+0)*4]
						movhps		xmm7, [edi+(3*5+2)*4]
						movss		xmm3, [esi+3*4]
						shufps		xmm3, xmm3, R_SHUFFLEPS( 0, 0, 0, 0 )
						mulps		xmm7, xmm3
						addps		xmm6, xmm7
						movlps		xmm7, [edi+(4*5+0)*4]
						movhps		xmm7, [edi+(4*5+2)*4]
						movss		xmm4, [esi+4*4]
						shufps		xmm4, xmm4, R_SHUFFLEPS( 0, 0, 0, 0 )
						mulps		xmm7, xmm4
						addps		xmm6, xmm7
						movlps		xmm7, [edi+(5*5+0)*4]
						movhps		xmm7, [edi+(5*5+2)*4]
						movss		xmm5, [esi+5*4]
						shufps		xmm5, xmm5, R_SHUFFLEPS( 0, 0, 0, 0 )
						mulps		xmm7, xmm5
						addps		xmm6, xmm7
						STORE4( 0, xmm6, xmm7 )
						movss		xmm6, [edi+(0*5+4)*4]
						mulss		xmm6, xmm0
						movss		xmm7, [edi+(1*5+4)*4]
						mulss		xmm7, xmm1
						addss		xmm6, xmm7
						movss		xmm7, [edi+(2*5+4)*4]
						mulss		xmm7, xmm2
						addss		xmm6, xmm7
						movss		xmm7, [edi+(3*5+4)*4]
						mulss		xmm7, xmm3
						addss		xmm6, xmm7
						movss		xmm7, [edi+(4*5+4)*4]
						mulss		xmm7, xmm4
						addss		xmm6, xmm7
						movss		xmm7, [edi+(5*5+4)*4]
						mulss		xmm7, xmm5
						addss		xmm6, xmm7
						STORE1( 16, xmm6, xmm7 )
					}
					return;
				}
				case 6: {		// 6x6 * 6x1
					__asm {
						mov			esi, vPtr
						mov			edi, mPtr
						mov			eax, dstPtr
						movlps		xmm0, [esi+0*4]
						movlps		xmm1, [esi+2*4]
						movlps		xmm2, [esi+4*4]
						movaps		xmm3, xmm0
						shufps		xmm3, xmm3, R_SHUFFLEPS( 0, 0, 0, 0 )
						mulps		xmm3, [edi+(0*6+0)*4]
						movlps		xmm5, [edi+(1*6+0)*4]
						movhps		xmm5, [edi+(1*6+2)*4]
						movaps		xmm6, xmm0
						shufps		xmm6, xmm6, R_SHUFFLEPS( 1, 1, 1, 1 )
						mulps		xmm5, xmm6
						addps		xmm3, xmm5
						movaps		xmm6, xmm1
						shufps		xmm6, xmm6, R_SHUFFLEPS( 0, 0, 0, 0 )
						mulps		xmm6, [edi+(2*6+0)*4]
						addps		xmm3, xmm6
						movaps		xmm6, xmm1
						shufps		xmm6, xmm6, R_SHUFFLEPS( 1, 1, 1, 1 )
						movlps		xmm5, [edi+(3*6+0)*4]
						movhps		xmm5, [edi+(3*6+2)*4]
						mulps		xmm5, xmm6
						addps		xmm3, xmm5
						movaps		xmm6, xmm2
						shufps		xmm6, xmm6, R_SHUFFLEPS( 0, 0, 0, 0 )
						mulps		xmm6, [edi+(4*6+0)*4]
						addps		xmm3, xmm6
						movaps		xmm6, xmm2
						shufps		xmm6, xmm6, R_SHUFFLEPS( 1, 1, 1, 1 )
						movlps		xmm5, [edi+(5*6+0)*4]
						movhps		xmm5, [edi+(5*6+2)*4]
						mulps		xmm5, xmm6
						addps		xmm3, xmm5
						STORE4( 0, xmm3, xmm7 )
						shufps		xmm0, xmm0, R_SHUFFLEPS( 0, 0, 1, 1 )
						shufps		xmm1, xmm1, R_SHUFFLEPS( 0, 0, 1, 1 )
						shufps		xmm2, xmm2, R_SHUFFLEPS( 0, 0, 1, 1 )
						movlps		xmm3, [edi+(0*6+4)*4]
						movhps		xmm3, [edi+(1*6+4)*4]
						mulps		xmm3, xmm0
						movlps		xmm4, [edi+(2*6+4)*4]
						movhps		xmm4, [edi+(3*6+4)*4]
						mulps		xmm4, xmm1
						addps		xmm3, xmm4
						movlps		xmm5, [edi+(4*6+4)*4]
						movhps		xmm5, [edi+(5*6+4)*4]
						mulps		xmm5, xmm2
						addps		xmm3, xmm5
						movhlps		xmm4, xmm3
						addps		xmm3, xmm4
						STORE2LO( 16, xmm3, xmm7 )
					}
					return;
				}
				default: {
					for ( int i = 0; i < numColumns; i++ ) {
						dstPtr[i] STOREC *(mPtr) * vPtr[0] + *(mPtr+numColumns) * vPtr[1] + *(mPtr+2*numColumns) * vPtr[2] +
								*(mPtr+3*numColumns) * vPtr[3] + *(mPtr+4*numColumns) * vPtr[4] + *(mPtr+5*numColumns) * vPtr[5];
						mPtr++;
					}
					return;
				}
			}
			break;
		default:
			int numRows = mat.GetNumRows();
			for ( int i = 0; i < numColumns; i++ ) {
				mPtr = mat.ToFloatPtr() + i;
				float sum = mPtr[0] * vPtr[0];
				for ( int j = 1; j < numRows; j++ ) {
					mPtr += numColumns;
					sum += mPtr[0] * vPtr[j];
				}
				dstPtr[i] STOREC sum;
			}
			break;
	}

#undef STOREC
#undef STORE4
#undef STORE2HI
#undef STORE2LO
#undef STORE1
}

/*
============
idSIMD_SSE::MatX_TransposeMultiplyAddVecX

	optimizes the following matrix multiplications:

	Nx6 * Nx1
	6xN * 6x1

	with N in the range [1-6]
============
*/
void VPCALL idSIMD_SSE::MatX_TransposeMultiplyAddVecX( idVecX &dst, const idMatX &mat, const idVecX &vec ) {
#define STORE1( offset, reg1, reg2 )		\
	__asm movss		reg2, [eax+offset]		\
	__asm addss		reg2, reg1				\
	__asm movss		[eax+offset], reg2
#define STORE2LO( offset, reg1, reg2 )		\
	__asm movlps	reg2, [eax+offset]		\
	__asm addps		reg2, reg1				\
	__asm movlps	[eax+offset], reg2
#define STORE2HI( offset, reg1, reg2 )		\
	__asm movhps	reg2, [eax+offset]		\
	__asm addps		reg2, reg1				\
	__asm movhps	[eax+offset], reg2
#define STORE4( offset, reg1, reg2 )		\
	__asm movlps	reg2, [eax+offset]		\
	__asm movhps	reg2, [eax+offset+8]	\
	__asm addps		reg2, reg1				\
	__asm movlps	[eax+offset], reg2		\
	__asm movhps	[eax+offset+8], reg2
#define STOREC		+=

	int numColumns;
	const float *mPtr, *vPtr;
	float *dstPtr;

	assert( vec.GetSize() >= mat.GetNumRows() );
	assert( dst.GetSize() >= mat.GetNumColumns() );

	mPtr = mat.ToFloatPtr();
	vPtr = vec.ToFloatPtr();
	dstPtr = dst.ToFloatPtr();
	numColumns = mat.GetNumColumns();
	switch( mat.GetNumRows() ) {
		case 1:
			switch( numColumns ) {
				case 6: {		// 1x6 * 1x1
					__asm {
						mov			esi, vPtr
						mov			edi, mPtr
						mov			eax, dstPtr
						movss		xmm0, [esi]
						shufps		xmm0, xmm0, R_SHUFFLEPS( 0, 0, 0, 0 )
						movaps		xmm1, xmm0
						mulps		xmm0, [edi]
						mulps		xmm1, [edi+16]
						STORE4( 0, xmm0, xmm2 )
						STORE2LO( 16, xmm1, xmm3 )
					}
					return;
				}
				default: {
					for ( int i = 0; i < numColumns; i++ ) {
						dstPtr[i] STOREC *(mPtr) * vPtr[0];
						mPtr++;
					}
					return;
				}
			}
			break;
		case 2:
			switch( numColumns ) {
				case 6: {		// 2x6 * 2x1
					__asm {
						mov			esi, vPtr
						mov			edi, mPtr
						mov			eax, dstPtr
						movlps		xmm0, [esi]
						movaps		xmm1, xmm0
						shufps		xmm0, xmm0, R_SHUFFLEPS( 0, 0, 0, 0 )
						shufps		xmm1, xmm1, R_SHUFFLEPS( 1, 1, 1, 1 )
						movaps		xmm2, [edi]
						mulps		xmm2, xmm0
						movlps		xmm3, [edi+24]
						movhps		xmm3, [edi+32]
						mulps		xmm3, xmm1
						addps		xmm2, xmm3
						shufps		xmm0, xmm1, R_SHUFFLEPS( 0, 0, 0, 0 )
						movlps		xmm4, [edi+16]
						movhps		xmm4, [edi+40]
						mulps		xmm4, xmm0
						movhlps		xmm3, xmm4
						addps		xmm3, xmm4
						STORE4( 0, xmm2, xmm5 )
						STORE2LO( 16, xmm3, xmm6 )
					}
					return;
				}
				default: {
					for ( int i = 0; i < numColumns; i++ ) {
						dstPtr[i] STOREC *(mPtr) * vPtr[0] + *(mPtr+numColumns) * vPtr[1];
						mPtr++;
					}
					return;
				}
			}
			break;
		case 3:
			switch( numColumns ) {
				case 6: {		// 3x6 * 3x1
					__asm {
						mov			esi, vPtr
						mov			edi, mPtr
						mov			eax, dstPtr
						movlps		xmm0, [esi+0*4]
						movss		xmm1, [esi+2*4]
						movlps		xmm3, [edi+(0*6+0)*4]
						movhps		xmm3, [edi+(0*6+2)*4]
						movaps		xmm4, xmm0
						shufps		xmm4, xmm4, R_SHUFFLEPS( 0, 0, 0, 0 )
						mulps		xmm3, xmm4
						movlps		xmm5, [edi+(1*6+0)*4]
						movhps		xmm5, [edi+(1*6+2)*4]
						movaps		xmm6, xmm0
						shufps		xmm6, xmm6, R_SHUFFLEPS( 1, 1, 1, 1 )
						mulps		xmm5, xmm6
						addps		xmm3, xmm5
						movlps		xmm4, [edi+(2*6+0)*4]
						movhps		xmm4, [edi+(2*6+2)*4]
						shufps		xmm1, xmm1, R_SHUFFLEPS( 0, 0, 0, 0 )
						mulps		xmm4, xmm1
						addps		xmm3, xmm4
						STORE4( 0, xmm3, xmm7 )
						shufps		xmm0, xmm0, R_SHUFFLEPS( 0, 0, 1, 1 )
						movlps		xmm3, [edi+(0*6+4)*4]
						movhps		xmm3, [edi+(1*6+4)*4]
						mulps		xmm3, xmm0
						movhlps		xmm4, xmm3
						addps		xmm3, xmm4
						movlps		xmm5, [edi+(2*6+4)*4]
						mulps		xmm5, xmm1
						addps		xmm3, xmm5
						STORE2LO( 16, xmm3, xmm7 )
					}
					return;
				}
				default: {
					for ( int i = 0; i < numColumns; i++ ) {
						dstPtr[i] STOREC *(mPtr) * vPtr[0] + *(mPtr+numColumns) * vPtr[1] + *(mPtr+2*numColumns) * vPtr[2];
						mPtr++;
					}
					return;
				}
			}
			break;
		case 4:
			switch( numColumns ) {
				case 6: {		// 4x6 * 4x1
					__asm {
						mov			esi, vPtr
						mov			edi, mPtr
						mov			eax, dstPtr
						movlps		xmm0, [esi+0*4]
						movlps		xmm1, [esi+2*4]
						movaps		xmm3, xmm0
						shufps		xmm3, xmm3, R_SHUFFLEPS( 0, 0, 0, 0 )
						mulps		xmm3, [edi+(0*6+0)*4]
						movlps		xmm5, [edi+(1*6+0)*4]
						movhps		xmm5, [edi+(1*6+2)*4]
						movaps		xmm6, xmm0
						shufps		xmm6, xmm6, R_SHUFFLEPS( 1, 1, 1, 1 )
						mulps		xmm5, xmm6
						addps		xmm3, xmm5
						movlps		xmm4, [edi+(2*6+0)*4]
						movhps		xmm4, [edi+(2*6+2)*4]
						movaps		xmm6, xmm1
						shufps		xmm6, xmm6, R_SHUFFLEPS( 0, 0, 0, 0 )
						mulps		xmm4, xmm6
						addps		xmm3, xmm4
						movlps		xmm5, [edi+(3*6+0)*4]
						movhps		xmm5, [edi+(3*6+2)*4]
						movaps		xmm6, xmm1
						shufps		xmm6, xmm6, R_SHUFFLEPS( 1, 1, 1, 1 )
						mulps		xmm5, xmm6
						addps		xmm3, xmm5
						STORE4( 0, xmm3, xmm7 )
						shufps		xmm0, xmm0, R_SHUFFLEPS( 0, 0, 1, 1 )
						shufps		xmm1, xmm1, R_SHUFFLEPS( 0, 0, 1, 1 )
						movlps		xmm3, [edi+(0*6+4)*4]
						movhps		xmm3, [edi+(1*6+4)*4]
						mulps		xmm3, xmm0
						movlps		xmm4, [edi+(2*6+4)*4]
						movhps		xmm4, [edi+(3*6+4)*4]
						mulps		xmm4, xmm1
						addps		xmm3, xmm4
						movhlps		xmm4, xmm3
						addps		xmm3, xmm4
						STORE2LO( 16, xmm3, xmm7 )
					}
					return;
				}
				default: {
					for ( int i = 0; i < numColumns; i++ ) {
						dstPtr[i] STOREC *(mPtr) * vPtr[0] + *(mPtr+numColumns) * vPtr[1] + *(mPtr+2*numColumns) * vPtr[2] +
								*(mPtr+3*numColumns) * vPtr[3];
						mPtr++;
					}
					return;
				}
			}
			break;
		case 5:
			switch( numColumns ) {
				case 6: {		// 5x6 * 5x1
					__asm {
						mov			esi, vPtr
						mov			edi, mPtr
						mov			eax, dstPtr
						movlps		xmm0, [esi+0*4]
						movlps		xmm1, [esi+2*4]
						movss		xmm2, [esi+4*4]
						movaps		xmm3, xmm0
						shufps		xmm3, xmm3, R_SHUFFLEPS( 0, 0, 0, 0 )
						mulps		xmm3, [edi+(0*6+0)*4]
						movlps		xmm5, [edi+(1*6+0)*4]
						movhps		xmm5, [edi+(1*6+2)*4]
						movaps		xmm6, xmm0
						shufps		xmm6, xmm6, R_SHUFFLEPS( 1, 1, 1, 1 )
						mulps		xmm5, xmm6
						addps		xmm3, xmm5
						movaps		xmm6, xmm1
						shufps		xmm6, xmm6, R_SHUFFLEPS( 0, 0, 0, 0 )
						mulps		xmm6, [edi+(2*6+0)*4]
						addps		xmm3, xmm6
						movlps		xmm5, [edi+(3*6+0)*4]
						movhps		xmm5, [edi+(3*6+2)*4]
						movaps		xmm6, xmm1
						shufps		xmm6, xmm6, R_SHUFFLEPS( 1, 1, 1, 1 )
						mulps		xmm5, xmm6
						addps		xmm3, xmm5
						shufps		xmm2, xmm2, R_SHUFFLEPS( 0, 0, 0, 0 )
						movaps		xmm4, xmm2
						mulps		xmm4, [edi+(4*6+0)*4]
						addps		xmm3, xmm4
						STORE4( 0, xmm3, xmm7 )
						shufps		xmm0, xmm0, R_SHUFFLEPS( 0, 0, 1, 1 )
						shufps		xmm1, xmm1, R_SHUFFLEPS( 0, 0, 1, 1 )
						movlps		xmm3, [edi+(0*6+4)*4]
						movhps		xmm3, [edi+(1*6+4)*4]
						mulps		xmm3, xmm0
						movlps		xmm4, [edi+(2*6+4)*4]
						movhps		xmm4, [edi+(3*6+4)*4]
						mulps		xmm4, xmm1
						addps		xmm3, xmm4
						movhlps		xmm4, xmm3
						addps		xmm3, xmm4
						movlps		xmm5, [edi+(4*6+4)*4]
						mulps		xmm5, xmm2
						addps		xmm3, xmm5
						STORE2LO( 16, xmm3, xmm7 )
					}
					return;
				}
				default: {
					for ( int i = 0; i < numColumns; i++ ) {
						dstPtr[i] STOREC *(mPtr) * vPtr[0] + *(mPtr+numColumns) * vPtr[1] + *(mPtr+2*numColumns) * vPtr[2] +
								*(mPtr+3*numColumns) * vPtr[3] + *(mPtr+4*numColumns) * vPtr[4];
						mPtr++;
					}
					return;
				}
			}
			break;
		case 6:
			switch( numColumns ) {
				case 1: {		// 6x1 * 6x1
					__asm {
						mov			esi, vPtr
						mov			edi, mPtr
						mov			eax, dstPtr
						movlps		xmm0, [esi]
						movhps		xmm0, [esi+8]
						movlps		xmm1, [esi+16]
						mulps		xmm0, [edi]
						mulps		xmm1, [edi+16]
						shufps		xmm1, xmm0, R_SHUFFLEPS( 0, 1, 3, 2 )
						addps		xmm0, xmm1
						movhlps		xmm2, xmm0
						addss		xmm2, xmm0
						shufps		xmm0, xmm0, R_SHUFFLEPS( 1, 0, 0, 0 )
						addss		xmm2, xmm0
						STORE1( 0, xmm2, xmm3 )
					}
					return;
				}
				case 2: {		// 6x2 * 6x1
					__asm {
						mov			esi, vPtr
						mov			edi, mPtr
						mov			eax, dstPtr
						movlps		xmm0, [esi+0*4]
						shufps		xmm0, xmm0, R_SHUFFLEPS( 0, 0, 1, 1 )
						movaps		xmm6, [edi+0*4]
						mulps		xmm6, xmm0
						movlps		xmm1, [esi+2*4]
						shufps		xmm1, xmm1, R_SHUFFLEPS( 0, 0, 1, 1 )
						movaps		xmm7, [edi+4*4]
						mulps		xmm7, xmm1
						addps		xmm6, xmm7
						movlps		xmm2, [esi+4*4]
						shufps		xmm2, xmm2, R_SHUFFLEPS( 0, 0, 1, 1 )
						movaps		xmm7, [edi+8*4]
						mulps		xmm7, xmm2
						addps		xmm6, xmm7
						movhlps		xmm3, xmm6
						addps		xmm3, xmm6
						STORE2LO( 0, xmm3, xmm7 )
					}
					return;
				}
				case 3: {		// 6x3 * 6x1
					__asm {
						mov			esi, vPtr
						mov			edi, mPtr
						mov			eax, dstPtr
						movss		xmm0, [edi+(0*3+2)*4]
						movhps		xmm0, [edi+(0*3+0)*4]
						shufps		xmm0, xmm0, R_SHUFFLEPS( 2, 1, 3, 0 )
						movss		xmm6, [esi+0*4]
						shufps		xmm6, xmm6, R_SHUFFLEPS( 0, 0, 0, 0 )
						mulps		xmm6, xmm0
						movss		xmm1, [edi+(1*3+0)*4]
						movhps		xmm1, [edi+(1*3+1)*4]
						movss		xmm7, [esi+1*4]
						shufps		xmm7, xmm7, R_SHUFFLEPS( 0, 0, 0, 0 )
						mulps		xmm7, xmm1
						addps		xmm6, xmm7
						movss		xmm2, [edi+(2*3+2)*4]
						movhps		xmm2, [edi+(2*3+0)*4]
						shufps		xmm2, xmm2, R_SHUFFLEPS( 2, 1, 3, 0 )
						movss		xmm7, [esi+2*4]
						shufps		xmm7, xmm7, R_SHUFFLEPS( 0, 0, 0, 0 )
						mulps		xmm7, xmm2
						addps		xmm6, xmm7
						movss		xmm3, [edi+(3*3+0)*4]
						movhps		xmm3, [edi+(3*3+1)*4]
						movss		xmm7, [esi+3*4]
						shufps		xmm7, xmm7, R_SHUFFLEPS( 0, 0, 0, 0 )
						mulps		xmm7, xmm3
						addps		xmm6, xmm7
						movss		xmm4, [edi+(4*3+2)*4]
						movhps		xmm4, [edi+(4*3+0)*4]
						shufps		xmm4, xmm4, R_SHUFFLEPS( 2, 1, 3, 0 )
						movss		xmm7, [esi+4*4]
						shufps		xmm7, xmm7, R_SHUFFLEPS( 0, 0, 0, 0 )
						mulps		xmm7, xmm4
						addps		xmm6, xmm7
						movss		xmm5, [edi+(5*3+0)*4]
						movhps		xmm5, [edi+(5*3+1)*4]
						movss		xmm7, [esi+5*4]
						shufps		xmm7, xmm7, R_SHUFFLEPS( 0, 0, 0, 0 )
						mulps		xmm7, xmm5
						addps		xmm6, xmm7
						STORE1( 0, xmm6, xmm7 )
						STORE2HI( 4, xmm6, xmm7 )
					}
					return;
				}
				case 4: {		// 6x4 * 6x1
					__asm {
						mov			esi, vPtr
						mov			edi, mPtr
						mov			eax, dstPtr
						movlps		xmm3, [edi+(0*4+0)*4]
						movhps		xmm3, [edi+(0*4+2)*4]
						movss		xmm4, [esi+0*4]
						shufps		xmm4, xmm4, R_SHUFFLEPS( 0, 0, 0, 0 )
						mulps		xmm3, xmm4
						movlps		xmm5, [edi+(1*4+0)*4]
						movhps		xmm5, [edi+(1*4+2)*4]
						movss		xmm6, [esi+1*4]
						shufps		xmm6, xmm6, R_SHUFFLEPS( 0, 0, 0, 0 )
						mulps		xmm5, xmm6
						addps		xmm3, xmm5
						movlps		xmm4, [edi+(2*4+0)*4]
						movhps		xmm4, [edi+(2*4+2)*4]
						movss		xmm6, [esi+2*4]
						shufps		xmm6, xmm6, R_SHUFFLEPS( 0, 0, 0, 0 )
						mulps		xmm4, xmm6
						addps		xmm3, xmm4
						movlps		xmm5, [edi+(3*4+0)*4]
						movhps		xmm5, [edi+(3*4+2)*4]
						movss		xmm6, [esi+3*4]
						shufps		xmm6, xmm6, R_SHUFFLEPS( 0, 0, 0, 0 )
						mulps		xmm5, xmm6
						addps		xmm3, xmm5
						movlps		xmm4, [edi+(4*4+0)*4]
						movhps		xmm4, [edi+(4*4+2)*4]
						movss		xmm6, [esi+4*4]
						shufps		xmm6, xmm6, R_SHUFFLEPS( 0, 0, 0, 0 )
						mulps		xmm4, xmm6
						addps		xmm3, xmm4
						movlps		xmm5, [edi+(5*4+0)*4]
						movhps		xmm5, [edi+(5*4+2)*4]
						movss		xmm6, [esi+5*4]
						shufps		xmm6, xmm6, R_SHUFFLEPS( 0, 0, 0, 0 )
						mulps		xmm5, xmm6
						addps		xmm3, xmm5
						STORE4( 0, xmm3, xmm7 )
					}
					return;
				}
				case 5: {		// 6x5 * 6x1
					__asm {
						mov			esi, vPtr
						mov			edi, mPtr
						mov			eax, dstPtr
						movlps		xmm6, [edi+(0*5+0)*4]
						movhps		xmm6, [edi+(0*5+2)*4]
						movss		xmm0, [esi+0*4]
						shufps		xmm0, xmm0, R_SHUFFLEPS( 0, 0, 0, 0 )
						mulps		xmm6, xmm0
						movlps		xmm7, [edi+(1*5+0)*4]
						movhps		xmm7, [edi+(1*5+2)*4]
						movss		xmm1, [esi+1*4]
						shufps		xmm1, xmm1, R_SHUFFLEPS( 0, 0, 0, 0 )
						mulps		xmm7, xmm1
						addps		xmm6, xmm7
						movlps		xmm7, [edi+(2*5+0)*4]
						movhps		xmm7, [edi+(2*5+2)*4]
						movss		xmm2, [esi+2*4]
						shufps		xmm2, xmm2, R_SHUFFLEPS( 0, 0, 0, 0 )
						mulps		xmm7, xmm2
						addps		xmm6, xmm7
						movlps		xmm7, [edi+(3*5+0)*4]
						movhps		xmm7, [edi+(3*5+2)*4]
						movss		xmm3, [esi+3*4]
						shufps		xmm3, xmm3, R_SHUFFLEPS( 0, 0, 0, 0 )
						mulps		xmm7, xmm3
						addps		xmm6, xmm7
						movlps		xmm7, [edi+(4*5+0)*4]
						movhps		xmm7, [edi+(4*5+2)*4]
						movss		xmm4, [esi+4*4]
						shufps		xmm4, xmm4, R_SHUFFLEPS( 0, 0, 0, 0 )
						mulps		xmm7, xmm4
						addps		xmm6, xmm7
						movlps		xmm7, [edi+(5*5+0)*4]
						movhps		xmm7, [edi+(5*5+2)*4]
						movss		xmm5, [esi+5*4]
						shufps		xmm5, xmm5, R_SHUFFLEPS( 0, 0, 0, 0 )
						mulps		xmm7, xmm5
						addps		xmm6, xmm7
						STORE4( 0, xmm6, xmm7 )
						movss		xmm6, [edi+(0*5+4)*4]
						mulss		xmm6, xmm0
						movss		xmm7, [edi+(1*5+4)*4]
						mulss		xmm7, xmm1
						addss		xmm6, xmm7
						movss		xmm7, [edi+(2*5+4)*4]
						mulss		xmm7, xmm2
						addss		xmm6, xmm7
						movss		xmm7, [edi+(3*5+4)*4]
						mulss		xmm7, xmm3
						addss		xmm6, xmm7
						movss		xmm7, [edi+(4*5+4)*4]
						mulss		xmm7, xmm4
						addss		xmm6, xmm7
						movss		xmm7, [edi+(5*5+4)*4]
						mulss		xmm7, xmm5
						addss		xmm6, xmm7
						STORE1( 16, xmm6, xmm7 )
					}
					return;
				}
				case 6: {		// 6x6 * 6x1
					__asm {
						mov			esi, vPtr
						mov			edi, mPtr
						mov			eax, dstPtr
						movlps		xmm0, [esi+0*4]
						movlps		xmm1, [esi+2*4]
						movlps		xmm2, [esi+4*4]
						movaps		xmm3, xmm0
						shufps		xmm3, xmm3, R_SHUFFLEPS( 0, 0, 0, 0 )
						mulps		xmm3, [edi+(0*6+0)*4]
						movlps		xmm5, [edi+(1*6+0)*4]
						movhps		xmm5, [edi+(1*6+2)*4]
						movaps		xmm6, xmm0
						shufps		xmm6, xmm6, R_SHUFFLEPS( 1, 1, 1, 1 )
						mulps		xmm5, xmm6
						addps		xmm3, xmm5
						movaps		xmm6, xmm1
						shufps		xmm6, xmm6, R_SHUFFLEPS( 0, 0, 0, 0 )
						mulps		xmm6, [edi+(2*6+0)*4]
						addps		xmm3, xmm6
						movaps		xmm6, xmm1
						shufps		xmm6, xmm6, R_SHUFFLEPS( 1, 1, 1, 1 )
						movlps		xmm5, [edi+(3*6+0)*4]
						movhps		xmm5, [edi+(3*6+2)*4]
						mulps		xmm5, xmm6
						addps		xmm3, xmm5
						movaps		xmm6, xmm2
						shufps		xmm6, xmm6, R_SHUFFLEPS( 0, 0, 0, 0 )
						mulps		xmm6, [edi+(4*6+0)*4]
						addps		xmm3, xmm6
						movaps		xmm6, xmm2
						shufps		xmm6, xmm6, R_SHUFFLEPS( 1, 1, 1, 1 )
						movlps		xmm5, [edi+(5*6+0)*4]
						movhps		xmm5, [edi+(5*6+2)*4]
						mulps		xmm5, xmm6
						addps		xmm3, xmm5
						STORE4( 0, xmm3, xmm7 )
						shufps		xmm0, xmm0, R_SHUFFLEPS( 0, 0, 1, 1 )
						shufps		xmm1, xmm1, R_SHUFFLEPS( 0, 0, 1, 1 )
						shufps		xmm2, xmm2, R_SHUFFLEPS( 0, 0, 1, 1 )
						movlps		xmm3, [edi+(0*6+4)*4]
						movhps		xmm3, [edi+(1*6+4)*4]
						mulps		xmm3, xmm0
						movlps		xmm4, [edi+(2*6+4)*4]
						movhps		xmm4, [edi+(3*6+4)*4]
						mulps		xmm4, xmm1
						addps		xmm3, xmm4
						movlps		xmm5, [edi+(4*6+4)*4]
						movhps		xmm5, [edi+(5*6+4)*4]
						mulps		xmm5, xmm2
						addps		xmm3, xmm5
						movhlps		xmm4, xmm3
						addps		xmm3, xmm4
						STORE2LO( 16, xmm3, xmm7 )
					}
					return;
				}
				default: {
					for ( int i = 0; i < numColumns; i++ ) {
						dstPtr[i] STOREC *(mPtr) * vPtr[0] + *(mPtr+numColumns) * vPtr[1] + *(mPtr+2*numColumns) * vPtr[2] +
								*(mPtr+3*numColumns) * vPtr[3] + *(mPtr+4*numColumns) * vPtr[4] + *(mPtr+5*numColumns) * vPtr[5];
						mPtr++;
					}
					return;
				}
			}
			break;
		default:
			int numRows = mat.GetNumRows();
			for ( int i = 0; i < numColumns; i++ ) {
				mPtr = mat.ToFloatPtr() + i;
				float sum = mPtr[0] * vPtr[0];
				for ( int j = 1; j < numRows; j++ ) {
					mPtr += numColumns;
					sum += mPtr[0] * vPtr[j];
				}
				dstPtr[i] STOREC sum;
			}
			break;
	}

#undef STOREC
#undef STORE4
#undef STORE2HI
#undef STORE2LO
#undef STORE1
}

/*
============
void idSIMD_SSE::MatX_TransposeMultiplySubVecX

	optimizes the following matrix multiplications:

	Nx6 * Nx1
	6xN * 6x1

	with N in the range [1-6]
============
*/
void VPCALL idSIMD_SSE::MatX_TransposeMultiplySubVecX( idVecX &dst, const idMatX &mat, const idVecX &vec ) {
#define STORE1( offset, reg1, reg2 )		\
	__asm movss		reg2, [eax+offset]		\
	__asm subss		reg2, reg1				\
	__asm movss		[eax+offset], reg2
#define STORE2LO( offset, reg1, reg2 )		\
	__asm movlps	reg2, [eax+offset]		\
	__asm subps		reg2, reg1				\
	__asm movlps	[eax+offset], reg2
#define STORE2HI( offset, reg1, reg2 )		\
	__asm movhps	reg2, [eax+offset]		\
	__asm subps		reg2, reg1				\
	__asm movhps	[eax+offset], reg2
#define STORE4( offset, reg1, reg2 )		\
	__asm movlps	reg2, [eax+offset]		\
	__asm movhps	reg2, [eax+offset+8]	\
	__asm subps		reg2, reg1				\
	__asm movlps	[eax+offset], reg2		\
	__asm movhps	[eax+offset+8], reg2
#define STOREC		-=

	int numColumns;
	const float *mPtr, *vPtr;
	float *dstPtr;

	assert( vec.GetSize() >= mat.GetNumRows() );
	assert( dst.GetSize() >= mat.GetNumColumns() );

	mPtr = mat.ToFloatPtr();
	vPtr = vec.ToFloatPtr();
	dstPtr = dst.ToFloatPtr();
	numColumns = mat.GetNumColumns();
	switch( mat.GetNumRows() ) {
		case 1:
			switch( numColumns ) {
				case 6: {		// 1x6 * 1x1
					__asm {
						mov			esi, vPtr
						mov			edi, mPtr
						mov			eax, dstPtr
						movss		xmm0, [esi]
						shufps		xmm0, xmm0, R_SHUFFLEPS( 0, 0, 0, 0 )
						movaps		xmm1, xmm0
						mulps		xmm0, [edi]
						mulps		xmm1, [edi+16]
						STORE4( 0, xmm0, xmm2 )
						STORE2LO( 16, xmm1, xmm3 )
					}
					return;
				}
				default: {
					for ( int i = 0; i < numColumns; i++ ) {
						dstPtr[i] STOREC *(mPtr) * vPtr[0];
						mPtr++;
					}
					return;
				}
			}
			break;
		case 2:
			switch( numColumns ) {
				case 6: {		// 2x6 * 2x1
					__asm {
						mov			esi, vPtr
						mov			edi, mPtr
						mov			eax, dstPtr
						movlps		xmm0, [esi]
						movaps		xmm1, xmm0
						shufps		xmm0, xmm0, R_SHUFFLEPS( 0, 0, 0, 0 )
						shufps		xmm1, xmm1, R_SHUFFLEPS( 1, 1, 1, 1 )
						movaps		xmm2, [edi]
						mulps		xmm2, xmm0
						movlps		xmm3, [edi+24]
						movhps		xmm3, [edi+32]
						mulps		xmm3, xmm1
						addps		xmm2, xmm3
						shufps		xmm0, xmm1, R_SHUFFLEPS( 0, 0, 0, 0 )
						movlps		xmm4, [edi+16]
						movhps		xmm4, [edi+40]
						mulps		xmm4, xmm0
						movhlps		xmm3, xmm4
						addps		xmm3, xmm4
						STORE4( 0, xmm2, xmm5 )
						STORE2LO( 16, xmm3, xmm6 )
					}
					return;
				}
				default: {
					for ( int i = 0; i < numColumns; i++ ) {
						dstPtr[i] STOREC *(mPtr) * vPtr[0] + *(mPtr+numColumns) * vPtr[1];
						mPtr++;
					}
					return;
				}
			}
			break;
		case 3:
			switch( numColumns ) {
				case 6: {		// 3x6 * 3x1
					__asm {
						mov			esi, vPtr
						mov			edi, mPtr
						mov			eax, dstPtr
						movlps		xmm0, [esi+0*4]
						movss		xmm1, [esi+2*4]
						movlps		xmm3, [edi+(0*6+0)*4]
						movhps		xmm3, [edi+(0*6+2)*4]
						movaps		xmm4, xmm0
						shufps		xmm4, xmm4, R_SHUFFLEPS( 0, 0, 0, 0 )
						mulps		xmm3, xmm4
						movlps		xmm5, [edi+(1*6+0)*4]
						movhps		xmm5, [edi+(1*6+2)*4]
						movaps		xmm6, xmm0
						shufps		xmm6, xmm6, R_SHUFFLEPS( 1, 1, 1, 1 )
						mulps		xmm5, xmm6
						addps		xmm3, xmm5
						movlps		xmm4, [edi+(2*6+0)*4]
						movhps		xmm4, [edi+(2*6+2)*4]
						shufps		xmm1, xmm1, R_SHUFFLEPS( 0, 0, 0, 0 )
						mulps		xmm4, xmm1
						addps		xmm3, xmm4
						STORE4( 0, xmm3, xmm7 )
						shufps		xmm0, xmm0, R_SHUFFLEPS( 0, 0, 1, 1 )
						movlps		xmm3, [edi+(0*6+4)*4]
						movhps		xmm3, [edi+(1*6+4)*4]
						mulps		xmm3, xmm0
						movhlps		xmm4, xmm3
						addps		xmm3, xmm4
						movlps		xmm5, [edi+(2*6+4)*4]
						mulps		xmm5, xmm1
						addps		xmm3, xmm5
						STORE2LO( 16, xmm3, xmm7 )
					}
					return;
				}
				default: {
					for ( int i = 0; i < numColumns; i++ ) {
						dstPtr[i] STOREC *(mPtr) * vPtr[0] + *(mPtr+numColumns) * vPtr[1] + *(mPtr+2*numColumns) * vPtr[2];
						mPtr++;
					}
					return;
				}
			}
			break;
		case 4:
			switch( numColumns ) {
				case 6: {		// 4x6 * 4x1
					__asm {
						mov			esi, vPtr
						mov			edi, mPtr
						mov			eax, dstPtr
						movlps		xmm0, [esi+0*4]
						movlps		xmm1, [esi+2*4]
						movaps		xmm3, xmm0
						shufps		xmm3, xmm3, R_SHUFFLEPS( 0, 0, 0, 0 )
						mulps		xmm3, [edi+(0*6+0)*4]
						movlps		xmm5, [edi+(1*6+0)*4]
						movhps		xmm5, [edi+(1*6+2)*4]
						movaps		xmm6, xmm0
						shufps		xmm6, xmm6, R_SHUFFLEPS( 1, 1, 1, 1 )
						mulps		xmm5, xmm6
						addps		xmm3, xmm5
						movlps		xmm4, [edi+(2*6+0)*4]
						movhps		xmm4, [edi+(2*6+2)*4]
						movaps		xmm6, xmm1
						shufps		xmm6, xmm6, R_SHUFFLEPS( 0, 0, 0, 0 )
						mulps		xmm4, xmm6
						addps		xmm3, xmm4
						movlps		xmm5, [edi+(3*6+0)*4]
						movhps		xmm5, [edi+(3*6+2)*4]
						movaps		xmm6, xmm1
						shufps		xmm6, xmm6, R_SHUFFLEPS( 1, 1, 1, 1 )
						mulps		xmm5, xmm6
						addps		xmm3, xmm5
						STORE4( 0, xmm3, xmm7 )
						shufps		xmm0, xmm0, R_SHUFFLEPS( 0, 0, 1, 1 )
						shufps		xmm1, xmm1, R_SHUFFLEPS( 0, 0, 1, 1 )
						movlps		xmm3, [edi+(0*6+4)*4]
						movhps		xmm3, [edi+(1*6+4)*4]
						mulps		xmm3, xmm0
						movlps		xmm4, [edi+(2*6+4)*4]
						movhps		xmm4, [edi+(3*6+4)*4]
						mulps		xmm4, xmm1
						addps		xmm3, xmm4
						movhlps		xmm4, xmm3
						addps		xmm3, xmm4
						STORE2LO( 16, xmm3, xmm7 )
					}
					return;
				}
				default: {
					for ( int i = 0; i < numColumns; i++ ) {
						dstPtr[i] STOREC *(mPtr) * vPtr[0] + *(mPtr+numColumns) * vPtr[1] + *(mPtr+2*numColumns) * vPtr[2] +
								*(mPtr+3*numColumns) * vPtr[3];
						mPtr++;
					}
					return;
				}
			}
			break;
		case 5:
			switch( numColumns ) {
				case 6: {		// 5x6 * 5x1
					__asm {
						mov			esi, vPtr
						mov			edi, mPtr
						mov			eax, dstPtr
						movlps		xmm0, [esi+0*4]
						movlps		xmm1, [esi+2*4]
						movss		xmm2, [esi+4*4]
						movaps		xmm3, xmm0
						shufps		xmm3, xmm3, R_SHUFFLEPS( 0, 0, 0, 0 )
						mulps		xmm3, [edi+(0*6+0)*4]
						movlps		xmm5, [edi+(1*6+0)*4]
						movhps		xmm5, [edi+(1*6+2)*4]
						movaps		xmm6, xmm0
						shufps		xmm6, xmm6, R_SHUFFLEPS( 1, 1, 1, 1 )
						mulps		xmm5, xmm6
						addps		xmm3, xmm5
						movaps		xmm6, xmm1
						shufps		xmm6, xmm6, R_SHUFFLEPS( 0, 0, 0, 0 )
						mulps		xmm6, [edi+(2*6+0)*4]
						addps		xmm3, xmm6
						movlps		xmm5, [edi+(3*6+0)*4]
						movhps		xmm5, [edi+(3*6+2)*4]
						movaps		xmm6, xmm1
						shufps		xmm6, xmm6, R_SHUFFLEPS( 1, 1, 1, 1 )
						mulps		xmm5, xmm6
						addps		xmm3, xmm5
						shufps		xmm2, xmm2, R_SHUFFLEPS( 0, 0, 0, 0 )
						movaps		xmm4, xmm2
						mulps		xmm4, [edi+(4*6+0)*4]
						addps		xmm3, xmm4
						STORE4( 0, xmm3, xmm7 )
						shufps		xmm0, xmm0, R_SHUFFLEPS( 0, 0, 1, 1 )
						shufps		xmm1, xmm1, R_SHUFFLEPS( 0, 0, 1, 1 )
						movlps		xmm3, [edi+(0*6+4)*4]
						movhps		xmm3, [edi+(1*6+4)*4]
						mulps		xmm3, xmm0
						movlps		xmm4, [edi+(2*6+4)*4]
						movhps		xmm4, [edi+(3*6+4)*4]
						mulps		xmm4, xmm1
						addps		xmm3, xmm4
						movhlps		xmm4, xmm3
						addps		xmm3, xmm4
						movlps		xmm5, [edi+(4*6+4)*4]
						mulps		xmm5, xmm2
						addps		xmm3, xmm5
						STORE2LO( 16, xmm3, xmm7 )
					}
					return;
				}
				default: {
					for ( int i = 0; i < numColumns; i++ ) {
						dstPtr[i] STOREC *(mPtr) * vPtr[0] + *(mPtr+numColumns) * vPtr[1] + *(mPtr+2*numColumns) * vPtr[2] +
								*(mPtr+3*numColumns) * vPtr[3] + *(mPtr+4*numColumns) * vPtr[4];
						mPtr++;
					}
					return;
				}
			}
			break;
		case 6:
			switch( numColumns ) {
				case 1: {		// 6x1 * 6x1
					__asm {
						mov			esi, vPtr
						mov			edi, mPtr
						mov			eax, dstPtr
						movlps		xmm0, [esi]
						movhps		xmm0, [esi+8]
						movlps		xmm1, [esi+16]
						mulps		xmm0, [edi]
						mulps		xmm1, [edi+16]
						shufps		xmm1, xmm0, R_SHUFFLEPS( 0, 1, 3, 2 )
						addps		xmm0, xmm1
						movhlps		xmm2, xmm0
						addss		xmm2, xmm0
						shufps		xmm0, xmm0, R_SHUFFLEPS( 1, 0, 0, 0 )
						addss		xmm2, xmm0
						STORE1( 0, xmm2, xmm3 )
					}
					return;
				}
				case 2: {		// 6x2 * 6x1
					__asm {
						mov			esi, vPtr
						mov			edi, mPtr
						mov			eax, dstPtr
						movlps		xmm0, [esi+0*4]
						shufps		xmm0, xmm0, R_SHUFFLEPS( 0, 0, 1, 1 )
						movaps		xmm6, [edi+0*4]
						mulps		xmm6, xmm0
						movlps		xmm1, [esi+2*4]
						shufps		xmm1, xmm1, R_SHUFFLEPS( 0, 0, 1, 1 )
						movaps		xmm7, [edi+4*4]
						mulps		xmm7, xmm1
						addps		xmm6, xmm7
						movlps		xmm2, [esi+4*4]
						shufps		xmm2, xmm2, R_SHUFFLEPS( 0, 0, 1, 1 )
						movaps		xmm7, [edi+8*4]
						mulps		xmm7, xmm2
						addps		xmm6, xmm7
						movhlps		xmm3, xmm6
						addps		xmm3, xmm6
						STORE2LO( 0, xmm3, xmm7 )
					}
					return;
				}
				case 3: {		// 6x3 * 6x1
					__asm {
						mov			esi, vPtr
						mov			edi, mPtr
						mov			eax, dstPtr
						movss		xmm0, [edi+(0*3+2)*4]
						movhps		xmm0, [edi+(0*3+0)*4]
						shufps		xmm0, xmm0, R_SHUFFLEPS( 2, 1, 3, 0 )
						movss		xmm6, [esi+0*4]
						shufps		xmm6, xmm6, R_SHUFFLEPS( 0, 0, 0, 0 )
						mulps		xmm6, xmm0
						movss		xmm1, [edi+(1*3+0)*4]
						movhps		xmm1, [edi+(1*3+1)*4]
						movss		xmm7, [esi+1*4]
						shufps		xmm7, xmm7, R_SHUFFLEPS( 0, 0, 0, 0 )
						mulps		xmm7, xmm1
						addps		xmm6, xmm7
						movss		xmm2, [edi+(2*3+2)*4]
						movhps		xmm2, [edi+(2*3+0)*4]
						shufps		xmm2, xmm2, R_SHUFFLEPS( 2, 1, 3, 0 )
						movss		xmm7, [esi+2*4]
						shufps		xmm7, xmm7, R_SHUFFLEPS( 0, 0, 0, 0 )
						mulps		xmm7, xmm2
						addps		xmm6, xmm7
						movss		xmm3, [edi+(3*3+0)*4]
						movhps		xmm3, [edi+(3*3+1)*4]
						movss		xmm7, [esi+3*4]
						shufps		xmm7, xmm7, R_SHUFFLEPS( 0, 0, 0, 0 )
						mulps		xmm7, xmm3
						addps		xmm6, xmm7
						movss		xmm4, [edi+(4*3+2)*4]
						movhps		xmm4, [edi+(4*3+0)*4]
						shufps		xmm4, xmm4, R_SHUFFLEPS( 2, 1, 3, 0 )
						movss		xmm7, [esi+4*4]
						shufps		xmm7, xmm7, R_SHUFFLEPS( 0, 0, 0, 0 )
						mulps		xmm7, xmm4
						addps		xmm6, xmm7
						movss		xmm5, [edi+(5*3+0)*4]
						movhps		xmm5, [edi+(5*3+1)*4]
						movss		xmm7, [esi+5*4]
						shufps		xmm7, xmm7, R_SHUFFLEPS( 0, 0, 0, 0 )
						mulps		xmm7, xmm5
						addps		xmm6, xmm7
						STORE1( 0, xmm6, xmm7 )
						STORE2HI( 4, xmm6, xmm7 )
					}
					return;
				}
				case 4: {		// 6x4 * 6x1
					__asm {
						mov			esi, vPtr
						mov			edi, mPtr
						mov			eax, dstPtr
						movlps		xmm3, [edi+(0*4+0)*4]
						movhps		xmm3, [edi+(0*4+2)*4]
						movss		xmm4, [esi+0*4]
						shufps		xmm4, xmm4, R_SHUFFLEPS( 0, 0, 0, 0 )
						mulps		xmm3, xmm4
						movlps		xmm5, [edi+(1*4+0)*4]
						movhps		xmm5, [edi+(1*4+2)*4]
						movss		xmm6, [esi+1*4]
						shufps		xmm6, xmm6, R_SHUFFLEPS( 0, 0, 0, 0 )
						mulps		xmm5, xmm6
						addps		xmm3, xmm5
						movlps		xmm4, [edi+(2*4+0)*4]
						movhps		xmm4, [edi+(2*4+2)*4]
						movss		xmm6, [esi+2*4]
						shufps		xmm6, xmm6, R_SHUFFLEPS( 0, 0, 0, 0 )
						mulps		xmm4, xmm6
						addps		xmm3, xmm4
						movlps		xmm5, [edi+(3*4+0)*4]
						movhps		xmm5, [edi+(3*4+2)*4]
						movss		xmm6, [esi+3*4]
						shufps		xmm6, xmm6, R_SHUFFLEPS( 0, 0, 0, 0 )
						mulps		xmm5, xmm6
						addps		xmm3, xmm5
						movlps		xmm4, [edi+(4*4+0)*4]
						movhps		xmm4, [edi+(4*4+2)*4]
						movss		xmm6, [esi+4*4]
						shufps		xmm6, xmm6, R_SHUFFLEPS( 0, 0, 0, 0 )
						mulps		xmm4, xmm6
						addps		xmm3, xmm4
						movlps		xmm5, [edi+(5*4+0)*4]
						movhps		xmm5, [edi+(5*4+2)*4]
						movss		xmm6, [esi+5*4]
						shufps		xmm6, xmm6, R_SHUFFLEPS( 0, 0, 0, 0 )
						mulps		xmm5, xmm6
						addps		xmm3, xmm5
						STORE4( 0, xmm3, xmm7 )
					}
					return;
				}
				case 5: {		// 6x5 * 6x1
					__asm {
						mov			esi, vPtr
						mov			edi, mPtr
						mov			eax, dstPtr
						movlps		xmm6, [edi+(0*5+0)*4]
						movhps		xmm6, [edi+(0*5+2)*4]
						movss		xmm0, [esi+0*4]
						shufps		xmm0, xmm0, R_SHUFFLEPS( 0, 0, 0, 0 )
						mulps		xmm6, xmm0
						movlps		xmm7, [edi+(1*5+0)*4]
						movhps		xmm7, [edi+(1*5+2)*4]
						movss		xmm1, [esi+1*4]
						shufps		xmm1, xmm1, R_SHUFFLEPS( 0, 0, 0, 0 )
						mulps		xmm7, xmm1
						addps		xmm6, xmm7
						movlps		xmm7, [edi+(2*5+0)*4]
						movhps		xmm7, [edi+(2*5+2)*4]
						movss		xmm2, [esi+2*4]
						shufps		xmm2, xmm2, R_SHUFFLEPS( 0, 0, 0, 0 )
						mulps		xmm7, xmm2
						addps		xmm6, xmm7
						movlps		xmm7, [edi+(3*5+0)*4]
						movhps		xmm7, [edi+(3*5+2)*4]
						movss		xmm3, [esi+3*4]
						shufps		xmm3, xmm3, R_SHUFFLEPS( 0, 0, 0, 0 )
						mulps		xmm7, xmm3
						addps		xmm6, xmm7
						movlps		xmm7, [edi+(4*5+0)*4]
						movhps		xmm7, [edi+(4*5+2)*4]
						movss		xmm4, [esi+4*4]
						shufps		xmm4, xmm4, R_SHUFFLEPS( 0, 0, 0, 0 )
						mulps		xmm7, xmm4
						addps		xmm6, xmm7
						movlps		xmm7, [edi+(5*5+0)*4]
						movhps		xmm7, [edi+(5*5+2)*4]
						movss		xmm5, [esi+5*4]
						shufps		xmm5, xmm5, R_SHUFFLEPS( 0, 0, 0, 0 )
						mulps		xmm7, xmm5
						addps		xmm6, xmm7
						STORE4( 0, xmm6, xmm7 )
						movss		xmm6, [edi+(0*5+4)*4]
						mulss		xmm6, xmm0
						movss		xmm7, [edi+(1*5+4)*4]
						mulss		xmm7, xmm1
						addss		xmm6, xmm7
						movss		xmm7, [edi+(2*5+4)*4]
						mulss		xmm7, xmm2
						addss		xmm6, xmm7
						movss		xmm7, [edi+(3*5+4)*4]
						mulss		xmm7, xmm3
						addss		xmm6, xmm7
						movss		xmm7, [edi+(4*5+4)*4]
						mulss		xmm7, xmm4
						addss		xmm6, xmm7
						movss		xmm7, [edi+(5*5+4)*4]
						mulss		xmm7, xmm5
						addss		xmm6, xmm7
						STORE1( 16, xmm6, xmm7 )
					}
					return;
				}
				case 6: {		// 6x6 * 6x1
					__asm {
						mov			esi, vPtr
						mov			edi, mPtr
						mov			eax, dstPtr
						movlps		xmm0, [esi+0*4]
						movlps		xmm1, [esi+2*4]
						movlps		xmm2, [esi+4*4]
						movaps		xmm3, xmm0
						shufps		xmm3, xmm3, R_SHUFFLEPS( 0, 0, 0, 0 )
						mulps		xmm3, [edi+(0*6+0)*4]
						movlps		xmm5, [edi+(1*6+0)*4]
						movhps		xmm5, [edi+(1*6+2)*4]
						movaps		xmm6, xmm0
						shufps		xmm6, xmm6, R_SHUFFLEPS( 1, 1, 1, 1 )
						mulps		xmm5, xmm6
						addps		xmm3, xmm5
						movaps		xmm6, xmm1
						shufps		xmm6, xmm6, R_SHUFFLEPS( 0, 0, 0, 0 )
						mulps		xmm6, [edi+(2*6+0)*4]
						addps		xmm3, xmm6
						movaps		xmm6, xmm1
						shufps		xmm6, xmm6, R_SHUFFLEPS( 1, 1, 1, 1 )
						movlps		xmm5, [edi+(3*6+0)*4]
						movhps		xmm5, [edi+(3*6+2)*4]
						mulps		xmm5, xmm6
						addps		xmm3, xmm5
						movaps		xmm6, xmm2
						shufps		xmm6, xmm6, R_SHUFFLEPS( 0, 0, 0, 0 )
						mulps		xmm6, [edi+(4*6+0)*4]
						addps		xmm3, xmm6
						movaps		xmm6, xmm2
						shufps		xmm6, xmm6, R_SHUFFLEPS( 1, 1, 1, 1 )
						movlps		xmm5, [edi+(5*6+0)*4]
						movhps		xmm5, [edi+(5*6+2)*4]
						mulps		xmm5, xmm6
						addps		xmm3, xmm5
						STORE4( 0, xmm3, xmm7 )
						shufps		xmm0, xmm0, R_SHUFFLEPS( 0, 0, 1, 1 )
						shufps		xmm1, xmm1, R_SHUFFLEPS( 0, 0, 1, 1 )
						shufps		xmm2, xmm2, R_SHUFFLEPS( 0, 0, 1, 1 )
						movlps		xmm3, [edi+(0*6+4)*4]
						movhps		xmm3, [edi+(1*6+4)*4]
						mulps		xmm3, xmm0
						movlps		xmm4, [edi+(2*6+4)*4]
						movhps		xmm4, [edi+(3*6+4)*4]
						mulps		xmm4, xmm1
						addps		xmm3, xmm4
						movlps		xmm5, [edi+(4*6+4)*4]
						movhps		xmm5, [edi+(5*6+4)*4]
						mulps		xmm5, xmm2
						addps		xmm3, xmm5
						movhlps		xmm4, xmm3
						addps		xmm3, xmm4
						STORE2LO( 16, xmm3, xmm7 )
					}
					return;
				}
				default: {
					for ( int i = 0; i < numColumns; i++ ) {
						dstPtr[i] STOREC *(mPtr) * vPtr[0] + *(mPtr+numColumns) * vPtr[1] + *(mPtr+2*numColumns) * vPtr[2] +
								*(mPtr+3*numColumns) * vPtr[3] + *(mPtr+4*numColumns) * vPtr[4] + *(mPtr+5*numColumns) * vPtr[5];
						mPtr++;
					}
					return;
				}
			}
			break;
		default:
			int numRows = mat.GetNumRows();
			for ( int i = 0; i < numColumns; i++ ) {
				mPtr = mat.ToFloatPtr() + i;
				float sum = mPtr[0] * vPtr[0];
				for ( int j = 1; j < numRows; j++ ) {
					mPtr += numColumns;
					sum += mPtr[0] * vPtr[j];
				}
				dstPtr[i] STOREC sum;
			}
			break;
	}

#undef STOREC
#undef STORE4
#undef STORE2HI
#undef STORE2LO
#undef STORE1
}

/*
============
idSIMD_SSE::MatX_MultiplyMatX

	optimizes the following matrix multiplications:

	NxN * Nx6
	6xN * Nx6
	Nx6 * 6xN
	6x6 * 6xN

	with N in the range [1-6].

	The hot cache clock cycle counts are generally better for the SIMD version than the
	FPU version. At times up to 40% less clock cycles on a P3. In practise however,
	the results are poor probably due to memory access.
============
*/
void VPCALL idSIMD_SSE::MatX_MultiplyMatX( idMatX &dst, const idMatX &m1, const idMatX &m2 ) {
	int i, j, k, l, n;
	float *dstPtr;
	const float *m1Ptr, *m2Ptr;
	double sum;

	assert( m1.GetNumColumns() == m2.GetNumRows() );

	dstPtr = dst.ToFloatPtr();
	m1Ptr = m1.ToFloatPtr();
	m2Ptr = m2.ToFloatPtr();
	k = m1.GetNumRows();
	l = m2.GetNumColumns();
	n = m1.GetNumColumns();

	switch( n ) {
		case 1: {
			if ( !(l^6) ) {
				switch( k ) {
					case 1:	{			// 1x1 * 1x6, no precision loss compared to FPU version
						__asm {
							mov			esi, m2Ptr
							mov			edi, m1Ptr
							mov			eax, dstPtr
							movss		xmm0, [edi]
							shufps		xmm0, xmm0, R_SHUFFLEPS( 0, 0, 0, 0 )
							movaps		xmm1, [esi]
							mulps		xmm1, xmm0
							movaps		[eax], xmm1
							movlps		xmm2, [esi+16]
							mulps		xmm2, xmm0
							movlps		[eax+16], xmm2
						}
						return;
					}
					case 6: {			// 6x1 * 1x6, no precision loss compared to FPU version
						__asm {
							mov			esi, m2Ptr
							mov			edi, m1Ptr
							mov			eax, dstPtr
							xorps		xmm1, xmm1
							movaps		xmm0, [edi]
							movlps		xmm1, [edi+16]
							movlhps		xmm1, xmm0
							movhlps		xmm2, xmm0
							movlhps		xmm2, xmm1
							// row 0 and 1
							movaps		xmm3, [esi]
							movaps		xmm4, xmm3
							shufps		xmm4, xmm4, R_SHUFFLEPS( 0, 0, 0, 0 )
							movaps		xmm5, xmm3
							shufps		xmm5, xmm5, R_SHUFFLEPS( 0, 0, 1, 1 )
							movaps		xmm6, xmm3
							shufps		xmm6, xmm6, R_SHUFFLEPS( 1, 1, 1, 1 )
							mulps		xmm4, xmm0
							mulps		xmm5, xmm1
							mulps		xmm6, xmm2
							movaps		[eax], xmm4
							movaps		[eax+16], xmm5
							movaps		[eax+32], xmm6
							// row 2 and 3
							movaps		xmm4, xmm3
							shufps		xmm4, xmm4, R_SHUFFLEPS( 2, 2, 2, 2 )
							movaps		xmm5, xmm3
							shufps		xmm5, xmm5, R_SHUFFLEPS( 2, 2, 3, 3 )
							shufps		xmm3, xmm3, R_SHUFFLEPS( 3, 3, 3, 3 )
							mulps		xmm4, xmm0
							mulps		xmm5, xmm1
							mulps		xmm3, xmm2
							movaps		[eax+48], xmm4
							movaps		[eax+64], xmm5
							movaps		[eax+80], xmm3
							// row 4 and 5
							movlps		xmm3, [esi+16]
							movaps		xmm4, xmm3
							shufps		xmm4, xmm4, R_SHUFFLEPS( 0, 0, 0, 0 )
							movaps		xmm5, xmm3
							shufps		xmm5, xmm5, R_SHUFFLEPS( 0, 0, 1, 1 )
							shufps		xmm3, xmm3, R_SHUFFLEPS( 1, 1, 1, 1 )
							mulps		xmm4, xmm0
							mulps		xmm5, xmm1
							mulps		xmm3, xmm2
							movaps		[eax+96], xmm4
							movaps		[eax+112], xmm5
							movaps		[eax+128], xmm3
						}
						return;
					}
				}
			}
			for ( i = 0; i < k; i++ ) {
				m2Ptr = m2.ToFloatPtr();
				for ( j = 0; j < l; j++ ) {
					*dstPtr++ = m1Ptr[0] * m2Ptr[0];
					m2Ptr++;
				}
				m1Ptr++;
			}
			break;
		}
		case 2: {
			if ( !(l^6) ) {
				switch( k ) {
					case 2: {			// 2x2 * 2x6

						#define MUL_Nx2_2x6_INIT								\
						__asm mov		esi, m2Ptr								\
						__asm mov		edi, m1Ptr								\
						__asm mov		eax, dstPtr								\
						__asm movaps	xmm0, [esi]								\
						__asm movlps	xmm1, [esi+16]							\
						__asm movhps	xmm1, [esi+40]							\
						__asm movlps	xmm2, [esi+24]							\
						__asm movhps	xmm2, [esi+32]

						#define MUL_Nx2_2x6_ROW2( row )							\
						__asm movaps	xmm3, [edi+row*16]						\
						__asm movaps	xmm5, xmm0								\
						__asm movaps	xmm4, xmm3								\
						__asm shufps	xmm4, xmm4, R_SHUFFLEPS( 0, 0, 0, 0 )	\
						__asm mulps		xmm5, xmm4								\
						__asm movaps	xmm4, xmm3								\
						__asm movaps	xmm6, xmm2								\
						__asm shufps	xmm4, xmm4, R_SHUFFLEPS( 1, 1, 1, 1 )	\
						__asm mulps		xmm6, xmm4								\
						__asm addps		xmm5, xmm6								\
						__asm movaps	[eax+row*48], xmm5						\
						__asm movaps	xmm4, xmm3								\
						__asm shufps	xmm4, xmm4, R_SHUFFLEPS( 0, 0, 1, 1 )	\
						__asm movaps	xmm7, xmm1								\
						__asm mulps		xmm7, xmm4								\
						__asm movaps	xmm4, xmm3								\
						__asm movaps	xmm5, xmm0								\
						__asm shufps	xmm4, xmm4, R_SHUFFLEPS( 2, 2, 2, 2 )	\
						__asm mulps		xmm5, xmm4								\
						__asm movaps	xmm4, xmm3								\
						__asm movaps	xmm6, xmm2								\
						__asm shufps	xmm4, xmm4, R_SHUFFLEPS( 3, 3, 3, 3 )	\
						__asm mulps		xmm6, xmm4								\
						__asm addps		xmm5, xmm6								\
						__asm shufps	xmm3, xmm3, R_SHUFFLEPS( 2, 2, 3, 3 )	\
						__asm movaps	xmm6, xmm1								\
						__asm mulps		xmm6, xmm3								\
						__asm movaps	xmm4, xmm7								\
						__asm movlhps	xmm7, xmm6								\
						__asm movhlps	xmm6, xmm4								\
						__asm addps		xmm6, xmm7								\
						__asm movlps	[eax+row*48+16], xmm6					\
						__asm movlps	[eax+row*48+24], xmm5					\
						__asm movhps	[eax+row*48+32], xmm5					\
						__asm movhps	[eax+row*48+40], xmm6

						MUL_Nx2_2x6_INIT
						MUL_Nx2_2x6_ROW2( 0 )

						return;
					}
					case 6: {			// 6x2 * 2x6

						MUL_Nx2_2x6_INIT
						MUL_Nx2_2x6_ROW2( 0 )
						MUL_Nx2_2x6_ROW2( 1 )
						MUL_Nx2_2x6_ROW2( 2 )

						return;
					}
				}
			}
			for ( i = 0; i < k; i++ ) {
				m2Ptr = m2.ToFloatPtr();
				for ( j = 0; j < l; j++ ) {
					*dstPtr++ = m1Ptr[0] * m2Ptr[0] + m1Ptr[1] * m2Ptr[l];
					m2Ptr++;
				}
				m1Ptr += 2;
			}
			break;
		}
		case 3: {
			if ( !(l^6) ) {
				switch( k ) {
					case 3: {			// 3x3 * 3x6
						__asm {
							mov		esi, m2Ptr
							mov		edi, m1Ptr
							mov		eax, dstPtr
							movaps	xmm5, xmmword ptr [esi]
							movlps	xmm6, qword ptr [esi+24]
							movhps	xmm6, qword ptr [esi+32]
							movaps	xmm7, xmmword ptr [esi+48]
							movss	xmm0, dword ptr [edi]
							shufps	xmm0, xmm0, 0
							mulps	xmm0, xmm5
							movss	xmm1, dword ptr [edi+4]
							shufps	xmm1, xmm1, 0
							mulps	xmm1, xmm6
							movss	xmm2, dword ptr [edi+8]
							shufps	xmm2, xmm2, 0
							mulps	xmm2, xmm7
							addps	xmm0, xmm1
							addps	xmm0, xmm2
							movaps	xmmword ptr [eax], xmm0
							movss	xmm3, dword ptr [edi+12]
							shufps	xmm3, xmm3, 0
							mulps	xmm3, xmm5
							movss	xmm4, dword ptr [edi+16]
							shufps	xmm4, xmm4, 0
							mulps	xmm4, xmm6
							movss	xmm0, dword ptr [edi+20]
							shufps	xmm0, xmm0, 0
							mulps	xmm0, xmm7
							addps	xmm3, xmm4
							addps	xmm0, xmm3
							movlps	qword ptr [eax+24], xmm0
							movhps	qword ptr [eax+32], xmm0
							movss	xmm1, dword ptr [edi+24]
							shufps	xmm1, xmm1, 0
							mulps	xmm1, xmm5
							movss	xmm2, dword ptr [edi+28]
							shufps	xmm2, xmm2, 0
							mulps	xmm2, xmm6
							movss	xmm3, dword ptr [edi+32]
							shufps	xmm3, xmm3, 0
							mulps	xmm3, xmm7
							addps	xmm1, xmm2
							addps	xmm1, xmm3
							movaps	xmmword ptr [eax+48], xmm1
							movlps	xmm5, qword ptr [esi+16]
							movlps	xmm6, qword ptr [esi+40]
							movlps	xmm7, qword ptr [esi+64]
							shufps	xmm5, xmm5, 0x44
							shufps	xmm6, xmm6, 0x44
							shufps	xmm7, xmm7, 0x44
							movaps	xmm3, xmmword ptr [edi]
							movlps	xmm4, qword ptr [edi+16]
							movaps	xmm0, xmm3
							shufps	xmm0, xmm0, 0xF0
							mulps	xmm0, xmm5
							movaps	xmm1, xmm3
							shufps	xmm1, xmm4, 0x05
							mulps	xmm1, xmm6
							shufps	xmm3, xmm4, 0x5A
							mulps	xmm3, xmm7
							addps	xmm1, xmm0
							addps	xmm1, xmm3
							movlps	qword ptr [eax+16], xmm1
							movhps	qword ptr [eax+40], xmm1
							movss	xmm0, dword ptr [edi+24]
							shufps	xmm0, xmm0, 0
							mulps	xmm0, xmm5
							movss	xmm2, dword ptr [edi+28]
							shufps	xmm2, xmm2, 0
							mulps	xmm2, xmm6
							movss	xmm4, dword ptr [edi+32]
							shufps	xmm4, xmm4, 0
							mulps	xmm4, xmm7
							addps	xmm0, xmm2
							addps	xmm0, xmm4
							movlps	qword ptr [eax+64], xmm0
						}
						return;
					}
					case 6: {			// 6x3 * 3x6
						#define MUL_Nx3_3x6_FIRST4COLUMNS_INIT						\
						__asm mov			esi, m2Ptr								\
						__asm mov			edi, m1Ptr								\
						__asm mov			eax, dstPtr								\
						__asm movlps		xmm0, [esi+ 0*4]						\
						__asm movhps		xmm0, [esi+ 2*4]						\
						__asm movlps		xmm1, [esi+ 6*4]						\
						__asm movhps		xmm1, [esi+ 8*4]						\
						__asm movlps		xmm2, [esi+12*4]						\
						__asm movhps		xmm2, [esi+14*4]

						#define MUL_Nx3_3x6_FIRST4COLUMNS_ROW( row )				\
						__asm movss			xmm3, [edi+(row*3+0)*4]					\
						__asm shufps		xmm3, xmm3, R_SHUFFLEPS( 0, 0, 0, 0 )	\
						__asm mulps			xmm3, xmm0								\
						__asm movss			xmm4, [edi+(row*3+1)*4]					\
						__asm shufps		xmm4, xmm4, R_SHUFFLEPS( 0, 0, 0, 0 )	\
						__asm mulps			xmm4, xmm1								\
						__asm addps			xmm3, xmm4								\
						__asm movss			xmm5, [edi+(row*3+2)*4]					\
						__asm shufps		xmm5, xmm5, R_SHUFFLEPS( 0, 0, 0, 0 )	\
						__asm mulps			xmm5, xmm2								\
						__asm addps			xmm3, xmm5								\
						__asm movlps		[eax+(row*6+0)*4], xmm3					\
						__asm movhps		[eax+(row*6+2)*4], xmm3

						#define MUL_Nx3_3x6_LAST2COLUMNS_ROW6						\
						__asm movlps		xmm0, [esi+ 4*4]						\
						__asm movlps		xmm1, [esi+10*4]						\
						__asm movlps		xmm2, [esi+16*4]						\
						__asm shufps		xmm0, xmm0, 0x44						\
						__asm shufps		xmm1, xmm1, 0x44						\
						__asm shufps		xmm2, xmm2, 0x44						\
						__asm movlps		xmm3, [edi+0*4]							\
						__asm movhps		xmm3, [edi+2*4]							\
						__asm movaps		xmm4, xmm3								\
						__asm movaps		xmm5, xmm3								\
						__asm shufps		xmm3, xmm3, 0xF0						\
						__asm mulps			xmm3, xmm0								\
						__asm movlps		xmm6, [edi+4*4]							\
						__asm movhps		xmm6, [edi+6*4]							\
						__asm shufps		xmm4, xmm6, 0x05						\
						__asm mulps			xmm4, xmm1								\
						__asm addps			xmm3, xmm4								\
						__asm shufps		xmm5, xmm6, 0x5A						\
						__asm mulps			xmm5, xmm2								\
						__asm addps			xmm3, xmm5								\
						__asm movlps		[eax+4*4], xmm3							\
						__asm movhps		[eax+10*4], xmm3						\
						__asm movaps		xmm5, xmm6								\
						__asm movlps		xmm3, [edi+8*4]							\
						__asm movhps		xmm3, [edi+10*4]						\
						__asm movaps		xmm4, xmm3								\
						__asm shufps		xmm5, xmm3, 0x5A						\
						__asm mulps			xmm5, xmm0								\
						__asm shufps		xmm6, xmm3, 0xAF						\
						__asm mulps			xmm6, xmm1								\
						__asm addps			xmm5, xmm6								\
						__asm shufps		xmm4, xmm4, 0xF0						\
						__asm mulps			xmm4, xmm2								\
						__asm addps			xmm4, xmm5								\
						__asm movlps		[eax+16*4], xmm4						\
						__asm movhps		[eax+22*4], xmm4						\
						__asm movlps		xmm6, [edi+12*4]						\
						__asm movhps		xmm6, [edi+14*4]						\
						__asm movaps		xmm5, xmm6								\
						__asm movaps		xmm4, xmm6								\
						__asm shufps		xmm6, xmm6, 0xF0						\
						__asm mulps			xmm6, xmm0								\
						__asm movlps		xmm3, [edi+16*4]						\
						__asm shufps		xmm5, xmm3, 0x05						\
						__asm mulps			xmm5, xmm1								\
						__asm addps			xmm5, xmm6								\
						__asm shufps		xmm4, xmm3, 0x5A						\
						__asm mulps			xmm4, xmm2								\
						__asm addps			xmm4, xmm5								\
						__asm movlps		[eax+28*4], xmm4						\
						__asm movhps		[eax+34*4], xmm4

						MUL_Nx3_3x6_FIRST4COLUMNS_INIT
						MUL_Nx3_3x6_FIRST4COLUMNS_ROW( 0 )
						MUL_Nx3_3x6_FIRST4COLUMNS_ROW( 1 )
						MUL_Nx3_3x6_FIRST4COLUMNS_ROW( 2 )
						MUL_Nx3_3x6_FIRST4COLUMNS_ROW( 3 )
						MUL_Nx3_3x6_FIRST4COLUMNS_ROW( 4 )
						MUL_Nx3_3x6_FIRST4COLUMNS_ROW( 5 )
						MUL_Nx3_3x6_LAST2COLUMNS_ROW6

						return;
					}
				}
			}
			for ( i = 0; i < k; i++ ) {
				m2Ptr = m2.ToFloatPtr();
				for ( j = 0; j < l; j++ ) {
					*dstPtr++ = m1Ptr[0] * m2Ptr[0] + m1Ptr[1] * m2Ptr[l] + m1Ptr[2] * m2Ptr[2*l];
					m2Ptr++;
				}
				m1Ptr += 3;
			}
			break;
		}
		case 4: {
			if ( !(l^6) ) {
				switch( k ) {
					case 4: {			// 4x4 * 4x6

						#define MUL_Nx4_4x6_FIRST4COLUMNS_INIT						\
						__asm mov			esi, m2Ptr								\
						__asm mov			edi, m1Ptr								\
						__asm mov			eax, dstPtr								\
						__asm movlps		xmm0, [esi+ 0*4]						\
						__asm movhps		xmm0, [esi+ 2*4]						\
						__asm movlps		xmm1, [esi+ 6*4]						\
						__asm movhps		xmm1, [esi+ 8*4]						\
						__asm movlps		xmm2, [esi+12*4]						\
						__asm movhps		xmm2, [esi+14*4]						\
						__asm movlps		xmm3, [esi+18*4]						\
						__asm movhps		xmm3, [esi+20*4]

						#define MUL_Nx4_4x6_FIRST4COLUMNS_ROW( row )				\
						__asm movss			xmm4, [edi+row*16+0*4]					\
						__asm shufps		xmm4, xmm4, R_SHUFFLEPS( 0, 0, 0, 0 )	\
						__asm mulps			xmm4, xmm0								\
						__asm movss			xmm5, [edi+row*16+1*4]					\
						__asm shufps		xmm5, xmm5, R_SHUFFLEPS( 0, 0, 0, 0 )	\
						__asm mulps			xmm5, xmm1								\
						__asm addps			xmm4, xmm5								\
						__asm movss			xmm6, [edi+row*16+2*4]					\
						__asm shufps		xmm6, xmm6, R_SHUFFLEPS( 0, 0, 0, 0 )	\
						__asm mulps			xmm6, xmm2								\
						__asm addps			xmm4, xmm6								\
						__asm movss			xmm7, [edi+row*16+3*4]					\
						__asm shufps		xmm7, xmm7, R_SHUFFLEPS( 0, 0, 0, 0 )	\
						__asm mulps			xmm7, xmm3								\
						__asm addps			xmm4, xmm7								\
						__asm movlps		[eax+row*24+0], xmm4					\
						__asm movhps		[eax+row*24+8], xmm4

						#define MUL_Nx4_4x6_LAST2COLUMNS_INIT						\
						__asm movlps		xmm0, [esi+ 4*4]						\
						__asm movlps		xmm1, [esi+10*4]						\
						__asm movlps		xmm2, [esi+16*4]						\
						__asm movlps		xmm3, [esi+22*4]						\
						__asm shufps		xmm0, xmm1, R_SHUFFLEPS( 0, 1, 0, 1 )	\
						__asm shufps		xmm1, xmm0, R_SHUFFLEPS( 0, 1, 0, 1 )	\
						__asm shufps		xmm2, xmm3, R_SHUFFLEPS( 0, 1, 0, 1 )	\
						__asm shufps		xmm3, xmm2, R_SHUFFLEPS( 0, 1, 0, 1 )

						#define MUL_Nx4_4x6_LAST2COLUMNS_ROW2( row )				\
						__asm movlps		xmm7, [edi+row*32+ 0*4]					\
						__asm movhps		xmm7, [edi+row*32+ 4*4]					\
						__asm movaps		xmm6, xmm7								\
						__asm shufps		xmm6, xmm6, R_SHUFFLEPS( 0, 0, 3, 3 )	\
						__asm mulps			xmm6, xmm0								\
						__asm shufps		xmm7, xmm7, R_SHUFFLEPS( 1, 1, 2, 2 )	\
						__asm mulps			xmm7, xmm1								\
						__asm addps			xmm6, xmm7								\
						__asm movlps		xmm4, [edi+row*32+ 2*4]					\
						__asm movhps		xmm4, [edi+row*32+ 6*4]					\
						__asm movaps		xmm5, xmm4								\
						__asm shufps		xmm5, xmm5, R_SHUFFLEPS( 0, 0, 3, 3 )	\
						__asm mulps			xmm5, xmm2								\
						__asm addps			xmm6, xmm5								\
						__asm shufps		xmm4, xmm4, R_SHUFFLEPS( 1, 1, 2, 2 )	\
						__asm mulps			xmm4, xmm3								\
						__asm addps			xmm6, xmm4								\
						__asm movlps		[eax+row*48+ 4*4], xmm6					\
						__asm movhps		[eax+row*48+10*4], xmm6

						MUL_Nx4_4x6_FIRST4COLUMNS_INIT
						MUL_Nx4_4x6_FIRST4COLUMNS_ROW( 0 )
						MUL_Nx4_4x6_FIRST4COLUMNS_ROW( 1 )
						MUL_Nx4_4x6_FIRST4COLUMNS_ROW( 2 )
						MUL_Nx4_4x6_FIRST4COLUMNS_ROW( 3 )
						MUL_Nx4_4x6_LAST2COLUMNS_INIT
						MUL_Nx4_4x6_LAST2COLUMNS_ROW2( 0 )
						MUL_Nx4_4x6_LAST2COLUMNS_ROW2( 1 )

						return;
					}
					case 6: {			// 6x4 * 4x6

						MUL_Nx4_4x6_FIRST4COLUMNS_INIT
						MUL_Nx4_4x6_FIRST4COLUMNS_ROW( 0 )
						MUL_Nx4_4x6_FIRST4COLUMNS_ROW( 1 )
						MUL_Nx4_4x6_FIRST4COLUMNS_ROW( 2 )
						MUL_Nx4_4x6_FIRST4COLUMNS_ROW( 3 )
						MUL_Nx4_4x6_FIRST4COLUMNS_ROW( 4 )
						MUL_Nx4_4x6_FIRST4COLUMNS_ROW( 5 )
						MUL_Nx4_4x6_LAST2COLUMNS_INIT
						MUL_Nx4_4x6_LAST2COLUMNS_ROW2( 0 )
						MUL_Nx4_4x6_LAST2COLUMNS_ROW2( 1 )
						MUL_Nx4_4x6_LAST2COLUMNS_ROW2( 2 )

						return;
					}
				}
			}
			for ( i = 0; i < k; i++ ) {
				m2Ptr = m2.ToFloatPtr();
				for ( j = 0; j < l; j++ ) {
					*dstPtr++ = m1Ptr[0] * m2Ptr[0] + m1Ptr[1] * m2Ptr[l] + m1Ptr[2] * m2Ptr[2*l] +
									 m1Ptr[3] * m2Ptr[3*l];
					m2Ptr++;
				}
				m1Ptr += 4;
			}
			break;
		}
		case 5: {
			if ( !(l^6) ) {
				switch( k ) {
					case 5: {			// 5x5 * 5x6

						#define MUL_Nx5_5x6_FIRST4COLUMNS_INIT						\
						__asm mov			esi, m2Ptr								\
						__asm mov			edi, m1Ptr								\
						__asm mov			eax, dstPtr								\
						__asm movlps		xmm0, [esi+ 0*4]						\
						__asm movhps		xmm0, [esi+ 2*4]						\
						__asm movlps		xmm1, [esi+ 6*4]						\
						__asm movhps		xmm1, [esi+ 8*4]						\
						__asm movlps		xmm2, [esi+12*4]						\
						__asm movhps		xmm2, [esi+14*4]						\
						__asm movlps		xmm3, [esi+18*4]						\
						__asm movhps		xmm3, [esi+20*4]						\
						__asm movlps		xmm4, [esi+24*4]						\
						__asm movhps		xmm4, [esi+26*4]

						#define MUL_Nx5_5x6_FIRST4COLUMNS_ROW( row )				\
						__asm movss			xmm6, [edi+row*20+0*4]					\
						__asm shufps		xmm6, xmm6, R_SHUFFLEPS( 0, 0, 0, 0 )	\
						__asm mulps			xmm6, xmm0								\
						__asm movss			xmm5, [edi+row*20+1*4]					\
						__asm shufps		xmm5, xmm5, R_SHUFFLEPS( 0, 0, 0, 0 )	\
						__asm mulps			xmm5, xmm1								\
						__asm addps			xmm6, xmm5								\
						__asm movss			xmm5, [edi+row*20+2*4]					\
						__asm shufps		xmm5, xmm5, R_SHUFFLEPS( 0, 0, 0, 0 )	\
						__asm mulps			xmm5, xmm2								\
						__asm addps			xmm6, xmm5								\
						__asm movss			xmm5, [edi+row*20+3*4]					\
						__asm shufps		xmm5, xmm5, R_SHUFFLEPS( 0, 0, 0, 0 )	\
						__asm mulps			xmm5, xmm3								\
						__asm addps			xmm6, xmm5								\
						__asm movss			xmm5, [edi+row*20+4*4]					\
						__asm shufps		xmm5, xmm5, R_SHUFFLEPS( 0, 0, 0, 0 )	\
						__asm mulps			xmm5, xmm4								\
						__asm addps			xmm6, xmm5								\
						__asm movlps		[eax+row*24+0], xmm6					\
						__asm movhps		[eax+row*24+8], xmm6

						#define MUL_Nx5_5x6_LAST2COLUMNS_INIT						\
						__asm movlps		xmm0, [esi+ 4*4]						\
						__asm movlps		xmm1, [esi+10*4]						\
						__asm movlps		xmm2, [esi+16*4]						\
						__asm movlps		xmm3, [esi+22*4]						\
						__asm movlps		xmm4, [esi+28*4]						\
						__asm shufps		xmm0, xmm1, R_SHUFFLEPS( 0, 1, 0, 1 )	\
						__asm shufps		xmm1, xmm2, R_SHUFFLEPS( 0, 1, 0, 1 )	\
						__asm shufps		xmm2, xmm3, R_SHUFFLEPS( 0, 1, 0, 1 )	\
						__asm shufps		xmm3, xmm4, R_SHUFFLEPS( 0, 1, 0, 1 )	\
						__asm shufps		xmm4, xmm0, R_SHUFFLEPS( 0, 1, 0, 1 )

						#define MUL_Nx5_5x6_LAST2COLUMNS_ROW2( row )				\
						__asm movlps		xmm7, [edi+row*40+ 0*4]					\
						__asm movhps		xmm7, [edi+row*40+ 6*4]					\
						__asm movaps		xmm6, xmm7								\
						__asm shufps		xmm6, xmm6, R_SHUFFLEPS( 0, 0, 2, 2 )	\
						__asm mulps			xmm6, xmm0								\
						__asm movaps		xmm5, xmm7								\
						__asm shufps		xmm5, xmm5, R_SHUFFLEPS( 1, 1, 3, 3 )	\
						__asm mulps			xmm5, xmm1								\
						__asm addps			xmm6, xmm5								\
						__asm movlps		xmm7, [edi+row*40+ 2*4]					\
						__asm movhps		xmm7, [edi+row*40+ 8*4]					\
						__asm movaps		xmm5, xmm7								\
						__asm shufps		xmm5, xmm5, R_SHUFFLEPS( 0, 0, 2, 2 )	\
						__asm mulps			xmm5, xmm2								\
						__asm addps			xmm6, xmm5								\
						__asm movaps		xmm5, xmm7								\
						__asm shufps		xmm5, xmm5, R_SHUFFLEPS( 1, 1, 3, 3 )	\
						__asm mulps			xmm5, xmm3								\
						__asm addps			xmm6, xmm5								\
						__asm movlps		xmm5, [edi+row*40+ 4*4]					\
						__asm shufps		xmm5, xmm5, R_SHUFFLEPS( 0, 0, 1, 1 )	\
						__asm mulps			xmm5, xmm4								\
						__asm addps			xmm6, xmm5								\
						__asm movlps		[eax+row*48+ 4*4], xmm6					\
						__asm movhps		[eax+row*48+10*4], xmm6

						#define MUL_Nx5_5x6_LAST2COLUMNS_ROW( row )					\
						__asm movlps		xmm6, [edi+20*4+0*4]					\
						__asm unpcklps		xmm6, xmm6								\
						__asm mulps			xmm6, xmm0								\
						__asm movlps		xmm5, [edi+20*4+2*4]					\
						__asm unpcklps		xmm5, xmm5								\
						__asm mulps			xmm5, xmm2								\
						__asm addps			xmm6, xmm5								\
						__asm movss			xmm5, [edi+20*4+4*4]					\
						__asm unpcklps		xmm5, xmm5								\
						__asm mulps			xmm5, xmm4								\
						__asm addps			xmm6, xmm5								\
						__asm movhlps		xmm7, xmm6								\
						__asm addps			xmm6, xmm7								\
						__asm movlps		[eax+row*24+4*4], xmm6

						MUL_Nx5_5x6_FIRST4COLUMNS_INIT
						MUL_Nx5_5x6_FIRST4COLUMNS_ROW( 0 )
						MUL_Nx5_5x6_FIRST4COLUMNS_ROW( 1 )
						MUL_Nx5_5x6_FIRST4COLUMNS_ROW( 2 )
						MUL_Nx5_5x6_FIRST4COLUMNS_ROW( 3 )
						MUL_Nx5_5x6_FIRST4COLUMNS_ROW( 4 )
						MUL_Nx5_5x6_LAST2COLUMNS_INIT
						MUL_Nx5_5x6_LAST2COLUMNS_ROW2( 0 )
						MUL_Nx5_5x6_LAST2COLUMNS_ROW2( 1 )
						MUL_Nx5_5x6_LAST2COLUMNS_ROW( 4 )

						return;
					}
					case 6: {			// 6x5 * 5x6

						MUL_Nx5_5x6_FIRST4COLUMNS_INIT
						MUL_Nx5_5x6_FIRST4COLUMNS_ROW( 0 )
						MUL_Nx5_5x6_FIRST4COLUMNS_ROW( 1 )
						MUL_Nx5_5x6_FIRST4COLUMNS_ROW( 2 )
						MUL_Nx5_5x6_FIRST4COLUMNS_ROW( 3 )
						MUL_Nx5_5x6_FIRST4COLUMNS_ROW( 4 )
						MUL_Nx5_5x6_FIRST4COLUMNS_ROW( 5 )
						MUL_Nx5_5x6_LAST2COLUMNS_INIT
						MUL_Nx5_5x6_LAST2COLUMNS_ROW2( 0 )
						MUL_Nx5_5x6_LAST2COLUMNS_ROW2( 1 )
						MUL_Nx5_5x6_LAST2COLUMNS_ROW2( 2 )

						return;
					}
				}
			}
			for ( i = 0; i < k; i++ ) {
				m2Ptr = m2.ToFloatPtr();
				for ( j = 0; j < l; j++ ) {
					*dstPtr++ = m1Ptr[0] * m2Ptr[0] + m1Ptr[1] * m2Ptr[l] + m1Ptr[2] * m2Ptr[2*l] +
									 m1Ptr[3] * m2Ptr[3*l] + m1Ptr[4] * m2Ptr[4*l];
					m2Ptr++;
				}
				m1Ptr += 5;
			}
			break;
		}
		case 6: {
			switch( k ) {
				case 1: {
					if ( !(l^1) ) {		// 1x6 * 6x1
						dstPtr[0] = m1Ptr[0] * m2Ptr[0] + m1Ptr[1] * m2Ptr[1] + m1Ptr[2] * m2Ptr[2] +
									 m1Ptr[3] * m2Ptr[3] + m1Ptr[4] * m2Ptr[4] + m1Ptr[5] * m2Ptr[5];
						return;
					}
					break;
				}
				case 2: {
					if ( !(l^2) ) {		// 2x6 * 6x2

						#define MUL_Nx6_6x2_INIT								\
						__asm mov		esi, m2Ptr								\
						__asm mov		edi, m1Ptr								\
						__asm mov		eax, dstPtr								\
						__asm movaps	xmm0, [esi]								\
						__asm movaps	xmm1, [esi+16]							\
						__asm movaps	xmm2, [esi+32]

						#define MUL_Nx6_6x2_ROW2( row )							\
						__asm movaps	xmm7, [edi+row*48+0*4]					\
						__asm movaps	xmm6, xmm7								\
						__asm shufps	xmm7, xmm7, R_SHUFFLEPS( 0, 0, 1, 1 )	\
						__asm mulps		xmm7, xmm0								\
						__asm shufps	xmm6, xmm6, R_SHUFFLEPS( 2, 2, 3, 3 )	\
						__asm mulps		xmm6, xmm1								\
						__asm addps		xmm7, xmm6								\
						__asm movaps	xmm6, [edi+row*48+4*4]					\
						__asm movaps	xmm5, xmm6								\
						__asm shufps	xmm6, xmm6, R_SHUFFLEPS( 0, 0, 1, 1 )	\
						__asm mulps		xmm6, xmm2								\
						__asm addps		xmm7, xmm6								\
						__asm shufps	xmm5, xmm5, R_SHUFFLEPS( 2, 2, 3, 3 )	\
						__asm mulps		xmm5, xmm0								\
						__asm movaps	xmm6, [edi+row*48+24+2*4]				\
						__asm movaps	xmm4, xmm6								\
						__asm shufps	xmm6, xmm6, R_SHUFFLEPS( 0, 0, 1, 1 )	\
						__asm mulps		xmm6, xmm1								\
						__asm addps		xmm5, xmm6								\
						__asm shufps	xmm4, xmm4, R_SHUFFLEPS( 2, 2, 3, 3 )	\
						__asm mulps		xmm4, xmm2								\
						__asm addps		xmm5, xmm4								\
						__asm movaps	xmm4, xmm5								\
						__asm movhlps	xmm5, xmm7								\
						__asm movlhps	xmm7, xmm4								\
						__asm addps		xmm7, xmm5								\
						__asm movaps	[eax+row*16], xmm7

						MUL_Nx6_6x2_INIT
						MUL_Nx6_6x2_ROW2( 0 )

						return;
					}
					break;
				}
				case 3: {
					if ( !(l^3) ) {		// 3x6 * 6x3

						#define MUL_Nx6_6x3_INIT								\
						__asm mov		esi, m2Ptr								\
						__asm mov		edi, m1Ptr								\
						__asm mov		eax, dstPtr								\
						__asm movss		xmm0, [esi+ 0*4]						\
						__asm movhps	xmm0, [esi+ 1*4]						\
						__asm movss		xmm1, [esi+ 3*4]						\
						__asm movhps	xmm1, [esi+ 4*4]						\
						__asm movss		xmm2, [esi+ 6*4]						\
						__asm movhps	xmm2, [esi+ 7*4]						\
						__asm movss		xmm3, [esi+ 9*4]						\
						__asm movhps	xmm3, [esi+10*4]						\
						__asm movss		xmm4, [esi+12*4]						\
						__asm movhps	xmm4, [esi+13*4]						\
						__asm movss		xmm5, [esi+15*4]						\
						__asm movhps	xmm5, [esi+16*4]

						#define MUL_Nx6_6x3_ROW( row )							\
						__asm movss		xmm7, [edi+row*24+0]					\
						__asm shufps	xmm7, xmm7, R_SHUFFLEPS( 0, 0, 0, 0 )	\
						__asm mulps		xmm7, xmm0								\
						__asm movss		xmm6, [edi+row*24+4]					\
						__asm shufps	xmm6, xmm6, R_SHUFFLEPS( 0, 0, 0, 0 )	\
						__asm mulps		xmm6, xmm1								\
						__asm addps		xmm7, xmm6								\
						__asm movss		xmm6, [edi+row*24+8]					\
						__asm shufps	xmm6, xmm6, R_SHUFFLEPS( 0, 0, 0, 0 )	\
						__asm mulps		xmm6, xmm2								\
						__asm addps		xmm7, xmm6								\
						__asm movss		xmm6, [edi+row*24+12]					\
						__asm shufps	xmm6, xmm6, R_SHUFFLEPS( 0, 0, 0, 0 )	\
						__asm mulps		xmm6, xmm3								\
						__asm addps		xmm7, xmm6								\
						__asm movss		xmm6, [edi+row*24+16]					\
						__asm shufps	xmm6, xmm6, R_SHUFFLEPS( 0, 0, 0, 0 )	\
						__asm mulps		xmm6, xmm4								\
						__asm addps		xmm7, xmm6								\
						__asm movss		xmm6, [edi+row*24+20]					\
						__asm shufps	xmm6, xmm6, R_SHUFFLEPS( 0, 0, 0, 0 )	\
						__asm mulps		xmm6, xmm5								\
						__asm addps		xmm7, xmm6								\
						__asm movss		[eax+row*12+0], xmm7					\
						__asm movhps	[eax+row*12+4], xmm7

						MUL_Nx6_6x3_INIT
						MUL_Nx6_6x3_ROW( 0 )
						MUL_Nx6_6x3_ROW( 1 )
						MUL_Nx6_6x3_ROW( 2 )

						return;
					}
					break;
				}
				case 4: {
					if ( !(l^4) ) {		// 4x6 * 6x4

						#define MUL_Nx6_6x4_INIT								\
						__asm mov		esi, m2Ptr								\
						__asm mov		edi, m1Ptr								\
						__asm mov		eax, dstPtr								\
						__asm movaps	xmm0, [esi]								\
						__asm movaps	xmm1, [esi+16]							\
						__asm movaps	xmm2, [esi+32]							\
						__asm movaps	xmm3, [esi+48]							\
						__asm movaps	xmm4, [esi+64]							\
						__asm movaps	xmm5, [esi+80]

						#define MUL_Nx6_6x4_ROW( row )							\
						__asm movss		xmm7, [edi+row*24+0]					\
						__asm shufps	xmm7, xmm7, R_SHUFFLEPS( 0, 0, 0, 0 )	\
						__asm mulps		xmm7, xmm0								\
						__asm movss		xmm6, [edi+row*24+4]					\
						__asm shufps	xmm6, xmm6, R_SHUFFLEPS( 0, 0, 0, 0 )	\
						__asm mulps		xmm6, xmm1								\
						__asm addps		xmm7, xmm6								\
						__asm movss		xmm6, [edi+row*24+8]					\
						__asm shufps	xmm6, xmm6, R_SHUFFLEPS( 0, 0, 0, 0 )	\
						__asm mulps		xmm6, xmm2								\
						__asm addps		xmm7, xmm6								\
						__asm movss		xmm6, [edi+row*24+12]					\
						__asm shufps	xmm6, xmm6, R_SHUFFLEPS( 0, 0, 0, 0 )	\
						__asm mulps		xmm6, xmm3								\
						__asm addps		xmm7, xmm6								\
						__asm movss		xmm6, [edi+row*24+16]					\
						__asm shufps	xmm6, xmm6, R_SHUFFLEPS( 0, 0, 0, 0 )	\
						__asm mulps		xmm6, xmm4								\
						__asm addps		xmm7, xmm6								\
						__asm movss		xmm6, [edi+row*24+20]					\
						__asm shufps	xmm6, xmm6, R_SHUFFLEPS( 0, 0, 0, 0 )	\
						__asm mulps		xmm6, xmm5								\
						__asm addps		xmm7, xmm6								\
						__asm movaps	[eax+row*16], xmm7

						MUL_Nx6_6x4_INIT
						MUL_Nx6_6x4_ROW( 0 )
						MUL_Nx6_6x4_ROW( 1 )
						MUL_Nx6_6x4_ROW( 2 )
						MUL_Nx6_6x4_ROW( 3 )

						return;
					}
					break;
				}
				case 5: {
					if ( !(l^5) ) {		// 5x6 * 6x5

						#define MUL_Nx6_6x5_INIT								\
						__asm mov		esi, m2Ptr								\
						__asm mov		edi, m1Ptr								\
						__asm mov		eax, dstPtr								\
						__asm movaps	xmm0, [esi]								\
						__asm movlps	xmm1, [esi+20]							\
						__asm movhps	xmm1, [esi+28]							\
						__asm movlps	xmm2, [esi+40]							\
						__asm movhps	xmm2, [esi+48]							\
						__asm movlps	xmm3, [esi+60]							\
						__asm movhps	xmm3, [esi+68]							\
						__asm movaps	xmm4, [esi+80]							\
						__asm movlps	xmm5, [esi+100]							\
						__asm movhps	xmm5, [esi+108]

						#define MUL_Nx6_6x5_ROW( row )							\
						__asm movss		xmm7, [edi+row*24+0]					\
						__asm shufps	xmm7, xmm7, R_SHUFFLEPS( 0, 0, 0, 0 )	\
						__asm mulps		xmm7, xmm0								\
						__asm fld		dword ptr [edi+(row*6+0)*4]				\
						__asm fmul		dword ptr [esi+(4+0*5)*4]				\
						__asm movss		xmm6, [edi+row*24+4]					\
						__asm shufps	xmm6, xmm6, R_SHUFFLEPS( 0, 0, 0, 0 )	\
						__asm mulps		xmm6, xmm1								\
						__asm addps		xmm7, xmm6								\
						__asm fld		dword ptr [edi+(row*6+1)*4]				\
						__asm fmul		dword ptr [esi+(4+1*5)*4]				\
						__asm faddp		st(1),st								\
						__asm movss		xmm6, [edi+row*24+8]					\
						__asm shufps	xmm6, xmm6, R_SHUFFLEPS( 0, 0, 0, 0 )	\
						__asm mulps		xmm6, xmm2								\
						__asm addps		xmm7, xmm6								\
						__asm fld		dword ptr [edi+(row*6+2)*4]				\
						__asm fmul		dword ptr [esi+(4+2*5)*4]				\
						__asm faddp		st(1),st								\
						__asm movss		xmm6, [edi+row*24+12]					\
						__asm shufps	xmm6, xmm6, R_SHUFFLEPS( 0, 0, 0, 0 )	\
						__asm mulps		xmm6, xmm3								\
						__asm addps		xmm7, xmm6								\
						__asm fld		dword ptr [edi+(row*6+3)*4]				\
						__asm fmul		dword ptr [esi+(4+3*5)*4]				\
						__asm faddp		st(1),st								\
						__asm movss		xmm6, [edi+row*24+16]					\
						__asm shufps	xmm6, xmm6, R_SHUFFLEPS( 0, 0, 0, 0 )	\
						__asm mulps		xmm6, xmm4								\
						__asm addps		xmm7, xmm6								\
						__asm fld		dword ptr [edi+(row*6+4)*4]				\
						__asm fmul		dword ptr [esi+(4+4*5)*4]				\
						__asm faddp		st(1),st								\
						__asm movss		xmm6, [edi+row*24+20]					\
						__asm shufps	xmm6, xmm6, R_SHUFFLEPS( 0, 0, 0, 0 )	\
						__asm mulps		xmm6, xmm5								\
						__asm addps		xmm7, xmm6								\
						__asm fld		dword ptr [edi+(row*6+5)*4]				\
						__asm fmul		dword ptr [esi+(4+5*5)*4]				\
						__asm faddp		st(1),st								\
						__asm fstp		dword ptr [eax+(row*5+4)*4]				\
						__asm movlps	[eax+row*20], xmm7						\
						__asm movhps	[eax+row*20+8], xmm7

						MUL_Nx6_6x5_INIT
						MUL_Nx6_6x5_ROW( 0 )
						MUL_Nx6_6x5_ROW( 1 )
						MUL_Nx6_6x5_ROW( 2 )
						MUL_Nx6_6x5_ROW( 3 )
						MUL_Nx6_6x5_ROW( 4 )

						return;
					}
					break;
				}
				case 6: {
					switch( l ) {
						case 1: {		// 6x6 * 6x1
							__asm {
								mov			esi, m2Ptr
								mov			edi, m1Ptr
								mov			eax, dstPtr
								movlps		xmm7, qword ptr [esi]
								movlps		xmm6, qword ptr [esi+8]
								shufps		xmm7, xmm7, 0x44
								shufps		xmm6, xmm6, 0x44
								movlps		xmm0, qword ptr [edi    ]
								movhps		xmm0, qword ptr [edi+ 24]
								mulps		xmm0, xmm7
								movlps		xmm3, qword ptr [edi+  8]
								movhps		xmm3, qword ptr [edi+ 32]
								mulps		xmm3, xmm6
								movlps		xmm1, qword ptr [edi+ 48]
								movhps		xmm1, qword ptr [edi+ 72]
								mulps		xmm1, xmm7
								movlps		xmm2, qword ptr [edi+ 96]
								movhps		xmm2, qword ptr [edi+120]
								mulps		xmm2, xmm7
								movlps		xmm4, qword ptr [edi+ 56]
								movhps		xmm4, qword ptr [edi+ 80]
								movlps		xmm5, qword ptr [edi+104]
								movhps		xmm5, qword ptr [edi+128]
								mulps		xmm4, xmm6
								movlps		xmm7, qword ptr [esi+16]
								addps		xmm0, xmm3
								shufps		xmm7, xmm7, 0x44
								mulps		xmm5, xmm6
								addps		xmm1, xmm4
								movlps		xmm3, qword ptr [edi+ 16]
								movhps		xmm3, qword ptr [edi+ 40]
								addps		xmm2, xmm5
								movlps		xmm4, qword ptr [edi+ 64]
								movhps		xmm4, qword ptr [edi+ 88]
								mulps		xmm3, xmm7
								movlps		xmm5, qword ptr [edi+112]
								movhps		xmm5, qword ptr [edi+136]
								addps		xmm0, xmm3
								mulps		xmm4, xmm7
								mulps		xmm5, xmm7
								addps		xmm1, xmm4
								addps		xmm2, xmm5
								movaps		xmm6, xmm0
								shufps		xmm0, xmm1, 0x88
								shufps		xmm6, xmm1, 0xDD
								movaps		xmm7, xmm2
								shufps		xmm7, xmm2, 0x88
								shufps		xmm2, xmm2, 0xDD
								addps		xmm0, xmm6
								addps		xmm2, xmm7
								movlps		[eax], xmm0
								movhps		[eax+8], xmm0
								movlps		[eax+16], xmm2
							}
							return;
						}
						case 2: {		// 6x6 * 6x2

							MUL_Nx6_6x2_INIT
							MUL_Nx6_6x2_ROW2( 0 )
							MUL_Nx6_6x2_ROW2( 1 )
							MUL_Nx6_6x2_ROW2( 2 )

							return;
						}
						case 3: {		// 6x6 * 6x3

							MUL_Nx6_6x3_INIT
							MUL_Nx6_6x3_ROW( 0 )
							MUL_Nx6_6x3_ROW( 1 )
							MUL_Nx6_6x3_ROW( 2 )
							MUL_Nx6_6x3_ROW( 3 )
							MUL_Nx6_6x3_ROW( 4 )
							MUL_Nx6_6x3_ROW( 5 )

							return;
						}
						case 4: {		// 6x6 * 6x4

							MUL_Nx6_6x4_INIT
							MUL_Nx6_6x4_ROW( 0 )
							MUL_Nx6_6x4_ROW( 1 )
							MUL_Nx6_6x4_ROW( 2 )
							MUL_Nx6_6x4_ROW( 3 )
							MUL_Nx6_6x4_ROW( 4 )
							MUL_Nx6_6x4_ROW( 5 )

							return;
						}
						case 5: {		// 6x6 * 6x5

							MUL_Nx6_6x5_INIT
							MUL_Nx6_6x5_ROW( 0 )
							MUL_Nx6_6x5_ROW( 1 )
							MUL_Nx6_6x5_ROW( 2 )
							MUL_Nx6_6x5_ROW( 3 )
							MUL_Nx6_6x5_ROW( 4 )
							MUL_Nx6_6x5_ROW( 5 )

							return;
						}
						case 6: {		// 6x6 * 6x6
							__asm {
								mov			ecx, dword ptr m2Ptr
								movlps		xmm3, qword ptr [ecx+72]
								mov			edx, dword ptr m1Ptr
								// Loading first 4 columns (upper 4 rows) of m2Ptr.
								movaps		xmm0, xmmword ptr [ecx]
								movlps		xmm1, qword ptr [ecx+24]
								movhps		xmm1, qword ptr [ecx+32]
								movaps		xmm2, xmmword ptr [ecx+48]
								movhps		xmm3, qword ptr [ecx+80]
								// Calculating first 4 elements in the first row of the destination matrix.
								movss		xmm4, dword ptr [edx]
								movss		xmm5, dword ptr [edx+4]
								mov			eax, dword ptr dstPtr
								shufps		xmm4, xmm4, 0
								movss		xmm6, dword ptr [edx+8]
								shufps		xmm5, xmm5, 0
								movss		xmm7, dword ptr [edx+12]
								mulps		xmm4, xmm0
								shufps		xmm6, xmm6, 0
								shufps		xmm7, xmm7, 0
								mulps		xmm5, xmm1
								mulps		xmm6, xmm2
								addps		xmm5, xmm4
								mulps		xmm7, xmm3
								addps		xmm6, xmm5
								addps		xmm7, xmm6
								movaps		xmmword ptr [eax], xmm7
								// Calculating first 4 elements in the second row of the destination matrix.
								movss		xmm4, dword ptr [edx+24]
								shufps		xmm4, xmm4, 0
								mulps		xmm4, xmm0
								movss		xmm5, dword ptr [edx+28]
								shufps		xmm5, xmm5, 0
								mulps		xmm5, xmm1
								movss		xmm6, dword ptr [edx+32]
								shufps		xmm6, xmm6, 0
								movss		xmm7, dword ptr [edx+36]
								shufps		xmm7, xmm7, 0
								mulps		xmm6, xmm2
								mulps		xmm7, xmm3
								addps		xmm7, xmm6
								addps		xmm5, xmm4
								addps		xmm7, xmm5
								// Calculating first 4 elements in the third row of the destination matrix.
								movss		xmm4, dword ptr [edx+48]
								movss		xmm5, dword ptr [edx+52]
								movlps		qword ptr [eax+24], xmm7 ; save 2nd
								movhps		qword ptr [eax+32], xmm7 ; row
								movss		xmm6, dword ptr [edx+56]
								movss		xmm7, dword ptr [edx+60]
								shufps		xmm4, xmm4, 0
								shufps		xmm5, xmm5, 0
								shufps		xmm6, xmm6, 0
								shufps		xmm7, xmm7, 0
								mulps		xmm4, xmm0
								mulps		xmm5, xmm1
								mulps		xmm6, xmm2
								mulps		xmm7, xmm3
								addps		xmm5, xmm4
								addps		xmm7, xmm6
								addps		xmm7, xmm5
								movaps		xmmword ptr [eax+48], xmm7
								// Calculating first 4 elements in the fourth row of the destination matrix.
								movss		xmm4, dword ptr [edx+72]
								movss		xmm5, dword ptr [edx+76]
								movss		xmm6, dword ptr [edx+80]
								movss		xmm7, dword ptr [edx+84]
								shufps		xmm4, xmm4, 0
								shufps		xmm5, xmm5, 0
								shufps		xmm6, xmm6, 0
								shufps		xmm7, xmm7, 0
								mulps		xmm4, xmm0
								mulps		xmm5, xmm1
								mulps		xmm6, xmm2
								mulps		xmm7, xmm3
								addps		xmm4, xmm5
								addps		xmm6, xmm4
								addps		xmm7, xmm6
								movlps		qword ptr [eax+72], xmm7
								movhps		qword ptr [eax+80], xmm7
								// Calculating first 4 elements in the fifth row of the destination matrix.
								movss		xmm4, dword ptr [edx+96]
								movss		xmm5, dword ptr [edx+100]
								movss		xmm6, dword ptr [edx+104]
								movss		xmm7, dword ptr [edx+108]
								shufps		xmm4, xmm4, 0
								shufps		xmm5, xmm5, 0
								shufps		xmm6, xmm6, 0
								shufps		xmm7, xmm7, 0
								mulps		xmm4, xmm0
								mulps		xmm5, xmm1
								mulps		xmm6, xmm2
								mulps		xmm7, xmm3
								addps		xmm5, xmm4
								addps		xmm7, xmm6
								addps		xmm7, xmm5
								movaps		xmmword ptr [eax+96], xmm7
								// Calculating first 4 elements in the sixth row of the destination matrix.
								movss		xmm4, dword ptr [edx+120]
								movss		xmm5, dword ptr [edx+124]
								movss		xmm6, dword ptr [edx+128]
								movss		xmm7, dword ptr [edx+132]
								shufps		xmm4, xmm4, 0
								shufps		xmm5, xmm5, 0
								shufps		xmm6, xmm6, 0
								shufps		xmm7, xmm7, 0
								mulps		xmm4, xmm0
								mulps		xmm5, xmm1
								mulps		xmm6, xmm2
								mulps		xmm7, xmm3
								addps		xmm4, xmm5
								addps		xmm6, xmm4
								addps		xmm7, xmm6
								movhps		qword ptr [eax+128], xmm7
								movlps		qword ptr [eax+120], xmm7
								// Loading first 4 columns (lower 2 rows) of m2Ptr.
								movlps		xmm0, qword ptr [ecx+96]
								movhps		xmm0, qword ptr [ecx+104]
								movlps		xmm1, qword ptr [ecx+120]
								movhps		xmm1, qword ptr [ecx+128]
								// Calculating first 4 elements in the first row of the destination matrix.
								movss		xmm2, dword ptr [edx+16]
								shufps		xmm2, xmm2, 0
								movss		xmm4, dword ptr [edx+40]
								movss		xmm3, dword ptr [edx+20]
								movss		xmm5, dword ptr [edx+44]
								movaps		xmm6, xmmword ptr [eax]
								movlps		xmm7, qword ptr [eax+24]
								shufps		xmm3, xmm3, 0
								shufps		xmm5, xmm5, 0
								movhps		xmm7, qword ptr [eax+32]
								shufps		xmm4, xmm4, 0
								mulps		xmm5, xmm1
								mulps		xmm2, xmm0
								mulps		xmm3, xmm1
								mulps		xmm4, xmm0
								addps		xmm6, xmm2
								addps		xmm7, xmm4
								addps		xmm7, xmm5
								addps		xmm6, xmm3
								movlps		qword ptr [eax+24], xmm7
								movaps		xmmword ptr [eax], xmm6
								movhps		qword ptr [eax+32], xmm7
								// Calculating first 4 elements in the third row of the destination matrix.
								movss		xmm2, dword ptr [edx+64]
								movss		xmm4, dword ptr [edx+88]
								movss		xmm5, dword ptr [edx+92]
								movss		xmm3, dword ptr [edx+68]
								movaps		xmm6, xmmword ptr [eax+48]
								movlps		xmm7, qword ptr [eax+72]
								movhps		xmm7, qword ptr [eax+80]
								shufps		xmm2, xmm2, 0
								shufps		xmm4, xmm4, 0
								shufps		xmm5, xmm5, 0
								shufps		xmm3, xmm3, 0
								mulps		xmm2, xmm0
								mulps		xmm4, xmm0
								mulps		xmm5, xmm1
								mulps		xmm3, xmm1
								addps		xmm6, xmm2
								addps		xmm6, xmm3
								addps		xmm7, xmm4
								addps		xmm7, xmm5
								movlps		qword ptr [eax+72], xmm7
								movaps		xmmword ptr [eax+48], xmm6
								movhps		qword ptr [eax+80], xmm7
								// Calculating first 4 elements in the fifth row of the destination matrix.
								movss		xmm2, dword ptr [edx+112]
								movss		xmm3, dword ptr [edx+116]
								movaps		xmm6, xmmword ptr [eax+96]
								shufps		xmm2, xmm2, 0
								shufps		xmm3, xmm3, 0
								mulps		xmm2, xmm0
								mulps		xmm3, xmm1
								addps		xmm6, xmm2
								addps		xmm6, xmm3
								movaps		xmmword ptr [eax+96], xmm6
								// Calculating first 4 elements in the sixth row of the destination matrix.
								movss		xmm4, dword ptr [edx+136]
								movss		xmm5, dword ptr [edx+140]
								movhps		xmm7, qword ptr [eax+128]
								movlps		xmm7, qword ptr [eax+120]
								shufps		xmm4, xmm4, 0
								shufps		xmm5, xmm5, 0
								mulps		xmm4, xmm0
								mulps		xmm5, xmm1
								addps		xmm7, xmm4
								addps		xmm7, xmm5
								// Calculating last 2 columns of the destination matrix.
								movlps		xmm0, qword ptr [ecx+16]
								movhps		xmm0, qword ptr [ecx+40]
								movhps		qword ptr [eax+128], xmm7
								movlps		qword ptr [eax+120], xmm7
								movlps		xmm2, qword ptr [ecx+64]
								movhps		xmm2, qword ptr [ecx+88]
								movaps		xmm3, xmm2
								shufps		xmm3, xmm3, 4Eh
								movlps		xmm4, qword ptr [ecx+112]
								movhps		xmm4, qword ptr [ecx+136]
								movaps		xmm5, xmm4
								shufps		xmm5, xmm5, 4Eh
								movlps		xmm6, qword ptr [edx]
								movhps		xmm6, qword ptr [edx+24]
								movaps		xmm7, xmm6
								shufps		xmm7, xmm7, 0F0h
								mulps		xmm7, xmm0
								shufps		xmm6, xmm6, 0A5h
								movaps		xmm1, xmm0
								shufps		xmm1, xmm1, 4Eh
								mulps		xmm1, xmm6
								addps		xmm7, xmm1
								movlps		xmm6, qword ptr [edx+8]
								movhps		xmm6, qword ptr [edx+32]
								movaps		xmm1, xmm6
								shufps		xmm1, xmm1, 0F0h
								shufps		xmm6, xmm6, 0A5h
								mulps		xmm1, xmm2
								mulps		xmm6, xmm3
								addps		xmm7, xmm1
								addps		xmm7, xmm6
								movhps		xmm6, qword ptr [edx+40]
								movlps		xmm6, qword ptr [edx+16]
								movaps		xmm1, xmm6
								shufps		xmm1, xmm1, 0F0h
								shufps		xmm6, xmm6, 0A5h
								mulps		xmm1, xmm4
								mulps		xmm6, xmm5
								addps		xmm7, xmm1
								addps		xmm7, xmm6
								movlps		qword ptr [eax+16], xmm7
								movhps		qword ptr [eax+40], xmm7
								movlps		xmm6, qword ptr [edx+48]
								movhps		xmm6, qword ptr [edx+72]
								movaps		xmm7, xmm6
								shufps		xmm7, xmm7, 0F0h
								mulps		xmm7, xmm0
								shufps		xmm6, xmm6, 0A5h
								movaps		xmm1, xmm0
								shufps		xmm1, xmm1, 4Eh
								mulps		xmm1, xmm6
								addps		xmm7, xmm1
								movhps		xmm6, qword ptr [edx+80]
								movlps		xmm6, qword ptr [edx+56]
								movaps		xmm1, xmm6
								shufps		xmm1, xmm1, 0F0h
								shufps		xmm6, xmm6, 0A5h
								mulps		xmm1, xmm2
								mulps		xmm6, xmm3
								addps		xmm7, xmm1
								addps		xmm7, xmm6
								movlps		xmm6, qword ptr [edx+64]
								movhps		xmm6, qword ptr [edx+88]
								movaps		xmm1, xmm6
								shufps		xmm1, xmm1, 0F0h
								shufps		xmm6, xmm6, 0A5h
								mulps		xmm1, xmm4
								mulps		xmm6, xmm5
								addps		xmm7, xmm1
								addps		xmm7, xmm6
								movlps		qword ptr [eax+64], xmm7
								movhps		qword ptr [eax+88], xmm7
								movlps		xmm6, qword ptr [edx+96]
								movhps		xmm6, qword ptr [edx+120]
								movaps		xmm7, xmm6
								shufps		xmm7, xmm7, 0F0h
								mulps		xmm7, xmm0
								shufps		xmm6, xmm6, 0A5h
								movaps		xmm1, xmm0
								shufps		xmm1, xmm1, 4Eh
								mulps		xmm1, xmm6
								addps		xmm7, xmm1
								movlps		xmm6, qword ptr [edx+104]
								movhps		xmm6, qword ptr [edx+128]
								movaps		xmm1, xmm6
								shufps		xmm1, xmm1, 0F0h
								shufps		xmm6, xmm6, 0A5h
								mulps		xmm1, xmm2
								mulps		xmm6, xmm3
								addps		xmm7, xmm1
								addps		xmm7, xmm6
								movlps		xmm6, qword ptr [edx+112]
								movhps		xmm6, qword ptr [edx+136]
								movaps		xmm1, xmm6
								shufps		xmm1, xmm1, 0F0h
								shufps		xmm6, xmm6, 0A5h
								mulps		xmm1, xmm4
								mulps		xmm6, xmm5
								addps		xmm7, xmm1
								addps		xmm7, xmm6
								movlps		qword ptr [eax+112], xmm7
								movhps		qword ptr [eax+136], xmm7
							}
							return;
						}
					}
				}
			}
			for ( i = 0; i < k; i++ ) {
				m2Ptr = m2.ToFloatPtr();
				for ( j = 0; j < l; j++ ) {
					*dstPtr++ = m1Ptr[0] * m2Ptr[0] + m1Ptr[1] * m2Ptr[l] + m1Ptr[2] * m2Ptr[2*l] +
									 m1Ptr[3] * m2Ptr[3*l] + m1Ptr[4] * m2Ptr[4*l] + m1Ptr[5] * m2Ptr[5*l];
					m2Ptr++;
				}
				m1Ptr += 6;
			}
			break;
		}
		default: {
			for ( i = 0; i < k; i++ ) {
				for ( j = 0; j < l; j++ ) {
					m2Ptr = m2.ToFloatPtr() + j;
					sum = m1Ptr[0] * m2Ptr[0];
					for ( n = 1; n < m1.GetNumColumns(); n++ ) {
						m2Ptr += l;
						sum += m1Ptr[n] * m2Ptr[0];
					}
					*dstPtr++ = sum;
				}
				m1Ptr += m1.GetNumColumns();
			}
			break;
		}
	}
}

/*
============
idSIMD_SSE::MatX_TransposeMultiplyMatX

	optimizes the following transpose matrix multiplications:

	Nx6 * NxN
	6xN * 6x6

	with N in the range [1-6].
============
*/
void VPCALL idSIMD_SSE::MatX_TransposeMultiplyMatX( idMatX &dst, const idMatX &m1, const idMatX &m2 ) {
	int i, j, k, l, n;
	float *dstPtr;
	const float *m1Ptr, *m2Ptr;
	double sum;

	assert( m1.GetNumRows() == m2.GetNumRows() );

	m1Ptr = m1.ToFloatPtr();
	m2Ptr = m2.ToFloatPtr();
	dstPtr = dst.ToFloatPtr();
	k = m1.GetNumColumns();
	l = m2.GetNumColumns();

	switch( m1.GetNumRows() ) {
		case 1:
			if ( !((k^6)|(l^1)) ) {			// 1x6 * 1x1
				__asm {
					mov		esi, m2Ptr
					mov		edi, m1Ptr
					mov		eax, dstPtr
					movss	xmm0, [esi]
					shufps	xmm0, xmm0, R_SHUFFLEPS( 0, 0, 0, 0 )
					movaps	xmm1, xmm0
					mulps	xmm0, [edi]
					mulps	xmm1, [edi+16]
					movaps	[eax], xmm0
					movlps	[eax+16], xmm1
				}
				return;
			}
			for ( i = 0; i < k; i++ ) {
				m2Ptr = m2.ToFloatPtr();
				for ( j = 0; j < l; j++ ) {
					*dstPtr++ = m1Ptr[0] * m2Ptr[0];
					m2Ptr++;
				}
				m1Ptr++;
			}
			break;
		case 2:
			if ( !((k^6)|(l^2)) ) {			// 2x6 * 2x2
				#define MUL_2xN_2x2_INIT								\
				__asm mov		esi, m2Ptr								\
				__asm mov		edi, m1Ptr								\
				__asm mov		eax, dstPtr								\
				__asm movlps	xmm0, [esi]								\
				__asm shufps	xmm0, xmm0, R_SHUFFLEPS( 0, 1, 0, 1 )	\
				__asm movlps	xmm1, [esi+8]							\
				__asm shufps	xmm1, xmm1, R_SHUFFLEPS( 0, 1, 0, 1 )

				#define MUL_2xN_2x2_ROW2( N, row )						\
				__asm movlps	xmm6, [edi+(row+0*N)*4]					\
				__asm shufps	xmm6, xmm6, R_SHUFFLEPS( 0, 0, 1, 1 )	\
				__asm movlps	xmm7, [edi+(row+1*N)*4]					\
				__asm shufps	xmm7, xmm7, R_SHUFFLEPS( 0, 0, 1, 1 )	\
				__asm mulps		xmm6, xmm0								\
				__asm mulps		xmm7, xmm1								\
				__asm addps		xmm6, xmm7								\
				__asm movaps	[eax+(row*2)*4], xmm6

				MUL_2xN_2x2_INIT
				MUL_2xN_2x2_ROW2( 6, 0 )
				MUL_2xN_2x2_ROW2( 6, 2 )
				MUL_2xN_2x2_ROW2( 6, 4 )

				return;
			}
			for ( i = 0; i < k; i++ ) {
				m2Ptr = m2.ToFloatPtr();
				for ( j = 0; j < l; j++ ) {
					*dstPtr++ = m1Ptr[0] * m2Ptr[0] + m1Ptr[k] * m2Ptr[l];
					m2Ptr++;
				}
				m1Ptr++;
			}
			break;
		case 3:
			if ( !((k^6)|(l^3)) ) {			// 3x6 * 3x3

				#define MUL_3xN_3x3_INIT								\
				__asm mov		esi, m2Ptr								\
				__asm mov		edi, m1Ptr								\
				__asm mov		eax, dstPtr								\
				__asm movss		xmm0, [esi+(0*3+0)*4]					\
				__asm movhps	xmm0, [esi+(0*3+1)*4]					\
				__asm movss		xmm1, [esi+(1*3+0)*4]					\
				__asm movhps	xmm1, [esi+(1*3+1)*4]					\
				__asm movss		xmm2, [esi+(2*3+0)*4]					\
				__asm movhps	xmm2, [esi+(2*3+1)*4]

				#define MUL_3xN_3x3_INIT_ROW4							\
				__asm shufps	xmm0, xmm0, R_SHUFFLEPS( 0, 2, 3, 0 )	\
				__asm shufps	xmm1, xmm1, R_SHUFFLEPS( 0, 2, 3, 0 )	\
				__asm shufps	xmm2, xmm2, R_SHUFFLEPS( 0, 2, 3, 0 )

				#define MUL_3xN_3x3_ROW4( N, row )						\
				__asm movlps	xmm3, [edi+(row+0*N+0)*4]				\
				__asm shufps	xmm3, xmm3, R_SHUFFLEPS( 0, 0, 0, 1 )	\
				__asm movlps	xmm4, [edi+(row+1*N+0)*4]				\
				__asm shufps	xmm4, xmm4, R_SHUFFLEPS( 0, 0, 0, 1 )	\
				__asm movlps	xmm5, [edi+(row+2*N+0)*4]				\
				__asm shufps	xmm5, xmm5, R_SHUFFLEPS( 0, 0, 0, 1 )	\
				__asm mulps		xmm3, xmm0								\
				__asm mulps		xmm4, xmm1								\
				__asm mulps		xmm5, xmm2								\
				__asm addps		xmm3, xmm4								\
				__asm addps		xmm3, xmm5								\
				__asm movaps	[eax+(row*3+0)*4], xmm3					\
				__asm shufps	xmm0, xmm0, R_SHUFFLEPS( 1, 2, 3, 1 )	\
				__asm shufps	xmm1, xmm1, R_SHUFFLEPS( 1, 2, 3, 1 )	\
				__asm shufps	xmm2, xmm2, R_SHUFFLEPS( 1, 2, 3, 1 )	\
				__asm movlps	xmm3, [edi+(row+0*N+1)*4]				\
				__asm shufps	xmm3, xmm3, R_SHUFFLEPS( 0, 0, 1, 1 )	\
				__asm movlps	xmm4, [edi+(row+1*N+1)*4]				\
				__asm shufps	xmm4, xmm4, R_SHUFFLEPS( 0, 0, 1, 1 )	\
				__asm movlps	xmm5, [edi+(row+2*N+1)*4]				\
				__asm shufps	xmm5, xmm5, R_SHUFFLEPS( 0, 0, 1, 1 )	\
				__asm mulps		xmm3, xmm0								\
				__asm mulps		xmm4, xmm1								\
				__asm mulps		xmm5, xmm2								\
				__asm addps		xmm3, xmm4								\
				__asm addps		xmm3, xmm5								\
				__asm movaps	[eax+(row*3+4)*4], xmm3					\
				__asm shufps	xmm0, xmm0, R_SHUFFLEPS( 1, 2, 3, 1 )	\
				__asm shufps	xmm1, xmm1, R_SHUFFLEPS( 1, 2, 3, 1 )	\
				__asm shufps	xmm2, xmm2, R_SHUFFLEPS( 1, 2, 3, 1 )	\
				__asm movlps	xmm3, [edi+(row+0*N+2)*4]				\
				__asm shufps	xmm3, xmm3, R_SHUFFLEPS( 0, 1, 1, 1 )	\
				__asm movlps	xmm4, [edi+(row+1*N+2)*4]				\
				__asm shufps	xmm4, xmm4, R_SHUFFLEPS( 0, 1, 1, 1 )	\
				__asm movlps	xmm5, [edi+(row+2*N+2)*4]				\
				__asm shufps	xmm5, xmm5, R_SHUFFLEPS( 0, 1, 1, 1 )	\
				__asm mulps		xmm3, xmm0								\
				__asm mulps		xmm4, xmm1								\
				__asm mulps		xmm5, xmm2								\
				__asm addps		xmm3, xmm4								\
				__asm addps		xmm3, xmm5								\
				__asm movaps	[eax+(row*3+8)*4], xmm3

				#define MUL_3xN_3x3_INIT_ROW4_ROW4						\
				__asm shufps	xmm0, xmm0, R_SHUFFLEPS( 1, 2, 3, 0 )	\
				__asm shufps	xmm1, xmm1, R_SHUFFLEPS( 1, 2, 3, 0 )	\
				__asm shufps	xmm2, xmm2, R_SHUFFLEPS( 1, 2, 3, 0 )

				#define MUL_3xN_3x3_INIT_ROW4_ROW						\
				__asm shufps	xmm0, xmm0, R_SHUFFLEPS( 1, 1, 2, 3 )	\
				__asm shufps	xmm1, xmm1, R_SHUFFLEPS( 1, 1, 2, 3 )	\
				__asm shufps	xmm2, xmm2, R_SHUFFLEPS( 1, 1, 2, 3 )

				#define MUL_3xN_3x3_ROW( N, row )						\
				__asm movss		xmm3, [edi+(row+0*N)*4]					\
				__asm shufps	xmm3, xmm3, R_SHUFFLEPS( 0, 0, 0, 0 )	\
				__asm movss		xmm4, [edi+(row+1*N)*4]					\
				__asm shufps	xmm4, xmm4, R_SHUFFLEPS( 0, 0, 0, 0 )	\
				__asm movss		xmm5, [edi+(row+2*N)*4]					\
				__asm shufps	xmm5, xmm5, R_SHUFFLEPS( 0, 0, 0, 0 )	\
				__asm mulps		xmm3, xmm0								\
				__asm mulps		xmm4, xmm1								\
				__asm mulps		xmm5, xmm2								\
				__asm addps		xmm3, xmm4								\
				__asm addps		xmm3, xmm5								\
				__asm movss		[eax+(row*3+0)*4], xmm3					\
				__asm movhps	[eax+(row*3+1)*4], xmm3

				MUL_3xN_3x3_INIT
				MUL_3xN_3x3_INIT_ROW4
				MUL_3xN_3x3_ROW4( 6, 0 )
				MUL_3xN_3x3_INIT_ROW4_ROW
				MUL_3xN_3x3_ROW( 6, 4 )
				MUL_3xN_3x3_ROW( 6, 5 )

				return;
			}
			for ( i = 0; i < k; i++ ) {
				m2Ptr = m2.ToFloatPtr();
				for ( j = 0; j < l; j++ ) {
					*dstPtr++ = m1Ptr[0] * m2Ptr[0] + m1Ptr[k] * m2Ptr[l] + m1Ptr[2*k] * m2Ptr[2*l];
					m2Ptr++;
				}
				m1Ptr++;
			}
			break;
		case 4:
			if ( !((k^6)|(l^4)) ) {			// 4x6 * 4x4

				#define MUL_4xN_4x4_INIT								\
				__asm mov		esi, m2Ptr								\
				__asm mov		edi, m1Ptr								\
				__asm mov		eax, dstPtr								\
				__asm movaps	xmm0, [esi]								\
				__asm movaps	xmm1, [esi+16]							\
				__asm movaps	xmm2, [esi+32]							\
				__asm movaps	xmm3, [esi+48]

				#define MUL_4xN_4x4_ROW( N, row )						\
				__asm movss		xmm7, [edi+(row+0*N)*4]					\
				__asm shufps	xmm7, xmm7, R_SHUFFLEPS( 0, 0, 0, 0 )	\
				__asm mulps		xmm7, xmm0								\
				__asm movss		xmm6, [edi+(row+1*N)*4]					\
				__asm shufps	xmm6, xmm6, R_SHUFFLEPS( 0, 0, 0, 0 )	\
				__asm mulps		xmm6, xmm1								\
				__asm addps		xmm7, xmm6								\
				__asm movss		xmm6, [edi+(row+2*N)*4]					\
				__asm shufps	xmm6, xmm6, R_SHUFFLEPS( 0, 0, 0, 0 )	\
				__asm mulps		xmm6, xmm2								\
				__asm addps		xmm7, xmm6								\
				__asm movss		xmm6, [edi+(row+3*N)*4]					\
				__asm shufps	xmm6, xmm6, R_SHUFFLEPS( 0, 0, 0, 0 )	\
				__asm mulps		xmm6, xmm3								\
				__asm addps		xmm7, xmm6								\
				__asm movaps	[eax+row*16], xmm7

				MUL_4xN_4x4_INIT
				MUL_4xN_4x4_ROW( 6, 0 )
				MUL_4xN_4x4_ROW( 6, 1 )
				MUL_4xN_4x4_ROW( 6, 2 )
				MUL_4xN_4x4_ROW( 6, 3 )
				MUL_4xN_4x4_ROW( 6, 4 )
				MUL_4xN_4x4_ROW( 6, 5 )

				return;
			}
			for ( i = 0; i < k; i++ ) {
				m2Ptr = m2.ToFloatPtr();
				for ( j = 0; j < l; j++ ) {
					*dstPtr++ = m1Ptr[0] * m2Ptr[0] + m1Ptr[k] * m2Ptr[l] + m1Ptr[2*k] * m2Ptr[2*l] +
									m1Ptr[3*k] * m2Ptr[3*l];
					m2Ptr++;
				}
				m1Ptr++;
			}
			break;
		case 5:
			if ( !((k^6)|(l^5)) ) {			// 5x6 * 5x5

				#define MUL_5xN_5x5_INIT								\
				__asm mov		esi, m2Ptr								\
				__asm mov		edi, m1Ptr								\
				__asm mov		eax, dstPtr								\
				__asm movlps	xmm0, [esi+ 0*4]						\
				__asm movhps	xmm0, [esi+ 2*4]						\
				__asm movlps	xmm1, [esi+ 5*4]						\
				__asm movhps	xmm1, [esi+ 7*4]						\
				__asm movlps	xmm2, [esi+10*4]						\
				__asm movhps	xmm2, [esi+12*4]						\
				__asm movlps	xmm3, [esi+15*4]						\
				__asm movhps	xmm3, [esi+17*4]						\
				__asm movlps	xmm4, [esi+20*4]						\
				__asm movhps	xmm4, [esi+22*4]

				#define MUL_5xN_5x5_ROW( N, row )						\
				__asm movss		xmm6, [edi+(row+0*N)*4]					\
				__asm shufps	xmm6, xmm6, R_SHUFFLEPS( 0, 0, 0, 0 )	\
				__asm mulps		xmm6, xmm0								\
				__asm fld		dword ptr [edi+(row+0*N)*4]				\
				__asm fmul		dword ptr [esi+ 4*4]					\
				__asm movss		xmm5, [edi+(row+1*N)*4]					\
				__asm shufps	xmm5, xmm5, R_SHUFFLEPS( 0, 0, 0, 0 )	\
				__asm mulps		xmm5, xmm1								\
				__asm addps		xmm6, xmm5								\
				__asm fld		dword ptr [edi+(row+1*N)*4]				\
				__asm fmul		dword ptr [esi+ 9*4]					\
				__asm faddp		st(1),st								\
				__asm movss		xmm5, [edi+(row+2*N)*4]					\
				__asm shufps	xmm5, xmm5, R_SHUFFLEPS( 0, 0, 0, 0 )	\
				__asm mulps		xmm5, xmm2								\
				__asm addps		xmm6, xmm5								\
				__asm fld		dword ptr [edi+(row+2*N)*4]				\
				__asm fmul		dword ptr [esi+14*4]					\
				__asm faddp		st(1),st								\
				__asm movss		xmm5, [edi+(row+3*N)*4]					\
				__asm shufps	xmm5, xmm5, R_SHUFFLEPS( 0, 0, 0, 0 )	\
				__asm mulps		xmm5, xmm3								\
				__asm addps		xmm6, xmm5								\
				__asm fld		dword ptr [edi+(row+3*N)*4]				\
				__asm fmul		dword ptr [esi+19*4]					\
				__asm faddp		st(1),st								\
				__asm movss		xmm5, [edi+(row+4*N)*4]					\
				__asm shufps	xmm5, xmm5, R_SHUFFLEPS( 0, 0, 0, 0 )	\
				__asm mulps		xmm5, xmm4								\
				__asm addps		xmm6, xmm5								\
				__asm fld		dword ptr [edi+(row+4*N)*4]				\
				__asm fmul		dword ptr [esi+24*4]					\
				__asm faddp		st(1),st								\
				__asm fstp		dword ptr [eax+(row*5+4)*4]				\
				__asm movlps	[eax+(row*5+0)*4], xmm6					\
				__asm movhps	[eax+(row*5+2)*4], xmm6

				MUL_5xN_5x5_INIT
				MUL_5xN_5x5_ROW( 6, 0 )
				MUL_5xN_5x5_ROW( 6, 1 )
				MUL_5xN_5x5_ROW( 6, 2 )
				MUL_5xN_5x5_ROW( 6, 3 )
				MUL_5xN_5x5_ROW( 6, 4 )
				MUL_5xN_5x5_ROW( 6, 5 )

				return;
			}
			for ( i = 0; i < k; i++ ) {
				m2Ptr = m2.ToFloatPtr();
				for ( j = 0; j < l; j++ ) {
					*dstPtr++ = m1Ptr[0] * m2Ptr[0] + m1Ptr[k] * m2Ptr[l] + m1Ptr[2*k] * m2Ptr[2*l] +
									m1Ptr[3*k] * m2Ptr[3*l] + m1Ptr[4*k] * m2Ptr[4*l];
					m2Ptr++;
				}
				m1Ptr++;
			}
			break;
		case 6:
			if ( !(l^6) ) {
				switch( k ) {
					case 1: {					// 6x1 * 6x6
						#define MUL_6xN_6x6_FIRST4COLUMNS_INIT					\
						__asm mov		esi, m2Ptr								\
						__asm mov		edi, m1Ptr								\
						__asm mov		eax, dstPtr								\
						__asm movlps	xmm0, [esi+ 0*4]						\
						__asm movhps	xmm0, [esi+ 2*4]						\
						__asm movlps	xmm1, [esi+ 6*4]						\
						__asm movhps	xmm1, [esi+ 8*4]						\
						__asm movlps	xmm2, [esi+12*4]						\
						__asm movhps	xmm2, [esi+14*4]						\
						__asm movlps	xmm3, [esi+18*4]						\
						__asm movhps	xmm3, [esi+20*4]						\
						__asm movlps	xmm4, [esi+24*4]						\
						__asm movhps	xmm4, [esi+26*4]						\
						__asm movlps	xmm5, [esi+30*4]						\
						__asm movhps	xmm5, [esi+32*4]

						#define MUL_6xN_6x6_FIRST4COLUMNS_ROW( N, row )			\
						__asm movss		xmm7, [edi+(row+0*N)*4]					\
						__asm shufps	xmm7, xmm7, R_SHUFFLEPS( 0, 0, 0, 0 )	\
						__asm mulps		xmm7, xmm0								\
						__asm movss		xmm6, [edi+(row+1*N)*4]					\
						__asm shufps	xmm6, xmm6, R_SHUFFLEPS( 0, 0, 0, 0 )	\
						__asm mulps		xmm6, xmm1								\
						__asm addps		xmm7, xmm6								\
						__asm movss		xmm6, [edi+(row+2*N)*4]					\
						__asm shufps	xmm6, xmm6, R_SHUFFLEPS( 0, 0, 0, 0 )	\
						__asm mulps		xmm6, xmm2								\
						__asm addps		xmm7, xmm6								\
						__asm movss		xmm6, [edi+(row+3*N)*4]					\
						__asm shufps	xmm6, xmm6, R_SHUFFLEPS( 0, 0, 0, 0 )	\
						__asm mulps		xmm6, xmm3								\
						__asm addps		xmm7, xmm6								\
						__asm movss		xmm6, [edi+(row+4*N)*4]					\
						__asm shufps	xmm6, xmm6, R_SHUFFLEPS( 0, 0, 0, 0 )	\
						__asm mulps		xmm6, xmm4								\
						__asm addps		xmm7, xmm6								\
						__asm movss		xmm6, [edi+(row+5*N)*4]					\
						__asm shufps	xmm6, xmm6, R_SHUFFLEPS( 0, 0, 0, 0 )	\
						__asm mulps		xmm6, xmm5								\
						__asm addps		xmm7, xmm6								\
						__asm movlps	[eax+(row*6+0)*4], xmm7					\
						__asm movhps	[eax+(row*6+2)*4], xmm7

						#define MUL_6xN_6x6_LAST2COLUMNS_INIT					\
						__asm movlps	xmm0, [esi+ 4*4]						\
						__asm movlps	xmm1, [esi+10*4]						\
						__asm shufps	xmm0, xmm0, R_SHUFFLEPS( 0, 1, 0, 1 )	\
						__asm shufps	xmm1, xmm1, R_SHUFFLEPS( 0, 1, 0, 1 )	\
						__asm movlps	xmm2, [esi+16*4]						\
						__asm movlps	xmm3, [esi+22*4]						\
						__asm shufps	xmm2, xmm2, R_SHUFFLEPS( 0, 1, 0, 1 )	\
						__asm shufps	xmm3, xmm3, R_SHUFFLEPS( 0, 1, 0, 1 )	\
						__asm movlps	xmm4, [esi+28*4]						\
						__asm movlps	xmm5, [esi+34*4]						\
						__asm shufps	xmm4, xmm4, R_SHUFFLEPS( 0, 1, 0, 1 )	\
						__asm shufps	xmm5, xmm5, R_SHUFFLEPS( 0, 1, 0, 1 )

						#define MUL_6xN_6x6_LAST2COLUMNS_ROW2( N, row )			\
						__asm movlps	xmm7, [edi+(row*2+0*N)*4]				\
						__asm shufps	xmm7, xmm7, R_SHUFFLEPS( 0, 0, 1, 1 )	\
						__asm mulps		xmm7, xmm0								\
						__asm movlps	xmm6, [edi+(row*2+1*N)*4]				\
						__asm shufps	xmm6, xmm6, R_SHUFFLEPS( 0, 0, 1, 1 )	\
						__asm mulps		xmm6, xmm1								\
						__asm addps		xmm7, xmm6								\
						__asm movlps	xmm6, [edi+(row*2+2*N)*4]				\
						__asm shufps	xmm6, xmm6, R_SHUFFLEPS( 0, 0, 1, 1 )	\
						__asm mulps		xmm6, xmm2								\
						__asm addps		xmm7, xmm6								\
						__asm movlps	xmm6, [edi+(row*2+3*N)*4]				\
						__asm shufps	xmm6, xmm6, R_SHUFFLEPS( 0, 0, 1, 1 )	\
						__asm mulps		xmm6, xmm3								\
						__asm addps		xmm7, xmm6								\
						__asm movlps	xmm6, [edi+(row*2+4*N)*4]				\
						__asm shufps	xmm6, xmm6, R_SHUFFLEPS( 0, 0, 1, 1 )	\
						__asm mulps		xmm6, xmm4								\
						__asm addps		xmm7, xmm6								\
						__asm movlps	xmm6, [edi+(row*2+5*N)*4]				\
						__asm shufps	xmm6, xmm6, R_SHUFFLEPS( 0, 0, 1, 1 )	\
						__asm mulps		xmm6, xmm5								\
						__asm addps		xmm7, xmm6								\
						__asm movlps	[eax+(row*12+ 4)*4], xmm7				\
						__asm movhps	[eax+(row*12+10)*4], xmm7

						#define MUL_6xN_6x6_LAST2COLUMNS_ROW( N, row )			\
						__asm movss		xmm7, [edi+(1*N-1)*4]					\
						__asm shufps	xmm7, xmm7, R_SHUFFLEPS( 0, 0, 0, 0 )	\
						__asm mulps		xmm7, xmm0								\
						__asm movss		xmm6, [edi+(2*N-1)*4]					\
						__asm shufps	xmm6, xmm6, R_SHUFFLEPS( 0, 0, 0, 0 )	\
						__asm mulps		xmm6, xmm1								\
						__asm addps		xmm7, xmm6								\
						__asm movss		xmm6, [edi+(3*N-1)*4]					\
						__asm shufps	xmm6, xmm6, R_SHUFFLEPS( 0, 0, 0, 0 )	\
						__asm mulps		xmm6, xmm2								\
						__asm addps		xmm7, xmm6								\
						__asm movss		xmm6, [edi+(4*N-1)*4]					\
						__asm shufps	xmm6, xmm6, R_SHUFFLEPS( 0, 0, 0, 0 )	\
						__asm mulps		xmm6, xmm3								\
						__asm addps		xmm7, xmm6								\
						__asm movss		xmm6, [edi+(5*N-1)*4]					\
						__asm shufps	xmm6, xmm6, R_SHUFFLEPS( 0, 0, 0, 0 )	\
						__asm mulps		xmm6, xmm4								\
						__asm addps		xmm7, xmm6								\
						__asm movss		xmm6, [edi+(6*N-1)*4]					\
						__asm shufps	xmm6, xmm6, R_SHUFFLEPS( 0, 0, 0, 0 )	\
						__asm mulps		xmm6, xmm5								\
						__asm addps		xmm7, xmm6								\
						__asm movlps	[eax+(row*6+4)*4], xmm7

						MUL_6xN_6x6_FIRST4COLUMNS_INIT
						MUL_6xN_6x6_FIRST4COLUMNS_ROW( 1, 0 )
						MUL_6xN_6x6_LAST2COLUMNS_INIT
						MUL_6xN_6x6_LAST2COLUMNS_ROW( 1, 0 )

						return;
					}
					case 2: {					// 6x2 * 6x6

						MUL_6xN_6x6_FIRST4COLUMNS_INIT
						MUL_6xN_6x6_FIRST4COLUMNS_ROW( 2, 0 )
						MUL_6xN_6x6_FIRST4COLUMNS_ROW( 2, 1 )
						MUL_6xN_6x6_LAST2COLUMNS_INIT
						MUL_6xN_6x6_LAST2COLUMNS_ROW2( 2, 0 )

						return;
					}
					case 3: {					// 6x3 * 6x6

						MUL_6xN_6x6_FIRST4COLUMNS_INIT
						MUL_6xN_6x6_FIRST4COLUMNS_ROW( 3, 0 )
						MUL_6xN_6x6_FIRST4COLUMNS_ROW( 3, 1 )
						MUL_6xN_6x6_FIRST4COLUMNS_ROW( 3, 2 )
						MUL_6xN_6x6_LAST2COLUMNS_INIT
						MUL_6xN_6x6_LAST2COLUMNS_ROW2( 3, 0 )
						MUL_6xN_6x6_LAST2COLUMNS_ROW( 3, 2 )

						return;
					}
					case 4: {					// 6x4 * 6x6

						MUL_6xN_6x6_FIRST4COLUMNS_INIT
						MUL_6xN_6x6_FIRST4COLUMNS_ROW( 4, 0 )
						MUL_6xN_6x6_FIRST4COLUMNS_ROW( 4, 1 )
						MUL_6xN_6x6_FIRST4COLUMNS_ROW( 4, 2 )
						MUL_6xN_6x6_FIRST4COLUMNS_ROW( 4, 3 )
						MUL_6xN_6x6_LAST2COLUMNS_INIT
						MUL_6xN_6x6_LAST2COLUMNS_ROW2( 4, 0 )
						MUL_6xN_6x6_LAST2COLUMNS_ROW2( 4, 1 )

						return;
					}
					case 5: {					// 6x5 * 6x6

						MUL_6xN_6x6_FIRST4COLUMNS_INIT
						MUL_6xN_6x6_FIRST4COLUMNS_ROW( 5, 0 )
						MUL_6xN_6x6_FIRST4COLUMNS_ROW( 5, 1 )
						MUL_6xN_6x6_FIRST4COLUMNS_ROW( 5, 2 )
						MUL_6xN_6x6_FIRST4COLUMNS_ROW( 5, 3 )
						MUL_6xN_6x6_FIRST4COLUMNS_ROW( 5, 4 )
						MUL_6xN_6x6_LAST2COLUMNS_INIT
						MUL_6xN_6x6_LAST2COLUMNS_ROW2( 5, 0 )
						MUL_6xN_6x6_LAST2COLUMNS_ROW2( 5, 1 )
						MUL_6xN_6x6_LAST2COLUMNS_ROW( 5, 4 )

						return;
					}
					case 6: {					// 6x6 * 6x6

						MUL_6xN_6x6_FIRST4COLUMNS_INIT
						MUL_6xN_6x6_FIRST4COLUMNS_ROW( 6, 0 )
						MUL_6xN_6x6_FIRST4COLUMNS_ROW( 6, 1 )
						MUL_6xN_6x6_FIRST4COLUMNS_ROW( 6, 2 )
						MUL_6xN_6x6_FIRST4COLUMNS_ROW( 6, 3 )
						MUL_6xN_6x6_FIRST4COLUMNS_ROW( 6, 4 )
						MUL_6xN_6x6_FIRST4COLUMNS_ROW( 6, 5 )
						MUL_6xN_6x6_LAST2COLUMNS_INIT
						MUL_6xN_6x6_LAST2COLUMNS_ROW2( 6, 0 )
						MUL_6xN_6x6_LAST2COLUMNS_ROW2( 6, 1 )
						MUL_6xN_6x6_LAST2COLUMNS_ROW2( 6, 2 )

						return;
					}
				}
			}
			for ( i = 0; i < k; i++ ) {
				m2Ptr = m2.ToFloatPtr();
				for ( j = 0; j < l; j++ ) {
					*dstPtr++ = m1Ptr[0] * m2Ptr[0] + m1Ptr[k] * m2Ptr[l] + m1Ptr[2*k] * m2Ptr[2*l] +
									m1Ptr[3*k] * m2Ptr[3*l] + m1Ptr[4*k] * m2Ptr[4*l] + m1Ptr[5*k] * m2Ptr[5*l];
					m2Ptr++;
				}
				m1Ptr++;
			}
			break;
		default:
			for ( i = 0; i < k; i++ ) {
				for ( j = 0; j < l; j++ ) {
					m1Ptr = m1.ToFloatPtr() + i;
					m2Ptr = m2.ToFloatPtr() + j;
					sum = m1Ptr[0] * m2Ptr[0];
					for ( n = 1; n < m1.GetNumRows(); n++ ) {
						m1Ptr += k;
						m2Ptr += l;
						sum += m1Ptr[0] * m2Ptr[0];
					}
					*dstPtr++ = sum;
				}
			}
		break;
	}
}

/*
============
idSIMD_SSE::MatX_LowerTriangularSolve

  solves x in Lx = b for the n * n sub-matrix of L
  if skip > 0 the first skip elements of x are assumed to be valid already
  L has to be a lower triangular matrix with (implicit) ones on the diagonal
  x == b is allowed
============
*/
void VPCALL idSIMD_SSE::MatX_LowerTriangularSolve( const idMatX &L, float *x, const float *b, const int n, int skip ) {
	int nc;
	const float *lptr;

	if ( skip >= n ) {
		return;
	}

	lptr = L.ToFloatPtr();
	nc = L.GetNumColumns();

	// unrolled cases for n < 8
	if ( n < 8 ) {
		#define NSKIP( n, s )	((n<<3)|(s&7))
		switch( NSKIP( n, skip ) ) {
			case NSKIP( 1, 0 ): x[0] = b[0];
				return;
			case NSKIP( 2, 0 ): x[0] = b[0];
			case NSKIP( 2, 1 ): x[1] = b[1] - lptr[1*nc+0] * x[0];
				return;
			case NSKIP( 3, 0 ): x[0] = b[0];
			case NSKIP( 3, 1 ): x[1] = b[1] - lptr[1*nc+0] * x[0];
			case NSKIP( 3, 2 ): x[2] = b[2] - lptr[2*nc+0] * x[0] - lptr[2*nc+1] * x[1];
				return;
			case NSKIP( 4, 0 ): x[0] = b[0];
			case NSKIP( 4, 1 ): x[1] = b[1] - lptr[1*nc+0] * x[0];
			case NSKIP( 4, 2 ): x[2] = b[2] - lptr[2*nc+0] * x[0] - lptr[2*nc+1] * x[1];
			case NSKIP( 4, 3 ): x[3] = b[3] - lptr[3*nc+0] * x[0] - lptr[3*nc+1] * x[1] - lptr[3*nc+2] * x[2];
				return;
			case NSKIP( 5, 0 ): x[0] = b[0];
			case NSKIP( 5, 1 ): x[1] = b[1] - lptr[1*nc+0] * x[0];
			case NSKIP( 5, 2 ): x[2] = b[2] - lptr[2*nc+0] * x[0] - lptr[2*nc+1] * x[1];
			case NSKIP( 5, 3 ): x[3] = b[3] - lptr[3*nc+0] * x[0] - lptr[3*nc+1] * x[1] - lptr[3*nc+2] * x[2];
			case NSKIP( 5, 4 ): x[4] = b[4] - lptr[4*nc+0] * x[0] - lptr[4*nc+1] * x[1] - lptr[4*nc+2] * x[2] - lptr[4*nc+3] * x[3];
				return;
			case NSKIP( 6, 0 ): x[0] = b[0];
			case NSKIP( 6, 1 ): x[1] = b[1] - lptr[1*nc+0] * x[0];
			case NSKIP( 6, 2 ): x[2] = b[2] - lptr[2*nc+0] * x[0] - lptr[2*nc+1] * x[1];
			case NSKIP( 6, 3 ): x[3] = b[3] - lptr[3*nc+0] * x[0] - lptr[3*nc+1] * x[1] - lptr[3*nc+2] * x[2];
			case NSKIP( 6, 4 ): x[4] = b[4] - lptr[4*nc+0] * x[0] - lptr[4*nc+1] * x[1] - lptr[4*nc+2] * x[2] - lptr[4*nc+3] * x[3];
			case NSKIP( 6, 5 ): x[5] = b[5] - lptr[5*nc+0] * x[0] - lptr[5*nc+1] * x[1] - lptr[5*nc+2] * x[2] - lptr[5*nc+3] * x[3] - lptr[5*nc+4] * x[4];
				return;
			case NSKIP( 7, 0 ): x[0] = b[0];
			case NSKIP( 7, 1 ): x[1] = b[1] - lptr[1*nc+0] * x[0];
			case NSKIP( 7, 2 ): x[2] = b[2] - lptr[2*nc+0] * x[0] - lptr[2*nc+1] * x[1];
			case NSKIP( 7, 3 ): x[3] = b[3] - lptr[3*nc+0] * x[0] - lptr[3*nc+1] * x[1] - lptr[3*nc+2] * x[2];
			case NSKIP( 7, 4 ): x[4] = b[4] - lptr[4*nc+0] * x[0] - lptr[4*nc+1] * x[1] - lptr[4*nc+2] * x[2] - lptr[4*nc+3] * x[3];
			case NSKIP( 7, 5 ): x[5] = b[5] - lptr[5*nc+0] * x[0] - lptr[5*nc+1] * x[1] - lptr[5*nc+2] * x[2] - lptr[5*nc+3] * x[3] - lptr[5*nc+4] * x[4];
			case NSKIP( 7, 6 ): x[6] = b[6] - lptr[6*nc+0] * x[0] - lptr[6*nc+1] * x[1] - lptr[6*nc+2] * x[2] - lptr[6*nc+3] * x[3] - lptr[6*nc+4] * x[4] - lptr[6*nc+5] * x[5];
				return;
		}
		return;
	}

	// process first 4 rows
	switch( skip ) {
		case 0: x[0] = b[0];
		case 1: x[1] = b[1] - lptr[1*nc+0] * x[0];
		case 2: x[2] = b[2] - lptr[2*nc+0] * x[0] - lptr[2*nc+1] * x[1];
		case 3: x[3] = b[3] - lptr[3*nc+0] * x[0] - lptr[3*nc+1] * x[1] - lptr[3*nc+2] * x[2];
				skip = 4;
	}

	lptr = L[skip];

	// this code assumes n > 4
	__asm {
		push		ebx
		mov			eax, skip				// eax = i
		shl			eax, 2					// eax = i*4
		mov			edx, n					// edx = n
		shl			edx, 2					// edx = n*4
		mov			esi, x					// esi = x
		mov			edi, lptr				// edi = lptr
		add			esi, eax
		add			edi, eax
		mov			ebx, b					// ebx = b

		// check for aligned memory
		mov			ecx, nc
		shl			ecx, 2
		or			ecx, esi
		or			ecx, edi
		and			ecx, 15
		jnz			loopurow

		// aligned
	looprow:
		mov			ecx, eax
		neg			ecx
		movaps		xmm0, [esi+ecx]
		mulps		xmm0, [edi+ecx]
		add			ecx, 12*4
		jg			donedot8
	dot8:
		movaps		xmm1, [esi+ecx-(8*4)]
		mulps		xmm1, [edi+ecx-(8*4)]
		addps		xmm0, xmm1
		movaps		xmm3, [esi+ecx-(4*4)]
		mulps		xmm3, [edi+ecx-(4*4)]
		addps		xmm0, xmm3
		add			ecx, 8*4
		jle			dot8
	donedot8:
		sub			ecx, 4*4
		jg			donedot4
	//dot4:
		movaps		xmm1, [esi+ecx-(4*4)]
		mulps		xmm1, [edi+ecx-(4*4)]
		addps		xmm0, xmm1
		add			ecx, 4*4
	donedot4:
		movhlps		xmm1, xmm0
		addps		xmm0, xmm1
		movaps		xmm1, xmm0
		shufps		xmm1, xmm1, R_SHUFFLEPS( 1, 0, 0, 0 )
		addss		xmm0, xmm1
		sub			ecx, 4*4
		jz			dot0
		add			ecx, 4
		jz			dot1
		add			ecx, 4
		jz			dot2
	//dot3:
		movss		xmm1, [esi-(3*4)]
		mulss		xmm1, [edi-(3*4)]
		addss		xmm0, xmm1
	dot2:
		movss		xmm3, [esi-(2*4)]
		mulss		xmm3, [edi-(2*4)]
		addss		xmm0, xmm3
	dot1:
		movss		xmm5, [esi-(1*4)]
		mulss		xmm5, [edi-(1*4)]
		addss		xmm0, xmm5
	dot0:
		movss		xmm1, [ebx+eax]
		subss		xmm1, xmm0
		movss		[esi], xmm1
		add			eax, 4
		cmp			eax, edx
		jge			done
		add			esi, 4
		mov			ecx, nc
		shl			ecx, 2
		add			edi, ecx
		add			edi, 4
		jmp			looprow

		// unaligned
	loopurow:
		mov			ecx, eax
		neg			ecx
		movups		xmm0, [esi+ecx]
		movups		xmm1, [edi+ecx]
		mulps		xmm0, xmm1
		add			ecx, 12*4
		jg			doneudot8
	udot8:
		movups		xmm1, [esi+ecx-(8*4)]
		movups		xmm2, [edi+ecx-(8*4)]
		mulps		xmm1, xmm2
		addps		xmm0, xmm1
		movups		xmm3, [esi+ecx-(4*4)]
		movups		xmm4, [edi+ecx-(4*4)]
		mulps		xmm3, xmm4
		addps		xmm0, xmm3
		add			ecx, 8*4
		jle			udot8
	doneudot8:
		sub			ecx, 4*4
		jg			doneudot4
	//udot4:
		movups		xmm1, [esi+ecx-(4*4)]
		movups		xmm2, [edi+ecx-(4*4)]
		mulps		xmm1, xmm2
		addps		xmm0, xmm1
		add			ecx, 4*4
	doneudot4:
		movhlps		xmm1, xmm0
		addps		xmm0, xmm1
		movaps		xmm1, xmm0
		shufps		xmm1, xmm1, R_SHUFFLEPS( 1, 0, 0, 0 )
		addss		xmm0, xmm1
		sub			ecx, 4*4
		jz			udot0
		add			ecx, 4
		jz			udot1
		add			ecx, 4
		jz			udot2
	//udot3:
		movss		xmm1, [esi-(3*4)]
		movss		xmm2, [edi-(3*4)]
		mulss		xmm1, xmm2
		addss		xmm0, xmm1
	udot2:
		movss		xmm3, [esi-(2*4)]
		movss		xmm4, [edi-(2*4)]
		mulss		xmm3, xmm4
		addss		xmm0, xmm3
	udot1:
		movss		xmm5, [esi-(1*4)]
		movss		xmm6, [edi-(1*4)]
		mulss		xmm5, xmm6
		addss		xmm0, xmm5
	udot0:
		movss		xmm1, [ebx+eax]
		subss		xmm1, xmm0
		movss		[esi], xmm1
		add			eax, 4
		cmp			eax, edx
		jge			done
		add			esi, 4
		mov			ecx, nc
		shl			ecx, 2
		add			edi, ecx
		add			edi, 4
		jmp			loopurow
	done:
		pop			ebx
	}
}

/*
============
idSIMD_SSE::MatX_LowerTriangularSolveTranspose

  solves x in L'x = b for the n * n sub-matrix of L
  L has to be a lower triangular matrix with (implicit) ones on the diagonal
  x == b is allowed
============
*/
void VPCALL idSIMD_SSE::MatX_LowerTriangularSolveTranspose( const idMatX &L, float *x, const float *b, const int n ) {
	int nc;
	const float *lptr;

	lptr = L.ToFloatPtr();
	nc = L.GetNumColumns();

	// unrolled cases for n < 8
	if ( n < 8 ) {
		switch( n ) {
			case 0:
				return;
			case 1:
				x[0] = b[0];
				return;
			case 2:
				x[1] = b[1];
				x[0] = b[0] - lptr[1*nc+0] * x[1];
				return;
			case 3:
				x[2] = b[2];
				x[1] = b[1] - lptr[2*nc+1] * x[2];
				x[0] = b[0] - lptr[2*nc+0] * x[2] - lptr[1*nc+0] * x[1];
				return;
			case 4:
				x[3] = b[3];
				x[2] = b[2] - lptr[3*nc+2] * x[3];
				x[1] = b[1] - lptr[3*nc+1] * x[3] - lptr[2*nc+1] * x[2];
				x[0] = b[0] - lptr[3*nc+0] * x[3] - lptr[2*nc+0] * x[2] - lptr[1*nc+0] * x[1];
				return;
			case 5:
				x[4] = b[4];
				x[3] = b[3] - lptr[4*nc+3] * x[4];
				x[2] = b[2] - lptr[4*nc+2] * x[4] - lptr[3*nc+2] * x[3];
				x[1] = b[1] - lptr[4*nc+1] * x[4] - lptr[3*nc+1] * x[3] - lptr[2*nc+1] * x[2];
				x[0] = b[0] - lptr[4*nc+0] * x[4] - lptr[3*nc+0] * x[3] - lptr[2*nc+0] * x[2] - lptr[1*nc+0] * x[1];
				return;
			case 6:
				x[5] = b[5];
				x[4] = b[4] - lptr[5*nc+4] * x[5];
				x[3] = b[3] - lptr[5*nc+3] * x[5] - lptr[4*nc+3] * x[4];
				x[2] = b[2] - lptr[5*nc+2] * x[5] - lptr[4*nc+2] * x[4] - lptr[3*nc+2] * x[3];
				x[1] = b[1] - lptr[5*nc+1] * x[5] - lptr[4*nc+1] * x[4] - lptr[3*nc+1] * x[3] - lptr[2*nc+1] * x[2];
				x[0] = b[0] - lptr[5*nc+0] * x[5] - lptr[4*nc+0] * x[4] - lptr[3*nc+0] * x[3] - lptr[2*nc+0] * x[2] - lptr[1*nc+0] * x[1];
				return;
			case 7:
				x[6] = b[6];
				x[5] = b[5] - lptr[6*nc+5] * x[6];
				x[4] = b[4] - lptr[6*nc+4] * x[6] - lptr[5*nc+4] * x[5];
				x[3] = b[3] - lptr[6*nc+3] * x[6] - lptr[5*nc+3] * x[5] - lptr[4*nc+3] * x[4];
				x[2] = b[2] - lptr[6*nc+2] * x[6] - lptr[5*nc+2] * x[5] - lptr[4*nc+2] * x[4] - lptr[3*nc+2] * x[3];
				x[1] = b[1] - lptr[6*nc+1] * x[6] - lptr[5*nc+1] * x[5] - lptr[4*nc+1] * x[4] - lptr[3*nc+1] * x[3] - lptr[2*nc+1] * x[2];
				x[0] = b[0] - lptr[6*nc+0] * x[6] - lptr[5*nc+0] * x[5] - lptr[4*nc+0] * x[4] - lptr[3*nc+0] * x[3] - lptr[2*nc+0] * x[2] - lptr[1*nc+0] * x[1];
				return;
		}
		return;
	}

#if 1

	int i, j, m;
	float *xptr;
	double s0;

	// if the number of columns is not a multiple of 2 we're screwed for alignment.
	// however, if the number of columns is a multiple of 2 but the number of to be
	// processed rows is not a multiple of 2 we can still run 8 byte aligned
	m = n;
	if ( m & 1 ) {

		m--;
		x[m] = b[m];

		lptr = L.ToFloatPtr() + m * nc + m - 4;
		xptr = x + m;
		__asm {
			push		ebx
			mov			eax, m					// eax = i
			mov			esi, xptr				// esi = xptr
			mov			edi, lptr				// edi = lptr
			mov			ebx, b					// ebx = b
			mov			edx, nc					// edx = nc*sizeof(float)
			shl			edx, 2
		process4rows_1:
			movlps		xmm0, [ebx+eax*4-16]	// load b[i-2], b[i-1]
			movhps		xmm0, [ebx+eax*4-8]		// load b[i-4], b[i-3]
			xor			ecx, ecx
			sub			eax, m
			neg			eax
			jz			done4x4_1
		process4x4_1:	// process 4x4 blocks
			movlps		xmm2, [edi+0]
			movhps		xmm2, [edi+8]
			add			edi, edx
			movss		xmm1, [esi+4*ecx+0]
			shufps		xmm1, xmm1, R_SHUFFLEPS( 0, 0, 0, 0 )
			movlps		xmm3, [edi+0]
			movhps		xmm3, [edi+8]
			add			edi, edx
			mulps		xmm1, xmm2
			subps		xmm0, xmm1
			movss		xmm1, [esi+4*ecx+4]
			shufps		xmm1, xmm1, R_SHUFFLEPS( 0, 0, 0, 0 )
			movlps		xmm4, [edi+0]
			movhps		xmm4, [edi+8]
			add			edi, edx
			mulps		xmm1, xmm3
			subps		xmm0, xmm1
			movss		xmm1, [esi+4*ecx+8]
			shufps		xmm1, xmm1, R_SHUFFLEPS( 0, 0, 0, 0 )
			movlps		xmm5, [edi+0]
			movhps		xmm5, [edi+8]
			add			edi, edx
			mulps		xmm1, xmm4
			subps		xmm0, xmm1
			movss		xmm1, [esi+4*ecx+12]
			shufps		xmm1, xmm1, R_SHUFFLEPS( 0, 0, 0, 0 )
			add			ecx, 4
			cmp			ecx, eax
			mulps		xmm1, xmm5
			subps		xmm0, xmm1
			jl			process4x4_1
		done4x4_1:		// process left over of the 4 rows
			movlps		xmm2, [edi+0]
			movhps		xmm2, [edi+8]
			movss		xmm1, [esi+4*ecx]
			shufps		xmm1, xmm1, R_SHUFFLEPS( 0, 0, 0, 0 )
			mulps		xmm1, xmm2
			subps		xmm0, xmm1
			imul		ecx, edx
			sub			edi, ecx
			neg			eax

			add			eax, m
			sub			eax, 4
			movaps		xmm1, xmm0
			shufps		xmm1, xmm1, R_SHUFFLEPS( 1, 1, 1, 1 )
			movaps		xmm2, xmm0
			shufps		xmm2, xmm2, R_SHUFFLEPS( 2, 2, 2, 2 )
			movaps		xmm3, xmm0
			shufps		xmm3, xmm3, R_SHUFFLEPS( 3, 3, 3, 3 )
			sub			edi, edx
			movss		[esi-4], xmm3			// xptr[-1] = s3
			movss		xmm4, xmm3
			movss		xmm5, xmm3
			mulss		xmm3, [edi+8]			// lptr[-1*nc+2] * s3
			mulss		xmm4, [edi+4]			// lptr[-1*nc+1] * s3
			mulss		xmm5, [edi+0]			// lptr[-1*nc+0] * s3
			subss		xmm2, xmm3
			movss		[esi-8], xmm2			// xptr[-2] = s2
			movss		xmm6, xmm2
			sub			edi, edx
			subss		xmm0, xmm5
			subss		xmm1, xmm4
			mulss		xmm2, [edi+4]			// lptr[-2*nc+1] * s2
			mulss		xmm6, [edi+0]			// lptr[-2*nc+0] * s2
			subss		xmm1, xmm2
			movss		[esi-12], xmm1			// xptr[-3] = s1
			subss		xmm0, xmm6
			sub			edi, edx
			cmp			eax, 4
			mulss		xmm1, [edi+0]			// lptr[-3*nc+0] * s1
			subss		xmm0, xmm1
			movss		[esi-16], xmm0			// xptr[-4] = s0
			jl			done4rows_1
			sub			edi, edx
			sub			edi, 16
			sub			esi, 16
			jmp			process4rows_1
		done4rows_1:
			pop			ebx
		}

	} else {

		lptr = L.ToFloatPtr() + m * nc + m - 4;
		xptr = x + m;
		__asm {
			push		ebx
			mov			eax, m					// eax = i
			mov			esi, xptr				// esi = xptr
			mov			edi, lptr				// edi = lptr
			mov			ebx, b					// ebx = b
			mov			edx, nc					// edx = nc*sizeof(float)
			shl			edx, 2
		process4rows:
			movlps		xmm0, [ebx+eax*4-16]	// load b[i-2], b[i-1]
			movhps		xmm0, [ebx+eax*4-8]		// load b[i-4], b[i-3]
			sub			eax, m
			jz			done4x4
			neg			eax
			xor			ecx, ecx
		process4x4:		// process 4x4 blocks
			movlps		xmm2, [edi+0]
			movhps		xmm2, [edi+8]
			add			edi, edx
			movss		xmm1, [esi+4*ecx+0]
			shufps		xmm1, xmm1, R_SHUFFLEPS( 0, 0, 0, 0 )
			movlps		xmm3, [edi+0]
			movhps		xmm3, [edi+8]
			add			edi, edx
			mulps		xmm1, xmm2
			subps		xmm0, xmm1
			movss		xmm1, [esi+4*ecx+4]
			shufps		xmm1, xmm1, R_SHUFFLEPS( 0, 0, 0, 0 )
			movlps		xmm4, [edi+0]
			movhps		xmm4, [edi+8]
			add			edi, edx
			mulps		xmm1, xmm3
			subps		xmm0, xmm1
			movss		xmm1, [esi+4*ecx+8]
			shufps		xmm1, xmm1, R_SHUFFLEPS( 0, 0, 0, 0 )
			movlps		xmm5, [edi+0]
			movhps		xmm5, [edi+8]
			add			edi, edx
			mulps		xmm1, xmm4
			subps		xmm0, xmm1
			movss		xmm1, [esi+4*ecx+12]
			shufps		xmm1, xmm1, R_SHUFFLEPS( 0, 0, 0, 0 )
			add			ecx, 4
			cmp			ecx, eax
			mulps		xmm1, xmm5
			subps		xmm0, xmm1
			jl			process4x4
			imul		ecx, edx
			sub			edi, ecx
			neg			eax
		done4x4:		// process left over of the 4 rows
			add			eax, m
			sub			eax, 4
			movaps		xmm1, xmm0
			shufps		xmm1, xmm1, R_SHUFFLEPS( 1, 1, 1, 1 )
			movaps		xmm2, xmm0
			shufps		xmm2, xmm2, R_SHUFFLEPS( 2, 2, 2, 2 )
			movaps		xmm3, xmm0
			shufps		xmm3, xmm3, R_SHUFFLEPS( 3, 3, 3, 3 )
			sub			edi, edx
			movss		[esi-4], xmm3			// xptr[-1] = s3
			movss		xmm4, xmm3
			movss		xmm5, xmm3
			mulss		xmm3, [edi+8]			// lptr[-1*nc+2] * s3
			mulss		xmm4, [edi+4]			// lptr[-1*nc+1] * s3
			mulss		xmm5, [edi+0]			// lptr[-1*nc+0] * s3
			subss		xmm2, xmm3
			movss		[esi-8], xmm2			// xptr[-2] = s2
			movss		xmm6, xmm2
			sub			edi, edx
			subss		xmm0, xmm5
			subss		xmm1, xmm4
			mulss		xmm2, [edi+4]			// lptr[-2*nc+1] * s2
			mulss		xmm6, [edi+0]			// lptr[-2*nc+0] * s2
			subss		xmm1, xmm2
			movss		[esi-12], xmm1			// xptr[-3] = s1
			subss		xmm0, xmm6
			sub			edi, edx
			cmp			eax, 4
			mulss		xmm1, [edi+0]			// lptr[-3*nc+0] * s1
			subss		xmm0, xmm1
			movss		[esi-16], xmm0			// xptr[-4] = s0
			jl			done4rows
			sub			edi, edx
			sub			edi, 16
			sub			esi, 16
			jmp			process4rows
		done4rows:
			pop			ebx
		}
	}

	// process left over rows
	for ( i = (m&3)-1; i >= 0; i-- ) {
		s0 = b[i];
		lptr = L[0] + i;
		for ( j = i + 1; j < n; j++ ) {
			s0 -= lptr[j*nc] * x[j];
		}
		x[i] = s0;
	}

#else

	int i, j, m;
	double s0, s1, s2, s3, t;
	const float *lptr2;
	float *xptr, *xptr2;

	m = n;
	if ( m & 1 ) {

		m--;
		x[m] = b[m];

		lptr = L.ToFloatPtr() + m * nc + m - 4;
		xptr = x + m;
		// process 4 rows at a time
		for ( i = m; i >= 4; i -= 4 ) {
			s0 = b[i-4];
			s1 = b[i-3];
			s2 = b[i-2];
			s3 = b[i-1];
			// process 4x4 blocks
			xptr2 = xptr;	// x + i;
			lptr2 = lptr;	// ptr = L[i] + i - 4;
			for ( j = 0; j < m-i; j += 4 ) {
				t = xptr2[0];
				s0 -= lptr2[0] * t;
				s1 -= lptr2[1] * t;
				s2 -= lptr2[2] * t;
				s3 -= lptr2[3] * t;
				lptr2 += nc;
				xptr2++;
				t = xptr2[0];
				s0 -= lptr2[0] * t;
				s1 -= lptr2[1] * t;
				s2 -= lptr2[2] * t;
				s3 -= lptr2[3] * t;
				lptr2 += nc;
				xptr2++;
				t = xptr2[0];
				s0 -= lptr2[0] * t;
				s1 -= lptr2[1] * t;
				s2 -= lptr2[2] * t;
				s3 -= lptr2[3] * t;
				lptr2 += nc;
				xptr2++;
				t = xptr2[0];
				s0 -= lptr2[0] * t;
				s1 -= lptr2[1] * t;
				s2 -= lptr2[2] * t;
				s3 -= lptr2[3] * t;
				lptr2 += nc;
				xptr2++;
			}
			t = xptr2[0];
			s0 -= lptr2[0] * t;
			s1 -= lptr2[1] * t;
			s2 -= lptr2[2] * t;
			s3 -= lptr2[3] * t;
			// process left over of the 4 rows
			lptr -= nc;
			s0 -= lptr[0] * s3;
			s1 -= lptr[1] * s3;
			s2 -= lptr[2] * s3;
			lptr -= nc;
			s0 -= lptr[0] * s2;
			s1 -= lptr[1] * s2;
			lptr -= nc;
			s0 -= lptr[0] * s1;
			lptr -= nc;
			// store result
			xptr[-4] = s0;
			xptr[-3] = s1;
			xptr[-2] = s2;
			xptr[-1] = s3;
			// update pointers for next four rows
			lptr -= 4;
			xptr -= 4;
		}

	} else {

		lptr = L.ToFloatPtr() + m * nc + m - 4;
		xptr = x + m;
		// process 4 rows at a time
		for ( i = m; i >= 4; i -= 4 ) {
			s0 = b[i-4];
			s1 = b[i-3];
			s2 = b[i-2];
			s3 = b[i-1];
			// process 4x4 blocks
			xptr2 = xptr;	// x + i;
			lptr2 = lptr;	// ptr = L[i] + i - 4;
			for ( j = 0; j < m-i; j += 4 ) {
				t = xptr2[0];
				s0 -= lptr2[0] * t;
				s1 -= lptr2[1] * t;
				s2 -= lptr2[2] * t;
				s3 -= lptr2[3] * t;
				lptr2 += nc;
				xptr2++;
				t = xptr2[0];
				s0 -= lptr2[0] * t;
				s1 -= lptr2[1] * t;
				s2 -= lptr2[2] * t;
				s3 -= lptr2[3] * t;
				lptr2 += nc;
				xptr2++;
				t = xptr2[0];
				s0 -= lptr2[0] * t;
				s1 -= lptr2[1] * t;
				s2 -= lptr2[2] * t;
				s3 -= lptr2[3] * t;
				lptr2 += nc;
				xptr2++;
				t = xptr2[0];
				s0 -= lptr2[0] * t;
				s1 -= lptr2[1] * t;
				s2 -= lptr2[2] * t;
				s3 -= lptr2[3] * t;
				lptr2 += nc;
				xptr2++;
			}
			// process left over of the 4 rows
			lptr -= nc;
			s0 -= lptr[0] * s3;
			s1 -= lptr[1] * s3;
			s2 -= lptr[2] * s3;
			lptr -= nc;
			s0 -= lptr[0] * s2;
			s1 -= lptr[1] * s2;
			lptr -= nc;
			s0 -= lptr[0] * s1;
			lptr -= nc;
			// store result
			xptr[-4] = s0;
			xptr[-3] = s1;
			xptr[-2] = s2;
			xptr[-1] = s3;
			// update pointers for next four rows
			lptr -= 4;
			xptr -= 4;
		}
	}
	// process left over rows
	for ( i--; i >= 0; i-- ) {
		s0 = b[i];
		lptr = L[0] + i;
		for ( j = i + 1; j < m; j++ ) {
			s0 -= lptr[j*nc] * x[j];
		}
		x[i] = s0;
	}

#endif
}

/*
============
idSIMD_SSE::MatX_LDLTFactor

  in-place factorization LDL' of the n * n sub-matrix of mat
  the reciprocal of the diagonal elements are stored in invDiag
  currently assumes the number of columns of mat is a multiple of 4
============
*/
bool VPCALL idSIMD_SSE::MatX_LDLTFactor( idMatX &mat, idVecX &invDiag, const int n ) {
#if 1

	int j, nc;
	float *v, *diag, *invDiagPtr, *mptr;
	double s0, s1, s2, sum, d;

	v = (float *) _alloca16( n * sizeof( float ) );
	diag = (float *) _alloca16( n * sizeof( float ) );
	invDiagPtr = invDiag.ToFloatPtr();

	nc = mat.GetNumColumns();

	assert( ( nc & 3 ) == 0 );

	if ( n <= 0 ) {
		return true;
	}

	mptr = mat[0];

	sum = mptr[0];

	if ( sum == 0.0f ) {
		return false;
	}

	diag[0] = sum;
	invDiagPtr[0] = d = 1.0f / sum;

	if ( n <= 1 ) {
		return true;
	}

	mptr = mat[0];
	for ( j = 1; j < n; j++ ) {
		mptr[j*nc+0] = ( mptr[j*nc+0] ) * d;
	}

	mptr = mat[1];

	v[0] = diag[0] * mptr[0]; s0 = v[0] * mptr[0];
	sum = mptr[1] - s0;

	if ( sum == 0.0f ) {
		return false;
	}

	mat[1][1] = sum;
	diag[1] = sum;
	invDiagPtr[1] = d = 1.0f / sum;

	if ( n <= 2 ) {
		return true;
	}

	mptr = mat[0];
	for ( j = 2; j < n; j++ ) {
		mptr[j*nc+1] = ( mptr[j*nc+1] - v[0] * mptr[j*nc+0] ) * d;
	}

	mptr = mat[2];

	v[0] = diag[0] * mptr[0]; s0 = v[0] * mptr[0];
	v[1] = diag[1] * mptr[1]; s1 = v[1] * mptr[1];
	sum = mptr[2] - s0 - s1;

	if ( sum == 0.0f ) {
		return false;
	}

	mat[2][2] = sum;
	diag[2] = sum;
	invDiagPtr[2] = d = 1.0f / sum;

	if ( n <= 3 ) {
		return true;
	}

	mptr = mat[0];
	for ( j = 3; j < n; j++ ) {
		mptr[j*nc+2] = ( mptr[j*nc+2] - v[0] * mptr[j*nc+0] - v[1] * mptr[j*nc+1] ) * d;
	}

	mptr = mat[3];

	v[0] = diag[0] * mptr[0]; s0 = v[0] * mptr[0];
	v[1] = diag[1] * mptr[1]; s1 = v[1] * mptr[1];
	v[2] = diag[2] * mptr[2]; s2 = v[2] * mptr[2];
	sum = mptr[3] - s0 - s1 - s2;

	if ( sum == 0.0f ) {
		return false;
	}

	mat[3][3] = sum;
	diag[3] = sum;
	invDiagPtr[3] = d = 1.0f / sum;

	if ( n <= 4 ) {
		return true;
	}

	mptr = mat[0];
	for ( j = 4; j < n; j++ ) {
		mptr[j*nc+3] = ( mptr[j*nc+3] - v[0] * mptr[j*nc+0] - v[1] * mptr[j*nc+1] - v[2] * mptr[j*nc+2] ) * d;
	}

	int ncf = nc * sizeof( float );
	mptr = mat[0];

	__asm {
		xorps		xmm2, xmm2
		xorps		xmm3, xmm3
		xorps		xmm4, xmm4

		push		ebx
		mov			ebx, 4

	loopRow:
			cmp			ebx, n
			jge			done

			mov			ecx, ebx				// esi = i
			shl			ecx, 2					// esi = i * 4
			mov			edx, diag				// edx = diag
			add			edx, ecx				// edx = &diag[i]
			mov			edi, ebx				// edi = i
			imul		edi, ncf				// edi = i * nc * sizeof( float )
			add			edi, mptr				// edi = mat[i]
			add			edi, ecx				// edi = &mat[i][i]
			mov			esi, v					// ecx = v
			add			esi, ecx				// ecx = &v[i]
			mov			eax, invDiagPtr			// eax = invDiagPtr
			add			eax, ecx				// eax = &invDiagPtr[i]
			neg			ecx

			movaps		xmm0, [edx+ecx]
			mulps		xmm0, [edi+ecx]
			movaps		[esi+ecx], xmm0
			mulps		xmm0, [edi+ecx]
			add			ecx, 12*4
			jg			doneDot8
		dot8:
			movaps		xmm1, [edx+ecx-(8*4)]
			mulps		xmm1, [edi+ecx-(8*4)]
			movaps		[esi+ecx-(8*4)], xmm1
			mulps		xmm1, [edi+ecx-(8*4)]
			addps		xmm0, xmm1
			movaps		xmm2, [edx+ecx-(4*4)]
			mulps		xmm2, [edi+ecx-(4*4)]
			movaps		[esi+ecx-(4*4)], xmm2
			mulps		xmm2, [edi+ecx-(4*4)]
			addps		xmm0, xmm2
			add			ecx, 8*4
			jle			dot8
		doneDot8:
			sub			ecx, 4*4
			jg			doneDot4
			movaps		xmm1, [edx+ecx-(4*4)]
			mulps		xmm1, [edi+ecx-(4*4)]
			movaps		[esi+ecx-(4*4)], xmm1
			mulps		xmm1, [edi+ecx-(4*4)]
			addps		xmm0, xmm1
			add			ecx, 4*4
		doneDot4:
			sub			ecx, 2*4
			jg			doneDot2
			movlps		xmm3, [edx+ecx-(2*4)]
			movlps		xmm4, [edi+ecx-(2*4)]
			mulps		xmm3, xmm4
			movlps		[esi+ecx-(2*4)], xmm3
			mulps		xmm3, xmm4
			addps		xmm0, xmm3
			add			ecx, 2*4
		doneDot2:
			sub			ecx, 1*4
			jg			doneDot1
			movss		xmm3, [edx+ecx-(1*4)]
			movss		xmm4, [edi+ecx-(1*4)]
			mulss		xmm3, xmm4
			movss		[esi+ecx-(1*4)], xmm3
			mulss		xmm3, xmm4
			addss		xmm0, xmm3
		doneDot1:
			movhlps		xmm2, xmm0
			addps		xmm0, xmm2
			movaps		xmm2, xmm0
			shufps		xmm2, xmm2, R_SHUFFLEPS( 1, 0, 0, 0 )
			addss		xmm0, xmm2
			movss		xmm1, [edi]
			subss		xmm1, xmm0
			movss		[edi], xmm1				// mptr[i] = sum;
			movss		[edx], xmm1				// diag[i] = sum;

			// if ( sum == 0.0f ) return false;
			movaps		xmm2, xmm1
			cmpeqss		xmm2, SIMD_SP_zero
			andps		xmm2, SIMD_SP_tiny
			orps		xmm1, xmm2

			rcpss		xmm7, xmm1
			mulss		xmm1, xmm7
			mulss		xmm1, xmm7
			addss		xmm7, xmm7
			subss		xmm7, xmm1
			movss		[eax], xmm7				// invDiagPtr[i] = 1.0f / sum;

			mov			edx, n					// edx = n
			sub			edx, ebx				// edx = n - i
			dec			edx						// edx = n - i - 1
			jle			doneSubRow				// if ( i + 1 >= n ) return true;

			mov			eax, ebx				// eax = i
			shl			eax, 2					// eax = i * 4
			neg			eax

		loopSubRow:
				add			edi, ncf
				mov			ecx, eax
				movaps		xmm0, [esi+ecx]
				mulps		xmm0, [edi+ecx]
				add			ecx, 12*4
				jg			doneSubDot8
			subDot8:
				movaps		xmm1, [esi+ecx-(8*4)]
				mulps		xmm1, [edi+ecx-(8*4)]
				addps		xmm0, xmm1
				movaps		xmm2, [esi+ecx-(4*4)]
				mulps		xmm2, [edi+ecx-(4*4)]
				addps		xmm0, xmm2
				add			ecx, 8*4
				jle			subDot8
			doneSubDot8:
				sub			ecx, 4*4
				jg			doneSubDot4
				movaps		xmm1, [esi+ecx-(4*4)]
				mulps		xmm1, [edi+ecx-(4*4)]
				addps		xmm0, xmm1
				add			ecx, 4*4
			doneSubDot4:
				sub			ecx, 2*4
				jg			doneSubDot2
				movlps		xmm3, [esi+ecx-(2*4)]
				movlps		xmm4, [edi+ecx-(2*4)]
				mulps		xmm3, xmm4
				addps		xmm0, xmm3
				add			ecx, 2*4
			doneSubDot2:
				sub			ecx, 1*4
				jg			doneSubDot1
				movss		xmm3, [esi+ecx-(1*4)]
				movss		xmm4, [edi+ecx-(1*4)]
				mulss		xmm3, xmm4
				addss		xmm0, xmm3
			doneSubDot1:
				movhlps		xmm2, xmm0
				addps		xmm0, xmm2
				movaps		xmm2, xmm0
				shufps		xmm2, xmm2, R_SHUFFLEPS( 1, 0, 0, 0 )
				addss		xmm0, xmm2
				movss		xmm1, [edi]
				subss		xmm1, xmm0
				mulss		xmm1, xmm7
				movss		[edi], xmm1
				dec			edx
				jg			loopSubRow
		doneSubRow:
			inc		ebx
			jmp		loopRow
	done:
		pop		ebx
	}

	return true;

#else

	int i, j, k, nc;
	float *v, *diag, *mptr;
	double s0, s1, s2, s3, sum, d;

	v = (float *) _alloca16( n * sizeof( float ) );
	diag = (float *) _alloca16( n * sizeof( float ) );

	nc = mat.GetNumColumns();

	if ( n <= 0 ) {
		return true;
	}

	mptr = mat[0];

	sum = mptr[0];

	if ( sum == 0.0f ) {
		return false;
	}

	diag[0] = sum;
	invDiag[0] = d = 1.0f / sum;

	if ( n <= 1 ) {
		return true;
	}

	mptr = mat[0];
	for ( j = 1; j < n; j++ ) {
		mptr[j*nc+0] = ( mptr[j*nc+0] ) * d;
	}

	mptr = mat[1];

	v[0] = diag[0] * mptr[0]; s0 = v[0] * mptr[0];
	sum = mptr[1] - s0;

	if ( sum == 0.0f ) {
		return false;
	}

	mat[1][1] = sum;
	diag[1] = sum;
	invDiag[1] = d = 1.0f / sum;

	if ( n <= 2 ) {
		return true;
	}

	mptr = mat[0];
	for ( j = 2; j < n; j++ ) {
		mptr[j*nc+1] = ( mptr[j*nc+1] - v[0] * mptr[j*nc+0] ) * d;
	}

	mptr = mat[2];

	v[0] = diag[0] * mptr[0]; s0 = v[0] * mptr[0];
	v[1] = diag[1] * mptr[1]; s1 = v[1] * mptr[1];
	sum = mptr[2] - s0 - s1;

	if ( sum == 0.0f ) {
		return false;
	}

	mat[2][2] = sum;
	diag[2] = sum;
	invDiag[2] = d = 1.0f / sum;

	if ( n <= 3 ) {
		return true;
	}

	mptr = mat[0];
	for ( j = 3; j < n; j++ ) {
		mptr[j*nc+2] = ( mptr[j*nc+2] - v[0] * mptr[j*nc+0] - v[1] * mptr[j*nc+1] ) * d;
	}

	mptr = mat[3];

	v[0] = diag[0] * mptr[0]; s0 = v[0] * mptr[0];
	v[1] = diag[1] * mptr[1]; s1 = v[1] * mptr[1];
	v[2] = diag[2] * mptr[2]; s2 = v[2] * mptr[2];
	sum = mptr[3] - s0 - s1 - s2;

	if ( sum == 0.0f ) {
		return false;
	}

	mat[3][3] = sum;
	diag[3] = sum;
	invDiag[3] = d = 1.0f / sum;

	if ( n <= 4 ) {
		return true;
	}

	mptr = mat[0];
	for ( j = 4; j < n; j++ ) {
		mptr[j*nc+3] = ( mptr[j*nc+3] - v[0] * mptr[j*nc+0] - v[1] * mptr[j*nc+1] - v[2] * mptr[j*nc+2] ) * d;
	}

	for ( i = 4; i < n; i++ ) {

		mptr = mat[i];

		v[0] = diag[0] * mptr[0]; s0 = v[0] * mptr[0];
		v[1] = diag[1] * mptr[1]; s1 = v[1] * mptr[1];
		v[2] = diag[2] * mptr[2]; s2 = v[2] * mptr[2];
		v[3] = diag[3] * mptr[3]; s3 = v[3] * mptr[3];
		for ( k = 4; k < i-3; k += 4 ) {
			v[k+0] = diag[k+0] * mptr[k+0]; s0 += v[k+0] * mptr[k+0];
			v[k+1] = diag[k+1] * mptr[k+1]; s1 += v[k+1] * mptr[k+1];
			v[k+2] = diag[k+2] * mptr[k+2]; s2 += v[k+2] * mptr[k+2];
			v[k+3] = diag[k+3] * mptr[k+3]; s3 += v[k+3] * mptr[k+3];
		}
		switch( i - k ) {
			case 3: v[k+2] = diag[k+2] * mptr[k+2]; s0 += v[k+2] * mptr[k+2];
			case 2: v[k+1] = diag[k+1] * mptr[k+1]; s1 += v[k+1] * mptr[k+1];
			case 1: v[k+0] = diag[k+0] * mptr[k+0]; s2 += v[k+0] * mptr[k+0];
		}
		sum = s3;
		sum += s2;
		sum += s1;
		sum += s0;
		sum = mptr[i] - sum;

		if ( sum == 0.0f ) {
			return false;
		}

		mat[i][i] = sum;
		diag[i] = sum;
		invDiag[i] = d = 1.0f / sum;

		if ( i + 1 >= n ) {
			return true;
		}

		mptr = mat[i+1];
		for ( j = i+1; j < n; j++ ) {
			s0 = mptr[0] * v[0];
			s1 = mptr[1] * v[1];
			s2 = mptr[2] * v[2];
			s3 = mptr[3] * v[3];
			for ( k = 4; k < i-7; k += 8 ) {
				s0 += mptr[k+0] * v[k+0];
				s1 += mptr[k+1] * v[k+1];
				s2 += mptr[k+2] * v[k+2];
				s3 += mptr[k+3] * v[k+3];
				s0 += mptr[k+4] * v[k+4];
				s1 += mptr[k+5] * v[k+5];
				s2 += mptr[k+6] * v[k+6];
				s3 += mptr[k+7] * v[k+7];
			}
			switch( i - k ) {
				case 7: s0 += mptr[k+6] * v[k+6];
				case 6: s1 += mptr[k+5] * v[k+5];
				case 5: s2 += mptr[k+4] * v[k+4];
				case 4: s3 += mptr[k+3] * v[k+3];
				case 3: s0 += mptr[k+2] * v[k+2];
				case 2: s1 += mptr[k+1] * v[k+1];
				case 1: s2 += mptr[k+0] * v[k+0];
			}
			sum = s3;
			sum += s2;
			sum += s1;
			sum += s0;
			mptr[i] = ( mptr[i] - sum ) * d;
			mptr += nc;
		}
	}

	return true;

#endif
}

/*
============
SSE_UpSample11kHzMonoPCMTo44kHz
============
*/
static void SSE_UpSample11kHzMonoPCMTo44kHz( float *dest, const short *src, const int numSamples ) {
	__asm {
		mov			esi, src
		mov			edi, dest

		mov			eax, numSamples
		and			eax, ~1
		jz			done2
		shl			eax, 1
		add			esi, eax
		neg			eax

		align		16
	loop2:
		add			edi, 2*4*4

		movsx		ecx, word ptr [esi+eax+0]
		cvtsi2ss	xmm0, ecx
		shufps		xmm0, xmm0, R_SHUFFLEPS( 0, 0, 0, 0 )
		movlps		[edi-2*4*4+0], xmm0
		movhps		[edi-2*4*4+8], xmm0

		movsx		edx, word ptr [esi+eax+2]
		cvtsi2ss	xmm1, edx
		shufps		xmm1, xmm1, R_SHUFFLEPS( 0, 0, 0, 0 )
		movlps		[edi-1*4*4+0], xmm1
		movhps		[edi-1*4*4+8], xmm1

		add			eax, 2*2
		jl			loop2

	done2:
		mov			eax, numSamples
		and			eax, 1
		jz			done

		movsx		ecx, word ptr [esi]
		cvtsi2ss	xmm0, ecx
		shufps		xmm0, xmm0, R_SHUFFLEPS( 0, 0, 0, 0 )
		movlps		[edi+0], xmm0
		movhps		[edi+8], xmm0

	done:
	}
}

/*
============
SSE_UpSample11kHzStereoPCMTo44kHz
============
*/
static void SSE_UpSample11kHzStereoPCMTo44kHz( float *dest, const short *src, const int numSamples ) {
	__asm {
		mov			esi, src
		mov			edi, dest

		mov			eax, numSamples
		test		eax, ~1
		jz			done2
		shl			eax, 1
		add			esi, eax
		neg			eax

		align		16
	loop2:
		add			edi, 8*4

		movsx		ecx, word ptr [esi+eax+0]
		cvtsi2ss	xmm0, ecx

		movsx		edx, word ptr [esi+eax+2]
		cvtsi2ss	xmm1, edx

		unpcklps	xmm0, xmm1

		movlps		[edi-8*4+0], xmm0
		movlps		[edi-8*4+8], xmm0
		movlps		[edi-4*4+0], xmm0
		movlps		[edi-4*4+8], xmm0

		add			eax, 2*2
		jl			loop2

	done2:
	}
}

/*
============
SSE_UpSample22kHzMonoPCMTo44kHz
============
*/
static void SSE_UpSample22kHzMonoPCMTo44kHz( float *dest, const short *src, const int numSamples ) {
	__asm {
		mov			esi, src
		mov			edi, dest

		mov			eax, numSamples
		and			eax, ~1
		jz			done2
		shl			eax, 1
		add			esi, eax
		neg			eax

		align		16
	loop2:
		add			edi, 4*4

		movsx		ecx, word ptr [esi+eax+0]
		cvtsi2ss	xmm0, ecx

		movsx		edx, word ptr [esi+eax+2]
		cvtsi2ss	xmm1, edx

		shufps		xmm0, xmm1, R_SHUFFLEPS( 0, 0, 0, 0 )
		movlps		[edi-4*4+0], xmm0
		movhps		[edi-4*4+8], xmm0

		add			eax, 2*2
		jl			loop2

	done2:
		mov			eax, numSamples
		and			eax, 1
		jz			done

		movsx		ecx, word ptr [esi]
		cvtsi2ss	xmm0, ecx
		shufps		xmm0, xmm0, R_SHUFFLEPS( 0, 0, 0, 0 )
		movlps		[edi], xmm0

	done:
	}
}

/*
============
SSE_UpSample22kHzStereoPCMTo44kHz
============
*/
static void SSE_UpSample22kHzStereoPCMTo44kHz( float *dest, const short *src, const int numSamples ) {
	__asm {
		mov			esi, src
		mov			edi, dest

		mov			eax, numSamples
		test		eax, ~1
		jz			done2
		shl			eax, 1
		add			esi, eax
		neg			eax

		align		16
	loop2:
		add			edi, 4*4

		movsx		ecx, word ptr [esi+eax+0]
		cvtsi2ss	xmm0, ecx
		movss		[edi-4*4], xmm0
		movss		[edi-2*4], xmm0

		movsx		edx, word ptr [esi+eax+2]
		cvtsi2ss	xmm1, edx
		movss		[edi-3*4], xmm1
		movss		[edi-1*4], xmm1

		add			eax, 2*2
		jl			loop2

	done2:
	}
}

/*
============
SSE_UpSample44kHzMonoPCMTo44kHz
============
*/
static void SSE_UpSample44kHzMonoPCMTo44kHz( float *dest, const short *src, const int numSamples ) {
	__asm {
		mov			esi, src
		mov			edi, dest

		mov			eax, numSamples
		and			eax, ~1
		jz			done2
		shl			eax, 1
		add			esi, eax
		neg			eax

		align		16
	loop2:
		add			edi, 2*4

		movsx		ecx, word ptr [esi+eax+0]
		cvtsi2ss	xmm0, ecx
		movss		[edi-2*4], xmm0

		movsx		edx, word ptr [esi+eax+2]
		cvtsi2ss	xmm1, edx
		movss		[edi-1*4], xmm1

		add			eax, 2*2
		jl			loop2

	done2:
		mov			eax, numSamples
		and			eax, 1
		jz			done

		movsx		ecx, word ptr [esi]
		cvtsi2ss	xmm0, ecx
		movss		[edi], xmm0

	done:
	}
}

/*
============
idSIMD_SSE::UpSamplePCMTo44kHz

  Duplicate samples for 44kHz output.
============
*/
void idSIMD_SSE::UpSamplePCMTo44kHz( float *dest, const short *src, const int numSamples, const int kHz, const int numChannels ) {
	if ( kHz == 11025 ) {
		if ( numChannels == 1 ) {
			SSE_UpSample11kHzMonoPCMTo44kHz( dest, src, numSamples );
		} else {
			SSE_UpSample11kHzStereoPCMTo44kHz( dest, src, numSamples );
		}
	} else if ( kHz == 22050 ) {
		if ( numChannels == 1 ) {
			SSE_UpSample22kHzMonoPCMTo44kHz( dest, src, numSamples );
		} else {
			SSE_UpSample22kHzStereoPCMTo44kHz( dest, src, numSamples );
		}
	} else if ( kHz == 44100 ) {
		SSE_UpSample44kHzMonoPCMTo44kHz( dest, src, numSamples );
	} else {
		assert( 0 );
	}
}

/*
============
SSE_UpSample11kHzMonoOGGTo44kHz
============
*/
static void SSE_UpSample11kHzMonoOGGTo44kHz( float *dest, const float *src, const int numSamples ) {
	float constant = 32768.0f;
	__asm {
		mov			esi, src
		mov			edi, dest
		movss		xmm7, constant
		shufps		xmm7, xmm7, R_SHUFFLEPS( 0, 0, 0, 0 )

		mov			eax, numSamples
		and			eax, ~1
		jz			done2
		shl			eax, 2
		add			esi, eax
		neg			eax

		align		16
	loop2:
		add			edi, 2*16

		movss		xmm0, [esi+eax+0]
		mulss		xmm0, xmm7
		shufps		xmm0, xmm0, R_SHUFFLEPS( 0, 0, 0, 0 )
		movlps		[edi-32], xmm0
		movlps		[edi-24], xmm0

		movss		xmm1, [esi+eax+4]
		mulss		xmm1, xmm7
		shufps		xmm1, xmm1, R_SHUFFLEPS( 0, 0, 0, 0 )
		movlps		[edi-16], xmm1
		movlps		[edi- 8], xmm1

		add			eax, 2*4
		jl			loop2

	done2:
		mov			eax, numSamples
		and			eax, 1
		jz			done

		movss		xmm0, [esi]
		mulss		xmm0, xmm7
		shufps		xmm0, xmm0, R_SHUFFLEPS( 0, 0, 0, 0 )
		movlps		[edi+0], xmm0
		movlps		[edi+8], xmm0

	done:
	}
}

/*
============
SSE_UpSample11kHzStereoOGGTo44kHz
============
*/
static void SSE_UpSample11kHzStereoOGGTo44kHz( float *dest, const float * const *src, const int numSamples ) {
	float constant = 32768.0f;
	__asm {
		mov			esi, src
		mov			ecx, [esi+0]
		mov			edx, [esi+4]
		mov			edi, dest
		movss		xmm7, constant
		shufps		xmm7, xmm7, R_SHUFFLEPS( 0, 0, 0, 0 )

		mov			eax, numSamples
		and			eax, ~1
		jz			done2
		shl			eax, 1
		add			ecx, eax
		add			edx, eax
		neg			eax

		align		16
	loop2:
		add			edi, 4*16

		movlps		xmm0, [ecx+eax]
		movlps		xmm1, [edx+eax]
		unpcklps	xmm0, xmm1
		mulps		xmm0, xmm7
		movlps		[edi-8*8], xmm0
		movlps		[edi-7*8], xmm0
		movlps		[edi-6*8], xmm0
		movlps		[edi-5*8], xmm0
		movhps		[edi-4*8], xmm0
		movhps		[edi-3*8], xmm0
		movhps		[edi-2*8], xmm0
		movhps		[edi-1*8], xmm0

		add			eax, 2*4
		jl			loop2

	done2:
		mov			eax, numSamples
		and			eax, 1
		jz			done

		movss		xmm0, [ecx]
		movss		xmm1, [edx]
		unpcklps	xmm0, xmm1
		mulps		xmm0, xmm7
		movlps		[edi+0*8], xmm0
		movlps		[edi+1*8], xmm0
		movlps		[edi+2*8], xmm0
		movlps		[edi+3*8], xmm0

	done:
	}
}

/*
============
SSE_UpSample22kHzMonoOGGTo44kHz
============
*/
static void SSE_UpSample22kHzMonoOGGTo44kHz( float *dest, const float *src, const int numSamples ) {
	float constant = 32768.0f;
	__asm {
		mov			esi, src
		mov			edi, dest
		movss		xmm7, constant
		shufps		xmm7, xmm7, R_SHUFFLEPS( 0, 0, 0, 0 )

		mov			eax, numSamples
		and			eax, ~1
		jz			done2
		shl			eax, 2
		add			esi, eax
		neg			eax

		align		16
	loop2:
		add			edi, 2*8

		movss		xmm0, [esi+eax+0]
		movss		xmm1, [esi+eax+4]
		shufps		xmm0, xmm1, R_SHUFFLEPS( 0, 0, 0, 0 )
		mulps		xmm0, xmm7
		movlps		[edi-16], xmm0
		movhps		[edi- 8], xmm0

		add			eax, 2*4
		jl			loop2

	done2:
		mov			eax, numSamples
		and			eax, 1
		jz			done

		movss		xmm0, [esi]
		mulss		xmm0, xmm7
		shufps		xmm0, xmm0, R_SHUFFLEPS( 0, 0, 0, 0 )
		movlps		[edi+0], xmm0

	done:
	}
}

/*
============
SSE_UpSample22kHzStereoOGGTo44kHz
============
*/
static void SSE_UpSample22kHzStereoOGGTo44kHz( float *dest, const float * const *src, const int numSamples ) {
	float constant = 32768.0f;
	__asm {
		mov			esi, src
		mov			ecx, [esi+0]
		mov			edx, [esi+4]
		mov			edi, dest
		movss		xmm7, constant
		shufps		xmm7, xmm7, R_SHUFFLEPS( 0, 0, 0, 0 )

		mov			eax, numSamples
		and			eax, ~1
		jz			done2
		shl			eax, 1
		add			ecx, eax
		add			edx, eax
		neg			eax

		align		16
	loop2:
		add			edi, 2*16

		movlps		xmm0, [ecx+eax]
		movlps		xmm1, [edx+eax]
		unpcklps	xmm0, xmm1
		mulps		xmm0, xmm7
		movlps		[edi-4*8], xmm0
		movlps		[edi-3*8], xmm0
		movhps		[edi-2*8], xmm0
		movhps		[edi-1*8], xmm0

		add			eax, 2*4
		jl			loop2

	done2:
		mov			eax, numSamples
		and			eax, 1
		jz			done

		movss		xmm0, [ecx]
		movss		xmm1, [edx]
		unpcklps	xmm0, xmm1
		mulps		xmm0, xmm7
		movlps		[edi+0*8], xmm0
		movlps		[edi+1*8], xmm0

	done:
	}
}

/*
============
SSE_UpSample44kHzMonoOGGTo44kHz
============
*/
static void SSE_UpSample44kHzMonoOGGTo44kHz( float *dest, const float *src, const int numSamples ) {
	float constant = 32768.0f;
	KFLOAT_CA( mul, dest, src, constant, numSamples )
}

/*
============
SSE_UpSample44kHzStereoOGGTo44kHz
============
*/
static void SSE_UpSample44kHzStereoOGGTo44kHz( float *dest, const float * const *src, const int numSamples ) {
	float constant = 32768.0f;
	__asm {
		mov			esi, src
		mov			ecx, [esi+0]
		mov			edx, [esi+4]
		mov			edi, dest
		movss		xmm7, constant
		shufps		xmm7, xmm7, R_SHUFFLEPS( 0, 0, 0, 0 )

		mov			eax, numSamples
		and			eax, ~1
		jz			done2
		shl			eax, 1
		add			ecx, eax
		add			edx, eax
		neg			eax

		align		16
	loop2:
		add			edi, 16

		movlps		xmm0, [ecx+eax]
		movlps		xmm1, [edx+eax]
		unpcklps	xmm0, xmm1
		mulps		xmm0, xmm7
		movlps		[edi-2*8], xmm0
		movhps		[edi-1*8], xmm0

		add			eax, 2*4
		jl			loop2

	done2:
		mov			eax, numSamples
		and			eax, 1
		jz			done

		movss		xmm0, [ecx]
		movss		xmm1, [edx]
		unpcklps	xmm0, xmm1
		mulps		xmm0, xmm7
		movlps		[edi+0*8], xmm0

	done:
	}
}

/*
============
idSIMD_SSE::UpSampleOGGTo44kHz

  Duplicate samples for 44kHz output.
============
*/
void idSIMD_SSE::UpSampleOGGTo44kHz( float *dest, const float * const *ogg, const int numSamples, const int kHz, const int numChannels ) {
	if ( kHz == 11025 ) {
		if ( numChannels == 1 ) {
			SSE_UpSample11kHzMonoOGGTo44kHz( dest, ogg[0], numSamples );
		} else {
			SSE_UpSample11kHzStereoOGGTo44kHz( dest, ogg, numSamples );
		}
	} else if ( kHz == 22050 ) {
		if ( numChannels == 1 ) {
			SSE_UpSample22kHzMonoOGGTo44kHz( dest, ogg[0], numSamples );
		} else {
			SSE_UpSample22kHzStereoOGGTo44kHz( dest, ogg, numSamples );
		}
	} else if ( kHz == 44100 ) {
		if ( numChannels == 1 ) {
			SSE_UpSample44kHzMonoOGGTo44kHz( dest, ogg[0], numSamples );
		} else {
			SSE_UpSample44kHzStereoOGGTo44kHz( dest, ogg, numSamples );
		}
	} else {
		assert( 0 );
	}
}

/*
============
idSIMD_SSE::MixSoundTwoSpeakerMono
============
*/
void VPCALL idSIMD_SSE::MixSoundTwoSpeakerMono( float *mixBuffer, const float *samples, const int numSamples, const float lastV[2], const float currentV[2] ) {
#if 1

	ALIGN16( float incs[2] );

	assert( numSamples == MIXBUFFER_SAMPLES );

	incs[0] = ( currentV[0] - lastV[0] ) / MIXBUFFER_SAMPLES;
	incs[1] = ( currentV[1] - lastV[1] ) / MIXBUFFER_SAMPLES;

	__asm {
		mov			eax, MIXBUFFER_SAMPLES
		mov			edi, mixBuffer
		mov			esi, samples
		shl			eax, 2
		add			esi, eax
		neg			eax

		mov			ecx, lastV
		movlps		xmm6, [ecx]
		xorps		xmm7, xmm7
		movhps		xmm7, incs
		shufps		xmm6, xmm6, R_SHUFFLEPS( 0, 1, 0, 1 )
		addps		xmm6, xmm7
		shufps		xmm7, xmm7, R_SHUFFLEPS( 2, 3, 2, 3 )
		addps		xmm7, xmm7

	loop16:
		add			edi, 4*4*4

		movaps		xmm0, [esi+eax+0*4*4]
		movaps		xmm1, xmm0
		shufps		xmm0, xmm0, R_SHUFFLEPS( 0, 0, 1, 1 )
		mulps		xmm0, xmm6
		addps		xmm0, [edi-4*4*4]
		addps		xmm6, xmm7
		movaps		[edi-4*4*4], xmm0

		shufps		xmm1, xmm1, R_SHUFFLEPS( 2, 2, 3, 3 )
		mulps		xmm1, xmm6
		addps		xmm1, [edi-3*4*4]
		addps		xmm6, xmm7
		movaps		[edi-3*4*4], xmm1

		movaps		xmm2, [esi+eax+1*4*4]
		movaps		xmm3, xmm2
		shufps		xmm2, xmm2, R_SHUFFLEPS( 0, 0, 1, 1 )
		mulps		xmm2, xmm6
		addps		xmm2, [edi-2*4*4]
		addps		xmm6, xmm7
		movaps		[edi-2*4*4], xmm2

		shufps		xmm3, xmm3, R_SHUFFLEPS( 2, 2, 3, 3 )
		mulps		xmm3, xmm6
		addps		xmm3, [edi-1*4*4]
		addps		xmm6, xmm7
		movaps		[edi-1*4*4], xmm3

		add			eax, 2*4*4

		jl			loop16
	}

#else

	int i;
	float incL;
	float incR;
	float sL0, sL1;
	float sR0, sR1;

	assert( numSamples == MIXBUFFER_SAMPLES );

	incL = ( currentV[0] - lastV[0] ) / MIXBUFFER_SAMPLES;
	incR = ( currentV[1] - lastV[1] ) / MIXBUFFER_SAMPLES;

	sL0 = lastV[0];
	sR0 = lastV[1];
	sL1 = lastV[0] + incL;
	sR1 = lastV[1] + incR;

	incL *= 2;
	incR *= 2;

	for( i = 0; i < MIXBUFFER_SAMPLES; i += 2 ) {
		mixBuffer[i*2+0] += samples[i+0] * sL0;
		mixBuffer[i*2+1] += samples[i+0] * sR0;
		mixBuffer[i*2+2] += samples[i+1] * sL1;
		mixBuffer[i*2+3] += samples[i+1] * sR1;
		sL0 += incL;
		sR0 += incR;
		sL1 += incL;
		sR1 += incR;
	}

#endif
}

/*
============
idSIMD_SSE::MixSoundTwoSpeakerStereo
============
*/
void VPCALL idSIMD_SSE::MixSoundTwoSpeakerStereo( float *mixBuffer, const float *samples, const int numSamples, const float lastV[2], const float currentV[2] ) {
#if 1

	ALIGN16( float incs[2] );

	assert( numSamples == MIXBUFFER_SAMPLES );

	incs[0] = ( currentV[0] - lastV[0] ) / MIXBUFFER_SAMPLES;
	incs[1] = ( currentV[1] - lastV[1] ) / MIXBUFFER_SAMPLES;

	__asm {
		mov			eax, MIXBUFFER_SAMPLES
		mov			edi, mixBuffer
		mov			esi, samples
		shl			eax, 3
		add			esi, eax
		neg			eax

		mov			ecx, lastV
		movlps		xmm6, [ecx]
		xorps		xmm7, xmm7
		movhps		xmm7, incs
		shufps		xmm6, xmm6, R_SHUFFLEPS( 0, 1, 0, 1 )
		addps		xmm6, xmm7
		shufps		xmm7, xmm7, R_SHUFFLEPS( 2, 3, 2, 3 )
		addps		xmm7, xmm7

	loop16:
		add			edi, 4*4*4

		movaps		xmm0, [esi+eax+0*4*4]
		mulps		xmm0, xmm6
		addps		xmm0, [edi-4*4*4]
		addps		xmm6, xmm7
		movaps		[edi-4*4*4], xmm0

		movaps		xmm2, [esi+eax+1*4*4]
		mulps		xmm2, xmm6
		addps		xmm2, [edi-3*4*4]
		addps		xmm6, xmm7
		movaps		[edi-3*4*4], xmm2

		movaps		xmm3, [esi+eax+2*4*4]
		mulps		xmm3, xmm6
		addps		xmm3, [edi-2*4*4]
		addps		xmm6, xmm7
		movaps		[edi-2*4*4], xmm3

		movaps		xmm4, [esi+eax+3*4*4]
		mulps		xmm4, xmm6
		addps		xmm4, [edi-1*4*4]
		addps		xmm6, xmm7
		movaps		[edi-1*4*4], xmm4

		add			eax, 4*4*4

		jl			loop16
	}

#else

	int i;
	float incL;
	float incR;
	float sL0, sL1;
	float sR0, sR1;

	assert( numSamples == MIXBUFFER_SAMPLES );

	incL = ( currentV[0] - lastV[0] ) / MIXBUFFER_SAMPLES;
	incR = ( currentV[1] - lastV[1] ) / MIXBUFFER_SAMPLES;

	sL0 = lastV[0];
	sR0 = lastV[1];
	sL1 = lastV[0] + incL;
	sR1 = lastV[1] + incR;

	incL *= 2;
	incR *= 2;

	for( i = 0; i < MIXBUFFER_SAMPLES; i += 2 ) {
		mixBuffer[i*2+0] += samples[i*2+0] * sL0;
		mixBuffer[i*2+1] += samples[i*2+1] * sR0;
		mixBuffer[i*2+2] += samples[i*2+2] * sL1;
		mixBuffer[i*2+3] += samples[i*2+3] * sR1;
		sL0 += incL;
		sR0 += incR;
		sL1 += incL;
		sR1 += incR;
	}

#endif
}

/*
============
idSIMD_SSE::MixSoundSixSpeakerMono
============
*/
void VPCALL idSIMD_SSE::MixSoundSixSpeakerMono( float *mixBuffer, const float *samples, const int numSamples, const float lastV[6], const float currentV[6] ) {
#if 1

	ALIGN16( float incs[6] );

	assert( numSamples == MIXBUFFER_SAMPLES );

	incs[0] = ( currentV[0] - lastV[0] ) / MIXBUFFER_SAMPLES;
	incs[1] = ( currentV[1] - lastV[1] ) / MIXBUFFER_SAMPLES;
	incs[2] = ( currentV[2] - lastV[2] ) / MIXBUFFER_SAMPLES;
	incs[3] = ( currentV[3] - lastV[3] ) / MIXBUFFER_SAMPLES;
	incs[4] = ( currentV[4] - lastV[4] ) / MIXBUFFER_SAMPLES;
	incs[5] = ( currentV[5] - lastV[5] ) / MIXBUFFER_SAMPLES;

	__asm {
		mov			eax, MIXBUFFER_SAMPLES
		mov			edi, mixBuffer
		mov			esi, samples
		shl			eax, 2
		add			esi, eax
		neg			eax

		mov			ecx, lastV
		movlps		xmm2, [ecx+ 0]
		movhps		xmm2, [ecx+ 8]
		movlps		xmm3, [ecx+16]
		movaps		xmm4, xmm2
		shufps		xmm3, xmm2, R_SHUFFLEPS( 0, 1, 0, 1 )
		shufps		xmm4, xmm3, R_SHUFFLEPS( 2, 3, 0, 1 )

		xorps		xmm5, xmm5
		movhps		xmm5, incs
		movlps		xmm7, incs+8
		movhps		xmm7, incs+16
		addps		xmm3, xmm5
		addps		xmm4, xmm7
		shufps		xmm5, xmm7, R_SHUFFLEPS( 2, 3, 0, 1 )
		movaps		xmm6, xmm7
		shufps		xmm6, xmm5, R_SHUFFLEPS( 2, 3, 0, 1 )
		addps		xmm5, xmm5
		addps		xmm6, xmm6
		addps		xmm7, xmm7

	loop24:
		add			edi, 6*16

		movaps		xmm0, [esi+eax]

		movaps		xmm1, xmm0
		shufps		xmm1, xmm1, R_SHUFFLEPS( 0, 0, 0, 0 )
		mulps		xmm1, xmm2
		addps		xmm1, [edi-6*16]
		addps		xmm2, xmm5
		movaps		[edi-6*16], xmm1

		movaps		xmm1, xmm0
		shufps		xmm1, xmm1, R_SHUFFLEPS( 0, 0, 1, 1 )
		mulps		xmm1, xmm3
		addps		xmm1, [edi-5*16]
		addps		xmm3, xmm6
		movaps		[edi-5*16], xmm1

		movaps		xmm1, xmm0
		shufps		xmm1, xmm1, R_SHUFFLEPS( 1, 1, 1, 1 )
		mulps		xmm1, xmm4
		addps		xmm1, [edi-4*16]
		addps		xmm4, xmm7
		movaps		[edi-4*16], xmm1

		movaps		xmm1, xmm0
		shufps		xmm1, xmm1, R_SHUFFLEPS( 2, 2, 2, 2 )
		mulps		xmm1, xmm2
		addps		xmm1, [edi-3*16]
		addps		xmm2, xmm5
		movaps		[edi-3*16], xmm1

		movaps		xmm1, xmm0
		shufps		xmm1, xmm1, R_SHUFFLEPS( 2, 2, 3, 3 )
		mulps		xmm1, xmm3
		addps		xmm1, [edi-2*16]
		addps		xmm3, xmm6
		movaps		[edi-2*16], xmm1

		shufps		xmm0, xmm0, R_SHUFFLEPS( 3, 3, 3, 3 )
		mulps		xmm0, xmm4
		addps		xmm0, [edi-1*16]
		addps		xmm4, xmm7
		movaps		[edi-1*16], xmm0

		add			eax, 4*4

		jl			loop24
	}

#else

	int i;
	float sL0, sL1, sL2, sL3, sL4, sL5, sL6, sL7, sL8, sL9, sL10, sL11;
	float incL0, incL1, incL2, incL3, incL4, incL5;

	assert( numSamples == MIXBUFFER_SAMPLES );

	incL0 = ( currentV[0] - lastV[0] ) / MIXBUFFER_SAMPLES;
	incL1 = ( currentV[1] - lastV[1] ) / MIXBUFFER_SAMPLES;
	incL2 = ( currentV[2] - lastV[2] ) / MIXBUFFER_SAMPLES;
	incL3 = ( currentV[3] - lastV[3] ) / MIXBUFFER_SAMPLES;
	incL4 = ( currentV[4] - lastV[4] ) / MIXBUFFER_SAMPLES;
	incL5 = ( currentV[5] - lastV[5] ) / MIXBUFFER_SAMPLES;

	sL0  = lastV[0];
	sL1  = lastV[1];
	sL2  = lastV[2];
	sL3  = lastV[3];
	sL4  = lastV[4];
	sL5  = lastV[5];

	sL6  = lastV[0] + incL0;
	sL7  = lastV[1] + incL1;
	sL8  = lastV[2] + incL2;
	sL9  = lastV[3] + incL3;
	sL10 = lastV[4] + incL4;
	sL11 = lastV[5] + incL5;

	incL0 *= 2;
	incL1 *= 2;
	incL2 *= 2;
	incL3 *= 2;
	incL4 *= 2;
	incL5 *= 2;

	for( i = 0; i <= MIXBUFFER_SAMPLES - 2; i += 2 ) {
		mixBuffer[i*6+ 0] += samples[i+0] * sL0;
		mixBuffer[i*6+ 1] += samples[i+0] * sL1;
		mixBuffer[i*6+ 2] += samples[i+0] * sL2;
		mixBuffer[i*6+ 3] += samples[i+0] * sL3;

		mixBuffer[i*6+ 4] += samples[i+0] * sL4;
		mixBuffer[i*6+ 5] += samples[i+0] * sL5;
		mixBuffer[i*6+ 6] += samples[i+1] * sL6;
		mixBuffer[i*6+ 7] += samples[i+1] * sL7;

		mixBuffer[i*6+ 8] += samples[i+1] * sL8;
		mixBuffer[i*6+ 9] += samples[i+1] * sL9;
		mixBuffer[i*6+10] += samples[i+1] * sL10;
		mixBuffer[i*6+11] += samples[i+1] * sL11;

		sL0  += incL0;
		sL1  += incL1;
		sL2  += incL2;
		sL3  += incL3;

		sL4  += incL4;
		sL5  += incL5;
		sL6  += incL0;
		sL7  += incL1;

		sL8  += incL2;
		sL9  += incL3;
		sL10 += incL4;
		sL11 += incL5;
	}

#endif
}

/*
============
idSIMD_SSE::MixSoundSixSpeakerStereo
============
*/
void VPCALL idSIMD_SSE::MixSoundSixSpeakerStereo( float *mixBuffer, const float *samples, const int numSamples, const float lastV[6], const float currentV[6] ) {
#if 1

	ALIGN16( float incs[6] );

	assert( numSamples == MIXBUFFER_SAMPLES );
	assert( SPEAKER_RIGHT == 1 );
	assert( SPEAKER_BACKRIGHT == 5 );

	incs[0] = ( currentV[0] - lastV[0] ) / MIXBUFFER_SAMPLES;
	incs[1] = ( currentV[1] - lastV[1] ) / MIXBUFFER_SAMPLES;
	incs[2] = ( currentV[2] - lastV[2] ) / MIXBUFFER_SAMPLES;
	incs[3] = ( currentV[3] - lastV[3] ) / MIXBUFFER_SAMPLES;
	incs[4] = ( currentV[4] - lastV[4] ) / MIXBUFFER_SAMPLES;
	incs[5] = ( currentV[5] - lastV[5] ) / MIXBUFFER_SAMPLES;

	__asm {
		mov			eax, MIXBUFFER_SAMPLES
		mov			edi, mixBuffer
		mov			esi, samples
		shl			eax, 3
		add			esi, eax
		neg			eax

		mov			ecx, lastV
		movlps		xmm2, [ecx+ 0]
		movhps		xmm2, [ecx+ 8]
		movlps		xmm3, [ecx+16]
		movaps		xmm4, xmm2
		shufps		xmm3, xmm2, R_SHUFFLEPS( 0, 1, 0, 1 )
		shufps		xmm4, xmm3, R_SHUFFLEPS( 2, 3, 0, 1 )

		xorps		xmm5, xmm5
		movhps		xmm5, incs
		movlps		xmm7, incs+ 8
		movhps		xmm7, incs+16
		addps		xmm3, xmm5
		addps		xmm4, xmm7
		shufps		xmm5, xmm7, R_SHUFFLEPS( 2, 3, 0, 1 )
		movaps		xmm6, xmm7
		shufps		xmm6, xmm5, R_SHUFFLEPS( 2, 3, 0, 1 )
		addps		xmm5, xmm5
		addps		xmm6, xmm6
		addps		xmm7, xmm7

	loop12:
		add			edi, 3*16

		movaps		xmm0, [esi+eax+0]

		movaps		xmm1, xmm0
		shufps		xmm1, xmm1, R_SHUFFLEPS( 0, 1, 0, 0 )
		mulps		xmm1, xmm2
		addps		xmm1, [edi-3*16]
		addps		xmm2, xmm5
		movaps		[edi-3*16], xmm1

		movaps		xmm1, xmm0
		shufps		xmm1, xmm1, R_SHUFFLEPS( 0, 1, 2, 3 )
		mulps		xmm1, xmm3
		addps		xmm1, [edi-2*16]
		addps		xmm3, xmm6
		movaps		[edi-2*16], xmm1

		add			eax, 4*4

		shufps		xmm0, xmm0, R_SHUFFLEPS( 2, 2, 2, 3 )
		mulps		xmm0, xmm4
		addps		xmm0, [edi-1*16]
		addps		xmm4, xmm7
		movaps		[edi-1*16], xmm0

		jl			loop12

		emms
	}

#else

	int i;
	float sL0, sL1, sL2, sL3, sL4, sL5, sL6, sL7, sL8, sL9, sL10, sL11;
	float incL0, incL1, incL2, incL3, incL4, incL5;

	assert( numSamples == MIXBUFFER_SAMPLES );
	assert( SPEAKER_RIGHT == 1 );
	assert( SPEAKER_BACKRIGHT == 5 );

	incL0 = ( currentV[0] - lastV[0] ) / MIXBUFFER_SAMPLES;
	incL1 = ( currentV[1] - lastV[1] ) / MIXBUFFER_SAMPLES;
	incL2 = ( currentV[2] - lastV[2] ) / MIXBUFFER_SAMPLES;
	incL3 = ( currentV[3] - lastV[3] ) / MIXBUFFER_SAMPLES;
	incL4 = ( currentV[4] - lastV[4] ) / MIXBUFFER_SAMPLES;
	incL5 = ( currentV[5] - lastV[5] ) / MIXBUFFER_SAMPLES;

	sL0  = lastV[0];
	sL1  = lastV[1];
	sL2  = lastV[2];
	sL3  = lastV[3];
	sL4  = lastV[4];
	sL5  = lastV[5];

	sL6  = lastV[0] + incL0;
	sL7  = lastV[1] + incL1;
	sL8  = lastV[2] + incL2;
	sL9  = lastV[3] + incL3;
	sL10 = lastV[4] + incL4;
	sL11 = lastV[5] + incL5;

	incL0 *= 2;
	incL1 *= 2;
	incL2 *= 2;
	incL3 *= 2;
	incL4 *= 2;
	incL5 *= 2;

	for( i = 0; i <= MIXBUFFER_SAMPLES - 2; i += 2 ) {
		mixBuffer[i*6+ 0] += samples[i*2+0+0] * sL0;
		mixBuffer[i*6+ 1] += samples[i*2+0+1] * sL1;
		mixBuffer[i*6+ 2] += samples[i*2+0+0] * sL2;
		mixBuffer[i*6+ 3] += samples[i*2+0+0] * sL3;

		mixBuffer[i*6+ 4] += samples[i*2+0+0] * sL4;
		mixBuffer[i*6+ 5] += samples[i*2+0+1] * sL5;
		mixBuffer[i*6+ 6] += samples[i*2+2+0] * sL6;
		mixBuffer[i*6+ 7] += samples[i*2+2+1] * sL7;

		mixBuffer[i*6+ 8] += samples[i*2+2+0] * sL8;
		mixBuffer[i*6+ 9] += samples[i*2+2+0] * sL9;
		mixBuffer[i*6+10] += samples[i*2+2+0] * sL10;
		mixBuffer[i*6+11] += samples[i*2+2+1] * sL11;

		sL0  += incL0;
		sL1  += incL1;
		sL2  += incL2;
		sL3  += incL3;

		sL4  += incL4;
		sL5  += incL5;
		sL6  += incL0;
		sL7  += incL1;

		sL8  += incL2;
		sL9  += incL3;
		sL10 += incL4;
		sL11 += incL5;
	}

#endif
}

/*
============
idSIMD_SSE::MixedSoundToSamples
============
*/
void VPCALL idSIMD_SSE::MixedSoundToSamples( short *samples, const float *mixBuffer, const int numSamples ) {
#if 1

	assert( ( numSamples % MIXBUFFER_SAMPLES ) == 0 );

	__asm {

		mov			eax, numSamples
		mov			edi, mixBuffer
		mov			esi, samples
		shl			eax, 2
		add			edi, eax
		neg			eax

	loop16:

		movaps		xmm0, [edi+eax+0*16]
		movaps		xmm2, [edi+eax+1*16]
		movaps		xmm4, [edi+eax+2*16]
		movaps		xmm6, [edi+eax+3*16]

		add			esi, 4*4*2

		movhlps		xmm1, xmm0
		movhlps		xmm3, xmm2
		movhlps		xmm5, xmm4
		movhlps		xmm7, xmm6

		prefetchnta	[edi+eax+64]

		cvtps2pi	mm0, xmm0
		cvtps2pi	mm2, xmm2
		cvtps2pi	mm4, xmm4
		cvtps2pi	mm6, xmm6

		prefetchnta	[edi+eax+128]

		cvtps2pi	mm1, xmm1
		cvtps2pi	mm3, xmm3
		cvtps2pi	mm5, xmm5
		cvtps2pi	mm7, xmm7

		add			eax, 4*16

		packssdw	mm0, mm1
		packssdw	mm2, mm3
		packssdw	mm4, mm5
		packssdw	mm6, mm7

		movq		[esi-4*4*2], mm0
		movq		[esi-3*4*2], mm2
		movq		[esi-2*4*2], mm4
		movq		[esi-1*4*2], mm6

		jl			loop16

		emms
	}

#else

	for ( int i = 0; i < numSamples; i++ ) {
		if ( mixBuffer[i] <= -32768.0f ) {
			samples[i] = -32768;
		} else if ( mixBuffer[i] >= 32767.0f ) {
			samples[i] = 32767;
		} else {
			samples[i] = (short) mixBuffer[i];
		}
	}

#endif
}

#endif /* _MSC_VER */
