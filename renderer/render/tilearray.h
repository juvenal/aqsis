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
 * \author Zachary Carter (zcarter@aqsis.org)
 *
 * \brief Defines a container for tiles which can load tiles and cache them, and return to the user a pixel at given coordinates.
 */
#ifndef TILEARRAY_H_INCLUDED
#define TILEARRAY_H_INCLUDED

// Aqsis primary header
#include "aqsis.h"

// Standard libraries
#include <string>
#include <iostream>
#include <vector>
#include <map>
#include <tiff.h> //< Including (temporarily) in order to get the typedefs like uint32

// Other Aqsis headers
#include "dtexinput.h"
#include "deeptexturetile.h"

// External libraries
#include <boost/shared_ptr.hpp>
#include <boost/intrusive_ptr.hpp>

namespace Aqsis
{

//------------------------------------------------------------------------------
/** \brief A container for deep tiles; requests new tiles when needed.
 * 
 * Can return a pixel (visibility function) at given (i,j) coordinates.
 * Manages tile caching.
 */
class CqDeepTileArray
{
	public:
		/** \brief Construct an instance of CqDeepTileArray
		 *
		 * \param filename - The full path and file name of the dtex file to open and read from.
		 */
		CqDeepTileArray( std::string filename );
		virtual ~CqDeepTileArray(){};
	  
		/** \brief (This is just a thought) Identify the tile that contains the requested pixel. 
		 * If the tile is already cached, return the visibility function for the desired pixel.
		 * If the tile is not in cache, load it from file, evicting the least recently used cached 
		 * tile if the cache is full, and proceeding normaly thereafter.
		 *
		 * \param x - Image x-coordinate of the pixel desired. 
		 * 			We load the entire enclosing tile to take advantage of spatial locality
		 * \param y - Image y-coordinate of the pixel desired.
		 * \return A const pointer to the visibility function for the requested pixel.
		 */
		const TqVisFuncPtr visibilityFunctionAtPixel( const TqUint x, const TqUint y );
		
	private:
		
		// Functions
		
		// Data
		/// Key to use when finding tiles in std::maps.
		/// Index like so: tileKey[topLeftY][topLeftX]
		/// For example: tileKey[0][64] identifies the tile rooted with its top left pixel at
		/// image coordinates (0,64).
		typedef std::pair<TqUint, TqUint> TileKey;

		// The map is indexed with a TileKey: a tile ID, which is a pair (topLeftX, topLeftY) 
		// that can identify the region of the larger picture covered by a particular tile.
		std::map<TileKey, boost::shared_ptr<CqDeepTextureTile> > m_hotTileMap;
		
		// The source from which we load tiles as they are needed
		CqDeepTexInputFile m_deepTextureInputFile;
};

//------------------------------------------------------------------------------
} // namespace Aqsis

#endif // TILEARRAY_H_INCLUDED
