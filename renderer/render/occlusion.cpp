// Aqsis
// Copyright (C) 1997 - 2002, Paul C. Gregory
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

std::vector<TqFloat>	CqOcclusionBox::m_depthTree;


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


//------------------------------------------------------------------------------
/** Return the tree index for leaf node containing the given point.
 *
 * treeDepth - depth of the tree, (root node has treeDepth == 1).
 *
 * p - a point which we'd like the index for.  We assume that the possible
 *     points lie in the box [0,1) x [0,1)
 *
 * Returns an index for a 0-based array.
 */
TqUint treeIndexForPoint_fast(TqUint treeDepth, const CqVector2D& p)
{
	assert(treeDepth > 0);
	assert(p.x() >= 0 && p.x() <= 1);
	assert(p.y() >= 0 && p.y() <= 1);

	const TqUint numXSubdivisions = treeDepth / 2;
	const TqUint numYSubdivisions = (treeDepth-1) / 2;
	// true if the last subdivison was along the x-direction.
	const bool lastSubdivisionInX = (treeDepth % 2 == 0);

	// integer coordinates of the point in terms of the subdivison which it
	// falls into, staring from 0 in the top left.
	TqUint x = static_cast<int>(floor(p.x() * (1 << numXSubdivisions)));
	TqUint y = static_cast<int>(floor(p.y() * (1 << numYSubdivisions)));

	// This is the base coordinate for the first leaf in a tree of depth "treeDepth".
	TqUint index = 1 << (treeDepth-1);
	if(lastSubdivisionInX)
	{
		// Every second bit of the index (starting with the LSB) should make up
		// the coordinate x, so distribute x into index in that fashion.
		//
		// Similarly for y, except alternating with the bits of x (starting
		// from the bit up from the LSB.)
		for(TqUint i = 0; i < numXSubdivisions; ++i)
		{
			index |= (x & (1 << i)) << i
				| (y & (1 << i)) << (i+1);
		}
	}
	else
	{
		// This is the opposite of the above: x and y are interlaced as before,
		// but now the LSB of y rather than x goes into the LSB of index.
		for(TqUint i = 0; i < numYSubdivisions; ++i)
		{
			index |= (y & (1 << i)) << i
				| (x & (1 << i)) << (i+1);
		}
	}
	return index - 1;
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

	// Now setup the new array based tree.
	// First work out how deep the tree needs to be.
	TqLong numLeafNodes = (bucket->RealHeight() * bucket->RealWidth()) * (bucket->PixelXSamples() * bucket->PixelYSamples());
	TqLong depth = lceil(log10(static_cast<double>(numLeafNodes))/log10(2.0));
	TqLong numTotalNodes = lceil(pow(2.0, static_cast<double>(depth+1)))-1;
	m_depthTree.resize(numTotalNodes, 0.0f);

	// Now initialise the node depths of those that contain sample points to infinity.
	std::vector<SqSampleData>& samples = bucket->SamplePoints();
	std::vector<SqSampleData>::iterator sample;
	for(sample = samples.begin(); sample != samples.end(); ++sample)
	{
		CqVector2D samplePos = sample->m_Position;
		samplePos.x((samplePos.x() - static_cast<TqFloat>(bucket->realXOrigin())) / static_cast<TqFloat>(bucket->RealWidth()));
		samplePos.y((samplePos.y() - static_cast<TqFloat>(bucket->realYOrigin())) / static_cast<TqFloat>(bucket->RealHeight()));
		TqUint sampleNodeIndex = treeIndexForPoint_fast(depth+1, samplePos);

		// Check that the index is within the tree
		assert(sampleNodeIndex < numTotalNodes);
		// Check that the index is a leaf node.
		assert((sampleNodeIndex*2)+1 >= numTotalNodes);

		m_depthTree[sampleNodeIndex] = FLT_MAX;
		TqInt parentIndex = (sampleNodeIndex-1)>>1;
		while(parentIndex >= 0)
		{
			m_depthTree[parentIndex] = FLT_MAX;
			parentIndex = (parentIndex - 1)>>1;
		}
		sample->m_occlusionIndex = sampleNodeIndex;
	} 
}

	struct SqNodeStack
	{
		SqNodeStack(TqFloat _minX, TqFloat _minY, TqFloat _maxX, TqFloat _maxY, TqInt _index, bool _splitInX) :
			minX(_minX), minY(_minY), maxX(_maxX), maxY(_maxY), index(_index), splitInX(_splitInX) {}
		TqFloat minX, minY;
		TqFloat maxX, maxY;
		TqInt	index;
		bool	splitInX;
	};

