// Aqsis
// Copyright ���� 1997 - 2001, Paul C. Gregory
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
		\brief Declares the CqImagePixel class responsible for storing the results.
		\author Paul C. Gregory (pgregory@aqsis.org)
*/

//? Is imagebuffer.h included already?
#ifndef IMAGEPIXEL_H_INCLUDED
//{
#define IMAGEPIXEL_H_INCLUDED 1

#include	"aqsis.h"

#include	<vector>
#include	<stack>
#include	<deque>
#include	<valarray>

#include	"csgtree.h"
#include	"color.h"
#include	"vector2d.h"

namespace Aqsis {

class CqBucketProcessor;

//-----------------------------------------------------------------------
/** Structure representing the information at a sample point in the image.
 */

enum EqSampleIndices
{
    Sample_Red = 0,
    Sample_Green = 1,
    Sample_Blue = 2,
    Sample_ORed = 3,
    Sample_OGreen = 4,
    Sample_OBlue = 5,
    Sample_Depth = 6,
    Sample_Coverage = 7,
    Sample_Alpha = 8,
};

enum EqFilterDepth
{
    Filter_Min = 0,
    Filter_MidPoint = 1,
    Filter_Max = 2,
    Filter_Average = 3,
};


class CqSampleDataPool
{
	public:
		CqSampleDataPool()	: m_theDataPool(10000), m_nextSlot(0), m_slotSize(9)
		{}
		~CqSampleDataPool()
		{
			// \todo except if there are still blocks allocated.
		}

		void	Initialise(TqInt slotSize)
		{
			// \todo except if there are slots that haven't been freed.
			m_slotSize = slotSize;
			m_nextSlot = 0;
			while(!m_freeSlots.empty())
				m_freeSlots.pop();
		}

		TqInt	Allocate()
		{
			// If there are slots in the deallocated stack, use one of those.
			if(!m_freeSlots.empty())
			{
				TqInt slot = m_freeSlots.top();
				m_freeSlots.pop();
				return(slot);
			}
			else
			{
				// If the pool isn't big enough, resize it.
				if((m_nextSlot + m_slotSize)>m_theDataPool.size())
					m_theDataPool.resize(m_theDataPool.size()*2);
				TqInt slot = m_nextSlot;
				m_nextSlot += m_slotSize;
				return(slot);
			}
		}

		void	DeAllocate(TqInt index)
		{
			m_freeSlots.push(index);
		}

		TqFloat*	SampleDataSlot(TqInt slot)
		{
			assert((slot+m_slotSize) < m_theDataPool.size());
			return(&m_theDataPool[slot]);
		}

		TqInt	slotSize()
		{
			return(m_slotSize);
		}

	private:
		std::vector<TqFloat>	m_theDataPool;
		TqUint			m_nextSlot;
		TqUint			m_slotSize;
		std::stack<TqInt>	m_freeSlots;
};


struct SqImageSample
{
	SqImageSample() : m_flags(0)
	{
		/// \todo Previous code for the sample pool
		/// m_sampleSlot = m_theSamplePool.Allocate();
		m_data = new TqFloat[m_sampleSize];
	}

	/// Copy constructor
	SqImageSample(const SqImageSample& from)
	{
		/// \todo Previous code for the sample pool
		/// m_sampleSlot = m_theSamplePool.Allocate();
		m_data = new TqFloat[m_sampleSize];
		*this = from;
	}

	~SqImageSample()
	{
		/// \todo Previous code for the sample pool
		/// m_theSamplePool.DeAllocate(m_sampleSlot);
		delete [] m_data;
	}

	SqImageSample& operator=(const SqImageSample& from)
	{
		m_flags = from.m_flags;
		m_pCSGNode = from.m_pCSGNode;

		const TqFloat* fromData = from.Data();
		TqFloat* toData = Data();
		/// \todo Previous code for the sample pool
		/// for(TqInt i=0; i<m_theSamplePool.slotSize(); ++i)
		for(TqUint i=0; i<m_sampleSize; ++i)
			toData[i] = fromData[i];

		return(*this);
	}

	static void SetSampleSize(TqUint size)
	{
		/// \todo Previous code for the sample pool
		/// m_theSamplePool.Initialise(size);
		m_sampleSize = size;
	}

