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

//*****************************************************************************
// The following is taken directoy from d_sdcBMP.cpp, temporarily, so that I may write BMP image sequences to disk during the testing phase.

typedef struct tagBITMAPFILEHEADER { // bmfh 
    short   bfType; 
    int     bfSize; 
    short   bfReserved1; 
    short   bfReserved2; 
    int     bfOffBits; 
} BITMAPFILEHEADER; 

/* instead of sizeof(BITMAPFILEHEADER) since this must be 14 */
#define BITMAPFILEHEADER_SIZEOF 14 

typedef struct tagRGBQUAD { // rgbq 
    unsigned char rgbBlue; 
    unsigned char rgbGreen; 
    unsigned char rgbRed; 
    unsigned char rgbReserved; 
} RGBQUAD; 

typedef struct tagBITMAPINFOHEADER{ // bmih 
    int     biSize; 
    long    biWidth; 
    long    biHeight; 
    short   biPlanes; 
    short   biBitCount;
    int     biCompression; 
    int     biSizeImage; 
    long    biXPelsPerMeter; 
    long    biYPelsPerMeter; 
    int     biClrUsed; 
    int     biClrImportant; 
} BITMAPINFOHEADER; 

typedef struct tagBITMAPINFO {
    BITMAPINFOHEADER    bmiHeader;
    RGBQUAD             bmiColors[1];
} BITMAPINFO;

#define BI_RGB 0

// -----------------------------------------------------------------------------
// Local structures
// -----------------------------------------------------------------------------

typedef struct
{
	// Bitmap data

	FILE              *fp;
	BITMAPFILEHEADER  bfh;
	char              *FileName;
	BITMAPINFO        bmi;
	char              *ImageData;
	int               Channels;
	int               RowSize;
	int               PixelBytes;
	long              TotalPixels;
}
AppData;

// -----------------------------------------------------------------------------
// Function Prototypes
// -----------------------------------------------------------------------------
static int  DWORDALIGN(int bits);
static bool bitmapfileheader(BITMAPFILEHEADER *bfh, FILE *fp);
static unsigned short swap2( unsigned short s );
static unsigned long  swap4( unsigned long l );
static bool lowendian();


// End section from d_sdcBMP.cpp
//***********************************************************************************

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
	// NOTE: members below are for the testing phase, writing out bitmap sequences
	FILE** fileHandleArray; ///< Array of file handles
	float** testDeepData; ///< Array of pointers to the row data
	int** functionLengths; ///< Array of pointers to arrays (one per row, terminated with an ARRAY_TERMINATOR) of vis function lengths (# of nodes) of each visbility function in testDeepData
	int maxFunctionLength; ///< Longest visibility function in the DSM == number of file handles we will need
}
DeepShadowData;

// -----------------------------------------------------------------------------
// Constants
static const int     DEFAULT_IMAGEWIDTH         = 512;
static const int     DEFAULT_IMAGEHEIGHT        = 512;
static const float   DEFAULT_PIXELASPECTRATIO   = 1.0f;
static const int	 ARRAY_TERMINATOR			= -9;
// -----------------------------------------------------------------------------

// -----------------------------------------------------------------------------
// Begin Functions
// -----------------------------------------------------------------------------

//******************************************************************************
// LengthOfLongestVisFunc
//
// For testing: find and return the length of the longest visibility function in the DSM
//******************************************************************************
int LengthOfLongestVisFunc(PtDspyImageHandle image)
{
	DeepShadowData *pData = (DeepShadowData *)image;
	int longest = 0;
	int i, j;
	
	for(i = 0; i < pData->dsmi.dsmiHeader.iHeight; ++i)
	{
		j = 0;
		while (pData->functionLengths[i][j] != ARRAY_TERMINATOR)
		{
			if (pData->functionLengths[i][j] > longest)
			{
				longest = pData->functionLengths[i][j];
			}
			++j;
		}
	}
	return longest;
}

