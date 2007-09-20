// Aqsis
// Copyright © 1997 - 2002, Paul C. Gregory
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
		\brief Implements the hierarchical occlusion culling class.
		\author Andy Gill (billybobjimboy@users.sf.net)
*/

#include "aqsis.h"

#if _MSC_VER
#pragma warning(disable : 4786)
#endif

#include "occlusion.h"
#include "bound.h"
#include "imagebuffer.h"
#include <deque>
#include <fstream>

START_NAMESPACE( Aqsis )

TqInt CqOcclusionTree::m_Tab = 0;

CqOcclusionTree::CqOcclusionTree(TqInt dimension)
		: m_Parent(0), m_Dimension(dimension)
{
	TqChildArray::iterator child = m_Children.begin();
	for(; child != m_Children.end(); ++child)
		(*child) = 0;
}

CqOcclusionTree::~CqOcclusionTree()
{
	TqChildArray::iterator child = m_Children.begin();
	for(; child != m_Children.end(); ++child)
	{
		if (*child != NULL)
		{
			delete (*child);
			(*child) = NULL;
		}
	};
}

void
CqOcclusionTree::SplitNode(CqOcclusionTreePtr& a, CqOcclusionTreePtr& b)
{
	SortElements(m_Dimension);

	TqInt samplecount = m_SampleIndices.size();
	TqInt median = samplecount / 2;

	// Create the children nodes.
	a = CqOcclusionTreePtr(new CqOcclusionTree());
	b = CqOcclusionTreePtr(new CqOcclusionTree());

	a->m_MinSamplePoint = m_MinSamplePoint;
	b->m_MinSamplePoint = m_MinSamplePoint;
	a->m_MaxSamplePoint = m_MaxSamplePoint;
	b->m_MaxSamplePoint = m_MaxSamplePoint;
	TqInt newdim = ( m_Dimension + 1 ) % Dimensions();
	a->m_Dimension = b->m_Dimension = newdim;

	TqFloat dividingplane = CqBucket::ImageElement(m_SampleIndices[median].first).SampleData(m_SampleIndices[median].second).m_Position[m_Dimension];

	a->m_MaxSamplePoint[m_Dimension] = dividingplane;
	b->m_MinSamplePoint[m_Dimension] = dividingplane;

	TqInt i;
	for(i = 0; i<median; ++i)
	{
		a->m_SampleIndices.push_back(m_SampleIndices[i]);
		const SqSampleData& sample = CqBucket::ImageElement(m_SampleIndices[i].first).SampleData(m_SampleIndices[i].second);
	}

	for(; i<samplecount; ++i)
	{
		b->m_SampleIndices.push_back(m_SampleIndices[i]);
		const SqSampleData& sample = CqBucket::ImageElement(m_SampleIndices[i].first).SampleData(m_SampleIndices[i].second);
	}
}

void CqOcclusionTree::ConstructTree()
{
	std::deque<CqOcclusionTreePtr> ChildQueue;
	ChildQueue.push_back(this/*shared_from_this()*/);

	TqInt NonLeafCount = NumSamples() >= 1 ? 1 : 0;
	TqInt split_counter = 0;

	while (NonLeafCount > 0 && ChildQueue.size() < s_ChildrenPerNode)
	{
		CqOcclusionTreePtr old = ChildQueue.front();
		ChildQueue.pop_front();
		if (old->NumSamples() > 1)
		{
			--NonLeafCount;
		}

		CqOcclusionTreePtr a;
		CqOcclusionTreePtr b;
		old->SplitNode(a, b);
		split_counter++;
		if (a)
		{
			ChildQueue.push_back(a);
			if (a->NumSamples() > 1)
			{
				++NonLeafCount;
			}
			else if(a->NumSamples() == 1)
			{	
				CqBucket::ImageElement(a->m_SampleIndices[0].first).SampleData(a->m_SampleIndices[0].second).m_occlusionBox = a;
			}
		}
		if (b)
		{
			ChildQueue.push_back(b);
			if (b->NumSamples() > 1)
			{
				++NonLeafCount;
			}
			else if(b->NumSamples() == 1)
			{	
				CqBucket::ImageElement(b->m_SampleIndices[0].first).SampleData(b->m_SampleIndices[0].second).m_occlusionBox = b;
			}
		};

		if(split_counter >1 )
			delete old;
	}

	TqChildArray::iterator ii;
	std::deque<CqOcclusionTreePtr>::const_iterator jj;
	for (ii = m_Children.begin(), jj = ChildQueue.begin(); jj != ChildQueue.end(); ++jj)
	{
		// Check if the child actually has any samples, ignore it if no.
		if( (*jj)->NumSamples() > 0)
		{
			*ii = *jj;
			(*ii)->m_Parent = this/*shared_from_this()*/;
			if ((*ii)->NumSamples() > 1)
			{
				(*ii)->ConstructTree();
			}
			++ii;
		}
	}

	while (ii != m_Children.end())
	{
		if (*ii != NULL)
		{
			delete *ii;
			*ii = NULL;
		}
		ii++;
	}
}


