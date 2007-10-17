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
		\brief Implements the basic shader operations. (Lights related)
		\author Paul C. Gregory (pgregory@aqsis.org)
*/

#include	"aqsis.h"

#include	<math.h>
#include	<map>
#include	<vector>
#include	<string>
#include	<stdio.h>

#include	"shaderexecenv.h"
#include	"shadervm.h"
#include	"irenderer.h"
#include	"ilightsource.h"
#include	"version.h"
#include	"logging.h"

START_NAMESPACE(    Aqsis )

//----------------------------------------------------------------------
// init_illuminance()
// NOTE: There is duplication here between SO_init_illuminance and 
// SO_advance_illuminance. This is required to ensure that the 
// first light is not skipped.
bool CqShaderExecEnv::SO_init_illuminance()
{
	// Check if lighting is turned off.
	if(getRenderContext())
	{
		const TqInt* enableLightingOpt = getRenderContext()->GetIntegerOption("EnableShaders", "lighting");
		if(NULL != enableLightingOpt && enableLightingOpt[0] == 0)
			return(false);
	}

	m_li = 0;
	while ( m_li < m_pAttributes ->cLights() &&
	        m_pAttributes ->pLight( m_li ) ->pShader() ->fAmbient() )
	{
		m_li++;
	}
	if ( m_li < m_pAttributes ->cLights() )
		return ( true );
	else
		return ( false );
}


//----------------------------------------------------------------------
// advance_illuminance()
bool CqShaderExecEnv::SO_advance_illuminance()
{
	// Check if lighting is turned off, should never need this check as SO_init_illuminance will catch first.
	if(getRenderContext())
	{
		const TqInt* enableLightingOpt = getRenderContext()->GetIntegerOption("EnableShaders", "lighting");
		if(NULL != enableLightingOpt && enableLightingOpt[0] == 0)
			return(false);
	}

	m_li++;
	while ( m_li < m_pAttributes ->cLights() &&
	        m_pAttributes ->pLight( m_li ) ->pShader() ->fAmbient() )
	{
		m_li++;
	}
	if ( m_li < m_pAttributes ->cLights() )
		return ( true );
	else
		return ( false );
}


//----------------------------------------------------------------------
// init_gather()
void CqShaderExecEnv::SO_init_gather(IqShaderData* samples, IqShader* pShader)
{
	bool __fVarying;
	TqUint __iGrid = 0;

	__fVarying=(samples)->Class()==class_varying;

	TqFloat _aq_samples;
	(samples)->GetFloat(_aq_samples,__iGrid);

	// Check if lighting is turned off.
	if(getRenderContext())
	{
		const TqInt* enableLightingOpt = getRenderContext()->GetIntegerOption("EnableShaders", "lighting");
		if(NULL != enableLightingOpt && enableLightingOpt[0] == 0)
			return;
	}

	m_gatherSample = static_cast<TqUint> (_aq_samples);
}


//----------------------------------------------------------------------
// advance_illuminance()
bool CqShaderExecEnv::SO_advance_gather()
{
	// Check if lighting is turned off, should never need this check as SO_init_illuminance will catch first.
	if(getRenderContext())
	{
		const TqInt* enableLightingOpt = getRenderContext()->GetIntegerOption("EnableShaders", "lighting");
		if(NULL != enableLightingOpt && enableLightingOpt[0] == 0)
			return(false);
	}

	return((--m_gatherSample) > 0);
}


void CqShaderExecEnv::ValidateIlluminanceCache( IqShaderData* pP, IqShaderData* pN, IqShader* pShader )
{
	// If this is the first call to illuminance this time round, call all lights and setup the Cl and L caches.
	if ( !m_IlluminanceCacheValid )
	{
		// Check if lighting is turned off.
		if(getRenderContext())
		{
			const TqInt* enableLightingOpt = getRenderContext()->GetIntegerOption("EnableShaders", "lighting");
			if(NULL != enableLightingOpt && enableLightingOpt[0] == 0)
			{
				m_IlluminanceCacheValid = true;
				return;
			}
		}

		IqShaderData* Ns = (pN != NULL )? pN : N();
		IqShaderData* Ps = (pP != NULL )? pP : P();
		TqUint li = 0;
		while ( li < m_pAttributes ->cLights() )
		{
			IqLightsource * lp = m_pAttributes ->pLight( li );
			// Initialise the lightsource
			lp->Initialise( uGridRes(), vGridRes(), microPolygonCount(), shadingPointCount() );
			m_Illuminate = 0;
			// Evaluate the lightsource
			lp->Evaluate( Ps, Ns, m_pCurrentSurface );
			li++;
		}
		m_IlluminanceCacheValid = true;
	}
}

