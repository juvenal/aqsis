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
 * \author Zachary Carter (zcarter@aqsis.org)
 *
 * \brief Facilities for constructing/caching tiles from smaller sets of deep data,
 * such as buckets a row or more from a single bucket.
 */
#ifndef DEEPTILECACHE_H_INCLUDED
#define DEEPTILECACHE_H_INCLUDED

//Aqsis primary header
#include "aqsis.h"

// Standard libraries
#include <string>
#include <iostream>
#include <vector>
#include <map>
#include <tiff.h> //< temporary include to get the typedefs like uint32

#include "dtexoutput.h"

// External libraries
#include <boost/shared_ptr.hpp>

class CqDeepTileAdaptor
{
	public:
		CqDeepTileAdaptor( TqUint32 imageWidth, TqUint32 imageHeight, TqUint32 tileWidth, TqUint32 tileHeight, 
				TqUint32 bucketWidth, TqUint32 bucketHeight, TqUint32 numberOfChannels);
		virtual ~CqDeepTileAdaptor();
		
		/** \brief Copy the given data into the tile map
		 *
		 * \param metadata - Data describing important attributes about the deep data.
		 *					In this case, values specifying the length, in units of number of nodes,
		 * 					of each visibility function. In the case of an empty bucket, the first value in
		 * 					metadata is -1 and data is empty/invalid.
		 * \param data - The deep data corresponding to the image region defined by (xmin,ymin) and (xmax,ymax)
		 */
		virtual void setData( const int xmin, const int ymin, const int xmax, const int ymax, 
				const unsigned char *data, const unsigned char* metadata );
		
		/** \brief 
		 *
		 * \param outputObject - The object that can accept full deep tiles for output via an outputTile() function.
		 */
		virtual void connectOutput( IqDeepTextureOutput& outputObject );
		
	private:
		// Functions
		
		/** \brief Create a new SqSubTileRegion and add it to the current tile's vector of sub-regions in the appropriate spot. 
		 * Allocate the correct storage space for this particular sub-region.
		 *
		 * \param currentTile - The tile to which we will add the new sub-region
		 * \param subRegionKey - The key into the subRegionMap for the current tile 
		 */		
		void createNewSubTileRegion(boost::shared_ptr<SqDeepDataTile> currentTile, const TqMapKey subRegionKey);
		
		/** \brief Create a new tile and add it to m_DeepDataTileMap with the given key: tileID
		 *
		 * \param tileKey - The new tile's map key into m_DeepDataTileMap. 
		 */
		void createNewTile(const TqMapKey tileKey);
		
		/** \brief Copy given metadata into the given std::vector.
		 * 
		 * \param toMetaData - The destination to which we want to copy metadata
		 * \param fromMetaData - The source of the data we want to copy.
		 * \param rxmin - The first x-coordinate, relative to the origin of the sub-region to which we are copying.
		 * \param rymin - The first y-coordinate, relative to the origin of the sub-region to which we are copying.
		 * \param rxmax - The x-coordinate, 
		 * 		relative to the origin of the sub-region to which we are copying, where copying should halt.
		 * \param rymax - The y-coordinate, 
		 * 		relative to the origin of the sub-region to which we are copying, where copying should halt.
		 */	
		static void copyMetaData(std::vector< std::vector<int> >& toMetaData, const int* fromMetaData, const int rxmin, const int rymin, const int rxmax, const int rymax) const;
		
		/** \brief Copy given data into the given std::vector.
		 * 
		 * \param toData - The destination to which we want to copy data
		 * \param fromData - The source of the data we want to copy.
		 * \param rxmin - The first x-coordinate, relative to the origin of the sub-region to which we are copying.
		 * \param rymin - The first y-coordinate, relative to the origin of the sub-region to which we are copying.
		 * \param rxmax - The x-coordinate, 
		 * 		relative to the origin of the sub-region to which we are copying, where copying should halt.
		 * \param rymax - The y-coordinate, 
		 * 		relative to the origin of the sub-region to which we are copying, where copying should halt.
		 */	
		void copyData(std::vector< std::vector<float> >& toData, const float* fromData, const int* functionLengths, const int rxmin, const int rymin, const int rxmax, const int rymax) const;
		
