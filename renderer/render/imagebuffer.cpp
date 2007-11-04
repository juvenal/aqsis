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
		\brief Implements the CqImageBuffer class responsible for rendering the primitives and storing the results.
		\author Paul C. Gregory (pgregory@aqsis.org)
*/

#include	"multitimer.h"

#include	"aqsis.h"

#ifdef WIN32
#include    <windows.h>
#endif
#include	<math.h>

#include	"stats.h"
#include	"options.h"
#include	"renderer.h"
#include	"surface.h"
#include	"micropolygon.h"
#include	"imagebuffer.h"
#include	"occlusion.h"


START_NAMESPACE( Aqsis )

static TqInt bucketmodulo = -1;
static TqInt bucketdirection = -1;

//----------------------------------------------------------------------
/** Destructor
 */

CqImageBuffer::~CqImageBuffer()
{
	DeleteImage();
}


//----------------------------------------------------------------------
/** Get the screen position for the current bucket.
 * \return Bucket position as 2d vector (xpos, ypos).
 */

CqVector2D	CqImageBuffer::BucketPosition( ) const
{
	return( BucketPosition( CurrentBucketCol(), CurrentBucketRow() ) );
}


//----------------------------------------------------------------------
/** Get the screen position for the bucket at x,y.
 * \return Bucket position as 2d vector (xpos, ypos).
 */

CqVector2D	CqImageBuffer::BucketPosition( TqInt x, TqInt y ) const
{
	CqVector2D	vecA;
	vecA.x( x );
	vecA.y( y );
	vecA.x( vecA.x() * XBucketSize() );
	vecA.y( vecA.y() * YBucketSize() );

	return ( vecA );
}

//----------------------------------------------------------------------
/** Get the bucket size for the current bucket.
 
    Usually the return value is just (XBucketSize, YBucketSize) except
    for the buckets on the right and bottom side of the image where the
    size can be smaller. The crop window is not taken into account.
 
 * \return Bucket size as 2d vector (xsize, ysize).
 */

CqVector2D CqImageBuffer::BucketSize( ) const
{
	return( BucketSize( CurrentBucketCol(), CurrentBucketRow() ) );
}


//----------------------------------------------------------------------
/** Get the bucket size for the bucket at x,y.
 
    Usually the return value is just (XBucketSize, YBucketSize) except
    for the buckets on the right and bottom side of the image where the
    size can be smaller. The crop window is not taken into account.
 
 * \return Bucket size as 2d vector (xsize, ysize).
 */

CqVector2D CqImageBuffer::BucketSize(TqInt x, TqInt y) const
{
	CqVector2D	vecA = BucketPosition(x,y);
	vecA.x( m_iXRes - vecA.x() );
	if ( vecA.x() > m_XBucketSize )
		vecA.x( m_XBucketSize );
	vecA.y( m_iYRes - vecA.y() );
	if ( vecA.y() > m_YBucketSize )
		vecA.y( m_YBucketSize );

	return ( vecA );
}


//----------------------------------------------------------------------
/** Construct the image buffer to an initial state using the current options.
 */

void	CqImageBuffer::SetImage()
{
	DeleteImage();

	m_XBucketSize = 16;
	m_YBucketSize = 16;
	const TqInt* poptBucketSize = QGetRenderContext() ->poptCurrent()->GetIntegerOption( "limits", "bucketsize" );
	if ( poptBucketSize != 0 )
	{
		m_XBucketSize = poptBucketSize[ 0 ];
		m_YBucketSize = poptBucketSize[ 1 ];
	}
	/* add artificially a border around based on the current filterwidth so the diced primitives
	    * may fit better within a bucket */
	m_FilterXWidth = QGetRenderContext() ->poptCurrent()->GetFloatOption( "System", "FilterWidth" ) [ 0 ];
	m_FilterYWidth = QGetRenderContext() ->poptCurrent()->GetFloatOption( "System", "FilterWidth" ) [ 1 ];

	m_iXRes = QGetRenderContext() ->poptCurrent()->GetIntegerOption( "System", "Resolution" ) [ 0 ];
	m_iYRes = QGetRenderContext() ->poptCurrent()->GetIntegerOption( "System", "Resolution" ) [ 1 ];
	m_CropWindowXMin = static_cast<TqInt>( CLAMP( CEIL( m_iXRes * QGetRenderContext() ->poptCurrent()->GetFloatOption( "System", "CropWindow" ) [ 0 ] ), 0, m_iXRes ) );
	m_CropWindowXMax = static_cast<TqInt>( CLAMP( CEIL( m_iXRes * QGetRenderContext() ->poptCurrent()->GetFloatOption( "System", "CropWindow" ) [ 1 ] ), 0, m_iXRes ) );
	m_CropWindowYMin = static_cast<TqInt>( CLAMP( CEIL( m_iYRes * QGetRenderContext() ->poptCurrent()->GetFloatOption( "System", "CropWindow" ) [ 2 ] ), 0, m_iYRes ) );
	m_CropWindowYMax = static_cast<TqInt>( CLAMP( CEIL( m_iYRes * QGetRenderContext() ->poptCurrent()->GetFloatOption( "System", "CropWindow" ) [ 3 ] ), 0, m_iYRes ) );
	m_cXBuckets = ( ( m_iXRes + ( m_XBucketSize-1 ) ) / m_XBucketSize );
	m_cYBuckets = ( ( m_iYRes + ( m_YBucketSize-1 ) ) / m_YBucketSize );
	m_PixelXSamples = QGetRenderContext() ->poptCurrent()->GetIntegerOption( "System", "PixelSamples" ) [ 0 ];
	m_PixelYSamples = QGetRenderContext() ->poptCurrent()->GetIntegerOption( "System", "PixelSamples" ) [ 1 ];

	m_ClippingNear = static_cast<TqFloat>( QGetRenderContext() ->poptCurrent()->GetFloatOption( "System", "Clipping" ) [ 0 ] );
	m_ClippingFar = static_cast<TqFloat>( QGetRenderContext() ->poptCurrent()->GetFloatOption( "System", "Clipping" ) [ 1 ] );
	m_DisplayMode = QGetRenderContext() ->poptCurrent()->GetIntegerOption( "System", "DisplayMode" ) [ 0 ];

	m_MaxEyeSplits = 10;
	const TqInt* poptEyeSplits = QGetRenderContext() ->poptCurrent()->GetIntegerOption( "limits", "eyesplits" );
	if ( poptEyeSplits != 0 )
		m_MaxEyeSplits = poptEyeSplits[ 0 ];

	CqBucket::SetImageBuffer(this);
	m_Buckets.resize( m_cYBuckets );
	std::vector<std::vector<CqBucket> >::iterator i;
	for( i = m_Buckets.begin(); i!=m_Buckets.end(); i++)
	{
		i->resize( m_cXBuckets );
		std::vector<CqBucket>::iterator b;
		for( b = i->begin(); b!=i->end(); b++)
			b->SetProcessed( false );
	}

	m_CurrentBucketCol = m_CurrentBucketRow = 0;
}


//----------------------------------------------------------------------
/** Delete the allocated memory for the image buffer.
 */

void	CqImageBuffer::DeleteImage()
{
	m_iXRes = 0;
	m_iYRes = 0;
}


//----------------------------------------------------------------------
/** This is called by the renderer to inform an image buffer it is no longer needed.
 */
void	CqImageBuffer::Release()
{
	delete( this );
}


//----------------------------------------------------------------------
/** Check if a surface can be culled and transform bound.
 
    This method checks if the surface lies outside the viewing volume
    and returns true if it does.
    Additionally it checks if the surface spans the eye plane and marks it
    as undiceable if it does (it might even be marked as discarded).
    It also grows the bound by half the filter width and transforms it
    into raster space.
 
 * \param Bound CqBound containing the geometric bound in camera space.
 * \param pSurface Pointer to the CqSurface derived class being processed.
 * \return Boolean indicating that the GPrim can be culled.
 
  \bug If the gprim spans the eye plane the bound is not transformed into raster
   space (how could it anyway), but PostSurface() relies on this behaviour and
   inserts EVERY gprim into buckets (using a bound that is still in camera space).
 */

