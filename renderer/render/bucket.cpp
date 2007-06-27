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
		\brief Implements the CqBucket class responsible for bookkeeping the primitives and storing the results.
		\author Paul C. Gregory (pgregory@aqsis.org)
*/

#include	"MultiTimer.h"

#include	"aqsis.h"

#ifdef WIN32
#include    <windows.h>
#endif
#include	<math.h>

#include	"surface.h"
#include	"imagepixel.h"
#include	"bucket.h"

#include	"imagers.h"

#include	<algorithm>
#include	<valarray>


START_NAMESPACE( Aqsis )

//----------------------------------------------------------------------
/** Static data on CqBucket
 */

TqInt	CqBucket::m_XSize;
TqInt	CqBucket::m_YSize;
TqInt	CqBucket::m_RealWidth;
TqInt	CqBucket::m_RealHeight;
TqInt	CqBucket::m_DiscreteShiftX;
TqInt	CqBucket::m_DiscreteShiftY;
TqInt	CqBucket::m_XOrigin;
TqInt	CqBucket::m_YOrigin;
TqInt	CqBucket::m_PixelXSamples;
TqInt	CqBucket::m_PixelYSamples;
TqFloat	CqBucket::m_FilterXWidth;
TqFloat	CqBucket::m_FilterYWidth;
TqInt	CqBucket::m_NumTimeRanges;
TqInt	CqBucket::m_NumDofBounds;
CqImageBuffer* CqBucket::m_ImageBuffer;
std::vector<CqBound>		CqBucket::m_DofBounds;
std::vector<CqImagePixel>	CqBucket::m_aieImage;
std::vector<SqSampleData>	CqBucket::m_SamplePoints;
TqInt	CqBucket::m_NextSamplePoint = 0;
std::vector<std::vector<CqVector2D> >	CqBucket::m_aSamplePositions;
std::vector<TqFloat> CqBucket::m_aFilterValues;
std::vector<TqFloat> CqBucket::m_aDatas;
std::vector<TqFloat> CqBucket::m_aCoverages;

/** Compare nodes for sorting in a priority queue
 * \param nodeComp the node compare against
 * \return true if this node is less (closer) than nodeComp
 */	
bool SqHitHeapNode::operator<( const SqHitHeapNode& nodeComp ) const
{	
	TqFloat s1depth, s2depth;
	if ( this->queueIndex == -1 )
		s1depth = this->samplepointer->m_OpaqueSample.Data()[Sample_Depth];
	else
		s1depth = this->samplepointer->m_Data[this->queueIndex].Data()[Sample_Depth];	
	if ( nodeComp.queueIndex == -1 )
		s2depth = nodeComp.samplepointer->m_OpaqueSample.Data()[Sample_Depth];
	else
		s2depth = nodeComp.samplepointer->m_Data[nodeComp.queueIndex].Data()[Sample_Depth];						
	return ( s1depth > s2depth ); // It works as desired with the ">" but logically this should be "<"
}

//----------------------------------------------------------------------
/** Initialise the static image storage area.
 *  Clear,Allocate, Init. the m_aieImage samples
 */

void CqBucket::PrepareBucket( TqInt xorigin, TqInt yorigin, TqInt xsize, TqInt ysize, bool fJitter, bool empty )
{
	m_XOrigin = xorigin;
	m_YOrigin = yorigin;
	m_XSize = xsize;
	m_YSize = ysize;
	m_PixelXSamples = m_ImageBuffer->PixelXSamples();
	m_PixelYSamples = m_ImageBuffer->PixelYSamples();
	m_FilterXWidth = m_ImageBuffer->FilterXWidth();
	m_FilterYWidth = m_ImageBuffer->FilterYWidth();
	m_DiscreteShiftX = FLOOR(m_FilterXWidth/2.0f);
	m_DiscreteShiftY = FLOOR(m_FilterYWidth/2.0f);
	m_RealWidth = m_XSize + (m_DiscreteShiftX*2);
	m_RealHeight = m_YSize + (m_DiscreteShiftY*2);

	m_NumTimeRanges = MAX(4, m_PixelXSamples * m_PixelYSamples);

        TqFloat opentime = QGetRenderContext() ->poptCurrent()->GetFloatOption( "System", "Shutter" ) [ 0 ];
        TqFloat closetime = QGetRenderContext() ->poptCurrent()->GetFloatOption( "System", "Shutter" ) [ 1 ];

	// Allocate the image element storage if this is the first bucket
	if(m_aieImage.empty())
	{
		SqImageSample::SetSampleSize(QGetRenderContext() ->GetOutputDataTotalSize());

		m_aieImage.resize( m_RealWidth * m_RealHeight );
		m_aSamplePositions.resize( m_RealWidth * m_RealHeight );
		m_SamplePoints.resize( m_RealWidth * m_RealHeight * m_PixelXSamples * m_PixelYSamples );
		m_NextSamplePoint = 0;

		CalculateDofBounds();

		// Initialise the samples for this bucket.
		TqInt which = 0;
		for ( TqInt i = 0; i < m_RealHeight; i++ )
		{
			for ( TqInt j = 0; j < m_RealWidth; j++ )
			{
				m_aieImage[which].Clear();
				m_aieImage[which].AllocateSamples( m_PixelXSamples, m_PixelYSamples );
				m_aieImage[which].InitialiseSamples( m_aSamplePositions[which] );
				//if(fJitter)
					m_aieImage[which].JitterSamples(m_aSamplePositions[which], opentime, closetime);

				which++;
			}
		}
	}

	// Shuffle the Sample and DOD positions 
	std::vector<CqImagePixel>::iterator itPix;
	TqUint size = m_aieImage.size();  
	TqUint i = 0;
	if (size > 1)
	{
		CqRandom rand(19);
		for( itPix = m_aieImage.begin(), i=0 ; itPix <= m_aieImage.end(), i < size - 1; itPix++, i++)
		{
			TqUint other = i + rand.RandomInt(size - i);
			if (other >= size) other = size - 1;
			(*itPix).m_SampleIndices.swap(m_aieImage[other].m_SampleIndices);  
			(*itPix).m_DofOffsetIndices.swap(m_aieImage[other].m_DofOffsetIndices); 
		};
	};

	// Jitter the samplepoints and adjust them for the new bucket position.
	TqInt which = 0;
	//TqInt numPixels = m_RealWidth*m_RealHeight;
	for ( TqInt ii = 0; ii < m_RealHeight; ii++ )
	{
		for ( TqInt j = 0; j < m_RealWidth; j++ )
		{
			CqVector2D bPos2( m_XOrigin, m_YOrigin );
			bPos2 += CqVector2D( ( j - m_DiscreteShiftX ), ( ii - m_DiscreteShiftY ) );

			if(!empty)
				m_aieImage[which].Clear();

			//if(fJitter)
				m_aieImage[which].JitterSamples(m_aSamplePositions[which], opentime, closetime);
			m_aieImage[which].OffsetSamples( bPos2, m_aSamplePositions[which] );

			which++;
		}
	}
}


