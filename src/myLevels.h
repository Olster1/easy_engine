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


static char *global_periodLevelStrings[] = { global_periodRoom0, 
											 global_periodRoom1,
											 global_periodRoom2,
											 global_periodRoom3 
											};



static void myLevels_generateLevel(char *level, MyEntityManager *entityManager, V3 roomPos) {
	Entity *room = initRoom(entityManager, roomPos);

	char *at = level;

	float startX = -2;
	float startY = 4;

	V2 posAt = v2(startX, startY);
	while(*at != '\0') {
		switch(*at) {
			case '*': { //empty space
				posAt.x++;
			} break;
			case '\n': { //empty space
				posAt.y--;
				posAt.x = startX;
			} break;
			case 'e': { //choc bar
				initChocBar(entityManager, findTextureAsset("choc_bar.png"), v3(posAt.x, posAt.y, LAYER0), room);
				posAt.x++;
			} break;
			case 'c': { //cramp
				initCramp(entityManager, findTextureAsset("cramp.PNG"), v3(posAt.x, posAt.y, LAYER0), room);
				posAt.x++;
			} break;
			case 'd': { //droplet
				initDroplet(entityManager, findTextureAsset("blood_droplet.PNG"), v3(posAt.x, posAt.y, LAYER0), room);
				posAt.x++;
			} break;
			case 't': { //toilet
				initBucket(entityManager, findTextureAsset("toilet1.png"), v3(posAt.x, posAt.y, LAYER0), room);
				posAt.x++;
			} break;
			default: {

			} 
		}

		at++;
	}

}