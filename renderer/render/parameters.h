// Aqsis
// Copyright  1997 - 2001, Paul C. Gregory
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
		\brief Declares the classes and support structures for handling parameters attached to GPrims.
		\author Paul C. Gregory (pgregory@aqsis.org)
*/

//? Is .h included already?
#ifndef PARAMETERS_H_INCLUDED
#define PARAMETERS_H_INCLUDED 1

#include	<vector>

#include	"aqsis.h"

#include	"isurface.h"
#include	"ishaderdata.h"
#include	"iparameter.h"
#include	"bilinear.h"
#include	<boost/shared_ptr.hpp>
#include	<boost/utility.hpp>

START_NAMESPACE( Aqsis )


//----------------------------------------------------------------------
/** \class CqParameter
 * Class storing a parameter with a name and value.
 */

class CqParameter : boost::noncopyable, public IqParameter
{
	public:
		CqParameter( const char* strName = "", TqInt Count = 1 );
		virtual	~CqParameter();

		/** Pure virtual, clone function, which only creates a new parameter with matching type, no data.
		 * \return A pointer to a new parameter with the same type.
		 */
		virtual	CqParameter* CloneType( const char* Name, TqInt Count = 1 ) const = 0;
		/** Pure virtual, duplicate function.
		 * \return A pointer to a new parameter with the same name and value.
		 */
		virtual	CqParameter* Clone() const = 0;
		/** Pure virtual, get value class.
		 * \return Class as an EqVariableClass.
		 */
		virtual	EqVariableClass	Class() const = 0;
		/** Pure virtual, get value type.
		 * \return Type as an EqVariableType.
		 */
		virtual	EqVariableType	Type() const = 0;
		/** Pure virtual, set value size, not array, but varying/vertex size.
		 */
		virtual	void	SetSize( TqInt size ) = 0;
		/** Pure virtual, get value size, not array, but varying/vertex size.
		 */
		virtual	TqUint	Size() const = 0;
		/** Pure virtual, clear value contents.
		 */
		virtual	void	Clear() = 0;

		/** \attention
		 * The subdivide functions perform common splitting and interpolation on primitive variables
		 * they are only of use if the variable is a bilinear quad (the most common kind)
		 * any other type of splitting or interpolation must be performed by the surface which 
		 * instantiates special types (i.e. polygons).
		 */
		/** Subdivide the value in the u direction, place one half in this and the other in the specified parameter.
		 * \param pResult1 Pointer to the parameter class to hold the first half of the split.
		 * \param pResult2 Pointer to the parameter class to hold the first half of the split.
		 * \param u Boolean indicating whether to split in the u direction, false indicates split in v.
		 * \param pSurface Pointer to the surface which this paramter belongs, used if the surface has special handling of parameter splitting.
		 */
		virtual void	Subdivide( CqParameter* /* pResult1 */, CqParameter* /* pResult2 */, bool /* u */, IqSurface* /* pSurface */ = 0 )
		{}
		/** Pure virtual, dice the value into a grid using appropriate interpolation for the class.
		 * \param u Integer dice count for the u direction.
		 * \param v Integer dice count for the v direction.
		 * \param pResult Pointer to storage for the result.
		 * \param pSurface Pointer to the surface we are processing used for vertex class variables to perform natural interpolation.
		 */
		virtual	void	Dice( TqInt u, TqInt v, IqShaderData* pResult, IqSurface* pSurface = 0 ) = 0;
		virtual	void	CopyToShaderVariable( IqShaderData* pResult ) = 0;

		/** Pure virtual, dice a single array element of the value into a grid using appropriate interpolation for the class.
		 * \param u Integer dice count for the u direction.
		 * \param v Integer dice count for the v direction.
		 * \param pResult Pointer to storage for the result.
		 * \param pSurface Pointer to the surface we are processing used for vertex class variables to perform natural interpolation.
		 */
		virtual	void	DiceOne( TqInt u, TqInt v, IqShaderData* pResult, IqSurface* pSurface = 0, TqInt ArrayIndex = 0 ) = 0;

		/** Pure virtual, set an indexed value from another parameter (must be the same type).
		 * \param pFrom Pointer to parameter to get values from.
		 * \param idxTarget Index of value to set,
		 * \param idxSource Index of value to get,
		 */
		virtual	void	SetValue( CqParameter* pFrom, TqInt idxTarget, TqInt idxSource ) = 0;

		/** Get a reference to the parameter name.
		 */
		const	CqString& strName() const;
		const TqUlong hash() const;

		/** Get the array size.
		 */
		TqInt	Count() const;

	protected:
		CqString	m_strName;		///< String name of the parameter.
		TqInt	m_Count;		///< Array size of value.
		TqUlong m_hash;
};


//----------------------------------------------------------------------
/** \class CqParameterTyped
 * Parameter templatised by its value type.
 */

template <class T, class SLT>
class CqParameterTyped : public CqParameter
{
	public:
		CqParameterTyped( const char* strName = "", TqInt Count = 1 ) :
				CqParameter( strName, Count )
		{}
		virtual	~CqParameterTyped()
		{}

		/** Get a pointer to the value (presumes uniform).
		 */
		virtual	const	T*	pValue() const = 0;
		/** Get a pointer to the value (presumes uniform).
		 */
		virtual	T*	pValue() = 0;
		/** Get a pointer to the value at the specified index, if uniform index is ignored.
		 */
		virtual	const	T*	pValue( const TqInt Index ) const = 0;
		/** Get a pointer to the value at the specified index, if uniform index is ignored.
		 */
		virtual	T*	pValue( const TqInt Index ) = 0;

		virtual	void	SetValue( CqParameter* pFrom, TqInt idxTarget, TqInt idxSource );

	protected:
};


//----------------------------------------------------------------------
/** \class CqParameterTypedVarying
 * Parameter with a varying type, templatised by value type and type id.
 */

template <class T, EqVariableType I, class SLT>
class CqParameterTypedVarying : public CqParameterTyped<T, SLT>
{
	public:
		CqParameterTypedVarying( const char* strName = "", TqInt Count = 1 );
		virtual	~CqParameterTypedVarying();

		// Overrridden from CqParameter
		virtual	CqParameter* CloneType( const char* Name, TqInt Count = 1 ) const;
		virtual	CqParameter* Clone() const;
		virtual	EqVariableClass	Class() const;
		virtual	EqVariableType	Type() const;
		virtual	void	SetSize( TqInt size );
		virtual	TqUint	Size() const;
		virtual	void	Clear();
		virtual void	Subdivide( CqParameter* pResult1, CqParameter* pResult2, bool u, IqSurface* pSurface = 0 );
		virtual	void	Dice( TqInt u, TqInt v, IqShaderData* pResult, IqSurface* pSurface = 0 );
		virtual	void	CopyToShaderVariable( IqShaderData* pResult );

		virtual	void	DiceOne( TqInt u, TqInt v, IqShaderData* pResult, IqSurface* pSurface = 0, TqInt ArrayIndex = 0 );

		// Overridden from CqParameterTyped<T>

		virtual	const	T*	pValue() const;
		virtual	T*	pValue();
		virtual	const	T*	pValue( const TqInt Index ) const;
		virtual	T*	pValue( const TqInt Index );

		/** Static constructor, to allow type free parameter construction.
		 * \param strName Character pointer to new parameter name.
		 * \param Count Integer array size.
		 */
		static	CqParameter*	Create( const char* strName, TqInt Count = 1 );

	protected:
		std::vector<T>	m_aValues;		///< Vector of values, one per varying index.
};


//----------------------------------------------------------------------
/** \class CqParameterTypedUniform
 * Parameter with a uniform type, templatised by value type and type id.
 */

template <class T, EqVariableType I, class SLT>
class CqParameterTypedUniform : public CqParameterTyped<T, SLT>
{
	public:
		CqParameterTypedUniform( const char* strName = "", TqInt Count = 1 );
		virtual	~CqParameterTypedUniform();

		virtual	CqParameter* CloneType( const char* Name, TqInt Count = 1 ) const;
		virtual	CqParameter* Clone() const;
		virtual	EqVariableClass	Class() const;
		virtual	EqVariableType	Type() const;
		virtual	void	SetSize( TqInt size );
		virtual	TqUint	Size() const;
		virtual	void	Clear();

		virtual void	Subdivide( CqParameter* pResult1, CqParameter* pResult2, bool u, IqSurface* pSurface = 0 );
		virtual	void	Dice( TqInt u, TqInt v, IqShaderData* pResult, IqSurface* pSurface = 0 );

		virtual	void	CopyToShaderVariable( IqShaderData* pResult );

		virtual	void	DiceOne( TqInt u, TqInt v, IqShaderData* pResult, IqSurface* pSurface = 0, TqInt ArrayIndex = 0 );

		// Overridden from CqParameterTyped<T>
		virtual	const	T*	pValue() const;
		virtual	T*	pValue();
		virtual	const	T*	pValue( const TqInt Index ) const;
		virtual	T*	pValue( const TqInt Index );

		/** Static constructor, to allow type free parameter construction.
		 * \param strName Character pointer to new parameter name.
		 * \param Count Integer array size.
		 */
		static	CqParameter*	Create( const char* strName, TqInt Count = 1 );
	private:
		std::vector<T>	m_aValues;		///< Vector of values, one per uniform index.
};


//----------------------------------------------------------------------
/** \class CqParameterTypedConstant
 * Parameter with a constant type, templatised by value type and type id.
 */

template <class T, EqVariableType I, class SLT>
class CqParameterTypedConstant : public CqParameterTyped<T, SLT>
{
	public:
		CqParameterTypedConstant( const char* strName = "", TqInt Count = 1 );
		virtual	~CqParameterTypedConstant();

		virtual	CqParameter* CloneType( const char* Name, TqInt Count = 1 ) const;
		virtual	CqParameter* Clone() const;
		virtual	EqVariableClass	Class() const;
		virtual	EqVariableType	Type() const;
		virtual	void	SetSize( TqInt size );
		virtual	TqUint	Size() const;
		virtual	void	Clear();

		virtual void	Subdivide( CqParameter* pResult1, CqParameter* pResult2, bool u, IqSurface* pSurface = 0 );
		virtual	void	Dice( TqInt u, TqInt v, IqShaderData* pResult, IqSurface* pSurface = 0 );
		virtual	void	CopyToShaderVariable( IqShaderData* pResult );

		virtual	void	DiceOne( TqInt u, TqInt v, IqShaderData* pResult, IqSurface* pSurface = 0, TqInt ArrayIndex = 0 );

		// Overridden from CqParameterTyped<T>
		virtual	const	T*	pValue() const;
		virtual	T*	pValue();
		virtual	const	T*	pValue( const TqInt Index ) const;
		virtual	T*	pValue( const TqInt Index );

		/** Static constructor, to allow type free parameter construction.
		 * \param strName Character pointer to new parameter name.
		 * \param Count Integer array size.
		 */
		static	CqParameter*	Create( const char* strName, TqInt Count = 1 );
	private:
		T	m_Value;	///< Single constant value.
}
;


//----------------------------------------------------------------------
/** \class CqParameterTypedVertex
 * Parameter with a vertex type, templatised by value type and type id.
 */

template <class T, EqVariableType I, class SLT>
class CqParameterTypedVertex : public CqParameterTypedVarying<T, I, SLT>
{
	public:
		CqParameterTypedVertex( const char* strName = "", TqInt Count = 1 );
		virtual	~CqParameterTypedVertex();

		virtual	CqParameter* CloneType( const char* Name, TqInt Count = 1 ) const;
		virtual	CqParameter* Clone() const;
		virtual	EqVariableClass	Class() const;
		virtual void	Subdivide( CqParameter* pResult1, CqParameter* pResult2, bool u, IqSurface* pSurface = 0 );
		virtual	void	Dice( TqInt u, TqInt v, IqShaderData* pResult, IqSurface* pSurface = 0 );
		virtual	void	CopyToShaderVariable( IqShaderData* pResult );

		virtual	void	DiceOne( TqInt u, TqInt v, IqShaderData* pResult, IqSurface* pSurface = 0, TqInt ArrayIndex = 0 );

		/** Static constructor, to allow type free parameter construction.
		 * \param strName Character pointer to new parameter name.
		 * \param Count Integer array size.
		 */
		static	CqParameter*	Create( const char* strName, TqInt Count = 1 );

	private:
};


//----------------------------------------------------------------------
/** \class CqParameterTypedFaceVarying
 * Parameter with a vertex type, templatised by value type and type id.
 */

template <class T, EqVariableType I, class SLT>
class CqParameterTypedFaceVarying : public CqParameterTypedVarying<T, I, SLT>
{
	public:
		CqParameterTypedFaceVarying( const char* strName = "", TqInt Count = 1 );
		virtual	~CqParameterTypedFaceVarying();

		virtual	CqParameter* CloneType( const char* Name, TqInt Count = 1 ) const;
		virtual	CqParameter* Clone() const;
		virtual	EqVariableClass	Class() const;

		/** Static constructor, to allow type free parameter construction.
		 * \param strName Character pointer to new parameter name.
		 * \param Count Integer array size.
		 */
		static	CqParameter*	Create( const char* strName, TqInt Count = 1 );

