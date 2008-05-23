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


/** \file
		\brief Declares classes required for the flexible performance timers.
		\author	Paul C. Gregory - originally based on code published on the CodeProject site
				http://www.codeproject.com/debug/multitimer.asp
*/

#include "aqsis.h"

#if USE_TIMERS

#include "multitimer_system.h"
#include "exception.h"

namespace Aqsis {

void CqHiFreqTimer::SqTimingDetails::Setup()
{
	perFreq = (double)CLOCKS_PER_SEC/1000.0;
	nextTimer = curBase = curManualBase = 0;
}

} // namespace Aqsis
#endif // USE_TIMERS
