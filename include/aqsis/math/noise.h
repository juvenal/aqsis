// Aqsis
// Copyright (C) 1997 - 2001, Paul C. Gregory
//
// Contact: pgregory@aqsis.org
//
// This library is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public
// License as published by the Free Software Foundation; either
// version 2 of the License, or (at your option) any later version.
//
// This library is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
// General Public License for more details.
//
// You should have received a copy of the GNU General Public
// License along with this library; if not, write to the Free Software
// Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA


/** \file
		\brief Declares the CqNoise class for producing Perlin noise.
		\author Paul C. Gregory (pgregory@aqsis.org)
		\author Stefan Gustavson (stegu@itn.liu.se)
*/

//? Is .h included already?
#ifndef NOISE_H_INCLUDED
#define NOISE_H_INCLUDED 1

#include	<aqsis/aqsis.h>

#include	<ImathVec.h>

#include	<aqsis/math/color.h>

namespace Aqsis {

//----------------------------------------------------------------------
/** \class CqNoise
 * Wrapper class for CqNoise1234, to produce float and vector Perlin noise
 * over 1D to 4D domains.
 */

class AQSIS_MATH_SHARE CqNoise
{
	public:

		CqNoise()
		{}

		~CqNoise()
		{}

		// These are functions with "SL-friendly" parameter lists and return types,
		// which each invoke one or three calls to one of the functions in CqNoise1234.

		static	TqFloat	FGNoise1( TqFloat x );
		static  TqFloat FGPNoise1( TqFloat x, TqFloat px );
		static	TqFloat	FGNoise2( TqFloat x, TqFloat y );
		static	TqFloat	FGPNoise2( TqFloat x, TqFloat y, TqFloat px, TqFloat py );
		static	TqFloat	FGNoise3( const Imath::V3f& v );
		static	TqFloat	FGPNoise3( const Imath::V3f& v, const Imath::V3f& pv );
		static	TqFloat	FGNoise4( const Imath::V3f& v, const TqFloat t );
		static	TqFloat	FGPNoise4( const Imath::V3f& v, const TqFloat t, const Imath::V3f& pv, const TqFloat pt );
		static	Imath::V3f	PGNoise1( TqFloat x );
		static	Imath::V3f	PGPNoise1( TqFloat x, TqFloat px );
		static	Imath::V3f	PGNoise2( TqFloat x, TqFloat y );
		static	Imath::V3f	PGPNoise2( TqFloat x, TqFloat y, TqFloat px, TqFloat py );
		static	Imath::V3f	PGNoise3( const Imath::V3f& v );
		static	Imath::V3f	PGPNoise3( const Imath::V3f& v, const Imath::V3f& pv );
		static	Imath::V3f	PGNoise4( const Imath::V3f& v, TqFloat t );
		static	Imath::V3f	PGPNoise4( const Imath::V3f& v, TqFloat t, const Imath::V3f& pv, TqFloat pt );
		static	CqColor	CGNoise1( TqFloat x );
		static	CqColor	CGPNoise1( TqFloat x, TqFloat px );
		static	CqColor	CGNoise2( TqFloat x, TqFloat y );
		static	CqColor	CGPNoise2( TqFloat x, TqFloat y, TqFloat px, TqFloat py );
		static	CqColor	CGNoise3( const Imath::V3f& v );
		static	CqColor	CGPNoise3( const Imath::V3f& v, const Imath::V3f& pv );
		static	CqColor	CGNoise4( const Imath::V3f& v, TqFloat t );
		static	CqColor	CGPNoise4( const Imath::V3f& v, TqFloat t, const Imath::V3f& pv, TqFloat pt );

};

//-----------------------------------------------------------------------

} // namespace Aqsis

#endif	// !NOISE_H_INCLUDED
