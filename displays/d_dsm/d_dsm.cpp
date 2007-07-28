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
int CountFunctions(const int* functionLengths, int rowWidth);
int CountNodes(const int* functionLengths, int rowWidth);
static int  DWORDALIGN(int bits);
static bool bitmapfileheader(BITMAPFILEHEADER *bfh, FILE *fp);
static unsigned short swap2( unsigned short s );
static unsigned long  swap4( unsigned long l );
static bool lowendian();


// End section from d_sdcBMP.cpp
//***********************************************************************************

struct FloatSort
{
     bool operator()(const float& a, const float& b)
     {
          return a < b;
     }
};

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
	int				flags;
	// NOTE: members below are for the testing phase, writing out bitmap sequences
	float** testDeepData; ///< Array of pointers to the row data
	int** functionLengths; ///< Array of pointers to arrays (one per row, terminated with an ARRAY_TERMINATOR) of vis function lengths (# of nodes) of each visbility function in testDeepData
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
// GreatestNodeDepth
//
// For testing: find and return the depth of the deepest visibility node in the DSM
//******************************************************************************
float GreatestNodeDepth(const DeepShadowData* pData)
{
	const int nodeSize = pData->Channels+1; // The channels field gets you 1,2, or 3 for the rgb channels, plus 1 for depth
	const int rowWidth = pData->dsmi.dsmiHeader.iWidth;
	const int columnHeight = pData->dsmi.dsmiHeader.iHeight;
	int rowNodeCount;	
	float greatestDepth = 0;
	float sampleDepth;
	int row, j;
	
	for(row = 0; row < columnHeight; ++row)
	{
		j = 0;
		rowNodeCount = CountNodes(pData->functionLengths[row], rowWidth);
		for (j = 0; j < rowNodeCount; ++j)
		{
			// Segfault line below: rowNodeCount is too big
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
	const DeepShadowData *pData = (DeepShadowData *)image;
	const int width = pData->dsmi.dsmiHeader.iWidth;
	const int height = pData->dsmi.dsmiHeader.iHeight;
	const int nodeSize = pData->Channels+1; // The channels field gets you 1,2, or 3 for the rgb channels, plus 1 for depth
	const int bucketWidth = 16; // \todo Somehow get the actual bucket width
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
				j += bucketWidth-2; // This is probably really bad to modify the loop variable outside the loop header
				++k;				
				continue;
			}
			printf("Depth is %f and vis is %f\n", (float)(pData->testDeepData[i][readPos]), (float)(pData->testDeepData[i][readPos+1]));
			++k;
			readPos += nodeSize;
		}
	}	
/*	
	for (i = ymin; i < ymaxplus1; ++i)
	{
		readPos = 0;
		k = 0; // Keep this variable in parallel with j, but always increment k by 1
		for (j = 0; j < width; ++j)
		{
			thisFunctionLength = pData->functionLengths[i][k];
			if( thisFunctionLength == -1)
			{
				j += bucketWidth-2; // This is probably really bad to modify the loop variable outside the loop header
				++k;				
				continue;
			}
			if((float)(pData->testDeepData[i][readPos]) != 0 || (float)(pData->testDeepData[i][readPos+1]) != 1)
			{
				printf("Error in d_dsm:checkData() first vis func node not as expected with depth %f and vis %f\n", (float)(pData->testDeepData[i][readPos]), (float)(pData->testDeepData[i][readPos+1]));
			}
			assert((float)(pData->testDeepData[i][readPos]) == 0); // Assert depth is 0
			assert((float)(pData->testDeepData[i][readPos+1]) == 1); // Assert white
			++k;
			readPos += nodeSize*thisFunctionLength;
		}
	}
	*/
}

