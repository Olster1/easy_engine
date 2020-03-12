/*
How to use: 

easyFlashText_addText(EasyFlashTextManager *m, char *message);
*/

typedef struct {
	float tAt;
	char *message;
} EasyFlashText;

typedef struct {

	int textCount;
	EasyFlashText texts[16];

	V2 resolution;
	float relSize;

	EasyFont_Font *font;
} EasyFlashTextManager;

static EasyFlashTextManager globalFlashTextManager;

static inline void easyFlashText_initManager(EasyFlashTextManager *m, EasyFont_Font *font, float aspectRatio_yOverX) {
	m->textCount = 0;
	m->font = font;

	float fuaxWidth = 1920.0f;
	m->resolution = v2(fuaxWidth, aspectRatio_yOverX*fuaxWidth);
	m->relSize = 1;
}	

static inline void easyFlashText_addText(EasyFlashTextManager *m, char *message) {
	assert(m->textCount < arrayCount(m->texts));

	EasyFlashText *t = &m->texts[m->textCount++];

	t->message = message;
	t->tAt = 0;
}


static inline void easyFlashText_updateManager(EasyFlashTextManager *m, RenderGroup *g, float dt) {
	for(int i = 0; i < m->textCount; ) {
		EasyFlashText *t = &m->texts[i];	
		bool increment = true;



		float timePeriod = 1.5f;

		V2 resolution = m->resolution;

		t->tAt += dt;

		float alphaValue = 1.0f - (t->tAt / timePeriod);

		if(t->tAt > timePeriod) {
			//pull last item to the new item place
			m->texts[i] = m->texts[--m->textCount];
			increment = false;
			alphaValue = 0;
		} 

		outputTextNoBacking(m->font, 0.5f*resolution.x, 0.5f*resolution.y, 1, resolution, t->message, rect2fMinMax(0, 0, resolution.x, resolution.y), v4(1, 1, 0, alphaValue), 1, true, m->relSize);

		if(increment) {
			i++;
		}
	} 
	

	
}


