/* Aqsis - bucketprocessor.h
 *
 * Copyright (C) 2007 Manuel A. Fernadez Montecelo <mafm@users.sourceforge.net>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of the
 * License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this library; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307
 * USA
 */

/** \file
 *
 * \brief File holding code to process buckets.
 *
 * \author Manuel A. Fernadez Montecelo <mafm@users.sourceforge.net>
 */

#ifndef BUCKETPROCESSOR_H_INCLUDED
#define BUCKETPROCESSOR_H_INCLUDED 1

#include	"aqsis.h"
#include	"bucketdata.h"

class CqBucket;


namespace Aqsis {


/**
 * \brief Class to process Buckets.
 */
class CqBucketProcessor
{
public:
	/** Default constructor */
	CqBucketProcessor();
	/** Destructor */
	~CqBucketProcessor();

	/** Set the bucket to be processed */
	void setBucket(CqBucket* bucket);
	/** Get the bucket to be processed */
	const CqBucket* getBucket() const;

	/** Reset the status of the object */
	void reset();

	/** Whether we can cull what's represented by the given bound */
	bool canCull(const CqBound* bound) const;

	/** Prepare the data for the bucket to be processed */
	void preProcess(const CqVector2D& pos, const CqVector2D& size,
			TqInt pixelXSamples, TqInt pixelYSamples, TqFloat filterXWidth, TqFloat filterYWidth,
			TqInt viewRangeXMin, TqInt viewRangeXMax, TqInt viewRangeYMin, TqInt viewRangeYMax,
			TqFloat clippingNear, TqFloat clippingFar);

	/** Process the bucket, basically rendering the waiting MPs
	 */
	void process();

	/** Post-process the bucket, which involves the operations
	 * Combine and Filter
	 */
	void postProcess( bool imager, EqFilterDepth depthfilter, const CqColor& zThreshold );

	/** Is the bucket empty initially?
	 */
	bool isInitiallyEmpty() const;
	/** Set if the bucket is empty initially
	 */
	void setInitiallyEmpty(bool value);

	/** Whether the bucket has pending surfaces to render
	 */
	bool hasPendingSurfaces() const;
	/** Whether the bucket has pending MPs to render
	 */
	bool hasPendingMPs() const;

	/** Get the top surface of the current bucket
	 */
	boost::shared_ptr<CqSurface> getTopSurface();
	/** Get the top surface of the current bucket
	 */
	void popSurface();

private:
	/// Pointer to the current bucket
	CqBucket* m_bucket;

	/// Whether the buckets is empty before starting to manipulate
	/// it
	bool m_initiallyEmpty;

	/// Bucket data for the current bucket
	CqBucketData m_bucketData;
};


} // namespace Aqsis

#endif