void CqBucket::CalculateDofBounds()
{
	m_NumDofBounds = m_PixelXSamples * m_PixelYSamples;
	m_DofBounds.resize(m_NumDofBounds);

	TqFloat dx = 2.0 / m_PixelXSamples;
	TqFloat dy = 2.0 / m_PixelYSamples;

	// I know this is far from an optimal way of calculating this,
	// but it's only done once so I don't care.
	// Calculate the bounding boxes that the dof offset positions fall into.
	TqFloat minX = -1.0;
	TqFloat minY = -1.0;
	TqInt which = 0;
	for(int j = 0; j < m_PixelYSamples; ++j)
	{
		for(int i = 0; i < m_PixelXSamples; ++i)
		{
			CqVector2D topLeft(minX, minY);
			CqVector2D topRight(minX + dx, minY);
			CqVector2D bottomLeft(minX, minY + dy);
			CqVector2D bottomRight(minX + dx, minY + dy);

			CqImagePixel::ProjectToCircle(topLeft);
			CqImagePixel::ProjectToCircle(topRight);
			CqImagePixel::ProjectToCircle(bottomLeft);
			CqImagePixel::ProjectToCircle(bottomRight);

			// if the bound straddles x=0 or y=0 then just using the corners
			// will give too small a bound, so we enlarge it by including the
			// non-projected coords.
			if((topLeft.y() > 0.0 && bottomLeft.y() < 0.0) ||
			        (topLeft.y() < 0.0 && bottomLeft.y() > 0.0))
			{
				topLeft.x(minX);
				bottomLeft.x(minX);
				topRight.x(minX + dx);
				bottomRight.x(minX + dx);
			}
			if((topLeft.x() > 0.0 && topRight.x() < 0.0) ||
			        (topLeft.x() < 0.0 && topRight.x() > 0.0))
			{
				topLeft.y(minY);
				bottomLeft.y(minY + dy);
				topRight.y(minY);
				bottomRight.y(minY + dy);
			}

			m_DofBounds[which].vecMin() = topLeft;
			m_DofBounds[which].vecMax() = topLeft;
			m_DofBounds[which].Encapsulate(topRight);
			m_DofBounds[which].Encapsulate(bottomLeft);
			m_DofBounds[which].Encapsulate(bottomRight);

			which++;
			minX += dx;
		}
		minX = -1.0;
		minY += dy;
	}
}

//----------------------------------------------------------------------
/** Initialise the static filter values.
 */

void CqBucket::InitialiseFilterValues()
{
	if( !m_aFilterValues.empty() )
		return;

	// Allocate and fill in the filter values array for each pixel.
	TqInt numsubpixels = ( PixelXSamples() * PixelYSamples() );
	TqInt numperpixel = numsubpixels * numsubpixels;

	TqUint numvalues = static_cast<TqUint>( ( ( CEIL(FilterXWidth()) + 1 ) * ( CEIL(FilterYWidth()) + 1 ) ) * ( numperpixel ) );

	m_aFilterValues.resize( numvalues );

	RtFilterFunc pFilter;
	pFilter = QGetRenderContext() ->poptCurrent()->funcFilter();

	// Sanity check
	if( NULL == pFilter )
		pFilter = RiBoxFilter;

	TqFloat xmax = m_DiscreteShiftX;
	TqFloat ymax = m_DiscreteShiftY;
	TqFloat xfwo2 = CEIL(FilterXWidth()) * 0.5f;
	TqFloat yfwo2 = CEIL(FilterYWidth()) * 0.5f;
	TqFloat xfw = CEIL(FilterXWidth());

	TqFloat subcellwidth = 1.0f / numsubpixels;
	TqFloat subcellcentre = subcellwidth * 0.5f;

	// Go over every pixel touched by the filter
	TqInt px, py;
	for ( py = static_cast<TqInt>( -ymax ); py <= static_cast<TqInt>( ymax ); py++ )
	{
		for( px = static_cast<TqInt>( -xmax ); px <= static_cast<TqInt>( xmax ); px++ )
		{
			// Get the index of the pixel in the array.
			TqInt index = static_cast<TqInt>
			              ( ( ( ( py + ymax ) * xfw ) + ( px + xmax ) ) * numperpixel );
			TqFloat pfx = px - 0.5f;
			TqFloat pfy = py - 0.5f;
			// Go over every subpixel in the pixel.
			TqInt sx, sy;
			for ( sy = 0; sy < PixelYSamples(); sy++ )
			{
				for ( sx = 0; sx < PixelXSamples(); sx++ )
				{
					// Get the index of the subpixel in the array
					TqInt sindex = index + ( ( ( sy * PixelXSamples() ) + sx ) * numsubpixels );
					TqFloat sfx = static_cast<TqFloat>( sx ) / PixelXSamples();
					TqFloat sfy = static_cast<TqFloat>( sy ) / PixelYSamples();
					// Go over each subcell in the subpixel
					TqInt cx, cy;
					for ( cy = 0; cy < PixelXSamples(); cy++ )
					{
						for ( cx = 0; cx < PixelYSamples(); cx++ )
						{
							// Get the index of the subpixel in the array
							TqInt cindex = sindex + ( ( cy * PixelYSamples() ) + cx );
							TqFloat fx = ( cx * subcellwidth ) + sfx + pfx + subcellcentre;
							TqFloat fy = ( cy * subcellwidth ) + sfy + pfy + subcellcentre;
							TqFloat w = 0.0f;
							if ( fx >= -xfwo2 && fy >= -yfwo2 && fx <= xfwo2 && fy <= yfwo2 )
								w = ( *pFilter ) ( fx, fy, CEIL(FilterXWidth()), CEIL(FilterYWidth()) );
							m_aFilterValues[ cindex ] = w;
						}
					}
				}
			}
		}
	}

}


