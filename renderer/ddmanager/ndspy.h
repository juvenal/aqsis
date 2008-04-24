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
 * \brief Renderman display driver interface.
 *
 * This file defines prototypes, typedefs, macros etc for the "standard"
 * renderman display driver interface.
 *
 * \author Paul C. Gregory (pgregory@aqsis.org)
 */

#ifndef	___ndspy_Loaded___
#define	___ndspy_Loaded___


#ifdef	WIN32
#define _qExport __declspec(dllexport)
#define _qImport __declspec(dllimport)
#else	// !WIN32
#define _qExport
#define _qImport
#endif // WIN32

#include <stdio.h>
#include <stdlib.h>

typedef	char*	RtToken;
typedef	void*	RtPointer;


typedef double PtDspyFloat64;
typedef float PtDspyFloat32;

#if defined(__mips)
#define PkDspyByteOrderNative	PkDspyByteOrderHiLo
#elif defined(__mips64)
#define PkDspyByteOrderNative	PkDspyByteOrderHiLo
#elif defined(__sparc)
#define PkDspyByteOrderNative	PkDspyByteOrderHiLo
#elif defined(__alpha)
#define PkDspyByteOrderNative	PkDspyByteOrderLoHi
#else
#define PkDspyByteOrderNative	PkDspyByteOrderLoHi
#endif

/// \todo Consider using TqUint32 etc from aqsis_types.h instead of the below.
#if defined(__mips64) || defined(__alpha) || defined(__x86_64) || defined(_M_X64)
typedef unsigned int PtDspyUnsigned32;
typedef int PtDspySigned32;
#else
typedef unsigned long PtDspyUnsigned32;
typedef long PtDspySigned32;
#endif

typedef unsigned short PtDspyUnsigned16;
typedef short PtDspySigned16;

typedef unsigned char PtDspyUnsigned8;
typedef char PtDspySigned8;

typedef PtDspyUnsigned32 PtDspyMsgLen;
typedef PtDspyUnsigned32 PtDspyServerMessage;


// Format types
#define PkDspyNone			0
#define PkDspyFloat32		1
#define PkDspyUnsigned32	2
#define PkDspySigned32		3
#define PkDspyUnsigned16	4
#define PkDspySigned16		5
#define PkDspyUnsigned8		6
#define PkDspySigned8		7
#define PkDspyString		8
#define PkDspyMatrix		9
#define PkDspyArrayBegin	10
#define PkDspyArrayEnd		11

#define PkDspyMaskType		8191

#define PkDspyMaskOrder (PkDspyByteOrderHiLo | PkDspyByteOrderLoHi)
#define PkDspyShiftOrder	13
#define PkDspyByteOrderHiLo	8192
#define PkDspyByteOrderLoHi	16384


typedef struct
{
	char *name;
	unsigned type;
}
PtDspyDevFormat;


typedef struct
{
	PtDspyUnsigned32 width;
	PtDspyUnsigned32 height;
	PtDspyFloat32 aspectRatio;
}
PtDspySizeInfo;


typedef struct
{
	PtDspyUnsigned8 overwrite;
	PtDspyUnsigned8 interactive;
}
PtDspyOverwriteInfo;


typedef enum
{
    PkSizeQuery,
    PkOverwriteQuery,
} PtDspyQueryType;


typedef enum
{
    PkDspyErrorNone = 0,
    PkDspyErrorNoMemory,
    PkDspyErrorUnsupported,
    PkDspyErrorBadParams,
    PkDspyErrorNoResource,
    PkDspyErrorUndefined
} PtDspyError;

#define PkDspyFlagsWantsScanLineOrder 1
#define PkDspyFlagsWantsEmptyBuckets 2
#define PkDspyFlagsWantsNullEmptyBuckets 4
typedef struct
{
	int flags;
}
PtFlagStuff;


typedef void *PtDspyImageHandle;

typedef void * PtDspyChannel;
typedef void * PtDspyOutput;

typedef struct uparam
{
	RtToken		name;
	char		vtype, vcount;
	RtPointer	value;
	int		nbytes;
}
UserParameter;

typedef PtDspyError (*DspyImageOpenMethod)(PtDspyImageHandle*,const char*,const char*,int,int,int,const UserParameter*,int,PtDspyDevFormat*,PtFlagStuff*);
typedef PtDspyError (*DspyImageQueryMethod)(PtDspyImageHandle,PtDspyQueryType,size_t,void*);
typedef PtDspyError (*DspyImageDataMethod)(PtDspyImageHandle,int,int,int,int,int,const unsigned char*);
typedef PtDspyError (*DspyImageCloseMethod)(PtDspyImageHandle);
typedef PtDspyError (*DspyImageDelayCloseMethod)(PtDspyImageHandle);

// Only define these functions if we are being used in a display
#ifndef	DSPY_INTERNAL

#ifdef __cplusplus
extern "C"
{
#endif

	void DspyPrintFormat(FILE *fp,PtDspyDevFormat *format,int formatCount);

	PtDspyDevFormat *DspyCopyDevFormat(PtDspyDevFormat *f,int fc);


	_qExport PtDspyError DspyImageOpen(PtDspyImageHandle * image,
	                                   const char *drivername,
	                                   const char *filename,
	                                   int width,
	                                   int height,
	                                   int paramCount,
	                                   const UserParameter *parameters,
	                                   int iFormatCount,
	                                   PtDspyDevFormat *format,
	                                   PtFlagStuff *flagstuff);

	_qExport PtDspyError DspyImageData(PtDspyImageHandle image,
	                                   int xmin,
	                                   int xmaxplus1,
	                                   int ymin,
	                                   int ymaxplus1,
	                                   int entrysize,
	                                   const unsigned char *data);

	_qExport PtDspyError DspyImageClose(PtDspyImageHandle);

	_qExport PtDspyError DspyImageDelayClose(PtDspyImageHandle);

	_qExport PtDspyError DspyImageQuery(PtDspyImageHandle image,
	                                    PtDspyQueryType type,
	                                    size_t size,
	                                    void *data);

#ifdef __cplusplus
}
#endif

#endif	//	DSPY_INTERNAL

#endif // ___ndspy_Loaded___





