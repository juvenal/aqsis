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

//Aqsis primary header
#include "aqsis.h"

// Standard libraries
#include <string>
#include <iostream>
#include <fstream>
#include <vector>
#include <map>
#include <tiff.h> //< temporary include to get the typedefs like uint32

// External libraries
#include <boost/shared_ptr.hpp>

//------------------------------------------------------------------------------
/** \brief A structure to store sub-region (bucket) deep data.
 *
 */
struct SqSubTileRegion
{
	/// The length (in units of number of nodes)
	/// of each visibility function in the tile. A particle function N may be indexed at position (sum of function lengths up to, but excluding N).
	/// The sum of all elements can be used to determine the size in bytes of the tile: 
	/// May be expressed as: std::accumulate(functionOffsets.begin(), functionOffsets.end(), 0);
	/// The number of elements in this vector must be equal to (tileWidth*tileHeight),
	/// so there is a length field for every pixel, empty or not.
	std::vector<std::vector<int> > functionLengths;
	/// data
	std::vector<std::vector<float> > tileData;
};

//------------------------------------------------------------------------------
/** \brief A structure to store the tile header and data for a tile.  
 *
 */
struct SqDeepDataTile
{
	/// Organize tile data as a set of sub-regions each with the dimensions of a bucket.
	/// Store an array or vector of pointers to bucket data. Store them in order so that the first pointer 
	/// points to the top-left bucket, and the last one points to the bottom left bucket.
	/// A problem with this is that  tile data will no longer be stored contiguously in memory,
	/// so it will not be possible to write the entire tile to file in a single write without first doing a copy of all the data into a contiguous region in memory.
	/// It is desirable to write once the whole tile in order to minimize disk accesses, which or slow.
	/// Therefore I plan to use dataSubRegions to accumulate tile data, then copy everything into functionLengths and tileData to write to file.
	std::vector<boost::shared_ptr<SqSubTileRegion> > subRegions;
	unsigned int tileCoordX;
	unsigned int tileCoordY;
};

//------------------------------------------------------------------------------
/** \brief A structure to serve as a data type for entries in the tile table (which follows immediately after the file header).
 *
 */
struct SqTileTableEntry
{
	/** Constructor
	 */ 
	SqTileTableEntry( uint32 x, uint32 y, uint32 fo )
		: tileCoordX( x ),
		tileCoordY( y ),
		fileOffset( fo )
	{
	}
	uint32 tileCoordX;
	uint32 tileCoordY;
	/// Absolute file offset to the beginning of a tile header (fileOffset == 0 if tile is empty)
	uint32 fileOffset;
};

//------------------------------------------------------------------------------
/** \brief A structure storing the constant/global DTEX file information
 *
 * This file header represents the first bytes of data in the file.
 * It is immediately followed by the tile table.
 */
struct SqDtexFileHeader
{
	/** Trivial Constructor: initialize an SqDtexFileHeader structure
	 */ 
	SqDtexFileHeader( uint32 fs, uint32 iw, uint32 ih, uint32 nc, 
			uint32 bpc, uint32 hs, uint32 ds, uint32 tw, uint32 th, uint32 nt)
		: // magicNumber( mn ),
		fileSize( fs ),
		imageWidth( iw ),
		imageHeight( ih ),
		numberOfChannels( nc ),
		bytesPerChannel( bpc ),
		headerSize( hs ),
		dataSize( ds ),
		tileWidth( tw ),
		tileHeight( th ),
		numberOfTiles( nt )
	{
	}
	/// The magic field contains the following bytes: Ò\0x89AqD\0x0b\0x0a\0x16\0x0a"
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
	uint32 headerSize;
	// Size of the deep data by itself, in bytes
	uint32 dataSize;
	// Width, in pixels, of a tile (unpadded, so edge tiles may be larger, but never smaller)
	uint32 tileWidth;
	// Height, in pixels, of a tile (unpadded)
	uint32 tileHeight;
	// Number of tiles in the image
	uint32 numberOfTiles;
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
 * simply by keeping mutiple instances of CqDeepTexOutputFile.
 */
class CqDeepTexOutputFile
{
	public:
		CqDeepTexOutputFile(std::string filename, uint32 imageWidth, uint32 imageHeight, uint32 tileWidth, uint32 tileHeight, 
				uint32 bucketWidth, uint32 bucketHeight, uint32 numberOfChannels, uint32 bytesPerChannel);
		virtual ~CqDeepTexOutputFile(){}
	  
