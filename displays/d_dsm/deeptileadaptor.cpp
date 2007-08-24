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

void CqDeepTileAdaptor::addTile(boost::shared_ptr<CqDeepTextureTile> newTile)
{
	// If we want to enforce that data is received a bucket at a time, 
	// then assert xmax-xmin <= bucketWidth (may be '<' in the case of edge buckets)
	if (xmax-xmin > m_bucketWidth)
	{
		throw Aqsis::XqInternal(std::string("Error is CqDeepTexOutputFile::setTileData():"
				" the provided image region is too large. Data will not be copied. Returning"
				" prematurely."), __FILE__, __LINE__);
		return;
	}
	const float* iData = reinterpret_cast<const float*>(data);
	const int* iMetaData = reinterpret_cast<const int*>(metadata);
	const int homeTileTopLeftX = xmin/m_tileWidth;
	const int homeTileTopLeftY = ymin/m_tileHeight;
	const TqMapKey homeTileKey(homeTileTopLeftX, homeTileTopLeftY);
	const int subRegionTopLeftX = xmin-homeTileTopLeftX;
	const int subRegionTopLeftY = ymin-homeTileTopLeftY;
	const TqMapKey subRegionKey(subRegionTopLeftX, subRegionTopLeftY);

	// If the tile that should hold this sebRegion has not been created yet, create it.
	if (m_deepTileMap.count(homeTileKey) == 0)
	{
		createNewTile(homeTileKey);
	}
	boost::shared_ptr<SqDeepDataTile> currentTile = m_deepTileMap[homeTileKey];
	// If it does not already exist, create the sub-tile region to hold the current data
	if (currentTile->subRegionMap.count(subRegionKey) == 0)
	{
		createNewSubTileRegion(currentTile, subRegionKey);
	}
	std::vector< std::vector<int> >& tFunctionLengths = currentTile->subRegionMap[subRegionKey]->functionLengths;
	std::vector< std::vector<float> >& tData = currentTile->subRegionMap[subRegionKey]->tileData;
	// Check for and deal with the special case of empty buckets
	if (iMetaData[0] == -1)
	{
		// This is an empty sub-region: We should fill tFunctionLengths with all 1's
		// and fill tData with visibility nodes (0,1), one per pixel.
		fillEmptyMetaData(tFunctionLengths, xmin%m_bucketWidth, ymin%m_bucketHeight, xmin%m_bucketWidth+xmax-xmin, ymin%m_bucketHeight+ymax-ymin);
		fillEmptyData(tData, xmin%m_bucketWidth, ymin%m_bucketHeight, xmin%m_bucketWidth+xmax-xmin, ymin%m_bucketHeight+ymax-ymin);
	}	
	else
	{
		// Copy data
		copyMetaData(tFunctionLengths, iMetaData, xmin%m_bucketWidth, ymin%m_bucketHeight, xmin%m_bucketWidth+xmax-xmin, ymin%m_bucketHeight+ymax-ymin);
		copyData(tData, iData, iMetaData, xmin%m_bucketWidth, ymin%m_bucketHeight, xmin%m_bucketWidth+xmax-xmin, ymin%m_bucketHeight+ymax-ymin);
	}	
	// Check for full tile
	// If full tile, send to output and reclaim the tile's memory
	if(isFullTile(homeTileKey))
	{
		// Add this full tile to the deep texture output file
		m_deepTexOutput.outputTile( currentTile );
		// Reclaim tile memory no longer needed
		m_deepTileMap.erase(homeTileKey);
	}
}

void CqDeepTileAdaptor::createNewSubTileRegion(boost::shared_ptr<SqDeepDataTile> currentTile, const TqMapKey subRegionKey)
{
	// If this sub-region is at the bottom of the image, and the image is not evenly 
	// divisible by bucket height, then this sub-region is a (bottom) tile padding region,
	// and its height should be less than the standard sub-region height.
	TqUint32 subRegionHeight = m_bucketHeight;
	
	if (subRegionKey[1]+m_bucketHeight > m_imageHeight)
	{
		subRegionHeight = m_imageHeight-subRegionKey[1];	
	}
	currentTile->subRegionMap[subRegionKey] = boost::shared_ptr<SqSubTileRegion>(new SqSubTileRegion);
	currentTile->subRegionMap[subRegionKey]->functionLengths.resize(subRegionHeight);
	currentTile->subRegionMap[subRegionKey]->tileData.resize(subRegionHeight);
}

void CqDeepTileAdaptor::createNewTile(const TqMapKey tileKey)
{
	int newTileWidth = m_tileWidth;
	int newTileHeight = m_tileHeight;

	// If this is an end tile, and the image width or height is not divisible by tile width or height,
	// Then perform padding: size this tile to a smaller size than usual.
	if (tileKey[0]+newTileWidth > m_imageWidth) 
	{
		newTileWidth = m_imageWidth-tileKey[0];
	}
	if (tileKey[1]+newTileHeight > m_imageHeight)
	{
		newTileHeight = m_imageHeight-tileKey[1];
	}
	
	boost::shared_ptr<SqDeepDataTile> newTile(new SqDeepDataTile(tileKey[0]/m_tileWidth, 
										tileKey[1]/m_tileHeight, newTileWidth, newTileHeight));	
	m_deepTileMap[tileKey] = newTile;
}

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

//------------------------------------------------------------------------------
} // namespace Aqsis
