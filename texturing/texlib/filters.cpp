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

#include "filters.h"

namespace Aqsis
{

/// brief: Static factory method returns a boost pointer to instance of appropriate filter class  
boost::shared_ptr<IqTexFilter> IqTexFilter::filterOfType(std::string type)
{
	boost::shared_ptr<IqTexFilter> ref;

	if ( type == "box" )
		ref = boost::shared_ptr<CqTexBoxFilter>(new CqTexBoxFilter());
	else if ( type == "gaussian" )
		ref = boost::shared_ptr<CqTexGaussianFilter>(new CqTexGaussianFilter());
	else if ( type == "mitchell" )
		ref = boost::shared_ptr<CqTexMitchellFilter>(new CqTexMitchellFilter());
	else if ( type == "triangle" )
		ref = boost::shared_ptr<CqTexTriangleFilter>(new CqTexTriangleFilter());
	else if ( type == "catmull-rom" )
		ref = boost::shared_ptr<CqTexCatmullRomFilter>(new CqTexCatmullRomFilter());
	else if ( type == "sinc" )
		ref = boost::shared_ptr<CqTexSincFilter>(new CqTexSincFilter());
	else if ( type == "disk" )
		ref = boost::shared_ptr<CqTexDiskFilter>(new CqTexDiskFilter());
	else if ( type == "bessel" )
		ref = boost::shared_ptr<CqTexBesselFilter>(new CqTexBesselFilter());
	return ref; 
}

CqTexMitchellFilter::CqTexMitchellFilter() :
	B (0),
	C (0),
	invXWidth(0),
	invYWidth(0)
{}

CqTexMitchellFilter::CqTexMitchellFilter(TqFloat b, TqFloat c, TqFloat xw, TqFloat yw) :
	B (b),
	C (c),
	invXWidth(1.0f/xw),
	invYWidth(1.0f/yw)
{}

TqFloat	CqTexMitchellFilter::weight( TqFloat x, TqFloat y, TqFloat xwidth, TqFloat ywidth )
{
	B = 1/3.0f;
	C = 1/3.0f;
	invXWidth = 1.0f/xwidth;
	invYWidth= 1.0f/ywidth;
	
	return Evaluate(x, y);
}

TqFloat CqTexMitchellFilter::Evaluate(TqFloat x, TqFloat y) const 
{
   return Evaluate(x * invXWidth) * Evaluate(y * invYWidth);
}

TqFloat CqTexMitchellFilter::Evaluate(TqFloat x) const 
{
   x = fabsf(2.f * x);
   if (x > 1.f)
      return ((-B - 6*C) * x*x*x + (6*B + 30*C) * x*x +
      (-12*B - 48*C) * x + (8*B + 24*C)) * (1.f/6.f);
   else
      return ((12 - 9*B - 6*C) * x*x*x +
      (-18 + 12*B + 6*C) * x*x +
      (6 - 2*B)) * (1.f/6.f);
}

//----------------------------------------------------------------------
// TexGaussianFilter
// Gaussian filter used as a possible value passed to TexPixelFilter.
//
TqFloat	CqTexGaussianFilter::weight( TqFloat x, TqFloat y, TqFloat xwidth, TqFloat ywidth )
{
	x /= xwidth;
	y /= ywidth;

	return exp( -8.0 * ( x * x + y * y ) );
}

//----------------------------------------------------------------------
// TexBoxFilter
// Box filter used as a possible value passed to TexPixelFIlter.
//
TqFloat	CqTexBoxFilter::weight( TqFloat x, TqFloat y, TqFloat xwidth, TqFloat ywidth )
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
TqFloat	CqTexTriangleFilter::weight( TqFloat x, TqFloat y, TqFloat xwidth, TqFloat ywidth )
{
	TqFloat	hxw = xwidth / 2.0;
	TqFloat	hyw = ywidth / 2.0;
	TqFloat	absx = fabs( x );
	TqFloat	absy = fabs( y );

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
TqFloat	CqTexCatmullRomFilter::weight( TqFloat x, TqFloat y, TqFloat xwidth, TqFloat ywidth )
{
	/* RI SPec 3.2 */
	TqFloat r2 = (x*x+y*y);
	TqFloat r = sqrt(r2);
	return (r>=2.0)?0.0:
	       (r<1.0)?(3.0*r*r2-5.0*r2+2.0):(-r*r2+5.0*r2-8.0*r+4.0);
}


//----------------------------------------------------------------------
// TexSincFilter
// Sinc filter used as a possible value passed to TexPixelFilter.
//
TqFloat	CqTexSincFilter::weight( TqFloat x, TqFloat y, TqFloat xwidth, TqFloat ywidth )
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
TqFloat	CqTexDiskFilter::weight( TqFloat x, TqFloat y, TqFloat xwidth, TqFloat ywidth )
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
TqFloat	CqTexBesselFilter::weight( TqFloat x, TqFloat y, TqFloat xwidth, TqFloat ywidth )
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
