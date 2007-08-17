// Aqsis
// Copyright (C) 1997 - 2001, Paul C. Gregory
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


/** \file deepshadow.cpp
 * 
 * \brief Implements the CqDeepShadowGen class for generating deep shadow maps: 
 * managing transmittance data, filtering, and computing visibility data.
 * 
 * \author Zachary L Carter (zcarter@aqsis.org)
*/

#include	"multitimer.h"

#include	"aqsis.h"

#ifdef WIN32
#include    <windows.h>
#endif
#include	<math.h>

#include	"surface.h"
#include	"imagepixel.h"
#include	"deepshadow.h"

#include	"imagers.h"

START_NAMESPACE( Aqsis )

//------------------------------------------------------------------------------
// Implementation for CqDeepShadowGen
//------------------------------------------------------------------------------
CqDeepShadowGen::CqDeepShadowGen()
	: m_dsmImage(),
	m_deepShadowMode(),
	m_currentPixelIndex(0)
{ 
}
		 
CqDeepShadowGen::~CqDeepShadowGen()
{
}

//Note: this is called once per display... which might be bad, since the second call overrides the first
void CqDeepShadowGen::initialize(TqInt xRes, TqInt yRes, bool colorShadow, bool motionBlurShadow, bool mipmapShadow)
{
	m_dsmImage.reserve(xRes*yRes);
	m_deepShadowMode.colorShadow = colorShadow;
	m_deepShadowMode.motionBlurShadow = motionBlurShadow;
	m_deepShadowMode.mipmapShadow = mipmapShadow;	
}

//---------------------------------------------
// Store a reference to the deep data for the current subpixel,
// by inserting into the current SqDeepShadowPixel's surfaceTransmittance vector.
// At this point, the lists need not yet be sorted
// This is called from from CqImagePixel::Combine()
//------------------------------------------------------------------------------
void CqDeepShadowGen::setDeepData(SqSampleData* samples)
{
	m_dsmImage[m_currentPixelIndex].surfaceTransmittances.push_back(samples->m_Data);
	
	TqInt subIndex = samples->m_SubCellIndex;
	TqFloat depth = samples->m_Data[1].Data()[Sample_Depth];
	TqFloat alpha = samples->m_Data[1].Data()[Sample_Alpha];
	TqFloat coverage = samples->m_Data[1].Data()[Sample_Coverage];
	TqFloat green = samples->m_Data[1].Data()[Sample_Green];
	TqFloat ogreen = samples->m_Data[1].Data()[Sample_OGreen];
	
	//'samples' also has time information we might use for motion blur
}

// For debugging: iterate over all the deep data to check that it is valid
// This is meant to test if the sample data we are referencing becomes freed
// It is, after all, part of the global sample pool, which is managed elsewhere
void CqDeepShadowGen::checkDeepData()
{
	//std::vector<SqDeepShadowPixel>::iterator deep_pixel = m_dsmImage.begin();
	//std::vector< std::deque<SqImageSample> >::iterator subpixel = deep_pixel->surfaceTransmittances.begin();
	//std::deque<SqImageSample>::iterator sample = subpixel->begin();
	
	TqFloat* sampleData;
	// iterate over the pixels
	for ( std::vector<SqDeepShadowPixel>::iterator deep_pixel = m_dsmImage.begin(); deep_pixel != m_dsmImage.end(); deep_pixel++)
	{
		// iterate over the sub-pixels
		for ( std::vector< std::deque<SqImageSample> >::iterator subpixel = deep_pixel->surfaceTransmittances.begin(); subpixel != deep_pixel->surfaceTransmittances.end(); subpixel++)
		{
			// iterate over the deep data
			for ( std::deque<SqImageSample>::iterator sample = subpixel->begin(); sample != subpixel->end(); sample++)
			{	
				sampleData = sample->Data();
				TqFloat depth = sampleData[Sample_Depth];
				TqFloat alpha = sampleData[Sample_Alpha];
				TqFloat coverage = sampleData[Sample_Coverage];
				TqFloat green = sampleData[Sample_Green];
				TqFloat ogreen = sampleData[Sample_OGreen];				
							
			}
		}
	}
}

END_NAMESPACE( Aqsis )
