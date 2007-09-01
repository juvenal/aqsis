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
		\brief Declares the CqBucket class responsible for bookeeping the primitives and storing the results.
		\author Paul C. Gregory (pgregory@aqsis.org)
*/

#ifndef BUCKET_H_INCLUDED
#define BUCKET_H_INCLUDED 1

#include	"aqsis.h"

#include	<vector>
#include	<queue>
#include	<deque>

#include	"iddmanager.h"
#include	"bitvector.h"
#include	"micropolygon.h"
#include	"renderer.h"
#include	"ri.h"
#include	"surface.h"
#include	"color.h"
#include	"vector2d.h"
#include    "imagepixel.h"

START_NAMESPACE( Aqsis )

//------------------------------------------------------------------------------
/** \brief Structure representing changes in visibility at a specific depth
 *
 * A visibility function is a collection of these structures.
 */
struct SqDeltaNode
{
	TqFloat zdepth;
	/// an [additive] change in light color associated with a transmittance function "hit" at this zdepth
	CqColor deltaTransmittance;
	/// an [additive] change in transmittance function slope occuring at this node
	CqColor deltaslope;
};

//------------------------------------------------------------------------------
/** \brief Structure a hit heap entry
 * These are used in the transmittance filtering function to process the transmittance data in depth order
 */
struct SqHitHeapNode
{
	/** Constructor: initialize an SqHitHeapNode
	 */ 
	SqHitHeapNode(const SqSampleData* sp, TqInt qI, CqColor rV, TqFloat w):
		samplePointer( sp ),
		queueIndex( qI ),
		runningVisibility( rV ),
		weight( w )
	{}
	
	/** Compare nodes for sorting in a priority queue
	 * \param nodeComp the node compare against
	 * \return true if this node is less (closer) than nodeComp
	 */	
	bool operator<( const SqHitHeapNode& nodeComp ) const;
	
	/// a pointer to a subpixel sample
	const SqSampleData* samplePointer;
	/// the index inside of samplePointer->m_Data we should use for constructing the next visibility node
	/// == -1 if we should use the SqSampleData::m_OpaqueEntry instead
	TqInt queueIndex;
	/// running total visibility at queueIndex of sample data
	CqColor runningVisibility;
	/// sample weight (0 <= weigh <= 1)
	TqFloat weight;  
};

typedef std::priority_queue<SqHitHeapNode, std::vector<SqHitHeapNode> > TqHitHeap;

/** \brief Class encapsulates operations on an SqHitHeapNode queue for visibility calculations.
* This class should handle micromanagement of the heap, such as adding new nodes (if required)
* when nodes are removed. This is a priority queue that sorts nodes based on depth, such that the
* deepest nodes are processed last.
*/
class CqHitHeap
{
	public:
		/** \brief Constructor
		*/
		CqHitHeap();

		/** \brief The user should call this method once per visibility function to enqueue the first
		* node from each visibility function on the heap. No extra identifier is needed to differentiate 
		* between them, since they are differentiated implicitly by the sub-sample data arrays they point to.
		*
		*/
		void push(SqHitHeapNode node);
		
		/** \brief Take and return the top node off the heap. Check if another node
		*  should be added, and if so, add it.
		*
		* \return A reference to the hit heap node popped from the heap.
		*/
		SqHitHeapNode pop();
		
		/** \brief Check if the heap is empty of nodes
		*
		* \return True if there are no nodes in the queue, false otherwise.
		*/		
		bool isEmpty() const;

	private:
	
		// Functions
	
		// Data
		std::priority_queue<SqHitHeapNode, std::vector<SqHitHeapNode> > m_heap;	
};

//-----------------------------------------------------------------------
/** Class holding data about a particular bucket.
 */

class CqBucket : public IqBucket
{
	public:
		CqBucket() : m_bProcessed( false ),
					 m_bEmpty( true )
		{}
		CqBucket( const CqBucket& From )
		{
			*this = From;
		}
		virtual ~CqBucket();

