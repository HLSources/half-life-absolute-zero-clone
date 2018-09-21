

#include "stukabat.h"


#include "studio.h"

#include "player.h"



EASY_CVAR_EXTERN(animationFramerateMulti)

extern float global_drawDebugPathfinding2;
extern float global_stukaAdvancedCombat;

extern float global_STUSpeedMulti;
extern float global_STUExplodeTest;
extern float global_STUYawSpeedMulti;
extern float global_STUDetection;

extern float global_shutupstuka;

extern float global_STUrepelMulti;
extern float global_STUcheckDistH;
extern float global_STUcheckDistV;
extern float global_STUcheckDistD;

extern float global_stukaPrintout;

extern float global_stukaInflictsBleeding;


//Stuka Bat's implementation.  Separate for organization.

//=========================================================
// Classify - indicates this monster's place in the 
// relationship table.
//=========================================================
int	CStukaBat :: Classify ( void )
{
	//would something more animal-related, like "prey" or "monster" make more sense here?
	//return CLASS_ALIEN_MILITARY;


	//return CLASS_ALIEN_PASSIVE;

	//note that stukabats are not supposed to attack the player unless provoked (I imagine the entire squad / nearby Stukas should also get provoked).
	//For now, using MONSTER to make it hostile to the player for testing aggressive AI
	return CLASS_ALIEN_MONSTER;

}



//=========================================================
// SetYawSpeed - allows each sequence to have a different
// turn rate associated with it.
//=========================================================
//(for better or worse, Controller's SetYawSpeed method came almost blank too).
void CStukaBat :: SetYawSpeed ( void )
{
	int ys;

	ys = 120;

	/*
	switch ( m_Activity )
	{

	}
	*/

	if(m_Activity == ACT_IDLE){
		ys=50;
	}
	if(m_Activity == ACT_WALK){
		ys=80;
	}
	if(m_Activity == ACT_FLY){
		ys=60;
	}
	if(m_Activity == ACT_EAT || m_Activity == ACT_CROUCHIDLE){
		//ys=260/0.88;
		//.... what
		ys = 140;
	}
	if(m_Activity == ACT_HOVER){
		ys=90;
	}

	EASY_CVAR_PRINTIF_PRE(stukaPrintout, easyPrintLine( "ACT SON!!!!! %d", m_Activity) );
	
	//consistent mod.
	ys *= 0.88;

	pev->yaw_speed = ys;
}


float CStukaBat::getMeleeAnimScaler(void){

	switch(g_iSkillLevel){
	case SKILL_EASY:
		return 1.08;
	break;
	case SKILL_MEDIUM:
		return 1.12;
	break;
	case SKILL_HARD:
		return 1.17;
	break;
	default:
		//easy?
		return 1.08;
	break;
	}
}


float CStukaBat::getAttackDelay(void){

	switch(g_iSkillLevel){
	case SKILL_EASY:
		return RANDOM_FLOAT(1.9, 2.7);
	break;
	case SKILL_MEDIUM:
		return RANDOM_FLOAT(1.4, 2.1);
	break;
	case SKILL_HARD:
		return RANDOM_FLOAT(0.8, 1.6);
	break;
	default:
		//easy?
		return RANDOM_FLOAT(1.9, 2.7);
	break;
	}
}



GENERATE_TRACEATTACK_IMPLEMENTATION(CStukaBat)
{
	GENERATE_TRACEATTACK_PARENT_CALL(CSquadMonster);
}

GENERATE_TAKEDAMAGE_IMPLEMENTATION(CStukaBat)
{
	// HACK HACK -- until we fix this.
	//NOTE: perhaps " if(pev->deadFlag == DEAD_NO) " would be more precise?
	//if ( IsAlive() )
	if(pev->deadflag == DEAD_NO){
		PainSound();
	}
	m_afMemory |= bits_MEMORY_PROVOKED;


	
	PRINTQUEUE_STUKA_SEND(stukaPrint.tookDamage, "TOOK DMG");

	//...also, why is this "CBaseMonster" and not "CSquadMonster", the direct parent of Controller OR StukaBat?  Ah well, I guess both work technically (CSquadMonster doesn't really affect how takeDamage works)
	//probably just doesn't matter.
	return GENERATE_TAKEDAMAGE_PARENT_CALL(CSquadMonster);
}



GENERATE_KILLED_IMPLEMENTATION(CStukaBat)
{

	//forget everything.
	blockSetActivity = -1;
	attackEffectDelay = -1;
	maxDiveTime = -1;

	queueAbortAttack = FALSE;

	chargeIndex = -1;

	queueToggleGround = FALSE;
	snappedToCeiling = FALSE;
	queueToggleSnappedToCeiling = FALSE;



	GENERATE_KILLED_PARENT_CALL(CSquadMonster);

	//just in case?
	pev->movetype = MOVETYPE_TOSS;

}

GENERATE_GIBMONSTER_IMPLEMENTATION_ROUTETOPARENT(CStukaBat, CSquadMonster)


void CStukaBat :: PainSound( void )
{
	if(global_shutupstuka != 1){
		//NOTE: lifted from "Controller".  Is a 1/3 chance of pain okay?
		if (RANDOM_LONG(0,5) < 2)
			EMIT_SOUND_ARRAY_STUKA_FILTERED( CHAN_VOICE, pPainSounds ); 
	}
}	





void CStukaBat::tryDetachFromCeiling(void){
	
	if(!onGround && snappedToCeiling && blockSetActivity == -1){

		//setAnimation("Take_off_from_ceiling", TRUE, FALSE, 2);
		m_flFramerateSuggestion = -1;
		
		setAnimationSmart("Land_ceiling", -1);
		m_flFramerateSuggestion = 1;

		blockSetActivity = gpGlobals->time + (31.2f/12.0f);
		
		queueToggleSnappedToCeiling = TRUE;
	}
}



void CStukaBat :: MakeIdealYaw( Vector vecTarget )
{
	
	CSquadMonster::MakeIdealYaw(vecTarget);

}
//=========================================================
// Changeyaw - turns a monster towards its ideal_yaw
//=========================================================
float CStukaBat::ChangeYaw ( int yawSpeed )
{

	//if alert?

	EASY_CVAR_PRINTIF_PRE(stukaPrintout, easyPrintLine("ENEMY??? A %d %d %d", m_pSchedule != slStukaBatAnimWait, this->onGround, this->m_MonsterState));

	if(m_hEnemy != NULL && m_pSchedule != slStukaBatAnimWait && this->onGround == 1 && (m_MonsterState == MONSTERSTATE_ALERT || m_MonsterState == MONSTERSTATE_COMBAT) && !seekingFoodOnGround ){
		//we can start flying...?

		EASY_CVAR_PRINTIF_PRE(stukaPrintout, easyPrintLine("ENEMY??? B %d", m_hEnemy != NULL, this->HasConditions(bits_COND_ENEMY_OCCLUDED), this->HasConditions(bits_COND_NEW_ENEMY), this->HasConditions(bits_COND_SEE_ENEMY)));

		//MODDD - TEST: or any "combat face" could do this too, try that if this isn't good!
		//SCHED_COMBAT_FACE ,  or TASK_FACE_COMBAT    something?  I dunno.
		BOOL passed = FALSE;
		if(!this->HasConditions(bits_COND_ENEMY_OCCLUDED) || this->HasConditions(bits_COND_NEW_ENEMY), this->HasConditions(bits_COND_SEE_ENEMY) ){
			passed = TRUE;
			//okay.
		}else{

		}
		
		//Test this, make sure we don't get up in the middle of looking for food.
		setAnimation("Take_off_from_land", TRUE, FALSE, 2);
		//41.6f/12.0f ???
		blockSetActivity = gpGlobals->time + (29.0f/12.0f);
		queueToggleGround = TRUE;

		RouteClear();
		//return GetScheduleOfType(SCHED_STUKABAT_ANIMWAIT);
		this->ChangeSchedule(slStukaBatAnimWait);
		
	}

	if(snappedToCeiling){

		if( m_afMemory & bits_MEMORY_PROVOKED){
			//Get off!
			tryDetachFromCeiling();
		}else{
			//don't care.
		}

		return 0;  //cannot rotate on the ceiling, do NOT go up the method heirarchy (actually turn).
	}else{
		//we can go.
	}

	return CSquadMonster::ChangeYaw(yawSpeed);
}



void CStukaBat :: ForceMakeIdealYaw( Vector vecTarget )
{
	
	CSquadMonster::MakeIdealYaw(vecTarget);

}
float CStukaBat::ForceChangeYaw ( int yawSpeed )
{


	return CSquadMonster::ChangeYaw(yawSpeed);
}





//NOTE: should hgrunts / hassaults also have methods similar to "callforelp" (alert nearby AI of
//the presence of a thread) so that they alert both hgrunts and hassaults nearby, not just those
//of their exact own class (if it works by using "netname" like this)?
void CStukaBat :: CallForHelp( char *szClassname, float flDist, EHANDLE hEnemy, Vector &vecLocation )
{
	
	if ( (!snappedToCeiling  ) ){
		return;
	}
	
	CBaseEntity *pEntity = NULL;

	//Netname?  Map only?  yay that yay.

	//while ((pEntity = UTIL_FindEntityByString( pEntity, "netname", STRING( pev->netname ))) != NULL)
	
	while ((pEntity = UTIL_FindEntityByClassname( pEntity, STRING( pev->classname ))) != NULL)
	{
		float d = (pev->origin - pEntity->pev->origin).Length();
		if (d < flDist)
		{
			
			CBaseMonster *pMonster = pEntity->MyMonsterPointer( );
			if (pMonster)
			{
				pMonster->m_afMemory |= bits_MEMORY_PROVOKED;
				pMonster->PushEnemy( hEnemy, vecLocation );
				//EASY_CVAR_PRINTIF_PRE(stukaPrintout, easyPrintLine("IM A jolly fellow!!!! %s", STRING(pev->classname) ) );
				if(FClassnameIs(pMonster->pev, "monster_stukabat")){
					//???
					//CStukaBat* tempStuka = (CStukaBat*)(pMonster);
					CStukaBat* tempStuka = static_cast<CStukaBat*>(pMonster);
					if(tempStuka->snappedToCeiling == TRUE){
						tempStuka->wakeUp = TRUE;

						//Wait.  Why not just unhinge right now?
						//MODDD - TESTING: is this okay???
						tempStuka->m_afMemory &= bits_MEMORY_PROVOKED;
						tempStuka->tryDetachFromCeiling();

					}

				}

			}
		}
	}
}





void CStukaBat :: AlertSound( void )
{
	//EASY_CVAR_PRINTIF_PRE(stukaPrintout, easyPrintLine("YOU SNEEKY errr!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!! %d", m_hEnemy==NULL) );
	if ( m_hEnemy != NULL )
	{
		//SENTENCEG_PlayRndSz(ENT(pev), "SLV_ALERT", 0.85, ATTN_NORM, 0, m_voicePitch);
		CallForHelp( "monster_stukabat", 512, m_hEnemy, m_vecEnemyLKP );
	}

	
	if(global_shutupstuka != 1){
	EMIT_SOUND_ARRAY_STUKA_FILTERED( CHAN_VOICE, pAlertSounds );
	}
}

void CStukaBat :: IdleSound( void )
{
	if(global_shutupstuka != 1){
	EMIT_SOUND_ARRAY_STUKA_FILTERED( CHAN_VOICE, pIdleSounds ); 
	}
}

void CStukaBat :: AttackSound( void )
{
	if(global_shutupstuka != 1){
	EMIT_SOUND_ARRAY_STUKA_FILTERED( CHAN_VOICE, pAttackSounds ); 
	}
}

void CStukaBat :: DeathSound( void )
{
	if(global_shutupstuka != 1){
	EMIT_SOUND_ARRAY_STUKA_FILTERED( CHAN_VOICE, pDeathSounds ); 
	}
}





void CStukaBat :: KeyValue( KeyValueData *pkvd )
{
	/*
	if (FStrEq(pkvd->szKeyName, "TriggerTarget"))
	{
		m_iszTriggerTarget = ALLOC_STRING( pkvd->szValue );
		pkvd->fHandled = TRUE;
	}
	else if (FStrEq(pkvd->szKeyName, "TriggerCondition") )
	{
		m_iTriggerCondition = atoi( pkvd->szValue );
		pkvd->fHandled = TRUE;
	}
	*/

	//var.
	//0 = automatic (default).
	//1 = on ground.
	//2 = midair
	//3 = on the ceiling.
	
	/*
	//NOTICE: we will use the flag "onGround" instead.
	if (FStrEq(pkvd->szKeyName, "spawnLoc")){
		m_iSpawnLoc = atoi( pkvd->szValue );
		pkvd->fHandled = TRUE;
	}else{
		CBaseMonster::KeyValue( pkvd );
	}
	*/

	CBaseMonster::KeyValue( pkvd );
}



//=========================================================
// HandleAnimEvent - catches the monster-specific messages
// that occur when tagged animation frames are played.
//=========================================================
void CStukaBat :: HandleAnimEvent( MonsterEvent_t *pEvent )
{
	//easyPrintLineGroup2("STUKABAT ANIM: %d", pEvent);

	switch( pEvent->event )
	{
		/*
		case STUKABAT_SOMEEVENT
		{

		}
		*/
		//break;
		default:
			CBaseMonster::HandleAnimEvent( pEvent );
		break;
	}//END OF switch(...)
	
}


CStukaBat::CStukaBat() : stukaPrint(StukaPrintQueueManager("STUKA")){

	recentActivity = ACT_RESET;

	attackIndex = -1;
	//-2 = still in delay b/w attacks (can still follow).
	//-1 = not attacking, ready when player is close enough.
	//0 = did "drill_cycle" or whatever, approaching player fast.
	//1 = delivering attack (claw or suicide).
	//2 = backing off (if did claw)

	attackEffectDelay = -1;
	attackAgainDelay = 0;
	maxDiveTime = -1;

	//my ID is the globalID.  Then, bump it (next bat created gets the next ID in line)
	//stukaID = globalStukaID;
	////!!! NO, using "monsterID" now.
	//globalStukaID++;

	timeToIdle = -1;

	combatCloseEnough = FALSE;

	//is that okay?
	//m_iSpawnLoc = 0;

	tempCheckTraceLineBlock = FALSE;

	//WARNING: verify loading a save does NOT cause this value to take precedence (IE, appearing w/o spawn on load making it force "false", even if the loaded game clearly wants "true").
	onGround = FALSE;  //assume false?   Determine more reasonably at spawn.


	blockSetActivity = -1;

	//Activity lastSetActivitySetting;
	//BOOL lastSetActivityforceReset;
	queueToggleGround = FALSE;
	snappedToCeiling = FALSE;
	queueToggleSnappedToCeiling = FALSE;

	queueActionIndex = -1;

	queueAbortAttack = FALSE;


	//set to creation time.  That's okay, right?
	//lastVelocityChange = gpGlobals->time;
	//be safe...
	lastVelocityChange = -1;


	moveFlyNoInterrupt = -1;
	dontResetActivity = FALSE;
	//seekingFoodOnGround = FALSE;

	chargeIndex = -1;

	wakeUp = FALSE;

	//turn off the bottom-auto-ground-push-sensors.
	turnThatOff = FALSE;


	landBrake = FALSE;

}



void CStukaBat::heardBulletHit(entvars_t* pevShooter){

	//TRIGGERED
	wakeUp = TRUE;
	m_afMemory |= bits_MEMORY_PROVOKED;

	//calling the usual routine of basemonster is okay?
	CSquadMonster::heardBulletHit(pevShooter);
}


BOOL CStukaBat::isProvokable(void){
	//ordinarily, yes.
	return TRUE;
}
BOOL CStukaBat::isProvoked(void){
	
	return (m_afMemory & bits_MEMORY_PROVOKED);
}


int CStukaBat::IRelationship( CBaseEntity *pTarget )
{

	BOOL isPlayerAlly = FALSE;
	BOOL potentialNeutral = FALSE;
	//if(FClassnameIs(pTarget->pev, "monster_scientist") || FClassnameIs(pTarget->pev, "monster_sitting_scientist") || FClassnameIs(pTarget->pev, "monster_barney") ){
	if(pTarget->isTalkMonster()){
		//all talk monsters are (usually) the player's friend, and should not alert Stukabats.
		isPlayerAlly = TRUE;
	}

	if ( (pTarget->IsPlayer()) || isPlayerAlly ) {
		potentialNeutral = TRUE;
	}
	
	//chance to disregard as not a threat, or fail to notice (like hanging, "sleeping").
	if(potentialNeutral && !(m_afMemory & bits_MEMORY_PROVOKED ) ){


		if(!snappedToCeiling || wakeUp == TRUE){
			switch( (int) global_STUDetection){
			case 0:
				//good enough, something serious needs to provoke me.
				return R_NO;
			break;
			case 1:
				//provoke if in line of sight.
			
				if(FInViewCone( pTarget )){
					m_afMemory |= bits_MEMORY_PROVOKED;
				}else{
					return R_NO;
				}

			break;
			case 2:
				m_afMemory |= bits_MEMORY_PROVOKED;
				//pass through.  Provoke as usual.
			break;
			default:
				//???
			break;
			}

		}else{
			//if snapped, always leniant.
			return R_NO;
		}


	}//END OF if(potentialNeutral && !(m_afMemory & bits_MEMORY_PROVOKED ) )


	/*
	if ( (pTarget->IsPlayer()) || isPlayerAlly)
		if ( (TRUE ) && ! (m_afMemory & bits_MEMORY_PROVOKED ))
			return R_NO;
	*/
	return CBaseMonster::IRelationship( pTarget );
}


//=========================================================
// Spawn
//=========================================================
void CStukaBat :: Spawn()
{
	Precache( );

	SET_MODEL(ENT(pev), "models/stukabat.mdl");

	//is this box okay?
	//CHECK EM

	//UTIL_printLineVector( "PROJECTION", UTIL_projectionComponent(Vector(13, -34, 21), Vector(-28, 10, -18) ) );

	UTIL_SetSize( pev, Vector( -8, -8, 0 ), Vector( 8, 8, 28 ));
	//UTIL_SetSize( pev, Vector( -34, -34, 0 ), Vector( 34, 34, 48 ));

	pev->classname = MAKE_STRING("monster_stukabat");

	pev->solid			= SOLID_SLIDEBOX;  //SOLID_TRIGGER?  Difference?
	pev->movetype		= MOVETYPE_FLY;

	pev->movetype		= MOVETYPE_BOUNCEMISSILE;
	
	pev->flags			|= FL_FLY;
	m_bloodColor		= BLOOD_COLOR_GREEN;

	//easyPrintLineGroup2("STUAK HEALTH: %.2f", gSkillData.stukaBatHealth);
	//CHANGENOTICE: stuka bat probably has a different amount of health.  Make vars to scale with difficulty too?
	pev->health			= gSkillData.stukaBatHealth;
	EASY_CVAR_PRINTIF_PRE(stukaPrintout, easyPrintLine("HEALTH BE %.2f", gSkillData.stukaBatHealth) );

	//CHANGENOTICE: check the model and change if needed.
	pev->view_ofs		= Vector( 0, 0, 15 );// position of the eyes relative to monster's origin.

	//makes the eye offset (view_ofs) more obvious at spawn.
	//UTIL_drawLine(pev->origin.x - 10, pev->origin.y, pev->origin.z + 15, pev->origin.x + 10, pev->origin.y, pev->origin.z + 15);

	//why is this not on by default in this case?   NO, they are "ranged" attacks instead for whatever reason.
	//m_afCapability |= bits_CAP_MELEE_ATTACK1;

	m_afCapability		= bits_CAP_SQUAD;

	m_voicePitch = randomValueInt(101, 107);
	
	//TEST
	//pev->spawnflags &= ~ SF_SQUADMONSTER_LEADER;


	m_flFieldOfView		= VIEW_FIELD_FULL;// indicates the width of this monster's forward view cone ( as a dotproduct result )
	m_MonsterState		= MONSTERSTATE_NONE;

	MonsterInit();


	this->SetTouch(&CStukaBat::customTouch);
	

	checkStartSnap();

}

