/***
*
*	Copyright (c) 1996-2002, Valve LLC. All rights reserved.
*	
*	This product contains software technology licensed from Id 
*	Software, Inc. ("Id Technology").  Id Technology (c) 1996 Id Software, Inc. 
*	All Rights Reserved.
*
*   Use, distribution, and modification of this source code and/or resulting
*   object code is restricted to non-commercial enhancements to products from
*   Valve LLC.  All other use, distribution, or modification is prohibited
*   without written permission from Valve LLC.
*
****/
#ifndef PLAYER_H
#define PLAYER_H



#include "pm_materials.h"


#ifndef CLIENT_DLL
	//MODDD - necessary to have a CRope instance variable.
	#include "CRope.h"

#endif


#include "friendly.h"



EASY_CVAR_EXTERN(testVar);


//MODDD - new const
//Player always has long jump.  Forces "m_flongjump" to true whenever possible, generally from loading a game or spawning.
#define PLAYER_ALWAYSHASLONGJUMP 1

//Does the long jump use the delay feature (must crouch for so long before doing the long jump), = 1, or store it to a charge that is taken from, 
//like a storage battery, = 0?
#define LONGJUMPUSESDELAY 0

//How much "charge" does each long jump use?  Implies "LONGJUMPUSESDELAY" is 0.
#define LONGJUMP_CHARGEUSE 25


//NO AGAIN!  Now stored in const.h for access anywhere (client / server).
//No.  This is now stored in weapons.h, which Player's files have access to.
//Edit "PLAYER_LONGJUMPCHARGE_MAX" for the same effect.  Note that "delay" uses this as the time to spend charging, so 100 seconds is unrealistic
//for that (make it 1.2 when "LONGJUMPUSESDELAY" is 1).
//#define PLAYER_LONGJUMPCHARGE_MAX 1.2
//#define PLAYER_LONGJUMPCHARGE_MAX 100

//What is the delay allowed between successive longjumnps?  Used onyl when "LONGJUMPUSESDELAY" is 0, or when they're using the stored battery.
#define PLAYER_LONGJUMP_DELAY 4




#define PLAYER_FATAL_FALL_SPEED		1024// approx 60 feet
#define PLAYER_MAX_SAFE_FALL_SPEED	580// approx 20 feet
#define DAMAGE_FOR_FALL_SPEED		(float) 100 / ( PLAYER_FATAL_FALL_SPEED - PLAYER_MAX_SAFE_FALL_SPEED )// damage per unit per second.
#define PLAYER_MIN_BOUNCE_SPEED		200
#define PLAYER_FALL_PUNCH_THRESHHOLD (float)350 // won't punch player's screen/make scrape noise unless player falling at least this fast.

//
// Player PHYSICS FLAGS bits
//
#define		PFLAG_ONLADDER		( 1<<0 )
#define		PFLAG_ONSWING		( 1<<0 )
#define		PFLAG_ONTRAIN		( 1<<1 )
#define		PFLAG_ONBARNACLE	( 1<<2 )
#define		PFLAG_DUCKING		( 1<<3 )		// In the process of ducking, but totally squatted yet
#define		PFLAG_USING			( 1<<4 )		// Using a continuous entity
#define		PFLAG_OBSERVER		( 1<<5 )		// player is locked in stationary cam mode. Spectators can move, observers can't.

//MODDD - new for rope.
#define PFLAG_ONROPE ( 1<<6 )



//
// generic player
//
//-----------------------------------------------------
//This is Half-Life player entity
//-----------------------------------------------------
#define CSUITPLAYLIST	4		// max of 4 suit sentences queued up at any time

#define SUIT_GROUP			TRUE
#define	SUIT_SENTENCE		FALSE

#define	SUIT_REPEAT_OK		0
#define SUIT_NEXT_IN_30SEC	30
#define SUIT_NEXT_IN_1MIN	60
#define SUIT_NEXT_IN_5MIN	300
#define SUIT_NEXT_IN_10MIN	600
//MODDD - new.
#define SUIT_NEXT_IN_20MIN	1200

#define SUIT_NEXT_IN_30MIN	1800
#define SUIT_NEXT_IN_1HOUR	3600

#define CSUITNOREPEAT		32

#define	SOUND_FLASHLIGHT_ON		"items/flashlight1.wav"
#define	SOUND_FLASHLIGHT_OFF	"items/flashlight1.wav"

