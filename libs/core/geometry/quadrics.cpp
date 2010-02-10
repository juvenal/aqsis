
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
		\brief Implements the standard RenderMan quadric primitive classes.
		\author Paul C. Gregory (pgregory@aqsis.org)
*/

#include	"quadrics.h"

#include	<aqsis/math/math.h>
#include	"imagebuffer.h"
#include	"nurbs.h"
#include	<aqsis/ri/ri.h>

namespace Aqsis {

static bool IntersectLine( Imath::V3f& P1, Imath::V3f& T1, Imath::V3f& P2, Imath::V3f& T2, Imath::V3f& P );
static void ProjectToLine( const Imath::V3f& S, const Imath::V3f& Trj, const Imath::V3f& pnt, Imath::V3f& p );
static void sinCosGrid(TqFloat t0, TqFloat t1, TqInt numSteps, TqFloat* sint, TqFloat* cost);

#define TOOLARGEQUADS 10000
static TqUlong RIH_P = CqString::hash("P");

CqQuadric::CqQuadric()
{
	m_uDiceSize = m_vDiceSize = 0;

	STATS_INC( GPR_quad );

}


//---------------------------------------------------------------------
/** Clone the data on this quadric to the (possibly derived) class
 *  passed in.
 */

void CqQuadric::CloneData( CqQuadric* clone ) const
{
	CqSurface::CloneData( clone );
	clone->m_matTx = m_matTx;
	clone->m_matITTx = m_matITTx;
	clone->m_uDiceSize = m_uDiceSize;
	clone->m_vDiceSize = m_vDiceSize;
}


//---------------------------------------------------------------------
/** Transform the quadric primitive by the specified matrix.
 */

void	CqQuadric::Transform( const CqMatrix& matTx, const CqMatrix& matITTx, const CqMatrix& matRTx, TqInt iTime )
{
	m_matTx.PreMultiply(matTx);
	m_matITTx.PreMultiply(matITTx);
}


//---------------------------------------------------------------------
/** Dice the quadric filling in as much information on the grid as possible.
 */

TqInt CqQuadric::DiceAll( CqMicroPolyGrid* pGrid )
{
	TqInt lUses = Uses();
	TqInt lDone = 0;

	Imath::V3f	P, N;
	Imath::V3f *pointGrid, *normalGrid;

	int v, u;

	CqParameterTyped<TqFloat, TqFloat>* ps = s();
	CqParameterTyped<TqFloat, TqFloat>* pt = t();
	CqParameterTyped<TqFloat, TqFloat>* pu = this->u();
	CqParameterTyped<TqFloat, TqFloat>* pv = this->v();
	CqParameterTyped<TqFloat, TqFloat>* pst = static_cast<CqParameterTyped<TqFloat, TqFloat>*>(FindUserParam( "st" ));

	TqFloat s0 = 0;
	TqFloat s1 = 0;
	TqFloat s2 = 0;
	TqFloat s3 = 0;
	if( USES( lUses, EnvVars_s ) && NULL != pGrid->pVar(EnvVars_s) && bHasVar(EnvVars_s) )
	{
		if( pst )
		{
			s0 = pst->pValue( 0 )[0];
			s1 = pst->pValue( 1 )[0];
			s2 = pst->pValue( 2 )[0];
			s3 = pst->pValue( 3 )[0];
		}
		else if( ps )
		{
			s0 = ps->pValue( 0 )[0];
			s1 = ps->pValue( 1 )[0];
			s2 = ps->pValue( 2 )[0];
			s3 = ps->pValue( 3 )[0];
		}

		DONE( lDone, EnvVars_s );
	}

	TqFloat t0 = 0;
	TqFloat t1 = 0;
	TqFloat t2 = 0;
	TqFloat t3 = 0;
	if( USES( lUses, EnvVars_t ) && NULL != pGrid->pVar(EnvVars_t) && bHasVar(EnvVars_t) )
	{
		if( pst )
		{
			t0 = pst->pValue( 0 )[1];
			t1 = pst->pValue( 1 )[1];
			t2 = pst->pValue( 2 )[1];
			t3 = pst->pValue( 3 )[1];
		}
		else if( pt )
		{
			t0 = pt->pValue( 0 )[0];
			t1 = pt->pValue( 1 )[0];
			t2 = pt->pValue( 2 )[0];
			t3 = pt->pValue( 3 )[0];
		}

		DONE( lDone, EnvVars_t );
	}

	TqFloat u0 = 0;
	TqFloat u1 = 0;
	TqFloat u2 = 0;
	TqFloat u3 = 0;
	if( USES( lUses, EnvVars_u ) && NULL != pGrid->pVar(EnvVars_u) && bHasVar(EnvVars_u) )
	{
		u0 = pu->pValue( 0 )[0];
		u1 = pu->pValue( 1 )[0];
		u2 = pu->pValue( 2 )[0];
		u3 = pu->pValue( 3 )[0];

		DONE( lDone, EnvVars_u );
	}

	TqFloat v0 = 0;
	TqFloat v1 = 0;
	TqFloat v2 = 0;
	TqFloat v3 = 0;
	if( USES( lUses, EnvVars_v ) && NULL != pGrid->pVar(EnvVars_v) && bHasVar(EnvVars_v) )
	{
		v0 = pv->pValue( 0 )[0];
		v1 = pv->pValue( 1 )[0];
		v2 = pv->pValue( 2 )[0];
		v3 = pv->pValue( 3 )[0];

		DONE( lDone, EnvVars_v );
	}

	// Normals need to be flipped depending on the orientation.  Because we
	// calculate normals in *object* space using DicePoint(), and the transform
	// with the matrix m_matITTx, we don't need to explicitly consider the
	// handedness of the current transformation here.  Instead we only need to
	// consider the current orientation when deciding whether to flip the
	// normals.
	bool flipNormals = pAttributes()->
		GetIntegerAttribute("System", "Orientation")[0] != 0;

	pGrid->pVar(EnvVars_P)->GetPointPtr(pointGrid);
	pGrid->pVar(EnvVars_Ng)->GetNormalPtr(normalGrid);

	// check pVars to see if we need to calc normals
	bool useNormals=false;

	if (USES( lUses, EnvVars_Ng ) && NULL != pGrid->pVar(EnvVars_Ng))
	{
		useNormals = true;
		DicePoints(pointGrid, normalGrid);
		// Indicate that P and Ng will be filled in by the loops below.
		DONE( lDone, EnvVars_P );
		DONE( lDone, EnvVars_Ng );
	}
	else
	{
		DicePoints(pointGrid, NULL);
		// Indicate that P (only) will be filled in by the loops below
		DONE( lDone, EnvVars_P );
	}
	
	TqFloat du = 1.0 / uDiceSize();
	TqFloat dv = 1.0 / vDiceSize();
	for ( v = 0; v <= vDiceSize(); v++ )
	{
		TqFloat vf = v * dv;
		for ( u = 0; u <= uDiceSize(); u++ )
		{
			TqFloat uf = u * du;
			TqInt igrid = ( v * ( uDiceSize() + 1 ) ) + u; // offset for grid point/normal buffer

			if(useNormals)
			{
				N = normalGrid[igrid];

				if(flipNormals)
				    N = -N;
				
				pointGrid[igrid] = m_matTx * pointGrid[igrid];
				normalGrid[igrid] =  m_matITTx * N;
			}
			else
			{
				P = pointGrid[igrid];
				pGrid->pVar(EnvVars_P)->SetPoint( m_matTx * P, igrid );
			}
		
			if( USES( lUses, EnvVars_s ) && NULL != pGrid->pVar(EnvVars_s) && bHasVar(EnvVars_s) )
			{
				TqFloat _s = BilinearEvaluate( s0, s1, s2, s3, uf, vf );
				pGrid->pVar(EnvVars_s)->SetFloat( _s, igrid );
			}
			if( USES( lUses, EnvVars_t ) && NULL != pGrid->pVar(EnvVars_t) && bHasVar(EnvVars_t) )
			{
				TqFloat _t = BilinearEvaluate( t0, t1, t2, t3, uf, vf );
				pGrid->pVar(EnvVars_t)->SetFloat( _t, igrid );
			}
			if( USES( lUses, EnvVars_u ) && NULL != pGrid->pVar(EnvVars_u) && bHasVar(EnvVars_u) )
			{
				TqFloat _u = BilinearEvaluate( u0, u1, u2, u3, uf, vf );
				pGrid->pVar(EnvVars_u)->SetFloat( _u, igrid );
			}
			if( USES( lUses, EnvVars_v ) && NULL != pGrid->pVar(EnvVars_v) && bHasVar(EnvVars_v) )
			{
				TqFloat _v = BilinearEvaluate( v0, v1, v2, v3, uf, vf );
				pGrid->pVar(EnvVars_v)->SetFloat( _v, igrid );
			}
		}
	}

	return( lDone );
}

//---------------------------------------------------------------------
/** Determine whether the quadric is suitable for dicing.
 */
bool CqQuadric::Diceable()
{
	// If the cull check showed that the primitive cannot be diced due to crossing the e and hither planes,
	// then we can return immediately.
	if ( !m_fDiceable )
		return ( false );

	TqUlong toomuch = EstimateGridSize();

	m_SplitDir = ( m_uDiceSize > m_vDiceSize ) ? SplitDir_U : SplitDir_V;

	TqFloat gs = 16.0f;
	const TqFloat* poptGridSize = QGetRenderContext() ->poptCurrent()->GetFloatOption( "System", "SqrtGridSize" );
	if( NULL != poptGridSize )
		gs = poptGridSize[0];

	if (toomuch > TOOLARGEQUADS)
		return false;

	if ( m_uDiceSize > gs)
		return false;
	if ( m_vDiceSize > gs)
		return false;


	return ( true );
}


//---------------------------------------------------------------------
/** Estimate the size of the micropolygrid required to dice this GPrim to a suitable shading rate.
 */

TqUlong CqQuadric::EstimateGridSize()
{
	TqFloat maxusize, maxvsize;
	maxusize = maxvsize = 0;

	CqMatrix matCtoR;
	QGetRenderContext() ->matSpaceToSpace( "camera", "raster", NULL, NULL, QGetRenderContext()->Time(), matCtoR );
	CqMatrix matTx = matCtoR * m_matTx;

	m_uDiceSize = m_vDiceSize = ESTIMATEGRIDSIZE;

	TqFloat udist, vdist;
	Imath::V3f p(0), pum1(0), pvm1[ ESTIMATEGRIDSIZE ];

	Imath::V3f pointGrid[(ESTIMATEGRIDSIZE+1)*(ESTIMATEGRIDSIZE+1)];

	DicePoints(pointGrid, NULL);

	int v, u;
	for ( v = 0; v <= ESTIMATEGRIDSIZE; v++ )
	{
		for ( u = 0; u <= ESTIMATEGRIDSIZE; u++ )
		{
			TqInt gridOffset = ( v * ( ESTIMATEGRIDSIZE + 1 ) ) + u;
			p = pointGrid[gridOffset];
			p = matTx * p;

			// If we are on row two or above, calculate the mp size.
			if ( v >= 1 && u >= 1 )
			{
				udist = ( p.x - pum1.x ) * ( p.x - pum1.x ) +
				        ( p.y - pum1.y ) * ( p.y - pum1.y );
				vdist = ( pvm1[ u - 1 ].x - pum1.x ) * ( pvm1[ u - 1 ].x - pum1.x ) +
				        ( pvm1[ u - 1 ].y - pum1.y ) * ( pvm1[ u - 1 ].y - pum1.y );

				maxusize = max( maxusize, udist );
				maxvsize = max( maxvsize, vdist );
			}
			if ( u >= 1 )
				pvm1[ u - 1 ] = pum1;
			pum1 = p;
		}
	}
	maxusize = sqrt( maxusize );
	maxvsize = sqrt( maxvsize );

	TqFloat sqrtShadingRate = sqrt(AdjustedShadingRate());
	m_uDiceSize = lceil(ESTIMATEGRIDSIZE * maxusize/sqrtShadingRate);
	m_vDiceSize = lceil(ESTIMATEGRIDSIZE * maxvsize/sqrtShadingRate);

	// Ensure power of 2 to avoid cracking
	const TqInt *binary = pAttributes() ->GetIntegerAttribute( "dice", "binary" );
	if ( binary && *binary)
	{
		m_uDiceSize = ceilPow2( m_uDiceSize );
		m_vDiceSize = ceilPow2( m_vDiceSize );
	}

	return  (TqUlong) m_uDiceSize * m_vDiceSize;
}


//---------------------------------------------------------------------
/** Constructor.
 */

CqSphere::CqSphere( TqFloat radius, TqFloat zmin, TqFloat zmax, TqFloat thetamin, TqFloat thetamax ) :
		m_Radius( radius ),
		m_ThetaMin( thetamin ),
		m_ThetaMax( thetamax )
{
	// Sanity check the values, while ensuring we keep the same signs.
	TqFloat frad = fabs(m_Radius);
	if( fabs(zmin) > frad )
		zmin = frad*(zmin<0)?-1:1;
	if( fabs(zmin) > frad )
		zmin = frad*(zmin<0)?-1:1;

	m_PhiMin = asin( zmin / m_Radius );
	m_PhiMax = asin( zmax / m_Radius );
}


//---------------------------------------------------------------------
/** Create a clone of this sphere
 */

CqSurface*	CqSphere::Clone( ) const
{
	CqSphere* clone = new CqSphere();
	CqQuadric::CloneData( clone );
	clone->m_Radius = m_Radius;
	clone->m_PhiMin = m_PhiMin;
	clone->m_PhiMax = m_PhiMax;
	clone->m_ThetaMin = m_ThetaMin;
	clone->m_ThetaMax = m_ThetaMax;

	return ( clone );
}

//---------------------------------------------------------------------
/** Get the geometric bound of this GPrim.
 */

void CqSphere::Bound(CqBound* bound) const
{
	std::vector<Imath::V3f> curve;
	Imath::V3f vA( 0, 0, 0 ), vB( 1, 0, 0 ), vC( 0, 0, 1 );
	Circle( vA, vB, vC, m_Radius, std::min(m_PhiMin, m_PhiMax), std::max(m_PhiMin, m_PhiMax), curve );

	CqMatrix matRot( degToRad ( m_ThetaMin ), vC );
	for ( std::vector<Imath::V3f>::iterator i = curve.begin(); i != curve.end(); i++ )
		*i = matRot * ( *i );

	CqBound	B( RevolveForBound( curve, vA, vC, degToRad( m_ThetaMax - m_ThetaMin ) ) );
	B.Transform( m_matTx );
	bound->vecMin() = B.vecMin();
	bound->vecMax() = B.vecMax();

	AdjustBoundForTransformationMotion( bound );
}

//---------------------------------------------------------------------
/** Split this GPrim into a NURBS surface. Temp implementation, should split into smalled quadrics.
 */

TqInt CqSphere::PreSubdivide( std::vector<boost::shared_ptr<CqSurface> >& aSplits, bool u )
{
	TqFloat phicent = ( m_PhiMin + m_PhiMax ) * 0.5;
	TqFloat arccent = ( m_ThetaMin + m_ThetaMax ) * 0.5;

	boost::shared_ptr<CqSphere> pNew1( new CqSphere() );
	boost::shared_ptr<CqSphere> pNew2( new CqSphere() );
	pNew1->m_matTx =pNew2->m_matTx = m_matTx;
	pNew1->m_matITTx = pNew2->m_matITTx = m_matITTx;
	pNew1->m_fDiceable = pNew2->m_fDiceable = m_fDiceable;
	pNew1->m_Radius = m_Radius;
	pNew2->m_Radius = m_Radius;
	pNew1->m_fDiceable = pNew2->m_fDiceable = m_fDiceable;
	pNew1->m_fDiscard = pNew2->m_fDiscard = m_fDiscard;

	if ( u )
	{
		pNew1->m_ThetaMax = arccent;
		pNew2->m_ThetaMin = arccent;
		pNew1->m_ThetaMin = m_ThetaMin;
		pNew2->m_ThetaMax = m_ThetaMax;
		pNew1->m_PhiMin = pNew2->m_PhiMin = m_PhiMin;
		pNew1->m_PhiMax = pNew2->m_PhiMax = m_PhiMax;
	}
	else
	{
		pNew1->m_PhiMax = phicent;
		pNew2->m_PhiMin = phicent;
		pNew1->m_PhiMin = m_PhiMin;
		pNew2->m_PhiMax = m_PhiMax;
		pNew1->m_ThetaMin = pNew2->m_ThetaMin = m_ThetaMin;
		pNew1->m_ThetaMax = pNew2->m_ThetaMax = m_ThetaMax;
	}

	aSplits.push_back( pNew1 );
	aSplits.push_back( pNew2 );

	return ( 2 );
}

/** Calculate all points on the surface indexed by the surface paramters passed.
  * \param pointGrid Imath::V3f surface points.
  * \param normalGrid Imath::V3f surface normals.
  */
void CqSphere::DicePoints( Imath::V3f* pointGrid, Imath::V3f* normalGrid )
{
	int thetaRes = m_uDiceSize+1;
	int phiRes = m_vDiceSize+1;

	boost::scoped_array<TqFloat> sinThetaTab(new TqFloat[thetaRes]);
	boost::scoped_array<TqFloat> cosThetaTab(new TqFloat[thetaRes]);
	boost::scoped_array<TqFloat> sinPhiTab(new TqFloat[phiRes]);
	boost::scoped_array<TqFloat> cosPhiTab(new TqFloat[phiRes]);

	//build look-up tables for sin(theta), cos(theta) + sin(phi), cos(phi)
	sinCosGrid(degToRad(m_ThetaMin), degToRad(m_ThetaMax), thetaRes, sinThetaTab.get(), cosThetaTab.get());
	sinCosGrid(m_PhiMin, m_PhiMax, phiRes, sinPhiTab.get(), cosPhiTab.get());

	for (TqInt v = 0; v <= m_vDiceSize; v++ )
	{
		for (TqInt u = 0; u <= m_uDiceSize; u++ )
		{
			TqFloat cosTheta = cosThetaTab[u];
			TqFloat sinTheta = sinThetaTab[u];
			TqFloat cosPhi = cosPhiTab[v];
			TqFloat sinPhi = sinPhiTab[v];
			
			// unit point vector
			Imath::V3f unitP = Imath::V3f(cosTheta*cosPhi, sinTheta*cosPhi, sinPhi);

			// calc surface point
			TqInt gridOffset = ( v * ( m_uDiceSize + 1 ) ) + u;
			pointGrid[gridOffset] = m_Radius * unitP;

			// calc normal vector
			if (normalGrid != NULL) 
				normalGrid[gridOffset] = unitP;
		}
	}
}


//---------------------------------------------------------------------
/** Constructor.
 */

CqCone::CqCone( TqFloat height, TqFloat radius, TqFloat thetamin, TqFloat thetamax, TqFloat vmin, TqFloat vmax ) :
		m_Height( height ),
		m_Radius( radius ),
		m_vMin( vmin ),
		m_vMax( vmax ),
		m_ThetaMin( thetamin ),
		m_ThetaMax( thetamax )
{}


//---------------------------------------------------------------------
/** Create a clone of this cone
 */

CqSurface*	CqCone::Clone( ) const
{
	CqCone* clone = new CqCone();
	CqQuadric::CloneData( clone );
	clone->m_Height = m_Height;
	clone->m_Radius = m_Radius;
	clone->m_vMin = m_vMin;
	clone->m_vMax = m_vMax;
	clone->m_ThetaMin = m_ThetaMin;
	clone->m_ThetaMax = m_ThetaMax;

	return ( clone );
}



//---------------------------------------------------------------------
/** Get the geometric bound of this GPrim.
 */

void CqCone::Bound(CqBound* bound) const
{
	std::vector<Imath::V3f> curve;
	TqFloat zmin = m_vMin * m_Height;
	TqFloat zmax = m_vMax * m_Height;
	Imath::V3f vA( m_Radius, 0, zmin ), vB( 0, 0, zmax ), vC( 0, 0, 0 ), vD( 0, 0, 1 );
	curve.push_back( vA );
	curve.push_back( vB );
	CqMatrix matRot( degToRad ( m_ThetaMin ), vD );
	for ( std::vector<Imath::V3f>::iterator i = curve.begin(); i != curve.end(); i++ )
		*i = matRot * ( *i );
	CqBound	B( RevolveForBound( curve, vC, vD, degToRad( m_ThetaMax - m_ThetaMin ) ) );
	B.Transform( m_matTx );
	bound->vecMin() = B.vecMin();
	bound->vecMax() = B.vecMax();

	AdjustBoundForTransformationMotion( bound );
}


//---------------------------------------------------------------------
/** Split this GPrim into a NURBS surface. Temp implementation, should split into smalled quadrics.
 */

TqInt CqCone::PreSubdivide( std::vector<boost::shared_ptr<CqSurface> >& aSplits, bool u )
{
	TqFloat vcent = ( m_vMin + m_vMax ) * 0.5;
	TqFloat arccent = ( m_ThetaMin + m_ThetaMax ) * 0.5;

	boost::shared_ptr<CqCone> pNew1( new CqCone() );
	boost::shared_ptr<CqCone> pNew2( new CqCone() );
	pNew1->m_matTx =pNew2->m_matTx = m_matTx;
	pNew1->m_matITTx = pNew2->m_matITTx = m_matITTx;
	pNew1->m_fDiceable = pNew2->m_fDiceable = m_fDiceable;
	pNew1->m_Height = pNew2->m_Height = m_Height;
	pNew1->m_Radius = pNew2->m_Radius = m_Radius;

	if ( u )
	{
		pNew1->m_ThetaMax = arccent;
		pNew2->m_ThetaMin = arccent;
		pNew1->m_ThetaMin = m_ThetaMin;
		pNew2->m_ThetaMax = m_ThetaMax;
		pNew1->m_vMin = pNew2->m_vMin = m_vMin;
		pNew1->m_vMax = pNew2->m_vMax = m_vMax;
	}
	else
	{
		pNew1->m_vMax = vcent;
		pNew2->m_vMin = vcent;
		pNew1->m_vMin = m_vMin;
		pNew2->m_vMax = m_vMax;
		pNew1->m_ThetaMin = pNew2->m_ThetaMin = m_ThetaMin;
		pNew1->m_ThetaMax = pNew2->m_ThetaMax = m_ThetaMax;
	}

	aSplits.push_back( pNew1 );
	aSplits.push_back( pNew2 );

	return ( 2 );
}

/** Calculate all points on the surface indexed by the surface paramters passed.
  * \param pointGrid Imath::V3f surface points.
  * \param normalGrid Imath::V3f surface normals.
  */
void CqCone::DicePoints( Imath::V3f* pointGrid, Imath::V3f* normalGrid )
{
	TqInt thetaRes = m_uDiceSize + 1;
	boost::scoped_array<TqFloat> sinThetaTab(new TqFloat[thetaRes]);
	boost::scoped_array<TqFloat> cosThetaTab(new TqFloat[thetaRes]);

	//build look-up tables for sin(theta), cos(theta)
	sinCosGrid(degToRad(m_ThetaMin), degToRad(m_ThetaMax), thetaRes, sinThetaTab.get(), cosThetaTab.get());

	TqFloat coneLength = sqrt( m_Height * m_Height + m_Radius * m_Radius );
	TqFloat xN = m_Height / coneLength;
	TqFloat normalZ = m_Radius / coneLength;
		
	for (TqInt v = 0; v <= m_vDiceSize; v++ )
	{
		for (TqInt u = 0; u <= m_uDiceSize; u++ )
		{
			// calc surface point
			TqFloat zmin = m_vMin * m_Height;
			TqFloat zmax = m_vMax * m_Height;
			TqFloat z = zmin + ( ( TqFloat ) v * ( zmax - zmin ) ) / m_vDiceSize;
			TqFloat vv = m_vMin + ( ( TqFloat ) v * ( m_vMax - m_vMin ) ) / m_vDiceSize;
			TqFloat r = m_Radius * ( 1.0 - vv );
			TqFloat cosTheta = cosThetaTab[u];
			TqFloat sinTheta = sinThetaTab[u];

			TqInt gridOffset = ( v * ( m_uDiceSize + 1 ) ) + u;
			pointGrid[gridOffset] = Imath::V3f( r * cosTheta, r * sinTheta, z );

			// calc normal
			if (normalGrid != NULL) 
			{		
				Imath::V3f Normal;
				Normal.x = xN * cosTheta;
				Normal.y = xN * sinTheta;
				Normal.z = normalZ;

				normalGrid[gridOffset] = Normal;
			}
		}
	}
}

//---------------------------------------------------------------------
/** Constructor.
 */

CqCylinder::CqCylinder( TqFloat radius, TqFloat zmin, TqFloat zmax, TqFloat thetamin, TqFloat thetamax ) :
		m_Radius( radius ),
		m_ZMin( zmin ),
		m_ZMax( zmax ),
		m_ThetaMin( thetamin ),
		m_ThetaMax( thetamax )
{}


//---------------------------------------------------------------------
/** Create a clone of this cylinder.
 */

CqSurface*	CqCylinder::Clone() const
{
	CqCylinder* clone = new CqCylinder();
	CqQuadric::CloneData( clone );
	clone->m_Radius = m_Radius;
	clone->m_ZMin = m_ZMin;
	clone->m_ZMax = m_ZMax;
	clone->m_ThetaMin = m_ThetaMin;
	clone->m_ThetaMax = m_ThetaMax;

	return ( clone );
}



//---------------------------------------------------------------------
/** Get the geometric bound of this GPrim.
 */

void	CqCylinder::Bound(CqBound* bound) const
{
	std::vector<Imath::V3f> curve;
	Imath::V3f vA( m_Radius, 0, m_ZMin ), vB( m_Radius, 0, m_ZMax ), vC( 0, 0, 0 ), vD( 0, 0, 1 );
	curve.push_back( vA );
	curve.push_back( vB );
	CqMatrix matRot( degToRad ( m_ThetaMin ), vD );
	for ( std::vector<Imath::V3f>::iterator i = curve.begin(); i != curve.end(); i++ )
		*i = matRot * ( *i );
	CqBound	B( RevolveForBound( curve, vC, vD, degToRad( m_ThetaMax - m_ThetaMin ) ) );
	B.Transform( m_matTx );
	bound->vecMin() = B.vecMin();
	bound->vecMax() = B.vecMax();

	AdjustBoundForTransformationMotion( bound );
}


//---------------------------------------------------------------------
/** Split this GPrim into a NURBS surface. Temp implementation, should split into smalled quadrics.
 */

TqInt CqCylinder::PreSubdivide( std::vector<boost::shared_ptr<CqSurface> >& aSplits, bool u )
{
	TqFloat zcent = ( m_ZMin + m_ZMax ) * 0.5;
	TqFloat arccent = ( m_ThetaMin + m_ThetaMax ) * 0.5;

	boost::shared_ptr<CqCylinder> pNew1( new CqCylinder() );
	boost::shared_ptr<CqCylinder> pNew2( new CqCylinder() );
	pNew1->m_matTx =pNew2->m_matTx = m_matTx;
	pNew1->m_matITTx = pNew2->m_matITTx = m_matITTx;
	pNew1->m_fDiceable = pNew2->m_fDiceable = m_fDiceable;
	pNew1->m_Radius = pNew2->m_Radius = m_Radius;

	if ( u )
	{
		pNew1->m_ThetaMax = arccent;
		pNew2->m_ThetaMin = arccent;
		pNew1->m_ThetaMin = m_ThetaMin;
		pNew2->m_ThetaMax = m_ThetaMax;
		pNew1->m_ZMin = pNew2->m_ZMin = m_ZMin;
		pNew1->m_ZMax = pNew2->m_ZMax = m_ZMax;
	}
	else
	{
		pNew1->m_ZMax = zcent;
		pNew2->m_ZMin = zcent;
		pNew1->m_ZMin = m_ZMin;
		pNew2->m_ZMax = m_ZMax;
		pNew1->m_ThetaMin = pNew2->m_ThetaMin = m_ThetaMin;
		pNew1->m_ThetaMax = pNew2->m_ThetaMax = m_ThetaMax;
	}

	aSplits.push_back( pNew1 );
	aSplits.push_back( pNew2 );

	return ( 2 );
}

/** Calculate all points on the surface indexed by the surface paramters passed.
  * \param pointGrid Imath::V3f surface points.
  * \param normalGrid Imath::V3f surface normals.
  */
void CqCylinder::DicePoints( Imath::V3f* pointGrid, Imath::V3f* normalGrid )
{
	TqInt thetaRes = m_uDiceSize + 1;
	boost::scoped_array<TqFloat> sinThetaTab(new TqFloat[thetaRes]);
	boost::scoped_array<TqFloat> cosThetaTab(new TqFloat[thetaRes]);

	//build look-up tables for sin(theta), cos(theta)
	sinCosGrid(degToRad(m_ThetaMin), degToRad(m_ThetaMax), thetaRes, sinThetaTab.get(), cosThetaTab.get());

	for (TqInt v = 0; v <= m_vDiceSize; v++ )
	{
		for (TqInt u = 0; u <= m_uDiceSize; u++ )
		{
			// calc surface point
			TqFloat sinTheta = sinThetaTab[u];
			TqFloat cosTheta = cosThetaTab[u];
			TqFloat vz = m_ZMin + ( ( TqFloat ) v * ( m_ZMax - m_ZMin ) ) / m_vDiceSize;
			Imath::V3f point =  Imath::V3f( m_Radius * cosTheta, m_Radius * sinTheta, vz );
			TqInt gridOffset = ( v * ( m_uDiceSize + 1 ) ) + u;
			pointGrid[gridOffset] = point;

			// calc normal
			if (normalGrid != NULL) 
			{
				Imath::V3f normal = point;
				normal.z = 0;

				normalGrid[gridOffset] = normal;
			}
		}
	}
}

//---------------------------------------------------------------------
/** Constructor.
 */

CqHyperboloid::CqHyperboloid( )
{
	m_Point1 = Imath::V3f( 0.0f, 0.0f, 0.0f );
	m_Point2 = Imath::V3f( 0.0f, 0.0f, 1.0f );
	m_ThetaMin = 0.0f;
	m_ThetaMax = 1.0f;
}

//---------------------------------------------------------------------
/** Constructor.
 */

CqHyperboloid::CqHyperboloid( Imath::V3f& point1, Imath::V3f& point2, TqFloat thetamin, TqFloat thetamax ) :
		m_Point1( point1 ),
		m_Point2( point2 ),
		m_ThetaMin( thetamin ),
		m_ThetaMax( thetamax )
{}


//---------------------------------------------------------------------
/** Clone a copy of this hyperboloid.
 */

CqSurface*	CqHyperboloid::Clone() const
{
	CqHyperboloid* clone = new CqHyperboloid();
	CqQuadric::CloneData( clone );
	clone->m_Point1 = m_Point1;
	clone->m_Point2 = m_Point2;
	clone->m_ThetaMin = m_ThetaMin;
	clone->m_ThetaMax = m_ThetaMax;

	return ( clone );
}


//---------------------------------------------------------------------
/** Get the geometric bound of this GPrim.
 */

void CqHyperboloid::Bound(CqBound* bound) const
{
	std::vector<Imath::V3f> curve;
	curve.push_back( m_Point1 );
	curve.push_back( m_Point2 );
	Imath::V3f vA( 0, 0, 0 ), vB( 0, 0, 1 );
	CqMatrix matRot( degToRad ( m_ThetaMin ), vB );
	for ( std::vector<Imath::V3f>::iterator i = curve.begin(); i != curve.end(); i++ )
		*i = matRot * ( *i );
	CqBound	B( RevolveForBound( curve, vA, vB, degToRad( m_ThetaMax - m_ThetaMin ) ) );
	B.Transform( m_matTx );
	bound->vecMin() = B.vecMin();
	bound->vecMax() = B.vecMax();

	AdjustBoundForTransformationMotion( bound );
}


//---------------------------------------------------------------------
/** Split this GPrim into a NURBS surface. Temp implementation, should split into smalled quadrics.
 */

TqInt CqHyperboloid::PreSubdivide( std::vector<boost::shared_ptr<CqSurface> >& aSplits, bool u )
{
	TqFloat arccent = ( m_ThetaMin + m_ThetaMax ) * 0.5;
	Imath::V3f midpoint = ( m_Point1 + m_Point2 ) / 2.0;

	boost::shared_ptr<CqHyperboloid> pNew1( new CqHyperboloid() );
	boost::shared_ptr<CqHyperboloid> pNew2( new CqHyperboloid() );
	pNew1->m_matTx =pNew2->m_matTx = m_matTx;
	pNew1->m_matITTx = pNew2->m_matITTx = m_matITTx;
	pNew1->m_fDiceable = pNew2->m_fDiceable = m_fDiceable;

	if ( u )
	{
		pNew1->m_ThetaMax = arccent;
		pNew2->m_ThetaMin = arccent;
		pNew1->m_ThetaMin = m_ThetaMin;
		pNew2->m_ThetaMax = m_ThetaMax;
		pNew1->m_Point1 = pNew2->m_Point1 = m_Point1;
		pNew1->m_Point2 = pNew2->m_Point2 = m_Point2;
	}
	else
	{
		pNew1->m_Point2 = midpoint;
		pNew2->m_Point1 = midpoint;
		pNew1->m_Point1 = m_Point1;
		pNew2->m_Point2 = m_Point2;
		pNew1->m_ThetaMin = pNew2->m_ThetaMin = m_ThetaMin;
		pNew1->m_ThetaMax = pNew2->m_ThetaMax = m_ThetaMax;
	}

	aSplits.push_back( pNew1 );
	aSplits.push_back( pNew2 );

	return ( 2 );
}

/** Calculate all points on the surface indexed by the surface paramters passed.
  * \param pointGrid Imath::V3f surface points.
  * \param normalGrid Imath::V3f surface normals.
  */
void CqHyperboloid::DicePoints( Imath::V3f* pointGrid, Imath::V3f* normalGrid )
{
	TqInt thetaRes = m_uDiceSize + 1;
	boost::scoped_array<TqFloat> sinThetaTab(new TqFloat[thetaRes]);
	boost::scoped_array<TqFloat> cosThetaTab(new TqFloat[thetaRes]);

	//build look-up tables for sin(theta), cos(theta)
	sinCosGrid(degToRad(m_ThetaMin), degToRad(m_ThetaMax), thetaRes, sinThetaTab.get(), cosThetaTab.get());

	for (TqInt v = 0; v <= m_vDiceSize; v++ )
	{
		for (TqInt u = 0; u <= m_uDiceSize; u++ )
		{
			TqFloat cosTheta = cosThetaTab[u];
			TqFloat sinTheta = sinThetaTab[u];

			// calc surface point
			Imath::V3f p;
			TqFloat vv = static_cast<TqFloat>( v ) / m_vDiceSize;
			p = m_Point1 * ( 1.0 - vv ) + m_Point2 * vv;

			TqInt gridOffset = ( v * ( m_uDiceSize + 1 ) ) + u;
			pointGrid[gridOffset] = Imath::V3f( p.x * cosTheta - p.y * sinTheta, p.x * sinTheta + p.y * cosTheta, p.z );

			// Calculate the normal vector - this is a bit tortuous, and uses the general
			// formula for the normal to a surface that is specified by two parametric
			// parameters.
			if (normalGrid != NULL) 
			{
				// Calculate a vector, a, of derivatives of coordinates w.r.t. u
				TqFloat dxdu = -p.x * m_ThetaMax * sinTheta - p.y * m_ThetaMax * cosTheta;
				TqFloat dydu =  p.x * m_ThetaMax * cosTheta - p.y * m_ThetaMax * sinTheta;
				TqFloat dzdu = 0.0;
				Imath::V3f a(dxdu, dydu, dzdu);

				// Calculate a vector, b, of derivatives of coordinates w.r.t. v
				Imath::V3f p2p1 = m_Point2 - m_Point1;
				TqFloat dxdv = p2p1.x * cosTheta  -  p2p1.y * sinTheta;
				TqFloat dydv = p2p1.x * sinTheta  +  p2p1.y * cosTheta;
				TqFloat dzdv = p2p1.z;
				Imath::V3f b(dxdv, dydv, dzdv);

				// The normal vector points in the direction of: a x b
				Imath::V3f Normal = a % b;
				normalGrid[gridOffset] = Normal;
			}
		}
	}
}

//---------------------------------------------------------------------
/** Constructor.
 */

CqParaboloid::CqParaboloid( TqFloat rmax, TqFloat zmin, TqFloat zmax, TqFloat thetamin, TqFloat thetamax ) :
		m_RMax( rmax ),
		m_ZMin( zmin ),
		m_ZMax( zmax ),
		m_ThetaMin( thetamin ),
		m_ThetaMax( thetamax )
{}


//---------------------------------------------------------------------
/** Create a clone of this paraboloid.
 */

CqSurface*	CqParaboloid::Clone() const
{
	CqParaboloid* clone = new CqParaboloid();
	CqQuadric::CloneData( clone );
	clone->m_RMax = m_RMax;
	clone->m_ZMin = m_ZMin;
	clone->m_ZMax = m_ZMax;
	clone->m_ThetaMin = m_ThetaMin;
	clone->m_ThetaMax = m_ThetaMax;

	return ( clone );
}



//---------------------------------------------------------------------
/** Get the geometric bound of this GPrim.
 */

void CqParaboloid::Bound(CqBound* bound) const
{
	TqFloat x1 = m_RMax * cos( degToRad( 0 ) );
	TqFloat x2 = m_RMax * cos( degToRad( 180 ) );
	TqFloat y1 = m_RMax * sin( degToRad( 90 ) );
	TqFloat y2 = m_RMax * sin( degToRad( 270 ) );

	Imath::V3f vecMin( min( x1, x2 ), min( y1, y2 ), min( m_ZMin, m_ZMax ) );
	Imath::V3f vecMax( max( x1, x2 ), max( y1, y2 ), max( m_ZMin, m_ZMax ) );

	bound->vecMin() = vecMin;
	bound->vecMax() = vecMax;
	bound->Transform( m_matTx );

	AdjustBoundForTransformationMotion( bound );
}


//---------------------------------------------------------------------
/** Split this GPrim into smaller quadrics.
 */

TqInt CqParaboloid::PreSubdivide( std::vector<boost::shared_ptr<CqSurface> >& aSplits, bool u )
{
	TqFloat zcent = ( m_ZMin + m_ZMax ) * 0.5;
	TqFloat arccent = ( m_ThetaMin + m_ThetaMax ) * 0.5;
	TqFloat rcent = m_RMax * sqrt( zcent / m_ZMax );

	boost::shared_ptr<CqParaboloid> pNew1( new CqParaboloid() );
	boost::shared_ptr<CqParaboloid> pNew2( new CqParaboloid() );
	pNew1->m_matTx =pNew2->m_matTx = m_matTx;
	pNew1->m_matITTx = pNew2->m_matITTx = m_matITTx;
	pNew1->m_fDiceable = pNew2->m_fDiceable = m_fDiceable;

	if ( u )
	{
		pNew1->m_ThetaMax = arccent;
		pNew2->m_ThetaMin = arccent;
		pNew1->m_ThetaMin = m_ThetaMin;
		pNew2->m_ThetaMax = m_ThetaMax;
		pNew1->m_RMax = pNew2->m_RMax = m_RMax;
		pNew1->m_ZMin = pNew2->m_ZMin = m_ZMin;
		pNew1->m_ZMax = pNew2->m_ZMax = m_ZMax;
	}
	else
	{
		pNew1->m_ZMax = zcent;
		pNew1->m_RMax = rcent;
		pNew2->m_ZMin = zcent;
		pNew1->m_ZMin = m_ZMin;
		pNew2->m_ZMax = m_ZMax;
		pNew2->m_RMax = m_RMax;
		pNew1->m_ThetaMin = pNew2->m_ThetaMin = m_ThetaMin;
		pNew1->m_ThetaMax = pNew2->m_ThetaMax = m_ThetaMax;
	}

	aSplits.push_back( pNew1 );
	aSplits.push_back( pNew2 );

	return ( 2 );
}

/** Calculate all points on the surface indexed by the surface paramters passed.
  * \param pointGrid Imath::V3f surface points.
  * \param normalGrid Imath::V3f surface normals.
  */
void CqParaboloid::DicePoints( Imath::V3f* pointGrid, Imath::V3f* normalGrid )
{
	TqInt thetaRes = m_uDiceSize + 1;
	boost::scoped_array<TqFloat> sinThetaTab(new TqFloat[thetaRes]);
	boost::scoped_array<TqFloat> cosThetaTab(new TqFloat[thetaRes]);

	//build look-up tables for sin(theta), cos(theta)
	sinCosGrid(degToRad(m_ThetaMin), degToRad(m_ThetaMax), thetaRes, sinThetaTab.get(), cosThetaTab.get());

	for (TqInt v = 0; v <= m_vDiceSize; v++ )
	{
		for (TqInt u = 0; u <= m_uDiceSize; u++ )
		{
			TqFloat cosTheta = cosThetaTab[u];
			TqFloat sinTheta = sinThetaTab[u];

			// calc surface point
			TqFloat z = m_ZMin + ( ( TqFloat ) v * ( m_ZMax - m_ZMin ) ) / m_vDiceSize;
			TqFloat r = m_RMax * sqrt( z / m_ZMax );

			TqInt gridOffset = ( v * ( m_uDiceSize + 1 ) ) + u;
			pointGrid[gridOffset] = Imath::V3f( r * cosTheta, r * sinTheta, z );

			// calc normal
			if (normalGrid != NULL) 
			{
				TqFloat normalZ;
				if (r == 0) 
					normalZ = -1;
				else
					normalZ = -0.5*m_RMax*m_RMax/m_ZMax / r;
					
				normalGrid[gridOffset] = Imath::V3f(cosTheta, sinTheta, normalZ);
			}
		}
	}
}


//---------------------------------------------------------------------
/** Constructor.
 */

CqTorus::CqTorus( TqFloat majorradius, TqFloat minorradius, TqFloat phimin, TqFloat phimax, TqFloat thetamin, TqFloat thetamax ) :
		m_MajorRadius( majorradius ),
		m_MinorRadius( minorradius ),
		m_PhiMin( phimin ),
		m_PhiMax( phimax ),
		m_ThetaMin( thetamin ),
		m_ThetaMax( thetamax )
{}


//---------------------------------------------------------------------
/** Create a clone copy of this torus.
 */

CqSurface*	CqTorus::Clone() const
{
	CqTorus* clone = new CqTorus();
	CqQuadric::CloneData( clone );
	clone->m_MajorRadius = m_MajorRadius;
	clone->m_MinorRadius = m_MinorRadius;
	clone->m_PhiMax = m_PhiMax;
	clone->m_PhiMin = m_PhiMin;
	clone->m_ThetaMin = m_ThetaMin;
	clone->m_ThetaMax = m_ThetaMax;

	return ( clone );
}



//---------------------------------------------------------------------
/** Get the geometric bound of this GPrim.
 */

void CqTorus::Bound(CqBound* bound) const
{
	std::vector<Imath::V3f> curve;
	Imath::V3f vA( m_MajorRadius, 0, 0 ), vB( 1, 0, 0 ), vC( 0, 0, 1 ), vD( 0, 0, 0 );
	Circle( vA, vB, vC, m_MinorRadius, degToRad( std::min(m_PhiMin, m_PhiMax) ), degToRad( std::max(m_PhiMin, m_PhiMax) ), curve );
	CqMatrix matRot( degToRad ( m_ThetaMin ), vC );
	for ( std::vector<Imath::V3f>::iterator i = curve.begin(); i != curve.end(); i++ )
		*i = matRot * ( *i );
	CqBound	B( RevolveForBound( curve, vD, vC, degToRad( m_ThetaMax - m_ThetaMin ) ) );
	B.Transform( m_matTx );
	bound->vecMin() = B.vecMin();
	bound->vecMax() = B.vecMax();

	AdjustBoundForTransformationMotion( bound );
}


//---------------------------------------------------------------------
/** Split this GPrim into a NURBS surface. Temp implementation, should split into smalled quadrics.
 */

TqInt CqTorus::PreSubdivide( std::vector<boost::shared_ptr<CqSurface> >& aSplits, bool u )
{
	TqFloat zcent = ( m_PhiMax + m_PhiMin ) * 0.5;
	TqFloat arccent = ( m_ThetaMin + m_ThetaMax ) * 0.5;

	boost::shared_ptr<CqTorus> pNew1( new CqTorus() );
	boost::shared_ptr<CqTorus> pNew2( new CqTorus() );
	pNew1->m_matTx =pNew2->m_matTx = m_matTx;
	pNew1->m_matITTx = pNew2->m_matITTx = m_matITTx;
	pNew1->m_fDiceable = pNew2->m_fDiceable = m_fDiceable;
	pNew1->m_MajorRadius = pNew2->m_MajorRadius = m_MajorRadius;
	pNew1->m_MinorRadius = pNew2->m_MinorRadius = m_MinorRadius;

	if ( u )
	{
		pNew1->m_ThetaMax = arccent;
		pNew2->m_ThetaMin = arccent;
		pNew1->m_ThetaMin = m_ThetaMin;
		pNew2->m_ThetaMax = m_ThetaMax;
		pNew1->m_PhiMax = pNew2->m_PhiMax = m_PhiMax;
		pNew1->m_PhiMin = pNew2->m_PhiMin = m_PhiMin;
	}
	else
	{
		pNew1->m_PhiMax = zcent;
		pNew2->m_PhiMin = zcent;
		pNew1->m_PhiMin = m_PhiMin;
		pNew2->m_PhiMax = m_PhiMax;
		pNew1->m_ThetaMin = pNew2->m_ThetaMin = m_ThetaMin;
		pNew1->m_ThetaMax = pNew2->m_ThetaMax = m_ThetaMax;
	}

	aSplits.push_back( pNew1 );
	aSplits.push_back( pNew2 );

	return ( 2 );
}

/** Calculate all points on the surface indexed by the surface paramters passed.
  * \param pointGrid Imath::V3f surface points.
  * \param normalGrid Imath::V3f surface normals.
  */
void CqTorus::DicePoints( Imath::V3f* pointGrid, Imath::V3f* normalGrid )
{
	int thetaRes = m_uDiceSize+1;
	int phiRes = m_vDiceSize+1;

	boost::scoped_array<TqFloat> sinThetaTab(new TqFloat[thetaRes]);
	boost::scoped_array<TqFloat> cosThetaTab(new TqFloat[thetaRes]);
	boost::scoped_array<TqFloat> sinPhiTab(new TqFloat[phiRes]);
	boost::scoped_array<TqFloat> cosPhiTab(new TqFloat[phiRes]);

	//build look-up tables for sin(theta), cos(theta) + sin(phi), cos(phi)
	sinCosGrid(degToRad(m_ThetaMin), degToRad(m_ThetaMax), thetaRes, sinThetaTab.get(), cosThetaTab.get());
	sinCosGrid(degToRad(m_PhiMin), degToRad(m_PhiMax), phiRes, sinPhiTab.get(), cosPhiTab.get());

	for (TqInt v = 0; v <= m_vDiceSize; v++ )
	{
		for (TqInt u = 0; u <= m_uDiceSize; u++ )
		{
			TqFloat sinTheta = sinThetaTab[u];
			TqFloat cosTheta = cosThetaTab[u];
			TqFloat sinPhi = sinPhiTab[v];
			TqFloat cosPhi = cosPhiTab[v];

			// calc surface point
			TqFloat r = m_MinorRadius * cosPhi;
			TqFloat z = m_MinorRadius * sinPhi;
			TqInt gridOffset = ( v * ( m_uDiceSize + 1 ) ) + u;
			pointGrid[gridOffset] = Imath::V3f( ( m_MajorRadius + r ) * cosTheta, ( m_MajorRadius + r ) * sinTheta, z );

			// calc normal
			if (normalGrid != NULL) 
			{
				Imath::V3f Normal;
				Normal.x = cosPhi * cosTheta;
				Normal.y = cosPhi * sinTheta;
				Normal.z = sinPhi;
				
				normalGrid[gridOffset] = Normal;
			}
		}
	}
}

//---------------------------------------------------------------------
/** Constructor.
 */

CqDisk::CqDisk( TqFloat height, TqFloat minorradius, TqFloat majorradius, TqFloat thetamin, TqFloat thetamax ) :
		m_Height( height ),
		m_MajorRadius( majorradius ),
		m_MinorRadius( minorradius ),
		m_ThetaMin( thetamin ),
		m_ThetaMax( thetamax )
{}


//---------------------------------------------------------------------
/** Create a clone of this disk.
 */

CqSurface*	CqDisk::Clone() const
{
	CqDisk* clone = new CqDisk();
	CqQuadric::CloneData( clone );
	clone->m_Height = m_Height;
	clone->m_MajorRadius = m_MajorRadius;
	clone->m_MinorRadius = m_MinorRadius;
	clone->m_ThetaMin = m_ThetaMin;
	clone->m_ThetaMax = m_ThetaMax;

	return ( clone );
}



//---------------------------------------------------------------------
/** Get the geometric bound of this GPrim.
 */

void CqDisk::Bound(CqBound* bound) const
{
	std::vector<Imath::V3f> curve;
	Imath::V3f vA( m_MajorRadius, 0, m_Height ), vB( m_MinorRadius, 0, m_Height ), vC( 0, 0, 0 ), vD( 0, 0, 1 );
	curve.push_back( vA );
	curve.push_back( vB );
	CqMatrix matRot( degToRad ( m_ThetaMin ), vD );
	for ( std::vector<Imath::V3f>::iterator i = curve.begin(); i != curve.end(); i++ )
		*i = matRot * ( *i );
	CqBound	B( RevolveForBound( curve, vC, vD, degToRad( m_ThetaMax - m_ThetaMin ) ) );
	B.Transform( m_matTx );
	bound->vecMin() = B.vecMin();
	bound->vecMax() = B.vecMax();

	AdjustBoundForTransformationMotion( bound );
}


//---------------------------------------------------------------------
/** Split this GPrim into a NURBS surface. Temp implementation, should split into smalled quadrics.
 */

TqInt CqDisk::PreSubdivide( std::vector<boost::shared_ptr<CqSurface> >& aSplits, bool u )
{
	TqFloat zcent = ( m_MajorRadius + m_MinorRadius ) * 0.5;
	TqFloat arccent = ( m_ThetaMin + m_ThetaMax ) * 0.5;

	boost::shared_ptr<CqDisk> pNew1( new CqDisk() );
	boost::shared_ptr<CqDisk> pNew2( new CqDisk() );
	pNew1->m_matTx =pNew2->m_matTx = m_matTx;
	pNew1->m_matITTx = pNew2->m_matITTx = m_matITTx;
	pNew1->m_fDiceable = pNew2->m_fDiceable = m_fDiceable;
	pNew1->m_Height = pNew2->m_Height = m_Height;

	if ( u )
	{
		pNew1->m_ThetaMax = arccent;
		pNew2->m_ThetaMin = arccent;
		pNew1->m_ThetaMin = m_ThetaMin;
		pNew2->m_ThetaMax = m_ThetaMax;
		pNew1->m_MajorRadius = pNew2->m_MajorRadius = m_MajorRadius;
		pNew1->m_MinorRadius = pNew2->m_MinorRadius = m_MinorRadius;
	}
	else
	{
		pNew1->m_MinorRadius = zcent;
		pNew2->m_MajorRadius = zcent;
		pNew1->m_MajorRadius = m_MajorRadius;
		pNew2->m_MinorRadius = m_MinorRadius;
		pNew1->m_ThetaMin = pNew2->m_ThetaMin = m_ThetaMin;
		pNew1->m_ThetaMax = pNew2->m_ThetaMax = m_ThetaMax;
	}

	aSplits.push_back( pNew1 );
	aSplits.push_back( pNew2 );

	return ( 2 );
}

/** Calculate all points on the surface indexed by the surface paramters passed.
  * \param pointGrid Imath::V3f surface points.
  * \param normalGrid Imath::V3f surface normals.
  */
void CqDisk::DicePoints( Imath::V3f* pointGrid, Imath::V3f* normalGrid )
{
	TqInt thetaRes = m_uDiceSize + 1;
	boost::scoped_array<TqFloat> sinThetaTab(new TqFloat[thetaRes]);
	boost::scoped_array<TqFloat> cosThetaTab(new TqFloat[thetaRes]);

	//build look-up tables for sin(theta), cos(theta)
	sinCosGrid(degToRad(m_ThetaMin), degToRad(m_ThetaMax), thetaRes, sinThetaTab.get(), cosThetaTab.get());

	for (TqInt v = 0; v <= m_vDiceSize; v++ )
	{
		for (TqInt u = 0; u <= m_uDiceSize; u++ )
		{
			TqFloat sinTheta = sinThetaTab[u];
			TqFloat cosTheta = cosThetaTab[u];

			// calc surface point
			TqFloat vv = m_MajorRadius - ( ( TqFloat ) v * ( m_MajorRadius - m_MinorRadius ) ) / m_vDiceSize;
			TqInt gridOffset = ( v * ( m_uDiceSize + 1 ) ) + u;
			pointGrid[gridOffset] = Imath::V3f( vv * cosTheta, vv * sinTheta, m_Height );

			// calc normal
			if (normalGrid != NULL) 
			{
				Imath::V3f Normal = Imath::V3f( 0, 0, m_ThetaMax > 0 ? 1 : -1 );
				normalGrid[gridOffset] = Normal;
			}
		}
	}
}

//------------------------------------------------------------------------------
/**
 *	Create the points which make up a NURBS circle control hull, for use during boundary
 *  generation.
 *
 *	\param	O	Origin of the circle.
 *	\param	X	X axis of the plane to generate the circle in.
 *	\param	Y	Y axis of the plane to generate the circle in.
 *	\param	r	Radius of the circle.
 *	\param	as	Start angle of the circle.
 *	\param	ae	End angle of the circle.
 *	\param	points	Storage for the points of the circle.
 */

void CqQuadric::Circle( const Imath::V3f& O, const Imath::V3f& X, const Imath::V3f& Y, TqFloat r, TqFloat as, TqFloat ae, std::vector<Imath::V3f>& points ) const
{
	TqFloat theta, angle, dtheta;
	TqUint narcs;

	while ( ae < as )
		ae += 2 * RI_PI;

	theta = ae - as;
	narcs = 4;

	dtheta = theta / static_cast<TqFloat>( narcs );
	TqUint n = 2 * narcs + 1;				// n control points ;

	Imath::V3f P0, T0, P2, T2, P1;
	P0 = O + X * r * cos( as ) + Y * r * sin( as );
	T0 = X * -sin( as ) + Y * cos( as );		// initialize start values

	points.resize( n );

	points[ 0 ] = P0;
	TqUint index = 0;
	angle = as;

	TqUint i;
	for ( i = 1; i <= narcs; i++ )
	{
		angle += dtheta;
		P2 = O + X * r * cos( angle ) + Y * r * sin( angle );
		points[ index + 2 ] = P2;
		T2 = X * -sin( angle ) + Y * cos( angle );
		IntersectLine( P0, T0, P2, T2, P1 );
		points[ index + 1 ] = P1;
		index += 2;
		if ( i < narcs )
		{
			P0 = P2;
			T0 = T2;
		}
	}
}



CqBound CqQuadric::RevolveForBound( const std::vector<Imath::V3f>& profile, const Imath::V3f& S, const Imath::V3f& Tvec, TqFloat theta ) const
{
	CqBound bound( FLT_MAX, FLT_MAX, FLT_MAX, -FLT_MAX, -FLT_MAX, -FLT_MAX );

	TqFloat angle, dtheta;
	TqUint narcs;
	TqUint i, j;

	if ( fabs( theta ) > 2.0 * RI_PI )
	{
		if ( theta < 0 )
			theta = -( 2.0 * RI_PI );
		else
			theta = 2.0 * RI_PI;
	}
	
	narcs = 4;
	dtheta = theta / static_cast<TqFloat>( narcs );

	std::vector<TqFloat> cosines( narcs + 1 );
	std::vector<TqFloat> sines( narcs + 1 );

	angle = 0.0;
	for ( i = 1; i <= narcs; i++ )
	{
		angle = dtheta * static_cast<TqFloat>( i );
		cosines[ i ] = cos( angle );
		sines[ i ] = sin( angle );
	}

	Imath::V3f P0, T0, P2, T2, P1;
	Imath::V3f vecTemp;

	for ( j = 0; j < profile.size(); j++ )
	{
		Imath::V3f O;
		Imath::V3f pj( profile[ j ] );

		ProjectToLine( S, Tvec, pj, O );
		Imath::V3f X, Y;
		X = pj - O;

		TqFloat r = X.length();

		if ( r < 1e-7 )
		{
			bound.Encapsulate( O );
			continue;
		}

		X.normalize();
		Y = Tvec % X;
		Y.normalize();

		P0 = profile[ j ];
		bound.Encapsulate( P0 );

		T0 = Y;
		for ( i = 1; i <= narcs; ++i )
		{
			angle = dtheta * static_cast<TqFloat>( i );
			P2 = O + r * cosines[ i ] * X + r * sines[ i ] * Y;
			bound.Encapsulate( P2 );
			T2 = -sines[ i ] * X + cosines[ i ] * Y;
			IntersectLine( P0, T0, P2, T2, P1 );
			bound.Encapsulate( P1 );
			if ( i < narcs )
			{
				P0 = P2;
				T0 = T2;
			}
		}
	}
	return ( bound );
}


/** \brief Compute a grid of sin(t) and cos(t) at regularly spaced values.
 *
 * Computes a regular grid of numSteps sin() and cos() values spaced equally
 * between t0 and t1 inclusive:
 *
 * sint[i] = sin(t0 + dt*i);
 * cost[i] = cos(t0 + dt*i);
 *
 * Where
 *
 * dt = (t1-t0)/(numSteps-1);
 */
void sinCosGrid(TqFloat t0, TqFloat t1, TqInt numSteps,
        TqFloat* sint, TqFloat* cost)
{
	TqDouble prevCos = cos(t0);
	TqDouble prevSin = sin(t0);
    TqDouble dt = (t1 - t0)/(numSteps-1);
    TqDouble cosDt = cos(dt);
    TqDouble sinDt = sin(dt);

	cost[0] = prevCos;
	sint[0] = prevSin;
	
    for(TqInt i = 1; i < numSteps; ++i)
    {
		TqDouble currCos = cosDt * prevCos - sinDt * prevSin;
		TqDouble currSin = sinDt * prevCos + cosDt * prevSin;
        
		// truncate to float:
		cost[i] = currCos;
		sint[i] = currSin;

		// save previous calculation as double 
		prevCos = currCos;
		prevSin = currSin;
    }
}

//---------------------------------------------------------------------
/** Find the point at which two infinite lines intersect.
 * The algorithm generates a plane from one of the lines and finds the
 * intersection point between this plane and the other line.
 * \return false if they are parallel, true if they intersect.
 */

bool IntersectLine( Imath::V3f& P1, Imath::V3f& T1, Imath::V3f& P2, Imath::V3f& T2, Imath::V3f& P )
{
	Imath::V3f	v, px;

	px = T1 % ( P1 - T2 );
	v = px % T1;

	TqFloat	t = ( P1 - P2 ).dot(v);
	TqFloat vw = v.dot(T2);
	if ( ( vw * vw ) < 1.0e-07 )
		return ( false );
	t /= vw;
	P = P2 + ( ( ( P1 - P2 ) * v ) / vw ) * T2 ;
	return ( true );
}


//---------------------------------------------------------------------
/** Project a point onto a line, returns the projection point in p.
 */

void ProjectToLine( const Imath::V3f& S, const Imath::V3f& Trj, const Imath::V3f& pnt, Imath::V3f& p )
{
	Imath::V3f a = pnt - S;
	TqFloat fraction, denom;
	denom = Trj.length2();
	fraction = ( denom == 0.0 ) ? 0.0 : ( Trj.dot(a) ) / denom;
	p = fraction * Trj;
	p += S;
}

}
// namespace Aqsis
//---------------------------------------------------------------------
