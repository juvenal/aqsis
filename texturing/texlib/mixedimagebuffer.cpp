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
 * \brief Implementation of CqMixedImageBuffer and related stuff.
 *
 * \author Chris Foster  chris42f _at_ gmail.com
 * \author Paul C. Gregory (pgregory@aqsis.org)
 *
 */

#include "mixedimagebuffer.h"

#include <boost/format.hpp>

#include "exception.h"
#include "logging.h"
#include "ndspy.h"

namespace Aqsis {

//------------------------------------------------------------------------------
// CqMixedImageBuffer implementation
CqMixedImageBuffer::CqMixedImageBuffer()
	: m_channelInfo(),
	m_width(0),
	m_height(0),
	m_data()
{ }

CqMixedImageBuffer::CqMixedImageBuffer(const CqChannelList& channels, TqInt width, TqInt height)
	: m_channelInfo(channels),
	m_width(width),
	m_height(height),
	m_data(new TqUchar[width*height*channels.bytesPerPixel()])
{ }

CqMixedImageBuffer::CqMixedImageBuffer(const CqChannelList& channels,
		boost::shared_array<TqUchar> data, TqInt width, TqInt height)
	: m_channelInfo(channels),
	m_width(width),
	m_height(height),
	m_data(data)
{ }


void CqMixedImageBuffer::saveToTiff(TIFF* pOut) const
{
	if ( !pOut )
		return;

	TIFFSetField( pOut, TIFFTAG_PLANARCONFIG, uint16(PLANARCONFIG_CONTIG) );
	TIFFSetField( pOut, TIFFTAG_PHOTOMETRIC, PHOTOMETRIC_RGB );
	if ( m_channelInfo.numChannels() == 4 )
	{
		short ExtraSamplesTypes[ 1 ] = {EXTRASAMPLE_ASSOCALPHA};
		TIFFSetField( pOut, TIFFTAG_EXTRASAMPLES, 1, ExtraSamplesTypes );
	}

	TIFFSetField( pOut, TIFFTAG_IMAGEWIDTH, uint32(m_width) );
	TIFFSetField( pOut, TIFFTAG_IMAGELENGTH, uint32(m_height) );
	TIFFSetField( pOut, TIFFTAG_ORIENTATION, ORIENTATION_TOPLEFT );
	TIFFSetField( pOut, TIFFTAG_SAMPLESPERPIXEL, m_channelInfo.numChannels() );
	TIFFSetField( pOut, TIFFTAG_ROWSPERSTRIP, TIFFDefaultStripSize( pOut, 0 ) );

	// Work out the format of the image to write.
	EqChannelType widestType = Channel_Unsigned8;
	for(CqChannelList::const_iterator ichan = m_channelInfo.begin();
			ichan != m_channelInfo.end() ; ++ichan)
		widestType = Aqsis::min(widestType, ichan->type);

	// Write out an 8 bits per pixel integer image.
	if ( widestType == Channel_Unsigned8 || widestType == Channel_Signed8 )
	{
		TIFFSetField( pOut, TIFFTAG_SAMPLEFORMAT, SAMPLEFORMAT_INT );
		TIFFSetField( pOut, TIFFTAG_BITSPERSAMPLE, 8 );
	}
	else if(widestType == Channel_Float32)
	{
		/* use uncompressed IEEEFP pixels */
		TIFFSetField( pOut, TIFFTAG_SAMPLEFORMAT, SAMPLEFORMAT_IEEEFP );
		TIFFSetField( pOut, TIFFTAG_BITSPERSAMPLE, 32 );
	}
	else if(widestType == Channel_Signed16)
	{
		TIFFSetField( pOut, TIFFTAG_SAMPLEFORMAT, SAMPLEFORMAT_INT );
		TIFFSetField( pOut, TIFFTAG_BITSPERSAMPLE, 16 );
	}
	else if(widestType == Channel_Unsigned16)
	{
		TIFFSetField( pOut, TIFFTAG_SAMPLEFORMAT, SAMPLEFORMAT_UINT );
		TIFFSetField( pOut, TIFFTAG_BITSPERSAMPLE, 16 );
	}
	else if(widestType == Channel_Signed32)
	{
		TIFFSetField( pOut, TIFFTAG_SAMPLEFORMAT, SAMPLEFORMAT_INT );
		TIFFSetField( pOut, TIFFTAG_BITSPERSAMPLE, 32 );
	}
	else if(widestType == Channel_Unsigned32)
	{
		TIFFSetField( pOut, TIFFTAG_SAMPLEFORMAT, SAMPLEFORMAT_UINT );
		TIFFSetField( pOut, TIFFTAG_BITSPERSAMPLE, 32 );
	}
	// Write out the actual pixel data.
	TqInt lineLength = m_channelInfo.bytesPerPixel() * m_width;
	TqUchar* dataPtr = m_data.get();
	for (TqInt row = 0; row < m_height; row++ )
	{
		if ( TIFFWriteScanline( pOut, reinterpret_cast<void*>(dataPtr) , row, 0 ) < 0 )
			break;
		dataPtr += lineLength;
	}
}

void CqMixedImageBuffer::clearBuffer(TqFloat f)
{
	// Fill all channels with the given constant.
	CqImageChannelConstant constChan(f);
	for(TqInt chanNum = 0; chanNum < m_channelInfo.numChannels(); ++chanNum)
		channel(chanNum)->copyFrom(constChan);
}

void CqMixedImageBuffer::initToCheckerboard(TqInt tileSize)
{
	// Fill all channels with checker pattern
	CqImageChannelCheckered checkerChan(tileSize);
	for(TqInt chanNum = 0; chanNum < m_channelInfo.numChannels(); ++chanNum)
		channel(chanNum)->copyFrom(checkerChan);
}

void CqMixedImageBuffer::resize(TqInt width, TqInt height,
		const CqChannelList& channels)
{
	if(width*height*channels.bytesPerPixel()
			!= m_width*m_height*m_channelInfo.bytesPerPixel())
	{
		// resize the buffer if the new buffer 
		m_data.reset(new TqUchar[width*height*channels.bytesPerPixel()]);
	}
	m_channelInfo = channels;
	m_width = width;
	m_height = height;
}

void CqMixedImageBuffer::getCopyRegionSize(TqInt offset, TqInt srcWidth, TqInt destWidth,
		TqInt& srcOffset, TqInt& destOffset, TqInt& copyWidth)
{
	destOffset = max(offset, 0);
	srcOffset = max(-offset, 0);
	copyWidth = min(destWidth, offset+srcWidth) - destOffset;
}

void CqMixedImageBuffer::copyFrom(const CqMixedImageBuffer& source, TqInt topLeftX, TqInt topLeftY)
{
	if(source.m_channelInfo.numChannels() != m_channelInfo.numChannels())
		throw XqInternal("Number of source and destination channels do not match", __FILE__, __LINE__);

	// compute size and top left coords of region to copy.
	TqInt copyWidth = 0;
	TqInt destTopLeftX = 0;
	TqInt srcTopLeftX = 0;
	getCopyRegionSize(topLeftX, source.m_width, m_width,
			srcTopLeftX, destTopLeftX, copyWidth);
	TqInt copyHeight = 0;
	TqInt destTopLeftY = 0;
	TqInt srcTopLeftY = 0;
	getCopyRegionSize(topLeftY, source.m_height, m_height,
			srcTopLeftY, destTopLeftY, copyHeight);
	// return if no overlap
	if(copyWidth <= 0 || copyHeight <= 0)
		return;

	for(TqInt i = 0; i < m_channelInfo.numChannels(); ++i)
	{
		channel(i, destTopLeftX, destTopLeftY, copyWidth, copyHeight)
			->copyFrom(*source.channel(i, srcTopLeftX, srcTopLeftY,
						copyWidth, copyHeight));
	}
}

void CqMixedImageBuffer::copyFrom(const CqMixedImageBuffer& source, const TqChannelNameMap& nameMap,
		TqInt topLeftX, TqInt topLeftY)
{
	// compute size and top left coords of region to copy.
	TqInt copyWidth = 0;
	TqInt destTopLeftX = 0;
	TqInt srcTopLeftX = 0;
	getCopyRegionSize(topLeftX, source.m_width, m_width,
			srcTopLeftX, destTopLeftX, copyWidth);
	TqInt copyHeight = 0;
	TqInt destTopLeftY = 0;
	TqInt srcTopLeftY = 0;
	getCopyRegionSize(topLeftY, source.m_height, m_height,
			srcTopLeftY, destTopLeftY, copyHeight);
	// return if no overlap
	if(copyWidth <= 0 || copyHeight <= 0)
		return;

	for(TqChannelNameMap::const_iterator i = nameMap.begin(), e = nameMap.end();
			i != e; ++i)
	{
		channel(i->first, destTopLeftX, destTopLeftY, copyWidth, copyHeight)
			->copyFrom(*source.channel(i->second, srcTopLeftX, srcTopLeftY,
						copyWidth, copyHeight));
	}
}

void CqMixedImageBuffer::compositeOver(const CqMixedImageBuffer& source,
		const TqChannelNameMap& nameMap, TqInt topLeftX, TqInt topLeftY,
		const std::string alphaName)
{
	if(!source.channels().hasChannel(alphaName))
	{
		copyFrom(source, nameMap, topLeftX, topLeftY);
	}
	else
	{
		// compute size and top left coords of region to copy.
		TqInt copyWidth = 0;
		TqInt destTopLeftX = 0;
		TqInt srcTopLeftX = 0;
		getCopyRegionSize(topLeftX, source.m_width, m_width,
				srcTopLeftX, destTopLeftX, copyWidth);
		TqInt copyHeight = 0;
		TqInt destTopLeftY = 0;
		TqInt srcTopLeftY = 0;
		getCopyRegionSize(topLeftY, source.m_height, m_height,
				srcTopLeftY, destTopLeftY, copyHeight);
		// return if no overlap
		if(copyWidth <= 0 || copyHeight <= 0)
			return;

		for(TqChannelNameMap::const_iterator i = nameMap.begin(), e = nameMap.end();
				i != e; ++i)
		{
			channel(i->first, destTopLeftX, destTopLeftY, copyWidth, copyHeight)
				->compositeOver(*source.channel(i->second, srcTopLeftX, srcTopLeftY, copyWidth, copyHeight),
						*source.channel(alphaName, srcTopLeftX, srcTopLeftY, copyWidth, copyHeight));
		}
	}
}

inline boost::shared_ptr<CqImageChannel> CqMixedImageBuffer::channel(const std::string& name,
		TqInt topLeftX, TqInt topLeftY, TqInt width, TqInt height)
{
	return channelImpl(m_channelInfo.findChannelIndex(name),
			topLeftX, topLeftY, width, height);
}

inline boost::shared_ptr<CqImageChannel> CqMixedImageBuffer::channel(TqInt index, TqInt topLeftX,
		TqInt topLeftY, TqInt width, TqInt height)
{
	return channelImpl(index, topLeftX, topLeftY, width, height);
}

inline boost::shared_ptr<const CqImageChannel> CqMixedImageBuffer::channel(const std::string& name,
		TqInt topLeftX, TqInt topLeftY, TqInt width, TqInt height) const
{
	return channelImpl(m_channelInfo.findChannelIndex(name),
			topLeftX, topLeftY, width, height);
}

inline boost::shared_ptr<const CqImageChannel> CqMixedImageBuffer::channel(TqInt index,
		TqInt topLeftX, TqInt topLeftY, TqInt width, TqInt height) const
{
	return channelImpl(index, topLeftX, topLeftY, width, height);
}

boost::shared_ptr<CqImageChannel> CqMixedImageBuffer::channelImpl(TqInt index,
		TqInt topLeftX, TqInt topLeftY, TqInt width, TqInt height) const
{
	if(width == 0)
		width = m_width;
	if(height == 0)
		height = m_height;
	assert(topLeftX + width <= m_width);
	assert(topLeftY + height <= m_height);
	TqInt stride = m_channelInfo.bytesPerPixel();
	// Start offset for the channel
	TqUchar* startPtr = m_data.get()
			+ (topLeftY*m_width + topLeftX)*stride
			+ m_channelInfo.channelByteOffset(index);
	TqInt rowSkip = m_width - width;
	switch(m_channelInfo[index].type)
	{
		case Channel_Float32:
			return boost::shared_ptr<CqImageChannel>(
					new CqImageChannelTyped<PtDspyFloat32>(m_channelInfo[index],
						startPtr, width, height, stride, rowSkip));
		case Channel_Unsigned32:
			return boost::shared_ptr<CqImageChannel>(
					new CqImageChannelTyped<PtDspyUnsigned32>(m_channelInfo[index],
						startPtr, width, height, stride, rowSkip));
		case Channel_Signed32:
			return boost::shared_ptr<CqImageChannel>(
					new CqImageChannelTyped<PtDspySigned32>(m_channelInfo[index],
						startPtr, width, height, stride, rowSkip));
		case Channel_Unsigned16:
			return boost::shared_ptr<CqImageChannel>(
					new CqImageChannelTyped<PtDspyUnsigned16>(m_channelInfo[index],
						startPtr, width, height, stride, rowSkip));
		case Channel_Signed16:
			return boost::shared_ptr<CqImageChannel>(
					new CqImageChannelTyped<PtDspySigned16>(m_channelInfo[index],
						startPtr, width, height, stride, rowSkip));
		case Channel_Signed8:
			return boost::shared_ptr<CqImageChannel>(
					new CqImageChannelTyped<PtDspySigned8>(m_channelInfo[index],
						startPtr, width, height, stride, rowSkip));
		case Channel_Unsigned8:
		default:
			return boost::shared_ptr<CqImageChannel>(
					new CqImageChannelTyped<PtDspyUnsigned8>(m_channelInfo[index],
						startPtr, width, height, stride, rowSkip));
	}
}

} // namespace Aqsis
