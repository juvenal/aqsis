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
		\brief Declares the classes for handling micropolygrids and micropolygons.
		\author Paul C. Gregory (pgregory@aqsis.org)
*/

//? Is .h included already?
#ifndef MICROPOLYGON_H_INCLUDED
#define MICROPOLYGON_H_INCLUDED 1

#include	"aqsis.h"

#include	"pool.h"
#include	"color.h"
#include	"list.h"
#include	"bound.h"
#include	"vector2d.h"
#include	"shaderexecenv.h"
#include	"ishaderdata.h"
#include	"motion.h"
#include	"csgtree.h"
#include	"refcount.h"
#include	"logging.h"
#include       <boost/utility.hpp>

namespace Aqsis {

class CqVector3D;
class CqImageBuffer;
class CqSurface;
class CqMicroPolygon;
struct SqSampleData;
class CqBucketProcessor;

// This struct holds info about a grid that can be cached and used for all its mpgs.
struct SqGridInfo
{
	TqFloat			m_ShadingRate;
	TqFloat			m_ShutterOpenTime;
	TqFloat			m_ShutterCloseTime;
	const TqFloat*	        m_LodBounds;
	bool			m_IsMatte;
	bool			m_IsCullable;
	bool			m_UsesDataMap;
};


//----------------------------------------------------------------------
/** \brief Cache of output sample data for a micropoly
 *
 * This cache holds colour and opacity output data for a micropolygon.  The
 * coefficients allow colour and opacity to be interpolated across the face of
 * the micropolygon.  For CqMicroPolygon, interpolation may be either "smooth"
 * (uses a linear approximation over the micropoly) or "constant" - using a
 * constant value for each micropolygon.
 *
 * AOV's are handled seperately - they do not support smooth shading in the
 * current implementation.
 *
 * \todo Code Review: This struct is a bit kludgy, holding stuff which should
 * be only visible to the micropolygon implementation classes.  It's made worse
 * by the fact that the smooth shading interpolation coefficients are
 * irrelevant to some micropolygon subclasses (CqMicroPolygonPoints for eg).
 */
struct SqMpgSampleInfo
{
	/// Color for flat shading and for the "zero" point of smooth shading.
	CqColor color;
	/// Color multipilers used for smooth shading interpolation
	CqColor colorMultX;
	CqColor colorMultY;

	/// Opacity
	CqColor opacity;
	/// Opacity multipilers used for smooth shading interpolation
	CqColor opacityMultX;
	CqColor opacityMultY;

	/// Whether to use smooth shading interpolation or not
	bool smoothInterpolation;

	/// Whether the opacity is full.
	bool occludes;
	/// Whether to store samples deriving from this micropoly in the opaque
	/// sample slot or not.
	bool isOpaque;
};


//----------------------------------------------------------------------
/** \class CqMicroPolyGridBase
 * Base class from which all MicroPolyGrids are derived.
 */

class CqMicroPolyGridBase : public CqRefCount
{
	public:
		CqMicroPolyGridBase() : m_fCulled( false ), m_fTriangular( false )
		{}
		virtual	~CqMicroPolyGridBase()
		{}

		/** Pure virtual function, splits the grid into micropolys.
		 * \param pBP Pointer to the bucket processor for the current bucket.
		 */
		virtual	void	Split( long xmin, long xmax, long ymin, long ymax ) = 0;
		/** Pure virtual, shade the grid.
		 */
		virtual	void	Shade(bool canCullGrid = true ) = 0;
		virtual	void	TransferOutputVariables() = 0;
		/*
		 * Delete all the variables per grid 
		 */
		virtual void DeleteVariables( bool all ) = 0;
		/** Pure virtual, get a pointer to the surface this grid belongs.
		 * \return Pointer to surface, only valid during grid shading.
		 */
		virtual CqSurface*	pSurface() const = 0;

		virtual	const IqConstAttributesPtr pAttributes() const = 0;

