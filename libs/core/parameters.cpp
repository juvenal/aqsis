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
		\brief Implements classes and support functionality for the shader virtual machine.
		\author Paul C. Gregory (pgregory@aqsis.org)
*/

#include	<aqsis/aqsis.h>
#include	"parameters.h"
#include	<aqsis/math/matrix.h>
#include	<aqsis/util/sstring.h>
#include	<aqsis/math/color.h>
#include	"renderer.h"

namespace Aqsis {


/** Default constructor
 * \param strName Character pointer to parameter name.
 * \param Count Integer value count, for arrays.
 */
CqParameter::CqParameter( const char* strName, TqInt Count ) :
		m_strName( strName ),
		m_Count( Count )
{
	/// \note Had to remove this as paramters are now created as part of the Renderer construction, so the
	///		  renderer context isn't ready yet.
	//	QGetRenderContext() ->Stats().IncParametersAllocated();

	assert( Count >= 1 );

	STATS_INC( PRM_created );
	STATS_INC( PRM_current );
	TqInt cPRM = STATS_GETI( PRM_current );
	TqInt cPeak = STATS_GETI( PRM_peak );

	STATS_SETI( PRM_peak, cPRM > cPeak ? cPRM : cPeak );
	m_hash = CqString::hash(strName);
}

/** Copy constructor
 */
CqParameter::CqParameter( const CqParameter& From ) :
		m_strName( From.m_strName ),
		m_Count( From.m_Count ),
		m_hash(From.m_hash)
{
	/// \note Had to remove this as paramters are now created as part of the Renderer construction, so the
	///		  renderer context isn't ready yet.
	//	QGetRenderContext() ->Stats().IncParametersAllocated();
	STATS_INC( PRM_created );
	STATS_INC( PRM_current );
	TqInt cPRM = STATS_GETI( PRM_current );
	TqInt cPeak = STATS_GETI( PRM_peak );

	STATS_SETI( PRM_peak, cPRM > cPeak ? cPRM : cPeak );
}

CqParameter::~CqParameter()
{
	/// \note Had to remove this as paramters are now created as part of the Renderer construction, so the
	///		  renderer context isn't ready yet.
	//	QGetRenderContext() ->Stats().IncParametersDeallocated();
	STATS_DEC( PRM_current );
}

CqParameter* CqParameter::Create(const CqPrimvarToken& tok)
{
	CqParameter* ( *createFunc ) ( const char* strName, TqInt Count ) = 0;
	if(tok.count() <= 1)
	{
		switch(tok.Class())
		{
			case class_constant:
				createFunc = gVariableCreateFuncsConstant[tok.type()];
				break;
			case class_uniform:
				createFunc = gVariableCreateFuncsUniform[tok.type()];
				break;
			case class_varying:
				createFunc = gVariableCreateFuncsVarying[tok.type()];
				break;
			case class_vertex:
				createFunc = gVariableCreateFuncsVertex[tok.type()];
				break;
			case class_facevarying:
				createFunc = gVariableCreateFuncsFaceVarying[tok.type()];
				break;
			case class_facevertex:
				createFunc = gVariableCreateFuncsFaceVertex[tok.type()];
				break;
			default:
				break;
		}
	}
	else
	{
		switch(tok.Class())
		{
			case class_constant:
				createFunc = gVariableCreateFuncsConstantArray[tok.type()];
				break;
			case class_uniform:
				createFunc = gVariableCreateFuncsUniformArray[tok.type()];
				break;
			case class_varying:
				createFunc = gVariableCreateFuncsVaryingArray[tok.type()];
				break;
			case class_vertex:
				createFunc = gVariableCreateFuncsVertexArray[tok.type()];
				break;
			case class_facevarying:
				createFunc = gVariableCreateFuncsFaceVaryingArray[tok.type()];
				break;
			case class_facevertex:
				createFunc = gVariableCreateFuncsFaceVertexArray[tok.type()];
				break;
			default:
				break;
		}
	}
	if(!createFunc)
	{
		AQSIS_THROW_XQERROR(XqInternal, EqE_Bug,
			"Could not create CqParameter for token \"" << tok << "\"");
	}
	return createFunc(tok.name().c_str(), tok.count());
}


CqParameter* ( *gVariableCreateFuncsConstant[] ) ( const char* strName, TqInt Count ) =
    {
        0,
        CqParameterTypedConstant<TqFloat, type_float, TqFloat>::Create,
        CqParameterTypedConstant<TqInt, type_integer, TqFloat>::Create,
        CqParameterTypedConstant<Imath::V3f, type_point, Imath::V3f>::Create,
        CqParameterTypedConstant<CqString, type_string, CqString>::Create,
        CqParameterTypedConstant<CqColor, type_color, CqColor>::Create,
        0,
        CqParameterTypedConstant<V4f, type_hpoint, Imath::V3f>::Create,
        CqParameterTypedConstant<Imath::V3f, type_normal, Imath::V3f>::Create,
        CqParameterTypedConstant<Imath::V3f, type_vector, Imath::V3f>::Create,
        0,
        CqParameterTypedConstant<CqMatrix, type_matrix, CqMatrix>::Create,
        0,
        0,
    };


CqParameter* ( *gVariableCreateFuncsUniform[] ) ( const char* strName, TqInt Count ) =
    {
        0,
        CqParameterTypedUniform<TqFloat, type_float, TqFloat>::Create,
        CqParameterTypedUniform<TqInt, type_integer, TqFloat>::Create,
        CqParameterTypedUniform<Imath::V3f, type_point, Imath::V3f>::Create,
        CqParameterTypedUniform<CqString, type_string, CqString>::Create,
        CqParameterTypedUniform<CqColor, type_color, CqColor>::Create,
        0,
        CqParameterTypedUniform<V4f, type_hpoint, Imath::V3f>::Create,
        CqParameterTypedUniform<Imath::V3f, type_normal, Imath::V3f>::Create,
        CqParameterTypedUniform<Imath::V3f, type_vector, Imath::V3f>::Create,
        0,
        CqParameterTypedUniform<CqMatrix, type_matrix, CqMatrix>::Create,
        0,
        0,
    };

CqParameter* ( *gVariableCreateFuncsVarying[] ) ( const char* strName, TqInt Count ) =
    {
        0,
        CqParameterTypedVarying<TqFloat, type_float, TqFloat>::Create,
        CqParameterTypedVarying<TqInt, type_integer, TqFloat>::Create,
        CqParameterTypedVarying<Imath::V3f, type_point, Imath::V3f>::Create,
        CqParameterTypedVarying<CqString, type_string, CqString>::Create,
        CqParameterTypedVarying<CqColor, type_color, CqColor>::Create,
        0,
        CqParameterTypedVarying<V4f, type_hpoint, Imath::V3f>::Create,
        CqParameterTypedVarying<Imath::V3f, type_normal, Imath::V3f>::Create,
        CqParameterTypedVarying<Imath::V3f, type_vector, Imath::V3f>::Create,
        0,
        CqParameterTypedVarying<CqMatrix, type_matrix, CqMatrix>::Create,
        0,
        0,
    };

CqParameter* ( *gVariableCreateFuncsVertex[] ) ( const char* strName, TqInt Count ) =
    {
        0,
        CqParameterTypedVertex<TqFloat, type_float, TqFloat>::Create,
        CqParameterTypedVertex<TqInt, type_integer, TqFloat>::Create,
        CqParameterTypedVertex<Imath::V3f, type_point, Imath::V3f>::Create,
        CqParameterTypedVertex<CqString, type_string, CqString>::Create,
        CqParameterTypedVertex<CqColor, type_color, CqColor>::Create,
        0,
        CqParameterTypedVertex<V4f, type_hpoint, Imath::V3f>::Create,
        CqParameterTypedVertex<Imath::V3f, type_normal, Imath::V3f>::Create,
        CqParameterTypedVertex<Imath::V3f, type_vector, Imath::V3f>::Create,
        0,
        CqParameterTypedVertex<CqMatrix, type_matrix, CqMatrix>::Create,
        0,
        0,
    };


CqParameter* ( *gVariableCreateFuncsFaceVarying[] ) ( const char* strName, TqInt Count ) =
    {
        0,
        CqParameterTypedFaceVarying<TqFloat, type_float, TqFloat>::Create,
        CqParameterTypedFaceVarying<TqInt, type_integer, TqFloat>::Create,
        CqParameterTypedFaceVarying<Imath::V3f, type_point, Imath::V3f>::Create,
        CqParameterTypedFaceVarying<CqString, type_string, CqString>::Create,
        CqParameterTypedFaceVarying<CqColor, type_color, CqColor>::Create,
        0,
        CqParameterTypedFaceVarying<V4f, type_hpoint, Imath::V3f>::Create,
        CqParameterTypedFaceVarying<Imath::V3f, type_normal, Imath::V3f>::Create,
        CqParameterTypedFaceVarying<Imath::V3f, type_vector, Imath::V3f>::Create,
        0,
        CqParameterTypedFaceVarying<CqMatrix, type_matrix, CqMatrix>::Create,
        0,
        0,
    };


CqParameter* ( *gVariableCreateFuncsFaceVertex[] ) ( const char* strName, TqInt Count ) =
    {
        0,
        CqParameterTypedFaceVertex<TqFloat, type_float, TqFloat>::Create,
        CqParameterTypedFaceVertex<TqInt, type_integer, TqFloat>::Create,
        CqParameterTypedFaceVertex<Imath::V3f, type_point, Imath::V3f>::Create,
        CqParameterTypedFaceVertex<CqString, type_string, CqString>::Create,
        CqParameterTypedFaceVertex<CqColor, type_color, CqColor>::Create,
        0,
        CqParameterTypedFaceVertex<V4f, type_hpoint, Imath::V3f>::Create,
        CqParameterTypedFaceVertex<Imath::V3f, type_normal, Imath::V3f>::Create,
        CqParameterTypedFaceVertex<Imath::V3f, type_vector, Imath::V3f>::Create,
        0,
        CqParameterTypedFaceVertex<CqMatrix, type_matrix, CqMatrix>::Create,
        0,
        0,
    };

CqParameter* ( *gVariableCreateFuncsConstantArray[] ) ( const char* strName, TqInt Count ) =
    {
        0,
        CqParameterTypedConstantArray<TqFloat, type_float, TqFloat>::Create,
        CqParameterTypedConstantArray<TqInt, type_integer, TqFloat>::Create,
        CqParameterTypedConstantArray<Imath::V3f, type_point, Imath::V3f>::Create,
        CqParameterTypedConstantArray<CqString, type_string, CqString>::Create,
        CqParameterTypedConstantArray<CqColor, type_color, CqColor>::Create,
        0,
        CqParameterTypedConstantArray<V4f, type_hpoint, Imath::V3f>::Create,
        CqParameterTypedConstantArray<Imath::V3f, type_normal, Imath::V3f>::Create,
        CqParameterTypedConstantArray<Imath::V3f, type_vector, Imath::V3f>::Create,
        0,
        CqParameterTypedConstantArray<CqMatrix, type_matrix, CqMatrix>::Create,
        0,
        0,
    };


CqParameter* ( *gVariableCreateFuncsUniformArray[] ) ( const char* strName, TqInt Count ) =
    {
        0,
        CqParameterTypedUniformArray<TqFloat, type_float, TqFloat>::Create,
        CqParameterTypedUniformArray<TqInt, type_integer, TqFloat>::Create,
        CqParameterTypedUniformArray<Imath::V3f, type_point, Imath::V3f>::Create,
        CqParameterTypedUniformArray<CqString, type_string, CqString>::Create,
        CqParameterTypedUniformArray<CqColor, type_color, CqColor>::Create,
        0,
        CqParameterTypedUniformArray<V4f, type_hpoint, Imath::V3f>::Create,
        CqParameterTypedUniformArray<Imath::V3f, type_normal, Imath::V3f>::Create,
        CqParameterTypedUniformArray<Imath::V3f, type_vector, Imath::V3f>::Create,
        0,
        CqParameterTypedUniformArray<CqMatrix, type_matrix, CqMatrix>::Create,
        0,
        0,
    };

CqParameter* ( *gVariableCreateFuncsVaryingArray[] ) ( const char* strName, TqInt Count ) =
    {
        0,
        CqParameterTypedVaryingArray<TqFloat, type_float, TqFloat>::Create,
        CqParameterTypedVaryingArray<TqInt, type_integer, TqFloat>::Create,
        CqParameterTypedVaryingArray<Imath::V3f, type_point, Imath::V3f>::Create,
        CqParameterTypedVaryingArray<CqString, type_string, CqString>::Create,
        CqParameterTypedVaryingArray<CqColor, type_color, CqColor>::Create,
        0,
        CqParameterTypedVaryingArray<V4f, type_hpoint, Imath::V3f>::Create,
        CqParameterTypedVaryingArray<Imath::V3f, type_normal, Imath::V3f>::Create,
        CqParameterTypedVaryingArray<Imath::V3f, type_vector, Imath::V3f>::Create,
        0,
        CqParameterTypedVaryingArray<CqMatrix, type_matrix, CqMatrix>::Create,
        0,
        0,
    };

CqParameter* ( *gVariableCreateFuncsVertexArray[] ) ( const char* strName, TqInt Count ) =
    {
        0,
        CqParameterTypedVertexArray<TqFloat, type_float, TqFloat>::Create,
        CqParameterTypedVertexArray<TqInt, type_integer, TqFloat>::Create,
        CqParameterTypedVertexArray<Imath::V3f, type_point, Imath::V3f>::Create,
        CqParameterTypedVertexArray<CqString, type_string, CqString>::Create,
        CqParameterTypedVertexArray<CqColor, type_color, CqColor>::Create,
        0,
        CqParameterTypedVertexArray<V4f, type_hpoint, Imath::V3f>::Create,
        CqParameterTypedVertexArray<Imath::V3f, type_normal, Imath::V3f>::Create,
        CqParameterTypedVertexArray<Imath::V3f, type_vector, Imath::V3f>::Create,
        0,
        CqParameterTypedVertexArray<CqMatrix, type_matrix, CqMatrix>::Create,
        0,
        0,
    };

CqParameter* ( *gVariableCreateFuncsFaceVaryingArray[] ) ( const char* strName, TqInt Count ) =
    {
        0,
        CqParameterTypedFaceVaryingArray<TqFloat, type_float, TqFloat>::Create,
        CqParameterTypedFaceVaryingArray<TqInt, type_integer, TqFloat>::Create,
        CqParameterTypedFaceVaryingArray<Imath::V3f, type_point, Imath::V3f>::Create,
        CqParameterTypedFaceVaryingArray<CqString, type_string, CqString>::Create,
        CqParameterTypedFaceVaryingArray<CqColor, type_color, CqColor>::Create,
        0,
        CqParameterTypedFaceVaryingArray<V4f, type_hpoint, Imath::V3f>::Create,
        CqParameterTypedFaceVaryingArray<Imath::V3f, type_normal, Imath::V3f>::Create,
        CqParameterTypedFaceVaryingArray<Imath::V3f, type_vector, Imath::V3f>::Create,
        0,
        CqParameterTypedFaceVaryingArray<CqMatrix, type_matrix, CqMatrix>::Create,
        0,
        0,
    };

CqParameter* ( *gVariableCreateFuncsFaceVertexArray[] ) ( const char* strName, TqInt Count ) =
    {
        0,
        CqParameterTypedFaceVertexArray<TqFloat, type_float, TqFloat>::Create,
        CqParameterTypedFaceVertexArray<TqInt, type_integer, TqFloat>::Create,
        CqParameterTypedFaceVertexArray<Imath::V3f, type_point, Imath::V3f>::Create,
        CqParameterTypedFaceVertexArray<CqString, type_string, CqString>::Create,
        CqParameterTypedFaceVertexArray<CqColor, type_color, CqColor>::Create,
        0,
        CqParameterTypedFaceVertexArray<V4f, type_hpoint, Imath::V3f>::Create,
        CqParameterTypedFaceVertexArray<Imath::V3f, type_normal, Imath::V3f>::Create,
        CqParameterTypedFaceVertexArray<Imath::V3f, type_vector, Imath::V3f>::Create,
        0,
        CqParameterTypedFaceVertexArray<CqMatrix, type_matrix, CqMatrix>::Create,
        0,
        0,
    };

//---------------------------------------------------------------------
/** Copy constructor.
 */

CqNamedParameterList::CqNamedParameterList( const CqNamedParameterList& From ) :
		m_strName( From.m_strName ),
		m_hash( From.m_hash)
{
	TqInt i = From.m_aParameters.size();
	while ( i-- > 0 )
	{
		m_aParameters.push_back( From.m_aParameters[ i ] ->Clone() );
	}
}


} // namespace Aqsis
//---------------------------------------------------------------------
