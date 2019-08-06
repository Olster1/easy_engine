/*
* Copyright (c) 2010 Erin Catto http://www.gphysics.com
*
* This software is provided 'as-is', without any express or implied
* warranty.  In no event will the authors be held liable for any damages
* arising from the use of this software.
* Permission is granted to anyone to use this software for any purpose,
* including commercial applications, and to alter it and redistribute it
* freely, subject to the following restrictions:
* 1. The origin of this software must not be misrepresented; you must not
* claim that you wrote the original software. If you use this software
* in a product, an acknowledgment in the product documentation would be
* appreciated but is not required.
* 2. Altered source versions must be plainly marked as such, and must not be
* misrepresented as being the original software.
* 3. This notice may not be removed or altered from any source distribution.
*/

#ifndef DISTANCE2D_H
#define DISTANCE2D_H

#include "MathUtils.h"
#include <climits>

/// Polygon used by the GJK algorithm.
struct Polygon
{
	/// Get the supporting point index in the given direction.
	int GetSupport(const Vec2& d) const;

	const Vec2* m_points;
	int m_count;
};

/// A simplex vertex.
struct SimplexVertex
{
	Vec2 point1;	// support point in polygon1
	Vec2 point2;	// support point in polygon2
	Vec2 point;		// point2 - point1
	float u;		// unnormalized barycentric coordinate for closest point
	int index1;		// point1 index
	int index2;		// point2 index
};

/// Simplex used by the GJK algorithm.
struct Simplex
{
	Vec2 GetSearchDirection() const;
	Vec2 GetClosestPoint() const;
	void GetWitnessPoints(Vec2* point1, Vec2* point2) const;
	void Solve2(const Vec2& P);
	void Solve3(const Vec2& P);

	SimplexVertex m_vertexA, m_vertexB, m_vertexC;
	float m_divisor;	// denominator to normalize barycentric coordinates
	int m_count;
};

/// Input for the distance function.
struct Input
{
	Polygon polygon1;
	Polygon polygon2;
	Transform transform1;
	Transform transform2;
};

/// Output for the distance function.
struct Output
{
	enum
	{
		e_maxSimplices = 20
	};

	Vec2 point1;		///< closest point on polygon 1
	Vec2 point2;		///< closest point on polygon 2
	float distance;
	int iterations;		///< number of GJK iterations used

	Simplex simplices[e_maxSimplices];
	int simplexCount;
};

/// Get the closest points between two point clouds.
void Distance2D(Output* output, const Input& input);

#endif