#define TEAM_NAME_LENGTH	16

typedef enum
{
	PLAYER_IDLE,
	PLAYER_WALK,
	PLAYER_JUMP,
	PLAYER_SUPERJUMP,
	PLAYER_DIE,
	PLAYER_ATTACK1,
} PLAYER_ANIM;

#define MAX_ID_RANGE 2048
#define SBAR_STRING_SIZE 128

enum sbar_data
{
	SBAR_ID_TARGETNAME = 1,
	SBAR_ID_TARGETHEALTH,
	SBAR_ID_TARGETARMOR,
	SBAR_END,
};

#define CHAT_INTERVAL 1.0f


class CBasePlayer : public CBaseMonster
{
public:


	float superDuperDelay;
	
	float m_fCustomHolsterWaitTime;

	BOOL m_bHolstering;
	CBasePlayerItem* m_pQueuedActiveItem;


	//BOOL forceNoWeaponLoop;


	//the player will be unkillable up to this time (implied to be set & since revive).
	float reviveSafetyTime;

	int nextSpecialNode;
	float nextSpecialNodeAlternateTime;

	BOOL iWasFrozenToday;


	float friendlyCheckTime;
	
	EHANDLE closestFriendlyMemEHANDLE;
	CFriendly* closestFriendlyMem;
	float horrorPlayTimePreDelay;
	float horrorPlayTime;

	
	int					random_seed;    // See that is shared between client & server for shared weapons code

	int					m_iPlayerSound;// the index of the sound list slot reserved for this player
	int					m_iTargetVolume;// ideal sound volume. 
	int					m_iWeaponVolume;// how loud the player's weapon is right now.
	int					m_iExtraSoundTypes;// additional classification for this weapon's sound
	int					m_iWeaponFlash;// brightness of the weapon flash
	float				m_flStopExtraSoundTime;
	
	float				m_flFlashLightTime;	// Time until next battery draw/Recharge
	int					m_iFlashBattery;		// Flashlight Battery Draw

	int					m_afButtonLast;
	int					m_afButtonPressed;
	int					m_afButtonReleased;
	
	edict_t			   *m_pentSndLast;			// last sound entity to modify player room type
	float				m_flSndRoomtype;		// last roomtype set by sound entity
	float				m_flSndRange;			// dist from player to sound entity

	float				m_flFallVelocity;
	
	int					m_rgItems[MAX_ITEMS];
	int					m_fKnownItem;		// True when a new item needs to be added
	int					m_fNewAmmo;			// True when a new item has been added

	unsigned int		m_afPhysicsFlags;	// physics flags - set when 'normal' physics should be revisited or overriden
	float				m_fNextSuicideTime; // the time after which the player can next use the suicide command


// these are time-sensitive things that we keep track of
	float				m_flTimeStepSound;	// when the last stepping sound was made
	float				m_flTimeWeaponIdle; // when to play another weapon idle animation.
	float				m_flSwimTime;		// how long player has been underwater
	float				m_flDuckTime;		// how long we've been ducking
	float				m_flWallJumpTime;	// how long until next walljump

	float				m_flSuitUpdate;					// when to play next suit update
	int					m_rgSuitPlayList[CSUITPLAYLIST];// next sentencenum to play for suit update
	
	//MODDD - also store the time for each suit sound to play.  Some may exceed the default 3.5 seconds
	//(causing the next in line to spill over).
	float				m_rgSuitPlayListDuration[CSUITPLAYLIST];// next sentencenum to play for suit update
	void (CBasePlayer::*m_rgSuitPlayListEvent[CSUITPLAYLIST])();
	float m_rgSuitPlayListEventDelay[CSUITPLAYLIST];
	float m_rgSuitPlayListFVoxCutoff[CSUITPLAYLIST];

	float currentSuitSoundEventTime;
	float currentSuitSoundFVoxCutoff;
	int sentenceFVoxCutoffStop;

	void (CBasePlayer::*currentSuitSoundEvent)();
	




	int					m_iSuitPlayNext;				// next sentence slot for queue storage;
	int					m_rgiSuitNoRepeat[CSUITNOREPEAT];		// suit sentence no repeat list
	float				m_rgflSuitNoRepeatTime[CSUITNOREPEAT];	// how long to wait before allowing repeat
	int					m_lastDamageAmount;		// Last damage taken

