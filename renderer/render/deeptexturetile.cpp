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

#include <numeric>

#include "deeptexturetile.h"
#include "exception.h"

namespace Aqsis
{

CqDeepTextureTile::CqDeepTextureTile(const boost::shared_array<TqFloat> data, 
		const boost::shared_array<TqUint> funcOffsets, const TqUint width, 
		const TqUint height, const TqUint topLeftX, const TqUint topLeftY, const TqUint colorChannels ) :
			m_data( data ),
			m_funcOffsets( funcOffets ),
			m_width( width ),
			m_height( height ),
			m_topLeftX( topLeftX ),
			m_topLeftY( topLeftY ),
			m_colorChannels( colorChannels )
{}

//------------------------------------------------------------------------------
} // namespace Aqsis
