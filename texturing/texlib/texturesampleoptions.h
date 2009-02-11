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
 * \brief Define texture sampling options structures and associated enums
 *
 * \author Chris Foster [chris42f (at) gmail (d0t) com]
 */

#ifndef TEXTURESAMPLEOPTIONS_H_INCLUDED
#define TEXTURESAMPLEOPTIONS_H_INCLUDED

#include "aqsis.h"

#include "aqsismath.h"
#include "enum.h"
#include "samplequad.h"
#include "texfileheader.h"
#include "wrapmode.h"

namespace Aqsis
{

//-----------------------------------------------------------------------------
/** \brief Texture filter types
 *
 * The texture filters define the weighting functions for texture filtering.
 *
 * EWA (Elliptical Weighted Average) or gaussian filtering is a very high
 * quality analytical filter, so it's expected that most people will want this.
 *
 * It's not so easy to evaluate other filter types, except by Monte Carlo
 * integration of a bilinearly reconstructed texture.  This leads to noise, in
 * contrast to the EWA case, and is also worse for the cache in the naive
 * implementation.
 */
enum EqTextureFilter
{
	TextureFilter_Box,		///< box filtering
	TextureFilter_Gaussian, ///< gaussian filter (via EWA)
	TextureFilter_None,		///< no filtering; nearest-neighbour reconstruction.
	TextureFilter_Unknown	///< Unknown filter type.
};

AQSIS_ENUM_INFO_BEGIN(EqTextureFilter, TextureFilter_Unknown)
	"box",
	"gaussian",
	"none",
	"unknown"
AQSIS_ENUM_INFO_END

//-----------------------------------------------------------------------------
/** \brief Represent when to do linear interpolation between mipmap levels.
 */
enum EqMipmapLerp
{
	/// Never do linear interpolation between mipmap levels.
	Lerp_Never,
	/// Always do linear interpolation between mipmap levels.
	Lerp_Always,
	/// Decide whether to use linear interpolation automatically (recommended)
	Lerp_Auto
};


//-----------------------------------------------------------------------------
/** \brief Contain renderman texture sampling options common to all samplers.
 *
 * This class holds many of the sampling parameters which may be passed to the
 * texture() and environment() RSL calls.  Note that blur and width are left
 * out; the more specific sBlur,tBlur, etc are used instead.
 */
class CqTextureSampleOptionsBase
{
	public:
		/// Trivial constructor.
		CqTextureSampleOptionsBase(TqFloat sBlur, TqFloat tBlur, TqFloat sWidth,
				TqFloat tWidth, EqTextureFilter filterType, TqInt startChannel,
				TqInt numChannels, EqWrapMode sWrapMode, EqWrapMode tWrapMode);
		/** Default constructor
		 *
		 * Set all numerical quantities to 0 except for sWidth, tWidth,
		 * and numChannels which are set to 1.  The filter is set to gaussian,
		 * and wrap modes to black.
		 */
		CqTextureSampleOptionsBase();

		//--------------------------------------------------
		/// \name Accessors for relevant texture sample options.
		//@{
		/// Get the blur in the s-direction
		TqFloat sBlur() const;
		/// Get the blur in the t-direction
		TqFloat tBlur() const;
		/// Get the width multiplier in the s-direction
		TqFloat sWidth() const;
		/// Get the width multiplier in the t-direction
		TqFloat tWidth() const;
		/// Get the filter type
		EqTextureFilter filterType() const;
		/// Get the start channel index where channels will be read from.
		TqInt startChannel() const;
		/// Get the number of channels to sample
		TqInt numChannels() const;
		/// Get the wrap mode in the s-direction
		EqWrapMode sWrapMode() const;
		/// Get the wrap mode in the t-direction
		EqWrapMode tWrapMode() const;
		//@}

		//--------------------------------------------------
		/// \name Modifiers for texture sampling options.
		//@{
		/// Set the blur in both directions
		void setBlur(TqFloat blur);
		/// Set the blur in the s-direction
		void setSBlur(TqFloat sBlur);
		/// Set the blur in the t-direction
		void setTBlur(TqFloat tBlur);
		/// Set the width multiplier in all directions.
		void setWidth(TqFloat width);
		/// Set the width multiplier in the s-direction
		void setSWidth(TqFloat sWidth);
		/// Set the width multiplier in the t-direction
		void setTWidth(TqFloat tWidth);
		/// Set the filter type
		void setFilterType(EqTextureFilter type);
		/// Set the start channel index where channels will be read from.
		void setStartChannel(TqInt startChannel);
		/// Set the number of channels to sample
		void setNumChannels(TqInt numChans);
		/// Set the wrap mode in both directions
		void setWrapMode(EqWrapMode wrapMode);
		/// Set the wrap mode in the s-direction
		void setSWrapMode(EqWrapMode sWrapMode);
		/// Set the wrap mode in the t-direction
		void setTWrapMode(EqWrapMode tWrapMode);
		//@}

