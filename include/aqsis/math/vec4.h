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
		\brief Declares a 4D vector class which encapsulates a homogenous 4D vector.
		This class is based on the equivalent 2D and 3D versions from IlmBase.
		\author Paul C. Gregory (pgregory@aqsis.org)
*/



#ifndef INCLUDED_VEC4_H
#define INCLUDED_VEC4_H

//----------------------------------------------------
//
//	4D point/vector class template!
//
//----------------------------------------------------

#include <aqsis/math/math.h>

#include "ImathExc.h"
#include "ImathLimits.h"
#include "ImathMath.h"

#include <iostream>

#if (defined _WIN32 || defined _WIN64) && defined _MSC_VER
// suppress exception specification warnings
#pragma warning(disable:4290)
#endif


namespace Aqsis {


template <class T> class Vec4
{
  public:

    //-------------------
    // Access to elements
    //-------------------

    T			x, y, z, h;

    T &			operator [] (int i);
    const T &		operator [] (int i) const;


    //-------------
    // Constructors
    //-------------

    Vec4 ();                        // no initialization
    explicit Vec4 (T a);            // (a a)
    Vec4 (T a, T b, T c, T d = 1.0f);      // (a b c d)


    //---------------------------------
    // Copy constructors and assignment
    //---------------------------------

    Vec4 (const Vec4 &v);
    template <class S> Vec4 (const Vec4<S> &v);

    const Vec4 &	operator = (const Vec4 &v);


    //----------------------
    // Compatibility with Sb
    //----------------------

    template <class S>
    void		setValue (S a, S b, S c, S d);

    template <class S>
    void		setValue (const Vec4<S> &v);

    template <class S>
    void		getValue (S &a, S &b, S &c, S &d) const;

    template <class S>
    void		getValue (Vec4<S> &v) const;

    T *			getValue ();
    const T *		getValue () const;

    
    //---------
    // Equality
    //---------

    template <class S>
    bool		operator == (const Vec4<S> &v) const;

    template <class S>
    bool		operator != (const Vec4<S> &v) const;


    //-----------------------------------------------------------------------
    // Compare two vectors and test if they are "approximately equal":
    //
    // equalWithAbsError (v, e)
    //
    //	    Returns true if the coefficients of this and v are the same with
    //	    an absolute error of no more than e, i.e., for all i
    //
    //      abs (this[i] - v[i]) <= e
    //
    // equalWithRelError (v, e)
    //
    //	    Returns true if the coefficients of this and v are the same with
    //	    a relative error of no more than e, i.e., for all i
    //
    //      abs (this[i] - v[i]) <= e * abs (this[i])
    //-----------------------------------------------------------------------

    bool		equalWithAbsError (const Vec4<T> &v, T e) const;
    bool		equalWithRelError (const Vec4<T> &v, T e) const;

    //------------
    // Dot product
    //------------

    T			dot (const Vec4 &v) const;
    T			operator ^ (const Vec4 &v) const;


    //---------------------------
    // Right-handed cross product
    //---------------------------

    Vec4<T>		cross (const Vec4 &v) const;
    Vec4<T>		operator % (const Vec4 &v) const;


    //------------------------
    // Component-wise addition
    //------------------------

    const Vec4 &	operator += (const Vec4 &v);
    Vec4		operator + (const Vec4 &v) const;


    //---------------------------
    // Component-wise subtraction
    //---------------------------

    const Vec4 &	operator -= (const Vec4 &v);
    Vec4		operator - (const Vec4 &v) const;


    //------------------------------------
    // Component-wise multiplication by -1
    //------------------------------------

    Vec4		operator - () const;
    const Vec4 &	negate ();


    //------------------------------
    // Component-wise multiplication
    //------------------------------

    const Vec4 &	operator *= (const Vec4 &v);
    const Vec4 &	operator *= (T a);
    Vec4		operator * (const Vec4 &v) const;
    Vec4		operator * (T a) const;


    //------------------------
    // Component-wise division
    //------------------------

    const Vec4 &	operator /= (const Vec4 &v);
    const Vec4 &	operator /= (T a);
    Vec4		operator / (const Vec4 &v) const;
    Vec4		operator / (T a) const;


    //----------------------------------------------------------------
    // Length and normalization:  If v.length() is 0.0, v.normalize()
    // and v.normalized() produce a null vector; v.normalizeExc() and
    // v.normalizedExc() throw a NullVecExc.
    // v.normalizeNonNull() and v.normalizedNonNull() are slightly
    // faster than the other normalization routines, but if v.length()
    // is 0.0, the result is undefined.
    //----------------------------------------------------------------

    T			length () const;
    T			length2 () const;

    const Vec4 &	normalize ();           // modifies *this
    const Vec4 &	normalizeExc () throw (Iex::MathExc);
    const Vec4 &	normalizeNonNull ();

    Vec4<T>		normalized () const;	// does not modify *this
    Vec4<T>		normalizedExc () const throw (Iex::MathExc);
    Vec4<T>		normalizedNonNull () const;

	const Vec4 &	homogenize();			// modifies *this
	Vec4<T>			homogenized() const;	// does not modify *this

    //--------------------------------------------------------
    // Number of dimensions, i.e. number of elements in a Vec2
    //--------------------------------------------------------

    static unsigned int	dimensions() {return 2;}


    //-------------------------------------------------
    // Limitations of type T (see also class limits<T>)
    //-------------------------------------------------

    static T		baseTypeMin()		{return Imath::limits<T>::min();}
    static T		baseTypeMax()		{return Imath::limits<T>::max();}
    static T		baseTypeSmallest()	{return Imath::limits<T>::smallest();}
    static T		baseTypeEpsilon()	{return Imath::limits<T>::epsilon();}


    //--------------------------------------------------------------
    // Base type -- in templates, which accept a parameter, V, which
    // could be either a Vec2<T> or a Vec3<T>, you can refer to T as
    // V::BaseType
    //--------------------------------------------------------------

    typedef T		BaseType;
};


//--------------
// Stream output
//--------------

template <class T>
std::ostream &	operator << (std::ostream &s, const Vec4<T> &v);


//----------------------------------------------------
// Reverse multiplication: S * Vec4<T>
//----------------------------------------------------

template <class T> Vec4<T>	operator * (T a, const Vec4<T> &v);


//-------------------------
// Typedefs for convenience
//-------------------------

typedef Vec4 <short>  V4s;
typedef Vec4 <int>    V4i;
typedef Vec4 <float>  V4f;
typedef Vec4 <double> V4d;


//-------------------------------------------------------------------
// Specializations for Vec4<short>, Vec4<int>
//-------------------------------------------------------------------

// Vec4<short>

template <> short
Vec4<short>::length () const;

template <> const Vec4<short> &
Vec4<short>::normalize ();

template <> const Vec4<short> &
Vec4<short>::normalizeExc () throw (Iex::MathExc);

template <> const Vec4<short> &
Vec4<short>::normalizeNonNull ();

template <> Vec4<short>
Vec4<short>::normalized () const;

template <> Vec4<short>
Vec4<short>::normalizedExc () const throw (Iex::MathExc);

template <> Vec4<short>
Vec4<short>::normalizedNonNull () const;


// Vec4<int>

template <> int
Vec4<int>::length () const;

template <> const Vec4<int> &
Vec4<int>::normalize ();

template <> const Vec4<int> &
Vec4<int>::normalizeExc () throw (Iex::MathExc);

template <> const Vec4<int> &
Vec4<int>::normalizeNonNull ();

template <> Vec4<int>
Vec4<int>::normalized () const;

template <> Vec4<int>
Vec4<int>::normalizedExc () const throw (Iex::MathExc);

template <> Vec4<int>
Vec4<int>::normalizedNonNull () const;


//------------------------
// Implementation of Vec4:
//------------------------

template <class T>
inline T &
Vec4<T>::operator [] (int i)
{
    return (&x)[i];
}

template <class T>
inline const T &
Vec4<T>::operator [] (int i) const
{
    return (&x)[i];
}

template <class T>
inline
Vec4<T>::Vec4 ()
{
    // empty
}

template <class T>
inline
Vec4<T>::Vec4 (T a)
{
    x = y = z = a;
	h = T(1);
}

template <class T>
inline
Vec4<T>::Vec4 (T a, T b, T c, T d)
{
    x = a;
    y = b;
	z = c;
	h = d;
}

template <class T>
inline
Vec4<T>::Vec4 (const Vec4 &v)
{
    x = v.x;
    y = v.y;
	z = v.z;
	h = v.h;
}

template <class T>
template <class S>
inline
Vec4<T>::Vec4 (const Vec4<S> &v)
{
    x = T (v.x);
    y = T (v.y);
    z = T (v.z);
    h = T (v.h);
}

template <class T>
inline const Vec4<T> &
Vec4<T>::operator = (const Vec4 &v)
{
    x = v.x;
    y = v.y;
    z = v.z;
    h = v.h;
    return *this;
}

template <class T>
template <class S>
inline void
Vec4<T>::setValue (S a, S b, S c, S d)
{
    x = T (a);
    y = T (b);
    z = T (c);
    h = T (d);
}

template <class T>
template <class S>
inline void
Vec4<T>::setValue (const Vec4<S> &v)
{
    x = T (v.x);
    y = T (v.y);
    z = T (v.z);
    h = T (v.h);
}

template <class T>
template <class S>
inline void
Vec4<T>::getValue (S &a, S &b, S &c, S &d) const
{
    a = S (x);
    b = S (y);
    c = S (z);
    d = S (h);
}

template <class T>
template <class S>
inline void
Vec4<T>::getValue (Vec4<S> &v) const
{
    v.x = S (x);
    v.y = S (y);
    v.z = S (z);
    v.h = S (h);
}

template <class T>
inline T *
Vec4<T>::getValue()
{
    return (T *) &x;
}

template <class T>
inline const T *
Vec4<T>::getValue() const
{
    return (const T *) &x;
}

template <class T>
template <class S>
inline bool
Vec4<T>::operator == (const Vec4<S> &v) const
{
    return x == v.x && y == v.y && z == v.z && h == v.h;
}

template <class T>
template <class S>
inline bool
Vec4<T>::operator != (const Vec4<S> &v) const
{
    return x != v.x || y != v.y || z != v.z || h != v.h;
}

template <class T>
bool
Vec4<T>::equalWithAbsError (const Vec4<T> &v, T e) const
{
    for (int i = 0; i < 4; i++)
	if (!Imath::equalWithAbsError ((*this)[i], v[i], e))
	    return false;

    return true;
}

template <class T>
bool
Vec4<T>::equalWithRelError (const Vec4<T> &v, T e) const
{
    for (int i = 0; i < 4; i++)
	if (!Imath::equalWithRelError ((*this)[i], v[i], e))
	    return false;

    return true;
}

template <class T>
inline T
Vec4<T>::dot (const Vec4 &v) const
{
	Vec4<T> a(*this);
	Vec4<T> b(v);

	a.homogenize();
	b.homogenize();

    return a.x * b.x + a.y * b.y + a.z * b.z;
}

template <class T>
inline T
Vec4<T>::operator ^ (const Vec4 &v) const
{
    return dot (v);
}

template <class T>
inline Vec4<T>
Vec4<T>::cross (const Vec4 &v) const
{
	Vec4<T>	a(*this);
	Vec4<T>	b(v);

	a.homogenize();
	b.homogenize();

    return Vec4 (a.y * b.z - a.z * b.y,
				 a.z * b.x - a.x * b.z,
				 a.x * b.y - a.y * b.x);
}

template <class T>
inline Vec4<T>
Vec4<T>::operator % (const Vec4 &v) const
{
    return cross(v);
}

template <class T>
inline const Vec4<T> &
Vec4<T>::operator += (const Vec4 &v)
{
	T hom = h / v.h;

    x += v.x * hom;
    y += v.y * hom;
    z += v.z * hom;
    return *this;
}

template <class T>
inline Vec4<T>
Vec4<T>::operator + (const Vec4 &v) const
{
	Vec4<T> r(*this);
	return r += v;
}

template <class T>
inline const Vec4<T> &
Vec4<T>::operator -= (const Vec4 &v)
{
	T hom = h / v.h;

    x -= v.x * hom;
    y -= v.y * hom;
    z -= v.z * hom;
    return *this;
}

template <class T>
inline Vec4<T>
Vec4<T>::operator - (const Vec4 &v) const
{
	Vec4<T> r(*this);
    return r -= v;
}

template <class T>
inline Vec4<T>
Vec4<T>::operator - () const
{
    return Vec4 (-x, -y, -z, h);
}

template <class T>
inline const Vec4<T> &
Vec4<T>::negate ()
{
    x = -x;
    y = -y;
    z = -z;
    return *this;
}

template <class T>
inline const Vec4<T> &
Vec4<T>::operator *= (const Vec4 &v)
{
	T hom = h / v.h;

	// TODO: Validate this.
    x *= v.x * hom;
    y *= v.y * hom;
    z *= v.z * hom;
    return *this;
}

template <class T>
inline const Vec4<T> &
Vec4<T>::operator *= (T a)
{
    x *= a;
    y *= a;
    z *= a;
    return *this;
}

template <class T>
inline Vec4<T>
Vec4<T>::operator * (const Vec4 &v) const
{
	Vec4<T> r(*this);
    return r *= v;
}

template <class T>
inline Vec4<T>
Vec4<T>::operator * (T a) const
{
	Vec4<T> r(*this);
    return r *= a;
}

template <class T>
inline const Vec4<T> &
Vec4<T>::operator /= (const Vec4 &v)
{
	T hom = h / v.h;

	// TODO: Validate this
    x /= v.x * hom;
    y /= v.y * hom;
    z /= v.z * hom;
    return *this;
}

template <class T>
inline const Vec4<T> &
Vec4<T>::operator /= (T a)
{
    h *= a;
    return *this;
}

template <class T>
inline Vec4<T>
Vec4<T>::operator / (const Vec4 &v) const
{
	Vec4<T> r(*this);
    return r /= v;
}

template <class T>
inline Vec4<T>
Vec4<T>::operator / (T a) const
{
	Vec4<T> r(*this);
    return r /= a;
}

template <class T>
inline T
Vec4<T>::length () const
{
    return Imath::Math<T>::sqrt (dot(*this));
}

template <class T>
inline T
Vec4<T>::length2 () const
{
    return dot (*this);
}

template <class T>
const Vec4<T> &
Vec4<T>::normalize ()
{
    T l = length();
	if(l != 0)
		h = l;

    return *this;
}

template <class T>
const Vec4<T> &
Vec4<T>::normalizeExc () throw (Iex::MathExc)
{
    T l = length();

    if (l == 0)
	throw Imath::NullVecExc ("Cannot normalize null vector.");

	h = l;
    return *this;
}

template <class T>
inline
const Vec4<T> &
Vec4<T>::normalizeNonNull ()
{
    h = length();
    return *this;
}

template <class T>
Vec4<T>
Vec4<T>::normalized () const
{
    T l = length();

    if (l == 0)
	return Vec4 (T (0));

    return Vec4 (x, y, z, l);
}

template <class T>
Vec4<T>
Vec4<T>::normalizedExc () const throw (Iex::MathExc)
{
    T l = length();

    if (l == 0)
	throw Imath::NullVecExc ("Cannot normalize null vector.");

    return Vec4 (x, y, z, l);
}

template <class T>
inline
Vec4<T>
Vec4<T>::normalizedNonNull () const
{
    T l = length();
    return Vec4 (x, y, z, l);
}


template <class T>
inline
const Vec4<T> &
Vec4<T>::homogenize()
{
	if(h != 0)
	{
		x /= h;
		y /= h;
		z /= h;
		h = 1.0f;
	}
	return *this;
}


template <class T>
inline
Vec4<T>
Vec4<T>::homogenized() const
{
	Vec4<T> r(*this);
	return r.homogenize();
}

//-----------------------------
// Stream output implementation
//-----------------------------

template <class T>
std::ostream &
operator << (std::ostream &s, const Vec4<T> &v)
{
    return s << '(' << v.x << ' ' << v.y << ' ' << v.z << ' ' << v.h << ')';
}

//-----------------------------------------
// Implementation of reverse multiplication
//-----------------------------------------

template <class T>
inline Vec4<T>
operator * (T a, const Vec4<T> &v)
{
    return Vec4<T> (a * v.x, a * v.y, a * v.z, v.h);
}

//------------------------------------------------------------------------------
template <class T>
inline bool 
isClose(const Vec4<T>& v1, const Vec4<T>& v2, T tol = 10*std::numeric_limits<TqFloat>::epsilon())
{
	T diff2 = (v1 - v2).length2();
	T tol2 = tol*tol;
	return diff2 <= tol2*v1.length2() || diff2 <= tol2*v2.length2();
}


#if (defined _WIN32 || defined _WIN64) && defined _MSC_VER
#pragma warning(default:4290)
#endif


} // namespace Aqsis

#endif