//----------------------------------------------------------------------
// reflect(I,N)
void CqShaderExecEnv::SO_reflect( IqShaderData* I, IqShaderData* N, IqShaderData* Result, IqShader* pShader )
{
	bool __fVarying;
	TqUint __iGrid;

	__fVarying=(I)->Class()==class_varying;
	__fVarying=(N)->Class()==class_varying||__fVarying;
	__fVarying=(Result)->Class()==class_varying||__fVarying;

	__iGrid = 0;
	CqBitVector& RS = RunningState();
	do
	{
		if(!__fVarying || RS.Value( __iGrid ) )
		{
			CqVector3D _aq_I;
			(I)->GetVector(_aq_I,__iGrid);
			CqVector3D _aq_N;
			(N)->GetNormal(_aq_N,__iGrid);
			TqFloat idn = 2.0f * ( _aq_I * _aq_N );
			CqVector3D res = _aq_I - ( idn * _aq_N );
			(Result)->SetVector(res,__iGrid);
		}
	}
	while( ( ++__iGrid < shadingPointCount() ) && __fVarying);
}


//----------------------------------------------------------------------
// reftact(I,N,eta)
void CqShaderExecEnv::SO_refract( IqShaderData* I, IqShaderData* N, IqShaderData* eta, IqShaderData* Result, IqShader* pShader )
{
	bool __fVarying;
	TqUint __iGrid;

	__fVarying=(I)->Class()==class_varying;
	__fVarying=(N)->Class()==class_varying||__fVarying;
	__fVarying=(eta)->Class()==class_varying||__fVarying;
	__fVarying=(Result)->Class()==class_varying||__fVarying;

	__iGrid = 0;
	CqBitVector& RS = RunningState();
	do
	{
		if(!__fVarying || RS.Value( __iGrid ) )
		{
			CqVector3D _aq_I;
			(I)->GetVector(_aq_I,__iGrid);
			CqVector3D _aq_N;
			(N)->GetNormal(_aq_N,__iGrid);
			TqFloat _aq_eta;
			(eta)->GetFloat(_aq_eta,__iGrid);
			TqFloat IdotN = _aq_I * _aq_N;
			TqFloat feta = _aq_eta;
			TqFloat k = 1 - feta * feta * ( 1 - IdotN * IdotN );
			(Result)->SetVector(( k < 0.0f ) ? CqVector3D( 0, 0, 0 ) : CqVector3D( feta * _aq_I - ( feta * IdotN + sqrt( k ) ) * _aq_N ),__iGrid);
		}
	}
	while( ( ++__iGrid < shadingPointCount() ) && __fVarying);
}


//----------------------------------------------------------------------
// fresnel(I,N,eta,Kr,Kt)

void CqShaderExecEnv::SO_fresnel( IqShaderData* I, IqShaderData* N, IqShaderData* eta, IqShaderData* Kr, IqShaderData* Kt, IqShader* pShader )
{
	bool __fVarying;
	TqUint __iGrid;

	__fVarying=(I)->Class()==class_varying;
	__fVarying=(N)->Class()==class_varying||__fVarying;
	__fVarying=(eta)->Class()==class_varying||__fVarying;
	__fVarying=(Kr)->Class()==class_varying||__fVarying;
	__fVarying=(Kt)->Class()==class_varying||__fVarying;

	__iGrid = 0;
	CqBitVector& RS = RunningState();
	do
	{
		if(!__fVarying || RS.Value( __iGrid ) )
		{
			CqVector3D _aq_I;
			(I)->GetVector(_aq_I,__iGrid);
			CqVector3D _aq_N;
			(N)->GetNormal(_aq_N,__iGrid);
			TqFloat _aq_eta;
			(eta)->GetFloat(_aq_eta,__iGrid);
			TqFloat _aq_Kr;
			(Kr)->GetFloat(_aq_Kr,__iGrid);
			TqFloat _aq_Kt;
			(Kt)->GetFloat(_aq_Kt,__iGrid);
			TqFloat cos_theta = -_aq_I * _aq_N;
			TqFloat fuvA = ((1.0f / _aq_eta)*(1.0f / _aq_eta)) - ( 1.0f - ((cos_theta)*(cos_theta)) );
			TqFloat fuvB = fabs( fuvA );
			TqFloat fu2 = ( fuvA + fuvB ) / 2;
			TqFloat fv2 = ( -fuvA + fuvB ) / 2;
			TqFloat fv2sqrt = ( fv2 == 0.0f ) ? 0.0f : sqrt( fabs( fv2 ) );
			TqFloat fu2sqrt = ( fu2 == 0.0f ) ? 0.0f : sqrt( fabs( fu2 ) );
			TqFloat fperp2 = ( ((cos_theta - fu2sqrt)*(cos_theta - fu2sqrt)) + fv2 ) / ( ((cos_theta + fu2sqrt)*(cos_theta + fu2sqrt)) + fv2 );
			TqFloat feta = _aq_eta;
			TqFloat fpara2 = ( ((((1.0f / feta)*(1.0f / feta)) * cos_theta - fu2sqrt)*(((1.0f / feta)*(1.0f / feta)) * cos_theta - fu2sqrt)) + ((-fv2sqrt)*(-fv2sqrt)) ) /
			                 ( ((((1.0f / feta)*(1.0f / feta)) * cos_theta + fu2sqrt)*(((1.0f / feta)*(1.0f / feta)) * cos_theta + fu2sqrt)) + ((fv2sqrt)*(fv2sqrt)) );

			TqFloat __Kr = 0.5f * ( fperp2 + fpara2 );
			(Kr)->SetFloat(__Kr,__iGrid);
			(Kt)->SetFloat(1.0f - __Kr,__iGrid);
		}
	}
	while( ( ++__iGrid < shadingPointCount() ) && __fVarying);
}

