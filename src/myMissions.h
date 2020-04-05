typedef enum {
	MY_MISSION_NULL = 0,
	MY_MISSION_COLLECT_AMBER = 1 << 0,
	MY_MISSION_DESTROY_ROBOTS = 1 << 1, 
} MyMission_Flag;

typedef struct {
	MyMission_Flag flags;
	
	u32 crystalCount;
	u32 levelId; //NOTE(ollie): For robots destroyed
} MyMission_Requirements;

typedef enum {
	MY_MISSION_REWARDS_NULL = 0,
	MY_MISSION_REWARDS_UNLOCK_LEVEL = 1 << 0,
	MY_MISSION_REWARDS_AMBER = 1 << 1, 
} MyMission_RewardsFlag;

typedef struct {
	MyMission_RewardsFlag flags;
	union {
		struct {
			u32 crystalCount;
		};
		struct {
			u32 levelId_ToUnlock;
		};
	};
} MyMission_Reward;

typedef struct {
	MyMission_Requirements requirements;
	MyMission_Reward reward;

	char *title;
	char *description;

	bool complete;
} MyMission;