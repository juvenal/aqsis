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


/** \file deepshadow.h
 * 
 * \brief Implements the DeepShadowGen class for generating deep shadow maps: 
 * managing transmittance data, filtering, and computing visibility data
 * 
 * \author Zachary L Carter (zcarter@aqsis.org)
*/

#ifndef DEEPSHADOW_H_INCLUDED
#define DEEPSHADOW_H_INCLUDED 1

#include	"aqsis.h"

#include	<vector>
#include	<queue>
#include	<deque>

#include	"iddmanager.h"
#include	"bitvector.h"
#include	"micropolygon.h"
#include	"renderer.h"
#include	"ri.h"
#include	"surface.h"
#include	"color.h"
#include	"vector2d.h"
#include    "imagepixel.h"

START_NAMESPACE( Aqsis )
 
 //------------------------------------------------------------------------------
/** \brief Structure representing a visibility at a specific depth
 *
 * A visibility function is a collection of these structures.
 */
struct SqVisibilityData
{
	TqFloat zdepth;
	/// A value [0,1] specifying how much light there should be at zdepth: 0 is complete shadow.
	TqFloat visibility;
};


//------------------------------------------------------------------------------
/** \brief Structure defining what mode to render deep shadow data
 * Render mode options include: colored shadows, motion blur, mip-mapping	
 */
struct SqDeepShadowMode
{
	/// if coloredShadows true then render colored shadows, otherwise greyscale 
	bool colorShadow;
	/// if motionBlurredShadows true then render motion blurred shadows
	bool motionBlurShadow;
	/// render mip-mapped shadows, but how to decide mip-mapping level?
	bool mipmapShadow;
	
	SqDeepShadowMode(const bool colorShadow = false, const bool motionBlurShadow = false, const bool mipmappShadow = false);
};

//-----------------------------------------------------------------------
/** Class for generating deep shadow maps: 
 * managing transmittance data, filtering, and computing visibility data
 */
class CqDeepShadowGen
{
	public:	
	
		/** \brief Constructor
		 */	
		CqDeepShadowGen();
		
		/** \brief Destructor is virutal so we can extend from CqDeepShadowGen
		 */			 
		virtual ~CqDeepShadowGen();
		
		/** \brief Initialize storage for deep shadow data
		 *
		 * This must be called before performing options on a class instance: it reserves space
		 * \param xRes, yRes - pixel dimensions of the shadow map
		 * \param mode - collection of render flags for turning on/off certain deep shadow features  
		 */
		void initialize(TqInt xRes, TqInt yRes, bool colorShadow, bool motionBlurShadow, bool mipmapShadow);
		
		/** \brief Set deep data for the current subpixel, 
		 * relative to the current pixel as defined by m_currentPixelIndex
		 *
		 * \param samples - pointer to data structure containing deep data (i.e. depth, opacity and color)  
		 */
		void setDeepData(SqSampleData* samples);

		/** \brief Set current pixel index: this is where subsequent deep data will be sent.
		 *
		 * \param index - pixel index  
		 */		
		inline void setCurrentPixelIndex(TqInt index);
		
		void checkDeepData();
		
		
	private:			
		
		//------------------------------------------------------------------------------
		/** \brief Structure representing all uncompressed deep shadow data for a single pixel
		 * 
		 */		
		struct SqDeepShadowPixel {
			/// Surface transmittance functions for each subpixel
			/// Stored as a vector of shared pointers to queues of image samples	
			std::vector< std::deque<SqImageSample> > surfaceTransmittances;
			/// Visibility data for the pixel
			std::vector<SqVisibilityData> visibilityFunction;				
		};
		
		/// All deep shadow data for the current render context 
		std::vector<SqDeepShadowPixel> m_dsmImage;
		SqDeepShadowMode m_deepShadowMode;
		TqInt m_currentPixelIndex;  
}
;

//------------------------------------------------------------------------------
// Inline function(s) for SqDeepShadowMode
//------------------------------------------------------------------------------
inline SqDeepShadowMode::SqDeepShadowMode(const bool colorShadow, const bool motionBlurShadow, const bool mipmapShadow)
	: colorShadow(colorShadow),
	motionBlurShadow(motionBlurShadow),
	mipmapShadow(mipmapShadow)
{ }

//------------------------------------------------------------------------------
// Inline function(s) for CqDeepShadowGen
//------------------------------------------------------------------------------
inline void CqDeepShadowGen::setCurrentPixelIndex(TqInt index)
{
	m_currentPixelIndex = index;
}

END_NAMESPACE( Aqsis )

#endif // DEEPSHADOW_H_INCLUDED