//******************************************************************************
// WriteDSMImageSequence
//
// For testing: output a sequence of image files for visualizing deep data.
//******************************************************************************
void WriteDSMImageSequence(PtDspyImageHandle image)
{
	DeepShadowData *pData = (DeepShadowData *)image;
	static AppData g_Data;
	const int imageFileCount = LengthOfLongestVisFunc(image);; ///< Determine how many files are in the sequence
	const int width = pData->dsmi.dsmiHeader.iWidth;
	const int height = pData->dsmi.dsmiHeader.iHeight;
	const int dataSizeInBytes = 3*width*height*sizeof(char);
	const int nodeSize = pData->Channels+1; // The channels field gets you 1,2, or 3 for the rgb channels, plus 1 for depth
	const int bucketWidth = 16; // \todo Somehow get the actual bucket width
	int i, j, k;
	int pnum;
	int readPos = 0;
	
	// Create a buffer the size of a bitmap
	char* imageBuffer = (char*)malloc(dataSizeInBytes);
	//memset(imageBuffer, 0, dataSizeInBytes); // Black out the image
	
	printf("There will be %d images in the sequence.\n", imageFileCount);
	printf("data size is %d.\n", dataSizeInBytes);
	
	//------------------------------------------------------------------------
	// Prepare BMP file header (same header used for all files in the sequence)
	memset(&g_Data, sizeof(AppData), 0);
	g_Data.Channels = pData->Channels;
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
	g_Data.bfh.bfType     = 0x4D42;    // ASCII "BM"
	g_Data.bfh.bfSize     = BITMAPFILEHEADER_SIZEOF +
	                        sizeof(BITMAPINFOHEADER) +
	                        g_Data.bmi.bmiHeader.biSizeImage;
	g_Data.bfh.bfOffBits  = BITMAPFILEHEADER_SIZEOF + sizeof(BITMAPINFOHEADER);		
	if (lowendian())
	{
    	g_Data.bfh.bfType = swap2(g_Data.bfh.bfType);
    	g_Data.bfh.bfSize = swap4(g_Data.bfh.bfSize);
    	g_Data.bfh.bfOffBits = swap4(g_Data.bfh.bfOffBits);
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
	//---------------------------------------------------------------------------------
	
	// Open files and write bitmap files
	//pData->fileHandleArray = (FILE**)malloc(imageFileCount*sizeof(FILE*));
	for (pnum = 0; pnum < imageFileCount; ++pnum)
	{
		char fileName[40];
		char sequenceNumber[10];
		sprintf(sequenceNumber, "%d", pnum);
		//strcpy(fileName, pData->FileName);
		strcpy(fileName, "dsmTestSequenceImage_");
		strcat(fileName, sequenceNumber);
		strcat(fileName, ".bmp");
		g_Data.FileName = strdup(fileName);
		//pData->fileHandleArray[i] = fopen(fileName, "wb");
		g_Data.fp = fopen(fileName, "wb");
		if ( ! g_Data.fp )
		{
			fprintf(stderr, "WriteDSMImagequence: Unable to open [%s]\n", fileName);
		}
		// Write out the BITMAPFILEHEADER
		if ( ! bitmapfileheader(&g_Data.bfh, g_Data.fp) )    
		{
			fprintf(stderr, "WriteDSMImagequence: Error writing to [%s]\n", g_Data.FileName);
		}
		// Write out the BITMAPINFOHEADER
		if ( !fwrite(&g_Data.bmi.bmiHeader, sizeof(BITMAPINFOHEADER), 1, g_Data.fp) )
		{
			fprintf(stderr, "WriteDSMImagequence: Error writing to [%s]\n", g_Data.FileName);
		}
		// Build the bitmap data
		for (i = 0; i < height; ++i)
		{
			readPos = 1+nodeSize*pnum*2; //< +1 to skip over the depth entry and get at the color/visibility part.
										// *2 because there are 2 nodes per unique z-depth in a vis function
			k = 0; // Keep this variable in parallel with j, but always increment k by 1
			for (j = 0; j < width; ++j)
			{
				if (pData->functionLengths[i][k] == -1)
				{
					// Empty bucket encountered
					// Need to fill in white pixels and increment j because the next bucketWidth pixels are empty pixels
					memset(imageBuffer+(3*((i*width)+j))*sizeof(char), 255, 3*bucketWidth*sizeof(char));
					j += bucketWidth-1; // This is probably really bad to modify the loop variable outside the loop construct
					readPos += 1; //Increment by 1, not nodeSize, because DeepData keeps a single float w/ value == -1 to represent empty bucket
				}
				else if (pData->functionLengths[i][k] <= pnum)
				{
					// Passed the end of this pixel's visibility function: output the last value in the vis function
					//imageBuffer[3*((i*width)+j)] = (int)(pData->testDeepData[i][readPos-4]*255); 
					//imageBuffer[3*((i*width)+j)+1] = (int)(pData->testDeepData[i][readPos-3]*255); //< Assuming a color image
					//imageBuffer[3*((i*width)+j)+2] = (int)(pData->testDeepData[i][readPos-2]*255); //< Assuming a color image
					
					memset(imageBuffer+((3*((i*width)+j))*sizeof(char)), 0, 3*sizeof(char));
				}
				else
				{	
					imageBuffer[3*((i*width)+j)] = (int)(pData->testDeepData[i][readPos]*255); 
					imageBuffer[3*((i*width)+j)+1] = (int)(pData->testDeepData[i][readPos+1]*255); //< Assuming a color image
					imageBuffer[3*((i*width)+j)+2] = (int)(pData->testDeepData[i][readPos+2]*255); //< Assuming a color image
					readPos += nodeSize*pData->functionLengths[i][k];
				}
				++k;
			}
		}
		// Write the whole bitmap at once
		if ( ! fwrite(imageBuffer, dataSizeInBytes, 1, g_Data.fp) )
		{
			fprintf(stderr, "WriteDSMImagequence: Error writing file\n");
		}
		// Close file
		fclose(g_Data.fp);
		g_Data.fp = NULL;		
	}
	free(imageBuffer);
	
	// Close files and free memory
	/*
	for (i = 0; i < imageFileCount; ++i)
	{	
		if ( pData->fileHandleArray[i] )
		{
			fclose(pData->fileHandleArray[i]);
			pData->fileHandleArray[i] = NULL;
		}
	}
	free(pData->fileHandleArray);
	*/
}

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
	g_Data.Channels = formatCount; // From this field, the display knows how many floats per visibility node to expect from DspyImageDeepData

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

	g_Data.testDeepData = (float**)malloc(height*sizeof(float*));
	g_Data.functionLengths = (int**)malloc(height*sizeof(int*));
	
	memcpy((void*) pData, (void *) &g_Data, sizeof(DeepShadowData));

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
                          const unsigned char* data,
                          int nodeCount, //< number of floats in data
                          const unsigned char* functionLengths,
                          int functionCount) //< number of vis functions in data, is == width iff there are no empty buckets, else is < width
{

#if SHOW_CALLSTACK
	fprintf(stderr, "DspyImageDeepData called.\n");
#endif

	if ( (ymin+1) != ymax_plusone )
	{
		fprintf(stderr, "DspyImageDeepData: Image data not in scanline format\n");
		return PkDspyErrorBadParams;
	}
	
	DeepShadowData *pData = (DeepShadowData *)image;
	
	pData->testDeepData[ymin] = (float*)malloc(nodeCount*sizeof(float));
	memcpy(pData->testDeepData[ymin], data, nodeCount*sizeof(float));
	pData->functionLengths[ymin] = (int*)malloc((functionCount+1)*sizeof(int));
	memcpy(pData->functionLengths[ymin], functionLengths, functionCount*sizeof(int));
	pData->functionLengths[ymin][functionCount] = ARRAY_TERMINATOR;
	//printf("DspyImageDeepData completed with nodeCount == %d and functionCount is %d.\n", nodeCount, functionCount);
	
	return PkDspyErrorNone;
}

