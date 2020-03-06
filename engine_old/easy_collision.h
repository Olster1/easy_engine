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
	bool wasInside;
	V3 normal;
} EasyCollisionOutput;


static inline int easyCollision_supportFunction(EasyCollisionPolygon *polygon, V3 direction) { 
	//polygon that is transformed

	float maxDistance = dotV2(polygon->pT[0].xy, direction.xy);
	int index = 0;

	for(int i = 1; i < polygon->count; i++) {
		float dist = dotV2(polygon->pT[i].xy, direction.xy);		
		if(dist > maxDistance) {
			maxDistance = dist;
			index = i;
		}
	}

	return index;

}

static inline void easyCollision_solve2(EasySimplex *simplex) {
	assert(simplex->count == 2);
	V2 A = simplex->points[0].point.xy;
	V2 B = simplex->points[1].point.xy;

	V2 BA = v2_minus(B, A);
	V2 QA = v2_minus(v2(0, 0), A);

	float v = dotV2(BA, QA);

	V2 AB = v2_minus(A, B);
	V2 QB = v2_minus(v2(0, 0), B);

	// error_printFloat2("A vector: ", AB.E);
	// error_printFloat2("B vector: ", BA.E);

	// error_printFloat2("Q vector: ", QB.E);
	// error_printFloat2("Q vector: ", QA.E);
	float d = dotV2(BA, BA);

	float u = dotV2(AB, QB);

	if(v <= 0.0f) {
		//vertex A
		// printf("%s\n", "region A");
		simplex->points[0].u = 1.0f; //baycentric contribution
		simplex->divisor = 1;
		simplex->count = 1;
	} else if(u <= 0.0f) {
		//vertex B
		// printf("%s\n", "region B");
		// printf("v value: %f\n", v/d);
		// printf("u value: %f\n", u/d);
		simplex->points[0] = simplex->points[1];
		simplex->points[0].u = 1.0f; //baycentric contribution
		simplex->divisor = 1;
		simplex->count = 1;
	} else {
		// printf("%s\n", "region A B");
		float divisor = dotV2(BA, BA);
		simplex->points[0].u = u; //baycentric contribution
		simplex->points[1].u = v;
		simplex->divisor = divisor;
		simplex->count = 2;
	}

}
static inline float easyCollision_getBaycentric(V3 point2, V3 point1) {
	V3 BA = v3_minus(point2, point1);
	V3 QA = v3_minus(v3(0, 0, 0), point1);

	float v = dotV3(BA, QA);
	return v;
}



static inline void easyCollision_solve3(EasySimplex *simplex) {
	assert(simplex->count == 3);
	
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
		// printf("a\n");
		simplex->points[0].u = 1.0f;
		simplex->divisor = 1.0f;
		simplex->count = 1;
		return;
	}

	// Region B
	if (ABu <= 0.0f && BCv <= 0.0f)
	{
		 // printf("b\n");
		simplex->points[0] = simplex->points[1];
		simplex->points[0].u = 1.0f;
		simplex->divisor = 1.0f;
		simplex->count = 1;
		return;
	}
	
	// Region C
	if (BCu <= 0.0f && ACv <= 0.0f)
	{
		// printf("c\n");
		simplex->points[0] = simplex->points[2];
		simplex->points[0].u = 1.0f;
		simplex->divisor = 1.0f;
		simplex->count = 1;
		return;
	}

	V2 A2 = A.xy;
	V2 B2 = B.xy;
	V2 C2 = C.xy;

	// Compute signed triangle area.
	//this can be negative, by letting it be signed it accounts for the winding of the simplex so we don't have to 
	//think about it. We can do this by multiplying by the area and just looking at the sign. if they are the same sign,
	//the point is inside the triangle, if they aren't then the signs won't match and the point isn't in the triangle
	float area = cross2D(v2_minus(B2, A2), v2_minus(C2, A2));

	//NOTE: Here we reduce the point to 2D by projecting it onto the triangle plane, by creating a view transform 
	//from the perespective of the triangle side

	// Compute triangle barycentric coordinates (pre-division).
	V2 Q = v2(0, 0);
	float uABC = cross2D(v2_minus(B2, Q), v2_minus(C2, Q)); 
	float vABC = cross2D(v2_minus(C2,  Q), v2_minus(A2, Q));
	float wABC = cross2D(v2_minus(A2, Q), v2_minus(B2, Q));

	// Region AB
	if (ABu > 0.0f && ABv > 0.0f && wABC * area <= 0.0f)
	{
		// printf("ab\n");
		simplex->points[0].u = ABu;
		simplex->points[1].u = ABv;
		V3 e = v3_minus(B, A);
		simplex->divisor = dotV3(e, e);
		simplex->count = 2;
		return;
	}

	// Region BC
	if (BCu > 0.0f && BCv > 0.0f && uABC * area <= 0.0f)
	{
		// printf("cb\n");
		simplex->points[0] = simplex->points[1];
		simplex->points[1] = simplex->points[2];

		simplex->points[0].u = BCu;
		simplex->points[1].u = BCv;
		V3 e = v3_minus(C, B);
		simplex->divisor = dotV3(e, e);
		simplex->count = 2;
		return;
	}

	// Region CA
	if (ACu > 0.0f && ACv > 0.0f && vABC * area <= 0.0f)
	{
		// printf("ac\n");
		simplex->points[1] = simplex->points[0];
		simplex->points[0] = simplex->points[2];
		

		simplex->points[0].u = ACu;
		simplex->points[1].u = ACv;
		V3 e = v3_minus(A, C);
		simplex->divisor = dotV3(e, e);
		simplex->count = 2;
		return;
	}

	// Region ABC
	// The triangle area is guaranteed to be non-zero.
	assert((uABC*area) > 0.0f && (vABC*area) > 0.0f && (wABC*area) > 0.0f);
	simplex->points[0].u = uABC;
	simplex->points[1].u = vABC;
	simplex->points[2].u = wABC;
	simplex->divisor = area;
	simplex->count = 3;
}

