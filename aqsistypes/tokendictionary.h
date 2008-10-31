// Aqsis
// Copyright (C) 1997 - 2007, Paul C. Gregory
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
 *
 * \brief A dictionary in which to look up primitive variable tokens.
 * \author Chris Foster [chris42f (at) g mail (d0t) com]
 */

#ifndef TOKENDICTIONARY_H_INCLUDED
#define TOKENDICTIONARY_H_INCLUDED

#include "aqsis.h"

#include <map>

#include "primvartoken.h"

namespace Aqsis
{

//------------------------------------------------------------------------------
/** \brief A dictionary in which to look up primitive variables.
 */
class CqTokenDictionary
{
	public:
		/** \brief Construct a token dictionary
		 *
		 * \param useDefaultVars - if true, fill the dictionary with the set of
		 * default primvars as returned by the standardPrimvars() function.
		 * If false, construct an empty dictionary.
		 */
		CqTokenDictionary(bool useDefaultVars = true);

		/** \brief Insert the token into the dictionary
		 *
		 * \note that this function *replaces* any existing primvar with the
		 * same name in the dictionary.
		 */
		void insert(const CqPrimvarToken& token);

		/** \brief Look and assign the primvar type.
		 *
		 * This method only has an effect if the input token previously
		 * lacked a type, as determined by CqPrimvarToken::hasType().
		 */
		void lookupType(CqPrimvarToken& token) const;

		/** \brief Find a primvar with the given name.
		 *
		 * \return the primvar, or 0 if the given name is not in the
		 * dictionary.
		 */
		const CqPrimvarToken* find(const std::string& name) const;
	private:
		typedef std::map<std::string, CqPrimvarToken> TqPrimvarMap;
		// TODO: Decide on whether there might be a better container to use
		// here - a hash map for instance.
		TqPrimvarMap m_dict;
};

/** \brief Get the list of standard predefined primitive variables.
 *
 * The returned vector includes predefined token declarations for tokens
 * involving:
 *   - Standard shader instance variables (eg, "Ka")
 *   - Standar primvars (eg, "P")
 *   - Arguments for standard attributes and options (eg, "gridsize")
 *   - Some aqsis-specific attributes and options.
 *
 * Note that the RISpec says nothing about which variables should be
 * predefined, so this list may be missing some variables.
 *
 * \return A vector of CqPrimvarTokens representing standard predefined tokens.
 */
const std::vector<CqPrimvarToken>& standardPrimvars();


//==============================================================================
// Implementation details.
//==============================================================================
inline void CqTokenDictionary::insert(const CqPrimvarToken& token)
{
	m_dict[token.name()] = token;
}

inline void CqTokenDictionary::lookupType(CqPrimvarToken& token) const
{
	if(!token.hasType())
	{
		TqPrimvarMap::const_iterator pos = m_dict.find(token.name());
		if(pos != m_dict.end())
			token = pos->second;
	}
}

inline const CqPrimvarToken* CqTokenDictionary::find(const std::string& name) const
{
	TqPrimvarMap::const_iterator pos = m_dict.find(name);
	if(pos == m_dict.end())
		return 0;
	else
		return &pos->second;
}

} // namespace Aqsis

#endif // TOKENDICTIONARY_H_INCLUDED
