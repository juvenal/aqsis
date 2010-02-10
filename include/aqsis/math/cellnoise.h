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
		\brief Declares the CqCellNoise class fro producing cellular noise.
		\author Paul C. Gregory (pgregory@aqsis.org)
*/

//? Is .h included already?
#ifndef CELLNOISE_H_INCLUDED
#define CELLNOISE_H_INCLUDED 1

#include	<aqsis/aqsis.h>

#include	<aqsis/math/vector3d.h>

namespace Aqsis {

//----------------------------------------------------------------------
/** \class CqCellNoise
 * Class encapsulating the functionality of the shader cell noise functions.
 */

class AQSIS_MATH_SHARE CqCellNoise
{
	public:
		// Default constructor.
		CqCellNoise()
		{}
		~CqCellNoise()
		{}

		TqFloat	FCellNoise1( TqFloat u );
		TqFloat	FCellNoise2( TqFloat u, TqFloat v );
		TqFloat	FCellNoise3( const Imath::V3f& P );
		TqFloat	FCellNoise4( const Imath::V3f& P, TqFloat v );

		Imath::V3f	PCellNoise1( TqFloat u );
		Imath::V3f	PCellNoise2( TqFloat u, TqFloat v );
		Imath::V3f	PCellNoise3( const Imath::V3f& P );
		Imath::V3f	PCellNoise4( const Imath::V3f& P, TqFloat v );

	private:
		static TqInt	m_PermuteTable[ 2*2048 ];		///< static permutation table.
		static TqFloat	m_RandomTable[ 2048 ];		///< static random table.
}
;


//-----------------------------------------------------------------------

} // namespace Aqsis

#endif	// !CELLNOISE_H_INCLUDED
