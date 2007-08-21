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
 * \brief Implementation of a deep texture file reader class to load deep texture tiles from an Aqsis dtex file into memory.
 */
#ifndef DTEXINPUT_H_INCLUDED
#define DTEXINPUT_H_INCLUDED

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
//#include <boost/intrusive_ptr.hpp>
#include <boost/shared_array.hpp>
#include <boost/shared_ptr.hpp>

//#include "smartptr.h"

namespace Aqsis
{

//------------------------------------------------------------------------------
/** \brief A class to store a single deep data tile loaded from a dtex file.
 *
 */
class CqDeepTextureTile
{
	public:
		/** \brief Construct a deep texture tile
		 *
		 * \param data - raw tile data.
		 * \param metaData - Information describing the raw data (for example, function lengths)
		 * \param width - tile width
		 * \param height - tile height
		 * \param topLeftX - top left pixel X-position in the larger array
		 * \param topLeftY - top left pixel Y-position in the larger array
		 * \param colorChannels - number of color channels per deep data node.
		 */
		CqDeepTextureTile(boost::shared_array<TqFloat> data, boost::shared_array<TqUint> metaData,
				const TqUint width, const TqUint height,
				const TqUint topLeftX, const TqUint topLeftY,
				const TqUint colorChannels, const TqUint bytesPerChannel) :
					m_data(data),
					m_metaData(metaData),
					m_width(width),
					m_height(height),
					m_topLeftX(topLeftX),
					m_topLeftY(topLeftY),
					m_colorChannels(colorChannels),
					m_bytesPerChannel(bytesPerChannel)
		{}

		/** \brief Destructor
		 */
		virtual ~CqDeepTextureTile();

		/** \brief Set the class data to the given data pointer
		 *
		 * \param data - A pointer to data which has already been allocated and initialized.
		 */
		inline void setData(boost::shared_array<TqFloat> data);

		/** \brief Set the class meta data to the given meta data pointer
		 *
		 * \param data - A pointer to meta data which has already been allocated and initialized.
		 */
		inline void setMetaData(boost::shared_array<TqUint> metaData);		
		
		/** \brief Get a const pointer to the visibility function at the requested pixel in the tile. 
		 *
		 * Positions are in tile coordinates, not image coordinates, counting
		 * from zero in the top-left.
		 * 
		 * \return a const pointer to the beginning of the visibility function.
		 */
		inline const TqFloat* visibilityFunctionAtPixel( const TqUint tileSpaceX, const TqUint tileSpaceY ) const;
		