void CStukaBat::checkStartSnap(){
	
	TraceResult trDown;
	TraceResult trUp;
	Vector vecStart = pev->origin;
	Vector vecEnd;
	
	float distFromUp = -1;
	float distFromDown = -1;

	BOOL certainOfLoc = FALSE;
	
	//BOOL spawnedDynamically = pev->spawnflags & SF_MONSTER_DYNAMICSPAWN;
	//~obsolete, now an instance var of all "CBaseEntity" (falls to this class) instead,
	// and is handled appropriately before this point.

	onGround = FALSE;
	//can be proven wrong (make "TRUE").


	if(spawnedDynamically && !flagForced){

		//default; find the best way.
		m_iSpawnLoc = 0;
	}else{
		if(!(pev->spawnflags & SF_MONSTER_STUKA_ONGROUND) ){
			m_iSpawnLoc = 3;
		}else{
			m_iSpawnLoc = 1;
		}
	}

	if(m_iSpawnLoc == 0){
		//Auto-choose if the player spawned me (cheats).  Need to determine whether to snap to the ground, ceiling, or nothing (if neither is close)
		//this time, "ignore_monsters" as opposed to "dont_ignore_monsters".
		vecEnd = vecStart + Vector(0, 0, 8);
		UTIL_TraceLine(vecStart, vecEnd, ignore_monsters, ENT(pev), &trUp);

		vecEnd = vecStart + Vector(0, 0, -8);
		UTIL_TraceLine(vecStart, vecEnd, ignore_monsters, ENT(pev), &trDown);

		distFromUp = -1;
		distFromDown = -1;
	
		//actually hit something?
		if(trUp.flFraction < 1.0){
			distFromUp = (vecStart - trUp.vecEndPos).Length();
		}
		if(trDown.flFraction < 1.0){
			distFromDown = (vecStart - trDown.vecEndPos).Length();
		}
		//don't re-do the distance-check logic for any choice down the road.
		certainOfLoc = TRUE;
		easyPrintLineGroup2("STUKA SPAWN VERTICAL DIST: from up: %.2f | from down: %.2f", distFromUp, distFromDown);

		if(distFromUp == -1 && distFromDown == -1){
			//cannot snap  to either surface in a short distance.  Giving up; hover mid-air at spawn.
			m_iSpawnLoc = 2;
		}else if(distFromUp == -1){
			m_iSpawnLoc = 1;
		}else if(distFromDown == -1){
			m_iSpawnLoc = 3;
		}else if(distFromDown <= distFromUp){
			m_iSpawnLoc = 1;
		}else{
			m_iSpawnLoc = 3;
		}
	}//END OF if m_iSpawnLoc == 0)
	if(m_iSpawnLoc == 1){
		//ground, snap to bottom.
		if(!certainOfLoc){
			//check for the ground.
			vecEnd = vecStart + Vector(0, 0, -800);
			UTIL_TraceLine(vecStart, vecEnd, ignore_monsters, ENT(pev), &trDown);

			if(trDown.flFraction < 1.0){
				certainOfLoc = true;
			}
		}
		if(certainOfLoc){
			canSetAnim = FALSE;
			SetActivity(ACT_CROUCHIDLE, TRUE );   //idle for ground.
			easyPrintLineGroup2("BUUUUUUUUUUUUUUUUUUUUUUUUUUUUUUUUUUUUUUUUUUUUUUUUUUUUUU");

			onGround = TRUE;
			pev->movetype = MOVETYPE_STEP;
			pev->flags &= ~FL_FLY;


			pev->origin = trDown.vecEndPos;
		}else{
			//give up.
			m_iSpawnLoc = 2;
		}
	}else if(m_iSpawnLoc == 3){
		//ceiling, snap to top

		if(!certainOfLoc){
			//check for the ceiling.
			vecEnd = vecStart + Vector(0, 0, 800);
			UTIL_TraceLine(vecStart, vecEnd, ignore_monsters, ENT(pev), &trUp);

			if(trUp.flFraction < 1.0){
				certainOfLoc = true;
			}
		}
		if(certainOfLoc){
			snappedToCeiling = TRUE;  //allow the stuka to start from the ceiling.
			SetActivity(ACT_IDLE);  //hanging from the ceiling.
			pev->origin = trUp.vecEndPos + Vector(0, 0, -33);
		}else{
			//give up.
			m_iSpawnLoc = 2;
		}
	}
	else if(m_iSpawnLoc == 2){
		//mid-air, no snapping.  Essentialy a "give-up" value on either of the above failing too.

		SetActivity(ACT_HOVER);  //correct.
		//setAnimation("Hover");

		if(!(pev->spawnflags & SF_MONSTER_STUKA_ONGROUND) ){
			easyPrintLineGroup2("MAP ERROR: Stuka Bat spawned wrongly. Could not find ceiling to snap to (OnGround flag is set to \"false\").");
		}else{
			easyPrintLineGroup2("MAP ERROR: Stuka Bat spawned wrongly. Could not find ground to snap to (OnGround flag is set to \"true\").");
		}
	}
	
	EASY_CVAR_PRINTIF_PRE(stukaPrintout, easyPrintLine("STUKA SPAWN END %d: %d", monsterID, m_iSpawnLoc) );


	//okay?
	lastVelocityChange = gpGlobals->time;


}


extern int global_useSentenceSave;
//=========================================================
// Precache - precaches all resources this monster needs
//=========================================================
void CStukaBat :: Precache()
{	
	//CHANGENOTICE: also add to "precacheAll" in cbase.cpp.
	PRECACHE_MODEL("models/stukabat.mdl");

	global_useSentenceSave = TRUE;

	
	PRECACHE_SOUND_ARRAY( pAttackHitSounds );
	PRECACHE_SOUND_ARRAY( pAttackMissSounds );
	PRECACHE_SOUND_ARRAY( pAttackSounds );
	PRECACHE_SOUND_ARRAY( pIdleSounds );
	PRECACHE_SOUND_ARRAY( pAlertSounds );
	PRECACHE_SOUND_ARRAY( pPainSounds );
	PRECACHE_SOUND_ARRAY( pDeathSounds );

	
	iPoisonSprite = PRECACHE_MODEL( "sprites/poison.spr" );



	//add anything else the stukabat needs (if applicable), especially sounds to benefit from the sound-sentence-save fix.

	global_useSentenceSave = FALSE;

}




//NOTE: DEFINE_CUSTOM_SCHEDULES
DEFINE_CUSTOM_SCHEDULES( CStukaBat)
{

	slStukaBatChaseEnemy,
	slStukaBatStrafe,
	slStukaBatTakeCover,
	slStukaBatFail,
	slStukaBatRangeAttack2,

	slStukaBatFindEat,
	slStukaBatAttemptLand,
	slStukaBatCrawlToFood,
	slStukaBatEat,


	
	slStukaIdleHover,
	slStukaIdleGround,

	slStukaBatAnimWait,

	slStukaPathfindStumped,

};

IMPLEMENT_CUSTOM_SCHEDULES( CStukaBat, CSquadMonster );





void CStukaBat:: getPathToEnemyCustom(){

	/*
	if(queueToggleGround){
		//nothin
		TaskComplete();
		return;
	}
	*/
	//CBaseEntity *pEnemy = m_hEnemy;

	BOOL enemyPresent = !(m_hEnemy==NULL||m_hEnemy.Get()==NULL|| CBaseMonster::Instance(m_hEnemy.Get())==NULL  || m_hEnemy->pev->deadflag != DEAD_NO);

	//EASY_CVAR_PRINTIF_PRE(stukaPrintout, easyPrintLine("POOPY %d", (m_hEnemy==NULL) ) );

	if ( !enemyPresent )
	{
		//easyPrintLineGroup2("YOU FAIL DEEEE!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!");
		
		//???
		EASY_CVAR_PRINTIF_PRE(stukaPrintout, easyPrintLine("FAIL TO GET PATH 1") );
		TaskFail();

		return;
	}else{
		//easyPrintLineGroup2("ASSSSSSSSSSSSS!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!");
	}
			
	
	forceFlyInterpretation = TRUE;
	

	//Vector vecDest = m_hEnemy->pev->origin;
	Vector vecDest = this->m_vecEnemyLKP;

	if ( BuildRouteSimple ( vecDest, bits_MF_TO_ENEMY, m_hEnemy )  )
	{
		//TaskComplete();
		//???!!!
	}
	else if (BuildNearestRouteSimple( vecDest, m_hEnemy->pev->view_ofs, 0, (vecDest - pev->origin).Length() )  )
	{
		//TaskComplete();
		//???!!!
	}
	else
	{
		// no way to get there =(
		//ALERT ( at_aiconsole, "GetPathToEnemy failed!!\n" );
		EASY_CVAR_PRINTIF_PRE(stukaPrintout, easyPrintLine("FAIL TO GET PATH 2") );
		//TaskFail();
		//no, say stumped.
		TaskComplete();
	}

	forceFlyInterpretation = FALSE;

}

//=========================================================
// StartTask
//=========================================================
void CStukaBat :: StartTask ( Task_t *pTask )
{
	//EASY_CVAR_PRINTIF_PRE(stukaPrintout, easyPrintLine("START TASK: %d", pTask->iTask));
	easyForcePrintLine("STUKA STARTTASK: %s %d", getScheduleName(), pTask->iTask);


	//EHANDLE* test = getEnemy();
	//??
	switch ( pTask->iTask )
	{
	case TASK_CHECK_STUMPED:
	{

		if(!HasConditions(bits_COND_SEE_ENEMY)){
			abortAttack();  //can't see them, we're not attacking.
			recentActivity = ACT_RESET;
			//just in case we drop this from abortAttack.
		}

		CSquadMonster::StartTask(pTask);
		break;
	}
	case TASK_SET_ACTIVITY:
		{
			blockSetActivity = -1;


			//easyForcePrintLine("DID YOU GET THAT YOU STUPID oh my goodness gracious me??");
			m_IdealActivity = (Activity)(int)pTask->flData;
			TaskComplete();
			break;
		}
	case TASK_SMALL_FLINCH:
	{
		m_IdealActivity = GetSmallFlinchActivity();
		break;
	}
	case TASK_WAIT:
		//this is all schedule.cpp does here.  Why did I not just do this?

		EASY_CVAR_PRINTIF_PRE(stukaPrintout, easyPrintLine("TASK_WAIT %s GT:%.2f WF:%.2f", m_pSchedule->pName, gpGlobals->time, m_flWaitFinished));
		m_flWaitFinished = gpGlobals->time + pTask->flData;	
		break;
	case TASK_EAT:
		
		ChangeYaw(16);

		CSquadMonster :: StartTask ( pTask );
		break;


		//see schedule.cpp for hte originals.
	case TASK_FIND_NEAR_NODE_COVER_FROM_ENEMY:
		{
			return;
		}
		break;
	case TASK_FIND_FAR_NODE_COVER_FROM_ENEMY:
		{
			return;
		}
		break;
	case TASK_FIND_NODE_COVER_FROM_ENEMY:
		{
			return;
		}
		break;
	case TASK_FIND_COVER_FROM_ENEMY:
		{
			return;
		}
		break;
	case TASK_FIND_COVER_FROM_ORIGIN:
		{
			return;
		}
		break;
	case TASK_FIND_COVER_FROM_BEST_SOUND:
		{
			CSound *pBestSound;

			pBestSound = PBestSound();

			ASSERT( pBestSound != NULL );
			/*
			if ( pBestSound && FindLateralCover( pBestSound->m_vecOrigin, g_vecZero ) )
			{
				// try lateral first
				m_flMoveWaitFinished = gpGlobals->time + pTask->flData;
				TaskComplete();
			}
			*/

			if ( pBestSound && FindCover( pBestSound->m_vecOrigin, g_vecZero, pBestSound->m_iVolume, CoverRadius() ) )
			{
				// then try for plain ole cover
				m_flMoveWaitFinished = gpGlobals->time + pTask->flData;
				TaskComplete();
			}
			else
			{
				// no coverwhatsoever. or no sound in list
				TaskFail();
			}
			break;
		}
		/*
	case TASK_GET_PATH_TO_ENEMY_LKP:
		{
			if (BuildNearestRoute( m_vecEnemyLKP, pev->view_ofs, pTask->flData, (m_vecEnemyLKP - pev->origin).Length() + 1024 ))
			{
				TaskComplete();
			}
			else
			{
				// no way to get there =(
				ALERT ( at_aiconsole, "GetPathToEnemyLKP failed!!\n" );
				TaskFail();
			}
			break;
		}
		*/

		/*
	case TASK_GET_PATH_TO_ENEMY:
		{
			CBaseEntity *pEnemy = m_hEnemy;

			if ( pEnemy == NULL )
			{
				TaskFail();
				return;
			}

			if (BuildNearestRoute( pEnemy->pev->origin, pEnemy->pev->view_ofs, pTask->flData, (pEnemy->pev->origin - pev->origin).Length() + 1024 ))
			{
				TaskComplete();
			}
			else
			{
				// no way to get there =(
				ALERT ( at_aiconsole, "GetPathToEnemy failed!!\n" );
				TaskFail();
			}
			break;
		}
		*/
	//pasted from monsters.cpp.
	case TASK_GET_PATH_TO_ENEMY_LKP:
	{
		

		//changed too.
		//if ( BuildRouteSimple ( m_vecEnemyLKP, bits_MF_TO_LOCATION, NULL ) )
		if ( BuildRouteSimple ( m_vecEnemyLKP, bits_MF_TO_ENEMY, NULL ) )
		{
			TaskComplete();
		}
			
		else if (BuildNearestRouteSimple( m_vecEnemyLKP, pev->view_ofs, 0, (m_vecEnemyLKP - pev->origin).Length() ))
		{
			TaskComplete();
		}
			
		else
		{
			// no way to get there =(
			//EASY_CVAR_PRINTIF_PRE(stukaPrintout, easyPrintLine("PATH FAIL 0") );
			//addToPrintQueue_path("PATH FAIL 0");



			ALERT ( at_aiconsole, "GetPathToEnemyLKP failed!!\n" );
			TaskFail();
		}
		break;
	}
	case TASK_GET_PATH_TO_ENEMY:
		{
			EASY_CVAR_PRINTIF_PRE(stukaPrintout, easyPrintLine("yayFACE 4") );
			getPathToEnemyCustom();
			break;
		}
	/*
	case TASK_STOP_MOVING:


		//WARNING: IS THIS SAFE?
		pev->velocity = Vector(0,0,0);
		break;
	*/
	case TASK_GET_PATH_TO_SPOT:
		{
			//NO SCREW IT...
			TaskFail();
			return;

			//eh, is that really okay?   Just to the "player"?...   sounds weird.
			CBaseEntity *pPlayer = CBaseEntity::Instance( FIND_ENTITY_BY_CLASSNAME( NULL, "player" ) );
			if ( BuildRouteSimple ( m_vecMoveGoal, bits_MF_TO_LOCATION, pPlayer ) )
			{
				TaskComplete();
			}
			else
			{
				// no way to get there =(
				ALERT ( at_aiconsole, "GetPathToSpot failed!!\n" );
				TaskFail();
			}
			break;
		}
	case TASK_RANGE_ATTACK1:
		//Ranges were blanked. Is that wise? Not even parent calls like "default" below does?
		//CSquadMonster :: StartTask ( pTask );
		break;
	case TASK_RANGE_ATTACK2:
		//CSquadMonster :: StartTask ( pTask );
		break;

		
	default:
		CSquadMonster :: StartTask ( pTask );
		break;
	}
}

//CHANGENOTICE
//there is an "Intersect" method defined in controller.cpp.  It should probably be put somewhere more global (util.cpp) so that it doesn't need to be re-defined for here (usually causes compile problems).
//DONE!


//note: probably exactly the same as controller, but hard to say if that is fine.
int CStukaBat::LookupFloat( )
{

	
	UTIL_MakeAimVectors( pev->angles );
	float x = DotProduct( gpGlobals->v_forward, m_velocity );
	float y = DotProduct( gpGlobals->v_right, m_velocity );
	float z = DotProduct( gpGlobals->v_up, m_velocity );

	if (fabs(x) > fabs(y) && fabs(x) > fabs(z))
	{
		/*
		if (x > 0)
			return LookupSequence( "forward");
		else
			return LookupSequence( "backward");
			*/
	}
	else if (fabs(y) > fabs(z))
	{
		/*
		//turn??
		if (y > 0)
			return LookupSequence( "right");
		else
			return LookupSequence( "left");
			*/
	}
	else
	{
		return LookupSequence( "Hover");
		/*
		if (z > 0)
			return LookupSequence( "up");
		else
			return LookupSequence( "down");
			*/
	}

	return LookupSequence("Flying_cycler");
	//no.

}



EHANDLE* CStukaBat::getEnemy(){

	EHANDLE* targetChoice;

	if ( m_movementGoal == MOVEGOAL_ENEMY ){
		targetChoice = &m_hEnemy;
	}else{
		targetChoice = &m_hTargetEnt;
	}

	float distance;
	//copied from "TASK_MOVE_TO_TARGET_RANGE" of schedule.cpp.
	//easyPrintLineGroup2("STUKA targetChoice: %d", targetChoice);
	if ( targetChoice == NULL || targetChoice->Get() == NULL )
		return NULL;

	return targetChoice;
		//TaskFail();

}


void CStukaBat::updateMoveAnim(){

	if(global_stukaAdvancedCombat == 1 && onGround == FALSE){
		//pass.
	}else{
		return;  //Do not affect.
	}
	if(attackIndex > -1){
		//don't modify!
		return;
	}

	EHANDLE* targetChoice;

	if ( m_movementGoal == MOVEGOAL_ENEMY ){
		targetChoice = &m_hEnemy;
	}else{
		targetChoice = &m_hTargetEnt;
	}

	float distance;
	//copied from "TASK_MOVE_TO_TARGET_RANGE" of schedule.cpp.
	//easyPrintLineGroup2("STUKA targetChoice: %d", targetChoice);
	if ( targetChoice == NULL || targetChoice->Get() == NULL ){
		//TaskFail();
	}else
	{
		//m_vecMoveGoal ???
		//distance = ( m_vecMoveGoal - pev->origin ).Length2D();
		distance = ( targetChoice->Get()->v.origin - pev->origin ).Length2D();

		//easyPrintLineGroup2("DISTANCE YO %.2f", distance);
		if ( distance < 280 && m_movementActivity != ACT_HOVER )
			m_movementActivity = ACT_HOVER;
		else if ( distance >= 400 && m_movementActivity != ACT_FLY )
			m_movementActivity = ACT_FLY;
	}
}


