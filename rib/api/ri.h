/*___________________________________________________________________________
**
** Based on Renderman Interface version 3.2
**
** Renderman Interface is Copyright (c) 1988 Pixar. All Rights reserved.
**
**___________________________________________________________________________
*/

/* Is ri.h included already? */
#ifndef	RI_H_INCLUDED
#define	RI_H_INCLUDED 1

#include	"float.h"

#ifdef	WIN32
#  ifdef	AQSIS_STATIC_LINK

#    define  RI_SHARE

#  else /* !AQSIS_STATIC_LINK */

#      ifdef RI_EXPORTS
#        define RI_SHARE __declspec(dllexport)
#      else
#        define RI_SHARE __declspec(dllimport)
#      endif

#  endif	/* AQSIS_STATIC_LINK */

#else	/* !WIN32 */

#  define  RI_SHARE

#endif /* WIN32 */

#ifdef	__cplusplus
extern	"C"
{
#endif


	/*
	 *		RenderMan Interface Standard Include File
	 */

	/* Definitions of Abstract Types Used in RI */

	typedef	short	RtBoolean;
	typedef	int		RtInt;
	typedef	float	RtFloat;

	typedef	char*	RtToken;

	typedef	RtFloat	RtColor[ 3 ];
	typedef	RtFloat	RtPoint[ 3 ];
	typedef	RtFloat	RtMatrix[ 4 ][ 4 ];
	typedef	RtFloat	RtBasis[ 4 ][ 4 ];
	typedef	RtFloat	RtBound[ 6 ];
	typedef	char*	RtString;

	typedef	void*	RtPointer;
	typedef	void	RtVoid;

	typedef	RtFloat	( *RtFilterFunc ) ( RtFloat, RtFloat, RtFloat, RtFloat );
	typedef	RtFloat	( *RtFloatFunc ) ();
	typedef	RtVoid	( *RtFunc ) ();
	typedef	RtVoid	( *RtErrorFunc ) ( RtInt code, RtInt severity, RtString message );
	typedef	RtErrorFunc	RtErrorHandler;

	typedef	RtVoid	( *RtProcSubdivFunc ) ( RtPointer, RtFloat );
	typedef	RtVoid	( *RtProcFreeFunc ) ( RtPointer );
	typedef	RtVoid	( *RtArchiveCallback ) ( RtToken, char *, ... );

	typedef	RtPointer	RtObjectHandle;
	typedef	RtPointer	RtLightHandle;
	typedef	RtPointer	RtContextHandle;

	/* Extern Declarations for Predefined RI Data Structures */

#define	RI_FALSE	0
#define	RI_TRUE		1
#define	RI_INFINITY	FLT_MAX
#define	RI_EPSILON	FLT_EPSILON
#define	RI_NULL		((RtToken)0)

#define	RI_FLOATMIN	FLT_MIN
#define	RI_FLOATMAX	FLT_MAX

#define	RI_PI		3.14159265359f
#define	RI_PIO2		RI_PI/2

#define	RI_SHADER_EXTENSION	".slx"

	RI_SHARE	extern	RtToken	RI_FRAMEBUFFER, RI_FILE;
	RI_SHARE	extern	RtToken	RI_RGB, RI_RGBA, RI_RGBZ, RI_RGBAZ, RI_A, RI_Z, RI_AZ;
	RI_SHARE	extern	RtToken	RI_MERGE, RI_ORIGIN;
	RI_SHARE	extern	RtToken	RI_PERSPECTIVE, RI_ORTHOGRAPHIC;
	RI_SHARE	extern	RtToken	RI_HIDDEN, RI_PAINT;
	RI_SHARE	extern	RtToken	RI_CONSTANT, RI_SMOOTH;
	RI_SHARE	extern	RtToken	RI_FLATNESS, RI_FOV;

	RI_SHARE	extern	RtToken	RI_AMBIENTLIGHT, RI_POINTLIGHT,
	RI_DISTANTLIGHT, RI_SPOTLIGHT;
	RI_SHARE	extern	RtToken	RI_INTENSITY, RI_LIGHTCOLOR, RI_FROM, RI_TO,
	RI_CONEANGLE, RI_CONEDELTAANGLE,
	RI_BEAMDISTRIBUTION;
	RI_SHARE	extern	RtToken	RI_MATTE, RI_METAL, RI_PLASTIC, RI_SHINYMETAL, RI_PAINTEDPLASTIC;
	RI_SHARE	extern	RtToken	RI_KA, RI_KD, RI_KS, RI_ROUGHNESS, RI_KR,
	RI_TEXTURENAME, RI_SPECULARCOLOR;
	RI_SHARE	extern	RtToken	RI_DEPTHCUE, RI_FOG, RI_BUMPY;
	RI_SHARE	extern	RtToken	RI_MINDISTANCE, RI_MAXDISTANCE, RI_BACKGROUND,
	RI_DISTANCE, RI_AMPLITUDE;

	RI_SHARE	extern	RtToken	RI_RASTER, RI_SCREEN, RI_CAMERA, RI_WORLD,
	RI_OBJECT;
	RI_SHARE	extern	RtToken	RI_INSIDE, RI_OUTSIDE, RI_LH, RI_RH;
	/*__declspec(dllimport) RtToken RI_P; */
	RI_SHARE	extern	RtToken	RI_P, RI_PZ, RI_PW, RI_N, RI_NP, RI_CS, RI_OS,
	RI_S, RI_T, RI_ST;
	RI_SHARE	extern	RtToken	RI_BILINEAR, RI_BICUBIC;
	RI_SHARE	extern	RtToken	RI_LINEAR, RI_CUBIC;
	RI_SHARE	extern	RtToken	RI_PRIMITIVE, RI_INTERSECTION, RI_UNION,
	RI_DIFFERENCE;
	RI_SHARE	extern	RtToken	RI_WRAP, RI_NOWRAP, RI_PERIODIC, RI_NONPERIODIC, RI_CLAMP,
	RI_BLACK;
	RI_SHARE	extern	RtToken	RI_IGNORE, RI_PRINT, RI_ABORT, RI_HANDLER;
	RI_SHARE	extern	RtToken	RI_IDENTIFIER, RI_NAME;
	RI_SHARE	extern	RtToken	RI_COMMENT, RI_STRUCTURE, RI_VERBATIM;
	RI_SHARE	extern	RtToken	RI_WIDTH, RI_CONSTANTWIDTH;

	RI_SHARE	extern	RtToken	RI_CURRENT, RI_SHADER, RI_EYE, RI_NDC;

	RI_SHARE	extern	RtBasis	RiBezierBasis, RiBSplineBasis, RiCatmullRomBasis,
	RiHermiteBasis, RiPowerBasis;

#define	RI_BEZIERSTEP		((RtInt)3)
#define	RI_BSPLINESTEP		((RtInt)1)
#define	RI_CATMULLROMSTEP	((RtInt)1)
#define	RI_HERMITESTEP		((RtInt)2)
#define	RI_POWERSTEP		((RtInt)4)

	RI_SHARE	extern	RtInt	RiLastError;

	/* Declarations of All of the RenderMan Interface Subroutines */

#define	PARAMETERLIST	RtInt count, RtToken tokens[], RtPointer values[]

	/* Include the automatically generated procedure declarations. 
	   Generated from api.xml, using apiheader.xsl */

#include	"ri.inl"

	/* Specific to Aqsis */

	typedef	RtVoid	( *RtProgressFunc ) ( RtFloat PercentComplete, RtInt FrameNo );

	RI_SHARE	RtBoolean	BasisFromName( RtBasis * b, const char * strName );
	RI_SHARE	RtVoid	RiProgressHandler( RtProgressFunc handler );
	RI_SHARE	RtFunc	RiPreRenderFunction( RtFunc function );
	RI_SHARE	RtFunc	RiPreWorldFunction( RtFunc function );

#ifdef	__cplusplus
}
#endif

