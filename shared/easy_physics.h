#ifndef PHYSICS_TIME_STEP
#define PHYSICS_TIME_STEP 0.002f 
#endif

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
	V3 hitP;
	float hitT; 
	bool didHit;
} EasyPhysics_RayCastAABB3f_Info;

static inline EasyPhysics_RayCastAABB3f_Info EasyPhysics_CastRayAgainstAABB3f(Matrix4 boxRotation, V3 boxCenter, Rect3f bounds, V3 rayDirection, V3 rayPos) {
	EasyPhysics_RayCastAABB3f_Info result = {};

	EasyRay r = {};
	r.origin = v3_minus(rayPos, boxCenter); //offset it from the box center

	//Doesn't have to be normalized direction since we do it in the function
	r.direction = rayDirection;
	r = EasyMath_transformRay(r, mat4_transpose(boxRotation));


	result.didHit = easyMath_rayVsAABB3f(r.origin, r.direction, bounds, &result.hitP, &result.hitT);

	return result;
}




typedef struct {
	int arrayIndex;
	
	V3 dP;
	V3 accumForce; //perFrame
	V3 accumTorque;

	union {
		struct { //2d
			float inverse_I;
		};
		struct { //3d
			V3 dA;
			// Matrix4 inverse_I;
		};
	};
	
	float inverseWeight;


} EasyRigidBody;

typedef enum {
	EASY_COLLISION_ENTER,
	EASY_COLLISION_STAY,
	EASY_COLLISION_EXIT,
} EasyCollisionType;

typedef struct {
	EasyCollisionType type;
	int objectId; 

	bool hitThisFrame_; //only used internally 
} EasyCollisionInfo;

typedef enum {
	EASY_COLLIDER_SPHERE,
	EASY_COLLIDER_CYLINDER,
	EASY_COLLIDER_BOX,
	EASY_COLLIDER_CIRCLE,
	EASY_COLLIDER_RECTANGLE,
	EASY_COLLIDER_CAPSULE,
	EASY_COLLIDER_CONVEX_HULL,
} EasyColliderType;

typedef struct {
	int arrayIndex;

	EasyTransform *T;
	EasyRigidBody *rb;

	EasyColliderType type;
	V3 offset;

	bool isTrigger;

	int collisionCount;
	EasyCollisionInfo collisions[256];

	union {
		struct {
			float radius;
		};
		struct {
			float capsuleRadius;
			float capsuleLength;
		};
		struct {
			V3 dim3f;
		};
		struct {
			V2 dim2f;
		};
	};
} EasyCollider;


typedef struct {
	Array_Dynamic colliders;
	Array_Dynamic rigidBodies;
	float physicsTime;
} EasyPhysics_World;


static void EasyPhysics_beginWorld(EasyPhysics_World *world) {
	initArray(&world->colliders, EasyCollider);
	initArray(&world->rigidBodies, EasyRigidBody);
}


/*
How to use Collision info:
	
	for(colliders) {
		if(was collision) {
			EasyCollider_addCollisionInfo(colA, hitObjectId_B);
			EasyCollider_addCollisionInfo(colB, hitObjectId_A);
		}
	}
		
	//clean up all collisions (basically checks if still colliding )
	for(colliders) {
		EasyCollider_removeCollisions(collider)
	}
*/

static EasyCollisionInfo *EasyCollider_addCollisionInfo(EasyCollider *col, int hitObjectId) {

	EasyCollisionInfo *result = 0;

	for(int i = 0; i < col->collisionCount; ++i) {
		EasyCollisionInfo *info = col->collisions + i;

		if(info->objectId == hitObjectId) {
			result = info;
			
			if(!result->hitThisFrame_ && result->type == EASY_COLLISION_ENTER) {
				result->type = EASY_COLLISION_STAY; //already in the array, so is a stay type
			}
			

			if (result->type == EASY_COLLISION_EXIT) {
				assert(!result->hitThisFrame_);
				result->type = EASY_COLLISION_ENTER;
			}
			
			break;
		}	
	}

	if(!result) {
		//get a new collision
		assert(col->collisionCount < arrayCount(col->collisions));
		result =  col->collisions + col->collisionCount++;
		result->type = EASY_COLLISION_ENTER;
	}

	assert(result);

	result->hitThisFrame_ = true;
	result->objectId = hitObjectId;

	return result;
}

