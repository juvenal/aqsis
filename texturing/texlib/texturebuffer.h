// Aqsis
// Copyright (C) 1997 - 2001, Paul C. Gregory
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

/**
 * \file
 *
 * \brief Declare a texture buffer class and associated machinery.
 *
 * \author Chris Foster  chris42f _at_ gmail.com
 */


#ifndef TEXTUREBUFFER_H_INCLUDED
#define TEXTUREBUFFER_H_INCLUDED

#include "aqsis.h"

#include <boost/shared_array.hpp>

#include "channellist.h"
#include "samplevector.h"

namespace Aqsis {

//------------------------------------------------------------------------------
/** \brief Holds homogeneous texture data in a flat buffer.
 *
 * This class holds a flat buffer of texture data, in which the channel types
 * are all the same.
 */
template<typename T>
class AQSISTEX_SHARE CqTextureBuffer
{
	public:
		/// Construct a texture buffer with width = height = 0
		CqTextureBuffer();

		/** \brief Construct a texture buffer
		 *
		 * \param width - buffer width
		 * \param height - buffer height
		 * \param numChannels - number of channels of type T per pixel.
		 */
		CqTextureBuffer(const TqInt width, const TqInt height,
				const TqInt numChannels);

		/** \brief Copy constructor
		 *
		 * This constructor creates a copy of rhs, but possibly with a
		 * different type of underlying pixel type.  The copying semantics is
		 * defined by operator=().
		 */
		template<typename T2>
		CqTextureBuffer(const CqTextureBuffer<T2>& rhs);

		//--------------------------------------------------
		/// \name Modifiers for pixel data
		//@{
		/** \brief Copy the given buffer onto this one.
		 *
		 * The copy operation resizes the current buffer to be the size of the
		 * rhs buffer if necessary.  Conversion between the pixel data formats
		 * is simply by calling setPixel(x, y, rhs(x, y)) for every pixel (x,y)
		 * in the buffer.
		 */
		template<typename T2>
		CqTextureBuffer<T>& operator=(const CqTextureBuffer<T2>& rhs);
		/** \brief Set the value of a pixel in the underlying data
		 *
		 * This function converts floating point values to the underlying
		 * channel type.  If the underlying type is an integral type, the
		 * function first clamps the given values between 0 and 1, followed by
		 * multiplication by the maximum value for the range.  If the
		 * underlying type is floating point, a trivial conversion is
		 * performed.
		 *
		 * \param x - pixel index in width direction (column index)
		 * \param y - pixel index in height direction (row index)
		 * \param pixelValue - vector type which may be indexed with operator[]
		 *                     to obtain floating point values for the pixel
		 *                     (may be TqFloat*)
		 */
		template<typename PixelVectorT>
		void setPixel(const TqInt x, const TqInt y, const PixelVectorT& pixelValue);
		/** \brief Resize the buffer
		 *
		 * \param width  - new width
		 * \param height - new height
		 * \param channelList - new channel list for the buffer.
		 *
		 * \throw XqInternal If the new required channel list has channel types
		 * which are incompatible with this texture buffer.
		 */
		void resize(TqInt width, TqInt height, const CqChannelList& channelList);
		/** \brief Resize the buffer
		 *
		 * \param width - new width
		 * \param height - new height
		 * \param numChannels - new number of channels per pixel.
		 */
		void resize(TqInt width, TqInt height, TqInt numChannels);
		//@}

		//--------------------------------------------------
		/// \name Indexing operations
		//@{
		/** \brief 2D Indexing operator - floating point pixel interface.
		 *
		 * The returned vector is a lightweight view onto the underlying pixel,
		 * which presents the data as floating point values.
		 *
		 * \param x - pixel index in width direction (column index)
		 * \param y - pixel index in height direction (row index)
		 * \return a lightweight vector holding a reference to the channels data
		 */
		const CqSampleVector<T> operator()(const TqInt x, const TqInt y) const;
		/** \brief 2D Indexing - access to the underlying typed channel data.
		 *
		 * \param x - pixel index in width direction (column index)
		 * \param y - pixel index in height direction (row index)
		 * \return a pointer to the channel data for the given pixel
		 */
		T* value(const TqInt x, const TqInt y);
		/// 2D indexing of typed channel data, const version.
		const T* value(const TqInt x, const TqInt y) const;
		//@}

		//--------------------------------------------------
		/// \name Access to buffer dimensions & metadata
		//@{
		/** \brief Get the list of channels for the buffer.
		 *
		 * \return a (synthetic) list of channels with the correct type and
		 * number for the buffer.
		 */
		CqChannelList channelList() const;
		/// Get the buffer width
		TqInt width() const;
		/// Get the buffer height
		TqInt height() const;
		/// Get the number of channels per pixel
		TqInt numChannels() const;
		//@}

		//--------------------------------------------------
		/// \name Access to raw pixel data
		//@{
		/// Get a pointer to the underlying raw data
		TqUint8* rawData();
		/// Get a pointer to the underlying raw data (const version)
		const TqUint8* rawData() const;
		//@}

