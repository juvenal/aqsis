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
#include "iddmanager.h" //< Include because I want access to the types, TqVisibilityFunction and SqVisibilityNode, defined therein

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
		CqDeepTileArray( std::string textureFileName );
		virtual ~CqDeepTileArray(){};
	  
		/** \brief (This is just a thought) Identify the ID of the tile that contains the requested pixel. If the tile is already cached, return the visibility function
		 * for the desired pixel and mark the tile as freshly used (so that it will not be evicted too soon), and increment the age of other cached tiles.
		 * If the tile is not in cache, load it from file, evicting the least recently used cached tile if the cache is full, and proceeding normaly thereafter.
		 *
		 * \param x - Image x-coordinate of the pixel desired. We load the entire enclosing tile because of the spatial locality heuristic; it is likely to be needed again soon.
		 * \param y - Image y-coordinate of the pixel desired. We load the entire enclosing tile because of the spatial locality heuristic; it is likely to be needed again soon.
		 * \param functionLength - A reference to a variable in which we may store the length, measured in number of nodes, of the returned visibility function.
		 * \return A const pointer to the visibility function for the requested pixel.
		 */
		// Instead, use 2 functions: one to get the data, and another to gett the function length
		const TqFloat* VisibilityFunctionAtPixel( const TqUint x, const TqUint y, TqUint& functionLength );
		// Note: Below is a possible function prototype that would require us to rebuild the original visibility functions using SqVisibilityNodes.
		// We would want to change the storage in CqDeepTextureTile to keep the data in that form, since otherwise we would be building copies of the data in this form for every function call.
		// The easier/faster thing to do, however, is to use the prototype above and simply keep the data in an array. The problem is that this potentially gives the caller access to more than the 
		// requested data.
		//virtual boost::shared_ptr<TqVisibilityFunction> VisibilityFunctionAtPixel( const TqUint x, const TqUint y );
		
	private:
		
		// Functions
		
		// Data
		/// Key to use when finding tiles in std::maps.
		typedef std::pair<TqUint, TqUint> TileKey;

		// The map is indexed with a TileKey: a tile ID, which is a pair (topLeftX, topLeftY) 
		// that can identify the region of the larger picture covered by a particular tile.
		std::map<TileKey, boost::shared_ptr<CqDeepTextureTile> > m_hotTileMap;
		
		
		CqDeepTexInputFile m_deepTextureInputFile;
		
};

//------------------------------------------------------------------------------
} // namespace Aqsis

#endif // TILEARRAY_H_INCLUDED
