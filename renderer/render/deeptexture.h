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
 * \brief Mipmapped deep textures and associated facilities.
 */
#ifndef DEEPTEXTURE_H_INCLUDED
#define DEEPTEXTURE_H_INCLUDED

// Aqsis primary header
#include "aqsis.h"

// Standard libraries
#include <string>
#include <iostream>
#include <vector>
#include <tiff.h> //< Including (temporarily) in order to get the typedefs like uint32

// Other Aqsis headers
#include "tilearray.h"
#include "color.h"
#include "iddmanager.h" //< Include because I want access to the types, TqVisibilityFunction and SqVisibilityNode, defined therein

// External libraries
#include <boost/shared_ptr.hpp>

namespace Aqsis
{

//------------------------------------------------------------------------------
/** \brief A class to manage a single mipmap level of a deep shadow map.
 * 
 * Holds a CqDeepTileArray for access to the underlying data.
 * Does texture filtering (We'll have to think about what filter types to implement... possibly Monte Carlo)
 */
class CqDeepMipmapLevel
{
	public:
		CqDeepMipmapLevel( std::string textureFileName );
		virtual ~CqDeepMipmapLevel(){};
	  
		/** \brief Identify the ID of the tile that contains the requested pixel. If the tile is already cached, return the visibility function
		 * for the desired pixel and mark the tile as freshly used (so that it will not be evicted too soon), and increment the age of other cached tiles.
		 * If the tile is not in cache, load it from file, evicting the least recently used cached tile if the cache is full, and proceeding normalyy from there.
		 *
		 * \param x - Image x-coordinate to which the scene point is projected on the shadow caster's image plane.
		 * \param y - Image y-coordinate to which the scene point is projected on the shadow caster's image plane.
		 * \param depth - Scene depth, measured from the shadow caster's position, of the point we seek shadow information for.
		 * \return A color representing the visibility at the requested point. 
		 */
		virtual CqColor VisibilityAt( const int x, const int y, const float depth );
		
	private:
		
		// Functions
		void filterFunction();

		// Data
		CqDeepTileArray m_deepTileArray;
		
};

//------------------------------------------------------------------------------
/** \brief The hieghest level deep texture map abstraction.
 * 
 * Holds a set of CqDeepMipmapLevel (one for each level; probably instantiation on read)
 * Has some sort of interface which the shading language can plug into.
 */
class CqDeepTexture
{
	public:
		
		CqDeepTexture( std::string textureFileName );
		virtual ~CqDeepTexture(){};
	
	private:
		
		// Functions
		
		// Data
		std::vector<CqDeepMipmapLevel> m_MipmapSet;
};

//------------------------------------------------------------------------------
} // namespace Aqsis

#endif // TILEARRAY_H_INCLUDED
