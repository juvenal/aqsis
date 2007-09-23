// Aqsis
// Copyright 1997 - 2001, Paul C. Gregory
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
		\brief Implement the filters of the RenderMan API functions.
		These are simply renamed copies of the filters in aqsislib.
		It was necessary to duplicate the code in order to avoid circular
		dependencies between our libraries.
		\author Paul C. Gregory (pgregory@aqsis.org)
*/

#include	<stdarg.h>
#include	<math.h>
#include	<list>

#include "filters.h"

namespace Aqsis
{

CqTexFilter::CqTexFilter() :
	m_filterFunc(NULL)
{
	setType("box");
}

CqTexFilter::CqTexFilter(std::string type)
{
	setType(type);
}

RtFloat CqTexFilter::weight(RtFloat x, RtFloat y, RtFloat width, RtFloat height)
{
	return (*this.*m_filterFunc)(x, y, width, height);
}

void CqTexFilter::setType(std::string type)
{
	m_filterFunc = &Aqsis::CqTexFilter::TexBoxFilter; // default

	if ( type == "box" )
		return;
	else if ( type == "gaussian" )
		m_filterFunc = &Aqsis::CqTexFilter::TexGaussianFilter;
	else if ( type == "mitchell" )
		m_filterFunc = &Aqsis::CqTexFilter::TexMitchellFilter;
	else if ( type == "triangle" )
		m_filterFunc = &Aqsis::CqTexFilter::TexTriangleFilter;
	else if ( type == "catmull-rom" )
		m_filterFunc = &Aqsis::CqTexFilter::TexCatmullRomFilter;
	else if ( type == "sinc" )
		m_filterFunc = &Aqsis::CqTexFilter::TexSincFilter;
	else if ( type == "disk" )
		m_filterFunc = &Aqsis::CqTexFilter::TexDiskFilter;
	else if ( type == "bessel" )
		m_filterFunc = &Aqsis::CqTexFilter::TexBesselFilter;
}

//----------------------------------------------------------------------
// TexGaussianFilter
// Gaussian filter used as a possible value passed to TexPixelFilter.
//
RtFloat	CqTexFilter::TexGaussianFilter( RtFloat x, RtFloat y, RtFloat xwidth, RtFloat ywidth )
{
	x /= xwidth;
	y /= ywidth;

	return exp( -8.0 * ( x * x + y * y ) );
}

//----------------------------------------------------------------------
// TexMitchellFilter
// Mitchell filter used as a possible value passed to TexPixelFIlter.
//
RtFloat	CqTexFilter::TexMitchellFilter( RtFloat x, RtFloat y, RtFloat xwidth, RtFloat ywidth )
{
	CqTexMitchellFilter mc(1/3.0f, 1/3.0f, xwidth, ywidth);

	return mc.Evaluate(x, y);
}

//----------------------------------------------------------------------
// TexBoxFilter
// Box filter used as a possible value passed to TexPixelFIlter.
//
RtFloat	CqTexFilter::TexBoxFilter( RtFloat x, RtFloat y, RtFloat xwidth, RtFloat ywidth )
{
	/* [UPST89] -- (RC p. 178) says that x and y will be in the
	 *    following intervals:
	 *           -xwidth/2 <= x <= xwidth/2
	 *           -ywidth/2 <= y <= ywidth/2
	 *    These constraints on x and y really simplifies the
	 *       the following code to just return (1.0).
	 *
	 */
	return min( ( fabs( x ) <= xwidth / 2.0 ? 1.0 : 0.0 ),
	            ( fabs( y ) <= ywidth / 2.0 ? 1.0 : 0.0 ) );
}

//----------------------------------------------------------------------
// TexTriangleFilter
// Triangle filter used as a possible value passed to TexPixelFilter
//
RtFloat	CqTexFilter::TexTriangleFilter( RtFloat x, RtFloat y, RtFloat xwidth, RtFloat ywidth )
{
	RtFloat	hxw = xwidth / 2.0;
	RtFloat	hyw = ywidth / 2.0;
	RtFloat	absx = fabs( x );
	RtFloat	absy = fabs( y );

	/* This function can be simplified as well by not worrying about
	 *    returning zero if the sample is beyond the filter window.
	 */
	return min( ( absx <= hxw ? ( hxw - absx ) / hxw : 0.0 ),
	            ( absy <= hyw ? ( hyw - absy ) / hyw : 0.0 ) );
}


//----------------------------------------------------------------------
// TexCatmullRomFilter
// Catmull Rom filter used as a possible value passed to TexPixelFilter.
//
RtFloat	CqTexFilter::TexCatmullRomFilter( RtFloat x, RtFloat y, RtFloat xwidth, RtFloat ywidth )
{
	/* RI SPec 3.2 */
	RtFloat r2 = (x*x+y*y);
	RtFloat r = sqrt(r2);
	return (r>=2.0)?0.0:
	       (r<1.0)?(3.0*r*r2-5.0*r2+2.0):(-r*r2+5.0*r2-8.0*r+4.0);
}


//----------------------------------------------------------------------
// TexSincFilter
// Sinc filter used as a possible value passed to TexPixelFilter.
//
RtFloat	CqTexFilter::TexSincFilter( RtFloat x, RtFloat y, RtFloat xwidth, RtFloat ywidth )
{
	/* Uses a -PI to PI cosine window. */
	if ( x != 0.0 )
	{
		x *= RI_PI;
		x = cos( 0.5 * x / xwidth ) * sin( x ) / x;
	}
	else
	{
		x = 1.0;
	}
	if ( y != 0.0 )
	{
		y *= RI_PI;
		y = cos( 0.5 * y / ywidth ) * sin( y ) / y;
	}
	else
	{
		y = 1.0;
	}

	/* This is a square separable filter and is the 2D Fourier
	 * transform of a rectangular box outlining a lowpass bandwidth
	* filter in the frequency domain.
	*/
	return x*y;
}


//----------------------------------------------------------------------
// TexDiskFilter -- this is in Pixar's ri.h
// Cylindrical filter used as a possible value passed to TexPixelFilter
//
RtFloat	CqTexFilter::TexDiskFilter( RtFloat x, RtFloat y, RtFloat xwidth, RtFloat ywidth )
{
	double d, xx, yy;

	xx = x * x;
	yy = y * y;
	xwidth *= 0.5;
	ywidth *= 0.5;

	d = ( xx ) / ( xwidth * xwidth ) + ( yy ) / ( ywidth * ywidth );
	if ( d < 1.0 )
	{
		return 1.0;
	}
	else
	{
		return 0.0;
	}
}


//----------------------------------------------------------------------
// TexBesselFilter -- this is in Pixar's ri.h
// Besselj0 filter used as a possible value passed to TexPixelFilter
//
RtFloat	CqTexFilter::TexBesselFilter( RtFloat x, RtFloat y, RtFloat xwidth, RtFloat ywidth )
{

	double d, w, xx, yy;

	xx = x * x;
	yy = y * y;

	xwidth *= 0.5;
	ywidth *= 0.5;

	w = ( xx ) / ( xwidth * xwidth ) + ( yy ) / ( ywidth * ywidth );
	if ( w < 1.0 )
	{
		d = sqrt( xx + yy );
		if ( d != 0.0 )
		{
			/* Half cosine window. */
			w = cos( 0.5 * RI_PI * sqrt( w ) );
			return w * 2*j1( RI_PI * d ) / d;
		}
		else
		{
			return RI_PI;
		}
	}
	else
	{
		return 0.0;
	}
}

//------------------------------------------------------------------------------
} // namespace Aqsis
