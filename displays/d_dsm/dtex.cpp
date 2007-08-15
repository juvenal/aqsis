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
	// Note: in the file header initialization above, fileSize and dataSize can't be determined until we receive all the data,
	// so we write 0 now, then seek back and re-write these fields before the file is closed.
	const int tilesX = imageWidth/tileWidth;
	const int tilesY = imageHeight/tileHeight;
	const int numberOfTiles = tilesX*tilesY; 

	if ((tileWidth % bucketWidth) != 0 || (tileHeight % bucketHeight) != 0)
	{
		throw Aqsis::XqInternal(std::string("Tile dimensions not an integer multiple of bucket dimensions."), __FILE__, __LINE__);
	}
	// Note: The following are set outside of the initialization list because they depend on the calculated constants above
	// Below: +8 for the magic number, which is currently removed from SqDtexFileHeader.
	m_fileHeader.headerSize = sizeof(SqDtexFileHeader) + 8;
	m_fileHeader.numberOfTiles = numberOfTiles;
	m_tileTable.reserve(numberOfTiles);
	m_xBucketsPerTile = tileWidth/bucketWidth;
	m_yBucketsPerTile = tileHeight/bucketHeight;
	m_fileHeader.fileSize = sizeof(SqDtexFileHeader) + numberOfTiles*sizeof(SqTileTableEntry) + 8; //< This is the first part of fileSize. Add the data size part later.

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

