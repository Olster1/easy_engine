typedef struct {
	V3 point;  //point b - point a
	V3 pointA; //original point on A
	V3 pointB; //original point on B

	int indexA; //used a unique id
	int indexB; //used a unique id

	float u; //baycentric contribution
} EasySimplexVertex;

typedef struct {
	EasySimplexVertex points[3];
	int count;
	float divisor;//used to normalize the bayecentric value in get witness points
} EasySimplex;

typedef struct {
	Matrix4 T; //transform for polygon a
	V3 p[16]; //polygon a
	V3 pT[16]; //transformed points
	int count; //count of a
} EasyCollisionPolygon;

typedef struct {
	EasyCollisionPolygon a;
	EasyCollisionPolygon b;
} EasyCollisionInput;

typedef struct {
	V3 pointA; //closest point on polygon a
	V3 pointB; //closest point on polygon a
	float distance;
} EasyCollisionOutput;


static inline int easyCollision_supportFunction(EasyCollisionPolygon *polygon, V3 direction) { 
	//polygon that is transformed

	float maxDistance = dotV3(polygon->pT[i], direction);
	int index = 0;

	for(int i = 1; i < input->Count; i++) {
		float dist = dotV3(polygon->pT[i], direction);		
		if(dist < maxDistance) {
			maxDistance = dist;
			index = i;
			break;
		}
	}

	return index;

}

static inline void easyCollision_solve2(EasySimplex *simplex) {
	assert(simplex.count == 2);
	V3 BA = v3_minus(simplex->points[1].point, simplex->points[0].point);
	V3 QA = v3_minus(v3(0, 0, 0), simplex->points[0].point);

	float v = dotV3(BA, QA);

	V3 AB = v3_minus(simplex->points[0].point, simplex->points[1].point);
	V3 QB = v3_minus(v3(0, 0, 0), simplex->points[1].point);

	float u = dotV3(AB, QB);

	if(v <= 0.0f) {
		//vertex A
		simplex->points[0].u = 1.0f; //baycentric contribution
		simplex->divisor = 1;;
		simplex.count = 1;
	} else if(v >= 1.0f) {
		//vertex B
		simplex->points[0] = simplex->points[1];
		simplex->points[0].u = 1.0f; //baycentric contribution
		simplex->divisor = 1;;
		simplex.count = 1;
	} else {
		float divisor = v3_dot(BA, BA);
		simplex->points[0].u = u; //baycentric contribution
		simplex->points[1].u = v;
		simplex->divisor = divisor;
		simplex.count = 2;
	}

}
static inline float easyCollision_getBaycentric(V3 point2, V3 point1) {
	V3 BA = v3_minus(point2, point1);
	V3 QA = v3_minus(v3(0, 0, 0), point1);

	float v = dotV3(BA, QA);
	return v;
}



static inline void easyCollision_solve3(EasySimplex *simplex) {
	assert(simplex.count == 3);
	
	V3 A = simplex->points[0].point;
	V3 B = simplex->points[1].point;
	V3 C = simplex->points[2].point;

	float ABu = easyCollision_getBaycentric(simplex->points[0].point, simplex->points[1].point);
	float ABv = easyCollision_getBaycentric(simplex->points[1].point, simplex->points[0].point);

	float BCu = easyCollision_getBaycentric(simplex->points[1].point, simplex->points[2].point);
	float BCv = easyCollision_getBaycentric(simplex->points[2].point, simplex->points[1].point);

	float ACu = easyCollision_getBaycentric(simplex->points[2].point, simplex->points[0].point);
	float ACv = easyCollision_getBaycentric(simplex->points[0].point, simplex->points[2].point);

	// Vec2 A = m_vertexA.point;
	// Vec2 B = m_vertexB.point;
	// Vec2 C = m_vertexC.point;

	// // Compute edge barycentric coordinates (pre-division).
	// float uAB = Dot(Q - B, A - B);
	// float vAB = Dot(Q - A, B - A);

	// float uBC = Dot(Q - C, B - C);
	// float vBC = Dot(Q - B, C - B);

	// float uCA = Dot(Q - A, C - A);
	// float vCA = Dot(Q - C, A - C);

	// Region A
	if (ABv <= 0.0f && ACu <= 0.0f)
	{
		simplex->points[0].u = 1.0f;
		simplex->divisor = 1.0f;
		simplex->count = 1;
		return;
	}

	// Region B
	if (ABu <= 0.0f && BCv <= 0.0f)
	{
		simplex->points[0] = simplex->points[1];
		simplex->points[0].u = 1.0f;
		simplex->divisor = 1.0f;
		simplex->count = 1;
		return;
	}
	
	// Region C
	if (BCu <= 0.0f && ACv <= 0.0f)
	{
		simplex->points[0] = simplex->points[2];
		simplex->points[0].u = 1.0f;
		simplex->divisor = 1.0f;
		simplex->count = 1;
		return;
	}

	// Compute signed triangle area.
	float area = getLengthV3(v3_crossProduct(v3_minus(B, A), v3_minus(C, A)));

	// Compute triangle barycentric coordinates (pre-division).
	V3 Q = v3(0, 0, 0);
	float uABC = getLengthV3(v3_crossProduct(v3_minus(B, Q), v3_minus(C, Q))); 
	float vABC = getLengthV3(v3_crossProduct(v3_minus(C,  Q), v3_minus(A, Q)));
	float wABC = getLengthV3(v3_crossProduct(v3_minus(A, Q), v3_minus(B, Q)));

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
		m_vertexB = m_vertexA;
		m_vertexA = m_vertexC;

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

static inline V3 easyCollision_getSearchDirection(EasySimplex *simplex) {
	V3 result = v3(0, 0);
	switch(simplex->count) {
		case 1: {
			result = v3_minus(v3(0, 0, 0), simplex->points[0].point);
		} break;
		case 2: {
			V3 A = simplex->points[0].point;
			V3 B = simplex->points[1].point;
			V3 BA = v3_minus(B, A);
			V3 QA = v3_minus(v3(0, 0, 0), A);
			
			float s = 1.0f / simplex->divisor;
			V3 p = (s * simplex->points[0].u) * simplex->points[0].point + (s * simplex->points[1].u) * simplex->points[1].point;
			result = v3_minus(v3(0, 0, 0), p);
		} break;
		case 3: {
			assert(false);
			result = v3(0, 0, 0);
		} break;
	}
	return result;
}

static inline void easyCollision_getWitnessPoints(EasySimplex *simplex, V3 *a, V3 *b) {

		float s = 1.0f / simplex->divisor;

		switch (m_count)
		{
		case 1:
			*point1 = simplex->points[0].pointA;
			*point2 = simplex->points[0].pointB;
			break;

		case 2:
			{
				*point1 = (s * simplex->points[0].u) * simplex->points[0].pointA + (s * simplex->points[1].u) * simplex->points[1].pointA;
				*point2 = (s * simplex->points[0].u) * simplex->points[0].pointB + (s * simplex->points[1].u) * simplex->points[1].pointB;
				// we use the divisor here since we only need the actual value (0 - 1) once we are mapping back to the original polygons 
				// this is our uA + vB equation -> (u being the percentage A - B), (v being the percentage B - A)
			}
			break;

		case 3:
			{
				*point1 = (s * simplex->points[0].u) * simplex->points[0].pointA + (s * simplex->points[1].u) * simplex->points[1].pointA + (s * simplex->points[2].u) * simplex->points[2].pointA;
				*point2 = *point1;
			}
			break;

		default:
			assert(false);
			break;
		}
	}
}