//----------------------------------------------------------------------
/** Combine the subsamples into single pixel samples and coverage information.
 */

void CqBucket::CombineElements(enum EqFilterDepth filterdepth, CqColor zThreshold)
{
	//TqInt index = 0;
	std::vector<CqImagePixel>::iterator end = m_aieImage.end();
	for ( std::vector<CqImagePixel>::iterator i = m_aieImage.begin(); i != end ; i++ )
	{
		i->Combine(filterdepth, zThreshold);
	}
}


//----------------------------------------------------------------------
/** Get the sample color for the specified screen position.
 * If position is outside bucket, returns black.
 * \param iXPos Screen position of sample.
 * \param iYPos Screen position of sample.
 */

CqColor CqBucket::Color( TqInt iXPos, TqInt iYPos )
{
	CqImagePixel * pie;
	ImageElement( iXPos, iYPos, pie );
	if( NULL != pie )
		return ( pie->Color() );
	else
		return ( gColBlack);
}

//----------------------------------------------------------------------
/** Get the sample opacity for the specified screen position.
 * If position is outside bucket, returns black.
 * \param iXPos Screen position of sample.
 * \param iYPos Screen position of sample.
 */

CqColor CqBucket::Opacity( TqInt iXPos, TqInt iYPos )
{
	CqImagePixel * pie;
	ImageElement( iXPos, iYPos, pie );
	if( NULL != pie )
		return ( pie->Opacity() );
	else
		return ( gColBlack);
}


//----------------------------------------------------------------------
/** Get the sample coverage for the specified screen position.
 * If position is outside bucket, returns 0.
 * \param iXPos Screen position of sample.
 * \param iYPos Screen position of sample.
 */

TqFloat CqBucket::Coverage( TqInt iXPos, TqInt iYPos )
{
	CqImagePixel * pie;
	ImageElement( iXPos, iYPos, pie );
	if( NULL != pie )
		return ( pie->Coverage() );
	else
		return ( 0.0f );
}


//----------------------------------------------------------------------
/** Get the sample depth for the specified screen position.
 * If position is outside bucket, returns FLT_MAX.
 * \param iXPos Screen position of sample.
 * \param iYPos Screen position of sample.
 */

TqFloat CqBucket::Depth( TqInt iXPos, TqInt iYPos )
{
	CqImagePixel * pie;
	ImageElement( iXPos, iYPos, pie );
	if( NULL != pie )
		return ( pie->Depth() );
	else
		return ( FLT_MAX );
}


//----------------------------------------------------------------------
/** Get a pointer to the samples for a given pixel.
 * If position is outside bucket, returns NULL.
 * \param iXPos Screen position of sample.
 * \param iYPos Screen position of sample.
 */

const TqFloat* CqBucket::Data( TqInt iXPos, TqInt iYPos )
{
	CqImagePixel * pie;
	ImageElement( iXPos, iYPos, pie );
	if( NULL != pie )
		return ( pie->Data() );
	else
		return ( NULL );
}


//----------------------------------------------------------------------
/** Filter the samples in this bucket according to type and filter widths.
 */

