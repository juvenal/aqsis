// Aqsis - bucketprocessor.cpp
//
// Copyright (C) 2007 Manuel A. Fernadez Montecelo <mafm@users.sourceforge.net>
//
// This library is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License as
// published by the Free Software Foundation; either version 2 of the
// License, or (at your option) any later version.
//
// This library is distributed in the hope that it will be useful, but
// WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
// General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this library; if not, write to the Free Software
// Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307 USA

#include	"bucketprocessor.h"

#include	<valarray>

#include	"aqsismath.h"
#include	"bucket.h"
#include	"imagebuffer.h"
#include	"multitimer.h"


namespace Aqsis {

CqBucketProcessor::CqBucketProcessor()
	: m_bucket(0),
	m_PixelXSamples(QGetRenderContext()->poptCurrent()->GetIntegerOption("System", "PixelSamples")[0]),
	m_PixelYSamples(QGetRenderContext()->poptCurrent()->GetIntegerOption("System", "PixelSamples")[1]),
	m_FilterXWidth(QGetRenderContext()->poptCurrent()->GetFloatOption("System", "FilterWidth")[0]),
	m_FilterYWidth(QGetRenderContext()->poptCurrent()->GetFloatOption("System", "FilterWidth")[1]),
	m_clippingNear(QGetRenderContext()->poptCurrent()->GetFloatOption("System", "Clipping")[0]),
	m_clippingFar(QGetRenderContext()->poptCurrent()->GetFloatOption("System", "Clipping")[1]),
	m_DiscreteShiftX(lfloor(m_FilterXWidth/2.0f)),
	m_DiscreteShiftY(lfloor(m_FilterYWidth/2.0f)),
	m_NumDofBounds(0),
	m_DofBounds(),
	m_aieImage(),
	m_pixelPool(m_PixelXSamples, m_PixelYSamples),
	m_aFilterValues(),
	m_CurrentMpgSampleInfo(),
	m_OcclusionTree(),
	m_DataRegion(),
	m_SampleRegion(),
	m_DisplayRegion(),
	m_hasValidSamples(false),
	m_channelBuffer()
{
	setupCacheInformation();
}

void CqBucketProcessor::setupCacheInformation()
{
	// Calculate and store the cache region sizes for overlap cacheing
	TqInt overlapx = m_DiscreteShiftX*2;
	TqInt overlapy = m_DiscreteShiftY*2;
	TqInt width = QGetRenderContext()->pImage()->XBucketSize() + overlapx; 
	TqInt height = QGetRenderContext()->pImage()->YBucketSize() + overlapy;
	m_cacheRegions[SqBucketCacheSegment::top_left] = CqRegion(0, 0, overlapx, overlapy);
	m_cacheRegions[SqBucketCacheSegment::top] = CqRegion(overlapx, 0, width-overlapx, overlapy);
	m_cacheRegions[SqBucketCacheSegment::top_right] = CqRegion(width-overlapx, 0, width, overlapy);
	m_cacheRegions[SqBucketCacheSegment::left] = CqRegion(0, overlapy, overlapx, height-overlapy);
	m_cacheRegions[SqBucketCacheSegment::right] = CqRegion(width-overlapx, overlapy, width, height-overlapy);
	m_cacheRegions[SqBucketCacheSegment::bottom_left] = CqRegion(0, height-overlapy, overlapx, height);
	m_cacheRegions[SqBucketCacheSegment::bottom] = CqRegion(overlapx, height-overlapy, width-overlapx, height);
	m_cacheRegions[SqBucketCacheSegment::bottom_right] = CqRegion(width-overlapx, height-overlapy, width, height);
}

void CqBucketProcessor::setBucket(CqBucket* bucket)
{
	assert(m_bucket == 0);

	m_bucket = bucket;
	m_hasValidSamples = false;
}

const CqBucket* CqBucketProcessor::getBucket() const
{
	return m_bucket;
}

void CqBucketProcessor::reset()
{
	if (!m_bucket)
		return;

	assert(m_bucket && m_bucket->IsProcessed());

	m_bucket = 0;
	m_hasValidSamples = false;
}

void CqBucketProcessor::preProcess(IqSampler* sampler)
{
	assert(m_bucket);

	{
		AQSIS_TIME_SCOPE(Prepare_bucket);

		TqInt xPos = m_bucket->getXPosition();
		TqInt yPos = m_bucket->getYPosition();
		TqInt xSize = m_bucket->getXSize();
		TqInt ySize = m_bucket->getYSize();

		m_DisplayRegion = CqRegion( xPos, yPos, xPos+xSize, yPos+ySize );
		m_DataRegion = CqRegion( xPos - m_DiscreteShiftX, yPos - m_DiscreteShiftY, 
								 xPos + QGetRenderContext()->pImage()->XBucketSize() + m_DiscreteShiftX, 
								 yPos + QGetRenderContext()->pImage()->YBucketSize() + m_DiscreteShiftY );

		TqInt sminx = xPos - m_DiscreteShiftX;
		TqInt sminy = yPos - m_DiscreteShiftY;
		TqInt smaxx = xPos + xSize + m_DiscreteShiftX;
		TqInt smaxy = yPos + ySize + m_DiscreteShiftY;

		// Adjust the sample region to take into account the crop window.
		if ( sminx < QGetRenderContext()->cropWindowXMin() - m_DiscreteShiftX )
			sminx = static_cast<TqInt>(QGetRenderContext()->cropWindowXMin() - m_DiscreteShiftX );
		if ( sminy < QGetRenderContext()->cropWindowYMin() - m_DiscreteShiftY )
			sminy = static_cast<TqInt>(QGetRenderContext()->cropWindowYMin() - m_DiscreteShiftY );
		if ( smaxx > QGetRenderContext()->cropWindowXMax() + m_DiscreteShiftX )
			smaxx = static_cast<TqInt>(QGetRenderContext()->cropWindowXMax() + m_DiscreteShiftX);
		if ( smaxy > QGetRenderContext()->cropWindowYMax() + m_DiscreteShiftY )
			smaxy = static_cast<TqInt>(QGetRenderContext()->cropWindowYMax() + m_DiscreteShiftY );

		// Now shrink the sample region according to any cache segments that have been
		// applied.
		if(m_bucket->cacheSegments()[SqBucketCacheSegment::left])
			sminx += m_DiscreteShiftX*2;
		if(m_bucket->cacheSegments()[SqBucketCacheSegment::right])
			smaxx -= m_DiscreteShiftX*2;
		if(m_bucket->cacheSegments()[SqBucketCacheSegment::top])
			sminy += m_DiscreteShiftY*2;
		if(m_bucket->cacheSegments()[SqBucketCacheSegment::bottom])
			smaxy -= m_DiscreteShiftY*2;
		m_SampleRegion = CqRegion(sminx, sminy, smaxx, smaxy);

		// Allocate the image element storage if this is the first bucket
		if(m_aieImage.empty())
		{
			SqImageSample::sampleSize = QGetRenderContext() ->GetOutputDataTotalSize();

			m_aieImage.resize( DataRegion().area() );
			CalculateDofBounds();

			// Initialise the samples for this bucket.
			TqInt which = 0;
			TqInt maxY = DataRegion().height();
			TqInt maxX = DataRegion().width();
			for ( TqInt i = 0; i < maxY; i++ )
			{
				for ( TqInt j = 0; j < maxX; j++ )
				{
					m_aieImage[which] = m_pixelPool.allocate();
					which++;
				}
			}
		}


		// Clear the sample points and and adjust them for the new bucket
		// position, jittering the samples if necessary.
		TqInt which = 0;
		TqInt originY = DisplayRegion().yMin();
		TqInt originX = DisplayRegion().xMin();
		TqInt stride = DataRegion().width();
		for ( TqInt y = SampleRegion().yMin(), yend = SampleRegion().yMax(); y < yend; ++y )
		{
			for ( TqInt x = SampleRegion().xMin(), xend = SampleRegion().xMax(); x < xend; ++x )
			{
				// Setup the offsets
				which = ((y-originY+m_DiscreteShiftY)*stride)+x-originX+m_DiscreteShiftX;
				CqVector2D bPos2 = CqVector2D(x, y);
				m_aieImage[which]->clear();
				m_aieImage[which]->setSamples(sampler, bPos2);
			}
		}
		InitialiseFilterValues();
	}
	
	const CqBucket::TqCache& cache = m_bucket->cacheSegments();
	if(cache[SqBucketCacheSegment::left])
		applyCacheSegment(SqBucketCacheSegment::left, cache[SqBucketCacheSegment::left]);
	if(cache[SqBucketCacheSegment::right])
		applyCacheSegment(SqBucketCacheSegment::right, cache[SqBucketCacheSegment::right]);
	if(cache[SqBucketCacheSegment::top])
		applyCacheSegment(SqBucketCacheSegment::top, cache[SqBucketCacheSegment::top]);
	if(cache[SqBucketCacheSegment::bottom])
		applyCacheSegment(SqBucketCacheSegment::bottom, cache[SqBucketCacheSegment::bottom]);
	if(cache[SqBucketCacheSegment::top_left])
		applyCacheSegment(SqBucketCacheSegment::top_left, cache[SqBucketCacheSegment::top_left]);
	if(cache[SqBucketCacheSegment::top_right])
		applyCacheSegment(SqBucketCacheSegment::top_right, cache[SqBucketCacheSegment::top_right]);
	if(cache[SqBucketCacheSegment::bottom_left])
		applyCacheSegment(SqBucketCacheSegment::bottom_left, cache[SqBucketCacheSegment::bottom_left]);
	if(cache[SqBucketCacheSegment::bottom_right])
		applyCacheSegment(SqBucketCacheSegment::bottom_right, cache[SqBucketCacheSegment::bottom_right]);

	{
		AQSIS_TIME_SCOPE(Occlusion_culling_initialisation);
		// Set up the occlusion tree to cover the region of the bucket that
		// will be used for sampling.
		m_OcclusionTree.setupTree(*this);
	}
}

void CqBucketProcessor::process()
{
	if (!m_bucket)
		return;

	{
		AQSIS_TIME_SCOPE(Render_MPGs);
		RenderWaitingMPs();
	}

	// Render any waiting subsurfaces.
	// \todo Need to refine the exit condition, to ensure that all previous buckets have been
	// duly processed.
	while ( m_bucket->hasPendingSurfaces() )
	{
		boost::shared_ptr<CqSurface> surface = m_bucket->pTopSurface();
		if (surface)
		{
			// Advance to next surface
			m_bucket->popSurface();
			RenderSurface( surface );
			{
				AQSIS_TIME_SCOPE(Render_MPGs);
				RenderWaitingMPs();
			}
		}
	}
	{
		AQSIS_TIME_SCOPE(Render_MPGs);
		RenderWaitingMPs();
	}
}

void CqBucketProcessor::postProcess( bool imager, EqFilterDepth depthfilter, const CqColor& zThreshold )
{
	if (!m_bucket)
		return;

	// Combine the colors at each pixel sample for any
	// micropolygons rendered to that pixel.
	{
		AQSIS_TIME_SCOPE(Combine_samples);
		CombineElements(depthfilter, zThreshold);
	}

	{
		AQSIS_TIME_SCOPE(Filter_samples);
		FilterBucket(imager);
		ExposeBucket();
	}

	boost::shared_ptr<SqBucketCacheSegment> top_left, top_right, bottom_left, bottom_right;

	std::vector<CqBucket*> neighbours;
	QGetRenderContext()->pImage()->axialNeighbours(*m_bucket, neighbours);
	if(neighbours[CqImageBuffer::left] && !neighbours[CqImageBuffer::left]->IsProcessed())
	{
		boost::shared_ptr<SqBucketCacheSegment> cacheSegment(new SqBucketCacheSegment);
		buildCacheSegment(SqBucketCacheSegment::left, cacheSegment);
		neighbours[CqImageBuffer::left]->setCacheSegment(SqBucketCacheSegment::right, cacheSegment);

		if(!neighbours[CqImageBuffer::left]->cacheSegments()[SqBucketCacheSegment::top_right])
		{
			top_left = boost::shared_ptr<SqBucketCacheSegment>(new SqBucketCacheSegment);
			buildCacheSegment(SqBucketCacheSegment::top_left, top_left);
			neighbours[CqImageBuffer::left]->setCacheSegment(SqBucketCacheSegment::top_right, top_left);
		}
		if(!neighbours[CqImageBuffer::left]->cacheSegments()[SqBucketCacheSegment::bottom_right])
		{
			bottom_left = boost::shared_ptr<SqBucketCacheSegment>(new SqBucketCacheSegment);
			buildCacheSegment(SqBucketCacheSegment::bottom_left, bottom_left);
			neighbours[CqImageBuffer::left]->setCacheSegment(SqBucketCacheSegment::bottom_right, bottom_left);
		}
	}
	if(neighbours[CqImageBuffer::right] && !neighbours[CqImageBuffer::right]->IsProcessed())
	{
		boost::shared_ptr<SqBucketCacheSegment> cacheSegment(new SqBucketCacheSegment);
		buildCacheSegment(SqBucketCacheSegment::right, cacheSegment);
		neighbours[CqImageBuffer::right]->setCacheSegment(SqBucketCacheSegment::left, cacheSegment);
		if(!neighbours[CqImageBuffer::right]->cacheSegments()[SqBucketCacheSegment::top_left])
		{
			top_right = boost::shared_ptr<SqBucketCacheSegment>(new SqBucketCacheSegment);
			buildCacheSegment(SqBucketCacheSegment::top_right, top_right);
			neighbours[CqImageBuffer::right]->setCacheSegment(SqBucketCacheSegment::top_left, top_right);
		}
		if(!neighbours[CqImageBuffer::right]->cacheSegments()[SqBucketCacheSegment::bottom_left])
		{
			bottom_right = boost::shared_ptr<SqBucketCacheSegment>(new SqBucketCacheSegment);
			buildCacheSegment(SqBucketCacheSegment::bottom_right, bottom_right);
			neighbours[CqImageBuffer::right]->setCacheSegment(SqBucketCacheSegment::bottom_left, bottom_right);
		}
	}
	if(neighbours[CqImageBuffer::above] && !neighbours[CqImageBuffer::above]->IsProcessed())
	{
		boost::shared_ptr<SqBucketCacheSegment> cacheSegment(new SqBucketCacheSegment);
		buildCacheSegment(SqBucketCacheSegment::top, cacheSegment);
		neighbours[CqImageBuffer::above]->setCacheSegment(SqBucketCacheSegment::bottom, cacheSegment);
		if(!neighbours[CqImageBuffer::above]->cacheSegments()[SqBucketCacheSegment::bottom_left])
		{
			if(!top_left)
			{
				top_left = boost::shared_ptr<SqBucketCacheSegment>(new SqBucketCacheSegment);
				buildCacheSegment(SqBucketCacheSegment::top_left, top_left);
			}
			neighbours[CqImageBuffer::above]->setCacheSegment(SqBucketCacheSegment::bottom_left, top_left);
		}
		if(!neighbours[CqImageBuffer::above]->cacheSegments()[SqBucketCacheSegment::bottom_right])
		{
			if(!top_right)
			{
				top_right = boost::shared_ptr<SqBucketCacheSegment>(new SqBucketCacheSegment);
				buildCacheSegment(SqBucketCacheSegment::top_right, top_right);
			}
			neighbours[CqImageBuffer::above]->setCacheSegment(SqBucketCacheSegment::bottom_right, top_right);
		}
	}
	if(neighbours[CqImageBuffer::below] && !neighbours[CqImageBuffer::below]->IsProcessed())
	{
		boost::shared_ptr<SqBucketCacheSegment> cacheSegment(new SqBucketCacheSegment);
		buildCacheSegment(SqBucketCacheSegment::bottom, cacheSegment);
		neighbours[CqImageBuffer::below]->setCacheSegment(SqBucketCacheSegment::top, cacheSegment);
		if(!neighbours[CqImageBuffer::below]->cacheSegments()[SqBucketCacheSegment::top_left])
		{
			if(!bottom_left)
			{
				bottom_left = boost::shared_ptr<SqBucketCacheSegment>(new SqBucketCacheSegment);
				buildCacheSegment(SqBucketCacheSegment::bottom_left, bottom_left);
			}
			neighbours[CqImageBuffer::below]->setCacheSegment(SqBucketCacheSegment::top_left, bottom_left);
		}
		if(!neighbours[CqImageBuffer::below]->cacheSegments()[SqBucketCacheSegment::top_right])
		{
			if(!bottom_right)
			{
				bottom_right = boost::shared_ptr<SqBucketCacheSegment>(new SqBucketCacheSegment);
				buildCacheSegment(SqBucketCacheSegment::bottom_right, bottom_right);
			}
			neighbours[CqImageBuffer::below]->setCacheSegment(SqBucketCacheSegment::top_right, bottom_right);
		}
	}

	for(TqInt cseg = 0; cseg < SqBucketCacheSegment::last; ++cseg)
	{
		if(m_bucket->cacheSegments()[cseg])
			dropSegment(cseg);
	}

	m_bucket->clearCache();

	assert(!m_bucket->IsProcessed());
	m_bucket->SetProcessed();
}

//----------------------------------------------------------------------
/** Combine the subsamples into single pixel samples and coverage information.
 */

void CqBucketProcessor::CombineElements(enum EqFilterDepth filterdepth, CqColor zThreshold)
{
	for(TqInt y = m_SampleRegion.yMin() - m_DisplayRegion.yMin() + m_DiscreteShiftY, endY = m_SampleRegion.yMax() - m_DisplayRegion.yMin() + m_DiscreteShiftY; y < endY; ++y)
	{
		for(TqInt x = m_SampleRegion.xMin() - m_DisplayRegion.xMin() + m_DiscreteShiftX, endX = m_SampleRegion.xMax() - m_DisplayRegion.xMin() + m_DiscreteShiftX; x < endX; ++x)
		{
			m_aieImage[(y*m_DataRegion.width())+x]->Combine(filterdepth, zThreshold);
		}
	}
}

//----------------------------------------------------------------------
/** Filter the samples in this bucket according to type and filter widths.
 */

void CqBucketProcessor::FilterBucket(bool fImager)
{
	CqImagePixelPtr * pie;

	std::map<TqInt, CqRenderer::SqOutputDataEntry> channelMap;
	// Setup the channel buffer ready to accept the output data.
	// First fill in the default display value r, g, b, a, and z.
	m_channelBuffer.clearChannels();
	channelMap[m_channelBuffer.addChannel("r", 1)] = CqRenderer::SqOutputDataEntry(Sample_Red, 1, type_float);
	channelMap[m_channelBuffer.addChannel("g", 1)] = CqRenderer::SqOutputDataEntry(Sample_Green, 1, type_float);
	channelMap[m_channelBuffer.addChannel("b", 1)] = CqRenderer::SqOutputDataEntry(Sample_Blue, 1, type_float);
	channelMap[m_channelBuffer.addChannel("or", 1)] = CqRenderer::SqOutputDataEntry(Sample_ORed, 1, type_float);
	channelMap[m_channelBuffer.addChannel("og", 1)] = CqRenderer::SqOutputDataEntry(Sample_OGreen, 1, type_float);
	channelMap[m_channelBuffer.addChannel("ob", 1)] = CqRenderer::SqOutputDataEntry(Sample_OBlue, 1, type_float);
	channelMap[m_channelBuffer.addChannel("a", 1)] = CqRenderer::SqOutputDataEntry(Sample_Alpha, 1, type_float);
	channelMap[m_channelBuffer.addChannel("z", 1)] = CqRenderer::SqOutputDataEntry(Sample_Depth, 1, type_float);
	channelMap[m_channelBuffer.addChannel("coverage", 1)] = CqRenderer::SqOutputDataEntry(Sample_Coverage, 1, type_float);
	std::map<std::string, CqRenderer::SqOutputDataEntry>& outputMap = QGetRenderContext()->GetMapOfOutputDataEntries();
	std::map<std::string, CqRenderer::SqOutputDataEntry>::iterator aov_i = outputMap.begin();
	std::map<std::string, CqRenderer::SqOutputDataEntry>::iterator aov_end = outputMap.end();
	for(; aov_i != aov_end; ++aov_i)
		channelMap[m_channelBuffer.addChannel(aov_i->first, aov_i->second.m_NumSamples)] = aov_i->second;

	TqInt depthIndex = m_channelBuffer.getChannelIndex("z");

	// Allocate a buffer for the channel information, big enough to hold the display region.
	m_channelBuffer.allocate(DisplayRegion().width(), DisplayRegion().height());
	TqInt datasize = QGetRenderContext()->GetOutputDataTotalSize();

	// Allocate a buffer to store the coverage at each pixel, calculated currently by counting sample hits.
	std::vector<TqFloat>	aCoverages;
	aCoverages.resize( DisplayRegion().area() );

	TqInt xmax = m_DiscreteShiftX;
	TqInt ymax = m_DiscreteShiftY;
	TqFloat xfwo2 = std::ceil(FilterXWidth()) * 0.5f;
	TqFloat yfwo2 = std::ceil(FilterYWidth()) * 0.5f;
	TqInt numSubPixels = ( PixelXSamples() * PixelYSamples() );

	TqInt	xlen = DataRegion().width();

	TqInt SampleCount = 0;

	TqInt x, y;
	TqInt i = 0;

	TqInt endy = DisplayRegion().yMax();
	TqInt endx = DisplayRegion().xMax();

	bool useSeperable = true;

	if(m_hasValidSamples)
	{
		// non-seperable is faster for very small filter widths.
		if(FilterXWidth() <= 16.0 || FilterYWidth() <= 16.0)
			useSeperable = false;

		if(useSeperable)
		{
			// seperable filter. filtering by fx,fy is equivalent to filtering
			// by fx,1 followed by 1,fy.

			TqInt size = DisplayRegion().width() * DisplayRegion().height() * PixelYSamples();
			std::valarray<TqFloat> intermediateSamples( 0.0f, size * datasize);
			std::valarray<TqInt> sampleCounts(0, size);
			for ( y = DisplayRegion().yMin() - ymax; y < endy + ymax ; y++ )
			{
				TqFloat ycent = y + 0.5f;
				TqInt pixelRow = (y-(DisplayRegion().yMin()-ymax)) * PixelYSamples();
				for ( x = DisplayRegion().xMin(); x < endx ; x++ )
				{
					TqFloat xcent = x + 0.5f;

					// Get the element at the left side of the filter area.
					ImageElement( x - xmax, y, pie );

					TqInt pixelIndex = pixelRow*DisplayRegion().width() + x-DisplayRegion().xMin();

					// filter just in x first
					for ( TqInt sy = 0; sy < PixelYSamples(); sy++ )
					{
						TqFloat gTot = 0.0;
						SampleCount = 0;
						std::valarray<TqFloat> samples( 0.0f, datasize);

						CqImagePixelPtr* pie2 = pie;
						for ( TqInt fx = -xmax; fx <= xmax; fx++ )
						{
							TqInt index = ( ( ymax * lceil(FilterXWidth()) ) + ( fx + xmax ) ) * numSubPixels;
							// Now go over each subsample within the pixel
							TqInt sampleIndex = sy * PixelXSamples();

							for ( TqInt sx = 0; sx < PixelXSamples(); sx++ )
							{
								SqSampleData const& sampleData = (*pie2)->SampleData( sampleIndex );
								CqVector2D vecS = sampleData.position;
								vecS -= CqVector2D( xcent, ycent );
								if ( vecS.x() >= -xfwo2 && vecS.y() >= -yfwo2 && vecS.x() <= xfwo2 && vecS.y() <= yfwo2 )
								{
									TqFloat g = m_aFilterValues[index + sampleIndex];
									gTot += g;
									SqImageSample& opv = (*pie2)->occludingHit(sampleIndex);
									if ( opv.flags & SqImageSample::Flag_Valid )
									{
										TqFloat* data = (*pie2)->sampleHitData(opv);
										for ( TqInt k = 0; k < datasize; ++k )
											samples[k] += data[k] * g;
										sampleCounts[pixelIndex]++;
									}
								}
								sampleIndex++;
							}
							pie2++;
						}

						// store the intermediate result
						for ( TqInt k = 0; k < datasize; k ++)
							intermediateSamples[pixelIndex*datasize + k] = samples[k] / gTot;

						pixelIndex += DisplayRegion().width();
					}
				}
			}

			// now filter in y.
			for ( y = DisplayRegion().yMin(); y < endy ; y++ )
			{
				TqFloat ycent = y + 0.5f;
				for ( x = DisplayRegion().xMin(); x < endx ; x++ )
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
						CqImagePixelPtr* pie2 = pie;

						TqInt index = ( ( ( fy + ymax ) * lceil(FilterXWidth()) ) + xmax ) * numSubPixels;
						// Now go over each y subsample within the pixel
						TqInt sx = PixelXSamples() / 2; // use the samples in the centre of the pixel.
						TqInt sy = 0;
						TqInt sampleIndex = sx;
						TqInt pixelRow = (y + fy - (DisplayRegion().yMin()-ymax)) * PixelYSamples();
						TqInt pixelIndex = pixelRow*DisplayRegion().width() + x-DisplayRegion().xMin();

						for ( sy = 0; sy < PixelYSamples(); sy++ )
						{
							SqSampleData const& sampleData = (*pie2)->SampleData( sampleIndex );
							CqVector2D vecS = sampleData.position;
							vecS -= CqVector2D( xcent, ycent );
							if ( vecS.x() >= -xfwo2 && vecS.y() >= -yfwo2 && vecS.x() <= xfwo2 && vecS.y() <= yfwo2 )
							{
								TqFloat g = m_aFilterValues[index + sampleIndex];
								gTot += g;
								if(sampleCounts[pixelIndex] > 0)
								{
									SampleCount += sampleCounts[pixelIndex];
									for ( TqInt k = 0; k < datasize; k++)
										samples[k] += intermediateSamples[pixelIndex * datasize + k] * g;
								}
							}
							sampleIndex += PixelXSamples();
							pixelIndex += DisplayRegion().width();
						}

						pie += xlen;
					}

					// Set depth to infinity if no samples.
					if ( SampleCount == 0 )
					{
						aCoverages[i] = 0.0;
					}
					else
					{
						float oneOverGTot = 1.0 / gTot;

						// Copy the filtered sample data into the channel buffer.
						for( std::map<TqInt, CqRenderer::SqOutputDataEntry>::iterator channel_i = channelMap.begin(); channel_i != channelMap.end(); ++channel_i )
						{
							for(TqInt i = 0; i < channel_i->second.m_NumSamples; ++i)
								m_channelBuffer(x, y, channel_i->first)[i] = samples[channel_i->second.m_Offset + i] * oneOverGTot;
						}

						if ( SampleCount >= numSubPixels)
							aCoverages[ i ] = 1.0;
						else
							aCoverages[ i ] = ( TqFloat ) SampleCount / ( TqFloat ) (numSubPixels );
					}

					i++;
				}
			}
		}
		else
		{
			// non-seperable filter
			for ( y = DisplayRegion().yMin(); y < endy ; y++ )
			{
				TqFloat ycent = y + 0.5f;
				for ( x = DisplayRegion().xMin(); x < endx ; x++ )
				{
					TqFloat xcent = x + 0.5f;
					TqFloat gTot = 0.0;
					SampleCount = 0;
					std::valarray<TqFloat> samples( 0.0f, datasize);

					// Get the element at the upper left corner of the filter area.
					ImageElement( x - xmax, y - ymax, pie );
					for (TqInt fy = -ymax; fy <= ymax; fy++, pie += xlen )
					{
						CqImagePixelPtr* pie2 = pie;
						for (TqInt fx = -xmax; fx <= xmax; fx++, ++pie2 )
						{
							TqInt index = ((fy + ymax)*lceil(FilterXWidth()) + fx + xmax) * numSubPixels;
							// Now go over each subsample within the pixel
							TqInt sampleIndex = 0;
							for (TqInt sy = 0; sy < PixelYSamples(); sy++ )
							{
								for (TqInt sx = 0; sx < PixelXSamples(); sx++ )
								{
									SqSampleData const& sampleData = (*pie2)->SampleData( sampleIndex );
									CqVector2D vecS = sampleData.position;
									vecS -= CqVector2D( xcent, ycent );
									if ( vecS.x() >= -xfwo2 && vecS.y() >= -yfwo2 && vecS.x() <= xfwo2 && vecS.y() <= yfwo2 )
									{
										TqFloat g = m_aFilterValues[index+sampleIndex];
										gTot += g;
										SqImageSample& opv = (*pie2)->occludingHit(sampleIndex);
										if ( opv.flags & SqImageSample::Flag_Valid )
										{
											TqFloat* data = (*pie2)->sampleHitData(opv);
											for ( TqInt k = 0; k < datasize; ++k )
												samples[k] += data[k] * g;
											SampleCount++;
										}
									}
									sampleIndex++;
								}
							}
						}
					}


					// Set depth to infinity if no samples.
					if ( SampleCount == 0 )
					{
						for( std::map<TqInt, CqRenderer::SqOutputDataEntry>::iterator channel_i = channelMap.begin(); channel_i != channelMap.end(); ++channel_i )
						{
							for(TqInt i = 0; i < channel_i->second.m_NumSamples; ++i)
								m_channelBuffer(x-DisplayRegion().xMin(), y-DisplayRegion().yMin(), channel_i->first)[i] = 0.0f;
						}
						// Set the depth to infinity.
						m_channelBuffer(x-DisplayRegion().xMin(), y-DisplayRegion().yMin(), depthIndex)[0] = FLT_MAX;
						aCoverages[i] = 0.0;
					}
					else
					{
						float oneOverGTot = 1.0 / gTot;
						
						// Copy the filtered sample data into the channel buffer.
						for( std::map<TqInt, CqRenderer::SqOutputDataEntry>::iterator channel_i = channelMap.begin(); channel_i != channelMap.end(); ++channel_i )
						{
							for(TqInt i = 0; i < channel_i->second.m_NumSamples; ++i)
								m_channelBuffer(x-DisplayRegion().xMin(), y-DisplayRegion().yMin(), channel_i->first)[i] = samples[channel_i->second.m_Offset + i] * oneOverGTot;
						}

						if ( SampleCount >= numSubPixels)
							aCoverages[ i ] = 1.0;
						else
							aCoverages[ i ] = ( TqFloat ) SampleCount / ( TqFloat ) (numSubPixels );
					}

					i++;
				}
			}
		}
	}
	else
	{
		// empty bucket.
		// Copy the filtered sample data into the channel buffer.
		for(TqInt y = 0; y < DisplayRegion().height(); ++y)
		{
			for(TqInt x = 0; x < DisplayRegion().width(); ++x)
			{
				for( std::map<TqInt, CqRenderer::SqOutputDataEntry>::iterator channel_i = channelMap.begin(); channel_i != channelMap.end(); ++channel_i )
				{
					for(TqInt i = 0; i < channel_i->second.m_NumSamples; ++i)
						m_channelBuffer(x, y, channel_i->first)[i] = 0.0f;
				}
				// Set the depth to infinity.
				m_channelBuffer(x, y, depthIndex)[0] = FLT_MAX;
				aCoverages[ i++ ] = 0.0f;
			}
		}
	}

	i = 0;
	endy = DisplayRegion().height();
	endx = DisplayRegion().width();

	// Set the coverage and alpha values for the pixel.
	TqInt redIndex = m_channelBuffer.getChannelIndex("r");
	TqInt greenIndex = m_channelBuffer.getChannelIndex("g");
	TqInt blueIndex = m_channelBuffer.getChannelIndex("b");
	TqInt redOIndex = m_channelBuffer.getChannelIndex("or");
	TqInt greenOIndex = m_channelBuffer.getChannelIndex("og");
	TqInt blueOIndex = m_channelBuffer.getChannelIndex("ob");
	TqInt alphaIndex = m_channelBuffer.getChannelIndex("a");
	TqInt coverageIndex = m_channelBuffer.getChannelIndex("coverage");
	for ( y = 0; y < endy; y++ )
	{
		for ( x = 0; x < endx; x++ )
		{
			TqFloat coverage = aCoverages[ i++ ];

			// Calculate the alpha as the combination of the opacity and the coverage.
			TqFloat a = ( m_channelBuffer(x, y, redOIndex)[0] + m_channelBuffer(x, y, greenOIndex)[0] + m_channelBuffer(x, y, blueOIndex)[0] ) / 3.0f;
			m_channelBuffer(x, y, alphaIndex)[0] = a * coverage;
			m_channelBuffer(x, y, coverageIndex)[0] = coverage;
		}
	}

	endy = DisplayRegion().height();
	endx = DisplayRegion().width();

	if ( QGetRenderContext() ->poptCurrent()->pshadImager() )
	{
		// Init & Execute the imager shader

		QGetRenderContext() ->poptCurrent()->InitialiseColorImager( DisplayRegion(), &m_channelBuffer );
		AQSIS_TIME_SCOPE(Imager_shading);

		if ( fImager )
		{
			CqColor imager;
			for ( y = 0; y < endy ; y++ )
			{
				for ( x = 0; x < endx ; x++ )
				{
					imager = QGetRenderContext() ->poptCurrent()->GetColorImager( x+DisplayRegion().xMin() , y+DisplayRegion().yMin() );
					// Normal case will be to poke the alpha from the image shader and
					// multiply imager color with it... but after investigation alpha is always
					// == 1 after a call to imager shader in 3delight and BMRT.
					// Therefore I did not ask for alpha value and set directly the pCols[i]
					// with imager value. see imagers.cpp
					m_channelBuffer(x, y, redIndex)[0] = imager.r();
					m_channelBuffer(x, y, greenIndex)[0] = imager.g();
					m_channelBuffer(x, y, blueIndex)[0] = imager.b();
					imager = QGetRenderContext() ->poptCurrent()->GetOpacityImager( x+DisplayRegion().xMin() , y+DisplayRegion().yMin() );
					m_channelBuffer(x, y, redOIndex)[0] = imager.r();
					m_channelBuffer(x, y, greenOIndex)[0] = imager.g();
					m_channelBuffer(x, y, blueOIndex)[0] = imager.b();
					TqFloat a = ( imager[0] + imager[1] + imager[2] ) / 3.0f;
					m_channelBuffer(x, y, alphaIndex)[0] = a;
				}
			}
		}
	}
}

