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
 * \brief Implementation of CqDeepMipmapLevel and CqDeepTexture classes.
 */

#include "deeptexture.h"

namespace Aqsis
{

// Begin CqDeepMipmapLevel implementation

CqDeepMipmapLevel::CqDeepMipmapLevel( std::string filename ) :
	m_deepTileArray( filename )
{

}

CqColor CqDeepMipmapLevel::visibilityAt( const int x, const int y, const float depth )
{
	return gColWhite;
}

//------------------------------------------------------------------------------
// Begin CqDeepTexture implementation

CqDeepTexture::CqDeepTexture( std::string filename )
{
	/// \toDo Instantiate a set of CqDeepMipmapLevel objects, one for each mipmap level.
	// Note: there needs to be a design change. Currently we create a instance of 
	// CqDeepMipmapLevel, which creates a CqTileArray, which in turn create a 
	// CqDeepDtexFileInput, which attemps to open a dsm file. This way we try to open the
	// file several time, but we should only open it once.
	boost::shared_ptr<CqDeepMipmapLevel> newMipmapLevel( new CqDeepMipmapLevel( filename ));
	m_MipmapSet.push_back(newMipmapLevel);
}

//------------------------------------------------------------------------------
} // namespace Aqsis
