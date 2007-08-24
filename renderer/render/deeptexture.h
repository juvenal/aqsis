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
#include "tilearray.h"
#include "color.h"

// External libraries
#include <boost/shared_ptr.hpp>

namespace Aqsis
{

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
	  
		/** \brief Identify the ID of the tile that contains the requested pixel. If the tile is already cached, return the visibility function
		 * for the desired pixel and mark the tile as freshly used (so that it will not be evicted too soon), and increment the age of other cached tiles.
		 * If the tile is not in cache, load it from file, evicting the least recently used cached tile if the cache is full, and proceeding normalyy from there.
		 *
		 * \param x - Image x-coordinate to which the scene point is projected on the shadow caster's image plane.
		 * \param y - Image y-coordinate to which the scene point is projected on the shadow caster's image plane.
		 * \param depth - Scene depth, measured from the shadow caster's position, of the point we seek shadow information for.
		 * \return A color representing the visibility at the requested point. 
		 */
		virtual CqColor visibilityAt( const int x, const int y, const float depth );
		
	private:
		
		// Functions
		//void filterFunction();

		// Data
		CqDeepTileArray m_deepTileArray;
		
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
		
		// Data
		CqDeepTexInputFile m_sourceFile;
		std::vector<boost::shared_ptr<CqDeepMipmapLevel> > m_MipmapSet;
};

//------------------------------------------------------------------------------
} // namespace Aqsis

#endif // TILEARRAY_H_INCLUDED