BOOL CStukaBat::getHasPathFindingModA(){
	return TRUE; //needs something more broad as not to get stuck.
}
BOOL CStukaBat::getHasPathFindingMod(){
	return TRUE; //needs something more broad as not to get stuck.
}


void CStukaBat::abortAttack(){
	//give up if so.
	easyPrintLineGroup2("ABORT ABORT ABORT");

	//abort attack!
	canSetAnim = TRUE;

	attackIndex = -2;
	attackAgainDelay = gpGlobals->time + getAttackDelay();


	attackEffectDelay = -1;
	maxDiveTime = -1;

	blockSetActivity = -1;
	//THIS IS IMPORTANT, PAY ATTENTION!

	timeToIdle = gpGlobals->time + 4;

	//combatCloseEnough = FALSE;

	//TaskComplete();
	//this->ChangeSchedule(slIdleStand);
	//TaskFail();

	queueAbortAttack = FALSE;

	chargeIndex = -1;

	recentActivity = ACT_RESET; //allow changing this activity, even to itself.

}


//=========================================================
// RunTask 
//=========================================================
void CStukaBat :: RunTask ( Task_t *pTask )
{
	//reset, no landBrake unless specified by this task running.
	landBrake = FALSE;
	
	Vector vecDiff;
	BOOL timetostop;

	CSound *pScent;
	Vector tempGoal;
	CBaseEntity* temper;
	BOOL tryPath = TRUE;

	//CBaseEntity *pEnemy = m_hEnemy;
	float dist2d;
	float dist3d;

	//shoot logic here?

	EASY_CVAR_PRINTIF_PRE(stukaPrintout, easyPrintLine("Stuka #%d RUNTASK: %d", monsterID, pTask->iTask) );

	switch ( pTask->iTask )
	{
	
	case TASK_STUKA_WAIT_FOR_ANIM:

		if(blockSetActivity == -1){
			//done!  Try something else.
			TaskComplete();
		}else{
			//nothing to do here.
			return;
		}

	break;
	
	case TASK_WAIT_FOR_MOVEMENT:
	case TASK_WAIT:
	case TASK_WAIT_FACE_ENEMY:
	case TASK_WAIT_PVS:
		{		
			EASY_CVAR_PRINTIF_PRE(stukaPrintout, easyPrintLine("BEWARE - the Stuka is waiting! %d %d F", eating, getEnemy()!=NULL));
			/*
			//no, wrap animation-changes into a schedule!
			//check if blockSetActivity is done?
			if(blockSetActivity == -1){
				TaskComplete();
				return;
			}
			*/
		CSquadMonster::RunTask(pTask);
		break;

		}
	
	case TASK_WAIT_INDEFINITE:
	
		//likely a mistake.
		//TaskComplete();

		EASY_CVAR_PRINTIF_PRE(stukaPrintout, easyPrintLine("MY SCHED BE DDDDDDDDDDDDDDDD %s %d", m_pSchedule->pName, getEnemy() != NULL));


		if(getEnemy() != NULL){
			ChangeSchedule( slStukaBatChaseEnemy);
		}else{

			
			if(onGround){
				ChangeSchedule( slIdleStand);
			}else{
				ChangeSchedule(slStukaIdleHover);
			}
			


			//TaskComplete();
		}
		//ChangeSchedule( slStukaBatChaseEnemy);

		return;

		//CSquadMonster::RunTask(pTask);
	break;
	
	//This task runs while following the monster and re-routes to stay accurate.
	//Maybe re-routes a little too often, consider doing it every once in a while or rely more
	//on the smart follow system that reroutes at the target moving too far from the previous path
	//destination (LKP). LKP matches the enemy's real position when in plain sight of course.
	case TASK_ACTION:


		//MODDD TODO - is "getEnemy" really necessary? Would "m_hEnemy" with a null check before be sufficient?
		//if(getEnemy() != NULL && getEnemy()->Get()->v.deadflag == DEAD_NO){
		if(m_hEnemy != NULL){

			MakeIdealYaw( m_vecEnemyLKP );
			ChangeYaw( pev->yaw_speed );

			/*
			if(pev->deadflag != DEAD_NO){
				//we're not doing this anymore.
				return;
			}
			*/


			if(global_stukaAdvancedCombat == 1){
				updateMoveAnim();

			}else{
			
			}

			EASY_CVAR_PRINTIF_PRE(stukaPrintout, easyPrintLine("schedule is: %s", m_pSchedule->pName));
			getPathToEnemyCustom();

			//pEnemy == NULL?
			BOOL enemyPresent = !(m_hEnemy==NULL||m_hEnemy.Get()==NULL|| CBaseMonster::Instance(m_hEnemy.Get())==NULL  || m_hEnemy->pev->deadflag != DEAD_NO);
			if(!enemyPresent){
				//Any other points that need this reset to work best first?
				abortAttack();

				if(onGround){
					EASY_CVAR_PRINTIF_PRE(stukaPrintout, easyPrintLine("I AM AN ABJECT FAILUR1"));
					this->ChangeSchedule(slIdleStand);
				}else{
					EASY_CVAR_PRINTIF_PRE(stukaPrintout, easyPrintLine("I AM AN ABJECT FAILUR2"));
					//??? hover or something?
					this->ChangeSchedule(slStukaBatFail);
				}
				//???????????????????????????????????????????????????????????????????????????
			
				//???
				//m_MonsterState = MONSTERSTATE_ALERT;
			}

			if(enemyPresent){
				easyPrintLineGroup1("THE bat ID: %d - rge2: %d ind: %d en: %d df: %d ename: \"%s\" ::: ep: %d, act: %d %d", monsterID, canRangedAttack2, attackIndex, (m_hEnemy==NULL), m_hEnemy->pev->deadflag, STRING(m_hEnemy->pev->classname), enemyPresent, m_Activity, m_IdealActivity  );
			}else{
				easyPrintLineGroup1("THE bat ID: %d - rge2: %d ind: %d en: %d ::: ep: %d,  act: %d %d", monsterID, canRangedAttack2, attackIndex, (m_hEnemy==NULL)  , enemyPresent, m_Activity, m_IdealActivity );
			}
		
			if(attackIndex > -1 && !enemyPresent ){
				//no enemy and in attack-mode?  Stop.

				easyPrintLineGroup1("STUKA %d: ABORT ATTACK!!!", monsterID);
				abortAttack();
			}

			if(maxDiveTime != -1 && maxDiveTime <= gpGlobals->time){
				abortAttack();
				blockSetActivity = -1;
			}

			if(!enemyPresent){

				//if(attackIndex == -1) attackIndex = -2;  //force to -2.
				attackIndex = -2;
				maxDiveTime = -1;
				combatCloseEnough = FALSE;

				//nothing to see here.
				return;
			}

			dist3d = (pev->origin - m_hEnemy->pev->origin).Length();
			dist2d = (pev->origin - m_hEnemy->pev->origin).Length2D();
			if(dist3d <= 58){
				combatCloseEnough = TRUE;
			}else{
				combatCloseEnough = FALSE;
			}

			m_flGroundSpeed = 380;

			//also, if index is non-negative, disallow changing anims automatically (and vice versa)?

			//EASY_CVAR_PRINTIF_PRE(stukaPrintout, easyPrintLine("ALL THE yay %d %.2f", attackIndex, dist3d));



			//MODDD TODO - only look like we're about to attack IF we're going towards the goal node
			//(typically #0, or with the GOAL bit set). And that the enemy is in sight.
			//If not, it's just weird looking to attack thin air or when the enemy isn't even directly in front yet.


			if(HasConditions(bits_COND_SEE_ENEMY) && m_Route[m_iRouteIndex].iType & bits_MF_IS_GOAL){
				//we care about the enemy being in range, proceed as usual.
			}else{
				//easyForcePrintLine("THAT AINT RIGHT");
				//end early and stop attacking if in progress.
				if(attackIndex > -1){
					abortAttack();
				}
				chargeIndex = -1; //at least this...?
				return;
			}




			/*
			if(attackIndex < 0 && this->m_fSequenceFinished){
				//don't be static, just hover.
				blockSetActivity = -1;  //nope.
				//recentActivity = ACT_RESET; //force an animation pick.
				
				//DO YOU HAVE TO DO THIS?!
				this->SetActivity(ACT_HOVER);
			}
			*/

			
			//NOTE - 3 is the melee seqeuence, right?
			if(pev->sequence == 3 && m_fSequenceFinished){
				blockSetActivity = -1;  //nope.
				this->SetActivity(ACT_HOVER);
			}
			

			
			if( (attackIndex == -1)){




				if(dist3d  <= 72 || (dist3d <= 114 && dist2d <= 55)  ){
					attackIndex = 1;
					m_flGroundSpeed = 80;   //??
					setAnimation("attack_claw", TRUE, FALSE, 2);
					float animScaler = getMeleeAnimScaler();
					pev->framerate = animScaler;
					blockSetActivity = gpGlobals->time + (12.2/12.0 / animScaler);
					//SetActivity(ACT_RANGE_ATTACK2);
					attackEffectDelay = gpGlobals->time + (4.9/12.0 / animScaler);

					//attackAgainDelay =  gpGlobals->time + 0.7;

					lastSetActivitySetting = ACT_HOVER;

					maxDiveTime = -1;
					chargeIndex = -1;

					if (RANDOM_LONG(0,1))
						AttackSound();

				}else{
					attackIndex = 0;

					if(global_stukaAdvancedCombat == 1){
						m_flGroundSpeed = 330;
						if(maxDiveTime == -1){
						maxDiveTime = gpGlobals->time + 2;
						}
					}

					if(dist3d < 600){
						EASY_CVAR_PRINTIF_PRE(stukaPrintout, easyPrintLine("START THE DIVING stuff fine fellow! %.2f", gpGlobals->time ));
						chargeIndex = 0;
						//block set activity??
					}
					if(dist3d < 300){
						chargeIndex = 1;
					}
				}

			}else if(attackIndex == 0){

				if(maxDiveTime != -1 && maxDiveTime <= gpGlobals->time){
					abortAttack();
					//diving for too long, not okay.
					//TODO - if this even still works now.
				}

				//gSkillData.
				if(m_hEnemy == NULL){
					easyPrintLineGroup2("Not good, no enemy.");
					return;
				}

				easyPrintLineGroup2("pretty messed up %.2f, %.2f, ", dist3d, dist2d);


				if(dist3d  <= 72 || (dist3d <= 114 && dist2d <= 55)  ){

					attackIndex = 1;
					m_flGroundSpeed = 80;  //???
					setAnimation("attack_claw", TRUE, FALSE, 2);
						
					float animScaler = getMeleeAnimScaler();
					pev->framerate = animScaler;
					blockSetActivity = gpGlobals->time + (12.2/12.0 / animScaler);
					//SetActivity(ACT_RANGE_ATTACK2);
					attackEffectDelay = gpGlobals->time + (4.9/12.0 / animScaler);


					lastSetActivitySetting = ACT_HOVER;

					//attackAgainDelay =  gpGlobals->time + 0.7;
					queueAbortAttack = TRUE;

					maxDiveTime = -1;
					chargeIndex = -1;

					if (RANDOM_LONG(0,1))
						AttackSound();
				}//END OF (dist check)
				else{
					if(dist3d < 600){
						EASY_CVAR_PRINTIF_PRE(stukaPrintout, easyPrintLine("START THE DIVING stuff fine fellow! %.2f", gpGlobals->time ));
						chargeIndex = 0;
						//block set activity??
					}
					if(dist3d < 300){
						chargeIndex = 1;
					}
					easyPrintLineGroup2("YAY NO");
				}


			}else if(attackIndex == 1){

				if(attackEffectDelay != -1 && attackEffectDelay <= gpGlobals->time){
				
					attackEffectDelay = -1;
					
					if(m_hEnemy == NULL)return;

					if(dist3d  <= 72 || (dist3d <= 114 && dist2d <= 55)  ){
						m_hEnemy->TakeDamage( pev, pev, gSkillData.stukaBatDmgClaw, DMG_SLASH, ((global_stukaInflictsBleeding>0)&&(RANDOM_FLOAT(0,1)<=global_stukaInflictsBleeding))?DMG_BLEEDING:0 );   //USED TO HAVE DMG_BLEEDING, test....

						//BEWARE: TakeDamage does NOT draw blood particles.  Do it manually too.
						//DrawAlphaBlood(flDamage, ptr );
						//...huh, we never did the trace for a hit location.   Try getting "CBaseEntity *pHurt = CheckTraceHullAttack( 70, gSkillData.panthereyeDmgClaw, DMG_BLEEDING );"  to work here later.

						UTIL_fromToBlood(this, m_hEnemy, gSkillData.stukaBatDmgClaw, 75);


						if ( m_hEnemy->pev->flags & (FL_MONSTER|FL_CLIENT) )
						{
							m_hEnemy->pev->punchangle.z = -18;
							m_hEnemy->pev->punchangle.x = 5;
						}
						// Play a random attack hit sound
						EMIT_SOUND_FILTERED ( ENT(pev), CHAN_WEAPON, pAttackHitSounds[ RANDOM_LONG(0,ARRAYSIZE(pAttackHitSounds)-1) ], 1.0, ATTN_NORM, 0, 100 + RANDOM_LONG(-5, 5 ) );
						//EMIT_SOUND_FILTERED(ENT(pev), CHAN_WEAPON, "zombie/claw_strike3.wav", 1.0, 1.0, 0, 100, FALSE);
						
					}
					else
					{
						// Play a random attack miss sound
						EMIT_SOUND_FILTERED ( ENT(pev), CHAN_WEAPON, pAttackMissSounds[ RANDOM_LONG(0,ARRAYSIZE(pAttackMissSounds)-1) ], 1.0, ATTN_NORM, 0, 100 + RANDOM_LONG(-5, 5) );
						//EMIT_SOUND_FILTERED(ENT(pev), CHAN_WEAPON, "zombie/claw_miss1.wav", 1.0, 1.0, 0, 100, FALSE);
					}
				}//END OF if(attackEffectDelay...)

				easyPrintLineGroup2("HOW IS THE SEQUENCE %d", m_fSequenceFinished);

				if(m_fSequenceFinished){
					abortAttack();
					
					//recentActivity = ACT_RESET;
					//m_Activity = ACT_RESET;  //is that ok?
					m_IdealActivity = ACT_HOVER;

					//FUCK THIS QUEER EARTH
					//SetActivity(ACT_HOVER);
					//pev->sequence = 13;
					//setAnimation("Hover", TRUE, TRUE, 0);

					//just in case we drop this from abortAttack.
				}
			}

			if(attackIndex == -2 && attackAgainDelay <= gpGlobals->time){
				//ready to strike again.
				attackIndex = -1;
				maxDiveTime = -1;
				chargeIndex = -1;
				//combatCloseEnough = FALSE;
			}


			//Assuming we're going towards the enemy LKP. If we're close enough, stop.

			if( (this->m_vecEnemyLKP - pev->origin).Length() < 10  ){
				//easyForcePrintLine("ILL enjoy A TURNIP");
				TaskComplete();

			}


		}//END OF if(getEnemey() != NULL)
		else{

			//easyForcePrintLine("YOUR face SURE IS very DISGUSTING");

			TaskComplete();
			//try something new.
			EASY_CVAR_PRINTIF_PRE(stukaPrintout, easyPrintLine("WHUT"));

		}

	break;
	case TASK_GET_PATH_TO_ENEMY_LKP:
	
		if(global_stukaAdvancedCombat == TRUE){
			updateMoveAnim();
		}
	
	break;
	case TASK_GET_PATH_TO_ENEMY:
	
		if(global_stukaAdvancedCombat == TRUE){
			updateMoveAnim();
		}
		//this is a dummy for now.
		TaskComplete();
	
	break;
	case TASK_GET_PATH_TO_SPOT:
	
		if(global_stukaAdvancedCombat == TRUE){
			updateMoveAnim();
		}
	
	break;
	case TASK_RANGE_ATTACK2:

	break;
	case TASK_GET_PATH_TO_BESTSCENT:
		{
			//getPathToEnemyCustom();

			pScent = PBestScent();
			
			if(pScent == NULL){
				TaskFail();
				//???
				seekingFoodOnGround = FALSE;
				return;
			}

			float scent_ZOffset = 5;
			
			Vector scent_Loc = pScent->m_vecOrigin + Vector(0, 0, scent_ZOffset);


			MakeIdealYaw( scent_Loc );
			ChangeYaw( pev->yaw_speed );



			
			this->m_movementGoal = MOVEGOAL_LOCATION;
			m_vecMoveGoal = scent_Loc;
			if ( BuildRouteSimple ( scent_Loc, bits_MF_TO_LOCATION, NULL ) )
			{

				TaskComplete();
			}
			//No need for viewoffset, the 2nd argument. scent_loc already has this.
			else if (BuildNearestRouteSimple( scent_Loc, Vector(0,0,0), 0, (scent_Loc - pev->origin).Length() ))
			{
				TaskComplete();
			}
			else
			{
				// no way to get there =(
				PRINTQUEUE_STUKA_SEND(stukaPrint.eatRelated, "PathToBestScent failed!!" );
				TaskFail();
			}

			break;
		}
	case TASK_GET_PATH_TO_LANDING:
		{

		//going to land soon, stall movement on the X-Y plane.
		landBrake = TRUE;

		/*
			if(onGround || queueToggleGround){
				///????
				TaskComplete();
				return;
			}
			*/


			timetostop = FALSE;
			if(blockSetActivity > -1 && queueToggleGround == TRUE){
				PRINTQUEUE_STUKA_SEND(stukaPrint.eatRelated, "timeTo Stop" );
				//TaskComplete();
				tryPath = FALSE;
				
				timetostop = TRUE;
				return;
			}

			if(!timetostop){
				pScent = PBestScent();
				if(pScent != NULL){
					tempGoal = Vector(pev->origin.x, pev->origin.y, pScent->m_vecOrigin.z + 6);
				}else{
					//no scent? we're done.
					TaskFail();
					return;
				}
			}

			
			vecDiff = (tempGoal - pev->origin);


			PRINTQUEUE_STUKA_SEND(stukaPrint.eatRelated, "vecDiffLength:%.2f", vecDiff.Length() );
			PRINTQUEUE_STUKA_SEND(stukaPrint.eatRelated, "blockSetAct:%.2f", blockSetActivity );
			PRINTQUEUE_STUKA_SEND(stukaPrint.eatRelated, "timetostop:%d", timetostop );

			//UTIL_drawLineFrameBoxAround(pev->origin, 3, 24, 255, 255, 0);
			//UTIL_drawLineFrameBoxAround(tempGoal, 3, 24, 255, 125, 0);
			
			if(timetostop){
				return;
			}

			BOOL queueFailure = FALSE;


			this->m_movementGoal = MOVEGOAL_LOCATION;
			m_vecMoveGoal = tempGoal;
			if ( BuildRouteSimple ( tempGoal, bits_MF_TO_LOCATION, NULL ) )
			{
				PRINTQUEUE_STUKA_SEND(stukaPrint.eatRelated, "LANDROUTE_1!"  );

				
				TaskComplete();
			}
			else if (BuildNearestRouteSimple( tempGoal, tempGoal + Vector(0, 0, 0), 0, (tempGoal - pev->origin).Length() ))
			{
				PRINTQUEUE_STUKA_SEND(stukaPrint.eatRelated, "LANDROUTE_2!"  );

				TaskComplete();
			}
			else
			{
				// no way to get there =(
				PRINTQUEUE_STUKA_SEND(stukaPrint.eatRelated, "LANDROUTE_hay"  );

				queueFailure = TRUE;  //only if we're not already close enough... doing the check below.
			}
			//that blockSetActivity == -1 was good, right?



			if( (tempGoal - pev->origin).Length() <= 12 && blockSetActivity == -1){
				
				PRINTQUEUE_STUKA_SEND(stukaPrint.eatRelated, "LANDROUTE_close"  );

				//pev->origin = tempGoal;

				Vector floorVect = UTIL_getFloor(pev->origin, 60, ignore_monsters, ENT(pev));


				if( isErrorVector(floorVect)){
					PRINTQUEUE_STUKA_SEND(stukaPrint.eatRelated, "LANDROUTE_floorFAIL"  );
					//uh, what?
					TaskFail();
					return;
				}else{
					PRINTQUEUE_STUKA_SEND(stukaPrint.eatRelated, "LANDROUTE_floorOK"  );
					UTIL_MoveToOrigin ( ENT(pev), floorVect, (pev->origin - floorVect).Length(), MOVE_STRAFE );
				}

				turnThatOff = TRUE;
				
				EASY_CVAR_PRINTIF_PRE(stukaPrintout, easyPrintLine("WHAT THE hay IS THIS? %.2f %.2f %.2f AND THAT? %.2f %.2f %.2f", floorVect.x, floorVect.y, floorVect.z, pev->origin.x,pev->origin.y,pev->origin.z));

				PRINTQUEUE_STUKA_SEND(stukaPrint.eatRelated, "ANI: Land_ground!");
				
				turnThatOff = TRUE;
				

				setAnimation("Land_ground", TRUE, FALSE, 2);

				//blockSetActivity = gpGlobals->time + (48.0f/12.0f);
				blockSetActivity = gpGlobals->time + (26.0f/12.0f);
				queueToggleGround = TRUE;
				
				if ( m_IdealActivity == m_movementActivity )
				{
					m_IdealActivity = GetStoppedActivity();
				}
				RouteClear();
				ChangeSchedule(slStukaBatAnimWait);

				/*
				setAnimation("Take_off_from_land", TRUE, FALSE, 2);
				//41.6f/12.0f ???
				blockSetActivity = gpGlobals->time + (29.0f/12.0f);
				queueToggleGround = TRUE;
				return GetScheduleOfType(SCHED_STUKABAT_ANIMWAIT);
				*/

			}else{
				if(queueFailure){
					//Not just too close for pathing but counts for success? Then fail.
					TaskFail();
					//??? ok.
					return;
				}
			}
			return;
		break;
	}
	case TASK_GET_PATH_TO_BESTSCENT_FOOT:

		pScent = PBestScent();
		//easyForcePrintLine("SO IS MY SCENT NULL? %d", (pScent==NULL));
		if(pScent != NULL){
			//tempGoal = Vector(pev->origin.x, pev->origin.y, pScent->m_vecOrigin.z + 6);
			tempGoal = pScent->m_vecOrigin;
			seekingFoodOnGround = TRUE;
		}else{
			//no scent? we're done.
			seekingFoodOnGround = FALSE;
			TaskFail();
			return;
		}
		//should there be a better way to turn off "seekingFoodOnGround"?

		//UTIL_drawLineFrameBoxAround(pev->origin, 3, 24, 255, 255, 0);
		//UTIL_drawLineFrameBoxAround(tempGoal, 3, 24, 255, 125, 0);

		//easyForcePrintLine("TELL ME ABOUT YO SELF %.2f %.2f %.2f", pScent->m_vecOrigin.x, pScent->m_vecOrigin.y, pScent->m_vecOrigin.z);
		
		MakeIdealYaw( pScent->m_vecOrigin );
		//not flying, perhaps "Move" or "MoveExecute" should handle this...?
		ChangeYaw( pev->yaw_speed );

		//RouteClear();
		this->m_movementGoal = MOVEGOAL_LOCATION;
		m_vecMoveGoal = pScent->m_vecOrigin;

		/*
		if ( BuildRouteSimple ( pScent->m_vecOrigin, bits_MF_TO_LOCATION, NULL ) )
		{
			TaskComplete();
		}
		else if (BuildNearestRouteSimple( pScent->m_vecOrigin, pScent->m_vecOrigin + Vector(0, 0, 0), 0, (pScent->m_vecOrigin - pev->origin).Length() ))
		{
			TaskComplete();
		}
		else
		//NOTICE: path-finding script removed. Stukabat starts eating here instead.
		*/
		{
			m_failSchedule = SCHED_STUKABAT_EAT;  //just start eating, it's okay to.

			//means, already facing the ideal Yaw.  So that "is facing?"  just goes with this.
			pev->ideal_yaw = this->pev->angles.y;

			eating = TRUE;
			eatingAnticipatedEnd = gpGlobals->time + 7;

			TaskFail();
		}


		
	break;




	default: 
		CSquadMonster :: RunTask ( pTask );
		break;
	}
}




