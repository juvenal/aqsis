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
 * \author Zachary Carter (zcarter@aqsis.org)
 *
 * \brief Mipmapped deep textures and associated facilities.
 */
#ifndef DEEPTEXTURE_H_INCLUDED
#define DEEPTEXTURE_H_INCLUDED

// Aqsis primary header
#include "aqsis.h"

// Standard libraries
#include <string>
#include <iostream>
#include <vector>
#include <tiff.h> //< Including (temporarily) in order to get the typedefs like uint32

// Other Aqsis headers
#include "sstring.h"
#include "vector3d.h"
#include "matrix.h"
#include "itexturemap.h" 
#include "deeptilearray.h"
#include "color.h"
#include "ri.h"

// External libraries
#include <boost/shared_ptr.hpp>

namespace Aqsis
{

//----------------------------------------------------------------------
/** CalculateFilter() Figure out which filter is applied to this mipmap.
*
* \todo Note: this function is copied out of texturemap.h
* It should be consolidated when the texture library is integrated.
*/
static RtFilterFunc CalculateFilter(CqString filter)
{
	RtFilterFunc filterfunc = RiBoxFilter;

	if ( filter == "gaussian" )
		filterfunc = RiGaussianFilter;
	if ( filter == "mitchell" )
		filterfunc = RiMitchellFilter;
	if ( filter == "box" )
		filterfunc = RiBoxFilter;
	if ( filter == "triangle" )
		filterfunc = RiTriangleFilter;
	if ( filter == "catmull-rom" )
		filterfunc = RiCatmullRomFilter;
	if ( filter == "sinc" )
		filterfunc = RiSincFilter;
	if ( filter == "disk" )
		filterfunc = RiDiskFilter;
	if ( filter == "bessel" )
		filterfunc = RiBesselFilter;

	return filterfunc;
}

//------------------------------------------------------------------------------
/** \brief A class to manage a single mipmap level of a deep shadow map.
 * 
 * Holds a CqDeepTileArray for access to the underlying data.
 * Does texture filtering (We'll have to think about what filter types to implement... possibly Monte Carlo)
 */
class CqDeepMipmapLevel
{
	public:
		/** \brief Construct an instance of CqDeepMipmapLevel
		 *
		 * \param filename - The full path and file name of the dtex file to open and read from.
		 */
		CqDeepMipmapLevel( IqDeepTextureInput& tileSource );
		virtual ~CqDeepMipmapLevel(){};
		
		/** \brief Access the tile array, apply filter to estimate visibility at the requested pixel and depth
		 *
		 * \param p1,p2,p3,p4 - points in texture space forming a quadrilateral over which we perform Monte Carlo filtering
		 * \param z - The depth pf the point of interest: we want to know the visibility at this point.
		 * \param numSampes - The number of times to sample with Monte Carlo integration
		 * \param filterFunc - Function pointer: use to get filter weights.
		 * 
		 * \return A color representing the visibility at the requested point. 
		 */
		CqColor filterVisibility(const TqFloat s1, const TqFloat s2, const TqFloat s3, const TqFloat s4,
				const TqFloat t1, const TqFloat t2, const TqFloat t3, const TqFloat t4,	const TqFloat z1, const TqFloat z2,
				const TqFloat z3, const TqFloat z4, const TqInt numSamples, RtFilterFunc filterFunc);
		
	private:
		
		// Functions
		
		/** \brief Access the tile array to fetch the visibility at the requested pixel and depth
		 *
		 * \param samplePoint - A vector containing the same point information as the overloaded function below.
		 * \return A color representing the visibility at the requested point. 
		 */
		virtual CqColor visibilityAt( const CqVector3D& samplePoint );
		
		/** \brief Access the tile array to fetch the visibility at the requested pixel and depth
		 *
		 * \param x - Image x-coordinate to which the scene point is projected on the shadow caster's image plane.
		 * \param y - Image y-coordinate to which the scene point is projected on the shadow caster's image plane.
		 * \param depth - Scene depth, measured from the shadow caster's (light) position, of the point we seek shadow information for.
		 * \return A color representing the visibility at the requested point. 
		 */
		virtual CqColor visibilityAt( const int x, const int y, const float depth );
		