// Note: I have not been consistent here in the naming of metadata, which I also call functionLengths. Metadata
// is whatever extra data the display device wants to store in the dtex file, along with the deep data. It just so happens that
// the extra data we want to store for DSMs is the function lengths.
void CqDeepTexOutputFile::SetTileData( const int xmin, const int ymin, const int xmax, const int ymax, const unsigned char *data, const unsigned char* metadata )
{
	//If we want to enforce that data is received in a bucket at a time, then assert that xmax-xmin <= bucketWidth (may be '<' in the case of edge buckets)
	const float* iData = reinterpret_cast<const float*>(data);
	const int* iMetaData = reinterpret_cast<const int*>(metadata);
	const int imageHeight = m_fileHeader.imageHeight;
	const int imageWidth = m_fileHeader.imageWidth;
	const int tileHeight = m_fileHeader.tileHeight;
	const int tileWidth = m_fileHeader.tileWidth;
	const int tilesPerRow = imageWidth/tileWidth;
	const int tilesPerCol = imageHeight/tileHeight;
	int subRegionsPerRow = tileWidth/m_bucketWidth;
	int homeTileRow = ymin/tileHeight;
	if (homeTileRow >= tilesPerCol)
	{
		homeTileRow = tilesPerCol-1;
	}
	int homeTileCol = xmin/tileWidth;
	if (homeTileCol >= tilesPerRow)
	{
		homeTileCol = tilesPerRow-1; 
	}
	// Find the tile that contains the top left corner of the goven data. Either all the data will go here,
	// or else to this tiles (greater ID) neighbors. 
	// homeTileID is the number of the tile if you were to count them in increasing order from left-to-right, and top-to-bottom.
	const int homeTileID = (homeTileRow*tilesPerRow)+homeTileCol;
	const int homeTileXmin = homeTileCol*tileWidth;  //< x-pixel coordinate of home tile's top-left pixel (its origin)
	const int homeTileYmin = homeTileRow*tileHeight; //< y-pixel coordinate of home tile's top-left pixel (its origin)
	const int relativeXmin = xmin-homeTileXmin;      //< sub-region xmin relative to tile origin 
	const int relativeYmin = ymin-homeTileYmin;      //< sub-region ymin relative to tile origin
	// Note: for now I am proceeding as though all the passed-in data lives within the same sub-region, which is true if
	// data is being sent as full buckets at a time, but not full image scanlines at a time. For the later case, we need to split data between various sub-regions.
	//if ((xmax == imageWidth) && (imageWidth-homeTileXmin > tileWidth))
	if (homeTileCol+1 == tilesPerRow)
	{
		// This tile is a right-padded tile
		subRegionsPerRow += (int)std::ceil((imageWidth-(homeTileXmin+tileWidth))/(float)m_bucketWidth);
	}
	const int subRegionRow = relativeYmin/m_bucketHeight;
	const int subRegionCol = relativeXmin/m_bucketWidth;
	const int subRegionID = (subRegionRow*subRegionsPerRow)+subRegionCol;

	// If the tile that should hold this data has not been created yet, create it.
	if (m_deepDataTileMap.count(homeTileID) == 0)
	{
		CreateNewTile(homeTileID, homeTileRow, homeTileCol);
	}
	boost::shared_ptr<SqDeepDataTile> currentTile = m_deepDataTileMap[homeTileID];
	// If it does not already exist, create the sub-tile region to hold the current data
	if (currentTile->subRegions[subRegionID].get() == NULL)
	{
		CreateNewSubTileRegion(currentTile, subRegionID, ymin);
	}
	std::vector< std::vector<int> >& tFunctionLengths = currentTile->subRegions[subRegionID]->functionLengths;
	std::vector< std::vector<float> >& tData = currentTile->subRegions[subRegionID]->tileData;
	// Check for and deal with the special case of empty buckets
	if (iMetaData[0] == -1)
	{
		// This is an empty sub-region: We should fill tFunctionLengths with all 1's
		// and fill tData with visibility nodes (0,1), one per pixel.
		FillEmptyMetaData(tFunctionLengths, xmin%m_bucketWidth, ymin%m_bucketHeight, xmin%m_bucketWidth+xmax-xmin, ymin%m_bucketHeight+ymax-ymin);
		FillEmptyData(tData, xmin%m_bucketWidth, ymin%m_bucketHeight, xmin%m_bucketWidth+xmax-xmin, ymin%m_bucketHeight+ymax-ymin);
	}	
	else
	{
		// Copy data
		CopyMetaData(tFunctionLengths, iMetaData, xmin%m_bucketWidth, ymin%m_bucketHeight, xmin%m_bucketWidth+xmax-xmin, ymin%m_bucketHeight+ymax-ymin);
		CopyData(tData, iData, iMetaData, xmin%m_bucketWidth, ymin%m_bucketHeight, xmin%m_bucketWidth+xmax-xmin, ymin%m_bucketHeight+ymax-ymin);
	}	
	// Check for full tile
	// If full tile, write tile to disk, update the tileTable, and reclaim the tile's memory
	if(IsFullTile(currentTile, homeTileID))
	{
		if (IsNeglectableTile(currentTile))
		{
			SqTileTableEntry entry(currentTile->tileCol, currentTile->tileRow, 0);
			m_tileTable.push_back(entry);
		}
		else
		{
			UpdateTileTable(currentTile);
			WriteTile(currentTile);
		}
		// How should the memory be reclaimed? Maybe like this:
		m_deepDataTileMap.erase(homeTileID);
	}
	// If all tiles have been written, write the tileTable, re-write the datasSize and fileSize fields in the file header, and close the image
	// We know all tiles have been written if the tile table is full
	if (m_tileTable.size() == m_fileHeader.numberOfTiles-1)
	{
		WriteTileTable();
		// Re-write the dataSize and fileSize fields which were not written properly in the constructor
		// Seek to the 9th byte position in the file and write fileSize field
		m_dtexFile.seekp(9, std::ios::beg);
		m_fileHeader.fileSize += m_fileHeader.dataSize;
		m_dtexFile.write((const char*)(&m_fileHeader.fileSize), sizeof(uint32));
		
		// Seek to the 29th byte position in the file and write dataSize.
		m_dtexFile.seekp(29, std::ios::beg);
		m_dtexFile.write((const char*)(&m_fileHeader.dataSize), sizeof(uint32));
		
		m_dtexFile.close();
	}
}

