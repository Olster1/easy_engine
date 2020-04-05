typedef enum {
	EASY_TRANSFORM_STATIC_ID,
	EASY_TRANSFORM_TRANSIENT_ID,
} EasyTransform_IdType;

typedef struct EasyTransform EasyTransform;
typedef struct EasyTransform {
	Matrix4 T;
	V3 pos;
	V3 scale;
	Quaternion Q;

	EasyTransform *parent;

	EasyTransform_IdType idType;
	int id;
	bool markForDeletion;
} EasyTransform;


static int GLOBAL_transformID_static = 0;
static int GLOBAL_transformID_transient = 0; 

static inline void easyTransform_initTransform(EasyTransform *t, V3 pos, EasyTransform_IdType idType) {
	t->T = mat4();
	t->pos = pos;
	t->scale = v3(1, 1, 1);
	t->Q = identityQuaternion();

	t->parent = 0;
	if(idType == EASY_TRANSFORM_STATIC_ID) {
		t->id = GLOBAL_transformID_static++;
		t->idType = idType; 
	} else if(idType == EASY_TRANSFORM_TRANSIENT_ID) {
		t->id = GLOBAL_transformID_transient++;
		t->idType = idType;
	}
	t->markForDeletion = false;
}

static inline void easyTransform_initTransform_withScale(EasyTransform *t, V3 pos, V3 scale, EasyTransform_IdType idType) {
	easyTransform_initTransform(t, pos, idType);
	t->scale =  scale;
}

static inline Matrix4 easyTransform_getTransform(EasyTransform *T) {
	Matrix4 result = mat4();
	EasyTransform *parent = T;

	/*
	//NOTE(ollie): The way the transform works is that it keeps the translate out seperatley,
					so things can just add the translation & doesn't effect the rotation of it 
	*/

	V3 translation = v3(0, 0, 0);
	while(parent) {

		Matrix4 thisT = Matrix4_scale(mat4(), parent->scale);
		thisT = Mat4Mult(quaternionToMatrix(parent->Q), thisT);
		

		//NOTE(ollie): Add the translation
		translation = v3_plus(parent->pos, translation);

		//NOTE(ollie): Combine it with the result
		result = Mat4Mult(result, thisT);

		parent = parent->parent;
	}

	//NOTE(ollie): Add the translation component. See comment above to see why
	result = Mat4Mult(Matrix4_translate(mat4(), translation), result);

	T->T = result;

	return result;
	
}

static inline Matrix4 easyTransform_getWorldRotation(EasyTransform *T) {
	Matrix4 result = mat4();

	EasyTransform *parent = T->parent;
	Quaternion q = T->Q;
	while(parent) {
		q = quaternion_mult(parent->Q, q);
		parent = parent->parent;
	}

	result = quaternionToMatrix(q);

	return result;
}

static inline V3 easyTransform_getWorldScale(EasyTransform *T) {
	EasyTransform *parent = T->parent;
	V3 s = T->scale;
	while(parent) {
		s = v3_hadamard(s, parent->scale);
		parent = parent->parent;
	}

	return s;
}

static inline V3 easyTransform_getWorldPos(EasyTransform *T) {
	V3 pos = T->pos;
	EasyTransform *parent = T->parent;

	int parentCount = 0;
	while(parent) {
		parentCount++;
		pos = v3_plus(pos, parent->pos);
		parent = parent->parent;
	}

	return pos;
}

static inline void easyTransform_setWorldPos(EasyTransform *T, V3 worldPos) {
	EasyTransform *parent = T->parent;

	V3 pos = v3(0, 0, 0);

	while(parent) {
		pos = v3_plus(pos, parent->pos);
		parent = parent->parent;
	}

	//NOTE(ollie): Take away the remainding postion to get it to be the world pos
	pos = v3_minus(worldPos, pos);

	//NOTE(ollie): Set the world pos
	T->pos = pos;
}

static inline V3 easyTransform_getZAxis(EasyTransform *T) {
	Matrix4 t = easyTransform_getTransform(T); //@speed dont know how expensive this could be?
	V3 result = easyMath_getZAxis(t);
	return result;
}

