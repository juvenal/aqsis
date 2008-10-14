// hairgen procedural
// Copyright (C) 2008 Christopher J. Foster [chris42f (at) gmail (d0t) com]
//
// This software is licensed under the GPLv2 - see the file COPYING for details.

#include "emitter.h"

//------------------------------------------------------------------------------
// EmitterMesh Implementation

EmitterMesh::EmitterMesh(
		const Aqsis::TqRiIntArray& nverts, const Aqsis::TqRiIntArray& verts,
		boost::shared_ptr<PrimVars> primVars, int totParticles)
	: m_faces(),
	m_P(),
	m_primVars(primVars),
	m_totParticles(totParticles),
	m_lowDiscrep(2)
{
	// initialize vertex positions
	const Aqsis::TqRiFloatArray* P = primVars->findPtr(
			Aqsis::CqPrimvarToken(Aqsis::class_vertex, Aqsis::type_point, 1, "P") );
	if(!P)
		throw std::runtime_error("\"vertex point[1] P\" must be present"
				"in parameter list for mesh");
	m_P.reserve(P->size()/3);
	for(int j = 0, endP = P->size(); j+2 < endP; j += 3)
		m_P.push_back(Vec3((*P)[j], (*P)[j+1], (*P)[j+2]));

	// Finally, create the list of faces
	// TODO: Ugly, since it uses m_P internally!
	createFaceList(nverts, verts, m_faces);
}

int EmitterMesh::numFaces() const
{
	return m_faces.size();
}

boost::shared_ptr<PrimVars> EmitterMesh::particlesOnFace(int faceIdx)
{
	const MeshFace& face = m_faces[faceIdx];

	boost::shared_ptr<PrimVars> interpVars(new PrimVars());

	float numParticlesCts = face.weight*m_totParticles;
	int numParticles = Aqsis::lfloor(face.weight*m_totParticles);
	if(numParticlesCts - numParticles > uRand())
		++numParticles;
	if(numParticles == 0)
		return boost::shared_ptr<PrimVars>();
	std::vector<int> storageCounts;
	// Create storage for all interpolated output parameters.
	for(PrimVars::const_iterator i = m_primVars->begin(), end = m_primVars->end();
			i != end; ++i)
	{
		if(!(i->token.Class() == Aqsis::class_constant
				|| i->token.Class() == Aqsis::class_uniform))
		{
			storageCounts.push_back(i->token.storageCount());
			// Construct new uniform token for output curves
			interpVars->append(Aqsis::CqPrimvarToken(Aqsis::class_uniform,
				i->token.type(), i->token.arraySize(), i->token.name() + "_emit"));
			// Allocate storage
			interpVars->back().value->assign(numParticles*storageCounts.back(), 0);
		}
		else
		{
			// TODO: constant and uniform class variables.
			storageCounts.push_back(0);
		}
	}

	// Float offsets for randomized quasi Monte-Carlo distribution
	float uOffset = float(std::rand())/RAND_MAX;
	float vOffset = float(std::rand())/RAND_MAX;
	// loop over child particles
	for(int particleNum = 0; particleNum < numParticles; ++particleNum)
	{
		// get random weights for the vertices of the current face.
		float u = uOffset + m_lowDiscrep.Generate(0, particleNum);
		if(u > 1)
			u -= 1;
		float v = vOffset + m_lowDiscrep.Generate(1, particleNum);
		if(v > 1)
			v -= 1;
		float weights[4];
		if(face.numVerts == 3)
		{
			if(u + v > 1)
			{
				u = 1-u;
				v = 1-v;
			}
			weights[0] = 1 - u - v;
			weights[1] = u;
			weights[2] = v;
		}
		else
		{
			weights[0] = (1-u)*(1-v);
			weights[1] = (1-u)*v;
			weights[2] = u*v;
			weights[3] = u*(1-v);
		}

		// loop over primitive variables.  Each varying/vertex/facevarying
		// /facevertex primvar is interpolated from the parent mesh to the
		// current child particle.
		int storageIndex = 0;
		PrimVars::iterator destVar = interpVars->begin();
		for(PrimVars::const_iterator srcVar = m_primVars->begin(),
				end = m_primVars->end(); srcVar != end;
				++srcVar, ++storageIndex, ++destVar)
		{
			bool useVertIndex = true;
			switch(srcVar->token.Class())
			{
				case Aqsis::class_varying:
				case Aqsis::class_vertex:
					useVertIndex = true;
					break;
				case Aqsis::class_facevarying:
				case Aqsis::class_facevertex:
					useVertIndex = false;
					break;
				default:
					continue;
			}
			int storageStride = storageCounts[storageIndex];
			// Get pointers to source parameters for the vertices
			const float* src[4] = {0,0,0,0};
			if(useVertIndex)
			{
				for(int i = 0; i < face.numVerts; ++i)
					src[i] = &(*srcVar->value)[storageStride*face.v[i]];
			}
			else
			{
				for(int i = 0; i < face.numVerts; ++i)
					src[i] = &(*srcVar->value)[ storageStride*(face.faceVaryingIndex+i) ];
			}

			// Interpolate the primvar pointed to by srcVar to the current
			// particle position.  This is just a a weighted average of values
			// attached to vertices of the current face.
			float* dest = &(*destVar->value)[storageStride*particleNum];
			for(int k = 0; k < storageStride; ++k, ++dest)
			{
				*dest = 0;
				for(int i = 0; i < face.numVerts; ++i)
				{
					*dest += *src[i] * weights[i];
					++src[i];
				}
			}
		}
	}

	// Finally, add face-constant parameters.
	Vec3 Ng_emitVec = faceNormal(face);
	float Ng_emit[] = {Ng_emitVec.x(), Ng_emitVec.y(), Ng_emitVec.z()};
	interpVars->append(Aqsis::CqPrimvarToken(Aqsis::class_constant, Aqsis::type_normal,
				1, "Ng_emit"), Aqsis::TqRiFloatArray(Ng_emit, Ng_emit+3));

	return interpVars;
}