	private:
};


//----------------------------------------------------------------------
/** \class CqParameterTypedFaceVertex
 * Parameter with a vertex type, templatised by value type and type id.
 */

template <class T, EqVariableType I, class SLT>
class CqParameterTypedFaceVertex : public CqParameterTypedVertex<T, I, SLT>
{
	public:
		CqParameterTypedFaceVertex( const char* strName = "", TqInt Count = 1 );
		virtual	~CqParameterTypedFaceVertex();

		virtual	CqParameter* CloneType( const char* Name, TqInt Count = 1 ) const;
		virtual	CqParameter* Clone() const;
		virtual	EqVariableClass	Class() const;

		/** Static constructor, to allow type free parameter construction.
		 * \param strName Character pointer to new parameter name.
		 * \param Count Integer array size.
		 */
		static	CqParameter*	Create( const char* strName, TqInt Count = 1 );

	private:
};

//----------------------------------------------------------------------
/** \class CqParameterTypedVaryingArray
 * Parameter with a varying array type, templatised by value type and type id.
 */

template <class T, EqVariableType I, class SLT>
class CqParameterTypedVaryingArray : public CqParameterTyped<T, SLT>
{
	public:
		CqParameterTypedVaryingArray( const char* strName = "", TqInt Count = 1 );
		virtual	~CqParameterTypedVaryingArray();

		virtual	CqParameter* CloneType( const char* Name, TqInt Count = 1 ) const;
		virtual	CqParameter* Clone() const;
		virtual	EqVariableClass	Class() const;
		virtual	EqVariableType	Type() const;
		virtual	void	SetSize( TqInt size );
		virtual	TqUint	Size() const;
		virtual	void	Clear();
		virtual void	Subdivide( CqParameter* pResult1, CqParameter* pResult2, bool u, IqSurface* pSurface = 0 );
		virtual	void	Dice( TqInt u, TqInt v, IqShaderData* pResult, IqSurface* pSurface = 0 );
		virtual	void	CopyToShaderVariable( IqShaderData* pResult );
		virtual	void	DiceOne( TqInt u, TqInt v, IqShaderData* pResult, IqSurface* pSurface = 0, TqInt ArrayIndex = 0 );

		// Overridden from CqParameterTyped<T>
		virtual	const	T*	pValue() const;
		virtual	T*	pValue();
		virtual	const	T*	pValue( const TqInt Index ) const;
		virtual	T*	pValue( const TqInt Index );

		virtual	void	SetValue( CqParameter* pFrom, TqInt idxTarget, TqInt idxSource );

		/** Static constructor, to allow type free parameter construction.
		 * \param strName Character pointer to new parameter name.
		 * \param Count Integer array size.
		 */
		static	CqParameter*	Create( const char* strName, TqInt Count = 1 );

	protected:
		std::vector<std::vector<T> >	m_aValues;		///< Array of varying values.
};


//----------------------------------------------------------------------
/** \class CqParameterTypedUniformArray
 * Parameter with a uniform array type, templatised by value type and type id.
 */

template <class T, EqVariableType I, class SLT>
class CqParameterTypedUniformArray : public CqParameterTyped<T, SLT>
{
	public:
		CqParameterTypedUniformArray( const char* strName = "", TqInt Count = 1 );
		virtual	~CqParameterTypedUniformArray();

		virtual	CqParameter* CloneType( const char* Name, TqInt Count = 1 ) const;
		virtual	CqParameter* Clone() const;
		virtual	EqVariableClass	Class() const;
		virtual	EqVariableType	Type() const;
		virtual	void	SetSize( TqInt size );
		virtual	TqUint	Size() const;
		virtual	void	Clear();

		virtual void	Subdivide( CqParameter* pResult1, CqParameter* pResult2, bool u, IqSurface* pSurface = 0 );
		virtual	void	Dice( TqInt u, TqInt v, IqShaderData* pResult, IqSurface* pSurface = 0 );
		virtual	void	CopyToShaderVariable( IqShaderData* pResult );

		virtual	void	DiceOne( TqInt u, TqInt v, IqShaderData* pResult, IqSurface* pSurface = 0, TqInt ArrayIndex = 0 );

		// Overridden from CqParameterTyped<T>
		virtual	const	T*	pValue() const;
		virtual	T*	pValue();
		virtual	const	T*	pValue( const TqInt Index ) const;
		virtual	T*	pValue( const TqInt Index );

		/** Static constructor, to allow type free parameter construction.
		 * \param strName Character pointer to new parameter name.
		 * \param Count Integer array size.
		 */
		static	CqParameter*	Create( const char* strName, TqInt Count = 1 );
	private:
		std::vector<T>	m_aValues;	///< Array of uniform values.
}
;

//----------------------------------------------------------------------
/** \class CqParameterTypedConstantArray
 * Parameter with a constant array type, templatised by value type and type id.
 */

template <class T, EqVariableType I, class SLT>
class CqParameterTypedConstantArray : public CqParameterTyped<T, SLT>
{
	public:
		CqParameterTypedConstantArray( const char* strName = "", TqInt Count = 1 );
		virtual	~CqParameterTypedConstantArray();

		virtual	CqParameter* CloneType( const char* Name, TqInt Count = 1 ) const;
		virtual	CqParameter* Clone() const;
		virtual	EqVariableClass	Class() const;
		virtual	EqVariableType	Type() const;
		virtual	void	SetSize( TqInt size );
		virtual	TqUint	Size() const;
		virtual	void	Clear();

		virtual void	Subdivide( CqParameter* pResult1, CqParameter* pResult2, bool u, IqSurface* pSurface = 0 );
		virtual	void	Dice( TqInt u, TqInt v, IqShaderData* pResult, IqSurface* pSurface = 0 );
		virtual	void	CopyToShaderVariable( IqShaderData* pResult );

		virtual	void	DiceOne( TqInt u, TqInt v, IqShaderData* pResult, IqSurface* pSurface = 0, TqInt ArrayIndex = 0 );

		// Overridden from CqParameterTyped<T>
		virtual	const	T*	pValue() const;
		virtual	T*	pValue();
		virtual	const	T*	pValue( const TqInt Index ) const;
		virtual	T*	pValue( const TqInt Index );

		/** Static constructor, to allow type free parameter construction.
		 * \param strName Character pointer to new parameter name.
		 * \param Count Integer array size.
		 */
		static	CqParameter*	Create( const char* strName, TqInt Count = 1 );
	private:
		std::vector<T>	m_aValues;	///< Array of uniform values.
};


//----------------------------------------------------------------------
/** \class CqParameterTypedVertexArray
 * Parameter with a vertex array type, templatised by value type and type id.
 */

template <class T, EqVariableType I, class SLT>
class CqParameterTypedVertexArray : public CqParameterTypedVaryingArray<T, I, SLT>
{
	public:
		CqParameterTypedVertexArray( const char* strName = "", TqInt Count = 1 );
		virtual	~CqParameterTypedVertexArray();

		virtual	CqParameter* CloneType( const char* Name, TqInt Count = 1 ) const;
		virtual	CqParameter* Clone() const;
		virtual	EqVariableClass	Class() const;
		virtual	void	Dice( TqInt u, TqInt v, IqShaderData* pResult, IqSurface* pSurface = 0 );
		virtual	void	CopyToShaderVariable( IqShaderData* pResult );

		virtual	void	DiceOne( TqInt u, TqInt v, IqShaderData* pResult, IqSurface* pSurface = 0, TqInt ArrayIndex = 0 );

		/** Static constructor, to allow type free parameter construction.
		 * \param strName Character pointer to new parameter name.
		 * \param Count Integer array size.
		 */
		static	CqParameter*	Create( const char* strName, TqInt Count = 1 );

	private:
};


//----------------------------------------------------------------------
/** \class CqParameterTypedFaceVaryingArray
 * Parameter with a facevarying array type, templatised by value type and type id.
 */

template <class T, EqVariableType I, class SLT>
class CqParameterTypedFaceVaryingArray : public CqParameterTypedVaryingArray<T, I, SLT>
{
	public:
		CqParameterTypedFaceVaryingArray( const char* strName = "", TqInt Count = 1 );
		virtual	~CqParameterTypedFaceVaryingArray();

		virtual	CqParameter* CloneType( const char* Name, TqInt Count = 1 ) const;
		virtual	CqParameter* Clone() const;
		virtual	EqVariableClass	Class() const;

		/** Static constructor, to allow type free parameter construction.
		 * \param strName Character pointer to new parameter name.
		 * \param Count Integer array size.
		 */
		static	CqParameter*	Create( const char* strName, TqInt Count = 1 );

	private:
};


//----------------------------------------------------------------------
/** \class CqParameterTypedFaceVertexArray
 * Parameter with a facevertex array type, templatised by value type and type id.
 */

template <class T, EqVariableType I, class SLT>
class CqParameterTypedFaceVertexArray : public CqParameterTypedVertexArray<T, I, SLT>
{
	public:
		CqParameterTypedFaceVertexArray( const char* strName = "", TqInt Count = 1 );
		virtual	~CqParameterTypedFaceVertexArray();

		virtual	CqParameter* CloneType( const char* Name, TqInt Count = 1 ) const;
		virtual	CqParameter* Clone() const;
		virtual	EqVariableClass	Class() const;

		/** Static constructor, to allow type free parameter construction.
		 * \param strName Character pointer to new parameter name.
		 * \param Count Integer array size.
		 */
		static	CqParameter*	Create( const char* strName, TqInt Count = 1 );

	private:
};

//----------------------------------------------------------------------
/** \class CqNamedParameterList
 */

class CqNamedParameterList
{
	public:
		CqNamedParameterList( const char* strName );
		CqNamedParameterList( const CqNamedParameterList& From );
		~CqNamedParameterList();

#ifdef _DEBUG
		CqString className() const;
#endif

		/** Get a refernece to the option name.
		 * \return A constant CqString reference.
		 */
		const	CqString&	strName() const;

		/** Add a new name/value pair to this option/attribute.
		 * \param pParameter Pointer to a CqParameter containing the name/value pair.
		 */
		void	AddParameter( const CqParameter* pParameter );
		/** Get a read only pointer to a named parameter.
		 * \param strName Character pointer pointing to zero terminated parameter name.
		 * \return A pointer to a CqParameter or 0 if not found.
		 */
		const	CqParameter* pParameter( const char* strName ) const;
		/** Get a pointer to a named parameter.
		 * \param strName Character pointer pointing to zero terminated parameter name.
		 * \return A pointer to a CqParameter or 0 if not found.
		 */
		CqParameter* pParameter( const char* strName );
		TqUlong hash();
	private:
		CqString	m_strName;			///< The name of this parameter list.
		std::vector<CqParameter*>	m_aParameters;		///< A vector of name/value parameters.
		TqUlong m_hash;
};

///////////////////////////////////////////////////////////////////////////////
// Inline functions.
///////////////////////////////////////////////////////////////////////////////

inline const CqString& CqParameter::strName() const
{
	return ( m_strName );
}

inline const TqUlong CqParameter::hash() const
{
	return m_hash;
}

inline TqInt CqParameter::Count() const
{
	return ( m_Count );
}

//---------------------

template <class T, class SLT>
void CqParameterTyped<T, SLT>::SetValue( CqParameter* pFrom, TqInt idxTarget, TqInt idxSource )
{
	assert( pFrom->Type() == this->Type() );

	CqParameterTyped<T, SLT>* pFromTyped = static_cast<CqParameterTyped<T, SLT>*>( pFrom );
	*pValue( idxTarget ) = *pFromTyped->pValue( idxSource );
}

template <class T, EqVariableType I, class SLT>
CqParameterTypedVarying<T, I, SLT>::CqParameterTypedVarying( const char* strName, TqInt Count ) :
		CqParameterTyped<T, SLT>( strName, Count )
{
	m_aValues.resize( 1 );
}

template <class T, EqVariableType I, class SLT>
CqParameterTypedVarying<T,I,SLT>::~CqParameterTypedVarying()
{
}

template <class T, EqVariableType I, class SLT>
CqParameter* CqParameterTypedVarying<T, I, SLT>::CloneType( const char* Name, TqInt Count ) const
{
	return ( new CqParameterTypedVarying<T, I, SLT>( Name, Count ) );
}

template <class T, EqVariableType I, class SLT>
EqVariableClass	CqParameterTypedVarying<T, I, SLT>::Class() const
{
	return ( class_varying );
}

template <class T, EqVariableType I, class SLT>
EqVariableType CqParameterTypedVarying<T, I, SLT>::Type() const
{
	return ( I );
}

template <class T, EqVariableType I, class SLT>
void CqParameterTypedVarying<T, I, SLT>::SetSize( TqInt size )
{
	m_aValues.resize( size );
}

template <class T, EqVariableType I, class SLT>
TqUint CqParameterTypedVarying<T, I, SLT>::Size() const
{
	return ( m_aValues.size() );
}

template <class T, EqVariableType I, class SLT>
void CqParameterTypedVarying<T, I, SLT>::Clear()
{
	m_aValues.clear();
}

