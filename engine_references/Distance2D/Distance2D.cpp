/*
* Copyright (c) 2010 Erin Catto http://wwwABC.gphysics.com
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

#include "Distance2D.h"

int Polygon::GetSupport(const Vec2& d) const
{
	int bestIndex = 0;
	float bestValue = Dot(m_points[0], d);
	for (int i = 1; i < m_count; ++i)
	{
		float value = Dot(m_points[i], d);
		if (value > bestValue)
		{
			bestIndex = i;
			bestValue = value;
		}
	}

	return bestIndex;
}

Vec2 Simplex::GetSearchDirection() const
{
	switch (m_count)
	{
	case 1:
		return -m_vertexA.point;

	case 2:
		{
			Vec2 edgeAB = m_vertexB.point - m_vertexA.point;
			float sgn = Cross(edgeAB, -m_vertexA.point);
			if (sgn > 0.0f)
			{
				// Origin is left of edgeAB.
				return Cross(1.0f, edgeAB); //perp to the left
			}
			else
			{
				// Origin is right of edgeAB.
				return Cross(edgeAB, 1.0f);
			}
		}

	default:
		assert(false);
		return Vec2(0.0f, 0.0f);
	}
}

Vec2 Simplex::GetClosestPoint() const
{
	switch (m_count)
	{
	case 1:
		return m_vertexA.point;

	case 2:
		{
			float s = 1.0f /  m_divisor;
			return (s * m_vertexA.u) * m_vertexA.point + (s * m_vertexB.u) * m_vertexB.point;
		}

	case 3:
		return Vec2(0.0f, 0.0f);

	default:
		assert(false);
		return Vec2(0.0f, 0.0f);
	}
}

void Simplex::GetWitnessPoints(Vec2* point1, Vec2* point2) const
{
	float factor = 1.0f / m_divisor;

	switch (m_count)
	{
	case 1:
		*point1 = m_vertexA.point1;
		*point2 = m_vertexA.point2;
		break;

	case 2:
		{
			// we use the divisor here since we only need the actual value (0 - 1) once we are mapping back to the original polygons 
			float s = 1.0f / m_divisor;
			// this is our uA + vB equation -> (u being the percentage A - B), (v being the percentage B - A)
			*point1 = (s * m_vertexA.u) * m_vertexA.point1 + (s * m_vertexB.u) * m_vertexB.point1;
			*point2 = (s * m_vertexA.u) * m_vertexA.point2 + (s * m_vertexB.u) * m_vertexB.point2;
		}
		break;

	case 3:
		{
			float s = 1.0f / m_divisor;
			*point1 = (s * m_vertexA.u) * m_vertexA.point1 + (s * m_vertexB.u) * m_vertexB.point1 + (s * m_vertexC.u) * m_vertexC.point1;
			*point2 = *point1;
		}
		break;

	default:
		assert(false);
		break;
	}
}


// Closest point on line segment to Q.
// Voronoi regions: A, B, AB
void Simplex::Solve2(const Vec2& Q)
{
	Vec2 A = m_vertexA.point;
	Vec2 B = m_vertexB.point;

	// Compute barycentric coordinates (pre-division).

	//use u & v for a percentage value
	float u = Dot(Q - B, A - B);
	float v = Dot(Q - A, B - A);

	// Region A
	if (v <= 0.0f)
	{
		// Simplex is reduced to just vertex A.
		m_vertexA.u = 1.0f;
		//since the closest point is the vertex, we set the u value to 1, so we have done the divide
		m_divisor = 1.0f;
		m_count = 1;
		return;
	}

	// Region B
	if (u <= 0.0f)
	{
		// Simplex is reduced to just vertex B.
		// We move vertex B into vertex A and reduce the count.
		m_vertexA = m_vertexB;
		m_vertexA.u = 1.0f;
		m_divisor = 1.0f;
		m_count = 1;
		return;
	}

	// Region AB. Due to the conditions above, we are
	// guaranteed the the edge has non-zero length and division
	// is safe.
	m_vertexA.u = u;
	m_vertexB.u = v;
	Vec2 e = B - A;
	//t
	m_divisor = Dot(e, e);  //length*length -> one used to normalize (b - a), and the other to divide by the length to get value between 0 - 1
	m_count = 2;
}

// Closest point on triangle to Q.
// Voronoi regions: A, B, C, AB, BC, CA, ABC
void Simplex::Solve3(const Vec2& Q)
{
	Vec2 A = m_vertexA.point;
	Vec2 B = m_vertexB.point;
	Vec2 C = m_vertexC.point;

	// Compute edge barycentric coordinates (pre-division).
	float uAB = Dot(Q - B, A - B);
	float vAB = Dot(Q - A, B - A);

	float uBC = Dot(Q - C, B - C);
	float vBC = Dot(Q - B, C - B);

	float uCA = Dot(Q - A, C - A);
	float vCA = Dot(Q - C, A - C);

	// Region A
	if (vAB <= 0.0f && uCA <= 0.0f)
	{
		m_vertexA.u = 1.0f;
		m_divisor = 1.0f;
		m_count = 1;
		return;
	}

	// Region B
	if (uAB <= 0.0f && vBC <= 0.0f)
	{
		m_vertexA = m_vertexB;
		m_vertexA.u = 1.0f;
		m_divisor = 1.0f;
		m_count = 1;
		return;
	}
	
	// Region C
	if (uBC <= 0.0f && vCA <= 0.0f)
	{
		m_vertexA = m_vertexC;
		m_vertexA.u = 1.0f;
		m_divisor = 1.0f;
		m_count = 1;
		return;
	}
	
	// Compute signed triangle area.
	float area = Cross(B - A, C - A);

	// Compute triangle barycentric coordinates (pre-division).
	float uABC = Cross(B - Q, C - Q);
	float vABC = Cross(C - Q, A - Q);
	float wABC = Cross(A - Q, B - Q);

	// Region AB
	if (uAB > 0.0f && vAB > 0.0f && wABC * area <= 0.0f)
	{
		m_vertexA.u = uAB;
		m_vertexB.u = vAB;
		Vec2 e = B - A;
		m_divisor = Dot(e, e);
		m_count = 2;
		return;
	}

	// Region BC
	if (uBC > 0.0f && vBC > 0.0f && uABC * area <= 0.0f)
	{
		m_vertexA = m_vertexB;
		m_vertexB = m_vertexC;

		m_vertexA.u = uBC;
		m_vertexB.u = vBC;
		Vec2 e = C - B;
		m_divisor = Dot(e, e);
		m_count = 2;
		return;
	}

	// Region CA
	if (uCA > 0.0f && vCA > 0.0f && vABC * area <= 0.0f)
	{
		m_vertexA = m_vertexC;
		m_vertexB = m_vertexA;

		m_vertexA.u = uCA;
		m_vertexB.u = vCA;
		Vec2 e = A - C;
		m_divisor = Dot(e, e);
		m_count = 2;
		return;
	}

	// Region ABC
	// The triangle area is guaranteed to be non-zero.
	assert(uABC > 0.0f && vABC > 0.0f && wABC > 0.0f);
	m_vertexA.u = uABC;
	m_vertexB.u = vABC;
	m_vertexC.u = wABC;
	m_divisor = area;
	m_count = 3;
}

// Compute the distance between two polygons using the GJK algorithm.
void Distance2D(Output* output, const Input& input)
{
	const Polygon* polygon1 = &input.polygon1;
	const Polygon* polygon2 = &input.polygon2;

	Transform transform1 = input.transform1;
	Transform transform2 = input.transform2;

	// Initialize the simplex.
	Simplex simplex;
	simplex.m_vertexA.index1 = 0;
	simplex.m_vertexA.index2 = 0;
	Vec2 localPoint1 = polygon1->m_points[0];
	Vec2 localPoint2 = polygon2->m_points[0];
	simplex.m_vertexA.point1 = Mul(transform1, localPoint1);
	simplex.m_vertexA.point2 = Mul(transform2, localPoint2);
	simplex.m_vertexA.point = simplex.m_vertexA.point2 - simplex.m_vertexA.point1;
	simplex.m_vertexA.u = 1.0f;
	simplex.m_vertexA.index1 = 0;
	simplex.m_vertexA.index2 = 0;
	simplex.m_count = 1;

	// Begin recording the simplices for visualization.
	output->simplexCount = 0;

	// Get simplex vertices as an array.
	SimplexVertex* vertices = &simplex.m_vertexA;

	// These store the vertices of the last simplex so that we
	// can check for duplicates and prevent cycling.
	int save1[3], save2[3];
	int saveCount = 0;

	// Main iteration loop.
	const int k_maxIters = 20;
	int iter = 0;
	while (iter < k_maxIters)
	{
		// Copy simplex so we can identify duplicates.
		saveCount = simplex.m_count;
		for (int i = 0; i < saveCount; ++i)
		{
			save1[i] = vertices[i].index1;
			save2[i] = vertices[i].index2;
		}

		// Determine the closest point on the simplex and
		// remove unused vertices.
		switch (simplex.m_count)
		{
		case 1:
			break;

		case 2:
			simplex.Solve2(Vec2(0.0f, 0.0f));
			break;

		case 3:
			simplex.Solve3(Vec2(0.0f, 0.0f));
			break;

		default:
			assert(false);
		}

		// Record for visualization.
		output->simplices[output->simplexCount++] = simplex;

		// If we have 3 points, then the origin is in the corresponding triangle.
		if (simplex.m_count == 3)
		{
			break;
		}

		// Get search direction.
		Vec2 d = simplex.GetSearchDirection();

		// Ensure the search direction non-zero.
		//this is if the vertex overlaps
		if (Dot(d, d) == 0.0f)
		{
			break;
		}

		// Compute a tentative new simplex vertex using support points.
		SimplexVertex* vertex = vertices + simplex.m_count;
		vertex->index1 = polygon1->GetSupport(MulT(transform1.R, -d));
		vertex->point1 = Mul(transform1, polygon1->m_points[vertex->index1]);
		vertex->index2 = polygon2->GetSupport(MulT(transform2.R, d));
		vertex->point2 = Mul(transform2, polygon2->m_points[vertex->index2]);
		vertex->point = vertex->point2 - vertex->point1;

		// Iteration count is equated to the number of support point calls.
		++iter;

		// Check for duplicate support points. This is the main termination criteria.
		bool duplicate = false;
		for (int i = 0; i < saveCount; ++i)
		{
			if (vertex->index1 == save1[i] && vertex->index2 == save2[i])
			{
				duplicate = true;
				break;
			}
		}

		// If we found a duplicate support point we must exit to avoid cycling.
		if (duplicate)
		{
			break;
		}

		// New vertex is ok and needed.
		++simplex.m_count;
	}

	// Prepare output.
	simplex.GetWitnessPoints(&output->point1, &output->point2);
	output->distance = Distance(output->point1, output->point2);
	output->iterations = iter;
}