	private:
		boost::shared_array<T> m_pixelData;	///< Pointer to the underlying pixel data.
		TqInt m_width;				///< Width of the buffer
		TqInt m_height;				///< Height of the buffer
		TqInt m_numChannels;		///< Number of channels per pixel
};



//==============================================================================
// Implementation of inline functions and templates
//==============================================================================

// Inline functions/templates for CqTextureBuffer

template<typename T>
inline CqTextureBuffer<T>::CqTextureBuffer()
	: m_pixelData(),
	m_width(0),
	m_height(0),
	m_numChannels(0)
{ }

template<typename T>
inline CqTextureBuffer<T>::CqTextureBuffer(TqInt width, TqInt height, TqInt numChannels)
	: m_pixelData(new T[width * height * numChannels]),
	m_width(width),
	m_height(height),
	m_numChannels(numChannels)
{ }

template<typename T>
template<typename T2>
CqTextureBuffer<T>::CqTextureBuffer(const CqTextureBuffer<T2>& rhs)
	: m_pixelData(),
	m_width(0),
	m_height(0),
	m_numChannels(0)
{
	*this = rhs;
}

template<typename T>
inline const CqSampleVector<T> CqTextureBuffer<T>::operator()(const TqInt x, const TqInt y) const
{
	return CqSampleVector<T>(value(x,y));
}

template<typename T>
template<typename PixelVectorT>
inline void CqTextureBuffer<T>::setPixel(const TqInt x, const TqInt y, const PixelVectorT& pixelValue)
{
	T* pixel = value(x,y);
	if(std::numeric_limits<T>::is_integer)
	{
		// T is an integral type; need to rescale channel...
		for(TqInt chan = 0; chan < m_numChannels; ++chan)
		{
			pixel[chan] = static_cast<T>(std::numeric_limits<T>::max()
					* clamp(pixelValue[chan], 0.0f, 1.0f));
		}
	}
	else
	{
		// floating point; do simple conversion.
		for(TqInt chan = 0; chan < m_numChannels; ++chan)
			pixel[chan] = static_cast<T>(pixelValue[chan]);
	}
}

template<typename T>
inline T* CqTextureBuffer<T>::value(const TqInt x, const TqInt y)
{
	// Call the const version
	return const_cast<T*>(
			const_cast<const CqTextureBuffer<T>*>(this)->value(x,y)
			);
}

template<typename T>
inline const T* CqTextureBuffer<T>::value(const TqInt x, const TqInt y) const
{
	assert(x >= 0);
	assert(x < m_width);
	assert(y >= 0);
	assert(y < m_height);
	return m_pixelData.get() + (y*m_width + x)*m_numChannels;
}

template<typename T>
void CqTextureBuffer<T>::resize(TqInt width, TqInt height, const CqChannelList& channelList)
{
	if(channelList.sharedChannelType() != getChannelTypeEnum<T>())
	{
		AQSIS_THROW(XqInternal, "CqTextureBuffer channel type is"
				"incompatible with new channel type requested");
	}
	resize(width, height, channelList.bytesPerPixel()/sizeof(T));
}

template<typename T>
inline void CqTextureBuffer<T>::resize(TqInt width, TqInt height, TqInt numChannels)
{
	TqInt newSize = width * height * numChannels;
	if(newSize != m_width * m_height * m_numChannels);
		m_pixelData.reset(new T[newSize]);
	// Set new buffer sizes
	m_width = width;
	m_height = height;
	m_numChannels = numChannels;
}

template<typename T>
inline CqChannelList CqTextureBuffer<T>::channelList() const
{
	CqChannelList chanList;
	chanList.addUnnamedChannels(getChannelTypeEnum<T>(), m_numChannels);
	return chanList;
}

template<typename T>
inline TqUint8* CqTextureBuffer<T>::rawData()
{
	return reinterpret_cast<TqUint8*>(m_pixelData.get());
}

template<typename T>
inline const TqUint8* CqTextureBuffer<T>::rawData() const
{
	return reinterpret_cast<const TqUint8*>(m_pixelData.get());
}

template<typename T>
inline TqInt CqTextureBuffer<T>::width() const
{
	return m_width;
}

template<typename T>
inline TqInt CqTextureBuffer<T>::height() const
{
	return m_height;
}

template<typename T>
inline TqInt CqTextureBuffer<T>::numChannels() const
{
	return m_numChannels;
}

template<typename T>
template<typename T2>
CqTextureBuffer<T>& CqTextureBuffer<T>::operator=(const CqTextureBuffer<T2>& rhs)
{
	resize(rhs.width(), rhs.height(), rhs.numChannels());
	for(TqInt y = 0; y < height(); ++y)
	{
		for(TqInt x = 0; x < width(); ++x)
		{
			setPixel(x, y, rhs(x,y));
		}
	}
	return *this;
}

} // namespace Aqsis

#endif // TEXTUREBUFFER_H_INCLUDED
