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
	std::vector<uint32> functionLengths;
	/// data
	std::vector<float> tileData;
};

//------------------------------------------------------------------------------
/** \brief A structure storing the constant/global DTEX file information
 *
 * This file header represents the first bytes of data in the file.
 * It is immediately followed by the tile table.
 */
/// \todo This structure should be stored in a shared location since it is used in this file and in dtex.cpp/dtex.h
struct SqDtexFileHeader
{
	/** \brief Construct an SqDtexFileHeader structure
	* \param fs - file size of the dtex file in bytes
	* \param iw - image width; the width in pixels of the dtex image
	* \param ih - image height; the hieght in pixels of the dtex image
	* \param nc - number of channels; for example 'rgb' has 3 channels, but 'r' has only 1 channel 
	* \param bpc - bytes per channel; the size in bytes used for each color channel. For example, float takes 4 bytes, but char takes only 1 byte
	* \param hs - header size; the size in bytes of SqDtexFileHeader. We can probably get rid of this field.
	* \param ds - data size; the size of only the data part of the dtex file
	 */ 
	SqDtexFileHeader( uint32 fs = 0, uint32 iw = 0, uint32 ih = 0, uint32 nc = 0, 
			uint32 bpc = 0, uint32 hs = 0, uint32 ds = 0, uint32 tw = 0, uint32 th = 0, uint32 nt = 0)
		: // magicNumber( mn ),
		fileSize( fs ),
		imageWidth( iw ),
		imageHeight( ih ),
		numberOfChannels( nc ),
		bytesPerChannel( bpc ),
		//headerSize( hs ),
		dataSize( ds ),
		tileWidth( tw ),
		tileHeight( th ),
		numberOfTiles( nt )
	{}
	/// The magic number field contains the following bytes: Ò\0x89AqD\0x0b\0x0a\0x16\0x0a"
	// The first byte has the high-bit set to detect transmission over a 7-bit communications channel.
	// This is highly unlikely, but it can't hurt to check. 
	// The next three bytes are the ASCII string "AqD" (short for "Aqsis DTEX").
	// The next two bytes are carriage return and line feed, which results in a "return" sequence on all major platforms (Windows, Unix and Macintosh).
	// This is followed by a DOS end of file (control-Z) and another line feed.
	// This sequence ensures that if the file is "typed" on a DOS shell or Windows command shell, the user will see "AqD" 
	// on a single line, preceded by a strange character.
	//char magicNumber[8]; // How can I store the magic number in such a way that its data, not the value of the
		// address that points to it, is written to file when we call write(m_dtexFileHeader)?
	/// Size of this file in bytes
	uint32 fileSize;
	// Number of horizontal pixels in the image
	uint32 imageWidth;
	// Number of vertical pixels
	uint32 imageHeight;
	/// Number if channels in a visibility node (1 for grayscale, 3 for full color)
	uint32 numberOfChannels;
	/// Depending on the precision, number of bytes per color channel
	uint32 bytesPerChannel;
	/// Number of bytes in this header (might not need this)
	//uint32 headerSize;
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
		CqDeepTexInputFile(std::string filename); //< Should we have just filename, or filenameAndPath?
		virtual ~CqDeepTexInputFile();
	  
		/** \brief Locate the tile containing the given pixel and copy it into memory. 
		 *
		 * \param x - Image x-coordinate of the pixel desired. We load the entire enclosing tile because of the spatial locality heuristic; it is likely to be needed again soon.
		 * \param y - Image x-coordinate of the pixel desired. We load the entire enclosing tile because of the spatial locality heuristic; it is likely to be needed again soon.
		 * \param tile - Pointer to an object to which we copy the tile.
		 */
		virtual void LoadTileForPixel( const int x, const int y, boost::shared_ptr<SqDeepDataTile> tile );
		
	private:
		
		// Functions
		void LoadTileTable();
		void CqDeepTexInputFile::LoadTileAtOffset(const int fileOffset, const int tileRow, const int tileCol, boost::shared_ptr<SqDeepDataTile> tile);

		// File handle for the file we read from
		std::ifstream m_dtexFile;
		
		// File header stuff
		SqDtexFileHeader m_fileHeader;
		std::vector< std::vector<uint32> > m_tileOffsets;
		
		// Other data
		int m_tilesPerRow;
		int m_tilesPerCol;
};

#endif // DTEXINPUT_H_INCLUDED
