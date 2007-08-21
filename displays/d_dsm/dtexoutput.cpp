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

// Magic number for a DTEX file is: "\0x89AqD\0x0b\0x0a\0x16\0x0a" Note 0x417144 represents ASCII AqD
static const char magicNumber[8] = { 0x89, 'A', 'q', 'D', 0x0b, 0x0a, 0x16, 0x0a };

SqDtexFileHeader( const char* magicNumber = NULL, const uint32 fileSize = 0, const uint32 imageWidth = 0, 
		const uint32 imageHeight = 0, const uint32 numberOfChannels = 0, const uint32 dataSize = 0, 
		const uint32 tileWidth = 0, const uint32 tileHeight = 0, const uint32 numberOfTiles = 0,
		const float matWorldToScreen[4][4] = NULL, const float matWorldToCamera[4][4] = NULL) :
	magicNumber( magicNumber ),
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
	// Write the magic number
	file.write(magicNumber, 8);
	file.write((const char*)(&fileSize), sizeof(uint32));
	file.write((const char*)(&imageWidth), sizeof(uint32));
	file.write((const char*)(&imageHeight), sizeof(uint32));
	file.write((const char*)(&numberOfChannels), sizeof(uint32));
	file.write((const char*)(&dataSize), sizeof(uint32));
	file.write((const char*)(&tileWidth), sizeof(uint32));
	file.write((const char*)(&tileHeight), sizeof(uint32));
	file.write((const char*)(&numberOfChannels), sizeof(uint32));
	file.write((const char*)(&matWorldToScreen), 16*sizeof(float));
	file.write((const char*)(&matWorldToCamera), 16*sizeof(float));
}

void SqDtexFileHeader::readFromFile( std::ifstream& file )
{
	file.read(magicNumber, 8);
	file.read((const char*)(&fileSize), sizeof(uint32));
	file.read((const char*)(&imageWidth), sizeof(uint32));
	file.read((const char*)(&imageHeight), sizeof(uint32));
	file.read((const char*)(&numberOfChannels), sizeof(uint32));
	file.read((const char*)(&dataSize), sizeof(uint32));
	file.read((const char*)(&tileWidth), sizeof(uint32));
	file.read((const char*)(&tileHeight), sizeof(uint32));
	file.read((const char*)(&numberOfChannels), sizeof(uint32));
	file.read((const char*)(&matWorldToScreen), 16*sizeof(float));
	file.read((const char*)(&matWorldToCamera), 16*sizeof(float));	
}

void SqDtexFileHeader::setTransformationMatrices(const float matWorldToScreen[4][4], 
												const float matWorldToCamera[4][4])
{
	int x, y;
	for (x = 0; x < 4; ++x)
	{
		for (y = 0; y < 4; ++y)
		{
			m_fileHeader.matWorldToScreen[x][y] = matWorldToScreen[x][y];
			m_fileHeader.matWorldToCamera[x][y] = matWorldToCamera[x][y];
		}
	}
}