template <class T, EqVariableType I, class SLT>
void CqParameterTypedVarying<T, I, SLT>::Subdivide( CqParameter* pResult1, CqParameter* pResult2, bool u, IqSurface* pSurface )
{
	assert( pResult1->Type() == this->Type() && pResult1->Type() == this->Type() &&
	        pResult1->Class() == this->Class() && pResult1->Class() == this->Class() );

	CqParameterTypedVarying<T, I, SLT>* pTResult1 = static_cast<CqParameterTypedVarying<T, I, SLT>*>( pResult1 );
	CqParameterTypedVarying<T, I, SLT>* pTResult2 = static_cast<CqParameterTypedVarying<T, I, SLT>*>( pResult2 );
	pTResult1->SetSize( 4 );
	pTResult2->SetSize( 4 );
	// Check if a valid 4 point quad, do nothing if not.
	if ( m_aValues.size() == 4 )
	{
		if ( u )
		{
			pTResult2->pValue( 1 ) [ 0 ] = pValue( 1 ) [ 0 ];
			pTResult2->pValue( 3 ) [ 0 ] = pValue( 3 ) [ 0 ];
			pTResult1->pValue( 1 ) [ 0 ] = pTResult2->pValue( 0 ) [ 0 ] = static_cast<T>( ( pValue( 0 ) [ 0 ] + pValue( 1 ) [ 0 ] ) * 0.5 );
			pTResult1->pValue( 3 ) [ 0 ] = pTResult2->pValue( 2 ) [ 0 ] = static_cast<T>( ( pValue( 2 ) [ 0 ] + pValue( 3 ) [ 0 ] ) * 0.5 );
		}
		else
		{
			pTResult2->pValue( 2 ) [ 0 ] = pValue( 2 ) [ 0 ];
			pTResult2->pValue( 3 ) [ 0 ] = pValue( 3 ) [ 0 ];
			pTResult1->pValue( 2 ) [ 0 ] = pTResult2->pValue( 0 ) [ 0 ] = static_cast<T>( ( pValue( 0 ) [ 0 ] + pValue( 2 ) [ 0 ] ) * 0.5 );
			pTResult1->pValue( 3 ) [ 0 ] = pTResult2->pValue( 1 ) [ 0 ] = static_cast<T>( ( pValue( 1 ) [ 0 ] + pValue( 3 ) [ 0 ] ) * 0.5 );
		}
	}
}

template <class T, EqVariableType I, class SLT>
void CqParameterTypedVarying<T, I, SLT>::DiceOne( TqInt u, TqInt v, IqShaderData* pResult, IqSurface* pSurface, TqInt ArrayIndex )
{
	assert( false );
	return;
}

template <class T, EqVariableType I, class SLT>
const T* CqParameterTypedVarying<T, I, SLT>::pValue() const
{
	assert( 0 < m_aValues.size() );
	return ( &m_aValues[ 0 ] );
}

template <class T, EqVariableType I, class SLT>
T* CqParameterTypedVarying<T, I, SLT>::pValue()
{
	assert( 0 < m_aValues.size() );
	return ( &m_aValues[ 0 ] );
}

template <class T, EqVariableType I, class SLT>
const T* CqParameterTypedVarying<T, I, SLT>::pValue( const TqInt Index ) const
{
	assert( Index < static_cast<TqInt>( m_aValues.size() ) );
	return ( &m_aValues[ Index ] );
}

template <class T, EqVariableType I, class SLT>
T* CqParameterTypedVarying<T, I, SLT>::pValue( const TqInt Index )
{
	assert( Index < static_cast<TqInt>( m_aValues.size() ) );
	return ( &m_aValues[ Index ] );
}

template <class T, EqVariableType I, class SLT>
CqParameter* CqParameterTypedVarying<T, I, SLT>::Create( const char* strName, TqInt Count )
{
	return ( new CqParameterTypedVarying<T, I, SLT>( strName, Count ) );
}

template <class T, EqVariableType I, class SLT>
CqParameter* CqParameterTypedVarying<T, I, SLT>::Clone() const
{
	CqParameterTypedVarying<T, I, SLT>* clone = new CqParameterTypedVarying<T, I, SLT>( this->strName().c_str(), this->Count() );
	TqInt size = m_aValues.size();

	clone->m_aValues.resize( size );

	for ( TqUint j = 0; j < (TqUint) size; j++ )
	{
		clone->m_aValues[ j ] = m_aValues[ j ];
	}
	return(clone);
}

/** Dice the value into a grid using bilinear interpolation.
 * \param u Integer dice count for the u direction.
 * \param v Integer dice count for the v direction.
 * \param pResult Pointer to storage for the result.
 * \param pSurface Pointer to the surface to which this parameter belongs. Used if the surface type has special handling for parameter dicing.
 */

template <class T, EqVariableType I, class SLT>
void CqParameterTypedVarying<T, I, SLT>::Dice( TqInt u, TqInt v, IqShaderData* pResult, IqSurface* pSurface )
{
	T res;

	SLT* pResData;
	pResult->GetValuePtr( pResData );
	assert( NULL != pResData );

	// Check if a valid 4 point quad, do nothing if not.
	if ( m_aValues.size() >= 4 )
	{
		// Note it is assumed that the variable has been
		// initialised to the correct size prior to calling.
		TqFloat diu = 1.0 / u;
		TqFloat div = 1.0 / v;
		TqInt iv;
		for ( iv = 0; iv <= v; iv++ )
		{
			TqInt iu;
			for ( iu = 0; iu <= u; iu++ )
			{
				res = BilinearEvaluate<T>( pValue( 0 ) [ 0 ],
				                           pValue( 1 ) [ 0 ],
				                           pValue( 2 ) [ 0 ],
				                           pValue( 3 ) [ 0 ],
				                           iu * diu, iv * div );
				( *pResData++ ) = res;
			}
		}
	}
	else
	{
		TqInt iv;
		res = pValue( 0 ) [ 0 ];
		for ( iv = 0; iv <= v; iv++ )
		{
			TqInt iu;
			for ( iu = 0; iu <= u; iu++ )
				( *pResData++ ) = res;
		}
	}
}

template <class T, EqVariableType I, class SLT>
void CqParameterTypedVarying<T, I, SLT>::CopyToShaderVariable( IqShaderData* pResult )
{
	SLT* pResData;
	pResult->GetValuePtr( pResData );
	assert( NULL != pResData );

	TqUint iu;
	for ( iu = 0; iu <= pResult->Size(); iu++ )
		( *pResData++ ) = pValue(iu)[0];
}

//-----------------

template <class T, EqVariableType I, class SLT>
CqParameterTypedUniform<T,I,SLT>::CqParameterTypedUniform( const char* strName, TqInt Count ) :
		CqParameterTyped<T, SLT>( strName, Count )
{
	m_aValues.resize( 1 );
}

template <class T, EqVariableType I, class SLT>
CqParameterTypedUniform<T,I,SLT>::~CqParameterTypedUniform()
{}

template <class T, EqVariableType I, class SLT>
CqParameter* CqParameterTypedUniform<T,I,SLT>::CloneType( const char* Name, TqInt Count ) const
{
	return ( new CqParameterTypedUniform<T,I,SLT>(Name, Count) );
}

template <class T, EqVariableType I, class SLT>
CqParameter* CqParameterTypedUniform<T,I,SLT>::Clone() const
{
	CqParameterTypedUniform<T,I,SLT>* clone = new CqParameterTypedUniform<T,I,SLT>(this->strName().c_str(), this->Count());
	clone->m_aValues.resize( m_aValues.size() );
	for ( TqUint j = 0; j < m_aValues.size(); j++ )
	{
		clone->m_aValues[ j ] = m_aValues[ j ];
	}
	return ( clone );
}

template <class T, EqVariableType I, class SLT>
EqVariableClass	CqParameterTypedUniform<T,I,SLT>::Class() const
{
	return ( class_uniform );
}

template <class T, EqVariableType I, class SLT>
EqVariableType	CqParameterTypedUniform<T,I,SLT>::Type() const
{
	return ( I );
}

template <class T, EqVariableType I, class SLT>
void CqParameterTypedUniform<T,I,SLT>::SetSize( TqInt size )
{
	m_aValues.resize( size );
}

template <class T, EqVariableType I, class SLT>
TqUint CqParameterTypedUniform<T,I,SLT>::Size() const
{
	return ( m_aValues.size() );
}

template <class T, EqVariableType I, class SLT>
void CqParameterTypedUniform<T,I,SLT>::Clear()
{
	m_aValues.clear();
}

template <class T, EqVariableType I, class SLT>
void CqParameterTypedUniform<T,I,SLT>::Subdivide( CqParameter* pResult1, CqParameter* pResult2, bool u, IqSurface* pSurface )
{
	assert( pResult1->Type() == this->Type() && pResult1->Type() == this->Type() &&
	        pResult1->Class() == Class() && pResult1->Class() == Class() );

	pResult1 = Clone();
	pResult2 = Clone();
}

template <class T, EqVariableType I, class SLT>
void CqParameterTypedUniform<T,I,SLT>::Dice( TqInt u, TqInt v, IqShaderData* pResult, IqSurface* pSurface )
{
	// Just promote the uniform value to varying by duplication.
	assert( pResult->Type() == this->Type() );
	// Note it is assumed that the variable has been
	// initialised to the correct size prior to calling.
	// Also note that the only time a Uniform value is diced is when it is on a single element, i.e. the patchmesh
	// has been split into isngle patches, or the polymesh has been split into polys.
	TqUint i;
	TqUint max = MAX( (TqUint)u * (TqUint)v, pResult->Size() );
	for ( i = 0; i < max; i++ )
		pResult->SetValue( m_aValues[ 0 ], i );
}

template <class T, EqVariableType I, class SLT>
void CqParameterTypedUniform<T,I,SLT>::CopyToShaderVariable( IqShaderData* pResult )
{
	// Just promote the uniform value to varying by duplication.
	assert( pResult->Type() == this->Type() );
	// Note it is assumed that the variable has been
	// initialised to the correct size prior to calling.
	// Also note that the only time a Uniform value is diced is when it is on a single element, i.e. the patchmesh
	// has been split into isngle patches, or the polymesh has been split into polys.
	TqUint i;
	TqUint max = pResult->Size();
	for ( i = 0; i < max; i++ )
		pResult->SetValue( m_aValues[ 0 ], i );
}

template <class T, EqVariableType I, class SLT>
void CqParameterTypedUniform<T,I,SLT>::DiceOne( TqInt u, TqInt v, IqShaderData* pResult, IqSurface* pSurface, TqInt ArrayIndex )
{
	assert( false );
	return;
}

template <class T, EqVariableType I, class SLT>
const T* CqParameterTypedUniform<T,I,SLT>::pValue() const
{
	assert( 0 < m_aValues.size() );
	return ( &m_aValues[ 0 ] );
}

template <class T, EqVariableType I, class SLT>
T* CqParameterTypedUniform<T,I,SLT>::pValue()
{
	assert( 0 < m_aValues.size() );
	return ( &m_aValues[ 0 ] );
}

template <class T, EqVariableType I, class SLT>
const T* CqParameterTypedUniform<T,I,SLT>::pValue( const TqInt Index ) const
{
	assert( 0 < m_aValues.size() );
	return ( &m_aValues[ Index ] );
}

template <class T, EqVariableType I, class SLT>
T* CqParameterTypedUniform<T,I,SLT>::pValue( const TqInt Index )
{
	assert( 0 < m_aValues.size() );
	return ( &m_aValues[ Index ] );
}

template <class T, EqVariableType I, class SLT>
CqParameter* CqParameterTypedUniform<T,I,SLT>::Create( const char* strName, TqInt Count )
{
	return ( new CqParameterTypedUniform<T, I, SLT>( strName, Count ) );
}

//----------------

template <class T, EqVariableType I, class SLT>
CqParameterTypedConstant<T,I,SLT>::CqParameterTypedConstant( const char* strName, TqInt Count ) :
		CqParameterTyped<T, SLT>( strName, Count )
{}

template <class T, EqVariableType I, class SLT>
CqParameterTypedConstant<T,I,SLT>::~CqParameterTypedConstant()
{}

template <class T, EqVariableType I, class SLT>
CqParameter* CqParameterTypedConstant<T,I,SLT>::CloneType( const char* Name, TqInt Count ) const
{
	return ( new CqParameterTypedConstant<T, I, SLT>( Name, Count ) );
}

template <class T, EqVariableType I, class SLT>
CqParameter* CqParameterTypedConstant<T,I,SLT>::Clone() const
{
	CqParameterTypedConstant<T,I,SLT>* clone = new CqParameterTypedConstant<T,I,SLT>(this->strName().c_str(), this->Count());
	clone->m_Value = m_Value;
	return ( clone );
}

template <class T, EqVariableType I, class SLT>
EqVariableClass	CqParameterTypedConstant<T,I,SLT>::Class() const
{
	return ( class_constant );
}

template <class T, EqVariableType I, class SLT>
EqVariableType	CqParameterTypedConstant<T,I,SLT>::Type() const
{
	return ( I );
}

