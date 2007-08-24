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
 * \author Zachary L. Carter (zcarter@aqsis.org)
 * 
 * \brief Implementation of CqDeepMipmapLevel and CqDeepTexture classes.
 */

#include "deeptexture.h"

namespace Aqsis
{

// Begin CqDeepMipmapLevel implementation

CqDeepMipmapLevel::CqDeepMipmapLevel( IqDeepTextureInput& tileSource ) :
	m_deepTileArray( tileSource )
{

}

CqColor CqDeepMipmapLevel::visibilityAt( const int x, const int y, const float depth )
{
	return gColWhite;
}

//------------------------------------------------------------------------------
// Begin CqDeepTexture implementation

CqDeepTexture::CqDeepTexture( std::string filename ) :
	m_sourceFile( filename ),
	m_MipmapSet( )
{
	/// \toDo Instantiate a set of CqDeepMipmapLevel objects, one for each mipmap level.
	// For now, just create one:
	boost::shared_ptr<CqDeepMipmapLevel> newMipmapLevel( new CqDeepMipmapLevel( m_sourceFile ));
	m_MipmapSet.push_back(newMipmapLevel);
}

/** Get the horizontal resolution of this image.
 */
TqUint CqDeepTexture::XRes() const
{
	return m_sourceFile.imageWidth();
}

/** Get the vertical resolution of this image.
 */
TqUint CqDeepTexture::YRes() const
{
	return m_sourceFile.imageHeight();
}

/** Get the number of samples per pixel.
 */
TqInt CqDeepTexture::SamplesPerPixel() const
{
	/// toDo return the proper value for deep data here (number of color channels?)
	return 0;
}

/** Get the storage format of this image.
 */
EqTexFormat	CqDeepTexture::Format() const
{
	// We should change this to TexFormat_MIPMAP
	// once we add mipmap support for deep data
	return TexFormat_Plain;
}

/** Get the image type.
 */
EqMapType CqDeepTexture::Type() const
{
	// Should we ad a MapType_DeepData to the EqTexFormat enum in itexturemap.h? 
	// Or use MapType_Shadow, as below?
	return MapType_Shadow; 	
}

/** Get the image name.
 */
const CqString&	CqDeepTexture::getName() const
{
	return m_sourceFile.fileName();
}

/** Open this image ready for reading.
 */
void CqDeepTexture::Open()
{
	// We open the file on instantion
}

/** Close this image file.
 */
void CqDeepTexture::Close()
{
	// The file is closed when this class is destroyed (i.e. goes out of scope)
}
/** Determine if this image file is valid, i.e. has been found and opened successfully.
 */
bool CqDeepTexture::IsValid() const
{
	return m_sourceFile.isValid();
}

void CqDeepTexture::PrepareSampleOptions( std::map<std::string, IqShaderData*>& paramMap )
{
	
}

void CqDeepTexture::SampleMap( TqFloat s1, TqFloat t1, TqFloat swidth, TqFloat twidth,
								std::valarray<TqFloat>& val)
{
	
}

void CqDeepTexture::SampleMap( TqFloat s1, TqFloat t1, TqFloat s2, TqFloat t2, TqFloat s3, TqFloat t3,
		TqFloat s4, TqFloat t4, std::valarray<TqFloat>& val )
{
	
}

void CqDeepTexture::SampleMap( CqVector3D& R, CqVector3D& swidth, CqVector3D& twidth,
                        std::valarray<TqFloat>& val, TqInt index, 
                        TqFloat* average_depth, TqFloat* shadow_depth )
{

}

void CqDeepTexture::SampleMap( CqVector3D& R1, CqVector3D& R2, CqVector3D& R3, CqVector3D& R4,
                        std::valarray<TqFloat>& val, TqInt index, 
                        TqFloat* average_depth, TqFloat* shadow_depth )
{
	
}

CqMatrix& CqDeepTexture::GetMatrix( TqInt which, TqInt index )
{
	
}

TqInt CqDeepTexture::NumPages() const
{
	// What should we return here? What is meant by pages in this context?
	return 0;
}

//------------------------------------------------------------------------------
} // namespace Aqsis