/*
  Error Codes
  
   1 - 10         System and File Errors
  11 - 20         Program Limitations
  21 - 40         State Errors
  41 - 60         Parameter and Protocol Errors
  61 - 80         Execution Errors
*/
#define RIE_NOERROR     ((RtInt)0)

#define RIE_NOMEM       ((RtInt)1)      /* Out of memory */
#define RIE_SYSTEM      ((RtInt)2)      /* Miscellaneous system error */
#define RIE_NOFILE      ((RtInt)3)      /* File nonexistent */
#define RIE_BADFILE     ((RtInt)4)      /* Bad file format */
#define RIE_VERSION     ((RtInt)5)      /* File version mismatch */
#define RIE_DISKFULL    ((RtInt)6)      /* Target disk is full */

#define RIE_INCAPABLE   ((RtInt)11)     /* Optional RI feature */
#define RIE_UNIMPLEMENT ((RtInt)12)     /* Unimplemented feature */
#define RIE_LIMIT       ((RtInt)13)     /* Arbitrary program limit */
#define RIE_BUG         ((RtInt)14)     /* Probably a bug in renderer */

#define RIE_NOTSTARTED  ((RtInt)23)     /* RiBegin not called */
#define RIE_NESTING     ((RtInt)24)     /* Bad begin-end nesting */
#define RIE_NOTOPTIONS  ((RtInt)25)     /* Invalid state for options */
#define RIE_NOTATTRIBS  ((RtInt)26)     /* Invalid state for attribs */
#define RIE_NOTPRIMS    ((RtInt)27)     /* Invalid state for primitives */
#define RIE_ILLSTATE    ((RtInt)28)     /* Other invalid state */
#define RIE_BADMOTION   ((RtInt)29)     /* Badly formed motion block */
#define RIE_BADSOLID    ((RtInt)30)     /* Badly formed solid block */

#define RIE_BADTOKEN    ((RtInt)41)     /* Invalid token for request */
#define RIE_RANGE       ((RtInt)42)     /* Parameter out of range */
#define RIE_CONSISTENCY ((RtInt)43)     /* Parameters inconsistent */
#define RIE_BADHANDLE   ((RtInt)44)     /* Bad object/light handle */
#define RIE_NOSHADER    ((RtInt)45)     /* Can't load requested shader */
#define RIE_MISSINGDATA ((RtInt)46)     /* Required parameters not provided */
#define RIE_SYNTAX      ((RtInt)47)     /* Declare type syntax error */

#define RIE_MATH        ((RtInt)61)     /* Zerodivide, noninvert matrix, etc. */

/* Error severity levels */
#define RIE_INFO        ((RtInt)0)      /* Rendering stats and other info */
#define RIE_WARNING     ((RtInt)1)      /* Something seems wrong, maybe okay */
#define RIE_ERROR       ((RtInt)2)      /* Problem. Results may be wrong */
#define RIE_SEVERE      ((RtInt)3)      /* So bad you should probably abort */


#endif