		/** \brief Get the length of the visibility function at the requested pixel in the tile
		 * 
		 * Positions are in tile coordinates, not image coordinates, counting
		 * from zero in the top-left.
		 *
		 * \return the length of the requested visibility function, measured in number of nodes.
		 */
		inline TqUint functionLengthOfPixel( const TqUint tileSpaceX, const TqUint tileSpaceY ) const;
		
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
		/// A standard use of meta data is to represent the offset (in units of number of nodes)
		/// of each visibility function from the start of the data block. A particle function N may be 
		/// indexed at position metaData[N], and its length is metaData[N+1]-metaData[N].
		/// The last element in metaData should therefor be the position of the end of the data, which
		/// also represents the size, or number of nodes in the data.
		/// The number of elements in this array must be equal to (tileWidth*tileHeight),
		/// so there is a length field for every pixel, empty or not.
		boost::shared_array<TqUint> m_metaData; ///< Pointer to the meta data, which describes the data.
		TqUint m_width;				///< Width of the tile
		TqUint m_height;			///< Height of the tile
		TqUint m_topLeftX;			///< Column index of the top left of the tile in the full array
		TqUint m_topLeftY;			///< Row index of the top left of the tile in the full array
		TqUint m_colorChannels;		///< Number of color channels per deep data node.
		TqUint m_bytesPerChannel;	///< Number of bytes used in each color channel
};

//------------------------------------------------------------------------------
/** \brief A structure storing the constant/global DTEX file information
 *
 * This file header represents the first bytes of data in the file.
 * It is immediately followed by the tile table.
 */
/// \todo This structure should be stored in a shared location since it is used in this file and in dtex.cpp/dtex.h
struct SqDtexFileHeader
{
	/** \brief Construct an SqDtexFileHeader structure
	* \param fs - file size of the dtex file in bytes
	* \param iw - image width; the width in pixels of the dtex image
	* \param ih - image height; the hieght in pixels of the dtex image
	* \param nc - number of channels; for example 'rgb' has 3 channels, but 'r' has only 1 channel 
	* \param bpc - bytes per channel; the size in bytes used for each color channel. For example, float takes 4 bytes, but char takes only 1 byte
	* \param hs - header size; the size in bytes of SqDtexFileHeader. We can probably get rid of this field.
	* \param ds - data size; the size of only the data part of the dtex file
	 */ 
	SqDtexFileHeader( uint32 fs = 0, uint32 iw = 0, uint32 ih = 0, uint32 nc = 0, 
			uint32 bpc = 0, uint32 hs = 0, uint32 ds = 0, uint32 tw = 0, uint32 th = 0, uint32 nt = 0)
		: // magicNumber( mn ),
		fileSize( fs ),
		imageWidth( iw ),
		imageHeight( ih ),
		numberOfChannels( nc ),
		bytesPerChannel( bpc ),
		//headerSize( hs ),
		dataSize( ds ),
		tileWidth( tw ),
		tileHeight( th ),
		numberOfTiles( nt )
	{}
	/// The magic number field contains the following bytes: �\0x89AqD\0x0b\0x0a\0x16\0x0a"
	// The first byte has the high-bit set to detect transmission over a 7-bit communications channel.
	// This is highly unlikely, but it can't hurt to check. 
	// The next three bytes are the ASCII string "AqD" (short for "Aqsis DTEX").
	// The next two bytes are carriage return and line feed, which results in a "return" sequence on all major platforms (Windows, Unix and Macintosh).
	// This is followed by a DOS end of file (control-Z) and another line feed.
	// This sequence ensures that if the file is "typed" on a DOS shell or Windows command shell, the user will see "AqD" 
	// on a single line, preceded by a strange character.
	//char magicNumber[8]; // How can I store the magic number in such a way that its data, not the value of the
		// address that points to it, is written to file when we call write(m_dtexFileHeader)?
	/// Size of this file in bytes
	uint32 fileSize;
	// Number of horizontal pixels in the image
	uint32 imageWidth;
	// Number of vertical pixels
	uint32 imageHeight;
	/// Number if channels in a visibility node (1 for grayscale, 3 for full color)
	uint32 numberOfChannels;
	/// Depending on the precision, number of bytes per color channel
	uint32 bytesPerChannel;
	/// Number of bytes in this header (might not need this)
	//uint32 headerSize;
	// Size of the deep data by itself, in bytes
	uint32 dataSize;
	// Width, in pixels, of a tile (unpadded, so edge tiles may be larger, but never smaller)
	uint32 tileWidth;
	// Height, in pixels, of a tile (unpadded)
	uint32 tileHeight;
	// Number of tiles in the image
	uint32 numberOfTiles;
	// World to Screen transformation matrix
	float matWorldToScreen[4][4];
	// World to Camera transformation matrix
	float matWorldToCamera[4][4];
};

//------------------------------------------------------------------------------
/** \brief A deep texture class to load deep shadow map tiles from a DTEX file into memory, but this class does not actually own said memory.
 * 
 * Usage works as follows: 
 * This class acts as an interface allowing retrieval of on-disk tiles from tiled images, specifically dtex files.
 * A single public member function called LoadTileForPixel() is responsible for locating the tile with the requested pixel from disk and
 * copying it to the memory provided by the caller. 
 */
class CqDeepTexInputFile
{
	public:
		CqDeepTexInputFile(std::string filename); //< Should we have just filename, or filenameAndPath?
		virtual ~CqDeepTexInputFile();
	  
		/** \brief Locate the tile containing the given pixel and copy it into memory. 
		 *
		 * \param x - Image x-coordinate of the pixel desired. We load the entire enclosing tile because of the spatial locality heuristic; it is likely to be needed again soon.
		 * \param y - Image y-coordinate of the pixel desired. We load the entire enclosing tile because of the spatial locality heuristic; it is likely to be needed again soon.
		 * \return a shared pointer to an object to the requested tile in memory.
		 */
		boost::shared_ptr<CqDeepTextureTile> LoadTileForPixel( const TqUint x, const TqUint y );
		