static void EasyCollider_removeCollisions(EasyCollider *col) {
	for(int i = 0; i < col->collisionCount;) {
		bool increment = true;
		EasyCollisionInfo *info = col->collisions + i;

		if(!info->hitThisFrame_) {
			if(info->type != EASY_COLLISION_EXIT) {
				//NOTE(ollie): set collision to an exit collision
				info->type = EASY_COLLISION_EXIT;
			} else {
				//NOTE(ollie): remove the collision info from the array
				//Take from the end & fill the gap
				col->collisions[i] = col->collisions[--col->collisionCount];
				increment = false;
			}
		}

		if(increment) {
			i++;
		}
	}

///////////////////////************ Empty out all hits this frame*************////////////////////
	for(int i = 0; i < col->collisionCount; i++) {
		EasyCollisionInfo *info = col->collisions + i;
		info->hitThisFrame_ = false;
	}
////////////////////////////////////////////////////////////////////

}

	
static inline EasyCollisionOutput EasyPhysics_SolveRigidBodies(EasyCollider *a_, EasyCollider *b_) {
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

void EasyPhysics_ResolveCollisions(EasyRigidBody *ent, EasyRigidBody *testEnt, EasyTransform *A, EasyTransform *B, EasyCollisionOutput *output) {
	// V2 AP = v2_minus(output->pointA.xy, A->pos.xy);
	// isNanErrorV2(AP);
	// // printf("%f %f\n", output->pointB.x, output->pointB.y);
	// // printf("%f %f\n", testEnt->T->pos.x, testEnt->T->pos.y);
	// V2 BP = v2_minus(output->pointB.xy, B->pos.xy);
	// isNanErrorV2(BP);
	    
	// V2 Velocity_A = v2_plus(ent->dP.xy, v2_scale(ent->dA, perp(AP)));
	// isNanErrorV2(Velocity_A);
	// V2 Velocity_B = v2_plus(testEnt->dP.xy, v2_scale(testEnt->dA, perp(BP)));
	// isNanErrorV2(Velocity_B);

	// V2 RelVelocity = v2_minus(Velocity_A, Velocity_B);
	// isNanErrorV2(RelVelocity);

	// float R = 1.0f; //NOTE(oliver): CoefficientOfRestitution

	// float Inv_BodyA_Mass = ent->inverseWeight;
	// float Inv_BodyB_Mass = testEnt->inverseWeight;
	        
	// float Inv_BodyA_I = ent->inverse_I;
	// float Inv_BodyB_I = testEnt->inverse_I;
	// V2 N = output->normal.xy;
	        
	// float J_Numerator = dotV2(v2_scale(-(1.0f + R), RelVelocity), N);
	// float J_Denominator = dotV2(N, N)*(Inv_BodyA_Mass + Inv_BodyB_Mass) +
	//     (sqr(dotV2(perp(AP), N)) * Inv_BodyA_I) + (sqr(dotV2(perp(BP), N)) * Inv_BodyB_I);

	// float J = J_Numerator / J_Denominator;
	// isNanf(J);

	// V2 Impulse = v2_scale(J, N);
	// isNanErrorV2(Impulse);
	    
	// ent->dP.xy = v2_plus(ent->dP.xy, v2_scale(Inv_BodyA_Mass, Impulse));                
	// isNanErrorV2(ent->dP.xy);
	// testEnt->dP.xy = v2_minus(testEnt->dP.xy, v2_scale(Inv_BodyB_Mass, Impulse));
	// isNanErrorV2(testEnt->dP.xy);
	// ent->dA = ent->dA + (dotV2(perp(AP), Impulse)*Inv_BodyA_I);
	// testEnt->dA = testEnt->dA - (dotV2(perp(BP), Impulse)*Inv_BodyB_I);
}

static EasyRigidBody *EasyPhysics_AddRigidBody(EasyPhysics_World *world, float inverseWeight, float inverseIntertia) {
    ArrayElementInfo arrayInfo = getEmptyElementWithInfo(&world->rigidBodies);

    EasyRigidBody *rb = (EasyRigidBody *)arrayInfo.elm;

    // memset(rb, 0, sizeof(EasyRigidBody));
    rb->arrayIndex = arrayInfo.absIndex;

    rb->dP = v3(0, 0, 0);
    rb->inverseWeight = inverseWeight;
    rb->inverse_I = inverseIntertia;
    rb->dA = NULL_VECTOR3;

    return rb;
}


static EasyCollider *EasyPhysics_AddCollider(EasyPhysics_World *world, EasyTransform *T, EasyRigidBody *rb, EasyColliderType type, V3 offset, bool isTrigger, V3 info) {
	ArrayElementInfo arrayInfo = getEmptyElementWithInfo(&world->colliders);

	EasyCollider *col = (EasyCollider *)arrayInfo.elm;
	memset(col, 0, sizeof(EasyCollider));
	
	col->arrayIndex = arrayInfo.absIndex;

	assert(T);
	col->T = T;
	col->rb = rb;

	col->type = type;
	col->offset = offset;

	col->isTrigger = isTrigger;

	col->collisionCount = 0;

	switch (type) {
		case EASY_COLLIDER_CIRCLE: {
			col->radius = info.x;
		} break;
		case EASY_COLLIDER_RECTANGLE: {
			col->dim2f = info.xy;
		} break;
		default: {
			assert(false);
		}
	}

    return col;
}

static void EasyPhysics_AddGravity(EasyRigidBody *rb, float scale) {
	V3 gravity = v3(0, scale*-9.81f, 0);

	rb->accumForce = v3_plus(rb->accumForce, gravity);
}

void ProcessPhysics(Array_Dynamic *colliders, Array_Dynamic *rigidBodies, float dt) {
	for (int i = 0; i < rigidBodies->count; ++i)
	{
	    EasyRigidBody *rb = (EasyRigidBody *)getElement(rigidBodies, i);
	    if(rb) {

	    ///////////////////////************* Integrate accel & velocity ************////////////////////
	    	
	        rb->dP = v3_plus(v3_scale(dt * rb->inverseWeight, rb->accumForce), rb->dP);

			//TODO(ollie): Integrate torque 

	        rb->dA = v3_plus(v3_scale(dt, rb->accumTorque), rb->dA);


	    ////////////////////////////////////////////////////////////////////
	        
	    }
	}


    for (int i = 0; i < colliders->count; ++i)
    {
        EasyCollider *a = (EasyCollider *)getElement(colliders, i);
        if(a) {

        ///////////////////////************* Integrate the physics ************////////////////////

	        if(a->rb) {
	        	EasyRigidBody *rb = a->rb;
	        	a->T->pos = v3_plus(v3_scale(dt, rb->dP), a->T->pos);

	        	a->T->Q = addScaledVectorToQuaternion(a->T->Q, rb->dA, dt);

	        	a->T->Q = easyMath_normalizeQuaternion(a->T->Q);
	        }

        ////////////////////////////////////////////////////////////////////

            float smallestDistance = 0.1f;
            EasyCollider *hitEnt = 0;

            EasyCollisionOutput outputInfo = {};

        ///////////////////////*********** Test for collisions **************////////////////////

            for (int j = i + 1; j < colliders->count; ++j)
            {
                assert(i != j);

                EasyCollider *b = (EasyCollider *)getElement(colliders, j);

                if(b) {
                	bool hit = false;
                	/*
                		If both objects aren't triggers, actually process the physics
                		using minkowski-baycentric coordinates

                	*/
                	if(!a->isTrigger && !b->isTrigger) {
                		/*
							Non trigger collisions
                		*/
                		EasyRigidBody *rb_a = a->rb;
                		EasyRigidBody *rb_b = b->rb;

                		// if(!(rb_a->inverseWeight == 0 && rb_b->inverseWeight == 0)) 
                		{
                		    EasyCollisionOutput out = EasyPhysics_SolveRigidBodies(a, b);

                		    if(out.distance <= smallestDistance) {
                		        hitEnt = b;
                		        smallestDistance = out.distance;
                		        outputInfo = out;
                		    }
                		}
                	} else {
                	/*
                		If there is a trigger involved, just do overlap collision detection
                	*/
                		
                		V3 aPos = v3_plus(easyTransform_getWorldPos(a->T), a->offset);
                		V3 bPos = v3_plus(easyTransform_getWorldPos(b->T), b->offset);

                		bool circle = a->type == EASY_COLLIDER_CIRCLE || b->type == EASY_COLLIDER_CIRCLE;
                		bool rectangle = a->type == EASY_COLLIDER_RECTANGLE || b->type == EASY_COLLIDER_RECTANGLE;
                		if(circle && rectangle) {
                			assert(false);
                		} else if(rectangle && !circle) { //both rectangles
                			//case not handled
                			assert(false);
                		} else if(circle && !rectangle) { //both circles
                			V3 centerDiff = v3_minus(aPos, bPos);

                			//TODO(ollie): Can use radius sqr for speed!
                			if(getLengthV3(centerDiff) <= (a->radius + b->radius)) {
                				hit = true;
                			}
                		} else {
                			//case not handled
                			assert(false);
                		}
                	}

                	if(hit) {
                		//add collision info
                		EasyCollider_addCollisionInfo(a, b->T->id);
                		EasyCollider_addCollisionInfo(b, a->T->id);
                	}
                }
            }

            if(hitEnt) {
                // EasyPhysics_ResolveCollisions(a->rb, hitEnt, &outputInfo);
            } 
        }
    }

}


static void EasyPhysics_UpdateWorld(EasyPhysics_World *world, float dt) {

	world->physicsTime += dt;
	float timeInterval = PHYSICS_TIME_STEP;
	while(world->physicsTime > 0.00000001f) {
		float t = min(timeInterval, world->physicsTime);
	    ProcessPhysics(&world->colliders, &world->rigidBodies, t);
	    world->physicsTime -= t;
	}

	///////////////////////************ Post process collisions *************////////////////////

	    for (int i = 0; i < world->colliders.count; ++i)
	    {
	        EasyCollider *col = (EasyCollider *)getElement(&world->colliders, i);
	        if(col) {
	        	EasyCollider_removeCollisions(col);
	        	if(col->rb) {
	        		col->rb->accumForce = NULL_VECTOR3;
	        		col->rb->accumTorque = NULL_VECTOR3;
	        	}
	        }
	    }

	////////////////////////////////////////////////////////////////////
}