static inline V3 easyCollision_getSearchDirection(EasySimplex *simplex) {
	V3 result = v3(0, 0, 0);
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
			V3 p = v3_plus(v3_scale((s * simplex->points[0].u), simplex->points[0].point), v3_scale((s * simplex->points[1].u), simplex->points[1].point));
			result = v3_minus(v3(0, 0, 0), p);
			assert(result.z == 0);
		} break;
		case 3: {
			assert(false);
			result = v3(0, 0, 0);
		} break;
	}
	return result;
}

static inline void easyCollision_getWitnessPoints(EasySimplex *simplex, V3 *point1, V3 *point2) {

		float s = 1.0f / simplex->divisor;

		switch (simplex->count)
		{
		case 1: {
			*point1 = simplex->points[0].pointA;
			*point2 = simplex->points[0].pointB;
		}break; 
		case 2:
			{
				// printf("%s %d\n", "indexA", simplex->points[0].indexA);
				// printf("%s %d\n", "indexB", simplex->points[0].indexB);
				// printf("%s %d\n", "indexA", simplex->points[1].indexA);
				// printf("%s %d\n", "indexB", simplex->points[1].indexB);
				assert(!(simplex->points[0].indexA == simplex->points[1].indexA && simplex->points[0].indexB == simplex->points[1].indexB));
				// printf("%f %f\n", simplex->points[0].u, simplex->points[1].u);
				// error_printFloat3("point A", simplex->points[0].pointA.E);
				// error_printFloat3("point A", simplex->points[1].pointA.E);
				// error_printFloat3("point B", simplex->points[0].pointB.E);
				// error_printFloat3("point B", simplex->points[1].pointB.E);
				
				float u = s * simplex->points[0].u;
				float v = s * simplex->points[1].u;
				*point1 = v3_plus(v3_scale(u, simplex->points[0].pointA), v3_scale(v, simplex->points[1].pointA));
				*point2 = v3_plus(v3_scale(u, simplex->points[0].pointB), v3_scale(v, simplex->points[1].pointB));
				// we use the divisor here since we only need the actual value (0 - 1) once we are mapping back to the original polygons 
				// this is our uA + vB equation -> (u being the percentage A - B), (v being the percentage B - A)
			}
			break;

		case 3:
			{
				V3 a = v3_scale((s * simplex->points[0].u), simplex->points[0].pointA);
				V3 b = v3_scale((s * simplex->points[1].u), simplex->points[1].pointA);
				V3 c = v3_scale((s * simplex->points[2].u), simplex->points[2].pointA);
				*point1 =  v3_plus(v3_plus(a, b), c);
				*point2 = *point1;
			}
			break;

		default: {
			assert(false);
			break;
		}
	}
}

