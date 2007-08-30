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
 * \brief Implementation of a deep texture file writer to create tiled binary DTEX files.
 */
#ifndef SHARED_H_INCLUDED
#define SHARED_H_INCLUDED

//Aqsis primary header
#include "aqsis.h"

// Standard libraries
#include <fstream>
#include <tiff.h> //< Including (temporarily) in order to get the typedefs like uint32

namespace Aqsis 
{

// Magic number for a DTEX file is: "\0x89AqD\0x0b\0x0a\0x16\0x0a" Note 0x417144 represents ASCII AqD
static const char dtexMagicNumber[8] = { 0x89, 'A', 'q', 'D', 0x0b, 0x0a, 0x16, 0x0a };

// To make eventual refactor to using Aqsis types nicer:
typedef uint32 TqUint32;

//------------------------------------------------------------------------------
/** \brief A structure storing the constant/global DTEX file information
 *
 * This file header represents the first bytes of data in the file.
 * It is immediately followed by the tile table.
 */
/// \todo This structure should be stored in a shared location since it is used in this file and in dtexinput.cpp/dtexinput.h
struct SqDtexFileHeader
{
	/** \brief Construct an SqDtexFileHeader structure
	* \param fileSize - file size of the dtex file in bytes
	* \param imageWidth - image width; the width in pixels of the dtex image
	* \param imageHeight - image height; the hieght in pixels of the dtex image
	* \param numberChannels - number of channels; for example 'rgb' has 3 channels, but 'r' has only 1 channel	
	* \param dataSize - data size; the size of only the data part of the dtex file
	* \param tileWidth -
	 */ 
	SqDtexFileHeader( const uint32 fileSize = 0, const uint32 imageWidth = 0, 
			const uint32 imageHeight = 0, const uint32 numberOfChannels = 0, const uint32 dataSize = 0, 
			const uint32 tileWidth = 0, const uint32 tileHeight = 0, const uint32 numberOfTiles = 0,
			const float matWorldToScreen[4][4] = NULL, const float matWorldToCamera[4][4] = NULL);
	
	/** \brief Write the dtex file header to open binary file.
	 *
	 * \param file  - An open binary file with output stream already set
	 *  to the correct position (beginnig of file) to write the header. 
	 */	
	void writeToFile( std::ofstream& file ) const;
	
	/** \brief Seek to the appropriate position in m_dtexFile and write the dtex file header.
	 *
	 * \param file  - An open binary file with input stream already set
	 *  to the correct position (beginnig of file) to write the header. 
	 */	
	void readFromFile( std::ifstream& file );
	
	/** \brief Copy the tranformation matrices to the local member data.
	 * 
	 */	
	void setTransformationMatrices(const float imatWorldToScreen[4][4], const float imatWorldToCamera[4][4]);
	
	/// The magic number field contains the following bytes: Ò\0x89AqD\0x0b\0x0a\0x16\0x0a"
	// The first byte has the high-bit set to detect transmission over a 7-bit communications channel.
	// This is highly unlikely, but it can't hurt to check. 
	// The next three bytes are the ASCII string "AqD" (short for "Aqsis DTEX").
	// The next two bytes are carriage return and line feed, which results in a "return" sequence on all major platforms (Windows, Unix and Macintosh).
	// This is followed by a DOS end of file (control-Z) and another line feed.
	// This sequence ensures that if the file is "typed" on a DOS shell or Windows command shell, the user will see "AqD" 
	// on a single line, preceded by a strange character.
	// Magic number for a DTEX file is: "\0x89AqD\0x0b\0x0a\0x16\0x0a" Note 0x417144 represents ASCII AqD
	//char magicNumber[8]; 
	/// Size of this file in bytes
	uint32 fileSize;
	// Number of horizontal pixels in the image
	uint32 imageWidth;
	// Number of vertical pixels
	uint32 imageHeight;
	/// Number if channels in a visibility node (1 for grayscale, 3 for full color)
	uint32 numberOfChannels;
	// Size of the deep data by itself, in bytes
	uint32 dataSize;
	// Width, in pixels, of a tile (unpadded, so edge tiles may be larger, but never smaller)
	uint32 tileWidth;
	// Height, in pixels, of a tile (unpadded)
	uint32 tileHeight;
	// Number of tiles in the image
	uint32 numberOfTiles;
	// World to Screen transformation matrix
	float matWorldToScreen[4][4];
	// World to Camera transformation matrix
	float matWorldToCamera[4][4];
};

//------------------------------------------------------------------------------
} // namespace Aqsis

#endif // SHARED_H_INCLUDED
