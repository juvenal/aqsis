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
 * \brief Implementation of a deep texture file reader to load deep data tiles from file into memory.
 */

#include <numeric>

#include "dtexinput.h"
#include "exception.h"

namespace Aqsis
{

// Magic number for a DTEX file is: "\0x89AqD\0x0b\0x0a\0x16\0x0a" Note 0x417144 represents ASCII AqD
static const char magicNumber[8] = { 0x89, 'A', 'q', 'D', 0x0b, 0x0a, 0x16, 0x0a };

CqDeepTexInputFile::CqDeepTexInputFile(std::string filename)
	: m_dtexFile( filename.c_str(), std::ios::in | std::ios::binary ),
	m_fileHeader(),
	m_tileOffsets(),
	m_tilesPerRow(0),
	m_tilesPerCol(0)
{
	// Verify that the magic number is valid
	std::string correctMN = magicNumber;
	char buffer[8];
	m_dtexFile.read(buffer, 8);
	std::string fileMN = buffer;
	if ( fileMN != correctMN )
	{
		throw XqInternal(std::string("Magic number for DTEX file: ") + filename + std::string(" is not valid."), __FILE__, __LINE__);
	}
	// Load the file header into memory
	m_dtexFile.read((char*)(&m_fileHeader), sizeof(SqDtexFileHeader));
	// Calculate some other variables
	m_tilesPerRow = m_fileHeader.imageWidth/m_fileHeader.tileWidth;
	m_tilesPerCol = m_fileHeader.imageHeight/m_fileHeader.tileHeight;
	// Load the tile table into memory
	LoadTileTable();
}

CqDeepTexInputFile::~CqDeepTexInputFile()
{
	m_dtexFile.close();
}

boost::shared_ptr<CqDeepTextureTile> CqDeepTexInputFile::LoadTileForPixel( const TqUint x, const TqUint y )
{
	assert( x < m_fileHeader.imageWidth );
	assert( y < m_fileHeader.imageHeight );
	int tileCol = x/m_fileHeader.tileWidth;
	int tileRow = y/m_fileHeader.tileHeight;
	// If this texture has padded tiles, we need to correct tileCol and tileRow
	if (tileRow >= m_tilesPerCol)
	{
		tileRow = m_tilesPerCol-1;
	}
	if (tileCol >= m_tilesPerRow)
	{
		tileCol = m_tilesPerRow-1; 
	}
	const int fileOffset = m_tileOffsets[tileRow][tileCol];
	
	// If fileOffset is 0 then there is no tile to load,
	// so either leave it null, or change this function's signature to make it return a value to indicate whether or not a tile was found. Leaving it null for now.
	if (fileOffset > 0)
	{
		return LoadTileAtOffset(fileOffset, tileRow, tileCol);
	}
	boost::shared_ptr<CqDeepTextureTile> nullTile;
	return nullTile; //< The requested tile is empty. Caller may assume visibility is 100% at all depths under this pixel
}

void CqDeepTexInputFile::LoadTileTable()
{
	// Assuming the file get pointer is currently at the beginning of the tile table:
	assert(m_dtexFile.tellg() == 8+sizeof(SqDtexFileHeader));
	
	TqUint tcol;
	TqUint trow;
	TqUint offset;
	TqUint i;
	
	// Allocate space to store the tile table as a vector of vectors of file offsets
	m_tileOffsets.resize(m_tilesPerCol);
	for (i = 0; i < m_tilesPerCol; ++i)
	{
		m_tileOffsets[i].resize(m_tilesPerRow);
	}
	
	// Read the tile table
	for (i = 0; i < m_fileHeader.numberOfTiles; ++i)
	{
		m_dtexFile.read((char*)(&tcol), sizeof(uint32));
		m_dtexFile.read((char*)(&trow), sizeof(uint32));
		m_dtexFile.read((char*)(&offset), sizeof(uint32));
		m_tileOffsets[trow][tcol] = offset; 
	}
}

boost::shared_ptr<CqDeepTextureTile> CqDeepTexInputFile::LoadTileAtOffset(const TqUint fileOffset, const TqUint tileRow, const TqUint tileCol)
{
	// Allocate sufficient memory to hold the tile
	// Determine the pixel dimensions of this tile so we know how much space to allocate for the metadata
	TqUint tileWidth = m_fileHeader.tileWidth;
	TqUint tileHeight = m_fileHeader.tileHeight;

	// If this is an end tile, and the image width or height is not divisible by tile width or height,
	// Then perform padding on the tile: resize to a larger size to accomodate the extra data.
	if (tileCol == (m_tilesPerRow-1)) //< subtract 1 from tilesPerRow since tileCol is zero-indexed (tileCol == 2 -> it is the 3rd tile from the left)
	{
		tileWidth += m_fileHeader.imageWidth%m_fileHeader.tileWidth;
	}
	if (tileRow == (m_tilesPerCol-1))
	{
		tileHeight += m_fileHeader.imageHeight%m_fileHeader.tileHeight;
	}
	
	boost::shared_array<TqUint> functionLengths(new TqUint[tileWidth*tileHeight]);
	//std::vector<uint32>& functionLengths = tile->functionLengths;
	//functionLengths.resize(tileWidth*tileHeight);
	
	// Seek to the poition of the beginning of the requested tile and read it in
	m_dtexFile.seekg(fileOffset, std::ios::beg);
	m_dtexFile.read((char*)(functionLengths.get()), (tileWidth*tileHeight*sizeof(uint32)));
	
	//std::vector<float>& data = tile->tileData;
	const int numberOfNodes = std::accumulate(functionLengths.get(), functionLengths.get()+(tileWidth*tileHeight), 0); //< MIGHT NEED +1 ON SECOND ITERATOR
	const int nodeSize = m_fileHeader.numberOfChannels+1;
	boost::shared_array<TqFloat> data(new TqFloat[numberOfNodes*nodeSize]);
	//data.resize(numberOfNodes*nodeSize);
	m_dtexFile.read((char*)(data.get()), (numberOfNodes*nodeSize*sizeof(TqFloat)));
	boost::shared_ptr<CqDeepTextureTile> tile(new CqDeepTextureTile(data, functionLengths, tileWidth, tileHeight,
												tileCol*m_fileHeader.tileWidth, tileRow*m_fileHeader.tileHeight, m_fileHeader.numberOfChannels, m_fileHeader.bytesPerChannel));
	
	return tile;
}

//------------------------------------------------------------------------------
} // namespace Aqsis