void CqBucket::FilterBucket(bool empty)
{
	CqImagePixel * pie;

	TqInt datasize = QGetRenderContext()->GetOutputDataTotalSize();	
	m_aDatas.resize( datasize * RealWidth() * RealHeight() );
	m_aCoverages.resize( RealWidth() * RealHeight() );

	TqInt xmax = m_DiscreteShiftX;
	TqInt ymax = m_DiscreteShiftY;
	TqFloat xfwo2 = CEIL(FilterXWidth()) * 0.5f;
	TqFloat yfwo2 = CEIL(FilterYWidth()) * 0.5f;
	TqInt numsubpixels = ( PixelXSamples() * PixelYSamples() );

	TqInt numperpixel = numsubpixels * numsubpixels;
	TqInt	xlen = RealWidth();

	TqInt SampleCount = 0;
	CqColor imager;

	TqInt x, y;
	TqInt i = 0;

	bool fImager = false;
	const CqString* systemOptions;
	if( ( systemOptions = QGetRenderContext() ->poptCurrent()->GetStringOption( "System", "Imager" ) ) != 0 )
		if( systemOptions[ 0 ].compare("null") != 0 )
			fImager = true;

	TqInt endy = YOrigin() + Height();
	TqInt endx = XOrigin() + Width();

	bool useSeperable = true;
	
	//If rendering a DSM, invoke the transmittance filtering function
	if ( QGetRenderContext()->poptCurrent()->GetStringOption( "System", "DisplayType" )[0] == "dsm" )
	{
		FilterTransmittance(empty);
	}

	if(!empty)
	{
		// non-seperable is faster for very small filter widths.
		if(FilterXWidth() <= 16.0 || FilterYWidth() <= 16.0)
			useSeperable = false;

		if(useSeperable)
		{
			// seperable filter. filtering by fx,fy is equivalent to filtering
			// by fx,1 followed by 1,fy.
			TqInt size = Width() * RealHeight() * PixelYSamples();
			std::valarray<TqFloat> intermediateSamples( 0.0f, size * datasize);
			std::valarray<TqInt> sampleCounts(0, size);
			for ( y = YOrigin() - ymax; y < endy + ymax ; y++ )
			{
				TqFloat ycent = y + 0.5f;
				TqInt pixelRow = (y-(YOrigin()-ymax)) * PixelYSamples();
				for ( x = XOrigin(); x < endx ; x++ )
				{
					TqFloat xcent = x + 0.5f;

					// Get the element at the left side of the filter area.
					ImageElement( x - xmax, y, pie );

					TqInt pixelIndex = pixelRow*Width() + x-XOrigin();

					// filter just in x first
					for ( TqInt sy = 0; sy < PixelYSamples(); sy++ )
					{
						TqFloat gTot = 0.0;
						SampleCount = 0;
						std::valarray<TqFloat> samples( 0.0f, datasize);

						CqImagePixel* pie2 = pie;
						for ( TqInt fx = -xmax; fx <= xmax; fx++ )
						{
							TqInt index = ( ( ymax * CEIL(FilterXWidth()) ) + ( fx + xmax ) ) * numperpixel;
							// Now go over each subsample within the pixel
							TqInt sampleIndex = sy * PixelXSamples();
							TqInt sindex = index + ( sy * PixelXSamples() * numsubpixels );

							for ( TqInt sx = 0; sx < PixelXSamples(); sx++ )
							{
								const SqSampleData& sampleData = pie2->SampleData( sampleIndex );
								CqVector2D vecS = sampleData.m_Position;
								vecS -= CqVector2D( xcent, ycent );
								if ( vecS.x() >= -xfwo2 && vecS.y() >= -yfwo2 && vecS.x() <= xfwo2 && vecS.y() <= yfwo2 )
								{
									TqInt cindex = sindex + sampleData.m_SubCellIndex;
									TqFloat g = m_aFilterValues[ cindex ];
									gTot += g;
									if ( pie2->OpaqueValues( sampleIndex ).m_flags & SqImageSample::Flag_Valid )
									{
										SqImageSample& pSample = pie2->OpaqueValues( sampleIndex );
										for ( TqInt k = 0; k < datasize; ++k )
											samples[k] += pSample.Data()[k] * g;
										sampleCounts[pixelIndex]++;
									}
								}
								sampleIndex++;
								sindex += numsubpixels;
							}
							pie2++;
						}

						// store the intermediate result
						for ( TqInt k = 0; k < datasize; k ++)
							intermediateSamples[pixelIndex*datasize + k] = samples[k] / gTot;

						pixelIndex += Width();
					}
				}
			}

			// now filter in y.
			for ( y = YOrigin(); y < endy ; y++ )
			{
				TqFloat ycent = y + 0.5f;
				for ( x = XOrigin(); x < endx ; x++ )
				{
					TqFloat xcent = x + 0.5f;
					TqFloat gTot = 0.0;
					SampleCount = 0;
					std::valarray<TqFloat> samples( 0.0f, datasize);

					TqInt fy;
					// Get the element at the top of the filter area.
					ImageElement( x, y - ymax, pie );
					for ( fy = -ymax; fy <= ymax; fy++ )
					{
						CqImagePixel* pie2 = pie;

						TqInt index = ( ( ( fy + ymax ) * CEIL(FilterXWidth()) ) + xmax ) * numperpixel;
						// Now go over each y subsample within the pixel
						TqInt sx = PixelXSamples() / 2; // use the samples in the centre of the pixel.
						TqInt sy = 0;
						TqInt sampleIndex = sx;
						TqInt pixelRow = (y + fy - (YOrigin()-ymax)) * PixelYSamples();
						TqInt pixelIndex = pixelRow*Width() + x-XOrigin();

						for ( sy = 0; sy < PixelYSamples(); sy++ )
						{
							TqInt sindex = index + ( ( ( sy * PixelXSamples() ) + sx ) * numsubpixels );
							const SqSampleData& sampleData = pie2->SampleData( sampleIndex );							
							CqVector2D vecS = sampleData.m_Position;
							vecS -= CqVector2D( xcent, ycent );
							if ( vecS.x() >= -xfwo2 && vecS.y() >= -yfwo2 && vecS.x() <= xfwo2 && vecS.y() <= yfwo2 )
							{
								TqInt cindex = sindex + sampleData.m_SubCellIndex;
								TqFloat g = m_aFilterValues[ cindex ];
								gTot += g;
								if(sampleCounts[pixelIndex] > 0)
								{
									SampleCount += sampleCounts[pixelIndex];
									for ( TqInt k = 0; k < datasize; k++)
										samples[k] += intermediateSamples[pixelIndex * datasize + k] * g;
								}
							}
							sampleIndex += PixelXSamples();
							pixelIndex += Width();
						}

						pie += xlen;
					}

					// Set depth to infinity if no samples.
					if ( SampleCount == 0 )
					{
						memset(&m_aDatas[i*datasize], 0, datasize * sizeof(float));
						m_aDatas[ i*datasize+6 ] = FLT_MAX;
						m_aCoverages[i] = 0.0;
					}
					else
					{
						float oneOverGTot = 1.0 / gTot;
						for ( TqInt k = 0; k < datasize; k ++)
							m_aDatas[ i*datasize + k ] = samples[k] * oneOverGTot;

						if ( SampleCount >= numsubpixels)
							m_aCoverages[ i ] = 1.0;
						else
							m_aCoverages[ i ] = ( TqFloat ) SampleCount / ( TqFloat ) (numsubpixels );
					}

					i++;
				}
			}
		}
		else
		{
			// non-seperable filter
			for ( y = YOrigin(); y < endy ; y++ )
			{
				TqFloat ycent = y + 0.5f;
				for ( x = XOrigin(); x < endx ; x++ )
				{
					TqFloat xcent = x + 0.5f;
					TqFloat gTot = 0.0;
					SampleCount = 0;
					std::valarray<TqFloat> samples( 0.0f, datasize);

					TqInt fx, fy;
					// Get the element at the upper left corner of the filter area.
					ImageElement( x - xmax, y - ymax, pie );
					for ( fy = -ymax; fy <= ymax; fy++ )
					{
						CqImagePixel* pie2 = pie;
						for ( fx = -xmax; fx <= xmax; fx++ )
						{
							TqInt index = ( ( ( fy + ymax ) * CEIL(FilterXWidth()) ) + ( fx + xmax ) ) * numperpixel;
							// Now go over each subsample within the pixel
							TqInt sx, sy;
							TqInt sampleIndex = 0;
							for ( sy = 0; sy < PixelYSamples(); sy++ )
							{
								for ( sx = 0; sx < PixelXSamples(); sx++ )
								{
									TqInt sindex = index + ( ( ( sy * PixelXSamples() ) + sx ) * numsubpixels );
									const SqSampleData& sampleData = pie2->SampleData( sampleIndex );
									CqVector2D vecS = sampleData.m_Position;
									vecS -= CqVector2D( xcent, ycent );
									if ( vecS.x() >= -xfwo2 && vecS.y() >= -yfwo2 && vecS.x() <= xfwo2 && vecS.y() <= yfwo2 )
									{
										TqInt cindex = sindex + sampleData.m_SubCellIndex;
										TqFloat g = m_aFilterValues[ cindex ];
										gTot += g;
										if ( pie2->OpaqueValues( sampleIndex ).m_flags & SqImageSample::Flag_Valid )
										{
											SqImageSample& pSample = pie2->OpaqueValues( sampleIndex );
											for ( TqInt k = 0; k < datasize; ++k )
												samples[k] += pSample.Data()[k] * g;
											SampleCount++;
										}
									}
									sampleIndex++;
								}
							}
							pie2++;
						}
						pie += xlen;
					}


					// Set depth to infinity if no samples.
					if ( SampleCount == 0 )
					{
						memset(&m_aDatas[i*datasize], 0, datasize * sizeof(float));
						m_aDatas[ i*datasize+6 ] = FLT_MAX;
						m_aCoverages[i] = 0.0;
					}
					else
					{
						float oneOverGTot = 1.0 / gTot;
						for ( TqInt k = 0; k < datasize; k ++)
							m_aDatas[ i*datasize + k ] = samples[k] * oneOverGTot;

						if ( SampleCount >= numsubpixels)
							m_aCoverages[ i ] = 1.0;
						else
							m_aCoverages[ i ] = ( TqFloat ) SampleCount / ( TqFloat ) (numsubpixels );
					}

					i++;
				}
			}
		}
	}
	else
	{
		// empty bucket.
		TqInt size = Width()*Height();
		memset(&m_aDatas[0], 0, size * datasize * sizeof(float));
		memset(&m_aCoverages[0], 0, size * sizeof(float));
		for(i = 0; i<size; ++i)
		{
			// Set the depth to infinity.
			m_aDatas[ i*datasize+6 ] = FLT_MAX;
		}
	}

	i = 0;
	ImageElement( XOrigin(), YOrigin(), pie );
	endy = Height();
	endx = Width();

	// Set the coverage and alpha values for the pixel.
	for ( y = 0; y < endy; y++ )
	{
		CqImagePixel* pie2 = pie;
		for ( x = 0; x < endx; x++ )
		{
			SqImageSample& spl = pie2->GetPixelSample();
			for (TqInt k=0; k < datasize; k++)
				spl.Data()[k] = m_aDatas[ i * datasize + k ];
			TqFloat* sample_data = spl.Data();
			sample_data[Sample_Coverage] = m_aCoverages[ i++ ];

			// Calculate the alpha as the combination of the opacity and the coverage.
			TqFloat a = ( sample_data[Sample_ORed] + sample_data[Sample_OGreen] + sample_data[Sample_OBlue] ) / 3.0f;
			pie2->SetAlpha(a * sample_data[Sample_Coverage]);

			pie2++;
		}
		pie += xlen;
	}

	endy = YOrigin() + Height();
	endx = XOrigin() + Width();

	if ( QGetRenderContext() ->poptCurrent()->pshadImager() )
	{
		// Init & Execute the imager shader

		QGetRenderContext() ->poptCurrent()->InitialiseColorImager( this );
		TIME_SCOPE("Imager shading")

		if ( fImager )
		{
			i = 0;
			ImageElement( XOrigin(), YOrigin(), pie );
			for ( y = YOrigin(); y < endy ; y++ )
			{
				CqImagePixel* pie2 = pie;
				for ( x = XOrigin(); x < endx ; x++ )
				{
					imager = QGetRenderContext() ->poptCurrent()->GetColorImager( x , y );
					// Normal case will be to poke the alpha from the image shader and
					// multiply imager color with it... but after investigation alpha is always
					// == 1 after a call to imager shader in 3delight and BMRT.
					// Therefore I did not ask for alpha value and set directly the pCols[i]
					// with imager value. see imagers.cpp
					pie2->SetColor( imager );
					imager = QGetRenderContext() ->poptCurrent()->GetOpacityImager( x , y );
					pie2->SetOpacity( imager );
					TqFloat a = ( imager[0] + imager[1] + imager[2] ) / 3.0f;
					pie2->SetAlpha( a );
					pie2++;
					i++;
				}
				pie += xlen;
			}
		}
	}
}


