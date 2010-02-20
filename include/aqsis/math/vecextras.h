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
 * \brief Forward declarations for short-vector types.
 *
 * \author Chris Foster [ chris42f (at) gmail (dot) com ]
 */

#ifndef VECEXTRAS_H_INCLUDED
#define VECEXTRAS_H_INCLUDED

#include <aqsis/aqsis.h>

#include <ImathVec.h>

#include <aqsis/math/math.h>

namespace Aqsis
{



//-----------------------------------------------------------------------
// Vec2<T> helpers

template <class T> inline Imath::Vec2<T> min(const Imath::Vec2<T>& v1, const Imath::Vec2<T>& v2)
{
	return Imath::Vec2<T>(min(v1.x, v2.x), min(v1.y, v2.y));
}

template <class T> inline Imath::Vec2<T> max(const Imath::Vec2<T>& v1, const Imath::Vec2<T>& v2)
{
	return Imath::Vec2<T>(max(v1.x, v2.x), max(v1.y, v2.y));
}

template <class T> inline Imath::Vec2<T> compMul(const Imath::Vec2<T>& trans, const Imath::Vec2<T>& v)
{
	return Imath::Vec2<T>(trans.x*v.x, trans.y*v.y);
}

template <class T> inline T maxNorm(Imath::Vec2<T> v)
{
	return max(std::fabs(v.x), std::fabs(v.y));
}

template <class T> inline bool isClose(const Imath::Vec2<T>& v1, const Imath::Vec2<T>& v2, T tol = 10*std::numeric_limits<TqFloat>::epsilon())
{
	T diff2 = (v1 - v2).length2();
	T tol2 = tol*tol;
	return diff2 <= tol2*v1.length2() || diff2 <= tol2*v2.length2();
}

//-----------------------------------------------------------------------
// Vec3<T> helpers

template <class T> inline bool operator<(const Imath::Vec3<T>& v1, const Imath::Vec3<T>& v2)
{
	return (v1.x < v2.x) && (v1.y < v2.y) && (v1.z < v2.z);
}

template <class T> inline bool operator<=(const Imath::Vec3<T>& v1, const Imath::Vec3<T>& v2)
{
	return (v1.x <= v2.x) && (v1.y <= v2.y) && (v1.z <= v2.z);
}

template <class T> inline bool operator>(const Imath::Vec3<T>& v1, const Imath::Vec3<T>& v2)
{
	return (v1.x > v2.x) && (v1.y > v2.y) && (v1.z > v2.z);
}

template <class T> inline bool operator>=(const Imath::Vec3<T>& v1, const Imath::Vec3<T>& v2)
{
	return (v1.x >= v2.x) && (v1.y >= v2.y) && (v1.z >= v2.z);
}

template <class T> inline Imath::Vec3<T> operator/(T f, const Imath::Vec3<T>& v)
{
	return Imath::Vec3<T>(f/v.x, f/v.y, f/v.z);
}
	
template <class T> inline Imath::Vec3<T> operator+(T f, const Imath::Vec3<T>& v)
{
	return Imath::Vec3<T>(f+v.x, f+v.y, f+v.z);
}

template <class T> inline Imath::Vec3<T> operator-(T f, const Imath::Vec3<T>& v)
{
	return Imath::Vec3<T>(f-v.x, f-v.y, f-v.z);
}


template <class T> inline Imath::Vec3<T> min(const Imath::Vec3<T>& a, const Imath::Vec3<T>& b)
{
	return Imath::Vec3<T>(min(a.x, b.x), min(a.y, b.y), min(a.z, b.z));
}

template <class T> inline Imath::Vec3<T> max(const Imath::Vec3<T>& a, const Imath::Vec3<T>& b)
{
	return Imath::Vec3<T>(max(a.x, b.x), max(a.y, b.y), max(a.z, b.z));
}

template <class T> inline Imath::Vec3<T> clamp(const Imath::Vec3<T>& v, const Imath::Vec3<T>& min,
		const Imath::Vec3<T>& max)
{
	return Imath::Vec3<T>(clamp(v.x, min.x, max.x),
			clamp(v.y, min.y, max.y),
			clamp(v.z, min.z, max.z));
}

template <class T> inline Imath::Vec3<T> lerp(T t, const Imath::Vec3<T>& v0, const Imath::Vec3<T>& v1)
{
	return Imath::Vec3<T>((1-t)*v0.x + t*v1.x,
					  (1-t)*v0.y + t*v1.y,
					  (1-t)*v0.z + t*v1.z);
}

template <class T> inline bool isClose(const Imath::Vec3<T>& v1, const Imath::Vec3<T>& v2, T tol = 10*std::numeric_limits<TqFloat>::epsilon())
{
	T diff2 = (v1 - v2).length2();
	T tol2 = tol*tol;
	return diff2 <= tol2*v1.length2() || diff2 <= tol2*v2.length2();
}

} // namespace Aqsis

#endif // VECFWD_H_INCLUDED
