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

namespace Aqsis
{

#define	RI_PI 3.14159265359f
#define	RI_PIO2 RI_PI/2

class CqTexFilter
{
	public:
		CqTexFilter();
		CqTexFilter(std::string type);
		
		TqFloat weight(TqFloat x, TqFloat y, TqFloat width, TqFloat height);
		
		void setType(std::string type);
	
	private:
		
		// Mitchell Filter Declarations
		class CqTexMitchellFilter 
		{
			public:	
			   // CqMitchellFilter Public Methods
			   CqTexMitchellFilter(TqFloat b, TqFloat c, TqFloat xw, TqFloat yw)
			   {
			      B = b;
			      C = c;
			      invXWidth = 1.0f/xw;
			      invYWidth = 1.0f/yw;
			   }
			   TqFloat Evaluate(TqFloat x, TqFloat y) const {
			      return Evaluate(x * invXWidth) * Evaluate(y * invYWidth);
			   }
			   TqFloat Evaluate(TqFloat x) const {
			      x = fabsf(2.f * x);
			      if (x > 1.f)
			         return ((-B - 6*C) * x*x*x + (6*B + 30*C) * x*x +
			         (-12*B - 48*C) * x + (8*B + 24*C)) * (1.f/6.f);
			      else
			         return ((12 - 9*B - 6*C) * x*x*x +
			         (-18 + 12*B + 6*C) * x*x +
			         (6 - 2*B)) * (1.f/6.f);
			   }
			private:
				TqFloat B, C;
				TqFloat invXWidth, invYWidth;
		};
		
		// Functions 
		TqFloat TexGaussianFilter( TqFloat x, TqFloat y, TqFloat xwidth, TqFloat ywidth );

		TqFloat	TexMitchellFilter( TqFloat x, TqFloat y, TqFloat xwidth, TqFloat ywidth );
		
		TqFloat TexBesselFilter( TqFloat x, TqFloat y, TqFloat xwidth, TqFloat ywidth );

		TqFloat	TexBoxFilter( TqFloat x, TqFloat y, TqFloat xwidth, TqFloat ywidth );

		TqFloat	TexTriangleFilter( TqFloat x, TqFloat y, TqFloat xwidth, TqFloat ywidth );

		TqFloat	TexCatmullRomFilter( TqFloat x, TqFloat y, TqFloat xwidth, TqFloat ywidth );

		TqFloat	TexSincFilter( TqFloat x, TqFloat y, TqFloat xwidth, TqFloat ywidth );

		TqFloat	TexDiskFilter( TqFloat x, TqFloat y, TqFloat xwidth, TqFloat ywidth );
		
		// Data
		TqFloat (CqTexFilter::*m_filterFunc)(TqFloat, TqFloat, TqFloat, TqFloat);
		
};

//------------------------------------------------------------------------------
} // namespace Aqsis

#endif // TEXFILTERS_H_INCLUDED