//----------------------------------------------------------------------
// fresnel(I,N,eta,Kr,Kt,R,T)
void CqShaderExecEnv::SO_fresnel( IqShaderData* I, IqShaderData* N, IqShaderData* eta, IqShaderData* Kr, IqShaderData* Kt, IqShaderData* R, IqShaderData* T, IqShader* pShader )
{
	bool __fVarying;
	TqUint __iGrid;

	__fVarying=(I)->Class()==class_varying;
	__fVarying=(N)->Class()==class_varying||__fVarying;
	__fVarying=(eta)->Class()==class_varying||__fVarying;
	__fVarying=(Kr)->Class()==class_varying||__fVarying;
	__fVarying=(Kt)->Class()==class_varying||__fVarying;
	__fVarying=(R)->Class()==class_varying||__fVarying;
	__fVarying=(T)->Class()==class_varying||__fVarying;

	__iGrid = 0;
	CqBitVector& RS = RunningState();
	do
	{
		if(!__fVarying || RS.Value( __iGrid ) )
		{
			CqVector3D _aq_I;
			(I)->GetVector(_aq_I,__iGrid);
			CqVector3D _aq_N;
			(N)->GetNormal(_aq_N,__iGrid);
			TqFloat _aq_eta;
			(eta)->GetFloat(_aq_eta,__iGrid);
			TqFloat _aq_Kr;
			(Kr)->GetFloat(_aq_Kr,__iGrid);
			TqFloat _aq_Kt;
			(Kt)->GetFloat(_aq_Kt,__iGrid);
			CqVector3D _aq_R;
			(R)->GetVector(_aq_R,__iGrid);
			CqVector3D _aq_T;
			(T)->GetVector(_aq_T,__iGrid);
			TqFloat cos_theta = -_aq_I * _aq_N;
			TqFloat fuvA = ((1.0f / _aq_eta)*(1.0f / _aq_eta)) - ( 1.0f - ((cos_theta)*(cos_theta)) );
			TqFloat fuvB = fabs( fuvA );
			TqFloat fu2 = ( fuvA + fuvB ) / 2;
			TqFloat fv2 = ( -fuvA + fuvB ) / 2;
			TqFloat feta = _aq_eta;
			TqFloat fv2sqrt = ( fv2 == 0.0f ) ? 0.0f : sqrt( fabs( fv2 ) );
			TqFloat fu2sqrt = ( fu2 == 0.0f ) ? 0.0f : sqrt( fabs( fu2 ) );
			TqFloat fperp2 = ( ((cos_theta - fu2sqrt)*(cos_theta - fu2sqrt)) + fv2 ) / ( ((cos_theta + fu2sqrt)*(cos_theta + fu2sqrt)) + fv2 );
			TqFloat fpara2 = ( ((((1.0f / feta)*(1.0f / feta)) * cos_theta - fu2sqrt)*(((1.0f / feta)*(1.0f / feta)) * cos_theta - fu2sqrt)) + ((-fv2sqrt)*(-fv2sqrt)) ) /
			                 ( ((((1.0f / feta)*(1.0f / feta)) * cos_theta + fu2sqrt)*(((1.0f / feta)*(1.0f / feta)) * cos_theta + fu2sqrt)) + ((fv2sqrt)*(fv2sqrt)) );
			TqFloat __Kr = 0.5f * ( fperp2 + fpara2 );
			(Kr)->SetFloat(__Kr,__iGrid);
			(Kt)->SetFloat(1.0f - __Kr,__iGrid);
		}
	}
	while( ( ++__iGrid < shadingPointCount() ) && __fVarying);

	SO_reflect( I, N, R );
	SO_refract( I, N, eta, T );
}


//----------------------------------------------------------------------
// depth(P)
void CqShaderExecEnv::SO_depth( IqShaderData* p, IqShaderData* Result, IqShader* pShader )
{
	bool __fVarying;
	TqUint __iGrid;

	if (!getRenderContext() )
		return ;

	__fVarying=(p)->Class()==class_varying;
	__fVarying=(Result)->Class()==class_varying||__fVarying;

	__iGrid = 0;
	CqBitVector& RS = RunningState();
	do
	{
		if(!__fVarying || RS.Value( __iGrid ) )
		{
			CqVector3D _aq_p;
			(p)->GetPoint(_aq_p,__iGrid);
			TqFloat d = _aq_p.z();
			d = ( d - getRenderContext() ->GetFloatOption( "System", "Clipping" ) [ 0 ] ) /
			    ( getRenderContext() ->GetFloatOption( "System", "Clipping" ) [ 1 ] - getRenderContext() ->GetFloatOption( "System", "Clipping" ) [ 0 ] );
			(Result)->SetFloat(d,__iGrid);
		}
	}
	while( ( ++__iGrid < shadingPointCount() ) && __fVarying);
}


//----------------------------------------------------------------------
// ambient()