		/** \brief Extract sample options from a texture file header.
		 *
		 * As many relevant texture file attributes are extracted from the
		 * header as possible and used to fill the fields of the sample options
		 * object.
		 *
		 * \param header - extract options from here.
		 */
		void fillFromFileHeader(const CqTexFileHeader& header);

	protected:
		/// Texture blur amount in the s and t directions.  
		TqFloat m_sBlur;
		TqFloat m_tBlur;
		/// Filter widths as a multiple of the given filter box.
		TqFloat m_sWidth;
		TqFloat m_tWidth;
		/// Type of filter - represents both the filter weight function and filtering algorithm.
		EqTextureFilter m_filterType;
		/// Index of the starting channel (the first channel has index 0)
		TqInt m_startChannel;
		/// Number of channels to sample from the texture.
		TqInt m_numChannels;
		/// Wrap modes specifying what the sampler should do with (s,t) coordinates
		/// lying off the image edge.
		EqWrapMode m_sWrapMode;
		EqWrapMode m_tWrapMode;
};

//------------------------------------------------------------------------------
/** \brief Contain the standard renderman plain-texture sampling options
 *
 * The standard renderman shadow sampling options may be passed to calls to the
 * texture() and environment() builtin functions in the RSL.
 */
class CqTextureSampleOptions : private CqTextureSampleOptionsBase
{
	public:
		/** \brief Set all options to sensible default values
		 *
		 * The defaults from CqTextureSampleOptionsBase are used, and in addition,
		 * lerp is set to Lerp_Auto.
		 */
		CqTextureSampleOptions();

		// Accessors from CqTextureSampleOptionsBase
		CqTextureSampleOptionsBase::sBlur;
		CqTextureSampleOptionsBase::tBlur;
		CqTextureSampleOptionsBase::sWidth;
		CqTextureSampleOptionsBase::tWidth;
		CqTextureSampleOptionsBase::filterType;
		CqTextureSampleOptionsBase::startChannel;
		CqTextureSampleOptionsBase::numChannels;
		CqTextureSampleOptionsBase::sWrapMode;
		CqTextureSampleOptionsBase::tWrapMode;

		// Modifiers from CqTextureSampleOptionsBase
		CqTextureSampleOptionsBase::setBlur;
		CqTextureSampleOptionsBase::setSBlur;
		CqTextureSampleOptionsBase::setTBlur;
		CqTextureSampleOptionsBase::setWidth;
		CqTextureSampleOptionsBase::setSWidth;
		CqTextureSampleOptionsBase::setTWidth;
		CqTextureSampleOptionsBase::setFilterType;
		CqTextureSampleOptionsBase::setStartChannel;
		CqTextureSampleOptionsBase::setNumChannels;
		CqTextureSampleOptionsBase::setWrapMode;
		CqTextureSampleOptionsBase::setSWrapMode;
		CqTextureSampleOptionsBase::setTWrapMode;

		// Other stuff from CqTextureSampleOptionsBase
		CqTextureSampleOptionsBase::fillFromFileHeader;

		//--------------------------------------------------
		// Plain-texture specific sample options
		/// Get the fill value for texture channels indices outside the available range
		TqFloat fill() const;
		/** \brief Get the "lerp" flag indicating whether to perform interpolation
		 * between mipmap levels.
		 */
		EqMipmapLerp lerp() const;

		/// Set the fill value for texture channels indices outside the available range
		void setFill(TqFloat fill);
		/// \brief Set the lerp flag
		void setLerp(EqMipmapLerp lerp);
	protected:
		/// Value to fill a channel with if the associated texture doesn't contain sufficiently many channels.
		TqFloat m_fill;
		/// Flag for mipmap interpolation
		EqMipmapLerp m_lerp;
};


//------------------------------------------------------------------------------
/** \brief Contain the standard renderman shadow sampling options
 *
 * The standard renderman shadow sampling options may be passed to calls to the
 * shadow() builtin function in the RSL.  These include all the basic texture
 * sampling options, as well as a few texture-specific things such as shadow bias.
 */
class CqShadowSampleOptions : private CqTextureSampleOptionsBase
{
	public:
		/** \brief Set all options to sensible default values
		 *
		 * The defaults from CqTextureSampleOptionsBase are used, and in addition,
		 * the shadow biases are set to 0 and number of samples to 32.
		 */
		CqShadowSampleOptions();

