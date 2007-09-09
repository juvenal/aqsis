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

#include "dtexinput.h"
#include "exception.h"
#include "aqsismath.h"
#include "logging.h"

// Magic number for a DTEX file is: "\0x89AqD\0x0b\0x0a\0x16\0x0a" Note 0x417144 represents ASCII AqD
const char dtexMagicNumber[8] = { 0x89, 'A', 'q', 'D', 0x0b, 0x0a, 0x16, 0x0a };
const std::string correctMN(dtexMagicNumber, 8);

namespace Aqsis
{

CqDeepTexInputFile::CqDeepTexInputFile( const std::string filename )
	: m_dtexFile( filename.c_str(), std::ios::in | std::ios::binary ),
	m_fileHeader(),
	m_isValid( true ),
	m_filename( filename ),
	m_tileOffsets(),
	m_tilesPerRow(0),
	m_tilesPerCol(0)
{
	// If file open failed, throw an exception.
	if (!m_dtexFile.is_open())
	{
		throw Aqsis::XqInternal(std::string("Failed to open file \"") + filename + std::string( "\""), __FILE__, __LINE__);
		m_isValid = false;
		return;
	}
	// Verify that the magic number is valid
	char buffer[8];
	m_dtexFile.read(buffer, 8);
	// PROBLEM: in the line below, correctMN ends up with the magic number twice, that is,
	// the string's length will be 16, and it will have the magic number, followed by the magic number again!
	// This is why I have the global correctMN defined above.
	//std::string correctMN = dtexMagicNumber;
	std::string testMN = buffer;
	if ( testMN != correctMN )
	{
		Aqsis::log() << "Magic number in file: \"" << filename.c_str() << "\" is not a valid"
				" DTEX magic number." << std::endl;
		//throw XqInternal(std::string("Magic number in file: ") + filename 
		//		+ std::string(" is not a valid DTEX magic number."), __FILE__, __LINE__);
		m_isValid = false;
		return;
	}
	// Load the file header into memory
	m_fileHeader.readFromFile(m_dtexFile);

	// Calculate some other variables
	m_tilesPerRow = m_fileHeader.imageWidth/max(m_fileHeader.tileWidth,(uint32)1);
	m_tilesPerCol = m_fileHeader.imageHeight/max(m_fileHeader.tileHeight,(uint32)1);
	// Load the tile table into memory
	loadTileTable();
}

boost::shared_ptr<CqDeepTextureTile> CqDeepTexInputFile::tileForPixel( const TqUint x, const TqUint y )
{
	assert( x < m_fileHeader.imageWidth );
	assert( y < m_fileHeader.imageHeight );
	
	const TqUint tileCol = x/m_fileHeader.tileWidth;
	const TqUint tileRow = y/m_fileHeader.tileHeight;
	const TqUint fileOffset = m_tileOffsets[tileRow][tileCol];
	
	if (fileOffset != 0)
	{
		return loadTile(tileRow, tileCol);
	}
	// If fileOffset is 0 then there is no tile to load, but create an empty tile
	TqUint tileWidth = m_fileHeader.tileWidth;
	TqUint tileHeight = m_fileHeader.tileHeight;
	
	// If this is an end tile, and the image width or height is not divisible by tile width or height,
	// then perform padding: size this tile to a smaller size than usual.
	if (tileCol == (m_tilesPerRow-1)) //< -1 because tileCol is zero-indexed (tileCol == 2 -> 3rd from left)
	{
		tileWidth = m_fileHeader.imageWidth-(tileCol*m_fileHeader.tileWidth);
	}
	if (tileRow == (m_tilesPerCol-1))
	{
		tileHeight = m_fileHeader.imageHeight-(tileRow*m_fileHeader.tileHeight);
	}
	boost::shared_array<TqFloat> nullBoostData;
	boost::shared_array<TqInt> nullBoostOffsets;
	boost::shared_ptr<CqDeepTextureTile> emptyTile(new CqDeepTextureTile(nullBoostData, nullBoostOffsets,
			tileWidth, tileHeight, tileCol*m_fileHeader.tileWidth, tileRow*m_fileHeader.tileHeight,
			m_fileHeader.numberOfChannels));
	return emptyTile; //< The requested tile is empty. Caller may assume visibility is 100%
}

void CqDeepTexInputFile::transformationMatrices( CqMatrix& matWorldToScreen, 
		CqMatrix& matWorldToCamera ) const
{
	matWorldToScreen = (const TqFloat*)m_fileHeader.matWorldToScreen;
	matWorldToCamera = (const TqFloat*)m_fileHeader.matWorldToCamera;
}

void CqDeepTexInputFile::loadTileTable()
{	
	TqUint trow; //< tile row index
	TqUint tcol; //< tile column index
	TqUint offset;
	TqUint i;
	
	// Allocate space to store the tile table as a vector of vectors of file offsets
	m_tileOffsets.resize(m_tilesPerCol);
	for (i = 0; i < m_tilesPerCol; ++i)
	{
		m_tileOffsets[i].resize(m_tilesPerRow, 0);
	}
	
	printf("Load Tile table\n");
	// Read the tile table
	for (i = 0; i < m_fileHeader.numberOfTiles; ++i)
	{
		m_dtexFile.read((char*)(&trow), sizeof(uint32));
		m_dtexFile.read((char*)(&tcol), sizeof(uint32));
		m_dtexFile.read((char*)(&offset), sizeof(uint32));
		m_tileOffsets[trow][tcol] = offset; 
		printf("(%d, %d) : %d\n", trow, tcol, offset);
	}
}

boost::shared_ptr<CqDeepTextureTile> CqDeepTexInputFile::loadTile(const TqUint tileRow, const TqUint tileCol)
{	
	// Allocate sufficient memory to hold the tile
	// Determine the pixel dimensions of this tile so we know how much space to allocate for the meta data
	TqUint tileWidth = m_fileHeader.tileWidth;
	TqUint tileHeight = m_fileHeader.tileHeight;
	const TqUint fileOffset = m_tileOffsets[tileRow][tileCol];
	
	// If this is an end tile, and the image width or height is not divisible by tile width or height,
	// then perform padding: size this tile to a smaller size than usual.
	if (tileCol == (m_tilesPerRow-1)) //< -1 because tileCol is zero-indexed (tileCol == 2 -> 3rd from left)
	{
		tileWidth = m_fileHeader.imageWidth-(tileCol*m_fileHeader.tileWidth);
	}
	if (tileRow == (m_tilesPerCol-1))
	{
		tileHeight = m_fileHeader.imageHeight-(tileRow*m_fileHeader.tileHeight);
	}
	
	boost::shared_array<TqInt> functionOffsets(new TqInt[tileWidth*tileHeight+1]);
	
	// Seek to the poition of the beginning of the requested tile and read it in
	m_dtexFile.seekg(fileOffset, std::ios::beg);
	m_dtexFile.read((char*)(functionOffsets.get()), ((tileWidth*tileHeight+1)*sizeof(uint32)));
	const int totalNumberOfNodes = functionOffsets[tileWidth*tileHeight];
	const int nodeSize = m_fileHeader.numberOfChannels+1;
	boost::shared_array<TqFloat> data(new TqFloat[totalNumberOfNodes*nodeSize]);
	m_dtexFile.read((char*)(data.get()), (totalNumberOfNodes*nodeSize*sizeof(TqFloat)));
	
	// Create a new tile from the data we just read
	boost::shared_ptr<CqDeepTextureTile> tile(new CqDeepTextureTile(data, functionOffsets,
			tileWidth, tileHeight, tileCol*m_fileHeader.tileWidth, tileRow*m_fileHeader.tileHeight,
			m_fileHeader.numberOfChannels));
	/*
	printf("Loading a new tile whose data is as follows:\n");
	for (int i = 0; i < totalNumberOfNodes; i += 4)
	{
		printf("depth: %f visibility: %f\n", data[i], data[i+1]);
	}
	*/
	return tile;
}

//------------------------------------------------------------------------------
} // namespace Aqsis