	//MODDD - moved to CBaseMonster
	//float				m_tbdPrev;				// Time-based damage timer

	float				m_flgeigerRange;		// range to nearest radiation source
	float				m_flgeigerDelay;		// delay per update of range msg to client
	int					m_igeigerRangePrev;
	int					m_iStepLeft;			// alternate left/right foot stepping sound
	char				m_szTextureName[CBTEXTURENAMEMAX];	// current texture name we're standing on
	char				m_chTextureType;		// current texture type

	int					m_idrowndmg;			// track drowning damage taken
	int					m_idrownrestored;		// track drowning damage restored

	int					m_bitsHUDDamage;		// Damage bits for the current fame. These get sent to 
												// the hude via the DAMAGE message
	//MODDD - complimentary
	int					m_bitsModHUDDamage;

	BOOL				m_fInitHUD;				// True when deferred HUD restart msg needs to be sent
	BOOL				m_fGameHUDInitialized;
	int					m_iTrain;				// Train control position
	BOOL				m_fWeapon;				// Set this to FALSE to force a reset of the current weapon HUD info

	EHANDLE				m_pTank;				// the tank which the player is currently controlling,  NULL if no tank
	float				m_fDeadTime;			// the time at which the player died  (used in PlayerDeathThink())

	BOOL			m_fNoPlayerSound;	// a debugging feature. Player makes no sound if this is true. 
	BOOL			m_fLongJump; // does this player have the longjump module?
	//MODDD
	BOOL			m_fLongJumpMemory;


	float       m_tSneaking;
	int			m_iUpdateTime;		// stores the number of frame ticks before sending HUD update messages
	int			m_iClientHealth;	// the health currently known by the client.  If this changes, send a new
	int			m_iClientBattery;	// the Battery currently known by the client.  If this changes, send a new

	//MODDD - added for the power canisters / syringes.
	int m_iClientAntidote;
	int m_iClientAdrenaline;
	int m_iClientRadiation;

	int			m_iHideHUD;		// the players hud weapon info is to be hidden
	int			m_iClientHideHUD;
	int			m_iFOV;			// field of view
	int			m_iClientFOV;	// client's known FOV
	// usable player items 
	CBasePlayerItem	*m_rgpPlayerItems[MAX_ITEM_TYPES];
	CBasePlayerItem *m_pActiveItem;
	CBasePlayerItem *m_pClientActiveItem;  // client version of the active item
	CBasePlayerItem *m_pLastItem;
	// shared ammo slots
	int	m_rgAmmo[MAX_AMMO_SLOTS];
	int	m_rgAmmoLast[MAX_AMMO_SLOTS];

	Vector				m_vecAutoAim;
	BOOL				m_fOnTarget;
	int					m_iDeaths;
	float				m_iRespawnFrames;	// used in PlayerDeathThink() to make sure players can always respawn

	int m_lastx, m_lasty;  // These are the previous update's crosshair angles, DON"T SAVE/RESTORE

	int m_nCustomSprayFrames;// Custom clan logo frames for this player
	float	m_flNextDecalTime;// next time this player can spray a decal

	char m_szTeamName[TEAM_NAME_LENGTH];


	

	
		
//MODDD - rope things.
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////


#ifndef CLIENT_DLL
	//MODD - some rope vars.
	private:
	CRope* m_pRope;
	float m_flLastClimbTime;
	BOOL m_bIsClimbing;
#endif


	public:

#ifndef CLIENT_DLL

	BOOL IsOnRope() const { return ( m_afPhysicsFlags & PFLAG_ONROPE ) != 0; }

	void SetOnRopeState( bool bOnRope ){
		if( bOnRope )
			m_afPhysicsFlags |= PFLAG_ONROPE;
		else
			m_afPhysicsFlags &= ~PFLAG_ONROPE;
	}

	CRope* GetRope(){ return m_pRope; }

	void SetRope( CRope* pRope ){
		m_pRope = pRope;
	}

	void SetIsClimbing( const bool bIsClimbing ){
		m_bIsClimbing = bIsClimbing;
	}

#endif
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////











	//MODDD - some setter methods.
	void setHealth(int newHealth);
	void setArmorBattery(int newBattery);
	void grantAllItems();
	void giveMaxAmmo();

	BOOL playerHasSuit();
	BOOL playerHasLongJump();

	BOOL blocksImpact(void);

