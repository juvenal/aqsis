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
 * \brief Main Aqsis include for basic macros and types needed by all aqsis headers.
 *
 * \author Paul C. Gregory (pgregory@aqsis.org)
 *
 * This file should be included by every aqsis source file.  Moreover, it
 * should be included before any other headers, since aqsis_compiler.h defines
 * some constants which potentially modify the behaviour of other standard
 * includes.
 *
 * ===================================================================
 * C-compatible header. C++ constructs must be preprocessor-protected.
 * ===================================================================
 */

#ifndef AQSIS_H_INCLUDED
#define AQSIS_H_INCLUDED

#include <assert.h>

#include "aqsis_types.h"
#include "aqsis_compiler.h"

// macro which stringizes the value held in another macro.
#define AQSIS_XSTR(s) AQSIS_STR(s)
#define AQSIS_STR(s) #s

#endif // AQSIS_H_INCLUDED