//----------------------------------------------------------------------
/** Expose the samples in this bucket according to specified gain and gamma settings.
 */

void CqBucket::ExposeBucket()
{
	if ( QGetRenderContext() ->poptCurrent()->GetFloatOption( "System", "Exposure" ) [ 0 ] == 1.0 &&
	        QGetRenderContext() ->poptCurrent()->GetFloatOption( "System", "Exposure" ) [ 1 ] == 1.0 )
		return ;
	else
	{
		CqImagePixel* pie;
		ImageElement( XOrigin(), YOrigin(), pie );
		TqInt x, y;
		TqFloat exposegain = QGetRenderContext() ->poptCurrent()->GetFloatOption( "System", "Exposure" ) [ 0 ];
		TqFloat exposegamma = QGetRenderContext() ->poptCurrent()->GetFloatOption( "System", "Exposure" ) [ 1 ];
		TqFloat oneovergamma = 1.0f / exposegamma;
		TqFloat endx, endy;
		TqInt nextx;
		endy = Height();
		endx = Width();
		nextx = RealWidth();

		for ( y = 0; y < endy; y++ )
		{
			CqImagePixel* pie2 = pie;
			for ( x = 0; x < endx; x++ )
			{
				// color=(color*gain)^1/gamma
				if ( exposegain != 1.0 )
					pie2->SetColor( pie2->Color() * exposegain );

				if ( exposegamma != 1.0 )
				{
					CqColor col = pie2->Color();
					col.SetfRed ( pow( col.fRed (), oneovergamma ) );
					col.SetfGreen( pow( col.fGreen(), oneovergamma ) );
					col.SetfBlue ( pow( col.fBlue (), oneovergamma ) );
					pie2->SetColor( col );
				}
				pie2++;
			}
			pie += nextx;
		}
	}
}


//----------------------------------------------------------------------
/** Quantize the samples in this bucket according to type.
 */

