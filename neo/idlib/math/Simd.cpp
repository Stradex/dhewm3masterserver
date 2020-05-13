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

#if MACOS_X
#include <stdlib.h>
#include <unistd.h>			// this is for sleep()
#include <sys/time.h>
#include <sys/resource.h>
#include <mach/mach_time.h>
#endif

#include "sys/platform.h"
#include "idlib/math/Simd_Generic.h"
#include "idlib/math/Simd_MMX.h"
#include "idlib/math/Simd_3DNow.h"
#include "idlib/math/Simd_SSE.h"
#include "idlib/math/Simd_SSE2.h"
#include "idlib/math/Simd_SSE3.h"
#include "idlib/math/Simd_AltiVec.h"
#include "idlib/math/Plane.h"
#include "idlib/Lib.h"
#include "framework/Common.h"

#include "idlib/math/Simd.h"

idSIMDProcessor	*	processor = NULL;			// pointer to SIMD processor
idSIMDProcessor *	generic = NULL;				// pointer to generic SIMD implementation
idSIMDProcessor *	SIMDProcessor = NULL;

/*
================
idSIMD::Init
================
*/
void idSIMD::Init( void ) {
	generic = new idSIMD_Generic;
	generic->cpuid = CPUID_GENERIC;
	processor = NULL;
	SIMDProcessor = generic;
}

/*
============
idSIMD::InitProcessor
============
*/
void idSIMD::InitProcessor( const char *module, bool forceGeneric ) {
	int cpuid;
	idSIMDProcessor *newProcessor;

	cpuid = idLib::sys->GetProcessorId();

	if ( forceGeneric ) {

		newProcessor = generic;

	} else {

		if ( !processor ) {
			if ( ( cpuid & CPUID_ALTIVEC ) ) {
				processor = new idSIMD_AltiVec;
			} else if ( ( cpuid & CPUID_MMX ) && ( cpuid & CPUID_SSE ) && ( cpuid & CPUID_SSE2 ) && ( cpuid & CPUID_SSE3 ) ) {
				processor = new idSIMD_SSE3;
			} else if ( ( cpuid & CPUID_MMX ) && ( cpuid & CPUID_SSE ) && ( cpuid & CPUID_SSE2 ) ) {
				processor = new idSIMD_SSE2;
			} else if ( ( cpuid & CPUID_MMX ) && ( cpuid & CPUID_SSE ) ) {
				processor = new idSIMD_SSE;
			} else if ( ( cpuid & CPUID_MMX ) && ( cpuid & CPUID_3DNOW ) ) {
				processor = new idSIMD_3DNow;
			} else if ( ( cpuid & CPUID_MMX ) ) {
				processor = new idSIMD_MMX;
			} else {
				processor = generic;
			}
			processor->cpuid = cpuid;
		}

		newProcessor = processor;
	}

	if ( newProcessor != SIMDProcessor ) {
		SIMDProcessor = newProcessor;
		idLib::common->Printf( "%s using %s for SIMD processing\n", module, SIMDProcessor->GetName() );
	}

	if ( cpuid & CPUID_SSE ) {
		idLib::sys->FPU_SetFTZ( true );
		idLib::sys->FPU_SetDAZ( true );
	}
}

/*
================
idSIMD::Shutdown
================
*/
void idSIMD::Shutdown( void ) {
	if ( processor != generic ) {
		delete processor;
	}
	delete generic;
	generic = NULL;
	processor = NULL;
	SIMDProcessor = NULL;
}


//===============================================================
//
// Test code
//
//===============================================================

#define COUNT		1024		// data count
#define NUMTESTS	2048		// number of tests

#define RANDOM_SEED		1013904223L	//((int)idLib::sys->GetClockTicks())

idSIMDProcessor *p_simd;
idSIMDProcessor *p_generic;
int baseClocks = 0;

#if defined(_MSC_VER) && defined(_M_IX86)

#define TIME_TYPE int

#pragma warning(disable : 4731)     // frame pointer register 'ebx' modified by inline assembly code

int saved_ebx = 0;

#define StartRecordTime( start )			\
	__asm mov saved_ebx, ebx				\
	__asm xor eax, eax						\
	__asm cpuid								\
	__asm rdtsc								\
	__asm mov start, eax					\
	__asm xor eax, eax						\
	__asm cpuid

#define StopRecordTime( end )				\
	__asm xor eax, eax						\
	__asm cpuid								\
	__asm rdtsc								\
	__asm mov end, eax						\
	__asm mov ebx, saved_ebx				\
	__asm xor eax, eax						\
	__asm cpuid

#elif MACOS_X

double ticksPerNanosecond;

#define TIME_TYPE uint64_t

#define StartRecordTime( start )			\
	start = mach_absolute_time();

#define StopRecordTime( end )				\
	end = mach_absolute_time();

#else

#define TIME_TYPE int

#define StartRecordTime( start )			\
	start = 0;

#define StopRecordTime( end )				\
	end = 1;

#endif

#define GetBest( start, end, best )			\
	if ( !best || end - start < best ) {	\
		best = end - start;					\
	}


/*
============
PrintClocks
============
*/
void PrintClocks( const char *string, int dataCount, int clocks, int otherClocks = 0 ) {
	int i;

	idLib::common->Printf( string );
	for ( i = idStr::LengthWithoutColors(string); i < 48; i++ ) {
		idLib::common->Printf(" ");
	}
	clocks -= baseClocks;
	if ( otherClocks && clocks ) {
		otherClocks -= baseClocks;
		int p = (int) ( (float) ( otherClocks - clocks ) * 100.0f / (float) otherClocks );
		idLib::common->Printf( "c = %4d, clcks = %5d, %d%%\n", dataCount, clocks, p );
	} else {
		idLib::common->Printf( "c = %4d, clcks = %5d\n", dataCount, clocks );
	}
}

/*
============
GetBaseClocks
============
*/
void GetBaseClocks( void ) {
	int i, start, end, bestClocks;

	bestClocks = 0;
	for ( i = 0; i < NUMTESTS; i++ ) {
		StartRecordTime( start );
		StopRecordTime( end );
		GetBest( start, end, bestClocks );
	}
	baseClocks = bestClocks;
}

/*
============
TestAdd
============
*/
void TestAdd( void ) {
	int i;
	TIME_TYPE start, end, bestClocksGeneric, bestClocksSIMD;
	ALIGN16( float fdst0[COUNT] );
	ALIGN16( float fdst1[COUNT] );
	ALIGN16( float fsrc0[COUNT] );
	ALIGN16( float fsrc1[COUNT] );
	const char *result;

	idRandom srnd( RANDOM_SEED );

	for ( i = 0; i < COUNT; i++ ) {
		fsrc0[i] = srnd.CRandomFloat() * 10.0f;
		fsrc1[i] = srnd.CRandomFloat() * 10.0f;
	}

	idLib::common->Printf("====================================\n" );

	bestClocksGeneric = 0;
	for ( i = 0; i < NUMTESTS; i++ ) {
		StartRecordTime( start );
		p_generic->Add( fdst0, 4.0f, fsrc1, COUNT );
		StopRecordTime( end );
		GetBest( start, end, bestClocksGeneric );
	}
	PrintClocks( "generic->Add( float + float[] )", COUNT, bestClocksGeneric );

	bestClocksSIMD = 0;
	for ( i = 0; i < NUMTESTS; i++ ) {
		StartRecordTime( start );
		p_simd->Add( fdst1, 4.0f, fsrc1, COUNT );
		StopRecordTime( end );
		GetBest( start, end, bestClocksSIMD );
	}

	for ( i = 0; i < COUNT; i++ ) {
		if ( idMath::Fabs( fdst0[i] - fdst1[i] ) > 1e-5f ) {
			break;
		}
	}
	result = ( i >= COUNT ) ? "ok" :  S_COLOR_RED "X";
	PrintClocks( va( "   simd->Add( float + float[] ) %s", result ), COUNT, bestClocksSIMD, bestClocksGeneric );

	bestClocksGeneric = 0;
	for ( i = 0; i < NUMTESTS; i++ ) {
		StartRecordTime( start );
		p_generic->Add( fdst0, fsrc0, fsrc1, COUNT );
		StopRecordTime( end );
		GetBest( start, end, bestClocksGeneric );
	}
	PrintClocks( "generic->Add( float[] + float[] )", COUNT, bestClocksGeneric );

	bestClocksSIMD = 0;
	for ( i = 0; i < NUMTESTS; i++ ) {
		StartRecordTime( start );
		p_simd->Add( fdst1, fsrc0, fsrc1, COUNT );
		StopRecordTime( end );
		GetBest( start, end, bestClocksSIMD );
	}

	for ( i = 0; i < COUNT; i++ ) {
		if ( idMath::Fabs( fdst0[i] - fdst1[i] ) > 1e-5f ) {
			break;
		}
	}
	result = ( i >= COUNT ) ? "ok" :  S_COLOR_RED "X";
	PrintClocks( va( "   simd->Add( float[] + float[] ) %s", result ), COUNT, bestClocksSIMD, bestClocksGeneric );
}

/*
============
TestSub
============
*/
void TestSub( void ) {
	int i;
	TIME_TYPE start, end, bestClocksGeneric, bestClocksSIMD;
	ALIGN16( float fdst0[COUNT] );
	ALIGN16( float fdst1[COUNT] );
	ALIGN16( float fsrc0[COUNT] );
	ALIGN16( float fsrc1[COUNT] );
	const char *result;

	idRandom srnd( RANDOM_SEED );

	for ( i = 0; i < COUNT; i++ ) {
		fsrc0[i] = srnd.CRandomFloat() * 10.0f;
		fsrc1[i] = srnd.CRandomFloat() * 10.0f;
	}

	idLib::common->Printf("====================================\n" );

	bestClocksGeneric = 0;
	for ( i = 0; i < NUMTESTS; i++ ) {
		StartRecordTime( start );
		p_generic->Sub( fdst0, 4.0f, fsrc1, COUNT );
		StopRecordTime( end );
		GetBest( start, end, bestClocksGeneric );
	}
	PrintClocks( "generic->Sub( float + float[] )", COUNT, bestClocksGeneric );

	bestClocksSIMD = 0;
	for ( i = 0; i < NUMTESTS; i++ ) {
		StartRecordTime( start );
		p_simd->Sub( fdst1, 4.0f, fsrc1, COUNT );
		StopRecordTime( end );
		GetBest( start, end, bestClocksSIMD );
	}

	for ( i = 0; i < COUNT; i++ ) {
		if ( idMath::Fabs( fdst0[i] - fdst1[i] ) > 1e-5f ) {
			break;
		}
	}
	result = ( i >= COUNT ) ? "ok" :  S_COLOR_RED "X";
	PrintClocks( va( "   simd->Sub( float + float[] ) %s", result ), COUNT, bestClocksSIMD, bestClocksGeneric );

	bestClocksGeneric = 0;
	for ( i = 0; i < NUMTESTS; i++ ) {
		StartRecordTime( start );
		p_generic->Sub( fdst0, fsrc0, fsrc1, COUNT );
		StopRecordTime( end );
		GetBest( start, end, bestClocksGeneric );
	}
	PrintClocks( "generic->Sub( float[] + float[] )", COUNT, bestClocksGeneric );

	bestClocksSIMD = 0;
	for ( i = 0; i < NUMTESTS; i++ ) {
		StartRecordTime( start );
		p_simd->Sub( fdst1, fsrc0, fsrc1, COUNT );
		StopRecordTime( end );
		GetBest( start, end, bestClocksSIMD );
	}

	for ( i = 0; i < COUNT; i++ ) {
		if ( idMath::Fabs( fdst0[i] - fdst1[i] ) > 1e-5f ) {
			break;
		}
	}
	result = ( i >= COUNT ) ? "ok" :  S_COLOR_RED "X";
	PrintClocks( va( "   simd->Sub( float[] + float[] ) %s", result ), COUNT, bestClocksSIMD, bestClocksGeneric );
}

