
#include "util_debugschedule.h"
#include "basemonster.h"


#if DEBUG_SCHEDULE == 1

const char* aryEntityClass_name[] =
{
	"NONE",
	"CBaseEntity",
	"CBaseMonster",
	"CHGrunt"
};


// Set arySchedEnumRecord_final_count to the current count from recnetly added
// enum's (arySchedEnumRecord_next), and reset it after.
void DebugSchedule_SchedEnum_Apply(CBaseMonster* arg_this){
	int i;

	arg_this->arySchedEnumRecord_final_count = arg_this->arySchedEnumRecord_next;

	int filteredCount;

	if(!arg_this->arySchedEnumRecord_overflow){
		filteredCount = arg_this->arySchedEnumRecord_next;
	}else{
		// clip at the max.  _final_count/_next is for recording overkill at this point.
		filteredCount = arySchedEnumRecord_Max;
	}
	arg_this->arySchedEnumRecord_final_overflow = arg_this->arySchedEnumRecord_overflow;


	for(i = 0; i < filteredCount; i++){	
		// Reset this.  GetScheduleOfType calls since last time should not
		// show up since a ChangeSchedule call.
		arg_this->arySchedEnumRecord_final[i] = arg_this->arySchedEnumRecord[i];
		arg_this->arySchedEnumRecord_final_classEnum[i] = arg_this->arySchedEnumRecord_classEnum[i];
	}

	// reset for next uses to start fresh.
	arg_this->arySchedEnumRecord_next = 0;
	arg_this->arySchedEnumRecord_overflow = FALSE;
}

// Forget everything
void DebugSchedule_SchedEnum_Reset(CBaseMonster* arg_this){
	arg_this->arySchedEnumRecord_final_count = 0;
	arg_this->arySchedEnumRecord_final_overflow = FALSE;
	arg_this->arySchedEnumRecord_next = 0;
	arg_this->arySchedEnumRecord_overflow = FALSE;
}

// Forget only the final's, for AI resets.
// Or maybe the plain ResetSchedEnum should be used for that too.
void DebugSchedule_SchedEnum_ResetFinal(CBaseMonster* arg_this){
	arg_this->arySchedEnumRecord_final_count = 0;
	arg_this->arySchedEnumRecord_final_overflow = FALSE;
}

void DebugSchedule_SchedEnum_Add(CBaseMonster* arg_this, int schedType, int entityClassEnumChoice){
	if(arg_this->arySchedEnumRecord_next < arySchedEnumRecord_Max){
		arg_this->arySchedEnumRecord[arg_this->arySchedEnumRecord_next] = schedType;
		arg_this->arySchedEnumRecord_classEnum[arg_this->arySchedEnumRecord_next] = entityClassEnumChoice;
		arg_this->arySchedEnumRecord_next++;
	}else{
		// Could shift everything backwards to lose the earliest
		// information (#0), then fill the new empty latest position.
		// OR, do that at the halfway point instead, and mark it with ellipses.
		// So we record the earliest schedule enums in the first half, latest schedule enums
		// in the last half.
		arg_this->arySchedEnumRecord_overflow = TRUE;

		int i;
		for (i = arySchedEnumRecord_Max/2 + 1; i < arySchedEnumRecord_Max; i++) {
		//for (i = arySchedEnumRecord_Max - 1; i > arySchedEnumRecord_Max/2; i--) {
			arg_this->arySchedEnumRecord[i - 1] = arg_this->arySchedEnumRecord[i];
			arg_this->arySchedEnumRecord_classEnum[i - 1] = arg_this->arySchedEnumRecord_classEnum[i];
		}

		arg_this->arySchedEnumRecord[arySchedEnumRecord_Max-1] = schedType;
		arg_this->arySchedEnumRecord_classEnum[arySchedEnumRecord_Max-1] = entityClassEnumChoice;
		
		// still count up on this to see how badly it overflowed
		arg_this->arySchedEnumRecord_next++;
	}
}