		virtual bool	usesCSG() const = 0;
		virtual	boost::shared_ptr<CqCSGTreeNode> pCSGNode() const = 0;
		bool vfCulled()
		{
			return m_fCulled;
		}
		/** Query whether this grid is being rendered as a triangle.
		 */
		virtual bool fTriangular() const
		{
			return ( m_fTriangular );
		}
		/** Set this grid as being rendered as a triangle or not.
		 */
		virtual void SetfTriangular( bool fTriangular )
		{
			m_fTriangular = fTriangular;
		}
		virtual	TqInt	uGridRes() const = 0;
		virtual	TqInt	vGridRes() const = 0;
		virtual	TqUint	numMicroPolygons(TqInt cu, TqInt cv) const = 0;
		virtual	TqUint	numShadingPoints(TqInt cu, TqInt cv) const = 0;
		virtual bool	hasValidDerivatives() const = 0;
		virtual IqShaderData* pVar(TqInt index) = 0;
		/** Get the points of the triangle split line if this grid represents a triangle.
		 */
		virtual void	TriangleSplitPoints(CqVector3D& v1, CqVector3D& v2, TqFloat Time );

		struct SqTriangleSplitLine
		{
			CqVector3D	m_TriangleSplitPoint1, m_TriangleSplitPoint2;
		};
	class CqTriangleSplitLine : public CqMotionSpec<SqTriangleSplitLine>
		{
			public:
				CqTriangleSplitLine( const SqTriangleSplitLine& def = SqTriangleSplitLine() ) : CqMotionSpec<SqTriangleSplitLine>( def )
				{}

				/**
				* \todo Review: Unused parameter A
				*/		
				virtual	void	ClearMotionObject( SqTriangleSplitLine& A ) const
				{}

				/**
				* \todo Review: Unused parameter B
				*/		
				virtual	SqTriangleSplitLine	ConcatMotionObjects( const SqTriangleSplitLine& A, const SqTriangleSplitLine& B ) const
				{
					return( A );
				}
				virtual	SqTriangleSplitLine	LinearInterpolateMotionObjects( TqFloat Fraction, const SqTriangleSplitLine& A, const SqTriangleSplitLine& B ) const
				{
					SqTriangleSplitLine sl;
					sl.m_TriangleSplitPoint1 = ( ( 1.0f - Fraction ) * A.m_TriangleSplitPoint1 ) + ( Fraction * B.m_TriangleSplitPoint1 );
					sl.m_TriangleSplitPoint2 = ( ( 1.0f - Fraction ) * A.m_TriangleSplitPoint2 ) + ( Fraction * B.m_TriangleSplitPoint2 );
					return( sl );
				}
		};
		virtual	IqShaderData* FindStandardVar( const char* pname ) = 0;
		virtual boost::shared_ptr<IqShaderExecEnv> pShaderExecEnv() = 0; 

		const SqGridInfo& GetCachedGridInfo() const
		{
			return m_CurrentGridInfo;
		}

	protected:
		bool m_fCulled; ///< Boolean indicating the entire grid is culled.
		CqTriangleSplitLine	m_TriangleSplitLine;	///< Two endpoints of the line that is used to turn the quad into a triangle at sample time.
		bool	m_fTriangular;			///< Flag indicating that this grid should be rendered as a triangular grid with a phantom fourth corner.

		/** Cached info about the given grid so it can be
		 * referenced by multiple mpgs. */
		SqGridInfo m_CurrentGridInfo;

		/** Cache some info about the given grid so it can be
		 * referenced by multiple mpgs. */
		void CacheGridInfo();
};


//----------------------------------------------------------------------
/** \class CqMicroPolyGrid
 * Class which stores a grid of micropolygons.
 */

class CqMicroPolyGrid : public CqMicroPolyGridBase
{
	public:
		CqMicroPolyGrid();
		virtual	~CqMicroPolyGrid();

#ifdef _DEBUG

		CqString className() const
		{
			return CqString("CqMicroPolyGrid");
		}
#endif

		virtual void CalcNormals();
		virtual void CalcSurfaceDerivatives();
		/** \brief Expand the boundary micropolygons to cover grid cracks.
		 *
		 * This function expands the boundary of a grid by moving the boundary
		 * vertices outward along the vectors which connect them to the
		 * interior vertices of the grid.  This is a computationally cheap and
		 * easy way to cover cracks between adjacent grids which result from
		 * differing dicing rates.
		 *
		 * \param amount - fraction of a micropolygon to expand the boundary
		 *                 outward by.
		 */
		void ExpandGridBoundaries(TqFloat amount);
		/** Set the shading normals flag, indicating this grid has shading (N) normals already specified.
		 * \param f The new state of the flag.
		 */
		void	SetbShadingNormals( bool f )
		{
			m_bShadingNormals = f;
		}
		/** Set the geometric normals flag, indicating this grid has geometric (Ng) normals already specified.
		 * \param f The new state of the flag.
		 */
		void	SetbGeometricNormals( bool f )
		{
			m_bGeometricNormals = f;
		}
		/** Query whether shading (N) normals have been filled in by the surface at dice time.
		 */
		bool bShadingNormals() const
		{
			return ( m_bShadingNormals );
		}
		/** Query whether geometric (Ng) normals have been filled in by the surface at dice time.
		 */
		bool bGeometricNormals() const
		{
			return ( m_bGeometricNormals );
		}

