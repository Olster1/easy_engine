/*
How to use: 

//NOTE(ollie): Setup the camera
EasyCamera camera;
easy3d_initCamera(&camera, v3(0, 0, 0));

//NOTE(ollie): Every frame 
while(true) {

	//NOTE(ollie): Move the timer
	EasyCamera_MoveType camMoveTypes = (EasyCamera_MoveType)(EASY_CAMERA_MOVE | EASY_CAMERA_ROTATE | EASY_CAMERA_ZOOM);
	
	//NOTE(ollie): Update the camera
	easy3d_updateCamera(&camera, &keyStates, sensitvity, cameraMovePower, dt, camMoveTypes);
	
	//NOTE(ollie): For diffuse lighting
	easy_setEyePosition(globalRenderGroup, camera.pos);	


}

*/

typedef struct {
	V3 pos;
	Quaternion orientation;

	float pitch;
	float heading;
	V3 velocity;

	float zoom;

	V2 lastMouseP;
	bool lastP_isSet;
} EasyCamera;

typedef enum {
	EASY_CAMERA_MOVE_NULL = 0,
	EASY_CAMERA_ROTATE = 1 << 0,
	EASY_CAMERA_MOVE = 1 << 1,
	EASY_CAMERA_ZOOM = 1 << 2,
	EASY_CAMERA_MOVE_XY = 1 << 3,
} EasyCamera_MoveType;

static inline void easy3d_updateCamera(EasyCamera *cam, AppKeyStates *keyStates, float sensitivity, float movePower, float dt, EasyCamera_MoveType moveType) {
	DEBUG_TIME_BLOCK()
	//update view direction
	if(!cam->lastP_isSet) {
		cam->lastMouseP = keyStates->mouseP;
		cam->lastP_isSet = true;
	}

	float deltaX = keyStates->mouseP.x - cam->lastMouseP.x;
	float deltaY = keyStates->mouseP.y - cam->lastMouseP.y;
	cam->lastMouseP = keyStates->mouseP;
	
	// printf("deltaY: %f\n", deltaY);
	// printf("sensi: %f\n", sensitivity);
	if(moveType & EASY_CAMERA_ROTATE) {
		cam->heading += deltaX*sensitivity; //degrees
		cam->pitch += deltaY*sensitivity; //degrees

		if(cam->pitch > 89.0f) {
			cam->pitch = 89.0f;
		}

		if(cam->pitch < -89.0f) {
			cam->pitch = -89.0f;
		}

		float h = 2*PI32*cam->heading/360.0f; //convert to radians
		float p = 2*PI32*cam->pitch/360.0f; //convert to radians

		//this was when we were using the graham smit? conversion to cacluate the direction vector
		// Matrix4 orientation = mat4_xyzAxis(cos(p) * cos(h), sin(p), cos(p) * sin(h));
		// printf("%f\n", h);
		// printf("%f\n", p);

		//NOTE: Heading first (rotate about y-axis), Pitch second (rotate about x-axis)
		cam->orientation = eulerAnglesToQuaternion(h, p, 0);
	}

	// printf("quert %f %f %f \n", cam->orientation.r, cam->orientation.i, cam->orientation.j, cam->orientation.k);

	if(moveType & EASY_CAMERA_ZOOM) {
		cam->zoom += keyStates->scrollWheelY;

		if(cam->zoom > 89.0f) {
			cam->zoom = 89.0f;
		}
		if(cam->zoom < 20.0f) {
			cam->zoom = 20.0f;
		}
	}
	
	float xPower = 0;
	float zPower = 0;
	float power = movePower;
	if(moveType & EASY_CAMERA_MOVE) {
		if(isDown(keyStates->gameButtons, BUTTON_RIGHT)) {
			xPower = power;
		}
		if(isDown(keyStates->gameButtons, BUTTON_LEFT)) {
			xPower = -power;
		}
		if(isDown(keyStates->gameButtons, BUTTON_UP)) {
			zPower = power;
		}
		if(isDown(keyStates->gameButtons, BUTTON_DOWN)) {
			zPower = -power;
		}
	}

	Matrix4 camOrientation = quaternionToMatrix(cam->orientation);

	V3 zAxis = normalizeV3(easyMath_getZAxis(camOrientation));
	V3 xAxis = normalizeV3(easyMath_getXAxis(camOrientation));


	if(moveType & EASY_CAMERA_MOVE_XY) {
		zAxis = v3(0, 1, 0);
		xAxis = v3(1, 0, 0);
	}

	V3 moveDir = v3_plus(v3_scale(xPower, xAxis), v3_scale(zPower, zAxis));
	

	cam->velocity = v3_plus(v3_scale(dt, moveDir), cam->velocity);

	cam->velocity = v3_minus(cam->velocity, v3_scale(0.8f, cam->velocity));
	cam->pos = v3_plus(v3_scale(dt, cam->velocity), cam->pos);

	// printf("pos: %f %f %f\n", cam->pos.x, cam->pos.y, cam->pos.z);
}

