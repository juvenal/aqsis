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
 * \brief Implements the deep shadow display device for Aqsis.
 * This driver stores deep shadow map image data in a DTEX tiled image file. 
*/

#include "aqsis.h"

#include <string>
#include <sstream>
#include <iostream>
#include <vector>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>

#include "dtex.h"
#include "ndspy.h"

// From displayhelpers.c
// Note: We should probably put these functions in a global library because they are used by this display as
// well as d_exr and the main display. Currently each subdirectory has its own copy of the functions.
#ifdef __cplusplus
extern "C"
{
#endif
	PtDspyError DspyReorderFormatting(int formatCount, PtDspyDevFormat *format, int outFormatCount, const PtDspyDevFormat *outFormat);
	PtDspyError DspyFindStringInParamList(const char *string, char **result, int n, const UserParameter *p);
	PtDspyError DspyFindIntInParamList(const char *string, int *result, int n, const UserParameter *p);
	PtDspyError DspyFindFloatInParamList(const char *string, float *result, int n, const UserParameter *p);
	PtDspyError DspyFindMatrixInParamList(const char *string, float *result, int n, const UserParameter *p);
	PtDspyError DspyFindIntsInParamList(const char *string, int *resultCount, int *result, int n, const UserParameter *p);
#ifdef __cplusplus
}
#endif

//*****************************************************************************
// The following is taken directly from d_sdcBMP.cpp, temporarily, so that I may write BMP image sequences to disk during the testing phase.

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
    int    biWidth; 
    int    biHeight; 
    short   biPlanes; 
    short   biBitCount;
    int     biCompression; 
    int     biSizeImage; 
    int    biXPelsPerMeter; 
    int    biYPelsPerMeter; 
    int     biClrUsed; 
    int     biClrImportant; 
} BITMAPINFOHEADER; 

typedef struct tagBITMAPINFO {
    BITMAPINFOHEADER    bmiHeader;
    RGBQUAD             bmiColors[1];
} BITMAPINFO;

#define BI_RGB 0

typedef struct
{
	// Bitmap data

	FILE              *fp;
	BITMAPFILEHEADER  bfh;
	std::string       FileName;
	BITMAPINFO        bmi;
	char              *ImageData;
	int               Channels;
	int               RowSize;
	int               PixelBytes;
	int              TotalPixels;
}
AppData;

// -----------------------------------------------------------------------------
// Function Prototypes
// -----------------------------------------------------------------------------
static int  DWORDALIGN(int bits);
static bool bitmapfileheader(BITMAPFILEHEADER *bfh, FILE *fp);
static unsigned short swap2( unsigned short s );
static unsigned int  swap4( unsigned int l );
static bool lowendian();

// End section from d_sdcBMP.cpp
//***********************************************************************************

typedef struct
{
	CqDeepTexOutputFile *DtexFile;
	int				iWidth; //< Image width in pixels
	int				iHeight;
	int				bucketDimensions[2]; //< Bucket width, height in pixels
	int             Channels;
	int				flags; //< Does the display want scanline order, null empty buckets, empty buckets at all?
	// NOTE: members below are for the testing phase, writing out bitmap sequences
	FILE	fp;
	float** testDeepData; ///< Array of pointers to the row data
	int**	functionLengths; ///< Array of pointers to arrays (one per row) of vis function lengths (# of nodes) of each visbility function in testDeepData
}
SqDeepShadowData;

// -----------------------------------------------------------------------------
// Function Prototypes
// -----------------------------------------------------------------------------
int CountFunctions(const int* functionLengths, const int rowWidth, const int bucketWidth);
int CountNodes(const int* functionLengths, const int rowWidth, const int bucketWidth);
float GreatestNodeDepth(const SqDeepShadowData* pData);
void checkData(PtDspyImageHandle image, int ymin, int ymaxplus1);
void PrepareBitmapHeader(const SqDeepShadowData* pData, AppData& g_Data);
void BuildBitmapData(char* imageBufferIn, const int pnum, const SqDeepShadowData* pData);
void WriteDSMImageSequence(PtDspyImageHandle image);