//same as "RadiusDamage", but do reduced damage to friendlies / neutrals.
void CStukaBat :: RadiusDamageNoFriendly(entvars_t* pevInflictor, entvars_t*	pevAttacker, float flDamage, int iClassIgnore, int bitsDamageType )
{
	Vector vecSrc = pev->origin;
	float flRadius = flDamage * 2.5;


	CBaseEntity *pEntity = NULL;
	TraceResult	tr;
	float		flAdjustedDamage, falloff;
	Vector		vecSpot;

	if ( flRadius )
		falloff = flDamage / flRadius;
	else
		falloff = 1.0;

	int bInWater = (UTIL_PointContents ( vecSrc ) == CONTENTS_WATER);

	vecSrc.z += 1;// in case grenade is lying on the ground

	if ( !pevAttacker )
		pevAttacker = pevInflictor;

	// iterate on all entities in the vicinity.
	while ((pEntity = UTIL_FindEntityInSphere( pEntity, vecSrc, flRadius )) != NULL)
	{
		if ( pEntity->pev->takedamage != DAMAGE_NO )
		{

			float damageMult = 1.0;

			int rel = IRelationship(pEntity);

			// UNDONE: this should check a damage mask, not an ignore
			if ( (iClassIgnore != CLASS_NONE && !pEntity->isForceHated(this) && pEntity->Classify() == iClassIgnore) ) 
			{
				//biggest reduction.
				damageMult = 0.18;
				//continue;
			}else if(rel == R_NO || rel == R_AL){
				damageMult = 0.25;
			}
			damageMult = 1.0;


			// blast's don't tavel into or out of water
			if (bInWater && pEntity->pev->waterlevel == 0)
				continue;
			if (!bInWater && pEntity->pev->waterlevel == 3)
				continue;

			vecSpot = pEntity->BodyTarget( vecSrc );
			
			UTIL_TraceLine ( vecSrc, vecSpot, dont_ignore_monsters, ENT(pevInflictor), &tr );

			if ( tr.flFraction == 1.0 || tr.pHit == pEntity->edict() )
			{// the explosion can 'see' this entity, so hurt them!
				if (tr.fStartSolid)
				{
					// if we're stuck inside them, fixup the position and distance
					tr.vecEndPos = vecSrc;
					tr.flFraction = 0.0;
				}
				
				// decrease damage for an ent that's farther from the bomb.
				flAdjustedDamage = ( vecSrc - tr.vecEndPos ).Length() * falloff;
				flAdjustedDamage = flDamage - flAdjustedDamage;
			
				if ( flAdjustedDamage < 0 )
				{
					flAdjustedDamage = 0;
				}
				
				flAdjustedDamage *= damageMult;

			
				// ALERT( at_console, "hit %s\n", STRING( pEntity->pev->classname ) );
				if (tr.flFraction != 1.0)
				{
					ClearMultiDamage( );
					pEntity->TraceAttack( pevInflictor, flAdjustedDamage, (tr.vecEndPos - vecSrc).Normalize( ), &tr, bitsDamageType );
					ApplyMultiDamage( pevInflictor, pevAttacker );
				}
				else
				{
					pEntity->TakeDamage ( pevInflictor, pevAttacker, flAdjustedDamage, bitsDamageType );
				}
			}
		}
	}


}


//CHECK ME!
//=========================================================
// GetSchedule - Decides which type of schedule best suits
// the monster's current state and conditions. Then calls
// monster's member function to get a pointer to a schedule
// of the proper type.
//=========================================================
Schedule_t *CStukaBat :: GetSchedule ( void )
{
	/*
	//IS THAT OKAY?
	attackIndex = -2;
	maxDiveTime = -1;
	combatCloseEnough = FALSE;
	//...as a matter of fact, it isn't!
	*/

	//MODDD - safety.
	if(iAmDead){
		return GetScheduleOfType( SCHED_DIE );
	}

	//MODDD - TODO: a check for "setActivityBlock".  If not negative one (animation in progress), uh, just stall (return "wait" or something?)


	//SNDREL:1:hearstuff:1,2:SND:4 4|
	PRINTQUEUE_STUKA_SEND(stukaPrint.soundRelated, "hearstuff:%d", HasConditions( bits_COND_HEAR_SOUND ) );
	if ( HasConditions( bits_COND_HEAR_SOUND ) )
	{
		//PRINTQUEUE_STUKA_SEND(stukaPrint.soundRelated, "I HEAR STUFF??")
		CSound *pSound;
		pSound = PBestSound();

		ASSERT( pSound != NULL );

		if(pSound != NULL){
			PRINTQUEUE_STUKA_SEND(stukaPrint.soundRelated, "SND:%d %d", pSound->m_iType, pSound->m_iType & (bits_SOUND_COMBAT | bits_SOUND_PLAYER));
		}else{
			PRINTQUEUE_STUKA_SEND(stukaPrint.soundRelated, "NULL? AH.");
		}

		//if ( pSound && (pSound->m_iType & bits_SOUND_DANGER) )
		//	return GetScheduleOfType( SCHED_TAKE_COVER_FROM_BEST_SOUND );
		if (pSound != NULL &&  pSound->m_iType & (bits_SOUND_COMBAT|bits_SOUND_PLAYER) ){
			m_afMemory |= bits_MEMORY_PROVOKED;
		}else{
			//maybe bait?
			SCHEDULE_TYPE baitSched = getHeardBaitSoundSchedule(pSound);
			if(baitSched != SCHED_NONE){
				return GetScheduleOfType ( baitSched );
			}
		}
	}


	BOOLEAN canSeeEnemy = FALSE;

	Look( m_flDistLook );

	if (!HasConditions(bits_COND_SEE_HATE)&&
			!HasConditions(bits_COND_SEE_FEAR)&&
			!HasConditions(bits_COND_SEE_DISLIKE)&&
			!HasConditions(bits_COND_SEE_ENEMY)&&
			!HasConditions(bits_COND_SEE_NEMESIS)
			){
			canSeeEnemy = FALSE;

			//MODDD TODO - m_hTargetEnt check? Is that really necessary?
	}else if(getEnemy() != NULL || (m_hEnemy != NULL && m_hEnemy->pev->deadflag == DEAD_NO) || m_hTargetEnt!=NULL ){

		canSeeEnemy = TRUE;
	}

	if(m_hEnemy!=NULL){
		PRINTQUEUE_STUKA_SEND(stukaPrint.enemyInfo, "SEE: %s %d", STRING(m_hEnemy->pev->classname), m_hEnemy->pev->deadflag);
	}else{
		PRINTQUEUE_STUKA_SEND(stukaPrint.enemyInfo, "SEE: NULL! %d %d %d", getEnemy()!=NULL, m_hEnemy!=NULL, m_hTargetEnt!=NULL);
	}

	PRINTQUEUE_STUKA_SEND(stukaPrint.enemyInfo, "ENESTA: %d %d %d %d %d :%d :: F: %d",
			HasConditions(bits_COND_SEE_HATE) != 0,
			HasConditions(bits_COND_SEE_FEAR) != 0,
			HasConditions(bits_COND_SEE_DISLIKE) != 0,
			HasConditions(bits_COND_SEE_ENEMY) != 0,
			HasConditions(bits_COND_SEE_NEMESIS) != 0,
			(getEnemy() != NULL),
			canSeeEnemy
			);

	if(canSeeEnemy){
		//appetite is gone.  Focus on combat!
		eating = FALSE;
		eatingAnticipatedEnd = -1;
		m_flHungryTime = -1;

		//YEP.
		m_MonsterState = MONSTERSTATE_COMBAT;
		m_IdealMonsterState = MONSTERSTATE_COMBAT;
	}

	//EASY_CVAR_PRINTIF_PRE(stukaPrintout, easyPrintLine("Stuka %d GetSchedule: %d", monsterID, m_MonsterState));
	//EASY_CVAR_PRINTIF_PRE(stukaPrintout, easyPrintLine("DO I SMELL food %d %d %d ", HasConditions(bits_COND_SMELL_FOOD), canSeeEnemy, FShouldEat()));

	//MODDD - PAY ATTENTION TO THIS,
	// AND THAT "TASK_ALERT_STAND" THING, THAT IS PRETTY NEW.
	//search "whut" for the part you know in TASK_ACTION that is if no enemy is around (do a DEAD_DEAD check on pev->deadflag too)

	PRINTQUEUE_STUKA_SEND(stukaPrint.getSchedule, "FOODREQ:%d %d %d %d", m_MonsterState, (m_MonsterState == MONSTERSTATE_COMBAT || m_MonsterState == MONSTERSTATE_ALERT), canSeeEnemy == FALSE, HasConditions(bits_COND_SMELL_FOOD) || eating  );
	
	BOOL fallToIdling = FALSE;
	if(m_MonsterState == MONSTERSTATE_IDLE || m_MonsterState == MONSTERSTATE_COMBAT || m_MonsterState == MONSTERSTATE_ALERT){

		if ( snappedToCeiling == FALSE && canSeeEnemy == FALSE &&
			HasConditions(bits_COND_SMELL_FOOD) || eating)
		{

			abortAttack();

			if(!eating){
			
			CSound *pScent;

			pScent = PBestScent();
			PRINTQUEUE_STUKA_SEND(stukaPrint.getSchedule, "scentex:%d", pScent!=NULL);
			

			if(pScent != NULL){

			if(!onGround){
			
				float dist = (pev->origin - pScent->m_vecOrigin).Length();

				if(dist >= 160){
					// food is right out in the open. Just go fly to it.
					return GetScheduleOfType( SCHED_STUKABAT_FINDEAT );

				}else{
					//land to eat da booty.

					TraceResult trDown;
					Vector vecStart = pev->origin + Vector(0, 0, 5);
					Vector vecEnd = vecStart + Vector(0, 0, -136);
					UTIL_TraceLine(vecStart, vecEnd, ignore_monsters, ENT(pev), &trDown);

					if(trDown.flFraction < 1.0){
						//found the floor, get down.
						return GetScheduleOfType( SCHED_STUKABAT_ATTEMPTLAND );
					}

					EASY_CVAR_PRINTIF_PRE(stukaPrintout, easyPrintLine("POO POO 5"));

				}//END of ELSE of distcheck

			}//END OF if(!onGround)
			else{

				float dist = (pev->origin - pScent->m_vecOrigin).Length();

				if(dist >= 180){
					if(onGround && blockSetActivity == -1){
						//for now, assume the only reason you want to move is you're pursuing an active enemy.
						//m_IdealActivity = ACT_LEAP;
						//SetActivity(ACT_LEAP, TRUE);
						setAnimation("Take_off_from_land", TRUE, FALSE, 2);
						//41.6f/12.0f ???
						blockSetActivity = gpGlobals->time + (29.0f/12.0f);
						queueToggleGround = TRUE;

						return GetScheduleOfType(SCHED_STUKABAT_ANIMWAIT);

						//onGround = FALSE;
					}

					EASY_CVAR_PRINTIF_PRE(stukaPrintout, easyPrintLine("POO POO 4"));
					
				}
				else if(dist >= 79){
					easyPrintLineGroup2("SCHED_STUKABAT_CRAWLTOFOOD DIST BE %.2f!", dist);
					return GetScheduleOfType( SCHED_STUKABAT_CRAWLTOFOOD );

				}else{
					easyPrintLineGroup2("SCHED_STUKABAT_EAT!");

					eating = TRUE;
					eatingAnticipatedEnd = gpGlobals->time + 7;

					MakeIdealYaw( pScent->m_vecOrigin );
					//EAT THAT
					return GetScheduleOfType( SCHED_STUKABAT_EAT );
				}
			}//END OF else...

			}//END OF if(pScent != NULL)
			else{
				//No enemies, no scents?  let's... try hovering / idle.  
				fallToIdling = TRUE;
			}
			}else{
				//EAT THAT
				return GetScheduleOfType( SCHED_STUKABAT_EAT );
			}
		}//END OF if can smell food
		else{
			if(!snappedToCeiling && !canSeeEnemy){
				fallToIdling = TRUE;
				//just idle out.
			}

		}
	}//END OF IF state is combat or alert

	if(canSeeEnemy){
		//let it raise.
		seekingFoodOnGround = FALSE;
	}



	if(fallToIdling){

		//Actually... path find to the enemy regardless.
		if(m_hEnemy != NULL){
			//Go to our enemy in the default monster script!
			//easyForcePrintLine("CAUGHT YOU YA STUPID butt fornicator");
		}else{
			if(onGround){
				return GetScheduleOfType( SCHED_STUKABAT_IDLE_GROUND );
			}else{
				return GetScheduleOfType( SCHED_STUKABAT_IDLE_HOVER );
			}
		}

		

	}

	switch	( m_MonsterState )
	{
	case MONSTERSTATE_IDLE:
		break;

	case MONSTERSTATE_ALERT:
		break;

	case MONSTERSTATE_COMBAT:
		{
			Vector vecTmp = UTIL_Intersect( Vector( 0, 0, 0 ), Vector( 100, 4, 7 ), Vector( 2, 10, -3 ), 20.0 );
			// dead enemy
			if ( HasConditions ( bits_COND_LIGHT_DAMAGE ) )
			{
				// m_iFrustration++;
			}
			if ( HasConditions ( bits_COND_HEAVY_DAMAGE ) )
			{
				// m_iFrustration++;
			}
			//NOTE: MONSTERSTATE_COMBAT of CBaseMonster::GetSchedule()
		}
		break;
	}

	PRINTQUEUE_STUKA_SEND(stukaPrint.getSchedule, "OUTSRC!!!");
	return CSquadMonster :: GetSchedule();
}

