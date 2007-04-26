// Aqsis
// Copyright © 1997 - 2001, Paul C. Gregory
//
// Contact: pgregory@aqsis.com
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
		\brief Implements the basic framebuffer functionality.
		\author Paul C. Gregory (pgregory@aqsis.com)
*/

#include "framebuffer.h"
#include "image.h"


void Fl_FrameBuffer_Widget::draw(void)
{
	if(image)
		fl_draw_image(image,x(),y(),w,h,d,w*d); // draw image
}


START_NAMESPACE( Aqsis )

CqFramebuffer::CqFramebuffer(TqUlong width, TqUlong height)
{
	m_theWindow = new Fl_Window(width, height);
	m_uiImageWidget = new Fl_FrameBuffer_Widget(0,0, width, height, 4, 0);
	m_theWindow->resizable(m_uiImageWidget);
//	m_theWindow->label(thisClient.m_image.m_filename.c_str());
	m_theWindow->end();
	Fl::visual(FL_RGB);
	m_theWindow->show();
}

CqFramebuffer::~CqFramebuffer()
{
	disconnect();
	m_theWindow->hide();
	delete m_theWindow;
}

void CqFramebuffer::show()
{
	m_theWindow->show();
}

void CqFramebuffer::connect(boost::shared_ptr<CqImage>& image)
{
	m_associatedImage = image;	
}

/*void CqFramebuffer::connect(boost::shared_ptr<CqConduit<CqImage, CqFramebuffer> >& conduit)
{
	if(m_associatedImage)
		m_associatedImage.reset();
	m_associatedImage = conduit;
}*/

void CqFramebuffer::disconnect()
{
	m_associatedImage.reset();
}

END_NAMESPACE( Aqsis )