// -----------------------------------------------------------------------------
// Constants
static const int DEFAULT_IMAGEWIDTH = 512;
static const int DEFAULT_IMAGEHEIGHT = 512;
static const int DEFAULT_BUCKETWIDTH = 16;
static const int DEFAULT_BUCKETHEIGHT = 16;
// -----------------------------------------------------------------------------

// -----------------------------------------------------------------------------
// Begin Functions
// -----------------------------------------------------------------------------

//******************************************************************************
// GreatestNodeDepth
//
// For testing: find and return the depth of the deepest visibility node in the DSM
//******************************************************************************
float GreatestNodeDepth(const SqDeepShadowData* pData)
{
	const int nodeSize = pData->Channels+1; // The channels field gets you 1,2, or 3 for the rgb channels, plus 1 for depth
	const int rowWidth = pData->iWidth;
	const int columnHeight = pData->iHeight;
	const int bucketWidth = pData->bucketDimensions[0];
	int rowNodeCount;	
	float greatestDepth = 0;
	float sampleDepth;
	int row, j;
	
	for(row = 0; row < columnHeight; ++row)
	{
		j = 0;
		rowNodeCount = CountNodes(pData->functionLengths[row], rowWidth, bucketWidth);
		for (j = 0; j < rowNodeCount; ++j)
		{
			sampleDepth = pData->testDeepData[row][j*nodeSize];
			if (sampleDepth > greatestDepth)
			{
				greatestDepth = sampleDepth;
			}
		}
	}
	return greatestDepth;
}

void checkData(PtDspyImageHandle image, int ymin, int ymaxplus1)
{
	const SqDeepShadowData *pData = (SqDeepShadowData *)image;
	const int width = pData->iWidth;
	const int height = pData->iHeight;
	const int nodeSize = pData->Channels+1; // The channels field gets you 1,2, or 3 for the rgb channels, plus 1 for depth
	const int bucketWidth = pData->bucketDimensions[0];
	int i, j, k, readPos, thisFunctionLength;

	for (i = ymin; i < ymaxplus1; ++i)
	{
		readPos = 0;
		k = 0; // Keep this variable in parallel with j, but always increment k by 1
		for (j = 0; j < width; ++j)
		{
			thisFunctionLength = pData->functionLengths[i][k];
			if( thisFunctionLength == -1)
			{
				j += bucketWidth-1; // This is probably really bad to modify the loop variable outside the loop header
				++k;				
				continue;
			}
			if ((pData->testDeepData[i][readPos] != 0) || (pData->testDeepData[i][readPos+1] != 1))
			{
				printf("Error in d_dsm:checkData():Depth is %f and vis is %f\n", pData->testDeepData[i][readPos], pData->testDeepData[i][readPos+1]);	
			}	
			++k;
			readPos += nodeSize*thisFunctionLength;
		}
	}	
}

void PrepareBitmapHeader(const SqDeepShadowData* pData, AppData& g_Data)
{
	const int width = pData->iWidth;
	const int height = pData->iHeight;
	
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
}

/* BuildBitmapData:
 * 
 * Scrapping the old strategy, the new strategy is to output a rough picture of the
 * DSM as a fixed number of bitmap images, where each bitmap is a "z-slice" of the dsm
 * at a regular interval. Determine the depth of the deepest visibility node in the DSM,
 * and divide by the fixed image count (say, 20 bitmaps) which yeilds the interval between bitmaps.
 * 
 * This is probably the most reasonable thing we can do: it lets you keep a tab on the number of bitmaps
 * generated while still providing a good picture of the DSM, assuming an even distribution of function lengths.
 * 
 */