void CqOcclusionTree::InitialiseBounds()
{
	if (m_SampleIndices.size() < 1)
		return;

	const SqSampleData& sample = CqBucket::ImageElement(m_SampleIndices[0].first).SampleData(m_SampleIndices[0].second);
	TqFloat minXVal = sample.m_Position.x();
	TqFloat maxXVal = minXVal;
	TqFloat minYVal = sample.m_Position.y();
	TqFloat maxYVal = minYVal;
	std::vector<std::pair<TqInt, TqInt> >::iterator i;
	for(i = m_SampleIndices.begin()+1; i!=m_SampleIndices.end(); ++i)
	{
		const SqSampleData& sample = CqBucket::ImageElement(i->first).SampleData(i->second);
		minXVal = MIN(minXVal, sample.m_Position.x());
		maxXVal = MAX(maxXVal, sample.m_Position.x());
		minYVal = MIN(minYVal, sample.m_Position.y());
		maxYVal = MAX(maxYVal, sample.m_Position.y());
	}
	m_MinSamplePoint[0] = minXVal;
	m_MaxSamplePoint[0] = maxXVal;
	m_MinSamplePoint[1] = minYVal;
	m_MaxSamplePoint[1] = maxYVal;

	// Set the opaque depths to the limits to begin with.
	m_MaxOpaqueZ = FLT_MAX;
}


void CqOcclusionTree::UpdateBounds()
{
	if (m_Children[0])
	{
		assert(m_SampleIndices.size() > 1);

		TqChildArray::iterator child = m_Children.begin();
		(*child)->UpdateBounds();

		m_MinSamplePoint[0] = (*child)->m_MinSamplePoint[0];
		m_MaxSamplePoint[0] = (*child)->m_MaxSamplePoint[0];
		m_MinSamplePoint[1] = (*child)->m_MinSamplePoint[1];
		m_MaxSamplePoint[1] = (*child)->m_MaxSamplePoint[1];

		for(++child; child != m_Children.end(); ++child)
		{
			if (*child)
			{
				(*child)->UpdateBounds();

				m_MinSamplePoint[0] = std::min(m_MinSamplePoint[0], (*child)->m_MinSamplePoint[0]);
				m_MaxSamplePoint[0] = std::max(m_MaxSamplePoint[0], (*child)->m_MaxSamplePoint[0]);
				m_MinSamplePoint[1] = std::min(m_MinSamplePoint[1], (*child)->m_MinSamplePoint[1]);
				m_MaxSamplePoint[1] = std::max(m_MaxSamplePoint[1], (*child)->m_MaxSamplePoint[1]);
			}
		}
	}
	else
	{
		assert(m_SampleIndices.size() == 1);

		const SqSampleData& sample = Sample();
		m_MinSamplePoint[0] = m_MaxSamplePoint[0] = sample.m_Position[0];
		m_MinSamplePoint[1] = m_MaxSamplePoint[1] = sample.m_Position[1];
	}

	// Set the opaque depths to the limits to begin with.
	m_MaxOpaqueZ = FLT_MAX;
}

void CqOcclusionTree::PropagateChanges()
{
	CqOcclusionTreePtr node = this/*shared_from_this()*/;
	// Update our opaque depth based on that our our children.
	while(node)
	{
		if( node->m_Children[0] )
		{
			TqFloat maxdepth = node->m_Children[0]->m_MaxOpaqueZ;
			TqChildArray::iterator child = node->m_Children.begin();
			for (++child; child != node->m_Children.end(); ++child)
			{
				if (*child)
				{
					maxdepth = MAX((*child)->m_MaxOpaqueZ, maxdepth);
				}
			}
			// Only if this has resulted in a change at this level, should we process the parent.
			if(maxdepth < node->m_MaxOpaqueZ)
			{
				node->m_MaxOpaqueZ = maxdepth;
				node = node->m_Parent/*.lock()*/;
			}
			else
			{
				break;
			}
		}
		else
		{
			node = node->m_Parent/*.lock()*/;
		}
	}
}