		CqBucket& operator=( const CqBucket& From )
		{
			m_ampgWaiting = From.m_ampgWaiting;
			m_agridWaiting = From.m_agridWaiting;
			m_bProcessed = From.m_bProcessed;
			m_bEmpty = From.m_bEmpty;

			return ( *this );
		}

		// Overridden from IqBucket
		virtual	TqInt	Width() const
		{
			return ( m_XSize );
		}
		virtual	TqInt	Height() const
		{
			return ( m_YSize );
		}
		virtual	TqInt	RealWidth() const
		{
			return ( m_RealWidth );
		}
		virtual	TqInt	RealHeight() const
		{
			return ( m_RealHeight );
		}
		virtual	TqInt	XOrigin() const
		{
			return ( m_XOrigin );
		}
		virtual	TqInt	YOrigin() const
		{
			return ( m_YOrigin );
		}
		static	TqInt	PixelXSamples()
		{
			return m_PixelXSamples;
		}
		static	TqInt	PixelYSamples()
		{
			return m_PixelYSamples;
		}
		static	TqFloat	FilterXWidth()
		{
			return m_FilterXWidth;
		}
		static	TqFloat	FilterYWidth()
		{
			return m_FilterYWidth;
		}
		static	TqInt	NumTimeRanges()
		{
			return m_NumTimeRanges;
		}
		static	TqInt	NumDofBounds()
		{
			return m_NumDofBounds;
		}
		static const CqBound& DofSubBound(TqInt index)
		{
			assert(index < m_NumDofBounds);
			return m_DofBounds[index];
		}

		virtual	CqColor Color( TqInt iXPos, TqInt iYPos );
		virtual	CqColor Opacity( TqInt iXPos, TqInt iYPos );
		virtual	TqFloat Coverage( TqInt iXPos, TqInt iYPos );
		virtual	TqFloat Depth( TqInt iXPos, TqInt iYPos );
		virtual	const TqFloat* Data( TqInt iXPos, TqInt iYPos );

		static	void	PrepareBucket( TqInt xorigin, TqInt yorigin, TqInt xsize, TqInt ysize, bool fJitter = true, bool empty = false );
		static	void	CalculateDofBounds();
		static	void	InitialiseFilterValues();
		static	void	ImageElement( TqInt iXPos, TqInt iYPos, CqImagePixel*& pie )
		{
			iXPos -= m_XOrigin;
			iYPos -= m_YOrigin;

			// Check within renderable range
			//assert( iXPos < -m_XMax && iXPos < m_XSize + m_XMax &&
			//		iYPos < -m_YMax && iYPos < m_YSize + m_YMax );

			TqInt i = ( ( iYPos + m_DiscreteShiftY ) * ( m_RealWidth ) ) + ( iXPos + m_DiscreteShiftX );
			pie = &m_aieImage[ i ];
		}
		static CqImagePixel& ImageElement(TqInt index)
		{
			assert(index < m_aieImage.size());
			return(m_aieImage[index]);
		}

		static	std::vector<SqSampleData>& SamplePoints()
		{
			return(m_SamplePoints);
		}

		static TqInt GetNextSamplePointIndex()
		{
			TqInt index = m_NextSamplePoint;
			m_NextSamplePoint++;
			return(index);
		}

		static	void	CombineElements(enum EqFilterDepth eDepthFilter, CqColor zThreshold);
		void	FilterBucket(bool empty);
		void	ExposeBucket();
		void	QuantizeBucket();
		static	void	ShutdownBucket();
		
		/** \brief Filter and produce visibility functions for all pixels in the bucket, if it is not empty.
		 * If it is empty, indicate so by storing a -1 at the front of the function length data for the bucket.
		 * 
		*/
		void FilterTransmittance(bool empty);
		
		/** \brief Filter and generate visibility function for a single pixel.
		 * 
		*/
		void CalculateVisibility(const TqFloat xcent, const TqFloat ycent, CqImagePixel* pie);
		