		/** Get a reference to the bitvector representing the culled status of each u-poly in this grid.
		 */
		CqBitVector& CulledPolys()
		{
			return ( m_CulledPolys );
		}
		/** Get a reference to the bitvector representing the culled status of each u-poly in this grid.
		 */
		const CqBitVector& CulledPolys() const
		{
			return ( m_CulledPolys );
		}

		void	Initialise( TqInt cu, TqInt cv, const boost::shared_ptr<CqSurface>& pSurface );

		void DeleteVariables( bool all );

		// Overrides from CqMicroPolyGridBase
		virtual	void	Split( long xmin, long xmax, long ymin, long ymax );
		virtual	void	Shade( bool canCullGrid = true );
		virtual	void	TransferOutputVariables();

		/** Get a pointer to the surface which this grid belongs.
		 * \return Surface pointer, only valid during shading.
		 */
		virtual CqSurface*	pSurface() const
		{
			return ( m_pSurface.get() );
		}
		virtual	const IqConstAttributesPtr pAttributes() const
		{
			assert( m_pShaderExecEnv );
			return ( m_pShaderExecEnv->pAttributes() );
		}
		virtual bool	usesCSG() const
		{
			return (m_pCSGNode.get() != NULL);
		}
		virtual	boost::shared_ptr<CqCSGTreeNode> pCSGNode() const
		{
			return ( m_pCSGNode );
		}

		// Redirect acces via IqShaderExecEnv
		virtual	TqInt	uGridRes() const
		{
			assert( m_pShaderExecEnv );
			return ( m_pShaderExecEnv->uGridRes() );
		}
		virtual	TqInt	vGridRes() const
		{
			assert( m_pShaderExecEnv );
			return ( m_pShaderExecEnv->vGridRes() );
		}
		virtual	TqUint	numMicroPolygons(TqInt cu, TqInt cv) const
		{
			return ( cu * cv );
		}
		virtual	TqUint	numShadingPoints(TqInt cu, TqInt cv) const
		{
			return ( ( cu + 1 ) * ( cv + 1 ) );
		}
		virtual bool	hasValidDerivatives() const
		{
			return true;
		}
		virtual	const CqMatrix&	matObjectToWorld() const
		{
			assert( m_pShaderExecEnv );
			return ( m_pShaderExecEnv->matObjectToWorld() );
		}
		virtual IqShaderData* pVar(TqInt index)
		{
			assert( m_pShaderExecEnv );
			return ( m_pShaderExecEnv->pVar(index) );
		}
		virtual	IqShaderData* FindStandardVar( const char* pname )
		{
			IqShaderData* pVar = NULL;
			if( ( pVar = m_pShaderExecEnv->FindStandardVar( pname ) ) == NULL )
			{
				std::vector<IqShaderData*>::iterator outputVar;
				for( outputVar = m_apShaderOutputVariables.begin(); outputVar != m_apShaderOutputVariables.end(); outputVar++ )
				{
					if( (*outputVar)->strName() == pname )
					{
						pVar = (*outputVar);
						break;
					}
				}
			}
			return( pVar );
		}
		virtual boost::shared_ptr<IqShaderExecEnv> pShaderExecEnv() 
		{
			return(m_pShaderExecEnv);
		}

		/** \brief Set surface derivative in u.
		 * 
		 *  Set the value of du if needed, du is constant across the grid being shaded.
		 */
		virtual void setDu();
		/** \brief Set surface derivative in v.
		 * 
		 *  Set the value of dv if needed, dv is constant across the grid being shaded.
		 */
		virtual void setDv();