template <class T, EqVariableType I, class SLT>
void CqParameterTypedConstant<T,I,SLT>::SetSize( TqInt size )
{}

template <class T, EqVariableType I, class SLT>
TqUint CqParameterTypedConstant<T,I,SLT>::Size() const
{
	return ( 1 );
}

template <class T, EqVariableType I, class SLT>
void CqParameterTypedConstant<T,I,SLT>::Clear()
{}

template <class T, EqVariableType I, class SLT>
void CqParameterTypedConstant<T,I,SLT>::Subdivide( CqParameter* pResult1, CqParameter* pResult2, bool u, IqSurface* pSurface )
{
	assert( pResult1->Type() == this->Type() && pResult1->Type() == this->Type() &&
	        pResult1->Class() == this->Class() && pResult1->Class() == this->Class() );

	pResult1 = Clone();
	pResult2 = Clone();
}

template <class T, EqVariableType I, class SLT>
void CqParameterTypedConstant<T,I,SLT>::Dice( TqInt u, TqInt v, IqShaderData* pResult, IqSurface* pSurface )
{
	// Just promote the constant value to varying by duplication.
	assert( pResult->Type() == this->Type() );
	// Note it is assumed that the variable has been
	// initialised to the correct size prior to calling.
	TqUint i;
	TqUint max = MAX( (TqUint) u * (TqUint) v, pResult->Size() );
	for ( i = 0; i < max ; i++ )
		pResult->SetValue( m_Value, i );
}

template <class T, EqVariableType I, class SLT>
void CqParameterTypedConstant<T,I,SLT>::CopyToShaderVariable( IqShaderData* pResult )
{
	// Just promote the constant value to varying by duplication.
	assert( pResult->Type() == this->Type() );
	// Note it is assumed that the variable has been
	// initialised to the correct size prior to calling.
	TqUint i;
	TqUint max = pResult->Size();
	for ( i = 0; i < max ; i++ )
		pResult->SetValue( m_Value, i );
}

template <class T, EqVariableType I, class SLT>
void CqParameterTypedConstant<T,I,SLT>::DiceOne( TqInt u, TqInt v, IqShaderData* pResult, IqSurface* pSurface, TqInt ArrayIndex )
{
	assert( false );
	return;
}

template <class T, EqVariableType I, class SLT>
const T* CqParameterTypedConstant<T,I,SLT>::pValue() const
{
	return ( &m_Value );
}

template <class T, EqVariableType I, class SLT>
T* CqParameterTypedConstant<T,I,SLT>::pValue()
{
	return ( &m_Value );
}

template <class T, EqVariableType I, class SLT>
const T* CqParameterTypedConstant<T,I,SLT>::pValue( const TqInt Index ) const
{
	return ( &m_Value );
}

template <class T, EqVariableType I, class SLT>
T* CqParameterTypedConstant<T,I,SLT>::pValue( const TqInt Index )
{
	return ( &m_Value );
}

template <class T, EqVariableType I, class SLT>
CqParameter* CqParameterTypedConstant<T,I,SLT>::Create( const char* strName, TqInt Count )
{
	return ( new CqParameterTypedConstant<T, I, SLT>( strName, Count ) );
}

//-------------

template <class T, EqVariableType I, class SLT>
CqParameterTypedVertex<T,I,SLT>::CqParameterTypedVertex( const char* strName, TqInt Count ) :
		CqParameterTypedVarying<T, I, SLT>( strName, Count )
{}

template <class T, EqVariableType I, class SLT>
CqParameterTypedVertex<T,I,SLT>::~CqParameterTypedVertex()
{}

template <class T, EqVariableType I, class SLT>
CqParameter* CqParameterTypedVertex<T,I,SLT>::CloneType( const char* Name, TqInt Count ) const
{
	return ( new CqParameterTypedVertex<T, I, SLT>( Name, Count ) );
}

template <class T, EqVariableType I, class SLT>
CqParameter* CqParameterTypedVertex<T,I,SLT>::Clone() const
{
	CqParameterTypedVertex<T, I, SLT>* clone = new CqParameterTypedVertex<T, I, SLT>( this->strName().c_str(), this->Count() );
	TqInt size = this->Size();

	clone->SetSize( size );

	for ( TqUint j = 0; j < (TqUint) size; j++ )
	{
		clone->m_aValues[ j ] = this->m_aValues[ j ];
	}
	return(clone);
}

template <class T, EqVariableType I, class SLT>
EqVariableClass	CqParameterTypedVertex<T,I,SLT>::Class() const
{
	return ( class_vertex );
}

template <class T, EqVariableType I, class SLT>
void CqParameterTypedVertex<T,I,SLT>::Subdivide( CqParameter* pResult1, CqParameter* pResult2, bool u, IqSurface* pSurface )
{
	assert( pResult1->Type() == this->Type() && pResult1->Type() == this->Type() &&
	        pResult1->Class() == this->Class() && pResult1->Class() == this->Class() );

	pSurface->NaturalSubdivide( this, pResult1, pResult2, u );
}

template <class T, EqVariableType I, class SLT>
void CqParameterTypedVertex<T,I,SLT>::Dice( TqInt u, TqInt v, IqShaderData* pResult, IqSurface* pSurface )
{
	// Just promote the constant value to varying by duplication.
	assert( pResult->Type() == this->Type() );
	assert( NULL != pSurface );
	// Note it is assumed that the variable has been
	// initialised to the correct size prior to calling.
	pSurface->NaturalDice( this, u, v, pResult );
}

template <class T, EqVariableType I, class SLT>
void CqParameterTypedVertex<T,I,SLT>::CopyToShaderVariable( IqShaderData* pResult )
{
	assert( pResult->Type() == this->Type() );
	TqUint i;
	TqUint max = pResult->Size();
	for ( i = 0; i < max ; i++ )
		pResult->SetValue( this->pValue(i)[0], i );
}

template <class T, EqVariableType I, class SLT>
void CqParameterTypedVertex<T,I,SLT>::DiceOne( TqInt u, TqInt v, IqShaderData* pResult, IqSurface* pSurface, TqInt ArrayIndex )
{
	assert( false );
	return;
}

template <class T, EqVariableType I, class SLT>
CqParameter* CqParameterTypedVertex<T,I,SLT>::Create( const char* strName, TqInt Count )
{
	return ( new CqParameterTypedVertex<T, I, SLT>( strName, Count ) );
}

//-------------------

template <class T, EqVariableType I, class SLT>
CqParameterTypedFaceVarying<T,I,SLT>::CqParameterTypedFaceVarying( const char* strName, TqInt Count ) :
		CqParameterTypedVarying<T, I, SLT>( strName, Count )
{}

template <class T, EqVariableType I, class SLT>
CqParameterTypedFaceVarying<T,I,SLT>::~CqParameterTypedFaceVarying()
{}

template <class T, EqVariableType I, class SLT>
CqParameter* CqParameterTypedFaceVarying<T,I,SLT>::CloneType( const char* Name, TqInt Count ) const
{
	return ( new CqParameterTypedFaceVarying<T, I, SLT>( Name, Count ) );
}

template <class T, EqVariableType I, class SLT>
CqParameter* CqParameterTypedFaceVarying<T,I,SLT>::Clone() const
{
	CqParameterTypedFaceVarying<T, I, SLT>* clone = new CqParameterTypedFaceVarying<T, I, SLT>( this->strName().c_str(), this->Count() );
	TqInt size = this->m_aValues.size();

	clone->m_aValues.resize( size );

	for ( TqUint j = 0; j < (TqUint) size; j++ )
	{
		clone->m_aValues[ j ] = this->m_aValues[ j ];
	}
	return(clone);
}

template <class T, EqVariableType I, class SLT>
EqVariableClass	CqParameterTypedFaceVarying<T,I,SLT>::Class() const
{
	return ( class_facevarying );
}

template <class T, EqVariableType I, class SLT>
CqParameter* CqParameterTypedFaceVarying<T,I,SLT>::Create( const char* strName, TqInt Count )
{
	return ( new CqParameterTypedFaceVarying<T, I, SLT>( strName, Count ) );
}

//------------------

template <class T, EqVariableType I, class SLT>
CqParameterTypedFaceVertex<T,I,SLT>::CqParameterTypedFaceVertex( const char* strName, TqInt Count ) :
		CqParameterTypedVertex<T, I, SLT>( strName, Count )
{}

template <class T, EqVariableType I, class SLT>
CqParameterTypedFaceVertex<T,I,SLT>::~CqParameterTypedFaceVertex()
{}

template <class T, EqVariableType I, class SLT>
CqParameter* CqParameterTypedFaceVertex<T,I,SLT>::CloneType( const char* Name, TqInt Count ) const
{
	return ( new CqParameterTypedFaceVertex<T, I, SLT>( Name, Count ) );
}

template <class T, EqVariableType I, class SLT>
CqParameter* CqParameterTypedFaceVertex<T,I,SLT>::Clone() const
{
	CqParameterTypedFaceVertex<T, I, SLT>* clone = new CqParameterTypedFaceVertex<T, I, SLT>( this->strName().c_str(), this->Count() );
	TqInt size = this->m_aValues.size();

	clone->m_aValues.resize( size );

	for ( TqUint j = 0; j < (TqUint) size; j++ )
	{
		clone->m_aValues[ j ] = this->m_aValues[ j ];
	}
	return(clone);
}

template <class T, EqVariableType I, class SLT>
EqVariableClass	CqParameterTypedFaceVertex<T,I,SLT>::Class() const
{
	return ( class_facevertex );
}

template <class T, EqVariableType I, class SLT>
CqParameter* CqParameterTypedFaceVertex<T,I,SLT>::Create( const char* strName, TqInt Count )
{
	return ( new CqParameterTypedFaceVertex<T, I, SLT>( strName, Count ) );
}

//-----------------

template <class T, EqVariableType I, class SLT>
CqParameterTypedVaryingArray<T,I,SLT>::CqParameterTypedVaryingArray( const char* strName, TqInt Count ) :
		CqParameterTyped<T, SLT>( strName, Count )
{
	m_aValues.resize( 1, std::vector<T>(Count) );
}

template <class T, EqVariableType I, class SLT>
CqParameterTypedVaryingArray<T,I,SLT>::~CqParameterTypedVaryingArray()
{}

template <class T, EqVariableType I, class SLT>
CqParameter* CqParameterTypedVaryingArray<T,I,SLT>::CloneType( const char* Name, TqInt Count ) const
{
	return ( new CqParameterTypedVaryingArray<T, I, SLT>( Name, Count ) );
}

template <class T, EqVariableType I, class SLT>
CqParameter* CqParameterTypedVaryingArray<T,I,SLT>::Clone() const
{
	CqParameterTypedVaryingArray<T, I, SLT>* clone = new CqParameterTypedVaryingArray<T, I, SLT>( this->strName().c_str(), this->Count() );
	clone->m_aValues.resize( m_aValues.size(), std::vector<T>(this->Count()) );
	clone->m_Count = this->Count();
	TqUint j;
	for ( j = 0; j < m_aValues.size(); j++ )
	{
		TqUint i;
		for ( i = 0; i < (TqUint) this->Count(); i++ )
			clone->m_aValues[ j ][ i ] = m_aValues[ j ][ i ];
	}
	return(clone);
}

template <class T, EqVariableType I, class SLT>
EqVariableClass	CqParameterTypedVaryingArray<T,I,SLT>::Class() const
{
	return ( class_varying );
}

template <class T, EqVariableType I, class SLT>
EqVariableType CqParameterTypedVaryingArray<T,I,SLT>::Type() const
{
	return ( I );
}

template <class T, EqVariableType I, class SLT>
void CqParameterTypedVaryingArray<T,I,SLT>::SetSize( TqInt size )
{
	m_aValues.resize( size, std::vector< T >(this->m_Count) );
}

template <class T, EqVariableType I, class SLT>
TqUint CqParameterTypedVaryingArray<T,I,SLT>::Size() const
{
	return ( m_aValues.size() );
}

template <class T, EqVariableType I, class SLT>
void CqParameterTypedVaryingArray<T,I,SLT>::Clear()
{
	m_aValues.clear();
}