void CqBucket::QuantizeBucket()
{
	// Initiliaze the random with a value based on the X,Y coordinate
	static CqRandom random( 61 );
	TqFloat endx, endy;
	TqInt   nextx;
	endy = Height();
	endx = Width();
	nextx = RealWidth();


	if ( QGetRenderContext() ->poptCurrent()->GetIntegerOption( "System", "DisplayMode" ) [ 0 ] & ModeRGB )
	{
		const TqFloat* pQuant = QGetRenderContext() ->poptCurrent()->GetFloatOption( "Quantize", "Color" );
		TqInt one = static_cast<TqInt>( pQuant [ 0 ] );
		TqInt min = static_cast<TqInt>( pQuant [ 1 ] );
		TqInt max = static_cast<TqInt>( pQuant [ 2 ] );
		double ditheramplitude = pQuant [ 3 ];

		// If settings are 0,0,0,0 then leave as floating point and we will save an FP tiff.
		if ( one == 0 && min == 0 && max == 0 )
			return ;

		CqImagePixel* pie;
		ImageElement( XOrigin(), YOrigin(), pie );
		TqInt x, y;

		for ( y = 0; y < endy; y++ )
		{
			CqImagePixel* pie2 = pie;
			for ( x = 0; x < endx; x++ )
			{
				double r, g, b, a;
				double _or, _og, _ob;
				double s = random.RandomFloat();
				CqColor col = pie2->Color();
				CqColor opa = pie2->Opacity();
				TqFloat alpha = pie2->Alpha();
				if ( modf( one * col.fRed () + ditheramplitude * s, &r ) > 0.5 )
					r += 1;
				if ( modf( one * col.fGreen() + ditheramplitude * s, &g ) > 0.5 )
					g += 1;
				if ( modf( one * col.fBlue () + ditheramplitude * s, &b ) > 0.5 )
					b += 1;
				if ( modf( one * opa.fRed () + ditheramplitude * s, &_or ) > 0.5 )
					_or += 1;
				if ( modf( one * opa.fGreen() + ditheramplitude * s, &_og ) > 0.5 )
					_og += 1;
				if ( modf( one * opa.fBlue () + ditheramplitude * s, &_ob ) > 0.5 )
					_ob += 1;
				if ( modf( one * alpha + ditheramplitude * s, &a ) > 0.5 )
					a += 1;
				r = CLAMP( r, min, max );
				g = CLAMP( g, min, max );
				b = CLAMP( b, min, max );
				_or = CLAMP( _or, min, max );
				_og = CLAMP( _og, min, max );
				_ob = CLAMP( _ob, min, max );
				a = CLAMP( a, min, max );
				col.SetfRed ( r );
				col.SetfGreen( g );
				col.SetfBlue ( b );
				opa.SetfRed ( _or );
				opa.SetfGreen( _og );
				opa.SetfBlue ( _ob );
				pie2->SetColor( col );
				pie2->SetOpacity( opa );
				pie2->SetAlpha( a );
				pie2++;
			}
			pie += nextx;
		}
	}

	if ( QGetRenderContext() ->poptCurrent()->GetIntegerOption( "System", "DisplayMode" ) [ 0 ] & ModeZ )
	{
		const TqFloat* pQuant = QGetRenderContext() ->poptCurrent()->GetFloatOption( "Quantize", "Depth" );
		TqInt one = static_cast<TqInt>( pQuant [ 0 ] );
		TqInt min = static_cast<TqInt>( pQuant [ 1 ] );
		TqInt max = static_cast<TqInt>( pQuant [ 2 ] );
		double ditheramplitude = pQuant [ 3 ];
		if( ditheramplitude == 0.0f && one == 0 && min == 0 && max == 0 )
			return;

		CqImagePixel* pie;
		ImageElement( XOrigin(), YOrigin(), pie );
		TqInt x, y;
		for ( y = 0; y < endy; y++ )
		{
			CqImagePixel* pie2 = pie;
			for ( x = 0; x < endx; x++ )
			{
				double d;
				if ( modf( one * pie2->Depth() + ditheramplitude * random.RandomFloat(), &d ) > 0.5 )
					d += 1;
				d = CLAMP( d, min, max );
				pie2->SetDepth( d );
				pie2++;
			}
			pie += nextx;
		}
	}

	// Now go through the other AOV's and quantize those if necessary.
	std::map<std::string, CqRenderer::SqOutputDataEntry>& DataMap = QGetRenderContext()->GetMapOfOutputDataEntries();
	std::map<std::string, CqRenderer::SqOutputDataEntry>::iterator entry;
	for( entry = DataMap.begin(); entry != DataMap.end(); entry++ )
	{
		const TqFloat* pQuant = QGetRenderContext() ->poptCurrent()->GetFloatOption( "Quantize", entry->first.c_str() );
		if( NULL != pQuant )
		{
			TqInt startindex = entry->second.m_Offset;
			TqInt endindex = startindex + entry->second.m_NumSamples;
			TqInt one = static_cast<TqInt>( pQuant [ 0 ] );
			TqInt min = static_cast<TqInt>( pQuant [ 1 ] );
			TqInt max = static_cast<TqInt>( pQuant [ 2 ] );
			double ditheramplitude = pQuant [ 3 ];

			CqImagePixel* pie;
			ImageElement( XOrigin(), YOrigin(), pie );
			TqInt x, y;
			for ( y = 0; y < endy; y++ )
			{
				CqImagePixel* pie2 = pie;
				for ( x = 0; x < endx; x++ )
				{
					TqInt sampleindex;
					for( sampleindex = startindex; sampleindex < endindex; sampleindex++ )
					{
						double d;
						if ( modf( one * pie2->GetPixelSample().Data()[sampleindex] + ditheramplitude * random.RandomFloat(), &d ) > 0.5 )
							d += 1.0f;
						d = CLAMP( d, min, max );
						pie2->GetPixelSample().Data()[sampleindex] = d;
					}
					pie2++;
				}
				pie += nextx;
			}
		}
	}
}

//----------------------------------------------------------------------
/** Clear any data on the bucket
 */
void CqBucket::ShutdownBucket()
{
	m_aieImage.clear();
	m_aFilterValues.clear();
	m_aCoverages.clear();
	m_aDatas.clear();
	std::vector<std::vector<CqVector2D> >::iterator i;
	for( i=m_aSamplePositions.begin(); i!=m_aSamplePositions.end(); i++ )
		(*i).clear();
	m_aSamplePositions.clear();
	m_SamplePoints.clear();
}

//----------------------------------------------------------------------
/** Generate piecewise-linear visibility functions for each
 * pixel in this bucket (for rendering Deep Shadow maps).
 * \param empty True iff this bucket is empty (i.e. covers no micropolygons)
 */