void BuildBitmapData(char* imageBufferIn, const int pnum, const SqDeepShadowData* pData)
{
	unsigned char* imageBuffer = reinterpret_cast<unsigned char*>(imageBufferIn);
	const int imageFileCount = 20; //< Number of bitmaps we want to output
	const float sliceInterval = GreatestNodeDepth(pData)/imageFileCount; //< Depth delta: distance between DSM slices
	const int width = pData->iWidth;
	const int height = pData->iHeight;
	const int nodeSize = pData->Channels+1; // The channels field gets you 1,2, or 3 for the rgb channels, plus 1 for depth
	const int bucketWidth = pData->bucketDimensions[0];
	const float currentDepth = pnum*sliceInterval; // the z-slice we are currently writing to bitmap
	int i, j, k;
	int readPos = 0; // array index into the deep data: points to the beginning (depth element) of a visibility node
	float nextNodeDepth;
	float endNodeDepth;
	int thisFunctionLength;
	int endNodePos;

	for (i = 0; i < height; ++i)
	{
		readPos = 0;
		k = 0; // Keep this variable in parallel with j, but always increment k by 1
		for (j = 0; j < width; ++j)
		{
			thisFunctionLength = pData->functionLengths[i][k];
			//printf("thisFunctionLength is %d\n", thisFunctionLength);
			// If current bucket is empty, write out white pixels, increment the column pointer, and increment readPos by 1 to 
			// point to the beginning of the next visibility function.
			if (thisFunctionLength == -1)
			{
				// Choose the appropriate width of pixels to write: if near the edge, there may be fewer than bucketWidth pixels remaining to be written
				const int writeWidth = (j+bucketWidth > width) ? width-j : bucketWidth;
				memset(imageBuffer+((3*((i*width)+j))*sizeof(char)), 255, 3*writeWidth*sizeof(unsigned char)); // Use white for 100% visibility (no shadow with this pixel)
				j += bucketWidth-1; // This is probably really bad to modify the loop variable outside the loop header
				++k;
				continue;
			}
			nextNodeDepth = pData->testDeepData[i][readPos];
			endNodePos = readPos+(nodeSize*(thisFunctionLength-1));
			endNodeDepth = pData->testDeepData[i][endNodePos];
//Debug
/*
			if (pData->testDeepData[i][readPos] != 0 || pData->testDeepData[i][readPos+1] != 1)
			{
				printf("Depth is %f and visibility is %f where i is %d and readPos is %d\n", pData->testDeepData[i][readPos], pData->testDeepData[i][readPos+1], i, readPos );
			}
			imageBuffer[3*((i*width)+j)] = (unsigned char)(pData->testDeepData[i][readPos+1]*255); 
			imageBuffer[3*((i*width)+j)+1] = (unsigned char)(pData->testDeepData[i][readPos+2]*255); //< Assuming a color image
			imageBuffer[3*((i*width)+j)+2] = (unsigned char)(pData->testDeepData[i][readPos+3]*255); //< Assuming a color image
			readPos += nodeSize*thisFunctionLength;
			++k;
			continue;
*/
// End debug	
			// Most common case first:
			// If current function's endNodeDepth > currentDepth, find via interpolation the visibility at current zdepth in
			// this visibility function: that is, find the two nodes that sandwhich the current z-depth, and linearly interpolate between them.
			// For surface transmittance we can just use the first node's values because there is no slope change
			if (endNodeDepth > currentDepth)
			{
				int node1Pos = readPos, node2Pos = readPos+nodeSize;
				int c;
				for (c = 0; c < thisFunctionLength; ++c)
				{
					if ((currentDepth >= (float)(pData->testDeepData[i][node1Pos])) && (currentDepth <= (float)(pData->testDeepData[i][node2Pos])))
					{							
						// I'm not quite sure how the linear interpolation should work. What should the interpolation parameter be?
						// joeedh: interpolation: a = a + (b - a)*percentage
						// joeedh: to get percentage, you use the formula (current_z-first_z) / (last_z-first_z)
						//float lerpParam = (currentDepth-node1.zdepth)/(node2.zdepth-node1.zdepth); ///< this is percentage
						//lerp(const T t, const V x0, const V x1)
						// Just use the first node's values for now, but keep in mind once you start vis func compression that there may in fact be slope changes introduced.
						imageBuffer[3*((i*width)+j)] = (unsigned char)(pData->testDeepData[i][node1Pos+1]*255); 
						imageBuffer[3*((i*width)+j)+1] = (unsigned char)(pData->testDeepData[i][node1Pos+2]*255); //< Assuming a color image
						imageBuffer[3*((i*width)+j)+2] = (unsigned char)(pData->testDeepData[i][node1Pos+3]*255); //< Assuming a color image
						readPos += nodeSize*thisFunctionLength;
						c = 0; // We can remove this later, when we remove the if(c == thisFunctionLength) block
						break;
					}
					node1Pos += nodeSize;
					node2Pos += nodeSize;
				}
				if (c == thisFunctionLength)
				{
					// This should never happen
					printf("Error: could not find sandwich nodes in WriteDSMImageSequence()\n");
				}
			}						
			// Else If the current function's endNodeDepth < currentDepth,  write a pixel color equal to
			// the last visibility node in this function.
			else if (endNodeDepth <= currentDepth)
			{	
				imageBuffer[3*((i*width)+j)] = (unsigned char)(pData->testDeepData[i][endNodePos+1]*255); 
				imageBuffer[3*((i*width)+j)+1] = (unsigned char)(pData->testDeepData[i][endNodePos+2]*255); //< Assuming a color image
				imageBuffer[3*((i*width)+j)+2] = (unsigned char)(pData->testDeepData[i][endNodePos+3]*255); //< Assuming a color image
				readPos += nodeSize*thisFunctionLength;
			}
			else
			{
				printf("Problem: the case didn't get matched.\n This function length is %d, pnum is %d, currentDepth is %d", 
						thisFunctionLength, pnum, currentDepth);
			}
			++k;
		}
	}
}

