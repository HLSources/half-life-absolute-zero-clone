// have a ifdef guard too, why not
#ifndef BUILD_SETTINGS_H
#define BUILD_SETTINGS_H

// Some quick project-wide macros to customize a build.
// There are still other important ones in other files such as PLAYER_ALWAYSHASLONGJUMP in dlls/player.h.

// The commonly referred to "CLIENT_WEAPONS" is defined in the visual studio projects as constants
// (cl_dlls/hl project, properties -> C/C++ -> Preprocessor -> Preprocessor definitions).
// Others such as "_DEBUG" and "CLIENT_DLL" are intuitive: _DEBUG for the "Debug" configuration for
// either project, "CLIENT_DLL" for cl_dlls looking at a file (useful for shared files that need to 
// do somthing different per client/serverside).



////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////
// SPAWN NAMES
// Little project-wide config, for handling the allowed names for spawning by map or 'give' commands
// (like "monster_human_grunt").
//
// Note that give commands wil still check for whether a version of a name with "monster_" in front
// is valid if the current isn't found.  That is, "give human_grunt" will still check and see that
// "monster_human_grunt" exists, regardless of EXTRA_NAMES choice below.
//Maps do not use this extra check.

#define REMOVE_ORIGINAL_NAMES 0
// 0: no effect.
// 1: original names from as-is script are no longer linked (like monster_human_grunt)

#define EXTRA_NAMES 2
// 0: no extra names.
// 1: Just one alternate name (a version without "monster_";  "human_grunt").
// 2: Same as 1, but also grants possible extra names, such as "hgrunt".
////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////


#define FORCE_PRINTOUT_PREFIX 1
// In debug mode, prefixes for printouts, a "CL" or "SV" label to indicate whether they came from
// the client or server, is included in any 'easyPrintLine' printouts.
// In release mode, this is usually skipped. Unless FORCE_PRINTOUT_PREFIX is 1.



//!!! TODO.  This is rough around the edges, revisit.  Nothing uses PROTECTION_FLAG right now.
// Keep in mind, FCVAR_PROTECTED makes any changes in multiplayer, even as the server owner
// (unsure of rcon but probably too)  impossible.  Seems like overkill.
// Non-dedicated-server-running players are not able to set any server CVars as it is.
// So this feature to let some CVars be 'impossible to change in multiplayer' likely has no use,
// nevermind.

// This controls whether cheat CVars (ones with the PROTECTION_FLAG CVar) can be modified directly
// in multiplayer.
#define CHEATS_ALLOWED_IN_MULTI_PLAYER 1

#if CHEATS_ALLOWED_IN_MULTI_PLAYER == 1
// If not, "PROTECTION_FLAG" is empty, contributing nothing to the CVar flags.
	#define PROTECTION_FLAG
#else
	//If so, the flag "FCVAR_PROTECTED" is added.
	#define PROTECTION_FLAG | FCVAR_PROTECTED
#endif

// If on, alien weapons (hornetgun, snark, chumtoad) go into their own slot #6.
#define SPLIT_ALIEN_WEAPONS_INTO_NEW_SLOT 1


// Test to fool around with scripted's, makes some things better (fixes a3a2, moving too fast at
// the opening = frozen agrunt), but breaks others (a2a1 garg doesn't move).  Yowza, fragile.
#define HACKY_SCRIPT_TEST 0

// little debug feature.  Force "func_door_rotating" to "func_door_health"
// for test maps that still haven't used the new func_door_health name.
// Use 0 for intended behavior otherwise (func_door_health must be used for wallhealth doors now).
#define FORCE_ROTDOOR_TO_HEALTHDOOR 0


// Set to 1 to avoid compiling the "ammo_9mm", "ammo_357", ... "ammo_hornets", "ammo_argrens" 
// named cache variables and any related script.  These are redundant with the player ammo
// array (m_rgAmmo) that can already be easily accessed by PlayerPrimaryAmmoCount.
// Even in the as-is SDK, there was no named cache variable for snark ammo, it used straight
// "m_pPlayer->m_rgAmmo[ m_iPrimaryAmmoType ]", so, odd move.  Clearly using the ammo count
// of the currently equipped ammo can work for anything.
// 0 for retail behavior, although a lot of places have already changed to using 
// PlayerPrimaryAmmoCount anyway, better support for named cached versions would have to 
// be added back, may as well be a macro switch-in at this point if wanted that badly.
#define SKIP_NAMED_AMMO_CACHE 1

// I don't see the point in generating DLL hooks for making weapons entities
// (LINK_ENTITY_TO_CLASS) and compiling any part of ammo pickup classes for clientside.
// This skips both of those if set to 1, 0 for retail behavior.
#define SLIM_WEAPON_CLIENT_COMPILE 1

// If on, a minimal "special" command is supported to let the "+bhop" alias used by
// NGHL in place of "+jump" (mapped to space instead) continue to work as expected.
// Otherwise, the "special" commands called by "bhop" won't work, so continuous jumping
// won't work and holding space underwater won't move upwards (only tapping does).
#define NGHL_BHOP_SUPPORT 1

// Turn on to get easy access to even more information about schedules, like task names
// and recently used schedule enum constants to track down how a schedule was picked.
// Enables a large chunk of util_debugschedule.h and other needed additions
#define DEBUG_SCHEDULE 1
//////////////////////////////////////////////////////////////////////



//***Descriptions seen in the server browser for a game running this mod***
// For CGameRules (gamerules.h).  Was "Half-Life"
#define GAME_NORMAL_DESCRIPTION "Half-Life: AZ"
// For CHalfLifeTeamplay (gamerules_teamplay.h).  Was "HL Teamplay"
#define GAME_TEAMPLAY_DESCRIPTION "HL:AZ Teamplay"



#define BUILD_INFO_TITLE "Half-Life: Absolute Zero Development Build"
// shortened?
//#define BUILD_INFO_TITLE "Half-Life: AZ - Dev Build"

#define BUILD_INFO_DEBUG "DEBUG"
#define BUILD_INFO_RELEASE "RELEASE"


// .............. nothing
#define secret_ymg 0

#endif// BUILD_SETTINGS_H