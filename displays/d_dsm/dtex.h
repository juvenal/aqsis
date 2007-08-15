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
	unsigned int tileCol;
	unsigned int tileRow;
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
		: tileCol( x ),
		tileRow( y ),
		fileOffset( fo )
	{
	}
	uint32 tileCol;
	uint32 tileRow;
	/// Absolute file offset to the beginning of a tile header (fileOffset == 0 if tile is empty)
	uint32 fileOffset;
};

//------------------------------------------------------------------------------
/** \brief A structure storing the constant/global DTEX file information
 *
 * This file header represents the first bytes of data in the file.
 * It is immediately followed by the tile table.
 */
/// \todo This structure should be stored in a shared location since it is used in this file and in dtexinput.cpp/dtexinput.h
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
		 * \param data - The deep data corresponding to the image region defined by (xmin,ymin) and (xmax,ymax)
		 */
		virtual void SetTileData( const int xmin, const int ymin, const int xmax, const int ymax, const unsigned char *data, const unsigned char* metadata );
		
	private:
		// Functions
		
		/** \brief Create a new SqSubTileRegion and add it to the current tile's vector of sub-regions in the appropriate spot. 
		 * Initialize the correct amount of storage for this particular sub-region.
		 *
		 * \param currentTile - The tile to which we will add the new sub-region
		 * \param subRegionID - The number of the new sub-region with respect to the first one in the tile, counted left-to-right and top-to-bottom
		 * \param ymin - The image space y-coordinate which will be this new sub-region's y-origin. It is important that this not be a larger coordinate
		 * 			because this parameter is used to determine how much memory to allocate for the sub-region. Therefore the user who calls SetTileData must always send 
		 * 			the first row of a bucket before sending any other row from within a bucket. 
		 */		
		void CreateNewSubTileRegion(boost::shared_ptr<SqDeepDataTile> currentTile, const int subRegionID, const int ymin);
		
		/** \brief Create a new tile and add it to m_DeepDataTileMap with the given key: tileID
		 *
		 * \param tileID - The tile's number, counting left-to-right, top-to-bottom. Used as its key in m_DeepDataTileMap.
		 * \param tileRow - The index of the row of tiles this tile resides in.
		 * \param tileCol - The index of the column if tiles this tile resides in. 
		 */
		void CreateNewTile(const int tileID, const int tileRow, const int tileCol);
		
		/** \brief Copy given metadata into the given std::vector.
		 * 
		 * \param toMetaData - The destination to which we want to copy metadata
		 * \param fromMetaData - The source of the data we want to copy.
		 * \param rxmin - The first x-coordinate, relative to the origin of the sub-region to which we are copying, from which copying should begin
		 * \param rymin - The first y-coordinate, relative to the origin of the sub-region to which we are copying, and the index into toMetaData
		 * \param rxmax - The x-coordinate, relative to the origin of the sub-region to which we are copying, where copying should halt
		 * \param rymax - The y-coordinate, relative to the origin of the sub-region to which we are copying, where copying should halt
		 */	
		void CopyMetaData(std::vector< std::vector<int> >& toMetaData, const int* fromMetaData, const int rxmin, const int rymin, const int rxmax, const int rymax);
		
		/** \brief Copy given data into the given std::vector.
		 * 
		 * \param toData - The destination to which we want to copy data
		 * \param fromData - The source of the data we want to copy.
		 * \param rxmin - The first x-coordinate, relative to the origin of the sub-region to which we are copying, from which copying should begin
		 * \param rymin - The first y-coordinate, relative to the origin of the sub-region to which we are copying, and the index into toMetaData
		 * \param rxmax - The x-coordinate, relative to the origin of the sub-region to which we are copying, where copying should halt
		 * \param rymax - The y-coordinate, relative to the origin of the sub-region to which we are copying, where copying should halt
		 */	
		void CopyData(std::vector< std::vector<float> >& toData, const float* fromData, const int* functionLengths, const int rxmin, const int rymin, const int rxmax, const int rymax);
		
		/** \brief Fill the sub-region, as specified by (rxmin,rymin,rxmax,rymax), with default metadata. The sub-region may be part of a neglectable tile,
		 * 		and therefore may never be written to disk, but we have to assume it might be part of a tile that will be written, and so we fill it with minimal data
		 * 		to indicate that the pixels in this region have 100% visibility.
		 * 
		 * \param toMetaData - The destination we want to fill with default metadata
		 * \param rxmin - The first x-coordinate, relative to the origin of the sub-region to which we are copying, from which copying should begin
		 * \param rymin - The first y-coordinate, relative to the origin of the sub-region to which we are copying, and the index into toMetaData
		 * \param rxmax - The x-coordinate, relative to the origin of the sub-region to which we are copying, where copying should halt
		 * \param rymax - The y-coordinate, relative to the origin of the sub-region to which we are copying, where copying should halt
		 */	
		void FillEmptyMetaData(std::vector< std::vector<int> >& toMetaData, const int rxmin, const int rymin, const int rxmax, const int rymax);
		
		/** \brief Fill the sub-region, as specified by (rxmin,rymin,rxmax,rymax), with default metadata. The sub-region may be part of a neglectable tile,
		 * 		and therefore may never be written to disk, but we have to assume it might be part of a tile that will be written, and so we fill it with minimal data
		 * 		to indicate that the pixels in this region have 100% visibility.
		 * 
		 * \param toData - The destination we want to populate with default deep data.
		 * \param rxmin - The first x-coordinate, relative to the origin of the sub-region to which we are copying, from which copying should begin
		 * \param rymin - The first y-coordinate, relative to the origin of the sub-region to which we are copying, and the index into toMetaData
		 * \param rxmax - The x-coordinate, relative to the origin of the sub-region to which we are copying, where copying should halt
		 * \param rymax - The y-coordinate, relative to the origin of the sub-region to which we are copying, where copying should halt
		 */
		void FillEmptyData(std::vector< std::vector<float> >& toData, const int rxmin, const int rymin, const int rxmax, const int rymax);
		
		/** \brief Determine if all data has been received for a specific tile, meaning it is full and ready for output to dile.
		 *
		 * \param tile - The tile; we want to know if it is full
		 * \param tileID - The tile's number, counting left-to-right, top-to-bottom
		 */				
		bool IsFullTile(const boost::shared_ptr<SqDeepDataTile> tile, const int tileID) const;
		
		/** \brief Determine if this tile is neglectable, that is, it covers no micropolygons or participating media in the DSM
		 * and can therefore be left out of the DTEX file, and represented instead with a fileOffset of 0 in the tile table, signifying that any pixels
		 * inside this tile have 100% visibility at all depths, and are therefore never in shadow.
		 *
		 * \param tile - The tile; we want to know if it has all of its sub-regions as empty sub-regions.
		 */	
		bool IsNeglectableTile(const boost::shared_ptr<SqDeepDataTile> tile) const;
		
		/** \brief Create a new SqTileTableEntry for the given tile and add it to the back of m_tileTable
		 *
		 * \param tile - The tile; we want to create a new tile table entry for it.
		 */		
		void UpdateTileTable(const boost::shared_ptr<SqDeepDataTile> tile);
		
		/** \brief Seek to the appropriate position in m_dtexFile and write the tile table.
		 *
		 */	
		void WriteTileTable();
		
		/** \brief Write to disk the metadata, followed by the deep data, for the given tile.
		 *
		 * \param tile - The tile; we want to write its data to m_dtexFile.
		 */	
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

#endif // DTEX_H_INCLUDED
