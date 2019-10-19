typedef struct EasyTransform EasyTransform;
typedef struct EasyTransform {
	Matrix4 T;
	V3 pos;
	V3 scale;
	Quaternion Q;

	EasyTransform *parent;


} EasyTransform;

static inline void easyTransform_initTransform(EasyTransform *t, V3 pos) {
	t->T = mat4();
	t->pos = pos;
	t->scale = v3(1, 1, 1);
	t->Q = identityQuaternion();

}

static inline Matrix4 easyTransform_getTransform(EasyTransform *T) {
	Matrix4 result;
	EasyTransform *parent = T->parent;
	Quaternion q = T->Q;
	V3 s = T->scale;
	V3 pos = T->pos;
	while(parent) {
		s = v3_hadamard(s, parent->scale);
		pos = v3_plus(pos, parent->pos);
		q = quaternion_mult(parent->Q, q);
		parent = parent->parent;
	}

	//@speed This could proabably be speed up. Finding a formula that takes SQT & turns it into a matrix 
	//Mmm, not sure the effect if I scale first then rotate, or vice versa?
	result = quaternionToMatrix(q);
	result = Mat4Mult(Matrix4_scale(mat4(), s), result);
	result = Mat4Mult(Matrix4_translate(mat4(), pos), result);
	////

	T->T = result;

	return result;
	
}

static inline V3 easyTransform_getZAxis(EasyTransform *T) {
	Matrix4 t = easyTransform_getTransform(T); //@speed dont know how expensive this could be?
	V3 result = easyMath_getZAxis(t);
	return result;
}