	private:
		bool	m_bShadingNormals;		///< Flag indicating shading normals have been filled in and don't need to be calculated during shading.
		bool	m_bGeometricNormals;	///< Flag indicating geometric normals have been filled in and don't need to be calculated during shading.
		boost::shared_ptr<CqSurface> m_pSurface;	///< Pointer to the surface for this grid.
		boost::shared_ptr<CqCSGTreeNode> m_pCSGNode;	///< Pointer to the CSG tree node this grid belongs to, NULL if not part of a solid.
		CqBitVector	m_CulledPolys;		///< Bitvector indicating whether the individual micro polygons are culled.
		std::vector<IqShaderData*>	m_apShaderOutputVariables;	///< Vector of pointers to shader output variables.
	protected:
		boost::shared_ptr<IqShaderExecEnv> m_pShaderExecEnv;	///< Pointer to the shader execution environment for this grid.

}
;


//----------------------------------------------------------------------
/** \class CqMotionMicroPolyGrid
 * Class which stores info about motion blurred micropolygrids.
 */

class CqMotionMicroPolyGrid : public CqMicroPolyGridBase, public CqMotionSpec<CqMicroPolyGridBase*>
{
	public:
		CqMotionMicroPolyGrid() : CqMicroPolyGridBase(), CqMotionSpec<CqMicroPolyGridBase*>( 0 )
		{}
		virtual	~CqMotionMicroPolyGrid();
		// Overrides from CqMicroPolyGridBase


		virtual	void	Split( long xmin, long xmax, long ymin, long ymax );
		virtual	void	Shade( bool canCullGrid = true );
		virtual	void	TransferOutputVariables();
		
		/**
		* \todo Review: Unused parameter all
		*/		
		void DeleteVariables( bool all )
		{}

		// Redirect acces via IqShaderExecEnv
		virtual	TqInt	uGridRes() const
		{
			assert( GetMotionObject( Time( 0 ) ) );
			return ( static_cast<CqMicroPolyGrid*>( GetMotionObject( Time( 0 ) ) ) ->uGridRes() );
		}
		virtual	TqInt	vGridRes() const
		{
			assert( GetMotionObject( Time( 0 ) ) );
			return ( static_cast<CqMicroPolyGrid*>( GetMotionObject( Time( 0 ) ) ) ->vGridRes() );
		}
		virtual	TqUint	numMicroPolygons(TqInt cu, TqInt cv) const
		{
			assert( GetMotionObject( Time( 0 ) ) );
			return ( static_cast<CqMicroPolyGrid*>( GetMotionObject( Time( 0 ) ) ) ->numMicroPolygons(cu, cv) );
		}
		virtual	TqUint	numShadingPoints(TqInt cu, TqInt cv) const
		{
			assert( GetMotionObject( Time( 0 ) ) );
			return ( static_cast<CqMicroPolyGrid*>( GetMotionObject( Time( 0 ) ) ) ->numShadingPoints(cu, cv) );
		}
		virtual bool	hasValidDerivatives() const
		{
			assert( GetMotionObject( Time( 0 ) ) );
			return ( static_cast<CqMicroPolyGrid*>( GetMotionObject( Time( 0 ) ) ) ->hasValidDerivatives() );
		}

		virtual IqShaderData* pVar(TqInt index)
		{
			assert( GetMotionObject( Time( 0 ) ) );
			return ( static_cast<CqMicroPolyGrid*>( GetMotionObject( Time( 0 ) ) ) ->pVar(index) );
		}
		virtual	IqShaderData* FindStandardVar( const char* pname )
		{
			assert( GetMotionObject( Time( 0 ) ) );
			return ( static_cast<CqMicroPolyGrid*>( GetMotionObject( Time( 0 ) ) ) ->FindStandardVar(pname) );
		}

		virtual boost::shared_ptr<IqShaderExecEnv> pShaderExecEnv() 
		{
			assert( GetMotionObject( Time( 0 ) ) );
			return ( static_cast<CqMicroPolyGrid*>( GetMotionObject( Time( 0 ) ) ) ->pShaderExecEnv() );
		}

		/** Get a pointer to the surface which this grid belongs.
		 * Actually returns the surface pointer from the first timeslot.
		 * \return Surface pointer, only valid during shading.
		 */
		virtual CqSurface*	pSurface() const
		{
			return ( static_cast<CqMicroPolyGrid*>( GetMotionObject( Time( 0 ) ) ) ->pSurface() );
		}

		virtual const IqConstAttributesPtr pAttributes() const
		{
			return ( static_cast<CqMicroPolyGrid*>( GetMotionObject( Time( 0 ) ) ) ->pAttributes() );
		}

