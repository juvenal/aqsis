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
 * \brief A C++ wrapper around tiff files for the functions of interest in aqsis.
 *
 * \author Chris Foster
 */

#ifndef TIFFFILE_H_INCLUDED
#define TIFFFILE_H_INCLUDED

#include <string>

#include <boost/utility.hpp>
#include <boost/shared_array.hpp>
#include <boost/intrusive_ptr.hpp>
#include <boost/format.hpp>
#include <tiffio.h>
#include <tiffio.hxx>

#include "aqsis.h"
#include "exception.h"
#include "logging.h"
#include "tilearray.h"

namespace Aqsis
{
//------------------------------------------------------------------------------

/** \brief Find a file in the current Ri Path.
 *
 * \param fileName - file name to search for
 * \param riPath - name of the Ri path to look in.
 *
 * \return the fully qualified path to the file.
 */
std::string findFileInRiPath(const std::string& fileName, const std::string& riPath);

//------------------------------------------------------------------------------
/** \brief Class for reporting errors encountered on reading TIFF files.
 */
class XqTiffError : public XqEnvironment
{
	public:
		XqTiffError (const std::string& reason, const std::string& detail,
		const std::string& file, const unsigned int line);
		
		XqTiffError (const std::string& reason,	const std::string& file,
			const unsigned int line);
		
		virtual const char* description () const;
		
		/// \todo should this really be virtual?
		virtual ~XqTiffError () throw ();
};


//------------------------------------------------------------------------------
/** \brief Class wrapper around the libtiff library functions for reading
 * multi-directory tiff files.
 *
 * CqTiffInputFile provides a uniform interface for dealing with both tiled and
 * strip-oriented TIFF files - here we just call both types "tiles".
 *
 * CqTiffInputFile attempts to provide a more C++-like interface, and as such
 * doesn't always do things in "the libtiff way".
 */
class CqTiffInputFile
{
	public:
		/** \brief Construct a tiff input file.  The resulting file object is
		 * guaranteed to be an valid and accessable TIFF file.
		 *
		 * \throw XqEnvironment
		 * \throw XqTiffError
		 *
		 * \param fileName - file name to open
		 * \param directory - tiff directory to attach to
		 */
		CqTiffInputFile(const std::string& fileName, const TqUint directory = 0);
		/** \brief Construct a tiff input file from an input stream.
		 *
		 * \throw XqTiffError if libtiff has a problem with the stream
		 *
		 * \param inputStream - an input stream
		 * \param directory - tiff directory to attach to
		 */
		CqTiffInputFile(std::istream& inputStream, const TqUint directory = 0);
		/** \brief Virtual destructor
		 */
		virtual ~CqTiffInputFile();

		/** \brief Get the image width.
		 * \return image width
		 */
		inline TqUint width() const;
		/** \brief Get the image height.
		 * \return image height
		 */
		inline TqUint height() const;
		/** \brief Get the width of tiles held in this tiff
		 * \return tile width
		 */
		inline TqUint tileWidth() const;
		/** \brief Get the height of tiles held in this tiff
		 * \return tile height
		 */
		inline TqUint tileHeight() const;

		/** \brief Read a tile or strip from the tiff file.
		 *
		 * \throw XqTiffError if the requested tile is outside the bounds of the image
		 *
		 * \param x - tile column index (counting from top left, starting with 0)
		 * \param y - tile row index (counting from top left, starting with 0)
		 * \return a tile containing the desired data.
		 */
		template<typename T>
		boost::intrusive_ptr<CqTextureTile<T> > readTile(const TqUint x, const TqUint y);

		/** \brief Check that this tiff file has directories with a given size.
		 *
		 * This function checks only that the tiff has the correct number of
		 * directories, and that the directories have the correct sizes.
		 *
		 * \return true if the file has directories of the given size.
		 */
		bool checkDirSizes(const std::vector<TqUint>& widths,
				const std::vector<TqUint>& heights);

		/** \brief Create a copy, of this Tiff file, but with a different directory.
		 *
		 * \return a copy of this tiff file, set to the given directory.
		 */
		CqTiffInputFile& cloneWithNewDirectory(const TqUint newDir);

		// Use compiler-generated copy constructor and assignment operator

	private:
		//----------------------------------------
		/** \brief Hold data describing the current directory
		 */
		struct SqDirectoryData
		{
			tdir_t index;			///< directory index
			bool isTiled;			///< true if tile-based, false if strip-based
			// The following are required tiff fields
			uint32 imageWidth;		///< full image width
			uint32 imageHeight;		///< full image height
			uint32 bitsPerSample;	///< number of bits per channel sample
			uint32 samplesPerPixel;	///< number of channels per pixel
			uint32 tileWidth;		///< tile or strip width
			uint32 tileHeight;		///< tile or strip height
			uint16 photometricInterp;	///< photometric interpretation for samples
			// the next two are computed
			uint32 numTilesX;		///< number of columns of tiles
			uint32 numTilesY;		///< number of rows of tiles
			// The rest are values which may or may not be present.  We try to
			// choose sensible defaults for these.
			uint16 sampleFormat;	///< the format of the samples - uint/int/float etc
			uint16 planarConfig;	///< are different channels stored packed together?
			uint16 orientation;		///< position of the (0,0) index to the tiles

