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
 * \brief Implementation of a deep texture file writer to create tiled binary DTEX files.
 */
#ifndef DTEX_H_INCLUDED
#define DTEX_H_INCLUDED 1

// Aqsis headers
#include "aqsis.h"

// Standard libraries
#include <string>
#include <iostream>
#include <vector>
#include <map>

// External libraries
#include <boost/shared_ptr.hpp>

#include "file.h"

namespace Aqsis
{

//------------------------------------------------------------------------------
/** \brief A structure to serve as a data type for entries in the tile table. 
 *
 */
struct SqDeepDataTile
{
	/// Total number of visibility nodes in this tile:
	/// This is important so that the user may pre-determine how much memory is required to buffer the tile
	unsigned int numberOfNodes;
	/// The relative file file offsets (from the beginning of the tile header)
	/// to thebeginning of each visibility function in the tile.
	std::vector<int> functionOffsets;
	/// data
	std::vector<float> tileData;
};

//------------------------------------------------------------------------------
/** \brief A structure to serve as a data type for entries in the tile table. 
 *
 */
struct SqTileTableEntry
{
	unsigned int tileCoordX;
	unsigned int tileCoordY;
	/// Absolute file offset to the beginning of a tile header (fileOffset == 0 if tile is empty)
	unsigned int fileOffset;
};

//------------------------------------------------------------------------------
/** \brief A structure storing the constant/global DTEX file information
 *
 * This file header represents the first bytes of data in the file.
 * It is immediately followed by the tile table.
 */
struct SqDtexFileHeader
{
	/// The magic field contains the following bytes: Ò\0x89AqD\0x0b\0x0a\0x16\0x0a"
	// The first byte has the high-bit set to detect transmission over a 7-bit communications channel.
	// This is highly unlikely, but it can't hurt to check. 
	// The next three bytes are the ASCII string "AqD" (short for "Aqsis DTEX").
	// The next two bytes are carriage return and line feed, which results in a "return" sequence on all major platforms (Windows, Unix and Macintosh).
	// This is followed by a DOS end of file (control-Z) and another line feed.
	// This sequence ensures that if the file is "typed" on a DOS shell or Windows command shell, the user will see "AqD" 
	// on a single line, preceded by a strange character.
	//unsigned long long magicNumber; ///< This won't accept the 64-bit magic number due to compiler error, but why not?
	unsigned int magicNumber[2];
	/// Size of this file in bytes
	unsigned long long fileSize;
	// Number of horizontal pixels in the image
	unsigned int imageWidth;
	// Number of vertical pixels
	unsigned int imageHeight;
	/// Number if channels in a visibility node (1 for grayscale, 3 for full color)
	unsigned int numberOfChannels;
	/// Depending on the precision, number of bytes per color channel
	unsigned int bytesPerChannel;
	/// Number of bytes in this header (might not need this)
	unsigned int headerSize;
	// Size of the deep data by itself, in bytes
	unsigned long long dataSize;
	// Width, in pixels, of a tile (unpadded)
	unsigned int tileWidth;
	// Height, in pixels, of a tile (unpadded)
	unsigned int tileHeight;
	// Number of tiles in the image
	unsigned int numberOfTiles;
};

//------------------------------------------------------------------------------
/** \brief A deep texture class to receive deep data from a deep display device, manage the DTEX file and tile
 * headers, and write all of these things to disk in a single binary DTEX file.
 *
 * The behavior works as follows:
 * When an instance of the class is instantiated, a binary file of the given name
 * is opened and the file header is created and written. Sufficient space to hold the tile table is reserved
 * immediately following the file header so that the tile table may be written once all data has been written.  
 * The display calls setTileData() on the deep texture instance, passing in a pointer to some deep data.
 * This texture class copies the data from the pointer and sorts it into a map of tiles. It then checks to see
 * if any tiles are full, and if any are, it writes them to the file, then frees the memory. Any tiles that are
 * not full remain in memory until they are filled.
 * This way the display does not store the deep data itself, and it can easily handle multiple deep shadow maps simultaneously
 * simply by keeping mutiple instances of CqDeepTexture.
 */
class CqDeepTexture
{
	public:
		CqDeepTexture(std::string filename, int imageWidth, int imageHeight, int tileWidth, int tileHeight, int numberOfChannels, int bytesPerChannel);
		~CqDeepTexture();
	  
		void setTileData( int xmin, int ymin, int xmax, int ymax, const unsigned char *data );

	private:
		// Functions
		void writeFileHeader();
		void writeTile();
		
		//-----------------------------------------------------------------------------------
		// Member Data
		
		CqFile m_dtexFile;
		
		// File header stuff
		SqDtexFileHeader m_fileHeader;
		std::vector<SqTileTableEntry> m_tileTable;
		
		// Data buffers
		std::map<int, std::vector<boost::shared_ptr<SqDeepDataTile> > > m_DeepDataTileMap;
		
};

} // namespace Aqsis

#endif // DTEX_H_INCLUDED
