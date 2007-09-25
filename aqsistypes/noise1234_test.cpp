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

/** \file
 *
 * \brief Unit tests for CqMatrix
 * \author Tobias Sauerwein
 */

#include "noise1234.h"


#ifdef AQSIS_SYSTEM_MACOSX
    #define BOOST_AUTO_TEST_MAIN
	#include <boost/test/included/unit_test.hpp>
#else
	#include <boost/test/auto_unit_test.hpp>
#endif

#include <boost/test/floating_point_comparison.hpp>


const TqFloat epsilon = 0.1f; 



BOOST_AUTO_TEST_CASE(CqNoise1234_1D_float_Perlin_noise_test)
{
	Aqsis::CqNoise1234 noise;
	
	BOOST_CHECK_CLOSE(noise.noise(1.5f), 0.141000003f, epsilon);
}

BOOST_AUTO_TEST_CASE(CqNoise1234_2D_float_Perlin_noise_test)
{
	Aqsis::CqNoise1234 noise;
	
	BOOST_CHECK_CLOSE(noise.noise(1.5f, 0.3f), -0.54007268f, epsilon);
}

BOOST_AUTO_TEST_CASE(CqNoise1234_3D_float_Perlin_noise_test)
{
	Aqsis::CqNoise1234 noise;
	
	BOOST_CHECK_CLOSE(noise.noise(1.5f, 0.4f, 2.1f), -0.186702728f, epsilon);
}

BOOST_AUTO_TEST_CASE(CqNoise1234_4D_float_Perlin_noise_test)
{
	Aqsis::CqNoise1234 noise;
	
	BOOST_CHECK_CLOSE(noise.noise(1.5f, -0.2f, 0.7f, 3.0f), 0.163102284f, epsilon);
}

BOOST_AUTO_TEST_CASE(CqNoise1234_1D_float_Perlin_periodic_noise_test)
{
	Aqsis::CqNoise1234 noise;
	
	BOOST_CHECK_CLOSE(noise.pnoise(1.5f, 2), -0.328999996f, epsilon);
}

BOOST_AUTO_TEST_CASE(CqNoise1234_2D_float_Perlin_periodic_noise_test)
{
	Aqsis::CqNoise1234 noise;
	
	BOOST_CHECK_CLOSE(noise.pnoise(1.5f, 0.3f, 3, 1), -0.507000029f, epsilon);
}

BOOST_AUTO_TEST_CASE(CqNoise1234_3D_float_Perlin_periodic_noise_test)
{
	Aqsis::CqNoise1234 noise;
	
	BOOST_CHECK_CLOSE(noise.pnoise(1.5f, 0.4f, 2.5f, 2, 8, -1), -0.646274328f, epsilon);
}

BOOST_AUTO_TEST_CASE(CqNoise1234_4D_float_Perlin_periodic_noise_test)
{
	Aqsis::CqNoise1234 noise;
	
	BOOST_CHECK_CLOSE(noise.pnoise(1.5f, -0.2f, 0.7f, 3.0f, 2, 5, -2, 3), -0.369483441f, epsilon);
}