		virtual bool	usesCSG() const
		{
			return(static_cast<CqMicroPolyGrid*>( GetMotionObject( Time( 0 ) ) ) ->usesCSG());
		}
		virtual boost::shared_ptr<CqCSGTreeNode> pCSGNode() const
		{
			return ( static_cast<CqMicroPolyGrid*>( GetMotionObject( Time( 0 ) ) ) ->pCSGNode() );
		}

		/**
		* \todo Review: Unused parameter A
		*/		
		virtual	void	ClearMotionObject( CqMicroPolyGridBase*& A ) const
		{}

		/** Overridden from CqMotionSpec, does nothing.
		 */
		virtual	CqMicroPolyGridBase* ConcatMotionObjects( CqMicroPolyGridBase* const & /* A */, CqMicroPolyGridBase* const & B ) const
		{
			return ( B );
		}
		/** Overridden from CqMotionSpec, does nothing.
		 */
		virtual	CqMicroPolyGridBase* LinearInterpolateMotionObjects( TqFloat /* Fraction */, CqMicroPolyGridBase* const & A, CqMicroPolyGridBase* const & /* B */ ) const
		{
			return ( A );
		}
		void	Initialise( TqInt cu, TqInt cv, const boost::shared_ptr<CqSurface>& pSurface )
		{
			//assert( GetMotionObject( Time( 0 ) ) );
			//static_cast<CqMicroPolyGrid*>( GetMotionObject( Time( 0 ) ) ) ->Initialise(cu, cv, pSurface);
			CacheGridInfo();
		}

	private:
};

//----------------------------------------------------------------------
/** \struct CqHitTestCache
 * struct holding data used during the point in poly test.
 */

struct CqHitTestCache
{
	// these 3 are used in calculating the interpolated depth value
	CqVector3D m_VecN;
	TqFloat m_OneOverVecNZ;
	TqFloat m_D;

	// these 4 hold values used in doing the edge tests. 1 of each for each edge.
	TqFloat m_YMultiplier[4];
	TqFloat m_XMultiplier[4];
	TqFloat m_X[4];
	TqFloat m_Y[4];

	// this holds the index of the last edge that failed an edge test, chances
	// are it will fail on the next sample as well, so we test this edge first.
	TqInt	m_LastFailedEdge;

	// These four vectors hold the circle of confusion dof offset multipliers
	// for each of the four vertices in the order PointB(), PointC, PointD(),
	// PointA().
	CqVector2D cocMult[4];

	// These two vectors hold the extents of the circle of confusion
	// multipliers, used during fast sample rejection based on the bounding
	// box.
	CqVector2D cocMultMin;
	CqVector2D cocMultMax;
};

//----------------------------------------------------------------------
/** \class CqMicroPolygon
 * Abstract base class from which static and motion micropolygons are derived.
 */

class CqMicroPolygon : boost::noncopyable
{
	public:
		/** Constructor, setting up the pointer to the grid
		 * this micropoly came from; and the index of the
		 * shading point associated with this micropolygon
		 * within the donor grid.
		 * 
		 * \param pGrid CqMicroPolyGrid pointer.
		 * \param Index Integer grid index.
		 */
		CqMicroPolygon( CqMicroPolyGridBase* pGrid, TqInt Index );
		virtual	~CqMicroPolygon();

		/** Overridden operator new to allocate micropolys from a pool.
		 * \todo Review: Unused parameter size
		 */
		void* operator new( size_t size )
		{
			return( m_thePool.alloc() );
		}

		/** Overridden operator delete to allocate micropolys from a pool.
		 */
		void operator delete( void* p )
		{
			m_thePool.free( reinterpret_cast<CqMicroPolygon*>(p) );
		}

#ifdef _DEBUG
		CqString className() const
		{
			return CqString("CqMicroPolygon");
		}
#endif

	private:
		enum EqMicroPolyFlags
		{
			MicroPolyFlags_Trimmed		= 0x0001,
			MicroPolyFlags_Hit		= 0x0002,
			MicroPolyFlags_PushedForward	= 0x0004,
		};

