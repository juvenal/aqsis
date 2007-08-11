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
 * \brief Implementation of a deep texture file reader class to load deep texture tiles from an Aqsis dtex file into memory.
 */
#ifndef DTEXINPUT_H_INCLUDED
#define DTEXINPUT_H_INCLUDED 1

//Aqsis primary header
#include "aqsis.h"

// Standard libraries
#include <string>
#include <iostream>
#include <fstream>
#include <vector>
#include <map>
#include <tiff.h> //< Including in order to get the typedefs like uint32

// External libraries
#include <boost/shared_ptr.hpp>

//------------------------------------------------------------------------------
/** \brief A structure to store a deep data tile loaded from a dtex file.
 *
 */
struct SqDeepDataTile
{
	/// The length (in units of number of nodes)
	/// of each visibility function in the tile. A particle function N may be indexed at position (sum of function lengths up to, but excluding N).
	/// The sum of all elements can be used to determine the size in bytes of the tile: 
	/// May be expressed as: std::accumulate(functionOffsets.begin(), functionOffsets.end(), 0);
	/// The number of elements in this vector must be equal to (tileWidth*tileHeight),
	/// so there is a length field for every pixel, empty or not.
	<std::vector<int> functionLengths;
	/// data
	<std::vector<float> tileData;
};

//------------------------------------------------------------------------------
/** \brief A deep texture class to load deep shadow map tiles from a DTEX file into memory.
 *
 */
class CqDeepTexInputFile
{
	public:
		CqDeepTexInputFile(std::string filename); //< Should we have just filename, or filenameAndPath?
		virtual ~CqDeepTexInputFile(){}
	  
		/** \brief Locate the tile containing the given pixel and read it into memory. 
		 * Should this return either a pointer to the beginning of the memory, or an instance of a tile object? Probably the later.
		 *
		 * \param x - Image x-coordinate of the pixel desired. We load the entire enclosing tile to take advantage of the spatial locality heuristic.
		 * \param y - Image x-coordinate of the pixel desired. We load the entire enclosing tile to take advantage of the spatial locality heuristic.
		 * \param tile - Reference to an object to which we copy the tile.
		 */
		virtual void ReadTile( const int x, const int y, SqDeepDataTile& tile );
		
	private:

		// File handle for the file we read from
		std::ifstream m_dtexFile;
		
};

#endif // DTEXINPUT_H_INCLUDED