bool CqImageBuffer::CullSurface( CqBound& Bound, const boost::shared_ptr<CqSurface>& pSurface )
{
	// If the primitive is completely outside of the hither-yon z range, cull it.
	if ( Bound.vecMin().z() >= QGetRenderContext() ->poptCurrent()->GetFloatOption( "System", "Clipping" ) [ 1 ] ||
	        Bound.vecMax().z() <= QGetRenderContext() ->poptCurrent()->GetFloatOption( "System", "Clipping" ) [ 0 ] )
		return ( true );

	if(QGetRenderContext()->clippingVolume().whereIs(Bound) == CqBound::Side_Outside)
	{
		return(true);
	}

	// If the primitive spans the epsilon plane and the hither plane and can be split,
	if ( Bound.vecMin().z() <= 0.0f && Bound.vecMax().z() > FLT_EPSILON )
	{
		// Mark the primitive as not dicable.
		pSurface->ForceUndiceable();

		if ( pSurface->EyeSplitCount() > m_MaxEyeSplits )
		{
			CqString objname( "unnamed" );
			const CqString* pattrName = pSurface->pAttributes() ->GetStringAttribute( "identifier", "name" );
			if ( pattrName != 0 )
				objname = pattrName[ 0 ];
			Aqsis::log() << warning << "Max eyesplits for object \"" << objname.c_str() << "\" exceeded" << std::endl;
			return( true );
		}
		return ( false );
	}

	TqFloat minz = Bound.vecMin().z();
	TqFloat maxz = Bound.vecMax().z();


	// Convert the bounds to raster space.
	Bound.Transform( QGetRenderContext() ->matSpaceToSpace( "camera", "raster", NULL, NULL, QGetRenderContext()->Time() ) );

	// Take into account depth-of-field
	if ( QGetRenderContext() ->UsingDepthOfField() )
	{
		const CqVector2D minZCoc = QGetRenderContext()->GetCircleOfConfusion( minz );
		const CqVector2D maxZCoc = QGetRenderContext()->GetCircleOfConfusion( maxz );
		TqFloat cocX = MAX( minZCoc.x(), maxZCoc.x() );
		TqFloat cocY = MAX( minZCoc.y(), maxZCoc.y() );
		Bound.vecMin().x( Bound.vecMin().x() - cocX );
		Bound.vecMin().y( Bound.vecMin().y() - cocY );
		Bound.vecMax().x( Bound.vecMax().x() + cocX );
		Bound.vecMax().y( Bound.vecMax().y() + cocY );
	}

	// And expand to account for filter size.
	Bound.vecMin().x( Bound.vecMin().x() - (m_FilterXWidth / 2.0f - 0.5f) );
	Bound.vecMin().y( Bound.vecMin().y() - (m_FilterYWidth / 2.0f - 0.5f) );
	Bound.vecMax().x( Bound.vecMax().x() + (m_FilterXWidth / 2.0f - 0.5f) );
	Bound.vecMax().y( Bound.vecMax().y() + (m_FilterYWidth / 2.0f - 0.5f) );

	// If the bounds are completely outside the viewing frustum, cull the primitive.
	if ( Bound.vecMin().x() > CropWindowXMax() ||
	        Bound.vecMin().y() > CropWindowYMax() ||
	        Bound.vecMax().x() < CropWindowXMin() ||
	        Bound.vecMax().y() < CropWindowYMin() )
		return ( true );

	// Restore Z-Values to camera space.
	Bound.vecMin().z( minz );
	Bound.vecMax().z( maxz );

	// Cache the Bound.
	pSurface->CacheRasterBound( Bound );
	return ( false );
}


//----------------------------------------------------------------------
/** Add a new surface to the front of the list of waiting ones.
 * \param pSurface A pointer to a CqSurface derived class, surface should at this point be in camera space.
 */

void CqImageBuffer::PostSurface( const boost::shared_ptr<CqSurface>& pSurface )
{
	TIME_SCOPE("Post Surface")
	// Count the number of total gprims
	STATS_INC( GPR_created_total );

	// Bound the primitive in its current space (camera) space taking into account any motion specification.
	CqBound Bound;
	pSurface->Bound(&Bound);

	// Take into account the displacement bound extension.
	TqFloat db = 0.0f;
	CqString strCoordinateSystem( "object" );
	const TqFloat* pattrDispclacementBound = pSurface->pAttributes() ->GetFloatAttribute( "displacementbound", "sphere" );
	const CqString* pattrCoordinateSystem = pSurface->pAttributes() ->GetStringAttribute( "displacementbound", "coordinatesystem" );
	if ( pattrDispclacementBound != 0 )
		db = pattrDispclacementBound[ 0 ];
	if ( pattrCoordinateSystem != 0 )
		strCoordinateSystem = pattrCoordinateSystem[ 0 ];

	if ( db != 0.0f )
	{
		CqVector3D	vecDB( db, 0, 0 );
		const IqTransform* transShaderToWorld = NULL;
		// Default "shader" space to the displacement shader, unless there isn't one, in which
		// case use the surface shader.
		if ( pSurface->pAttributes() ->pshadDisplacement(QGetRenderContextI()->Time()) )
			transShaderToWorld = pSurface->pAttributes() ->pshadDisplacement(QGetRenderContextI()->Time()) ->getTransform();
		else if ( pSurface->pAttributes() ->pshadSurface(QGetRenderContextI()->Time()) )
			transShaderToWorld = pSurface->pAttributes() ->pshadSurface(QGetRenderContextI()->Time()) ->getTransform();
		vecDB = QGetRenderContext() ->matVSpaceToSpace( strCoordinateSystem.c_str(), "camera", transShaderToWorld, pSurface->pTransform().get(), QGetRenderContextI()->Time() ) * vecDB;
		db = vecDB.Magnitude();

		Bound.vecMax() += db;
		Bound.vecMin() -= db;
	}

	// Check if the surface can be culled. (also adjusts for DOF and converts Bound to raster space).
	if ( CullSurface( Bound, pSurface ) )
	{
		STATS_INC( GPR_culled );
		return ;
	}

	// If the primitive has been marked as undiceable by the eyeplane check, then we cannot get a valid
	// bucket index from it as the projection of the bound would cross the camera plane and therefore give a false
	// result, so just put it back in the current bucket for further splitting.
	TqInt XMinb = CurrentBucketCol(), YMinb = CurrentBucketRow();
	if ( !pSurface->IsUndiceable() )
	{
		// Find out which bucket(s) the surface belongs to.
		if ( Bound.vecMin().x() < 0 )
			Bound.vecMin().x( 0.0f );
		if ( Bound.vecMin().y() < 0 )
			Bound.vecMin().y( 0.0f );

		XMinb = static_cast<TqInt>( Bound.vecMin().x() ) / XBucketSize();
		YMinb = static_cast<TqInt>( Bound.vecMin().y() ) / YBucketSize();

		if ( XMinb >= cXBuckets() || YMinb >= cYBuckets() )
			return;

		XMinb = CLAMP( XMinb, 0, cXBuckets() );
		YMinb = CLAMP( YMinb, 0, cYBuckets() );

		if( Bucket(XMinb, YMinb).IsProcessed() )
		{
			XMinb = CurrentBucketCol();
			YMinb = CurrentBucketRow();
		}
	}
	// Sanity check we are not putting into a bucket that has already been processed.
	assert( !Bucket(XMinb, YMinb).IsProcessed() );
	Bucket(XMinb, YMinb).AddGPrim( pSurface );

	return ;

}

//----------------------------------------------------------------------
/** Test if this surface can be occlusion culled. If it can then
 * transfer surface to the next bucket it covers, or delete it if it
 * covers no more.
 * \param pSurface A pointer to a CqSurface derived class.
 * \return Boolean indicating that the GPrim has been culled.
*/

bool CqImageBuffer::OcclusionCullSurface( const boost::shared_ptr<CqSurface>& pSurface )
{
	CqBound RasterBound( pSurface->GetCachedRasterBound() );

	if ( CqOcclusionBox::CanCull( &RasterBound ) )
	{
		// pSurface is behind everying in this bucket but it may be
		// visible in other buckets it overlaps.
		// bucket to the right
		TqInt nextBucket = CurrentBucketCol() + 1;
		CqVector2D pos = BucketPosition( nextBucket, CurrentBucketRow() );
		if ( ( nextBucket < cXBuckets() ) &&
		        ( RasterBound.vecMax().x() >= pos.x() ) )
		{
			Bucket( nextBucket, CurrentBucketRow() ).AddGPrim( pSurface );
			return true;
		}

		// next row
		nextBucket = CurrentBucketRow() + 1;
		// find bucket containing left side of bound
		TqInt nextBucketX = static_cast<TqInt>( RasterBound.vecMin().x() ) / XBucketSize();
		nextBucketX = MAX( nextBucketX, 0 );
		pos = BucketPosition( nextBucketX, nextBucket );

		if ( ( nextBucketX < cXBuckets() ) &&
		        ( nextBucket  < cYBuckets() ) &&
		        ( RasterBound.vecMax().y() >= pos.y() ) )
		{
			Bucket( nextBucketX, nextBucket ).AddGPrim( pSurface );
			return true;
		}

		// Bound covers no more buckets therefore we can delete the surface completely.
		CqString objname( "unnamed" );
		const CqString* pattrName = pSurface->pAttributes() ->GetStringAttribute( "identifier", "name" );
		if( pattrName )
			objname = *pattrName;
		Aqsis::log() << info << "GPrim: \"" << objname << "\" occlusion culled" << std::endl;
		STATS_INC( GPR_occlusion_culled );
		return true;
	}
	else
	{
		return false;
	}
}


//----------------------------------------------------------------------
/** Add a new micro polygon to the list of waiting ones.
 * \param pmpgNew Pointer to a CqMicroPolygon derived class.
 */