	//MODDD - new.  Accept a new parameter (optional: assuming "false" if not given)
	virtual void Spawn(BOOL revived);
	virtual void Spawn( void );

	//MODDD - added from inheritance heirarchy
	//(IE: available, but not referred / used for the "player" itself before)
	virtual void Activate( void );


	//MODDD - this method was found as-is, but used to be named "Pain" but was never called by the player. This means it never had a chance to be played.
	//The proper name is "PainSound" to override the Monster-provided method for playing a pain sound. So any calls there for "PainSound" did nothing while
	//this was named "Pain" - no association.
	void PainSound( void ) override;

	//Separate method for a different way of doing a chance.
	void PainChance(void);

//	virtual void Think( void );
	virtual void Jump( void );
	virtual void Duck( void );
	virtual void PreThink( void );
	virtual void PostThink( void );
	virtual Vector GetGunPosition( void );
	virtual Vector GetGunPositionAI(void);
	virtual int TakeHealth( float flHealth, int bitsDamageType );

	//MODDD
	GENERATE_TRACEATTACK_PROTOTYPE_VIRTUAL
	GENERATE_TAKEDAMAGE_PROTOTYPE_VIRTUAL
	
	
	void FadeMonster(void);

	
	GENERATE_KILLED_PROTOTYPE_VIRTUAL
	
	virtual Vector BodyTarget( const Vector &posSrc ) { return Center( ) + pev->view_ofs * RANDOM_FLOAT( 0.5, 1.1 ); };		// position to shoot at
	
	

	//MODDD
	//is one multiple of pev->view_ofs   okay?
	virtual Vector BodyTargetMod( const Vector &posSrc ) {
			/*
			Vector org = Center( ) + pev->view_ofs;
			if ( pev->flags & FL_DUCKING )
			{
				org = org + ( VEC_HULL_MIN - VEC_DUCK_HULL_MIN );
			}
			*/
			
			if ( !(pev->flags & FL_DUCKING) ){
				return Center() + pev->view_ofs;
			}else{

				/*
				if(EASY_CVAR_GET(testVar) == 1){
					return pev->origin + Vector(0, 0, VEC_DUCK_HULL_MAX.z);
				}else{
					//old way
					return Center() + pev->view_ofs;
				}
				*/
				return pev->origin + Vector(0, 0, VEC_DUCK_HULL_MAX.z);
			}


	};		// position to shoot at
	
	
	virtual void StartSneaking( void ) { m_tSneaking = gpGlobals->time - 1; }
	virtual void StopSneaking( void ) { m_tSneaking = gpGlobals->time + 30; }
	virtual BOOL IsSneaking( void ) { return m_tSneaking <= gpGlobals->time; }

	virtual BOOL IsAlive( void ) { return (pev->deadflag == DEAD_NO) && pev->health > 0; }
	virtual BOOL IsAlive_FromAI( CBaseMonster* whoWantsToKnow ) { return (pev->deadflag == DEAD_NO) && pev->health > 0; }

	virtual BOOL ShouldFadeOnDeath( void ) { return FALSE; }
	virtual	BOOL IsPlayer( void ) { return TRUE; }			// Spectators should return FALSE for this, they aren't "players" as far as game logic is concerned

	virtual BOOL IsNetClient( void ) { return TRUE; }		// Bots should return FALSE for this, they can't receive NET messages
															// Spectators should return TRUE for this

	

	virtual const char *TeamID( void );

	virtual int		Save( CSave &save );
	virtual int		Restore( CRestore &restore );
	void RenewItems(void);
	void PackDeadPlayerItems( void );
	void RemoveAllItems( BOOL removeSuit );
	BOOL SwitchWeapon( CBasePlayerItem *pWeapon );

	// JOHN:  sends custom messages if player HUD data has changed  (eg health, ammo)
	virtual void UpdateClientData( void );
	
	static	TYPEDESCRIPTION m_playerSaveData[];

	// Player is moved across the transition by other means
	virtual int		ObjectCaps( void ) { return CBaseMonster :: ObjectCaps() & ~FCAP_ACROSS_TRANSITION; }
	virtual void	Precache( void );
	BOOL			IsOnLadder( void );
	BOOL			FlashlightIsOn( void );
	void			FlashlightTurnOn( void );
	void			FlashlightTurnOff( void );
	
