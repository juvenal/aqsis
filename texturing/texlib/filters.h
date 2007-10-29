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
 * \brief Implement the filters of the RenderMan API functions.
 *		These are simply renamed copies of the filters in aqsislib.
 *		It was necessary to duplicate the code in order to avoid circular
 *		dependencies between our libraries.
*/

#ifndef TEXFILTERS_H_INCLUDED
#define TEXFILTERS_H_INCLUDED

#include "aqsis.h"
#include <string>
#include <math.h>
#include "aqsismath.h"

// External libraries
#include <boost/shared_ptr.hpp>

namespace Aqsis
{

#define	RI_PI 3.14159265359f
#define	RI_PIO2 RI_PI/2

//---------------------------------------------------------------------
/** \class IqTexFilter
 * An interface for textrue filtering classes, also serves as factory for various filter classes.
 */
struct IqTexFilter
{
	virtual ~IqTexFilter(){};
		
	virtual TqFloat weight(TqFloat x, TqFloat y, TqFloat width, TqFloat height) = 0;
	
	static boost::shared_ptr<IqTexFilter> filterOfType(std::string type);
};

// Mitchell Filter Declarations
class CqTexMitchellFilter : public IqTexFilter
{
	public:	
	   // CqMitchellFilter Public Methods
		CqTexMitchellFilter();
		CqTexMitchellFilter(TqFloat b, TqFloat c, TqFloat xw, TqFloat yw);
	   
	   TqFloat weight(TqFloat x, TqFloat y, TqFloat width, TqFloat height);
	   TqFloat Evaluate(TqFloat x, TqFloat y) const;
	   TqFloat Evaluate(TqFloat x) const;
	private:
		TqFloat B, C;
		TqFloat invXWidth, invYWidth;
};

class CqTexGaussianFilter : public IqTexFilter
{
	public:
		CqTexGaussianFilter(){};
	
		TqFloat weight(TqFloat x, TqFloat y, TqFloat width, TqFloat height);
};

class CqTexBesselFilter : public IqTexFilter
{
	public:
		CqTexBesselFilter(){};
	
		TqFloat weight(TqFloat x, TqFloat y, TqFloat width, TqFloat height);
};

class CqTexBoxFilter : public IqTexFilter
{
	public:
		CqTexBoxFilter(){};
	
		TqFloat weight(TqFloat x, TqFloat y, TqFloat width, TqFloat height);
};

class CqTexTriangleFilter : public IqTexFilter
{
	public:
		CqTexTriangleFilter(){};
	
		TqFloat weight(TqFloat x, TqFloat y, TqFloat width, TqFloat height);
};

class CqTexCatmullRomFilter : public IqTexFilter
{
	public:
		CqTexCatmullRomFilter(){};
	
		TqFloat weight(TqFloat x, TqFloat y, TqFloat width, TqFloat height);
};

class CqTexSincFilter : public IqTexFilter
{
	public:
		CqTexSincFilter(){};
	
		TqFloat weight(TqFloat x, TqFloat y, TqFloat width, TqFloat height);
};

class CqTexDiskFilter : public IqTexFilter
{
	public:
		CqTexDiskFilter(){};
	
		TqFloat weight(TqFloat x, TqFloat y, TqFloat width, TqFloat height);
};

//------------------------------------------------------------------------------
} // namespace Aqsis

#endif // TEXFILTERS_H_INCLUDED
