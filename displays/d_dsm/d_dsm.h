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
 * \brief Implementation of the deep shadow display driver which is
 * responsible for compression of deep shadow maps and output to file.
 */

#ifndef D_DSM_H_INCLUDED
#define D_DSM_H_INCLUDED

#include <string>
#include <iostream>
#include <sstream>
#include <iomanip>
#include <iosfwd>
#include <vector>
#include <list>

struct tag
{
  unsigned char value[4];
};

//------------------------------------------------------------------------------
/** \brief A deep shadow map processing class with functions to compress visibility data, 
 *  store store compressed and save to file or other display type.
 *
 */
class DeepShadowDisplay
{
	public:
	  DeepShadowDisplay(const char *filename, int width, int height, int bpp);
	  ~DeepShadowDisplay();
	  int processData( void *th, int x, int y, int xmax, int ymax, const unsigned char *data );     // processes one bucket of rendered pixels
	
	  int getColors();      // return the number of colors in the image
	  int saveFile();       // saves the desired .xpm image
	  int bpp();         // returns the number of bytes per pixel in this image
	  int width();       // returns image width
	  int height();      // returns image height

	private:

	  int addColor( struct aspRGB cor );      // adds a new color to the buffer and resizes the buffers if it needs to
	  int colorExists( struct aspRGB cor );   // iterates throught the whole buffer checking if the provided color is already there. returns the index
	
	  std::string aspFilename;                // this will contain the string with the filename
	  std::vector<struct tag> tags;           // buffer that will contain the color tags, 3 chars per color (max 65536 colors)
	  std::vector<struct aspRGB> cores;       // color buffer
	  std::vector<unsigned int> pixels;       // buffer that will contain the indexes into the palette buffer
	  unsigned long bufferSize;               // the current size of the color buffer
	  unsigned long numColors;                // the number of colors in the image
	  struct tag curtag;                      // variable to keep track of the tags chars
	  int aspBpp;                             // image number of bytes per pixel
	  int aspWidth;                           // image width
	  int aspHeight;                          // image height

};

#endif // D_DSM_H_INCLUDED
