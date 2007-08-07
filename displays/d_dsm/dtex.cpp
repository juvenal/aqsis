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

CqDeepTexOutputFile::CqDeepTexOutputFile(std::string filename, uint32 imageWidth, uint32 imageHeight, uint32 tileWidth, uint32 tileHeight,
		uint32 bucketWidth, uint32 bucketHeight, uint32 numberOfChannels, uint32 bytesPerChannel)
	: m_dtexFile( filename.c_str(), std::ios::out | std::ios::binary ),
	m_fileHeader( (uint32)0, (uint32)imageWidth, (uint32)imageHeight, (uint32)numberOfChannels, (uint32)bytesPerChannel, (uint32)0, (uint32)0, (uint32)tileWidth, (uint32)tileHeight, (uint32)0 ),
	m_tileTable(),
	m_deepDataTileMap(),
	m_bucketWidth(bucketWidth),
	m_bucketHeight(bucketHeight)
{
	// Note: in the file header initialization above, fileSize and dataSize can't be determine until we receive all the data,
	// so we write 0 now, then seek back and re-write this field before file is closed.
	const int tilesX = imageWidth/tileWidth;
	const int tilesY = imageHeight/tileHeight;
	const int numberOfTiles = tilesX*tilesY; 

	if ((tileWidth % bucketWidth) != 0 || (tileHeight % bucketHeight) != 0)
	{
		throw Aqsis::XqInternal(std::string("Tile dimensions not an integer multiple of bucket dimensions."), __FILE__, __LINE__);
	}
	// Note: The following are set outside of the initialization list because they depend on the calculated constants above
	m_fileHeader.headerSize = sizeof(SqDtexFileHeader) + numberOfTiles*sizeof(SqTileTableEntry);
	m_fileHeader.numberOfTiles = numberOfTiles;
	m_tileTable.reserve(numberOfTiles);
	m_xBucketsPerTile = tileWidth/bucketWidth;
	m_yBucketsPerTile = tileHeight/bucketHeight;

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

// Note: I have not been very consistent here in the naming of metadata, which I also call functionLengths, but have have the name metadata
// to name whatever extra data the display device wants to store in the dtex file, along with the deep data. It just so happens that
// the extra data we want to store for DSMs is the function lengths.
void CqDeepTexOutputFile::setTileData( int xmin, int ymin, int xmax, int ymax, const unsigned char *data, const unsigned char* metadata )
{
	//If we want to enforce that data is received in a bucket at a time, then assert that xmax-xmin <= bucketWidth (may be '<' for edge buckets)
	const float* iData = reinterpret_cast<const float*>(data);
	const int* iMetaData = reinterpret_cast<const int*>(metadata);
	const int imageHeight = m_fileHeader.imageHeight;
	const int imageWidth = m_fileHeader.imageWidth;
	const int tileHeight = m_fileHeader.tileHeight;
	const int tileWidth = m_fileHeader.tileWidth;
	const int xTilesPerImage = imageWidth/tileWidth;
	const int yTilesPerImage = imageHeight/tileHeight;
	
	// Identify what tiles the passed-in data covers
	// If those tiles are not already in the tile map, add them
	// Note: Ideally tile dimensions will be multiples or factors of bucket dimensions so that one
	// may be completely contained within the other annd sorting does not become complicated.
	// Find the tile that contains the top left corner of the goven data. Either all the data will go here,
	// or else to this tiles (greater ID) neighbors. 
	// Note: The ID is the number of the tile if you were to count them in increasing order from left-to-right, and top-to-bottom.
	const int homeTileID = (int)(imageWidth/tileWidth)*ymin+(int)(xmin/tileWidth); // tilesPerRow*numberOfRows + xmin/tileWidth
	// Find the pixel coordinates of this tile's top-left pixel (its origin)
	// Get the row the tile is on, then homeTileYmin is tileHeight*row
	// Get the column the tile is in, and homeTileXmin is tileWidth*col
	const int homeTileRow = homeTileID/xTilesPerImage;
	const int homeTileCol = homeTileID%xTilesPerImage;
	const int homeTileXmin = tileWidth*homeTileCol;
	const int homeTileYmin = tileHeight*homeTileRow;
	const int bucketRelativeXmin = xmin-homeTileXmin; //< xmin relative to tile origin 
	const int bucketRelativeYmin = ymin-homeTileYmin; //< ymin relative to tile origin
	// Note: for now I am proceeding as though all the passed-in data lives within the same sub-region, which is true if
	// data is being sent a bucket at a time.
	const int subRegionID = (int)(tileWidth/m_bucketWidth)*bucketRelativeYmin+(int)(bucketRelativeXmin/m_bucketWidth);
		
	// Copy the data into their respective tiles. Add the tile to the map if it is not already there.
	// If a tile is filled, write it to file, update the tileTable with a m_tileTable.push_back(new SqTileTableEntry), and reclaim tile's memory
	if (m_deepDataTileMap.count(homeTileID) == 0)
	{
		boost::shared_ptr<SqDeepDataTile> newTile(new SqDeepDataTile);
		newTile->subRegions.resize(m_xBucketsPerTile*m_yBucketsPerTile);
		// Create the sub-tile region corresponding to the given data
		newTile->subRegions[subRegionID] = boost::shared_ptr<SqSubTileRegion>(new SqSubTileRegion);
		std::vector< std::vector<int> >& tFunctionLengths = newTile->subRegions[subRegionID]->functionLengths;
		std::vector< std::vector<float> >& tData = newTile->subRegions[subRegionID]->tileData;
		tFunctionLengths.resize(m_bucketHeight);
		tData.resize(m_bucketHeight);
		m_deepDataTileMap[homeTileID].push_back(newTile);
	}
	boost::shared_ptr<SqDeepDataTile> currentTile = m_deepDataTileMap[homeTileID][subRegionID];
	currentTile->subRegions[subRegionID] = boost::shared_ptr<SqSubTileRegion>(new SqSubTileRegion);
	std::vector< std::vector<int> >& tFunctionLengths = currentTile->subRegions[subRegionID]->functionLengths;
	std::vector< std::vector<float> >& tData = currentTile->subRegions[subRegionID]->tileData;
	tFunctionLengths.resize(m_bucketHeight);
	tData.resize(m_bucketHeight);
	// Copy data
	CopyMetaData(tFunctionLengths, iMetaData, xmin, ymin, xmax, ymax);
	CopyData(tData, iData, iMetaData, xmin, ymin, xmax, ymax);
	
	// Check for full tile
	// If full tile, write tile to disk, update the tileTable, and reclaim the tile's memory
	
	// If all tiles have been written, write the tileTable, re-write the datasSize and fileSize fields in the file header, and close the image
	m_dtexFile.close();
	// Return
}

void CqDeepTexOutputFile::CopyMetaData(std::vector< std::vector<int> >& toMetaData, const int* fromMetaData, const int xmin, const int ymin, const int xmax, const int ymax)
{
	
}

void CqDeepTexOutputFile::CopyData(std::vector< std::vector<float> >& toData, const float* fromData, const int* functionLengths, const int xmin, const int ymin, const int xmax, const int ymax)
{
	// Note: I assume that at least a full row of a bucket at a time is sent to the dislay, if not full buckets, but never partial rows.
	//const int numberOfNodesToCopy = NodeCount(functionLengths, (xmax-xmin)*(ymax-ymin)); // Is it possible to use std::accumulate here (on an array)?
	const int rowWidth = xmax-xmin;
	const int colHeight = ymax-ymin;
	const int nodeSize = (m_fileHeader.numberOfChannels+1);
	
	int row;
	int col; 
	
	for(row = 0; row < rowWidth; ++row)
	{
		for (col = 0; col < colHeight; ++col)
		{
			toData[row].push_back(fromData[(row*rowWidth+col)*nodeSize]);
			toData[row].push_back(fromData[(row*rowWidth+col)*nodeSize+1]);
			toData[row].push_back(fromData[(row*rowWidth+col)*nodeSize+2]);
			toData[row].push_back(fromData[(row*rowWidth+col)*nodeSize+3]);
		}
	}
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
