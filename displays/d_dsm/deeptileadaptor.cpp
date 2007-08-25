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
 * \brief Implementation of deep tile caching machinery. 
*/

#include <cmath>

#include "deeptileadaptor.h"
#include "exception.h"
#include "aqsismath.h"

namespace Aqsis
{

SqDeepDataTile::SqDeepDataTile( const TqUint32 col, const TqUint32 row,
				const TqUint32 width, const TqUint32 height ) :
					subRegionMap(),
					col(col),
					row(row),
					width(width),
					height(height)
{}

void SqDeepDataTile::addSubRegion( boost::shared_ptr<CqDeepTextureTile> tile, TqMapKey subRegionKey )
{
	subRegionMap[subRegionKey] = tile;
}

CqDeepTileAdaptor::CqDeepTileAdaptor( boost::shared_ptr<IqDeepTextureOutput> outputObject, 
		TqUint32 imageWidth, TqUint32 imageHeight, TqUint32 tileWidth, TqUint32 tileHeight, 
		TqUint32 bucketWidth, TqUint32 bucketHeight, TqUint32 numberOfChannels) :
			m_deepTexOutput( outputObject ),
			m_imageWidth( imageWidth ),
			m_imageHeight( imageHeight ),
			m_tileWidth( tileWidth ),
			m_tileHeight( tileHeight ),
			m_bucketWidth( bucketWidth ),
			m_bucketHeight( bucketHeight ),
			m_tilesPerImageRow( lceil((float)imageWidth/tileWidth) ),
			m_tilesPerImageCol (lceil((float)imageHeight/tileHeight) ),
			m_bucketsPerTileRow( tileWidth/bucketWidth ),
			m_bucketsPerTileCol( tileHeight/bucketHeight ),
			m_numberOfChannels( numberOfChannels )		
{}

void CqDeepTileAdaptor::addTile( const boost::shared_ptr<CqDeepTextureTile> tile )
{
	// If this is already a full size tile, then send it to output and we are done.
	if ( (tile->width() == m_tileWidth) && (tile->height() == m_tileHeight) )
	{
		m_deepTexOutput.outputTile( tile );
		return;
	}
	
	const int homeTileTopLeftX = tile->topLeftX()/m_tileWidth;
	const int homeTileTopLeftY = tile->topLeftY()/m_tileHeight;
	const TqMapKey homeTileKey(homeTileTopLeftX, homeTileTopLeftY);
	const int subRegionTopLeftX = tile->topLeftX()-homeTileTopLeftX;
	const int subRegionTopLeftY = tile->topLeftY()-homeTileTopLeftY;
	const TqMapKey subRegionKey(subRegionTopLeftX, subRegionTopLeftY);
	
	// If the tile that should hold this sebRegion has not been created yet, create it.
	if (m_deepTileMap.count(homeTileKey) == 0)
	{
		createNewTile(homeTileKey);
	}
	boost::shared_ptr<SqDeepDataTile> homeTile = m_deepTileMap[homeTileKey];
	// If it does not already exist, create the sub-tile region
	if (currentTile->subRegionMap.count(subRegionKey) == 0)
	{
		// Store tile as a sub-region of homeTile
		homeTile->addSubRegion( tile, subRegionKey );
	}

	// Check for full tile
	// If full tile, send to output and reclaim memory
	if(isFullTile(homeTileKey))
	{
		rebuildAndOutputTile(homeTileKey);
	}
}

void CqDeepTileAdaptor::rebuildAndOutputTile(const TqMapKey tileKey)
{
	// Construct a single CqDeepTextureTile from its sub-regions
	// If all sub-region tiles are empty, then construct an empty tile
	// Otherwise construct a full tile, filling in any empty regions with default data
	const boost::shared_ptr<SqDeepDataTile> sourceTile = m_deepTileMap[tileKey];
	const TqSubRegionMap& subRegionMap = sourceTile->subRegionMap;
	boost::shared_ptr<CqDeepTextureTile> completeTile;
	boost::shared_array<TqFloat> data;
	boost::shared_array<TqInt> funcOffsets;
	TqUint nodeCount;
	
	if ( allSubRegionsEmpty(tileKey) )
	{
		funcOffsets = boost::shared_array<TqInt>(new TqInt[1] );
		funcOffsets[0] = -1;
	}
	else
	{
		// We cannot predict how much space must be allocated for deep data,
		// but we know there is exactly one function offset for each pixel, plus one at the end.
		funcOffsets = boost::shared_array<TqInt>(new TqInt[sourceTile->tileWidth*sourceTile->tileHeight+1]);	
		// Now we need to actually recalculate function offsets. We cannot copy the exisiting offsets
		// because they are all relative to their own sub-regions. We want offsets relative to the larger tile.
		nodeCount = rebuildFunctionOffsets(funcOffsets, tileKey);
		// Now we know how much data to allocate, so do it
		data = boost::shared_array<TqFloat>(new TqFloat[nodeCount*nodeSize]);
		// And now for another round of data copy
		rebuildVisibilityFunctions(data, tileKey);
	}
	
	completeTile = boost::shared_ptr<CqDeepTextureTile>( newCqDeepTextureTile(data, funcOffsets,
			sourcetile->Width, sourceTile->Height, tileKey.second, tileKey.first, TqUint colorChannels));
	// Add this full tile to the deep texture output file
	m_deepTexOutput.outputTile( completeTile );
	// Reclaim tile memory no longer needed
	m_deepTileMap.erase(tileKey);
}

TqUint CqDeepTileAdaptor::rebuildFunctionOffsets(boost::shared_array<TqInt> funcOffsets, const TqMapKey tileKey)
{
	const boost::shared_ptr<SqDeepDataTile> sourceTile = m_deepTileMap[tileKey];
	const TqSubRegionMap& subRegionMap = sourceTile->subRegionMap;
	boost::shared_ptr<CqDeepTextureTile> subTile;
	TqUint pixel;
	TqUint rowIndex;
	TqUint subRowIndex;
	TqUint colIndex;
	TqUint subRegionWidth;
	TqUint currentOffset = 0;
	TqMapKey mapKey(0,0);
	TqUint funcOffsetsArrayPos = 0;
	TqUint subTileWidth = subRegionMap[mapKey]->textureTile->width();
	TqUint subTileHeight = subRegionMap[mapKey]->textureTile->height();
	const TqUint tilesPerCol = lceil((float)sourceTile->tileHeight/subTileHeight);
	const TqUint tilesPerRow = lceil((float)sourceTile->tileWidth/subTileWidth);
	
	for ( rowIndex = 0; rowIndex < tilesPerCol; ++rowIndex )
	{
		mapKey.first = rowIndex*subTileHeight; mapKey.second = 0;
		subTileHeight = subRegionMap[mapKey]->second->textureTile->height();
		for (subRowIndex = 0; subRowIndex < subTileHeight; ++subRowIndex)
		{
			for (colIndex = 0; colIndex < tilesPerRow; ++colIndex)
			{
				mapKey.first = rowIndex*subTileHeight; mapKey.second = colIndex*subTileWidth;
				subTile = subRegionMap[mapKey]->second->textureTile;
				subRegionWidth = subTile->width();
				if (subTile->isEmpty())
				{
					// Fill default values: offset increases by 1 (function lengths == 1)
					for (pixel = 0; pixel < subRegionWidth; ++pixel)
					{
						funcOffsets[currentOffsetArrayPos] = currentOffset;
						currentOffsetArrayPos++;
						currentOffset++;
					}
				}
				else
				{
					const TqInt* sourceFuncOffsets = subTile->funcOffsets()+(subRowIndex*subRegionWidth);
					for (pixel = 0; pixel < subRegionWidth; ++pixel)
					{
						funcOffsets[currentOffsetArrayPos] = currentOffset;
						currentOffsetArrayPos++;
						currentOffset += sourceFuncOffsets[pixel+1]-sourceFuncOffsets[pixel];
					}
				}
			}
		}
	}
	return currentOffset;
}

void CqDeepTileAdaptor::rebuildVisibilityFunctions(boost::shared_array<TqFloat> data, const TqMapKey tileKey)
{
	const boost::shared_ptr<SqDeepDataTile> sourceTile = m_deepTileMap[tileKey];
	const TqSubRegionMap& subRegionMap = sourceTile->subRegionMap;
	boost::shared_ptr<CqDeepTextureTile> subTile;
	TqUint nodeCount;
	TqUint pixel;
	TqUint i;
	TqUint rowIndex;
	TqUint subRowIndex;
	TqUint colIndex;
	TqUint subRegionWidth;
	TqUint rowNodeCount;
	TqUint rowDataSource;
	TqMapKey mapKey(0,0);
	TqUint dataArrayPos = 0;
	TqUint subTileWidth = subRegionMap[mapKey]->textureTile->width();
	TqUint subTileHeight = subRegionMap[mapKey]->textureTile->height();
	const TqUint tilesPerCol = lceil((float)sourceTile->tileHeight/subTileHeight);
	const TqUint tilesPerRow = lceil((float)sourceTile->tileWidth/subTileWidth);
	const TqUint nodeSize = subRegionMap[mapKey]->textureTile->colorChannels()+1;
	
	for ( rowIndex = 0; rowIndex < tilesPerCol; ++rowIndex )
	{
		mapKey.first = rowIndex*subTileHeight; mapKey.second = 0;
		subTileHeight = subRegionMap[mapKey]->second->textureTile->height();
		for (subRowIndex = 0; subRowIndex < subTileHeight; ++subRowIndex)
		{
			for (colIndex = 0; colIndex < tilesPerRow; ++colIndex)
			{
				mapKey.first = rowIndex*subTileHeight; mapKey.second = colIndex*subTileWidth;
				subTile = subRegionMap[mapKey]->second->textureTile;
				subRegionWidth = subTile->width();
				if (subTile->isEmpty())
				{
					// Fill default values: a single 100% visibility node per pixel
					for (pixel = 0; pixel < subRegionWidth; ++pixel)
					{
						const TqUint arrayIndex = dataArrayPos*nodeSize 
						data[arrayIndex] = 0;
						for (i = 1 ; i <= subTile->colorChannels; ++i)
						{
							data[arrayIndex+i] = 1; 
						}
					}
					dataArrayPos += subRegionWidth;
				}
				else
				{
					const TqInt* sourceFuncOffsets = subTile->funcOffsets()+(subRowIndex*subRegionWidth);
					rowDataStart = sourceFuncOffsets[0];
					rowNodeCount = sourceFuncOffsets[subRegionWidth+1]-rowDataStartIndex;
					const TqFloat* sourceData = subTile->data()+rowDataStart;
					memcpy(data+(dataArrayPos*nodeSize*sizeof(TqFloat)), sourceData,
							rowNodeCount*nodeSize*sizeof(TqFloat));
					dataArrayPos += rowNodeCount;
				}
			}
		}
	}	
}

bool CqDeepTileAdaptor::allSubRegionsEmpty(const TqMapKey tileKey)
{
	const boost::shared_ptr<SqDeepDataTile> sourceTile = m_deepTileMap[tileKey];
	const TqSubRegionMap& subRegionMap = tile->subRegionMap;
	
	const TqSubRegionMap::const_iterator it;
	for (it = subRegionMap.begin(); it != subRegionMap.end(); ++it)
	{
		if ( !(it->second->textureTile->isEmpty()))
		{
			return false;
		}
	}
	return true;
}

void CqDeepTileAdaptor::createNewTile(const TqMapKey tileKey)
{
	int newTileWidth = m_tileWidth;
	int newTileHeight = m_tileHeight;

	// If this is an end tile, and the image width or height is not divisible by tile width or height,
	// Then perform padding: size this tile to a smaller size than usual.
	if (tileKey[0]+newTileWidth > m_imageWidth) 
	{
		newTileWidth = m_imageWidth-tileKey.first;
	}
	if (tileKey[1]+newTileHeight > m_imageHeight)
	{
		newTileHeight = m_imageHeight-tileKey.second;
	}
	
	boost::shared_ptr<SqDeepDataTile> newTile(new SqDeepDataTile(tileKey.second/m_tileWidth, 
										tileKey.first/m_tileHeight, newTileWidth, newTileHeight));	
	m_deepTileMap[tileKey] = newTile;
}
/*
void CqDeepTileAdaptor::copyMetaData(std::vector< std::vector<int> >& toMetaData, const int* fromMetaData,
									const int rxmin, const int rymin, const int rxmax, const int rymax) const
{
	// Note: I assume that at least a full row of a bucket at a time is sent to the dislay,
	// or even full buckets, but never partial rows. This code will break if partial rows are sent.
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

void CqDeepTileAdaptor::copyData(std::vector< std::vector<float> >& toData, const float* fromData, 
								const int* functionLengths, const int rxmin, const int rymin,
								const int rxmax, const int rymax) const
{
	// Note: I assume that at least a full row of a bucket at a time is sent to the dislay, if not full buckets, 
	// but never partial rows. This code will break if partial rows are sent.
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

void CqDeepTileAdaptor::fillEmptyMetaData(std::vector< std::vector<int> >& toMetaData, 
		const int rxmin, const int rymin, const int rxmax, const int rymax) const
{
	// Note: I assume that at least a full row of a bucket at a time is sent to the dislay,
	// or even full buckets, but never partial rows.
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

void CqDeepTileAdaptor::fillEmptyData(std::vector< std::vector<float> >& toData, 
		const int rxmin, const int rymin, const int rxmax, const int rymax) const
{
	// Note: I assume that at least a full row of a bucket at a time is sent to
	// the dislay, if not full buckets, but never partial rows.
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
*/

bool CqDeepTileAdaptor::isFullTile(const TqMapKey tileKey) const
{
	// An SqDeepDataTile can be considered full if you can 
	// add up all of its sub-region widths and get the tile width,
	// and you can add up all of the sub-region heights and get the tile height.
	const boost::shared_ptr<SqDeepDataTile> tile = m_deepTileMap[tileKey];
	const TqSubRegionMap& subRegionMap = tile->subRegionMap;
	TqUint totalSubRegionWidth = 0;
	TqUint totalSubRegionHeight = 0;
	
	const TqSubRegionMap::const_iterator it;
	for(it = subRegionMap.begin(); it != subRegionMap.end(); ++it)
	{
		totalSubRegionWidth += it->second->textureTile->width();
		totalSubRegionHeight += it->second->textureTile->height();
	}
	
	if ((totalSubRegionWidth == tile->tileWidth) && (totalSubRegionHeight == tile->tileHeight))
	{
		return true;
	}
	return false;
}

/*
bool CqDeepTileAdaptor::isFullTile(const TqMapKey tileKey) const
{
	// A tile is known to be full if all of its sub-tile regions have function lengths for all of their pixels.
	// Another way of saying this: the tile is full if and only if there are a total of tileWidth*tileHeight
	// function lengths.
	int functionCount = 0;
	const boost::shared_ptr<SqDeepDataTile> tile = m_deepTileMap[tileKey];
	const int subRegionsPerRow = Aqsis::lceil((float)tile->height/m_bucketHeight); 
	const int subRegionsPerCol = Aqsis::lceil((float)tile->width/m_bucketWidth);
	const int maxNumberOfSubRegions = subRegionsPerRow*subRegionsPerCol;
	const TqSubRegionMap& subRegionMap = tile->subRegionMap;
	
	if (subRegionMap.size() < maxNumberOfSubRegions)
	{
		// All subregions have not yet been created, so we know the tile is not full.
		return false;
	}
	
	const TqSubRegionMap::const_iterator it;
	for(it = subRegionMap.begin(); it != subRegionMap.end(); ++it)
	{
		const std::vector<std::vector<int> >& subRegionFunctionLengths = (*it)->functionLengths;
		for (j = 0; j < subRegionFunctionLengths.size(); ++j)
		{
			functionCount += subRegionFunctionLengths[j].size(); //< add the count of all functions in a sub-region row
		}
	}
	
	if (functionCount == (tile->Width*tile->Height))
	{
		return true;
	}
	return false;
}
*/

//------------------------------------------------------------------------------
} // namespace Aqsis