CqDeepTexOutputFile::CqDeepTexOutputFile(std::string filename, uint32 imageWidth, uint32 imageHeight,
		uint32 tileWidth, uint32 tileHeight, uint32 bucketWidth, uint32 bucketHeight, uint32 numberOfChannels,
		const float matWorldToScreen[4][4], const float matWorldToCamera[4][4]) :
			m_dtexFile( filename.c_str(), std::ios::out | std::ios::binary ),
			m_fileHeader( magicNumber, (uint32)0, (uint32)imageWidth, (uint32)imageHeight, 
						(uint32)numberOfChannels, (uint32)0, (uint32)tileWidth, (uint32)tileHeight, 
						((imageWidth/tileWidth)*(imageHeight/tileHeight;)),matWorldToScreen, matWorldToCamera),
			m_tileTable(),
			m_tileTablePositon(0),
			m_xBucketsPerTile(tileWidth/bucketWidth),
			m_yBucketsPerTile(tileHeight/bucketHeight),
			m_bucketWidth(bucketWidth),
			m_bucketHeight(bucketHeight)
{
	// Note: in the file header initialization above, fileSize and dataSize can't be determined 
	// until we receive all the data so we write 0 now, then seek back and re-write these fields 
	// before the file is closed.
	const int tilesX = imageWidth/tileWidth;
	const int tilesY = imageHeight/tileHeight;
	const int numberOfTiles = tilesX*tilesY; 

	// If file open failed, throw an exception. This happens for example, if the file already existed, or the destination directory 
	// did not have write permission.
	if (!m_dtexFile.is_open())
	{
		throw Aqsis::XqInternal(std::string("Failed to open file \"") + filename + std::string( "\""), __FILE__, __LINE__);
	}
	if ((tileWidth % bucketWidth) != 0 || (tileHeight % bucketHeight) != 0)
	{
		throw Aqsis::XqInternal(std::string("Tile dimensions not an integer multiple of bucket dimensions."), __FILE__, __LINE__);
	}
	m_tileTable.reserve(numberOfTiles); 
	
	// Write the file header
	m_fileHeader.writeToFile( m_dtexFile );
	
	// Save the file position so that we may return to this spot later, to write the tile table.
	m_tileTablePositon = m_dtexFile.tellp();
	// This is the first part of fileSize. Add the data size part later.
	m_fileHeader.fileSize = m_tileTablePositon + numberOfTiles*sizeof(SqTileTableEntry);
	// Seek forward in file to reserve space for writing the tile table later.
	// Seek from the current positon. Alternatively, you could seek from the file's beginning with std::ios::beg
	m_dtexFile.seekp(numberOfTiles*sizeof(SqTileTableEntry), std::ios::cur);
}

CqDeepTexOutputFile::~CqDeepTexOutputFile()
{}

void CqDeepTexOutputFile::updateTileTable(const boost::shared_ptr<SqDeepDataTile> tile)
{
	// Set the file offset to the current write-file position (given by tellp()).
	SqTileTableEntry entry(tile->tileCol, tile->tileRow, m_dtexFile.tellp());
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

void CqDeepTexOutputFile::writeTile(const boost::shared_ptr<SqDeepDataTile> tile)
{
	/// \todo Currently I write to file one sub-region row at a time, which results in many calls to 
	/// write(), and hence many disk acesses, which are slow. 
	// I should instead rebuild the tile in a contiguous region in memory, then write it to disk all at once.
	// On second thought, the file stream does some buffering of its own, so maybe we don't have to.
	int j, k;
	int currentOffset = 0;
	int tileSizeInBytes = 0;
	const TqSubRegionMap& subRegions = tile->subRegionMap;
	TqSubRegionMap::const_iterator it;
	
	// Iterate over all the function lengths, converting them to offsets then writing to file
	std::vector<TqUint32> offsets;
	offsets.push(currentOffset);
	for (it = subRegionMap.begin(); it != subRegionMap.end(); ++it)
	{	
		const std::vector<std::vector<int> >& subRegionFunctionLengths = (*it)->functionLengths;
		for (j = 0; j < subRegionFunctionLengths.size(); ++j)
		{
			for (k = 0; k < subRegionFunctionLengths[j].size(); ++k)
			{
				currentOffset += subRegionFunctionLengths[j][k];
				offsets.push_back(currentOffset);
			}
		}
	}
	m_dtexFile.write(reinterpret_cast<const char*>(&(offsets.front())), offsets.size()*sizeof(TqUint32));	
	tileSizeInBytes += offsets.size()*sizeof(float);
	
	// Then write the data. Note we are not concerned that tiles are placed sequentially in the file,
	// as long as the tile table is correct.
	for (it = subRegionMap.begin(); it != subRegionMap.end(); ++it)
	{	
		const std::vector<std::vector<float> >& subRegionData = (*it)->tileData;
		for (j = 0; j < subRegionData.size(); ++j)
		{
			m_dtexFile.write(reinterpret_cast<const char*>(&(subRegionData[j].front())), subRegionData[j].size()*sizeof(float));
			tileSizeInBytes += subRegionData[j].size()*sizeof(float);
		}
	}
	m_fileHeader.dataSize += tileSizeInBytes;
}