void CqShaderExecEnv::SO_ambient( IqShaderData* Result, IqShader* pShader )
{
	bool __fVarying;
	TqUint __iGrid;

	// Check if lighting is turned off.
	if(getRenderContext())
	{
		const TqInt* enableLightingOpt = getRenderContext()->GetIntegerOption("EnableShaders", "lighting");
		if(NULL != enableLightingOpt && enableLightingOpt[0] == 0)
			return;
	}

	// Use the lightsource stack on the current surface
	if ( m_pAttributes != 0 )
	{
		// If this is the first call to illuminance this time round, call all lights and setup the Cl and L caches.
		if ( !m_IlluminanceCacheValid )
		{
			ValidateIlluminanceCache( NULL, NULL, pShader );
		}

		Result->SetColor( gColBlack );

		for ( TqUint light_index = 0; light_index < m_pAttributes ->cLights(); light_index++ )
		{
			__fVarying = true;

			IqLightsource* lp = m_pAttributes ->pLight( light_index );
			if ( lp->pShader() ->fAmbient() )
			{
				__iGrid = 0;
				CqBitVector& RS = RunningState();
				do
				{
					if(!__fVarying || RS.Value( __iGrid ) )
					{
						// Now Combine the color of all ambient lightsources.
						CqColor _aq_Result;
						(Result)->GetColor(_aq_Result,__iGrid);
						CqColor colCl;
						if ( NULL != lp->Cl() )
							lp->Cl() ->GetColor( colCl, __iGrid );
						(Result)->SetColor(_aq_Result + colCl,__iGrid);

					}
				}
				while( ( ++__iGrid < shadingPointCount() ) && __fVarying);
			}
		}
	}
}


//----------------------------------------------------------------------
// diffuse(N)
void CqShaderExecEnv::SO_diffuse( IqShaderData* N, IqShaderData* Result, IqShader* pShader )
{
	bool __fVarying;
	TqUint __iGrid;

	// If the illuminance cache is already OK, then we don't need to bother filling in the illuminance parameters.
	if ( !m_IlluminanceCacheValid )
	{
		ValidateIlluminanceCache( NULL, N, pShader );
	}

	IqShaderData* pDefAngle = pShader->CreateTemporaryStorage( type_float, class_uniform );
	if ( NULL == pDefAngle )
		return ;

	pDefAngle->SetFloat( PIO2 );

	Result->SetColor( gColBlack );

	__fVarying = true;
	IqShaderData* __nondiffuse = NULL;
	__nondiffuse = pShader->CreateTemporaryStorage( type_float, class_varying );

	// SO_init_illuminance returns TRUE if there are any non ambient ligthsources available.
	if ( SO_init_illuminance() )
	{
		boost::shared_ptr<IqShader> pLightsource;
		do
		{
			// Get the "__nondiffuse" setting from the current lightsource, if specified.
			TqFloat	__nondiffuse_val;
			if ( m_li < m_pAttributes ->cLights() )
				pLightsource = m_pAttributes ->pLight( m_li ) ->pShader();
			if ( pLightsource )
			{
				pLightsource->GetVariableValue( "__nondiffuse", __nondiffuse );
				/// \note: This is OK here, outside the BEGIN_VARYING_SECTION as, varying in terms of lightsources
				/// is not valid.
				if( NULL != __nondiffuse )
				{
					__nondiffuse->GetFloat( __nondiffuse_val, 0 );
					if( __nondiffuse_val != 0.0f )
						continue;
				}
			}

			// SO_illuminance sets the current state to whether the lightsource illuminates the points or not.
			SO_illuminance( NULL, NULL, N, pDefAngle, NULL );

			PushState();
			GetCurrentState();

			__iGrid = 0;
			CqBitVector& RS = RunningState();
			do
			{
				if(!__fVarying || RS.Value( __iGrid ) )
				{

					// Get the light vector and color from the lightsource.
					CqVector3D Ln;
					L() ->GetVector( Ln, __iGrid );
					Ln.Unit();

					// Combine the light color into the result
					CqColor _aq_Result;
					(Result)->GetColor(_aq_Result,__iGrid);
					CqVector3D _aq_N;
					(N)->GetNormal(_aq_N,__iGrid);
					CqColor colCl;
					Cl() ->GetColor( colCl, __iGrid );
					(Result)->SetColor(_aq_Result + colCl * ( Ln * _aq_N ),__iGrid);

				}
			}
			while( ( ++__iGrid < shadingPointCount() ) && __fVarying);
			PopState();
			// SO_advance_illuminance returns TRUE if there are any more non ambient lightsources.
		}
		while ( SO_advance_illuminance() );
	}
	pShader->DeleteTemporaryStorage( __nondiffuse );
	pShader->DeleteTemporaryStorage( pDefAngle );
}


