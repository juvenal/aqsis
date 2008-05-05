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

/**
        \file
        \brief Implements the classes and support structures for 
                handling RenderMan Curves primitives.
        \author Jonathan Merritt (j.merritt@pgrad.unimelb.edu.au)
*/

#include <stdio.h>
#include <string.h>
#include "aqsis.h"
#include "imagebuffer.h"
#include "micropolygon.h"
#include "renderer.h"
#include "patch.h"
#include "vector2d.h"
#include "vector3d.h"
#include "curves.h"
namespace Aqsis {


static TqUlong hwidth = CqString::hash("width");
static TqUlong hcwidth = CqString::hash("constantwidth");

/**
 * CqCurve constructor.
 */
CqCurve::CqCurve() : CqSurface()
{
	m_widthParamIndex = -1;
	m_constantwidthParamIndex = -1;
	m_splitDecision = Split_Undecided;

	STATS_INC( GPR_crv );
}


/**
 * CqCurve destructor.
 */
CqCurve::~CqCurve()
{ }


/**
 * Adds a primitive variable to the list of user parameters.  This method
 * caches the indexes of the "width" and "constantwidth" parameters within
 * the array of user parameters for later access.
 *
 * @param pParam        Pointer to the parameter to add.
 */
void CqCurve::AddPrimitiveVariable( CqParameter* pParam )
{

	// add the primitive variable using the superclass method
	CqSurface::AddPrimitiveVariable( pParam );

	// trap the indexes of "width" and "constantwidth" parameters
	if ( pParam->hash() == hwidth )
	{
		assert( m_widthParamIndex == -1 );
		m_widthParamIndex = m_aUserParams.size() - 1;
	}
	else if ( pParam->hash() == hcwidth )
	{
		assert( m_constantwidthParamIndex == -1 );
		m_constantwidthParamIndex = m_aUserParams.size() - 1;
	}

}


/**
 * Calculates bounds for a CqCurve.
 *
 * NOTE: This method makes the same assumptions as 
 * CqSurfacePatchBicubic::Bound() does about the convex-hull property of the
 * curve.  This is fine most of the time, but the user can specify basis
 * matrices like Catmull-Rom, which are non-convex.
 *
 * FIXME: Make sure that all hulls which reach this method are convex!
 *
 * @return CqBound object containing the bounds.
 */
void CqCurve::Bound(IqBound* bound) const
{

	// Get the boundary in camera space.
	CqVector3D vecA( FLT_MAX, FLT_MAX, FLT_MAX );
	CqVector3D vecB( -FLT_MAX, -FLT_MAX, -FLT_MAX );
	TqFloat maxCameraSpaceWidth = 0;
	TqUint nWidthParams = cVarying();
	for ( TqUint i = 0; i < ( *P() ).Size(); i++ )
	{
		// expand the boundary if necessary to accomodate the
		//  current vertex
		CqVector3D vecV = P()->pValue( i )[0];
		if ( vecV.x() < vecA.x() )
			vecA.x( vecV.x() );
		if ( vecV.y() < vecA.y() )
			vecA.y( vecV.y() );
		if ( vecV.x() > vecB.x() )
			vecB.x( vecV.x() );
		if ( vecV.y() > vecB.y() )
			vecB.y( vecV.y() );
		if ( vecV.z() < vecA.z() )
			vecA.z( vecV.z() );
		if ( vecV.z() > vecB.z() )
			vecB.z( vecV.z() );

		// increase the maximum camera space width of the curve if
		//  necessary
		if ( i < nWidthParams )
		{
			TqFloat camSpaceWidth = width()->pValue( i )[0];
			if ( camSpaceWidth > maxCameraSpaceWidth )
			{
				maxCameraSpaceWidth = camSpaceWidth;
			}
		}

	}

	// increase the size of the boundary by half the width of the
	//  curve in camera space
	vecA -= ( maxCameraSpaceWidth / 2.0 );
	vecB += ( maxCameraSpaceWidth / 2.0 );

	bound->vecMin() = vecA;
	bound->vecMax() = vecB;
	AdjustBoundForTransformationMotion( bound );
}



/**
 * CqCurve CloneData function
 *
 */
void CqCurve::CloneData(CqCurve* clone) const
{
	CqSurface::CloneData(clone);
	clone->m_widthParamIndex = m_widthParamIndex;
	clone->m_constantwidthParamIndex = m_constantwidthParamIndex;
}



/**
 * Returns the approximate "length" of an edge of a grid in raster space.
 *
 * @return Approximate grid length.
 */
TqFloat CqCurve::GetGridLength() const
{

	// we want to find the number of micropolygons per grid - the default
	//  is 256 (16x16 micropolygon grid).
	TqFloat micropolysPerGrid = 256;
	const TqInt* poptGridSize =
	    QGetRenderContext() ->poptCurrent()->GetIntegerOption(
	        "limits", "gridsize"
	    );
	if ( poptGridSize != NULL )
		micropolysPerGrid = poptGridSize[0];

	// find the shading rate
	TqFloat ShadingRate = pAttributes() ->GetFloatAttribute(
	                          "System", "ShadingRate"
	                      ) [ 0 ];

	// we assume that the grids are square and take the square root to find
	//  the number of micropolygons along one side
	TqFloat mpgsAlongSide = sqrt( micropolysPerGrid );

	// now, the number of pixels (raster space length) taken up by one
	//  micropolygon is given by 1 / shading rate.  So, to find the length
	//  in raster space of the edge of the micropolygon grid, we divide its
	//  length (in micropolygons) by the shading rate
	return mpgsAlongSide / ShadingRate;

}



/**
 * Populates the "width" parameter if it is not already present (ie supplied
 * by the user).  The "width" is populated either by the value of
 * "constantwidth", or by the default object-space width 1.0.
 */
void CqCurve::PopulateWidth()
{

	// if the width parameter has been supplied by the user then bail
	//  immediately
	if ( width() != NULL )
		return ;

	// otherwise, find the value to fill the width array with; default
	//  value is 1.0 which can be overridden by the "constantwidth"
	//  parameter
	TqFloat widthvalue = 1.0;
	if ( constantwidth() != NULL )
	{
		widthvalue = *( constantwidth() ->pValue() );
	}

	// create and fill in the width array
	CqParameterTypedVarying<TqFloat, type_float, TqFloat>* widthP =
	    new CqParameterTypedVarying<TqFloat, type_float, TqFloat>(
	        "width"
	    );
	widthP->SetSize( cVarying() );
	for ( TqUint i = 0; i < cVarying(); i++ )
	{
		widthP->pValue( i )[0] = widthvalue;
	}

	// add the width array to the curve as a primitive variable
	AddPrimitiveVariable( widthP );
}


bool CqCurve::Diceable()
{
	// OK, here the CqCubicCurveSegment line has two options:
	//  1. split into two more lines
	//  2. turn into a bilinear patch for rendering
	// We don't want to go turning into a patch unless absolutely
	// necessary, since patches cost more.  We only want to become a patch
	// if the current curve is "best handled" as a patch.  For now, I'm
	// choosing to define that the curve is best handled as a patch under
	// one or more of the following two conditions:
	//  1. If the maximum width is a significant fraction of the length of
	//      the line (width greater than 0.75 x length; ignoring normals).
	//  2. If the length of the line (ignoring the width; cos' it's
	//      covered by point 1) is such that it's likely a bilinear
	//      patch would be diced immediately if we created one (so that
	//      patches don't have to get split!).
	//  3. If the curve crosses the eye plane (m_fDiceable == false).

	// find the length of the CqLinearCurveSegment line in raster space
	if( m_splitDecision == Split_Undecided )
	{
		// AGG - 31/07/04
		// well, if we follow the above statagy we end up splitting into
		// far too many grids (with roughly 1 mpg per grid). so after
		// profiling a few scenes, the fastest method seems to be just
		// to convert to a patch immediatly.
		// we really need a native dice for curves but until that time
		// i reckon this is best.
		//m_splitDecision = Split_Patch;

		CqMatrix matCtoR;
		QGetRenderContext() ->matSpaceToSpace(
									"camera", "raster",
									NULL, NULL,
									QGetRenderContextI()->Time(),
									matCtoR
									);
		CqVector2D hull[ 2 ];     // control hull
		hull[ 0 ] = matCtoR * P()->pValue( 0 )[0];
		hull[ 1 ] = matCtoR * P()->pValue( 1 )[0];
		CqVector2D lengthVector = hull[ 1 ] - hull[ 0 ];
		TqFloat lengthraster = lengthVector.Magnitude();

		// find the approximate "length" of a diced patch in raster space
		TqFloat gridlength = GetGridLength();

		// decide whether to split into more curve segments or a patch
		if(( lengthraster < gridlength ) || ( !m_fDiceable ))
		{
			// split into a patch
			m_splitDecision = Split_Patch;
		}
		else
		{
			// split into smaller curves
			m_splitDecision = Split_Curve;
		}
	}

	return false;
}


bool CqCurve::GetNormal( TqInt index, CqVector3D& normal ) const
{
	if ( N() != NULL )
	{
		normal = N()->pValue( index )[0];
		return true;
	}
	else
	{
		bool CSO = pTransform()->GetHandedness(pTransform()->Time(0));
		bool O = pAttributes() ->GetIntegerAttribute( "System", "Orientation" ) [ 0 ] != 0;
		if ( (O && CSO) || (!O && !CSO) )
			normal = CqVector3D(0, 0, -1);
		else
			normal = CqVector3D(0, 0, 1);
		return false;
	}
}

/**
 * Sets the default primitive variables.
 *
 * @param bUseDef_st
 */
void CqCurve::SetDefaultPrimitiveVariables( bool bUseDef_st )
{
	// we don't want any default primitive variables.

	// s,t are set to u,v for curves
}


/**
 * CqCurvesGroup constructor.
 */
CqCurvesGroup::CqCurvesGroup() : CqCurve(), m_ncurves( 0 ), m_periodic( false ),
		m_nTotalVerts( 0 )
{ }



/**
 * CqCurvesGroup copy constructor.
 */
/* CqCurvesGroup::CqCurvesGroup( const CqCurvesGroup& from ) : CqCurve()
 * {
 * 	( *this ) = from;
 * }
 */



/**
 * CqCurvesGroup destructor.
 */
CqCurvesGroup::~CqCurvesGroup()
{ }



/**
 * Clone the data from this curve group onto the one specified.
 *
 */
void CqCurvesGroup::CloneData( CqCurvesGroup* clone ) const
{
	CqCurve::CloneData(clone);

	// copy members
	clone->m_ncurves = m_ncurves;
	clone->m_periodic = m_periodic;
	clone->m_nvertices = m_nvertices;
	clone->m_nTotalVerts = m_nTotalVerts;
}


} // namespace Aqsis