void CqImageBuffer::AddMPG( CqMicroPolygon* pmpgNew )
{
	// Quick check for outside crop window.
	CqBound	B( pmpgNew->GetTotalBound() );
	ADDREF( pmpgNew );

	if ( B.vecMax().x() < m_CropWindowXMin - (m_FilterXWidth / 2.0f - 0.5f) || B.vecMax().y() < m_CropWindowYMin - (m_FilterYWidth / 2.0f - 0.5f)||
	        B.vecMin().x() > m_CropWindowXMax + (m_FilterXWidth / 2.0f - 0.5f) || B.vecMin().y() > m_CropWindowYMax + (m_FilterYWidth / 2.0f - 0.5f) )
	{
		RELEASEREF( pmpgNew );
		return ;
	}

	////////// Dump the micro polygon into a dump file //////////
#if ENABLE_MPDUMP
	if(m_mpdump.IsOpen())
		m_mpdump.dump(*pmpgNew);
#endif
	/////////////////////////////////////////////////////////////


	// Find out the minimum bucket touched by the micropoly bound.

	B.vecMin().x( B.vecMin().x() - (m_FilterXWidth / 2.0f - 0.5f));
	B.vecMin().y( B.vecMin().y() - (m_FilterYWidth / 2.0f - 0.5f));
	B.vecMax().x( B.vecMax().x() + (m_FilterXWidth / 2.0f - 0.5f));
	B.vecMax().y( B.vecMax().y() + (m_FilterYWidth / 2.0f - 0.5f));

	TqInt iXBa = static_cast<TqInt>( B.vecMin().x() / ( m_XBucketSize ) );
	TqInt iYBa = static_cast<TqInt>( B.vecMin().y() / ( m_YBucketSize ) );
	TqInt iXBb = static_cast<TqInt>( B.vecMax().x() / ( m_XBucketSize ) );
	TqInt iYBb = static_cast<TqInt>( B.vecMax().y() / ( m_YBucketSize ) );

	if ( ( iXBb < 0 ) || ( iYBb < 0 ) ||
	        ( iXBa >= m_cXBuckets ) || ( iYBa >= m_cYBuckets ) )
	{
		RELEASEREF( pmpgNew );
		return ;
	}

	if ( iXBa < 0 )
		iXBa = 0;
	if ( iYBa < 0 )
		iYBa = 0;

	// If the ideal bucket has already been processed we need to work out why we have got into this
	// situation.
	if ( Bucket(iXBa,iYBa).IsProcessed() )
	{
		PushMPGDown( pmpgNew, iXBa, iYBa );
		PushMPGForward( pmpgNew, iXBa, iYBa );
		RELEASEREF( pmpgNew );
		return ;
	}
	assert( !Bucket(iXBa, iYBa).IsProcessed() );
	Bucket(iXBa, iYBa).AddMPG( pmpgNew );
	ADDREF( pmpgNew );

	RELEASEREF( pmpgNew );
}


//----------------------------------------------------------------------
/** Add a new micro polygon to the list of waiting ones.
 * \param pmpg Pointer to a CqMicroPolygon derived class.
 */

bool CqImageBuffer::PushMPGForward( CqMicroPolygon* pmpg, TqInt Col, TqInt Row )
{
	// Should always mark as pushed forward even if not. As this is an idicator
	// that the attempt has been made, used by the PushDown function. If this wasn't set
	// then the mpg would be pushed down again when the next row is hit.
	pmpg->MarkPushedForward();

	// Check if there is anywhere to push forward to.
	if ( Col == ( cXBuckets() - 1 ) )
		return ( false );

	TqInt NextBucketForward = Col + 1;

	// If the next bucket forward has already been processed, try the one following that.
	if( Bucket( NextBucketForward, Row ).IsProcessed() )
		return( PushMPGForward( pmpg, NextBucketForward, Row ) );

	// Find out if any of the subbounds touch this bucket.
	CqVector2D BucketMin = BucketPosition( NextBucketForward, Row );
	CqVector2D BucketMax = BucketMin + BucketSize( NextBucketForward, Row );
	CqVector2D FilterWidth( (m_FilterXWidth * 0.5f - 0.5f), (m_FilterYWidth * 0.5f - 0.5f) );
	BucketMin -= FilterWidth;
	BucketMax += FilterWidth;

	const CqBound&	B = pmpg->GetTotalBound();

	const CqVector3D& vMin = B.vecMin();
	const CqVector3D& vMax = B.vecMax();
	if ( ( vMin.x() > BucketMax.x() ) ||
	        ( vMin.y() > BucketMax.y() ) ||
	        ( vMax.x() < BucketMin.x() ) ||
	        ( vMax.y() < BucketMin.y() ) )
	{
		return ( false );
	}
	else
	{
		ADDREF( pmpg );
		Bucket( NextBucketForward, Row ).AddMPG( pmpg );
		return ( true );
	}
	return ( false );
}


//----------------------------------------------------------------------
/** Add a new micro polygon to the list of waiting ones.
 * \param pmpg Pointer to a CqMicroPolygon derived class.
 * \param CurrBucketIndex Index of the bucket from which we are pushing down.
 */

bool CqImageBuffer::PushMPGDown( CqMicroPolygon* pmpg, TqInt Col, TqInt Row )
{
	if ( pmpg->IsPushedForward() )
		return ( false );

	// Check if there is anywhere to push down to.
	if ( Row == ( m_cYBuckets - 1 ) )
		return ( false );

	TqInt NextBucketDown = Row + 1;

	// If the next bucket down has already been processed,
	// try pushing forward from there.
	if( Bucket( Col, NextBucketDown ).IsProcessed() )
	{
		if( PushMPGForward( pmpg, Col, NextBucketDown ) )
			return( true );
		else
			// If that fails, push down again.
			return( PushMPGDown( pmpg, Col, NextBucketDown ) );
	}

	// Find out if any of the subbounds touch this bucket.
	CqVector2D BucketMin = BucketPosition( Col, NextBucketDown );
	CqVector2D BucketMax = BucketMin + BucketSize( Col, NextBucketDown );
	CqVector2D FilterWidth( (m_FilterXWidth * 0.5f - 0.5f), (m_FilterYWidth * 0.5f - 0.5f) );
	BucketMin -= FilterWidth;
	BucketMax += FilterWidth;

	const CqBound&	B = pmpg->GetTotalBound( );

	const CqVector3D& vMin = B.vecMin();
	const CqVector3D& vMax = B.vecMax();
	if ( ( vMin.x() > BucketMax.x() ) ||
	        ( vMin.y() > BucketMax.y() ) ||
	        ( vMax.x() < BucketMin.x() ) ||
	        ( vMax.y() < BucketMin.y() ) )
	{
		return ( false );
	}
	else
	{
		ADDREF( pmpg );
		Bucket(Col, NextBucketDown ).AddMPG( pmpg );
		// See if it needs to be pushed further down (extreme Motion Blur)
		if ( PushMPGDown( pmpg, Col, NextBucketDown ) )
			STATS_INC( MPG_pushed_far_down );
		return ( true );
	}
	return ( false );
}


/** Cache some info about the given grid so it can be referenced by multiple mpgs.
 
 * \param pGrid grid to cache info of.
 */
void CqImageBuffer::CacheGridInfo(CqMicroPolyGridBase* pGrid)
{
	m_CurrentGridInfo.m_IsMatte = pGrid->pAttributes() ->GetIntegerAttribute( "System", "Matte" ) [ 0 ] == 1;

	// this is true if the mpgs can safely be occlusion culled.
	m_CurrentGridInfo.m_IsCullable = !( DisplayMode() & ModeZ ) && !(pGrid->pCSGNode());

	m_CurrentGridInfo.m_UsesDataMap = !(QGetRenderContext() ->GetMapOfOutputDataEntries().empty());

	m_CurrentGridInfo.m_ShadingRate = pGrid->pAttributes() ->GetFloatAttribute( "System", "ShadingRate" ) [ 0 ];
	m_CurrentGridInfo.m_ShutterOpenTime = QGetRenderContext() ->poptCurrent()->GetFloatOption( "System", "Shutter" ) [ 0 ];
	m_CurrentGridInfo.m_ShutterCloseTime = QGetRenderContext() ->poptCurrent()->GetFloatOption( "System", "Shutter" ) [ 1 ];

	m_CurrentGridInfo.m_LodBounds = pGrid->pAttributes() ->GetFloatAttribute( "System", "LevelOfDetailBounds" );
}

//----------------------------------------------------------------------
/** Render any waiting MPGs.
 
    All micro polygon grids in the specified bucket are bust into
    individual micro polygons which are assigned to their appropriate
    bucket. Then RenderMicroPoly() is called for each micro polygon in
    the current bucket.
 
 * \param xmin Integer minimum extend of the image part being rendered, takes into account buckets and clipping.
 * \param xmax Integer maximum extend of the image part being rendered, takes into account buckets and clipping.
 * \param ymin Integer minimum extend of the image part being rendered, takes into account buckets and clipping.
 * \param ymax Integer maximum extend of the image part being rendered, takes into account buckets and clipping.
 */