//----------------------------------------------------------------------
// specular(N,V,roughness)
void CqShaderExecEnv::SO_specular( IqShaderData* N, IqShaderData* V, IqShaderData* roughness, IqShaderData* Result, IqShader* pShader )
{
	bool __fVarying;
	TqUint __iGrid;

	// If the illuminance cache is already OK, then we don't need to bother filling in the illuminance parameters.
	if ( !m_IlluminanceCacheValid )
	{
		ValidateIlluminanceCache( NULL, N, pShader );
	}

	IqShaderData* pDefAngle = pShader->CreateTemporaryStorage( type_float, class_uniform );
	if ( NULL == pDefAngle )
		return ;

	pDefAngle->SetFloat( PIO2 );

	Result->SetColor( gColBlack );
	__fVarying = true;

	IqShaderData* __nonspecular = NULL;
	__nonspecular = pShader->CreateTemporaryStorage( type_float, class_varying );

	// SO_init_illuminance returns TRUE if there are any non ambient ligthsources available.
	if ( SO_init_illuminance() )
	{
		boost::shared_ptr<IqShader> pLightsource;
		do
		{
			// Get the "__nonspecular" setting from the current lightsource, if specified.
			TqFloat	__nonspecular_val;
			if ( m_li < m_pAttributes ->cLights() )
				pLightsource = m_pAttributes ->pLight( m_li ) ->pShader();
			if ( pLightsource )
			{
				pLightsource->GetVariableValue( "__nonspecular", __nonspecular );
				/// \note: This is OK here, outside the BEGIN_VARYING_SECTION as, varying in terms of lightsources
				/// is not valid.
				if( NULL != __nonspecular )
				{
					__nonspecular->GetFloat( __nonspecular_val, 0 );
					if( __nonspecular_val != 0.0f )
						continue;
				}
			}

			// SO_illuminance sets the current state to whether the lightsource illuminates the points or not.
			SO_illuminance( NULL, NULL, N, pDefAngle, NULL );

			PushState();
			GetCurrentState();
			__iGrid = 0;
			CqBitVector& RS = RunningState();
			do
			{
				if(!__fVarying || RS.Value( __iGrid ) )
				{

					CqVector3D _aq_V;
					(V)->GetVector(_aq_V,__iGrid);
					// Get the ligth vector and color from the lightsource
					CqVector3D Ln;
					L() ->GetVector( Ln, __iGrid );
					Ln.Unit();
					CqVector3D	H = Ln + _aq_V;
					H.Unit();

					// Combine the color into the result.
					/// \note The (roughness/8) term emulates the BMRT behaviour for prmanspecular.
					CqColor _aq_Result;
					(Result)->GetColor(_aq_Result,__iGrid);
					CqVector3D _aq_N;
					(N)->GetNormal(_aq_N,__iGrid);
					TqFloat _aq_roughness;
					(roughness)->GetFloat(_aq_roughness,__iGrid);
					CqColor colCl;
					Cl() ->GetColor( colCl, __iGrid );
					(Result)->SetColor(_aq_Result + colCl * pow( MAX( 0.0f, _aq_N * H ), 1.0f / ( _aq_roughness / 8.0f ) ),__iGrid);

				}
			}
			while( ( ++__iGrid < shadingPointCount() ) && __fVarying);
			PopState();
			// SO_advance_illuminance returns TRUE if there are any more non ambient lightsources.
		}
		while ( SO_advance_illuminance() );
	}
	pShader->DeleteTemporaryStorage( __nonspecular );
	pShader->DeleteTemporaryStorage( pDefAngle );
}


//----------------------------------------------------------------------
// phong(N,V,size)
void CqShaderExecEnv::SO_phong( IqShaderData* N, IqShaderData* V, IqShaderData* size, IqShaderData* Result, IqShader* pShader )
{
	bool __fVarying;
	TqUint __iGrid;

	IqShaderData * pnV = pShader ->CreateTemporaryStorage( type_vector, class_varying );
	IqShaderData* pnN = pShader ->CreateTemporaryStorage( type_normal, class_varying );
	IqShaderData* pR = pShader ->CreateTemporaryStorage( type_vector, class_varying );

	/// note: Not happy about this, the shader should take care of this at construction time,
	/// but at the moment, it can't guarantee the validity of the m_u/vGridRes data members.
	pnV->Initialise( shadingPointCount() );
	pnN->Initialise( shadingPointCount() );
	pR->Initialise( shadingPointCount() );

	SO_normalize( V, pnV );
	SO_normalize( N, pnN );

	__fVarying = true;
	__iGrid = 0;
	CqBitVector& RS = RunningState();
	do
	{
		if(!__fVarying || RS.Value( __iGrid ) )
		{
			CqVector3D vecnV;
			pnV->GetVector( vecnV, __iGrid );
			pnV->SetVector( -vecnV, __iGrid );
		}
	}
	while( ( ++__iGrid < shadingPointCount() ) && __fVarying);

	SO_reflect( pnV, pnN, pR );

	pShader->DeleteTemporaryStorage( pnV );
	pShader->DeleteTemporaryStorage( pnN );

	// If the illuminance cache is already OK, then we don't need to bother filling in the illuminance parameters.
	if ( !m_IlluminanceCacheValid )
	{
		ValidateIlluminanceCache( NULL, N, pShader );
	}

	IqShaderData* pDefAngle = pShader->CreateTemporaryStorage( type_float, class_uniform );
	if ( NULL == pDefAngle )
		return ;

	pDefAngle->SetFloat( PIO2 );

	// Initialise the return value
	Result->SetColor( gColBlack );

	// SO_init_illuminance returns TRUE if there are any non ambient ligthsources available.
	if ( SO_init_illuminance() )
	{
		do
		{
			// SO_illuminance sets the current state to whether the lightsource illuminates the points or not.
			SO_illuminance( NULL, NULL, N, pDefAngle, NULL );

			PushState();
			GetCurrentState();

			__iGrid = 0;
			CqBitVector& RS = RunningState();
			do
			{
				if(!__fVarying || RS.Value( __iGrid ) )
				{

					// Get the light vector and color from the light source.
					CqVector3D Ln;
					L() ->GetVector( Ln, __iGrid );
					Ln.Unit();

					// Now combine the color into the result.
					CqColor _aq_Result;
					(Result)->GetColor(_aq_Result,__iGrid);
					CqVector3D vecR;
					pR->GetVector( vecR, __iGrid );
					TqFloat _aq_size;
					(size)->GetFloat(_aq_size,__iGrid);
					CqColor colCl;
					Cl() ->GetColor( colCl, __iGrid );
					(Result)->SetColor(_aq_Result + colCl * pow( MAX( 0.0f, vecR * Ln ), _aq_size ),__iGrid);

				}
			}
			while( ( ++__iGrid < shadingPointCount() ) && __fVarying);

			PopState();
			// SO_advance_illuminance returns TRUE if there are any more non ambient lightsources.
		}
		while ( SO_advance_illuminance() );
	}
	pShader->DeleteTemporaryStorage( pDefAngle );
	pShader->DeleteTemporaryStorage( pR );
}