void CqDeepTexOutputFile::CreateNewSubTileRegion(boost::shared_ptr<SqDeepDataTile> currentTile, const int subRegionID, const int ymin)
{
	// If this sub-region is at the bottom of the image, and the image is not evenly divisible by bucket height,
	// then this sub-region is a (bottom) tile padding region, and its height should be less than the standard sub-region height.
	int subRegionHeight = m_bucketHeight;
	
	if (ymin+m_bucketHeight > m_fileHeader.imageHeight)
	{
		subRegionHeight = m_fileHeader.imageHeight-ymin;	
	}
	currentTile->subRegions[subRegionID] = boost::shared_ptr<SqSubTileRegion>(new SqSubTileRegion);
	currentTile->subRegions[subRegionID]->functionLengths.resize(subRegionHeight);
	currentTile->subRegions[subRegionID]->tileData.resize(subRegionHeight);
}

void CqDeepTexOutputFile::CreateNewTile(const int tileID, const int tileRow, const int tileCol)
{
	const int imageHeight = m_fileHeader.imageHeight;
	const int imageWidth = m_fileHeader.imageWidth;
	const int tileHeight = m_fileHeader.tileHeight;
	const int tileWidth = m_fileHeader.tileWidth;
//	const int tileXmin = tileCol*tileWidth;
//	const int tileYmin = tileRow*tileHeight;
	const int tilesPerRow = imageWidth/tileWidth;
	const int tilesPerCol = imageHeight/tileHeight;
	int xSize = m_xBucketsPerTile;
	int ySize = m_yBucketsPerTile;

	// If this is an end tile, and the image width or height is not divisible by tile width or height,
	// Then perform padding on the tile: resize to a larger size to accomodate the extra data.
	if (tileCol == (tilesPerRow-1)) //< subtract 1 from tilesPerRow since tileCol is zero-indexed (tileCol == 2 -> it is the 3rd tile from the left)
	{
		xSize += (int)std::ceil((imageWidth%tileWidth)/(float)m_bucketWidth);
	}
	if (tileRow == (tilesPerCol-1))
	{
		ySize += (int)std::ceil((imageHeight%tileHeight)/(float)m_bucketHeight);
	}	
	boost::shared_ptr<SqDeepDataTile> newTile(new SqDeepDataTile);
	newTile->subRegions.resize(xSize*ySize);		
	newTile->tileCol = tileCol;
	newTile->tileRow = tileRow;
	m_deepDataTileMap[tileID] = newTile;
}

void CqDeepTexOutputFile::CopyMetaData(std::vector< std::vector<int> >& toMetaData, const int* fromMetaData, const int rxmin, const int rymin, const int rxmax, const int rymax)
{
	// Note: I assume that at least a full row of a bucket at a time is sent to the dislay, or even full buckets, but never partial rows. This code will break if partial rows are sent.
	int row;
	int col; 
	int i, readPos = 0;
	
	for(row = rymin; row < rymax; ++row)
	{
		for (col = rxmin; col < rxmax; ++col)
		{
			toMetaData[row].push_back(fromMetaData[readPos]);
			++readPos;
		}
	}	
}

void CqDeepTexOutputFile::CopyData(std::vector< std::vector<float> >& toData, const float* fromData, const int* functionLengths, const int rxmin, const int rymin, const int rxmax, const int rymax)
{
	// Note: I assume that at least a full row of a bucket at a time is sent to the dislay, if not full buckets, but never partial rows. This code will break if partial rows are sent.
	const int nodeSize = (m_fileHeader.numberOfChannels+1);
	int row;
	int col; 
	int i, readPos = 0;
	
	for(row = rymin; row < rymax; ++row)
	{
		for (col = rxmin; col < rxmax; ++col)
		{
			const int numberOfElements = functionLengths[readPos]*nodeSize;
			i = 0;
			while(i < numberOfElements)
			{
				toData[row].push_back(fromData[readPos+i]);
				++i;
			}
			++readPos;
		}
	}
}

