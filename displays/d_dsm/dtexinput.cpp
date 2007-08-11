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
 * \brief Implementation of a deep texture file reader to load deep data tile from file into memory.
 */

#include "dtexinput.h"
#include "exception.h"

CqDeepTexInputFile::CqDeepTexOutputFile(std::string filename)
	: m_dtexFile( filename.c_str(), std::ios::in | std::ios::binary ),
	m_fileHeader( (uint32)0, (uint32)imageWidth, (uint32)imageHeight, (uint32)numberOfChannels, (uint32)bytesPerChannel, (uint32)0, (uint32)0, (uint32)tileWidth, (uint32)tileHeight, (uint32)0 )
{

}

void CqDeepTexInputFile::ReadTile( const int x, const int y, SqDeepDataTile& tile )
{
	
}
