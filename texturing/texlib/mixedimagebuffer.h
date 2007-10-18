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
 * \brief Define an image buffer class for manipulating raw image data.
 *
 * \author Chris Foster  chris42f _at_ gmail.com
 *
 */

#ifndef IMAGEBUFFER_H_INCLUDED
#define IMAGEBUFFER_H_INCLUDED

#include "aqsis.h"

#include <vector>
#include <string>
#include <map>

#include <boost/shared_ptr.hpp>
#include <boost/shared_array.hpp>
#include <boost/utility.hpp>

#include "aqsismath.h"
#include "exception.h"
#include "imagechannel.h"
#include "channellist.h"

namespace Aqsis {

typedef std::map<std::string, std::string> TqChannelNameMap;

//------------------------------------------------------------------------------
/** \brief A wrapper for an array of image data with mixed channel types
 *
 * This class wraps around raw image data, composed of multiple channels, and
 * provides interfaces for accessing the channels individually.
 *
 * A central design feature is the ability to place channels with different
 * types contiguously in the same pixel data.
 *
 * Rectangular subregions of individual image channels can be selected out of
 * one buffer, and copied without restriction into an arbitrary channel of   
 * another buffer, provided the width and height of the regions match.  The
 * usage is simple:
 *   CqMixedImageBuffer buf1(some_args...);
 *   CqMixedImageBuffer buf2(some_args2...);
 *   // copy a subregion of the buf1 green channel onto buf2 red channel:
 *   buf2.channel("r")->copyFrom(*buf1.channel("g", topLeftX, topLeftY, width, height));
 *
 * This class is noncopyable currently, but only because the default copy
 * operation may not make sense.  It would be extremly easy to implement some
 * sensible copying behaviour.
 */
COMMON_SHARE class CqMixedImageBuffer : boost::noncopyable
{
	public:
		/** \brief Construct an empty image buffer
		 *
		 * This constructor sets channel list to empty and width = height = 0.
		 * The raw data is uninitialized.
		 */
		CqMixedImageBuffer();
		/** \brief Construct an image buffer with the given channels per pixel.
 		 *
 		 * \param channelList - description of image channels
 		 * \param width - buffer width in pixels
 		 * \param height - buffer height in pixels
		 */
		CqMixedImageBuffer(const CqChannelList& channelList, TqInt width, TqInt height);
		/** \brief Construct an image buffer with the given channels per pixel.
 		 *
 		 * \param channelList - description of image channels
 		 * \param data - preexisting raw data to wrap the channel around
 		 * \param width - buffer width in pixels
 		 * \param height - buffer height in pixels
		 */
		CqMixedImageBuffer(const CqChannelList& channelList, boost::shared_array<TqUchar> data,
				TqInt width, TqInt height);

		//------------------------------------------------------------
		/// \name Functions to (re)initialize the buffer
		//@{
		/** \brief Set all channels to a fixed value.
		 */
		void clearBuffer(TqFloat f = 0.0f);
		/** \brief Initialise the buffer to a checkerboard to show alpha.
		 *
		 * \param tileSize - size of the square checker "tiles" in pixels
		 */
		void initToCheckerboard(TqInt tileSize = 16);
		/** \brief Resize the buffer and change the channel structure.
		 *
		 * \param width  - new width
		 * \param height - new height
		 * \param channelList - new channel list for the buffer.
		 */
		void resize(TqInt width, TqInt height, const CqChannelList& channelList);
		//@}

		//------------------------------------------------------------
		/// \name Functions to copy or composite from another buffer onto this one
		//@{
		/** \brief Copy a source buffer onto this one.
		 *
 		 * Parts of the source buffer may fall outside the destination buffer
 		 * (for instance, if topLeftX or topLeftY are negative).  In this case,
 		 * the regions lying outside the destination buffer are quietly ignored.
 		 *
		 * \param source - source buffer
		 * \param topLeftX - x coordinate where the top left of the source
		 *                   buffer will map onto.
		 * \param topLeftY - y coordinate where the top left of the source
		 *                   buffer will map onto.
		 */
		void copyFrom(const CqMixedImageBuffer& source, TqInt topLeftX = 0, TqInt topLeftY = 0);
		/** \brief Copy a source buffer onto this one.
		 *
		 * This allows arbitrary source channels be mapped onto arbitrary
		 * destination channels by name.
		 *
 		 * Parts of the source buffer may fall outside the destination buffer
 		 * (for instance, if topLeftX or topLeftY are negative).  In this case,
 		 * the regions lying outside the destination buffer are quietly ignored.
 		 *
		 * \param source - source buffer
		 * \param nameMap - a map between the names of the destination (this)
		 *                  and source buffers.
		 * \param topLeftX - x coordinate where the top left of the source
		 *                   buffer will map onto.
		 * \param topLeftY - y coordinate where the top left of the source
		 *                   buffer will map onto.
		 */
		void copyFrom(const CqMixedImageBuffer& source, const TqChannelNameMap& nameMap,
				TqInt topLeftX = 0, TqInt topLeftY = 0);
		/** \brief Composite the given image buffer on top of this one.
		 *
		 * If the alpha channel name is not present in the source channel,
		 * attempt a simple copy using the equivilant copyFrom() method.
		 *
		 * \param source - source buffer
		 * \param nameMap - a map between the names of the destination (this)
		 *                  and source buffers.
		 * \param alphaName - name of the alpha channel
		 * \param topLeftX - x coordinate where the top left of the source
		 *                   buffer will map onto.
		 * \param topLeftY - y coordinate where the top left of the source
		 *                   buffer will map onto.
		 */
		void compositeOver(const CqMixedImageBuffer& source,
				const TqChannelNameMap& nameMap, TqInt topLeftX = 0,
				TqInt topLeftY = 0, const std::string alphaName = "a");
		//@}

		//------------------------------------------------------------
		/// \name Accessors for buffer metadata and raw pixel data
		//@{
		/// \brief Get the channel information for this buffer
		inline const CqChannelList& channelList() const;
		/// Get the width of the image buffer in pixels
		inline TqInt width() const;
		/// Get the height of the image buffer in pixels
		inline TqInt height() const;
		/// Get a const pointer to the underlying raw data.
		inline const TqUchar* rawData() const;
		/// Get a pointer to the underlying raw data.
		inline TqUchar* rawData();
		//@}

		//------------------------------------------------------------
		/// \name Functions to retrive image channel subregions
		//@{
		/** \brief Retreive the image channel subregion with the given name
		 *
		 * \param name - name of the channel
		 * \param topLeftX - x-coordinate for the region, counting from the top left of the image
		 * \param topLeftY - y-coordinate for the region, counting from the top left of the image
		 * \param width - width of the region.  If zero, use the full image width.
		 * \param height - height of the region.  If zero, use the full image height.
		 *
		 * \return The named channel subregion.
		 */
		inline boost::shared_ptr<CqImageChannel> channel(const std::string& name,
				TqInt topLeftX = 0, TqInt topLeftY = 0, TqInt width = 0,
				TqInt height = 0);
		/// Retrieve a named image channel subregion (const version)
		inline boost::shared_ptr<const CqImageChannel> channel(const std::string& name,
				TqInt topLeftX = 0, TqInt topLeftY = 0, TqInt width = 0,
				TqInt height = 0) const;
		/** \brief Retreive the image channel subregion with the given index in
		 * the list of channels.
		 *
		 * \see channel(const std::string& name, ...)
		 */
		inline boost::shared_ptr<CqImageChannel> channel(TqInt index,
				TqInt topLeftX = 0, TqInt topLeftY = 0, TqInt width = 0,
				TqInt height = 0);
		/// Retrieve an image channel subregion by index (const version)
		inline boost::shared_ptr<const CqImageChannel> channel(TqInt index,
				TqInt topLeftX = 0, TqInt topLeftY = 0, TqInt width = 0,
				TqInt height = 0) const;
		//@}

	private:
		/** \brief Implmentation for the various possible channel() calls in
		 * the interface.
		 *
		 * \see channel
		 */
		boost::shared_ptr<CqImageChannel> channelImpl(TqInt index, TqInt topLeftX,
				TqInt topLeftY, TqInt width, TqInt height) const;
		/** \brief Compute the byte offsets into each image channel.
		 *
		 * \param channelList - list of channel information
		 * \return The byte-offsets for the start of each channel in the image.
		 * This is one element longer than channels; the last element contains
		 * the 
		 */
		static std::vector<TqInt> channelOffsets(const CqChannelList& channelList);
		/** \brief Compute region offsets/sizes when copying one buffer onto another.
		 *
		 * \param offset - desired start index in destination region (may be < 0)
		 * \param srcWidth - natural width of source region
		 * \param destWidth - natural width of destination region
		 * \param srcOffset - start index in source region (output)
		 * \param destOffset - start index in destination region (output)
		 * \param copyWidth - width of the actual region to copy (output).
		 *                    Will be negative the source and destinations don't
		 *                    overlap
		 */
		static void getCopyRegionSize(TqInt offset, TqInt srcWidth, TqInt destWidth,
				TqInt& srcOffset, TqInt& destOffset, TqInt& copyWidth);

	private:
		CqChannelList m_channelList; ///< vector of channel information
		TqInt m_width; ///< buffer width in pixels
		TqInt m_height; ///< buffer height in pixels
		boost::shared_array<TqUchar> m_data; ///< raw image data
};


//------------------------------------------------------------------------------
// Implementation details
//------------------------------------------------------------------------------

//------------------------------------------------------------------------------
// CqMixedImageBuffer

inline const CqChannelList& CqMixedImageBuffer::channelList() const
{
	return m_channelList;
}

inline TqInt CqMixedImageBuffer::width() const
{
	return m_width;
}

inline TqInt CqMixedImageBuffer::height() const
{
	return m_height;
}

inline const TqUchar* CqMixedImageBuffer::rawData() const
{
	return m_data.get();
}

inline TqUchar* CqMixedImageBuffer::rawData()
{
	return m_data.get();
}

} // namespace Aqsis

#endif // IMAGEBUFFER_H_INCLUDED
