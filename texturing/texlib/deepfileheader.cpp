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
 * \brief Implementation of SqDtexFileHeader constructor and functions.
*/

#include "deepfileheader.h"
#include "exception.h"

// Magic number for a DTEX file is: "\0x89AqD\0x0b\0x0a\0x16\0x0a" Note 0x417144 represents ASCII AqD
const char dtexMagicNumber[8] = { 0x89, 'A', 'q', 'D', 0x0b, 0x0a, 0x16, 0x0a };

namespace Aqsis 
{

SqDtexFileHeader::SqDtexFileHeader( const uint32 fileSize, const uint32 imageWidth, 
		const uint32 imageHeight, const uint32 numberOfChannels, const uint32 dataSize, 
		const uint32 tileWidth, const uint32 tileHeight, const uint32 numberOfTiles,
		const float matWorldToScreen[4][4], const float matWorldToCamera[4][4]) :
	fileSize( fileSize ),
	imageWidth( imageWidth ),
	imageHeight( imageHeight ),
	numberOfChannels( numberOfChannels ),
	dataSize( dataSize ),
	tileWidth( tileWidth ),
	tileHeight( tileHeight ),
	numberOfTiles( numberOfTiles ),
	matWorldToScreen(),
	matWorldToCamera()
{
	setTransformationMatrices( matWorldToScreen, matWorldToCamera );
}

void SqDtexFileHeader::writeToFile( std::ofstream& file ) const
{
	file.write(dtexMagicNumber, 8);
	file.write((const char*)(&fileSize), sizeof(uint32));
	file.write((const char*)(&imageWidth), sizeof(uint32));
	file.write((const char*)(&imageHeight), sizeof(uint32));
	file.write((const char*)(&numberOfChannels), sizeof(uint32));
	file.write((const char*)(&dataSize), sizeof(uint32));
	file.write((const char*)(&tileWidth), sizeof(uint32));
	file.write((const char*)(&tileHeight), sizeof(uint32));
	file.write((const char*)(&numberOfTiles), sizeof(uint32));
	file.write((const char*)(&matWorldToScreen), 16*sizeof(float));
	file.write((const char*)(&matWorldToCamera), 16*sizeof(float));
}

void SqDtexFileHeader::readFromFile( std::ifstream& file )
{
	//file.read(magicNumber, 8);
	file.read((char*)(&fileSize), sizeof(uint32));
	file.read((char*)(&imageWidth), sizeof(uint32));
	file.read((char*)(&imageHeight), sizeof(uint32));
	file.read((char*)(&numberOfChannels), sizeof(uint32));
	file.read((char*)(&dataSize), sizeof(uint32));
	file.read((char*)(&tileWidth), sizeof(uint32));
	file.read((char*)(&tileHeight), sizeof(uint32));
	file.read((char*)(&numberOfTiles), sizeof(uint32));
	file.read((char*)(&matWorldToScreen), 16*sizeof(float));
	file.read((char*)(&matWorldToCamera), 16*sizeof(float));
}

void SqDtexFileHeader::setTransformationMatrices(const float imatWorldToScreen[4][4], 
												const float imatWorldToCamera[4][4])
{
	int x, y;
	for (x = 0; x < 4; ++x)
	{
		for (y = 0; y < 4; ++y)
		{
			matWorldToScreen[x][y] = matWorldToScreen[x][y];
			matWorldToCamera[x][y] = matWorldToCamera[x][y];
		}
	}
}

//------------------------------------------------------------------------------
} // namespace Aqsis
