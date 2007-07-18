// Aqsis
// Copyright © 1997 - 2001, Paul C. Gregory
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
		\brief Implements the common piqsl functionality.
		\author Paul C. Gregory (pgregory@aqsis.org)
*/

#include "piqslbase.h"
#include "book.h"
#include "tinyxml.h"
#include "file.h"

#include <algorithm>

#include <FL/Fl_File_Chooser.H>

START_NAMESPACE( Aqsis )


boost::shared_ptr<CqBook> CqPiqslBase::addNewBook(std::string name)
{
	boost::shared_ptr<CqBook> newBook(new CqBook(name));
	m_books.push_back(newBook);
	m_currentBook = newBook;

	return(newBook);
}

void CqPiqslBase::setCurrentBook(boost::shared_ptr<CqBook>& book)
{
	if(std::find(m_books.begin(), m_books.end(), book) != m_books.end())
		m_currentBook = book;
}


boost::shared_ptr<CqBook>& CqPiqslBase::currentBook()
{
	return(m_currentBook);
}

TqUlong CqPiqslBase::addImageToCurrentBook(boost::shared_ptr<CqImage>& image)
{
	Aqsis::log() << Aqsis::debug << "Piqsl adding image" << std::endl;
	if(!m_currentBook)
	{
		std::map<std::string, boost::shared_ptr<CqBook> >::size_type numBooks = m_books.size();
		std::stringstream strBkName;
		strBkName << "Book" << numBooks+1;
		addNewBook(strBkName.str());
	}
	TqUlong id = currentBook()->addImage(image);
	Aqsis::log() << Aqsis::debug << "Piqsl connecting image to framebuffer" << std::endl;
	if(currentBook()->framebuffer())
		currentBook()->framebuffer()->connect(image);
	return( id );	
}

void CqPiqslBase::saveConfiguration()
{
	saveConfigurationAs(m_currentConfigName);
}

void CqPiqslBase::saveConfigurationAs(const std::string& name)
{
	Fl::lock();
#if	1
	if(!name.empty())
	{
		std::string _base = CqFile::basePath(name);
		m_currentConfigName = name;
		TiXmlDocument doc(name);
		TiXmlDeclaration* decl = new TiXmlDeclaration("1.0","","yes");
		TiXmlElement* booksXML = new TiXmlElement("Books");

		std::vector<boost::shared_ptr<CqBook> >::iterator book;
		for(book = m_books.begin(); book != m_books.end(); ++book) 
		{
			TiXmlElement* bookXML = new TiXmlElement("Book");
			bookXML->SetAttribute("name", (*book)->name());
			booksXML->LinkEndChild(bookXML);
			TiXmlElement* imagesXML = new TiXmlElement("Images");

			CqBook::TqImageListIterator image;
			for(image = (*book)->imagesBegin(); image != (*book)->imagesEnd(); ++image)
			{
				// Serialise the image first.
				(*image)->serialise(_base);
				TiXmlElement* imageXML = (*image)->serialiseToXML();
				imagesXML->LinkEndChild(imageXML);
			}
			bookXML->LinkEndChild(imagesXML);
		}
		doc.LinkEndChild(decl);
		doc.LinkEndChild(booksXML);
		doc.SaveFile(name);
	}
#else
	// A possible alternative saving mechanism, that embeds internal images in the XML file.
	// Not properly tested, and as yet not supported.
	if(name != NULL)
	{
		m_currentConfigName = name;
		TiXmlDocument doc(name);
		TiXmlDeclaration* decl = new TiXmlDeclaration("1.0","","yes");
		TiXmlElement* booksXML = new TiXmlElement("Books");

		std::map<std::string, boost::shared_ptr<CqBook> >::iterator book;
		for(book = m_books.begin(); book != m_books.end(); ++book) 
		{
			TiXmlElement* bookXML = new TiXmlElement("Book");
			bookXML->SetAttribute("name", book->first);
			booksXML->LinkEndChild(bookXML);
			TiXmlElement* imagesXML = new TiXmlElement("Images");

			std::map<TqUlong, boost::shared_ptr<CqImage> >::iterator image;
			for(image = book->second->images().begin(); image != book->second->images().end(); ++image)
			{
				// Serialise the image first.
				TiXmlElement* imageXML = image->second->serialiseToXML();
				imagesXML->LinkEndChild(imageXML);
			}
			bookXML->LinkEndChild(imagesXML);
		}
		doc.LinkEndChild(decl);
		doc.LinkEndChild(booksXML);
		doc.SaveFile(name);
	}
#endif
	Fl::unlock();
}

void CqPiqslBase::loadConfiguration(const std::string& name)
{
	Fl::lock();

	if(!name.empty())
	{
		std::string _base = CqFile::basePath(name);
		m_currentConfigName = name;
		TiXmlDocument doc(name);
		bool loadOkay = doc.LoadFile();
		if(loadOkay)
		{
			TiXmlElement* booksXML = doc.RootElement();
			if(booksXML)
			{
				TiXmlElement* bookXML = booksXML->FirstChildElement("Book");
				while(bookXML)
				{
					std::string bookName = bookXML->Attribute("name");
					addNewBook(bookName);

					TiXmlElement* imagesXML = bookXML->FirstChildElement("Images");
					if(imagesXML)
					{
						TiXmlElement* imageXML = imagesXML->FirstChildElement("Image");
						while(imageXML)
						{
							std::string imageName("");
							std::string imageFilename("");
							TiXmlElement* nameXML = imageXML->FirstChildElement("Name");
							if(nameXML && nameXML->GetText())
								imageName = nameXML->GetText();
							TiXmlElement* fileNameXML = imageXML->FirstChildElement("Filename");
							if(fileNameXML && fileNameXML->GetText())
								imageFilename = fileNameXML->GetText();
							loadImageToCurrentBook(imageName, imageFilename);
							imageXML = imageXML->NextSiblingElement("Image");
						}
					}
					bookXML = bookXML->NextSiblingElement("Book");
				}
			}
		}
		else
		{
			Aqsis::log() << Aqsis::error << "Failed to load configuration file" << std::endl;
		}
	}
	Fl::unlock();
}

void CqPiqslBase::loadImageToCurrentBook(const std::string& name, const std::string& filename)
{
	boost::shared_ptr<CqImage> newImage(new CqImage(name));
	newImage->loadFromTiff(filename);
	TqUlong id = addImageToCurrentBook(newImage);

	setCurrentImage(id);
}

//---------------------------------------------------------------------

END_NAMESPACE( Aqsis )