void CqImageBuffer::RenderMPGs( long xmin, long xmax, long ymin, long ymax )
{
	{
		TIME_SCOPE("Render MPGs")
		// Render any waiting MPGs
		std::vector<CqMicroPolygon*>::iterator lastmpg = CurrentBucket().aMPGs().end();
		CqMicroPolyGridBase* pPrevGrid = NULL;
		for ( std::vector<CqMicroPolygon*>::iterator impg = CurrentBucket().aMPGs().begin(); impg != lastmpg; impg++ )
		{
			CqMicroPolygon* pMpg = *impg;

			if(pMpg->pGrid() != pPrevGrid)
			{
				pPrevGrid = pMpg->pGrid();
				CacheGridInfo(pPrevGrid);
			}

			RenderMicroPoly( pMpg, xmin, xmax, ymin, ymax );
			if ( PushMPGDown( ( pMpg ), CurrentBucketCol(), CurrentBucketRow() ) )
				STATS_INC( MPG_pushed_down );
			if ( PushMPGForward( ( pMpg ), CurrentBucketCol(), CurrentBucketRow() ) )
				STATS_INC( MPG_pushed_forward );
			RELEASEREF( ( pMpg ) );
		}
		CurrentBucket().aMPGs().clear();
	}

	// Split any grids in this bucket waiting to be processed.
	if ( !CurrentBucket().aGrids().empty() )
	{
		std::vector<CqMicroPolyGridBase*>::iterator lastgrid = CurrentBucket().aGrids().end();

		for ( std::vector<CqMicroPolyGridBase*>::iterator igrid = CurrentBucket().aGrids().begin(); igrid != lastgrid; igrid++ )
		{
			CqMicroPolyGridBase* pGrid = *igrid;
			pGrid->Split( this, xmin, xmax, ymin, ymax );

			CacheGridInfo(pGrid);
			
			{
				TIME_SCOPE("Render MPGs")
				// Render any waiting MPGs
				std::vector<CqMicroPolygon*>::iterator lastmpg = CurrentBucket().aMPGs().end();
				for ( std::vector<CqMicroPolygon*>::iterator impg = CurrentBucket().aMPGs().begin(); impg != lastmpg; impg++ )
				{
					CqMicroPolygon* pMpg = *impg;
					RenderMicroPoly( pMpg, xmin, xmax, ymin, ymax );
					if ( PushMPGDown( ( pMpg ), CurrentBucketCol(), CurrentBucketRow() ) )
						STATS_INC( MPG_pushed_down );
					if ( PushMPGForward( ( pMpg ), CurrentBucketCol(), CurrentBucketRow() ) )
						STATS_INC( MPG_pushed_forward );
					RELEASEREF( ( pMpg ) );
				}
				CurrentBucket().aMPGs().clear();
			}
		}
		CurrentBucket().aGrids().clear();
	}
}


//----------------------------------------------------------------------
/** Render a particular micropolygon.
 
 * \param pMPG Pointer to the micropolygon to process.
 * \param xmin Integer minimum extend of the image part being rendered, takes into account buckets and clipping.
 * \param xmax Integer maximum extend of the image part being rendered, takes into account buckets and clipping.
 * \param ymin Integer minimum extend of the image part being rendered, takes into account buckets and clipping.
 * \param ymax Integer maximum extend of the image part being rendered, takes into account buckets and clipping.
 
   \see CqBucket, CqImagePixel
 */

void CqImageBuffer::RenderMicroPoly( CqMicroPolygon* pMPG, long xmin, long xmax, long ymin, long ymax )
{
	const CqBound& Bound = pMPG->GetTotalBound();

	// if bounding box is outside our viewing range, then cull it.
	if ( Bound.vecMax().x() < xmin || Bound.vecMax().y() < ymin ||
	        Bound.vecMin().x() > xmax || Bound.vecMin().y() > ymax ||
	        Bound.vecMin().z() > ClippingFar() || Bound.vecMax().z() < ClippingNear())
	{
		STATS_INC( MPG_culled );
		return;
	}

	// fill in sample info for this mpg so we don't have to keep fetching it for each sample.
	// Must check if colour is needed, as if not, the variable will have been deleted from the grid.
	if ( QGetRenderContext() ->pDDmanager() ->fDisplayNeeds( "Ci" ) )
	{
		m_CurrentMpgSampleInfo.m_Colour = pMPG->colColor()[0];
	}
	else
	{
		m_CurrentMpgSampleInfo.m_Colour = gColWhite;
	}

	// Must check if opacity is needed, as if not, the variable will have been deleted from the grid.
	if ( QGetRenderContext() ->pDDmanager() ->fDisplayNeeds( "Oi" ) )
	{
		m_CurrentMpgSampleInfo.m_Opacity = pMPG->colOpacity()[0];
		m_CurrentMpgSampleInfo.m_Occludes = m_CurrentMpgSampleInfo.m_Opacity >= gColWhite;
	}
	else
	{
		m_CurrentMpgSampleInfo.m_Opacity = gColWhite;
		m_CurrentMpgSampleInfo.m_Occludes = true;
	}

	// use the single imagesample rather than the list if possible.
	// transparent, matte or csg samples, or if we need more than the first depth
	// value have to use the (slower) list.
	m_CurrentMpgSampleInfo.m_IsOpaque = m_CurrentMpgSampleInfo.m_Occludes &&
	                                    !pMPG->pGrid()->pCSGNode() &&
	                                    !( DisplayMode() & ModeZ ) &&
	                                    !m_CurrentGridInfo.m_IsMatte;

	bool UsingDof = QGetRenderContext() ->UsingDepthOfField();
	bool IsMoving = pMPG->IsMoving();

	if(IsMoving || UsingDof)
	{
		RenderMPG_MBOrDof( pMPG, xmin, xmax, ymin, ymax, IsMoving, UsingDof );
	}
	else
	{
		RenderMPG_Static( pMPG, xmin, xmax, ymin, ymax );
	}
}