//******************************************************************************
// WriteDSMImageSequence
//
// For testing: output a sequence of image files for visualizing deep data.
//******************************************************************************
void WriteDSMImageSequence(PtDspyImageHandle image)
{
	const SqDeepShadowData *pData = (SqDeepShadowData *)image;
	static AppData g_Data;
	const int imageFileCount = 20; //< Number of bitmaps we want to output
	const int width = pData->iWidth;
	const int height = pData->iHeight;
	const int dataSizeInBytes = 3*width*height*sizeof(char); // size in bytes of the bitmap data
	int pnum; // the number of the current bitmap image we are building in the sequence
	
	// Create a buffer the size of a bitmap
	char* imageBuffer = new char[dataSizeInBytes];
	//memset(imageBuffer, 128, dataSizeInBytes); // Black out the image
	
	PrepareBitmapHeader(pData, g_Data);
	
	// Open files and write bitmap files
	for (pnum = 0; pnum < imageFileCount; ++pnum)
	{
		char fileName[40];
		char sequenceNumber[4];
		sprintf(sequenceNumber, "%d", pnum);
		// When we finally output dsm files, we will use the name below:
		//strcpy(fileName, pData->FileName);
		// But for now we output bitmap sequences
		strcpy(fileName, "dsmTestSequenceImage_");
		strcat(fileName, sequenceNumber);
		strcat(fileName, ".bmp");
		g_Data.FileName = strdup(fileName);
		
		g_Data.fp = fopen(fileName, "wb");
		if ( ! g_Data.fp )
		{
			fprintf(stderr, "WriteDSMImagequence: Unable to open [%s]\n", fileName);
		}
		// Write out the BITMAPFILEHEADER
		if ( ! bitmapfileheader(&g_Data.bfh, g_Data.fp) )    
		{
			fprintf(stderr, "WriteDSMImagequence: Error writing to [%s]\n", g_Data.FileName.c_str());
		}
		// Write out the BITMAPINFOHEADER
		if ( !fwrite(&g_Data.bmi.bmiHeader, sizeof(BITMAPINFOHEADER), 1, g_Data.fp) )
		{
			fprintf(stderr, "WriteDSMImagequence: Error writing to [%s]\n", g_Data.FileName.c_str());
		}
		// Build the bitmap data
		memset(imageBuffer, 128, dataSizeInBytes); //< gray out the whole image
		BuildBitmapData(imageBuffer, pnum, pData);
		// Write the whole bitmap at once
		if ( ! fwrite(imageBuffer, dataSizeInBytes, 1, g_Data.fp) )
		{
			fprintf(stderr, "WriteDSMImagequence: Error writing file\n");
		}
		
		// Close file
		fclose(g_Data.fp);
		g_Data.fp = NULL;		
	}
	delete imageBuffer;
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
                          const UserParameter  *parameters, //< Tell you near and far plane. host computer name, bucket dimensions
                          int                  formatCount,
                          PtDspyDevFormat      *format, //< Note: format[1] will tell you if a node is 1, 2, or 4 bytes (PkDspyUnsigned8, PkDspyUnsigned6, or PkDspyUnsigned32)
                          PtFlagStuff          *flagstuff)
{
#if SHOW_CALLSTACK
	fprintf(stderr, "d_dsm_DspyImageOpen called.\n");
#endif
	
	PtDspyError rval = PkDspyErrorNone;
	const int tileWidth = 64; //< We will want to extract the real tile dimensions from parameters. Use 64 for now.
	const int tileHeight = 64;

	if ( width <= 0 )
		width = DEFAULT_IMAGEWIDTH;
	if ( height <= 0 )
		height = DEFAULT_IMAGEHEIGHT;
	
	//static SqDeepShadowData g_Data;
	SqDeepShadowData *pData = new SqDeepShadowData();
	//pData = (SqDeepShadowData *) calloc(1, sizeof(g_Data));
	*image = pData; // This is how the caller gets a handle on this new image
	
	// Extract any important data from the user parameters.
	// Extract bucket dimensions: bucketWidth and bucketHeight
	// Note that these numbers are not constant across the image:
	// bucket width and height will be smaller than the common case
	// when the bucket lies at a boundary position (bottom or far right)
	// and the image pixel dimensions are not evenly divisible by the common
	// bucket dimensions
	pData->bucketDimensions[0] = 16;
	pData->bucketDimensions[1] = 16;
	TqInt count = 2;
	DspyFindIntsInParamList("BucketDimensions", &count, pData->bucketDimensions, paramCount, parameters);

	// Initialize our global resources
	//memset(&g_Data, sizeof(SqDeepShadowData), 0);

	// Note: flags can also be binary ANDed with PkDspyFlagsWantsEmptyBuckets and PkDspyFlagsWantsNullEmptyBuckets
	flagstuff->flags = PkDspyFlagsWantsScanLineOrder; //< Let's try bucket order now
	//g_Data.flags = PkDspyFlagsWantsScanLineOrder;
	pData->flags = PkDspyFlagsWantsScanLineOrder;
/*
	g_Data.Channels = formatCount; // From this field, the display knows how many floats per visibility node to expect from DspyImageDeepData
	g_Data.iWidth = width;
	g_Data.iHeight = height;
*/	
	pData->Channels = formatCount; // From this field, the display knows how many floats per visibility node to expect from DspyImageDeepData
	pData->iWidth = width;
	pData->iHeight = height;
	
	pData->DtexFile = new CqDeepTexOutputFile(std::string(filename), width, height, 
			tileWidth, tileHeight, pData->bucketDimensions[0], pData->bucketDimensions[1], formatCount, (1+formatCount)*sizeof(float));
	
	// Stuff for testing/debugging
/*
	g_Data.testDeepData = new float*[height*sizeof(float*)];
	g_Data.functionLengths = new int*[height*sizeof(int*)];
*/	
	pData->testDeepData = new float*[height*sizeof(float*)];
	pData->functionLengths = new int*[height*sizeof(int*)];
	//memcpy((void*) pData, (void *) &g_Data, sizeof(SqDeepShadowData));

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
                          int entrysize, //< This tells you the number of color channels in a data element, (exludes the depth field)
                          const unsigned char* data,
                          const unsigned char* functionLengths) //< # of nodes in each non-empty visibility function in data
{

#if SHOW_CALLSTACK
	fprintf(stderr, "DspyImageDeepData called.\n");
#endif
	
	SqDeepShadowData *pData = (SqDeepShadowData *)image;
	if ( ((ymin+1) != ymax_plusone) && (pData->flags == PkDspyFlagsWantsScanLineOrder))
	{
		fprintf(stderr, "DspyImageDeepData: Image data not in scanline format\n");
		return PkDspyErrorBadParams;
	}
	// Make sure that the deep data sent in from ddmanager has the expected number of color channels per node
	if ( pData->Channels != entrysize )
	{
		fprintf(stderr, "DspyImageDeepData: Bad number of color channels in deep data. Expected %d channels, got %d\n", pData->Channels, entrysize);
		return PkDspyErrorBadParams;		
	}

	const float* visData = reinterpret_cast<const float*>(data);
	const int* funLengths = reinterpret_cast<const int*>(functionLengths);
	const int nodeSize = pData->Channels+1;
	const int functionCount = CountFunctions(funLengths, xmax_plusone-xmin, pData->bucketDimensions[0]); //< # of visibility functions in the data
	const int nodeCount = CountNodes(funLengths, xmax_plusone-xmin, pData->bucketDimensions[0]); //< Total # of visbility nodes in data
	
	pData->DtexFile->setTileData(xmin, ymin, xmax_plusone, ymax_plusone, data, functionLengths);
	
	// Everything below is for testing and debugging
	
	//printf("d_dsm: total functionCount is %d\n", functionCount);
	//printf("d_dsm: total nodeCount is %d\n", nodeCount);
	
	pData->testDeepData[ymin] = new float[nodeCount*4];
	memcpy(pData->testDeepData[ymin], visData, nodeSize*nodeCount*sizeof(float));
	pData->functionLengths[ymin] = new int[functionCount];
	memcpy(pData->functionLengths[ymin], funLengths, functionCount*sizeof(int));
	
	// begin debug
	/*
	int readPos = 0;
	int count  = 0;
	for(int c = 0; c < functionCount; ++c)
	{
		//printf("function length is %d\n",funLengths[c]); // THEES FUNCTION LENGTHS ARE BOGUS
		if (funLengths[c] > -1)
		{
			if (visData[readPos] != 0 || visData[readPos+1] != 1)
			{
				printf("Error in DspyImageDeepData: depth is %f and visibility is %f\n", visData[readPos], visData[readPos+1]);
			}
			++count;
			readPos += 4*funLengths[c];
		}
	}
	printf("d_dsm debug, primary node count is %d\n", count);
	checkData(image, ymin, ymax_plusone);
	*/
	// end debug
	
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
	
	// First write out the images to disk (bitmaps for testing)
	WriteDSMImageSequence(image);	
	
	SqDeepShadowData *pData = (SqDeepShadowData *)image;

	delete pData->DtexFile;
	delete[] pData->testDeepData;
	delete[] pData->functionLengths;
	
	return PkDspyErrorNone;
}

int CountFunctions(const int* fLengths, const int rowWidth, const int bucketWidth)
{
	const int bucketWidthMinusOne = bucketWidth-1;
	int functionCount = 0;
	int temp;
	int k = 0;
	
	while( k < rowWidth )
	{
		temp = fLengths[functionCount];
		if (temp == -1)
		{
			k += bucketWidthMinusOne; //< -1 because 1 space was used to hold a (-1) to indicate empty bucket
		}
		++functionCount;
		++k;
	}
	return functionCount;
}

int CountNodes(const int* fLengths, const int rowWidth, const int bucketWidth)
{
	int nodeCount = 0;
	int temp;
	int i = 0;
	int k = 0;

	while( k < rowWidth )
	{
		temp = fLengths[i];
		if (temp > -1)
		{
			nodeCount += temp;
			++k;
		}
		else 
		{
			k += bucketWidth;
		}
		++i;
	}
	return nodeCount;	
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
// Swap a int if you are not on NT/Pentium you must swap
//******************************************************************************
static unsigned int swap4(unsigned int l)
{
unsigned int n;
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