//=========================================================
//=========================================================
Schedule_t* CStukaBat :: GetScheduleOfType ( int Type ) 
{
	//TASK_STRAFE_PATH

	EASY_CVAR_PRINTIF_PRE(stukaPrintout, easyPrintLine("STUKA: GET SCHED OF TYPE %d!!!", Type));

	// ALERT( at_console, "%d\n", m_iFrustration );
	switch	( Type )
	{
	case SCHED_CHASE_ENEMY:
	{
		easyPrintLineGroup1("STUKA SCHED %d: slStukaBatChaseEnemy", monsterID);
		return slStukaBatChaseEnemy;
		break;
	}
	case SCHED_ALERT_STAND:
	{

		return slStukaBatChaseEnemy;
		break;
	}

	case SCHED_RANGE_ATTACK2:
	{
		return &slStukaBatRangeAttack2[ 0 ];
	}
	case SCHED_TAKE_COVER_FROM_ENEMY:
	case SCHED_TAKE_COVER_FROM_BEST_SOUND:
	case SCHED_TAKE_COVER_FROM_ORIGIN:
	case SCHED_COWER:
	{
		//ignore!
		//easyPrintLineGroup2("STUKA SCHED: slStukaBatTakeCover");
		//return slStukaBatTakeCover;
		easyPrintLineGroup1("STUKA %d::: HAHAHA THERE IS NO COVER FOR THE WEAK", monsterID);
		//return &slStukaBatRangeAttack2[0];
		return &slStukaBatChaseEnemy[0];
	}
	case SCHED_FAIL:
	{
		easyPrintLineGroup1("STUKA %d SCHED: slStukaBatFail", monsterID );
		return &slStukaBatFail[0];
	}
	case SCHED_STUKABAT_FINDEAT:
	{
		easyPrintLineGroup1("STUKA %d SCHED: slStukaBatFindEat", monsterID );
		return &slStukaBatFindEat[0];

	}
	case SCHED_STUKABAT_ATTEMPTLAND:
	{
		easyPrintLineGroup1("STUKA %d SCHED: slStukaBatAttemptLand", monsterID );
		return &slStukaBatAttemptLand[0];

	}
	case SCHED_STUKABAT_CRAWLTOFOOD:
	{
		easyPrintLineGroup1("STUKA %d SCHED: slStukaBatCrawlToFood", monsterID );
		return &slStukaBatCrawlToFood[0];

	}
	case SCHED_STUKABAT_EAT:
	{
		//ALSO INCLUDED: reset fail task.
		m_failSchedule = SCHED_NONE;
		easyPrintLineGroup1("STUKA %d SCHED: slStukaBatEat", monsterID );
		return &slStukaBatEat[0];
	}
	case SCHED_STUKABAT_IDLE_HOVER:
		//just in case?
		blockSetActivity = -1;
		return &slStukaIdleHover[0];
	break;
	case SCHED_STUKABAT_IDLE_GROUND:
		return &slStukaIdleGround[0];
	break;
	
	case SCHED_STUKABAT_ANIMWAIT:
		return &slStukaBatAnimWait[0];
	break;
	//case SCHED_STUKABAT_PATHFIND_STUMPED:
	case SCHED_PATHFIND_STUMPED:  //actually no, just override the default call for the stumped wait method.
		return &slStukaPathfindStumped[0];
	break;


	//SCHED_TARGET_CHASE
	break;
	}//END OF switch(...)
	
	easyPrintLineGroup1("STUKA %d SCHED: %d", monsterID, Type);
	return CBaseMonster :: GetScheduleOfType( Type );
}



//NOTE: being done by logic instead of the schedule system.
BOOL CStukaBat :: CheckRangeAttack1 ( float flDot, float flDist )
{
	//make this require being the leader, and make it kind of rare, possibly (or have a delay b/w 'suicides' of squad members)
	return FALSE;

	/*
	BOOL toReturn;

	//check for a straight-shot too!
	if ( flDot > 0.5 && flDist > 0 && flDist <= 400 )
	{
		toReturn = TRUE;
	}else{
		toReturn = FALSE;
	}
	return toReturn;
	*/
}

BOOL CStukaBat :: CheckRangeAttack2 ( float flDot, float flDist )
{
	return FALSE;
}


BOOL CStukaBat :: CheckMeleeAttack1 ( float flDot, float flDist )
{
	//no activity.  Disabled.
	return FALSE;

	BOOL toReturn;

	if ( flDist <= 64 && flDot >= 0.7 && m_hEnemy != NULL) //&& FBitSet ( m_hEnemy->pev->flags, FL_ONGROUND ) )
	{
		 toReturn = TRUE;
	}else{
		toReturn = FALSE;
	}
	
	//BOOL toReturn = CBaseMonster::CheckMeleeAttack1(flDot, flDist);

	return toReturn;
}



BOOL CStukaBat::allowedToSetActivity(void){
	return TRUE;
}


void CStukaBat :: SetActivity( Activity NewActivity){

	SetActivity(NewActivity, FALSE);
}

void CStukaBat :: SetActivity ( Activity NewActivity, BOOL forceReset )
{

	//CBaseMonster::SetActivity(NewActivity);
	//return;

	//return;
	////easyForcePrintLine("STEP tried setact");
	
	forceReset = FALSE;

	if(recentActivity == ACT_RESET){
		//NO NO NO
		//recentActivity = NewActivity;
	}


	if(NewActivity == ACT_RESET){
		//no can do.
		//recentActivity = ACT_RESET;  //this okay?
		return;
	}

	BOOL warpRandomAnim = FALSE;

	if(eating){
		//nothing else allowed.
		return;
	}



	easyPrintLine("LISTEN HERE YA LITTLE man %d %d bat:%.2f : ct:%.2f", m_Activity, m_IdealActivity, blockSetActivity, gpGlobals->time);

	////easyForcePrintLine("STEP 0");
	easyPrintLineGroup3("blockSetActivity:%.2f time:%.2f", blockSetActivity, gpGlobals->time);

	if(blockSetActivity == -1){
		//pass.

	}else{
		lastSetActivitySetting = NewActivity;
		lastSetActivityforceReset = forceReset;
		return;
	}
	//canSetAnim = TRUE;

	////easyForcePrintLine("STEP 1");
	if(NewActivity == ACT_RANGE_ATTACK2){
		canSetAnim = FALSE;
		//do the drill!
		//canSetAnim = TRUE;


	}else if(NewActivity == ACT_RANGE_ATTACK1){
		canSetAnim = FALSE;

	}else{
		//not attacking now.  -2?
		//attackIndex = -1;

		//canSetAnim = TRUE;

	}
	EASY_CVAR_PRINTIF_PRE(stukaPrintout, easyPrintLine("STUKABAT: SETACTIVITY: %d %d %d", NewActivity, recentActivity, m_Activity)); 

	
	////easyForcePrintLine("STEP 2");
	if(!onGround){
		if(NewActivity == ACT_IDLE){
			//BOOL someBool = (getEnemy() == NULL);
			EASY_CVAR_PRINTIF_PRE(stukaPrintout, easyPrintLine("SO WHAT THE yay IS THAT yay %d %d %d", NewActivity, snappedToCeiling, attackIndex));

			//easyPrintLineGroup2("ATTA::::: %d %d %d", attackIndex, m_MonsterState, someBool);
			if(attackIndex > -1){
				easyForcePrintLine("I am incredibly 1");
				return;
			}else{
				//if(m_MonsterState == MONSTERSTATE_COMBAT || someBool){
				//only when snapped to ceiling, this check isn't too bad though.

				if(!snappedToCeiling){
					//hover instead, no "hanging" idle anim while mid-air.
					NewActivity = ACT_HOVER;
				}else{
					//?
				}
			}
		}
		////easyForcePrintLine("STEP 3");


		if(NewActivity == ACT_HOVER){
			if(snappedToCeiling){
				NewActivity = ACT_IDLE;  //don't try to hover when tied to the ceiling.
			}

		}
		
	}
	////easyForcePrintLine("STEP 4");

	//easyPrintLineGroup2("STUKA %d: SET ACTIVITY: %d", monsterID, NewActivity);

	if(recentActivity == NewActivity){
		easyPrintLine("I am incredibly 2");
		EASY_CVAR_PRINTIF_PRE(stukaPrintout, easyPrintLine("OVERDONE ACT, PHAIL! %d", NewActivity));
		return;
	}else{
		easyForcePrintLine("OH SHIT!!!");
	}

	EASY_CVAR_PRINTIF_PRE(stukaPrintout, easyPrintLine("ASSSS %d", NewActivity));
	
	////easyForcePrintLine("STEP 5");

	if(onGround){
		if(NewActivity == ACT_IDLE){
			NewActivity = ACT_CROUCHIDLE;
		}
	}else{
		if(!snappedToCeiling && NewActivity == ACT_IDLE){
			
			easyPrintLine("AHAH I SEE THAT stuff MAN");
			//you do it in the air right?
			NewActivity = ACT_HOVER;
		}
		//If we're snapped to the ceiling, leave ACT_IDLE the way it is.
	}


	//this is for more control.
	recentActivity = NewActivity;
	////easyForcePrintLine("STEP 6");

	BOOL allowedToInterruptSelf = TRUE;
	BOOL forceLoop = FALSE;

	if(NewActivity == ACT_CROUCHIDLE){
		//warp it!
		//warpRandomAnim = TRUE;

		forceLoop = TRUE;
		allowedToInterruptSelf = FALSE;
	}

	int	iSequence;
	//CBaseMonster::SetActivity( NewActivity );

	//easyPrintLineGroup2("deb3 %d %d %d", (NewActivity != ACT_CROUCHIDLE), !warpRandomAnim, m_fSequenceFinished) ;
	
	easyPrintLine("LISTEN HERE YA LITTLE man2 na:%d ma:%d mi:%d mf:%d", NewActivity, m_Activity, m_IdealActivity, m_fSequenceFinished);

	if(NewActivity != ACT_CROUCHIDLE || !warpRandomAnim || m_fSequenceFinished){


		//FUCK THIS GAY EARTH AND ALL THE GODDAMN GAY PUPPIES MOTHERFUCKER
		CBaseMonster::SetActivity(NewActivity);

		////iSequence = LookupActivity ( NewActivity );
		//int activity = NewActivity;
		//ASSERT( activity != 0 );
		//void *pmodel = GET_MODEL_PTR( ENT(pev) );

		////easyPrintLine("YOU lucky person %d", warpRandomAnim);
		//if(!warpRandomAnim){
		//	//iSequence = ::LookupActivity( pmodel, pev, activity );
		//	iSequence = LookupActivityHard( activity );
		//}else{

		//	while(true){
		//		studiohdr_t *pstudiohdr;
	
		//		pstudiohdr = (studiohdr_t *)pmodel;
		//		if (! pstudiohdr){
		//			iSequence = 0;
		//			break;
		//		}

		//		mstudioseqdesc_t	*pseqdesc;

		//		pseqdesc = (mstudioseqdesc_t *)((byte *)pstudiohdr + pstudiohdr->seqindex);

		//		int weighttotal = 0;
		//		int seq = ACTIVITY_NOT_AVAILABLE;

		//		if(!warpRandomAnim){
		//			for (int i = 0; i < pstudiohdr->numseq; i++)
		//			{
		//				if (pseqdesc[i].activity == activity)
		//				{
		//					weighttotal += pseqdesc[i].actweight;
		//					if (!weighttotal || RANDOM_LONG(0,weighttotal-1) < pseqdesc[i].actweight){
		//						//easyPrintLineGroup2("deb4 %d %d", i, pseqdesc[i].actweight);
		//						seq = i;
		//					}
		//				}
		//			}
		//		}else{
		//			for (int i = 0; i < pstudiohdr->numseq; i++)
		//			{
		//				if (pseqdesc[i].activity == activity)
		//				{
		//					weighttotal += pseqdesc[i].actweight;
		//					if (!weighttotal || RANDOM_LONG(0,weighttotal-1) < pseqdesc[i].actweight){
		//						//easyPrintLineGroup2("yay YOU %d %d", i, pseqdesc[i].actweight);
		//						seq = i;
		//					}
		//				}
		//			}
		//			//easyPrintLineGroup2("deb5 %d", seq);
		//		}
		//		//return seq;
		//		iSequence = seq;
		//		break;
		//	}//END OF while(true)    method immitation.

		//}
		////easyPrintLineGroup2("deb6 %d %d %d", allowedToInterruptSelf, pev->sequence != iSequence, m_fSequenceFinished);

		//easyPrintLine("LISTEN HERE YA LITTLE man3 na:%d ma:%d mi:%d mf:%d atis:%d cs:%d ns:%d", NewActivity, m_Activity, m_IdealActivity, m_fSequenceFinished, allowedToInterruptSelf, pev->sequence, iSequence);



		//if(allowedToInterruptSelf || pev->sequence != iSequence || m_fSequenceFinished){
		//	// Set to the desired anim, or default anim if the desired is not present
		//	if ( iSequence > ACTIVITY_NOT_AVAILABLE )
		//	{
		//		//MODDD - added "forceReset"
		//		if ( forceReset || (pev->sequence != iSequence || !m_fSequenceLoops) )
		//		{
		//			// don't reset frame between walk and run
		//			if ( !(m_Activity == ACT_WALK || m_Activity == ACT_RUN) || !(NewActivity == ACT_WALK || NewActivity == ACT_RUN))
		//				pev->frame = 0;
		//		}

		//		
		//		//animFrameCutoffSuggestion = 255;   //in case it has leftover changes from anywhere else.

		//		easyPrintLineGroup2("ohship");
		//		easyPrintLine("I JUST GOT SEQUENCE %d", iSequence);
		//		pev->sequence		= iSequence;	// Set to the reset anim (if it's there)
		//		ResetSequenceInfo( );
		//		SetYawSpeed();

		//		//is that okay?...
		//		if(forceLoop){
		//			m_fSequenceLoops = TRUE;
		//		}else{
		//			//m_fSequenceLoops = FALSE;
		//			//???
		//		}


		//	}
		//	else
		//	{
		//		// Not available try to get default anim
		//		ALERT ( at_aiconsole, "%s has no sequence for act:%d\n", STRING(pev->classname), NewActivity );
		//		pev->sequence		= 0;	// Set to the reset anim (if it's there)
		//	}
		//}//END OF if(allowedToInterruptSelf...)

		//m_Activity = NewActivity; // Go ahead and set this so it doesn't keep trying when the anim is not present
	
		//// In case someone calls this with something other than the ideal activity
		//m_IdealActivity = m_Activity;

	}//END OF if(NewActivity != ACT_CROUCHIDLE || !warpRandomAnim || m_fSequenceFinished)



	//?????????????????????????????????????
	iSequence = ACTIVITY_NOT_AVAILABLE;


	//switch ( m_Activity)???
	switch(NewActivity)
	{
	case ACT_WALK:
		m_flGroundSpeed = 65;

		
		//iSequence = LookupActivity ( ACT_WALK_HURT );

		break;

	case ACT_HOVER:
		m_flGroundSpeed = 75;

		break;
	case ACT_FLY:
		m_flGroundSpeed = 350;
		//m_flGroundSpeed = 88;

		break;
	case ACT_FLY_LEFT:
		m_flGroundSpeed = 280;
		break;
	case ACT_FLY_RIGHT:
		m_flGroundSpeed = 280;
		break;




	case ACT_RANGE_ATTACK2:

		break;

	default:
		if(attackIndex <= -1){
			m_flGroundSpeed = 65;
		}
		break;
	}


}

//=========================================================
// RunAI
//=========================================================
void CStukaBat :: RunAI( void )
{
	CBaseMonster :: RunAI();

	if ( HasMemory( bits_MEMORY_KILLED ) )
		return;
}

//extern void DrawRoute( entvars_t *pev, WayPoint_t *m_Route, int m_iRouteIndex, int r, int g, int b );


void CStukaBat::SetTurnActivity(){
	//this is for ground only.
	//NOTE: stukabat has no (ground) turn anim.  Oh well.

	
	//activity for any turning in flight is hovering.
	
	if(!onGround && !snappedToCeiling){
		m_IdealActivity = ACT_HOVER;
	}




	//CSquadMonster::SetTurnActivity();
}

//void CStukaBat :: SetTurnActivity ( void )
void CStukaBat :: SetTurnActivityCustom ( void )
{

	//easyForcePrintLine("SetTurnActivityCustom::: %d %d %d %d", m_IdealActivity, combatCloseEnough, blockSetActivity, onGround);
	if(m_IdealActivity == ACT_FLY && !combatCloseEnough  && blockSetActivity == -1 && !onGround){

	}else{
		return;
	}

	float flYD;
	//what is this garbage...
	//flYD = FlYawDiff();

	flYD = pev->ideal_yaw - pev->angles.y;

	if(abs( flYD) > 180){
		flYD -= 360;
	}
	//return

	//if(flYD
	
	dontResetActivity = TRUE;
	//stukas turn fast, have a low tolerance...

	if ( flYD <= -6 )
	{// big right turn
		//used to set  m_idealActivity
		//m_movementActivity = ACT_FLY_RIGHT;
		if(moveFlyNoInterrupt == -1){
			setAnimation("Flying_turn_right", TRUE, TRUE, 3);
			m_flGroundSpeed = 280;
			//recentActivity = ACT_FLY;
			moveFlyNoInterrupt = gpGlobals->time + 9.2/12.0;
		}
	}
	else if ( flYD >= 6 )
	{// big left turn
		//m_movementActivity = ACT_FLY_LEFT;
		if(moveFlyNoInterrupt == -1){
			setAnimation("Flying_turn_left", TRUE, TRUE, 3);
			m_flGroundSpeed = 280;
			//recentActivity = ACT_FLY;
			moveFlyNoInterrupt = gpGlobals->time + 9.2/12.0;
		}
	}else{
		if(moveFlyNoInterrupt == -1){

			m_flGroundSpeed = 350;
			//if(m_IdealActivity == ACT_FLY)
				setAnimation("Flying_Cycler", TRUE, FALSE, 3);
				//recentActivity = ACT_FLY;
				//moveFlyNoInterrupt = gpGlobals->time + 11.0/12.0;
				moveFlyNoInterrupt = gpGlobals->time + (26.0-1.6)/35.0;
			//}
		}
	}
	
	dontResetActivity = FALSE;

	//float flCurrentYaw = UTIL_AngleMod( pev->angles.y );
	//MakeIdealYaw( m_vecEnemyLKP );

	ChangeYaw( pev->yaw_speed );

	/*
	easyPrintLineGroup2("STUKA #%d : FLYING yay SON %.2 %df", monsterID, flYD, m_movementActivity);
	UTIL_printLineVector("ANG", pev->angles);
	easyPrintLineGroup2("yay?  angle.y: %.2f ideal_yaw:%.2f :::::: flYD:%.2f",  pev->angles.y, pev->ideal_yaw, flYD);
	*/

}