void CqImageBuffer::RenderMPG_MBOrDof( CqMicroPolygon* pMPG,
										long xmin, long xmax, long ymin, long ymax,
										bool IsMoving, bool UsingDof )
{
    CqBucket & Bucket = CurrentBucket();
    CqStats& theStats = QGetRenderContext() ->Stats();

    const TqFloat* LodBounds = m_CurrentGridInfo.m_LodBounds;
    bool UsingLevelOfDetail = LodBounds[ 0 ] >= 0.0f;

    TqInt sample_hits = 0;
    TqFloat shd_rate = m_CurrentGridInfo.m_ShadingRate;

	CqHitTestCache hitTestCache;
	bool cachedHitData = false;

	bool mustDraw = !m_CurrentGridInfo.m_IsCullable;

	TqInt iXSamples = PixelXSamples();
    TqInt iYSamples = PixelYSamples();

	TqFloat opentime = m_CurrentGridInfo.m_ShutterOpenTime;
	TqFloat closetime = m_CurrentGridInfo.m_ShutterCloseTime;
	TqFloat timePerSample;
	if(IsMoving)
	{
		TqInt numSamples = iXSamples * iYSamples;
		timePerSample = (float)numSamples / ( closetime - opentime );
	}

    TqInt bound_maxMB = pMPG->cSubBounds();
    TqInt bound_maxMB_1 = bound_maxMB - 1;
	TqInt currentIndex = 0;
    for ( TqInt bound_numMB = 0; bound_numMB < bound_maxMB; bound_numMB++ )
    {
        TqFloat time0;
        TqFloat time1;
        const CqBound& Bound = pMPG->SubBound( bound_numMB, time0 );

		// get the index of the first and last samples that can fall inside
		// the time range of this bound
		TqInt indexT0;
		TqInt indexT1;
		if(IsMoving)
		{
			if ( bound_numMB != bound_maxMB_1 )
				pMPG->SubBound( bound_numMB + 1, time1 );
			else
				time1 = closetime;//QGetRenderContext() ->optCurrent().GetFloatOptionWrite( "System", "Shutter" ) [ 1 ];

			indexT0 = static_cast<TqInt>(FLOOR((time0 - opentime) * timePerSample));
			indexT1 = static_cast<TqInt>(CEIL((time1 - opentime) * timePerSample));
		}

		TqFloat maxCocX;
		TqFloat maxCocY;

		TqFloat bminx;
		TqFloat bmaxx;
		TqFloat bminy;
		TqFloat bmaxy;
		TqFloat bminz;
		TqFloat bmaxz;
		// these values are the bound of the mpg not including dof extension.
		// reduce the mpg bound so it doesn't include the coc.
		TqFloat mpgbminx;
		TqFloat mpgbmaxx;
		TqFloat mpgbminy;
		TqFloat mpgbmaxy;
		TqInt bound_maxDof;
		if(UsingDof)
		{
			const CqVector2D& minZCoc = QGetRenderContext()->GetCircleOfConfusion( Bound.vecMin().z() );
			const CqVector2D& maxZCoc = QGetRenderContext()->GetCircleOfConfusion( Bound.vecMax().z() );
			maxCocX = MAX( minZCoc.x(), maxZCoc.x() );
			maxCocY = MAX( minZCoc.y(), maxZCoc.y() );

			mpgbminx = Bound.vecMin().x() + maxCocX;
			mpgbmaxx = Bound.vecMax().x() - maxCocX;
			mpgbminy = Bound.vecMin().y() + maxCocY;
			mpgbmaxy = Bound.vecMax().y() - maxCocY;
			bminz = Bound.vecMin().z();
			bmaxz = Bound.vecMax().z();

			bound_maxDof = CqBucket::NumDofBounds();
		}
		else
		{
			bminx = Bound.vecMin().x();
			bmaxx = Bound.vecMax().x();
			bminy = Bound.vecMin().y();
			bmaxy = Bound.vecMax().y();
			bminz = Bound.vecMin().z();
			bmaxz = Bound.vecMax().z();

			bound_maxDof = 1;
		}

		for ( TqInt bound_numDof = 0; bound_numDof < bound_maxDof; bound_numDof++ )
		{
			if(UsingDof)
			{
				// now shift the bounding box to cover only a given range of
				// lens positions.
				const CqBound DofBound = CqBucket::DofSubBound( bound_numDof );
				TqFloat leftOffset = DofBound.vecMax().x() * maxCocX;
				TqFloat rightOffset = DofBound.vecMin().x() * maxCocX;
				TqFloat topOffset = DofBound.vecMax().y() * maxCocY;
				TqFloat bottomOffset = DofBound.vecMin().y() * maxCocY;

				bminx = mpgbminx - leftOffset;
				bmaxx = mpgbmaxx - rightOffset;
				bminy = mpgbminy - topOffset;
				bmaxy = mpgbmaxy - bottomOffset;
			}

			// if bounding box is outside our viewing range, then cull it.
			if ( bmaxx < (float)xmin || bmaxy < (float)ymin ||
				bminx > (float)xmax || bminy > (float)ymax ||
				bminz > ClippingFar() || bmaxz < ClippingNear())
			{
				continue;
			}

			// Now go across all pixels touched by the micropolygon bound.
			// The first pixel position is at (sX, sY), the last one
			// at (eX, eY).
			TqInt eX = CEIL( bmaxx );
			TqInt eY = CEIL( bmaxy );
			if ( eX > xmax ) eX = xmax;
			if ( eY > ymax ) eY = ymax;

			TqInt sX = FLOOR( bminx );
			TqInt sY = FLOOR( bminy );
			if ( sY < ymin ) sY = ymin;
			if ( sX < xmin ) sX = xmin;

			CqImagePixel* pie, *pie2;

			TqInt nextx = Bucket.RealWidth();
			Bucket.ImageElement( sX, sY, pie );

			for( int iY = sY; iY < eY; ++iY)
			{
				pie2 = pie;
				pie += nextx;

				for(int iX = sX; iX < eX; ++iX, ++pie2)
				{
					TqInt index;
					if(UsingDof)
					{
						// when using dof only one sample per pixel can
						// possibbly hit (the one corresponding to the
						// current bounding box).
						index = pie2->GetDofOffsetIndex(bound_numDof);
					}
					else
					{
						// when using mb without dof, a range of samples
						// may have times within the current mb bounding box.
						index = indexT0;
					}
					// only bother sampling if the mpg is not occluded in this pixel.
					if(mustDraw || bminz <= pie2->SampleData(index).m_occlusionBox->MaxOpaqueZ())
					{

						// loop over potential samples
						do
						{
							const SqSampleData& sampleData = pie2->SampleData( index );
							const CqVector2D& vecP = sampleData.m_Position;
							const TqFloat time = sampleData.m_Time;

							index++;

							CqStats::IncI( CqStats::SPL_count );

							if(IsMoving && (time < time0 || time > time1))
							{
								continue;
							}

							// check if sample lies inside mpg bounding box.
							if ( UsingDof )
							{
								CqBound DofBound(bminx, bminy, bminz, bmaxx, bmaxy, bmaxz);

								if(!DofBound.Contains2D( vecP ))
									continue;

								// Check to see if the sample is within the sample's level of detail
								if ( UsingLevelOfDetail)
								{
									TqFloat LevelOfDetail = sampleData.m_DetailLevel;
									if ( LodBounds[ 0 ] > LevelOfDetail || LevelOfDetail >= LodBounds[ 1 ] )
									{
										continue;
									}
								}


								CqStats::IncI( CqStats::SPL_bound_hits );

								// Now check if the subsample hits the micropoly
								bool SampleHit;
								TqFloat D;

								SampleHit = pMPG->Sample( sampleData, D, time, UsingDof );
								if ( SampleHit )
								{
									sample_hits++;
									// note index has already been incremented, so we use the previous value.
									StoreSample( pMPG, pie2, index-1, D );
								}
							}
							else
							{
								if(!Bound.Contains2D( vecP ))
									continue;
								// Check to see if the sample is within the sample's level of detail
								if ( UsingLevelOfDetail)
								{
									TqFloat LevelOfDetail = sampleData.m_DetailLevel;
									if ( LodBounds[ 0 ] > LevelOfDetail || LevelOfDetail >= LodBounds[ 1 ] )
									{
										continue;
									}
								}


								CqStats::IncI( CqStats::SPL_bound_hits );

								// Now check if the subsample hits the micropoly
								bool SampleHit;
								TqFloat D;

								pMPG->CacheHitTestValues(&hitTestCache);
								cachedHitData = true;

								SampleHit = pMPG->Sample( sampleData, D, time );
								if ( SampleHit )
								{
									sample_hits++;
									// note index has already been incremented, so we use the previous value.
									StoreSample( pMPG, pie2, index-1, D );
								}
							}
						} while (!UsingDof && index < indexT1);
					}
				}
			}
		}
    }
}


void CqImageBuffer::RenderMPG_Static( CqMicroPolygon* pMPG, long xmin, long xmax, long ymin, long ymax )
{
    CqBucket & Bucket = CurrentBucket();
    CqStats& theStats = QGetRenderContext() ->Stats();

    const TqFloat* LodBounds = m_CurrentGridInfo.m_LodBounds;
    bool UsingLevelOfDetail = LodBounds[ 0 ] >= 0.0f;

    TqInt sample_hits = 0;
    TqFloat shd_rate = m_CurrentGridInfo.m_ShadingRate;

	CqHitTestCache hitTestCache;
	bool cachedHitData = false;

	bool mustDraw = !m_CurrentGridInfo.m_IsCullable;

    CqBound Bound = pMPG->GetTotalBound();

	TqFloat bminx = Bound.vecMin().x();
	TqFloat bmaxx = Bound.vecMax().x();
	TqFloat bminy = Bound.vecMin().y();
	TqFloat bmaxy = Bound.vecMax().y();
	TqFloat bminz = Bound.vecMin().z();

	// Now go across all pixels touched by the micropolygon bound.
	// The first pixel position is at (sX, sY), the last one
	// at (eX, eY).
	TqInt eX = CEIL( bmaxx );
	TqInt eY = CEIL( bmaxy );
	if ( eX > xmax ) eX = xmax;
	if ( eY > ymax ) eY = ymax;

	TqInt sX = FLOOR( bminx );
	TqInt sY = FLOOR( bminy );
	if ( sY < ymin ) sY = ymin;
	if ( sX < xmin ) sX = xmin;

	CqImagePixel* pie, *pie2;

	TqInt iXSamples = PixelXSamples();
	TqInt iYSamples = PixelYSamples();

	TqInt im = ( bminx < sX ) ? 0 : FLOOR( ( bminx - sX ) * iXSamples );
	TqInt in = ( bminy < sY ) ? 0 : FLOOR( ( bminy - sY ) * iYSamples );
	TqInt em = ( bmaxx > eX ) ? iXSamples : CEIL( ( bmaxx - ( eX - 1 ) ) * iXSamples );
	TqInt en = ( bmaxy > eY ) ? iYSamples : CEIL( ( bmaxy - ( eY - 1 ) ) * iYSamples );

	TqInt nextx = Bucket.RealWidth();
	Bucket.ImageElement( sX, sY, pie );

	for( int iY = sY; iY < eY; ++iY)
	{
		pie2 = pie;
		pie += nextx;

		for(int iX = sX; iX < eX; ++iX, ++pie2)
		{
			// only bother sampling if the mpg is not occluded in this pixel.
			//if(mustDraw || bminz <= pie2->SampleData(index).m_occlusionBox->MaxOpaqueZ())
			{
				if(!cachedHitData)
				{
					pMPG->CacheHitTestValues(&hitTestCache);
					cachedHitData = true;
				}

				// Now sample the micropolygon at several subsample positions
				// within the pixel. The subsample indices range from (start_m, n)
				// to (end_m-1, end_n-1).
				register int m, n;
				n = ( iY == sY ) ? in : 0;
				int end_n = ( iY == ( eY - 1 ) ) ? en : iYSamples;
				int start_m = ( iX == sX ) ? im : 0;
				int end_m = ( iX == ( eX - 1 ) ) ? em : iXSamples;
				int index_start = n*iXSamples + start_m;

				for ( ; n < end_n; n++ )
				{
					int index = index_start;
					for ( m = start_m; m < end_m; m++, index++ )
					{
						const SqSampleData& sampleData = pie2->SampleData( index );
						//if(mustDraw || bminz <= pie2->SampleData(index).m_occlusionBox->MaxOpaqueZ())
						{
							const CqVector2D& vecP = sampleData.m_Position;
							const TqFloat time = 0.0;

							CqStats::IncI( CqStats::SPL_count );

							if(!Bound.Contains2D( vecP ))
								continue;

							// Check to see if the sample is within the sample's level of detail
							if ( UsingLevelOfDetail)
							{
								TqFloat LevelOfDetail = sampleData.m_DetailLevel;
								if ( LodBounds[ 0 ] > LevelOfDetail || LevelOfDetail >= LodBounds[ 1 ] )
								{
									continue;
								}
							}

							CqStats::IncI( CqStats::SPL_bound_hits );

							// Now check if the subsample hits the micropoly
							bool SampleHit;
							TqFloat D;

							SampleHit = pMPG->Sample( sampleData, D, time );

							if ( SampleHit )
							{
								sample_hits++;
								StoreSample( pMPG, pie2, index, D );
							}
						}
					}
					index_start += iXSamples;
				}
			}
	/*        // Now compute the % of samples that hit...
			TqInt scount = iXSamples * iYSamples;
			TqFloat max_hits = scount * shd_rate;
			TqInt hit_rate = ( sample_hits / max_hits ) / 0.125;
			STATS_INC( MPG_sample_coverage0_125 + CLAMP( hit_rate - 1 , 0, 7 ) );
	*/  }
	}
}


