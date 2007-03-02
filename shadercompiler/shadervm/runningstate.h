#ifndef RUNNINGSTATE_H_INCLUDED
#define RUNNINGSTATE_H_INCLUDED

#include <vector>
#include <iostream>

#include "aqsis.h"
#include "bitvector.h"

namespace Aqsis {

//------------------------------------------------------------------------------
class  CqRunningState
{
	public:
		typedef TqUint value_type;
		typedef std::vector<value_type>::size_type size_type;
		typedef std::vector<value_type>::const_iterator iterator;

		CqRunningState(TqInt maxLength = 0);
		// Use default copy constructor.
		CqRunningState(CqBitVector& bitVector);
		void intersect(const CqRunningState& from);
		void complement();
		void unionRS(const CqRunningState& from);
		void fromBitVector(CqBitVector& bv);
		iterator begin() const;
		iterator end() const;
		size_type size() const;
		size_type maxSize() const;
		// Use default assignment operator
		value_type operator[](const size_type idx) const;

		friend std::ostream& operator<<(std::ostream &stream, CqRunningState& rsOut);
	private:
		typedef std::vector<value_type> RsVector;
		RsVector m_data;
		size_type m_maxLength;
};


//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
// Implementation for CqRunningState inline functions.
inline CqRunningState::CqRunningState(TqInt maxLength)
	: m_data(),
	m_maxLength(maxLength)
{
	m_data.reserve(maxLength);
}

inline CqRunningState::iterator CqRunningState::begin() const
{
	return m_data.begin();
}

inline CqRunningState::iterator CqRunningState::end() const
{
	return m_data.end();
}

inline CqRunningState::size_type CqRunningState::size() const
{
	return m_data.size();
}

inline CqRunningState::size_type CqRunningState::maxSize() const
{
	return m_maxLength;
}

inline CqRunningState::value_type CqRunningState::operator[](const size_type idx) const
{
	return m_data[idx];
}

//------------------------------------------------------------------------------
} // namespace Aqsis

#endif // !RUNNINGSTATE_H_INCLUDED
