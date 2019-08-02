typedef struct {
	V3 point;  //point b - point a
	V3 pointA; //original point on A
	V3 pointB; //original point on B

	float u; //baycentric contribution
	float divisor;//used to normalize the bayecentric value in get witness points

} EasySimplexVertex;

typedef struct {
	EasySimplexVertex points[3];
	int count;
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

	V3 maxDistance = 

	for(int iA = 0; iA < input->aCount; iA++) {
		for(int iB = 0; iB < input->aCount; iB++) {

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
}