//******************************************************************************
// DspyImageClose
//******************************************************************************
extern "C" PtDspyError DspyImageClose(PtDspyImageHandle image)
{
#if SHOW_CALLSTACK
	fprintf(stderr, "d_dsm_DspyImageClose called.\n");
#endif
	
	// First write out the images to disk
	WriteDSMImageSequence(image);	
	
	DeepShadowData *pData = (DeepShadowData *)image;
	int i;
	
	for (i = 0; i < pData->dsmi.dsmiHeader.iHeight; ++i)
	{
		free(pData->testDeepData[i]);
		free(pData->functionLengths[i]);
	}
	free(pData->testDeepData);
	free(pData->functionLengths);


/*
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

//******************************************************************************
// DWORDALIGN
//******************************************************************************
static int DWORDALIGN(int bits)
{
	return int(((bits + 31) >> 5) << 2);
}

//******************************************************************************
// Save an header for bitmap; must be save field by field since the sizeof is 14
//******************************************************************************
static bool bitmapfileheader(BITMAPFILEHEADER *bfh, FILE *fp)
{
bool retval = true;


    retval = retval && (fwrite(&bfh->bfType, 1, 2, fp) == 2);
    retval = retval && (fwrite(&bfh->bfSize, 1, 4, fp) == 4);
    retval = retval && (fwrite(&bfh->bfReserved1, 1, 2, fp)== 2);
    retval = retval && (fwrite(&bfh->bfReserved2, 1, 2, fp)== 2);
    retval = retval && (fwrite(&bfh->bfOffBits, 1, 4, fp)== 4);
   

return retval;
}


//******************************************************************************
// Swap a short if you are not on NT/Pentium you must swap
//******************************************************************************
static unsigned short swap2( unsigned short s )
{
        unsigned short n;
        unsigned char *in, *out;

        out = ( unsigned char* ) & n;
        in = ( unsigned char* ) & s;

        out[ 0 ] = in[ 1 ];
        out[ 1 ] = in[ 0 ];
        return n;

}

//******************************************************************************
// Swap a long if you are not on NT/Pentium you must swap
//******************************************************************************
static unsigned long swap4(unsigned long l)
{
unsigned long n;
unsigned char *c, *d;

   c = (unsigned char*) &n;
   d = (unsigned char*) &l;


   c[0] = d[3];
   c[1] = d[2];
   c[2] = d[1];
   c[3] = d[0];
   return n;

}

//******************************************************************************
// Determine if we are low endian or big endian
//******************************************************************************
static bool lowendian()
{
 unsigned short low = 0xFFFE;
 unsigned char * pt = (unsigned char *) &low;

 return  (*pt == 0xFF); 
}
