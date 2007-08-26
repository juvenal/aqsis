// Aqsis
// Copyright (C) 1997 - 2007, Paul C. Gregory
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
 * 
 * \author Zachary L. Carter (zcarter@aqsis.org)
 * 
 * \brief Implementation of deep texture tile class.
 */

#include "deeptexturetile.h"
#include "exception.h"

namespace Aqsis
{

CqDeepTextureTile::CqDeepTextureTile(const boost::shared_array<TqFloat> data, 
		const boost::shared_array<TqInt> funcOffsets, const TqUint width, 
		const TqUint height, const TqUint topLeftX, const TqUint topLeftY, const TqUint colorChannels ) :
			m_data( data ),
			m_funcOffsets( funcOffsets ),
			m_width( width ),
			m_height( height ),
			m_topLeftX( topLeftX ),
			m_topLeftY( topLeftY ),
			m_colorChannels( colorChannels ),
			m_flagEmpty( (funcOffsets[0] == -1 ? true : false) )
{}

CqDeepTextureTile::CqDeepTextureTile( const TqFloat* data, const TqInt* funcOffsets, TqUint xmin, TqUint ymin,
		TqUint xmaxplus1, TqUint ymaxplus1, TqUint colorChannels ) :
			m_data( ), //< initialized below 
			m_funcOffsets( ), //< initialized below
			m_width( xmaxplus1-xmin ),
			m_height( ymaxplus1-ymin ),
			m_topLeftX( xmin ),
			m_topLeftY( ymin ),
			m_colorChannels( colorChannels ),
			m_flagEmpty( (funcOffsets[0] == -1 ? true : false) )			
{
	// If the tile is not empty, copy the data. Otherwise do nothing.
	if ( !m_flagEmpty )
	{
		// Allocate memory for function offsets
		m_funcOffsets = boost::shared_array<TqInt>(new TqInt[m_width*m_height+1]);
		// Copy function offsets
		memcpy( m_funcOffsets.get(), funcOffsets, (m_width*m_height+1)*sizeof(TqInt) );
		// Allocate memory for data
		m_data = boost::shared_array<TqFloat>(new TqFloat[m_funcOffsets[m_width*m_height+1]*(colorChannels+1)]);
		// Copy data
		memcpy( m_data.get(), data, m_funcOffsets[m_width*m_height+1]*(colorChannels+1)*sizeof(TqFloat));
	}
}

//------------------------------------------------------------------------------
} // namespace Aqsis