void CqDeepTexOutputFile::FillEmptyMetaData(std::vector< std::vector<int> >& toMetaData, const int rxmin, const int rymin, const int rxmax, const int rymax)
{
	// Note: I assume that at least a full row of a bucket at a time is sent to the dislay, or even full buckets, but never partial rows.
	int row;
	int col; 
	int i;
	
	for(row = rymin; row < rymax; ++row)
	{
		for (col = rxmin; col < rxmax; ++col)
		{
			toMetaData[row].push_back(1);
		}
	}	
}

void CqDeepTexOutputFile::FillEmptyData(std::vector< std::vector<float> >& toData, const int rxmin, const int rymin, const int rxmax, const int rymax)
{
	// Note: I assume that at least a full row of a bucket at a time is sent to the dislay, if not full buckets, but never partial rows.
	const int nodeSize = (m_fileHeader.numberOfChannels+1);
	int row;
	int col; 
	int i;
	
	for(row = rymin; row < rymax; ++row)
	{
		for (col = rxmin; col < rxmax; ++col)
		{
			// First set the node depth to 0 
			toData[row].push_back(0.0);
			// Then set the visibility to 100%
			i = 0;
			while(i < m_fileHeader.numberOfChannels)
			{
				toData[row].push_back(1.0);
				++i;
			}
		}
	}
}

bool CqDeepTexOutputFile::IsFullTile(const boost::shared_ptr<SqDeepDataTile> tile, const int tileID) const
{
	// A tile is known to be full if all of its sub-tile regions have function lengths for all of their pixels.
	// Another way of saying this: the tile is full if and only if there are a total of tileWidth*tileHeight function lengths, or in the case os padded tiles,
	// ((tileWidth+padWidth)*(tileHeight+padHeight)) function length fields.
	const int imageHeight = m_fileHeader.imageHeight;
	const int imageWidth = m_fileHeader.imageWidth;
	const int tileHeight = m_fileHeader.tileHeight;
	const int tileWidth = m_fileHeader.tileWidth;
	const int tileRow = tileID/(imageWidth/tileWidth);
	const int tileCol = tileID%(imageWidth/tileWidth);
	const int tileXmin = tileCol*tileWidth;
	const int tileYmin = tileRow*tileHeight;
	const int tilesPerRow = imageWidth/tileWidth;
	const int tilePerCol = imageHeight/tileHeight;
	int functionCount = 0;
	int padWidth = 0;
	int padHeight = 0;
	int i, j;
	
	// If this is an end tile, and the image width or height is not divisible by tile width or height,
	// Then this is a padded tile: resize to a larger size to accomodate the extra data.
	if((tileCol == (tilesPerRow-1)) && (imageWidth%tileWidth != 0)) //< subtract 1 from tilesPerRow since tileCol is zero-indexed (tileCol == 2 -> it is the 3rd tile from the left)
	{
		padWidth = imageWidth%tileWidth;
	}
	if((tileRow == (tilePerCol-1)) && (imageHeight%tileHeight != 0))
	{
		padHeight = imageHeight%tileHeight;
	}
	
	const std::vector<boost::shared_ptr<SqSubTileRegion> >& subRegions = tile->subRegions;
	for(i = 0; i < subRegions.size(); ++i)
	{	
		if (subRegions[i].get() == NULL)
		{
			// This subregion is empty, so we know the tile is not full.
			return false;
		}
		const std::vector<std::vector<int> >& subRegionFunctionLengths = subRegions[i]->functionLengths;
		for (j = 0; j < subRegionFunctionLengths.size(); ++j)
		{
			functionCount += subRegionFunctionLengths[j].size(); //< size() yeilds the count of all functions in a sub-region row 
		}
	}
	if (functionCount == (m_fileHeader.tileWidth+padWidth)*(m_fileHeader.tileHeight+padHeight))
	{
		return true;
	}
	return false;
}

