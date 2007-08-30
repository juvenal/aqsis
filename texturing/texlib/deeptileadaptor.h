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
//typedef uint32 TqUint32;

// Key to use when finding elements in std::maps.
typedef std::pair<TqUint32, TqUint32> TqMapKey;

typedef std::map<TqMapKey, boost::shared_ptr<CqDeepTextureTile> > TqSubRegionMap;

//------------------------------------------------------------------------------
/** \brief A structure to organize tile data as a set of sub-regions,
 * until all sub-regions are full and we have a full tile, at which time
 * a single CqDeepTextureTile can be created from the sub-regions.
 *
 */
struct SqDeepDataTile
{
	SqDeepDataTile( const TqUint32 width, const TqUint32 height );
	
	/** \brief Add small tile pointer to subRegionMap.
	 *
	 * \param tile - A pointer to a small tile.
	 * \param subRegionKey - key specifies where in the subRegionMap to place the tile pointer. 
	 */
	void addSubRegion( boost::shared_ptr<CqDeepTextureTile> tile, TqMapKey subRegionKey );

	// A map of sub-regions indexable by TqMapKeys
	TqSubRegionMap subRegionMap;
	TqUint32 width; //< width of this tile in pixels
	TqUint32 height; //< height of this tile in pixels
};

class CqDeepTileAdaptor
{
	public:
		CqDeepTileAdaptor( boost::shared_ptr<IqDeepTextureOutput> outputObject, 
				TqUint32 imageWidth, TqUint32 imageHeight, TqUint32 tileWidth, TqUint32 tileHeight, 
				TqUint32 bucketWidth, TqUint32 bucketHeight, TqUint32 numberOfChannels);
		virtual ~CqDeepTileAdaptor(){};
		
		/** \brief Accept a new pointer to a new tile and store it in tile map. If it is full, 
		 * send it to the output,  otherwise make it part of a bigger tile and wait for the rest
		 * of the data to come in.
		 *
		 * \param newTile - A pointer to a new tile. It may or may not be full. 
		 */
		virtual void addTile( const boost::shared_ptr<CqDeepTextureTile> tile );
		
		/** \brief connect this tile adaptor to an output object to which we send tiles
		 * once they are full.
		 *
		 * \param outputObject - The object that can accept full deep tiles for output via an outputTile() function.
		 */
		virtual void connectOutput( boost::shared_ptr<IqDeepTextureOutput> outputObject );
		
	private:
		// Functions
		
		/** \brief Create a full sized CqDeepTextureTile from the subregions
		 * in the SqDeepDataTile designated by tileKey. Send this tile to output
		 * then release its memory.
		 *
		 * \param tileKey - the key into m_deepTileMap to retrieve the SqDeepDataTile we want to rebuild
		 */
		void rebuildAndOutputTile(const TqMapKey tileKey);

		/** \brief Get a boolean flag indicating whether of not the specified SqDeepDataTile
		 * is empty. 
		 *
		 * \param tileKey - the key into m_deepTileMap to retrieve the SqDeepDataTile;
		 *  we want to check is it is empty.
		 * 
		 * \return True is all reb-regions are empty in the SqDeepDataTile specified by tileKey.
		 */
		bool CqDeepTileAdaptor::allSubRegionsEmpty(const TqMapKey tileKey);
	
		/** \brief Create a new tile and add it to m_deepTileMap with the given key: tileID
		 *
		 * \param tileKey - The new tile's map key into m_deepTileMap. 
		 */
		void createNewTile(const TqMapKey tileKey);
		
		/** \brief Recalculate the visibility function offsets for this tile by
		 * iterating over the sub-regions, and filling in and empty regions with default data.
		 * 
		 * \param funcOffsets - A pointer to pre-alloacted storage where we should store the function offsets
		 * \param tileKey - The tile's key into m_deepTileMap.
		 * \return the value of the largest function offset, which represents the
		 * number of visibility nodes in the tile.   
		 */		
		TqUint rebuildFunctionOffsets(boost::shared_array<TqInt> funcOffsets, const TqMapKey tileKey);

		/** \brief Rebuild the visibility functions for this tile by
		 * iterating over the sub-regions, and filling in and empty regions with default data.
		 * 
		 * \param data - A pointer to pre-alloacted storage where we should store the data
		 * \param tileKey - The tile's key into m_deepTileMap.
		 */
		void rebuildVisibilityFunctions(boost::shared_array<TqFloat> data, const TqMapKey tileKey);
		
		/** \brief Determine if all data has been received for a specific tile,
		 *  meaning it is full and ready for output to file.
		 *
		 * \param tileKey - The key for the tile; we want to know if the tile is full
		 * \return True if the sum of the tile's reb-region widths is equal to the tile's width
		 * 		and the sum of the sub-region heights is equal to the tile height, false otherwise.
		 */				
		bool isFullTile(const TqMapKey tileKey);
		
		//-----------------------------------------------------------------------------------------
		// Data
		
		// The map is indexed with a TileKey: a tile ID, which is a pair (topLeftX, topLeftY) 
		// that can identify the region of the larger picture covered by a particular tile.
		std::map<TqMapKey, boost::shared_ptr<SqDeepDataTile> > m_deepTileMap;
		
		// This class implements an outputTile() function that, in our case, writes the tile to disk
		// in a deep texture file. 
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
