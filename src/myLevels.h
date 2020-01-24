
static char *global_periodRoom0 = "\
**d**\n\
**d**\n\
**d**\n\
**d**\n\
*****\n\
";

static char *global_periodRoom1 = "\
c** d*\n\
t***d*\n\
*** d*\n\
e** d*\n\
*****\n\
";

static char *global_periodRoom2 = "\
*** d*\n\
*c****\n\
c** c*\n\
e** d*\n\
*****\n\
";

static char *global_periodRoom3 = "\
*** d*\n\
*c****\n\
c** c*\n\
e** d*\n\
*****\n\
";

static char *global_periodRoom4 = "\
*****\n\
s*d*s\n\
s*c*s\n\
s*d*s\n\
*****\n\
";

static char *global_periodRoom5 = "\
**s**\n\
s**s*\n\
ss**s\n\
sss*s\n\
*****\n\
";


static char *global_periodLevelStrings[] = { global_periodRoom0, 
											 global_periodRoom1,
											 global_periodRoom2,
											 global_periodRoom3,
											 global_periodRoom4,
											 global_periodRoom5  
											};

static inline int myLevels_getLevel(int lastLevel) {
	int r = (int)randomBetween(0, arrayCount(global_periodLevelStrings));

	if(r >= arrayCount(global_periodLevelStrings)) r -= 1;

	return r;
}

static inline char *myLevels_getLevelForIndex(int i) {
	assert(i >= 0 && i < arrayCount(global_periodLevelStrings));
	return global_periodLevelStrings[i];
}