/*
============
TestMul
============
*/
void TestMul( void ) {
	int i;
	TIME_TYPE start, end, bestClocksGeneric, bestClocksSIMD;
	ALIGN16( float fdst0[COUNT] );
	ALIGN16( float fdst1[COUNT] );
	ALIGN16( float fsrc0[COUNT] );
	ALIGN16( float fsrc1[COUNT] );
	const char *result;

	idRandom srnd( RANDOM_SEED );

	for ( i = 0; i < COUNT; i++ ) {
		fsrc0[i] = srnd.CRandomFloat() * 10.0f;
		fsrc1[i] = srnd.CRandomFloat() * 10.0f;
	}

	idLib::common->Printf("====================================\n" );

	bestClocksGeneric = 0;
	for ( i = 0; i < NUMTESTS; i++ ) {
		StartRecordTime( start );
		p_generic->Mul( fdst0, 4.0f, fsrc1, COUNT );
		StopRecordTime( end );
		GetBest( start, end, bestClocksGeneric );
	}
	PrintClocks( "generic->Mul( float * float[] )", COUNT, bestClocksGeneric );

	bestClocksSIMD = 0;
	for ( i = 0; i < NUMTESTS; i++ ) {
		StartRecordTime( start );
		p_simd->Mul( fdst1, 4.0f, fsrc1, COUNT );
		StopRecordTime( end );
		GetBest( start, end, bestClocksSIMD );
	}

	for ( i = 0; i < COUNT; i++ ) {
		if ( idMath::Fabs( fdst0[i] - fdst1[i] ) > 1e-5f ) {
			break;
		}
	}
	result = ( i >= COUNT ) ? "ok" :  S_COLOR_RED "X";
	PrintClocks( va( "   simd->Mul( float * float[] ) %s", result ), COUNT, bestClocksSIMD, bestClocksGeneric );


	bestClocksGeneric = 0;
	for ( i = 0; i < NUMTESTS; i++ ) {
		StartRecordTime( start );
		p_generic->Mul( fdst0, fsrc0, fsrc1, COUNT );
		StopRecordTime( end );
		GetBest( start, end, bestClocksGeneric );
	}
	PrintClocks( "generic->Mul( float[] * float[] )", COUNT, bestClocksGeneric );

	bestClocksSIMD = 0;
	for ( i = 0; i < NUMTESTS; i++ ) {
		StartRecordTime( start );
		p_simd->Mul( fdst1, fsrc0, fsrc1, COUNT );
		StopRecordTime( end );
		GetBest( start, end, bestClocksSIMD );
	}

	for ( i = 0; i < COUNT; i++ ) {
		if ( idMath::Fabs( fdst0[i] - fdst1[i] ) > 1e-5f ) {
			break;
		}
	}
	result = ( i >= COUNT ) ? "ok" :  S_COLOR_RED "X";
	PrintClocks( va( "   simd->Mul( float[] * float[] ) %s", result ), COUNT, bestClocksSIMD, bestClocksGeneric );
}

/*
============
TestDiv
============
*/
void TestDiv( void ) {
	int i;
	TIME_TYPE start, end, bestClocksGeneric, bestClocksSIMD;
	ALIGN16( float fdst0[COUNT] );
	ALIGN16( float fdst1[COUNT] );
	ALIGN16( float fsrc0[COUNT] );
	ALIGN16( float fsrc1[COUNT] );
	const char *result;

	idRandom srnd( RANDOM_SEED );

	for ( i = 0; i < COUNT; i++ ) {
		fsrc0[i] = srnd.CRandomFloat() * 10.0f;
		do {
			fsrc1[i] = srnd.CRandomFloat() * 10.0f;
		} while( idMath::Fabs( fsrc1[i] ) < 0.1f );
	}

	idLib::common->Printf("====================================\n" );


	bestClocksGeneric = 0;
	for ( i = 0; i < NUMTESTS; i++ ) {
		StartRecordTime( start );
		p_generic->Div( fdst0, 4.0f, fsrc1, COUNT );
		StopRecordTime( end );
		GetBest( start, end, bestClocksGeneric );
	}
	PrintClocks( "generic->Div( float * float[] )", COUNT, bestClocksGeneric );

	bestClocksSIMD = 0;
	for ( i = 0; i < NUMTESTS; i++ ) {
		StartRecordTime( start );
		p_simd->Div( fdst1, 4.0f, fsrc1, COUNT );
		StopRecordTime( end );
		GetBest( start, end, bestClocksSIMD );
	}

	for ( i = 0; i < COUNT; i++ ) {
		if ( idMath::Fabs( fdst0[i] - fdst1[i] ) > 1e-5f ) {
			break;
		}
	}
	result = ( i >= COUNT ) ? "ok" :  S_COLOR_RED "X";
	PrintClocks( va( "   simd->Div( float * float[] ) %s", result ), COUNT, bestClocksSIMD, bestClocksGeneric );


	bestClocksGeneric = 0;
	for ( i = 0; i < NUMTESTS; i++ ) {
		StartRecordTime( start );
		p_generic->Div( fdst0, fsrc0, fsrc1, COUNT );
		StopRecordTime( end );
		GetBest( start, end, bestClocksGeneric );
	}
	PrintClocks( "generic->Div( float[] * float[] )", COUNT, bestClocksGeneric );

	bestClocksSIMD = 0;
	for ( i = 0; i < NUMTESTS; i++ ) {
		StartRecordTime( start );
		p_simd->Div( fdst1, fsrc0, fsrc1, COUNT );
		StopRecordTime( end );
		GetBest( start, end, bestClocksSIMD );
	}

	for ( i = 0; i < COUNT; i++ ) {
		if ( idMath::Fabs( fdst0[i] - fdst1[i] ) > 1e-3f ) {
			break;
		}
	}
	result = ( i >= COUNT ) ? "ok" :  S_COLOR_RED "X";
	PrintClocks( va( "   simd->Div( float[] * float[] ) %s", result ), COUNT, bestClocksSIMD, bestClocksGeneric );
}

/*
============
TestMulAdd
============
*/
void TestMulAdd( void ) {
	int i, j;
	TIME_TYPE start, end, bestClocksGeneric, bestClocksSIMD;
	ALIGN16( float fdst0[COUNT] );
	ALIGN16( float fdst1[COUNT] );
	ALIGN16( float fsrc0[COUNT] );
	const char *result;

	idRandom srnd( RANDOM_SEED );

	for ( i = 0; i < COUNT; i++ ) {
		fsrc0[i] = srnd.CRandomFloat() * 10.0f;
	}

	idLib::common->Printf("====================================\n" );

	for ( j = 0; j < 50 && j < COUNT; j++ ) {

		bestClocksGeneric = 0;
		for ( i = 0; i < NUMTESTS; i++ ) {
			for ( int k = 0; k < COUNT; k++ ) {
				fdst0[k] = k;
			}
			StartRecordTime( start );
			p_generic->MulAdd( fdst0, 0.123f, fsrc0, j );
			StopRecordTime( end );
			GetBest( start, end, bestClocksGeneric );
		}
		PrintClocks( va( "generic->MulAdd( float * float[%2d] )", j ), 1, bestClocksGeneric );

		bestClocksSIMD = 0;
		for ( i = 0; i < NUMTESTS; i++ ) {
			for ( int k = 0; k < COUNT; k++ ) {
				fdst1[k] = k;
			}
			StartRecordTime( start );
			p_simd->MulAdd( fdst1, 0.123f, fsrc0, j );
			StopRecordTime( end );
			GetBest( start, end, bestClocksSIMD );
		}

		for ( i = 0; i < COUNT; i++ ) {
			if ( idMath::Fabs( fdst0[i] - fdst1[i] ) > 1e-5f ) {
				break;
			}
		}
		result = ( i >= COUNT ) ? "ok" :  S_COLOR_RED "X";
		PrintClocks( va( "   simd->MulAdd( float * float[%2d] ) %s", j, result ), 1, bestClocksSIMD, bestClocksGeneric );
	}
}

/*
============
TestMulSub
============
*/
void TestMulSub( void ) {
	int i, j;
	TIME_TYPE start, end, bestClocksGeneric, bestClocksSIMD;
	ALIGN16( float fdst0[COUNT] );
	ALIGN16( float fdst1[COUNT] );
	ALIGN16( float fsrc0[COUNT] );
	const char *result;

	idRandom srnd( RANDOM_SEED );

	for ( i = 0; i < COUNT; i++ ) {
		fsrc0[i] = srnd.CRandomFloat() * 10.0f;
	}

	idLib::common->Printf("====================================\n" );

	for ( j = 0; j < 50 && j < COUNT; j++ ) {

		bestClocksGeneric = 0;
		for ( i = 0; i < NUMTESTS; i++ ) {
			for ( int k = 0; k < COUNT; k++ ) {
				fdst0[k] = k;
			}
			StartRecordTime( start );
			p_generic->MulSub( fdst0, 0.123f, fsrc0, j );
			StopRecordTime( end );
			GetBest( start, end, bestClocksGeneric );
		}
		PrintClocks( va( "generic->MulSub( float * float[%2d] )", j ), 1, bestClocksGeneric );

		bestClocksSIMD = 0;
		for ( i = 0; i < NUMTESTS; i++ ) {
			for ( int k = 0; k < COUNT; k++ ) {
				fdst1[k] = k;
			}
			StartRecordTime( start );
			p_simd->MulSub( fdst1, 0.123f, fsrc0, j );
			StopRecordTime( end );
			GetBest( start, end, bestClocksSIMD );
		}

		for ( i = 0; i < COUNT; i++ ) {
			if ( idMath::Fabs( fdst0[i] - fdst1[i] ) > 1e-5f ) {
				break;
			}
		}
		result = ( i >= COUNT ) ? "ok" :  S_COLOR_RED "X";
		PrintClocks( va( "   simd->MulSub( float * float[%2d] ) %s", j, result ), 1, bestClocksSIMD, bestClocksGeneric );
	}
}


