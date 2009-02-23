// hairgen procedural
// Copyright (C) 2008 Christopher J. Foster [chris42f (at) gmail (d0t) com]
//
// This software is licensed under the GPLv2 - see the file COPYING for details.

#ifndef HAIRGEN_REQUESTS_H_INCLUDED
#define HAIRGEN_REQUESTS_H_INCLUDED

#include <algorithm>
#include <cassert>
#include <iostream>
#include <vector>
#include <stdexcept>

#include <boost/shared_ptr.hpp>

#include <primvartoken.h>
#include <ri.h>

#include "util.h"

//------------------------------------------------------------------------------
/** A renderman token-value pair
 *
 * This may be used to represent variables attached to any renderman primitive,
 * otherwise known as "primitive variables" or "primvars".
 */
template<typename T>
struct TokValPair
{
	/// name, type, etc of the primvar
	Aqsis::CqPrimvarToken token;
	/// array of values for the primvar.
	boost::shared_ptr<T> value;

	/// constructor, *copying* the provided value.
	TokValPair(const Aqsis::CqPrimvarToken& token, const T& value)
		: token(token), value(new T(value)) {}
	/// constructor creating a new empty value type.
	TokValPair(const Aqsis::CqPrimvarToken& token)
		: token(token), value(new T()) {}
};


/// token value pairs with float arrays as the value.
typedef TokValPair<FloatArray> TokFloatValPair;
typedef std::vector<TokFloatValPair> FloatPrimVars;


//------------------------------------------------------------------------------
/** A vector-like container for primvars.
 *
 * This class is a fairly simple container supporting searching through the
 * primvars using various keys, and iterators to walk over the set of primvars.
 */
class PrimVars
{
	public:
		/// Iterator types for primvars
		typedef FloatPrimVars::const_iterator const_iterator;
		typedef FloatPrimVars::iterator iterator;

		// Find primvars by name
		//   Methods returning pointers return null if not found;
		//   Methods returning references throw if not found.
		const FloatArray* findPtr(const std::string& name) const;
		const FloatArray& find(const std::string& name) const;
		const FloatArray* findPtr(const Aqsis::CqPrimvarToken& tok) const;
		const FloatArray& find(const Aqsis::CqPrimvarToken& tok) const;

		FloatArray* findPtr(const std::string& name);
		FloatArray& find(const std::string& name);
		FloatArray* findPtr(const Aqsis::CqPrimvarToken& tok);
		FloatArray& find(const Aqsis::CqPrimvarToken& tok);

		/// Convenience function appending the given (token,value) pair to the array.
		void append(const Aqsis::CqPrimvarToken& token,
				const FloatArray& value = FloatArray());

		// Iterator access
		const_iterator begin() const;
		iterator begin();
		const_iterator end() const;
		iterator end();

		/// Number of primvars
		int size() const;
		/// Indexing
		TokFloatValPair& operator[](int i);
		const TokFloatValPair& operator[](int i) const;
		/// Access to the primvar last appended to the container.
		TokFloatValPair& back();

	private:
		FloatPrimVars m_vars;

		template<typename T>
		const FloatArray* findPtrImpl(const T& id) const;
		template<typename T>
		const FloatArray& findImpl(const T& id) const;
};


//------------------------------------------------------------------------------
/** A class converting values held in a PrimVars array into primvar token and
 * value arrays compatible with the traditional renderman interface.
 */
class ParamList
{
	private:
		std::vector<std::string> m_tokenStorage;
		std::vector<RtToken> m_tokens;
		std::vector<RtPointer> m_values;

	public:
		/** Take a set of primvars held in a PrimVars class, and convert it
		 * into the tranditional RI representation of a set of primvars.
		 *
		 * The representation is a set of bare pointers to the value arrays
		 * held by primVars, so arrays held in primVars should *not* be
		 * modified afterward if the representation is to remain valid.
		 */
		ParamList(const PrimVars& primVars)
			: m_tokenStorage(),
			m_tokens(),
			m_values()
		{
			for(PrimVars::const_iterator i = primVars.begin(); i != primVars.end(); ++i)
			{
				std::ostringstream out;
				out << i->token.Class() << " "
					<< i->token.type() << " "
					<< "[" << i->token.count() << "] "
					<< i->token.name();
				m_tokenStorage.push_back(out.str());
				m_tokens.push_back(const_cast<RtToken>(m_tokenStorage.back().c_str()));
				m_values.push_back(reinterpret_cast<RtPointer>(&(*i->value)[0]));
			}
		}

