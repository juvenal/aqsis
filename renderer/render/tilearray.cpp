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
 * \brief Implementation of a deep texture file reader to load deep data tiles from file into memory.
 */

#include <numeric>

#include "tilearray.h"

namespace Aqsis
{

CqDeepTileArray::CqDeepTileArray( std::string textureFileName ) :
	m_hotTileMap(),
	m_deepTextureInputFile(textureFileName)
{}

//boost::shared_ptr<TqVisibilityFunction> CqDeepTileArray::VisibilityFunctionAtPixel( const TqUint x, const TqUint y )
const TqFloat* VisibilityFunctionAtPixel( const TqUint x, const TqUint y, TqUint& functionLength );
{
	// Identify the tile the requested pixel belongs to
	const TqUint homeTileCol = x/m_deepTextureInputFile.imageWidth();
	const TqUint homeTileRow = y/m_deepTextureInputFile.imageHeight();
	const TqUint homeTileXmin = homeTileCol*m_deepTextureInputFile.standardTileWidth(); 
	const TqUint homeTileYmin = homeTileRow*m_deepTextureInputFile.standardTileHeight();
	const TileKey homeTileKey(homeTileXmin, homeTileYmin);
	const TqUint homeTileX = x-homeTileXmin;
	const TqUint homeTileY = y-tomeTileYmin;
	
	// Check if the tile is cached. If so, use it, otherwise first load it, then use it.
	if (m_hotTileMap.count(homeTileKey) == 0)
	{
		// Tile is not cached, so load it.
		m_hotTileMap[hoemTileKey] = m_deepTextureInputFile.LoadTileForPixel( x, y );
	}
	functionLength = m_hotTileMap[hoemTileKey]->functionLengthOfPixel( homeTileX, homeTileY );
	return m_hotTileMap[hoemTileKey]->visibilityFunctionAtPixel( homeTileX, homeTileY );
}

//------------------------------------------------------------------------------
} // namespace Aqsis
