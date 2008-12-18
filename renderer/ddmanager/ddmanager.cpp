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


/** \file
		\brief Compliant display device manager.
		\author Paul C. Gregory (pgregory@aqsis.org)
*/


#include	"aqsis.h"

#ifdef	AQSIS_SYSTEM_WIN32
#include	"winsock2.h"
#endif

#include	<cstring>

#include	<boost/static_assert.hpp>

#include	"sstring.h"
#include	"ddmanager.h"
#include	"rifile.h"
#include	"imagebuffer.h"
#include	"shaderexecenv.h"
#include	"logging.h"
#include	"ndspy.h"
#include	"version.h"
#include	"debugdd.h"

namespace Aqsis {


/// Required function that implements Class Factory design pattern for DDManager libraries
IqDDManager* CreateDisplayDriverManager()
{
	Aqsis::log() << debug << "CreateDisplayDriverManager()" << std::endl;
	return new CqDDManager;
}

// Note: This seems like a strange way to initialize static member variable
// m_MemberData. Maybe it need not be static, since DDManager is already a singleton class.
// Maybe this should be done in a DDManager constructor,
// or in the CreateDisplayDriverManager function, above.
SqDDMemberData CqDDManager::m_MemberData("DspyImageOpen", "DspyImageQuery",
        "DspyImageData", "DspyImageClose", "DspyImageDelayClose",
        "r", "g", "b", "a", "z");

TqInt CqDDManager::AddDisplay( const TqChar* name, const TqChar* type, const TqChar* mode, TqInt modeID, TqInt dataOffset, TqInt dataSize, std::map<std::string, void*> mapOfArguments )
{
	/// \todo The shared_ptr should be declared before the if-else block and initialized inside,
	// then the last 2 lines in the if-else blocks should follow afterward. I couldn't figure out
	// how to declare the boost pointer separately from its initialization.
	//if (std::string(type) == "dsm")
	if (false)  // Removed until DSM is integrated after aqsis-1.4
	{
		boost::shared_ptr<CqDisplayRequest> req(new CqDeepDisplayRequest(false, name, type, mode, CqString::hash( mode ), modeID,
		                                        dataOffset,	dataSize, 0.0f, 255.0f, 0.0f, 0.0f, 0.0f, false, false));
		// Create the array of UserParameter structures for all the unrecognised extra parameters,
		// while extracting information for the recognised ones.
		req->PrepareCustomParameters(mapOfArguments);
		m_displayRequests.push_back(req);
	}
	else
	{
		boost::shared_ptr<CqDisplayRequest> req(new CqDisplayRequest(false, name, type, mode, CqString::hash( mode ), modeID,
		                                        dataOffset,	dataSize, 0.0f, 255.0f, 0.0f, 0.0f, 0.0f, false, false));
		// Create the array of UserParameter structures for all the unrecognised extra parameters,
		// while extracting information for the recognised ones.
		req->PrepareCustomParameters(mapOfArguments);
		m_displayRequests.push_back(req);
	}

	// Create the array of UserParameter structures for all the unrecognised extra parameters,
	// while extracting information for the recognised ones.
	//req->PrepareCustomParameters(mapOfArguments);

	//m_displayRequestsNew.push_back(req);

	return ( 0 );
}

CqDisplayRequest::~CqDisplayRequest()
{
	std::vector<UserParameter>::iterator iup;
	for (iup = m_customParams.begin(); iup != m_customParams.end(); ++iup )
	{
		if ( iup->nbytes )
		{
			free(iup->name);
			free(iup->value);
		}
	}
}

TqInt CqDDManager::ClearDisplays()
{
	m_displayRequests.clear();
	return ( 0 );
}

TqInt CqDDManager::OpenDisplays(TqInt width, TqInt height)
{
	// Now go over any requested displays launching the clients.
	std::vector< boost::shared_ptr<CqDisplayRequest> >::iterator i;
	for (i = m_displayRequests.begin(); i!= m_displayRequests.end(); ++i)
	{
		(*i)->LoadDisplayLibrary(m_MemberData, m_DspyPlugin, width, height);
		m_MemberData.m_strOpenMethod = "DspyImageOpen";
		m_MemberData.m_strQueryMethod = "DspyImageQuery";
		m_MemberData.m_strDataMethod = "DspyImageData";
		m_MemberData.m_strCloseMethod = "DspyImageClose";
		m_MemberData.m_strDelayCloseMethod = "DspyImageDelayClose";
	}
	return ( 0 );
}

TqInt CqDDManager::CloseDisplays()
{
	// Now go over any requested displays launching the clients.
	std::vector< boost::shared_ptr<CqDisplayRequest> >::iterator i;
	for (i = m_displayRequests.begin(); i!= m_displayRequests.end(); ++i)
		(*i)->CloseDisplayLibrary();
	return ( 0 );
}

TqInt CqDDManager::DisplayBucket( const CqRegion& DRegion, const IqChannelBuffer* pBuffer )
{
	static CqRandom random( 61 );

	if ( (pBuffer->width() == 0) || (pBuffer->height() == 0) )
		return(0);
	TqUint xmin = DRegion.vecMin().x();
	TqUint ymin = DRegion.vecMin().y();
	TqUint xmaxplus1 = DRegion.vecMax().x();
	TqUint ymaxplus1 = DRegion.vecMax().y();

	// If completely outside the crop rectangle, don't bother sending.
	if( xmaxplus1 <= QGetRenderContext()->cropWindowXMin() ||
		ymaxplus1 <= QGetRenderContext()->cropWindowYMin() ||
		xmin > QGetRenderContext()->cropWindowXMax() ||
		ymin > QGetRenderContext()->cropWindowYMax() )
		return(0);

	std::vector< boost::shared_ptr<CqDisplayRequest> >::iterator i;
	for ( i = m_displayRequests.begin(); i != m_displayRequests.end(); ++i )
	{
		(*i)->DisplayBucket(DRegion, pBuffer);
	}
	return ( 0 );

}

bool CqDDManager::fDisplayNeeds( const TqChar* var )
{
	static TqUlong rgb = CqString::hash( "rgb" );
	static TqUlong rgba = CqString::hash( "rgba" );
	static TqUlong Ci = CqString::hash( "Ci" );
	static TqUlong Oi = CqString::hash( "Oi" );
	static TqUlong Cs = CqString::hash( "Cs" );
	static TqUlong Os = CqString::hash( "Os" );

	TqUlong htoken = CqString::hash( var );

	// Scan all registered displays to see if any of them need the variable specified.
	std::vector< boost::shared_ptr<CqDisplayRequest> >::iterator i;
	for (i = m_displayRequests.begin(); i!= m_displayRequests.end(); ++i)
	{
		if ( (*i)->ThisDisplayNeeds(htoken, rgb, rgba, Ci, Oi, Cs, Os) )
		{
			return true;
		}
	}
	//printf("fDisplayNeeds(%s) returns false ???\n", var);
	return ( false);
}

TqInt CqDDManager::Uses()
{
	if (m_Uses) return m_Uses;

	// Scan all registered displays to combine the required variables.
	std::vector< boost::shared_ptr<CqDisplayRequest> >::iterator i;
	for (i = m_displayRequests.begin(); i!= m_displayRequests.end(); ++i)
	{
		(*i)->ThisDisplayUses(m_Uses);
	}
	return ( m_Uses );
}


namespace {

// Make sure that we've got the correct sizes for the PtDspy* integral
// constants
BOOST_STATIC_ASSERT(sizeof(PtDspyUnsigned32) == 4);
BOOST_STATIC_ASSERT(sizeof(PtDspySigned32) == 4);
BOOST_STATIC_ASSERT(sizeof(PtDspyUnsigned16) == 2);
BOOST_STATIC_ASSERT(sizeof(PtDspySigned16) == 2);
BOOST_STATIC_ASSERT(sizeof(PtDspyUnsigned8) == 1);
BOOST_STATIC_ASSERT(sizeof(PtDspySigned8) == 1);
// the above are in units of sizeof(char); check that this is indeed 8 bits...
BOOST_STATIC_ASSERT(std::numeric_limits<unsigned char>::digits == 8);


/** \brief Select the appropriate output data type given quantization parameters
 *
 * According to the RISpec,
 *
 * \verbatim
 *
 *   The value "one" defines the mapping from floating-point values to fixed
 *   point values. If one is 0, then quantization is not done and values are
 *   output as floating point numbers.  
 *   [...]
 *   Quantized values are computed using the following formula:
 *
 *     value = round( one * value + ditheramplitude * random() );
 *     value = clamp( value, min, max );
 *
 * \endverbatim
 *
 * Unless oneVal == 0 (no quantization), we choose here an integral data type
 * which has sufficient range to encompass both minVal and maxVal.
 *
 * \param oneVal - value which 1 should be mapped into by the quantization
 * \param minVal - minimum value in quantized range.
 * \param maxVal - maximum value in quantized range.
 */
TqUint selectDataFormat(TqFloat oneVal, TqFloat minVal, TqFloat maxVal)
{
	if(oneVal == 0)
	{
		// no quantization
		return PkDspyFloat32;
	}
	else
	{
		// We need to quantize the data; select an integer format which
		// can handle the requested [min, max] range.
		if(minVal >= 0)
		{
			// Minimum is positive; use unsigned data
			if(maxVal <= std::numeric_limits<PtDspyUnsigned8>::max())
				return PkDspyUnsigned8;
			else if (maxVal <= std::numeric_limits<PtDspyUnsigned16>::max())
				return PkDspyUnsigned16;
			else
				return PkDspyUnsigned32;
		}
		else
		{
			// Minimum is negative; use signed data
			if(minVal >= std::numeric_limits<PtDspySigned8>::min()
					&& maxVal <= std::numeric_limits<PtDspySigned8>::max())
				return PkDspySigned8;
			else if(minVal >= std::numeric_limits<PtDspySigned16>::min()
					&& maxVal <= std::numeric_limits<PtDspySigned16>::max())
				return PkDspySigned16;
			else
				return PkDspySigned32;
		}
	}
}

} // anonymous namespace

void CqDisplayRequest::LoadDisplayLibrary( SqDDMemberData& ddMemberData, CqSimplePlugin& dspyPlugin, TqInt width, TqInt height )
{
	// First store the width and height, as this is the first time we know about them.
	m_width = width;
	m_height = height;
	// Get the display mapping from the "display" options, if one exists.
	CqString strDriverFile = "";
	CqString displayType = m_type;
	const CqString* poptDisplay = QGetRenderContext()->poptCurrent()->GetStringOption("display", displayType.c_str());
	if (0 != poptDisplay)
		strDriverFile = poptDisplay[0];
	else
	{
		const CqString* poptDisplayMapping = QGetRenderContext()->poptCurrent()->GetStringOption("display", "mapping");
		if (0 != poptDisplayMapping)
		{
			CqString strMapping = poptDisplayMapping[0];
			strDriverFile.Format(strMapping.c_str(), displayType.c_str());
		}
	}
	Aqsis::log() << debug << "Attempting to load \"" << strDriverFile.c_str() << "\" for display type \""<< displayType.c_str() << "\"" << std::endl;
	// Display type not found.
	if ( strDriverFile.empty() )
		throw( CqString( "Invalid display type \"" ) + CqString( m_type ) + CqString( "\"" ) + CqString(" (") + strDriverFile + CqString(")") );
	if ( strDriverFile != "debugdd")
	{
		// Try to open the file to see if it's really there
		CqRiFile fileDriver( strDriverFile.c_str(), "display" );
		if ( !fileDriver.IsValid() )
			throw( CqString( "Error loading display driver [ " ) + strDriverFile + CqString( " ]" ) );
		CqString strDriverPathAndFile = fileDriver.strRealName();

		// Load the dynamic object and locate the relevant symbols.
		m_DriverHandle = dspyPlugin.SimpleDLOpen( &strDriverPathAndFile );
		if ( m_DriverHandle != NULL )
		{
			m_OpenMethod = (DspyImageOpenMethod)dspyPlugin.SimpleDLSym( m_DriverHandle, &ddMemberData.m_strOpenMethod );
			if (!m_OpenMethod)
			{
				ddMemberData.m_strOpenMethod = "_" + ddMemberData.m_strOpenMethod;
				m_OpenMethod = (DspyImageOpenMethod)dspyPlugin.SimpleDLSym( m_DriverHandle, &ddMemberData.m_strOpenMethod );
			}

			m_QueryMethod = (DspyImageQueryMethod)dspyPlugin.SimpleDLSym( m_DriverHandle, &ddMemberData.m_strQueryMethod );
			if (!m_QueryMethod)
			{
				ddMemberData.m_strQueryMethod = "_" + ddMemberData.m_strQueryMethod;
				m_QueryMethod = (DspyImageQueryMethod)dspyPlugin.SimpleDLSym( m_DriverHandle, &ddMemberData.m_strQueryMethod );
			}

			m_DataMethod = (DspyImageDataMethod)dspyPlugin.SimpleDLSym( m_DriverHandle, &ddMemberData.m_strDataMethod );
			if (!m_DataMethod)
			{
				ddMemberData.m_strDataMethod = "_" + ddMemberData.m_strDataMethod;
				m_DataMethod = (DspyImageDataMethod)dspyPlugin.SimpleDLSym( m_DriverHandle, &ddMemberData.m_strDataMethod );
			}

			m_CloseMethod = (DspyImageCloseMethod)dspyPlugin.SimpleDLSym( m_DriverHandle, &ddMemberData.m_strCloseMethod );
			if (!m_CloseMethod)
			{
				ddMemberData.m_strCloseMethod = "_" + ddMemberData.m_strCloseMethod;
				m_CloseMethod = (DspyImageCloseMethod)dspyPlugin.SimpleDLSym( m_DriverHandle, &ddMemberData.m_strCloseMethod );
			}

			m_DelayCloseMethod = (DspyImageDelayCloseMethod)dspyPlugin.SimpleDLSym( m_DriverHandle, &ddMemberData.m_strDelayCloseMethod );
			if (!m_DelayCloseMethod)
			{
				ddMemberData.m_strDelayCloseMethod = "_" + ddMemberData.m_strDelayCloseMethod;
				m_DelayCloseMethod = (DspyImageDelayCloseMethod)dspyPlugin.SimpleDLSym( m_DriverHandle, &ddMemberData.m_strDelayCloseMethod );
			}
		}
		else
			Aqsis::log() << error << "Could not successfully load the display driver" << std::endl;
	}
	else
	{
		// We are using the in-library internal debugging DD.
		m_OpenMethod =  ::DebugDspyImageOpen ;
		m_QueryMethod =  ::DebugDspyImageQuery ;
		m_DataMethod = ::DebugDspyImageData ;
		m_CloseMethod = ::DebugDspyImageClose ;
		m_DelayCloseMethod = ::DebugDspyDelayImageClose ;
	}

	// Nullified the data part
	m_DataRow = 0;
	m_DataBucket = 0;

	if ( NULL != m_OpenMethod )
	{
		// If the quantization options haven't been set in the RiDisplay call, get the appropriate values out
		// of the RiQuantize option.
		const TqFloat* pQuant = 0;
		if (!m_QuantizeSpecified || !m_QuantizeDitherSpecified)
		{
			if (m_modeID & ModeZ)
				pQuant = QGetRenderContext() ->poptCurrent()->GetFloatOption( "Quantize", "Depth" );
			else
				pQuant = QGetRenderContext() ->poptCurrent()->GetFloatOption( "Quantize", "Color" );
			if ( pQuant && !m_QuantizeSpecified)
			{
				m_QuantizeOneVal = pQuant[0];
				m_QuantizeMinVal = pQuant[1];
				m_QuantizeMaxVal = pQuant[2];
				m_QuantizeSpecified = true;
			}

			if ( pQuant && !m_QuantizeDitherSpecified)
			{
				m_QuantizeDitherVal = pQuant[3];
				m_QuantizeDitherSpecified = true;
			}
		}
		// Prepare the information and call the DspyImageOpen function in the display device.
		TqUint dataFormat = selectDataFormat(m_QuantizeOneVal, m_QuantizeMinVal, m_QuantizeMaxVal);
		if (m_modeID & ( ModeRGB | ModeA | ModeZ) )
		{
			PtDspyDevFormat fmt;

			fmt.type = dataFormat;
			if (m_modeID & ModeA)
			{
				fmt.name = const_cast<char*>( ddMemberData.m_AlphaName );
				m_formats.push_back(fmt);
			}
			if (m_modeID & ModeRGB)
			{
				fmt.name = const_cast<char*>( ddMemberData.m_RedName );
				m_formats.push_back(fmt);
				fmt.name = const_cast<char*>( ddMemberData.m_GreenName );
				m_formats.push_back(fmt);
				fmt.name = const_cast<char*>( ddMemberData.m_BlueName );
				m_formats.push_back(fmt);
			}
			if (m_modeID & ModeZ)
			{
				fmt.name = const_cast<char*>( ddMemberData.m_ZName );
				m_formats.push_back(fmt);
			}
		}
		// Otherwise we are dealing with AOV and should therefore fill in the
		// formats according to its type.
		else
		{
			// Determine the type of the AOV data being displayed.
			TqInt type = QGetRenderContext()->OutputDataType(m_mode.c_str());
			std::string componentNames = "";
			switch (type)
			{
				case type_point:
				case type_normal:
				case type_vector:
				case type_hpoint:
					componentNames = "XYZ";
					break;
				case type_color:
					componentNames = "rgb";
					break;
			}
			// Now create the channels formats.
			PtDspyDevFormat fmt;
			TqUint i;
			for ( i = 0; i < (TqUint) m_AOVSize; i++ )
			{
				if (componentNames.size()>i)
				{

					if (componentNames.substr(i, 1) == "r")
						fmt.name = const_cast<char*>( ddMemberData.m_RedName );
					else if (componentNames.substr(i, 1) == "g")
						fmt.name = const_cast<char*>( ddMemberData.m_GreenName );
					else if (componentNames.substr(i, 1) == "b")
						fmt.name = const_cast<char*>( ddMemberData.m_BlueName );
					else if (componentNames.substr(i, 1) == "a")
						fmt.name = const_cast<char*>( ddMemberData.m_AlphaName );
					else if (componentNames.substr(i, 1) == "z")
						fmt.name = const_cast<char*>( ddMemberData.m_ZName );
					else if (componentNames.substr(i, 1) == "X")
						fmt.name = const_cast<char*>( ddMemberData.m_RedName );
					else if (componentNames.substr(i, 1) == "Y")
						fmt.name = const_cast<char*>( ddMemberData.m_GreenName );
					else if (componentNames.substr(i, 1) == "Z")
						fmt.name = const_cast<char*>( ddMemberData.m_BlueName );
					else
						fmt.name = const_cast<char*>( ddMemberData.m_RedName );
				}
				else
				{
					// by default we will stored into red channel eg. "s" will be saved into 'r' channel
					fmt.name = const_cast<char*>( ddMemberData.m_RedName );
				}
				fmt.type = dataFormat;
				m_AOVnames.push_back(fmt.name);
				m_formats.push_back(fmt);
			}
		}

		// If we got here, we are dealing with a valid display device, so now is the time
		// to fill in the system parameters.
		PrepareSystemParameters();

		// Call the DspyImageOpen method on the display to initialise things.
		PtDspyError err = (*m_OpenMethod)(&m_imageHandle,
		                                  m_type.c_str(), m_name.c_str(),
		                                  width,
		                                  height,
		                                  m_customParams.size(),
		                                  &m_customParams[0],
		                                  m_formats.size(), &m_formats[0],
		                                  &m_flags);

		// Check for an error
		if ( err != PkDspyErrorNone )
		{
			// The display did not successfully open, so clean up after it and leave the
			// request as invalid.
			Aqsis::log() << error << "Cannot open display \"" << m_name << "\" : ";
			switch (err)
			{
				case PkDspyErrorNoMemory:
					Aqsis::log() << error << "Out of memory" << std::endl;
					break;
				case PkDspyErrorUnsupported:
					Aqsis::log() << error << "Unsupported" << std::endl;
					break;
				case PkDspyErrorBadParams:
					Aqsis::log() << error << "Bad params" << std::endl;
					break;
				case PkDspyErrorNoResource:
					Aqsis::log() << error << "No resource" << std::endl;
					break;
				case PkDspyErrorUndefined:
				default:
					Aqsis::log() << error << "Undefined" << std::endl;
					break;
			}

			CloseDisplayLibrary();
			return;
		}
		else
			m_valid = true;

		// Now scan the returned format list to make sure that we pass the data in the order the display wants it.
		std::vector<PtDspyDevFormat>::iterator i;
		for (i=m_formats.begin(); i!=m_formats.end(); i++)
		{
			if (m_modeID & ( ModeRGB | ModeA | ModeZ) )
			{
				if ( i->name == ddMemberData.m_RedName )
					m_dataOffsets.push_back(Sample_Red);
				else if ( i->name == ddMemberData.m_GreenName )
					m_dataOffsets.push_back(Sample_Green);
				else if ( i->name == ddMemberData.m_BlueName )
					m_dataOffsets.push_back(Sample_Blue);
				else if ( i->name == ddMemberData.m_AlphaName )
					m_dataOffsets.push_back(Sample_Alpha);
				else if ( i->name == ddMemberData.m_ZName )
					m_dataOffsets.push_back(Sample_Depth);
			}
			else
			{
				// Scan through the generated names to find the ones specified, and use the index
				// of the found name as an offset into the data from the dataOffset passed in originally.
				TqUint iname;
				for (iname = 0; iname < m_AOVnames.size(); iname++)
				{
					if (i->name == m_AOVnames[iname])
					{
						m_dataOffsets.push_back(m_AOVOffset + iname );
						break;
					}
				}
				// If we got here, and didn't find it, add 0 as the offset, and issue an error.
				if ( iname == m_AOVnames.size() )
				{
					Aqsis::log() << error << "Couldn't find format entry returned from display : " << i->name << std::endl;
					m_dataOffsets.push_back(m_AOVOffset);
				}
			}
		}

		// Determine how big each pixel is by summing the format type sizes.
		m_elementSize = 0;
		std::vector<PtDspyDevFormat>::iterator iformat;
		for (iformat = m_formats.begin(); iformat != m_formats.end(); iformat++)
		{
			TqInt type = iformat->type & PkDspyMaskType;
			switch ( type )
			{
				case PkDspyFloat32:
					m_elementSize += sizeof(PtDspyFloat32);
					break;
				case PkDspyUnsigned32:
				case PkDspySigned32:
					m_elementSize += sizeof(PtDspyUnsigned32);
					break;
				case PkDspyUnsigned16:
				case PkDspySigned16:
					m_elementSize += sizeof(PtDspyUnsigned16);
					break;
				case PkDspyUnsigned8:
				case PkDspySigned8:
					m_elementSize += sizeof(PtDspyUnsigned8);
					break;
			}
		}

		if ( NULL != m_QueryMethod )
		{
			PtDspySizeInfo size;
			err = (*m_QueryMethod)(m_imageHandle, PkSizeQuery, sizeof(size), &size);
			PtDspyOverwriteInfo owinfo;
			owinfo.interactive = 0;
			owinfo.overwrite = 1;
			err = (*m_QueryMethod)(m_imageHandle, PkOverwriteQuery, sizeof(owinfo), &owinfo);
		}
	}
}

void CqDisplayRequest::CloseDisplayLibrary()
{
	// Call the DspyImageClose method on the display to shut things down.
	// If there is a delayed close method, call it in preference.
	if ( m_DelayCloseMethod)
		(*m_DelayCloseMethod)(m_imageHandle);
	else if ( NULL != m_CloseMethod )
		(*m_CloseMethod)(m_imageHandle);

	if (m_DataBucket != 0)
	{
		delete [] m_DataBucket;
		m_DataBucket = 0;
	}
	if (m_DataRow != 0)
	{
		delete [] m_DataRow;
		m_DataRow = 0;
	}

	// Empty out the display request data
	m_CloseMethod = NULL;
	m_DataMethod = NULL;
	m_DelayCloseMethod = NULL;
	m_DriverHandle = 0;
	m_imageHandle = 0;
	m_OpenMethod = NULL;
	m_QueryMethod = NULL;

	/// \note We don't close the driver shared libraries here because doing so caused
	/// some problems with Win2K and FLTK. It seems that detaching from the drive DLL
	/// causes some important data to be altered and when a new window is opened it crashes.
	/// The cleanup of the drivers is left to when the CqDDManager instance closes down, and the
	/// CqSimplePlugin class gets destroyed, which will be at the end of the render, which is fine.
}

void CqDDManager::InitialiseDisplayNameMap()
{
	CqString strConfigFile("displays.ini");
	const CqString* displays = QGetRenderContext()->poptCurrent()->GetStringOption( "searchpath", "display" );
	if ( displays )
		strConfigFile = displays[ 0 ] + "/" + strConfigFile;

	Aqsis::log() << info << "Loading display configuration from file \"" << strConfigFile << "\"" << std::endl;

	CqRiFile fileINI( strConfigFile.c_str(), "display" );
	if ( fileINI.IsValid() )
	{
		// On each line, read the first string, then the second and store them in the map.
		std::string strLine;
		std::istream& strmINI = static_cast<std::istream&>( fileINI );

		while ( std::getline( strmINI, strLine ) )
		{
			std::string strName, strDriverName;
			std::string::size_type iStartN = strLine.find_first_not_of( "\t " );
			std::string::size_type iEndN = strLine.find_first_of( "\t ", iStartN );
			std::string::size_type iStartD = strLine.find_first_not_of( "\t ", iEndN );
			std::string::size_type iEndD = strLine.find_first_of( "\t ", iStartD );
			if ( iStartN != std::string::npos && iEndN != std::string::npos &&
			        iStartD != std::string::npos )
			{
				strName = strLine.substr( iStartN, iEndN );
				strDriverName = strLine.substr( iStartD, iEndD );
				m_mapDisplayNames[ strName ] = strDriverName;
			}
		}
		m_fDisplayMapInitialised = true;
	}
	else
	{
		Aqsis::log() << error << "Could not find " << strConfigFile << " configuration file." << std::endl;
	}
}

/**
  Return the substring with the given index.

  The string \a s is conceptually broken into substrings that are separated by blanks
  or tabs. A continuous sequence of blanks/tabs counts as one individual separator.
  The substring with number \a idx is returned (0-based). If \a idx is higher than the
  number of substrings then an empty string is returned.

  \param s Input string.
  \param idx Index (0-based)
  \return Sub string with given index
*/
std::string CqDDManager::GetStringField( const std::string& s, int idx )
{
	int z = 1;   /* state variable  0=skip whitespace  1=skip chars  2=search end  3=end */
	std::string::const_iterator it;
	std::string::size_type start = 0;
	std::string::size_type end = 0;

	for ( it = s.begin(); it != s.end(); it++ )
	{
		char c = *it;

		if ( idx == 0 && z < 2 )
		{
			z = 2;
		}

		switch ( z )
		{
			case 0:
				if ( c != ' ' && c != '\t' )
				{
					idx--;
					end = start + 1;
					z = 1;
				}
				if ( idx > 0 )
					start++;
				break;
			case 1:
				if ( c == ' ' || c == '\t' )
				{
					z = 0;
				}
				start++;
				break;
			case 2:
				if ( c == ' ' || c == '\t' )
				{
					z = 3;
				}
				else
				{
					end++;
				}
				break;
		}
	}

	if ( idx == 0 )
		return s.substr( start, end - start );
	else
		return std::string( "" );

}

void CqDisplayRequest::ConstructMatrixParameter(const char* name, const CqMatrix* mats, TqInt count, UserParameter& parameter)
{
	// Allocate and fill in the name.
	char* pname = reinterpret_cast<char*>(malloc(strlen(name)+1));
	strcpy(pname, name);
	parameter.name = pname;
	// Allocate a 16 element float array.
	TqInt totallen = 16 * count * sizeof(RtFloat);
	RtFloat* pfloats = reinterpret_cast<RtFloat*>(malloc(totallen));
	TqInt i;
	for ( i=0; i<count; i++)
	{
		const TqFloat* floats = mats[i].pElements();
		TqInt m;
		for (m=0; m<16; m++)
			pfloats[(i*16)+m]=floats[m];
	}
	parameter.value = reinterpret_cast<RtPointer>(pfloats);
	parameter.vtype = 'f';
	parameter.vcount = count * 16;
	parameter.nbytes = totallen;
}

void CqDisplayRequest::ConstructFloatsParameter(const char* name, const TqFloat* floats, TqInt count, UserParameter& parameter)
{
	// Allocate and fill in the name.
	char* pname = reinterpret_cast<char*>(malloc(strlen(name)+1));
	strcpy(pname, name);
	parameter.name = pname;
	// Allocate a float array.
	TqInt totallen = count * sizeof(RtFloat);
	RtFloat* pfloats = reinterpret_cast<RtFloat*>(malloc(totallen));
	// Then just copy the whole lot in one go.
	memcpy(pfloats, floats, totallen);
	parameter.value = reinterpret_cast<RtPointer>(pfloats);
	parameter.vtype = 'f';
	parameter.vcount = count;
	parameter.nbytes = totallen;
}

void CqDisplayRequest::ConstructIntsParameter(const char* name, const TqInt* ints, TqInt count, UserParameter& parameter)
{
	// Allocate and fill in the name.
	char* pname = reinterpret_cast<char*>(malloc(strlen(name)+1));
	strcpy(pname, name);
	parameter.name = pname;
	// Allocate a float array.
	TqInt totallen = count * sizeof(RtInt);
	RtInt* pints = reinterpret_cast<RtInt*>(malloc(totallen));
	// Then just copy the whole lot in one go.
	memcpy(pints, ints, totallen);
	parameter.value = reinterpret_cast<RtPointer>(pints);
	parameter.vtype = 'i';
	parameter.vcount = count;
	parameter.nbytes = totallen;
}

void CqDisplayRequest::ConstructStringsParameter(const char* name, const char** strings, TqInt count, UserParameter& parameter)
{
	// Allocate and fill in the name.
	char* pname = reinterpret_cast<char*>(malloc(strlen(name)+1));
	strcpy(pname, name);
	parameter.name = pname;
	// Allocate enough space for the string pointers, and the strings, in one big block,
	// makes it easy to deallocate later.
	TqInt totallen = count * sizeof(char*);
	TqInt i;
	for ( i = 0; i < count; i++ )
		totallen += (strlen(strings[i])+1) * sizeof(char);
	char** pstringptrs = reinterpret_cast<char**>(malloc(totallen));
	char* pstrings = reinterpret_cast<char*>(&pstringptrs[count]);
	for ( i = 0; i < count; i++ )
	{
		// Copy each string to the end of the block.
		strcpy(pstrings, strings[i]);
		pstringptrs[i] = pstrings;
		pstrings += strlen(strings[i])+1;
	}
	parameter.value = reinterpret_cast<RtPointer>(pstringptrs);
	parameter.vtype = 's';
	parameter.vcount = count;
	parameter.nbytes = totallen;
}

void CqDisplayRequest::PrepareCustomParameters( std::map<std::string, void*>& mapParams )
{
	// Scan the map of extra parameters
	std::map<std::string, void*>::iterator param;
	for ( param = mapParams.begin(); param != mapParams.end(); param++ )
	{
		// First check if it is one of the recognised parameters that the renderer should handle.
		if (param->first.compare("quantize")==0)
		{
			// Extract the quantization information and store it with the display request.
			const RtFloat* floats = static_cast<float*>( param->second );
			m_QuantizeZeroVal = floats[0];
			m_QuantizeOneVal = floats[1];
			m_QuantizeMinVal = floats[2];
			m_QuantizeMaxVal = floats[3];
			m_QuantizeSpecified = true;
		}
		else if (param->first.compare("dither")==0)
		{
			// Extract the quantization information and store it with the display request.
			const RtFloat* floats = static_cast<float*>( param->second );
			m_QuantizeDitherVal = floats[0];
			m_QuantizeDitherSpecified = true;
		}
		else
		{
			// Otherwise, construct a UserParameter structure and fill in the details.
			CqPrimvarToken tok;
			try
			{
				tok = QGetRenderContext()->tokenDict().parseAndLookup(param->first.c_str());
			}
			catch (XqValidation& e)
			{
				Aqsis::log() << error << e.what() << std::endl;
				return;
			}

			// Check the parameter type is uniform, not valid for non-surface requests otherwise.
			if ( tok.Class() != class_uniform )
			{
				assert( false );
				continue;
			}

			UserParameter parameter;
			parameter.name = 0;
			parameter.value = 0;
			parameter.vtype = 0;
			parameter.vcount = 0;
			parameter.nbytes = 0;

			switch ( tok.type() )
			{
				case type_string:
				{
					const char** strings = static_cast<const char**>( param->second );
					ConstructStringsParameter(tok.name().c_str(), strings, tok.count(), parameter);
				}
				break;

				case type_float:
				{
					const RtFloat* floats = static_cast<RtFloat*>( param->second );
					ConstructFloatsParameter(tok.name().c_str(), floats, tok.count(), parameter);
				}
				break;

				case type_integer:
				{
					const RtInt* ints = static_cast<RtInt*>( param->second );
					ConstructIntsParameter(tok.name().c_str(), ints, tok.count(), parameter);
				}
				break;
				default:
					break;
			}
			m_customParams.push_back(parameter);
		}
	}
}

void CqDisplayRequest::PrepareSystemParameters()
{
	// Fill in "standard" parameters that the renderer must supply
	UserParameter parameter;

	// "NP"
	CqMatrix matWorldToScreen;
	QGetRenderContext() ->matSpaceToSpace( "world", "screen", NULL, NULL, QGetRenderContextI()->Time(), matWorldToScreen );
	ConstructMatrixParameter("NP", &matWorldToScreen, 1, parameter);
	m_customParams.push_back(parameter);

	// "Nl"
	CqMatrix matWorldToCamera;
	QGetRenderContext() ->matSpaceToSpace( "world", "camera", NULL, NULL, QGetRenderContextI()->Time(), matWorldToCamera );
	ConstructMatrixParameter("Nl", &matWorldToCamera, 1, parameter);
	m_customParams.push_back(parameter);

	// "near"
	TqFloat nearval = static_cast<TqFloat>( QGetRenderContext() ->poptCurrent()->GetFloatOption( "System", "Clipping" ) [ 0 ] );
	ConstructFloatsParameter("near", &nearval, 1, parameter);
	m_customParams.push_back(parameter);

	// "far"
	TqFloat farval = static_cast<TqFloat>( QGetRenderContext() ->poptCurrent()->GetFloatOption( "System", "Clipping" ) [ 1 ] );
	ConstructFloatsParameter("far", &farval, 1, parameter);
	m_customParams.push_back(parameter);

	// "OriginalSize"
	TqInt OriginalSize[2];
	OriginalSize[0] = QGetRenderContext() ->poptCurrent()->GetIntegerOption( "System", "Resolution" ) [ 0 ];
	OriginalSize[1] = QGetRenderContext() ->poptCurrent()->GetIntegerOption( "System", "Resolution" ) [ 1 ];
	ConstructIntsParameter("OriginalSize", OriginalSize, 2, parameter);
	m_customParams.push_back(parameter);

	// "origin"
	TqInt origin[2];
	origin[0] = QGetRenderContext()->cropWindowXMin();
	origin[1] = QGetRenderContext()->cropWindowYMin();
	ConstructIntsParameter("origin", origin, 2, parameter);
	m_customParams.push_back(parameter);

	// "PixelAspectRatio"
	TqFloat PixelAspectRatio = QGetRenderContext() ->poptCurrent()->GetFloatOption( "System", "PixelAspectRatio" ) [ 0 ];
	ConstructFloatsParameter("PixelAspectRatio", &PixelAspectRatio, 1, parameter);
	m_customParams.push_back(parameter);

	// "Software"
	char SoftwareName[ 80 ];
	const char* Software = SoftwareName;
	sprintf( SoftwareName, "%s %s (%s %s)", STRNAME, VERSION_STR, __DATE__, __TIME__ );
	ConstructStringsParameter("Software", &Software, 1, parameter);
	m_customParams.push_back(parameter);

	// "HostComputer"
	char HostComputerName[255];
	const char* HostComputer = HostComputerName;
#ifdef AQSIS_SYSTEM_WIN32

	WSADATA wsaData;
	WSAStartup( MAKEWORD( 2, 0 ), &wsaData );
#endif // AQSIS_SYSTEM_WIN32

	gethostname( HostComputerName, 255 );
#ifdef	AQSIS_SYSTEM_WIN32

	WSACleanup();
#endif

	ConstructStringsParameter("HostComputer", &HostComputer, 1, parameter);
	m_customParams.push_back(parameter);
}

void CqDisplayRequest::DisplayBucket( const CqRegion& DRegion, const IqChannelBuffer* pBuffer )
{
	// If the display is not validated, don't send it data.
	// Or if a DspyImageData function was not found for
	// this display request, then we cannot continue
	if ( !m_valid || !m_DataMethod )
		return;

	TqUint	xmin = DRegion.vecMin().x();
	TqUint	ymin = DRegion.vecMin().y();
	TqUint	xmaxplus1 = DRegion.vecMax().x();
	TqUint	ymaxplus1 = DRegion.vecMax().y();
	PtDspyError err;

	// Dispatch to display sub-type methods
	// Copy relevant data from the bucket and store locally,
	// while quantizing and/or compressing
	FormatBucketForDisplay( DRegion, pBuffer );
	// Now that the bucket data has been constructed, send it to the display
	// either lines by lines or bucket by bucket.
	// Check if the display needs scanlines, and if so, accumulate bucket data
	// until a scanline is complete. Send to display when complete.
	if (m_flags.flags & PkDspyFlagsWantsScanLineOrder)
	{
		if (CollapseBucketsToScanlines( DRegion ))
		{
			// Filled a scan line: time to send complete rows to display
			SendToDisplay(ymin, ymaxplus1);
		}
	}
	else
	{
		// Send the bucket information as they come in
		err = (m_DataMethod)(m_imageHandle, xmin, xmaxplus1, ymin, ymaxplus1, m_elementSize, m_DataBucket);
	}

}

void CqDisplayRequest::FormatBucketForDisplay( const CqRegion& DRegion, const IqChannelBuffer* pBuffer )
{
	static CqRandom random( 61 );

	if (m_DataBucket == 0)
		m_DataBucket = new unsigned char[m_elementSize * static_cast<int>(DRegion.area())];
	if ((m_flags.flags & PkDspyFlagsWantsScanLineOrder) && m_DataRow == 0)
	{
		m_DataRow = new unsigned char[m_elementSize * m_width * m_height];
	}

	SqImageSample val;
	// Fill in the bucket data for each channel in each element, honoring the requested order and formats.
	unsigned char* pdata = m_DataBucket;
	TqUint y;

	for ( y = 0; y < pBuffer->height(); ++y )
	{
		TqUint x;
		for ( x = 0; x < pBuffer->width(); ++x )
		{
			TqInt index = 0;
			std::vector<PtDspyDevFormat>::iterator iformat;
			double s = random.RandomFloat();
			for (iformat = m_formats.begin(); iformat != m_formats.end(); iformat++)
			{
				double value = 0.0;
				try
				{
					value = (*pBuffer)(x, y, pBuffer->getChannelIndex(iformat->name))[0];
				}
				catch(std::string v)
				{
					std::cout << "Invalid channel name " << iformat->name << std::endl;
				}
				if ( m_QuantizeOneVal != 0 )
				{
					// Perform the quantization
					value = lround(m_QuantizeZeroVal + value * (m_QuantizeOneVal - m_QuantizeZeroVal) + ( m_QuantizeDitherVal * s ) );
					value = clamp<double>(value, m_QuantizeMinVal, m_QuantizeMaxVal) ;
				}
				TqInt type = iformat->type & PkDspyMaskType;
				switch (type)
				{
					case PkDspyFloat32:
						reinterpret_cast<PtDspyFloat32*>(pdata)[0] = value;
						pdata += sizeof(PtDspyFloat32);
						break;
					case PkDspyUnsigned32:
						/** \note: We need to do this extra clamp as the quantisation values are stored
						    single precision floats, as mandated by the spec.,
						    but single precision floats cannot accurately represent the maximum
						    PtDspyUnsigned32 value of 4294967295. Doing this ensures that the
						    PtDspyUnsigned32 value is clamped before being cast, and the clamp is
						    performed in double precision math to retain accuracy.
						*/
						value = clamp<double>(value, 0,
								std::numeric_limits<PtDspyUnsigned32>::max());
						reinterpret_cast<PtDspyUnsigned32*>(pdata)[0] = static_cast<PtDspyUnsigned32>( value );
						pdata += sizeof(PtDspyUnsigned32);
						break;
					case PkDspySigned32:
						value = clamp<double>(value,
								std::numeric_limits<PtDspySigned32>::min(),
								std::numeric_limits<PtDspySigned32>::max());
						reinterpret_cast<PtDspySigned32*>(pdata)[0] = static_cast<PtDspySigned32>( value );
						pdata += sizeof(PtDspySigned32);
						break;
					case PkDspyUnsigned16:
						reinterpret_cast<PtDspyUnsigned16*>(pdata)[0] = static_cast<PtDspyUnsigned16>( value );
						pdata += sizeof(PtDspyUnsigned16);
						break;
					case PkDspySigned16:
						reinterpret_cast<PtDspySigned16*>(pdata)[0] = static_cast<PtDspySigned16>( value );
						pdata += sizeof(PtDspySigned16);
						break;
					case PkDspyUnsigned8:
						reinterpret_cast<PtDspyUnsigned8*>(pdata)[0] = static_cast<PtDspyUnsigned8>( value );
						pdata += sizeof(PtDspyUnsigned8);
						break;
					case PkDspySigned8:
						reinterpret_cast<PtDspySigned8*>(pdata)[0] = static_cast<PtDspySigned8>( value );
						pdata += sizeof(PtDspySigned8);
						break;
				}
				index++;
			}
		}
	}
}


void CqDeepDisplayRequest::FormatBucketForDisplay( const CqRegion& DRegion, const IqChannelBuffer* pBuffer )
{

}

//-----------------------------------------------------------------------------
// Return true if a scanline of buckets has been accumulated, false otherwise.
//-----------------------------------------------------------------------------
bool CqDisplayRequest::CollapseBucketsToScanlines( const CqRegion& DRegion )
{

	unsigned char* pdata = m_DataBucket;
	TqUint	xmin = DRegion.vecMin().x();
	TqUint	ymin = DRegion.vecMin().y();
	TqUint	xmaxplus1 = DRegion.vecMax().x();
	TqUint	ymaxplus1 = DRegion.vecMax().y();
	TqUint x, y;

	for (y = ymin; y < ymaxplus1; y++)
	{
		for (x = xmin; x < xmaxplus1; x++)
		{
			memcpy(&(m_DataRow[m_width * m_elementSize * (y - ymin) + m_elementSize * x]), pdata, m_elementSize);
			pdata += m_elementSize;
		}
	}

	if (xmaxplus1 >= m_width)
	{
		// Filled a scan line
		Aqsis::log() << debug << "filled a scanline" << std::endl;
		return true;
	}
	return false;
}

bool CqDeepDisplayRequest::CollapseBucketsToScanlines( const CqRegion& DRegion )
{
	return false;
}

void CqDisplayRequest::SendToDisplay(TqUint ymin, TqUint ymaxplus1)
{
	//Aqsis::log() << debug << "CqDisplayRequest::SendToDisplay()" << std::endl;
	TqUint y;
	PtDspyError err;
	unsigned char* pdata = m_DataRow;

	// send to the display one line at a time
	for (y = ymin; y < ymaxplus1; y++)
	{
		err = (m_DataMethod)(m_imageHandle, 0, m_width, y, y+1, m_elementSize, pdata);
		pdata += m_elementSize * m_width;
	}
}

void CqDeepDisplayRequest::SendToDisplay(TqUint ymin, TqUint ymaxplus1)
{

}

bool CqDisplayRequest::ThisDisplayNeeds( const TqUlong& htoken, const TqUlong& rgb, const TqUlong& rgba,
        const TqUlong& Ci, const TqUlong& Oi, const TqUlong& Cs, const TqUlong& Os )
{
	bool usage = ( ( m_modeHash == rgba ) || ( m_modeHash == rgb ) );

	if ( (( htoken == Ci )|| ( htoken == Cs))&& usage )
		return ( true );
	else if ( (( htoken == Oi )|| ( htoken == Os))&& usage )
		return ( true );
	else if ( m_modeHash == htoken  )
		return ( true );
	return false;
}

void CqDisplayRequest::ThisDisplayUses( TqInt& Uses )
{
	TqInt ivar;

	for ( ivar = 0; ivar < EnvVars_Last; ++ivar )
	{
		if ( m_modeHash == gVariableTokens[ ivar ] )
			Uses |= 1 << ivar;
	}

}

} // namespace Aqsis