			// Non format-related data.
			boost::shared_ptr<TIFFRGBAImage> m_rgbaImage; ///< libtiff RGBA image for decoding unusual formats

			/// Default constructor
			SqDirectoryData();
			/// Constructor taking a directory handle to grab the directory data from
			SqDirectoryData(const CqTiffDirHandle& dirHandle);
			/// Use compiler generated assignment operator and copy constructor.
		};

		//----------------------------------------
		// Private member functions
		/** \brief Set the directory for this tiff file.
		 *
		 * \throw XqTiffError if the required directory could not be read.
		 *
		 * \param directory - directory to connect to.
		 */
		void setDirectory(const TqUint directory);
		/** \brief Set data to all ones, and a data read error to the log.
		 *
		 * \param data - array which wasn't written because of a tiff read error.
		 * \param dataSize - length of array.
		 */
		template<typename T>
		void handleTiffReadError(boost::shared_array<T> data, const TqUint size) const;
		//----------------------------------------
		// Member data
		std::string m_fileName;			///< file name
		boost::shared_ptr<CqTiffFileHandle> m_fileHandlePtr; ///< Thread and directory-safe file handle.
		//boost::shared_ptr<TIFFRGBAImage> m_rgbaImage; ///< libtiff RGBA image for decoding unusual formats

		SqDirectoryData m_currDir;		///< data describing current directory.
};




//==============================================================================
// Implementation of inline functions and templates
//==============================================================================


//------------------------------------------------------------------------------
// CqTiffInputFile
//------------------------------------------------------------------------------
// inlines
inline TqUint CqTiffInputFile::width() const
{
	return m_currDir.imageWidth;
}

inline TqUint CqTiffInputFile::height() const
{
	return m_currDir.imageHeight;
}

inline TqUint CqTiffInputFile::tileWidth() const
{
	return m_currDir.tileWidth;
}

inline TqUint CqTiffInputFile::tileHeight() const
{
	return m_currDir.tileHeight;
}

//------------------------------------------------------------------------------
// templates
template<typename T>
boost::intrusive_ptr<CqTextureTile<T> > CqTiffInputFile::readTile(const TqUint x, const TqUint y)
{
	if(x >= m_currDir.numTilesX)
		throw XqTiffError("Column index out of bounds", __FILE__, __LINE__);
	if(y >= m_currDir.numTilesY)
		throw XqTiffError("Row index out of bounds", __FILE__, __LINE__);

	// Check if the data size, T, matches up with the size of the internal
	// representation.  Ideally we'd check if the types are equal, but I'm not
	// sure the extra machinery would be worth it.
	//
	// We could do data format conversion here instead of throwing an error I suppose...
	if(sizeof(T)*8 != m_currDir.bitsPerSample)
		throw XqTiffError("miss-match between requested vs actual bits per sample", __FILE__, __LINE__);

	// The tile size is smaller if it lies partly off the edge of the image -
	// take account of this.
	TqUint tileWidth = m_currDir.tileWidth;
	TqUint tileHeight = m_currDir.tileHeight;
	if((x+1)*tileWidth > m_currDir.imageWidth)
		tileWidth = m_currDir.imageWidth - x*tileWidth;
	if((y+1)*tileHeight > m_currDir.imageHeight)
		tileHeight = m_currDir.imageHeight - y*tileHeight;

	// Read in the tile data.
	boost::shared_array<T> tileData(0);
	CqTiffDirHandle dirHandle(m_fileHandlePtr, m_currDir.index);
	if(m_currDir.isTiled)
	{
		// Tiff is stored as tiles
		tsize_t dataSize = TIFFTileSize(dirHandle.tiffPtr());
		tileData = tiffMalloc<T>(dataSize);
		if(TIFFReadTile(dirHandle.tiffPtr(), tileData.get(), x*m_currDir.tileWidth,
					y*m_currDir.tileHeight, 0, 0) == -1)
			handleTiffReadError(tileData, dataSize/sizeof(T));
	}
	else
	{
		// Tiff is stored in strips - treat each strip as a wide tile.
		tsize_t dataSize = TIFFStripSize(dirHandle.tiffPtr());
		tileData = tiffMalloc<T>(dataSize);
		if(TIFFReadEncodedStrip(dirHandle.tiffPtr(), TIFFComputeStrip(dirHandle.tiffPtr(),
					y*m_currDir.tileHeight, 0), tileData.get(), -1) == -1)
			handleTiffReadError(tileData, dataSize/sizeof(T));
	}

	return boost::intrusive_ptr<CqTextureTile<T> > ( new CqTextureTile<T>(
				tileData, tileWidth, tileHeight, x*tileWidth, y*tileHeight,
				m_currDir.samplesPerPixel) );
}


//------------------------------------------------------------------------------
template<typename T>
void CqTiffInputFile::handleTiffReadError(boost::shared_array<T> data, const TqUint size) const
{
	// gracefully degrade when there's an error reading data from a tiff file.
	Aqsis::log() << error << "Error reading data from tiff file \""
		<< m_fileName << "\".  Using blank (white) tile\n";
	for(TqUint i = 0; i < size; i++)
		data[i] = 1;
}


//------------------------------------------------------------------------------
} // namespace Aqsis

#endif // TIFFFILE_H_INCLUDED
