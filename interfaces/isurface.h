//------------------------------------------------------------------------------
/**
 *	@file	isurface.h
 *	@author	Authors name
 *	@brief	Brief description of the file contents
 *
 *	Last change by:		$Author$
 *	Last change date:	$Date$
 */
//------------------------------------------------------------------------------
#ifndef	___isurface_Loaded___
#define	___isurface_Loaded___

#include	<vector>
#include	<boost/shared_ptr.hpp>

#include	"aqsis.h"
#include	"matrix.h"
#include	"sstring.h"
#include	"ibound.h"

START_NAMESPACE( Aqsis )


struct IqAttributes;
struct IqTransform;
class CqParameter;
struct IqShaderData;
class CqSurface;
class CqMicroPolyGrid;


//----------------------------------------------------------------------
/** \struct IqSurface
 * Abstract base surface class, which provides interfaces to geometry.  
 */

struct IqSurface
{
	virtual	~IqSurface()
	{}
	/** Transform this GPrim using the specified matrices.
	 * \param matTx Reference to the transformation matrix.
	 * \param matITTx Reference to the inverse transpose of the transformation matrix, used to transform normals.
	 * \param matRTx Reference to the rotation only transformation matrix, used to transform vectors.
	 * \param iTime Float value representing the frame time at which to perform the transformation.
	 */
	virtual void	Transform( const CqMatrix& matTx, const CqMatrix& matITTx, const CqMatrix& matRTx, TqInt iTime = 0 ) = 0;
	/** Get the number of uniform parameters required for this GPrim.
	 */
	virtual	TqUint	cUniform() const = 0;
	/** Get the number of varying parameters required for this GPrim.
	 */
	virtual	TqUint	cVarying() const = 0;
	/** Get the number of vertex parameters required for this GPrim.
	 */
	virtual	TqUint	cVertex() const = 0;
	/** Get the number of facevarying parameters required for this GPrim.
	 */
	virtual	TqUint	cFaceVarying() const = 0;
	/** Get the number of facevertex parameters required for this GPrim.
	 */
	virtual	TqUint	cFaceVertex() const = 0;

	virtual CqString	strName() const = 0;
	virtual	TqInt	Uses() const = 0;

	/** Get a pointer to the attributes state associated with this GPrim.
	 * \return A pointer to a CqAttributes class.
	 */
	virtual IqAttributes* pAttributes() const = 0;
	/** Get a pointer to the transformation state associated with this GPrim.
	 * \return A pointer to a CqTransform class.
	 */
	virtual boost::shared_ptr<IqTransform> pTransform() const = 0;
	/** Perform any precalculation required before dicing.
	 *  \param uDiceSize Size that the surface will be diced to in u.
	 *  \param vDiceSize Size that the surface will be diced to in u.
	 */
	virtual void	PreDice( TqInt uDiceSize, TqInt vDiceSize ) = 0;
	/** Fill in any primitive variables the surface can in one go.
	 *  \return A bitvector representing the standard primitive variables initialised.
	 */
	virtual TqInt	DiceAll( CqMicroPolyGrid* pGrid ) = 0;
	/** Interpolate the specified value using the natural interpolation method for the surface.
	 *  Fills in the given shader data with the resulting data.
	 */
	virtual void	NaturalDice( CqParameter* pParameter, TqInt uDiceSize, TqInt vDiceSize, IqShaderData* pData ) = 0;
	/** Perform any post cleanup after dicing.
	 */
	virtual void	PostDice(CqMicroPolyGrid * pGrid) = 0;

	/** Perform any precalculation required before subdividing.
	 *  \param u Flag indicating if we are subdividing in u, if false we are doing v.
	 *	\param aSplits Vector which can be filled with surfaces generated by the split operation.
	 */
	virtual TqInt	PreSubdivide( std::vector<boost::shared_ptr<CqSurface> >& aSplits, bool u ) = 0;
	/** Interpolate the specified value using the natural interpolation method for the surface.
	 *  \param pParam Pointer to the primitive variable we are subdividing.
	 *  \param pParam1 Pointer to the new primitive variable to store the first half.
	 *  \param pParam2 Pointer to the new primitive variable to store the second half.
	 *  \param u Flag indicating if we are subdividing in u, if false we are doing v.
	 */
	virtual void	NaturalSubdivide( CqParameter* pParam, CqParameter* pParam1, CqParameter* pParam2, bool u ) = 0;
	/** Perform any post cleanup after dicing.
	 */
	virtual void	PostSubdivide( std::vector<boost::shared_ptr<CqSurface> >& aSplits ) = 0;
	/** The main processing loop is finished with this surface now, so clean up any data it doesn't need anymore.
	 */
	virtual void	RenderComplete() = 0;
	/** Get the gemoetric bound of this GPrim.
	 */
	virtual void	Bound(IqBound* bound) const = 0;
};


END_NAMESPACE( Aqsis )


#endif	//	___isurface_Loaded___