		/** \brief Using a non-separable filter (good when image filters are smaller than 16 pixels in width) filter the deep data
		 * to produce a heap containing the first (most shallow) hit from each sum-sample in the filter.
		 * 
		 * \return A float, the inverse of the normalized filter weight (sum of all filter weights) 
		*/
		TqFloat filterNonSeparable( const TqFloat xcent, const TqFloat ycent, CqImagePixel* pixel, TqHitHeap& nextHitHeap );
		
		/** \brief Get a const pointer to the visibility function at the rquested bucket space pixel coordinates (hpos, vpos)
		 * 
		*/
		void ReconstructVisibilityNode( const SqDeltaNode& deltaNode, CqColor& slopeAtJ, boost::shared_ptr<TqVisibilityFunction> currentVisFunc );
		
		/** \brief Get a const pointer to the visibility function at the rquested bucket space pixel coordinates (hpos, vpos)
		 * 
		*/
		virtual	const TqVisibilityFunction* DeepData( TqInt hpos, TqInt vpos ) const;
		
		// Assert that the heap is sorted (DEBUGGING)
		void CheckHeapSorted(std::priority_queue< SqHitHeapNode, std::vector<SqHitHeapNode> > nexthitheap);
		
		// Print to standard output the nodes in a given visibility function (DEBUGGING)
		void CheckVisibilityFunction(TqInt index) const;
		
		/** \brief Get the size of the visibility data for this bucket
		 *
		 * Note: this function is currently not used
		 * 
		 * \return The total number of TqFloats used in representing all
		 * visibility functions for this bucket
		 */
		inline TqInt VisibilityDataTotalSize() const;

		/** Add a GPRim to the stack of deferred GPrims.
		* \param The Gprim to be added.
		 */
		void	AddGPrim( const boost::shared_ptr<CqSurface>& pGPrim )
		{
			m_aGPrims.push(pGPrim);
		}

		/** Add an MPG to the list of deferred MPGs.
		 */
		void	AddMPG( CqMicroPolygon* pmpgNew )
		{
#ifdef _DEBUG
			std::vector<CqMicroPolygon*>::iterator end = m_ampgWaiting.end();
			for (std::vector<CqMicroPolygon*>::iterator i = m_ampgWaiting.begin(); i != end; i++)
				if ((*i) == pmpgNew)
					assert( false );
#endif

			m_ampgWaiting.push_back( pmpgNew );
		}
		/** Add a Micropoly grid to the list of deferred grids.
		 */
		void	AddGrid( CqMicroPolyGridBase* pgridNew )
		{
#ifdef _DEBUG
			std::vector<CqMicroPolyGridBase*>::iterator end = m_agridWaiting.end();
			for (std::vector<CqMicroPolyGridBase*>::iterator i = m_agridWaiting.begin(); i != end; i++)
				if ((*i) == pgridNew)
					assert( false );
#endif

			m_agridWaiting.push_back( pgridNew );
		}
		/** Get a pointer to the top GPrim in the stack of deferred GPrims.
		 */
		boost::shared_ptr<CqSurface> pTopSurface()
		{
			if (!m_aGPrims.empty())
			{
				return m_aGPrims.top();
			}
			else
			{
				return boost::shared_ptr<CqSurface>();
			}
		}
		/** Pop the top GPrim in the stack of deferred GPrims.
		 */
		void popSurface()
		{
			m_aGPrims.pop();
		}
		/** Get a count of deferred GPrims.
		 */
		TqInt cGPrims()
		{
			return ( m_aGPrims.size() );
		}
		/** Get a reference to the vector of deferred MPGs.
		 */
		std::vector<CqMicroPolygon*>& aMPGs()
		{
			return ( m_ampgWaiting );
		}
		/** Get a reference to the vector of deferred grids.
		 */
		std::vector<CqMicroPolyGridBase*>& aGrids()
		{
			return ( m_agridWaiting );
		}
		/** Get the flag that indicates if the bucket has been processed yet.
		 */
		bool IsProcessed() const
		{
			return( m_bProcessed );
		}
		/** Get the flag that indicates if the bucket's visibility (deep) data is empty
		 */
		bool IsDeepEmpty() const
		{
			return( m_bEmpty );
		}		