/*
============
TestCompare
============
*/
void TestCompare( void ) {
	int i;
	TIME_TYPE start, end, bestClocksGeneric, bestClocksSIMD;
	ALIGN16( float fsrc0[COUNT] );
	ALIGN16( byte bytedst[COUNT] );
	ALIGN16( byte bytedst2[COUNT] );
	const char *result;

	idRandom srnd( RANDOM_SEED );

	for ( i = 0; i < COUNT; i++ ) {
		fsrc0[i] = srnd.CRandomFloat() * 10.0f;
	}

	idLib::common->Printf("====================================\n" );

	bestClocksGeneric = 0;
	for ( i = 0; i < NUMTESTS; i++ ) {
		StartRecordTime( start );
		p_generic->CmpGT( bytedst, fsrc0, 0.0f, COUNT );
		StopRecordTime( end );
		GetBest( start, end, bestClocksGeneric );
	}
	PrintClocks( "generic->CmpGT( float[] >= float )", COUNT, bestClocksGeneric );

	bestClocksSIMD = 0;
	for ( i = 0; i < NUMTESTS; i++ ) {
		StartRecordTime( start );
		p_simd->CmpGT( bytedst2, fsrc0, 0.0f, COUNT );
		StopRecordTime( end );
		GetBest( start, end, bestClocksSIMD );
	}

	for ( i = 0; i < COUNT; i++ ) {
		if ( bytedst[i] != bytedst2[i] ) {
			break;
		}
	}
	result = ( i >= COUNT ) ? "ok" :  S_COLOR_RED "X";
	PrintClocks( va( "   simd->CmpGT( float[] >= float ) %s", result ), COUNT, bestClocksSIMD, bestClocksGeneric );

	bestClocksGeneric = 0;
	for ( i = 0; i < NUMTESTS; i++ ) {
		memset( bytedst, 0, COUNT );
		StartRecordTime( start );
		p_generic->CmpGT( bytedst, 2, fsrc0, 0.0f, COUNT );
		StopRecordTime( end );
		GetBest( start, end, bestClocksGeneric );
	}
	PrintClocks( "generic->CmpGT( 2, float[] >= float )", COUNT, bestClocksGeneric );

	bestClocksSIMD = 0;
	for ( i = 0; i < NUMTESTS; i++ ) {
		memset( bytedst2, 0, COUNT );
		StartRecordTime( start );
		p_simd->CmpGT( bytedst2, 2, fsrc0, 0.0f, COUNT );
		StopRecordTime( end );
		GetBest( start, end, bestClocksSIMD );
	}

	for ( i = 0; i < COUNT; i++ ) {
		if ( bytedst[i] != bytedst2[i] ) {
			break;
		}
	}
	result = ( i >= COUNT ) ? "ok" :  S_COLOR_RED "X";
	PrintClocks( va( "   simd->CmpGT( 2, float[] >= float ) %s", result ), COUNT, bestClocksSIMD, bestClocksGeneric );

	// ======================

	bestClocksGeneric = 0;
	for ( i = 0; i < NUMTESTS; i++ ) {
		StartRecordTime( start );
		p_generic->CmpGE( bytedst, fsrc0, 0.0f, COUNT );
		StopRecordTime( end );
		GetBest( start, end, bestClocksGeneric );
	}
	PrintClocks( "generic->CmpGE( float[] >= float )", COUNT, bestClocksGeneric );

	bestClocksSIMD = 0;
	for ( i = 0; i < NUMTESTS; i++ ) {
		StartRecordTime( start );
		p_simd->CmpGE( bytedst2, fsrc0, 0.0f, COUNT );
		StopRecordTime( end );
		GetBest( start, end, bestClocksSIMD );
	}

	for ( i = 0; i < COUNT; i++ ) {
		if ( bytedst[i] != bytedst2[i] ) {
			break;
		}
	}
	result = ( i >= COUNT ) ? "ok" :  S_COLOR_RED "X";
	PrintClocks( va( "   simd->CmpGE( float[] >= float ) %s", result ), COUNT, bestClocksSIMD, bestClocksGeneric );

	bestClocksGeneric = 0;
	for ( i = 0; i < NUMTESTS; i++ ) {
		memset( bytedst, 0, COUNT );
		StartRecordTime( start );
		p_generic->CmpGE( bytedst, 2, fsrc0, 0.0f, COUNT );
		StopRecordTime( end );
		GetBest( start, end, bestClocksGeneric );
	}
	PrintClocks( "generic->CmpGE( 2, float[] >= float )", COUNT, bestClocksGeneric );

	bestClocksSIMD = 0;
	for ( i = 0; i < NUMTESTS; i++ ) {
		memset( bytedst2, 0, COUNT );
		StartRecordTime( start );
		p_simd->CmpGE( bytedst2, 2, fsrc0, 0.0f, COUNT );
		StopRecordTime( end );
		GetBest( start, end, bestClocksSIMD );
	}

	for ( i = 0; i < COUNT; i++ ) {
		if ( bytedst[i] != bytedst2[i] ) {
			break;
		}
	}
	result = ( i >= COUNT ) ? "ok" :  S_COLOR_RED "X";
	PrintClocks( va( "   simd->CmpGE( 2, float[] >= float ) %s", result ), COUNT, bestClocksSIMD, bestClocksGeneric );

	// ======================

	bestClocksGeneric = 0;
	for ( i = 0; i < NUMTESTS; i++ ) {
		StartRecordTime( start );
		p_generic->CmpLT( bytedst, fsrc0, 0.0f, COUNT );
		StopRecordTime( end );
		GetBest( start, end, bestClocksGeneric );
	}
	PrintClocks( "generic->CmpLT( float[] >= float )", COUNT, bestClocksGeneric );

	bestClocksSIMD = 0;
	for ( i = 0; i < NUMTESTS; i++ ) {
		StartRecordTime( start );
		p_simd->CmpLT( bytedst2, fsrc0, 0.0f, COUNT );
		StopRecordTime( end );
		GetBest( start, end, bestClocksSIMD );
	}

	for ( i = 0; i < COUNT; i++ ) {
		if ( bytedst[i] != bytedst2[i] ) {
			break;
		}
	}
	result = ( i >= COUNT ) ? "ok" :  S_COLOR_RED "X";
	PrintClocks( va( "   simd->CmpLT( float[] >= float ) %s", result ), COUNT, bestClocksSIMD, bestClocksGeneric );

	bestClocksGeneric = 0;
	for ( i = 0; i < NUMTESTS; i++ ) {
		memset( bytedst, 0, COUNT );
		StartRecordTime( start );
		p_generic->CmpLT( bytedst, 2, fsrc0, 0.0f, COUNT );
		StopRecordTime( end );
		GetBest( start, end, bestClocksGeneric );
	}
	PrintClocks( "generic->CmpLT( 2, float[] >= float )", COUNT, bestClocksGeneric );

	bestClocksSIMD = 0;
	for ( i = 0; i < NUMTESTS; i++ ) {
		memset( bytedst2, 0, COUNT );
		StartRecordTime( start );
		p_simd->CmpLT( bytedst2, 2, fsrc0, 0.0f, COUNT );
		StopRecordTime( end );
		GetBest( start, end, bestClocksSIMD );
	}

	for ( i = 0; i < COUNT; i++ ) {
		if ( bytedst[i] != bytedst2[i] ) {
			break;
		}
	}
	result = ( i >= COUNT ) ? "ok" :  S_COLOR_RED "X";
	PrintClocks( va( "   simd->CmpLT( 2, float[] >= float ) %s", result ), COUNT, bestClocksSIMD, bestClocksGeneric );

	// ======================

	bestClocksGeneric = 0;
	for ( i = 0; i < NUMTESTS; i++ ) {
		StartRecordTime( start );
		p_generic->CmpLE( bytedst, fsrc0, 0.0f, COUNT );
		StopRecordTime( end );
		GetBest( start, end, bestClocksGeneric );
	}
	PrintClocks( "generic->CmpLE( float[] >= float )", COUNT, bestClocksGeneric );

	bestClocksSIMD = 0;
	for ( i = 0; i < NUMTESTS; i++ ) {
		StartRecordTime( start );
		p_simd->CmpLE( bytedst2, fsrc0, 0.0f, COUNT );
		StopRecordTime( end );
		GetBest( start, end, bestClocksSIMD );
	}

	for ( i = 0; i < COUNT; i++ ) {
		if ( bytedst[i] != bytedst2[i] ) {
			break;
		}
	}
	result = ( i >= COUNT ) ? "ok" :  S_COLOR_RED "X";
	PrintClocks( va( "   simd->CmpLE( float[] >= float ) %s", result ), COUNT, bestClocksSIMD, bestClocksGeneric );

	bestClocksGeneric = 0;
	for ( i = 0; i < NUMTESTS; i++ ) {
		memset( bytedst, 0, COUNT );
		StartRecordTime( start );
		p_generic->CmpLE( bytedst, 2, fsrc0, 0.0f, COUNT );
		StopRecordTime( end );
		GetBest( start, end, bestClocksGeneric );
	}
	PrintClocks( "generic->CmpLE( 2, float[] >= float )", COUNT, bestClocksGeneric );

	bestClocksSIMD = 0;
	for ( i = 0; i < NUMTESTS; i++ ) {
		memset( bytedst2, 0, COUNT );
		StartRecordTime( start );
		p_simd->CmpLE( bytedst2, 2, fsrc0, 0.0f, COUNT );
		StopRecordTime( end );
		GetBest( start, end, bestClocksSIMD );
	}

	for ( i = 0; i < COUNT; i++ ) {
		if ( bytedst[i] != bytedst2[i] ) {
			break;
		}
	}
	result = ( i >= COUNT ) ? "ok" :  S_COLOR_RED "X";
	PrintClocks( va( "   simd->CmpLE( 2, float[] >= float ) %s", result ), COUNT, bestClocksSIMD, bestClocksGeneric );
}


/*
============
TestClamp
============
*/
void TestClamp( void ) {
	int i;
	TIME_TYPE start, end, bestClocksGeneric, bestClocksSIMD;
	ALIGN16( float fdst0[COUNT] );
	ALIGN16( float fdst1[COUNT] );
	ALIGN16( float fsrc0[COUNT] );
	const char *result;

	idRandom srnd( RANDOM_SEED );

	for ( i = 0; i < COUNT; i++ ) {
		fsrc0[i] = srnd.CRandomFloat() * 10.0f;
	}

	idLib::common->Printf("====================================\n" );

	bestClocksGeneric = 0;
	for ( i = 0; i < NUMTESTS; i++ ) {
		StartRecordTime( start );
		p_generic->Clamp( fdst0, fsrc0, -1.0f, 1.0f, COUNT );
		StopRecordTime( end );
		GetBest( start, end, bestClocksGeneric );
	}
	PrintClocks( "generic->Clamp( float[] )", COUNT, bestClocksGeneric );

	bestClocksSIMD = 0;
	for ( i = 0; i < NUMTESTS; i++ ) {
		StartRecordTime( start );
		p_simd->Clamp( fdst1, fsrc0, -1.0f, 1.0f, COUNT );
		StopRecordTime( end );
		GetBest( start, end, bestClocksSIMD );
	}

	for ( i = 0; i < COUNT; i++ ) {
		if ( fdst0[i] != fdst1[i] ) {
			break;
		}
	}
	result = ( i >= COUNT ) ? "ok" :  S_COLOR_RED "X";
	PrintClocks( va( "   simd->Clamp( float[] ) %s", result ), COUNT, bestClocksSIMD, bestClocksGeneric );


	bestClocksGeneric = 0;
	for ( i = 0; i < NUMTESTS; i++ ) {
		StartRecordTime( start );
		p_generic->ClampMin( fdst0, fsrc0, -1.0f, COUNT );
		StopRecordTime( end );
		GetBest( start, end, bestClocksGeneric );
	}
	PrintClocks( "generic->ClampMin( float[] )", COUNT, bestClocksGeneric );

	bestClocksSIMD = 0;
	for ( i = 0; i < NUMTESTS; i++ ) {
		StartRecordTime( start );
		p_simd->ClampMin( fdst1, fsrc0, -1.0f, COUNT );
		StopRecordTime( end );
		GetBest( start, end, bestClocksSIMD );
	}

	for ( i = 0; i < COUNT; i++ ) {
		if ( fdst0[i] != fdst1[i] ) {
			break;
		}
	}
	result = ( i >= COUNT ) ? "ok" :  S_COLOR_RED "X";
	PrintClocks( va( "   simd->ClampMin( float[] ) %s", result ), COUNT, bestClocksSIMD, bestClocksGeneric );


	bestClocksGeneric = 0;
	for ( i = 0; i < NUMTESTS; i++ ) {
		StartRecordTime( start );
		p_generic->ClampMax( fdst0, fsrc0, 1.0f, COUNT );
		StopRecordTime( end );
		GetBest( start, end, bestClocksGeneric );
	}
	PrintClocks( "generic->ClampMax( float[] )", COUNT, bestClocksGeneric );

	bestClocksSIMD = 0;
	for ( i = 0; i < NUMTESTS; i++ ) {
		StartRecordTime( start );
		p_simd->ClampMax( fdst1, fsrc0, 1.0f, COUNT );
		StopRecordTime( end );
		GetBest( start, end, bestClocksSIMD );
	}

	for ( i = 0; i < COUNT; i++ ) {
		if ( fdst0[i] != fdst1[i] ) {
			break;
		}
	}
	result = ( i >= COUNT ) ? "ok" :  S_COLOR_RED "X";
	PrintClocks( va( "   simd->ClampMax( float[] ) %s", result ), COUNT, bestClocksSIMD, bestClocksGeneric );
}

/*
============
TestMemcpy
============
*/
void TestMemcpy( void ) {
	int i, j;
	byte test0[8192];
	byte test1[8192];

	idRandom random( RANDOM_SEED );

	idLib::common->Printf("====================================\n" );

	for ( i = 5; i < 8192; i += 31 ) {
		for ( j = 0; j < i; j++ ) {
			test0[j] = random.RandomInt( 255 );
		}
		p_simd->Memcpy( test1, test0, 8192 );
		for ( j = 0; j < i; j++ ) {
			if ( test1[j] != test0[j] ) {
				idLib::common->Printf( "   simd->Memcpy() " S_COLOR_RED "X\n" );
				return;
			}
		}
	}
	idLib::common->Printf( "   simd->Memcpy() ok\n" );
}

/*
============
TestMemset
============
*/
void TestMemset( void ) {
	int i, j, k;
	byte test[8192];

	for ( i = 0; i < 8192; i++ ) {
		test[i] = 0;
	}

	for ( i = 5; i < 8192; i += 31 ) {
		for ( j = -1; j <= 1; j++ ) {
			p_simd->Memset( test, j, i );
			for ( k = 0; k < i; k++ ) {
				if ( test[k] != (byte)j ) {
					idLib::common->Printf( "   simd->Memset() " S_COLOR_RED "X\n" );
					return;
				}
			}
		}
	}
	idLib::common->Printf( "   simd->Memset() ok\n" );
}