	public:
		/** Get the pointer to the grid this micropoly came from.
		 * \return Pointer to the CqMicroPolyGrid.
		 */
		CqMicroPolyGridBase* pGrid() const
		{
			return ( m_pGrid );
		}
		/** Get the index into the grid of this MPG
		 */
		TqInt GetIndex() const
		{
			return( m_Index );
		}
		/** Get the color of this micropoly.
		 * \return CqColor reference.
		 */
		const CqColor*	colColor() const
		{
			CqColor* col;
			m_pGrid->pVar(EnvVars_Ci) ->GetColorPtr( col );
			return ( &col[m_Index] );
		}
		/** Get the opacity of this micropoly.
		 * \return CqColor reference.
		 */
		const	CqColor* colOpacity() const
		{
			CqColor* col;
			m_pGrid->pVar(EnvVars_Oi) ->GetColorPtr( col );
			return ( &col[m_Index] );
		}

		/** \brief Get the tight micropolygon bound (not including DoF).
		 *
		 * This bound should tightly cover the region in raster space directly
		 * touched by the micropolygon.  In the moving case this includes any
		 * parts of the area and depth swept out during the motion.
		 */
		const CqBound& GetBound() const
		{
			return m_Bound;
		}

		// Overridables
		virtual	TqInt	cSubBounds( TqUint timeRanges )
		{
			return ( 1 );
		}

		/**
		* \todo Review: Unused parameter iIndex
		*/				
		virtual	CqBound SubBound( TqInt iIndex, TqFloat& time ) const
		{
			time = 0.0f;
			return ( GetBound() );
		}

		/** Query if the micropolygon has been successfully hit by a pixel sample.
		 */
		bool IsHit() const
		{
			return ( ( m_Flags & MicroPolyFlags_Hit ) != 0 );
		}
		/** Set the flag to state that the MPG has eben hit by a sample point.
		 */
		void MarkHit()
		{
			m_Flags |= MicroPolyFlags_Hit;
		}
		/** Get the flag indicating if the micropoly has already beed pushed forward to the next bucket.
		 */
		bool	IsPushedForward() const
		{
			return ( ( m_Flags & MicroPolyFlags_PushedForward ) != 0 );
		}
		/** Set the flag indicating if the micropoly has already beed pushed forward to the next bucket.
		 */
		void	MarkPushedForward()
		{
			m_Flags |= MicroPolyFlags_PushedForward;
		}
		virtual void	MarkTrimmed()
		{
			m_Flags |= MicroPolyFlags_Trimmed;
		}
		virtual bool	IsTrimmed() const
		{
			return ( ( m_Flags & MicroPolyFlags_Trimmed ) != 0 );
		}

		virtual bool IsMoving() const
		{
			return false;
		}

		/** Check if the sample point is within the micropoly.
		 * \param vecSample 2D sample point.
		 * \param time The frame time at which to check.
		 * \param D storage to put the depth at the sample point if success.
		 * \return Boolean success.
		 */
		virtual	bool	Sample( CqHitTestCache& hitTestCache, const SqSampleData& sample, TqFloat& D, TqFloat time, bool UsingDof = false ) const;

		virtual bool	fContains( CqHitTestCache& hitTestCache, const CqVector2D& vecP, TqFloat& Depth, TqFloat time ) const;
		virtual void	CacheHitTestValues(CqHitTestCache* cache) const;
		/** \brief Cache sampling data related to depth of field.
		 *
		 * The circle of confusion multipliers for each vertex of a static
		 * micropolygon are independent of the sample point and can be cached.
		 * The minimum and maximum of these multipliers can also be cached, and
		 * used in the bounding box test for fast sample rejection.
		 *
		 * Child classes should override this function in order to cache
		 * any relevant depth of field data which can be reused for multiple
		 * sample points.
		 *
		 * \param cache - storage for the cached CoC multiplers.
		 */
		virtual void CacheCocMultipliers(CqHitTestCache& cache) const;

		/** \brief Cache information needed to interpolate colour and opacity
		 * across the micropolygon.
		 *
		 * This function should caches constant values for the micropolygon, or
		 * computes the interpolation coefficients necessary for smooth
		 * shading.
		 *
		 * \param cache - location into which to store the interpolation coefficients.
		 */
		virtual void CacheOutputInterpCoeffs(SqMpgSampleInfo& cache) const;

		/** \brief Get colour and opacity at pos using cached coefficients
		 *
		 * \param cache - previously cached coefficients for this micropolygon
		 * \param pos - position to evaluate color and opacity at
		 * \param outCol - interpolated colour output will be placed here.
		 * \param outOpac - interpolated opacity output will be placed here.
		 */
		virtual void InterpolateOutputs(const SqMpgSampleInfo& cache,
				const CqVector2D& pos, CqColor& outCol, CqColor& outOpac) const;