/// Get the area for a triangle
float EmitterMesh::triangleArea(const int* v) const
{
	Vec3 edge1 = m_P[v[0]] - m_P[v[1]];
	Vec3 edge2 = m_P[v[1]] - m_P[v[2]];

	return (edge1 % edge2).Magnitude()/2;
}

/// Get the area for a face
float EmitterMesh::faceArea(const MeshFace& face) const
{
	float area = 0;
	for(int i = 3; i <= face.numVerts; ++i)
		area += triangleArea(&face.v[0] + i-3);
	return area;
}

/// Get the normal for a face
Vec3 EmitterMesh::faceNormal(const MeshFace& face) const
{
	return ((m_P[face.v[1]] - m_P[face.v[0]]) %
		(m_P[face.v[2]] - m_P[face.v[1]])).Unit();
}

/** Initialise the list of faces for the mesh
 *
 * \param nverts - number of vertices per face
 * \param verts - concatenated array of vertex indices into the primvar arrays.
 * \param faces - newly initialised faces go here.
 */
void EmitterMesh::createFaceList(const Aqsis::TqRiIntArray& nverts,
		const Aqsis::TqRiIntArray& verts,
		FaceVec& faces) const
{
	// Create face list
	int faceStart = 0;
	float totWeight = 0;
	int sizeNVerts = nverts.size();
	int totVerts = 0;
	faces.reserve(sizeNVerts);
	for(int i = 0; i < sizeNVerts; ++i)
	{
		if(nverts[i] != 3 && nverts[i] != 4)
		{
			assert(0 && "emitter mesh can only deal with 3 and 4-sided faces");
			continue;
		}
		faces.push_back(MeshFace(&verts[0]+faceStart, totVerts, nverts[i]));
		faceStart += nverts[i];
		// Get weight for face
		float w = faceArea(faces.back());
		faces.back().weight = w;
		totWeight += w;
		totVerts += nverts[i];
	}
	// normalized areas so that total area = 1.
	float scale = 1/totWeight;
	for(int i = 0; i < sizeNVerts; ++i)
		faces[i].weight *= scale;
}

