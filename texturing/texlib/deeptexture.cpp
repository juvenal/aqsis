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
#include "ishaderdata.h"
#include "random.h"

namespace Aqsis
{

// Some costants
const TqFloat MinSize = 3.0f;
const TqInt NumSamples = 16;
const TqInt MinSamples = 3;

// Static member initialization
std::vector<boost::shared_ptr<CqDeepTexture> > CqDeepTexture::m_textureCache(0);

//-----------------------------------------------------------------------------
// Begin CqDeepMipmapLevel implementation

CqDeepMipmapLevel::CqDeepMipmapLevel( IqDeepTextureInput& tileSource ) :
	m_deepTileArray( tileSource ),
	m_numberOfChannels( tileSource.numberOfColorChannels() ),
	m_xRes( tileSource.imageWidth() ), //< Note: eventually we should ensure this is the res of our specific mipmap level
	m_yRes( tileSource.imageHeight() )
{}

CqColor CqDeepMipmapLevel::filterVisibility(const TqFloat s1, const TqFloat s2, const TqFloat s3, const TqFloat s4,
		const TqFloat t1, const TqFloat t2, const TqFloat t3, const TqFloat t4,	const TqFloat z1, const TqFloat z2,
		const TqFloat z3, const TqFloat z4, const TqInt numSamples, boost::shared_ptr<IqTexFilter> filter)
{
/*
	for ( int j = 0; j < 256; ++j)
	{
		for ( int i = 0; i < 256; ++i)
		{
			CqColor result = visibilityAt( i, j, 50 );
			//printf("(%d, %d) : %f\n", j, i, result.fRed());
		}
	}
	exit(0);
*/
	// The points (p1,p2,p3,p4) represent a quadrilateral in texture coordinates which is the region to be filtered over.
	// For N samples,
	// Randomly choose a point, P in texture coordinates from the quadrilateral:
	// Choose two random numbers, ds and dt between 0 and 1; form the average P = bilerp(ds,dt, p1,p2,p3,p4). bilerp is bilinear interpoation.
	// Sample the texture at P to get T(P). In principle any reconstruction filter can be used to evaulate T(P), but bilinear filtering seems to be a good tradeoff between quality and performance.
	// Find the weight w(P) = filter_weight_func(ds-0.5, dt-0.5, 1, 1)
	// Add w(P)*T(P) to the current computed average
	// Return the average over N samples normalized by the total weight
	CqColor sum = gColBlack;
	TqFloat weightTot = 0;
	TqInt sample = 0;
	//while(sample < numSamples)
	//{
		CqRandom rand(42+sample); // Generate unique random number sequence per sample
	    //randomly choose s,t,z between 1,2,3,4:
		TqFloat ds = rand.RandomFloat(); //< uniform random number betwen 0 & 1
		TqFloat dt = rand.RandomFloat();
	    // bilinear interpolation between p1,p2,p3,p4.
	    TqFloat s = lerp(dt, lerp(ds, s1, s2), lerp(ds, s4, s3));  // bilinear interpolation between s1,s2,s3,s4.
	    TqFloat t = lerp(ds, lerp(dt, t1, t2), lerp(dt, t3, t4));
	    TqFloat z = lerp(ds, lerp(dt, z1, z2), lerp(dt, z3, z4));
	    TqFloat weight = filter->weight(ds-0.5, dt-0.5, 1.0, 1.0);
	    sum += weight*visibilityAt( s, t, z);
	    weightTot += weight;
	    sample++;
	//}
	return (sum / weightTot);
}

CqColor CqDeepMipmapLevel::visibilityAt( const CqVector3D& samplePoint )
{
	CqColor retColor = gColWhite; //< default 100% visibility
	const TqFloat sampleDepth = samplePoint.z();
	const TqVisFuncPtr visFunc = m_deepTileArray.visibilityFunctionAtPixel( samplePoint.x(), samplePoint.y() );
	TqInt i;

	if (visFunc.get() != NULL)
	{
		const TqInt nodeSize = m_numberOfChannels+1;
		const TqInt indexEndNode = visFunc->functionLength-1;
		const TqFloat* functionPtr = visFunc->functionPtr;
		// Search through the visibility function for the last node whose depth is 
		// less than or equal to sampleDepth;
		// \todo We should do a binary search here, since the visibility function is potentially long
		for (i = 0; i < indexEndNode; ++i)
		{
			if (sampleDepth <= functionPtr[i*nodeSize])
			{
				break;
			}
		}
		// Now 'i' should index the node of interest, whether it is the past node in
		// the function, or somewhere within.
		// NOTE: I am not sure how I should handle the case where there are 2, or more than 3 color channels
		// I am assuming there are either 1 or 3 color channels.
		if (m_numberOfChannels == 3)
		{
			retColor.SetColorRGB(functionPtr[i*nodeSize+1],
								 functionPtr[i*nodeSize+2],	
								 functionPtr[i*nodeSize+3]);
		}
		else
		{
			// assume greyscale
			retColor.SetColorRGB(functionPtr[i*nodeSize+1],
								 functionPtr[i*nodeSize+1],	
								 functionPtr[i*nodeSize+1]);	
		}
	}
	// If the function was null then there are no "hits" under this pixel, so visibility is 100%
	return retColor;
}

CqColor CqDeepMipmapLevel::visibilityAt( const int x, const int y, const float depth )
{
	CqColor retColor = gColWhite; //< default 100% visibility
	const TqVisFuncPtr visFunc = m_deepTileArray.visibilityFunctionAtPixel( x, y );
	TqInt i;
	
	//printf("at (%d, %d) length is: %d\n", x, y, visFunc->functionLength);
	if (visFunc->functionLength > 1)
	{
		const TqInt nodeSize = m_numberOfChannels+1;
		const TqInt indexEndNode = visFunc->functionLength-1;
		const TqFloat* functionPtr = visFunc->functionPtr;
		// Search through the visibility function for the last node whose depth is 
		// less than or equal to sampleDepth;
		// \todo We should do a binary search here, since the visibility function is potentially long
		for (i = 0; i < indexEndNode; ++i)
		{
			if (depth <= functionPtr[i*nodeSize])
			{
				break;
			}
		}
		// Now 'i' should index the node of interest, whether it is the last node in
		// the function, or somewhere within.
		// NOTE: I am not sure how I should handle the case where there are 2, or more than 3 color channels
		// I am assuming there are either 1 or 3 color channels.
		if (m_numberOfChannels == 3)
		{
			retColor.SetColorRGB(functionPtr[i*nodeSize+1],
								 functionPtr[i*nodeSize+2],	
								 functionPtr[i*nodeSize+3]);
		}
		else
		{
			// assume greyscale
			retColor.SetColorRGB(functionPtr[i*nodeSize+1],
								 functionPtr[i*nodeSize+1],	
								 functionPtr[i*nodeSize+1]);	
		}
	}
	// If the function was null then there are no "hits" under this pixel, so visibility is 100%
	return retColor;
}

//------------------------------------------------------------------------------
// Begin CqDeepTexture implementation

CqDeepTexture::CqDeepTexture( std::string filename ) :
	m_sourceFile( filename ),
	m_mipmapSet( ),
	m_XRes(m_sourceFile.imageWidth()),
	m_YRes(m_sourceFile.imageHeight()),
	m_filter(boost::shared_ptr<IqTexFilter>(new CqTexBoxFilter()))
{
	/// \toDo Instantiate a set of CqDeepMipmapLevel objects, one for each mipmap level.
	// For now, just create one:
	boost::shared_ptr<CqDeepMipmapLevel> newMipmapLevel( new CqDeepMipmapLevel( m_sourceFile ));
	m_mipmapSet.push_back(newMipmapLevel);
	m_sourceFile.transformationMatrices( m_matWorldToScreen, m_matWorldToCamera );
	//m_XRes = m_sourceFile.imageWidth();
	//m_YRes = m_sourceFile.imageHeight();
	//m_filter = RiGaussianFilter; // Note: shouldn't set this here, but I don't know what else to do right now (just want to see renders)
}

// Static factory method
IqTextureMap* CqDeepTexture::GetDeepShadowMap( const std::string& strName )
{
	// First search the texture map cache
	for ( std::vector<boost::shared_ptr<CqDeepTexture> >::const_iterator i = m_textureCache.begin();
																	i != m_textureCache.end(); ++i )
	{
		if ( ( *i ) ->getName2() == strName )
		{
			return i->get();
		}
	}

	// If we got here, the map doesn't exist yet, so we must create and load it.
	boost::shared_ptr<CqDeepTexture> newDeepTexture(new CqDeepTexture( strName ));
	if (newDeepTexture->IsValid())
	{
		m_textureCache.push_back(newDeepTexture);
		return newDeepTexture.get();
	}
	return NULL;
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

/** Get the image name.
 */
const std::string CqDeepTexture::getName2() const
{
	return m_sourceFile.fileName();
}

/** Open this image ready for reading.
 */
void CqDeepTexture::Open()
{
	// We open the file on instantion (IiI)
}

/** Close this image file.
 */
void CqDeepTexture::Close()
{
	// The file is closed when this class is destroyed (i.e. goes out of scope)
	// If we want to close the file, then do so and set the CqDtexInputFile::m_valid bit to false.
}

/** Determine if this image file is valid, i.e. has been found and opened successfully.
 */
bool CqDeepTexture::IsValid() const
{
	return m_sourceFile.isValid();
}

void CqDeepTexture::PrepareSampleOptions( std::map<std::string, IqShaderData*>& paramMap )
{
	m_sblur = 0.0f;   // TurnOff the blur per texture(), environment() or shadow() by default
	m_tblur = 0.0f;
	m_pswidth = 1.0f; // In case of sampling
	m_ptwidth = 1.0f;
	m_samples = 16.0f; // The shadow required to be init. at 16.0 by default

	if (Type() != MapType_Shadow)
		m_samples = 8.0f;
	if (Type() != MapType_Environment)
		m_samples = 8.0f;
	
	// Get parameters out of the map.
	if ( paramMap.size() != 0 )
	{
		if ( paramMap.find( "width" ) != paramMap.end() )
		{
			paramMap[ "width" ] ->GetFloat( m_pswidth );
			m_ptwidth = m_pswidth;
		}
		else
		{
			if ( paramMap.find( "swidth" ) != paramMap.end() )
				paramMap[ "swidth" ] ->GetFloat( m_pswidth );
			if ( paramMap.find( "twidth" ) != paramMap.end() )
				paramMap[ "twidth" ] ->GetFloat( m_ptwidth );
		}
		if ( paramMap.find( "blur" ) != paramMap.end() )
		{
			paramMap[ "blur" ] ->GetFloat( m_sblur );
			m_tblur = m_sblur;
		}
		else
		{
			if ( paramMap.find( "sblur" ) != paramMap.end() )
			{
				paramMap[ "sblur" ] ->GetFloat( m_sblur );
			}
			if ( paramMap.find( "tblur" ) != paramMap.end() )
			{
				paramMap[ "tblur" ] ->GetFloat( m_tblur );
			}
		}

		if ( paramMap.find( "samples" ) != paramMap.end() )
		{
			paramMap[ "samples" ] ->GetFloat( m_samples );
		}

		if ( paramMap.find( "filter" ) != paramMap.end() )
		{
			CqString filter;

			paramMap[ "filter" ] ->GetString( filter );
			//Aqsis::log() << warning << "filter will be " << filter << std::endl;

			m_filter = CqTexFilter::filterOfType(filter);
		}

		if ( paramMap.find( "pixelvariance" ) != paramMap.end() )
		{
			paramMap[ "pixelvariance" ] ->GetFloat( m_pixelvariance );
		}
	}
	// Extend the shadow() call to accept bias, if set, override global bias
	m_minBias = 0.0f;
	TqFloat maxBias = 0.0f;
	m_biasRange = 0.0f;

	if ( ( !paramMap.empty() ) && ( paramMap.find( "bias" ) != paramMap.end() ) )
	{
		paramMap[ "bias" ] ->GetFloat( m_minBias );
		maxBias = m_minBias;
	}
	if ( ( !paramMap.empty() ) && ( paramMap.find( "bias0" ) != paramMap.end() ) )
	{
		paramMap[ "bias0" ] ->GetFloat( m_minBias );
		maxBias = m_minBias;
	}
	if ( ( !paramMap.empty() ) && ( paramMap.find( "bias1" ) != paramMap.end() ) )
	{
		paramMap[ "bias1" ] ->GetFloat( maxBias );
	}
	
	// Sanity check: make sure min biases is less than max bias.
	if(m_minBias > maxBias)
	{
		TqFloat tmp = m_minBias;
		m_minBias = maxBias;
		maxBias = tmp;
	}
		
	m_biasRange = maxBias - m_minBias;
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
	// The value returned in val should represent (1-visibility()) averaged over the region defined by
	// the four vectors. Therefore the value is the shadow coefficient: how much is the point in shadow?
	// ... 0 means 'not in shadow" and 1 means "in shadow."
	
	// Check the memory and make sure we don't abuse it
	//if (index == 0)
	//	CriticalMeasure();
	//TIME_SCOPE("Shadow Mapping")

	// If no map defined, not in shadow.
	val.resize( 1 );
	val[ 0 ] = 0.0f;

	CqVector3D vecRlaverage; 
	CqVector3D vecR1l, vecR2l, vecR3l, vecR4l;
	CqVector3D vecR1m, vecR2m, vecR3m, vecR4m;

	// Generate a matrix to transform points from camera space into the space of the light source used in the
	// definition of the shadow map.
	CqMatrix& matCameraToLight = m_matWorldToCamera;
	// Generate a matrix to transform points from camera space into the space of the shadow map.
	CqMatrix& matCameraToMap = m_matWorldToScreen;

	vecRlaverage = matCameraToLight * ( ( R1 + R2 + R3 + R4 ) * 0.25f );
	TqFloat z_average = vecRlaverage.z();
	vecR1l = matCameraToLight * R1;
	vecR2l = matCameraToLight * R2;
	vecR3l = matCameraToLight * R3;
	vecR4l = matCameraToLight * R4;
	TqFloat z1 = vecR1l.z();
	TqFloat z2 = vecR2l.z();
	TqFloat z3 = vecR3l.z();
	TqFloat z4 = vecR4l.z();

	// If point is behind light, call it not in shadow.
	if (z_average <= 0.0f)
		return;

	vecR1m = matCameraToMap * R1;
	if ((R1 == R2) && (R2 == R3) && (R3 == R4))
	{
		vecR2m = vecR3m = vecR4m = vecR1m;
	}
 	else
 	{
 		vecR2m = matCameraToMap * R2;
 		vecR3m = matCameraToMap * R3;
 		vecR4m = matCameraToMap * R4;
 	}

	TqFloat xro2 = (m_XRes - 1) * 0.5f;
	TqFloat yro2 = (m_YRes - 1) * 0.5f;

	TqFloat sbo2 = m_sblur * xro2;
	TqFloat tbo2 = m_tblur * yro2;


	TqFloat s1 = vecR1m.x() * xro2 + xro2;
	TqFloat t1 = m_YRes - ( vecR1m.y() * yro2 + yro2 ) - 1;
	TqFloat s2 = vecR2m.x() * xro2 + xro2;
	TqFloat t2 = m_YRes - ( vecR2m.y() * yro2 + yro2 ) - 1;
	TqFloat s3 = vecR3m.x() * xro2 + xro2;
	TqFloat t3 = m_YRes - ( vecR3m.y() * yro2 + yro2 ) - 1;
	TqFloat s4 = vecR4m.x() * xro2 + xro2;
	TqFloat t4 = m_YRes - ( vecR4m.y() * yro2 + yro2 ) - 1;

	TqFloat smin = ( s1 < s2 ) ? s1 : ( s2 < s3 ) ? s2 : ( s3 < s4 ) ? s3 : s4;
	TqFloat smax = ( s1 > s2 ) ? s1 : ( s2 > s3 ) ? s2 : ( s3 > s4 ) ? s3 : s4;
	TqFloat tmin = ( t1 < t2 ) ? t1 : ( t2 < t3 ) ? t2 : ( t3 < t4 ) ? t3 : t4;
	TqFloat tmax = ( t1 > t2 ) ? t1 : ( t2 > t3 ) ? t2 : ( t3 > t4 ) ? t3 : t4;

	// Cull if outside bounding box.
	TqUint lu = static_cast<TqInt>( lfloor( smin - sbo2 ) );
	TqUint hu = static_cast<TqInt>( lceil( smax + sbo2 ) );
	TqUint lv = static_cast<TqInt>( lfloor( tmin - tbo2 ) );
	TqUint hv = static_cast<TqInt>( lceil( tmax + tbo2 ) );

	if ( lu >= m_XRes || hu < 0 || lv >= m_YRes || hv < 0 )
		return;

	lu = max(0, (int)lu);
	lv = max(0, (int)lv);
	hu = min(m_XRes - 1,(int)hu);
	hv = min(m_YRes - 1,(int)hv);

	TqFloat sres = (1.0 + m_pswidth/2.0) * (hu - lu);
	TqFloat tres = (1.0 + m_ptwidth/2.0) * (hv - lv);

	if (sres < MinSize)
		sres = MinSize;
	if (tres < MinSize)
		tres = MinSize;

	if (sres > m_XRes/2)
		sres = m_XRes/2;
	if (tres > m_YRes/2)
		tres = m_YRes/2;

	// Calculate no. of samples.
	TqUint nt, ns;

	if ( m_samples > 0 )
	{

		nt = ns =  3 * static_cast<TqInt>( lceil( sqrt(  m_samples ) ) );
	}
	else
	{
		if ( sres * tres * 4.0 < NumSamples )
		{
			ns = static_cast<TqInt>( sres * 2.0 + 0.5 );
			ns = ( ns < MinSamples ? MinSamples : ( ns > NumSamples ? NumSamples : ns ) );
			nt = static_cast<TqInt>( tres * 2.0 + 0.5 );
			nt = ( nt < MinSamples ? MinSamples : ( nt > NumSamples ? NumSamples : nt ) );
		}
		else
		{
			nt = static_cast<TqInt>( sqrt( tres * NumSamples / sres ) + 0.5 );
			nt = ( nt < MinSamples ? MinSamples : ( nt > NumSamples ? NumSamples : nt ) );
			ns = static_cast<TqInt>( static_cast<TqFloat>( NumSamples ) / nt + 0.5 );
			ns = ( ns < MinSamples ? MinSamples : ( ns > NumSamples ? NumSamples : ns ) );
		}
	}
	// Indexing the mipmap set lets us filter over one of several mipmap levels
	CqColor visibility;
	visibility = m_mipmapSet[index]->filterVisibility(s1, s2, s3, s4,
			t1, t2, t3, t4, z1, z2, z3, z4, ns*nt, m_filter);
	val[0] = (1.0-visibility.fRed()); //greyscale shadow
/*
	// I've really no idea what rbias does, but in the process of fixing the
	// bias behaviour quickly (prior to the texture refactor which will replace
	// much of this), I'm leaving it with the same value which it previously
	// contained - CJF.
	TqFloat rbias = m_minBias + 0.5*m_biasRange;

  	// A conservative z value is the worst case scenario
	// for the high bias value will be between 0..2.0 * rbias
    TqDouble minz = MinZ(index);
	if ((minz != RI_FLOATMAX) && ((z + 2.0 * rbias) < minz))
		return;

	// Don't do the area computing of the shadow if the conservative z 
	// value is either lower than the current minz value of the tile or 
	// even if conservative z value is higher than the maxz value 
	// (assuming the maxz is not infinite)
  	// A conservative z value is the worst case scenario
	// for the high bias value will be between 0..2.0 * rbias

	if ( (NumPages() > 1) && valid )
    {
		TqFloat minz, maxz;
		minMax(minz, maxz, 0);
		if ((z + 2.0 * rbias) < minz ) 
		{
			return;
		}
		if (maxz != RI_FLOATMAX) 
		{
			if ((z - 2.0 * rbias) > maxz  )
			{
				val[0] = 1.0; // note: this is not the correct thing to do for deep shadows. Instead, use the last node in the visibility function
				return;
			}
		}
	}

	for ( i = 0; i < ns; ++i, s += ds )
	{
		t = lv - tdelta;
		TqUint j;

		for ( j = 0; j < nt; j++, t += dt )
		{
			// Jitter s and t
			m_rand_index = ( m_rand_index + 1 ) & 1023;
			TqInt iu = static_cast<TqUint>( s + m_aRand_no[ m_rand_index ] * js );
			m_rand_index = ( m_rand_index + 1 ) & 1023;
			TqInt iv = static_cast<TqUint>( t + m_aRand_no[ m_rand_index ] * jt );

			if( iu < 0 || iu >= (TqInt) m_XRes || iv < 0 || iv >= (TqInt) m_YRes )
			{
				continue;
			}

			if( ( pTMBa == NULL )  || !pTMBa->IsValid( iu, iv, index ) )
			{
				pTMBa = GetBuffer( iu, iv, index );
			}

			if( ( pTMBa != NULL) && ( pTMBa->pVoidBufferData() != 0 ) )
			{
				iu -= pTMBa->sOrigin();
				iv -= pTMBa->tOrigin();

				TqFloat mapz = pTMBa->GetValue( iu, iv, 0 );

				m_rand_index = ( m_rand_index + 1 ) & 1023;
				TqFloat bias = m_biasRange*m_aRand_no[m_rand_index] + m_minBias;

				if ( z > mapz + bias)
				{
					inshadow ++;
					sample_z = z - mapz;
				}
				avz += mapz;
			}
		}
	}

	if( NULL != average_depth )
	{
		avz /= ( ns * nt );
		*average_depth = avz;
	}

	if( NULL != shadow_depth )
	{
		sample_z /= ( ns * nt );

		*shadow_depth = sample_z;
	}
*/
}

CqMatrix& CqDeepTexture::GetMatrix( TqInt which, TqInt index )
{
	//boost::shared_ptr<CqDeepTexture> sourceTexture = m_textureCache[index];
	//return sourceTexture->m_mipmapSet.front()->getMatrix(which);
	if ( which == 0 )
		return m_matWorldToCamera;
	else if ( which == 1 )
		return m_matWorldToScreen;
	//else if ( which == 2 )
	//	return m_ITTCameraToLightMatrices[index];
	return m_matWorldToCamera;
}

TqInt CqDeepTexture::NumPages() const
{
	// What should we return here? What is meant by pages in this context?
	return 1;
}

//------------------------------------------------------------------------------
} // namespace Aqsis