bool CqDeepTexOutputFile::IsNeglectableTile(const boost::shared_ptr<SqDeepDataTile> tile) const
{
	// If all function lengths in the tile have length 1, then the tile is empty, and we do not write it to disk, but instead signify an empty tile by writing a file offset
	// of 0 in the tile table.
	int i, j, k;
	const std::vector<boost::shared_ptr<SqSubTileRegion> >& subRegions = tile->subRegions;
	
	for(i = 0; i < subRegions.size(); ++i)
	{	
		if (subRegions[i].get() == NULL)
		{
			// This subregion is empty, so we know the tile is not full.
			// Note: A tile must be full in order for us to determine whether or not it is neglectable.
			// IsFullTile() should be invoked immediately before any calls to IsNeglectableTile.
			return false;
		}
		const std::vector<std::vector<int> >& subRegionFunctionLengths = subRegions[i]->functionLengths;
		for (j = 0; j < subRegionFunctionLengths.size(); ++j)
		{
			for (k = 0; k < subRegionFunctionLengths[j].size(); ++k)
			{
				if (subRegionFunctionLengths[j][k] != 1)
				{
					return false;
				}
			}
		}
	}
	return true;
}

void CqDeepTexOutputFile::UpdateTileTable(const boost::shared_ptr<SqDeepDataTile> tile)
{
	// Set the file offset to the current write-file position (given by tellp()).
	SqTileTableEntry entry(tile->tileCol, tile->tileRow, m_dtexFile.tellp());
	m_tileTable.push_back(entry);
}

void CqDeepTexOutputFile::WriteTileTable()
{
	// Seek to the 65th byte position in the file and write the tile table
	m_dtexFile.seekp(65, std::ios::beg);

	// I assume that the member data in a C struct is gauranteed to be contiguous and ordered in memory to reflect the order of declaration.
	// This is very likely an incorrect assumption, but it might be true in the case of SqTileTableEntry, since it has simply 3 floats.
	// If not, then we have to write each float individually.
	m_dtexFile.write(reinterpret_cast<const char*>(&(m_tileTable.front())), m_tileTable.size()*sizeof(SqTileTableEntry));
}

void CqDeepTexOutputFile::WriteTile(const boost::shared_ptr<SqDeepDataTile> tile)
{
	/// \todo Currently I write to file one sub-region row at a time, which results in many calls to write(), and hence many disk acesses,
	/// which are slow. I should instead rebuild the tile in a contiguous region in memory, then write it to disk all at once. 
	int i, j;
	int tileSizeInBytes = 0;
	const std::vector<boost::shared_ptr<SqSubTileRegion> >& subRegions = tile->subRegions;
	
	// First write all the function lengths
	for(i = 0; i < subRegions.size(); ++i)
	{	
		const std::vector<std::vector<int> >& subRegionFunctionLengths = subRegions[i]->functionLengths;
		for (j = 0; j < subRegionFunctionLengths.size(); ++j)
		{
			m_dtexFile.write(reinterpret_cast<const char*>(&(subRegionFunctionLengths[j].front())), subRegionFunctionLengths[j].size()*sizeof(uint32));
		}
		tileSizeInBytes += subRegionFunctionLengths.size()*sizeof(float);
	}
	
	// Then write the data. Note we are not concerned that tiles are placed sequentially in the file, as long as the tile table is correct.
	for(i = 0; i < subRegions.size(); ++i)
	{	
		const std::vector<std::vector<float> >& subRegionData = subRegions[i]->tileData;
		for (j = 0; j < subRegionData.size(); ++j)
		{
			m_dtexFile.write(reinterpret_cast<const char*>(&(subRegionData[j].front())), subRegionData[j].size()*sizeof(float));
			tileSizeInBytes += subRegionData[j].size()*sizeof(float);
		}
	}
	m_fileHeader.dataSize += tileSizeInBytes;
}