		/** \brief Fill the sub-region specified by (rxmin,rymin,rxmax,rymax), with default metadata. 
		 * The sub-region may be part of a neglectable tile, and therefore may never be written to disk,
		 * but we have to assume it might be part of a tile that will be written, and so we fill it with minimal data
		 * to indicate that the pixels in this region have 100% visibility.
		 * 
		 * \param toMetaData - The destination we want to fill with default metadata
		 * \param rxmin - The first x-coordinate, relative to the origin of the sub-region to which we are copying.
		 * \param rymin - The first y-coordinate, relative to the origin of the sub-region to which we are copying.
		 * \param rxmax - The x-coordinate, 
		 * 		relative to the origin of the sub-region to which we are copying, where copying should halt.
		 * \param rymax - The y-coordinate, 
		 * 		relative to the origin of the sub-region to which we are copying, where copying should halt.
		 */	
		static void fillEmptyMetaData(std::vector< std::vector<int> >& toMetaData, const int rxmin, const int rymin, const int rxmax, const int rymax) const;
		
		/** \brief Fill the sub-region specified by (rxmin,rymin,rxmax,rymax), with default metadata. 
		 * The sub-region may be part of a neglectable tile, and therefore may never be written to disk,
		 * but we have to assume it might be part of a tile that will be written, and so we fill it with minimal data
		 * to indicate that the pixels in this region have 100% visibility.
		 * 
		 * \param rxmin - The first x-coordinate, relative to the origin of the sub-region to which we are copying.
		 * \param rymin - The first y-coordinate, relative to the origin of the sub-region to which we are copying.
		 * \param rxmax - The x-coordinate, 
		 * 		relative to the origin of the sub-region to which we are copying, where copying should halt.
		 * \param rymax - The y-coordinate, 
		 * 		relative to the origin of the sub-region to which we are copying, where copying should halt.
		 */	
		void fillEmptyData(std::vector< std::vector<float> >& toData, const int rxmin, const int rymin, const int rxmax, const int rymax);
		
		/** \brief Determine if all data has been received for a specific tile,
		 *  meaning it is full and ready for output to file.
		 *
		 * \param tileKey - The key for the tile; we want to know if the tile is full
		 */				
		bool isFullTile(const TqMapKey tileKey) const;
		
		/** \brief Determine if this tile is neglectable, that is, it covers no micropolygons 
		 * or participating media in the DSM and can therefore be left out of the DTEX file,
		 * and represented instead with a fileOffset of 0 in the tile table, signifying that any pixels
		 * inside this tile have 100% visibility at all depths, and are therefore never in shadow.
		 *
		 * \param tileKey - Use to access the tile in m_deepDataTileMap;
		 * 		 we want to know if the tile has all of its sub-regions as empty sub-regions.
		 */	
		bool isNeglectableTile(const TqMapKey tileKey) const;
		
		//-----------------------------------------------------------------------------------------
		// Data
		
		// The map is indexed with a TileKey: a tile ID, which is a pair (topLeftX, topLeftY) 
		// that can identify the region of the larger picture covered by a particular tile.
		std::map<TqMapKey, boost::shared_ptr<SqDeepDataTile> > m_deepDataTileMap;
		
		IqDeepTextureOutput& m_deepTexOutput;
		
		TqUint32 m_imageWidth;
		TqUint32 m_imageHeight;
		TqUint32 m_tileWidth;
		TqUint32 m_tileHeight;
		TqUint32 m_bucketWidth;
		TqUint32 m_bucketHeight;
		TqUint32 m_tilesPerImageRow;
		TqUint32 m_tilesPerImageCol;
		TqUint32 m_bucketsPerTileRow;
		TqUint32 m_bucketsPerTileCol;
		TqUint32 m_numberOfChannels;
};

#endif // DEEPTILECACHE_H_INCLUDED