void PrepareBitmapHeader(const DeepShadowData* pData, AppData& g_Data)
{
	const int width = pData->dsmi.dsmiHeader.iWidth;
	const int height = pData->dsmi.dsmiHeader.iHeight;
	
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
void BuildBitmapData(char* imageBuffer, const int pnum, const DeepShadowData* pData)
{
	const int imageFileCount = 20; //< Number of bitmaps we want to output
	const float sliceInterval = GreatestNodeDepth(pData)/imageFileCount; //< Depth delta: distance between DSM slices
	const int width = pData->dsmi.dsmiHeader.iWidth;
	const int height = pData->dsmi.dsmiHeader.iHeight;
	const int nodeSize = pData->Channels+1; // The channels field gets you 1,2, or 3 for the rgb channels, plus 1 for depth
	const int bucketWidth = 16; // \todo Somehow get the actual bucket width
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
			// If current bucket is empty, write out white pixels, increment the column pointer, and increment readPos by 1 to 
			// point to the beginning of the next visibility function.
			
			if (thisFunctionLength == -1)
			{
				// Choose the appropriate width of pixels to write: if near the edge, may be fewer than bucketWidth pixels
				int writeWidth = bucketWidth;
				if (j+bucketWidth > width)
				{
					writeWidth = width-j;
				}
				memset(imageBuffer+((3*((i*width)+j))*sizeof(char)), 255, 3*writeWidth*sizeof(char)); // Use white for 100% visibility (no shadow with this pixel)
				j += bucketWidth-2; // This is probably really bad to modify the loop variable outside the loop header
				++k;
				continue;
			}
			nextNodeDepth = (float)(pData->testDeepData[i][readPos]);
			endNodePos = readPos+nodeSize*(thisFunctionLength-1);
			endNodeDepth = (float)(pData->testDeepData[i][endNodePos]);
			// This stuff should be white (1) but it is black (0)
			// At some point data is corrupted or copied incorrectly or otherwise out of range
			// Write a checkData() function to assert that the first node in all the visibility functions is white (0,1)
			imageBuffer[3*((i*width)+j)] = (int)(pData->testDeepData[i][readPos+1]*255); 
			imageBuffer[3*((i*width)+j)+1] = (int)(pData->testDeepData[i][readPos+2]*255); //< Assuming a color image
			imageBuffer[3*((i*width)+j)+2] = (int)(pData->testDeepData[i][readPos+3]*255); //< Assuming a color image
			readPos += nodeSize*thisFunctionLength;
			continue;
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
						imageBuffer[3*((i*width)+j)] = (int)(pData->testDeepData[i][node1Pos+1]*255); 
						imageBuffer[3*((i*width)+j)+1] = (int)(pData->testDeepData[i][node1Pos+2]*255); //< Assuming a color image
						imageBuffer[3*((i*width)+j)+2] = (int)(pData->testDeepData[i][node1Pos+3]*255); //< Assuming a color image
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
			// Else If the current function's endNodeDepth <= currentDepth,  write a pixel color equal to
			// the last visibility node in this function.
			else if (endNodeDepth <= currentDepth)
			{	
				imageBuffer[3*((i*width)+j)] = (int)(pData->testDeepData[i][endNodePos+1]*255); 
				imageBuffer[3*((i*width)+j)+1] = (int)(pData->testDeepData[i][endNodePos+2]*255); //< Assuming a color image
				imageBuffer[3*((i*width)+j)+2] = (int)(pData->testDeepData[i][endNodePos+3]*255); //< Assuming a color image
				//printf("color R is %d\n", (int)(pData->testDeepData[i][endNodePos+1]*255));
				//printf("color G is %d\n", (int)(pData->testDeepData[i][endNodePos+2]*255));
				//printf("color B is %d\n", (int)(pData->testDeepData[i][endNodePos+3]*255));
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
	const DeepShadowData *pData = (DeepShadowData *)image;
	static AppData g_Data;
	const int imageFileCount = 20; //< Number of bitmaps we want to output
	const int width = pData->dsmi.dsmiHeader.iWidth;
	const int height = pData->dsmi.dsmiHeader.iHeight;
	const int dataSizeInBytes = 3*width*height*sizeof(char); // size in bytes of the bitmap data
	int pnum; // the number of the current bitmap image we are building in the sequence
	
	// Create a buffer the size of a bitmap
	char* imageBuffer = new char[dataSizeInBytes];
	//memset(imageBuffer, 0, dataSizeInBytes); // Black out the image
	
	PrepareBitmapHeader(pData, g_Data);
	
	// Open files and write bitmap files
	for (pnum = 0; pnum < 1; ++pnum)
	{
		char fileName[40];
		char sequenceNumber[10];
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
			fprintf(stderr, "WriteDSMImagequence: Error writing to [%s]\n", g_Data.FileName);
		}
		// Write out the BITMAPINFOHEADER
		if ( !fwrite(&g_Data.bmi.bmiHeader, sizeof(BITMAPINFOHEADER), 1, g_Data.fp) )
		{
			fprintf(stderr, "WriteDSMImagequence: Error writing to [%s]\n", g_Data.FileName);
		}
		// Build the bitmap data
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

	// Note: flags can also be binary ANDed with PkDspyFlagsWantsEmptyBuckets and PkDspyFlagsWantsNullEmptyBuckets
	flagstuff->flags = PkDspyFlagsWantsScanLineOrder;
	g_Data.flags = PkDspyFlagsWantsScanLineOrder;

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

	g_Data.testDeepData = new float*[height*sizeof(float*)];
	g_Data.functionLengths = new int*[height*sizeof(int*)];
	
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
                          const unsigned char* functionLengths)
{

#if SHOW_CALLSTACK
	fprintf(stderr, "DspyImageDeepData called.\n");
#endif
	
	DeepShadowData *pData = (DeepShadowData *)image;
	if ( ((ymin+1) != ymax_plusone) && (pData->flags == PkDspyFlagsWantsScanLineOrder))
	{
		fprintf(stderr, "DspyImageDeepData: Image data not in scanline format\n");
		return PkDspyErrorBadParams;
	}
	const int functionCount = CountFunctions((const int*)functionLengths, xmax_plusone-xmin);
	const int nodeCount = CountNodes((const int*)functionLengths, xmax_plusone-xmin);
	
	//printf("functionCount is %d\n", functionCount);
	//printf("nodeCount is %d\n", nodeCount);
	
	pData->testDeepData[ymin] = new float[nodeCount*4*sizeof(float)];
	memcpy(pData->testDeepData[ymin], data, nodeCount*sizeof(float));
	pData->functionLengths[ymin] = new int[functionCount*sizeof(int)];
	memcpy(pData->functionLengths[ymin], functionLengths, functionCount*sizeof(int));
	
	checkData(image, ymin, ymax_plusone);
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
	
	DeepShadowData *pData = (DeepShadowData *)image;
	int i;

	delete[] pData->testDeepData;
	delete[] pData->functionLengths;
	
	return PkDspyErrorNone;
}

int CountFunctions(const int* fLengths, int rowWidth)
{
	const int bucketWidthMinusOne = 15; //< I really need to access the true bucket width somehow
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

int CountNodes(const int* fLengths, int rowWidth)
{
	const int bucketWidth = 16; //< I really need to access the true bucket width somehow
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