		// Accessors from CqTextureSampleOptionsBase
		CqTextureSampleOptionsBase::sBlur;
		CqTextureSampleOptionsBase::tBlur;
		CqTextureSampleOptionsBase::sWidth;
		CqTextureSampleOptionsBase::tWidth;
		CqTextureSampleOptionsBase::filterType;
		CqTextureSampleOptionsBase::startChannel;
		CqTextureSampleOptionsBase::numChannels;
		CqTextureSampleOptionsBase::sWrapMode;
		CqTextureSampleOptionsBase::tWrapMode;

		// Modifiers from CqTextureSampleOptionsBase
		CqTextureSampleOptionsBase::setBlur;
		CqTextureSampleOptionsBase::setSBlur;
		CqTextureSampleOptionsBase::setTBlur;
		CqTextureSampleOptionsBase::setWidth;
		CqTextureSampleOptionsBase::setSWidth;
		CqTextureSampleOptionsBase::setTWidth;
		CqTextureSampleOptionsBase::setFilterType;
		CqTextureSampleOptionsBase::setStartChannel;
		CqTextureSampleOptionsBase::setNumChannels;
		CqTextureSampleOptionsBase::setWrapMode;
		CqTextureSampleOptionsBase::setSWrapMode;
		CqTextureSampleOptionsBase::setTWrapMode;

		// Other stuff from CqTextureSampleOptionsBase
		CqTextureSampleOptionsBase::fillFromFileHeader;

		//--------------------------------------------------
		// Shadow-specific sample options
		/// Get the number of samples used by stochastic sampling methods.
		TqInt numSamples() const;
		/// Get the low shadow bias
		TqFloat biasLow() const;
		/// Get the high shadow bias
		TqFloat biasHigh() const;