// and now for the moment we've all been waiting for.
void DebugSchedule_SchedEnum_PrintFinal(CBaseMonster* arg_this){
	easyForcePrintLine("---Schedule Enum trace---");
	if(arg_this->arySchedEnumRecord_final_count > 0){
		int i;

		if(!arg_this->arySchedEnumRecord_final_overflow){
			for(i = 0; i < arg_this->arySchedEnumRecord_final_count; i++){
				const char* theName = arg_this->getScheduleEnumName((int)arg_this->arySchedEnumRecord_final[i]);
				easyForcePrintLine("#%d: %s - %s", i, aryEntityClass_name[arg_this->arySchedEnumRecord_final_classEnum[i]], theName);
			}
		}else{
			easyForcePrintLine("WARNING! SchedEnum overflow, over %d (%d dropped)", arySchedEnumRecord_Max, arg_this->arySchedEnumRecord_final_count - arySchedEnumRecord_Max);
			for(i = 0; i < arySchedEnumRecord_Max/2; i++){
				const char* theName = arg_this->getScheduleEnumName((int)arg_this->arySchedEnumRecord_final[i]);
				easyForcePrintLine("#%d: %s - %s", i, aryEntityClass_name[arg_this->arySchedEnumRecord_final_classEnum[i]], theName);
			}
			easyForcePrintLine("---...---");
			for(i = arySchedEnumRecord_Max/2; i < arySchedEnumRecord_Max; i++){
				const char* theName = arg_this->getScheduleEnumName((int)arg_this->arySchedEnumRecord_final[i]);
				easyForcePrintLine("#%d: %s - %s", i, aryEntityClass_name[arg_this->arySchedEnumRecord_final_classEnum[i]], theName);
			}
		}
	}else{
		easyForcePrintLine("No schedule enum recorded!  Picked in an unusual way");
	}
	easyForcePrintLine("------------------------");
}



#else
// No DEBUG_SCHEDULE?  Dummy ahoy

void DebugSchedule_SchedEnum_Apply(CBaseMonster* arg_this){}
void DebugSchedule_SchedEnum_Reset(CBaseMonster* arg_this){}
void DebugSchedule_SchedEnum_ResetFinal(CBaseMonster* arg_this){}
void DebugSchedule_SchedEnum_Add(CBaseMonster* arg_this, int schedType, int entityClassEnumChoice){}
void DebugSchedule_SchedEnum_PrintFinal(CBaseMonster* arg_this){}

#endif







// may as well handle CbaseMonster's extra schedule debug methods in here
#if DEBUG_SCHEDULE == 1


#define CBaseMonster_ScheduleEnum_count 64

const char* CBaseMonster_ScheduleEnum_name[] = 
{
	"SCHED_NONE",
	"SCHED_IDLE_STAND",
	"SCHED_IDLE_WALK",
	"SCHED_WAKE_ANGRY",
	"SCHED_WAKE_CALLED",
	"SCHED_ALERT_FACE",
	"SCHED_ALERT_FACE_IF_VISIBLE",
	"SCHED_ALERT_SMALL_FLINCH",
	"SCHED_ALERT_BIG_FLINCH",
	"SCHED_ALERT_STAND",
	"SCHED_INVESTIGATE_SOUND",
	"SCHED_COMBAT_FACE",
	"SCHED_COMBAT_FACE_NOSTUMP",
	"SCHED_COMBAT_LOOK",
	"SCHED_WAIT_FOR_ENEMY_TO_ENTER_WATER",
	"SCHED_COMBAT_STAND",
	"SCHED_CHASE_ENEMY",
	"SCHED_CHASE_ENEMY_STOP_SIGHT",
	"SCHED_CHASE_ENEMY_SMART",
	"SCHED_CHASE_ENEMY_SMART_STOP_SIGHT",
	"SCHED_CHASE_ENEMY_FAILED",
	"SCHED_VICTORY_DANCE",
	"SCHED_TARGET_FACE",
	"SCHED_TARGET_CHASE",
	"SCHED_SMALL_FLINCH",
	"SCHED_BIG_FLINCH",
	"SCHED_TAKE_COVER_FROM_ENEMY",
	"SCHED_TAKE_COVER_FROM_BEST_SOUND",
	"SCHED_TAKE_COVER_FROM_ORIGIN",
	"SCHED_TAKE_COVER_FROM_ORIGIN_WALK",
	"SCHED_MOVE_FROM_ORIGIN",
	"SCHED_COWER",
	"SCHED_MELEE_ATTACK1",
	"SCHED_MELEE_ATTACK2",
	"SCHED_RANGE_ATTACK1",
	"SCHED_RANGE_ATTACK2",
	"SCHED_SPECIAL_ATTACK1",
	"SCHED_SPECIAL_ATTACK2",
	"SCHED_STANDOFF",
	"SCHED_ARM_WEAPON",
	"SCHED_RELOAD",
	"SCHED_GUARD",
	"SCHED_AMBUSH",
	"SCHED_DIE",
	"SCHED_DIE_LOOP",
	"SCHED_DIE_FALL_LOOP",
	"SCHED_WAIT_TRIGGER",
	"SCHED_FOLLOW",
	"SCHED_SLEEP",
	"SCHED_WAKE",
	"SCHED_BARNACLE_VICTIM_GRAB",
	"SCHED_BARNACLE_VICTIM_CHOMP",
	"SCHED_AISCRIPT",
	"SCHED_FAIL",
	"SCHED_FAIL_QUICK",
	"SCHED_INVESTIGATE_SOUND_BAIT",
	"SCHED_CANT_FOLLOW_BAIT",
	"SCHED_RANDOMWANDER",
	"SCHED_RANDOMWANDER_UNINTERRUPTABLE",
	"SCHED_FIGHT_OR_FLIGHT",
	"SCHED_TAKE_COVER_FROM_ENEMY_OR_CHASE",
	"SCHED_PATHFIND_STUMPED",
	"SCHED_PATHFIND_STUMPED_LOOK_AT_PREV_LKP",
	"SCHED_WALK_TO_POINT"

	// going to be replaced by whatever subclass anyway
	//LAST_COMMON_SCHEDULE
};