	void UpdatePlayerSound ( void );

	void DeathSound ( void );
	//MODDD - new argument possibility, see player.cpp for more info.
	void DeathSound ( BOOL plannedRevive);

	//MODDD - new.  MOVED TO CBaseMonster.
	//int convert_itbd_to_damage(int i);


	int Classify ( void );
	void SetAnimation( PLAYER_ANIM playerAnim );
	void SetWeaponAnimType( const char *szExtention );
	char m_szAnimExtention[32];

	// custom player functions
	virtual void ImpulseCommands( void );
	void CheatImpulseCommands( int iImpulse );

	void StartDeathCam( void );
	void StartObserver( Vector vecPosition, Vector vecViewAngle );

	void AddPoints( int score, BOOL bAllowNegativeScore );
	void AddPointsToTeam( int score, BOOL bAllowNegativeScore );
	BOOL AddPlayerItem( CBasePlayerItem *pItem );

	//MODDD - new
	void printOutWeapons(void);
	BOOL CanAddPlayerItem( int arg_iItemSlot, const char* arg_classname, const char* arg_ammoname, int arg_iMaxAmmo);
	


	BOOL RemovePlayerItem( CBasePlayerItem *pItem );
	void DropPlayerItem ( char *pszItemName );
	BOOL HasPlayerItem( CBasePlayerItem *pCheckItem );
	//MODDD - new
    BOOL HasPlayerItem( int arg_iItemSlot, const char* arg_className );

	BOOL HasNamedPlayerItem( const char *pszItemName );
	//MODDD
	CBasePlayerItem* FindNamedPlayerItem(const char* pszItemName );

	BOOL HasWeapons( void );// do I have ANY weapons?
	void SelectPrevItem( int iItem );
	void SelectNextItem( int iItem );
	void SelectLastItem(void);
	void SelectItem(const char *pstr);

	void setActiveItem(CBasePlayerItem* argItem);

	void ItemPreFrame( void );
	void ItemPostFrame( void );

	//MODDD - new.  Calls "GiveNamedItem" IF the player does not have the item named "pszName".
	void GiveNamedItemIfLacking( const char *pszName );

	//MODDD - new versions too.
	//default (no extra args, just the name) uses the name of the thing to spawn.  Assumes the player's origin (pev->origin) is the spawn point.
	//If given 3 coords, turn that into a vector and send.
	//With a vector, use that as the origin instead.
	//GiveNamedItem, return the created item's edict for possible use again.
	
	
	
	edict_t* GiveNamedItem( const char *pszName );
	edict_t* GiveNamedItem( const char *pszName, float xCoord, float yCoord, float zCoord );
	edict_t* GiveNamedItem( const char *pszName, float xCoord, float yCoord, float zCoord, BOOL factorSpawnSize );
	edict_t* GiveNamedItem( const char *pszName, const Vector& coord );
	edict_t* GiveNamedItem( const char *pszName, const Vector& coord, BOOL factorSpawnSize );
	
	edict_t* GiveNamedItem( const char *pszName, int pszSpawnFlags );
	edict_t* GiveNamedItem( const char *pszName, int pszSpawnFlags, float xCoord, float yCoord, float zCoord );
	edict_t* GiveNamedItem( const char *pszName, int pszSpawnFlags, float xCoord, float yCoord, float zCoord, BOOL factorSpawnSize );
	edict_t* GiveNamedItem( const char *pszName, int pszSpawnFlags, const Vector& coord );
	edict_t* GiveNamedItem( const char *pszName, int pszSpawnFlags, const Vector& coord, BOOL factorSpawnSize );
	edict_t* GiveNamedItem( const char *pszName, int pszSpawnFlags, float xCoord, float yCoord, float zCoord, BOOL factorSpawnSize, TraceResult* tr );
	edict_t* GiveNamedItem( const char *pszName, int pszSpawnFlags, const Vector& coord, BOOL factorSpawnSize, TraceResult* tr );
	





	
	
	void EnableControl(BOOL fControl);

	int  GiveAmmo( int iAmount, char *szName, int iMax );
	void SendAmmoUpdate(void);

	void WaterMove( void );
	void EXPORT PlayerDeathThink( void );
	void PlayerUse( void );