template <class T, EqVariableType I, class SLT>
void CqParameterTypedVaryingArray<T,I,SLT>::Subdivide( CqParameter* pResult1, CqParameter* pResult2, bool u, IqSurface* pSurface )
{
	assert( pResult1->Type() == this->Type() && pResult1->Type() == this->Type() &&
	        pResult1->Class() == Class() && pResult1->Class() == Class() );

	CqParameterTypedVaryingArray<T, I, SLT>* pTResult1 = static_cast<CqParameterTypedVaryingArray<T, I, SLT>*>( pResult1 );
	CqParameterTypedVaryingArray<T, I, SLT>* pTResult2 = static_cast<CqParameterTypedVaryingArray<T, I, SLT>*>( pResult2 );
	pTResult1->SetSize( 4 );
	pTResult2->SetSize( 4 );
	// Check if a valid 4 point quad, do nothing if not.
	if ( m_aValues.size() == 4 )
	{
		if ( u )
		{
			TqInt index;
			for( index = this->Count()-1; index >= 0; index-- )
			{
				pTResult2->pValue( 1 ) [ index ] = pValue( 1 ) [ index ];
				pTResult2->pValue( 3 ) [ index ] = pValue( 3 ) [ index ];
				pTResult1->pValue( 1 ) [ index ] = pTResult2->pValue( 0 ) [ index ] = static_cast<T>( ( pValue( 0 ) [ index ] + pValue( 1 ) [ index ] ) * 0.5 );
				pTResult1->pValue( 3 ) [ index ] = pTResult2->pValue( 2 ) [ index ] = static_cast<T>( ( pValue( 2 ) [ index ] + pValue( 3 ) [ index ] ) * 0.5 );
			}
		}
		else
		{
			TqInt index;
			for( index = this->Count()-1; index >= 0; index-- )
			{
				pTResult2->pValue( 2 ) [ index ] = pValue( 2 ) [ index ];
				pTResult2->pValue( 3 ) [ index ] = pValue( 3 ) [ index ];
				pTResult1->pValue( 2 ) [ index ] = pTResult2->pValue( 0 ) [ index ] = static_cast<T>( ( pValue( 0 ) [ index ] + pValue( 2 ) [ index ] ) * 0.5 );
				pTResult1->pValue( 3 ) [ index ] = pTResult2->pValue( 1 ) [ index ] = static_cast<T>( ( pValue( 1 ) [ index ] + pValue( 3 ) [ index ] ) * 0.5 );
			}
		}
	}
}

template <class T, EqVariableType I, class SLT>
const T* CqParameterTypedVaryingArray<T,I,SLT>::pValue() const
{
	assert( 0 < m_aValues.size() );
	assert( 0 < m_aValues[0].size() );
	return ( &m_aValues[ 0 ][ 0 ] );
}

template <class T, EqVariableType I, class SLT>
T* CqParameterTypedVaryingArray<T,I,SLT>::pValue()
{
	assert( 0 < m_aValues.size() );
	assert( 0 < m_aValues[0].size() );
	return ( &m_aValues[ 0 ][ 0 ] );
}

template <class T, EqVariableType I, class SLT>
const T* CqParameterTypedVaryingArray<T,I,SLT>::pValue( const TqInt Index ) const
{
	assert( Index < static_cast<TqInt>( m_aValues.size() ) );
	assert( 0 < m_aValues[0].size() );
	return ( &m_aValues[ Index ][ 0 ] );
}

template <class T, EqVariableType I, class SLT>
T* CqParameterTypedVaryingArray<T,I,SLT>::pValue( const TqInt Index )
{
	assert( Index < static_cast<TqInt>( m_aValues.size() ) );
	assert( 0 < m_aValues[0].size() );
	return ( &m_aValues[ Index ][ 0 ] );
}


template <class T, EqVariableType I, class SLT>
void CqParameterTypedVaryingArray<T,I,SLT>::SetValue( CqParameter* pFrom, TqInt idxTarget, TqInt idxSource )
{
	assert( pFrom->Type() == this->Type() );
	assert( pFrom->Count() == this->Count() );

	CqParameterTyped<T, SLT>* pFromTyped = static_cast<CqParameterTyped<T, SLT>*>( pFrom );
	TqInt index;
	T* pTargetValues = pValue( idxTarget );
	T* pSourceValues = pFromTyped->pValue( idxSource );
	for( index = 0; index < this->Count(); index++ )
		pTargetValues[ index ] = pSourceValues[ index ];
}

template <class T, EqVariableType I, class SLT>
CqParameter* CqParameterTypedVaryingArray<T,I,SLT>::Create( const char* strName, TqInt Count )
{
	return ( new CqParameterTypedVaryingArray<T, I, SLT>( strName, Count ) );
}


/** Dice the value into a grid using bilinear interpolation.
 * \param u Integer dice count for the u direction.
 * \param v Integer dice count for the v direction.
 * \param pResult Pointer to storage for the result.
 * \param pSurface Pointer to the surface to which this parameter belongs. Used if the surface type has special handling for parameter dicing.
 */

template <class T, EqVariableType I, class SLT>
void CqParameterTypedVaryingArray<T, I, SLT>::Dice( TqInt u, TqInt v, IqShaderData* pResult, IqSurface* pSurface )
{
	assert( pResult->Type() == this->Type() );
	assert( pResult->Class() == class_varying );
	assert( pResult->Size() == m_aValues.size() );

	T res;

	std::vector<SLT*> pResData(this->Count());
	
	TqInt arrayIndex;
	for(arrayIndex = 0; arrayIndex < this->Count(); arrayIndex++)
		pResult->ArrayEntry(arrayIndex)->GetValuePtr( pResData[arrayIndex] );

	// Check if a valid 4 point quad, do nothing if not.
	if ( m_aValues.size() == 4 )
	{
		// Note it is assumed that the variable has been
		// initialised to the correct size prior to calling.
		TqFloat diu = 1.0 / u;
		TqFloat div = 1.0 / v;
		TqInt iv;
		for ( iv = 0; iv <= v; iv++ )
		{
			TqInt iu;
			for ( iu = 0; iu <= u; iu++ )
			{
				for( arrayIndex = 0; arrayIndex < this->Count(); arrayIndex++ )
				{
					res = BilinearEvaluate<T>( pValue( 0 ) [ arrayIndex ],
								   pValue( 1 ) [ arrayIndex ],
								   pValue( 2 ) [ arrayIndex ],
								   pValue( 3 ) [ arrayIndex ],
								   iu * diu, iv * div );
					( *(pResData[arrayIndex])++ ) = res;
				}
			}
		}
	}
}

template <class T, EqVariableType I, class SLT>
void CqParameterTypedVaryingArray<T, I, SLT>::CopyToShaderVariable( IqShaderData* pResult )
{
	SLT* pResData;
	pResult->GetValuePtr( pResData );
	assert( NULL != pResData );

	TqUint iu;
	for ( iu = 0; iu <= pResult->Size(); iu++ )
		( *pResData++ ) = pValue(iu)[0];
}

/** Dice the value into a grid using bilinear interpolation.
 * \param u Integer dice count for the u direction.
 * \param v Integer dice count for the v direction.
 * \param pResult Pointer to storage for the result.
 * \param pSurface Pointer to the surface to which this parameter belongs. Used if the surface type has special handling for parameter dicing.
 */

template <class T, EqVariableType I, class SLT>
void CqParameterTypedVaryingArray<T, I, SLT>::DiceOne( TqInt u, TqInt v, IqShaderData* pResult, IqSurface* pSurface, TqInt ArrayIndex )
{
	assert( pResult->Type() == this->Type() );
	assert( pResult->Class() == class_varying );
	assert( this->Count() > ArrayIndex );

	T res;

	SLT* pResData;
	pResult->GetValuePtr( pResData );
	assert( NULL != pResData );

	// Check if a valid 4 point quad, do nothing if not.
	if ( m_aValues.size() == 4 )
	{
		// Note it is assumed that the variable has been
		// initialised to the correct size prior to calling.
		TqFloat diu = 1.0 / u;
		TqFloat div = 1.0 / v;
		TqInt iv;
		for ( iv = 0; iv <= v; iv++ )
		{
			TqInt iu;
			for ( iu = 0; iu <= u; iu++ )
			{
				res = BilinearEvaluate<T>( pValue( 0 ) [ ArrayIndex ],
				                           pValue( 1 ) [ ArrayIndex ],
				                           pValue( 2 ) [ ArrayIndex ],
				                           pValue( 3 ) [ ArrayIndex ],
				                           iu * diu, iv * div );
				( *pResData++ ) = res;
			}
		}
	}
}

//-----------------------

template <class T, EqVariableType I, class SLT>
CqParameterTypedUniformArray<T,I,SLT>::CqParameterTypedUniformArray( const char* strName, TqInt Count ) :
		CqParameterTyped<T, SLT>( strName, Count )
{
	m_aValues.resize( Count );
}

template <class T, EqVariableType I, class SLT>
CqParameterTypedUniformArray<T,I,SLT>::~CqParameterTypedUniformArray()
{}

template <class T, EqVariableType I, class SLT>
CqParameter* CqParameterTypedUniformArray<T,I,SLT>::CloneType( const char* Name, TqInt Count ) const
{
	return ( new CqParameterTypedUniformArray<T, I, SLT>( Name, Count ) );
}

template <class T, EqVariableType I, class SLT>
CqParameter* CqParameterTypedUniformArray<T,I,SLT>::Clone() const
{
	CqParameterTypedUniformArray<T, I, SLT> *clone = new CqParameterTypedUniformArray<T, I, SLT>( this->strName().c_str(), this->Count() );
	clone->m_aValues.resize( m_aValues.size() );
	TqInt i2 = 0;
	for (typename  std::vector<T>::const_iterator i = m_aValues.begin(); i != m_aValues.end(); i++, i2++ )
		clone->m_aValues[ i2 ] = ( *i );
	return (clone);
}

template <class T, EqVariableType I, class SLT>
EqVariableClass	CqParameterTypedUniformArray<T,I,SLT>::Class() const
{
	return ( class_uniform );
}

template <class T, EqVariableType I, class SLT>
EqVariableType CqParameterTypedUniformArray<T,I,SLT>::Type() const
{
	return ( I );
}

template <class T, EqVariableType I, class SLT>
void CqParameterTypedUniformArray<T,I,SLT>::SetSize( TqInt size )
{}

template <class T, EqVariableType I, class SLT>
TqUint CqParameterTypedUniformArray<T,I,SLT>::Size() const
{
	return ( 1 );
}

template <class T, EqVariableType I, class SLT>
void CqParameterTypedUniformArray<T,I,SLT>::Clear()
{}

template <class T, EqVariableType I, class SLT>
void CqParameterTypedUniformArray<T,I,SLT>::Subdivide( CqParameter* pResult1, CqParameter* pResult2, bool u, IqSurface* pSurface )
{}

template <class T, EqVariableType I, class SLT>
void CqParameterTypedUniformArray<T,I,SLT>::Dice( TqInt u, TqInt v, IqShaderData* pResult, IqSurface* pSurface )
{
	// Just promote the uniform value to varying by duplication.
	assert( pResult->Type() == this->Type() );
	// Note it is assumed that the variable has been
	// initialised to the correct size prior to calling.
	TqUint i; 
	TqInt  j;
	TqUint max = ( MAX( (TqUint)u * (TqUint) v, pResult->Size() ) );
	for ( i = 0; i < max; ++i )
		for( j = 0; j < this->Count(); ++j )
			pResult->SetValue( pValue( 0 ) [ j ], i );
}

template <class T, EqVariableType I, class SLT>
void CqParameterTypedUniformArray<T,I,SLT>::CopyToShaderVariable( IqShaderData* pResult )
{
	// Just promote the uniform value to varying by duplication.
	assert( pResult->Type() == this->Type() );
	// Note it is assumed that the variable has been
	// initialised to the correct size prior to calling.
	TqUint i;
	TqUint max = pResult->Size();
	for ( i = 0; i < max; i++ )
		pResult->SetValue( pValue( 0 ) [ 0 ], i );
}

template <class T, EqVariableType I, class SLT>
void CqParameterTypedUniformArray<T,I,SLT>::DiceOne( TqInt u, TqInt v, IqShaderData* pResult, IqSurface* pSurface, TqInt ArrayIndex )
{
	// Just promote the uniform value to varying by duplication.
	assert( pResult->Type() == this->Type() );
	assert( this->Count() > ArrayIndex );
	// Note it is assumed that the variable has been
	// initialised to the correct size prior to calling.
	TqUint i;
	TqUint max = ( MAX( (TqUint)u * (TqUint) v, pResult->Size() ) );
	for ( i = 0; i < max; i++ )
		pResult->SetValue( pValue( 0 ) [ ArrayIndex ], i );
}

template <class T, EqVariableType I, class SLT>
const T* CqParameterTypedUniformArray<T,I,SLT>::pValue() const
{
	assert( 0 < m_aValues.size() );
	return ( &m_aValues[ 0 ] );
}

template <class T, EqVariableType I, class SLT>
T* CqParameterTypedUniformArray<T,I,SLT>::pValue()
{
	assert( 0 < m_aValues.size() );
	return ( &m_aValues[ 0 ] );
}

template <class T, EqVariableType I, class SLT>
const T* CqParameterTypedUniformArray<T,I,SLT>::pValue( const TqInt Index ) const
{
	assert( 0 < m_aValues.size() );
	return ( &m_aValues[ 0 ] );
}

template <class T, EqVariableType I, class SLT>
T* CqParameterTypedUniformArray<T,I,SLT>::pValue( const TqInt Index )
{
	assert( 0 < m_aValues.size() );
	return ( &m_aValues[ 0 ] );
}

template <class T, EqVariableType I, class SLT>
CqParameter* CqParameterTypedUniformArray<T,I,SLT>::Create( const char* strName, TqInt Count )
{
	return ( new CqParameterTypedUniformArray<T, I, SLT>( strName, Count ) );
}

//--------------------

