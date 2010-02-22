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
 *
 * \brief Declares the Imath::Color3f class for handling generic 3 element colors.
 * \author Paul C. Gregory (pgregory@aqsis.org)
 */

#ifndef COLOREXTRAS_H_INCLUDED
#define COLOREXTRAS_H_INCLUDED

#include <aqsis/aqsis.h>

#include <ImathColor.h>

namespace Aqsis {

template<class T> Imath::Color3<T>  rgb2hsl(const Imath::Color3<T> &rgb)
{
	return rgb;
}

template<class T> Imath::Color3<T>  hsl2rgb(const Imath::Color3<T> &hsl)
{
	return hsl;
}

template<class T> Imath::Color3<T>  rgb2XYZ(const Imath::Color3<T> &rgb)
{
	return rgb;
}

template<class T> Imath::Color3<T>  XYZ2rgb(const Imath::Color3<T> &XYZ)
{
	return XYZ;
}

template<class T> Imath::Color3<T>  rgb2xyY(const Imath::Color3<T> &rgb)
{
	return rgb;
}

template<class T> Imath::Color3<T>  xyY2rgb(const Imath::Color3<T> &xyY)
{
	return xyY;
}

template<class T> Imath::Color3<T>  rgb2YIQ(const Imath::Color3<T> &rgb)
{
	return rgb;
}

template<class T> Imath::Color3<T>  YIQ2rgb(const Imath::Color3<T> &YIQ)
{
	return YIQ;
}

} // namespace Aqsis

#endif

