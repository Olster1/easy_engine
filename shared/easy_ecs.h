typedef enum {
	EASY_COMPONENT_SPRITE,
	EASY_COMPONENT_LIGHT,
	EASY_COMPONENT_MODEL,
	EASY_COMPONENT_AUDIO_SOURCE,
	EASY_COMPONENT_COLLIDER,
	EASY_COMPONENT_RIGIDBODY,
	EASY_COMPONENT_SKELETAL_ANIMATION,
} EasyComponentType;

typedef struct {
	EasyComponentType type;
	int padding0;

	void *c;
} EasyComponent;

typedef struct {
	EasyTransform transform;
	char *name;

	int componentCount;
	EasyComponent components[64];
} EasyEntity;


typedef struct {
	EasyTransform *T; 
	EasyTransform *listener;
	PlayingSound *sound;

	float volume;
	float pitch;
	float innerRadiusSqr; 
	float outerRadiusSqr;
	
} EasyComponent_AudioSource;

typedef struct {
	ArrayDynamic entities;

	ArrayDynamic models;
	ArrayDynamic colliders;
	ArrayDynamic rigidBodies;
	ArrayDynamic lights;
	ArrayDynamic audioSources;

} EasyEntityManager;

static inline void easyEntity_initEntity(EasyEntity *e, char *name) {
	easyTransform_initTransform(&e->transform);
	e->name = name;
}

static inline void easyEntity_initEntityManager(EasyEntityManager *m) {
	initArray(&m->entities, EasyEntity);
	initArray(&m->models, EasyModel);
	initArray(&m->colliders, EasyCollider);
	initArray(&m->rigidBodies, EasyRigidBody);
	initArray(&m->lights, EasyLight);
	initArray(&m->audioSources, EasyComponent_AudioSource);
}
static inline EasyEntity *easyEntity_getNewEntity(EasyEntityManager *m) {
	ArrayElementInfo packet = getEmptyElementWithInfo(&m->entities);
	EasyEntity *result = (EasyEntity *)packet.elm;

	easyEntity_initEntity(result);
	return result;
}	

static inline void *easyEntity_addComponentToEntity(EasyEntityManager *m, EasyEntity *e, EasyComponentType t) {
	assert(e->componentCount < arrayCount(e->components));
	EasyComponent *c = e->components + e->componentCount++;
	c->type = t;

	switch(c->type) {
		case EASY_COMPONENT_MODEL: {
			c->data = getEmptyElementWithInfo(&m->models);
		} break;
		case EASY_COMPONENT_LIGHT: {
			c->data = getEmptyElementWithInfo(&m->lights);
		} break;
		default: {

		} break;
	}

	return c;

}

static inline void *easyEntity_findComponent(EasyEntity *e, EasyComponentType t) {

}

static inline void easyEntity_updateWorld(EasyEntityManager *m, RenderGroup *g) {
		g->lightCount = 0; //NOTE: empty lights out
		for(int i = 0; i < m->lights.count; ++i) {
			EasyLight *l = (EasyLight *)getElement(& m->lights, i);
			if(l) {
				easy_addLight(g, l);
			}
		}

		//models
		for(int i = 0; i < m->models.count; ++i) {
			EasyModel *l = (EasyModel *)getElement(& m->models, i);
			if(l) {
				Matrix4 T = easyTransform_getTransform(l->T);
				
				setModelTransform(g, T);
				renderModel(g, l, l->colorTint);
			}
		}

		//NOTE: Update 3d sounds
		for(int i = 0; i < m->audioSources.count; ++i) {
			EasyComponent_AudioSource *l = (EasyComponent_AudioSource *)getElement(& m->models, i);
			if(l) {
				EasyTransform T = l->T; 

				V3 difference = v3_minus(listener->pos, T->pos);
				V3 distSqr = v3_dot(difference, difference);
				volumePercent = 0.0f;
				assert(innerRadiusSqr < outerRadiusSqr);
				if(distSqr < outerRadiusSqr) {
					if(distSqr < innerRadiusSqr) {
						volumePercent = 1.0f;
					} else {
						float denom = (outerRadiusSqr - innerRadiusSqr);
						if(denom > 0) {
							volumePercent = (distSqr - innerRadiusSqr) / denom;	
							volumePercent = 1.0f - volumePercent;
						}
					}
				}
				l->sound->volume = volumePercent;	
			}
		}
		/////
}

