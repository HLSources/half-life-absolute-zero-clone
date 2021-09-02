
#include "const.h"
#include "activity.h"


// Array that mirrors the Activity enum with strings for easier debugging.
// Don't know of any other way to handle that
const char* activity_name[] =
{
	"ACT_RESET",
	"ACT_IDLE",
	"ACT_GUARD",
	"ACT_WALK",
	"ACT_RUN",
	"ACT_FLY",
	"ACT_SWIM",
	"ACT_HOP",
	"ACT_LEAP",
	"ACT_FALL",
	"ACT_LAND",
	"ACT_STRAFE_LEFT",
	"ACT_STRAFE_RIGHT",
	"ACT_ROLL_LEFT",
	"ACT_ROLL_RIGHT",
	"ACT_TURN_LEFT",
	"ACT_TURN_RIGHT",
	"ACT_CROUCH",
	"ACT_CROUCHIDLE",
	"ACT_STAND",
	"ACT_USE",
	"ACT_SIGNAL1",
	"ACT_SIGNAL2",
	"ACT_SIGNAL3",
	"ACT_TWITCH",
	"ACT_COWER",
	"ACT_SMALL_FLINCH",
	"ACT_BIG_FLINCH",
	"ACT_RANGE_ATTACK1",
	"ACT_RANGE_ATTACK2",
	"ACT_MELEE_ATTACK1",
	"ACT_MELEE_ATTACK2",
	"ACT_RELOAD",
	"ACT_ARM",
	"ACT_DISARM",
	"ACT_EAT",
	"ACT_DIESIMPLE",
	"ACT_DIEBACKWARD",
	"ACT_DIEFORWARD",
	"ACT_DIEVIOLENT",
	"ACT_BARNACLE_HIT",
	"ACT_BARNACLE_PULL",
	"ACT_BARNACLE_CHOMP",
	"ACT_BARNACLE_CHEW",
	"ACT_SLEEP",
	"ACT_INSPECT_FLOOR",
	"ACT_INSPECT_WALL",
	"ACT_IDLE_ANGRY",
	"ACT_WALK_HURT",
	"ACT_RUN_HURT",
	"ACT_HOVER",
	"ACT_GLIDE",
	"ACT_FLY_LEFT",
	"ACT_FLY_RIGHT",
	"ACT_DETECT_SCENT",
	"ACT_SNIFF",
	"ACT_BITE",
	"ACT_THREAT_DISPLAY",
	"ACT_FEAR_DISPLAY",
	"ACT_EXCITED",
	"ACT_SPECIAL_ATTACK1",
	"ACT_SPECIAL_ATTACK2",
	"ACT_COMBAT_IDLE",
	"ACT_WALK_SCARED",
	"ACT_RUN_SCARED",
	"ACT_VICTORY_DANCE",
	"ACT_DIE_HEADSHOT",
	"ACT_DIE_CHESTSHOT",
	"ACT_DIE_GUTSHOT",
	"ACT_DIE_BACKSHOT",
	"ACT_FLINCH_HEAD",
	"ACT_FLINCH_CHEST",
	"ACT_FLINCH_STOMACH",
	"ACT_FLINCH_LEFTARM",
	"ACT_FLINCH_RIGHTARM",
	"ACT_FLINCH_LEFTLEG",
	"ACT_FLINCH_RIGHTLEG",
};



const char* getActivityName(Activity arg_act){
	static char strTemp[16];
	int actAsNumber = (int)arg_act;
	if(actAsNumber >= 0 && actAsNumber < activity_count){
		return activity_name[actAsNumber];
	}
	// Give the number that could not be read then.
	// Out of range activity choices are generally not good unless they are
	// done to force an update sometimes, if being set to -1 even happens anymore?
	sprintf(&strTemp[0], "INVALID(%d)", actAsNumber);
	return strTemp;
}


