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
#ifndef DEEPTILEADAPTOR_H_INCLUDED
#define DEEPTILEADAPTOR_H_INCLUDED

//Aqsis primary header
#include "aqsis.h"

// Standard libraries
#include <string>
#include <iostream>
#include <vector>
#include <map>
#include <tiff.h> //< temporary include to get the typedefs like uint32

#include "dtexoutput.h"
#include "deeptexturetile.h"

// External libraries
#include <boost/shared_ptr.hpp>

namespace Aqsis 
{

// To make eventual refactor to using Aqsis types nicer:
typedef uint32 TqUint32;

// Key to use when finding elements in std::maps.
typedef std::pair<TqUint32, TqUint32> TqMapKey;

class CqDeepTileAdaptor
{
	public:
		CqDeepTileAdaptor( boost::shared_ptr<IqDeepTextureOutput> outputObject, 
				TqUint32 imageWidth, TqUint32 imageHeight, TqUint32 tileWidth, TqUint32 tileHeight, 
				TqUint32 bucketWidth, TqUint32 bucketHeight, TqUint32 numberOfChannels);
		virtual ~CqDeepTileAdaptor();
		
		
		/** \brief Accept a new pointer to a new tile and store it in tile map. If it is full, 
		 * send it to the output,  otherwise make it part of a bigger tile and wait for the rest
		 * of the data to come in.
		 *
		 * \param newTile - A pointer to a new tile. It may or may not be full. 
		 */
		virtual void addTile(boost::shared_ptr<CqDeepTextureTile> newTile);
		
		/** \brief connect this tile adaptor to an output object to which we send tiles
		 * once they are full.
		 *
		 * \param outputObject - The object that can accept full deep tiles for output via an outputTile() function.
		 */
		virtual void connectOutput( boost::shared_ptr<IqDeepTextureOutput> outputObject );
		
	private:
		// Functions
		
		/** \brief Create a new SqSubTileRegion and add it to the current tile's vector of sub-regions in the appropriate spot. 
		 * Allocate the correct storage space for this particular sub-region.
		 *
		 * \param currentTile - The tile to which we will add the new sub-region
		 * \param subRegionKey - The key into the subRegionMap for the current tile 
		 */		
		void createNewSubTileRegion(boost::shared_ptr<SqDeepDataTile> currentTile, const TqMapKey subRegionKey);
		
		/** \brief Create a new tile and add it to m_deepTileMap with the given key: tileID
		 *
		 * \param tileKey - The new tile's map key into m_deepTileMap. 
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
		void copyMetaData(std::vector< std::vector<int> >& toMetaData, const int* fromMetaData, const int rxmin, const int rymin, const int rxmax, const int rymax) const;
		
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
		void fillEmptyMetaData(std::vector< std::vector<int> >& toMetaData, const int rxmin, const int rymin, const int rxmax, const int rymax) const;
		
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
		
		//-----------------------------------------------------------------------------------------
		// Data
		
		// The map is indexed with a TileKey: a tile ID, which is a pair (topLeftX, topLeftY) 
		// that can identify the region of the larger picture covered by a particular tile.
		std::map<TqMapKey, boost::shared_ptr<SqDeepDataTile> > m_deepTileMap;
		
		boost::shared_ptr<IqDeepTextureOutput> m_deepTexOutput;
		
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

//------------------------------------------------------------------------------
} // namespace Aqsis

#endif // DEEPTILEADAPTOR_H_INCLUDED