void CqBucket::FilterTransmittance(bool empty)
{
	CqImagePixel* pie;
	TqInt xmax = m_DiscreteShiftX;
	TqInt ymax = m_DiscreteShiftY;
	TqInt x, y;
	TqInt endy = YOrigin() +  Height(); 
	TqInt endx = XOrigin() + Width();
	bool useSeperable = true;
	// non-separable is faster for very small filter widths.
	if(FilterXWidth() <= 16.0 || FilterYWidth() <= 16.0)
		useSeperable = false;	

	if (!empty)
	{
		// non-separable filter
		// is this correct, below??
		m_VisibilityFunctions.reserve(RealWidth() * RealHeight()); // RealWidth, or should it be Width()??
		for ( y = YOrigin(); y < endy ; ++y )
		{
			TqFloat ycent = y + 0.5f;
			for ( x = XOrigin(); x < endx ; ++x )
			{	
				TqFloat xcent = x + 0.5f;
				// Get the pixel at the top left of the filter area
				ImageElement( x - xmax, y - ymax, pie );
				CalculateVisibility(xcent, ycent, pie);
			}
		}
		//CheckVisibilityFunction(1);
	}
}

//----------------------------------------------------------------------
/** Generate a visibility function for the given
 * pixel (for rendering Deep Shadow maps).
 * \param xcent the x-coordinate of the current pixel's center, in raster space
 * \param ycent the y-coordinate of the current pixel's center, in raster space
 * \param pie a pointer to the current pixel
 */
void CqBucket::CalculateVisibility( TqFloat xcent, TqFloat ycent, CqImagePixel* pie )
{
	CqImagePixel* pie2;
	TqInt fx, fy;
	TqInt SampleCount = 0;
	TqFloat xfwo2 = CEIL(FilterXWidth()) * 0.5f;
	TqFloat yfwo2 = CEIL(FilterYWidth()) * 0.5f;
	TqInt xmax = m_DiscreteShiftX;
	TqInt ymax = m_DiscreteShiftY;
	TqInt numsubpixels = ( PixelXSamples() * PixelYSamples() );
	TqInt numperpixel = numsubpixels * numsubpixels;
	TqInt xlen = RealWidth();
	TqFloat sumFilterValues = 0; // For normalizing the filter values
	TqFloat inverseSumFilterValues = 0; // so we can do multiplication instead of division (faster?)
	std::priority_queue< SqHitHeapNode, std::vector<SqHitHeapNode> > nexthitheap; // min-heap keeps next-closest "hit" in each transmittance data set
	CqColor slopeAtJ(0,0,0); // Keeps track of the sum of the changes in slope up until node j (the current node)
							 // which happens to equal the slope of the visibility curve at that point
	// Setup						
	boost::shared_ptr<TqVisibilityFunction> currentVisFunc(new TqVisibilityFunction);
	currentVisFunc->reserve(numperpixel); // There may be more, but at least this many
	m_VisibilityFunctions.push_back(currentVisFunc);
	// First, build and insert node 0, which has 100% visibility at depth z=0
	boost::shared_ptr<SqVisibilityNode> visibilityNode(new SqVisibilityNode);	
	visibilityNode->zdepth = 0;
	visibilityNode->visibility.SetColorRGB(1,1,1);
	currentVisFunc->push_back(visibilityNode);
	
	for ( fy = -ymax; fy <= ymax; ++fy )
	{	
		pie2 = pie;
		for ( fx = -xmax; fx <= xmax; ++fx )
		{
			TqInt index = ( ( ( fy + ymax ) * CEIL(FilterXWidth()) ) + ( fx + xmax ) ) * numperpixel;
			// Now go over each subsample within the pixel
			TqInt sx, sy;
			TqInt sampleIndex = 0;
			for ( sy = 0; sy < PixelYSamples(); sy++ )
			{
				for ( sx = 0; sx < PixelXSamples(); sx++ )
				{
					TqInt sindex = index + ( ( ( sy * PixelXSamples() ) + sx ) * numsubpixels );
					const SqSampleData& currentSampleData = pie2->SampleData(sampleIndex);
					CqVector2D vecS = currentSampleData.m_Position;
					vecS -= CqVector2D( xcent, ycent );
					if ( vecS.x() >= -xfwo2 && vecS.y() >= -yfwo2 && vecS.x() <= xfwo2 && vecS.y() <= yfwo2 )
					{
						TqInt cindex = sindex + currentSampleData.m_SubCellIndex;
						//printf("Filter value: %f\n", m_aFilterValues[ cindex ] );
						if ( !currentSampleData.m_Data.empty() )
						{ // This subpixel covers multiple samples, and the frontmost sample is not opaque 
							SqHitHeapNode hitNode(&currentSampleData, 0, gColWhite, m_aFilterValues[ cindex ]); // I should really divide by Sum(weights)
							nexthitheap.push(hitNode);
						}
						else 
						{ // This subpixel only had one hit (m_OpaqueSample), so construct its tuple, add it to the heap and we are done
							SqHitHeapNode hitNode(&currentSampleData, -1, gColWhite, m_aFilterValues[ cindex ]);
							nexthitheap.push(hitNode); // Because for some filters we don't want to divide by numsubpixels
						}
						SampleCount++;
						sumFilterValues += m_aFilterValues[ cindex ];
					}
					sampleIndex++;
				}
			}
			pie2++;
		}
		pie += xlen; // This is for filter widths larger than a single pixel
	}
	inverseSumFilterValues = 1.0/sumFilterValues;
	// Check if the heap is sorted
	//CheckHeapSorted(nexthitheap);
	// The nested loops above will place the first hit from each sub-sample into the heap.			
	while ( !nexthitheap.empty() && currentVisFunc->back()->visibility > gColBlack )
	{
		// NOTE: In here we will want to premultiply the volume 
		// and surface transmittance data before calculatting the deltas
		SqHitHeapNode currentHit = nexthitheap.top();
		nexthitheap.pop(); // I need this because top() does not actually dequeue the heap
		SqDeltaNode deltaNode;
		if (currentHit.queueIndex == -1)
		{ // special case: we only have the opaque entry
			deltaNode.zdepth = currentHit.samplepointer->m_OpaqueSample.Data()[Sample_Depth];
			
			deltaNode.deltatransmittance.SetColorRGB( currentHit.samplepointer->m_OpaqueSample.Data()[Sample_ORed],
												currentHit.samplepointer->m_OpaqueSample.Data()[Sample_OGreen],
												currentHit.samplepointer->m_OpaqueSample.Data()[Sample_OBlue]);
			deltaNode.deltatransmittance *= (-1)*currentHit.weight*inverseSumFilterValues; // apply filter weight and negate the delta  
			deltaNode.deltaslope.SetColorRGB(0,0,0); // Normally no slope change, but if we have participating media? Might have slope change.
		}
		else
		{
			// Construct a delta node from the dequeued item
			// If the source transmittance function of the above still has more to contribute, enqueue another node 
			deltaNode.zdepth = currentHit.samplepointer->m_Data[currentHit.queueIndex].Data()[Sample_Depth];
			
			deltaNode.deltatransmittance.SetColorRGB(  currentHit.samplepointer->m_Data[currentHit.queueIndex].Data()[Sample_ORed],
												 currentHit.samplepointer->m_Data[currentHit.queueIndex].Data()[Sample_OGreen],
												 currentHit.samplepointer->m_Data[currentHit.queueIndex].Data()[Sample_OBlue]);
			deltaNode.deltatransmittance *= (-1)*currentHit.weight*inverseSumFilterValues; // apply filter weight and negate the delta  
			// NOTE: We should compute the slope change here
			deltaNode.deltaslope.SetColorRGB(0,0,0); // 0s for now, since slope change should only occur when rendering participating media
			// Here we should also multiply the deltaslope with the filter weight
			// Now we decide whether we should enque another node from currentHit.samplepointer
			// We want to do so iff: 
					// 1) Visibility for that transmittance function has not yet reached 0 as we step along its nodes
					// 2) That particular transmittance function still has nodes remaining
			if ((currentHit.samplepointer->m_Data.size()-1 > currentHit.queueIndex) && (currentHit.runningVisibility > gColBlack))
			{ // Enqueue another node
				SqHitHeapNode hitNode(currentHit.samplepointer,
									currentHit.queueIndex+1,
									currentHit.runningVisibility+deltaNode.deltatransmittance, // This needs to also include slope
									currentHit.weight);
				nexthitheap.push(hitNode);						
			}		
		}
		// Add next visibility node to the current visibility function	
		ReconstructVisibilityNode( deltaNode, slopeAtJ, currentVisFunc );
	}
}