#define	MATX_SIMD_EPSILON			1e-5f

/*
============
TestMatXMultiplyVecX
============
*/
void TestMatXMultiplyVecX( void ) {
	int i, j;
	TIME_TYPE start, end, bestClocksGeneric, bestClocksSIMD;
	const char *result;
	idMatX mat;
	idVecX src(6);
	idVecX dst(6);
	idVecX tst(6);

	src[0] = 1.0f;
	src[1] = 2.0f;
	src[2] = 3.0f;
	src[3] = 4.0f;
	src[4] = 5.0f;
	src[5] = 6.0f;

	idLib::common->Printf("================= NxN * Nx1 ===================\n" );

	for ( i = 1; i <= 6; i++ ) {
		mat.Random( i, i, RANDOM_SEED, -10.0f, 10.0f );

		bestClocksGeneric = 0;
		for ( j = 0; j < NUMTESTS; j++ ) {
			dst.Zero();
			StartRecordTime( start );
			p_generic->MatX_MultiplyVecX( dst, mat, src );
			StopRecordTime( end );
			GetBest( start, end, bestClocksGeneric );
		}
		tst = dst;

		PrintClocks( va( "generic->MatX_MultiplyVecX %dx%d*%dx1", i, i, i ), 1, bestClocksGeneric );

		bestClocksSIMD = 0;
		for ( j = 0; j < NUMTESTS; j++ ) {
			dst.Zero();
			StartRecordTime( start );
			p_simd->MatX_MultiplyVecX( dst, mat, src );
			StopRecordTime( end );
			GetBest( start, end, bestClocksSIMD );
		}

		result = dst.Compare( tst, MATX_SIMD_EPSILON ) ? "ok" :  S_COLOR_RED "X";
		PrintClocks( va( "   simd->MatX_MultiplyVecX %dx%d*%dx1 %s", i, i, i, result ), 1, bestClocksSIMD, bestClocksGeneric );
	}

	idLib::common->Printf("================= Nx6 * 6x1 ===================\n" );

	for ( i = 1; i <= 6; i++ ) {
		mat.Random( i, 6, RANDOM_SEED, -10.0f, 10.0f );

		bestClocksGeneric = 0;
		for ( j = 0; j < NUMTESTS; j++ ) {
			dst.Zero();
			StartRecordTime( start );
			p_generic->MatX_MultiplyVecX( dst, mat, src );
			StopRecordTime( end );
			GetBest( start, end, bestClocksGeneric );
		}
		tst = dst;

		PrintClocks( va( "generic->MatX_MultiplyVecX %dx6*6x1", i ), 1, bestClocksGeneric );

		bestClocksSIMD = 0;
		for ( j = 0; j < NUMTESTS; j++ ) {
			dst.Zero();
			StartRecordTime( start );
			p_simd->MatX_MultiplyVecX( dst, mat, src );
			StopRecordTime( end );
			GetBest( start, end, bestClocksSIMD );
		}

		result = dst.Compare( tst, MATX_SIMD_EPSILON ) ? "ok" :  S_COLOR_RED "X";
		PrintClocks( va( "   simd->MatX_MultiplyVecX %dx6*6x1 %s", i, result ), 1, bestClocksSIMD, bestClocksGeneric );
	}

	idLib::common->Printf("================= 6xN * Nx1 ===================\n" );

	for ( i = 1; i <= 6; i++ ) {
		mat.Random( 6, i, RANDOM_SEED, -10.0f, 10.0f );

		bestClocksGeneric = 0;
		for ( j = 0; j < NUMTESTS; j++ ) {
			dst.Zero();
			StartRecordTime( start );
			p_generic->MatX_MultiplyVecX( dst, mat, src );
			StopRecordTime( end );
			GetBest( start, end, bestClocksGeneric );
		}
		tst = dst;

		PrintClocks( va( "generic->MatX_MultiplyVecX 6x%d*%dx1", i, i ), 1, bestClocksGeneric );

		bestClocksSIMD = 0;
		for ( j = 0; j < NUMTESTS; j++ ) {
			StartRecordTime( start );
			p_simd->MatX_MultiplyVecX( dst, mat, src );
			StopRecordTime( end );
			GetBest( start, end, bestClocksSIMD );
		}

		result = dst.Compare( tst, MATX_SIMD_EPSILON ) ? "ok" :  S_COLOR_RED "X";
		PrintClocks( va( "   simd->MatX_MultiplyVecX 6x%d*%dx1 %s", i, i, result ), 1, bestClocksSIMD, bestClocksGeneric );
	}
}

/*
============
TestMatXMultiplyAddVecX
============
*/
void TestMatXMultiplyAddVecX( void ) {
	int i, j;
	TIME_TYPE start, end, bestClocksGeneric, bestClocksSIMD;
	const char *result;
	idMatX mat;
	idVecX src(6);
	idVecX dst(6);
	idVecX tst(6);

	src[0] = 1.0f;
	src[1] = 2.0f;
	src[2] = 3.0f;
	src[3] = 4.0f;
	src[4] = 5.0f;
	src[5] = 6.0f;

	idLib::common->Printf("================= NxN * Nx1 ===================\n" );

	for ( i = 1; i <= 6; i++ ) {
		mat.Random( i, i, RANDOM_SEED, -10.0f, 10.0f );

		bestClocksGeneric = 0;
		for ( j = 0; j < NUMTESTS; j++ ) {
			dst.Zero();
			StartRecordTime( start );
			p_generic->MatX_MultiplyAddVecX( dst, mat, src );
			StopRecordTime( end );
			GetBest( start, end, bestClocksGeneric );
		}
		tst = dst;

		PrintClocks( va( "generic->MatX_MultiplyAddVecX %dx%d*%dx1", i, i, i ), 1, bestClocksGeneric );

		bestClocksSIMD = 0;
		for ( j = 0; j < NUMTESTS; j++ ) {
			dst.Zero();
			StartRecordTime( start );
			p_simd->MatX_MultiplyAddVecX( dst, mat, src );
			StopRecordTime( end );
			GetBest( start, end, bestClocksSIMD );
		}

		result = dst.Compare( tst, MATX_SIMD_EPSILON ) ? "ok" :  S_COLOR_RED "X";
		PrintClocks( va( "   simd->MatX_MultiplyAddVecX %dx%d*%dx1 %s", i, i, i, result ), 1, bestClocksSIMD, bestClocksGeneric );
	}

	idLib::common->Printf("================= Nx6 * 6x1 ===================\n" );

	for ( i = 1; i <= 6; i++ ) {
		mat.Random( i, 6, RANDOM_SEED, -10.0f, 10.0f );

		bestClocksGeneric = 0;
		for ( j = 0; j < NUMTESTS; j++ ) {
			dst.Zero();
			StartRecordTime( start );
			p_generic->MatX_MultiplyAddVecX( dst, mat, src );
			StopRecordTime( end );
			GetBest( start, end, bestClocksGeneric );
		}
		tst = dst;

		PrintClocks( va( "generic->MatX_MultiplyAddVecX %dx6*6x1", i ), 1, bestClocksGeneric );

		bestClocksSIMD = 0;
		for ( j = 0; j < NUMTESTS; j++ ) {
			dst.Zero();
			StartRecordTime( start );
			p_simd->MatX_MultiplyAddVecX( dst, mat, src );
			StopRecordTime( end );
			GetBest( start, end, bestClocksSIMD );
		}

		result = dst.Compare( tst, MATX_SIMD_EPSILON ) ? "ok" :  S_COLOR_RED "X";
		PrintClocks( va( "   simd->MatX_MultiplyAddVecX %dx6*6x1 %s", i, result ), 1, bestClocksSIMD, bestClocksGeneric );
	}

	idLib::common->Printf("================= 6xN * Nx1 ===================\n" );

	for ( i = 1; i <= 6; i++ ) {
		mat.Random( 6, i, RANDOM_SEED, -10.0f, 10.0f );

		bestClocksGeneric = 0;
		for ( j = 0; j < NUMTESTS; j++ ) {
			dst.Zero();
			StartRecordTime( start );
			p_generic->MatX_MultiplyAddVecX( dst, mat, src );
			StopRecordTime( end );
			GetBest( start, end, bestClocksGeneric );
		}
		tst = dst;

		PrintClocks( va( "generic->MatX_MultiplyAddVecX 6x%d*%dx1", i, i ), 1, bestClocksGeneric );

		bestClocksSIMD = 0;
		for ( j = 0; j < NUMTESTS; j++ ) {
			dst.Zero();
			StartRecordTime( start );
			p_simd->MatX_MultiplyAddVecX( dst, mat, src );
			StopRecordTime( end );
			GetBest( start, end, bestClocksSIMD );
		}

		result = dst.Compare( tst, MATX_SIMD_EPSILON ) ? "ok" :  S_COLOR_RED "X";
		PrintClocks( va( "   simd->MatX_MultiplyAddVecX 6x%d*%dx1 %s", i, i, result ), 1, bestClocksSIMD, bestClocksGeneric );
	}
}

/*
============
TestMatXTransposeMultiplyVecX
============
*/
void TestMatXTransposeMultiplyVecX( void ) {
	int i, j;
	TIME_TYPE start, end, bestClocksGeneric, bestClocksSIMD;
	const char *result;
	idMatX mat;
	idVecX src(6);
	idVecX dst(6);
	idVecX tst(6);

	src[0] = 1.0f;
	src[1] = 2.0f;
	src[2] = 3.0f;
	src[3] = 4.0f;
	src[4] = 5.0f;
	src[5] = 6.0f;

	idLib::common->Printf("================= Nx6 * Nx1 ===================\n" );

	for ( i = 1; i <= 6; i++ ) {
		mat.Random( i, 6, RANDOM_SEED, -10.0f, 10.0f );

		bestClocksGeneric = 0;
		for ( j = 0; j < NUMTESTS; j++ ) {
			dst.Zero();
			StartRecordTime( start );
			p_generic->MatX_TransposeMultiplyVecX( dst, mat, src );
			StopRecordTime( end );
			GetBest( start, end, bestClocksGeneric );
		}
		tst = dst;

		PrintClocks( va( "generic->MatX_TransposeMulVecX %dx6*%dx1", i, i ), 1, bestClocksGeneric );

		bestClocksSIMD = 0;
		for ( j = 0; j < NUMTESTS; j++ ) {
			dst.Zero();
			StartRecordTime( start );
			p_simd->MatX_TransposeMultiplyVecX( dst, mat, src );
			StopRecordTime( end );
			GetBest( start, end, bestClocksSIMD );
		}

		result = dst.Compare( tst, MATX_SIMD_EPSILON ) ? "ok" :  S_COLOR_RED "X";
		PrintClocks( va( "   simd->MatX_TransposeMulVecX %dx6*%dx1 %s", i, i, result ), 1, bestClocksSIMD, bestClocksGeneric );
	}

	idLib::common->Printf("================= 6xN * 6x1 ===================\n" );

	for ( i = 1; i <= 6; i++ ) {
		mat.Random( 6, i, RANDOM_SEED, -10.0f, 10.0f );

		bestClocksGeneric = 0;
		for ( j = 0; j < NUMTESTS; j++ ) {
			dst.Zero();
			StartRecordTime( start );
			p_generic->MatX_TransposeMultiplyVecX( dst, mat, src );
			StopRecordTime( end );
			GetBest( start, end, bestClocksGeneric );
		}
		tst = dst;

		PrintClocks( va( "generic->MatX_TransposeMulVecX 6x%d*6x1", i ), 1, bestClocksGeneric );

		bestClocksSIMD = 0;
		for ( j = 0; j < NUMTESTS; j++ ) {
			dst.Zero();
			StartRecordTime( start );
			p_simd->MatX_TransposeMultiplyVecX( dst, mat, src );
			StopRecordTime( end );
			GetBest( start, end, bestClocksSIMD );
		}

		result = dst.Compare( tst, MATX_SIMD_EPSILON ) ? "ok" :  S_COLOR_RED "X";
		PrintClocks( va( "   simd->MatX_TransposeMulVecX 6x%d*6x1 %s", i, result ), 1, bestClocksSIMD, bestClocksGeneric );
	}
}