void CqImageBuffer::StoreSample( CqMicroPolygon* pMPG, CqImagePixel* pie2, TqInt index, TqFloat D )
{
    bool Occludes = m_CurrentMpgSampleInfo.m_Occludes;
	bool opaque =  m_CurrentMpgSampleInfo.m_IsOpaque;

	SqImageSample& currentOpaqueSample = pie2->OpaqueValues(index);
	//static SqImageSample localImageVal( QGetRenderContext() ->GetOutputDataTotalSize() );
	SqImageSample localImageVal;

	SqImageSample& ImageVal = opaque ? currentOpaqueSample : localImageVal;

	std::deque<SqImageSample>& aValues = pie2->Values( index );
	std::deque<SqImageSample>::iterator sample = aValues.begin();
	std::deque<SqImageSample>::iterator end = aValues.end();

	// return if the sample is occluded and can be culled.
	if(opaque)
	{
		if((currentOpaqueSample.m_flags & SqImageSample::Flag_Valid) &&
			currentOpaqueSample.Data()[Sample_Depth] <= D)
		{
			return;
		}
	}
	else
	{
		// Sort the color/opacity into the visible point list
		// return if the sample is occluded and can be culled.
		while( sample != end )
		{
			if((*sample).Data()[Sample_Depth] >= D)
				break;

			if(((*sample).m_flags & SqImageSample::Flag_Occludes) &&
				!(*sample).m_pCSGNode && m_CurrentGridInfo.m_IsCullable)
				return;

			++sample;
		}
	}

    ImageVal.Data()[Sample_Depth] = D ;

	CqStats::IncI( CqStats::SPL_hits );
	pMPG->MarkHit();

    TqFloat* val = ImageVal.Data();
    val[ Sample_Red ] = m_CurrentMpgSampleInfo.m_Colour.fRed();
    val[ Sample_Green ] = m_CurrentMpgSampleInfo.m_Colour.fGreen();
    val[ Sample_Blue ] = m_CurrentMpgSampleInfo.m_Colour.fBlue();
    val[ Sample_ORed ] = m_CurrentMpgSampleInfo.m_Opacity.fRed();
    val[ Sample_OGreen ] = m_CurrentMpgSampleInfo.m_Opacity.fGreen();
    val[ Sample_OBlue ] = m_CurrentMpgSampleInfo.m_Opacity.fBlue();
    val[ Sample_Depth ] = D;

    // Now store any other data types that have been registered.
	if(m_CurrentGridInfo.m_UsesDataMap)
	{
		StoreExtraData(pMPG, val);
	}

	if(!opaque)
	{
		// If depth is exactly the same as previous sample, chances are we've
		// hit a MPG grid line.
		// \note: Cannot do this if there is CSG involved, as all samples must be taken and kept the same.
		if ( sample != end && (*sample).Data()[Sample_Depth] == ImageVal.Data()[Sample_Depth] && !(*sample).m_pCSGNode )
		{
			//(*sample).m_Data = ( (*sample).m_Data + val ) * 0.5f;
			return;
		}
	}

    // Update max depth values
    //if ( !( DisplayMode() & ModeZ ) && Occludes )
    if ( opaque )
    {
		if( D < pie2->SampleData(index).m_occlusionBox->MaxOpaqueZ() )
		{
			pie2->SampleData(index).m_occlusionBox->SetMaxOpaqueZ(D);
			pie2->SampleData(index).m_occlusionBox->PropagateChanges();
		}
    }

    ImageVal.m_pCSGNode = pMPG->pGrid() ->pCSGNode();

    ImageVal.m_flags = 0;
    if ( Occludes )
    {
        ImageVal.m_flags |= SqImageSample::Flag_Occludes;
    }
    if( m_CurrentGridInfo.m_IsMatte )
    {
        ImageVal.m_flags |= SqImageSample::Flag_Matte;
    }

	if(!opaque)
	{
		aValues.insert( sample, ImageVal );
	}
	else
	{
		// mark this sample as having been written into.
		ImageVal.m_flags |= SqImageSample::Flag_Valid;
	}
}

void CqImageBuffer::StoreExtraData( CqMicroPolygon* pMPG, TqFloat* val)
{
    std::map<std::string, CqRenderer::SqOutputDataEntry>& DataMap = QGetRenderContext() ->GetMapOfOutputDataEntries();
	std::map<std::string, CqRenderer::SqOutputDataEntry>::iterator entry;
    for ( entry = DataMap.begin(); entry != DataMap.end(); ++entry )
    {
        IqShaderData* pData;
        if ( ( pData = pMPG->pGrid() ->FindStandardVar( entry->first.c_str() ) ) != NULL )
        {
            switch ( pData->Type() )
            {
            case type_float:
            case type_integer:
                {
                    TqFloat f;
                    pData->GetFloat( f, pMPG->GetIndex() );
                    val[ entry->second.m_Offset ] = f;
                    break;
                }
            case type_point:
            case type_normal:
            case type_vector:
            case type_hpoint:
                {
                    CqVector3D v;
                    pData->GetPoint( v, pMPG->GetIndex() );
                    val[ entry->second.m_Offset ] = v.x();
                    val[ entry->second.m_Offset + 1 ] = v.y();
                    val[ entry->second.m_Offset + 2 ] = v.z();
                    break;
                }
            case type_color:
                {
                    CqColor c;
                    pData->GetColor( c, pMPG->GetIndex() );
                    val[ entry->second.m_Offset ] = c.fRed();
                    val[ entry->second.m_Offset + 1 ] = c.fGreen();
                    val[ entry->second.m_Offset + 2 ] = c.fBlue();
                    break;
                }
            case type_matrix:
                {
                    CqMatrix m;
                    pData->GetMatrix( m, pMPG->GetIndex() );
                    TqFloat* pElements = m.pElements();
                    val[ entry->second.m_Offset ] = pElements[ 0 ];
                    val[ entry->second.m_Offset + 1 ] = pElements[ 1 ];
                    val[ entry->second.m_Offset + 2 ] = pElements[ 2 ];
                    val[ entry->second.m_Offset + 3 ] = pElements[ 3 ];
                    val[ entry->second.m_Offset + 4 ] = pElements[ 4 ];
                    val[ entry->second.m_Offset + 5 ] = pElements[ 5 ];
                    val[ entry->second.m_Offset + 6 ] = pElements[ 6 ];
                    val[ entry->second.m_Offset + 7 ] = pElements[ 7 ];
                    val[ entry->second.m_Offset + 8 ] = pElements[ 8 ];
                    val[ entry->second.m_Offset + 9 ] = pElements[ 9 ];
                    val[ entry->second.m_Offset + 10 ] = pElements[ 10 ];
                    val[ entry->second.m_Offset + 11 ] = pElements[ 11 ];
                    val[ entry->second.m_Offset + 12 ] = pElements[ 12 ];
                    val[ entry->second.m_Offset + 13 ] = pElements[ 13 ];
                    val[ entry->second.m_Offset + 14 ] = pElements[ 14 ];
                    val[ entry->second.m_Offset + 15 ] = pElements[ 15 ];
                    break;
                }
            default:
                // left blank to avoid compiler warnings about unhandled
                //  types
                break;
            }
        }
    }
}


//----------------------------------------------------------------------
/** Render any waiting Surfaces
 
    This method loops through all the gprims stored in the specified bucket
    and checks if the gprim can be diced and turned into a grid of micro
    polygons or if it is still too large and has to be split (this check
    is done in CqSurface::Diceable()).
 
    The dicing is done by the gprim in CqSurface::Dice(). After that
    the entire grid is shaded by calling CqMicroPolyGridBase::Shade().
    The shaded grid is then stored in the current bucket and will eventually
    be further processed by RenderMPGs().
 
    If the gprim could not yet be diced, it is split into a number of
    smaller gprims (CqSurface::Split()) which are again assigned to
    buckets (this doesn't necessarily have to be the current one again)
    by calling PostSurface() (just as if it were regular gprims).
 
    Finally, when all the gprims are diced and the resulting micro polygons
    are rendered, the individual subpixel samples are combined into one
    pixel color and opacity which is then exposed and quantized.
    After that the method BucketComplete() and IqDDManager::DisplayBucket()
    is called which can be used to display the bucket inside a window or
    save it to disk.
 
 * \param xmin Integer minimum extend of the image part being rendered, takes into account buckets and clipping.
 * \param xmax Integer maximum extend of the image part being rendered, takes into account buckets and clipping.
 * \param ymin Integer minimum extend of the image part being rendered, takes into account buckets and clipping.
 * \param ymax Integer maximum extend of the image part being rendered, takes into account buckets and clipping.
 */

