#include "runningstate.h"
#include <algorithm>

namespace Aqsis {

//----------------------------------------------------------------------
void CqRunningState::Intersect(const CqRunningState& from)
{
	CqRunningState::RsVector oldData = m_data;
	CqRunningState::RsVector::iterator dataEnd = std::set_intersection(oldData.begin(), oldData.end(),
			from.m_data.begin(), from.m_data.end(), m_data.begin());
	m_data.erase(dataEnd, m_data.end());
}


//----------------------------------------------------------------------
void CqRunningState::Complement()
{
	CqRunningState::RsVector oldData = m_data;
	m_data.resize(m_length - m_data.size());
	CqRunningState::RsVector::iterator newI = m_data.begin();
	CqRunningState::RsVector::iterator oldI = oldData.begin();
	for(TqInt j = 0; j < m_length; j++)
	{
		if(oldI != oldData.end() && j == *oldI)
			oldI++;
		else
		{
			*newI = j;
			newI++;
		}
	}
}


//----------------------------------------------------------------------
void CqRunningState::fromBitVector(CqBitVector& bv)
{
	m_data.resize(bv.Count());
	CqRunningState::RsVector::iterator i = m_data.begin();
	for(TqInt j = 0; j < bv.Size(); j++)
	{
		if(bv.Value(j))
		{
			*i = j;
			i++;
		}
	}
}

//----------------------------------------------------------------------

} // namespace Aqsis