		void	Initialise();
		CqVector2D ReverseBilinear( const CqVector2D& v ) const;

		virtual const CqVector3D& PointA() const
		{
			CqVector3D * pP = NULL;
			m_pGrid->pVar(EnvVars_P) ->GetPointPtr( pP );
			return ( pP[ GetCodedIndex( m_IndexCode, 0 ) ] );
		}
		virtual const CqVector3D& PointB() const
		{
			CqVector3D * pP = NULL;
			m_pGrid->pVar(EnvVars_P) ->GetPointPtr( pP );
			return ( pP[ GetCodedIndex( m_IndexCode, 1 ) ] );
		}
		virtual const CqVector3D& PointC() const
		{
			CqVector3D * pP = NULL;
			m_pGrid->pVar(EnvVars_P) ->GetPointPtr( pP );
			return ( pP[ GetCodedIndex( m_IndexCode, 2 ) ] );
		}
		virtual const CqVector3D& PointD() const
		{
			CqVector3D * pP = NULL;
			m_pGrid->pVar(EnvVars_P) ->GetPointPtr( pP );
			return ( pP[ GetCodedIndex( m_IndexCode, 3 ) ] );
		}
		virtual const bool IsDegenerate() const
		{
			return ( ( m_IndexCode & 0x8000000 ) != 0 );
		}

	protected:
		/** \brief Cache output interpolation coefficients for constant shading
		 * \see CacheOutputInterpCoeffs
		 */
		virtual void CacheOutputInterpCoeffsConstant(SqMpgSampleInfo& cache) const;
		/** \brief Cache output interpolation coefficients for smooth shading
		 * \see CacheOutputInterpCoeffs
		 */
		virtual void CacheOutputInterpCoeffsSmooth(SqMpgSampleInfo& cache) const;

		/** \brief Calculate and store the bound of the micropoly.
		 */
		void CalculateTotalBound();

		TqInt GetCodedIndex( TqShort code, TqShort shift ) const
		{
			switch ( ( ( code >> ( shift << 1 ) ) & 0x3 ) )
			{
					case 1:
					return ( m_Index + 1 );
					case 2:
					return ( m_Index + m_pGrid->uGridRes() + 2 );
					case 3:
					return ( m_Index + m_pGrid->uGridRes() + 1 );
					default:
					return ( m_Index );
			}
		}
		TqInt m_IndexCode;
		CqBound	m_Bound;		///< Tight bound, not including DoF.

		CqMicroPolyGridBase*	m_pGrid;		///< Pointer to the donor grid.
		TqInt	m_Index;		///< Index within the donor grid.

		TqShort	m_Flags;		///< Bitvector of general flags, using EqMicroPolyFlags as bitmasks.

		virtual void	CacheHitTestValues(CqHitTestCache* cache, CqVector3D* points) const;

	private:
		static	CqObjectPool<CqMicroPolygon> m_thePool;
}
;



//----------------------------------------------------------------------
/** \class CqMovingMicroPolygonKey
 * Base lass for static micropolygons. Stores point information about the geometry of the micropoly.
 */

class CqMovingMicroPolygonKey
{
	public:
		CqMovingMicroPolygonKey()
		{}
		CqMovingMicroPolygonKey( const CqVector3D& vA, const CqVector3D& vB, const CqVector3D& vC, const CqVector3D& vD )
		{
			Initialise( vA, vB, vC, vD );
		}
		~CqMovingMicroPolygonKey()
		{}

		/** Overridden operator new to allocate micropolys from a pool.
		 * \todo Review: Unused parameter size
		 */
		void* operator new( size_t size )
		{
			return( m_thePool.alloc() );
		}

		/** Overridden operator delete to allocate micropolys from a pool.
		 */
		void operator delete( void* p )
		{
			m_thePool.free( reinterpret_cast<CqMovingMicroPolygonKey*>(p) );
		}


	public:
		const CqBound&	GetBound();
		void	Initialise( const CqVector3D& vA, const CqVector3D& vB, const CqVector3D& vC, const CqVector3D& vD );
		CqVector2D ReverseBilinear( const CqVector2D& v ) const;

		const bool IsDegenerate() const
		{
			return ( m_Point2 == m_Point3 );
		}

		CqVector3D	m_Point0;
		CqVector3D	m_Point1;
		CqVector3D	m_Point2;
		CqVector3D	m_Point3;
		CqVector3D	m_N;			///< The normal to the micropoly.

