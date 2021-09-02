#ifndef UTIL_DEBUGSCHEDULE_H
#define UTIL_DEBUGSCHEDULE_H

// Can't tell what was even intended without seeing DEBUG_SCHEDULE from this file
#include "build_settings.h"
#include "util_shared.h"  //includes util_printout.h
#include "cbase.h"



// prototypes expected regardless of the setting, dummied if not used  (All Server only)
#if (!defined(CLIENT_DLL))
	void DebugSchedule_SchedEnum_Apply(CBaseMonster* arg_this);
	void DebugSchedule_SchedEnum_Reset(CBaseMonster* arg_this);
	void DebugSchedule_SchedEnum_ResetFinal(CBaseMonster* arg_this);
	void DebugSchedule_SchedEnum_Add(CBaseMonster* arg_this, int schedType, int entityClassEnumChoice);
	void DebugSchedule_SchedEnum_PrintFinal(CBaseMonster* arg_this);
#endif

// constant DEBUGSCHEDULE MUST be set to 1 for this to exist
// also, don't even bother if this is the client DLL.  Helpful for AI only
#if ((DEBUG_SCHEDULE == 1) && (!defined(CLIENT_DLL)))
////////////////////////////////////////////////////////////////////////////////////

// Need to be aware of CBaseMonster without including basemonster.h, that file will include here.
class CBaseMonster;


// enum for the array of classnames to use here for printout convenience,
// kept in sync with a string array in the .cpp file
enum {
	EntityClass_None = 0,
	EntityClass_CBaseEntity,
	EntityClass_CBaseMonster,
	EntityClass_CHGrunt
};
extern const char* aryEntityClass_name[];


// Be aware of these macros.
#define arySchedEnumRecord_Max 8


// What extra vars does CBaseMonster need to support this?
#define DEBUG_SCHEDULE_VARS\
	byte arySchedEnumRecord[arySchedEnumRecord_Max];\
	byte arySchedEnumRecord_classEnum[arySchedEnumRecord_Max];\
	byte arySchedEnumRecord_final[arySchedEnumRecord_Max];\
	byte arySchedEnumRecord_final_classEnum[arySchedEnumRecord_Max];\
	int arySchedEnumRecord_final_count;\
	BOOL arySchedEnumRecord_final_overflow;\
	int arySchedEnumRecord_next;\
	BOOL arySchedEnumRecord_overflow;

#define GET_SCHEDULE_ENUM_PROTOTYPE virtual const char* getScheduleEnumName(int schedType);
#define GET_TASK_ENUM_NAME_PROTOTYPE virtual const char* getTaskNumberName(int taskType);


// In any subclasses overriding the sched/task enum - name-get metods, need to start
// from their own 0 points, most likely "LAST_COMMON_whatever + 1".
#define GET_SCHEDULE_ENUM_NAME_IMPLEMENTATION(className, parentClassName, firstCustomEntry, aryName)\
const char* className::getScheduleEnumName(int schedType){\
	int schedTypeRelative = schedType - (firstCustomEntry);\
	if(schedTypeRelative >= 0 && schedTypeRelative < ARRAYSIZE(aryName)){\
		return aryName[schedTypeRelative];\
	}\
	return parentClassName::getScheduleEnumName(schedType);\
}


#define GET_TASK_ENUM_NAME_IMPLEMENTATION(className, parentClassName, firstCustomEntry, aryName)\
const char* className::getTaskNumberName(int taskType){\
	int taskTypeRelative = taskType - (firstCustomEntry);\
	if(taskTypeRelative >= 0 && taskTypeRelative < ARRAYSIZE(aryName)){\
		return aryName[taskTypeRelative];\
	}\
	return parentClassName::getTaskNumberName(taskType);\
}


////////////////////////////////////////////////////////////////////////////////////
#else
////////////////////////////////////////////////////////////////////////////////////
// no DEBUGSCHEDULE?  Dummy these
#define DEBUG_SCHEDULE_VARS
#define GET_SCHEDULE_ENUM_PROTOTYPE
#define GET_TASK_ENUM_NAME_PROTOTYPE 
#define GET_SCHEDULE_ENUM_NAME_IMPLEMENTATION(className, parentClassName, firstCustomEntry, aryName, aryCount)
#define GET_TASK_ENUM_NAME_IMPLEMENTATION(className, parentClassName, firstCustomEntry, aryName, aryCount)


////////////////////////////////////////////////////////////////////////////////////
#endif// DEBUG_SCHEDULE, !CLIENT_DLL

#endif// UTIL_DEBUGSCHEDULE_H