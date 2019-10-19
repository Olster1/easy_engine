typedef struct {
	bool collided;
	V2 point;
	V2 normal;
	float distance;
} RayCastInfo;

//TODO: Think more about when the line is parrelel to the edge. 

void easy_phys_updatePosAndVel(V3 *pos, V3 *dP, V3 dPP, float dtValue, float dragFactor) {
	*pos = v3_plus(*pos, v3_plus(v3_scale(sqr(dtValue), dPP),  v3_scale(dtValue, *dP)));
	*dP = v3_plus(*dP, v3_minus(v3_scale(dtValue, dPP), v3_scale(dragFactor, *dP)));
}

float getDtValue(float idealFrameTime, int loopIndex, float dt, float remainder) {
	float dtValue = idealFrameTime;
	if((dtValue * (loopIndex + 1)) > dt) {
	    dtValue = remainder;
	}
	return dtValue;
}

//assumes the shape is clockwise
RayCastInfo easy_phys_castRay(V2 startP, V2 ray, V2 *points, int count) {
	isNanErrorV2(startP);
	RayCastInfo result = {};
	if(!(ray.x == 0 && ray.y == 0)) {
		float min_tAt = 0;
		bool isSet = false;
		for(int i = 0; i < count; ++i) {
			V2 pA = points[i];
			isNanErrorV2(pA);
			int bIndex = i + 1;
			if(bIndex == count) {
				bIndex = 0;
			}

			V2 pB = points[bIndex];
			isNanErrorV2(pB);
			V2 endP = v2_plus(startP, ray);
			isNanErrorV2(endP);

			V2 ba = v2_minus(pB, pA);
			isNanErrorV2(ba);

			V2 sa = v2_minus(startP, pA);
			isNanErrorV2(sa);
			V2 ea = v2_minus(endP, pA);
			isNanErrorV2(ea);
			float sideLength = getLength(ba);
			V2 normalBA = normalize_(ba, sideLength);
			isNanErrorV2(normalBA);
			V2 perpBA = perp(normalBA);
			isNanErrorV2(perpBA);
			////
			float endValueX = dotV2(perpBA, ea);
			isNanErrorf(endValueX);
			float startValueX = dotV2(perpBA, sa); 
			isNanErrorf(startValueX);

			float endValueY = dotV2(normalBA, ea);
			isNanErrorf(endValueY);
			float startValueY = dotV2(normalBA, sa); 
			isNanErrorf(startValueY);

			float tAt = inverse_lerp(startValueX, 0, endValueX);
			if(startValueX == endValueX) { //This is when the line is parrellel to the side. 
				if(startValueX != 0) {
					tAt = -1;
				}
			}
			isNanErrorf(tAt);

			if(tAt >= 0 && tAt < 1) {
				float yAt = lerp(startValueY, tAt, endValueY);
				isNanErrorf(yAt);
				if(yAt >= 0 && yAt < sideLength) {
					if(tAt < min_tAt || !isSet) {
						assert(signOf(startValueX) != signOf(endValueX) || (startValueX == 0 && endValueX == 0) || (startValueX == 0.0f || endValueX == 0.0f));
						float xAt = lerp(startValueX, tAt, endValueX); 
						assert(floatEqual_withError(xAt, 0));
						isNanErrorf(tAt);
						isNanErrorf(startValueX);
						isNanErrorf(endValueX);
						isNanErrorf(xAt);
						result.collided = true;
						result.normal = perp(normalBA);
						result.point = v2_plus(v2(xAt, yAt), pA);
						result.distance = getLength(v2_minus(result.point, startP));
						min_tAt = tAt;
					}
				}
			}
		}
	}
	return result;
}




typedef struct {
	EasyTransform *T;
	V3 dP;
	float angle; //quartenion in 3D
	float dA;
	float inverseWeight;
	float inverse_I;
	// float inertia;
} EasyRigidBody;


typedef enum {
	EASY_COLLIDER_SPHERE,
	EASY_COLLIDER_CYLINDER,
	EASY_COLLIDER_BOX,
	EASY_COLLIDER_CAPSULE,
	EASY_COLLIDER_CONVEX_HULL,
} EasyColliderType;

typedef struct {
	EasyTransform *T;
	EasyRigidBody *rb;

	EasyColliderType type;

	union {
		struct {
			float radius;
		};
		struct {
			float capsuleRadius;
			float capsuleLength;
		};
		struct {
			V3 dim;
		};
	};
} EasyCollider;

	
static inline EasyCollisionOutput EasyPhysics_SolveRigidBodies(EasyRigidBody *a_, EasyRigidBody *b_) {
	EasyCollisionPolygon a = {};
	EasyCollisionPolygon b = {};

	a.T = Matrix4_translate(a_->T->T, a_->T->pos); 
	a.p[0] = v3(-0.5, -0.5, 0);
	a.p[1] = v3(-0.5, 0.5, 0);
	a.p[2] = v3(0.5, 0.5, 0);
	a.p[3] = v3(0.5, -0.5, 0);
	a.count = 4; 

	b = a;
	b.T = Matrix4_translate(b_->T->T, b_->T->pos); 

	EasyCollisionInput input = {};
	input.a = a;
	input.b = b;

	EasyCollisionOutput output = {};
	easyCollision_GetClosestPoint(&input, &output);

	// ouput.pointA; 
	// ouput.pointB; 
	if(output.wasInside) {
		printf("collision: %f\n", output.distance);
	}
	return output;
}