#define CBaseMonster_TaskEnum_count 127

const char* CBaseMonster_TaskEnum_name[] = 
{
	"TASK_INVALID",
	"TASK_WAIT",
	"TASK_WAIT_FACE_IDEAL",
	"TASK_WAIT_FACE_ENEMY",
	"TASK_WAIT_PVS",
	"TASK_SUGGEST_STATE",
	"TASK_WALK_TO_TARGET",
	"TASK_RUN_TO_TARGET",
	"TASK_MOVE_TO_TARGET_RANGE",
	"TASK_MOVE_TO_ENEMY_RANGE",
	"TASK_GET_PATH_TO_ENEMY",
	"TASK_GET_PATH_TO_ENEMY_LKP",
	"TASK_GET_PATH_TO_ENEMY_CORPSE",
	"TASK_GET_PATH_TO_LEADER",
	"TASK_GET_PATH_TO_TARGET",
	"TASK_GET_PATH_TO_HINTNODE",
	"TASK_GET_PATH_TO_LASTPOSITION",
	"TASK_GET_PATH_TO_BESTSOUND",
	"TASK_GET_PATH_TO_BESTSCENT",
	"TASK_RUN_PATH",
	"TASK_WALK_PATH",
	"TASK_STRAFE_PATH",
	"TASK_CLEAR_MOVE_WAIT",
	"TASK_STORE_LASTPOSITION",
	"TASK_CLEAR_LASTPOSITION",
	"TASK_PLAY_ACTIVE_IDLE",
	"TASK_FIND_HINTNODE",
	"TASK_CLEAR_HINTNODE",
	"TASK_SMALL_FLINCH",
	"TASK_BIG_FLINCH",
	"TASK_FACE_IDEAL",
	"TASK_FACE_IDEAL_IF_VISIBLE",
	"TASK_FACE_ROUTE",
	"TASK_FACE_ENEMY",
	"TASK_FACE_HINTNODE",
	"TASK_FACE_TARGET",
	"TASK_FACE_LASTPOSITION",
	"TASK_FACE_GOAL",
	"TASK_RANGE_ATTACK1",
	"TASK_RANGE_ATTACK2",
	"TASK_MELEE_ATTACK1",
	"TASK_MELEE_ATTACK2",
	"TASK_RELOAD",
	"TASK_RANGE_ATTACK1_NOTURN",
	"TASK_RANGE_ATTACK2_NOTURN",
	"TASK_MELEE_ATTACK1_NOTURN",
	"TASK_MELEE_ATTACK2_NOTURN",
	"TASK_RELOAD_NOTURN",
	"TASK_SPECIAL_ATTACK1",
	"TASK_SPECIAL_ATTACK2",
	"TASK_CROUCH",
	"TASK_STAND",
	"TASK_GUARD",
	"TASK_STEP_LEFT",
	"TASK_STEP_RIGHT",
	"TASK_STEP_FORWARD",
	"TASK_STEP_BACK",
	"TASK_DODGE_LEFT",
	"TASK_DODGE_RIGHT",
	"TASK_SOUND_ANGRY",
	"TASK_SOUND_DEATH",
	"TASK_SET_ACTIVITY",
	"TASK_SET_ACTIVITY_FORCE",
	"TASK_SET_SCHEDULE",
	"TASK_SET_FAIL_SCHEDULE",
	"TASK_CLEAR_FAIL_SCHEDULE",
	"TASK_PLAY_SEQUENCE",
	"TASK_PLAY_SEQUENCE_FACE_ENEMY",
	"TASK_PLAY_SEQUENCE_FACE_TARGET",
	"TASK_SOUND_IDLE",
	"TASK_SOUND_WAKE",
	"TASK_SOUND_PAIN",
	"TASK_SOUND_DIE",
	"TASK_FIND_COVER_FROM_BEST_SOUND",
	"TASK_FIND_COVER_FROM_ENEMY",
	"TASK_FIND_LATERAL_COVER_FROM_ENEMY",
	"TASK_FIND_NODE_COVER_FROM_ENEMY",
	"TASK_FIND_NEAR_NODE_COVER_FROM_ENEMY",
	"TASK_FIND_FAR_NODE_COVER_FROM_ENEMY",
	"TASK_FIND_COVER_FROM_ORIGIN",
	"TASK_MOVE_FROM_ORIGIN",
	"TASK_EAT",
	"TASK_DIE",
	"TASK_DIE_SIMPLE",
	"TASK_DIE_LOOP",
	"TASK_SET_FALL_DEAD_TOUCH",
	"TASK_WAIT_FOR_SCRIPT",
	"TASK_PLAY_SCRIPT",
	"TASK_ENABLE_SCRIPT",
	"TASK_PLANT_ON_SCRIPT",
	"TASK_FACE_SCRIPT",
	"TASK_WAIT_RANDOM",
	"TASK_WAIT_INDEFINITE",
	"TASK_WAIT_ENEMY_LOOSE_SIGHT",
	"TASK_WAIT_ENEMY_ENTER_WATER",
	"TASK_STOP_MOVING",
	"TASK_TURN_LEFT",
	"TASK_TURN_RIGHT",
	"TASK_TURN_LEFT_FORCE_ACT",
	"TASK_TURN_RIGHT_FORCE_ACT",
	"TASK_REMEMBER",
	"TASK_FORGET",
	"TASK_WAIT_FOR_MOVEMENT",
	"TASK_WAIT_FOR_MOVEMENT_TIMED",
	"TASK_SET_SEQUENCE_BY_NAME",
	"TASK_WAIT_FOR_MOVEMENT_RANGE",
	"TASK_WAIT_FOR_MOVEMENT_GOAL_IN_SIGHT",
	"TASK_SET_SEQUENCE_BY_NUMBER",
	"TASK_WAIT_FOR_SEQUENCEFINISH",
	"TASK_RESTORE_FRAMERATE",
	"TASK_MOVE_TO_POINT_RANGE",
	"TASK_FACE_POINT",
	"TASK_RANDOMWANDER_CHECKSEEKSHORT",
	"TASK_RANDOMWANDER_TEST",
	"TASK_FIND_COVER_FROM_ENEMY_OR_CHASE",
	"TASK_FIND_COVER_FROM_ENEMY_OR_FIGHT",
	"TASK_CHECK_STUMPED",
	"TASK_FACE_PREV_LKP",
	"TASK_WAIT_STUMPED",
	"TASK_SET_FAIL_SCHEDULE_HARD",
	"TASK_CHECK_RANGED_ATTACK_1",
	"TASK_FACE_BEST_SOUND",
	"TASK_UPDATE_LKP",
	"TASK_WATER_DEAD_FLOAT",
	"TASK_GATE_ORGANICLOGIC_NEAR_LKP",
	"TASK_RECORD_DEATH_STATS",
	"TASK_GET_PATH_TO_GOALVEC"

	// going to be replaced by whatever subclass anyway
	//LAST_COMMON_TASK
};


// Subclasses can use macros to create these instead (see util_debugschedule.h)
const char*CBaseMonster::getScheduleEnumName(int schedType){
	static char strTemp[16];
	if(schedType >= 0 && schedType < CBaseMonster_ScheduleEnum_count){
		return  CBaseMonster_ScheduleEnum_name[schedType];
	}
	sprintf(&strTemp[0], "INVALID(%d)", schedType);
	return strTemp;
}
const char* CBaseMonster::getTaskNumberName(int taskType){
	static char strTemp[16];
	if(taskType >= 0 && taskType < CBaseMonster_TaskEnum_count){
		return  CBaseMonster_TaskEnum_name[taskType];
	}
	sprintf(&strTemp[0], "INVALID(%d)", taskType);
	return strTemp;
}

#endif