void CqBucketProcessor::ImageElement( TqInt iXPos, TqInt iYPos, CqImagePixelPtr*& pie )
{
	iXPos -= DisplayRegion().xMin();
	iYPos -= DisplayRegion().yMin();
	
	// Check within renderable range
	//assert( iXPos < -m_XMax && iXPos < m_XSize + m_XMax &&
	//	iYPos < -m_YMax && iYPos < m_YSize + m_YMax );

	TqInt i = (iYPos + m_DiscreteShiftY)*DataRegion().width() + iXPos + m_DiscreteShiftX;

	assert(i < static_cast<TqInt>(m_aieImage.size()));
	assert(i >= 0);
	pie = &m_aieImage[ i ];
}

//----------------------------------------------------------------------
/** Expose the samples in this bucket according to specified gain and gamma settings.
 */

void CqBucketProcessor::ExposeBucket()
{
	if(m_hasValidSamples)
	{
		if ( QGetRenderContext() ->poptCurrent()->GetFloatOption( "System", "Exposure" ) [ 0 ] == 1.0 &&
			 QGetRenderContext() ->poptCurrent()->GetFloatOption( "System", "Exposure" ) [ 1 ] == 1.0 )
			return ;
		else
		{
			TqFloat exposegain = QGetRenderContext() ->poptCurrent()->GetFloatOption( "System", "Exposure" ) [ 0 ];
			TqFloat exposegamma = QGetRenderContext() ->poptCurrent()->GetFloatOption( "System", "Exposure" ) [ 1 ];
			TqFloat oneovergamma = 1.0f / exposegamma;
			TqFloat endx, endy;
			endy = DisplayRegion().height();
			endx = DisplayRegion().width();
			TqInt r_index = m_channelBuffer.getChannelIndex("r");
			TqInt g_index = m_channelBuffer.getChannelIndex("g");
			TqInt b_index = m_channelBuffer.getChannelIndex("b");

			TqInt x, y;
			for ( y = 0; y < endy; y++ )
			{
				for ( x = 0; x < endx; x++ )
				{
					// color=(color*gain)^1/gamma
					if ( exposegain != 1.0 )
					{
						m_channelBuffer(x, y, r_index)[0] *= exposegain;
						m_channelBuffer(x, y, g_index)[0] *= exposegain;
						m_channelBuffer(x, y, b_index)[0] *= exposegain;
					}

					if ( exposegamma != 1.0 )
					{
						m_channelBuffer(x, y, r_index)[0] = pow(m_channelBuffer(x, y, r_index)[0], oneovergamma);
						m_channelBuffer(x, y, g_index)[0] = pow(m_channelBuffer(x, y, g_index)[0], oneovergamma);
						m_channelBuffer(x, y, b_index)[0] = pow(m_channelBuffer(x, y, b_index)[0], oneovergamma);
					}
				}
			}
		}
	}
}