		/** \brief Get the transformation matrices freom the deep shadow map
		 *
		 * \param matWorldToScreen - A preallocated 2-D array to hold the transformation matrix by the same name from the dsm.
		 * \paran matWorldToCamera - A preallocated 2-D array to hold the transformation matrix by the same name from the dsm.
		 */
		inline void transformationMatrices( TqFloat matWorldToScreen[4][4], TqFloat matWorldToCamera[4][4] ) const;
		
		/** \brief Get the width of the deep texture map
		 *
		 * \return the width in pixels
		 */
		inline TqUint imageWidth() const;

		/** \brief Get the height of the deep texture map
		 *
		 * \return the height in pixels
		 */
		inline TqUint imageHeight() const;
		
		/** \brief Get the width of a standard (unpadded) tile in the deep texture map
		 *
		 * \return the width in pixels
		 */
		inline TqUint standardTileWidth() const;
		
		/** \brief Get the height of a standard (unpadded) tile in the deep texture map
		 *
		 * \return the width in pixels
		 */
		inline TqUint standardTileHeight() const;
		
		/** \brief Get the number of color channels in a deep data node in the deep texture map.
		 *
		 * \return the number of color channels.
		 */
		inline TqUint numberOfColorChannels() const;
	
		/** \brief Get the number of bytes used in each color channel representation. For example, if the color channels are represented as floats, this number if 4.
		 *
		 * \return the number of bytes
		 */
		inline TqUint bytesPerColorChannel() const;
		
	private:
		
		// Functions
		void LoadTileTable();
		boost::shared_ptr<CqDeepTextureTile> LoadTileAtOffset(const TqUint fileOffset, const TqUint tileRow, const TqUint tileCol);

		// File handle for the file we read from
		std::ifstream m_dtexFile;
		
		// File header stuff
		SqDtexFileHeader m_fileHeader;
		std::vector< std::vector<TqUint> > m_tileOffsets;
		
		// Other data
		TqUint m_tilesPerRow;
		TqUint m_tilesPerCol;
};

//------------------------------------------------------------------------------
// Implementation of inline functions for CqDeepTextureTile
//------------------------------------------------------------------------------

inline void CqDeepTextureTile::setData(boost::shared_array<TqFloat> data)
{
	m_data = data;
}

inline void CqDeepTextureTile::setMetaData(boost::shared_array<TqUint> metaData)
{
	m_metaData = metaData;
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
// Implementation of inline functions for CqDeepTextureTile
//------------------------------------------------------------------------------
inline void CqDeepTexInputFile::transformationMatrices( TqFloat matWorldToScreen[4][4], TqFloat matWorldToCamera[4][4] ) const
{
	assert(matWorldToScreen != NULL);
	assert(matWorldToCamera != NULL);
	TqUint x, y;
	for (x = 0; x < 4; ++x)
	{
		for (y = 0; y < 4; ++y)
		{
			assert(matWorldToCamera[x] != NULL);
			assert(matWorldToScreen[x] != NULL);
			matWorldToScreen[x][y] = m_fileHeader.matWorldToScreen[x][y];
			matWorldToCamera[x][y] = m_fileHeader.matWorldToCamera[x][y];
		}
	}
}

inline const TqFloat* CqDeepTextureTile::visibilityFunctionAtPixel( const TqUint tileSpaceX, const TqUint tileSpaceY ) const
{
	const TqUint nodeSize = 1+m_colorChannels;
	
	return (m_data.get()+(tileSpaceX*tileSpaceY*nodeSize));
}

inline TqUint CqDeepTextureTile::functionLengthOfPixel( const TqUint tileSpaceX, const TqUint tileSpaceY ) const
{
	return m_metaData[tileSpaceX*tileSpaceY];
}

inline TqUint CqDeepTexInputFile::imageWidth() const
{
	return m_fileHeader.imageWidth;
}

inline TqUint CqDeepTexInputFile::imageHeight() const
{
	return m_fileHeader.imageHeight;
}

inline TqUint CqDeepTexInputFile::standardTileWidth() const
{
	return m_fileHeader.tileWidth;
}

inline TqUint CqDeepTexInputFile::standardTileHeight() const
{
	return m_fileHeader.tileHeight;	
}

inline TqUint CqDeepTexInputFile::numberOfColorChannels() const
{
	return m_fileHeader.numberOfChannels;
}

inline TqUint CqDeepTexInputFile::bytesPerColorChannel() const
{
	return m_fileHeader.bytesPerChannel;	
}

//------------------------------------------------------------------------------
} // namespace Aqsis

#endif // DTEXINPUT_H_INCLUDED