template <class T, EqVariableType I, class SLT>
CqParameterTypedConstantArray<T,I,SLT>::CqParameterTypedConstantArray( const char* strName, TqInt Count ) :
		CqParameterTyped<T, SLT>( strName, Count )
{
	m_aValues.resize( Count );
}

template <class T, EqVariableType I, class SLT>
CqParameterTypedConstantArray<T,I,SLT>::~CqParameterTypedConstantArray()
{}

template <class T, EqVariableType I, class SLT>
CqParameter* CqParameterTypedConstantArray<T,I,SLT>::CloneType( const char* Name, TqInt Count ) const
{
	return ( new CqParameterTypedConstantArray<T, I, SLT>( Name, Count ) );
}

template <class T, EqVariableType I, class SLT>
CqParameter* CqParameterTypedConstantArray<T,I,SLT>::Clone() const
{
	CqParameterTypedConstantArray<T, I, SLT>*clone = new CqParameterTypedConstantArray<T, I, SLT>( this->strName().c_str(), this->Count() );
	clone->m_aValues.resize( m_aValues.size() );
	TqInt i2 = 0;
	for ( typename std::vector<T>::const_iterator i = m_aValues.begin(); i != m_aValues.end(); i++, i2++ )
		clone->m_aValues[ i2 ] = ( *i );
	return ( clone );
}

template <class T, EqVariableType I, class SLT>
EqVariableClass	CqParameterTypedConstantArray<T,I,SLT>::Class() const
{
	return ( class_constant );
}

template <class T, EqVariableType I, class SLT>
EqVariableType CqParameterTypedConstantArray<T,I,SLT>::Type() const
{
	return ( I );
}

template <class T, EqVariableType I, class SLT>
void CqParameterTypedConstantArray<T,I,SLT>::SetSize( TqInt size )
{}

template <class T, EqVariableType I, class SLT>
TqUint CqParameterTypedConstantArray<T,I,SLT>::Size() const
{
	return ( 1 );
}

template <class T, EqVariableType I, class SLT>
void CqParameterTypedConstantArray<T,I,SLT>::Clear()
{}

template <class T, EqVariableType I, class SLT>
void CqParameterTypedConstantArray<T,I,SLT>::Subdivide( CqParameter* pResult1, CqParameter* pResult2, bool u, IqSurface* pSurface )
{}

template <class T, EqVariableType I, class SLT>
void CqParameterTypedConstantArray<T,I,SLT>::Dice( TqInt u, TqInt v, IqShaderData* pResult, IqSurface* pSurface )
{
	// Just promote the constant value to varying by duplication.
	assert( pResult->Type() == this->Type() );
	// Note it is assumed that the variable has been
	// initialised to the correct size prior to calling.
	TqUint i; 
	TqInt  j;
	TqUint max = ( MAX( (TqUint) u * (TqUint) v, pResult->Size() ) );
	for ( i = 0; i < max; ++i )
		for( j = 0; j < this->Count(); ++j )
			pResult->SetValue( pValue( 0 ) [ j ], i );
}

template <class T, EqVariableType I, class SLT>
void CqParameterTypedConstantArray<T,I,SLT>::CopyToShaderVariable( IqShaderData* pResult )
{
	// Just promote the constant value to varying by duplication.
	assert( pResult->Type() == this->Type() );
	// Note it is assumed that the variable has been
	// initialised to the correct size prior to calling.
	TqUint i;
	TqUint max = pResult->Size();
	for ( i = 0; i < max; i++ )
		pResult->SetValue( pValue( 0 ) [ 0 ], i );
}

template <class T, EqVariableType I, class SLT>
void CqParameterTypedConstantArray<T,I,SLT>::DiceOne( TqInt u, TqInt v, IqShaderData* pResult, IqSurface* pSurface, TqInt ArrayIndex )
{
	// Just promote the constant value to varying by duplication.
	assert( pResult->Type() == this->Type() );
	assert( this->Count() > ArrayIndex );
	// Note it is assumed that the variable has been
	// initialised to the correct size prior to calling.
	TqUint i;
	TqUint max = ( MAX( (TqUint) u * (TqUint) v, pResult->Size() ) );
	for ( i = 0; i < max; i++ )
		pResult->SetValue( pValue( 0 ) [ ArrayIndex ], i );
}

template <class T, EqVariableType I, class SLT>
const T* CqParameterTypedConstantArray<T,I,SLT>::pValue() const
{
	assert( 0 < m_aValues.size() );
	return ( &m_aValues[ 0 ] );
}

template <class T, EqVariableType I, class SLT>
T* CqParameterTypedConstantArray<T,I,SLT>::pValue()
{
	assert( 0 < m_aValues.size() );
	return ( &m_aValues[ 0 ] );
}

template <class T, EqVariableType I, class SLT>
const T* CqParameterTypedConstantArray<T,I,SLT>::pValue( const TqInt Index ) const
{
	assert( 0 < m_aValues.size() );
	return ( &m_aValues[ 0 ] );
}

template <class T, EqVariableType I, class SLT>
T* CqParameterTypedConstantArray<T,I,SLT>::pValue( const TqInt Index )
{
	assert( 0 < m_aValues.size() );
	return ( &m_aValues[ 0 ] );
}


template <class T, EqVariableType I, class SLT>
CqParameter* CqParameterTypedConstantArray<T,I,SLT>::Create( const char* strName, TqInt Count )
{
	return ( new CqParameterTypedConstantArray<T, I, SLT>( strName, Count ) );
}

//-----------------

template <class T, EqVariableType I, class SLT>
CqParameterTypedVertexArray<T,I,SLT>::CqParameterTypedVertexArray( const char* strName, TqInt Count ) :
		CqParameterTypedVaryingArray<T, I, SLT>( strName, Count )
{}

template <class T, EqVariableType I, class SLT>
CqParameterTypedVertexArray<T,I,SLT>::~CqParameterTypedVertexArray()
{}

template <class T, EqVariableType I, class SLT>
CqParameter* CqParameterTypedVertexArray<T,I,SLT>::CloneType( const char* Name, TqInt Count ) const
{
	return ( new CqParameterTypedVertexArray<T, I, SLT>( Name, Count ) );
}

template <class T, EqVariableType I, class SLT>
CqParameter* CqParameterTypedVertexArray<T,I,SLT>::Clone() const
{
	CqParameterTypedVertexArray<T, I, SLT>* clone = new CqParameterTypedVertexArray<T, I, SLT>( this->strName().c_str(), this->Count() );
	clone->m_aValues.resize( this->m_aValues.size(), std::vector<T>(this->Count()) );
	clone->m_Count = this->Count();
	TqUint j;
	for ( j = 0; j < this->m_aValues.size(); j++ )
	{
		TqUint i;
		for ( i = 0; i < (TqUint) this->Count(); i++ )
			clone->m_aValues[ j ][ i ] = this->m_aValues[ j ][ i ];
	}
	return(clone);
}

template <class T, EqVariableType I, class SLT>
EqVariableClass	CqParameterTypedVertexArray<T,I,SLT>::Class() const
{
	return ( class_vertex );
}

template <class T, EqVariableType I, class SLT>
void CqParameterTypedVertexArray<T,I,SLT>::Dice( TqInt u, TqInt v, IqShaderData* pResult, IqSurface* pSurface )
{
	// Just promote the uniform value to varying by duplication.
	assert( pResult->Type() == this->Type() );
	assert( NULL != pSurface );
	// Note it is assumed that the variable has been
	// initialised to the correct size prior to calling.
	pSurface->NaturalDice( this, u, v, pResult );
}

template <class T, EqVariableType I, class SLT>
void CqParameterTypedVertexArray<T,I,SLT>::CopyToShaderVariable( IqShaderData* pResult )
{
	// Just promote the uniform value to varying by duplication.
	assert( pResult->Type() == this->Type() );
	// Note it is assumed that the variable has been
	// initialised to the correct size prior to calling.
	TqUint i;
	TqUint max = pResult->Size();
	for ( i = 0; i < max; i++ )
		pResult->SetValue( this->pValue( 0 ) [ 0 ], i );
}

template <class T, EqVariableType I, class SLT>
void CqParameterTypedVertexArray<T,I,SLT>::DiceOne( TqInt u, TqInt v, IqShaderData* pResult, IqSurface* pSurface, TqInt ArrayIndex )
{
	/// \note: Need to work out how to do this...
	return;
}

template <class T, EqVariableType I, class SLT>
CqParameter* CqParameterTypedVertexArray<T,I,SLT>::Create( const char* strName, TqInt Count )
{
	return ( new CqParameterTypedVertexArray<T, I, SLT>( strName, Count ) );
}

//-------------------

template <class T, EqVariableType I, class SLT>
CqParameterTypedFaceVaryingArray<T,I,SLT>::CqParameterTypedFaceVaryingArray( const char* strName, TqInt Count ) :
		CqParameterTypedVaryingArray<T, I, SLT>( strName, Count )
{}

template <class T, EqVariableType I, class SLT>
CqParameterTypedFaceVaryingArray<T,I,SLT>::~CqParameterTypedFaceVaryingArray()
{}

template <class T, EqVariableType I, class SLT>
CqParameter* CqParameterTypedFaceVaryingArray<T,I,SLT>::CloneType( const char* Name, TqInt Count ) const
{
	return ( new CqParameterTypedFaceVaryingArray<T, I, SLT>( Name, Count ) );
}

template <class T, EqVariableType I, class SLT>
CqParameter* CqParameterTypedFaceVaryingArray<T,I,SLT>::Clone() const
{
	CqParameterTypedFaceVaryingArray<T, I, SLT>* clone = new CqParameterTypedFaceVaryingArray<T, I, SLT>( this->strName().c_str(), this->Count() );
	clone->m_aValues.resize( this->m_aValues.size(), std::vector<T>(this->Count()) );
	clone->m_Count = this->Count();
	TqUint j;
	for ( j = 0; j < this->m_aValues.size(); j++ )
	{
		TqUint i;
		for ( i = 0; i < (TqUint) this->Count(); i++ )
			clone->m_aValues[ j ][ i ] = this->m_aValues[ j ][ i ];
	}
	return(clone);
}

template <class T, EqVariableType I, class SLT>
EqVariableClass CqParameterTypedFaceVaryingArray<T,I,SLT>::Class() const
{
	return ( class_facevarying );
}

template <class T, EqVariableType I, class SLT>
CqParameter* CqParameterTypedFaceVaryingArray<T,I,SLT>::Create( const char* strName, TqInt Count )
{
	return ( new CqParameterTypedFaceVaryingArray<T, I, SLT>( strName, Count ) );
}

//----------------------

template <class T, EqVariableType I, class SLT>
CqParameterTypedFaceVertexArray<T,I,SLT>::CqParameterTypedFaceVertexArray( const char* strName, TqInt Count ) :
		CqParameterTypedVertexArray<T, I, SLT>( strName, Count )
{}

template <class T, EqVariableType I, class SLT>
CqParameterTypedFaceVertexArray<T,I,SLT>::~CqParameterTypedFaceVertexArray()
{}

template <class T, EqVariableType I, class SLT>
CqParameter* CqParameterTypedFaceVertexArray<T,I,SLT>::CloneType( const char* Name, TqInt Count ) const
{
	return ( new CqParameterTypedFaceVertexArray<T, I, SLT>( Name, Count ) );
}

template <class T, EqVariableType I, class SLT>
CqParameter* CqParameterTypedFaceVertexArray<T,I,SLT>::Clone() const
{
	CqParameterTypedFaceVertexArray<T, I, SLT>* clone = new CqParameterTypedFaceVertexArray<T, I, SLT>( this->strName().c_str(), this->Count() );
	clone->m_aValues.resize( this->m_aValues.size(), std::vector<T>(this->Count()) );
	clone->m_Count = this->Count();
	TqUint j;
	for ( j = 0; j < this->m_aValues.size(); j++ )
	{
		TqUint i;
		for ( i = 0; i < (TqUint) this->Count(); i++ )
			clone->m_aValues[ j ][ i ] = this->m_aValues[ j ][ i ];
	}
	return(clone);
}

template <class T, EqVariableType I, class SLT>
EqVariableClass	CqParameterTypedFaceVertexArray<T,I,SLT>::Class() const
{
	return ( class_facevertex );
}

template <class T, EqVariableType I, class SLT>
CqParameter* CqParameterTypedFaceVertexArray<T,I,SLT>::Create( const char* strName, TqInt Count )
{
	return ( new CqParameterTypedFaceVertexArray<T, I, SLT>( strName, Count ) );
}

//-----------------------

inline CqNamedParameterList::CqNamedParameterList( const char* strName ) : m_strName( strName )
{
	m_hash = CqString::hash( strName );
}

inline CqNamedParameterList::~CqNamedParameterList()
{
	for ( std::vector<CqParameter*>::iterator i = m_aParameters.begin(); i != m_aParameters.end(); i++ )
		delete( ( *i ) );
}

#ifdef _DEBUG
inline CqString CqNamedParameterList::className() const
{
	return CqString("CqNamedParameterList");
}
#endif