	TqFloat* Data()
	{
		/// \todo Previous code for the sample pool
		/// return(m_theSamplePool.SampleDataSlot(m_sampleSlot));
		return m_data;
	}
	const TqFloat* Data() const
	{
		/// \todo Previous code for the sample pool
		/// return(m_theSamplePool.SampleDataSlot(m_sampleSlot));
		return m_data;
	}

	void resetFlags()
	{
		m_flags = 0;
	}
	bool isValid() const
	{
		return (m_flags & Flag_Valid) != 0;
	}
	void setValid()
	{
		m_flags |= Flag_Valid;
	}
	bool isMatte() const
	{
		return (m_flags & Flag_Matte) != 0;
	}
	void setMatte()
	{
		m_flags |= Flag_Matte;
	}
	bool isOccludes() const
	{
		return (m_flags & Flag_Occludes) != 0;
	}
	void setOccludes()
	{
		m_flags |= Flag_Occludes;
	}

	boost::shared_ptr<CqCSGTreeNode>	m_pCSGNode;	///< Pointer to the CSG node this sample is part of, NULL if not part of a solid.

private:
	enum {
	    Flag_Occludes = 0x0001,
	    Flag_Matte = 0x0002,
	    Flag_Valid = 0x0004
	};

	TqInt	m_flags;
	TqInt	m_sampleSlot;

	/// \todo Previous code for the sample pool
	/// static  CqSampleDataPool m_theSamplePool;
	static TqUint	m_sampleSize;
	TqFloat*	m_data;

}
;


/** Structure to hold the info about a sample point.
 */
struct SqSampleData
{
	CqVector2D	m_Position;				///< Sample position
	CqVector2D	m_DofOffset;			///< Dof lens offset.
	TqInt		m_DofOffsetIndex;
	TqInt		m_SubCellIndex;		///< Subcell index.
	TqFloat		m_Time;				///< Float sample time.
	TqFloat		m_DetailLevel;		///< Float level-of-detail sample.
	std::deque<SqImageSample>	m_Data;	///< Array of sampled surface data for this sample.
	SqImageSample m_OpaqueSample;	///< Single opaque sample for optimised processing if all encountered surfaces are opaque
};

//-----------------------------------------------------------------------
/** Storage class for all data relating to a single pixel in the image.
 */

class CqImagePixel
{
	public:
		CqImagePixel();
		CqImagePixel( const CqImagePixel& ieFrom );
		virtual	~CqImagePixel();

		/** Get the number of horizontal samples in this pixel
		 * \return The number of samples as an integer.
		 */
		TqInt	XSamples() const
		{
			return ( m_XSamples );
		}
		/** Get the number of vertical samples in this pixel
		 * \return The number of samples as an integer.
		 */
		TqInt	YSamples() const
		{
			return ( m_YSamples );
		}
		void	AllocateSamples( CqBucketProcessor* bp, TqInt XSamples, TqInt YSamples );
		void	InitialiseSamples( std::vector<SqSampleData>& samplePoints, std::vector<CqVector2D>& vecSamples );
		void	JitterSamples( std::vector<SqSampleData>& samplePoints, std::vector<CqVector2D>& vecSamples, TqFloat opentime, TqFloat closetime );
		void	OffsetSamples( std::vector<SqSampleData>& samplePoints, CqVector2D& vecPixel, std::vector<CqVector2D>& vecSamples );

