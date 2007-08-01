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

//
// This is a Display Driver that was written to comply with the PhotoRealistic
// RenderMan Display Driver Implementation Guide (on the web at:
// www.pixar.com/products/rendermandocs/toolkit/Toolkit/dspy.html).
//

/** \file
 * 
 * \author Zachary L. Carter (zcarter@aqsis.org)
 * 
 * \brief Implementation of a deep texture file writer to create tiled binary DTEX files.
*/

#include "dtex.h"

namespace Aqsis
{

const long long* magicNumber = (long long*)"\0x89AqD\0x0b\0x0a\0x16\0x0a";

CqDeepTexture::CqDeepTexture(std::string filename, int imageWidth, int imageHeight, int tileWidth, int tileHeight, int numberOfChannels, int bytesPerChannel)
{
	const int tilesX = imageWidth/tileWidth;
	const int tilesY = imageHeight/tileHeight;
	const int numberOfTiles = tilesX*tilesY; 
	
	m_fileHeader.magicNumber = *magicNumber;
	m_fileHeader.fileSize = 0; ///< We can't determine this until we receive all the data. We should write once, then seek back and re-write this field before file close
	m_fileHeader.imageWidth = imageWidth;
	m_fileHeader.imageHeight = imageHeight;
	m_fileHeader.numberOfChannels = numberOfChannels;
	m_fileHeader.bytesPerChannel = bytesPerChannel;
	m_fileHeader.headerSize = sizeof(SqDtexFileHeader) + numberOfTiles*sizeof(SqTileTableEntry);
	m_fileHeader.dataSize = 0; ///< Can't know this early
	m_fileHeader.tileWidth = tileWidth;
	m_fileHeader.tileHeight = tileHeight;
	m_fileHeader.numberOfTiles = numberOfTiles;
	
	m_tileTable.reserve(numberOfTiles);
	
	// Open a file in binary write mode
	m_dtexFile.Open( filename.c_str(), "", std::ios::out );
	
	// Write the file header
	
	//Seek forward to reserve space for writting the tile table later
}

void CqDeepTexture::setTileData( int xmin, int ymin, int xmax, int ymax, const unsigned char *data )
{
	// Identify what tiles the passed-in data covers
	// If those tiles are not already in the tile map, add them
	
	// Copy the data into their respective tiles
	// If a tile is filled, write it to file, update the tileTable with a m_tileTable.push_back(), and reclaim its memory

	// If all tiles have been written, write the tileTable, re-write the datasSize and fileSize fields in the file header, and close the image
	
	// Return
}

void CqDeepTexture::writeFileHeader()
{

}

void CqDeepTexture::writeTile()
{
	
}

} // namespace Aqsis
