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
#ifndef DTEXOUTPUT_H_INCLUDED
#define DTEXOUTPUT_H_INCLUDED

//Aqsis primary header
#include "aqsis.h"

// Standard libraries
#include <string>
#include <iostream>
#include <fstream>
#include <vector>
#include <map>
#include <tiff.h> //< temporary include to get the typedefs like uint32

// Additional Aqsis headers
#include "deeptexturetile.h"

// External libraries
#include <boost/shared_ptr.hpp>

namespace Aqsis 
{

// To make eventual refactor to using Aqsis types nicer:
typedef uint32 TqUint32;

//------------------------------------------------------------------------------
/** \brief Interfaces for classes handling output of deep texture tiles.
 *
 */
typedef struct IqDeepTextureOutput
{
	virtual ~IqDeepTextureOutput(){};
	
	/** \brief Send deep tile to a class equipped to output the tile.
	 *
	 * \param tile_ptr - a const pointer to a deep tile to be "output" in some way, like to file.
	 */	
	virtual	void outputTile( const boost::shared_ptr<CqDeepTextureTile> tile_ptr) = 0;
};

//------------------------------------------------------------------------------
/** \brief A structure to serve as a data type for entries in the tile table 
 * (which follows immediately after the file header).
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
	{}
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
	SqDtexFileHeader( const uint32 fileSize = 0, const uint32 imageWidth = 0, 
			const uint32 imageHeight = 0, const uint32 numberOfChannels = 0, const uint32 dataSize = 0, 
			const uint32 tileWidth = 0, const uint32 tileHeight = 0, const uint32 numberOfTiles = 0,
			const float matWorldToScreen[4][4] = NULL, const float matWorldToCamera[4][4] = NULL);
	
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
	void setTransformationMatrices(const float imatWorldToScreen[4][4], const float imatWorldToCamera[4][4]);
	
	/// The magic number field contains the following bytes: Ò\0x89AqD\0x0b\0x0a\0x16\0x0a"
	// The first byte has the high-bit set to detect transmission over a 7-bit communications channel.
	// This is highly unlikely, but it can't hurt to check. 
	// The next three bytes are the ASCII string "AqD" (short for "Aqsis DTEX").
	// The next two bytes are carriage return and line feed, which results in a "return" sequence on all major platforms (Windows, Unix and Macintosh).
	// This is followed by a DOS end of file (control-Z) and another line feed.
	// This sequence ensures that if the file is "typed" on a DOS shell or Windows command shell, the user will see "AqD" 
	// on a single line, preceded by a strange character.
	// Magic number for a DTEX file is: "\0x89AqD\0x0b\0x0a\0x16\0x0a" Note 0x417144 represents ASCII AqD
	//static char magicNumber[8];
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
/** \brief A deep texture class to receive deep data from a deep display device, manage the DTEX file and tile
 * headers, and write all of these things to disk in a single binary DTEX file.
 *
 * The behavior works as follows:
 * When an instance of the class is instantiated, a binary file of the given name
 * is opened and the file header is created and written. Sufficient space to hold the tile table is reserved
 * on file via a seekp() call immediately following the file header so that the tile table may be written
 * once all data has been written.
 *   
 * Tiles are passed in and written to file via calls to outputTile(). 
 * Currently this is done from CqDeepTileAdaptor, which stores the tile data as tiles are built,
 * and frees their memory as they are written to disk.
 */
class CqDeepTexOutputFile : public IqDeepTextureOutput
{
	public:
		/** \brief Construct an instance of CqDeepTexOutputFile
		 *
		 * \param filename - The full path and file name of the dtex file to create/open and write to.
		 */
		CqDeepTexOutputFile(std::string filename, uint32 imageWidth, uint32 imageHeight, uint32 tileWidth, uint32 tileHeight, 
				uint32 bucketWidth, uint32 bucketHeight, uint32 numberOfChannels,
				const float matWorldToScreen[4][4], const float matWorldToCamera[4][4]);
		virtual ~CqDeepTexOutputFile();
		
		//---------------------------------------------------------------------------------
		// Implementation of inherited pure virtual function:
		
		/** \brief Write the given tile to disk, first updating the tile table with an entry for this tile.
		 *
		 *
		 * \param tile_ptr - A pointer to the tile we want to output to disk.
		 */
		virtual	void outputTile( const boost::shared_ptr<CqDeepTextureTile> tile_ptr);
		
	private:
		// Functions

		/** \brief Create a new SqTileTableEntry for the given tile and add it to the back of m_tileTable
		 *
		 * \param tile - The tile; we want to create a new tile table entry for it.
		 */		
		void updateTileTable(const boost::shared_ptr<CqDeepTextureTile> tile);
		
		/** \brief Seek to the appropriate position in m_dtexFile and write the tile table.
		 *
		 */	
		void writeTileTable();
		
		/** \brief Write to disk the metadata, followed by the deep data, for the given tile.
		 *
		 * \param tile - The tile; we want to write its data to m_dtexFile.
		 */	
		void writeTile(const boost::shared_ptr<CqDeepTextureTile> tile);
		
		/** \brief Determine if this tile is neglectable, that is, if it covers no micropolygons 
		 * or participating media in the DSM and can therefore be left out of the DTEX file,
		 * and represented instead with a fileOffset of 0 in the tile table, signifying that any pixels
		 * inside this tile have 100% visibility at all depths, and are therefore never in shadow.
		 *
		 * \param tile - The tile; we want to know if it is empty.
		 */	
		bool isNeglectableTile(const boost::shared_ptr<CqDeepTextureTile> tile) const;
		
		//-----------------------------------------------------------------------------------
		// Member Data
		
		// File handle for the file we write to
		std::ofstream m_dtexFile;
		// File header stuff
		SqDtexFileHeader m_fileHeader;
		// Tile stores tile (xmin,ymin) pairs along with file offsets to the beginning of the tile
		std::vector<SqTileTableEntry> m_tileTable;
		
		unsigned long m_tileTablePositon;

};

//------------------------------------------------------------------------------
} // namespace Aqsis

#endif // DTEXOUTPUT_H_INCLUDED
