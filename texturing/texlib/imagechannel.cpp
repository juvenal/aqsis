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
 * \brief Implementations of classes for manipulating image channels.
 *
 * \author Chris Foster  chris42f _at_ gmail.com
 *
 */

#include "imagechannel.h"

#include "exception.h"
#include "ndspy.h"

namespace Aqsis {
//------------------------------------------------------------------------------

EqChannelType chanFormatFromPkDspy(TqInt dspyFormat)
{
	switch(dspyFormat)
	{
		case PkDspyFloat32:
			return Channel_Float32;
		case PkDspyUnsigned32:
			return Channel_Unsigned32;
		case PkDspySigned32:
			return Channel_Signed32;
		case PkDspyUnsigned16:
			return Channel_Unsigned16;
		case PkDspySigned16:
			return Channel_Signed16;
		case PkDspyUnsigned8:
			return Channel_Unsigned8;
		case PkDspySigned8:
			return Channel_Signed8;
		default:
			AQSIS_THROW(XqInternal, "Unknown PkDspy data format");
	}
}

TqInt pkDspyFromChanFormat(EqChannelType format)
{
	switch(format)
	{
		case Channel_Float32:
			return PkDspyFloat32;
		case Channel_Unsigned32:
			return PkDspyUnsigned32;
		case Channel_Signed32:
			return PkDspySigned32;
		case Channel_Unsigned16:
			return PkDspyUnsigned16;
		case Channel_Signed16:
			return PkDspySigned16;
		case Channel_Unsigned8:
			return PkDspyUnsigned8;
		default:
		case Channel_Signed8:
			return PkDspySigned8;
	}
}

//------------------------------------------------------------------------------
// CqImageChannelConstant implementation
CqImageChannelConstant::CqImageChannelConstant(TqFloat value)
	: m_value(value),
	m_rowBuf()
{ }

void CqImageChannelConstant::requireSize(TqInt width, TqInt height) const
{
	if(static_cast<TqInt>(m_rowBuf.size()) != width)
		m_rowBuf.assign(width, m_value);
}

const TqFloatConv* CqImageChannelConstant::getRow(TqInt row) const
{
	return &m_rowBuf[0];
}

//------------------------------------------------------------------------------
// CqImageChannelCheckered implementation

CqImageChannelCheckered::CqImageChannelCheckered(TqInt tileSize)
	: m_tileSize(tileSize),
	m_checkerRow0(),
	m_checkerRow1()
{ }

void CqImageChannelCheckered::requireSize(TqInt width, TqInt height) const
{
	if(static_cast<TqInt>(m_checkerRow0.size()) != width)
	{
		m_checkerRow0.resize(width);
		m_checkerRow1.resize(width);
		// Make two buffers, one holding the first row of the checker pattern
		// and one holding the second.
		for(TqInt col = 0; col < width; ++col)
		{
			TqInt whichTile = (col % (m_tileSize*2)) / m_tileSize;
			m_checkerRow0[col] = (whichTile+1)*0.5f;
			m_checkerRow1[col] = (2-whichTile)*0.5f;
		}
	}
}

const TqFloatConv* CqImageChannelCheckered::getRow(TqInt row) const
{
	if( ((row % (m_tileSize*2)) / m_tileSize) == 0 )
		return &m_checkerRow0[0];
	else
		return &m_checkerRow1[0];
}


//------------------------------------------------------------------------------
// CqImageChannelZoom implementation
CqImageChannelZoom::CqImageChannelZoom(const IqImageChannelSource& src,
		TqInt zoomFactor)
	: m_source(src),
	m_zoomFactor(zoomFactor > 0 ? zoomFactor : 1),
	m_rowBuf()
{ }

void CqImageChannelZoom::requireSize(TqInt width, TqInt height) const
{
	m_source.requireSize(width/m_zoomFactor, height/m_zoomFactor);
	if(static_cast<TqInt>(m_rowBuf.size()) != width)
		m_rowBuf.resize(width);
}

const TqFloatConv* CqImageChannelZoom::getRow(TqInt row) const
{
	const TqFloatConv* srcRow = m_source.getRow(row/m_zoomFactor);
	TqInt srcSize = m_rowBuf.size()/m_zoomFactor;
	for(TqInt i = 0; i < srcSize; ++i)
	{
		for(TqInt j = i*m_zoomFactor, jEnd = (i+1)*m_zoomFactor; j < jEnd; ++j)
			m_rowBuf[j] = srcRow[i];
	}
	return &m_rowBuf[0];
}


//------------------------------------------------------------------------------
// CqImageChannel implementation
CqImageChannel::CqImageChannel(const SqChannelInfo& chanInfo, TqUchar* data,
		TqInt width, TqInt height, TqInt stride, TqInt rowSkip)
	: m_chanInfo(chanInfo),
	m_data(data),
	m_width(width),
	m_height(height),
	m_stride(stride),
	m_rowSkip(rowSkip),
	m_copyBuf(width)
{ }

void CqImageChannel::requireSize(TqInt width, TqInt height) const
{
	// Normal image channels cannot change size; just check that the sizes match.
	if(m_width != width || m_height != height)
	{
		AQSIS_THROW_DETAIL(XqInternal,
				"Image channel cannot produce required size", 
				"required size = " << width << "x" << height 
				<< "; actual size = " << m_width << "x" << m_height);
	}
}

void CqImageChannel::copyFrom(const IqImageChannelSource& source)
{
	source.requireSize(m_width, m_height);

	/** \todo make copying from the same type efficient...  Need also to take
	 * into account the min and max quantization parameters when they are
	 * implemented.
	 */

	for(TqInt row = 0; row < m_height; ++row)
		replaceRow(row, source.getRow(row));
}

void CqImageChannel::compositeOver(const IqImageChannelSource& source,
		const IqImageChannelSource& sourceAlpha)
{
	source.requireSize(m_width, m_height);
	sourceAlpha.requireSize(m_width, m_height);

	for(TqInt row = 0; row < m_height; ++row)
		compositeRow(row, source.getRow(row), sourceAlpha.getRow(row));
}

// Old stuff used for compositing 8-bit channels.  May possibly be useful for
// future optimizations.
#if 0

// Tricky macros from [Smith 1995] (see ref below)

/** Compute int((a/255.0)*b) with only integer arithmetic.  Assumes a, b are
 * between 0 and 255.
 */
#define INT_MULT(a,b,t) ( (t) = (a) * (b) + 0x80, ( ( ( (t)>>8 ) + (t) )>>8 ) )
/** Compute a composite of alpha-premultiplied values using integer arithmetic.
 *
 * Assumes p, q are between 0 and 255.
 *
 * \return int(q + (1-a/255.0)*p)
 */
#define INT_PRELERP(p, q, a, t) ( (p) + (q) - INT_MULT( a, p, t) )

/** \brief Composite two integers with a given alpha
 *
 * See for eg:
 * [Smith 1995]  Alvy Ray Smith, "Image Compositing Fundamentals", Technical
 * Memo 4, 1995.  ftp://ftp.alvyray.com/Acrobat/4_Comp.pdf
 *
 * \param r - top surface; alpha-premultiplied.  Assumed to be between 0 and 255.
 * \param R - bottom surface; alpha-premultiplied
 * \param alpha - alpha for top surface
 */
void CompositeAlpha(TqInt r, unsigned char &R, unsigned char alpha )
{ 
	TqInt t;
	TqInt R1 = static_cast<TqInt>(INT_PRELERP( R, r, alpha, t ));
	R = Aqsis::clamp( R1, 0, 255 );
}

#endif

//------------------------------------------------------------------------------
} // namespace Aqsis
