// Aqsis
// Copyright � 1997 - 2001, Paul C. Gregory
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
 * \brief Implements the deep shadow display devices for Aqsis.
 * This driver stores deep shadow map image data in a DTEX tiled image file.
 * 
 * \author Zachary L. Carter (zcarter@aqsis.org)
*/

#include <string.h>
#include <stdio.h>
#include <stdlib.h>


#include "ndspy.h"  // NOTE: Use Pixar's ndspy.h if you've got it.

typedef struct tagDSMFILEHEADER {	  
    short   bfType; 
    int     bfSize; 
    short   bfReserved1; 
    short   bfReserved2; 
    int     bfOffBits;
/*    
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

//******************************************************************************
// DspyImageOpen
//
// Initializes the display driver, allocates necessary resources, checks image
// size, specifies format in which incoming data will arrive.
//******************************************************************************

PtDspyError DspyImageOpen(PtDspyImageHandle    *image,
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
	PtDspyError rval = PkDspyErrorNone;

   static AppData g_Data;
   AppData *pData;

#if SHOW_CALLSTACK

	fprintf(stderr, "sdcBMP_DspyImageOpen called.\n");
#endif

   pData = (AppData *) calloc(1, sizeof(g_Data));
   *image = pData;

   // Initialize our global resources

	memset(&g_Data, sizeof(AppData), 0);

	flagstuff->flags = PkDspyFlagsWantsScanLineOrder;

	if ( width <= 0 )
		width = DEFAULT_IMAGEWIDTH;

	if ( height <= 0 )
		height = DEFAULT_IMAGEHEIGHT;

	
	g_Data.FileName = strdup(filename);
	g_Data.Channels = formatCount;

	g_Data.PixelBytes = 3; // One byte for red, one for green, and one for blue.

	g_Data.bmi.bmiHeader.biSize        = sizeof(BITMAPINFOHEADER);
	g_Data.bmi.bmiHeader.biWidth       = width;
	g_Data.bmi.bmiHeader.biHeight      = height;
	g_Data.bmi.bmiHeader.biPlanes      = 1;
	g_Data.bmi.bmiHeader.biBitCount    = 24;
	g_Data.bmi.bmiHeader.biCompression = BI_RGB;

	g_Data.RowSize                     = DWORDALIGN(width * g_Data.bmi.bmiHeader.biBitCount);
	g_Data.bmi.bmiHeader.biSizeImage   = g_Data.RowSize * height;

	g_Data.TotalPixels     = width * height;


	// Prepare the file header

	g_Data.bfh.bfType     = 0x4D42;    // ASCII "BM"
	g_Data.bfh.bfSize     = BITMAPFILEHEADER_SIZEOF +
	                        sizeof(BITMAPINFOHEADER) +
	                        g_Data.bmi.bmiHeader.biSizeImage;
	g_Data.bfh.bfOffBits  = BITMAPFILEHEADER_SIZEOF + sizeof(BITMAPINFOHEADER);


	// Create a buffer for the image data (calloc clears all pixels to black)

	g_Data.ImageData = (char *)calloc(1, g_Data.RowSize);

	if ( ! g_Data.ImageData )
	{
		fprintf(stderr, "sdcBMP_DspyImageOpen_sdcBMP: Insufficient Memory\n");
		rval = PkDspyErrorNoResource;
	}


	// Open the file and get ready to write.

	g_Data.fp = fopen(g_Data.FileName, "wb");

	if ( ! g_Data.fp )
	{
		fprintf(stderr, "sdcBMP_DspyImageOpen: Unable to open [%s]\n", g_Data.FileName);
		rval = PkDspyErrorNoResource;
		goto Exit;
	}

	// Write out the BITMAPFILEHEADER
	if (lowendian())
	{
    		g_Data.bfh.bfType = swap2(g_Data.bfh.bfType);
    		g_Data.bfh.bfSize = swap4(g_Data.bfh.bfSize);
    		g_Data.bfh.bfOffBits = swap4(g_Data.bfh.bfOffBits);
	}
	if ( ! bitmapfileheader(&g_Data.bfh, g_Data.fp) )
      
	{
		fprintf(stderr, "sdcBMP_SaveBitmap: Error writing to [%s]\n", g_Data.FileName);
		goto Exit;
	}

	if (lowendian())
	{
    		g_Data.bfh.bfType = swap2(g_Data.bfh.bfType);
    		g_Data.bfh.bfSize = swap4(g_Data.bfh.bfSize);
    		g_Data.bfh.bfOffBits = swap4(g_Data.bfh.bfOffBits);
	}

	if (lowendian())
	{
   		g_Data.bmi.bmiHeader.biSize = swap4(g_Data.bmi.bmiHeader.biSize);
		   g_Data.bmi.bmiHeader.biWidth= swap4(g_Data.bmi.bmiHeader.biWidth);
   		g_Data.bmi.bmiHeader.biHeight= swap4(g_Data.bmi.bmiHeader.biHeight);
   		g_Data.bmi.bmiHeader.biPlanes = swap2(g_Data.bmi.bmiHeader.biPlanes);
   		g_Data.bmi.bmiHeader.biBitCount = swap2(g_Data.bmi.bmiHeader.biBitCount);
   		g_Data.bmi.bmiHeader.biCompression= swap4(g_Data.bmi.bmiHeader.biCompression); 
   		g_Data.bmi.bmiHeader.biSizeImage= swap4(g_Data.bmi.bmiHeader.biSizeImage);
   		g_Data.bmi.bmiHeader.biXPelsPerMeter= swap4(g_Data.bmi.bmiHeader.biXPelsPerMeter); 
   		g_Data.bmi.bmiHeader.biYPelsPerMeter= swap4(g_Data.bmi.bmiHeader.biYPelsPerMeter); 
   		g_Data.bmi.bmiHeader.biClrUsed= swap4(g_Data.bmi.bmiHeader.biClrUsed);
   		g_Data.bmi.bmiHeader.biClrImportant= swap4(g_Data.bmi.bmiHeader.biClrImportant);
	}
	// Write out the BITMAPINFOHEADER

	if ( ! fwrite(&g_Data.bmi.bmiHeader,
	              sizeof(BITMAPINFOHEADER),
	              1,
	              g_Data.fp) )
	{
		fprintf(stderr, "sdcBMP_SaveBitmap: Error writing to [%s]\n", g_Data.FileName);
		rval = PkDspyErrorNoResource;
		goto Exit;
	}

	if (lowendian())
	{
   		g_Data.bmi.bmiHeader.biSize = swap4(g_Data.bmi.bmiHeader.biSize);
		   g_Data.bmi.bmiHeader.biWidth= swap4(g_Data.bmi.bmiHeader.biWidth);
   		g_Data.bmi.bmiHeader.biHeight= swap4(g_Data.bmi.bmiHeader.biHeight);
   		g_Data.bmi.bmiHeader.biPlanes = swap2(g_Data.bmi.bmiHeader.biPlanes);
   		g_Data.bmi.bmiHeader.biBitCount = swap2(g_Data.bmi.bmiHeader.biBitCount);
   		g_Data.bmi.bmiHeader.biCompression= swap4(g_Data.bmi.bmiHeader.biCompression); 
   		g_Data.bmi.bmiHeader.biSizeImage= swap4(g_Data.bmi.bmiHeader.biSizeImage);
   		g_Data.bmi.bmiHeader.biXPelsPerMeter= swap4(g_Data.bmi.bmiHeader.biXPelsPerMeter); 
   		g_Data.bmi.bmiHeader.biYPelsPerMeter= swap4(g_Data.bmi.bmiHeader.biYPelsPerMeter); 
   		g_Data.bmi.bmiHeader.biClrUsed= swap4(g_Data.bmi.bmiHeader.biClrUsed);
   		g_Data.bmi.bmiHeader.biClrImportant= swap4(g_Data.bmi.bmiHeader.biClrImportant);
	}

   
   memcpy((void*) pData, (void *) &g_Data, sizeof(AppData));
   
Exit:

	if ( rval != PkDspyErrorNone )
	{
		if ( g_Data.fp )
			fclose(g_Data.fp);
		g_Data.fp = NULL;
	}

	return rval;
}

//******************************************************************************
// DspyImageDeepData
//
// Send data to the display driver.
// This function is called once per bucket. It should add the bucket's data to the image.
// This does not write the image to disk. That is done at the end by WriteDTEX
//******************************************************************************
PtDspyError DspyImageDeepData(PtDspyImageHandle image,
                          int xmin,
                          int xmax_plusone,
                          int ymin,
                          int ymax_plusone,
                          int entrysize,
                          const struct DeepPixel* data)
{
	int  x;
	int  r, g, b;
     
	char *to;
	long spot;

   	AppData *pData = (AppData *)image;
   	r = g = b = 0;

#if SHOW_CALLSTACK

	fprintf(stderr, "d_dsm_DspyImageDeepData called.\n");
#endif

	if ( (ymin+1) != ymax_plusone )
	{
		fprintf(stderr, "sdcBMP_DspyImageData: Image data not in scanline format\n");
		return PkDspyErrorBadParams;
	}

	spot  = pData->bfh.bfOffBits +
	        (pData->RowSize * (pData->bmi.bmiHeader.biHeight - ymin - 1));
	spot += pData->PixelBytes * xmin;

	if ( fseek(pData->fp, spot, SEEK_SET) != 0 )
	{
		fprintf(stderr, "sdcBMP_DspyImageData: Seek failure\n");
		return PkDspyErrorUndefined;
	}


	to = pData->ImageData;

	for (x = xmin; x < xmax_plusone; x++)
	{
		// Extract the r, g, and b values from data

		if ( ! data )
			r = g = b = 0;
		else
			if ( pData->Channels == 1 )
				r = g = b = data[0];
			else
				if ( pData->Channels >= 3 )
				{
					r = data[pData->Channels - 1];
					g = data[pData->Channels - 2];
					b = data[pData->Channels - 3];
				}


		if ( data )
			data += entrysize;

		// Place the r, g, and b values into our bitmap

		*to++ = r;
		*to++ = g;
		*to++ = b;
	}

	if ( ! fwrite(pData->ImageData, int(to - pData->ImageData), 1, pData->fp) )
	{
		fprintf(stderr, "sdcBMP_DspyImageData: Error writing file\n");
		return PkDspyErrorUndefined;
	}

	return PkDspyErrorNone;
}