		/** \brief Copy the given data into the tile map
		 *
		 *
		 * \param metadata - Data describing important attributes about the deep data.
		 *					In this case, values specifying the length, in units of number of nodes,
		 * 					of each visibility function. In the case of an empty bucket, the first value in
		 * 					metadata is -1 and data is empty/invalid.
		 */
		virtual void SetTileData( const int xmin, const int ymin, const int xmax, const int ymax, const unsigned char *data, const unsigned char* metadata );
		
		/** \brief Dtermine if all data has been received for a specific tile
		 *
		 *
		 * \param tile - The tile; we want to know if it is full
		 */				
		bool IsFullTile(const boost::shared_ptr<SqDeepDataTile> tile) const;
		
	private:
		// Functions
		void CreateNewSubTileRegion(boost::shared_ptr<SqDeepDataTile> currentTile, const int subRegionID, const int ymin);
		void CreateNewTile(const int tileID);
		void CopyMetaData(std::vector< std::vector<int> >& toMetaData, const int* fromMetaData, const int xmin, const int ymin, const int xmax, const int ymax);
		void CopyData(std::vector< std::vector<float> >& toData, const float* fromData, const int* functionLengths, const int xmin, const int ymin, const int xmax, const int ymax);
		void FillEmptyMetaData(std::vector< std::vector<int> >& toMetaData, const int xmin, const int ymin, const int xmax, const int ymax);
		void FillEmptyData(std::vector< std::vector<float> >& toData, const int xmin, const int ymin, const int xmax, const int ymax);
		inline int NodeCount(const int* functionLengths, const int numberOfPixels) const;
		void WriteTileTable();
		/** \brief Determine if this tile is empty, that is, it covers no micropolygons or participating media in the DSM
		 * and can therefore be left out of the DTEX file, and represented instead with a fileOffset of 0 in the tile table, signifying that any pixels
		 * inside this tile have 100% visibility at all depths, and are therefore never in shadow.
		 *
		 * \param tile - The tile; we want to know if it has all of its sub-regions as empty sub-regions.
		 */	
		bool IsNeglectableTile(const boost::shared_ptr<SqDeepDataTile> tile);
		void UpdateTileTable(const boost::shared_ptr<SqDeepDataTile> tile);
		void WriteTile(const boost::shared_ptr<SqDeepDataTile> tile);
		
		//-----------------------------------------------------------------------------------
		// Member Data
		
		// File handle for the file we write to
		std::ofstream m_dtexFile;
		// File header stuff
		SqDtexFileHeader m_fileHeader;
		std::vector<SqTileTableEntry> m_tileTable;
		
		// Data buffers
		// The map is indexed with a key: a tile ID, which is the number of the tile 
		// assuming tiles are layed out in increasing order from left-to-right, top-to-bottom.
		//std::map<int, std::vector<boost::shared_ptr<SqDeepDataTile> > > m_deepDataTileMap;
		std::map<int, boost::shared_ptr<SqDeepDataTile> > m_deepDataTileMap;
		
		// Fields
		int m_xBucketsPerTile;
		int m_yBucketsPerTile;
		int m_bucketWidth;
		int m_bucketHeight;
		
};

//------------------------------------------------------------------------------
// Inline function(s) for CqDeepTexOutputFile
//------------------------------------------------------------------------------
inline int CqDeepTexOutputFile::NodeCount(const int* functionLengths, const int numberOfPixels) const
{
	int count = 0;
	int i;
	
	for(i = 0; i < numberOfPixels; ++i)
	{
		if (functionLengths[i] != -1)
		{
			count += functionLengths[i];
		}
	}
	return count;
}

#endif // DTEX_H_INCLUDED