/*
============
TestMatXTransposeMultiplyAddVecX
============
*/
void TestMatXTransposeMultiplyAddVecX( void ) {
	int i, j;
	TIME_TYPE start, end, bestClocksGeneric, bestClocksSIMD;
	const char *result;
	idMatX mat;
	idVecX src(6);
	idVecX dst(6);
	idVecX tst(6);

	src[0] = 1.0f;
	src[1] = 2.0f;
	src[2] = 3.0f;
	src[3] = 4.0f;
	src[4] = 5.0f;
	src[5] = 6.0f;

	idLib::common->Printf("================= Nx6 * Nx1 ===================\n" );

	for ( i = 1; i <= 6; i++ ) {
		mat.Random( i, 6, RANDOM_SEED, -10.0f, 10.0f );

		bestClocksGeneric = 0;
		for ( j = 0; j < NUMTESTS; j++ ) {
			dst.Zero();
			StartRecordTime( start );
			p_generic->MatX_TransposeMultiplyAddVecX( dst, mat, src );
			StopRecordTime( end );
			GetBest( start, end, bestClocksGeneric );
		}
		tst = dst;

		PrintClocks( va( "generic->MatX_TransposeMulAddVecX %dx6*%dx1", i, i ), 1, bestClocksGeneric );

		bestClocksSIMD = 0;
		for ( j = 0; j < NUMTESTS; j++ ) {
			dst.Zero();
			StartRecordTime( start );
			p_simd->MatX_TransposeMultiplyAddVecX( dst, mat, src );
			StopRecordTime( end );
			GetBest( start, end, bestClocksSIMD );
		}

		result = dst.Compare( tst, MATX_SIMD_EPSILON ) ? "ok" :  S_COLOR_RED "X";
		PrintClocks( va( "   simd->MatX_TransposeMulAddVecX %dx6*%dx1 %s", i, i, result ), 1, bestClocksSIMD, bestClocksGeneric );
	}

	idLib::common->Printf("================= 6xN * 6x1 ===================\n" );

	for ( i = 1; i <= 6; i++ ) {
		mat.Random( 6, i, RANDOM_SEED, -10.0f, 10.0f );

		bestClocksGeneric = 0;
		for ( j = 0; j < NUMTESTS; j++ ) {
			dst.Zero();
			StartRecordTime( start );
			p_generic->MatX_TransposeMultiplyAddVecX( dst, mat, src );
			StopRecordTime( end );
			GetBest( start, end, bestClocksGeneric );
		}
		tst = dst;

		PrintClocks( va( "generic->MatX_TransposeMulAddVecX 6x%d*6x1", i ), 1, bestClocksGeneric );

		bestClocksSIMD = 0;
		for ( j = 0; j < NUMTESTS; j++ ) {
			dst.Zero();
			StartRecordTime( start );
			p_simd->MatX_TransposeMultiplyAddVecX( dst, mat, src );
			StopRecordTime( end );
			GetBest( start, end, bestClocksSIMD );
		}

		result = dst.Compare( tst, MATX_SIMD_EPSILON ) ? "ok" :  S_COLOR_RED "X";
		PrintClocks( va( "   simd->MatX_TransposeMulAddVecX 6x%d*6x1 %s", i, result ), 1, bestClocksSIMD, bestClocksGeneric );
	}
}

/*
============
TestMatXMultiplyMatX
============
*/
#define TEST_VALUE_RANGE			10.0f
#define	MATX_MATX_SIMD_EPSILON		1e-4f

void TestMatXMultiplyMatX( void ) {
	int i, j;
	TIME_TYPE start, end, bestClocksGeneric, bestClocksSIMD;
	const char *result;
	idMatX m1, m2, dst, tst;

	idLib::common->Printf("================= NxN * Nx6 ===================\n" );

	// NxN * Nx6
	for ( i = 1; i <= 5; i++ ) {
		m1.Random( i, i, RANDOM_SEED, -TEST_VALUE_RANGE, TEST_VALUE_RANGE );
		m2.Random( i, 6, RANDOM_SEED, -TEST_VALUE_RANGE, TEST_VALUE_RANGE );
		dst.SetSize( i, 6 );

		bestClocksGeneric = 0;
		for ( j = 0; j < NUMTESTS; j++ ) {
			StartRecordTime( start );
			p_generic->MatX_MultiplyMatX( dst, m1, m2 );
			StopRecordTime( end );
			GetBest( start, end, bestClocksGeneric );
		}
		tst = dst;

		PrintClocks( va( "generic->MatX_MultiplyMatX %dx%d*%dx6", i, i, i ), 1, bestClocksGeneric );

		bestClocksSIMD = 0;
		for ( j = 0; j < NUMTESTS; j++ ) {
			StartRecordTime( start );
			p_simd->MatX_MultiplyMatX( dst, m1, m2 );
			StopRecordTime( end );
			GetBest( start, end, bestClocksSIMD );
		}

		result = dst.Compare( tst, MATX_MATX_SIMD_EPSILON ) ? "ok" :  S_COLOR_RED "X";
		PrintClocks( va( "   simd->MatX_MultiplyMatX %dx%d*%dx6 %s", i, i, i, result ), 1, bestClocksSIMD, bestClocksGeneric );
	}

	idLib::common->Printf("================= 6xN * Nx6 ===================\n" );

	// 6xN * Nx6
	for ( i = 1; i <= 5; i++ ) {
		m1.Random( 6, i, RANDOM_SEED, -TEST_VALUE_RANGE, TEST_VALUE_RANGE );
		m2.Random( i, 6, RANDOM_SEED, -TEST_VALUE_RANGE, TEST_VALUE_RANGE );
		dst.SetSize( 6, 6 );

		bestClocksGeneric = 0;
		for ( j = 0; j < NUMTESTS; j++ ) {
			StartRecordTime( start );
			p_generic->MatX_MultiplyMatX( dst, m1, m2 );
			StopRecordTime( end );
			GetBest( start, end, bestClocksGeneric );
		}
		tst = dst;

		PrintClocks( va( "generic->MatX_MultiplyMatX 6x%d*%dx6", i, i ), 1, bestClocksGeneric );

		bestClocksSIMD = 0;
		for ( j = 0; j < NUMTESTS; j++ ) {
			StartRecordTime( start );
			p_simd->MatX_MultiplyMatX( dst, m1, m2 );
			StopRecordTime( end );
			GetBest( start, end, bestClocksSIMD );
		}

		result = dst.Compare( tst, MATX_MATX_SIMD_EPSILON ) ? "ok" :  S_COLOR_RED "X";
		PrintClocks( va( "   simd->MatX_MultiplyMatX 6x%d*%dx6 %s", i, i, result ), 1, bestClocksSIMD, bestClocksGeneric );
	}

	idLib::common->Printf("================= Nx6 * 6xN ===================\n" );

	// Nx6 * 6xN
	for ( i = 1; i <= 5; i++ ) {
		m1.Random( i, 6, RANDOM_SEED, -TEST_VALUE_RANGE, TEST_VALUE_RANGE );
		m2.Random( 6, i, RANDOM_SEED, -TEST_VALUE_RANGE, TEST_VALUE_RANGE );
		dst.SetSize( i, i );

		bestClocksGeneric = 0;
		for ( j = 0; j < NUMTESTS; j++ ) {
			StartRecordTime( start );
			p_generic->MatX_MultiplyMatX( dst, m1, m2 );
			StopRecordTime( end );
			GetBest( start, end, bestClocksGeneric );
		}
		tst = dst;

		PrintClocks( va( "generic->MatX_MultiplyMatX %dx6*6x%d", i, i ), 1, bestClocksGeneric );

		bestClocksSIMD = 0;
		for ( j = 0; j < NUMTESTS; j++ ) {
			StartRecordTime( start );
			p_simd->MatX_MultiplyMatX( dst, m1, m2 );
			StopRecordTime( end );
			GetBest( start, end, bestClocksSIMD );
		}

		result = dst.Compare( tst, MATX_MATX_SIMD_EPSILON ) ? "ok" :  S_COLOR_RED "X";
		PrintClocks( va( "   simd->MatX_MultiplyMatX %dx6*6x%d %s", i, i, result ), 1, bestClocksSIMD, bestClocksGeneric );
	}

	idLib::common->Printf("================= 6x6 * 6xN ===================\n" );

	// 6x6 * 6xN
	for ( i = 1; i <= 6; i++ ) {
		m1.Random( 6, 6, RANDOM_SEED, -TEST_VALUE_RANGE, TEST_VALUE_RANGE );
		m2.Random( 6, i, RANDOM_SEED, -TEST_VALUE_RANGE, TEST_VALUE_RANGE );
		dst.SetSize( 6, i );

		bestClocksGeneric = 0;
		for ( j = 0; j < NUMTESTS; j++ ) {
			StartRecordTime( start );
			p_generic->MatX_MultiplyMatX( dst, m1, m2 );
			StopRecordTime( end );
			GetBest( start, end, bestClocksGeneric );
		}
		tst = dst;

		PrintClocks( va( "generic->MatX_MultiplyMatX 6x6*6x%d", i ), 1, bestClocksGeneric );

		bestClocksSIMD = 0;
		for ( j = 0; j < NUMTESTS; j++ ) {
			StartRecordTime( start );
			p_simd->MatX_MultiplyMatX( dst, m1, m2 );
			StopRecordTime( end );
			GetBest( start, end, bestClocksSIMD );
		}

		result = dst.Compare( tst, MATX_MATX_SIMD_EPSILON ) ? "ok" :  S_COLOR_RED "X";
		PrintClocks( va( "   simd->MatX_MultiplyMatX 6x6*6x%d %s", i, result ), 1, bestClocksSIMD, bestClocksGeneric );
	}
}

/*
============
TestMatXTransposeMultiplyMatX
============
*/
void TestMatXTransposeMultiplyMatX( void ) {
	int i, j;
	TIME_TYPE start, end, bestClocksGeneric, bestClocksSIMD;
	const char *result;
	idMatX m1, m2, dst, tst;

	idLib::common->Printf("================= Nx6 * NxN ===================\n" );

	// Nx6 * NxN
	for ( i = 1; i <= 5; i++ ) {
		m1.Random( i, 6, RANDOM_SEED, -TEST_VALUE_RANGE, TEST_VALUE_RANGE );
		m2.Random( i, i, RANDOM_SEED, -TEST_VALUE_RANGE, TEST_VALUE_RANGE );
		dst.SetSize( 6, i );

		bestClocksGeneric = 0;
		for ( j = 0; j < NUMTESTS; j++ ) {
			StartRecordTime( start );
			p_generic->MatX_TransposeMultiplyMatX( dst, m1, m2 );
			StopRecordTime( end );
			GetBest( start, end, bestClocksGeneric );
		}
		tst = dst;

		PrintClocks( va( "generic->MatX_TransMultiplyMatX %dx6*%dx%d", i, i, i ), 1, bestClocksGeneric );

		bestClocksSIMD = 0;
		for ( j = 0; j < NUMTESTS; j++ ) {
			StartRecordTime( start );
			p_simd->MatX_TransposeMultiplyMatX( dst, m1, m2 );
			StopRecordTime( end );
			GetBest( start, end, bestClocksSIMD );
		}

		result = dst.Compare( tst, MATX_MATX_SIMD_EPSILON ) ? "ok" :  S_COLOR_RED "X";
		PrintClocks( va( "   simd->MatX_TransMultiplyMatX %dx6*%dx%d %s", i, i, i, result ), 1, bestClocksSIMD, bestClocksGeneric );
	}

	idLib::common->Printf("================= 6xN * 6x6 ===================\n" );

	// 6xN * 6x6
	for ( i = 1; i <= 6; i++ ) {
		m1.Random( 6, i, RANDOM_SEED, -TEST_VALUE_RANGE, TEST_VALUE_RANGE );
		m2.Random( 6, 6, RANDOM_SEED, -TEST_VALUE_RANGE, TEST_VALUE_RANGE );
		dst.SetSize( i, 6 );

		bestClocksGeneric = 0;
		for ( j = 0; j < NUMTESTS; j++ ) {
			StartRecordTime( start );
			p_generic->MatX_TransposeMultiplyMatX( dst, m1, m2 );
			StopRecordTime( end );
			GetBest( start, end, bestClocksGeneric );
		}
		tst = dst;

		PrintClocks( va( "generic->MatX_TransMultiplyMatX 6x%d*6x6", i ), 1, bestClocksGeneric );

		bestClocksSIMD = 0;
		for ( j = 0; j < NUMTESTS; j++ ) {
			StartRecordTime( start );
			p_simd->MatX_TransposeMultiplyMatX( dst, m1, m2 );
			StopRecordTime( end );
			GetBest( start, end, bestClocksSIMD );
		}

		result = dst.Compare( tst, MATX_MATX_SIMD_EPSILON ) ? "ok" :  S_COLOR_RED "X";
		PrintClocks( va( "   simd->MatX_TransMultiplyMatX 6x%d*6x6 %s", i, result ), 1, bestClocksSIMD, bestClocksGeneric );
	}
}