static inline void easyCollision_GetClosestPoint(EasyCollisionInput *input, EasyCollisionOutput *output) {
	//transform points to world space;
	EasyCollisionPolygon *a = input->a;
	for(int i = 0; i < a->count; ++i) {
		a->pT[i] = transformPositionV3(a->p[i], a->T)
	}
	EasyCollisionPolygon *b = input->b;
	for(int i = 0; i < b->count; ++i) {
		b->pT[i] = transformPositionV3(b->p[i], b->T)
	}
	//

	EasySimplex simplex = {};

	EasySimplexVertex *vertex = &simplex.points[simplex.count++];
	vertex->point = v3_minus(b->pT[0], a->pT[0]);
	vertex->pointA = a->p[0];
	vertex->pointB = b->p[0];
	vertex->indexA = 0;
	vertex->indexB = 0;

	vertex->u = 1;
	simplex.divisor = 1;

	int saveA[3] = {};
	int saveB[3] = {};

	bool found = false;
	// float lstDirection 
	for(int itIndex = 0; itIndex < 20 && !found; itIndex++) {
		int saveCount = simplex.count;
		//store the saved
		for(int i = 0; i < saveCount; ++i) {
			saveA[i] = simplex->points[i].indexA;
			saveB[i] = simplex->points[i].indexB;
		}

		//drop non-contributing verticies
		switch(simplex.count) {
			case 1: {
				//nothing happens
			} break;
			case 2: {
				easyCollision_solve2(&simplex);
			} case 3: {
				easyCollision_solve3(&simplex);
			} break;

		}

		if(simplex.count == 3) {
			// still a triangle, so point is inside triangle
			found = true;
			continue;
		}

		direction = easyCollision_getSearchDirection(&simplex);

		if(v3_dot(direction) == 0.0f) {
			//overlaps vertex
			found = true;
			continue;
		}

		if(!found) {

			int aIndex = easyCollision_supportFunction(a, -direction);
			int bIndex = easyCollision_supportFunction(b, direction);


			EasySimplexVertex *vertex = &simplex.points[simplex.count];
			vertex->point = v3_minus(b->pT[bIndex], a->pT[aIndex]);
			vertex->pointA = a->p[aIndex];
			vertex->pointB = b->p[bIndex];
			vertex->indexA = aIndex;
			vertex->indexB = bIndex;
			vertex->u = 1; //actually set in the redunancy check
			
			//main termination case
			bool duplicate = false;
			for(int i = 0; i < saveCount; ++i) {
				if(saveA[i] == aIndex && saveB[i] == bIndex) {
					found = true;
					break;
				}
			}
			if(!found) {
				simplex.count++
			}
		}
	}
	easyCollision_getWitnessPoints(&simplex, &output->point1, &output->point2);
	output->distance = Distance(output->point1, output->point2);
}