		// Data
		CqDeepTileArray m_deepTileArray;
		TqInt m_numberOfChannels;
		TqInt m_xRes; //< width (in pixels) of this mipmap level
		TqInt m_yRes; //< height (in pixels) of this mipmap level
};

//------------------------------------------------------------------------------
/** \brief The highest level deep texture map abstraction.
 * 
 * This works as follows:
 * Upon instantiation, open texture file via new CqDeepTexInputFile and create as many 
 * mipmap levels as needed, passing to each CqDeepMipMapLevel instance a reference to
 * the CqDeepTexInputFile from which it should load tiles. 
 * 
 * Holds a set of CqDeepMipmapLevel (one for each level; probably instantiation on read)
 * Has some sort of interface which the shading language can plug into.
 */
class CqDeepTexture : public IqTextureMap
{
	public:
		/** \brief Construct an instance of CqDeepTexture
		 *
		 * \param filename - The full path and file name of the dtex file to open and read from.
		 */
		CqDeepTexture( const std::string filename );
		virtual ~CqDeepTexture(){};
		
		/** Get a pointer to a deep shadow map. This is a factory method.
		 * 
		 * \return A pointer to a deep texture instance holding the resuested shadow map,
		 * assuming the given file name specifies a valid shadow map. Otherwise return NULL.
		 */
		static IqTextureMap* GetDeepShadowMap( const std::string& strName );
		
		//---------------------------------------------------
		// Pure virtual functions inherited from IqTextureMap
		
		/** Get the horizontal resolution of this image.
		 */
		virtual TqUint XRes() const;
		/** Get the vertical resolution of this image.
		 */
		virtual TqUint YRes() const;
		/** Get the number of samples per pixel.
		 */
		virtual TqInt SamplesPerPixel() const;
		/** Get the storage format of this image.
		 */
		virtual	EqTexFormat	Format() const;
		/** Get the image type.
		 */
		virtual	EqMapType Type() const;
		/** Get the image name.
		 */
		virtual	const CqString&	getName() const;
		
		virtual const std::string getName2() const;
		/** Open this image ready for reading.
		 */
		virtual	void Open();
		/** Close this image file.
		 */
		virtual	void Close();
		/** Determine if this image file is valid, i.e. has been found and opened successfully.
		 */
		virtual bool IsValid() const;

		virtual void PrepareSampleOptions(std::map<std::string, IqShaderData*>& paramMap );

		virtual	void SampleMap( TqFloat s1, TqFloat t1, TqFloat swidth, TqFloat twidth,
								std::valarray<TqFloat>& val);
		virtual	void SampleMap( TqFloat s1, TqFloat t1, TqFloat s2, TqFloat t2, TqFloat s3,
								TqFloat t3, TqFloat s4, TqFloat t4, std::valarray<TqFloat>& val );
		virtual	void SampleMap( CqVector3D& R, CqVector3D& swidth, CqVector3D& twidth,
		                        std::valarray<TqFloat>& val, TqInt index = 0, 
		                        TqFloat* average_depth = NULL, TqFloat* shadow_depth = NULL );
		virtual	void SampleMap( CqVector3D& R1, CqVector3D& R2, CqVector3D& R3, CqVector3D& R4,
		                        std::valarray<TqFloat>& val, TqInt index = 0, 
		                        TqFloat* average_depth = NULL, TqFloat* shadow_depth = NULL );
		virtual CqMatrix& GetMatrix( TqInt which, TqInt index = 0 );

		virtual	TqInt NumPages() const;
	
	private:
		
		// Functions
		
		// static data
		static std::vector<boost::shared_ptr<CqDeepTexture> > m_textureCache;
		
		// member data
		CqDeepTexInputFile m_sourceFile;
		std::vector<boost::shared_ptr<CqDeepMipmapLevel> > m_mipmapSet;
		CqMatrix m_matWorldToScreen;	///< Matrix to convert points from world space to screen space.
		CqMatrix m_matWorldToCamera;	///< Matrix to convert points from world space to camera space.
		
		RtFilterFunc m_filterFunc; ///< Catmull-Rom, sinc, disk, ... pixelfilter
		TqInt m_XRes;
		TqInt m_YRes;
		TqFloat	m_sblur;
		TqFloat	m_tblur;
		TqFloat	m_pswidth;
		TqFloat	m_ptwidth;          
		TqFloat	m_samples;			///< How many samplings
		TqFloat	m_pixelvariance;    ///< Smallest Difference between two distinct samples
		TqFloat	m_swidth, m_twidth; ///< for the pixel's filter
		TqFloat	m_minBias;
		TqFloat m_biasRange; 
};

//------------------------------------------------------------------------------
} // namespace Aqsis

#endif // DEEPTEXTURE_H_INCLUDED