Activity CStukaBat::GetStoppedActivity(){

	//if on the ground, "ACT_IDLE".
	//otherwise, "ACT_HOVER".
	//for now, flying all the time.  Edit later.

	Activity actreturn;
	if(onGround){
		actreturn = ACT_CROUCHIDLE;
	}else{
		if(combatCloseEnough){
			
			actreturn = ACT_HOVER;
		}else{
			
			actreturn = ACT_FLY;
		}
	}
	//NO
	//damn...?
	actreturn = ACT_RESET;

	easyPrintLineGroup2("WHAT THE yay OUTTA NOWHERE %d", actreturn);
	return actreturn;

}


void CStukaBat::Stop( void ) 
{ 
	//EASY_CVAR_PRINTIF_PRE(stukaPrintout, easyPrintLine("YOU SHALL PERISH ALONG WITH THE REST OF YOUR amazing KIN"));
	//m_IdealActivity = GetStoppedActivity();
	//NO STOPPING fine fellow!
}



//#define DIST_TO_CHECK	200
#define DIST_TO_CHECK	3000
void CStukaBat :: Move ( float flInterval ) 
{
	//CSquadMonster::Move(flInterval);
	//IS THIS WISE???
	CBaseMonster::Move( flInterval );
}



BOOL CStukaBat:: ShouldAdvanceRoute( float flWaypointDist )
{
	//was 32?
	if ( flWaypointDist <= 500  )
	{
		return TRUE;
	}
	return FALSE;
	//return CSquadMonster::ShouldAdvanceRoute(flWaypointDist);
}







//NOTICE - there are three possible routes here. Comment the ones above to fall down to that route.
/*
1. (most of this method) LOOSE. like how the controller does it. TraceHull (instead of the usual WALK_MOVE).
Unfortunately any larger than point_hull can cause false positives (places the stuka mistakenly thinks it can't go
but obviously can fit through).

2. At the very end (CheckLocalMove), the usual way monsters do collision. Unsure if this is always ok for flyers.
May make them averse to going over floorless / steep gaps (thinks walking monsters can't cross).
*/
int CStukaBat :: CheckLocalMove ( const Vector &vecStart, const Vector &vecEnd, CBaseEntity *pTarget, float *pflDist )
{
	int iReturn = LOCALMOVE_VALID;
	//MODDD - experimental.  Using a copy of the controller's CheckLocalMove again, but with a different hull type.
	
	TraceResult tr;
	//UTIL_TraceHull( vecStart + Vector( 0, 0, 32), vecEnd + Vector( 0, 0, 32), dont_ignore_monsters, large_hull, edict(), &tr );
	//UTIL_TraceHull( vecStart + Vector( 0, 0, 32), vecEnd + Vector( 0, 0, 32), dont_ignore_monsters, head_hull, edict(), &tr );
	UTIL_TraceHull( vecStart + Vector( 0, 0, 4), vecEnd + Vector( 0, 0, 4), dont_ignore_monsters, point_hull, edict(), &tr );

	if (pflDist)
	{
		*pflDist = ( (tr.vecEndPos - Vector( 0, 0, 4 )) - vecStart ).Length();// get the distance.
	}

	// ALERT( at_console, "check %d %d %f\n", tr.fStartSolid, tr.fAllSolid, tr.flFraction );
	if (tr.fStartSolid || tr.flFraction < 1.0)
	{
		if ( pTarget && pTarget->edict() == gpGlobals->trace_ent ){
			iReturn = LOCALMOVE_VALID;
		}else{
			iReturn = LOCALMOVE_INVALID;
		}
	}
	//SPECIAL DRAWS just for the stuka. uncomment to see this.
	/*
	switch(iReturn){
		case LOCALMOVE_INVALID:
			//ORANGE
			//DrawRoute( pev, m_Route, m_iRouteIndex, 239, 165, 16 );
			DrawRoute( pev, m_Route, m_iRouteIndex, 48, 33, 4 );
		break;
		case LOCALMOVE_INVALID_DONT_TRIANGULATE:
			//RED
			//DrawRoute( pev, m_Route, m_iRouteIndex, 234, 23, 23 );
			DrawRoute( pev, m_Route, m_iRouteIndex, 47, 5, 5 );
		break;
		case LOCALMOVE_VALID:
			//GREEN
			//DrawRoute( pev, m_Route, m_iRouteIndex, 97, 239, 97 );
			DrawRoute( pev, m_Route, m_iRouteIndex, 20, 48, 20 );
		break;
	}
	*/
	return iReturn;

	//Not reached. Comment out all above to do this instead.
	return CBaseMonster::CheckLocalMove(vecStart, vecEnd, pTarget, pflDist);
}





void CStukaBat::setAnimation(char* animationName){
	if(queueToggleSnappedToCeiling)return;
	if(!dontResetActivity){recentActivity = ACT_RESET;}
	EASY_CVAR_PRINTIF_PRE(stukaPrintout, easyPrintLine("ANIMATION SET1: %s ", animationName));

	pev->framerate = 1;
	animFrameCutoffSuggestion = 255;

	CBaseMonster::setAnimation(animationName, FALSE, -1, 0);
	m_flFramerateSuggestion = 1;
}

void CStukaBat::setAnimation(char* animationName, BOOL forceException){
	if(queueToggleSnappedToCeiling)return;
	if(!dontResetActivity){recentActivity = ACT_RESET;}
	EASY_CVAR_PRINTIF_PRE(stukaPrintout, easyPrintLine("ANIMATION SET2: %s ", animationName));
	
	pev->framerate = 1;
	animFrameCutoffSuggestion = 255;

	CBaseMonster::setAnimation(animationName, forceException, -1, 0);
	m_flFramerateSuggestion = 1;
}

void CStukaBat::setAnimation(char* animationName, BOOL forceException, BOOL forceLoopsProperty){
	if(queueToggleSnappedToCeiling)return;
	if(!dontResetActivity){recentActivity = ACT_RESET;}
	EASY_CVAR_PRINTIF_PRE(stukaPrintout, easyPrintLine("ANIMATION SET3: %s ", animationName));
	
	pev->framerate = 1;
	animFrameCutoffSuggestion = 255;

	CBaseMonster::setAnimation(animationName, forceException, forceLoopsProperty, 0);
	m_flFramerateSuggestion = 1;
}

void CStukaBat::setAnimation(char* animationName, BOOL forceException, BOOL forceLoopsProperty, int extraLogic){
	if(queueToggleSnappedToCeiling)return;

	//AHHH SHIT?
	if(!dontResetActivity){recentActivity = ACT_RESET;}
	EASY_CVAR_PRINTIF_PRE(stukaPrintout, easyPrintLine("ANIMATION SET4: %s ", animationName));
	
	pev->framerate = 1;
	animFrameCutoffSuggestion = 255;

	if( !strcmp(animationName, "attack_claw")){
	//	animFrameCutoffSuggestion = 120;
	}

	CBaseMonster::setAnimation(animationName, forceException, forceLoopsProperty, extraLogic);
	m_flFramerateSuggestion = 1;
}


void CStukaBat::safeSetMoveFlyNoInterrupt(float timer){
	moveFlyNoInterrupt = gpGlobals->time + timer;

}
void CStukaBat::safeSetBlockSetActivity(float timer){
	blockSetActivity = gpGlobals->time + timer;
	queueToggleGround = FALSE;
	snappedToCeiling = FALSE;
	queueToggleSnappedToCeiling = FALSE;
	queueAbortAttack = FALSE;
}


void CStukaBat::MoveExecute( CBaseEntity *pTargetEnt, const Vector &vecDir, float flInterval )
{

	tryDetachFromCeiling();

	PRINTQUEUE_STUKA_SEND(stukaPrint.moveRelated, "MOVE1");
	if(snappedToCeiling){
		//that okay?
		m_velocity = Vector(0,0,0);
		pev->velocity = m_velocity;
		lastVelocityChange = gpGlobals->time;
		return;   //can't move.
	}
	
	
	PRINTQUEUE_STUKA_SEND(stukaPrint.moveRelated, "MOVE2 (%d)", seekingFoodOnGround);
	//Notice that this method being called means we are trying to move.
	//Only movement on the ground to crawl to food is allowed.  Otherwise, take to flight.
	if(!seekingFoodOnGround){

		if(onGround && blockSetActivity == -1){
			//for now, assume the only reason you want to move is you're pursuing an active enemy.
			//m_IdealActivity = ACT_LEAP;
			//SetActivity(ACT_LEAP, TRUE);
			setAnimation("Take_off_from_land", TRUE, FALSE, 2);
			//41.6f/12.0f ???
			blockSetActivity = gpGlobals->time + (29.0f/12.0f);
			queueToggleGround = TRUE;
			//onGround = FALSE;

			//MODDD - EXPERIMENTAL.  Is that okay?
			ChangeSchedule( slStukaBatAnimWait);

			//!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
			//should we "return" here...?
			//!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
		}
		//"Land_ceiling"
		//easyPrintLineGroup2("EEEEEEE %.2f, %.2f", blockSetActivity, gpGlobals->time);

		if(onGround){
			easyPrintLineGroup2("YOU ARE A goodness gracious me!");
			m_velocity = m_velocity * 0.2;
			pev->velocity = m_velocity;
			lastVelocityChange = gpGlobals->time;
			return;
		}
	
	}
	
	PRINTQUEUE_STUKA_SEND(stukaPrint.moveRelated, "MOVE3 (%d)", onGround);
	
	//EASY_CVAR_PRINTIF_PRE(stukaPrintout, easyPrintLine("&&&&& %.2f %.2f", pev->framerate, this->m_flFrameRate));
	//pev->yaw_speed = 4;

	if(!onGround){
		//flying!
		//EASY_CVAR_PRINTIF_PRE(stukaPrintout, easyPrintLine("ERRR flyint: %.2f,  attackin: %d", moveFlyNoInterrupt, attackIndex));

		//charge-anims not allowed at death, needless to say.
		if(pev->deadflag == DEAD_NO){
			//if(blockSetActivity == -1 && moveFlyNoInterrupt == -1 && attackIndex <= 0 ){ //m_movementActivity != ACT_HOVER){
			if(chargeIndex == 0){
				//setAnimation("Hover");
				//moveFlyNoInterrupt = 
				//dist check....
				EHANDLE* tempEnemy = getEnemy();

				if(tempEnemy != NULL && tempEnemy->Get() != NULL){

					m_flGroundSpeed = 410;
					
					dontResetActivity = TRUE;
					setAnimation("Dive_cycler", TRUE, FALSE, 3);
					dontResetActivity = FALSE;

					safeSetMoveFlyNoInterrupt(15.0f / 15.0f);
					safeSetBlockSetActivity(15.0f / 15.0f);
					

				}

				//setAnimation("Attack_bomb", TRUE, TRUE, 3);
				//moveFlyNoInterrupt = 26.0f/20.0f


				//setAnimation("Dive_cycler", TRUE, TRUE, 3);
				//moveFlyNoInterrupt = 7.0f/15.0f


			}else if(chargeIndex == 1){
				EHANDLE* tempEnemy = getEnemy();

				if(tempEnemy != NULL && tempEnemy->Get() != NULL){

					m_flGroundSpeed = 480;
					dontResetActivity = TRUE;
					setAnimation("Attack_bomb", TRUE, FALSE, 3);
					dontResetActivity = FALSE;
					moveFlyNoInterrupt = gpGlobals->time + 25.0f/20.0f;
					blockSetActivity = gpGlobals->time +  25.0f/20.0f;

					//block set activity??
				}

			}
		

		}//END OF if(pev->deadflag == DEAD_NO)



		//if(moveFlyNoInterrupt == -1){
		if(chargeIndex == -1){
		
			//UTIL_MakeAimVectors( pev->angles );
			UTIL_MakeVectors( pev->angles );
			float x = DotProduct( gpGlobals->v_forward, m_velocity );
			float y = DotProduct( gpGlobals->v_right, m_velocity );
			float z = DotProduct( gpGlobals->v_up, m_velocity );

			if (fabs(x) > fabs(y) && fabs(x) > fabs(z))
			{
				m_flGroundSpeed = 350;
				m_movementActivity = ACT_FLY;

				SetTurnActivityCustom();
				/*
				if (x > 0)
					return LookupSequence( "forward");
				else
					return LookupSequence( "backward");
				*/
				//m_movementActivity = m_IdealActivity;
			}
			else if (fabs(y) > fabs(z))
			{
				m_flGroundSpeed = 350;
				m_movementActivity = ACT_FLY;

				SetTurnActivityCustom();
				/*
				if (y > 0)
					m_movementActivity = ACT_FLY_LEFT;
				else
					m_movementActivity = ACT_FLY_RIGHT;
				*/
				//SetTurnActivity();
				/*
				//turn??
				if (y > 0)
					return LookupSequence( "right");
				else
					return LookupSequence( "left");
					*/
				//m_movementActivity = m_IdealActivity;
			}
			else
			{
				m_flGroundSpeed = 75;
				m_movementActivity = ACT_HOVER;

				//return LookupSequence( "Hover");
				/*
				if (z > 0)
					return LookupSequence( "up");
				else
					return LookupSequence( "down");
					*/
			}
			//return LookupSequence("Flying_cycler");
		}//END OF if(moveFlyNoInterrupt == -1)

	}//END OF if(!on
	else{

		m_flGroundSpeed = 65;
		m_movementActivity = ACT_WALK;

	}

	if(onGround){
		//just to be safe...
		combatCloseEnough = FALSE;
	}

	PRINTQUEUE_STUKA_SEND(stukaPrint.moveRelated, "MOVE4 (%d)", combatCloseEnough);
	if(combatCloseEnough){
		//slow down!
		m_flGroundSpeed = 75;
		m_movementActivity = ACT_HOVER;

		if ( m_IdealActivity != m_movementActivity )
			m_IdealActivity = m_movementActivity;

		m_velocity = m_velocity * 0.2;
		pev->velocity = m_velocity;
		lastVelocityChange = gpGlobals->time;
		return;  //no movement if so.
	}

	PRINTQUEUE_STUKA_SEND(stukaPrint.moveRelated, "MOVE5");

	//m_movementActivity = ACT_FLY;
	easyPrintLineGroup2("AAAAAAAAACTIVITY IDEAL & MOV: %d %d ", m_IdealActivity, m_movementActivity);


	if(onGround && !FacingIdeal()){

		//Trust the schedule is already telling it to turn to face the right way (but be aware if it freezes when it SHOULD be turning, then we could do it here too)
		//MakeIdealYaw( m_vecEnemyLKP );
		//ChangeYaw( pev->yaw_speed );

		if(m_fSequenceFinished){
			//can't really do anything, just idle?
			setAnimation("Subtle_fidget_on_ground", TRUE, TRUE, 2);
		}

		//NOTE: stuka has no turn activity...
		//SetTurnActivity();
		//don't go, wait for turn to finish?
		return;
	}
	if(onGround){
		//let's let the usual movement handle ground movement.
		CBaseMonster::MoveExecute(pTargetEnt, vecDir, flInterval);
		return;
	}

	if ( m_IdealActivity != m_movementActivity )
		m_IdealActivity = m_movementActivity;

	//easyPrintLineGroup2("TIMEVAR %.2f :: %.2f", pev->framerate, flInterval);

	float flTotal = 0;
	float flStepTimefactored = m_flGroundSpeed*global_STUSpeedMulti * pev->framerate * EASY_CVAR_GET(animationFramerateMulti) * flInterval;
	float flStep = m_flGroundSpeed * 1 * 1;
	


	float velMag = flStep * global_STUSpeedMulti;

	float timeAdjust = (pev->framerate * EASY_CVAR_GET(animationFramerateMulti) * flInterval);
	float distOneFrame = velMag * pev->framerate * EASY_CVAR_GET(animationFramerateMulti) * flInterval;
	
	Vector dest = m_Route[ m_iRouteIndex ].vecLocation;
	Vector vectBetween = (dest - pev->origin);
	float distBetween = vectBetween.Length();
	Vector dirTowardsDest = vectBetween.Normalize();
	Vector _velocity;

	if(distOneFrame <= distBetween){
		_velocity = dirTowardsDest * velMag;
	}else{
		_velocity = dirTowardsDest * distBetween/timeAdjust;
	}

	//UTIL_printLineVector("MOVEOUT", velMag);
	//easyPrintLineGroup2("HELP %.8ff %.8f", velMag, flInterval);

	//UTIL_drawLineFrame(pev->origin, dest, 64, 255, 0, 0);

	
	m_velocity = m_velocity * 0.8 + _velocity * 0.2;
	lastVelocityChange = gpGlobals->time;

	//m_velocity = m_velocity * 0.8 + m_flGroundSpeed * vecDir * 0.2;

	Vector flatVelocity = Vector(_velocity.x, _velocity.y, 0);
	Vector vertVelocity = Vector(0, 0, _velocity.z);

	//pev->velocity = _velocity;
	pev->velocity = m_velocity;
	lastVelocityChange = gpGlobals->time;

	Vector vecSuggestedDir = (m_Route[m_iRouteIndex].vecLocation - pev->origin).Normalize();

	checkFloor(vecSuggestedDir, velMag, flInterval);
	//CBaseMonster::MoveExecute(pTargetEnt, vecDir, flInterval);

	//easyForcePrintLine("WHATS YOUR oh hay VELOCITY?? %.2f %.2f %.2f", pev->velocity.x, pev->velocity.y, pev->velocity.z);
	
	//easyForcePrintLine("ON YOUR WAY TO THE GOAL?? %d: %d", m_iRouteIndex, m_Route[m_iRouteIndex].iType & bits_MF_IS_GOAL);


	/*
	EASY_CVAR_EXTERN(testVar);

	//UTIL_MoveToOrigin ( ENT(pev), pev->origin + vecTotalAdjust , vecTotalAdjust.Length(), MOVE_STRAFE );
	UTIL_MoveToOrigin ( ENT(pev), pev->origin + Vector(0, EASY_CVAR_GET(testVar), 0), abs(EASY_CVAR_GET(testVar)), MOVE_STRAFE );
	*/

	/*
	easyForcePrintLine("WELL ya???? %.2f : %d", EASY_CVAR_GET(testVar), (EASY_CVAR_GET(testVar) == -0.1f));
	if(EASY_CVAR_GET(testVar) == -0.1f){
		this->FRefreshRoute();
		EASY_CVAR_SET(testVar, 0);
	}
	*/



	if(global_drawDebugPathfinding2 == 1){
		//if( ((int)gpGlobals->time) % 2 == 1){
		UTIL_drawLineFrame(pev->origin, pev->origin + vecSuggestedDir * velMag * 5, 48, 0, 255, 255);
		//}else{
			//UTIL_drawLineFrame(pev->origin, pev->origin + vecDir * flStep * 12, 16, 135, 0, 0);
		//}
	}

}


int CStukaBat :: ISoundMask ( void )
{
	
	return	bits_SOUND_WORLD	|
			bits_SOUND_COMBAT	|
			bits_SOUND_CARCASS	|
			bits_SOUND_MEAT		|
			bits_SOUND_GARBAGE	|
			bits_SOUND_PLAYER	|
			//MODDD - new
			bits_SOUND_BAIT;
	//MODDD - give me a schedule for going to investigate the chumtoad croak?

	
}



void CStukaBat::Activate(void){
	
	//EASY_CVAR_PRINTIF_PRE(stukaPrintout, easyPrintLine("STU flag@@@  %d  ", pev->spawnflags ));
	CSquadMonster::Activate();
}