	void CheckSuitUpdate();


	
	BOOL SetSuitUpdatePRE();
	BOOL SetSuitUpdatePRE(BOOL fvoxException);
	BOOL SetSuitUpdatePRE(char *name, int fgroup, int& isentence);
	BOOL SetSuitUpdatePRE(char *name, int fgroup, int& isentence, BOOL fvoxException);

	BOOL SetSuitUpdatePOST(int iempty, int isentence, float fNoRepeatTime, float playDuration, BOOL canPlay);
	
	BOOL SetSuitUpdateEventPOST(int iempty, int isentence, float fNoRepeatTime, float playDuration, BOOL canPlay, float eventDelay, void (CBasePlayer::*eventMethod)()  );

	BOOL SetSuitUpdateEventFVoxCutoffPOST(int iempty, int isentence, float fNoRepeatTime, float playDuration, BOOL canPlay, float eventDelay, void (CBasePlayer::*eventMethod)(), float fvoxCutoff );


	BOOL SetSuitUpdateNoRepeatSweep(int& iempty, int isentence);
	BOOL SetSuitUpdateCheckNoRepeatApply(int& iempty, int isentence);
	BOOL SetSuitUpdateCheckNoRepeat(int& iempty, int isentence);
	
	void SetSuitUpdateNumber(int number, float fNoRepeatTime, int noRepeatID, BOOL arg_getBatteryValueRealTime);
	void SetSuitUpdateNumber(int number, float fNoRepeatTime, int noRepeatID, BOOL arg_getBatteryValueRealTime, BOOL fvoxException);


	

	void SetSuitUpdateFVoxException(char *name, int fgroup, float fNoRepeatTime);
	void SetSuitUpdateFVoxException(char *name, int fgroup, float fNoRepeatTime, float playDuration);
	void SetSuitUpdate(char *name, int fgroup, float fNoRepeatTime);
	void SetSuitUpdate(char *name, int fgroup, float fNoRepeatTime, float playDuration);
	void SetSuitUpdate(char *name, int fgroup, float fNoRepeatTime, float playDuration, BOOL fvoxException );


	void SetSuitUpdateEvent(char *name, int fgroup, float fNoRepeatTime, float eventDelay, void (CBasePlayer::*eventMethod)() );
	void SetSuitUpdateEvent(char *name, int fgroup, float fNoRepeatTime, float playDuration, float eventDelay, void (CBasePlayer::*eventMethod)() );
	void SetSuitUpdateEvent(char *name, int fgroup, float fNoRepeatTime, float playDuration, BOOL fvoxException, float eventDelay, void (CBasePlayer::*eventMethod)() );
	void SetSuitUpdateEventFVoxCutoff(char *name, int fgroup, float fNoRepeatTime, float eventDelay, void (CBasePlayer::*eventMethod)(), float fvoxOffCutoff );
	void SetSuitUpdateEventFVoxCutoff(char *name, int fgroup, float fNoRepeatTime, float playDuration, float eventDelay, void (CBasePlayer::*eventMethod)(), float fvoxOffCutoff );
	void SetSuitUpdateEventFVoxCutoff(char *name, int fgroup, float fNoRepeatTime, float playDuration, BOOL fvoxException, float eventDelay, void (CBasePlayer::*eventMethod)(), float fvoxOffCutoff );



	










	void SetSuitUpdateAndForceBlock(char *name, int fgroup, float fNoRepeatTime);
	void SetSuitUpdateAndForceBlock(char *name, int fgroup, float fNoRepeatTime, float playDuration);
	void SetSuitUpdateAndForceBlock(char *name, int fgroup, float fNoRepeatTime, float playDuration, BOOL fvoxException);

	void forceRepeatBlock(char *name, int fgroup, float fNoRepeatTime);
	void forceRepeatBlock(char *name, int fgroup, float fNoRepeatTime, BOOL fvoxException);
	
	BOOL suitCanPlay(char* name, int fgroup);
	BOOL suitCanPlay(char *name, int fgroup, BOOL fvoxException);

	
	
	void consumeAntidote(void);
	void consumeRadiation(void);
	void consumeAdrenaline(void);
	
	void removeTimedDamage(int arg_type, int* m_bitsDamageTypeRef);








	//MODDD
	int getGeigerChannel();

	void UpdateGeigerCounter( void );
	void CheckTimeBasedDamage( void );

	BOOL FBecomeProne ( void );
	void BarnacleVictimBitten ( entvars_t *pevBarnacle );
	void BarnacleVictimReleased ( void );
	static int GetAmmoIndex(const char *psz);
	int AmmoInventory( int iAmmoIndex );
	int Illumination( void );

