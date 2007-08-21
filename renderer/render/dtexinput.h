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
#include <tiff.h> //< Including (temporarily) in order to get the typedefs like uint32

// Additional Aqsis headers
#include "deeptexturetile.h"

// External libraries
//#include <boost/intrusive_ptr.hpp>
#include <boost/shared_array.hpp>
#include <boost/shared_ptr.hpp>

//#include "smartptr.h"

namespace Aqsis
{

//------------------------------------------------------------------------------
/** \brief A structure storing the constant/global DTEX file information
 *
 * This file header represents the first bytes of data in the file.
 * It is immediately followed by the tile table.
 */
/// \todo This structure should be stored in a shared location since it is used in this file and in dtexinput.cpp/dtexinput.h
struct SqDtexFileHeader
{
	/** \brief Construct an SqDtexFileHeader structure
	* \param mn - magic number
	* \param fs - file size of the dtex file in bytes
	* \param iw - image width; the width in pixels of the dtex image
	* \param ih - image height; the hieght in pixels of the dtex image
	* \param nc - number of channels; for example 'rgb' has 3 channels, but 'r' has only 1 channel 
	* \param bpc - bytes per channel; the size in bytes used for each color channel. 
	* 				For example, float takes 4 bytes, but char takes only 1 byte
	* \param hs - header size; the size in bytes of SqDtexFileHeader. We can probably get rid of this field.
	* \param ds - data size; the size of only the data part of the dtex file
	 */ 
	SqDtexFileHeader( const char* magicNumber = NULL, const uint32 fileSize = 0, const uint32 imageWidth = 0, 
			const uint32 imageHeight = 0, const uint32 numberOfChannels = 0, const uint32 dataSize = 0, 
			const uint32 tileWidth = 0, const uint32 tileHeight = 0, const uint32 numberOfTiles = 0
			const float matWorldToScreen[4][4] = NULL, const float matWorldToCamera[4][4] = NULL) :
		magicNumber( magicNumber ),
		fileSize( fileSize ),
		imageWidth( imageWidth ),
		imageHeight( imageHeight ),
		numberOfChannels( numberOfChannels ),
		dataSize( dataSize ),
		tileWidth( tileWidth ),
		tileHeight( tileHeight ),
		numberOfTiles( numberOfTiles ),
		matWorldToScreen(),
		matWorldToCamera()
	{}
	
	/** \brief Write the dtex file header to open binary file.
	 *
	 * \param file  - An open binary file with output stream already set
	 *  to the correct position (beginnig of file) to write the header. 
	 */	
	void writeToFile( std::ofstream& file ) const;
	
	/** \brief Seek to the appropriate position in m_dtexFile and write the dtex file header.
	 *
	 * \param file  - An open binary file with input stream already set
	 *  to the correct position (beginnig of file) to write the header. 
	 */	
	void readFromFile( std::ifstream& file );
	
	/** \brief Copy the tranformation matrices to the local member data.
	 * 
	 */	
	void setTransformationMatrices(const float matWorldToScreen[4][4], const float matWorldToCamera[4][4]);
	
	/// The magic number field contains the following bytes: Ò\0x89AqD\0x0b\0x0a\0x16\0x0a"
	// The first byte has the high-bit set to detect transmission over a 7-bit communications channel.
	// This is highly unlikely, but it can't hurt to check. 
	// The next three bytes are the ASCII string "AqD" (short for "Aqsis DTEX").
	// The next two bytes are carriage return and line feed, which results in a "return" sequence on all major platforms (Windows, Unix and Macintosh).
	// This is followed by a DOS end of file (control-Z) and another line feed.
	// This sequence ensures that if the file is "typed" on a DOS shell or Windows command shell, the user will see "AqD" 
	// on a single line, preceded by a strange character.
	// Magic number for a DTEX file is: "\0x89AqD\0x0b\0x0a\0x16\0x0a" Note 0x417144 represents ASCII AqD
	static char* magicNumber; 
	/// Size of this file in bytes
	uint32 fileSize;
	// Number of horizontal pixels in the image
	uint32 imageWidth;
	// Number of vertical pixels
	uint32 imageHeight;
	/// Number if channels in a visibility node (1 for grayscale, 3 for full color)
	uint32 numberOfChannels;
	// Size of the deep data by itself, in bytes
	uint32 dataSize;
	// Width, in pixels, of a tile (unpadded, so edge tiles may be larger, but never smaller)
	uint32 tileWidth;
	// Height, in pixels, of a tile (unpadded)
	uint32 tileHeight;
	// Number of tiles in the image
	uint32 numberOfTiles;
	// World to Screen transformation matrix
	float matWorldToScreen[4][4];
	// World to Camera transformation matrix
	float matWorldToCamera[4][4];
};

//------------------------------------------------------------------------------
/** \brief A deep texture class to load deep shadow map tiles from a DTEX file into memory, but this class does not actually own said memory.
 * 
 * Usage works as follows: 
 * This class acts as an interface allowing retrieval of on-disk tiles from tiled images, specifically dtex files.
 * A single public member function called LoadTileForPixel() is responsible for locating the tile with the requested pixel from disk and
 * copying it to the memory provided by the caller. 
 */
class CqDeepTexInputFile
{
	public:
		/** \brief Construct an instance of CqDeepTexOutputFile
		 *
		 * \param filename - The full path and file name of the dtex file to open and read from.
		 */
		CqDeepTexInputFile(std::string filename); //< Should we have just filename, or filenameAndPath?
	  
		/** \brief Locate the tile containing the given pixel and copy it into memory. 
		 *
		 * \param x - Image x-coordinate of the pixel desired. We load the entire enclosing tile because of the spatial locality heuristic; it is likely to be needed again soon.
		 * \param y - Image y-coordinate of the pixel desired. We load the entire enclosing tile because of the spatial locality heuristic; it is likely to be needed again soon.
		 * \return a shared pointer to an object to the requested tile in memory.
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
		 *
		 * \return the width in pixels
		 */
		inline TqUint standardTileHeight() const;
		
		/** \brief Get the number of color channels in a deep data node in the deep texture map.
		 *
		 * \return the number of color channels.
		 */
		inline TqUint numberOfColorChannels() const;
		
	private:
		
		// Functions
		void loadTileTable();
		boost::shared_ptr<CqDeepTextureTile> loadTile(const TqUint tileRow, const TqUint tileCol);

		// File handle for the file we read from
		std::ifstream m_dtexFile;
		
		// File header stuff
		SqDtexFileHeader m_fileHeader;
		// m_tileOffsets serves as the tile table, indexed by [tileTopLeftY][tileTopLeftX],
		// returns the file byte position of the start of the tile.
		std::vector< std::vector<TqUint> > m_tileOffsets;
		
		// Other data
		TqUint m_tilesPerRow;
		TqUint m_tilesPerCol;
};

//------------------------------------------------------------------------------
// Implementation of inline functions for CqDeepTexInputFile
//------------------------------------------------------------------------------
inline const TqFloat* CqDeepTextureTile::visibilityFunctionAtPixel( const TqUint tileSpaceX, const TqUint tileSpaceY ) const
{
	const TqUint nodeSize = 1+m_colorChannels;
	
	return (m_data.get()+(tileSpaceX*tileSpaceY*nodeSize));
}

inline TqUint CqDeepTextureTile::functionLengthOfPixel( const TqUint tileSpaceX, const TqUint tileSpaceY ) const
{
	return m_metaData[tileSpaceX*tileSpaceY];
}

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

//------------------------------------------------------------------------------
} // namespace Aqsis

#endif // DTEXINPUT_H_INCLUDED
