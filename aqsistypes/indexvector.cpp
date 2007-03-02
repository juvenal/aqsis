#include <algorithm>
#include <iterator>
#include <assert.h>

#include "indexvector.h"

namespace Aqsis {

//------------------------------------------------------------------------------
CqIndexVector::CqIndexVector(CqBitVector& bitVector)
	: m_data(),
	m_maxLength(0)
{
	fromBitVector(bitVector);
}

//------------------------------------------------------------------------------
void CqIndexVector::intersect(const CqIndexVector& from)
{
	CqIndexVector::RsVector oldData = m_data;
	m_data.clear();
	std::set_intersection(oldData.begin(), oldData.end(),
			from.m_data.begin(), from.m_data.end(), std::back_inserter(m_data));
}

//------------------------------------------------------------------------------
void CqIndexVector::complement()
{
	CqIndexVector::RsVector oldData = m_data;
	m_data.clear();
	CqIndexVector::RsVector::iterator oldI = oldData.begin();
	for(TqUint j = 0; j < m_maxLength; j++)
	{
		if(oldI != oldData.end() && j == *oldI)
			oldI++;
		else
			m_data.push_back(j);
	}
}

//------------------------------------------------------------------------------
void CqIndexVector::unionRS(const CqIndexVector& from)
{
	CqIndexVector::RsVector oldData = m_data;
	m_data.clear();
	std::set_union(oldData.begin(), oldData.end(),
			from.m_data.begin(), from.m_data.end(), std::back_inserter(m_data));
}

//------------------------------------------------------------------------------
void CqIndexVector::fromBitVector(CqBitVector& bv)
{
	m_maxLength = bv.Size();
	m_data.reserve(m_maxLength);
	m_data.resize(bv.Count());
	CqIndexVector::RsVector::iterator i = m_data.begin();
	for(TqUint j = 0; j < m_maxLength; j++)
	{
		if(bv.Value(j))
		{
			*i = j;
			i++;
		}
	}
}

//------------------------------------------------------------------------------
std::ostream& operator<<( std::ostream &stream, CqIndexVector& rsOut)
{
	for(CqIndexVector::RsVector::iterator itr = rsOut.m_data.begin();
			itr != rsOut.m_data.end(); itr++)
		stream << *itr << " ";
	return stream;
}
//------------------------------------------------------------------------------

} // namespace Aqsis