	void ResetAutoaim( void );
	Vector GetAutoaimVector( float flDelta  );
	Vector AutoaimDeflection( Vector &vecSrc, float flDist, float flDelta  );

	void ForceClientDllUpdate( void );  // Forces all client .dll specific data to be resent to client.

	void DeathMessage( entvars_t *pevKiller );

	void SetCustomDecalFrames( int nFrames );
	int GetCustomDecalFrames( void );

	void CBasePlayer::TabulateAmmo( void );







	//VARIABLES
	BOOL antidoteQueued;
	BOOL radiationQueued;
	BOOL adrenalineQueued;

	float rawDamageSustained;



	//MODDD - phase these out.
	///////////////////////////////////////////////////////////////////////////
	///////////////////////////////////////////////////////////////////////////
	//MODDD - new.
	void DebugCall1(void);
	void DebugCall2(void);
	void DebugCall3(void);
	
	BOOL debugPoint1Given;
	BOOL debugPoint2Given;
	BOOL debugPoint3Given;
	Vector debugPoint1;
	Vector debugPoint2;
	Vector debugPoint3;

	
	//MODDD
	Vector debugDrawVect;
	Vector debugDrawVectB;
	Vector debugDrawVect2;
	Vector debugDrawVect3;
	Vector debugDrawVect4;
	Vector debugDrawVect5;

	Vector debugDrawVectRecentGive1;
	Vector debugDrawVectRecentGive2;


	///////////////////////////////////////////////////////////////////////////
	///////////////////////////////////////////////////////////////////////////






	

	float m_flStartCharge;
	//MODDD - new
	float m_flStartChargeAnim;
	float m_flStartChargePreSuperDuper;


	float m_flAmmoStartCharge;
	float m_flPlayAftershock;
	float m_flNextAmmoBurn;// while charging, when to absorb another unit of player's ammo?
	
	//Player ID
	void InitStatusBar( void );
	void UpdateStatusBar( void );
	int m_izSBarState[ SBAR_END ];
	float m_flNextSBarUpdateTime;
	float m_flStatusBarDisappearDelay;
	char m_SbarString0[ SBAR_STRING_SIZE ];
	char m_SbarString1[ SBAR_STRING_SIZE ];
	
	float m_flNextChatTime;


	void resetLongJumpCharge();

	//MODDD - added.
	CBasePlayer(void);
	void autoSneakyCheck(void);
	void turnOnSneaky(void);
	void turnOffSneaky(void);

	BOOL queueTotalFOVUpdate;
	BOOL queueZoomFOVUpdate;
	BOOL alreadySpawned;

	cvar_t* sv_cheatsRef;
	//Measuring the amount of time the player can breathe underwater with an air tank.
	BOOL airTankAirTimeNeedsUpdate;
	float airTankAirTime;
	float airTankAirTimeMem;
	float oldWaterMoveTime;
	float longJumpCharge;
	float longJumpChargeMem;
	float longJumpDelay;
	BOOL longJump_waitForRelease;
	BOOL longJumpChargeNeedsUpdate;
	float oldThinkTime;
	float lastDuckVelocityLength;


	//MODD
	float nextMadEffect;

	//MODDD - remember the setting to see if it has changed in a particular frame.
	int auto_adjust_zoomfovMem;
	float pythonZoomFOV;
	float crossbowZoomFOV;
	int auto_adjust_fov_aspectmem;
	int the_default_fovmem;
	int deadflagmem;

	BOOL recentlyGrantedGlockSilencer;

	//float glockSilencerOnVar;

	//float egonAltFireOnVar;


	float cheat_infiniteclipMem;
	float cheat_infiniteammoMem;
	float cheat_minimumfiredelayMem;
	float cheat_minimumfiredelaycustomMem;

	float cheat_nogaussrecoilMem;
	float gaussRecoilSendsUpInSPMem;



	int recoveryIndex;
	int recoveryDelay;

	int recoveryIndexMem;
	int recoveryDelayMin;

	BOOL recentlyGibbed;
	BOOL recentlyGibbedMem;


	cvar_t* the_default_fov;
	/*
	cvar_t* auto_adjust_fov_aspect;
	cvar_t* auto_adjust_zoomfov;
	cvar_t* python_zoomfov;
	cvar_t* crossbow_zoomfov;
	cvar_t* canApplyDefaultFOV;
	*/