//----------------------------------------------------------------------
/** Initialise the cache of filter values.
 */
void CqBucketProcessor::InitialiseFilterValues()
{
	if( !m_aFilterValues.empty() )
		return;

	// Allocate and fill in the filter values array for each pixel.
	TqInt numSubPixels = PixelXSamples() * PixelYSamples();
	TqInt numPixels = (lceil(FilterXWidth()) + 1) * (lceil(FilterYWidth()) + 1);

	m_aFilterValues.resize(numPixels*numSubPixels);

	RtFilterFunc pFilter;
	pFilter = QGetRenderContext() ->poptCurrent()->funcFilter();

	TqInt xmax = m_DiscreteShiftX;
	TqInt ymax = m_DiscreteShiftY;
	TqFloat xfwo2 = std::ceil(FilterXWidth()) * 0.5f;
	TqFloat yfwo2 = std::ceil(FilterYWidth()) * 0.5f;
	TqInt xfw = lceil(FilterXWidth());

	// Go over every pixel touched by the filter
	for(TqInt py = -ymax; py <= ymax; py++)
	{
		for(TqInt px = -xmax; px <= xmax; px++)
		{
			// index of the start of the filter storage for the pixel.
			TqInt subPixelIndex = ((py + ymax)*xfw + px + xmax)*numSubPixels;
			// Go over every subpixel in the pixel.
			for (TqInt sy = 0; sy < PixelYSamples(); sy++ )
			{
				for (TqInt sx = 0; sx < PixelXSamples(); sx++, ++subPixelIndex )
				{
					// Evaluate the filter at the centre of the subpixel.  The
					// samples aren't actually at the subpixel centers but the
					// small inaccuracy doesn't seem to affect image quality
					// since the noise from the underlying image is much greater.
					TqFloat fx = (sx + 0.5f) / PixelXSamples() + px - 0.5f;
					TqFloat fy = (sy + 0.5f) / PixelYSamples() + py - 0.5f;
					TqFloat w = 0;
					if ( fx >= -xfwo2 && fy >= -yfwo2 && fx <= xfwo2 && fy <= yfwo2 )
						w = ( *pFilter ) ( fx, fy, std::ceil(FilterXWidth()), std::ceil(FilterYWidth()) );
					m_aFilterValues[ subPixelIndex ] = w;
				}
			}
		}
	}
}