#define MATX_LTS_SIMD_EPSILON		1.0f
#define MATX_LTS_SOLVE_SIZE			100

/*
============
TestMatXLowerTriangularSolve
============
*/
void TestMatXLowerTriangularSolve( void ) {
	int i, j;
	TIME_TYPE start, end, bestClocksGeneric, bestClocksSIMD;
	const char *result;
	idMatX L;
	idVecX x, b, tst;

	idLib::common->Printf("====================================\n" );

	L.Random( MATX_LTS_SOLVE_SIZE, MATX_LTS_SOLVE_SIZE, 0, -1.0f, 1.0f );
	x.SetSize( MATX_LTS_SOLVE_SIZE );
	b.Random( MATX_LTS_SOLVE_SIZE, 0, -1.0f, 1.0f );

	for ( i = 1; i < MATX_LTS_SOLVE_SIZE; i++ ) {

		x.Zero( i );

		bestClocksGeneric = 0;
		for ( j = 0; j < NUMTESTS; j++ ) {
			StartRecordTime( start );
			p_generic->MatX_LowerTriangularSolve( L, x.ToFloatPtr(), b.ToFloatPtr(), i );
			StopRecordTime( end );
			GetBest( start, end, bestClocksGeneric );
		}
		tst = x;
		x.Zero();

		PrintClocks( va( "generic->MatX_LowerTriangularSolve %dx%d", i, i ), 1, bestClocksGeneric );

		bestClocksSIMD = 0;
		for ( j = 0; j < NUMTESTS; j++ ) {
			StartRecordTime( start );
			p_simd->MatX_LowerTriangularSolve( L, x.ToFloatPtr(), b.ToFloatPtr(), i );
			StopRecordTime( end );
			GetBest( start, end, bestClocksSIMD );
		}

		result = x.Compare( tst, MATX_LTS_SIMD_EPSILON ) ? "ok" :  S_COLOR_RED "X";
		PrintClocks( va( "   simd->MatX_LowerTriangularSolve %dx%d %s", i, i, result ), 1, bestClocksSIMD, bestClocksGeneric );
	}
}

/*
============
TestMatXLowerTriangularSolveTranspose
============
*/
void TestMatXLowerTriangularSolveTranspose( void ) {
	int i, j;
	TIME_TYPE start, end, bestClocksGeneric, bestClocksSIMD;
	const char *result;
	idMatX L;
	idVecX x, b, tst;

	idLib::common->Printf("====================================\n" );

	L.Random( MATX_LTS_SOLVE_SIZE, MATX_LTS_SOLVE_SIZE, 0, -1.0f, 1.0f );
	x.SetSize( MATX_LTS_SOLVE_SIZE );
	b.Random( MATX_LTS_SOLVE_SIZE, 0, -1.0f, 1.0f );

	for ( i = 1; i < MATX_LTS_SOLVE_SIZE; i++ ) {

		x.Zero( i );

		bestClocksGeneric = 0;
		for ( j = 0; j < NUMTESTS; j++ ) {
			StartRecordTime( start );
			p_generic->MatX_LowerTriangularSolveTranspose( L, x.ToFloatPtr(), b.ToFloatPtr(), i );
			StopRecordTime( end );
			GetBest( start, end, bestClocksGeneric );
		}
		tst = x;
		x.Zero();

		PrintClocks( va( "generic->MatX_LowerTriangularSolveT %dx%d", i, i ), 1, bestClocksGeneric );

		bestClocksSIMD = 0;
		for ( j = 0; j < NUMTESTS; j++ ) {
			StartRecordTime( start );
			p_simd->MatX_LowerTriangularSolveTranspose( L, x.ToFloatPtr(), b.ToFloatPtr(), i );
			StopRecordTime( end );
			GetBest( start, end, bestClocksSIMD );
		}

		result = x.Compare( tst, MATX_LTS_SIMD_EPSILON ) ? "ok" :  S_COLOR_RED "X";
		PrintClocks( va( "   simd->MatX_LowerTriangularSolveT %dx%d %s", i, i, result ), 1, bestClocksSIMD, bestClocksGeneric );
	}
}

#define MATX_LDLT_SIMD_EPSILON			0.1f
#define MATX_LDLT_FACTOR_SOLVE_SIZE		64

/*
============
TestMatXLDLTFactor
============
*/
void TestMatXLDLTFactor( void ) {
	int i, j;
	TIME_TYPE start, end, bestClocksGeneric, bestClocksSIMD;
	const char *result;
	idMatX src, original, mat1, mat2;
	idVecX invDiag1, invDiag2;

	idLib::common->Printf("====================================\n" );

	original.SetSize( MATX_LDLT_FACTOR_SOLVE_SIZE, MATX_LDLT_FACTOR_SOLVE_SIZE );
	src.Random( MATX_LDLT_FACTOR_SOLVE_SIZE, MATX_LDLT_FACTOR_SOLVE_SIZE, 0, -1.0f, 1.0f );
	src.TransposeMultiply( original, src );

	for ( i = 1; i < MATX_LDLT_FACTOR_SOLVE_SIZE; i++ ) {

		bestClocksGeneric = 0;
		for ( j = 0; j < NUMTESTS; j++ ) {
			mat1 = original;
			invDiag1.Zero( MATX_LDLT_FACTOR_SOLVE_SIZE );
			StartRecordTime( start );
			p_generic->MatX_LDLTFactor( mat1, invDiag1, i );
			StopRecordTime( end );
			GetBest( start, end, bestClocksGeneric );
		}

		PrintClocks( va( "generic->MatX_LDLTFactor %dx%d", i, i ), 1, bestClocksGeneric );

		bestClocksSIMD = 0;
		for ( j = 0; j < NUMTESTS; j++ ) {
			mat2 = original;
			invDiag2.Zero( MATX_LDLT_FACTOR_SOLVE_SIZE );
			StartRecordTime( start );
			p_simd->MatX_LDLTFactor( mat2, invDiag2, i );
			StopRecordTime( end );
			GetBest( start, end, bestClocksSIMD );
		}

		result = mat1.Compare( mat2, MATX_LDLT_SIMD_EPSILON ) && invDiag1.Compare( invDiag2, MATX_LDLT_SIMD_EPSILON ) ? "ok" :  S_COLOR_RED "X";
		PrintClocks( va( "   simd->MatX_LDLTFactor %dx%d %s", i, i, result ), 1, bestClocksSIMD, bestClocksGeneric );
	}
}


/*
============
TestSoundUpSampling
============
*/
#define SOUND_UPSAMPLE_EPSILON		1.0f

void TestSoundUpSampling( void ) {
	int i;
	TIME_TYPE start, end, bestClocksGeneric, bestClocksSIMD;
	ALIGN16( short pcm[MIXBUFFER_SAMPLES*2] );
	ALIGN16( float ogg0[MIXBUFFER_SAMPLES*2] );
	ALIGN16( float ogg1[MIXBUFFER_SAMPLES*2] );
	ALIGN16( float samples1[MIXBUFFER_SAMPLES*2] );
	ALIGN16( float samples2[MIXBUFFER_SAMPLES*2] );
	float *ogg[2];
	int kHz, numSpeakers;
	const char *result;

	idRandom srnd( RANDOM_SEED );

	for ( i = 0; i < MIXBUFFER_SAMPLES*2; i++ ) {
		pcm[i] = srnd.RandomInt( (1<<16) ) - (1<<15);
		ogg0[i] = srnd.RandomFloat();
		ogg1[i] = srnd.RandomFloat();
	}

	ogg[0] = ogg0;
	ogg[1] = ogg1;

	for ( numSpeakers = 1; numSpeakers <= 2; numSpeakers++ ) {

		for ( kHz = 11025; kHz <= 44100; kHz *= 2 ) {
			bestClocksGeneric = 0;
			for ( i = 0; i < NUMTESTS; i++ ) {
				StartRecordTime( start );
				p_generic->UpSamplePCMTo44kHz( samples1, pcm, MIXBUFFER_SAMPLES*numSpeakers*kHz/44100, kHz, numSpeakers );
				StopRecordTime( end );
				GetBest( start, end, bestClocksGeneric );
			}
			PrintClocks( va( "generic->UpSamplePCMTo44kHz( %d, %d )", kHz, numSpeakers ), MIXBUFFER_SAMPLES*numSpeakers*kHz/44100, bestClocksGeneric );

			bestClocksSIMD = 0;
			for ( i = 0; i < NUMTESTS; i++ ) {
				StartRecordTime( start );
				p_simd->UpSamplePCMTo44kHz( samples2, pcm, MIXBUFFER_SAMPLES*numSpeakers*kHz/44100, kHz, numSpeakers );
				StopRecordTime( end );
				GetBest( start, end, bestClocksSIMD );
			}

			for ( i = 0; i < MIXBUFFER_SAMPLES*numSpeakers; i++ ) {
				if ( idMath::Fabs( samples1[i] - samples2[i] ) > SOUND_UPSAMPLE_EPSILON ) {
					break;
				}
			}
			result = ( i >= MIXBUFFER_SAMPLES*numSpeakers ) ? "ok" :  S_COLOR_RED "X";
			PrintClocks( va( "   simd->UpSamplePCMTo44kHz( %d, %d ) %s", kHz, numSpeakers, result ), MIXBUFFER_SAMPLES*numSpeakers*kHz/44100, bestClocksSIMD, bestClocksGeneric );
		}
	}

	for ( numSpeakers = 1; numSpeakers <= 2; numSpeakers++ ) {

		for ( kHz = 11025; kHz <= 44100; kHz *= 2 ) {
			bestClocksGeneric = 0;
			for ( i = 0; i < NUMTESTS; i++ ) {
				StartRecordTime( start );
				p_generic->UpSampleOGGTo44kHz( samples1, ogg, MIXBUFFER_SAMPLES*numSpeakers*kHz/44100, kHz, numSpeakers );
				StopRecordTime( end );
				GetBest( start, end, bestClocksGeneric );
			}
			PrintClocks( va( "generic->UpSampleOGGTo44kHz( %d, %d )", kHz, numSpeakers ), MIXBUFFER_SAMPLES*numSpeakers*kHz/44100, bestClocksGeneric );

			bestClocksSIMD = 0;
			for ( i = 0; i < NUMTESTS; i++ ) {
				StartRecordTime( start );
				p_simd->UpSampleOGGTo44kHz( samples2, ogg, MIXBUFFER_SAMPLES*numSpeakers*kHz/44100, kHz, numSpeakers );
				StopRecordTime( end );
				GetBest( start, end, bestClocksSIMD );
			}

			for ( i = 0; i < MIXBUFFER_SAMPLES*numSpeakers; i++ ) {
				if ( idMath::Fabs( samples1[i] - samples2[i] ) > SOUND_UPSAMPLE_EPSILON ) {
					break;
				}
			}
			result = ( i >= MIXBUFFER_SAMPLES ) ? "ok" :  S_COLOR_RED "X";
			PrintClocks( va( "   simd->UpSampleOGGTo44kHz( %d, %d ) %s", kHz, numSpeakers, result ), MIXBUFFER_SAMPLES*numSpeakers*kHz/44100, bestClocksSIMD, bestClocksGeneric );
		}
	}
}

/*
============
TestSoundMixing
============
*/
#define SOUND_MIX_EPSILON		2.0f