void CqImageBuffer::RenderSurfaces( long xmin, long xmax, long ymin, long ymax, bool fImager, enum EqFilterDepth depthfilter, CqColor zThreshold)
{
	bool bIsEmpty = IsCurrentBucketEmpty();

	// Render any waiting micro polygon grids.
	RenderMPGs( xmin, xmax, ymin, ymax );

	CqBucket& Bucket = CurrentBucket();

	// Render any waiting subsurfaces.
	boost::shared_ptr<CqSurface> pSurface = Bucket.pTopSurface();
	while ( pSurface )
	{
		if ( m_fQuit )
			return ;


		//Cull surface if it's hidden
		if ( !( DisplayMode() & ModeZ ) && !pSurface->pCSGNode() )
		{
			bool fCull = false;
			if ( !bIsEmpty && pSurface->fCachedBound() )
			{
				TIME_SCOPE("Occlusion culling surfaces")
				fCull = OcclusionCullSurface( pSurface );
			}
			if ( fCull )
			{
				Bucket.popSurface();
				pSurface = Bucket.pTopSurface();
				continue;
			}
		}

		// If the epsilon check has deemed this surface to be undiceable, don't bother asking.
		bool fDiceable = false;
		// Dice & shade the surface if it's small enough...
		{
			TIME_SCOPE("Dicable check")
			fDiceable = pSurface->Diceable();
		}

		if ( fDiceable )
		{

			Bucket.popSurface();
			CqMicroPolyGridBase* pGrid;
			{
				TIME_SCOPE("Dicing")
				pGrid = pSurface->Dice();
			}

			if ( NULL != pGrid )
			{
				ADDREF( pGrid );
				// Only shade in all cases since the Displacement could be called in the shadow map creation too.
				pGrid->Shade();
				pGrid->TransferOutputVariables();

				if ( pGrid->vfCulled() == false )
				{
					// Only project micropolygon not culled
					Bucket.AddGrid( pGrid );
					// Render any waiting micro polygon grids.
					RenderMPGs( xmin, xmax, ymin, ymax );
				}
				RELEASEREF( pGrid );
			}
		}
		// The surface is not small enough, so split it...
		else if ( !pSurface->fDiscard() )
		{
			Bucket.popSurface();

			// Decrease the total gprim count since this gprim is replaced by other gprims
			STATS_DEC( GPR_created_total );

			// Split it
			{
				TqInt cSplits = 0;
				std::vector<boost::shared_ptr<CqSurface> > aSplits;
				{
					TIME_SCOPE("Splits")
					cSplits = pSurface->Split( aSplits );
				}
				TqInt i;
				for ( i = 0; i < cSplits; i++ )
					PostSurface( aSplits[ i ] );

				/// \debug:
				/*				if(pSurface->IsUndiceable())
								{
								    CqBound Bound( pSurface->Bound() );
									std::cout << pSurface << " - " << Bound.vecMin().z() << " --> " << Bound.vecMax().z() << std::endl;
									for ( i = 0; i < cSplits; i++ )
									{
									    CqBound SBound( aSplits[i]->Bound() );
										std::cout << "\t" << aSplits[i] << " - " << SBound.vecMin().z() << " --> " << SBound.vecMax().z() << std::endl;
									}
								}
				*/			
			}
		}
		else if ( pSurface == Bucket.pTopSurface() )
		{
			// Make sure we will break the while()
			//   e.g.  !fDiceable  &&  pSurface->fDiscard()
			Bucket.popSurface();
		}

		pSurface = Bucket.pTopSurface();
		// Render any waiting micro polygon grids.
		RenderMPGs( xmin, xmax, ymin, ymax );
	}

	// Now combine the colors at each pixel sample for any micropolygons rendered to that pixel.
	if ( m_fQuit )
		return ;

	if(!bIsEmpty)
	{
		TIME_SCOPE("Combine")
		CqBucket::CombineElements(depthfilter, zThreshold);
	}

	TIMER_START("Filter")
	if (fImager)
		bIsEmpty = false;

	Bucket.FilterBucket(bIsEmpty);
	if(!bIsEmpty)
	{
		Bucket.ExposeBucket();
		// \note: Used to quantize here too, but not any more, as it is handled by
		//	  ddmanager in a specific way for each display.
	}

	TIMER_STOP("Filter")

	BucketComplete();
	{
		TIME_SCOPE("Display bucket")
		QGetRenderContext() ->pDDmanager() ->DisplayBucket( &CurrentBucket() );
	}
}

//----------------------------------------------------------------------
/** Return if a certain bucket is completely empty.
 
    True or False.
 
     It is empty only when this bucket doesn't contain any surface, 
	 micropolygon or grids.
 */
bool CqImageBuffer::IsCurrentBucketEmpty()
{
	bool retval = false;

	CqBucket& Bucket = CurrentBucket();

	if ( ( !Bucket.pTopSurface() ) &&
	        Bucket.aGrids().empty() &&
	        Bucket.aMPGs().empty() )
		retval = true;

	return retval;
}
//----------------------------------------------------------------------
/** Render any waiting Surfaces
 
    Starting from the upper left corner of the image every bucket is
    processed by computing its extent and calling RenderSurfaces().
    After the image is complete ImageComplete() is called.
 
    It will be nice to be able to remove Occlusion at demands.
	However I did not see a case when Occlusion took longer without occlusion.
 */

