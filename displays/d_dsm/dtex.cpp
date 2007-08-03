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
 * \author Zachary L. Carter (zcarter@aqsis.org)
 * 
 * \brief Implementation of a deep texture file writer to create tiled binary DTEX files.
*/

#include "dtex.h"
#include "exception.h"

// Magic number for a DTEX file is: "\0x89AqD\0x0b\0x0a\0x16\0x0a" Note 0x417144 represents ASCII AqD
static const char magicNumber[8] = { 0x89, 'A', 'q', 'D', 0x0b, 0x0a, 0x16, 0x0a };

CqDeepTexOutputFile::CqDeepTexOutputFile(std::string filename, uint32 imageWidth, uint32 imageHeight, uint32 tileWidth, uint32 tileHeight, uint32 numberOfChannels, uint32 bytesPerChannel)
	: m_dtexFile( filename.c_str(), std::ios::out | std::ios::binary ),
	m_fileHeader( (uint32)0, (uint32)imageWidth, (uint32)imageHeight, (uint32)numberOfChannels, (uint32)bytesPerChannel, (uint32)0, (uint32)0, (uint32)tileWidth, (uint32)tileHeight, (uint32)0 ),
	m_tileTable(),
	m_deepDataTileMap()
{
	const int tilesX = imageWidth/tileWidth;
	const int tilesY = imageHeight/tileHeight;
	const int numberOfTiles = tilesX*tilesY; 

	// Note: The following are set outside of the initialization list because they depend on the calculated constants above
	m_fileHeader.headerSize = sizeof(SqDtexFileHeader) + numberOfTiles*sizeof(SqTileTableEntry);
	m_fileHeader.numberOfTiles = numberOfTiles;
	m_tileTable.reserve(numberOfTiles);
	// Note: fileSize and dataSize can't be  determine until we receive all the data,
	// so we write 0 now, then seek back and re-write this field before file close

	// If file open failed, throw an exception
	if (!m_dtexFile.is_open())
	{
		throw Aqsis::XqInternal(std::string("Failed to open file \"") + filename + std::string( "\""), __FILE__, __LINE__);
	}

	// Write the file header
	m_dtexFile.write(magicNumber, 8); // Would rather this be a part of the file header, but how?	
	m_dtexFile.write((const char*)(&m_fileHeader), sizeof(m_fileHeader));
	
	// Seek forward in file to reserve space for writing the tile table later.
	// Seek from the current positon. Alternatively, you could seek from the file's beginning with std::ios::beg
	m_dtexFile.seekp(numberOfTiles*sizeof(SqTileTableEntry), std::ios::cur);
}

void CqDeepTexOutputFile::setTileData( int xmin, int ymin, int xmax, int ymax, const unsigned char *data, const unsigned char* metadata )
{
	const int imageHeight = m_fileHeader.imageHeight;
	const int imageWidth = m_fileHeader.imageWidth;
	const int tileHeight = m_fileHeader.tileHeight;
	const int tileWidth = m_fileHeader.tileWidth;
	// Identify what tiles the passed-in data covers
	// If those tiles are not already in the tile map, add them
	// Note: Ideally tile dimensions will be multiples or factors of bucket dimensions so that one
	// may be completely contained within the other annd sorting does not become complicated.
	// Find the tile that contains the top left corner of the goven data. Either all the data will go here,
	// or else to this tiles (greater ID) neighbors.
	int homeTileID = (int)(imageWidth/tileWidth)*ymin+(int)(xmin/tileWidth); // tilesPerRow*numberOfRows + xmin/tileWidth
	
	// Copy the data into their respective tiles
	// If a tile is filled, write it to file, update the tileTable with a m_tileTable.push_back(), and reclaim its memory

	// If all tiles have been written, write the tileTable, re-write the datasSize and fileSize fields in the file header, and close the image
	m_dtexFile.close();
	// Return
}

void CqDeepTexOutputFile::TilesCovered( const int xmin, const int ymin, const int xmax, const int ymax, std::vector<int>& tileIDs) const
{
	const int imageHeight = m_fileHeader.imageHeight;
	const int imageWidth = m_fileHeader.imageWidth;
	const int tileHeight = m_fileHeader.tileHeight;
	const int tileWidth = m_fileHeader.tileWidth;
	
	 
}

void CqDeepTexOutputFile::writeFileHeader()
{

}

void CqDeepTexOutputFile::writeTile()
{
	
}
