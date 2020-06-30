
typedef struct {
	GameButton gameButtons[BUTTON_COUNT];
    	
    V2 mouseP_01;
	V2 mouseP;
	V2 mouseP_yUp;
	V2 mouseP_left_up;

	char *inputString;
    char *droppedFilePath;
    
	int scrollWheelY;
} AppKeyStates;



static inline V2 easyInput_mouseToResolution(AppKeyStates *input, V2 resolution) {
	V2 result = v2(input->mouseP_01.x*resolution.x, input->mouseP_01.y*resolution.y);
	return result;
}

static inline V2 easyInput_mouseToResolution_originLeftBottomCorner(AppKeyStates *input, V2 resolution) {
	V2 result = v2(input->mouseP_01.x*resolution.x, (1.0f - input->mouseP_01.y)*resolution.y);
	return result;
}