static inline V3 EasyCamera_getZAxis(EasyCamera *cam) {
	DEBUG_TIME_BLOCK()
	Matrix4 camOrientation = quaternionToMatrix(cam->orientation);

	V3 zAxis = normalizeV3(easyMath_getZAxis(camOrientation));

	return zAxis;
}

static inline EasyCamera_MoveType easyCamera_addFlag(EasyCamera_MoveType camMove, EasyCamera_MoveType flag) {
	return ((EasyCamera_MoveType)(((int)flag) | (int)camMove));
}


static inline EasyCamera_MoveType easyCamera_removeFlag(EasyCamera_MoveType camMove, EasyCamera_MoveType flag) {
	return ((EasyCamera_MoveType)((~(int)flag) & (int)camMove));
}

static inline void easy3d_initCamera(EasyCamera *cam, V3 pos) {
	memset(cam, 0, sizeof(EasyCamera));
	cam->orientation = identityQuaternion();
	cam->zoom = 60.0f;//start at 60 degrees FOV
	cam->pos = pos;
}

static inline Matrix4 easy3d_getViewToWorld(EasyCamera *camera) {
	DEBUG_TIME_BLOCK()
	Matrix4 result = mat4();

	Matrix4 rotation = quaternionToMatrix(camera->orientation);
	
	Matrix4 translation = Matrix4_translate(mat4(), camera->pos);
	result = Mat4Mult(translation, rotation);

	return result;
}

static inline Matrix4 easy3d_getWorldToView(EasyCamera *camera) {
	DEBUG_TIME_BLOCK()

	Matrix4 rotation = quaternionToMatrix(camera->orientation);

	rotation = mat4_transpose(rotation);

	Matrix4 translation = Matrix4_translate(mat4(), v3_negate(camera->pos));

	//NOTE(): Translation first then rotation
	Matrix4 result = Mat4Mult(rotation, translation);

	return result;
}

//this function makes a world -> view matrix for a camera looking at a position
Matrix4 easy3d_lookAt(V3 cameraPos, V3 targetPos, V3 upVec) {
	DEBUG_TIME_BLOCK()
	Matrix4 result = mat4();
	V3 direction = v3_minus(targetPos, cameraPos); //assuming we are in left handed coordinate system
	direction = normalizeV3(direction);
	// printf("DIRECTION: %f%f%f\n", direction.x, direction.y, direction.z);

	V3 rightVec = normalizeV3(v3_crossProduct(upVec, direction));
	if(getLengthV3(rightVec) == 0) {
		// printf("%s\n", "is == 0");
		rightVec = v3(1, 0, 0);
	}
	V3 yAxis = normalizeV3(v3_crossProduct(direction, rightVec));
	assert(getLengthV3(yAxis) != 0);
	
	result = mat4_xyzAxis(rightVec, yAxis, direction);

	// printf("%f %f %f \n", result.a.x, result.a.y, result.a.z);
	// printf("%f %f %f \n", result.b.x, result.b.y, result.b.z);
	// printf("%f %f %f \n", result.c.x, result.c.y, result.c.z);
	// printf("%s\n", "----------------");

	result = mat4_transpose(result);

	Matrix4 cameraTrans = Matrix4_translate(mat4(), v3_negate(cameraPos));

	result = Mat4Mult(result, cameraTrans);

	return result;
}

