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
 * \brief Texture cache implementation.
 *
 * \author Chris Foster [ chris42f (at) gmail (dot) com ]
 */

#include "texturecache.h"

#include "exception.h"
#include "file.h"
#include "ishadowsampler.h"
#include "itexinputfile.h"
#include "itexturesampler.h"
#include "logging.h"
#include "sstring.h"
#include "texexception.h"

namespace Aqsis {

//------------------------------------------------------------------------------
// CqTextureCache

CqTextureCache::CqTextureCache(TqSearchPathCallback searchPathCallback)
	: m_textureCache(),
	m_shadowCache(),
	m_camToWorld(),
	m_searchPathCallback(searchPathCallback)
{ }

IqTextureSampler& CqTextureCache::findTextureSampler(const char* name)
{
	return findSampler(m_textureCache, name);
}

IqShadowSampler& CqTextureCache::findShadowSampler(const char* name)
{
	return findSampler(m_shadowCache, name);
}

CqTexFileHeader* CqTextureCache::textureInfo(const char* texName)
{
	/// \todo Return something sensible here.
	assert(0);
	return 0;
}

void CqTextureCache::setCamToWorldMatrix(const CqMatrix& camToWorld)
{
	m_camToWorld = camToWorld;
}

//--------------------------------------------------
// Private methods
template<typename SamplerT>
SamplerT& CqTextureCache::findSampler(
		std::map<TqUlong, boost::shared_ptr<SamplerT> >& samplerMap,
		const char* name)
{
	TqUlong hash = CqString::hash(name);
	typename std::map<TqUlong, boost::shared_ptr<SamplerT> >::const_iterator
		texIter = samplerMap.find(hash);
	if(texIter != samplerMap.end())
	{
		// The desired texture sampler is already created - return it.
		return *(texIter->second);
	}
	else
	{
		// Couldn't find in the currently open texture samplers - create a new
		// instance.
		boost::shared_ptr<SamplerT> newTex;
		try
		{
			newTex = newSamplerFromFile<SamplerT>(
					findFileInPath(name, m_searchPathCallback()).c_str() );
		}
		catch(XqInvalidFile& e)
		{
			Aqsis::log() << warning
				<< "Could not open file: \"" << name << "\": " << e.what() << "\n";
			/// \todo Use newDummySampler() here.
			throw e;
		}
		catch(XqBadTexture& e)
		{
			Aqsis::log() << warning
				<< "Bad texture file: \"" << name << "\": " << e.what() << "\n";
			throw e;
		}
		samplerMap[CqString::hash(name)] = newTex;
		return *newTex;
	}
}

template<typename SamplerT>
boost::shared_ptr<SamplerT> CqTextureCache::newSamplerFromFile(
		const char* name)
{
	return SamplerT::create(IqMultiTexInputFile::open(name));
}

// Special case of newSamplerFromFile() for shadow maps - they need access to
// the camera->world transformation matrix.
template<>
boost::shared_ptr<IqShadowSampler>
CqTextureCache::newSamplerFromFile(const char* name)
{
	return IqShadowSampler::create(IqTexInputFile::open(name), m_camToWorld);
}

template<typename SamplerT>
boost::shared_ptr<SamplerT> newDummySampler()
{
	return boost::shared_ptr<SamplerT>();
};

} // namespace Aqsis