bool CqOcclusionBox::CanCull( CqBound* bound )
{
	bool oldCull = m_KDTree->CanCull(bound);
	// Check the new system.
	// Normalize the bound
	TqFloat minX = m_Bucket->realXOrigin();
	TqFloat minY = m_Bucket->realYOrigin(); 
	TqFloat maxX = m_Bucket->realXOrigin() + m_Bucket->RealWidth();
	TqFloat maxY = m_Bucket->realYOrigin() + m_Bucket->RealHeight();
	// If the test bound is bigger than the overall bucket, then crop it.
	TqFloat tminX = std::max(bound->vecMin().x(), minX);
	TqFloat tminY = std::max(bound->vecMin().y(), minY);
	TqFloat tmaxX = std::min(bound->vecMax().x(), maxX);
	TqFloat tmaxY = std::min(bound->vecMax().y(), maxY);

	std::deque<SqNodeStack> stack;
	stack.push_front(SqNodeStack(minX, minY, maxX, maxY, 0, true));
	bool newCull = false;
	SqNodeStack terminatingNode(0.0f, 0.0f, 0.0f, 0.0f, 0, true);
	
	while(!stack.empty())
	{
		SqNodeStack node = stack.front();
		stack.pop_front();

		if ( ( tminX >= node.minX && tmaxX <= node.maxX ) &&
			 ( tminY >= node.minY && tmaxY <= node.maxY ) )
		{
			if(bound->vecMin().z() > m_depthTree[node.index]) 
			{
				terminatingNode = node;
				newCull = true;
				break;
			}

			if(node.index * 2 + 1 >= m_depthTree.size())
				continue;

			if(node.splitInX)
			{
				TqFloat median = ((node.maxX - node.minX) * 0.5f) + node.minX;
				stack.push_front(SqNodeStack(node.minX, node.minY, median, node.maxY, node.index * 2 + 1, !node.splitInX));
				stack.push_front(SqNodeStack(median, node.minY, node.maxX, node.maxY, node.index * 2 + 2, !node.splitInX));
			}
			else
			{
				TqFloat median = ((node.maxY - node.minY) * 0.5f) + node.minY;
				stack.push_front(SqNodeStack(node.minX, node.minY, node.maxX, median, node.index * 2 + 1, !node.splitInX));
				stack.push_front(SqNodeStack(node.minX, median, node.maxX, node.maxY, node.index * 2 + 2, !node.splitInX));
			}
		}
	}
	if(oldCull != newCull)
		Aqsis::log() << error << "Culling mismatch : " << oldCull << " - " << newCull << std::endl;
	else
		Aqsis::log() << debug << "Culling match : " << oldCull << " - " << newCull << std::endl;
	return(newCull);
}

void CqOcclusionBox::UpdateDepth(TqInt index, TqFloat depth)
{
	assert(index < m_depthTree.size());
	// Check that we're only updating leaf nodes.
	assert((index*2)+1 >= m_depthTree.size());

	m_depthTree[index] = std::min(m_depthTree[index], depth);

	TqInt parentIndex = (index - 1)>>1;
	while(parentIndex >= 0)
	{
		m_depthTree[parentIndex] = std::max(m_depthTree[(parentIndex<<1)+1], m_depthTree[(parentIndex<<1)+2]);
		parentIndex = (parentIndex - 1)>>1;
	}
}

END_NAMESPACE( Aqsis )

