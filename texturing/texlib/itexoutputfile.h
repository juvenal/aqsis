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
 * \brief A scanline-based output interface for texture files.
 *
 * \author Chris Foster
 */

#ifndef ITEXOUTPUTFILE_H_INCLUDED
#define ITEXOUTPUTFILE_H_INCLUDED

#include "aqsis.h"

#include <string>

#include "exception.h"
#include "imagefiletype.h"
#include "mixedimagebuffer.h"
#include "smartptr.h"
#include "texfileheader.h"

namespace Aqsis {

//------------------------------------------------------------------------------
/** \brief Scanline-oriented image file output.
 *
 * This interface wraps around the various image file types, providing a
 * uniform interface for placing image data into a file.  Metadata is handled
 * via the CqTexFileHeader container, while pixel data may be written into the
 * file one line at a time, or a whole set of lines at once.
 *
 * Although the interface is scanline-oriented, output of tiled data is
 * possible for those formats which support it.  The user signals the intent
 * to produce tiled output data by setting the appropriate attributes in the
 * file attribute header.
 */
class AQSISTEX_SHARE IqTexOutputFile
{
	public:
		virtual ~IqTexOutputFile() {};

		//--------------------------------------------------
		/// \name Metadata access
		//@{
		/// get the file name
		virtual const char* fileName() const = 0;
		/// get the file type
		virtual EqImageFileType fileType() = 0;
		/// Get the file header data
		virtual const CqTexFileHeader& header() const = 0;
		//@}

		/** Get the index for the current line
		 *
		 * The "current line" is equal to number of scanlines written so far + 1
		 * That is, the current line is the next line which will be written to
		 * in a subsequent call of writePixels()
		 *
		 * \return the current scan line
		 */
		virtual TqInt currentLine() const = 0;

		/** \brief Write a region of scanlines
		 *
		 * Array2DType is a type modelling a 2D array interface.  It should
		 * provide the following methods:
		 *
		 *   - TqUint8* Array2dType::rawData() returns a pointer to the raw
		 *     data.  The raw data is assumed at this stage to be contiguous -
		 *     (ie, not a nontrivial slice).
		 *
		 *   - Array2DType::channelList() returns a channel list for the array
		 *
		 *   - Array2DType::width() and
		 *   - Array2DType::height() return the dimensions of the array.
		 *
		 * All the scanlines in buffer are read and written to the output file,
		 * starting from the current write line as reported by currentLine().
		 * If the buffer is higher than the specified image height, use only
		 * the first few rows.
		 *
		 * \param buffer - buffer to read scanline data from.
		 */
		template<typename Array2DType>
		void writePixels(const Array2DType& buffer);

		/** \brief Open an input image file in a given format
		 *
		 * \param fileName - file to open.  Can be in any of the formats
		 * understood by aqsistex.
		 * \param fileType - the file type.
		 * \return The newly opened input file
		 */
		static boost::shared_ptr<IqTexOutputFile> open(const std::string& fileName,
				EqImageFileType fileType, const CqTexFileHeader& header);

	protected:
		/** \brief Low-level virtual implementation for writePixels().
		 *
		 * The parameter is a CqMixedImageBuffer - this allows image formats
		 * the maximum flexibility in deciding what to do, since they have full
		 * access to the channel structure.
		 *
		 * \param buffer - pixel data will be read from here.
		 */
		virtual void writePixelsImpl(const CqMixedImageBuffer& buffer) = 0;
};


//------------------------------------------------------------------------------
/** \brief Texture output file interface supporting multiple sub-images
 *
 * Some file types, such as TIFF support storage of multiple sub-images inside
 * a single file.  This interface supports such usage by allowing 
 */
class AQSISTEX_SHARE IqMultiTexOutputFile : public IqTexOutputFile
{
	public:
		/** \brief Create a new subimage
		 *
		 * This function first finalizes the current subimage before setting up
		 * the file for a new subimage with the same attributes as the previous
		 * subimage, apart from width and height which may be specified as
		 * arguments.
		 *
		 * \param width - width for the new subimage
		 * \param height - height for the new subimage
		 */
		virtual void newSubImage(TqInt width, TqInt height) = 0;

		/** \brief Create a new subimage with extra header data
		 *
		 * This function first finalizes the current subimage before creating a
		 * new subimage with the same header attributes as the previous one,
		 * apart from width and height which may be specified as arguments.
		 *
		 * \param header - header data for the current subimage.
		 */
		virtual void newSubImage(const CqTexFileHeader& header) = 0;

		/** \brief Open an input image file in a given format
		 *
		 * \param fileName - file to open.  Can be in any of the formats
		 * understood by aqsistex.
		 * \param fileType - the file type.
		 * \return The newly opened input file
		 */
		static boost::shared_ptr<IqMultiTexOutputFile> open(
				const std::string& fileName, EqImageFileType fileType,
				const CqTexFileHeader& header);
};

//==============================================================================
// Implementation of inline functions and templates
//==============================================================================

template<typename Array2DType>
void IqTexOutputFile::writePixels(const Array2DType& buffer)
{
	TqInt numScanlines = min(buffer.height(), header().height() - currentLine());
	if(buffer.width() != header().width())
	{
		AQSIS_THROW_XQERROR(XqInternal, EqE_Bug, "Cannot put pixels from buffer "
				"into file \"" << fileName() << "\": buffer has incorrect width.");
	}
	if(numScanlines > 0)
	{
		CqMixedImageBuffer newBuf(buffer.channelList(),
				boost::shared_array<TqUint8>(const_cast<TqUint8*>(buffer.rawData()),
					nullDeleter), buffer.width(), numScanlines);
		writePixelsImpl(newBuf);
	}
	else
	{
		AQSIS_THROW_XQERROR(XqInternal, EqE_Bug,
			"Attempt to write buffer off the end of an image");
	}
}

} // namespace Aqsis

#endif // ITEXOUTPUTFILE_H_INCLUDED