void CqImageBuffer::RenderImage()
{
	////////// Create a new dump file  //////////
#if ENABLE_MPDUMP
	const TqInt* poptDump = QGetRenderContext() ->poptCurrent()->GetIntegerOption( "mpdump", "enabled" );
	if(poptDump && (*poptDump != 0))
	{
		m_mpdump.open();
		m_mpdump.dumpImageInfo();
	}
#endif
	/////////////////////////////////////////////

	STATS_SETF( MPG_min_area, FLT_MAX );
	STATS_SETF( MPG_max_area, FLT_MIN );

	if ( bucketmodulo == -1 )
	{
		// Small change which allows full control of virtual memory on NT swapping
		bucketmodulo = m_cXBuckets;
		TqInt *poptModulo = ( TqInt * ) QGetRenderContext() ->poptCurrent()->GetIntegerOption( "limits", "bucketmodulo" );
		if ( poptModulo != 0 )
		{
			bucketmodulo = poptModulo[ 0 ];
		}
		if ( bucketmodulo <= 0 )
			bucketmodulo = m_cXBuckets;
	}

	bool fImager = false;
	const CqString* systemOptions;
	if( ( systemOptions = QGetRenderContext() ->poptCurrent()->GetStringOption( "System", "Imager" ) ) != 0 )
		if( systemOptions[ 0 ].compare("null") != 0 )
			fImager = true;

	const CqString* pstrDepthFilter = QGetRenderContext() ->poptCurrent()->GetStringOption( "Hider", "depthfilter" );
	const CqColor* pzThreshold = QGetRenderContext() ->poptCurrent()->GetColorOption( "limits", "zthreshold" );
	CqColor zThreshold(1.0f, 1.0f, 1.0f);	// Default threshold of 1,1,1 means that any objects that are partially transparent won't appear in shadow maps.
	if(NULL != pzThreshold)
		zThreshold = pzThreshold[0];
	enum EqFilterDepth depthfilter = Filter_Min;
	if ( NULL != pstrDepthFilter )
	{
		Aqsis::log() << debug << "Depth filter = " << pstrDepthFilter[0].c_str() << std::endl;
		if( !pstrDepthFilter[ 0 ].compare( "min" ) )
			depthfilter = Filter_Min;
		else if ( !pstrDepthFilter[ 0 ].compare( "midpoint" ) )
			depthfilter = Filter_MidPoint;
		else if ( !pstrDepthFilter[ 0 ].compare( "max" ) )
			depthfilter = Filter_Max;
		else if ( !pstrDepthFilter[ 0 ].compare( "average" ) )
			depthfilter = Filter_Average;
		else
			Aqsis::log() << warning << "Invalid depthfilter \"" << pstrDepthFilter[ 0 ].c_str() << "\", depthfilter set to \"min\"" << std::endl;
	}

	// Render the surface at the front of the list.
	m_fDone = false;

	CqVector2D bHalf = CqVector2D( FLOOR(m_FilterXWidth / 2.0f - 0.5f), FLOOR(m_FilterYWidth / 2.0f - 0.5f) );

	RtProgressFunc pProgressHandler = NULL;
	pProgressHandler = QGetRenderContext()->pProgressHandler();

	const CqString* pstrBucketOrder = QGetRenderContext() ->poptCurrent()->GetStringOption( "render", "bucketorder" );
	enum EqBucketOrder order = Bucket_Horizontal;
	if ( NULL != pstrBucketOrder )
	{
	if( !pstrBucketOrder[ 0 ].compare( "vertical" ) )
		order = Bucket_Vertical;
	else if ( !pstrBucketOrder[ 0 ].compare( "horizontal" ) )
		order = Bucket_Horizontal;
	else {
		Aqsis::log() << warning << "Not supported \"" << pstrBucketOrder[ 0 ] << "\" " << std::endl;
	}
#ifdef NOTREADY
	else if ( !pstrBucketOrder[ 0 ].compare( "zigzag" ) )
		order = Bucket_ZigZag;
	else if ( !pstrBucketOrder[ 0 ].compare( "circle" ) )
		order = Bucket_Circle;
	else if ( !pstrBucketOrder[ 0 ].compare( "random" ) )
		order = Bucket_Random;
#endif
	}

	// A counter for the number of processed buckets (used for progress reporting)
	TqInt iBucket = 0;

	// Iterate over all buckets...
	do
	{
		bool bIsEmpty = IsCurrentBucketEmpty();
		// Prepare the bucket.
		CqVector2D bPos = BucketPosition();
		CqVector2D bSize = BucketSize();

		bool fImager = false;
		const CqString* systemOptions;
		if( ( systemOptions = QGetRenderContext() ->poptCurrent()->GetStringOption( "System", "Imager" ) ) != 0 )
			if( systemOptions[ 0 ].compare("null") != 0 )
				fImager = true;

		if (fImager)
			bIsEmpty = false;

		{
			TIME_SCOPE("Prepare bucket")
			CqBucket::PrepareBucket( static_cast<TqInt>( bPos.x() ), static_cast<TqInt>( bPos.y() ), static_cast<TqInt>( bSize.x() ), static_cast<TqInt>( bSize.y() ), true, bIsEmpty );
			CqBucket::InitialiseFilterValues();
		}

		////////// Dump the pixel sample positions into a dump file //////////
#if ENABLE_MPDUMP
		if(m_mpdump.IsOpen())
			m_mpdump.dumpPixelSamples();
#endif
		/////////////////////////////////////////////////////////////////////////

		// Set up some bounds for the bucket.
		CqVector2D vecMin = bPos;
		CqVector2D vecMax = bPos + bSize;
		vecMin -= bHalf;
		vecMax += bHalf;

		long xmin = static_cast<long>( vecMin.x() );
		long ymin = static_cast<long>( vecMin.y() );
		long xmax = static_cast<long>( vecMax.x() );
		long ymax = static_cast<long>( vecMax.y() );

		if ( xmin < CropWindowXMin() - (m_FilterXWidth / 2 - 0.5f) )
			xmin = static_cast<long>(CropWindowXMin() - (m_FilterXWidth / 2.0f - 0.5f));
		if ( ymin < CropWindowYMin() - (m_FilterYWidth / 2 - 0.5f) )
			ymin = static_cast<long>(CropWindowYMin() - (m_FilterYWidth / 2.0f - 0.5f));
		if ( xmax > CropWindowXMax() + (m_FilterXWidth / 2 - 0.5f) )
			xmax = static_cast<long>(CropWindowXMax() + (m_FilterXWidth / 2.0f - 0.5f));
		if ( ymax > CropWindowYMax() + (m_FilterYWidth / 2 - 0.5f) )
			ymax = static_cast<long>(CropWindowYMax() + (m_FilterYWidth / 2.0f - 0.5f));


		if ( !bIsEmpty )
		{
			TIME_SCOPE("Occlusion culling initialisation")
			CqOcclusionBox::SetupHierarchy( &CurrentBucket(), xmin, ymin, xmax, ymax );
		}


		if ( pProgressHandler )
		{
			// Inform the status class how far we have got, and update UI.
			float Complete = ( float ) ( cXBuckets() * cYBuckets() );
			Complete = 100.0f * iBucket / Complete;
			QGetRenderContext() ->Stats().SetComplete( Complete );
			( *pProgressHandler ) ( Complete, QGetRenderContext() ->CurrentFrame() );
		}


		RenderSurfaces( xmin, xmax, ymin, ymax, fImager, depthfilter, zThreshold );
		if ( m_fQuit )
		{
			m_fDone = true;
			return ;
		}
#ifdef WIN32
		if ( !( iBucket % bucketmodulo ) )
			SetProcessWorkingSetSize( GetCurrentProcess(), 0xffffffff, 0xffffffff );
#endif
		CurrentBucket().SetProcessed();
//		CqOcclusionBox::KDTree()->DumpOpaqueZ();
//		std::cout << std::endl;
		// Increase the bucket counter...
		iBucket += 1;
	} while( NextBucket(order) );

	ImageComplete();
	CqBucket::ShutdownBucket();
	CqOcclusionBox::DeleteHierarchy();

	// Pass >100 through to progress to allow it to indicate completion.

	if ( pProgressHandler )
	{
		( *pProgressHandler ) ( 100.0f, QGetRenderContext() ->CurrentFrame() );
	}
	m_fDone = true;
}


//----------------------------------------------------------------------
/** Stop rendering.
 */

void CqImageBuffer::Quit()
{
	m_fQuit = true;
}

//----------------------------------------------------------------------
/** Move to the next bucket to process.

  Computes the next bucket based on the "render" "bucketorder" given.

  \return True if there is still an unprocessed bucket left, otherwise False.
 */
bool CqImageBuffer::NextBucket(EqBucketOrder order)
{

	TqInt cnt = 0;
	TqInt total = ((m_cXBuckets - 1) * (m_cYBuckets - 1));


	for (TqInt i =0; i < m_cYBuckets - 1 ; i++)
		for (TqInt j = 0; j < m_cXBuckets - 1; j++)
			if (Bucket(j, i).IsProcessed() )
				cnt ++;
         
	if ((order != Bucket_Vertical) &&
	        (order != Bucket_Horizontal) &&
	        (cnt == total))
		return false;

	switch (order)
	{

		case Bucket_Random :
		{
			CqRandom rg;
			do
			{
				m_CurrentBucketCol = (TqInt) rg.RandomFloat(m_cXBuckets);
				m_CurrentBucketRow = (TqInt) rg.RandomFloat(m_cYBuckets);
				m_CurrentBucketCol = CLAMP(m_CurrentBucketCol, 0, m_cXBuckets - 1);
				m_CurrentBucketRow = CLAMP(m_CurrentBucketRow, 0, m_cYBuckets - 1);
			}
			while ( Bucket(m_CurrentBucketCol, m_CurrentBucketRow).IsProcessed() );

		}
		break;

		case Bucket_Circle :
		{
			static TqInt radius = 0;
			static float theta = 0.0f;

			if ((m_CurrentBucketCol == m_CurrentBucketRow) &&
				(m_CurrentBucketRow == 0))
			{
				radius = 0;
				theta = 0.0f;
			}
			TqFloat r = (TqFloat) sqrt(static_cast<double>((m_cXBuckets * m_cXBuckets) + (m_cYBuckets * m_cYBuckets))) + 2;
			TqInt midx = m_cXBuckets/2;
			TqInt midy = m_cYBuckets/2;


			TqFloat delta = 0.01f;

			do
			{
				if (radius > r)
					break;
				m_CurrentBucketCol = midx + (TqInt) (radius * cos(theta));
				m_CurrentBucketRow = midy + (TqInt) (radius * sin(theta));

				theta += delta;
				if (theta > 6.28f)
				{
					theta = 0.0f;
					radius ++;
				}
				if (radius > r)
					break;

				m_CurrentBucketCol = CLAMP(m_CurrentBucketCol, 0, m_cXBuckets - 1);
				m_CurrentBucketRow = CLAMP(m_CurrentBucketRow, 0, m_cYBuckets - 1);
			}
			while (Bucket(m_CurrentBucketCol, m_CurrentBucketRow).IsProcessed());

			if (radius > r)
			{
				// Maybe a bucket was not done
				for (TqInt i =0; i < m_cYBuckets - 1 ; i++)
					for (TqInt j = 0; j < m_cXBuckets - 1; j++)
						if (!Bucket(j, i).IsProcessed() )
						{
							m_CurrentBucketCol = j;
							m_CurrentBucketRow = i;

						}
			}


		}
		break;

		case Bucket_ZigZag :
		{

			if (bucketdirection == 1)
				m_CurrentBucketCol++;
			else
				m_CurrentBucketCol--;

			if((bucketdirection == 1) && ( m_CurrentBucketCol >= m_cXBuckets ))
			{

				m_CurrentBucketCol = m_cXBuckets-1;
				m_CurrentBucketRow++;
				bucketdirection = -1;
				if( m_CurrentBucketRow >= m_cYBuckets )
					return( false );

			}
			else if((bucketdirection == -1) && ( m_CurrentBucketCol < 0 ))
			{

				m_CurrentBucketCol = 0;
				m_CurrentBucketRow++;
				bucketdirection = 1;

				if( m_CurrentBucketRow >= m_cYBuckets )
					return( false );

			}
		}
		break;
		case Bucket_Vertical :
		{

			m_CurrentBucketRow++;

			if( m_CurrentBucketRow >= m_cYBuckets )
			{

				m_CurrentBucketRow = 0;
				m_CurrentBucketCol++;

				if( m_CurrentBucketCol >= m_cXBuckets )
					return( false );

			}
		}
		break;
		default:
		case Bucket_Horizontal :
		{

			m_CurrentBucketCol++;

			if( m_CurrentBucketCol >= m_cXBuckets )
			{

				m_CurrentBucketCol = 0;
				m_CurrentBucketRow++;
				if( m_CurrentBucketRow >= m_cYBuckets )
					return( false );

			}
		}
		break;
	}
	return( true );
}

//---------------------------------------------------------------------

END_NAMESPACE( Aqsis )



