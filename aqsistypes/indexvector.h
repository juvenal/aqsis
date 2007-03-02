#ifndef RUNNINGSTATE_H_INCLUDED
#define RUNNINGSTATE_H_INCLUDED

#include <vector>
#include <iostream>

#include "aqsis.h"
#include "bitvector.h"

namespace Aqsis {

//------------------------------------------------------------------------------
class  CqIndexVector
{
	public:
		typedef TqUint value_type;
		typedef std::vector<value_type>::size_type size_type;
		typedef std::vector<value_type>::const_iterator iterator;

		CqIndexVector(TqInt maxLength = 0);
		// Use default copy constructor.
		CqIndexVector(CqBitVector& bitVector);
		void intersect(const CqIndexVector& from);
		void complement();
		void unionRS(const CqIndexVector& from);
		void fromBitVector(CqBitVector& bv);
		iterator begin() const;
		iterator end() const;
		size_type size() const;
		size_type maxSize() const;
		// Use default assignment operator
		value_type operator[](const size_type idx) const;

		friend std::ostream& operator<<(std::ostream &stream, CqIndexVector& rsOut);
	private:
		typedef std::vector<value_type> RsVector;
		RsVector m_data;
		size_type m_maxLength;
};


//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
// Implementation for CqIndexVector inline functions.
inline CqIndexVector::CqIndexVector(TqInt maxLength)
	: m_data(),
	m_maxLength(maxLength)
{
	m_data.reserve(maxLength);
}

inline CqIndexVector::iterator CqIndexVector::begin() const
{
	return m_data.begin();
}

inline CqIndexVector::iterator CqIndexVector::end() const
{
	return m_data.end();
}

inline CqIndexVector::size_type CqIndexVector::size() const
{
	return m_data.size();
}

inline CqIndexVector::size_type CqIndexVector::maxSize() const
{
	return m_maxLength;
}

inline CqIndexVector::value_type CqIndexVector::operator[](const size_type idx) const
{
	return m_data[idx];
}

//------------------------------------------------------------------------------
} // namespace Aqsis

#endif // !RUNNINGSTATE_H_INCLUDED