//----------------------------------------------------------------------
// trace(P,R)
void CqShaderExecEnv::SO_trace( IqShaderData* P, IqShaderData* R, IqShaderData* Result, IqShader* pShader )
{
	bool __fVarying;
	TqUint __iGrid;

	__fVarying=(P)->Class()==class_varying;
	__fVarying=(R)->Class()==class_varying||__fVarying;
	__fVarying=(Result)->Class()==class_varying||__fVarying;

	__iGrid = 0;
	CqBitVector& RS = RunningState();
	do
	{
		if(!__fVarying || RS.Value( __iGrid ) )
		{
			(Result)->SetColor(CqColor( 0, 0, 0 ),__iGrid);
		}
	}
	while( ( ++__iGrid < shadingPointCount() ) && __fVarying);
}


//----------------------------------------------------------------------
// illuminance(P,nsamples)
void CqShaderExecEnv::SO_illuminance( IqShaderData* Category, IqShaderData* P, IqShaderData* Axis, IqShaderData* Angle, IqShader* pShader )
{
	bool __fVarying;
	TqUint __iGrid;

	__iGrid = 0;
	CqString cat( "" );
	if ( NULL != Category )
		Category->GetString( cat );


	__fVarying = true;

	// Fill in the lightsource information, and transfer the results to the shader variables,
	if ( m_pAttributes != 0 )
	{
		IqLightsource * lp = m_pAttributes ->pLight( m_li );

		if ( NULL != Axis )
			__fVarying=(Axis)->Class()==class_varying||__fVarying;
		if ( NULL != Angle )
			__fVarying=(Angle)->Class()==class_varying||__fVarying;

		bool exec = true;

		if( cat.size() )
		{

			bool exclude = false;
			CqString lightcategories;
			CqString catname;


			if( cat.find( "-" ) == 0 )
			{
				exclude = true;
				catname = cat.substr( 1, cat.size() );
			}
			else
			{
				catname = cat;
			}

			IqShaderData* pcats = lp->pShader()->FindArgument("__category");
			if( pcats )
			{
				pcats->GetString( lightcategories );

				exec = false;
				// While no matching category has been found...
				CqString::size_type tokenpos = 0, tokenend;
				while( 1 )
				{
					tokenend = lightcategories.find(',', tokenpos);
					CqString token = lightcategories.substr( tokenpos, tokenend );
					if( catname.compare( token ) == 0 )
					{
						if( !exclude )
						{
							exec = true;
							break;
						}
					}
					if( tokenend == std::string::npos )
						break;
					else
						tokenpos = tokenend+1;
				}
			}
		}

		if( exec )
		{
			__iGrid = 0;
			CqBitVector& RS = RunningState();
			do
			{
				if(!__fVarying || RS.Value( __iGrid ) )
				{

					CqVector3D Ln;
					lp->L() ->GetVector( Ln, __iGrid );
					Ln = -Ln;

					// Store them locally on the surface.
					L() ->SetVector( Ln, __iGrid );
					CqColor colCl;
					lp->Cl() ->GetColor( colCl, __iGrid );
					Cl() ->SetColor( colCl, __iGrid );

					// Check if its within the cone.
					Ln.Unit();
					CqVector3D vecAxis( 0, 1, 0 );
					if ( NULL != Axis )
						Axis->GetVector( vecAxis, __iGrid );
					TqFloat fAngle = PI;
					if ( NULL != Angle )
						Angle->GetFloat( fAngle, __iGrid );

					TqFloat cosangle = Ln * vecAxis;
					cosangle = CLAMP( cosangle, -1, 1 );
					if ( acos( cosangle ) > fAngle )
						m_CurrentState.SetValue( __iGrid, false );
					else
						m_CurrentState.SetValue( __iGrid, true );
				}
			}
			while( ( ++__iGrid < shadingPointCount() ) && __fVarying);
		}
	}
}