inline const CqString& CqNamedParameterList::strName() const
{
	return ( m_strName );
}

inline void CqNamedParameterList::AddParameter( const CqParameter* pParameter )
{
	for ( std::vector<CqParameter*>::iterator i = m_aParameters.begin(); i != m_aParameters.end(); i++ )
	{
		if ( ( *i ) ->hash() == pParameter->hash() )
		{
			delete( *i );
			( *i ) = const_cast<CqParameter*>( pParameter );
			return ;
		}
	}
	// If not append it.
	m_aParameters.push_back( const_cast<CqParameter*>( pParameter ) );
}

inline const CqParameter* CqNamedParameterList::pParameter( const char* strName ) const
{
	TqUlong hash = CqString::hash( strName );
	for ( std::vector<CqParameter*>::const_iterator i = m_aParameters.begin(); i != m_aParameters.end(); i++ )
		if ( ( *i ) ->hash() == hash )
			return ( *i );
	return ( 0 );
}

inline CqParameter* CqNamedParameterList::pParameter( const char* strName )
{
	TqUlong hash = CqString::hash( strName );
	for ( std::vector<CqParameter*>::iterator i = m_aParameters.begin(); i != m_aParameters.end(); i++ )
		if ( ( *i ) ->hash() == hash )
			return ( *i );
	return ( 0 );
}

inline TqUlong CqNamedParameterList::hash()
{
	return m_hash;
}

///////////////////////////////////////////////////////////////////////////////
	typedef CqParameterTyped<TqFloat, TqFloat> CqFloatParameter;
	typedef boost::shared_ptr<CqFloatParameter> CqFloatParameterPtr;
	
	typedef CqParameterTyped<TqInt, TqFloat> CqIntParameter;
	typedef boost::shared_ptr<CqIntParameter> CqIntParameterPtr;

	typedef CqParameterTyped<CqVector3D, CqVector3D> CqPointParameter;
	typedef boost::shared_ptr<CqPointParameter> CqPointParameterPtr;

	typedef CqParameterTyped<CqString, CqString> CqStringParameter;
	typedef boost::shared_ptr<CqStringParameter> CqStringParameterPtr;

	typedef CqParameterTyped<CqColor, CqColor> CqColorParameter;
	typedef boost::shared_ptr<CqColorParameter> CqColorParameterPtr;

	typedef CqParameterTyped<CqVector4D, CqVector3D> CqHPointParameter;
	typedef boost::shared_ptr<CqHPointParameter> CqHPointParameterPtr;

	typedef CqParameterTyped<CqVector3D, CqVector3D> CqNormalParameter;
	typedef boost::shared_ptr<CqNormalParameter> CqNormalParameterPtr;

	typedef CqParameterTyped<CqVector3D, CqVector3D> CqVectorParameter;
	typedef boost::shared_ptr<CqVectorParameter> CqVectorParameterPtr;

	typedef CqParameterTyped<CqMatrix, CqMatrix> CqMatrixParameter;
	typedef boost::shared_ptr<CqMatrixParameter> CqMatrixParameterPtr;

// Typedefs for the constants
	typedef CqParameterTypedConstant<TqFloat, type_float, TqFloat> CqFloatConstantParameter;
	typedef boost::shared_ptr<CqFloatConstantParameter> CqFloatConstantParameterPtr;
	
	typedef CqParameterTypedConstant<TqInt, type_integer, TqFloat> CqIntConstantParameter;
	typedef boost::shared_ptr<CqIntConstantParameter> CqIntConstantParameterPtr;

	typedef CqParameterTypedConstant<CqVector3D, type_point, CqVector3D> CqPointConstantParameter;
	typedef boost::shared_ptr<CqPointConstantParameter> CqPointConstantParameterPtr;

	typedef CqParameterTypedConstant<CqString, type_string, CqString> CqStringConstantParameter;
	typedef boost::shared_ptr<CqStringConstantParameter> CqStringConstantParameterPtr;

	typedef CqParameterTypedConstant<CqColor, type_color, CqColor> CqColorConstantParameter;
	typedef boost::shared_ptr<CqColorConstantParameter> CqColorConstantParameterPtr;

	typedef CqParameterTypedConstant<CqVector4D, type_hpoint, CqVector3D> CqHPointConstantParameter;
	typedef boost::shared_ptr<CqHPointConstantParameter> CqHPointConstantParameterPtr;

	typedef CqParameterTypedConstant<CqVector3D, type_normal, CqVector3D> CqNormalConstantParameter;
	typedef boost::shared_ptr<CqNormalConstantParameter> CqNormalConstantParameterPtr;

	typedef CqParameterTypedConstant<CqVector3D, type_vector, CqVector3D> CqVectorConstantParameter;
	typedef boost::shared_ptr<CqVectorConstantParameter> CqVectorConstantParameterPtr;

	typedef CqParameterTypedConstant<CqMatrix, type_matrix, CqMatrix> CqMatrixConstantParameter;
	typedef boost::shared_ptr<CqMatrixConstantParameter> CqMatrixConstantParameterPtr;

// Uniforms
	typedef CqParameterTypedUniform<TqFloat, type_float, TqFloat> CqFloatUniformParameter;
	typedef boost::shared_ptr<CqFloatUniformParameter> CqFloatUniformParameterPtr;
	
	typedef CqParameterTypedUniform<TqInt, type_integer, TqFloat> CqIntUniformParameter;
	typedef boost::shared_ptr<CqIntUniformParameter> CqIntUniformParameterPtr;

	typedef CqParameterTypedUniform<CqVector3D, type_point, CqVector3D> CqPointUniformParameter;
	typedef boost::shared_ptr<CqPointUniformParameter> CqPointUniformParameterPtr;

	typedef CqParameterTypedUniform<CqString, type_string, CqString> CqStringUniformParameter;
	typedef boost::shared_ptr<CqStringUniformParameter> CqStringUniformParameterPtr;

	typedef CqParameterTypedUniform<CqColor, type_color, CqColor> CqColorUniformParameter;
	typedef boost::shared_ptr<CqColorUniformParameter> CqColorUniformParameterPtr;

	typedef CqParameterTypedUniform<CqVector4D, type_hpoint, CqVector3D> CqHPointUniformParameter;
	typedef boost::shared_ptr<CqHPointUniformParameter> CqHPointUniformParameterPtr;

	typedef CqParameterTypedUniform<CqVector3D, type_normal, CqVector3D> CqNormalUniformParameter;
	typedef boost::shared_ptr<CqNormalUniformParameter> CqNormalUniformParameterPtr;

	typedef CqParameterTypedUniform<CqVector3D, type_vector, CqVector3D> CqVectorUniformParameter;
	typedef boost::shared_ptr<CqVectorUniformParameter> CqVectorUniformParameterPtr;

	typedef CqParameterTypedUniform<CqMatrix, type_matrix, CqMatrix> CqMatrixUniformParameter;
	typedef boost::shared_ptr<CqMatrixUniformParameter> CqMatrixUniformParameterPtr;

// Typedefs for Varying
	typedef CqParameterTypedVarying<TqFloat, type_float, TqFloat> CqFloatVaryingParameter;
	typedef boost::shared_ptr<CqFloatVaryingParameter> CqFloatVaryingParameterPtr;
	
	typedef CqParameterTypedVarying<TqInt, type_integer, TqFloat> CqIntVaryingParameter;
	typedef boost::shared_ptr<CqIntVaryingParameter> CqIntVaryingParameterPtr;

	typedef CqParameterTypedVarying<CqVector3D, type_point, CqVector3D> CqPointVaryingParameter;
	typedef boost::shared_ptr<CqPointVaryingParameter> CqPointVaryingParameterPtr;

	typedef CqParameterTypedVarying<CqString, type_string, CqString> CqStringVaryingParameter;
	typedef boost::shared_ptr<CqStringVaryingParameter> CqStringVaryingParameterPtr;

	typedef CqParameterTypedVarying<CqColor, type_color, CqColor> CqColorVaryingParameter;
	typedef boost::shared_ptr<CqColorVaryingParameter> CqColorVaryingParameterPtr;

	typedef CqParameterTypedVarying<CqVector4D, type_hpoint, CqVector3D> CqHPointVaryingParameter;
	typedef boost::shared_ptr<CqHPointVaryingParameter> CqHPointVaryingParameterPtr;

	typedef CqParameterTypedVarying<CqVector3D, type_normal, CqVector3D> CqNormalVaryingParameter;
	typedef boost::shared_ptr<CqNormalVaryingParameter> CqNormalVaryingParameterPtr;

	typedef CqParameterTypedVarying<CqVector3D, type_vector, CqVector3D> CqVectorVaryingParameter;
	typedef boost::shared_ptr<CqVectorVaryingParameter> CqVectorVaryingParameterPtr;

	typedef CqParameterTypedVarying<CqMatrix, type_matrix, CqMatrix> CqMatrixVaryingParameter;
	typedef boost::shared_ptr<CqMatrixVaryingParameter> CqMatrixVaryingParameterPtr;
	
// Vertex
	typedef CqParameterTypedVertex<TqFloat, type_float, TqFloat> CqFloatVertexParameter;
	typedef boost::shared_ptr<CqFloatVertexParameter> CqFloatVertexParameterPtr;
	
	typedef CqParameterTypedVertex<TqInt, type_integer, TqFloat> CqIntVertexParameter;
	typedef boost::shared_ptr<CqIntVertexParameter> CqIntVertexParameterPtr;

	typedef CqParameterTypedVertex<CqVector3D, type_point, CqVector3D> CqPointVertexParameter;
	typedef boost::shared_ptr<CqPointVertexParameter> CqPointVertexParameterPtr;

	typedef CqParameterTypedVertex<CqString, type_string, CqString> CqStringVertexParameter;
	typedef boost::shared_ptr<CqStringVertexParameter> CqStringVertexParameterPtr;

	typedef CqParameterTypedVertex<CqColor, type_color, CqColor> CqColorVertexParameter;
	typedef boost::shared_ptr<CqColorVertexParameter> CqColorVertexParameterPtr;

	typedef CqParameterTypedVertex<CqVector4D, type_hpoint, CqVector3D> CqHPointVertexParameter;
	typedef boost::shared_ptr<CqHPointVertexParameter> CqHPointVertexParameterPtr;

	typedef CqParameterTypedVertex<CqVector3D, type_normal, CqVector3D> CqNormalVertexParameter;
	typedef boost::shared_ptr<CqNormalVertexParameter> CqNormalVertexParameterPtr;

	typedef CqParameterTypedVertex<CqVector3D, type_vector, CqVector3D> CqVectorVertexParameter;
	typedef boost::shared_ptr<CqVectorVertexParameter> CqVectorVertexParameterPtr;

	typedef CqParameterTypedVertex<CqMatrix, type_matrix, CqMatrix> CqMatrixVertexParameter;
	typedef boost::shared_ptr<CqMatrixVertexParameter> CqMatrixVertexParameterPtr;

// FaceVarying
	typedef CqParameterTypedFaceVarying<TqFloat, type_float, TqFloat> CqFloatFaceVaryingParameter;
	typedef boost::shared_ptr<CqFloatFaceVaryingParameter> CqFloatFaceVaryingParameterPtr;
	
	typedef CqParameterTypedFaceVarying<TqInt, type_integer, TqFloat> CqIntFaceVaryingParameter;
	typedef boost::shared_ptr<CqIntFaceVaryingParameter> CqIntFaceVaryingParameterPtr;

	typedef CqParameterTypedFaceVarying<CqVector3D, type_point, CqVector3D> CqPointFaceVaryingParameter;
	typedef boost::shared_ptr<CqPointFaceVaryingParameter> CqPointFaceVaryingParameterPtr;

	typedef CqParameterTypedFaceVarying<CqString, type_string, CqString> CqStringFaceVaryingParameter;
	typedef boost::shared_ptr<CqStringFaceVaryingParameter> CqStringFaceVaryingParameterPtr;

	typedef CqParameterTypedFaceVarying<CqColor, type_color, CqColor> CqColorFaceVaryingParameter;
	typedef boost::shared_ptr<CqColorFaceVaryingParameter> CqColorFaceVaryingParameterPtr;

	typedef CqParameterTypedFaceVarying<CqVector4D, type_hpoint, CqVector3D> CqHPointFaceVaryingParameter;
	typedef boost::shared_ptr<CqHPointFaceVaryingParameter> CqHPointFaceVaryingParameterPtr;

	typedef CqParameterTypedFaceVarying<CqVector3D, type_normal, CqVector3D> CqNormalFaceVaryingParameter;
	typedef boost::shared_ptr<CqNormalFaceVaryingParameter> CqNormalFaceVaryingParameterPtr;

	typedef CqParameterTypedFaceVarying<CqVector3D, type_vector, CqVector3D> CqVectorFaceVaryingParameter;
	typedef boost::shared_ptr<CqVectorFaceVaryingParameter> CqVectorFaceVaryingParameterPtr;

	typedef CqParameterTypedFaceVarying<CqMatrix, type_matrix, CqMatrix> CqMatrixFaceVaryingParameter;
	typedef boost::shared_ptr<CqMatrixFaceVaryingParameter> CqMatrixFaceVaryingParameterPtr;

