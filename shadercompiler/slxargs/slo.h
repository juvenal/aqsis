/* Aqsis
 / Copyright (C) 1997 - 2001, Paul C. Gregory
 /
 / Contact: pgregory@aqsis.org
 /
 / This library is free software; you can redistribute it and/or
 / modify it under the terms of the GNU General Public
 / License as published by the Free Software Foundation; either
 / version 2 of the License, or (at your option) any later version.
 /
 / This library is distributed in the hope that it will be useful,
 / but WITHOUT ANY WARRANTY; without even the implied warranty of
 / MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 / General Public License for more details.
 /
 / You should have received a copy of the GNU General Public
 / License along with this library; if not, write to the Free Software
 / Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
*/

/** \file
 * \brief Compatibility interface to Pixar's sloarg shader argument library.
 * \author Matthias Baas
 *
 * ===================================================================
 * C-compatible header. C++ constructs must be preprocessor-protected.
 * ===================================================================
 */

#ifndef SLO_H_INCLUDED
#define SLO_H_INCLUDED

#include "slx.h"

#ifdef __cplusplus
extern "C"
{
#endif

	typedef enum {
	    SLO_TYPE_UNKNOWN,
	    SLO_TYPE_POINT,
	    SLO_TYPE_COLOR,
	    SLO_TYPE_SCALAR,
	    SLO_TYPE_STRING,
	    SLO_TYPE_SURFACE,
	    SLO_TYPE_LIGHT,
	    SLO_TYPE_DISPLACEMENT,
	    SLO_TYPE_VOLUME,
	    SLO_TYPE_TRANSFORMATION,
	    SLO_TYPE_IMAGER,
	    SLO_TYPE_VECTOR,
	    SLO_TYPE_NORMAL,
	    SLO_TYPE_MATRIX
	} SLO_TYPE;

	typedef enum {
	    SLO_STOR_UNKNOWN,
	    SLO_STOR_CONSTANT,
	    SLO_STOR_VARIABLE,
	    SLO_STOR_TEMPORARY,
	    SLO_STOR_PARAMETER,
	    SLO_STOR_GSTATE
	} SLO_STORAGE;

	typedef enum {
	    SLO_DETAIL_UNKNOWN,
	    SLO_DETAIL_VARYING,
	    SLO_DETAIL_UNIFORM
	} SLO_DETAIL;

	typedef struct
	{
		float xval;
		float yval;
		float zval;
	}
	SLO_POINT;

	typedef struct
	{
		float val[4][4];
	}
	SLO_MATRIX;

	typedef float SLO_SCALAR;

	typedef struct SLOvissymdef
	{
		char *svd_name;
		SLO_TYPE svd_type;
		SLO_STORAGE svd_storage;
		SLO_DETAIL svd_detail;
		char *	svd_spacename;
		int	svd_arraylen;
		union {
			SLO_POINT	*pointval;
			SLO_SCALAR	*scalarval;
			SLO_MATRIX	*matrixval;
			char	*stringval;
		} svd_default;
	}
	SLO_VISSYMDEF;

	SLXARGS_SHARE extern void Slo_SetPath ( char * path );
	SLXARGS_SHARE extern int Slo_SetShader ( char * name );
	SLXARGS_SHARE extern char *Slo_GetName ( void );
	SLXARGS_SHARE extern SLO_TYPE Slo_GetType ( void );
	SLXARGS_SHARE extern int Slo_GetNArgs ( void );
	SLXARGS_SHARE extern SLO_VISSYMDEF *Slo_GetArgById ( int id );
	SLXARGS_SHARE extern SLO_VISSYMDEF *Slo_GetArgByName ( char * name );
	SLXARGS_SHARE extern SLO_VISSYMDEF *Slo_GetArrayArgElement( SLO_VISSYMDEF * array, int index );
	SLXARGS_SHARE extern void Slo_EndShader ( void );
	SLXARGS_SHARE extern char *Slo_TypetoStr ( SLO_TYPE type );
	SLXARGS_SHARE extern char *Slo_StortoStr ( SLO_STORAGE storage );
	SLXARGS_SHARE extern char *Slo_DetailtoStr ( SLO_DETAIL detail );

#ifdef __cplusplus
}
#endif

#endif /* SLO_H_INCLUDED */
