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
 * \brief Define several filter functors
 *
 * \author Chris Foster [ chris42f (at) gmail (dot) com ]
 */

#ifndef CACHEDFILTER_H_INCLUDED
#define CACHEDFILTER_H_INCLUDED

#include "aqsis.h"

#include <iosfwd>
#include <vector>

#include "aqsismath.h"
#include "filtersupport.h"
#include "maketexture.h"

namespace Aqsis
{

//------------------------------------------------------------------------------
/** \brief Cached filter weights for resampling an image.
 *
 * When resampling images for such purposes as mipmapping, the positions of
 * sample points are fixed relative to each output pixel.  This means it's
 * possible to cache the filter weights once and use them for each pixel.  This
 * is good, because many filter functions are rather costly to compute.
 */
class CqCachedFilter
{
	public:
		/** \brief Create a zeroed filter on a regular grid.
		 *
		 * The filter is cached on a regular lattice centered about (0,0).  If
		 * the filter contains an odd number of points in the x or y
		 * directions, the central filter value will be evaluated at (0,0).
		 * Otherwise the central filter values will straddle the origin.
		 *
		 * \param scale - Scale factor which will produce the new image
		 *                dimensions from the old ones.  For example, scale
		 *                should be 1/2 when downsampling by a factor of 2
		 *                during mipmapping.
		 */
		CqCachedFilter(const SqFilterInfo& filerInfo,
				bool includeZeroX, bool includeZeroY, TqFloat scale);

		/** \brief Get the cached filter weight.
		 *
		 * \param x - x-coordinate in the support [support().startX, support().endX-1]
		 * \param y - y-coordinate in the support [support().startY, support().endY-1]
		 */
		TqFloat operator()(TqInt x, TqInt y) const;

		/// Cached filters are normalized on construction; return true.
		static bool isNormalized() { return true; }

		/** \brief Get the number of points in the x-direction for the discrete
		 * filter kernel.
		 *
		 * Note that this is different from the floating point width provided
		 * to the constructor.
		 */
		TqInt width() const;
		/** \brief Get the number of points in the y-direction for the discrete
		 * filter kernel
		 *
		 * Note that this is different from the floating point height provided
		 * to the constructor.
		 */
		TqInt height() const;

		/** \brief Get the support for the filter in the source image.
		 *
		 * The support is the (rectangular) region over which the filter has
		 * nonzero coefficients.
		 */
		SqFilterSupport support() const;
		/// Set the top left point in the filter support
		void setSupportTopLeft(TqInt x, TqInt y);

	private:
		TqInt m_width; ///< number of points in horizontal lattice directon
		TqInt m_height; ///< number of points in vertical lattice directon
		TqInt m_topLeftX; ///< top left x-position in filter support
		TqInt m_topLeftY; ///< top left y-position in filter support
		std::vector<TqFloat> m_weights; ///< cached weights.
};

/// Stream insertion operator for printing a filter kernel
std::ostream& operator<<(std::ostream& out, const CqCachedFilter& filter);



//==============================================================================
// Implementation details
//==============================================================================
// CqCachedFilter

inline TqFloat CqCachedFilter::operator()(TqInt x, TqInt y) const
{
	return m_weights[(y-m_topLeftY)*m_width + (x-m_topLeftX)];
}

inline TqInt CqCachedFilter::width() const
{
	return m_width;
}

inline TqInt CqCachedFilter::height() const
{
	return m_height;
}

inline SqFilterSupport CqCachedFilter::support() const
{
	return SqFilterSupport(m_topLeftX, m_topLeftX+m_width,
			m_topLeftY, m_topLeftY+m_height);
}

inline void CqCachedFilter::setSupportTopLeft(TqInt x, TqInt y)
{
	m_topLeftX = x;
	m_topLeftY = y;
}

} // namespace Aqsis

#endif // CACHEDFILTER_H_INCLUDED