void EasyPhysics_ResolveCollisions(EasyRigidBody *ent, EasyRigidBody *testEnt, EasyCollisionOutput *output) {
	V2 AP = v2_minus(output->pointA.xy, ent->T->pos.xy);
	isNanErrorV2(AP);
	// printf("%f %f\n", output->pointB.x, output->pointB.y);
	// printf("%f %f\n", testEnt->T->pos.x, testEnt->T->pos.y);
	V2 BP = v2_minus(output->pointB.xy, testEnt->T->pos.xy);
	isNanErrorV2(BP);
	    
	V2 Velocity_A = v2_plus(ent->dP.xy, v2_scale(ent->dA, perp(AP)));
	isNanErrorV2(Velocity_A);
	V2 Velocity_B = v2_plus(testEnt->dP.xy, v2_scale(testEnt->dA, perp(BP)));
	isNanErrorV2(Velocity_B);

	V2 RelVelocity = v2_minus(Velocity_A, Velocity_B);
	isNanErrorV2(RelVelocity);

	float R = 1.0f; //NOTE(oliver): CoefficientOfRestitution

	float Inv_BodyA_Mass = ent->inverseWeight;
	float Inv_BodyB_Mass = testEnt->inverseWeight;
	        
	float Inv_BodyA_I = ent->inverse_I;
	float Inv_BodyB_I = testEnt->inverse_I;
	V2 N = output->normal.xy;
	        
	float J_Numerator = dotV2(v2_scale(-(1.0f + R), RelVelocity), N);
	float J_Denominator = dotV2(N, N)*(Inv_BodyA_Mass + Inv_BodyB_Mass) +
	    (sqr(dotV2(perp(AP), N)) * Inv_BodyA_I) + (sqr(dotV2(perp(BP), N)) * Inv_BodyB_I);

	float J = J_Numerator / J_Denominator;
	isNanf(J);

	V2 Impulse = v2_scale(J, N);
	isNanErrorV2(Impulse);
	    
	ent->dP.xy = v2_plus(ent->dP.xy, v2_scale(Inv_BodyA_Mass, Impulse));                
	isNanErrorV2(ent->dP.xy);
	testEnt->dP.xy = v2_minus(testEnt->dP.xy, v2_scale(Inv_BodyB_Mass, Impulse));
	isNanErrorV2(testEnt->dP.xy);
	ent->dA = ent->dA + (dotV2(perp(AP), Impulse)*Inv_BodyA_I);
	testEnt->dA = testEnt->dA - (dotV2(perp(BP), Impulse)*Inv_BodyB_I);
}

void EasyPhysics_AddRigidBody(Array_Dynamic *gameObjects, float inverseWeight, float inverseIntertia, V3 pos) {
    EasyRigidBody *rb = (EasyRigidBody *)getEmptyElement(gameObjects);
    
    EasyTransform *T = pushStruct(&globalLongTermArena, EasyTransform);
    T->pos = pos;

    rb->T = T;
    rb->dP = v3(0, 0, 0);
    rb->inverseWeight = inverseWeight;
    rb->inverse_I = inverseIntertia;
    rb->angle = 0; 
    rb->dA = 0;
}

void ProcessPhysics(Array_Dynamic *gameObjects, float dt) {
    // printf("%d\n",  gameObjects->count);
    for (int i = 0; i < gameObjects->count; ++i)
    {
        EasyRigidBody *a = (EasyRigidBody *)getElement(gameObjects, i);
        if(a) {
            V3 lastPos = a->T->pos;
            float lastAngle = a->dA;
            V3 lastVelocity = a->dP;

            V3 nextPos = v3_plus(v3_scale(dt, a->dP), a->T->pos);
            float nextAngle = a->dA*dt + a->angle;
            V3 nextVelocity = v3_plus(v3_scale(dt, v3(0, a->inverseWeight*-9.81f, 0)), a->dP);
            // printf("%f %f\n", nextVelocity.x, nextVelocity.y);
            // nextVelocity = v3_minus(nextVelocity, v3_scale(0.1f, nextVelocity));

            a->T->pos = nextPos;
            a->angle = nextAngle;
            a->dP = nextVelocity;
            
            float smallestDistance = 0.1f;
            EasyRigidBody *hitEnt = 0;
            EasyCollisionOutput outputInfo;
            bool didHit = false;
            for (int j = 0; j < gameObjects->count; ++j)
            {
                if(i != j) {
                    EasyRigidBody *b = (EasyRigidBody *)getElement(gameObjects, j);
                    if(b && !(a->inverseWeight == 0 && b->inverseWeight == 0)) {
                        EasyCollisionOutput out = EasyPhysics_SolveRigidBodies(a, b);

                        // out.pointA.z = -10;
                        // out.pointB.z = -10;
                        if(out.distance <= smallestDistance) {
                            hitEnt = b;
                            smallestDistance = out.distance;
                            outputInfo = out;
                        }
                        // renderDrawRectCenterDim(out.pointA, v2(0.1, 0.1), COLOR_BLUE, 0, mat4(), perspectiveMatrix);
                        // renderDrawRectCenterDim(out.pointB, v2(0.1, 0.1), COLOR_BLUE, 0, mat4(), perspectiveMatrix);            
                    }
                }
            }

            if(hitEnt) {
                EasyPhysics_ResolveCollisions(a, hitEnt, &outputInfo);
            } else {
                // printf("%s\n", "hey");
                //keep at new positions
            }
        }
    }

}