static inline void easyCollision_GetClosestPoint(EasyCollisionInput *input, EasyCollisionOutput *output) {
	//transform points to world space;
	EasyCollisionPolygon *a = &input->a;
	for(int i = 0; i < a->count; ++i) {
		a->pT[i] = transformPositionV3(a->p[i], a->T);
	}
	EasyCollisionPolygon *b = &input->b;
	for(int i = 0; i < b->count; ++i) {
		b->pT[i] = transformPositionV3(b->p[i], b->T);
	}
	//

	EasySimplex simplex = {};

	EasySimplexVertex *vertex = &simplex.points[simplex.count++];
	vertex->point = v3_minus(b->pT[0], a->pT[0]);
	vertex->pointA = a->pT[0];
	vertex->pointB = b->pT[0];
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
			saveA[i] = simplex.points[i].indexA;
			saveB[i] = simplex.points[i].indexB;
		}

		// printf("before %d\n", simplex.count);
		// printf("%s %d\n", "indexA", simplex.points[0].indexA);
		// printf("%s %d\n", "indexB", simplex.points[0].indexB);
		// printf("%s %d\n", "indexA", simplex.points[1].indexA);
		// printf("%s %d\n", "indexB", simplex.points[1].indexB);
		// printf("%s %d\n", "indexA", simplex.points[2].indexA);
		// printf("%s %d\n", "indexB", simplex.points[2].indexB);


		//drop non-contributing verticies
		switch(simplex.count) {
			case 1: {
				// printf("%s\n", "solve1");
				//nothing happens
			} break;
			case 2: {
				// printf("%s\n", "solve2");
				easyCollision_solve2(&simplex);
			} break; 
			case 3: {
				// printf("%s\n", "solve3");
				easyCollision_solve3(&simplex);
			} break;

		}

		// printf("count %d\n", simplex.count);
		// printf("%s %d\n", "indexA", simplex.points[0].indexA);
		// printf("%s %d\n", "indexB", simplex.points[0].indexB);
		// printf("%s %d\n", "indexA", simplex.points[1].indexA);
		// printf("%s %d\n", "indexB", simplex.points[1].indexB);
		// printf("%s %d\n", "indexA", simplex.points[2].indexA);
		// printf("%s %d\n", "indexB", simplex.points[2].indexB);

		if(simplex.count == 3) {
			// still a triangle, so point is inside triangle
			found = true;
			continue;
		}

		V3 direction = easyCollision_getSearchDirection(&simplex);
		// printf("%s %f %f\n", "dir: ", direction.x, direction.y);

		if(dotV3(direction, direction) == 0.0f) {
			//overlaps vertex
			// printf("%s\n", "was null");
			found = true;
			continue;
		}

		if(!found) {

			int aIndex = easyCollision_supportFunction(a, v3_scale(-1, direction));
			int bIndex = easyCollision_supportFunction(b, v3_scale(1, direction));


			EasySimplexVertex *vertex2 = &simplex.points[simplex.count];
			vertex2->point = v3_minus(b->pT[bIndex], a->pT[aIndex]);
			vertex2->pointA = a->pT[aIndex];
			vertex2->pointB = b->pT[bIndex];
			vertex2->indexA = aIndex;
			vertex2->indexB = bIndex;
			vertex2->u = 1; //actually set in the redunancy check
			
			//main termination case
			bool duplicate = false;
			for(int i = 0; i < saveCount; ++i) {
				if(saveA[i] == aIndex && saveB[i] == bIndex) {
					found = true;
					// printf("a %d\n", aIndex);
					// printf("b %d\n", bIndex);
					break;
				}
			}
			if(!found) {
				// printf("added index a: %d\n", aIndex);
				// printf("added index b: %d\n", bIndex);
				simplex.count++;
			} else {
				// printf("duplicate: %d\n", simplex.count);
				// printf("added index a: %d\n", aIndex);
				// printf("added index b: %d\n", bIndex);
			}
		}
	}
	
	// printf("%d\n", simplex.count);
	easyCollision_getWitnessPoints(&simplex, &output->pointA, &output->pointB);
	output->normal = v3_minus(output->pointA, output->pointB);
	output->distance = getLengthV3(output->normal);
	output->normal = normalizeV3(output->normal);
	if(dotV3(output->normal, output->normal) == 0) {
		output->normal = v3(0, 1, 0);
	}
	output->wasInside = (simplex.count == 3);
}