void	CqShaderExecEnv::SO_illuminance( IqShaderData* Category, IqShaderData* P, IqShader* pShader )
{
	SO_illuminance( Category, P, NULL, NULL );
}


//----------------------------------------------------------------------
// illuminate(P)
void CqShaderExecEnv::SO_illuminate( IqShaderData* P, IqShaderData* Axis, IqShaderData* Angle, IqShader* pShader )
{
	bool __fVarying;
	TqUint __iGrid;

	bool res = true;
	if ( m_Illuminate > 0 )
		res = false;

	__fVarying = true;
	if ( res )
	{
		__iGrid = 0;
		CqBitVector& RS = RunningState();
		do
		{
			if(!__fVarying || RS.Value( __iGrid ) )
			{
				// Get the point being lit and set the ligth vector.
				CqVector3D _aq_P;
				(P)->GetPoint(_aq_P,__iGrid);
				CqVector3D vecPs;
				Ps() ->GetPoint( vecPs, __iGrid );
				L() ->SetVector( vecPs - _aq_P, __iGrid );

				// Check if its within the cone.
				CqVector3D Ln;
				L() ->GetVector( Ln, __iGrid );
				Ln.Unit();

				CqVector3D vecAxis( 0.0f, 1.0f, 0.0f );
				if ( NULL != Axis )
					Axis->GetVector( vecAxis, __iGrid );
				TqFloat fAngle = PI;
				if ( NULL != Angle )
					Angle->GetFloat( fAngle, __iGrid );
				TqFloat cosangle = Ln * vecAxis;
				cosangle = CLAMP( cosangle, -1, 1 );
				if ( acos( cosangle ) > fAngle )
				{
					// Make sure we set the light color to zero in the areas that won't be lit.
					Cl() ->SetColor( CqColor( 0, 0, 0 ), __iGrid );
					m_CurrentState.SetValue( __iGrid, false );
				}
				else
					m_CurrentState.SetValue( __iGrid, true );
			}
		}
		while( ( ++__iGrid < shadingPointCount() ) && __fVarying);
	}

	m_Illuminate++;
}


void	CqShaderExecEnv::SO_illuminate( IqShaderData* P, IqShader* pShader )
{
	SO_illuminate( P, NULL, NULL, pShader );
}


//----------------------------------------------------------------------
// solar()
void CqShaderExecEnv::SO_solar( IqShaderData* Axis, IqShaderData* Angle, IqShader* pShader )
{
	// TODO: Check light cone, and exclude points outside.
	bool __fVarying;
	TqUint __iGrid;

	bool res = true;
	if ( m_Illuminate > 0 )
		res = false;

	__fVarying = true;
	__iGrid = 0;
	CqBitVector& RS = RunningState();
	do
	{
		if(!__fVarying || RS.Value( __iGrid ) )
		{
			if ( res )
			{
				CqVector3D vecAxis;
				Ns()->GetNormal(vecAxis,__iGrid);
				vecAxis = -vecAxis;
				if ( NULL != Axis )
					Axis->GetVector( vecAxis, __iGrid );
				L() ->SetVector( vecAxis, __iGrid );
				m_CurrentState.SetValue( __iGrid, true );
			}
		}
	}
	while( ( ++__iGrid < shadingPointCount() ) && __fVarying);

	m_Illuminate++;
}


void	CqShaderExecEnv::SO_solar( IqShader* pShader )
{
	SO_solar( NULL, NULL, pShader );
}

//----------------------------------------------------------------------
// gather(category,P,N,angle,nsamples)
void CqShaderExecEnv::SO_gather( IqShaderData* category, IqShaderData* P, IqShaderData* N, IqShaderData* angle, IqShaderData* samples, IqShader* pShader, int cParams, IqShaderData** apParams)
{
	bool __fVarying;
	TqUint __iGrid;

	__iGrid = 0;
	__fVarying = true;
	CqBitVector& RS = RunningState();
	do
	{
		if(!__fVarying || RS.Value( __iGrid ) )
		{
			m_CurrentState.SetValue( __iGrid, false );
		}
		else
			m_CurrentState.SetValue( __iGrid, false );
	}
	while( ( ++__iGrid < shadingPointCount() ) && __fVarying);
}

//----------------------------------------------------------------------
// incident

void CqShaderExecEnv::SO_incident( IqShaderData* name, IqShaderData* pV, IqShaderData* Result, IqShader* pShader )
{
	TqUint __iGrid;

	__iGrid = 0;
	Result->SetValue( 0.0f, 0 );

}


//----------------------------------------------------------------------
// opposite

void CqShaderExecEnv::SO_opposite( IqShaderData* name, IqShaderData* pV, IqShaderData* Result, IqShader* pShader )
{
	TqUint __iGrid;

	__iGrid = 0;
	Result->SetValue( 0.0f, 0 );

}