		/// Set the number of samples used by stochastic sampling methods.
		void setNumSamples(TqInt numSamples);
		/// Set the shadow bias
		void setBias(TqFloat bias);
		/// Set the low and high shadow biases
		void setBiasLow(TqFloat bias0);
		void setBiasHigh(TqFloat bias1);
	protected:
		/// Number of samples to take when using a stochastic sampler
		TqInt m_numSamples;
		/// Low end of linear shadow bias ramp
		TqFloat m_biasLow;
		/// High end of linear shadow bias ramp
		TqFloat m_biasHigh;
};


//==============================================================================
// Implementation details.
//==============================================================================

// CqTextureSampleOptionsBase implementation

inline CqTextureSampleOptionsBase::CqTextureSampleOptionsBase(
		TqFloat sBlur, TqFloat tBlur, TqFloat sWidth, TqFloat tWidth,
		EqTextureFilter filterType, TqInt startChannel, TqInt numChannels,
		EqWrapMode sWrapMode, EqWrapMode tWrapMode)
	: m_sBlur(sBlur),
	m_tBlur(tBlur),
	m_sWidth(sWidth),
	m_tWidth(tWidth),
	m_filterType(filterType),
	m_startChannel(startChannel),
	m_numChannels(numChannels),
	m_sWrapMode(sWrapMode),
	m_tWrapMode(tWrapMode)
{ }

inline CqTextureSampleOptionsBase::CqTextureSampleOptionsBase()
	: m_sBlur(0),
	m_tBlur(0),
	m_sWidth(1),
	m_tWidth(1),
	m_filterType(TextureFilter_Gaussian),
	m_startChannel(0),
	m_numChannels(1),
	m_sWrapMode(WrapMode_Black),
	m_tWrapMode(WrapMode_Black)
{ }


inline TqFloat CqTextureSampleOptionsBase::sBlur() const
{
	return m_sBlur;
}

inline TqFloat CqTextureSampleOptionsBase::tBlur() const
{
	return m_tBlur;
}

inline TqFloat CqTextureSampleOptionsBase::sWidth() const
{
	return m_sWidth;
}

inline TqFloat CqTextureSampleOptionsBase::tWidth() const
{
	return m_tWidth;
}

inline EqTextureFilter CqTextureSampleOptionsBase::filterType() const
{
	return m_filterType;
}

inline TqInt CqTextureSampleOptionsBase::startChannel() const
{
	return m_startChannel;
}

inline TqInt CqTextureSampleOptionsBase::numChannels() const
{
	return m_numChannels;
}

inline EqWrapMode CqTextureSampleOptionsBase::sWrapMode() const
{
	return m_sWrapMode;
}

inline EqWrapMode CqTextureSampleOptionsBase::tWrapMode() const
{
	return m_tWrapMode;
}


inline void CqTextureSampleOptionsBase::setBlur(TqFloat blur)
{
	setSBlur(blur);
	setTBlur(blur);
}

inline void CqTextureSampleOptionsBase::setSBlur(TqFloat sBlur)
{
	assert(sBlur >= 0);
	m_sBlur = sBlur;
}

inline void CqTextureSampleOptionsBase::setTBlur(TqFloat tBlur)
{
	assert(tBlur >= 0);
	m_tBlur = tBlur;
}

inline void CqTextureSampleOptionsBase::setWidth(TqFloat width)
{
	setSWidth(width);
	setTWidth(width);
}

inline void CqTextureSampleOptionsBase::setSWidth(TqFloat sWidth)
{
	assert(sWidth >= 0);
	m_sWidth = sWidth;
}

inline void CqTextureSampleOptionsBase::setTWidth(TqFloat tWidth)
{
	assert(tWidth >= 0);
	m_tWidth = tWidth;
}

inline void CqTextureSampleOptionsBase::setFilterType(EqTextureFilter type)
{
	assert(type != TextureFilter_Unknown);
	m_filterType = type;
}

inline void CqTextureSampleOptionsBase::setStartChannel(TqInt startChannel)
{
	assert(startChannel >= 0);
	m_startChannel = startChannel;
}

inline void CqTextureSampleOptionsBase::setNumChannels(TqInt numChans)
{
	assert(numChans >= 0);
	m_numChannels = numChans;
}

inline void CqTextureSampleOptionsBase::setWrapMode(EqWrapMode wrapMode)
{
	setSWrapMode(wrapMode);
	setTWrapMode(wrapMode);
}

inline void CqTextureSampleOptionsBase::setSWrapMode(EqWrapMode sWrapMode)
{
	m_sWrapMode = sWrapMode;
}

inline void CqTextureSampleOptionsBase::setTWrapMode(EqWrapMode tWrapMode)
{
	m_tWrapMode = tWrapMode;
}

inline void CqTextureSampleOptionsBase::fillFromFileHeader(
		const CqTexFileHeader& header)
{
	/// \todo Find a way to store & retrieve the downsampling filter?
	const SqWrapModes* wrapModes = header.findPtr<Attr::WrapModes>();
	if(wrapModes)
	{
		m_sWrapMode = wrapModes->sWrap;
		m_tWrapMode = wrapModes->tWrap;
	}
}

//------------------------------------------------------------------------------
// CqTextureSampleOptions implementation

inline CqTextureSampleOptions::CqTextureSampleOptions()
	: CqTextureSampleOptionsBase(),
	m_lerp(Lerp_Auto)
{ }

inline TqFloat CqTextureSampleOptions::fill() const
{
	return m_fill;
}

inline EqMipmapLerp CqTextureSampleOptions::lerp() const
{
	return m_lerp;
}

inline void CqTextureSampleOptions::setFill(TqFloat fill)
{
	m_fill = fill;
}

inline void CqTextureSampleOptions::setLerp(EqMipmapLerp lerp)
{
	m_lerp = lerp;
}

//------------------------------------------------------------------------------
// CqShadowSampleOptions implementation

inline CqShadowSampleOptions::CqShadowSampleOptions()
	: CqTextureSampleOptionsBase(),
	m_numSamples(32),
	m_biasLow(0),
	m_biasHigh(0)
{ }

inline TqInt CqShadowSampleOptions::numSamples() const
{
	return m_numSamples;
}

inline TqFloat CqShadowSampleOptions::biasLow() const
{
	return m_biasLow;
}

inline TqFloat CqShadowSampleOptions::biasHigh() const
{
	return m_biasHigh;
}

inline void CqShadowSampleOptions::setNumSamples(TqInt numSamples)
{
	m_numSamples = numSamples;
}

inline void CqShadowSampleOptions::setBias(TqFloat bias)
{
	m_biasLow = bias;
	m_biasHigh = bias;
}

inline void CqShadowSampleOptions::setBiasLow(TqFloat bias0)
{
	if(bias0 > m_biasHigh)
		m_biasHigh = bias0;
	m_biasLow = bias0;
}

inline void CqShadowSampleOptions::setBiasHigh(TqFloat bias1)
{
	if(bias1 < m_biasLow)
		m_biasLow = bias1;
	m_biasHigh = bias1;
}

} // namespace Aqsis

#endif // TEXTURESAMPLEOPTIONS_H_INCLUDED