// Constant Array
	typedef CqParameterTypedConstantArray<TqFloat, type_float, TqFloat> CqFloatConstantArrayParameter;
	typedef boost::shared_ptr<CqFloatConstantArrayParameter> CqFloatConstantArrayParameterPtr;
	
	typedef CqParameterTypedConstantArray<TqInt, type_integer, TqFloat> CqIntConstantArrayParameter;
	typedef boost::shared_ptr<CqIntConstantArrayParameter> CqIntConstantArrayParameterPtr;

	typedef CqParameterTypedConstantArray<CqVector3D, type_point, CqVector3D> CqPointConstantArrayParameter;
	typedef boost::shared_ptr<CqPointConstantArrayParameter> CqPointConstantArrayParameterPtr;

	typedef CqParameterTypedConstantArray<CqString, type_string, CqString> CqStringConstantArrayParameter;
	typedef boost::shared_ptr<CqStringConstantArrayParameter> CqStringConstantArrayParameterPtr;

	typedef CqParameterTypedConstantArray<CqColor, type_color, CqColor> CqColorConstantArrayParameter;
	typedef boost::shared_ptr<CqColorConstantArrayParameter> CqColorConstantArrayParameterPtr;

	typedef CqParameterTypedConstantArray<CqVector4D, type_hpoint, CqVector3D> CqHPointConstantArrayParameter;
	typedef boost::shared_ptr<CqHPointConstantArrayParameter> CqHPointConstantArrayParameterPtr;

	typedef CqParameterTypedConstantArray<CqVector3D, type_normal, CqVector3D> CqNormalConstantArrayParameter;
	typedef boost::shared_ptr<CqNormalConstantArrayParameter> CqNormalConstantArrayParameterPtr;

	typedef CqParameterTypedConstantArray<CqVector3D, type_vector, CqVector3D> CqVectorConstantArrayParameter;
	typedef boost::shared_ptr<CqVectorConstantArrayParameter> CqVectorConstantArrayParameterPtr;

	typedef CqParameterTypedConstantArray<CqMatrix, type_matrix, CqMatrix> CqMatrixConstantArrayParameter;
	typedef boost::shared_ptr<CqMatrixConstantArrayParameter> CqMatrixConstantArrayParameterPtr;

// Uniform array
	typedef CqParameterTypedUniformArray<TqFloat, type_float, TqFloat> CqFloatUniformArrayParameter;
	typedef boost::shared_ptr<CqFloatUniformArrayParameter> CqFloatUniformArrayParameterPtr;
	
	typedef CqParameterTypedUniformArray<TqInt, type_integer, TqFloat> CqIntUniformArrayParameter;
	typedef boost::shared_ptr<CqIntUniformArrayParameter> CqIntUniformArrayParameterPtr;

	typedef CqParameterTypedUniformArray<CqVector3D, type_point, CqVector3D> CqPointUniformArrayParameter;
	typedef boost::shared_ptr<CqPointUniformArrayParameter> CqPointUniformArrayParameterPtr;

	typedef CqParameterTypedUniformArray<CqString, type_string, CqString> CqStringUniformArrayParameter;
	typedef boost::shared_ptr<CqStringUniformArrayParameter> CqStringUniformArrayParameterPtr;

	typedef CqParameterTypedUniformArray<CqColor, type_color, CqColor> CqColorUniformArrayParameter;
	typedef boost::shared_ptr<CqColorUniformArrayParameter> CqColorUniformArrayParameterPtr;

	typedef CqParameterTypedUniformArray<CqVector4D, type_hpoint, CqVector3D> CqHPointUniformArrayParameter;
	typedef boost::shared_ptr<CqHPointUniformArrayParameter> CqHPointUniformArrayParameterPtr;

	typedef CqParameterTypedUniformArray<CqVector3D, type_normal, CqVector3D> CqNormalUniformArrayParameter;
	typedef boost::shared_ptr<CqNormalUniformArrayParameter> CqNormalUniformArrayParameterPtr;

	typedef CqParameterTypedUniformArray<CqVector3D, type_vector, CqVector3D> CqVectorUniformArrayParameter;
	typedef boost::shared_ptr<CqVectorUniformArrayParameter> CqVectorUniformArrayParameterPtr;

	typedef CqParameterTypedUniformArray<CqMatrix, type_matrix, CqMatrix> CqMatrixUniformArrayParameter;
	typedef boost::shared_ptr<CqMatrixUniformArrayParameter> CqMatrixUniformArrayParameterPtr;

// Varying array
	typedef CqParameterTypedVaryingArray<TqFloat, type_float, TqFloat> CqFloatVaryingArrayParameter;
	typedef boost::shared_ptr<CqFloatVaryingArrayParameter> CqFloatVaryingArrayParameterPtr;
	
	typedef CqParameterTypedVaryingArray<TqInt, type_integer, TqFloat> CqIntVaryingArrayParameter;
	typedef boost::shared_ptr<CqIntVaryingArrayParameter> CqIntVaryingArrayParameterPtr;

	typedef CqParameterTypedVaryingArray<CqVector3D, type_point, CqVector3D> CqPointVaryingArrayParameter;
	typedef boost::shared_ptr<CqPointVaryingArrayParameter> CqPointVaryingArrayParameterPtr;

	typedef CqParameterTypedVaryingArray<CqString, type_string, CqString> CqStringVaryingArrayParameter;
	typedef boost::shared_ptr<CqStringVaryingArrayParameter> CqStringVaryingArrayParameterPtr;

	typedef CqParameterTypedVaryingArray<CqColor, type_color, CqColor> CqColorVaryingArrayParameter;
	typedef boost::shared_ptr<CqColorVaryingArrayParameter> CqColorVaryingArrayParameterPtr;

	typedef CqParameterTypedVaryingArray<CqVector4D, type_hpoint, CqVector3D> CqHPointVaryingArrayParameter;
	typedef boost::shared_ptr<CqHPointVaryingArrayParameter> CqHPointVaryingArrayParameterPtr;

	typedef CqParameterTypedVaryingArray<CqVector3D, type_normal, CqVector3D> CqNormalVaryingArrayParameter;
	typedef boost::shared_ptr<CqNormalVaryingArrayParameter> CqNormalVaryingArrayParameterPtr;

	typedef CqParameterTypedVaryingArray<CqVector3D, type_vector, CqVector3D> CqVectorVaryingArrayParameter;
	typedef boost::shared_ptr<CqVectorVaryingArrayParameter> CqVectorVaryingArrayParameterPtr;

	typedef CqParameterTypedVaryingArray<CqMatrix, type_matrix, CqMatrix> CqMatrixVaryingArrayParameter;
	typedef boost::shared_ptr<CqMatrixVaryingArrayParameter> CqMatrixVaryingArrayParameterPtr;

// Vertex array
	typedef CqParameterTypedVertexArray<TqFloat, type_float, TqFloat> CqFloatVertexArrayParameter;
	typedef boost::shared_ptr<CqFloatVertexArrayParameter> CqFloatVertexArrayParameterPtr;
	
	typedef CqParameterTypedVertexArray<TqInt, type_integer, TqFloat> CqIntVertexArrayParameter;
	typedef boost::shared_ptr<CqIntVertexArrayParameter> CqIntVertexArrayParameterPtr;

	typedef CqParameterTypedVertexArray<CqVector3D, type_point, CqVector3D> CqPointVertexArrayParameter;
	typedef boost::shared_ptr<CqPointVertexArrayParameter> CqPointVertexArrayParameterPtr;

	typedef CqParameterTypedVertexArray<CqString, type_string, CqString> CqStringVertexArrayParameter;
	typedef boost::shared_ptr<CqStringVertexArrayParameter> CqStringVertexArrayParameterPtr;

	typedef CqParameterTypedVertexArray<CqColor, type_color, CqColor> CqColorVertexArrayParameter;
	typedef boost::shared_ptr<CqColorVertexArrayParameter> CqColorVertexArrayParameterPtr;

	typedef CqParameterTypedVertexArray<CqVector4D, type_hpoint, CqVector3D> CqHPointVertexArrayParameter;
	typedef boost::shared_ptr<CqHPointVertexArrayParameter> CqHPointVertexArrayParameterPtr;

	typedef CqParameterTypedVertexArray<CqVector3D, type_normal, CqVector3D> CqNormalVertexArrayParameter;
	typedef boost::shared_ptr<CqNormalVertexArrayParameter> CqNormalVertexArrayParameterPtr;

	typedef CqParameterTypedVertexArray<CqVector3D, type_vector, CqVector3D> CqVectorVertexArrayParameter;
	typedef boost::shared_ptr<CqVectorVertexArrayParameter> CqVectorVertexArrayParameterPtr;

	typedef CqParameterTypedVertexArray<CqMatrix, type_matrix, CqMatrix> CqMatrixVertexArrayParameter;
	typedef boost::shared_ptr<CqMatrixVertexArrayParameter> CqMatrixVertexArrayParameterPtr;

// FaceVarying array
	typedef CqParameterTypedFaceVaryingArray<TqFloat, type_float, TqFloat> CqFloatFaceVaryingArrayParameter;
	typedef boost::shared_ptr<CqFloatFaceVaryingArrayParameter> CqFloatFaceVaryingArrayParameterPtr;
	
	typedef CqParameterTypedFaceVaryingArray<TqInt, type_integer, TqFloat> CqIntFaceVaryingArrayParameter;
	typedef boost::shared_ptr<CqIntFaceVaryingArrayParameter> CqIntFaceVaryingArrayParameterPtr;

	typedef CqParameterTypedFaceVaryingArray<CqVector3D, type_point, CqVector3D> CqPointFaceVaryingArrayParameter;
	typedef boost::shared_ptr<CqPointFaceVaryingArrayParameter> CqPointFaceVaryingArrayParameterPtr;

	typedef CqParameterTypedFaceVaryingArray<CqString, type_string, CqString> CqStringFaceVaryingArrayParameter;
	typedef boost::shared_ptr<CqStringFaceVaryingArrayParameter> CqStringFaceVaryingArrayParameterPtr;

	typedef CqParameterTypedFaceVaryingArray<CqColor, type_color, CqColor> CqColorFaceVaryingArrayParameter;
	typedef boost::shared_ptr<CqColorFaceVaryingArrayParameter> CqColorFaceVaryingArrayParameterPtr;

	typedef CqParameterTypedFaceVaryingArray<CqVector4D, type_hpoint, CqVector3D> CqHPointFaceVaryingArrayParameter;
	typedef boost::shared_ptr<CqHPointFaceVaryingArrayParameter> CqHPointFaceVaryingArrayParameterPtr;

	typedef CqParameterTypedFaceVaryingArray<CqVector3D, type_normal, CqVector3D> CqNormalFaceVaryingArrayParameter;
	typedef boost::shared_ptr<CqNormalFaceVaryingArrayParameter> CqNormalFaceVaryingArrayParameterPtr;

	typedef CqParameterTypedFaceVaryingArray<CqVector3D, type_vector, CqVector3D> CqVectorFaceVaryingArrayParameter;
	typedef boost::shared_ptr<CqVectorFaceVaryingArrayParameter> CqVectorFaceVaryingArrayParameterPtr;

	typedef CqParameterTypedFaceVaryingArray<CqMatrix, type_matrix, CqMatrix> CqMatrixFaceVaryingArrayParameter;
	typedef boost::shared_ptr<CqMatrixFaceVaryingArrayParameter> CqMatrixFaceVaryingArrayParameterPtr;
///////////////////////////////////////////////////////////////////////////////

extern CqParameter* ( *gVariableCreateFuncsConstant[] ) ( const char* strName, TqInt Count );
extern CqParameter* ( *gVariableCreateFuncsUniform[] ) ( const char* strName, TqInt Count );
extern CqParameter* ( *gVariableCreateFuncsVarying[] ) ( const char* strName, TqInt Count );
extern CqParameter* ( *gVariableCreateFuncsVertex[] ) ( const char* strName, TqInt Count );
extern CqParameter* ( *gVariableCreateFuncsFaceVarying[] ) ( const char* strName, TqInt Count );
extern CqParameter* ( *gVariableCreateFuncsFaceVertex[] ) ( const char* strName, TqInt Count );
extern CqParameter* ( *gVariableCreateFuncsConstantArray[] ) ( const char* strName, TqInt Count );
extern CqParameter* ( *gVariableCreateFuncsUniformArray[] ) ( const char* strName, TqInt Count );
extern CqParameter* ( *gVariableCreateFuncsVaryingArray[] ) ( const char* strName, TqInt Count );
extern CqParameter* ( *gVariableCreateFuncsVertexArray[] ) ( const char* strName, TqInt Count );
extern CqParameter* ( *gVariableCreateFuncsFaceVaryingArray[] ) ( const char* strName, TqInt Count );
extern CqParameter* ( *gVariableCreateFuncsFaceVertexArray[] ) ( const char* strName, TqInt Count );

//-----------------------------------------------------------------------

END_NAMESPACE( Aqsis )

#endif	// !PARAMETERS_H_INCLUDED