//----------------------------------------------------------------------
// specularbrdf(L,N,V,rough)
void CqShaderExecEnv::SO_specularbrdf( IqShaderData* L, IqShaderData* N, IqShaderData* V, IqShaderData* rough, IqShaderData* Result, IqShader* pShader )
{
	bool __fVarying;
	TqUint __iGrid;

	__fVarying=(L)->Class()==class_varying;
	__fVarying=(N)->Class()==class_varying||__fVarying;
	__fVarying=(V)->Class()==class_varying||__fVarying;
	__fVarying=(rough)->Class()==class_varying||__fVarying;
	__fVarying=(Result)->Class()==class_varying||__fVarying;

	__iGrid = 0;
	CqBitVector& RS = RunningState();
	do
	{
		if(!__fVarying || RS.Value( __iGrid ) )
		{
			CqVector3D _aq_L;
			(L)->GetVector(_aq_L,__iGrid);
			CqVector3D _aq_V;
			(V)->GetVector(_aq_V,__iGrid);
			_aq_L.Unit();

			CqVector3D	H = _aq_L + _aq_V;
			H.Unit();
			/// \note The (roughness/8) term emulates the BMRT behaviour for prmanspecular.
			CqVector3D _aq_N;
			(N)->GetNormal(_aq_N,__iGrid);
			TqFloat _aq_rough;
			(rough)->GetFloat(_aq_rough,__iGrid);
			CqColor colCl;
			Cl() ->GetColor( colCl, __iGrid );
			(Result)->SetColor(colCl * pow( MAX( 0.0f, _aq_N * H ), 1.0f / ( _aq_rough / 8.0f ) ),__iGrid);
		}
	}
	while( ( ++__iGrid < shadingPointCount() ) && __fVarying);
}

//----------------------------------------------------------------------
// calculatenormal(P)
void CqShaderExecEnv::SO_calculatenormal( IqShaderData* p, IqShaderData* Result, IqShader* pShader )
{
	CqVector3D Defvec( 0.0f, 0.0f, 0.0f );
	bool __fVarying;
	TqUint __iGrid;

	// Find out if the orientation is inverted.
	bool CSO = pTransform()->GetHandedness(getRenderContext()->Time());
	bool O = false;
	if( pAttributes() )
		O = pAttributes() ->GetIntegerAttribute( "System", "Orientation" ) [ 0 ] != 0;
	TqFloat neg = 1;
	if ( !( (O && CSO) || (!O && !CSO) ) )
		neg = -1;

	__fVarying=(p)->Class()==class_varying;
	__fVarying=(Result)->Class()==class_varying||__fVarying;

	__iGrid = 0;
	CqBitVector& RS = RunningState();
	do
	{
		if(!__fVarying || RS.Value( __iGrid ) )
		{
			//CqVector3D	dPdu = SO_DuType<CqVector3D>( p, __iGrid, this, Defvec );
			//CqVector3D	dPdv = SO_DvType<CqVector3D>( p, __iGrid, this, Defvec );
			//CqVector3D	N = dPdu % dPdv;

			CqVector3D Ret, Ret2;
			TqInt uRes = uGridRes();
			TqInt GridX = __iGrid % ( uRes + 1 );

			CqVector3D v1, v2;
			if ( GridX < uRes )
			{
				p->GetValue( v1, __iGrid + 1 );
				p->GetValue( v2, __iGrid );
				Ret = ( v1 - v2 );
			}
			else
			{
				p->GetValue( v1, __iGrid );
				p->GetValue( v2, __iGrid - 1 );
				Ret = ( v1 - v2 );
			}
			TqInt vRes = vGridRes();
			TqInt GridY = ( __iGrid / ( uRes + 1 ) );

			if ( GridY < vRes )
			{
				p->GetValue( v1, __iGrid + uRes + 1 );
				p->GetValue( v2, __iGrid );
				Ret2 = ( v1 - v2 );
			}
			else
			{
				p->GetValue( v1, __iGrid );
				p->GetValue( v2, __iGrid - ( uRes + 1 ) );
				Ret2 = ( v1 - v2 );
			}

			CqVector3D N = Ret % Ret2;
			N.Unit();
			N *= neg;
			(Result)->SetNormal(N,__iGrid);
		}
	}
	while( ( ++__iGrid < shadingPointCount() ) && __fVarying);
}


//----------------------------------------------------------------------
// occlusion(P,N,samples)
void CqShaderExecEnv::SO_occlusion_rt( IqShaderData* P, IqShaderData* N, IqShaderData* samples, IqShaderData* Result, IqShader* pShader, int cParams, IqShaderData** apParams )
{
	bool __fVarying;
	TqUint __iGrid;

	if ( !getRenderContext() )
		return ;

	__fVarying = true;
	__iGrid = 0;
	CqBitVector& RS = RunningState();
	do
	{
		if(!__fVarying || RS.Value( __iGrid ) )
		{
			(Result)->SetFloat(0.0f,__iGrid);	// Default, completely lit
		}
	}
	while( ( ++__iGrid < shadingPointCount() ) && __fVarying);
}

//----------------------------------------------------------------------
// rayinfo
//

void CqShaderExecEnv::SO_rayinfo( IqShaderData* dataname, IqShaderData* pV, IqShaderData* Result, IqShader* pShader )
{
	TqUint __iGrid;

	if ( !getRenderContext() )
		return ;

	TqFloat Ret = 0.0f;

	__iGrid = 0;

	(Result)->SetFloat(Ret,__iGrid);
}

END_NAMESPACE(    Aqsis )
//---------------------------------------------------------------------
