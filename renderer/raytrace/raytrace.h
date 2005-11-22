//------------------------------------------------------------------------------
/**
 *	@file	raytrace.h
 *	@author	Paul Gregory
 *	@brief	Implement the basic raytracer subsystem interface implementation.
 *
 *	Last change by:		$Author$
 *	Last change date:	$Date$
 */ 
//------------------------------------------------------------------------------


#ifndef	___raytrace_Loaded___
#define	___raytrace_Loaded___

#include	"aqsis.h"
#include	"iraytrace.h"

START_NAMESPACE( Aqsis )


struct CqRaytrace : public IqRaytrace
{
			 CqRaytrace()
	{}
    virtual ~CqRaytrace()
    {}


	// Interface functions overridden from IqRaytrace
	virtual	void	Initialise();
	virtual	void	AddPrimitive(const boost::shared_ptr<IqSurface>& pSurface);
	virtual void	Finalise();
};




//-----------------------------------------------------------------------

END_NAMESPACE( Aqsis )

#endif	//	___raytrace_Loaded___
