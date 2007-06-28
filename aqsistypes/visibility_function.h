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
		\brief Declares the CqBucket class responsible for bookeeping the primitives and storing the results.
		\author Paul C. Gregory (pgregory@aqsis.org)
*/

#ifndef VISIBILITY_FUNCTION_H_INCLUDED
#define VISIBILITY_FUNCTION_H_INCLUDED 1

#include	<boost/shared_ptr.hpp>
#include	"aqsis.h"
#include	"color.h"

START_NAMESPACE( Aqsis )

//------------------------------------------------------------------------------
/** \brief Structure representing a visibility function node
 * This is a (depth, visibility) tuple as described by the Lokovic and Veach DSM paper
 * A visibility function is a collection of these structures.
 */
struct SqVisibilityNode
{
	TqFloat zdepth;
	/// an [additive] change in light color associated with a transmittance function "hit" at this zdepth
	CqColor visibility;
};

/// \todo Performance tuning: Consider alternatives to shared_ptr here.  Possibly intrusive_ptr, or holding the structures by value, with a memory pooling mechanism like SqImageSample
typedef std::vector< boost::shared_ptr<SqVisibilityNode> > TqVisibilityFunction;

END_NAMESPACE( Aqsis )

#endif /*VISIBILITY_FUNCTION_H_INCLUDED*/
