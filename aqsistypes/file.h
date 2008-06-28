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
		\brief Declares the CqFile class for handling files with RenderMan searchpath option support.
		\author Paul C. Gregory (pgregory@aqsis.org)
*/

//? Is .h included already?
#ifndef FILE_H_INCLUDED
#define FILE_H_INCLUDED 1

#include	<iostream>
#include	<list>
#include	<vector>

#include	"aqsis.h"

#include	"sstring.h"

namespace Aqsis {

// This should'nt really be in here, but for now it will do
#ifdef AQSIS_SYSTEM_WIN32
#define DIRSEP "\\"
#else
#define DIRSEP "/"
#endif

//-----------------------------------------------------------------------
/** \brief Find an absolute path to the given file name in the provided search path.
 *
 * This function should deprecate a common usage of CqFile simply to get the
 * absolute file name in the search path.
 *
 * \param fileName - path-relative name of the file.
 * \param searchPath - colon-separated string of directories in which to search
 *                     for the given file name.
 * \return Absolute file name; throws an XqInvalidFile if the file is not found.
 */
COMMON_SHARE std::string findFileInPath(const std::string& fileName,
		const std::string& searchPath);


//----------------------------------------------------------------------
/** \class CqFile
 *  \brief Standard handling of all file types utilising the searchpath options.
 */
class COMMON_SHARE CqFile
{
	public:
		/** Default constructor
		 */
		CqFile() : m_pStream( 0 ), m_bInternal( false )
		{}
		/** Constructor taking an open stream pointer and a name.
		 * \param Stream a pointer to an already opened input stream to attach this object to.
		 * \param strRealName the name of the file associated with this stream.
		 */
		CqFile( std::istream* Stream, const char* strRealName ) :
				m_pStream( Stream ), m_strRealName( strRealName ), m_bInternal( false )
		{}
		CqFile( const char* strFilename, const char* strSearchPathOption = "" );
		/** Dectructor. Takes care of closing the stream if the constructor opened it.
		 */
		virtual	~CqFile()
		{
			if ( m_pStream != NULL && m_bInternal )
				delete( m_pStream );
		}

		void	Open( const char* strFilename, const char* strSearchPathOption = "", std::ios::openmode mode = std::ios::in );
		/** Close any opened stream associated with this object.
		 */
		void	Close()
		{
			if ( m_pStream != NULL )
				delete( m_pStream );
			m_pStream = NULL;
		}
		/** Find out if the stream associated with this object is valid.
		 * \return boolean indicating validity.
		 */
		bool	IsValid() const
		{
			return ( m_pStream != NULL );
		}
		/** Get the name asociated with this file object.
		 * \return a read only reference to the string object.
		 */
		const CqString&	strRealName() const
		{
			return ( m_strRealName );
		}

		/** Cast to a stream reference.
		 */
		operator std::istream&()
		{
			return ( *m_pStream );
		}
		/** Cast to a stream pointer.
		 */
		operator std::istream*()
		{
			return ( m_pStream );
		}

		/** Get the current position within the stream if appropriate.
		 * \return long integer indicating the offest from the start.
		 */
		TqLong	Position()
		{
			return ( m_pStream->tellg() );
		}
		/** Get the length of the stream if a file.
		 * \return the lenght as a long integer.
		 */
		TqLong	Length()
		{
			/// \todo Should check if it is a file here.
			long pos = Position();
			m_pStream->seekg( 0, std::ios::end );
			long len = Position();
			m_pStream->seekg( pos, std::ios::beg );
			return ( len );
		}

		static CqString FixupPath(CqString& strPath);
		static std::list<CqString*> Glob( const CqString& strFileGlob );
		static std::list<CqString*> cliGlob( const CqString& strFileGlob );
		static std::string basePath( const CqString& strFilespec );
		static std::string fileName( const CqString& strFilespec );
		static std::string extension( const CqString& strFilespec );
		static std::string baseName( const CqString& strFilespec );
		static std::string pathSep();
		static std::vector<std::string> searchPaths( const CqString& searchPath );

	private:
		std::istream*	m_pStream;		///< a poimter to the stream associated with this file object.
		CqString	m_strRealName;	///< the name of this file object, usually the filename.
		bool	m_bInternal;	///< a flag indicating whether the stream originated internally, or was externally created and passed in.
}
;


//-----------------------------------------------------------------------

} // namespace Aqsis

#endif	// !FILE_H_INCLUDED