bool CqOcclusionTree::CanCull( CqBound* bound )
{
	// Recursively call each level to see if it can be culled at that point.
	// Stop recursing at a level that doesn't contain the whole bound.
	std::deque<CqOcclusionTree*> stack;
	stack.push_front(this);
	bool	top_level = true;
	while(!stack.empty())
	{
		CqOcclusionTree* node = stack.front();
		stack.pop_front();
		// Check the bound against the 2D limits of this level, if not entirely contained, then we
		// cannot cull at this level, nor at any of the children.
		CqBound b1(node->MinSamplePoint(), node->MaxSamplePoint());
		if(b1.Contains2D(bound) || top_level)
		{
			top_level = false;
			if( bound->vecMin().z() > node->MaxOpaqueZ() )
				// If the bound is entirely contained within this node's 2D bound, and is further
				// away than the furthest opaque point, then cull.
				return(true);
			// If contained, but not behind the furthest point, push the children nodes onto the stack for
			// processing.
			CqOcclusionTree::TqChildArray::iterator childNode;
			for (childNode = node->m_Children.begin(); childNode != node->m_Children.end(); ++childNode)
			{
				if (*childNode)
				{
					stack.push_front(*childNode/*->get()*/);
				}
			}
		}
	}
	return(false);
}


//----------------------------------------------------------------------
// Static Variables

CqBucket* CqOcclusionBox::m_Bucket = NULL;

bool CqOcclusionTree::CqOcclusionTreeComparator::operator()(const std::pair<TqInt, TqInt>& a, const std::pair<TqInt, TqInt>& b)
{
	const SqSampleData& A = CqBucket::ImageElement(a.first).SampleData(a.second);
	const SqSampleData& B = CqBucket::ImageElement(b.first).SampleData(b.second);
	return( A.m_Position[m_Dim] < B.m_Position[m_Dim] );
}


CqOcclusionTreePtr	CqOcclusionBox::m_KDTree;	///< KD Tree representing the samples in the bucket.


//----------------------------------------------------------------------
/** Constructor
*/

CqOcclusionBox::CqOcclusionBox()
{}


//----------------------------------------------------------------------
/** Destructor
*/

CqOcclusionBox::~CqOcclusionBox()
{}



//----------------------------------------------------------------------
/** Delete the static hierarchy created in CreateHierachy(). static.
*/
void CqOcclusionBox::DeleteHierarchy()
{
	delete m_KDTree;
	m_KDTree = NULL;
}


//----------------------------------------------------------------------
/** Setup the hierarchy for one bucket. Static.
	This should be called before rendering each bucket
 *\param bucket: the bucket we are about to render
 *\param xMin: left edge of this bucket (taking into account crop windows etc)
 *\param yMin: Top edge of this bucket
 *\param xMax: Right edge of this bucket
 *\param yMax: Bottom edge of this bucket
*/

void CqOcclusionBox::SetupHierarchy( CqBucket* bucket, TqInt xMin, TqInt yMin, TqInt xMax, TqInt yMax )
{
	assert( bucket );
	m_Bucket = bucket;

	if(!m_KDTree)
	{
		m_KDTree = CqOcclusionTreePtr(new CqOcclusionTree());
		// Setup the KDTree of samples
		TqInt numpixels = bucket->RealHeight() * bucket->RealWidth();
		TqInt numsamples = bucket->PixelXSamples() * bucket->PixelYSamples();
		for ( TqInt j = 0; j < numpixels; j++ )
		{
			// Gather all samples within the pixel
			for ( TqInt i = 0; i < numsamples; i++ )
			{
				m_KDTree->AddSample(std::pair<TqInt, TqInt>(j,i));
			}
		}
		// Now split the tree down until each leaf has only one sample.
		m_KDTree->InitialiseBounds();
		m_KDTree->ConstructTree();
	}

	m_KDTree->UpdateBounds();
}


bool CqOcclusionBox::CanCull( CqBound* bound )
{
	return(m_KDTree->CanCull(bound));
}

END_NAMESPACE( Aqsis )