		/** Mark this bucket as processed
		 */
		void SetProcessed( bool bProc =  true)
		{
			m_bProcessed = bProc;
		}
		/** Set the pointer to the image buffer
		 */
		static void SetImageBuffer( CqImageBuffer* pBuffer )
		{
			m_ImageBuffer = pBuffer;
		}


	private:
		static	TqInt	m_XOrigin;		///< Origin in discrete coordinates of this bucket.
		static	TqInt	m_YOrigin;		///< Origin in discrete coordinates of this bucket.
		static	TqInt	m_XSize;		///< Size of the rendered area of this bucket in discrete coordinates.
		static	TqInt	m_YSize;		///< Size of the rendered area of this bucket in discrete coordinates.
		static	TqInt	m_RealWidth;	///< Actual size of the data for this bucket including filter overlap.
		static	TqInt	m_RealHeight;	///< Actual size of the data for this bucket including filter overlap.
		static	TqInt	m_DiscreteShiftX;	///<
		static	TqInt	m_DiscreteShiftY;
		static	TqInt	m_PixelXSamples;
		static	TqInt	m_PixelYSamples;
		static	TqFloat	m_FilterXWidth;
		static	TqFloat	m_FilterYWidth;
		static	TqInt	m_NumTimeRanges;
		static	TqInt	m_NumDofBounds;
		static	std::vector<CqBound>		m_DofBounds;
		static	std::vector<CqImagePixel>	m_aieImage;
		static	std::vector<SqSampleData>	m_SamplePoints; ///< The micropolygon sample data relevent to this bucket
		static	TqInt	m_NextSamplePoint;
		static	std::vector<std::vector<CqVector2D> >	m_aSamplePositions;///< Vector of vectors of jittered sample positions precalculated.
		static	std::vector<TqFloat>	m_aFilterValues;				///< Vector of filter weights precalculated.
		static	std::vector<TqFloat>	m_aDatas;
		static	std::vector<TqFloat>	m_aCoverages;
		static	CqImageBuffer*	m_ImageBuffer;	///< Pointer to the image buffer this bucket belongs to.

		// this is a compare functor for sorting surfaces in order of depth.
		struct closest_surface
		{
			bool operator()(const boost::shared_ptr<CqSurface>& s1, const boost::shared_ptr<CqSurface>& s2) const
			{
				if ( s1->fCachedBound() && s2->fCachedBound() )
				{
					return ( s1->GetCachedRasterBound().vecMin().z() > s2->GetCachedRasterBound().vecMin().z() );
				}

				// don't have bounds for the surface(s). I suspect we should assert here.
				return true;
			}
		};
	
		TqInt m_VisibilityDataSize; ///< Total number of TqFloats stored in the visibility functions: 2 per-node if grayscale, 4 per-node if color 
		std::vector< boost::shared_ptr<TqVisibilityFunction> >	m_VisibilityFunctions; ///< The Visibility functions (one per pixel),
															//stored row after row, in this bucket; only used when rendering a DSM
		std::vector<CqMicroPolygon*> m_ampgWaiting;			///< Vector of vectors of waiting micropolygons in this bucket
		std::vector<CqMicroPolyGridBase*> m_agridWaiting;		///< Vector of vectors of waiting micropolygrids in this bucket

		/// A sorted list of primitives for this bucket
		std::priority_queue<boost::shared_ptr<CqSurface>, std::deque<boost::shared_ptr<CqSurface> >, closest_surface> m_aGPrims;
		bool	m_bProcessed;	///< Flag indicating if this bucket has been processed yet.
		bool	m_bEmpty; ///< Flag indicating if this bucket's visibility data is empty.
}
;

//------------------------------------------------------------------------------
// Inline function(s) for CqBucket
//------------------------------------------------------------------------------
inline TqInt CqBucket::VisibilityDataTotalSize() const
{
	return m_VisibilityDataSize;
}

END_NAMESPACE( Aqsis )

//}  // End of #ifdef BUCKET_H_INCLUDED
#endif