		/** Get the approximate coverage of this pixel.
		 * \return Float fraction of the pixel covered.
		 */
		TqFloat	Coverage() const
		{
			return ( m_Data.Data()[Sample_Coverage] );
		}
		void	SetCoverage( TqFloat c )
		{
			m_Data.Data()[Sample_Coverage] = c;
		}
		/** Get the averaged color of this pixel
		 * \return A color representing the averaged color at this pixel.
		 * \attention Only call this after already calling FilterBucket().
		 */
		CqColor	Color() const
		{
			const TqFloat* data = m_Data.Data();
			return ( CqColor(data[Sample_Red], data[Sample_Green], data[Sample_Blue]) );
		}
		void	SetColor(const CqColor& col)
		{
			TqFloat* data = m_Data.Data();
			data[Sample_Red] = col.r();
			data[Sample_Green] = col.g();
			data[Sample_Blue] = col.b();
		}
		/** Get the averaged opacity of this pixel
		 * \return A color representing the averaged opacity at this pixel.
		 * \attention Only call this after already calling FilterBucket().
		 */
		CqColor	Opacity() const
		{
			const TqFloat* data = m_Data.Data();
			return ( CqColor(data[Sample_ORed], data[Sample_OGreen], data[Sample_OBlue]) );
		}
		void	SetOpacity(const CqColor& col)
		{
			TqFloat* data = m_Data.Data();
			data[Sample_ORed] = col.r();
			data[Sample_OGreen] = col.g();
			data[Sample_OBlue] = col.b();
		}
		/** Get the averaged depth of this pixel
		 * \return A float representing the averaged depth at this pixel.
		 * \attention Only call this after already calling FilterBucket().
		 */
		TqFloat	Depth() const
		{
			return ( m_Data.Data()[Sample_Depth] );
		}
		void	SetDepth( TqFloat d )
		{
			m_Data.Data()[Sample_Depth] = d;
		}
		/** Get the premultiplied alpha of this pixel
		 * \return A float representing the premultiplied alpha value of this pixel.
		 * \attention Only call this after already calling FilterBucket().
		 */
		TqFloat	Alpha() const
		{
			return ( m_Data.Data()[Sample_Alpha] );
		}
		void	SetAlpha( TqFloat a )
		{
			m_Data.Data()[Sample_Alpha] = a;
		}
		/** Get a pointer to the sample data
		 * \return A constant pointer to the sample data.
		 */
		const TqFloat*	Data()
		{
			return ( &m_Data.Data()[0] );
		}
		SqImageSample&	GetPixelSample()
		{
			return ( m_Data );
		}
		/** Get a count of data
		 * \return A count of the samples on this pixel.
		 *
		TqInt	DataSize()
		{
		    return ( m_Data.Data().size() );
		}*/

		/** Clear all sample information from this pixel.
		 */
		void	Clear( std::vector<SqSampleData>& samplePoints );
		/** Get a reference to the array of values for the specified sample.
		 * \param m The horizontal index of the required sample point.
		 * \param n The vertical index of the required sample point.
		 * \return A Reference to a vector of SqImageSample data.
		 */
		std::deque<SqImageSample>&	Values( std::vector<SqSampleData>& samplePoints, TqInt index );

		SqImageSample& OpaqueValues( std::vector<SqSampleData>& samplePoints, TqInt index );

		void	Combine( std::vector<SqSampleData>& samplePoints, EqFilterDepth eDepthFilter, CqColor zThreshold );

		/** Get the sample data for the specified sample index.
		 * \param The index of the required sample point.
		 * \return A reference to the sample data.
		 */
		const SqSampleData& SampleData( std::vector<SqSampleData>& samplePoints, TqInt index ) const;

		/** Get the sample data for the specified sample index.
		 * \param The index of the required sample point.
		 * \return A reference to the sample data.
		 */
		SqSampleData& SampleData( std::vector<SqSampleData>& samplePoints, TqInt index );

		/** Get the index of the sample that contains a dof offset that lies
		 *  in bounding-box number i.
		 * \param The index of the bounding box in question.
		 * \return The index of the sample that contains a dof offset in said bb.
		 */
		TqInt GetDofOffsetIndex(TqInt i) const
		{
			return m_DofOffsetIndices[i];
		}

		/** Convert a coord in the unit square to one inside the unit circle.
		 *  used in generating dof sample positions.
		 */
		static void ProjectToCircle(CqVector2D& pos)
		{
			TqFloat r = pos.Magnitude();
			if( r == 0.0 )
				return;

			TqFloat adj = max(fabs(pos.x()), fabs(pos.y())) / r;
			pos.x(pos.x() * adj);
			pos.y(pos.y() * adj);
		}

		/* These are public to allow direct shuffling */
		std::vector<TqInt> m_SampleIndices;
		std::vector<TqInt> m_DofOffsetIndices;	///< A mapping from dof bounding-box index to the sample that contains a dof offset in that bb.
	private:
		TqInt	m_XSamples;						///< The number of samples in the horizontal direction.
		TqInt	m_YSamples;						///< The number of samples in the vertical direction.
		//std::vector<SqSampleData> m_Samples;	///< A Vector of samples. Holds position, time, dof offset etc for each sample.
		SqImageSample	m_Data;
}
;




//-----------------------------------------------------------------------

} // namespace Aqsis

//}  // End of #ifdef IMAGEPIXEL_H_INCLUDED
#endif