//----------------------------------------------------------------------
/** Generate a visibility function node and add it to the current
 *  visibility function at the top of the list: m_VisibilityFunctions. 
 *  This is another deep shadow map generator function.
 * \param deltaNode data from which to construct visibility node.
 * \param slopeAtJ the current slope of the visibility function
 * \param currentVisFunc a pointer to the visibility function for the current pixel 
 */
void CqBucket::ReconstructVisibilityNode( const SqDeltaNode& deltaNode, CqColor& slopeAtJ, boost::shared_ptr<TqVisibilityFunction> currentVisFunc )
{		
	// \todo Performance tuning: Consider alternatives to shared_ptr here.
	// Possibly intrusive_ptr, or holding the structures by value, with a memory pooling mechanism like SqImageSample
	boost::shared_ptr<SqVisibilityNode> visibilityNodePreHit(new SqVisibilityNode);
	boost::shared_ptr<SqVisibilityNode> visibilityNodePostHit(new SqVisibilityNode);
	
	visibilityNodePreHit->zdepth = deltaNode.zdepth;
	visibilityNodePreHit->visibility = MAX((currentVisFunc->back()->visibility + (visibilityNodePreHit->zdepth-currentVisFunc->back()->zdepth)*slopeAtJ), gColBlack);
	visibilityNodePostHit->zdepth = deltaNode.zdepth;
	visibilityNodePostHit->visibility = MAX((visibilityNodePreHit->visibility + deltaNode.deltatransmittance), gColBlack); 
	slopeAtJ += deltaNode.deltaslope;
	currentVisFunc->push_back(visibilityNodePreHit);
	currentVisFunc->push_back(visibilityNodePostHit);
}

//----------------------------------------------------------------------
/** Get a pointer to the visibility data for the given pixel
 * If position is outside bucket, return NULL
 * \param iXPos Screen position of sample.
 * \param iYPos Screen position of sample.
 */
const TqVisibilityFunction* CqBucket::VisibilityData( TqInt iXPos, TqInt iYPos )
{
	TqInt index = iYPos*m_RealWidth+iXPos;
	if ( index < m_VisibilityFunctions.size() )
	{
		return m_VisibilityFunctions[index].get();
	}
	return NULL;	
}

//----------------------------------------------------------------------
/** Assert that the heap is sorted by depth
 * Note: This takes nexthitheap in by copy, so that it may be dequeued without affecting the original.
 */
void CqBucket::CheckHeapSorted(std::priority_queue< SqHitHeapNode, std::vector<SqHitHeapNode> > nexthitheap)
{
	TqFloat prevDepth = -1;
	
	while (!nexthitheap.empty())
	{
		SqHitHeapNode hitHeapNode = nexthitheap.top();
		nexthitheap.pop(); // I need this because top() does not actually dequeue the heap
		if (hitHeapNode.queueIndex == -1)
		{ // special case: we only have the opaque entry
			//std::cout << "Next depth: " << hitHeapNode.samplepointer->m_OpaqueSample.Data()[Sample_Depth] << std::endl;
			assert(hitHeapNode.samplepointer->m_OpaqueSample.Data()[Sample_Depth] >= prevDepth);
			prevDepth = hitHeapNode.samplepointer->m_OpaqueSample.Data()[Sample_Depth];
		}
		else
		{
			//std::cout << "Next depth: " << hitHeapNode.samplepointer->m_Data[hitHeapNode.queueIndex].Data()[Sample_Depth] << std::endl;
			assert(hitHeapNode.samplepointer->m_Data[hitHeapNode.queueIndex].Data()[Sample_Depth] >= prevDepth);
			prevDepth = hitHeapNode.samplepointer->m_Data[hitHeapNode.queueIndex].Data()[Sample_Depth];		
		}
	}		
}

/** Print a specified visibility function to standard out as a sequence of (depth, visibility) pairs
 */
void CqBucket::CheckVisibilityFunction(TqInt index) const
{
	TqInt j;
	boost::shared_ptr<TqVisibilityFunction> checkFunction = m_VisibilityFunctions[index];
	
	std::cout << "This visibility function has # nodes: " << checkFunction->size() << std::endl;
	std::cout << "Printing (depth, visibility)\n";
	for (j = 0; j < checkFunction->size(); ++j)
	{
		std::cout << (*checkFunction)[j]->zdepth << ", " << (*checkFunction)[j]->visibility.fRed() << std::endl;
	}
}

//---------------------------------------------------------------------
/* Pure virtual destructor for CqBucket
 */
CqBucket::~CqBucket()
{
}

END_NAMESPACE( Aqsis )

