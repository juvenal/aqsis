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
 * \brief Implements the deep shadow display devices for Aqsis.
 * This driver stores deep shadow map image data in a DTEX tiled image file. 
*/

#include <string.h>
#include <stdio.h>
#include <stdlib.h>

#include "d_dsm.h"
#include "ndspy.h"  // NOTE: Use Pixar's ndspy.h if you've got it.

typedef struct tagDSMFILEHEADER {	  
    short   fType; 
    int     fSize; 
    short   fReserved1; 
    short   fReserved2; 
    int     fOffBits;
/*    
 * From Pseudonym's Aqsis file spec wiki page:
    bits		Value			Comment
0-39		"DSMFF"			String
40-48		Uint8			Major_Version
49-57		Uint8			Minor_Version
58-66		Uint8			Patch_Version
67-68		2-bit			Color (00=Monochrome, 11=Color, 10,01=reserved)
69-85		Uint16			X_Resolution
86-102		Uint16			Y_Resulution
103		Boolean			Access_Mode(0=scanline, 1=tiled)
104-112		Uint8			Tile_Size
113-255		Zeros-padded		Reserved for future use
256-		Variable-length chunk	data (Pixel Chunks)
We should store pointers to the beginning of each tile.
Each tile should in turn have pointers to the beginning of each pixel because pixel sizes are variable.
If Tile_Size is 64, for 64x64 pixel tiles, you can index to the desired tile using X_res and Y_res.
Hopefully all resolutions are power of 2.
-??		1111000011110000	End_flag
*/ 
} DSMFILEHEADER; 

typedef struct tagDSMINFOHEADER{ // bmih 
    int     iSize; 
    long    iWidth; 
    long    iHeight; 
    short   iPlanes; 
    int     iCompression; 
    int     iSizeImage; 
    long    iXPelsPerMeter; 
    long    iYPelsPerMeter; 
    int     iClrUsed; 
    int     iClrImportant; 
} DSMINFOHEADER; 

typedef struct tagDSMINFO {
    DSMINFOHEADER    	dsmiHeader;
} DSMINFO;

typedef struct
{
	FILE			*fp;
	DSMFILEHEADER	dsmfh;
	char            *FileName;
	DSMINFO			dsmi;
	char            *ImageData;
	int             Channels;
	long            TotalPixels;
}
DeepShadowData;

// -----------------------------------------------------------------------------
// Constants
// -----------------------------------------------------------------------------
static const int     DEFAULT_IMAGEWIDTH         = 512;
static const int     DEFAULT_IMAGEHEIGHT        = 512;
static const float   DEFAULT_PIXELASPECTRATIO   = 1.0f;


//******************************************************************************
// DspyImageOpen
//
// Initializes the display driver, allocates necessary resources, checks image
// size, specifies format in which incoming data will arrive.
//******************************************************************************
extern "C" PtDspyError DspyImageOpen(PtDspyImageHandle    *image,
                          const char           *drivername,
                          const char           *filename,
                          int                  width,
                          int                  height,
                          int                  paramCount,
                          const UserParameter  *parameters,
                          int                  formatCount,
                          PtDspyDevFormat      *format,
                          PtFlagStuff          *flagstuff)
{
#if SHOW_CALLSTACK
	fprintf(stderr, "d_dsm_DspyImageOpen called.\n");
#endif
	
	PtDspyError rval = PkDspyErrorNone;		

	// This is all bookkeeping information
	static DeepShadowData g_Data;
	DeepShadowData *pData;
	pData = (DeepShadowData *) calloc(1, sizeof(g_Data));
	*image = pData; // This is how the caller gets a handle on this new image
	

	// Initialize our global resources
	memset(&g_Data, sizeof(DeepShadowData), 0);

	flagstuff->flags = PkDspyFlagsWantsScanLineOrder;

	if ( width <= 0 )
		width = DEFAULT_IMAGEWIDTH;
	if ( height <= 0 )
		height = DEFAULT_IMAGEHEIGHT;

	
	g_Data.FileName = strdup(filename);
	g_Data.Channels = formatCount;

	g_Data.dsmi.dsmiHeader.iSize        = sizeof(DSMINFOHEADER);
	g_Data.dsmi.dsmiHeader.iWidth       = width;
	g_Data.dsmi.dsmiHeader.iHeight      = height;
	g_Data.dsmi.dsmiHeader.iPlanes      = 1;
	g_Data.dsmi.dsmiHeader.iCompression = 0;

	g_Data.TotalPixels     = width*height;
	
	// Prepare the file header
	g_Data.dsmfh.fType     = 0x4D42;    // ASCII "BM"
	g_Data.dsmfh.fSize     = sizeof(DSMFILEHEADER) +
	                         sizeof(DSMINFOHEADER);
	g_Data.dsmfh.fOffBits  = sizeof(DSMFILEHEADER) + sizeof(DSMINFOHEADER);
	
	// We would pre-allocate the memory for the deep shadow map, but there
	// is no way of knowing ahead of time how big it will be
		
	// More stuff to do here, but probably not worth writing out until we want to store the real
	// DSM to file. But for now we want to store it as a sequence of bitmaps.	
	
// Note from the formatCount field we can decide if this will be a monochrome or color depth map,
// and choose the storage type accordingly

	return rval;
}

//******************************************************************************
// DspyImageQuery
//
// Query the display driver for image size (if not specified in the open call)
// and aspect ratio.
//******************************************************************************
extern "C" PtDspyError DspyImageQuery(PtDspyImageHandle image,
                           PtDspyQueryType   type,
                           size_t            size,
                           void              *data)
{
#if SHOW_CALLSTACK
	fprintf(stderr, "d_dsm_DspyImageQuery called, type: %d.\n", type);
#endif

	PtDspyError ret = PkDspyErrorNone; // No-error signal

	return ret;
}

//******************************************************************************
// DspyImageDeepData
//
// Send data to the display driver.
// This function is called once per bucket. It should add the bucket's data to the image.
// This does not write the image to disk. That is done at the end by WriteDTEX
//******************************************************************************
extern "C" PtDspyError DspyImageDeepData(PtDspyImageHandle image,
                          int xmin,
                          int xmax_plusone,
                          int ymin,
                          int ymax_plusone,
                          int entrysize,
                          const unsigned char *data)
{

	return PkDspyErrorNone;
}

// Write a DspyImageDeepData() function with an additional parameter: an array of integers specifying
// the length of each visibility function, so we can break up the data correctly. This will replace DspyImageData 
// for the dsm display device

//******************************************************************************
// DspyImageClose
//******************************************************************************
extern "C" PtDspyError DspyImageClose(PtDspyImageHandle image)
{
#if SHOW_CALLSTACK
	fprintf(stderr, "d_dsm_DspyImageClose called.\n");
#endif

// This is where we will write to disk the DSM
// When doing testing, write out a sequence of bitmaps for visualizing the deep data

/*
   AppData *pData = (AppData *)image;

	if ( pData->fp )
		fclose(pData->fp);
	pData->fp = NULL;

	if ( pData->FileName )
		free(pData->FileName);
	pData->FileName = NULL;

	if ( pData->ImageData )
		free(pData->ImageData);
	pData->ImageData = NULL;

   free(pData);
*/
	return PkDspyErrorNone;
}
