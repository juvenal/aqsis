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

// Additional Aqsis headers
#include "smartptr.h"
#include "deepfileheader.h"
#include "deeptexturetile.h"

// External libraries
#include <boost/shared_ptr.hpp>

namespace Aqsis 
{

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
