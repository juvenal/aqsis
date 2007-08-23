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
 * \brief Implementation of deep texture tile class.
 */
#ifndef DEEPTEXTILE_H_INCLUDED
#define DEEPTEXTILE_H_INCLUDED

//Aqsis primary header
#include "aqsis.h"

// Standard libraries
#include <string>
#include <iostream>
#include <fstream>
#include <vector>
#include <map>
#include <tiff.h> //< Including (temporarily) in order to get the typedefs like uint32

// External libraries
#include <boost/shared_array.hpp>
#include <boost/shared_ptr.hpp>


namespace Aqsis
{

/** \brief A lightweight wrapper class for a visibility function.
 *
 */
class CqVisibilityFunction
{
	public:
		CqVisibilityFunction( const TqInt functionLength, const TqFloat* functionPtr);
		
	const TqInt functionLength; //< number of nodes in function
	const TqFloat* functionPtr; //< raw pointer to beginning of function
};

typedef boost::shared_ptr<CqVisibilityFunction> TqVisFuncPtr;

/** \brief A class to store a single deep data tile and functions.
 *
 */
class CqDeepTextureTile
{
	public:
		/** \brief Construct a deep texture tile
		 *
		 * \param data - raw tile data.
		 * \param funcOffsets - Function offsets indicating the start position (node #) 
		 * 						of each visibility function in the tile
		 * \param width - tile width
		 * \param height - tile height
		 * \param topLeftX - top left pixel X-position in the larger array
		 * \param topLeftY - top left pixel Y-position in the larger array
		 * \param colorChannels - number of color channels per deep data node.
		 */
		CqDeepTextureTile(const boost::shared_array<TqFloat> data, const boost::shared_array<TqUint> funcOffsets,
				const TqUint width, const TqUint height, const TqUint topLeftX, const TqUint topLeftY,
				const TqUint colorChannels);

		/** \brief Set the class data to the given data pointer
		 *
		 * \param data - A pointer to data which has already been allocated and initialized.
		 */
		inline void setData(const boost::shared_array<TqFloat> data);

		/** \brief Set the class meta data to the given meta data pointer
		 *
		 * \param data - A pointer to meta data which has already been allocated and initialized.
		 */
		inline void setFuncOffsets(const boost::shared_array<TqUint> funcOffsets);		
		
		/** \brief Get a const pointer to the visibility function at the requested pixel in the tile. 
		 *
		 * Positions are in tile coordinates, not image coordinates, counting
		 * from zero in the top-left.
		 * 
		 * \return a const pointer to a wrapper class for the requested visibility function.
		 */
		inline const TqVisFuncPtr visibilityFunctionAtPixel( const TqUint tileSpaceX, const TqUint tileSpaceY ) const;
		
		/** \brief Get tile width
		 *
		 * \return tile width
		 */
		inline TqUint width() const;

		/** \brief Get tile height
		 *
		 * \return tile height
		 */
		inline TqUint height() const;

		/** \brief Get top left X-position of tile in the larger array
		 *
		 * \return top left pixel X-position in the larger array
		 */
		inline TqUint topLeftX() const;

		/** \brief Get top left Y-position of tile in the larger array
		 *
		 * \return top left pixel Y-position in the larger array
		 */
		inline TqUint topLeftY() const;

		/** \brief Get the number of color channels per deep data node
		 *
		 * \return number of color channels per visibility node
		 */
		inline TqUint colorChannels() const;
		
	private:

		// Data members
		boost::shared_array<TqFloat> m_data;	///< Pointer to the underlying data.
		/// m_funcOffsets below is meta data to represent the offset (in units of number of nodes)
		/// of each visibility function from the start of the data block. A particular function N may be 
		/// indexed at position metaData[N], and its length is metaData[N+1]-metaData[N].
		/// The last element in metaData should therefore be the position of the end of the data, which
		/// also represents the size, or number of nodes in the data.
		/// The number of elements in this array must be equal to (tileWidth*tileHeight),
		/// so there is a length field for every pixel, empty or not.
		boost::shared_array<TqUint> m_funcOffsets; ///< visibility function offsets (w.r.t tile start)
		TqUint m_width;				///< Width of the tile
		TqUint m_height;			///< Height of the tile
		TqUint m_topLeftX;			///< Column index of the top left of the tile in the full array
		TqUint m_topLeftY;			///< Row index of the top left of the tile in the full array
		TqUint m_colorChannels;		///< Number of color channels per deep data node.
};

//------------------------------------------------------------------------------
// Implementation of inline functions for CqDeepTextureTile

inline void CqDeepTextureTile::setData(const boost::shared_array<TqFloat> data)
{
	m_data = data;
}

inline void CqDeepTextureTile::setFuncOffsets( const boost::shared_array<TqUint> funcOffsets)
{
	m_funcOffsets = funcOffsets;
}

inline const TqVisFuncPtr CqDeepTextureTile::visibilityFunctionAtPixel( const TqUint tileSpaceX, const TqUint tileSpaceY ) const
{
	// Construct and return a pointer to new instance of CqVisibilityFunction(functionLength, dataPtr) 
	return TqVisFuncPtr( new CqVisibilityFunction(
			m_funcOffsets[tileSpaceX*tileSpaceY+1]-m_funcOffsets[tileSpaceX*tileSpaceY+1],
			m_data.get()+(tileSpaceX*tileSpaceY*(1+m_colorChannels)))); 
}

inline TqUint CqDeepTextureTile::width() const
{
	return m_width;
}

inline TqUint CqDeepTextureTile::height() const
{
	return m_height;
}

inline TqUint CqDeepTextureTile::topLeftX() const
{
	return m_topLeftX;
}

inline TqUint CqDeepTextureTile::topLeftY() const
{
	return m_topLeftY;
}

inline TqUint CqDeepTextureTile::colorChannels() const
{
	return m_colorChannels;
}

//------------------------------------------------------------------------------
} // namespace Aqsis

#endif // DEEPTEXTILE_H_INCLUDED