	/*
	cvar_t* cheat_infiniteclip;
	cvar_t* cheat_infiniteammo;
	cvar_t* cheat_minimumfiredelay;
	cvar_t* cheat_minimumfiredelaycustom;
	cvar_t* cheat_nogaussrecoil;
	cvar_t* gaussRecoilSendsUpInSP;
	*/



	float canApplyDefaultFOVMem;




	float skillMem;



	cvar_t* timedDamageReviveRemoveMode;

	float timedDamageReviveRemoveModeMem;



	BOOL airTankWaitingStart;


	
	BOOL scheduleRemoveAllItems;
	BOOL scheduleRemoveAllItemsIncludeSuit;

	//NEW METHODS for organization.
	void _commonReset(void);
	void commonReset(void);
	void updateTimedDamageDurations(int difficultyIndex);

	//MODDD - created to differentiate between "m_fLongJump" (always on now) and having ever picked
	//up the long jump item itself, if needed (mostly to satisfy the hazard course).
	BOOL hasLongJumpItem;




	//MODDD - variables to record the currently selected duration for each damage type (based on difficulty).
	//This is easy to get durations often at runtime with little extra processing.
	//~Moved to CBASEMONSTER, made static.
	/*
	float paralyzeDuration;
	float nervegasDuration;
	float poisonDuration;
	float radiationDuration;
	float acidDuration;
	float slowburnDuration;
	float slowfreezeDuration;
	float bleedingDuration;
	*/

	//MOVED TO CBaseMonster
	//BOOL	m_rgbTimeBasedFirstFrame[CDMG_TIMEBASED];

	int hasGlockSilencer;
	int hasGlockSilencerMem;


	int obligedCustomSentence;

	float barnacleCanGibMem;

	cvar_t* fvoxEnabled;
	cvar_t* cl_fvoxMem;

	float fvoxEnabledMem;

	BOOL fvoxOn;

	float recentlySaidBattery;

	//void Think(void);
	//void MonsterThink(void);


	//MODDD - is this the first time in a while the player has been close to radation (false)?
	BOOL foundRadiation;
	BOOL altLadderStep;

	BOOL getBatteryValueRealTime;
	int batterySayPhase;

	BOOL batteryInitiative;

	BOOL alreadyDroppedItemsAtDeath;
	BOOL sentCarcassScent;

	//MODDD - utility.
	void suitSoundFilter(const char* snd);

	//MODDD
	float myRef_barnacleEatsEverything;
	int drowning;  //actually a BOOL, but the client doesn't know what bools are.  Probably wouldn't hurt anyways.
	int drowningMem;


	float playerBrightLightMem;
	
	cvar_t* cl_ladder;
	float cl_ladderMem;



	cvar_t* mp5GrenadeInheritsPlayerVelocity;
	float mp5GrenadeInheritsPlayerVelocityMem;

	float cameraModeMem;


	float mirrorsDoNotReflectPlayerMem;

	
	cvar_t* crossbowInheritsPlayerVelocity;
	float crossbowInheritsPlayerVelocityMem;
	
	cvar_t* fastHornetsInheritsPlayerVelocity;
	float fastHornetsInheritsPlayerVelocityMem;
	
	//cvar_t* autoSneaky;
	float autoSneakyMem;
	
	//cvar_t* infiniteLongJumpCharge;
	float infiniteLongJumpChargeMem;
	

	int framesUntilPushStops;
	
	float pushSpeedMulti;
	float pushSpeedMultiMem;

	float noclipSpeedMultiMem;
	float normalSpeedMultiMem;
	float jumpForceMultiMem;
	float ladderCycleMultiMem;
	float ladderSpeedMultiMem;
	
	int clearWeaponFlag;

	BOOL alreadyPassedLadderCheck;


	BOOL grabbedByBarnacle;
	BOOL grabbedByBarnacleMem;

	float minimumRespawnDelay;



};

#define AUTOAIM_2DEGREES  0.0348994967025
#define AUTOAIM_5DEGREES  0.08715574274766
#define AUTOAIM_8DEGREES  0.1391731009601
#define AUTOAIM_10DEGREES 0.1736481776669


extern BOOL gInitHUD;



#endif // PLAYER_H
