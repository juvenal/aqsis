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

#include <cmath>

#include "dtexoutput.h"
#include "exception.h"

namespace Aqsis 
{

// Magic number for a DTEX file is: "\0x89AqD\0x0b\0x0a\0x16\0x0a" Note 0x417144 represents ASCII AqD
static const char dtexMagicNumber[8] = { 0x89, 'A', 'q', 'D', 0x0b, 0x0a, 0x16, 0x0a };

SqDtexFileHeader::SqDtexFileHeader( const uint32 fileSize, const uint32 imageWidth, 
		const uint32 imageHeight, const uint32 numberOfChannels, const uint32 dataSize, 
		const uint32 tileWidth, const uint32 tileHeight, const uint32 numberOfTiles,
		const float matWorldToScreen[4][4], const float matWorldToCamera[4][4]) :
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
			matWorldToScreen[x][y] = matWorldToScreen[x][y];
			matWorldToCamera[x][y] = matWorldToCamera[x][y];
		}
	}
}

CqDeepTexOutputFile::CqDeepTexOutputFile(std::string filename, uint32 imageWidth, uint32 imageHeight,
		uint32 tileWidth, uint32 tileHeight, uint32 bucketWidth, uint32 bucketHeight, uint32 numberOfChannels,
		const float matWorldToScreen[4][4], const float matWorldToCamera[4][4]) :
			m_dtexFile( filename.c_str(), std::ios::out | std::ios::binary ),
			m_fileHeader( (uint32)0, (uint32)imageWidth, (uint32)imageHeight, 
						(uint32)numberOfChannels, (uint32)0, (uint32)tileWidth, (uint32)tileHeight, 
						((imageWidth/tileWidth)*(imageHeight/tileHeight)), matWorldToScreen, matWorldToCamera),
			m_tileTable(),
			m_tileTablePositon( 0 ),
			m_xBucketsPerTile( tileWidth/bucketWidth ),
			m_yBucketsPerTile( tileHeight/bucketHeight ),
			m_bucketWidth( bucketWidth ),
			m_bucketHeight( bucketHeight )
{
	// Note: in the file header initialization above, fileSize and dataSize can't be determined 
	// until we receive all the data so we write 0 now, then seek back and re-write these fields 
	// before the file is closed.
	const int tilesX = imageWidth/tileWidth;
	const int tilesY = imageHeight/tileHeight;
	const int numberOfTiles = tilesX*tilesY; 

	// If file open failed, throw an exception. This happens for example, if the file already existed,
	// or the destination directory did not have write permission.
	if (!m_dtexFile.is_open())
	{
		throw Aqsis::XqInternal(std::string("Failed to open file \"") + filename +
				std::string( "\""), __FILE__, __LINE__);
	}
	if ((tileWidth % bucketWidth) != 0 || (tileHeight % bucketHeight) != 0)
	{
		throw Aqsis::XqInternal(std::string("User specified tile dimensions are not an integer multiple "
				"of bucket dimensions."), __FILE__, __LINE__);
	}
	m_tileTable.reserve(numberOfTiles); 
	
	// Write the file header
	m_fileHeader.writeToFile( m_dtexFile );
	
	// Save the file position so that we may return to this spot later, to write the tile table.
	m_tileTablePositon = m_dtexFile.tellp();
	// This is the first part of fileSize. Add the data size part later.
	m_fileHeader.fileSize = m_tileTablePositon + numberOfTiles*sizeof(SqTileTableEntry);
	// Seek forward in file to reserve space for writing the tile table later.
	// Seek from the current positon. 
	// Alternatively, you could seek from the file's beginning with std::ios::beg
	m_dtexFile.seekp(numberOfTiles*sizeof(SqTileTableEntry), std::ios::cur);
}

CqDeepTexOutputFile::~CqDeepTexOutputFile()
{
	// Finish by writing the tile table to file.
	writeTileTable();
	// re-write the datasSize and fileSize fields in the file header, then we are done.
	// Re-write the dataSize and fileSize fields which were not written properly in the constructor
	// Seek to the 9th byte position in the file and write fileSize field
	m_dtexFile.seekp(9, std::ios::beg);
	m_fileHeader.fileSize += m_fileHeader.dataSize;
	m_dtexFile.write((const char*)(&m_fileHeader.fileSize), sizeof(TqUint32));
	
	// Seek to the 25th byte position in the file and write dataSize.
	m_dtexFile.seekp(25, std::ios::beg);
	m_dtexFile.write((const char*)(&m_fileHeader.dataSize), sizeof(TqUint32));
}

void CqDeepTexOutputFile::updateTileTable(const boost::shared_ptr<CqDeepTextureTile> tile)
{
	// Set the file offset to the current write-file position (given by tellp()).
	// If the tile is empty, then it will not be written to disk, so use an offset of 0;
	TqUlong offset = m_dtexFile.tellp();
	if (tile->isEmpty())
	{
		offset = 0;
	}
	SqTileTableEntry entry(tile->col, tile->row, offset);
	m_tileTable.push_back(entry);
}

void CqDeepTexOutputFile::writeTileTable()
{
	// Seek to the correct byte position in the file, immediately following the file header, and write the tile table
	m_dtexFile.seekp(m_tileTablePositon, std::ios::beg);

	// I assume that the member data in a C struct is gauranteed to be contiguous and ordered in memory to reflect the order of declaration.
	// This is very likely an incorrect assumption, but it might be true in the case of SqTileTableEntry, since it has simply 3 floats.
	// If not, then we have to write each float individually.
	m_dtexFile.write(reinterpret_cast<const char*>(&(m_tileTable.front())), m_tileTable.size()*sizeof(SqTileTableEntry));
}

void CqDeepTexOutputFile::outputTile( const boost::shared_ptr<CqDeepTextureTile> tile )
{
	// Update the tile table with this tile.
	updateTileTable(tile);
	
	// Check to see if this tile is "neglectable" in the dtex file,
	// that is, see if it is an empty tile. If it is, we do not write it to disc. Return now.
	if (tile->isEmpty())
	{
		return;
	}
	// Otherwise, write tile to disc.
	writeTile(tile_ptr);
}

void CqDeepTexOutputFile::writeTile(const boost::shared_ptr<CqDeepTextureTile> tile)
{
	const TqUint metaDataSizeInBytes = tile->width()*tile->height()+1*sizeof(TqUint32);
	const TqUint dataSizeInBytes = (tile->funcOffsets()[tile->width()*tile->height()+1])*
									(tile->colorChannels()+1)*sizeof(TqFloat);
	m_fileHeader.dataSize += metaDataSizeInBytes + dataSizeInBytes;
	
	// Write the function offsets
	m_dtexFile.write(reinterpret_cast<const char*>(tile->funcOffsets()), metaDataSizeInBytes);
	
	// Write the deep data
	m_dtexFile.write(reinterpret_cast<const char*>(tile->data()), dataSizeInBytes);
}

bool CqDeepTexOutputFile::isNeglectableTile(const boost::shared_ptr<CqDeepTextureTile> tile) const
{
	// If all visibility functions in this tile have only 1 node
	// then the tile is empty (visibility is 100% everywhere in the tile).
	// We can test if this is so simply by checking if the last offset in the 
	// tile's function offsets is equal to the number of pixels in the tile.
	if ( tile->funcOffsets()[tile->width()*tile->height()+1] == (tile->width()*tile->height()) )
	{
		return true;	
	}
	return false;
}

//------------------------------------------------------------------------------
} // namespace Aqsis