void CqBucketProcessor::CalculateDofBounds()
{
	m_NumDofBounds = PixelXSamples() * PixelYSamples();
	m_DofBounds.resize(m_NumDofBounds);

	TqFloat dx = 2.0 / PixelXSamples();
	TqFloat dy = 2.0 / PixelYSamples();

	// I know this is far from an optimal way of calculating this,
	// but it's only done once so I don't care.
	// Calculate the bounding boxes that the dof offset positions fall into.
	TqFloat minX = -1.0;
	TqFloat minY = -1.0;
	TqInt which = 0;
	for(int j = 0; j < PixelYSamples(); ++j)
	{
		for(int i = 0; i < PixelXSamples(); ++i)
		{
			CqVector2D topLeft = CqImagePixel::projectToCircle(CqVector2D(minX, minY)); 
			CqVector2D topRight = CqImagePixel::projectToCircle(CqVector2D(minX + dx, minY)); 
			CqVector2D bottomLeft = CqImagePixel::projectToCircle(CqVector2D(minX, minY + dy)); 
			CqVector2D bottomRight = CqImagePixel::projectToCircle(CqVector2D(minX + dx, minY + dy)); 

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

			m_DofBounds[which].vecMin() = vectorCast<CqVector3D>(topLeft);
			m_DofBounds[which].vecMax() = vectorCast<CqVector3D>(topLeft);
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
/** Render any waiting MPs.
 
    Render ready micro polygons waiting to be processed, so that we
    have as few as possible MPs waiting and using memory at any given
    moment
 */

void CqBucketProcessor::RenderWaitingMPs()
{
	for ( std::vector<boost::shared_ptr<CqMicroPolygon> >::iterator itMP = m_bucket->micropolygons().begin();
			itMP != m_bucket->micropolygons().end();
			itMP++ )
	{
		CqMicroPolygon* mp = (*itMP).get();
		RenderMicroPoly( mp );
	}
	m_bucket->micropolygons().clear();

	m_OcclusionTree.updateTree();
}


//----------------------------------------------------------------------
/** Render the given Surface
 */
void CqBucketProcessor::RenderSurface( boost::shared_ptr<CqSurface>& surface )
{
	// Cull surface if it's hidden
	if ( !( QGetRenderContext() ->poptCurrent()->GetIntegerOption( "System", "DisplayMode" )[0] & ModeZ ) && !surface->pCSGNode() )
	{
		AQSIS_TIME_SCOPE(Occlusion_culling);
		if ( surface->fCachedBound() &&
			 occlusionCullSurface( surface ) )
		{
			return;
		}
	}

	// If the epsilon check has deemed this surface to be undiceable, don't bother asking.
	bool fDiceable = false;
	{
		AQSIS_TIME_SCOPE(Dicable_check);
		fDiceable = surface->Diceable();
	}

	// Dice & shade the surface if it's small enough...
	if ( fDiceable )
	{
		CqMicroPolyGridBase* pGrid = 0;
		{
			AQSIS_TIME_SCOPE(Dicing);
			pGrid = surface->Dice();
		}

		if ( NULL != pGrid )
		{
			ADDREF( pGrid );
			// Only shade in all cases since the Displacement could be called in the shadow map creation too.
			// \note Timings for shading are broken down into component parts within this function.
			pGrid->Shade();
			pGrid->TransferOutputVariables();

			if ( pGrid->vfCulled() == false )
			{
				AQSIS_TIME_SCOPE(Bust_grids);
				// Split any grids in this bucket waiting to be processed.
				pGrid->Split( SampleRegion().xMin(), SampleRegion().xMax(), SampleRegion().yMin(), SampleRegion().yMax());
			}

			RELEASEREF( pGrid );
		}
	}
	// The surface is not small enough, so split it...
	else if ( !surface->fDiscard() )
	{
		// Decrease the total gprim count since this gprim is replaced by other gprims
		STATS_DEC( GPR_created_total );

		// Split it
		{
			AQSIS_TIME_SCOPE(Splitting);
			std::vector<boost::shared_ptr<CqSurface> > aSplits;
			TqInt cSplits = surface->Split( aSplits );
			for ( TqInt i = 0; i < cSplits; i++ )
			{
				QGetRenderContext()->pImage()->PostSurface( aSplits[ i ] );
			}
		}
	}
}

//----------------------------------------------------------------------
/** Render a particular micropolygon.
 
 * \param pMP Pointer to the micropolygon to process.
   \see CqBucket, CqImagePixel
 */
void CqBucketProcessor::RenderMicroPoly( CqMicroPolygon* pMP )
{
	bool UsingDof = QGetRenderContext()->UsingDepthOfField();
	bool IsMoving = pMP->IsMoving();

	// At this stage, only use smooth shading interpolation for stationary
	// grids without DoF.
	/// \todo Allow smooth shading with MB or DoF.
	m_CurrentMpgSampleInfo.smoothInterpolation = !(UsingDof || IsMoving)
		&& pMP->pGrid()->GetCachedGridInfo().useSmoothShading;

	// Cache output sample info for this mpg so we don't have to keep fetching
	// it for each sample.
	pMP->CacheOutputInterpCoeffs(m_CurrentMpgSampleInfo);

	if(IsMoving || UsingDof)
		RenderMPG_MBOrDof( pMP, IsMoving, UsingDof );
	else
		RenderMPG_Static( pMP );
}



// this function assumes that neither dof or mb are being used. it is much
// simpler than the general case dealt with above.
void CqBucketProcessor::RenderMPG_Static( CqMicroPolygon* pMPG)
{
	const SqGridInfo& currentGridInfo = pMPG->pGrid()->GetCachedGridInfo();
    const TqFloat* LodBounds = currentGridInfo.lodBounds;
    bool UsingLevelOfDetail = LodBounds[ 0 ] >= 0.0f;

    TqInt sample_hits = 0;
    //TqFloat shd_rate = m_CurrentGridInfo.shadingRate;

	CqHitTestCache hitTestCache;
	bool cachedHitData = false;

	//bool mustDraw = !m_CurrentGridInfo.isCullable;

    CqBound Bound = pMPG->GetBound();

	TqFloat bminx = Bound.vecMin().x();
	TqFloat bmaxx = Bound.vecMax().x();
	TqFloat bminy = Bound.vecMin().y();
	TqFloat bmaxy = Bound.vecMax().y();
	//TqFloat bminz = Bound.vecMin().z();

	// Now go across all pixels touched by the micropolygon bound.
	// The first pixel position is at (sX, sY), the last one
	// at (eX, eY).
	TqInt eX = lceil( bmaxx );
	TqInt eY = lceil( bmaxy );
	if ( eX > SampleRegion().xMax() ) eX = SampleRegion().xMax();
	if ( eY > SampleRegion().yMax() ) eY = SampleRegion().yMax();

	TqInt sX = static_cast<TqInt>(std::floor( bminx ));
	TqInt sY = static_cast<TqInt>(std::floor( bminy ));
	if ( sY < SampleRegion().yMin() ) sY = SampleRegion().yMin();
	if ( sX < SampleRegion().xMin() ) sX = SampleRegion().xMin();

	CqImagePixelPtr* pie, *pie2;

	TqInt iXSamples = PixelXSamples();
	TqInt iYSamples = PixelYSamples();

	TqInt im = ( bminx < sX ) ? 0 : static_cast<TqInt>(std::floor( ( bminx - sX ) * iXSamples ));
	TqInt in = ( bminy < sY ) ? 0 : static_cast<TqInt>(std::floor( ( bminy - sY ) * iYSamples ));
	TqInt em = ( bmaxx > eX ) ? iXSamples : lceil( ( bmaxx - ( eX - 1 ) ) * iXSamples );
	TqInt en = ( bmaxy > eY ) ? iYSamples : lceil( ( bmaxy - ( eY - 1 ) ) * iYSamples );

	TqInt nextx = DataRegion().width();
	// If the start is less than or equal to the end, probably due to cropping region, exit.
	if(sX >= eX || sY >= eY)
		return;
	ImageElement( sX, sY, pie );

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
						SqSampleData const& sampleData = (*pie2)->SampleData( index );
						//if(mustDraw || bminz <= pie2->SampleData(index).m_occlusionBox->MaxOpaqueZ())
						{
							const CqVector2D& vecP = sampleData.position;
							const TqFloat time = 0.0;

							CqStats::IncI( CqStats::SPL_count );

							if(!Bound.Contains2D( vecP ))
								continue;

							// Check to see if the sample is within the sample's level of detail
							if ( UsingLevelOfDetail)
							{
								TqFloat LevelOfDetail = sampleData.detailLevel;
								if ( LodBounds[ 0 ] > LevelOfDetail || LevelOfDetail >= LodBounds[ 1 ] )
								{
									continue;
								}
							}

							CqStats::IncI( CqStats::SPL_bound_hits );

							// Now check if the subsample hits the micropoly
							bool SampleHit;
							TqFloat D;

							SampleHit = pMPG->Sample( hitTestCache, sampleData, D, time );

							if ( SampleHit )
							{
								sample_hits++;
								StoreSample( pMPG, pie2->get(), index, D );
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

// this function assumes that either dof or mb or both are being used.
void CqBucketProcessor::RenderMPG_MBOrDof( CqMicroPolygon* pMPG, bool IsMoving, bool UsingDof )
{
	const SqGridInfo& currentGridInfo = pMPG->pGrid()->GetCachedGridInfo();

    const TqFloat* LodBounds = currentGridInfo.lodBounds;
    bool UsingLevelOfDetail = LodBounds[ 0 ] >= 0.0f;

    TqInt sample_hits = 0;
    //TqFloat shd_rate = m_CurrentGridInfo.shadingRate;

	CqHitTestCache hitTestCache;

	TqInt iXSamples = PixelXSamples();
    TqInt iYSamples = PixelYSamples();

	TqFloat opentime = currentGridInfo.shutterOpenTime;
	TqFloat closetime = currentGridInfo.shutterCloseTime;
	TqFloat timePerSample = 0;
	if(IsMoving)
	{
		TqInt numSamples = iXSamples * iYSamples;
		timePerSample = (float)numSamples / ( closetime - opentime );
	}

	if(UsingDof)
		pMPG->CacheCocMultipliers(hitTestCache);

	const TqInt timeRanges = std::max(4, PixelXSamples() * PixelYSamples() );
	TqInt bound_maxMB = pMPG->cSubBounds( timeRanges );
    TqInt bound_maxMB_1 = bound_maxMB - 1;
	//TqInt currentIndex = 0;
    for ( TqInt bound_numMB = 0; bound_numMB < bound_maxMB; bound_numMB++ )
    {
        TqFloat time0;
        TqFloat time1;
        const CqBound& Bound = pMPG->SubBound( bound_numMB, time0 );

		// get the index of the first and last samples that can fall inside
		// the time range of this bound
		TqInt indexT0 = 0;
		TqInt indexT1 = 0;
		if(IsMoving)
		{
			if ( bound_numMB != bound_maxMB_1 )
				pMPG->SubBound( bound_numMB + 1, time1 );
			else
				time1 = closetime;//QGetRenderContext() ->optCurrent().GetFloatOptionWrite( "System", "Shutter" ) [ 1 ];
	
			indexT0 = static_cast<TqInt>(std::floor((time0 - opentime) * timePerSample));
			indexT1 = static_cast<TqInt>(lceil((time1 - opentime) * timePerSample));
		}

		TqFloat maxCocX = 0;
		TqFloat maxCocY = 0;

		TqFloat bminx = Bound.vecMin().x();
		TqFloat bmaxx = Bound.vecMax().x();
		TqFloat bminy = Bound.vecMin().y();
		TqFloat bmaxy = Bound.vecMax().y();
		TqFloat bminz = Bound.vecMin().z();
		TqFloat bmaxz = Bound.vecMax().z();
		// if bounding box is outside our viewing range, then cull it.
		if ( bminz > m_clippingFar || bmaxz < m_clippingNear )
			continue;
		TqFloat mpgbminx = bminx;
		TqFloat mpgbmaxx = bmaxx;
		TqFloat mpgbminy = bminy;
		TqFloat mpgbmaxy = bmaxy;
		TqInt bound_maxDof = 1;
		if(UsingDof)
		{
			const CqVector2D& minZCoc = QGetRenderContext()->GetCircleOfConfusion( Bound.vecMin().z() );
			const CqVector2D& maxZCoc = QGetRenderContext()->GetCircleOfConfusion( Bound.vecMax().z() );
			maxCocX = max( minZCoc.x(), maxZCoc.x() );
			maxCocY = max( minZCoc.y(), maxZCoc.y() );
			bound_maxDof = m_NumDofBounds;
		}
		else
		{
			bound_maxDof = 1;
		}

		for ( TqInt bound_numDof = 0; bound_numDof < bound_maxDof; bound_numDof++ )
		{
			if(UsingDof)
			{
				// now shift the bounding box to cover only a given range of
				// lens positions.
				const CqBound DofBound = DofSubBound( bound_numDof );
				TqFloat leftOffset = DofBound.vecMax().x() * maxCocX;
				TqFloat rightOffset = DofBound.vecMin().x() * maxCocX;
				TqFloat topOffset = DofBound.vecMax().y() * maxCocY;
				TqFloat bottomOffset = DofBound.vecMin().y() * maxCocY;

				bminx = mpgbminx - leftOffset;
				bmaxx = mpgbmaxx - rightOffset;
				bminy = mpgbminy - topOffset;
				bmaxy = mpgbmaxy - bottomOffset;
			}

			// Now go across all pixels touched by the micropolygon bound.
			// The first pixel position is at (sX, sY), the last one
			// at (eX, eY).
			TqInt eX = lceil( bmaxx );
			TqInt eY = lceil( bmaxy );
			if ( eX > SampleRegion().xMax() ) eX = SampleRegion().xMax();
			if ( eY > SampleRegion().yMax() ) eY = SampleRegion().yMax();

			TqInt sX = static_cast<TqInt>(std::floor( bminx ));
			TqInt sY = static_cast<TqInt>(std::floor( bminy ));
			if ( sY < SampleRegion().yMin() ) sY = SampleRegion().yMin();
			if ( sX < SampleRegion().xMin() ) sX = SampleRegion().xMin();

			CqImagePixelPtr* pie, *pie2;

			TqInt nextx = DataRegion().width();
			// If the start is less than or equal to the end, probably due to cropping region, exit.
			if(sX >= eX || sY >= eY)
				continue;

			ImageElement( sX, sY, pie );

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
						index = (*pie2)->GetDofOffsetIndex(bound_numDof);
					}
					else
					{
						// when using mb without dof, a range of samples
						// may have times within the current mb bounding box.
						index = indexT0;
					}
					// only bother sampling if the mpg is not occluded in this pixel.
					//if(mustDraw || bminz <= pie2->SampleData(index).m_occlusionBox->MaxOpaqueZ())
					{

						// loop over potential samples
						do
						{
							SqSampleData const& sampleData = (*pie2)->SampleData( index );
							const CqVector2D& vecP = sampleData.position;
							const TqFloat time = sampleData.time;

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
									TqFloat LevelOfDetail = sampleData.detailLevel;
									if ( LodBounds[ 0 ] > LevelOfDetail || LevelOfDetail >= LodBounds[ 1 ] )
									{
										continue;
									}
								}


								CqStats::IncI( CqStats::SPL_bound_hits );

								// Now check if the subsample hits the micropoly
								bool SampleHit;
								TqFloat D;

								SampleHit = pMPG->Sample( hitTestCache, sampleData, D, time, UsingDof );
								if ( SampleHit )
								{
									sample_hits++;
									// note index has already been incremented, so we use the previous value.
									StoreSample( pMPG, pie2->get(), index-1, D );
								}
							}
							else
							{
								if(!Bound.Contains2D( vecP ))
									continue;
								// Check to see if the sample is within the sample's level of detail
								if ( UsingLevelOfDetail)
								{
									TqFloat LevelOfDetail = sampleData.detailLevel;
									if ( LodBounds[ 0 ] > LevelOfDetail || LevelOfDetail >= LodBounds[ 1 ] )
									{
										continue;
									}
								}

								CqStats::IncI( CqStats::SPL_bound_hits );

								// Now check if the subsample hits the micropoly
								bool SampleHit;
								TqFloat D;

								SampleHit = pMPG->Sample( hitTestCache, sampleData, D, time, UsingDof );
								if ( SampleHit )
								{
									sample_hits++;
									// note index has already been incremented, so we use the previous value.
									StoreSample( pMPG, pie2->get(), index-1, D );
								}
							}
						} while (!UsingDof && index < indexT1);
					}
				}
			}
		}
    }
}

void CqBucketProcessor::StoreSample( CqMicroPolygon* pMPG, CqImagePixel* pie2, TqInt index, TqFloat D )
{
	const SqGridInfo& currentGridInfo = pMPG->pGrid()->GetCachedGridInfo();
	SqImageSample& occlHit = pie2->occludingHit(index);

	if(currentGridInfo.isCullable && (occlHit.flags & SqImageSample::Flag_Valid)
			&& pie2->sampleHitData(occlHit)[Sample_Depth] <= D)
	{
		// If the sample hit is occluded and can be culled then we return early
		// without storing the hit data at all.
		return;
	}

	CqStats::IncI( CqStats::SPL_hits );
	pMPG->MarkHit();
	// Record the fact that we have valid samples in the bucket.
	m_hasValidSamples = true;

	// Compute the color and opacity of the micropolygon at the hit point.
	// This is where ShadingInterpolation "smooth" or "constant" comes in.
	CqColor col;
	CqColor opa;
	SqSampleData const& sampleData = pie2->SampleData( index );
	const CqVector2D& vecP = sampleData.position;
	pMPG->InterpolateOutputs(m_CurrentMpgSampleInfo, vecP, col, opa);

	// Get a pointer to the hit storage.
	SqImageSample* hit = 0;
	if(m_CurrentMpgSampleInfo.isOpaque && currentGridInfo.isCullable)
	{
		// Use the occluding sample storage when possible, since this is
		// faster.  Hits may be stored in the occluding storage whenever they
		// can occlude other samples.  This happens when
		// 1) The micropoly is fully opaque
		// 2) The micropoly is not part of a CSG
		// 3) We don't need the entire set of samples for depth filtering.
		hit = &occlHit;
		hit->flags = SqImageSample::Flag_Valid;
		m_OcclusionTree.setSampleDepth(D, sampleData.occlusionIndex);
	}
	else
	{
		// Otherwise create some new storage for the hit data in sample hit
		// vector.
		pie2->Values(index).push_back(SqImageSample());
		hit = &pie2->Values(index).back();
		pie2->allocateHitData(*hit);
	}
	// Store the hit data for later use.
	TqFloat* hitData = pie2->sampleHitData(*hit);
	hitData[ Sample_Red ] = col[0];
	hitData[ Sample_Green ] = col[1];
	hitData[ Sample_Blue ] = col[2];
	hitData[ Sample_ORed ] = opa[0];
	hitData[ Sample_OGreen ] = opa[1];
	hitData[ Sample_OBlue ] = opa[2];
	hitData[ Sample_Depth ] = D;
	if(currentGridInfo.usesDataMap)
		StoreExtraData(pMPG, hitData);

	// Update CSG pointer and flags.
	hit->csgNode = pMPG->pGrid()->pCSGNode();
	if(currentGridInfo.isMatte)
		hit->flags |= SqImageSample::Flag_Matte;

	// Mark the pixel as containing valid samples, used later for the cacheing and reuse.
	pie2->markHasValidSamples();
}



void CqBucketProcessor::StoreExtraData( CqMicroPolygon* pMPG, TqFloat* hitData)
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
						hitData[ entry->second.m_Offset ] = f;
						break;
					}
					case type_point:
					case type_normal:
					case type_vector:
					case type_hpoint:
					{
						CqVector3D v;
						pData->GetPoint( v, pMPG->GetIndex() );
						hitData[ entry->second.m_Offset ] = v.x();
						hitData[ entry->second.m_Offset + 1 ] = v.y();
						hitData[ entry->second.m_Offset + 2 ] = v.z();
						break;
					}
					case type_color:
					{
						CqColor c;
						pData->GetColor( c, pMPG->GetIndex() );
						hitData[ entry->second.m_Offset ] = c.r();
						hitData[ entry->second.m_Offset + 1 ] = c.g();
						hitData[ entry->second.m_Offset + 2 ] = c.b();
						break;
					}
					case type_matrix:
					{
						CqMatrix m;
						pData->GetMatrix( m, pMPG->GetIndex() );
						TqFloat* pElements = m.pElements();
						hitData[ entry->second.m_Offset ] = pElements[ 0 ];
						hitData[ entry->second.m_Offset + 1 ] = pElements[ 1 ];
						hitData[ entry->second.m_Offset + 2 ] = pElements[ 2 ];
						hitData[ entry->second.m_Offset + 3 ] = pElements[ 3 ];
						hitData[ entry->second.m_Offset + 4 ] = pElements[ 4 ];
						hitData[ entry->second.m_Offset + 5 ] = pElements[ 5 ];
						hitData[ entry->second.m_Offset + 6 ] = pElements[ 6 ];
						hitData[ entry->second.m_Offset + 7 ] = pElements[ 7 ];
						hitData[ entry->second.m_Offset + 8 ] = pElements[ 8 ];
						hitData[ entry->second.m_Offset + 9 ] = pElements[ 9 ];
						hitData[ entry->second.m_Offset + 10 ] = pElements[ 10 ];
						hitData[ entry->second.m_Offset + 11 ] = pElements[ 11 ];
						hitData[ entry->second.m_Offset + 12 ] = pElements[ 12 ];
						hitData[ entry->second.m_Offset + 13 ] = pElements[ 13 ];
						hitData[ entry->second.m_Offset + 14 ] = pElements[ 14 ];
						hitData[ entry->second.m_Offset + 15 ] = pElements[ 15 ];
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
/** Test if this surface can be occlusion culled. If it can then
 * transfer surface to the next bucket it covers, or delete it if it
 * covers no more.
 * \param pSurface A pointer to a CqSurface derived class.
*/

bool CqBucketProcessor::occlusionCullSurface( const boost::shared_ptr<CqSurface>& surface )
{
	const CqBound RasterBound( surface->GetCachedRasterBound() );

	if ( m_OcclusionTree.canCull( RasterBound ) )
	{
		CqString objname( "unnamed" );
		const CqString* pattrName = surface->pAttributes() ->GetStringAttribute( "identifier", "name" );
		if( pattrName )
			objname = *pattrName;

		// Surface is behind everying in this bucket but it may be
		// visible in other buckets it overlaps.
		// bucket to the right
		TqInt nextBucketX = m_bucket->getCol() + 1;
		TqInt xpos = m_bucket->getXPosition() + m_bucket->getXSize();
		if ( ( nextBucketX < QGetRenderContext()->pImage()->cXBuckets() ) &&
			 ( RasterBound.vecMax().x() >= xpos ) )
		{
			Aqsis::log() << info << "GPrim: \"" << objname << 
					"\" occluded in bucket: " << m_bucket->getCol() << ", " << m_bucket->getRow() << 
					" shifted into bucket: " << nextBucketX << ", " << m_bucket->getRow() << std::endl;
			QGetRenderContext()->pImage()->Bucket( nextBucketX, m_bucket->getRow() ).AddGPrim( surface );
			return true;
		}

		// next row
		TqInt nextBucketY = m_bucket->getRow() + 1;
		// find bucket containing left side of bound
		nextBucketX = static_cast<TqInt>( RasterBound.vecMin().x() ) / QGetRenderContext()->pImage()->XBucketSize();
		nextBucketX = max( nextBucketX, 0 );
		TqInt ypos = m_bucket->getYPosition() + m_bucket->getYSize();

		if ( ( nextBucketX < QGetRenderContext()->pImage()->cXBuckets() ) &&
			 ( nextBucketY  < QGetRenderContext()->pImage()->cYBuckets() ) &&
			 ( RasterBound.vecMax().y() >= ypos ) )
		{
			Aqsis::log() << info << "GPrim: \"" << objname << 
				"\" occluded in bucket: " << m_bucket->getCol() << ", " << m_bucket->getRow() << 
				" shifted into bucket: " << nextBucketX << ", " << nextBucketY << std::endl;
			QGetRenderContext()->pImage()->Bucket( nextBucketX, nextBucketY ).AddGPrim( surface );
			return true;
		}

		// Bound covers no more buckets therefore we can delete the surface completely.
		Aqsis::log() << info << "GPrim: \"" << objname << "\" occlusion culled" << std::endl;
		STATS_INC( GPR_occlusion_culled );
		return true;
	}
	else
	{
		return false;
	}
}

void CqBucketProcessor::buildCacheSegment(SqBucketCacheSegment::EqBucketCacheSide side, boost::shared_ptr<SqBucketCacheSegment>& seg)
{
	CqRegion& segmentRegion = m_cacheRegions[side];

	TqInt segRowLen = segmentRegion.width();
	TqInt numPixels = segmentRegion.area();
	seg->cache.resize(numPixels);

	for(TqInt y = segmentRegion.yMin(), sy = 0, endY = segmentRegion.yMax(); y < endY; ++y, ++sy)
	{
		for(TqInt x = segmentRegion.xMin(), sx = 0, endX = segmentRegion.xMax(); x < endX; ++x, ++sx)
		{
			TqInt which = (y*m_DataRegion.width())+x;
			seg->cache[(sy*segRowLen)+sx] = m_aieImage[which];
			m_aieImage[which] = m_pixelPool.allocate();
		}
	}
}

void CqBucketProcessor::applyCacheSegment(SqBucketCacheSegment::EqBucketCacheSide side, const boost::shared_ptr<SqBucketCacheSegment>& seg)
{
	CqRegion& segmentRegion = m_cacheRegions[side];
	TqInt segRowLen = segmentRegion.width();

	for(TqInt y = segmentRegion.yMin(), sy = 0, endY = segmentRegion.yMax(); y < endY; ++y, ++sy)
	{
		for(TqInt x = segmentRegion.xMin(), sx = 0, endX = segmentRegion.xMax(); x < endX; ++x, ++sx)
		{
			TqInt which = (y*m_DataRegion.width())+x;
			m_pixelPool.free(m_aieImage[which]);
			m_aieImage[which] = seg->cache[(sy*segRowLen)+sx];
			m_hasValidSamples |= m_aieImage[which]->hasValidSamples();
		}
	}
}


void CqBucketProcessor::dropSegment(TqInt side)
{
	CqRegion& segmentRegion = m_cacheRegions[side]; 

	for(TqInt y = segmentRegion.yMin(), sy = 0, endY = segmentRegion.yMax(); y < endY; ++y, ++sy)
	{
		for(TqInt x = segmentRegion.xMin(), sx = 0, endX = segmentRegion.xMax(); x < endX; ++x, ++sx)
		{
			TqInt which = (y*m_DataRegion.width())+x;
			m_aieImage[which] = m_pixelPool.allocate();
		}
	}
}


} // namespace Aqsis
