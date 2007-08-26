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

namespace Aqsis
{

// Magic number for a DTEX file is: "\0x89AqD\0x0b\0x0a\0x16\0x0a" Note 0x417144 represents ASCII AqD
static const char dtexMagicNumber[8] = { 0x89, 'A', 'q', 'D', 0x0b, 0x0a, 0x16, 0x0a };

SqDtexFileHeader::SqDtexFileHeader( const uint32 fileSize, 
		const uint32 imageWidth, const uint32 imageHeight, const uint32 numberOfChannels, const uint32 dataSize, 
		const uint32 tileWidth, const uint32 tileHeight, const uint32 numberOfTiles,
		const float matWorldToScreen[4][4], const float matWorldToCamera[4][4] ) :
	//magicNumber( magicNumber ),
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
{
	setTransformationMatrices( matWorldToScreen, matWorldToCamera );
}

void SqDtexFileHeader::writeToFile( std::ofstream& file ) const
{
	file.write((const char*)magicNumber, 8);
	file.write((const char*)(&fileSize), sizeof(uint32));
	file.write((const char*)(&imageWidth), sizeof(uint32));
	file.write((const char*)(&imageHeight), sizeof(uint32));
	file.write((const char*)(&numberOfChannels), sizeof(uint32));
	file.write((const char*)(&dataSize), sizeof(uint32));
	file.write((const char*)(&tileWidth), sizeof(uint32));
	file.write((const char*)(&tileHeight), sizeof(uint32));
	file.write((const char*)(&numberOfTiles), sizeof(uint32));
	file.write((const char*)(&matWorldToScreen), 16*sizeof(float));
	file.write((const char*)(&matWorldToCamera), 16*sizeof(float));
}

void SqDtexFileHeader::readFromFile( std::ifstream& file )
{
	file.read(magicNumber, 8);
	file.read((char*)(&fileSize), sizeof(uint32));
	file.read((char*)(&imageWidth), sizeof(uint32));
	file.read((char*)(&imageHeight), sizeof(uint32));
	file.read((char*)(&numberOfChannels), sizeof(uint32));
	file.read((char*)(&dataSize), sizeof(uint32));
	file.read((char*)(&tileWidth), sizeof(uint32));
	file.read((char*)(&tileHeight), sizeof(uint32));
	file.read((char*)(&numberOfTiles), sizeof(uint32));
	file.read((char*)(&matWorldToScreen), 16*sizeof(float));
	file.read((char*)(&matWorldToCamera), 16*sizeof(float));	
}

void SqDtexFileHeader::setTransformationMatrices(const float imatWorldToScreen[4][4], 
												const float imatWorldToCamera[4][4])
{
	int x, y;
	for (x = 0; x < 4; ++x)
	{
		for (y = 0; y < 4; ++y)
		{
			matWorldToScreen[x][y] = imatWorldToScreen[x][y];
			matWorldToCamera[x][y] = imatWorldToCamera[x][y];
		}
	}
}

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
	}
	// Load the file header into memory
	m_fileHeader.readFromFile(m_dtexFile);
	// Verify that the magic number is valid
	std::string correctMN = dtexMagicNumber;
	std::string testMN = m_fileHeader.magicNumber;
	if ( testMN != correctMN )
	{
		throw XqInternal(std::string("Magic number in DTEX file: ") + filename 
				+ std::string(" is not valid."), __FILE__, __LINE__);
		m_isValid = false;
	}
	// Calculate some other variables
	m_tilesPerRow = m_fileHeader.imageWidth/m_fileHeader.tileWidth;
	m_tilesPerCol = m_fileHeader.imageHeight/m_fileHeader.tileHeight;
	// Load the tile table into memory
	loadTileTable();
}

boost::shared_ptr<CqDeepTextureTile> CqDeepTexInputFile::tileForPixel( const TqUint x, const TqUint y )
{
	assert( x < m_fileHeader.imageWidth );
	assert( y < m_fileHeader.imageHeight );
	const TqUint tileCol = lceil((float)x/m_fileHeader.tileWidth);
	const TqUint tileRow = lceil((float)y/m_fileHeader.tileHeight);
	const TqUint fileOffset = m_tileOffsets[tileRow][tileCol];
	
	// If fileOffset is 0 then there is no tile to load.
	if (fileOffset != 0)
	{
		return loadTile(tileRow, tileCol);
	}
	boost::shared_ptr<CqDeepTextureTile> nullTile;
	return nullTile; //< The requested tile is empty. Caller may assume visibility is 100%
}

void CqDeepTexInputFile::transformationMatrices( TqFloat matWorldToScreen[4][4], 
		TqFloat matWorldToCamera[4][4] ) const
{
	assert(matWorldToScreen != NULL);
	assert(matWorldToCamera != NULL);
	TqUint x, y;
	for (x = 0; x < 4; ++x)
	{
		for (y = 0; y < 4; ++y)
		{
			assert(matWorldToCamera[x] != NULL);
			assert(matWorldToScreen[x] != NULL);
			matWorldToScreen[x][y] = m_fileHeader.matWorldToScreen[x][y];
			matWorldToCamera[x][y] = m_fileHeader.matWorldToCamera[x][y];
		}
	}
}

void CqDeepTexInputFile::loadTileTable()
{	
	TqUint tcol;
	TqUint trow;
	TqUint offset;
	TqUint i;
	
	// Allocate space to store the tile table as a vector of vectors of file offsets
	m_tileOffsets.resize(m_tilesPerCol);
	for (i = 0; i < m_tilesPerCol; ++i)
	{
		m_tileOffsets[i].resize(m_tilesPerRow, 0);
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

boost::shared_ptr<CqDeepTextureTile> CqDeepTexInputFile::loadTile(const TqUint tileRow, const TqUint tileCol)
{	
	// Allocate sufficient memory to hold the tile
	// Determine the pixel dimensions of this tile so we know how much space to allocate for the meta data
	TqUint tileWidth = m_fileHeader.tileWidth;
	TqUint tileHeight = m_fileHeader.tileHeight;
	const TqUint fileOffset = m_tileOffsets[tileRow*m_fileHeader.tileHeight][tileCol*m_fileHeader.tileWidth];
	
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
	const int totalNumberOfNodes = functionOffsets[tileWidth*tileHeight+1];
	const int nodeSize = m_fileHeader.numberOfChannels+1;
	boost::shared_array<TqFloat> data(new TqFloat[totalNumberOfNodes*nodeSize]);
	m_dtexFile.read((char*)(data.get()), (totalNumberOfNodes*nodeSize*sizeof(TqFloat)));
	
	// Create a new tile from the data we just read
	boost::shared_ptr<CqDeepTextureTile> tile(new CqDeepTextureTile(data, functionOffsets,
			tileWidth, tileHeight, tileCol*m_fileHeader.tileWidth, tileRow*m_fileHeader.tileHeight,
			m_fileHeader.numberOfChannels));
	
	return tile;
}

//------------------------------------------------------------------------------
} // namespace Aqsis