void TestSoundMixing( void ) {
	int i, j;
	TIME_TYPE start, end, bestClocksGeneric, bestClocksSIMD;
	ALIGN16( float origMixBuffer[MIXBUFFER_SAMPLES*6] );
	ALIGN16( float mixBuffer1[MIXBUFFER_SAMPLES*6] );
	ALIGN16( float mixBuffer2[MIXBUFFER_SAMPLES*6] );
	ALIGN16( float samples[MIXBUFFER_SAMPLES*6] );
	ALIGN16( short outSamples1[MIXBUFFER_SAMPLES*6] );
	ALIGN16( short outSamples2[MIXBUFFER_SAMPLES*6] );
	float lastV[6];
	float currentV[6];
	const char *result;

	idRandom srnd( RANDOM_SEED );

	for ( i = 0; i < 6; i++ ) {
		lastV[i] = srnd.CRandomFloat();
		currentV[i] = srnd.CRandomFloat();
	}

	for ( i = 0; i < MIXBUFFER_SAMPLES*6; i++ ) {
		origMixBuffer[i] = srnd.CRandomFloat();
		samples[i] = srnd.RandomInt( (1<<16) ) - (1<<15);
	}

	bestClocksGeneric = 0;
	for ( i = 0; i < NUMTESTS; i++ ) {
		for ( j = 0; j < MIXBUFFER_SAMPLES*6; j++ ) {
			mixBuffer1[j] = origMixBuffer[j];
		}
		StartRecordTime( start );
		p_generic->MixSoundTwoSpeakerMono( mixBuffer1, samples, MIXBUFFER_SAMPLES, lastV, currentV );
		StopRecordTime( end );
		GetBest( start, end, bestClocksGeneric );
	}
	PrintClocks( "generic->MixSoundTwoSpeakerMono()", MIXBUFFER_SAMPLES, bestClocksGeneric );


	bestClocksSIMD = 0;
	for ( i = 0; i < NUMTESTS; i++ ) {
		for ( j = 0; j < MIXBUFFER_SAMPLES*6; j++ ) {
			mixBuffer2[j] = origMixBuffer[j];
		}
		StartRecordTime( start );
		p_simd->MixSoundTwoSpeakerMono( mixBuffer2, samples, MIXBUFFER_SAMPLES, lastV, currentV );
		StopRecordTime( end );
		GetBest( start, end, bestClocksSIMD );
	}

	for ( i = 0; i < MIXBUFFER_SAMPLES*6; i++ ) {
		if ( idMath::Fabs( mixBuffer1[i] - mixBuffer2[i] ) > SOUND_MIX_EPSILON ) {
			break;
		}
	}
	result = ( i >= MIXBUFFER_SAMPLES*6 ) ? "ok" :  S_COLOR_RED "X";
	PrintClocks( va( "   simd->MixSoundTwoSpeakerMono() %s", result ), MIXBUFFER_SAMPLES, bestClocksSIMD, bestClocksGeneric );

	bestClocksGeneric = 0;
	for ( i = 0; i < NUMTESTS; i++ ) {
		for ( j = 0; j < MIXBUFFER_SAMPLES*6; j++ ) {
			mixBuffer1[j] = origMixBuffer[j];
		}
		StartRecordTime( start );
		p_generic->MixSoundTwoSpeakerStereo( mixBuffer1, samples, MIXBUFFER_SAMPLES, lastV, currentV );
		StopRecordTime( end );
		GetBest( start, end, bestClocksGeneric );
	}
	PrintClocks( "generic->MixSoundTwoSpeakerStereo()", MIXBUFFER_SAMPLES, bestClocksGeneric );


	bestClocksSIMD = 0;
	for ( i = 0; i < NUMTESTS; i++ ) {
		for ( j = 0; j < MIXBUFFER_SAMPLES*6; j++ ) {
			mixBuffer2[j] = origMixBuffer[j];
		}
		StartRecordTime( start );
		p_simd->MixSoundTwoSpeakerStereo( mixBuffer2, samples, MIXBUFFER_SAMPLES, lastV, currentV );
		StopRecordTime( end );
		GetBest( start, end, bestClocksSIMD );
	}

	for ( i = 0; i < MIXBUFFER_SAMPLES*6; i++ ) {
		if ( idMath::Fabs( mixBuffer1[i] - mixBuffer2[i] ) > SOUND_MIX_EPSILON ) {
			break;
		}
	}
	result = ( i >= MIXBUFFER_SAMPLES*6 ) ? "ok" :  S_COLOR_RED "X";
	PrintClocks( va( "   simd->MixSoundTwoSpeakerStereo() %s", result ), MIXBUFFER_SAMPLES, bestClocksSIMD, bestClocksGeneric );


	bestClocksGeneric = 0;
	for ( i = 0; i < NUMTESTS; i++ ) {
		for ( j = 0; j < MIXBUFFER_SAMPLES*6; j++ ) {
			mixBuffer1[j] = origMixBuffer[j];
		}
		StartRecordTime( start );
		p_generic->MixSoundSixSpeakerMono( mixBuffer1, samples, MIXBUFFER_SAMPLES, lastV, currentV );
		StopRecordTime( end );
		GetBest( start, end, bestClocksGeneric );
	}
	PrintClocks( "generic->MixSoundSixSpeakerMono()", MIXBUFFER_SAMPLES, bestClocksGeneric );


	bestClocksSIMD = 0;
	for ( i = 0; i < NUMTESTS; i++ ) {
		for ( j = 0; j < MIXBUFFER_SAMPLES*6; j++ ) {
			mixBuffer2[j] = origMixBuffer[j];
		}
		StartRecordTime( start );
		p_simd->MixSoundSixSpeakerMono( mixBuffer2, samples, MIXBUFFER_SAMPLES, lastV, currentV );
		StopRecordTime( end );
		GetBest( start, end, bestClocksSIMD );
	}

	for ( i = 0; i < MIXBUFFER_SAMPLES*6; i++ ) {
		if ( idMath::Fabs( mixBuffer1[i] - mixBuffer2[i] ) > SOUND_MIX_EPSILON ) {
			break;
		}
	}
	result = ( i >= MIXBUFFER_SAMPLES*6 ) ? "ok" :  S_COLOR_RED "X";
	PrintClocks( va( "   simd->MixSoundSixSpeakerMono() %s", result ), MIXBUFFER_SAMPLES, bestClocksSIMD, bestClocksGeneric );

	bestClocksGeneric = 0;
	for ( i = 0; i < NUMTESTS; i++ ) {
		for ( j = 0; j < MIXBUFFER_SAMPLES*6; j++ ) {
			mixBuffer1[j] = origMixBuffer[j];
		}
		StartRecordTime( start );
		p_generic->MixSoundSixSpeakerStereo( mixBuffer1, samples, MIXBUFFER_SAMPLES, lastV, currentV );
		StopRecordTime( end );
		GetBest( start, end, bestClocksGeneric );
	}
	PrintClocks( "generic->MixSoundSixSpeakerStereo()", MIXBUFFER_SAMPLES, bestClocksGeneric );


	bestClocksSIMD = 0;
	for ( i = 0; i < NUMTESTS; i++ ) {
		for ( j = 0; j < MIXBUFFER_SAMPLES*6; j++ ) {
			mixBuffer2[j] = origMixBuffer[j];
		}
		StartRecordTime( start );
		p_simd->MixSoundSixSpeakerStereo( mixBuffer2, samples, MIXBUFFER_SAMPLES, lastV, currentV );
		StopRecordTime( end );
		GetBest( start, end, bestClocksSIMD );
	}

	for ( i = 0; i < MIXBUFFER_SAMPLES*6; i++ ) {
		if ( idMath::Fabs( mixBuffer1[i] - mixBuffer2[i] ) > SOUND_MIX_EPSILON ) {
			break;
		}
	}
	result = ( i >= MIXBUFFER_SAMPLES*6 ) ? "ok" :  S_COLOR_RED "X";
	PrintClocks( va( "   simd->MixSoundSixSpeakerStereo() %s", result ), MIXBUFFER_SAMPLES, bestClocksSIMD, bestClocksGeneric );


	for ( i = 0; i < MIXBUFFER_SAMPLES*6; i++ ) {
		origMixBuffer[i] = srnd.RandomInt( (1<<17) ) - (1<<16);
	}

	bestClocksGeneric = 0;
	for ( i = 0; i < NUMTESTS; i++ ) {
		for ( j = 0; j < MIXBUFFER_SAMPLES*6; j++ ) {
			mixBuffer1[j] = origMixBuffer[j];
		}
		StartRecordTime( start );
		p_generic->MixedSoundToSamples( outSamples1, mixBuffer1, MIXBUFFER_SAMPLES*6 );
		StopRecordTime( end );
		GetBest( start, end, bestClocksGeneric );
	}
	PrintClocks( "generic->MixedSoundToSamples()", MIXBUFFER_SAMPLES, bestClocksGeneric );

	bestClocksSIMD = 0;
	for ( i = 0; i < NUMTESTS; i++ ) {
		for ( j = 0; j < MIXBUFFER_SAMPLES*6; j++ ) {
			mixBuffer2[j] = origMixBuffer[j];
		}
		StartRecordTime( start );
		p_simd->MixedSoundToSamples( outSamples2, mixBuffer2, MIXBUFFER_SAMPLES*6 );
		StopRecordTime( end );
		GetBest( start, end, bestClocksSIMD );
	}

	for ( i = 0; i < MIXBUFFER_SAMPLES*6; i++ ) {
		if ( outSamples1[i] != outSamples2[i] ) {
			break;
		}
	}
	result = ( i >= MIXBUFFER_SAMPLES*6 ) ? "ok" :  S_COLOR_RED "X";
	PrintClocks( va( "   simd->MixedSoundToSamples() %s", result ), MIXBUFFER_SAMPLES, bestClocksSIMD, bestClocksGeneric );
}