void CStukaBat::DefaultSpawnNotice(void){
	
	CSquadMonster::DefaultSpawnNotice();
}
void CStukaBat::ForceSpawnFlag(int arg_spawnFlag){

	//set our spawn flag first...
	CSquadMonster::ForceSpawnFlag(arg_spawnFlag);
	

}



void CStukaBat::MonsterThink(){
	//UTIL_drawLineFrame(pev->origin, pev->origin + UTIL_YawToVec(pev->ideal_yaw)*60, 4, 255, 255, 24);
	//Vector vecTry = UTIL_YawToVec(pev->ideal_yaw);
	//EASY_CVAR_PRINTIF_PRE(stukaPrintout, easyPrintLine("ang: %.2f vect: (%.2f %.2f %.2f)", pev->ideal_yaw, vecTry.x, vecTry.y, vecTry.z));
	//EASY_CVAR_PRINTIF_PRE(stukaPrintout, easyPrintLine("onGround:%d queueToggleGround%d", onGround, queueToggleGround));

	SetYawSpeed();

	if(!deadSetActivityBlock && !iAmDead && pev->deadflag == DEAD_NO){

		if(pev->deadflag == DEAD_NO){
			//not dead? influences on velocity not allowed.
			//TEST...
			//pev->velocity = Vector(0,0,0);
		}
		//easyPrintLineGroup2("STUKA m_movementActivity: %d", m_movementActivity);
		//easyPrintLineGroup2("STUKA m_IdealActivity: %d %d", m_IdealActivity, m_Activity);
		////easyForcePrintLine("BLOCKSET:::: %.2f", blockSetActivity);

		if(pev->deadflag == DEAD_NO && lastVelocityChange != -1 && (gpGlobals->time - lastVelocityChange) > 0.24  ){
			//no edits to velocity?  Start slowing down a lot.
			m_velocity = m_velocity * 0.15;
			pev->velocity = m_velocity;
			lastVelocityChange = gpGlobals->time;
		}else{

		}
	
		if(landBrake == TRUE){
			pev->velocity.x = pev->velocity.x * 0.07;
			pev->velocity.y = pev->velocity.y * 0.07;
			//Z is unaffected.
		}


		if(moveFlyNoInterrupt != -1 && moveFlyNoInterrupt <= gpGlobals->time){
			moveFlyNoInterrupt = -1;
		}

		if(eatingAnticipatedEnd != -1 && eatingAnticipatedEnd <= gpGlobals->time){
			eating = FALSE;
			eatingAnticipatedEnd = -1;
		}

		if(eating){
			//setAnimation("Eat_on_ground", TRUE, TRUE, 2);
			//???
			setAnimation("Eat_on_ground", TRUE, TRUE, 2);
		}

		//easyPrintLineGroup2("GGGGGGG %.2f %.2f", blockSetActivity, gpGlobals->time);
		if(blockSetActivity != -1 && blockSetActivity <= gpGlobals->time){
			//done:
			blockSetActivity = -1;
			////easyForcePrintLine("lastSetActivitySetting: %d", lastSetActivitySetting);
			if(lastSetActivitySetting != ACT_RESET){

				////easyForcePrintLine("toggleGround:%d snappedToCeil:%d", queueToggleGround, queueToggleSnappedToCeiling);
				if(queueToggleGround){
					//SetActivity(ACT_HOVER, lastSetActivityforceReset);
				}else if(queueToggleSnappedToCeiling){

				}else{
				//if(!queueToggleSnappedToCeiling && !queueToggleGround){
					SetActivity(lastSetActivitySetting, lastSetActivityforceReset);
				}
			}

			EASY_CVAR_PRINTIF_PRE(stukaPrintout, easyPrintLine("TOGGLE GROUND!  NOW IS %d %d", onGround, queueToggleGround));
			if(queueToggleGround){
			
				onGround = !onGround;

				if(!onGround){
					//ENABLE???
					pev->movetype = MOVETYPE_FLY;
					pev->flags |= FL_FLY;
				
					seekingFoodOnGround = FALSE;
					//in the air?  not interested in food, clearly.

					//pev->origin.z += 41;
					UTIL_MoveToOrigin ( ENT(pev), pev->origin + Vector(0, 0, 41), 41, MOVE_STRAFE );
				
					m_IdealActivity = ACT_FLY;

					//setactivity?

					//pev->movetype =


					recentActivity = ACT_RESET;
					SetActivity(ACT_FLY);

					//turn the auto-pushers back on, don't want to snag.
					turnThatOff = FALSE;

					//adjust for the suggested change in position from that take-off anim
				}else{

					//imply we landed, adjust Z.
					//pev->origin = Vector(pev->origin.x, pev->origin.y, pev->origin.z - 8);
					//
					//UTIL_MoveToOrigin ( ENT(pev), pev->origin + Vector(0, 0, 41), 41, MOVE_STRAFE );
					//snap to ground!

					//Vector vecEnd = vecStart + Vector(0, 0, 38);


					TraceResult tr;
					UTIL_TraceLine(pev->origin, pev->origin + Vector(0, 0, -20), ignore_monsters, ENT(pev), &tr);
					if(tr.flFraction < 1.0){
						pev->origin = tr.vecEndPos;
					}else{
						UTIL_MoveToOrigin ( ENT(pev), pev->origin + Vector(0, 0, -8), 41, MOVE_STRAFE );
					}

					pev->movetype = MOVETYPE_STEP;
					pev->flags &= ~FL_FLY;

					//on-ground is true?  okay, finish task.
					seekingFoodOnGround = TRUE;
					TaskComplete();
					//queueToggleGround();


					//???  Just landed!

				}
				lastSetActivitySetting = ACT_RESET;
			}
			queueToggleGround = FALSE;

			if(queueToggleSnappedToCeiling){
				snappedToCeiling = !snappedToCeiling;

				easyPrintLineGroup2("ONCEILING IS NOW %d!", snappedToCeiling);
			

				if(!snappedToCeiling){
					//pev->origin.z += 41;  //??????
					m_IdealActivity = ACT_FLY;
					//adjust for the suggested change in position from that take-off anim

					recentActivity = ACT_RESET;
					SetActivity(ACT_FLY);


				}else{
					//snpped?  IDLE.

					m_IdealActivity = ACT_IDLE;
					//adjust for the suggested change in position from that take-off anim
					//pev->origin.z += 41;  //??????
					recentActivity = ACT_RESET;
					SetActivity(ACT_IDLE);


				}
				lastSetActivitySetting = ACT_RESET;

			}
			queueToggleSnappedToCeiling = FALSE;

			if(queueAbortAttack){
				abortAttack();
			}
			queueAbortAttack = FALSE;

		}else{
		
		}
		//easyPrintLineGroup2("Debug Test 12345 %.2f ::: %d %d ::: %d %d ::: %d ::: %d", blockSetActivity, onGround, snappedToCeiling, queueToggleGround, queueToggleSnappedToCeiling, m_IdealActivity, seekingFoodOnGround);
	
		EHANDLE* tempEnemy = getEnemy();
	
		//EASY_CVAR_PRINTIF_PRE(stukaPrintout, easyPrintLine("STET %d %d, %.2f %.2f", HasConditions( bits_COND_HEAR_SOUND ), m_MonsterState, gpGlobals->time, m_flWaitFinished));

		//global_STUDetection
		//0: never care.  Wait until a loud noise / getting hit by an attack provokes me.
		//1: reduced sight when not provoked and not hanging.
		//2: normal sight all the time except for when starting off hanging.

		switch( (int) global_STUDetection){
		case 0:
			if( m_afMemory & bits_MEMORY_PROVOKED){
				m_flFieldOfView		= VIEW_FIELD_FULL;
			}else{
				m_flFieldOfView		= VIEW_FIELD_FULL;	
			}
		break;
		case 1:
			if( m_afMemory & bits_MEMORY_PROVOKED){
				m_flFieldOfView		= VIEW_FIELD_FULL;
			}else{
				m_flFieldOfView		= 0.35; //slim.
			}
		break;
		case 2:
			if( m_afMemory & bits_MEMORY_PROVOKED){
				m_flFieldOfView		= VIEW_FIELD_FULL;
			}else{
				m_flFieldOfView		= VIEW_FIELD_FULL;	
			}
		break;
		default:
			//???
		break;
		}

		//HEARING STUFF USED TO BE HERE, MOVED TO getSchedule (where hearing something forces "getSchedule" to get called again).

		if(m_hEnemy != NULL && m_hTargetEnt != NULL){
			//EASY_CVAR_PRINTIF_PRE(stukaPrintout, easyPrintLine("ENEMEH INFO: ENEM: %s TARG: %s", STRING(m_hEnemy->pev->classname), STRING(m_hTargetEnt->pev->classname) ));
			stukaPrint.enemyInfo.sendToPrintQueue("ENEMEH INFO: ENEM: %s TARG: %s", STRING(m_hEnemy->pev->classname), STRING(m_hTargetEnt->pev->classname)  );
		}if(m_hEnemy != NULL){
			//EASY_CVAR_PRINTIF_PRE(stukaPrintout, easyPrintLine("ENEMEH INFO: ENEM: %s", STRING(m_hEnemy->pev->classname) ));
			stukaPrint.enemyInfo.sendToPrintQueue("ENEMEH INFO: ENEM: %s", STRING(m_hEnemy->pev->classname)   );

		}else if(m_hTargetEnt != NULL){
			//EASY_CVAR_PRINTIF_PRE(stukaPrintout, easyPrintLine("ENEMEH INFO: TARG: %s", STRING(m_hTargetEnt->pev->classname) ));
			stukaPrint.enemyInfo.sendToPrintQueue("ENEMEH INFO: TARG: %s", STRING(m_hTargetEnt->pev->classname)   );
		}else{
			//EASY_CVAR_PRINTIF_PRE(stukaPrintout, easyPrintLine("ENEMEH INFO: NOTHING" ));
			stukaPrint.enemyInfo.sendToPrintQueue("E1: nothin" );
		}

		stukaPrint.general.sendToPrintQueue("GENERAL: ATTA: %d STAT: %d", attackIndex, m_MonsterState );

	}//END OF dead check


	if(global_stukaPrintout == 1){
		stukaPrint.printOutAll();
	}

	stukaPrint.clearAll();

	EASY_CVAR_PRINTIF_PRE(stukaPrintout, easyPrintLine("HOW DARE YOU %d %d %d", monsterID, m_afMemory & bits_MEMORY_PROVOKED, wakeUp));

	CSquadMonster::MonsterThink();
}

float CStukaBat::HearingSensitivity(){

	if(snappedToCeiling){
		return 0.58f;
	}else if( !(m_afMemory & bits_MEMORY_PROVOKED)){
		//not provoked?  Hearing ability depends on stuka detection.
		switch((int)global_STUDetection){
		case 0:
			return 0.65f;
		break;
		case 1:
			return 0.89f;
		break;
		case 2:
			//always hear.
			return 1.5f;
		break;
		default:
			return 1.5f;
		break;
		}//END OF switch(global_STUDetection)
	}else{
		//provoked?  We hear well.
		return 1.5f;
	}

}



void CStukaBat::customTouch(CBaseEntity *pOther){

	//easyPrintLineGroup2("STUKA %D: I\'M TOUCHIN\'", monsterID);

	//CBaseMonster::Touch(pOther); lots of other touch methods don't do this.  Unnecessary?
}



//inline
void CStukaBat::checkTraceLine(const Vector& vecSuggestedDir, const float& travelMag, const float& flInterval, const Vector& vecStart, const Vector& vecRelativeEnd, const int& moveDist){
	checkTraceLine(vecSuggestedDir, travelMag, flInterval, vecStart, vecRelativeEnd, moveDist, TRUE);
}
//Vector& const vecRelstar, ???   Vector& const reactionMove,
//inline
void CStukaBat::checkTraceLine(const Vector& vecSuggestedDir, const float& travelMag, const float& flInterval, const Vector& vecStart, const Vector& vecRelativeEnd, const int& moveDist, const BOOL canBlockFuture){
	

	//WELL WHAT THE whatIN what IS MOVIN YA.
	//return;

	TraceResult tr;

	//    * moveDist ??
	Vector vecRelativeEndScale = vecRelativeEnd * moveDist;

	if(!tempCheckTraceLineBlock){

		//Vector vecEnd = vecStart + Vector(0, 0, 38);
		UTIL_TraceLine(vecStart, vecStart + vecRelativeEndScale, ignore_monsters, ENT(pev), &tr);
		if(tr.flFraction < 1.0){
			//hit something!

			//Get projection
			// = sugdir - proj. of sugdir onto the normal vector.

			//does this work?
			float dist = tr.flFraction * (float)moveDist;
			float toMove = moveDist - dist;
			//pev->origin = pev->origin + -toMove*vecRelativeEnd;
		
			float timeAdjust = (pev->framerate * EASY_CVAR_GET(animationFramerateMulti) * flInterval);
			
			Vector vecMoveParallel = UTIL_projectionComponent(vecSuggestedDir, tr.vecPlaneNormal).Normalize() * (travelMag * 1);
			//Vector vecMoveParallel = Vector(0,0,0);

			if(timeAdjust == 0){
				//easyPrintLineGroup2("!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!");
				return;
			}else{
				//...
			}

			Vector vecMoveRepel = (tr.vecPlaneNormal*toMove*global_STUrepelMulti)/1;
			
			//pev->origin = pev->origin + vecMoveParallel;
			////UTIL_MoveToOrigin ( ENT(pev), pev->origin + -toMove*vecRelativeEnd + vecMoveParallel , travelMag, MOVE_STRAFE );
		
			//Vector vecTotalAdjust = vecMoveParallel + vecMoveRepel;
			Vector vecTotalAdjust = vecMoveParallel*timeAdjust + vecMoveRepel;


			//???    + -(toMove*1)*vecRelativeEnd
			//pev->velocity = pev->velocity  + ((vecMoveParallel + vecMoveRepel)/timeAdjust);
			
			//MODDD NOTICE - We have a big problem here.
			/*
			UTIL_MoveToOrigin is nice because it only moves the origin of a given entity (this one) up to so far
			until it collides with anything, other monsters or map geometry, if anything is in the way. 
			A direct pev->origin set does not offer this at all.
			Problem is, UTIL_MoveToOrigin can also hang on the same corner we are, so it won't move the stuka
			at all past a corner it is caught on because it is "blocked" by that same corner.
			Way to get around: Move one coord at a time, all of the X-ways, then Y-ways, then Z-ways.
			
			//TODO - this still isn't perfect. It would be better to let the direction we're repelling
			//from play a role in whether the X or Y gets to run first for instance, but generally doing x, y, z
			//individually at all is still better than not.
			
			*/
			//JUST SPLIT IT UP!
			Vector vecTotalAdjustX = Vector(vecTotalAdjust.x, 0, 0);
			Vector vecTotalAdjustY = Vector(0, vecTotalAdjust.y, 0);
			Vector vecTotalAdjustZ = Vector(0, 0, vecTotalAdjust.z);

			UTIL_MoveToOrigin ( ENT(pev), pev->origin + vecTotalAdjustX , vecTotalAdjustX.Length(), MOVE_STRAFE );
			UTIL_MoveToOrigin ( ENT(pev), pev->origin + vecTotalAdjustY , vecTotalAdjustY.Length(), MOVE_STRAFE );
			UTIL_MoveToOrigin ( ENT(pev), pev->origin + vecTotalAdjustZ , vecTotalAdjustZ.Length(), MOVE_STRAFE );




			//pev->origin = pev->origin + tr.vecPlaneNormal*toMove*global_repelMulti;
			//easyPrintLineGroup2("MOOO %s: SPEED: %.2f", STRING(tr.pHit->v.classname), travelMag );
			EASY_CVAR_PRINTIF_PRE(stukaPrintout, UTIL_printLineVector("VECCCC", tr.vecPlaneNormal ) );

			if(canBlockFuture){
				tempCheckTraceLineBlock = TRUE;
			}
			//MODDAHHH 0.91, 5, 0.44, 4.56
			//easyPrintLineGroup2("MODDAHHH %.2f, %d, %.2f, %.2f ", tr.flFraction, moveDist, toMove, (tr.vecEndPos - vecStart).Length());

		}//END OF if(tr.flFraction < 1.0)
	}//END OF if(!tempCheckTraceLineBlock)
	
	if(global_drawDebugPathfinding2 == 1){
		UTIL_drawLineFrame(vecStart, vecStart + vecRelativeEndScale, 16, 0, 255, 0);
	}

}




