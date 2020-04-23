typedef enum {
	INTERACTION_NULL,
	INTERACTION_WINDOW_RESIZE,
	INTERACTION_MOVE_LOGIC_BLOCK,
	INTERACTION_EDIT_PARAMETER,
} InteractionType;

typedef struct {
	WindowScene *windowId;
	LogicBlock *block;
	InteractionType type;
	LogicBlockType blockType;

	s32 index;
	s32 index2;
	s32 index3;
} InteractionId;

typedef struct {
	InteractionId id;

	bool isActive;
	bool shouldEnd;
	
	//NOTE(ollie): To seeif we should release the item
	bool wasVisitedInFrame; 

	union {
		struct {
			//NOTE(ollie): For the windows resize
			s32 sideIndex;
		};
		struct {
			//NOTE(ollie): For letting the logic ui know if it should splice in the array
			s32 indexToSpliceAt;
			bool okToSplice;
		};
	};

	//NOTE(ollie): For animation info
	V2 startPos;
	V2 currentPos;

	V2 startMouseP;

	//NOTE(ollie): Out grab offset so the logic block doesn't jump when we grab it
	V2 grabOffset;

	Timer animationTimer;
} ProgramInteraction;


typedef struct {
	ProgramInteraction interaction;
} InteractionState;


static inline InteractionId interaction_getNullId() {
	InteractionId result = {0};
	return result;
}

static void initInteractionState(InteractionState *state) {
	state->interaction.isActive = false;
}

static inline bool interaction_areIdsEqual(InteractionId a, InteractionId b) {
	bool result = (a.windowId == b.windowId) && (a.block == b.block) && (a.type == b.type) && (a.blockType == b.blockType) && (a.index == b.index) && (a.index2 == b.index2) && (a.index3 == b.index3);
	return result;
}


static inline bool logicUi_isInteraction(InteractionState *state, InteractionId id) {
	bool result = state->interaction.isActive && interaction_areIdsEqual(state->interaction.id, id);

	if(result) {
		state->interaction.wasVisitedInFrame = true;		
	}
	return result;
}

static inline ProgramInteraction *logicUI_visitInteraction(InteractionState *state) {
	state->interaction.wasVisitedInFrame = true;
	return &state->interaction;
}

static inline bool logicUI_isInteracting(InteractionState *state) {
	return state->interaction.isActive;
}


static inline void logicUI_endInteraction(InteractionState *state) {
	state->interaction.isActive = false;
	zeroStruct(&state->interaction, ProgramInteraction);

}

static ProgramInteraction *logicUI_startInteraction(InteractionState *state, InteractionId id) {
	if(state->interaction.isActive) {
		logicUI_endInteraction(state);
	}

	zeroStruct(&state->interaction, ProgramInteraction);
	state->interaction.id = id;
	state->interaction.isActive = true;
	state->interaction.wasVisitedInFrame = true;
	state->interaction.shouldEnd = false;

	return &state->interaction; 

}