// Aqsis
// Copyright � 1997 - 2001, Paul C. Gregory
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
		\brief Implements an image class getting it's data from the Dspy server.
		\author Paul C. Gregory (pgregory@aqsis.org)
*/


#include	"aqsis.h"

#include	<fstream>
#include	<map>
#include	<algorithm>

#include	<boost/archive/iterators/base64_from_binary.hpp>
#include	<boost/archive/iterators/transform_width.hpp>
#include	<boost/archive/iterators/insert_linebreaks.hpp>

#include	"file.h"
#include	"displayserverimage.h"
#include	"aqsismath.h"
#include 	"logging.h"

START_NAMESPACE( Aqsis )

//---------------------------------------------------------------------
/** Close the socket this client is associated with.
 */
void CqDisplayServerImage::close()
{
	m_socket.close();
}


//----------------------------------------------------------------------

/// Do-nothing deleter for holding non-reference counted data in a boost::shared_ptr/array.
void nullDeleter(const void*)
{ }

void CqDisplayServerImage::acceptData(TqUlong xmin, TqUlong xmaxplus1, TqUlong ymin, TqUlong ymaxplus1, TqInt elementSize, const unsigned char* bucketData )
{
	assert(elementSize == m_realData->channelInfo().bytesPerPixel());

	// yuck.  To fix the casts, refactor Image to use signed ints.
	TqInt cropXmin = static_cast<TqInt>(xmin) - static_cast<TqInt>(m_originX);
	TqInt cropYmin = static_cast<TqInt>(ymin) - static_cast<TqInt>(m_originY);

	boost::mutex::scoped_lock lock(mutex());

	if(m_realData && m_displayData)
	{
		// The const_cast below is ugly, but I don't see how to avoid it
		// without some notion of "const constructor" which isn't present in
		// C++
		const CqMixedImageBuffer bucketBuf(m_realData->channelInfo(),
				boost::shared_array<TqUchar>(const_cast<TqUchar*>(bucketData),
					nullDeleter), xmaxplus1 - xmin, ymaxplus1 - ymin);

		m_realData->copyFrom(bucketBuf, cropXmin, cropYmin);
		m_displayData->compositeOver(bucketBuf, m_displayMap, cropXmin, cropYmin);

		if(m_updateCallback)
			m_updateCallback(xmin, ymin, xmaxplus1-xmin, ymaxplus1-ymin);
	}
}

void CqDisplayServerImage::serialise(const std::string& folder)
{
	// Generate a unique name for the managed image in the specified folder.
	std::string _ext = CqFile::extension(name());
	std::string _basename = CqFile::baseName(name());
	std::stringstream strFilename;
	strFilename << folder << CqFile::pathSep() << name();
	FILE* tfile;
	int index = 1;
	while((tfile = fopen(strFilename.str().c_str(), "r")) != NULL)
	{
		fclose(tfile);
		strFilename.clear();
		strFilename.str("");
		strFilename << folder << CqFile::pathSep() << _basename<< "." << index << _ext;
		index++;
	}
		
	setFilename(strFilename.str());
	saveToTiff(strFilename.str());
}



// Define a base64 encoding stream iterator using the boost archive data flow iterators.
typedef 
    boost::archive::iterators::insert_linebreaks<         // insert line breaks every 72 characters
        boost::archive::iterators::base64_from_binary<    // convert binary values ot base64 characters
            boost::archive::iterators::transform_width<   // retrieve 6 bit integers from a sequence of 8 bit bytes
                const char *,
                6,
                8
            >
        > 
        ,72
    > 
    base64_text; // compose all the above operations in to a new iterator

TiXmlElement* CqDisplayServerImage::serialiseToXML()
{
	TiXmlElement* imageXML = new TiXmlElement("Image");

	TiXmlElement* typeXML = new TiXmlElement("Type");
	TiXmlText* typeText = new TiXmlText("managed");
	typeXML->LinkEndChild(typeText);
	imageXML->LinkEndChild(typeXML);

	TiXmlElement* nameXML = new TiXmlElement("Name");
	TiXmlText* nameText = new TiXmlText(name());
	nameXML->LinkEndChild(nameText);
	imageXML->LinkEndChild(nameXML);

	if(filename().empty())
	{
		TiXmlElement* dataXML = new TiXmlElement("Bitmap");
		std::stringstream base64Data;
		size_t dataLen = m_displayData->width() * m_displayData->height() * numChannels() * sizeof(TqUchar);
		std::copy(	base64_text(BOOST_MAKE_PFTO_WRAPPER(m_displayData->rawData())), 
					base64_text(BOOST_MAKE_PFTO_WRAPPER(m_displayData->rawData() + dataLen)), 
					std::ostream_iterator<char>(base64Data));
		TiXmlText* dataTextXML = new TiXmlText(base64Data.str());
		dataTextXML->SetCDATA(true);
		dataXML->LinkEndChild(dataTextXML);
		imageXML->LinkEndChild(dataXML);
	}
	else
	{
		TiXmlElement* filenameXML = new TiXmlElement("Filename");
		TiXmlText* filenameText = new TiXmlText(filename());
		filenameXML->LinkEndChild(filenameText);
		imageXML->LinkEndChild(filenameXML);
	}

	return(imageXML);
}

END_NAMESPACE( Aqsis )