inline
void CStukaBat::checkTraceLineTest(const Vector& vecSuggestedDir, const float& travelMag, const float& flInterval, const Vector& vecStart, const Vector& vecRelativeEnd, const int& moveDist){
	checkTraceLineTest(vecSuggestedDir, travelMag, flInterval, vecStart, vecRelativeEnd, moveDist, TRUE);
}
//Vector& const vecRelstar, ???   Vector& const reactionMove,
inline
void CStukaBat::checkTraceLineTest(const Vector& vecSuggestedDir, const float& travelMag, const float& flInterval, const Vector& vecStart, const Vector& vecRelativeEnd, const int& moveDist, const BOOL canBlockFuture){
	
	
	//WELL WHAT THE whatIN what IS MOVIN YA.
	//return;

	//tempCheckTraceLineBlock = FALSE;

	TraceResult tr;

	//    * moveDist ??
	Vector vecRelativeEndScale = vecRelativeEnd * moveDist;

	if(!tempCheckTraceLineBlock){

		//Vector vecEnd = vecStart + Vector(0, 0, 38);
		UTIL_TraceLine(vecStart, vecStart + vecRelativeEndScale, ignore_monsters, ENT(pev), &tr);
		if(tr.flFraction < 1.0){
			//hit something!

			//Get projection
			// = sugdir - proj. of sugdir onto the normal vector.

			//does this work?
			float dist = tr.flFraction * (float)moveDist;
			float toMove = moveDist - dist;
			//pev->origin = pev->origin + -toMove*vecRelativeEnd;
		
			float timeAdjust = (pev->framerate * EASY_CVAR_GET(animationFramerateMulti) * flInterval);
			
			Vector vecMoveParallel = UTIL_projectionComponent(vecSuggestedDir, tr.vecPlaneNormal).Normalize() * (travelMag * 1);
			//Vector vecMoveParallel = Vector(0,0,0);

			if(timeAdjust == 0){
				//easyPrintLineGroup2("!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!");
				return;
			}else{
				//...
			}

			Vector vecMoveRepel = (tr.vecPlaneNormal*toMove*global_STUrepelMulti)/1;
			
			//pev->origin = pev->origin + vecMoveParallel;
			////UTIL_MoveToOrigin ( ENT(pev), pev->origin + -toMove*vecRelativeEnd + vecMoveParallel , travelMag, MOVE_STRAFE );
		
			//Vector vecTotalAdjust = vecMoveParallel + vecMoveRepel;
			Vector vecTotalAdjust = vecMoveParallel*timeAdjust + vecMoveRepel;


			//???    + -(toMove*1)*vecRelativeEnd
			//pev->velocity = pev->velocity  + ((vecMoveParallel + vecMoveRepel)/timeAdjust);
			
			//UTIL_MoveToOrigin ( ENT(pev), pev->origin + vecTotalAdjust , vecTotalAdjust.Length(), MOVE_STRAFE );
			//pev->origin = pev->origin + vecTotalAdjust;

			//MODDD NOTICE - We have a big problem here.
			/*
			UTIL_MoveToOrigin is nice because it only moves the origin of a given entity (this one) up to so far
			until it collides with anything, other monsters or map geometry, if anything is in the way. 
			A direct pev->origin set does not offer this at all.
			Problem is, UTIL_MoveToOrigin can also hang on the same corner we are, so it won't move the stuka
			at all past a corner it is caught on because it is "blocked" by that same corner.
			Way to get around: Move one coord at a time, all of the X-ways, then Y-ways, then Z-ways.
			
			//TODO - this still isn't perfect. It would be better to let the direction we're repelling
			//from play a role in whether the X or Y gets to run first for instance, but generally doing x, y, z
			//individually at all is still better than not.
			
			*/
			//JUST SPLIT IT UP!
			Vector vecTotalAdjustX = Vector(vecTotalAdjust.x, 0, 0);
			Vector vecTotalAdjustY = Vector(0, vecTotalAdjust.y, 0);
			Vector vecTotalAdjustZ = Vector(0, 0, vecTotalAdjust.z);

			UTIL_MoveToOrigin ( ENT(pev), pev->origin + vecTotalAdjustX , vecTotalAdjustX.Length(), MOVE_STRAFE );
			UTIL_MoveToOrigin ( ENT(pev), pev->origin + vecTotalAdjustY , vecTotalAdjustY.Length(), MOVE_STRAFE );
			UTIL_MoveToOrigin ( ENT(pev), pev->origin + vecTotalAdjustZ , vecTotalAdjustZ.Length(), MOVE_STRAFE );




			//easyForcePrintLine("BUT YOU MOVE????? %.2f ", vecTotalAdjust.Length());
			::UTIL_drawLineFrame(pev->origin, pev->origin + vecTotalAdjust,40, 255, 0, 0);

			//pev->origin = pev->origin + tr.vecPlaneNormal*toMove*global_repelMulti;
			//easyPrintLineGroup2("MOOO %s: SPEED: %.2f", STRING(tr.pHit->v.classname), travelMag );
			EASY_CVAR_PRINTIF_PRE(stukaPrintout, UTIL_printLineVector("VECCCC", tr.vecPlaneNormal ) );

			if(canBlockFuture){
				tempCheckTraceLineBlock = TRUE;
			}
			//MODDAHHH 0.91, 5, 0.44, 4.56
			//easyPrintLineGroup2("MODDAHHH %.2f, %d, %.2f, %.2f ", tr.flFraction, moveDist, toMove, (tr.vecEndPos - vecStart).Length());

		}//END OF if(tr.flFraction < 1.0)
	}//END OF if(!tempCheckTraceLineBlock)
	
	if(global_drawDebugPathfinding2 == 1){
		UTIL_drawLineFrame(vecStart, vecStart + vecRelativeEndScale, 16, 0, 255, 0);
	}

}









void CStukaBat::checkFloor(const Vector& vecSuggestedDir, const float& travelMag, const float& flInterval){

	if(turnThatOff){
		//we're not doing the checks in this case.
		return;
	}
	//UTIL_drawBoxFrame(pev->absmin, pev->absmax, 16, 0, 0, 255);
	if(global_drawDebugPathfinding2 == 1){
		UTIL_drawBoxFrame(pev->origin + pev->mins, pev->origin + pev->maxs, 16, 0, 0, 255);
	}
	
	int maxX = pev->maxs.x;
	int maxY = pev->maxs.y;
	int maxZ = pev->maxs.z;
	
	int minX = pev->mins.x;
	int minY = pev->mins.y;
	int minZ = pev->mins.z;
	//     Min      Max
	//z = bottom / top
	//x = left / right
	//y = back / forward


	float boundMultiple = 0.7f;

	Vector vecTopRightForward = pev->origin + pev->maxs*boundMultiple;
	
	Vector vecTopLeftForward = pev->origin + Vector(minX, maxY, maxZ)*boundMultiple;
	Vector vecTopRightBackward = pev->origin + Vector(maxX, minY, maxZ)*boundMultiple;
	Vector vecTopLeftBackward = pev->origin + Vector(minX, minY, maxZ)*boundMultiple;

	Vector vecBottomLeftBackward = pev->origin + pev->mins*boundMultiple;
	
	Vector vecBottomLeftForward = pev->origin + Vector(minX, maxY, minZ)*boundMultiple;
	Vector vecBottomRightBackward = pev->origin + Vector(maxX, minY, minZ)*boundMultiple;
	Vector vecBottomRightForward = pev->origin + Vector(maxX, maxY, minZ)*boundMultiple;
	
	//const float root2 = 1.41421356;
	//const float root3 = ?;
	const float root2rec = 0.70710678;
	const float root3rec = 0.57735027;

	/*
	int checkDist = 18;
	int checkDistV = 32;
	
	int checkDistD = 38;
	*/

	int checkDist = global_STUcheckDistH;
	int checkDistV = global_STUcheckDistV;
	
	int checkDistD = global_STUcheckDistD;


	//float Vector push;
	
	if(vecSuggestedDir.x > 0.8){
		//checkCollisionLeft(vecTopLeftForward, 2);
		tempCheckTraceLineBlock = FALSE;
		checkTraceLine(vecSuggestedDir, travelMag, flInterval, vecTopRightForward, Vector(1, 0, 0), checkDist);
		checkTraceLine(vecSuggestedDir, travelMag, flInterval, vecTopRightBackward, Vector(1, 0, 0), checkDist);
		checkTraceLine(vecSuggestedDir, travelMag, flInterval, vecBottomRightForward, Vector(1, 0, 0), checkDist);
		checkTraceLine(vecSuggestedDir, travelMag, flInterval, vecBottomRightBackward, Vector(1, 0, 0), checkDist);
		
	}else if (vecSuggestedDir.x < -0.8){
		
		tempCheckTraceLineBlock = FALSE;
		checkTraceLine(vecSuggestedDir, travelMag, flInterval, vecTopLeftForward, Vector(-1, 0, 0), checkDist);
		checkTraceLine(vecSuggestedDir, travelMag, flInterval, vecTopLeftBackward, Vector(-1, 0, 0), checkDist);
		checkTraceLine(vecSuggestedDir, travelMag, flInterval, vecBottomLeftForward, Vector(-1, 0, 0), checkDist);
		checkTraceLine(vecSuggestedDir, travelMag, flInterval, vecBottomLeftBackward, Vector(-1, 0, 0), checkDist);
		
	}

	if(vecSuggestedDir.y > 0.8){
		//checkCollisionLeft(vecTopLeftForward, 2);
		tempCheckTraceLineBlock = FALSE;
		checkTraceLine(vecSuggestedDir, travelMag, flInterval, vecTopLeftForward, Vector(0, 1, 0), checkDist);
		checkTraceLine(vecSuggestedDir, travelMag, flInterval, vecTopRightForward, Vector(0, 1, 0), checkDist);
		checkTraceLine(vecSuggestedDir, travelMag, flInterval, vecBottomLeftForward, Vector(0, 1, 0), checkDist);
		checkTraceLine(vecSuggestedDir, travelMag, flInterval, vecBottomRightForward, Vector(0, 1, 0), checkDist);
		
	}else if (vecSuggestedDir.y < -0.8){

		tempCheckTraceLineBlock = FALSE;
		checkTraceLine(vecSuggestedDir, travelMag, flInterval, vecTopLeftBackward, Vector(0, -1, 0), checkDist);
		checkTraceLine(vecSuggestedDir, travelMag, flInterval, vecTopRightBackward, Vector(0, -1, 0), checkDist);
		checkTraceLine(vecSuggestedDir, travelMag, flInterval, vecBottomLeftBackward, Vector(0, -1, 0), checkDist);
		checkTraceLine(vecSuggestedDir, travelMag, flInterval, vecBottomRightBackward, Vector(0, -1, 0), checkDist);
	}




	if(!onGround){

		if(vecSuggestedDir.z > 0){
			//checkCollisionLeft(vecTopLeftForward, 2);

			if(vecSuggestedDir.z > 0.3){
				tempCheckTraceLineBlock = FALSE;
				checkTraceLine(vecSuggestedDir, travelMag, flInterval, vecTopLeftForward, Vector(0, 0, 1), checkDistV);
				checkTraceLine(vecSuggestedDir, travelMag, flInterval, vecTopRightForward, Vector(0, 0, 1), checkDistV);
				checkTraceLine(vecSuggestedDir, travelMag, flInterval, vecTopLeftBackward, Vector(0, 0, 1), checkDistV);
				checkTraceLine(vecSuggestedDir, travelMag, flInterval, vecTopRightBackward, Vector(0, 0, 1), checkDistV);
			}
			

			BOOL topLeftForwardCheck = FALSE;
			BOOL topLeftBackwardCheck = FALSE;
			BOOL topRightForwardCheck = FALSE;
			BOOL topRightBackwardCheck = FALSE;

			if(vecSuggestedDir.x > 0.5){
				//do the right ones.
				topRightForwardCheck = TRUE;
				topRightBackwardCheck = TRUE;
			}else if(vecSuggestedDir.x < 0.5){
				topLeftForwardCheck = TRUE;
				topLeftBackwardCheck = TRUE;
			}

			if(vecSuggestedDir.y > 0.5){
				//do the forward ones.
				topRightForwardCheck = TRUE;
				topLeftForwardCheck = TRUE;
			}else if(vecSuggestedDir.y < 0.5){
				topRightBackwardCheck = TRUE;
				topLeftBackwardCheck = TRUE;
			}
			
			tempCheckTraceLineBlock = FALSE; //is that okay?
			if(topRightForwardCheck)checkTraceLine(vecSuggestedDir, travelMag, flInterval, vecTopRightForward, Vector(root3rec, root3rec, -root3rec), checkDistD);
			if(topLeftForwardCheck)checkTraceLine(vecSuggestedDir, travelMag, flInterval, vecTopRightBackward, Vector(root3rec, -root3rec, -root3rec), checkDistD);
			if(topRightBackwardCheck)checkTraceLine(vecSuggestedDir, travelMag, flInterval, vecTopLeftForward, Vector(-root3rec, root3rec, -root3rec), checkDistD);
			if(topLeftBackwardCheck)checkTraceLine(vecSuggestedDir, travelMag, flInterval, vecTopLeftBackward, Vector(-root3rec, -root3rec, -root3rec), checkDistD);
			


			//easyForcePrintLine("AWWWWW SHIT %.2f %.2f", vecSuggestedDir.x, vecSuggestedDir.y);


			/*
			if(vecSuggestedDir.x > 0){
				if(vecSuggestedDir.y > 0){
					checkTraceLine(vecSuggestedDir, travelMag, flInterval, vecTopRightForward, Vector(root3rec, root3rec, -root3rec), checkDistD);
				}else if(vecSuggestedDir.y < 0){
					checkTraceLine(vecSuggestedDir, travelMag, flInterval, vecTopRightBackward, Vector(root3rec, -root3rec, -root3rec), checkDistD);
				}
			}else if(vecSuggestedDir.x < 0){
				if(vecSuggestedDir.y > 0){
					checkTraceLine(vecSuggestedDir, travelMag, flInterval, vecTopLeftForward, Vector(-root3rec, root3rec, -root3rec), checkDistD);
				}else if(vecSuggestedDir.y < 0){
					checkTraceLine(vecSuggestedDir, travelMag, flInterval, vecTopLeftBackward, Vector(-root3rec, -root3rec, -root3rec), checkDistD);
				}
			}
			*/






			//just try bottom checks at least, even with no Z direction. Diagonals can be important.
		}else if (vecSuggestedDir.z <= 0){
		
			if(vecSuggestedDir.z < -0.3){
				tempCheckTraceLineBlock = FALSE;
				checkTraceLine(vecSuggestedDir, travelMag, flInterval, vecBottomLeftForward, Vector(0, 0, -1), checkDistV);
				checkTraceLine(vecSuggestedDir, travelMag, flInterval, vecBottomRightForward, Vector(0, 0, -1), checkDistV);
				checkTraceLine(vecSuggestedDir, travelMag, flInterval, vecBottomLeftBackward, Vector(0, 0, -1), checkDistV);
				checkTraceLine(vecSuggestedDir, travelMag, flInterval, vecBottomRightBackward, Vector(0, 0, -1), checkDistV);
			}

			

			
			BOOL bottomLeftForwardCheck = FALSE;
			BOOL bottomLeftBackwardCheck = FALSE;
			BOOL bottomRightForwardCheck = FALSE;
			BOOL bottomRightBackwardCheck = FALSE;

			if(vecSuggestedDir.x > 0.5){
				//do the right ones.
				bottomRightForwardCheck = TRUE;
				bottomRightBackwardCheck = TRUE;
			}else if(vecSuggestedDir.x < 0.5){
				bottomLeftForwardCheck = TRUE;
				bottomLeftBackwardCheck = TRUE;
			}

			if(vecSuggestedDir.y > 0.5){
				//do the forward ones.
				bottomRightForwardCheck = TRUE;
				bottomLeftForwardCheck = TRUE;
			}else if(vecSuggestedDir.y < 0.5){
				bottomRightBackwardCheck = TRUE;
				bottomLeftBackwardCheck = TRUE;
			}

			
			tempCheckTraceLineBlock = FALSE; //is that okay?
			if(bottomRightForwardCheck)checkTraceLine(vecSuggestedDir, travelMag, flInterval, vecBottomRightForward, Vector(root3rec, root3rec, -root3rec), checkDistD);
			if(bottomLeftForwardCheck)checkTraceLine(vecSuggestedDir, travelMag, flInterval, vecBottomRightBackward, Vector(root3rec, -root3rec, -root3rec), checkDistD);
			if(bottomRightBackwardCheck)checkTraceLine(vecSuggestedDir, travelMag, flInterval, vecBottomLeftForward, Vector(-root3rec, root3rec, -root3rec), checkDistD);
			if(bottomLeftBackwardCheck)checkTraceLine(vecSuggestedDir, travelMag, flInterval, vecBottomLeftBackward, Vector(-root3rec, -root3rec, -root3rec), checkDistD);
			



			/*
			if(vecSuggestedDir.x > 0){
				if(vecSuggestedDir.y > 0){
					checkTraceLine(vecSuggestedDir, travelMag, flInterval, vecBottomRightForward, Vector(root3rec, root3rec, -root3rec), checkDistD);
				}else if(vecSuggestedDir.y < 0){
					checkTraceLine(vecSuggestedDir, travelMag, flInterval, vecBottomRightBackward, Vector(root3rec, -root3rec, -root3rec), checkDistD);
				}
			}else if(vecSuggestedDir.x < 0){
				if(vecSuggestedDir.y > 0){
					checkTraceLine(vecSuggestedDir, travelMag, flInterval, vecBottomLeftForward, Vector(-root3rec, root3rec, -root3rec), checkDistD);
				}else if(vecSuggestedDir.y < 0){
					checkTraceLine(vecSuggestedDir, travelMag, flInterval, vecBottomLeftBackward, Vector(-root3rec, -root3rec, -root3rec), checkDistD);
				}
			}
			*/

			
			//checkTraceLineTest(vecSuggestedDir, travelMag, flInterval, vecBottomRightBackward, Vector(root3rec, -root3rec, -root3rec), checkDistD, FALSE);
			


		}

	}//END OF if(!onGround)


}

//plus or minus, so effectively double these in degrees.
//Fliers get more tolerance at least while flying.
//The method that involves this from CBaseMonster is probably overridden here and this call is never made.
float CStukaBat::MoveYawDegreeTolerance(){
	//return -1;

	if(onGround){
		//tight, not a great ground mover.
		return 20;
	}else{
		return 60; //I shine in the skies.
	}

	
}//END OF MoveYawDegreeTolerance





BOOL CStukaBat::usesAdvancedAnimSystem(void){
	return TRUE;
}

/*
void CStukaBat::SetActivity( Activity NewActivity ){
	CBaseMonster::SetActivity(NewActivity);
}
*/


//NOTICE - the stukabat has never used this system before.
//Proceed with extreme caution.
int CStukaBat::LookupActivityHard(int activity){
	int i = 0;
	m_flFramerateSuggestion = 1;
	m_iForceLoops = -1;
	pev->framerate = 1;
	//any animation events in progress?  Clear it.
	resetEventQueue();

	animFrameCutoffSuggestion = -1; //just in case?


	//Within an ACTIVITY, pick an animation like this (with whatever logic / random check first):
	//    this->animEventQueuePush(10.0f / 30.0f, 3);  //Sets event #3 to happen at 1/3 of a second
	//    return LookupSequence("die_backwards");      //will play animation die_backwards

	//no need for default, just falls back to the normal activity lookup.
	switch(activity){
		case ACT_CROUCHIDLE:
			m_iForceLoops = TRUE;
			return CBaseAnimating::LookupActivity(activity);
		break;
		case ACT_RANGE_ATTACK1:
			//won't work, stuka directly sets the claw animation.
			animFrameCutoffSuggestion = 200; //cutoff?
			return CBaseAnimating::LookupActivity(activity);
		break;
		case ACT_FLY:
			m_iForceLoops = TRUE;
			return CBaseAnimating::LookupActivity(activity);
		break;
		case ACT_HOVER:
			//force this to loop?

			m_iForceLoops = TRUE;
			return CBaseAnimating::LookupActivity(activity);
		break;
		case ACT_SMALL_FLINCH:

			animFrameCutoffSuggestion = 140;

			return CBaseAnimating::LookupActivity(activity);
		break;
	}//END OF switch(...)
	
	//not handled by above?  try the real deal.
	return CBaseAnimating::LookupActivity(activity);
}//END OF LookupActivityHard(...)


int CStukaBat::tryActivitySubstitute(int activity){
	int i = 0;

	//no need for default, just falls back to the normal activity lookup.
	switch(activity){
		case ACT_HOVER:
			return CBaseAnimating::LookupActivity(activity);
		break;
	}//END OF switch(...)


	//not handled by above? Rely on the model's anim for this activity if there is one.
	return CBaseAnimating::LookupActivity(activity);
}//END OF tryActivitySubstitute(...)




void CStukaBat::ReportAIState(void){

	//call the parent, and add on to that.
	CSquadMonster::ReportAIState();

	easyForcePrintLine("attackIndex:%d chargeIndex:%d seqfin:%d seq:%d fr:%.2f frr:%.2f co:%.2f, cos:%.2f", attackIndex, chargeIndex, m_fSequenceFinished, pev->sequence, pev->frame, pev->framerate, animFrameCutoff, animFrameCutoffSuggestion);
	easyForcePrintLine("man %d %d bat:%.2f : ct:%.2f", m_Activity, m_IdealActivity, blockSetActivity, gpGlobals->time);
	

}//END OF ReportAIState()


//TODO - Pretty sure the stuka has turn activities, use them.


Schedule_t* CStukaBat::GetStumpedWaitSchedule(){
	return slStukaPathfindStumped;
}//END OF GetStumpedWaitSchedule
