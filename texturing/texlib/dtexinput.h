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
#define DTEXINPUT_H_INCLUDED

// Aqsis primary header
#include "aqsis.h"

// Standard libraries
#include <string>
#include <iostream>
#include <fstream>
#include <vector>
#include <map>

// Additional Aqsis headers
#include "deepfileheader.h"
#include "deeptexturetile.h"

// External libraries
#include <boost/shared_array.hpp>
#include <boost/shared_ptr.hpp>

namespace Aqsis
{

//------------------------------------------------------------------------------
/** \brief Interfaces for classes handling input (loading) of deep texture tiles.
 *
 */
typedef struct IqDeepTextureInput
{
	virtual ~IqDeepTextureInput(){};
	
	/** \brief Locate the tile containing the given pixel coordinate, load it and return a pointer.
	 *
	 * \param x - Image x-coordinate of the pixel desired. 
	 * \param y - Image y-coordinate of the pixel desired.
	 * \return a shared pointer to an object to the requested tile in memory.
	 */
	virtual boost::shared_ptr<CqDeepTextureTile> tileForPixel( const TqUint x, const TqUint y ) = 0;

	virtual void transformationMatrices( TqFloat matWorldToScreen[4][4], TqFloat matWorldToCamera[4][4] ) const = 0;
	
	/** \brief Get the width of the deep texture map
	 *
	 * \return the width in pixels
	 */
	virtual TqUint imageWidth() const = 0;

	/** \brief Get the height of the deep texture map
	 *
	 * \return the height in pixels
	 */
	virtual TqUint imageHeight() const = 0;
	
	/** \brief Get the (standard, unpadded tile) width of a tile from the current texture file. 
	 *
	 * \return the width in pixels of a tile.
	 */
	virtual TqUint standardTileWidth() const = 0;
	
	/** \brief Get the (standard, unpadded tile) height of a tile from the current texture file. 
	 *
	 * \return the height in pixels of a tile.
	 */
	virtual TqUint standardTileHeight() const = 0;
	
	/** \brief Get the number of color channels in a deep data node in the deep texture map.
	 *
	 * \return the number of color channels.
	 */
	virtual TqUint numberOfColorChannels() const = 0;

	/** \brief Get the status of the texture file. Is it open and valid?
	 *
	 * \return true if the input file is open and valid, false otherwise.
	 */
	virtual bool isValid() const = 0;	
};

//------------------------------------------------------------------------------
/** \brief A deep texture class to load deep shadow map tiles from a DTEX file into memory, but this class does not actually own said memory.
 * 
 * Usage works as follows: 
 * This class acts as an interface allowing retrieval of on-disk tiles from tiled images, specifically dtex files.
 * A single public member function called LoadTileForPixel() is responsible for locating the tile with the requested pixel from disk and
 * copying it to the memory provided by the caller. 
 */
class CqDeepTexInputFile : public IqDeepTextureInput
{
	public:
		/** \brief Construct an instance of CqDeepTexOutputFile
		 *
		 * \param filename - The full path and file name of the dtex file to open and read from.
		 */
		CqDeepTexInputFile( const std::string filename ); 
	  
		
		//-----------------------------------------------------------------
		// Override pure virtual function inherited from IqDeepTextureInput
		
		/** \brief Locate the tile containing the given pixel and copy it into memory. 
		 *
		 * \param x - Image x-coordinate of the pixel desired. 
		 * We load the entire enclosing tile because of 
		 * the spatial locality heuristic: it is likely to be needed again soon.
		 * 
		 * \param y - Image y-coordinate of the pixel desired.
		 * \return a shared pointer to an object holding the requested tile in memory.
		 */
		boost::shared_ptr<CqDeepTextureTile> tileForPixel( const TqUint x, const TqUint y );
		
		/** \brief Get the transformation matrices freom the deep shadow map
		 *
		 * \param matWorldToScreen - A preallocated 2-D array to hold the transformation matrix by the same name from the dsm.
		 * \paran matWorldToCamera - A preallocated 2-D array to hold the transformation matrix by the same name from the dsm.
		 */
		void transformationMatrices( TqFloat matWorldToScreen[4][4], TqFloat matWorldToCamera[4][4] ) const;
		
		/** \brief Get the width of the deep texture map
		 *
		 * \return the width in pixels
		 */
		inline TqUint imageWidth() const;

		/** \brief Get the height of the deep texture map
		 *
		 * \return the height in pixels
		 */
		inline TqUint imageHeight() const;
		
		/** \brief Get the width of a standard (unpadded) tile in the deep texture map
		 *
		 * \return the width in pixels
		 */
		inline TqUint standardTileWidth() const;
		
		/** \brief Get the height of a standard (unpadded) tile in the deep texture map
		 *	The distinction from non-standard (padded) tiles is that padded tiles
		 *	only occur on the right and bottom image borders, when the image dimensions
		 *	are not evenly divisible by the tile dimension. Such border tiles are smaller
		 *	than standard tiles.
		 *
		 * \return the width in pixels
		 */
		inline TqUint standardTileHeight() const;
		
		/** \brief Get the number of color channels in a deep data node in the deep texture map.
		 *
		 * \return the number of color channels.
		 */
		inline TqUint numberOfColorChannels() const;

		/** \brief Get the status of the texture file. Is it open and valid?
		 *
		 * \return true if the input file is open and valid, false otherwise.
		 */
		inline bool isValid() const;
		
		/** \brief Get he file name of the deep texture file from which we read.
		 *
		 * \return a std::string with the file name
		 */
		inline const std::string& fileName() const;
		
	private:
		
		// Functions
		void loadTileTable();
		boost::shared_ptr<CqDeepTextureTile> loadTile(const TqUint tileRow, const TqUint tileCol);

		// File handle for the file we read from
		std::ifstream m_dtexFile;
		
		// File header stuff
		SqDtexFileHeader m_fileHeader;
		bool m_isValid; 				///< Is this input file open and valid (is it a deep texture file?)
		std::string m_filename;
		// m_tileOffsets serves as the tile table, indexed by [tileTopLeftY][tileTopLeftX],
		// returns the file byte position of the start of the tile.
		std::vector< std::vector<TqUint> > m_tileOffsets;
		
		// Other data
		TqUint m_tilesPerRow;
		TqUint m_tilesPerCol;
};

//------------------------------------------------------------------------------
// Implementation of inline functions for CqDeepTexInputFile

inline TqUint CqDeepTexInputFile::imageWidth() const
{
	return m_fileHeader.imageWidth;
}

inline TqUint CqDeepTexInputFile::imageHeight() const
{
	return m_fileHeader.imageHeight;
}

inline TqUint CqDeepTexInputFile::standardTileWidth() const
{
	return m_fileHeader.tileWidth;
}

inline TqUint CqDeepTexInputFile::standardTileHeight() const
{
	return m_fileHeader.tileHeight;	
}

inline TqUint CqDeepTexInputFile::numberOfColorChannels() const
{
	return m_fileHeader.numberOfChannels;
}

inline bool CqDeepTexInputFile::isValid() const
{
	return m_isValid;
}

inline const std::string& CqDeepTexInputFile::fileName() const
{
	return m_filename;
}

//------------------------------------------------------------------------------
} // namespace Aqsis

#endif // DTEXINPUT_H_INCLUDED
