#ifndef RUNNINGSTATE_H_INCLUDED
#define RUNNINGSTATE_H_INCLUDED

#include <vector>

#include "aqsis.h"

namespace Aqsis {

//-----------------------------------------------------------------------
class  CqRunningState
{
	public:
		typedef TqInt value_type;
		typedef std::vector<value_type> RsVector;
		typedef RsVector::size_type size_type;
		typedef RsVector::iterator iterator;

		CqRunningState(TqInt length = 0)
			: m_data(),
			m_length(length)
		{
			m_data.reserve(length);
		}
		void Intersect(const CqRunningState& from);
		void Complement();
		void fromBitVector(CqBitVector& bv);
		iterator begin()
		{
			return m_data.begin();
		}
		iterator end()
		{
			return m_data.end();
		}
		CqRunningState& operator=(const CqRunningState& from)
		{
			m_data = from.m_data;
			m_length = from.m_length;
			return *this;
		}
		value_type operator[](const size_type idx)
		{
			return m_data[idx];
		}
	private:
		RsVector m_data;
		TqInt	m_length;
};

//-----------------------------------------------------------------------

} // namespace Aqsis

#endif // !RUNNINGSTATE_H_INCLUDED