		/// Total number of primvars
		const int count() const
		{
			return m_tokens.size();
		}
		/// Get a bare array of RI token pointers
		RtToken* tokens() const
		{
			return const_cast<RtToken*>(&m_tokens[0]);
		}
		/// Get a bare array of RI value pointers
		RtPointer* values() const
		{
			return const_cast<RtPointer*>(&m_values[0]);
		}
};


//==============================================================================
// Implementation details
//==============================================================================

// for finding names in arrays of token value pairs.
template<typename T>
inline bool operator==(const TokValPair<T>& pair, const std::string& name)
{
	return pair.token.name() == name;
}
template<typename T>
inline bool operator==(const TokValPair<T>& pair, const Aqsis::CqPrimvarToken& tok)
{
	return pair.token == tok;
}


//------------------------------------------------------------------------------
// PrimVars implementation

inline void PrimVars::append(const Aqsis::CqPrimvarToken& token,
		const FloatArray& value)
{
	m_vars.push_back(TokFloatValPair(token, value));
}

inline const FloatArray* PrimVars::findPtr(const std::string& name) const
{
	return findPtrImpl(name);
}
inline const FloatArray& PrimVars::find(const std::string& name) const
{
	return findImpl(name);
}

inline const FloatArray* PrimVars::findPtr(
		const Aqsis::CqPrimvarToken& tok) const
{
	return findPtrImpl(tok);
}
inline const FloatArray& PrimVars::find(
		const Aqsis::CqPrimvarToken& tok) const
{
	return findImpl(tok);
}

inline FloatArray* PrimVars::findPtr(const std::string& name)
{
	return const_cast<FloatArray*>(findPtrImpl(name));
}
inline FloatArray& PrimVars::find(const std::string& name)
{
	return const_cast<FloatArray&>(findImpl(name));
}
inline FloatArray* PrimVars::findPtr(const Aqsis::CqPrimvarToken& tok)
{
	return const_cast<FloatArray*>(findPtrImpl(tok));
}
inline FloatArray& PrimVars::find(const Aqsis::CqPrimvarToken& tok)
{
	return const_cast<FloatArray&>(findImpl(tok));
}

inline PrimVars::const_iterator PrimVars::begin() const
{
	return m_vars.begin();
}
inline PrimVars::iterator PrimVars::begin()
{
	return m_vars.begin();
}
inline PrimVars::const_iterator PrimVars::end() const
{
	return m_vars.end();
}
inline PrimVars::iterator PrimVars::end()
{
	return m_vars.end();
}
inline TokFloatValPair& PrimVars::back()
{
	return m_vars.back();
}
inline TokFloatValPair& PrimVars::operator[](int i)
{
	return m_vars[i];
}
inline const TokFloatValPair& PrimVars::operator[](int i) const
{
	return m_vars[i];
}

inline int PrimVars::size() const
{
	return m_vars.size();
}

/// Search for a primvar, returning a pointer or null if not found.
template<typename T>
inline const FloatArray* PrimVars::findPtrImpl(const T& id) const
{
	// dumb linear search...  The primvars array is expected to be pretty short
	// in pretty much all cases so this should be ok.
	FloatPrimVars::const_iterator tokValPair = 
		std::find(m_vars.begin(), m_vars.end(), id);
	if(tokValPair == m_vars.end())
		return 0;
	return &(*tokValPair->value);
}

/// Search for a primvar, returning a reference or throwing if not found.
template<typename T>
inline const FloatArray& PrimVars::findImpl(const T& id) const
{
	const FloatArray* var = findPtr(id);
	if(!var)
		throw std::runtime_error("Primvar not found");
	return *var;
}



#endif // HAIRGEN_REQUESTS_H_INCLUDED
