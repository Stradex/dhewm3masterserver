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

#ifndef __MATH_SIMD_GENERIC_H__
#define __MATH_SIMD_GENERIC_H__

#include "idlib/math/Simd.h"

/*
===============================================================================

	Generic implementation of idSIMDProcessor

===============================================================================
*/

class idSIMD_Generic : public idSIMDProcessor {
public:
	virtual const char * VPCALL GetName( void ) const;

	virtual void VPCALL Add( float *dst,			const float constant,	const float *src,		const int count );
	virtual void VPCALL Add( float *dst,			const float *src0,		const float *src1,		const int count );
	virtual void VPCALL Sub( float *dst,			const float constant,	const float *src,		const int count );
	virtual void VPCALL Sub( float *dst,			const float *src0,		const float *src1,		const int count );
	virtual void VPCALL Mul( float *dst,			const float constant,	const float *src,		const int count );
	virtual void VPCALL Mul( float *dst,			const float *src0,		const float *src1,		const int count );
	virtual void VPCALL Div( float *dst,			const float constant,	const float *src,		const int count );
	virtual void VPCALL Div( float *dst,			const float *src0,		const float *src1,		const int count );
	virtual void VPCALL MulAdd( float *dst,			const float constant,	const float *src,		const int count );
	virtual void VPCALL MulAdd( float *dst,			const float *src0,		const float *src1,		const int count );
	virtual void VPCALL MulSub( float *dst,			const float constant,	const float *src,		const int count );
	virtual void VPCALL MulSub( float *dst,			const float *src0,		const float *src1,		const int count );

	virtual void VPCALL Dot( float *dst,			const idVec3 &constant,	const idVec3 *src,		const int count );
	virtual void VPCALL Dot( float *dst,			const idVec3 &constant,	const idPlane *src,		const int count );
	virtual void VPCALL Dot( float *dst,			const idPlane &constant,const idVec3 *src,		const int count );
	virtual void VPCALL Dot( float *dst,			const idPlane &constant,const idPlane *src,		const int count );
	virtual void VPCALL Dot( float *dst,			const idVec3 *src0,		const idVec3 *src1,		const int count );
	virtual void VPCALL Dot( float &dot,			const float *src1,		const float *src2,		const int count );

	virtual void VPCALL CmpGT( byte *dst,			const float *src0,		const float constant,	const int count );
	virtual void VPCALL CmpGT( byte *dst,			const byte bitNum,		const float *src0,		const float constant,	const int count );
	virtual void VPCALL CmpGE( byte *dst,			const float *src0,		const float constant,	const int count );
	virtual void VPCALL CmpGE( byte *dst,			const byte bitNum,		const float *src0,		const float constant,	const int count );
	virtual void VPCALL CmpLT( byte *dst,			const float *src0,		const float constant,	const int count );
	virtual void VPCALL CmpLT( byte *dst,			const byte bitNum,		const float *src0,		const float constant,	const int count );
	virtual void VPCALL CmpLE( byte *dst,			const float *src0,		const float constant,	const int count );
	virtual void VPCALL CmpLE( byte *dst,			const byte bitNum,		const float *src0,		const float constant,	const int count );

	virtual void VPCALL MinMax( float &min,			float &max,				const float *src,		const int count );
	virtual	void VPCALL MinMax( idVec2 &min,		idVec2 &max,			const idVec2 *src,		const int count );
	virtual void VPCALL MinMax( idVec3 &min,		idVec3 &max,			const idVec3 *src,		const int count );

	virtual void VPCALL Clamp( float *dst,			const float *src,		const float min,		const float max,		const int count );
	virtual void VPCALL ClampMin( float *dst,		const float *src,		const float min,		const int count );
	virtual void VPCALL ClampMax( float *dst,		const float *src,		const float max,		const int count );

	virtual void VPCALL Memcpy( void *dst,			const void *src,		const int count );
	virtual void VPCALL Memset( void *dst,			const int val,			const int count );

	virtual void VPCALL Zero16( float *dst,			const int count );
	virtual void VPCALL Negate16( float *dst,		const int count );
	virtual void VPCALL Copy16( float *dst,			const float *src,		const int count );
	virtual void VPCALL Add16( float *dst,			const float *src1,		const float *src2,		const int count );
	virtual void VPCALL Sub16( float *dst,			const float *src1,		const float *src2,		const int count );
	virtual void VPCALL Mul16( float *dst,			const float *src1,		const float constant,	const int count );
	virtual void VPCALL AddAssign16( float *dst,	const float *src,		const int count );
	virtual void VPCALL SubAssign16( float *dst,	const float *src,		const int count );
	virtual void VPCALL MulAssign16( float *dst,	const float constant,	const int count );

	virtual void VPCALL MatX_MultiplyVecX( idVecX &dst, const idMatX &mat, const idVecX &vec );
	virtual void VPCALL MatX_MultiplyAddVecX( idVecX &dst, const idMatX &mat, const idVecX &vec );
	virtual void VPCALL MatX_MultiplySubVecX( idVecX &dst, const idMatX &mat, const idVecX &vec );
	virtual void VPCALL MatX_TransposeMultiplyVecX( idVecX &dst, const idMatX &mat, const idVecX &vec );
	virtual void VPCALL MatX_TransposeMultiplyAddVecX( idVecX &dst, const idMatX &mat, const idVecX &vec );
	virtual void VPCALL MatX_TransposeMultiplySubVecX( idVecX &dst, const idMatX &mat, const idVecX &vec );
	virtual void VPCALL MatX_MultiplyMatX( idMatX &dst, const idMatX &m1, const idMatX &m2 );
	virtual void VPCALL MatX_TransposeMultiplyMatX( idMatX &dst, const idMatX &m1, const idMatX &m2 );
	virtual void VPCALL MatX_LowerTriangularSolve( const idMatX &L, float *x, const float *b, const int n, int skip = 0 );
	virtual void VPCALL MatX_LowerTriangularSolveTranspose( const idMatX &L, float *x, const float *b, const int n );
	virtual bool VPCALL MatX_LDLTFactor( idMatX &mat, idVecX &invDiag, const int n );

	virtual void VPCALL UpSamplePCMTo44kHz( float *dest, const short *pcm, const int numSamples, const int kHz, const int numChannels );
	virtual void VPCALL UpSampleOGGTo44kHz( float *dest, const float * const *ogg, const int numSamples, const int kHz, const int numChannels );
	virtual void VPCALL MixSoundTwoSpeakerMono( float *mixBuffer, const float *samples, const int numSamples, const float lastV[2], const float currentV[2] );
	virtual void VPCALL MixSoundTwoSpeakerStereo( float *mixBuffer, const float *samples, const int numSamples, const float lastV[2], const float currentV[2] );
	virtual void VPCALL MixSoundSixSpeakerMono( float *mixBuffer, const float *samples, const int numSamples, const float lastV[6], const float currentV[6] );
	virtual void VPCALL MixSoundSixSpeakerStereo( float *mixBuffer, const float *samples, const int numSamples, const float lastV[6], const float currentV[6] );
	virtual void VPCALL MixedSoundToSamples( short *samples, const float *mixBuffer, const int numSamples );
};

#endif /* !__MATH_SIMD_GENERIC_H__ */
