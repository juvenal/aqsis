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
// CountUniqueZDepths
//
// For testing: find and return the number of unique z-depths in the DSM
//******************************************************************************
int CountUniqueZDepths(PtDspyImageHandle image)
{
	DeepShadowData *pData = (DeepShadowData *)image;
	const int nodeSize = pData->Channels+1; // The channels field gets you 1,2, or 3 for the rgb channels, plus 1 for depth
	int count = 0;
	int row, j;
	int stopPoint;
	float sampleDepth;
	//std::vector<float> uniqueVals;
	//std::priority_queue<float, std::list<float> > uniqueVals;
	std::vector<float> uniqueVals;
	
	for(row = 0; row < pData->dsmi.dsmiHeader.iHeight; ++row)
	{
		j = 0;
		stopPoint = 0;
		while (pData->functionLengths[row][j] != ARRAY_TERMINATOR)
		{
			if (pData->functionLengths[row][j] != -1)
			{
				stopPoint += pData->functionLengths[row][j];
			}
			++j;
		}
		for (j = 0; j < stopPoint; ++j)
		{
			sampleDepth = pData->testDeepData[row][j*nodeSize];
			if (!std::binary_search(uniqueVals.begin(), uniqueVals.end(), sampleDepth))
			{
				uniqueVals.push_back(sampleDepth);
				std::sort(uniqueVals.begin(), uniqueVals.end());
				count++;
			}
		}
	}
	return count;
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
	// I guess what we really want is not the # of nodes in the longest function, but how many unique depths are there in the dsm
	// The number of bitmaps in the sequence will equal the number of unique z-depths in the dsm: bit this might be a very big number?
	int imageFileCount = 1;
	//const int imageFileCount = LengthOfLongestVisFunc(image); ///< Determine how many files are in the sequence
	// The CountUniqueZDepths function is terribly inefficient: I have a different strategy below.
	//const int imageFileCount = CountUniqueZDepths(image); ///< Determine how many files are in the sequence
	const int width = pData->dsmi.dsmiHeader.iWidth;
	const int height = pData->dsmi.dsmiHeader.iHeight;
	const int dataSizeInBytes = 3*width*height*sizeof(char); // size in bytes of the bitmap data
	const int nodeSize = pData->Channels+1; // The channels field gets you 1,2, or 3 for the rgb channels, plus 1 for depth
	const int bucketWidth = 16; // \todo Somehow get the actual bucket width
	int i, j, k;
	float currentDepth = 0; // the z-slice we are currently writing to bitmap
	std::vector<float> uniqueDepths;
	uniqueDepths.reserve(100);
	uniqueDepths.push_back(0);
	float greatestDepth = 0; 
	int pnum; // the number of the current bitmap image we are building in the sequence
	int readPos = 0; // array index into the deep data: points to the beginning (z-depth) of a visibility node
	
	// Create a buffer the size of a bitmap
	char* imageBuffer = (char*)malloc(dataSizeInBytes);
	//memset(imageBuffer, 0, dataSizeInBytes); // Black out the image
	
	//printf("There will be %d images in the sequence.\n", imageFileCount);
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
		
		/* Here is the new strategy for building these bitmaps:
		 * The problem I perceive with this strategy is that if one visibility function's hits
		 * are all nested inside of a range in the main (longest) visibility function, then
		 * those changes will not be recorded in the bitmap sequence. Likewise, if one function's hits come after the end of 
		 * the main function (the hits are at greater z-depths) they fail to be recorded. 
		 * This is why I think we need a sorted list of all the unique z-depths in the DSM.
		 * Unfortunately, that results in a huge number of depths, and the job of building the list
		 * is computationally prohibitive.
		 * 
		 * 1) Iterate over the DSM data one z-slice at a time: maintain a variable indicating the current z-depth
		 * 2) Always start at the beginning of each visibility function
		 * 3) If the current function's length < current pnum, and its last zdepth < current zdepth, write a pixel color equal to
		 * the last visibility node in this function.
		 * Else if current function's length < current pnum, and last zdepth >= current zdepth, then find the last node in
		 * this function whos zdepth is less than or equa to current zdepth and use its color.
		 * Else if current function's length > current pnum, find via interpolation the visibility at current zdepth in
		 * this visibility function: that is, find the two nodes that sandwhich the current z-depth, and linearly interpolate between them.
		 * Else if current bucket is empty, write out white pixels, increment the column pointer, and increment readPos by 1 to 
		 * point to the beginning of the next visibility function.
		 * 4) Get to the next function by adding nodeSize*currentFunctionLength to readPos
		 */
		
		/*
		 * Possible efficient strategy for building a list of unique z-depths
		*
		* keep a single float variable to track the largest zDepth in a visibility function we have seen so far.
		* Always compare it against the current node's zDepth and update if the current one is greater. 
		* Every time you have to update the variable, add 1 to the imageFileCount, so that you will 
		* output a bitmap corresponding to that depth. I still fear there will be way too many z-depths, however.
		*
		* Perhaps we should do compression on the visibility functions so there will be fewer bitmaps.
		* 
		* Ok. So this strategy also fails because depths are not necessary checked in least-to-greatest order,
		* so we are bound to skip over some. What an issue!
		*/
		
		// Note: We should probably make this a qeueue, and deque on the line below.
		currentDepth = uniqueDepths[pnum];
		printf("currentDepth is %f\n", currentDepth);
		float nextNodeDepth;
		int thisFunctionLength;
		int endNodePos;
		// Build the bitmap data
		for (i = 0; i < height; ++i)
		{
			readPos = 0;
			k = 0; // Keep this variable in parallel with j, but always increment k by 1
			for (j = 0; j < width; ++j)
			{
				thisFunctionLength = pData->functionLengths[i][k];
				nextNodeDepth = (float)(pData->testDeepData[i][readPos]);
				if (nextNodeDepth > greatestDepth)
				{
					printf("Adding to uniqueDepths: %f\n", nextNodeDepth);
					uniqueDepths.push_back(nextNodeDepth);
					greatestDepth = nextNodeDepth;
					++imageFileCount; // This modifys our sentinal variable. Bad idea?
				}
				endNodePos = readPos+nodeSize*(thisFunctionLength-1);
				// Most common case first:
				// If current function's length > current pnum, find via interpolation the visibility at current zdepth in
				// this visibility function: that is, find the two nodes that sandwhich the current z-depth, and linearly interpolate between them.
				// For surface transmittance we can just use the first node's values because there is no slope change
				// ATTENTION: THE IS CLAUSE SHOULD ALSO CHECK THAT THE LAST NODE IS DEEPER THAN CURRENT DEPTH
				if (thisFunctionLength > pnum)
				{
					int node1Pos = readPos, node2Pos = readPos+nodeSize;
					int c;
					for (c = 0; c < thisFunctionLength; ++c)
					{
						if ((currentDepth >= (float)(pData->testDeepData[i][node1Pos])) && (currentDepth <= (float)(pData->testDeepData[i][node2Pos])))
						{							
							// I'm not quite sure how the linear interpolation should work. What should the interpolation parameter be?
							//float lerpParam = (currentDepth-node1.zdepth)/(node2.zdepth-node1.zdepth);
							//lerp(const T t, const V x0, const V x1)
							// Just use the first node's values for now
							imageBuffer[3*((i*width)+j)] = (int)(pData->testDeepData[i][node1Pos+1]*255); 
							imageBuffer[3*((i*width)+j)+1] = (int)(pData->testDeepData[i][node1Pos+2]*255); //< Assuming a color image
							imageBuffer[3*((i*width)+j)+2] = (int)(pData->testDeepData[i][node1Pos+3]*255); //< Assuming a color image
							readPos += nodeSize*thisFunctionLength;
							break;
						}
						node1Pos += nodeSize;
						node2Pos += nodeSize;
					}
					if (c == thisFunctionLength)
					{
						//printf("Error: could not find sandwich nodes in WriteDSMImageSequence()\n");
					}
				}		
				// Else if current bucket is empty, write out white pixels, increment the column pointer, and increment readPos by 1 to 
				// point to the beginning of the next visibility function.				
				else if (thisFunctionLength == -1)
				{
					memset(imageBuffer+(3*((i*width)+j))*sizeof(char), 255, 3*bucketWidth*sizeof(char));
					j += bucketWidth; // This is probably really bad to modify the loop variable outside the loop header
					readPos += 1; 
				}				
				// Else If the current function's length < current pnum, and its last zdepth < current zdepth, write a pixel color equal to
				// the last visibility node in this function.
				else if ((thisFunctionLength < pnum) && ((float)(pData->testDeepData[i][endNodePos]) < currentDepth))
				{	
					imageBuffer[3*((i*width)+j)] = (int)(pData->testDeepData[i][endNodePos+1]*255); 
					imageBuffer[3*((i*width)+j)+1] = (int)(pData->testDeepData[i][endNodePos+2]*255); //< Assuming a color image
					imageBuffer[3*((i*width)+j)+2] = (int)(pData->testDeepData[i][endNodePos+3]*255); //< Assuming a color image
					readPos += nodeSize*thisFunctionLength;
				}
				// Else if current function's length < current pnum, but last zdepth >= current zdepth, then find the last node in
				// this function whose zdepth is less than or equal to current zdepth and use its color.
				// This case should be default if none of the above cases pass
				else if ((thisFunctionLength < pnum) && ((float)(pData->testDeepData[i][endNodePos]) >= currentDepth))
				{
					int tempNodePos = endNodePos;
					while (pData->testDeepData[i][tempNodePos] > currentDepth)
					{
						tempNodePos -= nodeSize;
					}
					imageBuffer[3*((i*width)+j)] = (int)(pData->testDeepData[i][tempNodePos+1]*255); 
					imageBuffer[3*((i*width)+j)+1] = (int)(pData->testDeepData[i][tempNodePos+2]*255); //< Assuming a color image
					imageBuffer[3*((i*width)+j)+2] = (int)(pData->testDeepData[i][tempNodePos+3]*255); //< Assuming a color image
					readPos += nodeSize*thisFunctionLength;					
				}
				else
				{
					//printf("Problem: the case didn't get matched.\n This function length is %d, pnum is %d, currentDepth is %d", 
					//		thisFunctionLength, pnum, currentDepth);
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