/*
============
TestMath
============
*/
void TestMath( void ) {
	int i;
	TIME_TYPE start, end, bestClocks;

	idLib::common->Printf("====================================\n" );

	float tst = -1.0f;
	float tst2 = 1.0f;
	float testvar = 1.0f;
	idRandom rnd;

	bestClocks = 0;
	tst = rnd.CRandomFloat();
	for ( i = 0; i < NUMTESTS; i++ ) {
		StartRecordTime( start );
		tst = fabs( tst );
		StopRecordTime( end );
		GetBest( start, end, bestClocks );
		testvar = ( testvar + tst ) * tst;
		tst = rnd.CRandomFloat();
	}
	PrintClocks( "            fabs( tst )", 1, bestClocks );

	bestClocks = 0;
	tst = rnd.CRandomFloat();
	for ( i = 0; i < NUMTESTS; i++ ) {
		StartRecordTime( start );
		int tmp = * ( int * ) &tst;
		tmp &= 0x7FFFFFFF;
		tst = * ( float * ) &tmp;
		StopRecordTime( end );
		GetBest( start, end, bestClocks );
		testvar = ( testvar + tst ) * tst;
		tst = rnd.CRandomFloat();
	}
	PrintClocks( "    idMath::Fabs( tst )", 1, bestClocks );

	bestClocks = 0;
	tst = 10.0f + 100.0f * rnd.RandomFloat();
	for ( i = 0; i < NUMTESTS; i++ ) {
		StartRecordTime( start );
		tst = sqrt( tst );
		StopRecordTime( end );
		GetBest( start, end, bestClocks );
		testvar = ( testvar + tst ) * tst * 0.01f;
		tst = 10.0f + 100.0f * rnd.RandomFloat();
	}
	PrintClocks( "            sqrt( tst )", 1, bestClocks );

	bestClocks = 0;
	tst = rnd.RandomFloat();
	for ( i = 0; i < NUMTESTS; i++ ) {
		StartRecordTime( start );
		tst = idMath::Sqrt( tst );
		StopRecordTime( end );
		GetBest( start, end, bestClocks );
		testvar = ( testvar + tst ) * tst;
		tst = rnd.RandomFloat();
	}
	PrintClocks( "    idMath::Sqrt( tst )", 1, bestClocks );

	bestClocks = 0;
	tst = rnd.RandomFloat();
	for ( i = 0; i < NUMTESTS; i++ ) {
		StartRecordTime( start );
		tst = idMath::Sqrt16( tst );
		StopRecordTime( end );
		GetBest( start, end, bestClocks );
		testvar = ( testvar + tst ) * tst;
		tst = rnd.RandomFloat();
	}
	PrintClocks( "  idMath::Sqrt16( tst )", 1, bestClocks );

	bestClocks = 0;
	tst = rnd.RandomFloat();
	for ( i = 0; i < NUMTESTS; i++ ) {
		StartRecordTime( start );
		tst = idMath::Sqrt64( tst );
		StopRecordTime( end );
		GetBest( start, end, bestClocks );
		testvar = ( testvar + tst ) * tst;
		tst = rnd.RandomFloat();
	}
	PrintClocks( "  idMath::Sqrt64( tst )", 1, bestClocks );

	bestClocks = 0;
	tst = rnd.RandomFloat();
	for ( i = 0; i < NUMTESTS; i++ ) {
		StartRecordTime( start );
		tst = tst * idMath::RSqrt( tst );
		StopRecordTime( end );
		GetBest( start, end, bestClocks );
		testvar = ( testvar + tst ) * tst;
		tst = rnd.RandomFloat();
	}
	PrintClocks( "   idMath::RSqrt( tst )", 1, bestClocks );

	bestClocks = 0;
	tst = rnd.CRandomFloat();
	for ( i = 0; i < NUMTESTS; i++ ) {
		StartRecordTime( start );
		tst = idMath::Sin( tst );
		StopRecordTime( end );
		GetBest( start, end, bestClocks );
		testvar = ( testvar + tst ) * tst;
		tst = rnd.CRandomFloat();
	}
	PrintClocks( "     idMath::Sin( tst )", 1, bestClocks );

	bestClocks = 0;
	tst = rnd.CRandomFloat();
	for ( i = 0; i < NUMTESTS; i++ ) {
		StartRecordTime( start );
		tst = idMath::Sin16( tst );
		StopRecordTime( end );
		GetBest( start, end, bestClocks );
		testvar = ( testvar + tst ) * tst;
		tst = rnd.CRandomFloat();
	}
	PrintClocks( "   idMath::Sin16( tst )", 1, bestClocks );

	bestClocks = 0;
	tst = rnd.CRandomFloat();
	for ( i = 0; i < NUMTESTS; i++ ) {
		StartRecordTime( start );
		tst = idMath::Cos( tst );
		StopRecordTime( end );
		GetBest( start, end, bestClocks );
		testvar = ( testvar + tst ) * tst;
		tst = rnd.CRandomFloat();
	}
	PrintClocks( "     idMath::Cos( tst )", 1, bestClocks );

	bestClocks = 0;
	tst = rnd.CRandomFloat();
	for ( i = 0; i < NUMTESTS; i++ ) {
		StartRecordTime( start );
		tst = idMath::Cos16( tst );
		StopRecordTime( end );
		GetBest( start, end, bestClocks );
		testvar = ( testvar + tst ) * tst;
		tst = rnd.CRandomFloat();
	}
	PrintClocks( "   idMath::Cos16( tst )", 1, bestClocks );

	bestClocks = 0;
	tst = rnd.CRandomFloat();
	for ( i = 0; i < NUMTESTS; i++ ) {
		StartRecordTime( start );
		idMath::SinCos( tst, tst, tst2 );
		StopRecordTime( end );
		GetBest( start, end, bestClocks );
		testvar = ( testvar + tst ) * tst;
		tst = rnd.CRandomFloat();
	}
	PrintClocks( "  idMath::SinCos( tst )", 1, bestClocks );

	bestClocks = 0;
	tst = rnd.CRandomFloat();
	for ( i = 0; i < NUMTESTS; i++ ) {
		StartRecordTime( start );
		idMath::SinCos16( tst, tst, tst2 );
		StopRecordTime( end );
		GetBest( start, end, bestClocks );
		testvar = ( testvar + tst ) * tst;
		tst = rnd.CRandomFloat();
	}
	PrintClocks( "idMath::SinCos16( tst )", 1, bestClocks );

	bestClocks = 0;
	tst = rnd.CRandomFloat();
	for ( i = 0; i < NUMTESTS; i++ ) {
		StartRecordTime( start );
		tst = idMath::Tan( tst );
		StopRecordTime( end );
		GetBest( start, end, bestClocks );
		testvar = ( testvar + tst ) * tst;
		tst = rnd.CRandomFloat();
	}
	PrintClocks( "     idMath::Tan( tst )", 1, bestClocks );

	bestClocks = 0;
	tst = rnd.CRandomFloat();
	for ( i = 0; i < NUMTESTS; i++ ) {
		StartRecordTime( start );
		tst = idMath::Tan16( tst );
		StopRecordTime( end );
		GetBest( start, end, bestClocks );
		testvar = ( testvar + tst ) * tst;
		tst = rnd.CRandomFloat();
	}
	PrintClocks( "   idMath::Tan16( tst )", 1, bestClocks );

	bestClocks = 0;
	tst = rnd.CRandomFloat();
	for ( i = 0; i < NUMTESTS; i++ ) {
		StartRecordTime( start );
		tst = idMath::ASin( tst );
		StopRecordTime( end );
		GetBest( start, end, bestClocks );
		testvar = ( testvar + tst ) * tst * ( 1.0f / idMath::PI );
		tst = rnd.CRandomFloat();
	}
	PrintClocks( "    idMath::ASin( tst )", 1, bestClocks );

	bestClocks = 0;
	tst = rnd.CRandomFloat();
	for ( i = 0; i < NUMTESTS; i++ ) {
		StartRecordTime( start );
		tst = idMath::ASin16( tst );
		StopRecordTime( end );
		GetBest( start, end, bestClocks );
		testvar = ( testvar + tst ) * tst * ( 1.0f / idMath::PI );
		tst = rnd.CRandomFloat();
	}
	PrintClocks( "  idMath::ASin16( tst )", 1, bestClocks );

	bestClocks = 0;
	tst = rnd.CRandomFloat();
	for ( i = 0; i < NUMTESTS; i++ ) {
		StartRecordTime( start );
		tst = idMath::ACos( tst );
		StopRecordTime( end );
		GetBest( start, end, bestClocks );
		testvar = ( testvar + tst ) * tst * ( 1.0f / idMath::PI );
		tst = rnd.CRandomFloat();
	}
	PrintClocks( "    idMath::ACos( tst )", 1, bestClocks );

	bestClocks = 0;
	tst = rnd.CRandomFloat();
	for ( i = 0; i < NUMTESTS; i++ ) {
		StartRecordTime( start );
		tst = idMath::ACos16( tst );
		StopRecordTime( end );
		GetBest( start, end, bestClocks );
		testvar = ( testvar + tst ) * tst * ( 1.0f / idMath::PI );
		tst = rnd.CRandomFloat();
	}
	PrintClocks( "  idMath::ACos16( tst )", 1, bestClocks );

	bestClocks = 0;
	tst = rnd.CRandomFloat();
	for ( i = 0; i < NUMTESTS; i++ ) {
		StartRecordTime( start );
		tst = idMath::ATan( tst );
		StopRecordTime( end );
		GetBest( start, end, bestClocks );
		testvar = ( testvar + tst ) * tst;
		tst = rnd.CRandomFloat();
	}
	PrintClocks( "    idMath::ATan( tst )", 1, bestClocks );

	bestClocks = 0;
	tst = rnd.CRandomFloat();
	for ( i = 0; i < NUMTESTS; i++ ) {
		StartRecordTime( start );
		tst = idMath::ATan16( tst );
		StopRecordTime( end );
		GetBest( start, end, bestClocks );
		testvar = ( testvar + tst ) * tst;
		tst = rnd.CRandomFloat();
	}
	PrintClocks( "  idMath::ATan16( tst )", 1, bestClocks );

	bestClocks = 0;
	tst = rnd.CRandomFloat();
	for ( i = 0; i < NUMTESTS; i++ ) {
		StartRecordTime( start );
		tst = idMath::Pow( 2.7f, tst );
		StopRecordTime( end );
		GetBest( start, end, bestClocks );
		testvar = ( testvar + tst ) * tst * 0.1f;
		tst = rnd.CRandomFloat();
	}
	PrintClocks( "    idMath::Pow( tst )", 1, bestClocks );

	bestClocks = 0;
	tst = rnd.CRandomFloat();
	for ( i = 0; i < NUMTESTS; i++ ) {
		StartRecordTime( start );
		tst = idMath::Pow16( 2.7f, tst );
		StopRecordTime( end );
		GetBest( start, end, bestClocks );
		testvar = ( testvar + tst ) * tst * 0.1f;
		tst = rnd.CRandomFloat();
	}
	PrintClocks( "  idMath::Pow16( tst )", 1, bestClocks );

	bestClocks = 0;
	tst = rnd.CRandomFloat();
	for ( i = 0; i < NUMTESTS; i++ ) {
		StartRecordTime( start );
		tst = idMath::Exp( tst );
		StopRecordTime( end );
		GetBest( start, end, bestClocks );
		testvar = ( testvar + tst ) * tst * 0.1f;
		tst = rnd.CRandomFloat();
	}
	PrintClocks( "    idMath::Exp( tst )", 1, bestClocks );

	bestClocks = 0;
	tst = rnd.CRandomFloat();
	for ( i = 0; i < NUMTESTS; i++ ) {
		StartRecordTime( start );
		tst = idMath::Exp16( tst );
		StopRecordTime( end );
		GetBest( start, end, bestClocks );
		testvar = ( testvar + tst ) * tst * 0.1f;
		tst = rnd.CRandomFloat();
	}
	PrintClocks( "  idMath::Exp16( tst )", 1, bestClocks );

	bestClocks = 0;
	tst = rnd.CRandomFloat();
	for ( i = 0; i < NUMTESTS; i++ ) {
		tst = fabs( tst ) + 1.0f;
		StartRecordTime( start );
		tst = idMath::Log( tst );
		StopRecordTime( end );
		GetBest( start, end, bestClocks );
		testvar = ( testvar + tst ) * tst;
		tst = rnd.CRandomFloat();
	}
	PrintClocks( "    idMath::Log( tst )", 1, bestClocks );

	bestClocks = 0;
	tst = rnd.CRandomFloat();
	for ( i = 0; i < NUMTESTS; i++ ) {
		tst = fabs( tst ) + 1.0f;
		StartRecordTime( start );
		tst = idMath::Log16( tst );
		StopRecordTime( end );
		GetBest( start, end, bestClocks );
		testvar = ( testvar + tst ) * tst;
		tst = rnd.CRandomFloat();
	}
	PrintClocks( "  idMath::Log16( tst )", 1, bestClocks );

	idLib::common->Printf( "testvar = %f\n", testvar );
}

/*
============
TestNegate
============
*/

// this wasn't previously in the test
void TestNegate( void ) {
	int i;
	TIME_TYPE start, end, bestClocksGeneric, bestClocksSIMD;
	ALIGN16( float fsrc0[COUNT] );
	ALIGN16( float fsrc1[COUNT] );
	ALIGN16( float fsrc2[COUNT] );

	const char *result;

	idRandom srnd( RANDOM_SEED );

	for ( i = 0; i < COUNT; i++ ) {
		fsrc0[i] = fsrc1[i] = fsrc2[i] = srnd.CRandomFloat() * 10.0f;
		//fsrc1[i] = srnd.CRandomFloat() * 10.0f;
	}

	idLib::common->Printf("====================================\n" );

	bestClocksGeneric = 0;
	for ( i = 0; i < NUMTESTS; i++ ) {

		memcpy( &fsrc1[0], &fsrc0[0], COUNT * sizeof(float) );

		StartRecordTime( start );
		p_generic->Negate16( fsrc1, COUNT );
		StopRecordTime( end );
		GetBest( start, end, bestClocksGeneric );
	}
	PrintClocks( "generic->Negate16( float[] )", COUNT, bestClocksGeneric );

	bestClocksSIMD = 0;
	for ( i = 0; i < NUMTESTS; i++ ) {

		memcpy( &fsrc2[0], &fsrc0[0], COUNT * sizeof(float) );

		StartRecordTime( start );
		p_simd->Negate16( fsrc2, COUNT );
		StopRecordTime( end );
		GetBest( start, end, bestClocksSIMD );
	}

	for ( i = 0; i < COUNT; i++ ) {
		if ( fsrc1[i] != fsrc2[i] ) {
			break;
		}
	}
	result = ( i >= COUNT ) ? "ok" :  S_COLOR_RED "X";
	PrintClocks( va( "   simd->Negate16( float[] ) %s", result ), COUNT, bestClocksSIMD, bestClocksGeneric );
}