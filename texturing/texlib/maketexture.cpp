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
 * \brief Implementation of functions for creating texture maps.
 *
 * \author Chris Foster [chris42f (at) gmail (d0t) com]
 */

#include "maketexture.h"

#include <algorithm>

#include <boost/shared_ptr.hpp>

#include "bake.h"
#include "itexinputfile.h"
#include "itexoutputfile.h"
#include "magicnumber.h"
#include "downsample.h"
#include "texturebuffer.h"
#include "texexception.h"
#include "version.h"

namespace Aqsis {

//------------------------------------------------------------------------------
namespace {

//------------------------------------------------------------------------------
// Helper functions and classes
//------------------------------------------------------------------------------

/** \brief Downsample the provided buffer into the given output file.
 *
 * \param buf - Pointer to source data.  This smart pointer is reset to save
 *              memory during the mipmapping process, so make sure a copy of it
 *              is kept elsewhere if desired.
 * \param outFile - output file for the mipmapped data
 * \param filterInfo - information about which filter type and size to use
 * \param wrapModes - specifies how the texture will be wrapped at the edges.
 */
template<typename ChannelT>
void downsampleToFile(boost::shared_ptr<CqTextureBuffer<ChannelT> >& buf,
		IqMultiTexOutputFile& outFile, const SqFilterInfo& filterInfo,
		const SqWrapModes wrapModes)
{
	outFile.writePixels(*buf);
	typedef CqDownsampleIterator<CqTextureBuffer<ChannelT> > TqDownsampleIter;
	for(TqDownsampleIter i = ++TqDownsampleIter(buf, filterInfo, wrapModes),
			end = TqDownsampleIter(); i != end; ++i)
	{
		buf = *i;
		outFile.newSubImage(buf->width(), buf->height());
		outFile.writePixels(*buf);
	}
}

/** \brief Create a mipmap from pixel data in the given input file.
 *
 * ChannelT is the pixel component type.
 *
 * \param inFile - input file from which the data should be read
 * \param outFile - name of output file for the mipmapped data
 * \param filterInfo - information about which filter type and size to use
 * \param wrapModes - specifies how the texture will be wrapped at the edges.
 */
template<typename TexSrcT, typename ChannelT>
void createMipmapTyped(const TexSrcT& texSrc, IqMultiTexOutputFile& outFile,
		const SqFilterInfo& filterInfo, const SqWrapModes wrapModes)
{
	// Read pixels into the input buffer.
	boost::shared_ptr<CqTextureBuffer<ChannelT> > buf(new CqTextureBuffer<ChannelT>());
	texSrc.readPixels(*buf);
	downsampleToFile(buf, outFile, filterInfo, wrapModes);
}

/// Specialization for OpenEXR half data format (TIFF can't handle half data)
template<typename TexSrcT>
void createMipmapTypedHalf(const TexSrcT& texSrc, IqMultiTexOutputFile& outFile,
		const SqFilterInfo& filterInfo, const SqWrapModes wrapModes)
{
#	ifdef USE_OPENEXR
	// Read pixels into the input buffer and convert to 32-bit floating point
	// since TIFF can't the half data type.
	CqTextureBuffer<half> halfBuf;
	texSrc.readPixels(halfBuf);
	boost::shared_ptr<CqTextureBuffer<TqFloat> > buf(
			new CqTextureBuffer<TqFloat>(halfBuf));
	downsampleToFile(buf, outFile, filterInfo, wrapModes);
#	else
	assert(0 && "Compiled without OpenEXR support");
#	endif
}

/** \brief Create a mipmap given a texture source and save it to a file.
 *
 * \param texSrc - a "texture source" class.  Needs one method, readPixels().
 *                 IqTexInputFile is a model of this type.
 * \param chanType - texture channel type of input.
 * \param outFile - output file into which texture data will be placed.
 * \param filterInfo - information about mipmap downsampling filter type and size
 * \param wrapModes - specify how texture will be wrapped at edges during
 *            downsampling.
 */
template<typename TexSrcT>
void createMipmap(const TexSrcT& texSrc, const EqChannelType chanType,
		IqMultiTexOutputFile& outFile, const SqFilterInfo& filterInfo,
		const SqWrapModes& wrapModes)
{
	// Dispatche to mipmapping function based on the type of the input
	// texture data.
	switch(chanType)
	{
		case Channel_Float32:
			createMipmapTyped<TexSrcT,TqFloat>(texSrc, outFile, filterInfo, wrapModes);
			break;
		case Channel_Unsigned32:
			createMipmapTyped<TexSrcT,TqUint32>(texSrc, outFile, filterInfo, wrapModes);
			break;
		case Channel_Signed32:
			createMipmapTyped<TexSrcT,TqInt32>(texSrc, outFile, filterInfo, wrapModes);
			break;
		case Channel_Unsigned16:
			createMipmapTyped<TexSrcT,TqUint16>(texSrc, outFile, filterInfo, wrapModes);
			break;
		case Channel_Signed16:
			createMipmapTyped<TexSrcT,TqInt16>(texSrc, outFile, filterInfo, wrapModes);
			break;
		case Channel_Unsigned8:
			createMipmapTyped<TexSrcT,TqUint8>(texSrc, outFile, filterInfo, wrapModes);
			break;
		case Channel_Signed8:
			createMipmapTyped<TexSrcT,TqInt8>(texSrc, outFile, filterInfo, wrapModes);
			break;
		case Channel_Float16:
			createMipmapTypedHalf(texSrc, outFile, filterInfo, wrapModes);
			break;
		default:
			AQSIS_THROW(XqBadTexture, "Cannot create mipmap for input channel types");
	}
}

/** Copy pixels of one texture buffer onto part of another.
 */
template<typename ChannelT>
void copyPixels(const CqTextureBuffer<ChannelT>& src, TqInt topLeftX, TqInt topLeftY,
		CqTextureBuffer<ChannelT>& dest)
{
	assert(topLeftX >= 0);
	assert(topLeftY >= 0);
	assert(topLeftX + src.width() <= dest.width());
	assert(topLeftY + src.height() <= dest.height());
	assert(src.numChannels() == dest.numChannels());

	TqInt srcHeight = src.height();
	TqInt bytesPerPixel = src.numChannels()*sizeof(ChannelT);
	TqInt srcRowStride = src.width()*bytesPerPixel;
	TqInt destRowStride = dest.width()*bytesPerPixel;

	const TqUint8* rawSrc = src.rawData();
	TqUint8* rawDest = dest.rawData() + destRowStride*topLeftY + topLeftX*bytesPerPixel;

	for(int i = 0; i < srcHeight; ++i)
	{
		std::copy(rawSrc, rawSrc+srcRowStride, rawDest);
		rawDest += destRowStride;
		rawSrc += srcRowStride;
	}
}

/** \brief Source class for concatenated cube face environment textures
 *
 * This class is a proxy for a texture input file.  We need it because it's
 * convenient to assume (for the other functions) that the input texture for
 * mipmapping will come from a single call to readPixels().  The readPixels()
 * in this class therefore stands in for IqTexInputFile::readPixels(),
 * performing texture concatenation in the correct order for cube face
 * environment texture generation.
 */
class CqCubeFaceTextureSource
{
	private:
		const IqTexInputFile& m_px;
		const IqTexInputFile& m_nx;
		const IqTexInputFile& m_py;
		const IqTexInputFile& m_ny;
		const IqTexInputFile& m_pz;
		const IqTexInputFile& m_nz;
	public:
		/// \brief Create from six input files, one for each cube face.
		CqCubeFaceTextureSource(
				const IqTexInputFile& px, const IqTexInputFile& nx,
				const IqTexInputFile& py, const IqTexInputFile& ny,
				const IqTexInputFile& pz, const IqTexInputFile& nz )
			: m_px(px), m_nx(nx),
			m_py(py), m_ny(ny),
			m_pz(pz), m_nz(nz)
		{ }
		/** \brief Read pixels from the six faces and concatenate into buf.
		 *
		 * \param buf - output buffer for pixel data.
		 */
		template<typename ChannelT>
		void readPixels(CqTextureBuffer<ChannelT>& buf) const
		{
			assert(m_px.header().channelList().sharedChannelType()
					== getChannelTypeEnum<ChannelT>());

			// Make sure the buffer has the correct size.
			TqInt faceWidth = m_px.header().width();
			TqInt faceHeight = m_px.header().height();
			TqInt numChans = m_px.header().channelList().numChannels();
			buf.resize(faceWidth*3, faceHeight*2, numChans);

			// Extract pixels from the input files and copy into buf.
			CqTextureBuffer<ChannelT> tmpBuf;
			m_px.readPixels(tmpBuf); copyPixels(tmpBuf, 0, 0, buf);
			m_nx.readPixels(tmpBuf); copyPixels(tmpBuf, 0, faceHeight, buf);
			m_py.readPixels(tmpBuf); copyPixels(tmpBuf, faceWidth, 0, buf);
			m_ny.readPixels(tmpBuf); copyPixels(tmpBuf, faceWidth, faceHeight, buf);
			m_pz.readPixels(tmpBuf); copyPixels(tmpBuf, 2*faceWidth, 0, buf);
			m_nz.readPixels(tmpBuf); copyPixels(tmpBuf, 2*faceWidth, faceHeight, buf);
		}
};

/** \brief Check that two texture files have compatible width,height and
 * channel list for cube face mapping.
 *
 * This is necessary at this stage, since we don't want to write special
 * purpose code to handle all the possible combinations of cube faces.
 *
 * \param file1 - first cube face
 * \param file2 - second cube face
 */
void checkCubeFaceCompatible(const IqTexInputFile& file1, const IqTexInputFile& file2)
{
	const CqTexFileHeader& h1 = file1.header();
	const CqTexFileHeader& h2 = file2.header();
	if(h1.width() != h2.width())
		AQSIS_THROW(XqBadTexture, "cube face widths not compatible for "
				<< file1.fileName() << " and " << file2.fileName());
	if(h1.height() != h2.height())
		AQSIS_THROW(XqBadTexture, "cube face heights not compatible for "
				<< file1.fileName() << " and " << file2.fileName());
	if(!h1.channelList().channelTypesMatch(h2.channelList()))
		AQSIS_THROW(XqBadTexture, "cube face channels not compatible for "
				<< file1.fileName() << " and " << file2.fileName());
}

/** \brief Fill an output file header with texture file metadata
 *
 * \param header - header to fill with metadata
 * \param wrapModes - wrapmodes to be saved into the header
 * \param texFormat - texture format to be saved in the header
 * \param paramList - parameter list from the assiciated renderman interface
 *                    call containing optional parameters.
 */
void fillOutputHeader(CqTexFileHeader& header, const SqWrapModes& wrapModes,
		const EqTextureFormat texFormat, const CqRiParamList& paramList)
{
	header.set<Attr::WrapModes>(wrapModes);
	header.set<Attr::TextureFormat>(texFormat);
	header.set<Attr::TileInfo>(SqTileInfo(32,32));
	header.set<Attr::Software>(STRNAME " " VERSION_STR_PRINT);
	if(const char* const* comp = paramList.find<char*>("compression"))
		header.set<Attr::Compression>(*comp);
	if(const TqFloat* quality = paramList.find<TqFloat>("quality"))
		header.set<Attr::CompressionQuality>(static_cast<TqInt>(*quality));

	if(header.channelList().sharedChannelType() == Channel_Float16)
	{
		// Modify the channel list type from float16 data float32 data since we
		// can't handle it with TIFF.  We also don't bother to preserve the
		// channel names since they can't be stored natively by TIFF anyway.
		header.channelList() = CqChannelList(Channel_Float32,
					header.channelList().numChannels());
	}
}

} // unnamed namespace

//------------------------------------------------------------------------------
// Interface function implementations
//------------------------------------------------------------------------------

void makeTexture(const std::string& inFileName, const std::string& outFileName,
		const SqFilterInfo& filterInfo, const SqWrapModes& wrapModes,
		const CqRiParamList& paramList)
{
	// Convert bakefile if necessary.
	std::string inFileRealName = inFileName;
	if(guessFileType(inFileName) == ImageFile_AqsisBake)
	{
		inFileRealName = inFileName + ".tif";
		TqInt bakeRes = static_cast<TqInt>(paramList.find<TqFloat>("bake", 256));
		bakeToTiff(inFileName.c_str(), inFileRealName.c_str(), bakeRes);
	}

	// Open the input file
	boost::shared_ptr<IqTexInputFile> inFile = IqTexInputFile::open(inFileRealName);

	// Take a copy of the file header.  This means that the output file will
	// inherit all the recognized attributes of the input file.
	CqTexFileHeader header = inFile->header();
	fillOutputHeader(header, wrapModes, TextureFormat_Plain, paramList);

	// Create the output file.
	boost::shared_ptr<IqMultiTexOutputFile> outFile
		= IqMultiTexOutputFile::open(outFileName, ImageFile_Tiff, header);

	// Create mipmap, saving to the output file.
	createMipmap(*inFile, inFile->header().channelList().sharedChannelType(),
			*outFile, filterInfo, wrapModes);
}


void makeCubeFaceEnvironment(
		const std::string& inNamePx, const std::string& inNameNx, 
		const std::string& inNamePy, const std::string& inNameNy, 
		const std::string& inNamePz, const std::string& inNameNz, 
		const std::string& outFileName, TqFloat fieldOfView,
		const SqFilterInfo& filterInfo, const CqRiParamList& paramList)
{
	// Open the input files
	boost::shared_ptr<IqTexInputFile> inPx = IqTexInputFile::open(inNamePx);
	boost::shared_ptr<IqTexInputFile> inNx = IqTexInputFile::open(inNameNx);
	boost::shared_ptr<IqTexInputFile> inPy = IqTexInputFile::open(inNamePy);
	boost::shared_ptr<IqTexInputFile> inNy = IqTexInputFile::open(inNameNy);
	boost::shared_ptr<IqTexInputFile> inPz = IqTexInputFile::open(inNamePz);
	boost::shared_ptr<IqTexInputFile> inNz = IqTexInputFile::open(inNameNz);
	// Check that input files have compatible sizes and channel types etc.
	checkCubeFaceCompatible(*inPx, *inNx);
	checkCubeFaceCompatible(*inPx, *inPy);
	checkCubeFaceCompatible(*inPx, *inNy);
	checkCubeFaceCompatible(*inPx, *inPz);
	checkCubeFaceCompatible(*inPx, *inNz);

	// Build the file metadata header
	CqTexFileHeader header = inPx->header();
	header.setWidth(header.width()*3);
	header.setHeight(header.height()*2);
	header.set<Attr::FieldOfViewCot>(1 / std::tan(degToRad(fieldOfView/2)) );
	SqWrapModes wrapModes(WrapMode_Clamp, WrapMode_Clamp);
	fillOutputHeader(header, wrapModes, TextureFormat_CubeEnvironment, paramList);
	header.erase<Attr::DisplayWindow>();

	// Create the output file.
	boost::shared_ptr<IqMultiTexOutputFile> outFile
		= IqMultiTexOutputFile::open(outFileName, ImageFile_Tiff, header);

	// Create mipmap, saving to the output file.
	createMipmap(CqCubeFaceTextureSource(*inPx, *inNx, *inPy, *inNy, *inPz, *inNz),
			inPx->header().channelList().sharedChannelType(),
			*outFile, filterInfo, wrapModes);
}


void makeLatLongEnvironment(const std::string& inFileName, 
		const std::string& outFileName, const SqFilterInfo& filterInfo, 
		const CqRiParamList& paramList)
{
	// Open input file
	boost::shared_ptr<IqTexInputFile> inFile = IqTexInputFile::open(inFileName);

	// Build the file metadata header
	/// \todo: Consider whether we want to start from an empty header instead?
	CqTexFileHeader header = inFile->header();
	SqWrapModes wrapModes(WrapMode_Periodic, WrapMode_Clamp);
	fillOutputHeader(header, wrapModes, TextureFormat_LatLongEnvironment, paramList);

	// Create the output file.
	boost::shared_ptr<IqMultiTexOutputFile> outFile
		= IqMultiTexOutputFile::open(outFileName, ImageFile_Tiff, header);

	// Create mipmap, saving to the output file.
	createMipmap(*inFile, inFile->header().channelList().sharedChannelType(),
			*outFile, filterInfo, wrapModes);
}


void makeShadow(const std::string& inFileName, const std::string& outFileName,
		const CqRiParamList& paramList)
{
	boost::shared_ptr<IqTexInputFile> inFile = IqTexInputFile::open(inFileName);

	// Take a copy of the file header.  This means that the output file will
	// inherit all the recognized attributes of the input file.
	CqTexFileHeader header = inFile->header();

	// Ensure that the header contains 32-bit floating poing data.  It might be
	// possible to relax this requirement to also allow 16-bit OpenEXR "half"
	// data...
	if(header.channelList().sharedChannelType() != Channel_Float32)
		AQSIS_THROW(XqBadTexture, "input for shadow map creation must "
				"contain 32 bit floating point data");

	// Ensure that the screen and camera transformation matrices are present.
	// If not, the texture will be useless for shadow mapping.
	if( header.findPtr<Attr::WorldToCameraMatrix>() == 0
			|| header.findPtr<Attr::WorldToCameraMatrix>() == 0 )
	{
		AQSIS_THROW(XqBadTexture, "world->camera and world->screen"
				" matrices not specified in input file");
	}

	// Set some attributes in the new file header.
	fillOutputHeader(header, SqWrapModes(WrapMode_Trunc, WrapMode_Trunc),
			TextureFormat_Shadow, paramList);

	// Read all pixels into a buffer (not particularly memory efficient...)
	CqTextureBuffer<TqFloat> pixelBuf;
	inFile->readPixels(pixelBuf);
	// Open output file and write pixel data.
	boost::shared_ptr<IqTexOutputFile> outFile
		= IqTexOutputFile::open(outFileName, ImageFile_Tiff, header);
	outFile->writePixels(pixelBuf);
}

void makeOcclusion(const std::vector<std::string>& inFiles,
		const std::string& outFileName, const CqRiParamList& paramList)
{
	boost::shared_ptr<IqMultiTexOutputFile> outFile;

	for(std::vector<std::string>::const_iterator fName = inFiles.begin();
			fName != inFiles.end(); ++fName)
	{
		boost::shared_ptr<IqTexInputFile> inFile = IqTexInputFile::open(*fName);

		// Take a copy of the file header.
		CqTexFileHeader header = inFile->header();
		// Set some extra attributes in the new file header.
		fillOutputHeader(header, SqWrapModes(WrapMode_Trunc, WrapMode_Trunc),
				TextureFormat_Occlusion, paramList);

		// Ensure that the header contains 32-bit floating poing data.
		if(header.channelList().sharedChannelType() != Channel_Float32)
			AQSIS_THROW(XqBadTexture, "input for occlusion map creation doesn't "
					"contain 32 bit floating point data in " << *fName);

		// Ensure that the screen and camera transformation matrices are
		// present.
		if( header.findPtr<Attr::WorldToCameraMatrix>() == 0
				|| header.findPtr<Attr::WorldToCameraMatrix>() == 0 )
		{
			AQSIS_THROW(XqBadTexture, "world->camera and world->screen"
					" matrices not specified in input file" << *fName);
		}

		if(!outFile)
		{
			// Open output file
			outFile = IqMultiTexOutputFile::open(outFileName, ImageFile_Tiff, header);
		}
		else
		{
			// Else make a new subimage with the given metadata.
			outFile->newSubImage(header);
		}

		// Read all pixels into a buffer and write to output file.
		CqTextureBuffer<TqFloat> pixelBuf;
		inFile->readPixels(pixelBuf);
		outFile->writePixels(pixelBuf);
	}
}

} // namespace Aqsis