	protected:
		TqFloat	m_D;				///< Distance of the plane from the origin, used for calculating sample depth.

		CqBound m_Bound;
		bool	m_BoundReady;

		static	CqObjectPool<CqMovingMicroPolygonKey>	m_thePool;
}
;


//----------------------------------------------------------------------
/** \class CqMicroPolygonMotion
 * Class which stores a single moving micropolygon.
 */

class CqMicroPolygonMotion : public CqMicroPolygon
{
	public:
		CqMicroPolygonMotion( CqMicroPolyGridBase* pGrid, TqInt Index ) :
			CqMicroPolygon( pGrid, Index ), m_BoundReady( false )
		{ }
		virtual	~CqMicroPolygonMotion()
		{
			std::vector<CqMovingMicroPolygonKey*>::iterator	ikey;
			for ( ikey = m_Keys.begin(); ikey != m_Keys.end(); ikey++ )
				delete( ( *ikey ) );
		}

		/** Overridden operator new to avoid the pool allocator from CqMicroPolygon.
		 */
		void* operator new( size_t size )
		{
			return( malloc(size) );
		}

		/** Overridden operator delete to allocate micropolys from a pool.
		 */
		void operator delete( void* p )
		{
			free( p );
		}


	public:
		void	AppendKey( const CqVector3D& vA, const CqVector3D& vB, const CqVector3D& vC, const CqVector3D& vD, TqFloat time );
		void	DeleteVariables( bool all )
		{}

		// Overrides from CqMicroPolygon
		virtual	TqInt	cSubBounds( TqUint timeRanges )
		{
			if ( !m_BoundReady )
				BuildBoundList( timeRanges );
			return ( m_BoundList.Size() );
		}
		virtual	CqBound	SubBound( TqInt iIndex, TqFloat& time ) const
		{
			if ( !m_BoundReady )
				Aqsis::log() << error << "MP bound list not ready" << std::endl;
			assert( iIndex < static_cast<TqInt>(m_BoundList.Size()) );
			time = m_BoundList.GetTime( iIndex );
			return ( m_BoundList.GetBound( iIndex ) );
		}
		virtual void	BuildBoundList( TqUint timeRanges );

		virtual	bool	Sample( CqHitTestCache& hitTestCache, const SqSampleData& sample, TqFloat& D, TqFloat time, bool UsingDof = false ) const;

		virtual void CacheCocMultipliers(CqHitTestCache& cache) const;

		virtual void	MarkTrimmed()
		{
			m_fTrimmed = true;
		}

		virtual bool IsMoving() const
		{
			return true;
		}

		virtual const bool IsDegenerate() const
		{
			return ( m_Keys[0]->IsDegenerate() );
		}

		virtual TqInt cKeys() const
		{
			return(m_Keys.size());
		}

		virtual TqFloat Time(TqUint index) const
		{
			assert( index < m_Times.size() );
			return(m_Times[index]);
		}

		virtual CqMovingMicroPolygonKey* Key(TqUint index) const
		{
			assert( index < m_Keys.size() );
			return(m_Keys[index]);
		}

	protected:
		CqBoundList	m_BoundList;			///< List of bounds to get a tighter fit.
		bool	m_BoundReady;				///< Flag indicating the boundary has been initialised.
		std::vector<TqFloat> m_Times;
		std::vector<CqMovingMicroPolygonKey*>	m_Keys;
		bool	m_fTrimmed;		///< Flag indicating that the MPG spans a trim curve.
}
;

//==============================================================================
// Implementation details
//==============================================================================
inline void CqMicroPolyGrid::setDu()
{
	float f0 = 0;
	float f1 = 0;
	pVar(EnvVars_u)->GetValue(f0, 0);
	pVar(EnvVars_u)->GetValue(f1, 1);
	pVar(EnvVars_du)->SetFloat(f1 - f0);
}

inline void CqMicroPolyGrid::setDv()
{
	float f0 = 0;
	float f1 = 0;
	pVar(EnvVars_v)->GetValue(f0, 0);
	pVar(EnvVars_v)->GetValue(f1, uGridRes() + 1);
	pVar(EnvVars_dv)->SetFloat(f1 - f0);
}


//-----------------------------------------------------------------------

} // namespace Aqsis

#endif	// !MICROPOLYGON_H_INCLUDED


