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
//
// Health.cpp
//
// implementation of CHudHealth class
//

#include "STDIO.H"
#include "STDLIB.H"
#include "MATH.H"

#include "hud.h"
#include "cl_util.h"
#include "parsemsg.h"
#include <string.h>

DECLARE_MESSAGE(m_Health, Health )
DECLARE_MESSAGE(m_Health, Damage )


//MODDD
DECLARE_MESSAGE(m_Health, Drowning )
DECLARE_MESSAGE(m_Health, HUDItemFsh )









//MODDD - changed to refer to the whitened version (more flexible w/ different colors)
//#define PAIN_NAME "sprites/%d_pain.spr"
#define PAIN_NAME "sprites/%d_painmod.spr"

//also, it appears that this is ununsed.  Perhaps it stored the damage-icons at some point?  File no longer exist.
#define DAMAGE_NAME "sprites/%d_dmg.spr"

int giDmgHeight, giDmgWidth;

int giDmgFlags[NUM_DMG_TYPES] = 
{
	DMG_POISON,
	DMG_ACID,
	DMG_FREEZE|DMG_SLOWFREEZE,
	DMG_DROWN,
	DMG_BURN|DMG_SLOWBURN,
	DMG_NERVEGAS, 
	DMG_RADIATION,
	DMG_SHOCK,
	DMG_CALTROP,
	DMG_TRANQ,
	DMG_CONCUSS,
	DMG_HALLUC,
	//MODDD - addition.
	DMG_BLEEDING
};




//MODDD - externs

extern float global2_painArrowColorMode;
extern float global2_painFlashColorMode;
extern float global2_drownDrawPainMode;
extern float global2_allowPainDrawWithoutSuit;
extern float global2_hud_version;
extern float global2_preE3ShowsDamageIcons;
extern float global2_E3ShowsDamageIcons;
extern float global2_skipTFDamageTextures;

extern float global2_timedDamageDeathRemoveMode;

extern float global2_hud_weaponselecthideslower;


extern float global2_painFlashDmgMin;
extern float global2_painFlashDmgExMult;
extern float global2_painFlashCumulativeMinDrowning;
extern float global2_painFlashCumulativeMax;
extern float global2_painFlashDrawOpacityMax;
extern float global2_painArrowDrawOpacityMin;
extern float global2_painArrowDrawOpacityMax;
extern float global2_painFlashFadeMult;
extern float global2_painArrowFadeMult;
extern float global2_painFlashDirTolerance;


extern float global2_painArrowCumulativeAppearMin;
extern float global2_painArrowCumulativeDmgJump;
extern float global2_painFlashPrintouts;

extern float global2_painFlashIgnoreArmor;

extern float global2_itemFlashCumulativeJump;
extern float global2_itemFlashDrawOpacityMax;
extern float global2_itemFlashDrawOpacityMin;
extern float global2_itemFlashFadeMult;





//MODDD - new vars for the new fade system.
float cumulativeFade = 0;
//float fadeIncrement = 0.45;
float fAttackFrontMem = 0;
float fAttackRearMem = 0;
float fAttackRightMem = 0;
float fAttackLeftMem = 0;





CHudHealth::CHudHealth(void){

	itemFlashGiven = 0;
}





int CHudHealth::Init(void)
{
	
	
	HOOK_MESSAGE(Health);
	HOOK_MESSAGE(Damage);
	
	HOOK_MESSAGE(Damage);

	//MODDD
	HOOK_MESSAGE(Drowning);
	HOOK_MESSAGE(HUDItemFsh);
	


	m_iHealth = 100;
	m_fFade = 0;
	m_iFlags = 0;
	m_bitsDamage = 0;
	m_fAttackFront = m_fAttackRear = m_fAttackRight = m_fAttackLeft = 0;
	m_fAttackFrontDamage = m_fAttackRearDamage = m_fAttackRightDamage = m_fAttackLeftDamage = 0;
	giDmgHeight = 0;
	giDmgWidth = 0;

	//This comes from a message sent by the player, "Yes" or "No".
	drowning = FALSE;

	memset(m_dmg, 0, sizeof(DAMAGE_IMAGE) * NUM_DMG_TYPES);


	gHUD.AddHudElem(this);
	return 1;
}

void CHudHealth::Reset( void )
{

	// make sure the pain compass is cleared when the player respawns
	m_fAttackFront = m_fAttackRear = m_fAttackRight = m_fAttackLeft = 0;
	m_fAttackFrontDamage = m_fAttackRearDamage = m_fAttackRightDamage = m_fAttackLeftDamage = 0;


	// force all the flashing damage icons to expire
	m_bitsDamage = 0;
	//MODDD - the new bitmask should be affected too.
	m_bitsDamageMod = 0;

	for ( int i = 0; i < NUM_DMG_TYPES; i++ )
	{
		m_dmg[i].fExpire = 0;
	}


	//MODDD - new
	cumulativeFade = 0;
	fAttackFrontMem = 0;
	fAttackRearMem = 0;
	fAttackRightMem = 0;
	fAttackLeftMem = 0;

	m_fFade = 0;
	m_bitsDamage = 0;
	m_bitsDamageMod = 0;
	itemFlashCumulative = 0;
	m_fAttackFront = m_fAttackRear = m_fAttackRight = m_fAttackLeft = 0;
	m_fAttackFrontDamage = m_fAttackRearDamage = m_fAttackRightDamage = m_fAttackLeftDamage = 0;

}

int CHudHealth::VidInit(void)
{
	m_SpriteHandle_t = 0;

	m_HUD_dmg_bio = gHUD.GetSpriteIndex( "dmg_bio" ) + 1;
	m_HUD_cross = gHUD.GetSpriteIndex( "cross" );

	giDmgHeight = gHUD.GetSpriteRect(m_HUD_dmg_bio).right - gHUD.GetSpriteRect(m_HUD_dmg_bio).left;
	giDmgWidth = gHUD.GetSpriteRect(m_HUD_dmg_bio).bottom - gHUD.GetSpriteRect(m_HUD_dmg_bio).top;
	return 1;
}




int CHudHealth:: MsgFunc_Health(const char *pszName,  int iSize, void *pbuf )
{
	// TODO: update local health data
	BEGIN_READ( pbuf, iSize );
	int x = READ_BYTE();

	m_iFlags |= HUD_ACTIVE;

	// Only update the fade if we've changed health
	if (x != m_iHealth)
	{
		m_fFade = FADE_TIME;
		m_iHealth = x;
	}

	return 1;
}




//MODDD - odd place.
float cumulativeFadeDrown = 0;

int CHudHealth:: MsgFunc_Damage(const char *pszName,  int iSize, void *pbuf )
{
	BEGIN_READ( pbuf, iSize );

	int armor = READ_BYTE();	// armor
	int damageTaken = READ_BYTE();	// health
	int rawDamageTaken = READ_BYTE();	// MODDD NEW - raw damage, before armor reduction (what was intended)
	long bitsDamage = READ_LONG(); // damage bits

	//MODDD
	long bitsDamageMod = READ_LONG(); // damage bits



	vec3_t vecFrom;

	for ( int i = 0 ; i < 3 ; i++)
		vecFrom[i] = READ_COORD();

	//MODDD
	//UpdateTiles(gHUD.m_flTime, bitsDamage);
	UpdateTiles(gHUD.m_flTime, bitsDamage, bitsDamageMod);



	
	gHUD.recentDamageBitmask = bitsDamage;

	if(bitsDamage & DMG_DROWN){
		//if this is "drown" damage, get how to draw pain differnetly (default is nothing at all)
		if(global2_drownDrawPainMode == 2){
			//just do this.
			cumulativeFadeDrown = 1.0f;
			//return 1;
		}else if(global2_drownDrawPainMode == 3){
			//m_fAttackFront = m_fAttackRear = m_fAttackRight = m_fAttackLeft = 1;
			//MODDD TODO - ditto.

			
			const float damageFlashModTotal = damageTaken + rawDamageTaken * global2_painFlashIgnoreArmor;

			m_fAttackFront = m_fAttackRear = m_fAttackRight = m_fAttackLeft = 0.25;
			setUniformDamage(damageFlashModTotal);
			return 1;
		}
	}


	//...is "armor" unused?  It comes from "pev->dmg_save". Does it have any purpose than to
	//trigger a damage draw on any "takeDamage" event, even if the damage is 0?

	if(global2_painFlashPrintouts == 1)easyForcePrintLine("RAW DAMAGE %d %d", armor, damageTaken);
	// Actually took damage
	//if ( damageTaken > 0 || armor > 0 )
	if ( damageTaken > 0 || (global2_painFlashIgnoreArmor > 0 && rawDamageTaken > 0) || armor > 0 )
		CalcDamageDirection(vecFrom, damageTaken, rawDamageTaken);


	return 1;
}





int CHudHealth:: MsgFunc_Drowning(const char *pszName,  int iSize, void *pbuf )
{
	BEGIN_READ( pbuf, iSize );
	int x = READ_BYTE();
	drowning = x;
	return 1;
}



int CHudHealth:: MsgFunc_HUDItemFsh(const char *pszName,  int iSize, void *pbuf )
{
	BEGIN_READ( pbuf, iSize );
	int determiner = READ_BYTE();
	
	if(determiner == 0){
		//antidote. yellow (gold?)
		itemFlashColorStartR = 255;
		itemFlashColorStartG = 255;
		itemFlashColorStartB = 0;
		itemFlashGiven = 1;
		itemFlashCumulative = global2_itemFlashCumulativeJump;

	}else if(determiner == 1){
		//radiation. green. (lime; bright?)
		itemFlashColorStartR = 0;
		itemFlashColorStartG = 255;
		itemFlashColorStartB = 0;
		itemFlashGiven = 1;
		itemFlashCumulative = global2_itemFlashCumulativeJump;

	}else if(determiner == 2){
		//adrenaline. orange (red?)
		itemFlashColorStartR = 255;
		itemFlashColorStartG = 127;
		itemFlashColorStartB = 0;
		itemFlashGiven = 1;
		itemFlashCumulative = global2_itemFlashCumulativeJump;

	}


	return 1;
}





// Returns back a color from the
// Green <-> Yellow <-> Red ramp
void CHudHealth::GetPainColor( int &r, int &g, int &b )
{
	int iHealth = m_iHealth;

	if (iHealth > 25)
		iHealth -= 25;
	else if ( iHealth < 0 )
		iHealth = 0;
#if 0
	g = iHealth * 255 / 100;
	r = 255 - g;
	b = 0;
#else
	//easyPrintLine("MY HEALTH? %d", m_iHealth);
	if (m_iHealth > 25)
	{
		//MODDD - use our new generic orange instead:
		//UnpackRGB(r,g,b, RGB_YELLOWISH);
		gHUD.getGenericOrangeColor(r, g, b);
	}
	else
	{
		r = 250;
		g = 0;
		b = 0;
	}
#endif 
}

int CHudHealth::Draw(float flTime)
{

	int r, g, b;
	int a = 0, x, y;



	//MODDD - moved here to prevent mentioned bug.  Also, note that the name PAIN_NAME has been altered to use a white-version (the red isn't as flexible with different colors)
	if ( !m_SpriteHandle_t )
		m_SpriteHandle_t = LoadSprite(PAIN_NAME);





	
	//MODDD - only draw this if the weapon select screen is not on.
	//if(gHUD.canDrawBottomStats){
	if( !(!gHUD.canDrawBottomStats && global2_hud_weaponselecthideslower == 1)  ){

		int HealthWidth;


		if ( (gHUD.m_iHideHUDDisplay & HIDEHUD_HEALTH) || gEngfuncs.IsSpectateOnly() )
			return 1;


		//MODDD - BUG FIX - if going to a new map ("loading..."), maybe loading a game, the pain sprite will be one of the spritesheets. Very strange.  Moved above.
		//if ( !m_SpriteHandle_t )
		//	m_SpriteHandle_t = LoadSprite(PAIN_NAME);
	
		// Has health changed? Flash the health #
		if (m_fFade)
		{
			m_fFade -= (gHUD.m_flTimeDelta * 20);
			if (m_fFade <= 0)
			{
				a = MIN_ALPHA;
				m_fFade = 0;
			}

			// Fade the health number back to dim

			a = MIN_ALPHA +  (m_fFade/FADE_TIME) * 128;

		}
		else
			a = MIN_ALPHA;

		// If health is getting low, make it bright red
		if (m_iHealth <= 15)
			a = 255;
		

		deriveColorFromHealth(r, g, b, a);


		// Only draw health if we have the suit.
		if (gHUD.m_iWeaponBits & (1<<(WEAPON_SUIT)))
		{
			//Use Box Numbers instead (m_HUD_number_0 + 10 gets boxed number 0).
			//HealthWidth = gHUD.GetSpriteRect(gHUD.m_HUD_number_0).right - gHUD.GetSpriteRect(gHUD.m_HUD_number_0).left;
			HealthWidth = gHUD.GetSpriteRect(gHUD.m_HUD_number_0+10).right - gHUD.GetSpriteRect(gHUD.m_HUD_number_0+10).left;
		
		
			//MODDD - new height var.
			int HealthHeight = gHUD.GetSpriteRect(gHUD.m_HUD_number_0+10).bottom - gHUD.GetSpriteRect(gHUD.m_HUD_number_0+10).top;


			if(global2_hud_version == 0){

				//MODDD - say "true" for "useBoxedNumbers".  Also, set x to HealthWidth / 2 (commented out above) and set y properly.
				x = HealthWidth /2;
				//MODDD - higher
				y = ScreenHeight - ((int)(HealthHeight*1.5)) - 1;

				//If not dead, draw normally.
				x = gHUD.DrawHudNumber(x, y, DHN_3DIGITS | DHN_DRAWZERO, m_iHealth, r, g, b, 1, 1);


				x += HealthWidth/2;


			}else{

			
				//(41, 544)
				//DHN_3DIGITS | DHN_DRAWZERO
				x = 16 + 19 - 24 - 1;
				y = ScreenHeight - 45;
				x = gHUD.DrawHudNumber(x, y, DHN_3DIGITS|DHN_DRAWPLACE|DHN_DRAWZERO|DHN_EMPTYDIGITSUNFADED , m_iHealth, r, g, b, 3);
			
				gHUD.getGenericOrangeColor(r, g, b);
			
				//Hard-coded bar graphic.
				x = 79;
				y = ScreenHeight - 48;
				int iHeight = 33;
				int iWidth = 3;
				FillRGBA(x, y, iWidth, iHeight, r, g, b, a);

			}

		
		}

	}//END OF if(canDraw)

	//MODDD - pretty sure pain effects should still be drawn regardless.
	DrawDamage(flTime);
	DrawItemFlash(flTime);
	return DrawPain(flTime);
}

void CHudHealth::CalcDamageDirection(vec3_t vecFrom, int damageAmount, int rawDamageAmount)
{
	vec3_t	forward, right, up;
	float	side, front;
	vec3_t vecOrigin, vecAngles;

	if (!vecFrom[0] && !vecFrom[1] && !vecFrom[2])
	{
		//easyPrintLine("OH NO I HAVE FAILED");
		m_fAttackFront = m_fAttackRear = m_fAttackRight = m_fAttackLeft = 0;
		setUniformDamage(0);
		return;
	}


	int damageBlockedByArmor = rawDamageAmount - damageAmount;
	//this gets reduced by a factor of, "painFlashIgnoreArmor".


	memcpy(vecOrigin, gHUD.m_vecOrigin, sizeof(vec3_t));
	memcpy(vecAngles, gHUD.m_vecAngles, sizeof(vec3_t));


	VectorSubtract (vecFrom, vecOrigin, vecFrom);

	float flDistToTarget = vecFrom.Length();

	vecFrom = vecFrom.Normalize();
	AngleVectors (vecAngles, forward, right, up);

	front = DotProduct (vecFrom, right);
	side = DotProduct (vecFrom, forward);


	const float damageFlashModTotal = damageAmount + rawDamageAmount * global2_painFlashIgnoreArmor;


	if (flDistToTarget <= 50)
	{
		//MODDD TODO - should this be changed to "0.25", to set all directions to slight?
		//             At least for the screen flash.  Arrows are okay getting brightened like this.
		//m_fAttackFront = m_fAttackRear = m_fAttackRight = m_fAttackLeft = 1;
		m_fAttackFront = m_fAttackRear = m_fAttackRight = m_fAttackLeft = 0.25;
		setUniformDamage(damageFlashModTotal);
	}
	else 
	{

		const float dirStrictness = 1 - global2_painFlashDirTolerance;

		if (side > 0)
		{
			//MODDD - right-hand side was 0.3!!!
			//if (side > 0.3){
			if(side > dirStrictness){
				m_fAttackFront = max(m_fAttackFront, side);
				m_fAttackFrontDamage = damageFlashModTotal;
			}
		}
		else
		{
			float f = fabs(side);
			if (f > dirStrictness){
				m_fAttackRear = max(m_fAttackRear, f);
				m_fAttackRearDamage = damageFlashModTotal;
			}
		}

		if (front > 0)
		{
			if (front > dirStrictness){
				m_fAttackRight = max(m_fAttackRight, front);
				m_fAttackRightDamage = damageFlashModTotal;
			}
		}
		else
		{
			float f = fabs(front);
			if (f > dirStrictness){
				m_fAttackLeft = max(m_fAttackLeft, f);
				m_fAttackLeftDamage = damageFlashModTotal;
			}
		}
	}
	//easyPrintLine("EEE CLIENT DAMAGE STAT: %.2f %.2f %.2f %.2f", m_fAttackFront, m_fAttackRear, m_fAttackRight, m_fAttackLeft);
}






//Do NOT supply alpha, the 1st is the mode, not r, g, or b (those 3 follow)!
void CHudHealth::getPainColorMode(int mode, int& r, int& g, int& b){

	switch(mode){
		case 0:
			//nothing.  Plan on not drawing at all.

		break;
		case 1: 
		//note that "1" is reserved for saying, fetch modes 2, 3, or 4 based on the current GUI setup (Pre-E3 or E3).
			if(global2_hud_version == 0){
				getPainColorMode(4, r, g, b);	
			}else{
				getPainColorMode(2, r, g, b);
			}
		break;
		case 2:
			//pure bright red.
			r = 255;
			g = 0;
			b = 0;
		break;
		case 3:
			//green.
			gHUD.getGenericGreenColor(r, g, b);
		break;
		case 4:
			gHUD.m_Health.deriveColorFromHealth(r, g, b);
		break;

		}

}



int CHudHealth::DrawItemFlash(float flTime){
	

	if(itemFlashCumulative == 0){
		//no cumulative?  skip this.
		return 1;
	}

	int r, g, b;
	int x, y, a, shade;

	//NOTE - drawing "itemFlash" here too.

	r = itemFlashColorStartR;
	g = itemFlashColorStartG;
	b = itemFlashColorStartB;
	a = 255;

	const float fFade = gHUD.m_flTimeDelta * global2_itemFlashFadeMult;
	
	
	//if(itemFlashCumulative > 0.00){
		//maxAttackFade += 0.2;
		//GetPainColor(r,g,b);
		//getPainColorMode((int)global2_painFlashColorMode, r, g, b);
		
		
		//Clip cumulativeFade to 1.00, as you can't shade anymore intensely than 255, opaque.
		//(as opaque as the engine will allow, it seems)
		
		
		//PENDING (???)
		//if(global2_itemFlashColorMode > 0){
			//draw it.

			//WAS 1.0 on the right!!
			shade = a * max(min( itemFlashCumulative * global2_itemFlashDrawOpacityMax, global2_itemFlashDrawOpacityMax ), global2_itemFlashDrawOpacityMin);
			//!!!!!

			//shade = a * 1;
			ScaleColors(r, g, b, shade);
			FillRGBA(0, 0, ScreenWidth, ScreenHeight, r, g, b, shade);
		//}

	//}

	//global2_painArrowDrawOpacityMax
	

	//reduce the cumulative fade by "fFade", time.
	//if(itemFlashCumulative > cumulativeFadeMinimum){
	itemFlashCumulative = max( 0, itemFlashCumulative - fFade );

	//}


	return 1;
}//END OF DrawItemFlash(...)






int CHudHealth::DrawPain(float flTime)
{




	
	if ( global2_allowPainDrawWithoutSuit == 0 && !(gHUD.m_iWeaponBits & (1<<(WEAPON_SUIT)) ) )
		return 1;



	//MODDD - no longer effective for the new cumulative method.
	/*
	if (!(m_fAttackFront || m_fAttackRear || m_fAttackLeft || m_fAttackRight))
		return 1;
	*/

	int r, g, b;
	int x, y, a, shade;

	// TODO:  get the shift value of the health
	a = 255;	// max brightness until then

	//MODDD - changing the multiple on the Delta changes the rate of fade change.
	//float fFade = gHUD.m_flTimeDelta * 2;
	const float fFade = gHUD.m_flTimeDelta * global2_painFlashFadeMult;

	const float fFadeARROW = gHUD.m_flTimeDelta * global2_painArrowFadeMult;

	





	//MODDD - FINAL PAIN SYSTEM:  a combination of the cumulative fade and fade arrows:

	//So, if the m_fAttack<dir> is above 0 (updated recently), the copy of that direction var for this file (fAttack<dir>Mem) is assigned it.
	//The m_fAttack<dir> is set to 0 so that it does not increment cumulativeFade multiple times for one call.
	//fAttack<dir>Mem is then used by the old fade effect to work with the version of the time-based var that would behave like the old
	//were never changed.
	
	//ALSO: on drowning, the pain effect may get a minimum above 0 (constant red).
	float cumulativeFadeMinimum = 0;

	if(global2_drownDrawPainMode == 1 && drowning){
		cumulativeFadeMinimum = global2_painFlashCumulativeMinDrowning;		
	}
	//easyForcePrintLine("OH naw %.2f %d %.2f", global2_drownDrawPainMode, drowning, global2_painFlashCumulativeMinDrowning);


	
	int allowArrows = 1;
	int allowFlash = 1;
	
	//if(gHUD.recentDamageBitmask & DMG_DROWN){
	if(drowning){
		if(global2_drownDrawPainMode == 2){
			//allowFlash = 0;

			if(cumulativeFadeDrown > 0.00){
				//maxAttackFade += 0.2;
				//GetPainColor(r,g,b);
				getPainColorMode((int)global2_painFlashColorMode, r, g, b);
		
				//Clip cumulativeFade to 1.00, as you can't shade anymore intensely than 255, opaque.
				//(as opaque as the engine will allow, it seems)
		
				if(global2_painFlashColorMode > 0 ){
					//draw it.
					shade = a * min( cumulativeFadeDrown, 1.00 );
					//shade = a * 1;
					ScaleColors(r, g, b, shade);
					FillRGBA(0, 0, ScreenWidth, ScreenHeight, r, g, b, shade);
				}

			}
			//reduce the cumulative fade by "fFade", time.
			if(cumulativeFadeDrown > 0){
				cumulativeFadeDrown = max( 0, cumulativeFadeDrown - fFade );
			}


		}else{

		}
	}
	
	cumulativeFade = max(cumulativeFadeMinimum, cumulativeFade);
	
	
	if(m_fAttackFront > 0){
		const float fadeContributionARROW = m_fAttackFront * global2_painArrowCumulativeDmgJump;
		fAttackFrontMem = fadeContributionARROW;
		const float fadeContribution = global2_painFlashDmgMin + m_fAttackFront * m_fAttackFrontDamage * (global2_painFlashDmgExMult);
		//const float fadeContributionARROW = ...
		cumulativeFade += fadeContribution;
		if(global2_painFlashPrintouts == 1)easyForcePrintLine("OW front: raw:%.2f dam:%.2f result:%.2f resultA:%.2f", m_fAttackRight, m_fAttackRightDamage, fadeContribution, fadeContributionARROW);
		m_fAttackFront = 0;
	}
	if(m_fAttackRear > 0){
		const float fadeContributionARROW = m_fAttackRear * global2_painArrowCumulativeDmgJump;
		fAttackRearMem = fadeContributionARROW;
		const float fadeContribution = global2_painFlashDmgMin + m_fAttackRear * m_fAttackRearDamage * (global2_painFlashDmgExMult);
		cumulativeFade += fadeContribution;
		if(global2_painFlashPrintouts == 1)easyForcePrintLine("OW rear raw:%.2f dam:%.2f result:%.2f resultA:%.2f", m_fAttackRight, m_fAttackRightDamage, fadeContribution, fadeContributionARROW);
		m_fAttackRear = 0;
	}
	if(m_fAttackLeft > 0){
		const float fadeContributionARROW = m_fAttackLeft * global2_painArrowCumulativeDmgJump;
		fAttackLeftMem = fadeContributionARROW;
		const float fadeContribution = global2_painFlashDmgMin + m_fAttackLeft * m_fAttackLeftDamage * (global2_painFlashDmgExMult);
		cumulativeFade += fadeContribution;
		if(global2_painFlashPrintouts == 1)easyForcePrintLine("OW left raw:%.2f dam:%.2f result:%.2f resultA:%.2f", m_fAttackRight, m_fAttackRightDamage, fadeContribution, fadeContributionARROW);
		m_fAttackLeft = 0;
		
	}
	if(m_fAttackRight > 0){
		const float fadeContributionARROW = m_fAttackRight * global2_painArrowCumulativeDmgJump;
		fAttackRightMem = fadeContributionARROW;
		const float fadeContribution = global2_painFlashDmgMin + m_fAttackRight * m_fAttackRightDamage * (global2_painFlashDmgExMult);
		cumulativeFade += fadeContribution;
		if(global2_painFlashPrintouts == 1)easyForcePrintLine("OW right dir:%.2f dam:%.2f result:%.2f resultA:%.2f", m_fAttackRight, m_fAttackRightDamage, fadeContribution, fadeContributionARROW);
		m_fAttackRight = 0;
		
	}



	//cumulativeFade is capped at "1.2", so if lots of damage is taken, it will remain at max intensity for 0.2 seconds at most
	//(if no further damage comes to bump it again)



	//was 1.2 on the right!!!
	cumulativeFade = min(cumulativeFade, global2_painFlashCumulativeMax);
	//!!!!!


	
	//easyPrintLine("cumula %.2f", cumulativeFade);

	if(cumulativeFade > 0.00){
		//maxAttackFade += 0.2;
		//GetPainColor(r,g,b);
		getPainColorMode((int)global2_painFlashColorMode, r, g, b);
		
		
		//Clip cumulativeFade to 1.00, as you can't shade anymore intensely than 255, opaque.
		//(as opaque as the engine will allow, it seems)
		
		
		if(global2_painFlashColorMode > 0 && allowFlash == 1){
			//draw it.

			//WAS 1.0 on the right!!
			shade = a * min( cumulativeFade * global2_painFlashDrawOpacityMax, global2_painFlashDrawOpacityMax );
			//!!!!!

			//shade = a * 1;
			ScaleColors(r, g, b, shade);
			FillRGBA(0, 0, ScreenWidth, ScreenHeight, r, g, b, shade);
		}

	}

	//global2_painArrowDrawOpacityMax
	

	//reduce the cumulative fade by "fFade", time.
	if(cumulativeFade > cumulativeFadeMinimum){
		cumulativeFade = max( cumulativeFadeMinimum, cumulativeFade - fFade );
	}




	//MODDD - and below is the original pain system, theoretically unabridged.
	//This time around, the difference is that "fAttack<dir>mem" vars are used instead, see the explanation above.
	// SPR_Draw top


	
	if(global2_painArrowColorMode > 0 && allowArrows == 1){
	
		if (fAttackFrontMem > global2_painArrowCumulativeAppearMin)
		{
			//GetPainColor(r,g,b);
			getPainColorMode((int)global2_painArrowColorMode, r, g, b);
			
			shade = a * min(max( fAttackFrontMem*global2_painArrowDrawOpacityMax, global2_painArrowDrawOpacityMin ), global2_painArrowDrawOpacityMax) ;
			ScaleColors(r, g, b, shade);
			SPR_Set(m_SpriteHandle_t, r, g, b );

			x = ScreenWidth/2 - SPR_Width(m_SpriteHandle_t, 0)/2;
			y = ScreenHeight/2 - SPR_Height(m_SpriteHandle_t,0) * 3;
			SPR_DrawAdditive(0, x, y, NULL);
			fAttackFrontMem = max( 0, fAttackFrontMem - fFadeARROW );
		} else
			fAttackFrontMem = 0;

		if (fAttackRightMem > global2_painArrowCumulativeAppearMin)
		{
			//GetPainColor(r,g,b);
			getPainColorMode((int)global2_painArrowColorMode, r, g, b);
			
			shade = a * min(max( fAttackRightMem*global2_painArrowDrawOpacityMax, global2_painArrowDrawOpacityMin ), global2_painArrowDrawOpacityMax) ;
			ScaleColors(r, g, b, shade);
			SPR_Set(m_SpriteHandle_t, r, g, b );

			x = ScreenWidth/2 + SPR_Width(m_SpriteHandle_t, 1) * 2;
			y = ScreenHeight/2 - SPR_Height(m_SpriteHandle_t,1)/2;
			SPR_DrawAdditive(1, x, y, NULL);
			fAttackRightMem = max( 0, fAttackRightMem - fFadeARROW );
		} else
			fAttackRightMem = 0;

		if (fAttackRearMem > global2_painArrowCumulativeAppearMin)
		{
			//GetPainColor(r,g,b);
			getPainColorMode((int)global2_painArrowColorMode, r, g, b);
			
			shade = a * min(max( fAttackRearMem*global2_painArrowDrawOpacityMax, global2_painArrowDrawOpacityMin ), global2_painArrowDrawOpacityMax) ;
			ScaleColors(r, g, b, shade);
			SPR_Set(m_SpriteHandle_t, r, g, b );

			x = ScreenWidth/2 - SPR_Width(m_SpriteHandle_t, 2)/2;
			y = ScreenHeight/2 + SPR_Height(m_SpriteHandle_t,2) * 2;
			SPR_DrawAdditive(2, x, y, NULL);
			fAttackRearMem = max( 0, fAttackRearMem - fFadeARROW );
		} else
			fAttackRearMem = 0;

		if (fAttackLeftMem > global2_painArrowCumulativeAppearMin)
		{
			//GetPainColor(r,g,b);
			getPainColorMode((int)global2_painArrowColorMode, r, g, b);
			
			shade = a * min(max( fAttackLeftMem*global2_painArrowDrawOpacityMax, global2_painArrowDrawOpacityMin ), global2_painArrowDrawOpacityMax) ;
			ScaleColors(r, g, b, shade);
			SPR_Set(m_SpriteHandle_t, r, g, b );

			x = ScreenWidth/2 - SPR_Width(m_SpriteHandle_t, 3) * 3;
			y = ScreenHeight/2 - SPR_Height(m_SpriteHandle_t,3)/2;
			SPR_DrawAdditive(3, x, y, NULL);

			fAttackLeftMem = max( 0, fAttackLeftMem - fFadeARROW );
		} else
			fAttackLeftMem = 0;

	}//END OF if(painFlashColorModeVar != 0)
	
	
	//MODDD - OVERVIEW OF NEW CUMULATIVE FADE METHOD:
	//~Meant to mimic the fade of this youtube video:
	//https://www.youtube.com/watch?v=GWqNkOMyKAA
	//For any attack in any of the directions, if it is above 0, "cumulativeFade" is upped by "fadeIncrement".
	//
	

	return 1;
}


//MODDD - method added so that any changes to health-related colors across the program are consistent.
void CHudHealth::deriveColorFromHealth(int &r, int &g, int &b, int &a){
	
	
	if(global2_hud_version == 0){
		//NOTICE: not involving the "a" value (for scaling colors) in Pre E3.
		
		//How bright can all of the color components (r, g, b) potentially be?  (0 - 255)
		const int brightness = COLOR_PRE_E3_BRIGHTNESS;

		//At what point is the GUI going from green to yellow? Measure of health (0 - 100)
		const int yellowMark = 70;
	
		if(m_iHealth >= yellowMark){
			//r = (int) (( ( -m_iHealth +yellowMark+100  ) /((float)yellowMark)) *175 );
			r = (int) (( ( -m_iHealth +100  ) /((float) (100-yellowMark) )) *brightness );
			g = brightness;
			b = 5;
		}else{
			r = brightness;
			g = (int) (( ( m_iHealth  ) /((float)yellowMark)) *brightness );
			b = 5;
		}

	}else{

		/*
		//modled after retail's color for health:
		if(m_iHealth > 15){
			gHUD.getGenericOrangeColor(r, g, b);
			a = 175;
		}else{
			r = 250;
			g = 0;
			b = 0;
			a = 255;
		}
		*/
		//...Or we can just go ahead and use the retail's color.
		
		GetPainColor( r, g, b );
		ScaleColors(r, g, b, a );


	}
}





//MODDD - version that accepts no alpha.
void CHudHealth::deriveColorFromHealth(int &r, int &g, int &b){
	
	
	if(global2_hud_version == 0){
		//NOTICE: not involving the "a" value (for scaling colors) in Pre E3.
		
		//How bright can all of the color components (r, g, b) potentially be?  (0 - 255)
		const int brightness = COLOR_PRE_E3_BRIGHTNESS;

		//At what point is the GUI going from green to yellow? Measure of health (0 - 100)
		const int yellowMark = 70;
	
		if(m_iHealth >= yellowMark){
			//r = (int) (( ( -m_iHealth +yellowMark+100  ) /((float)yellowMark)) *175 );
			r = (int) (( ( -m_iHealth +100  ) /((float) (100-yellowMark) )) *brightness );
			g = brightness;
			b = 5;
		}else{
			r = brightness;
			g = (int) (( ( m_iHealth  ) /((float)yellowMark)) *brightness );
			b = 5;
		}

	}else{

		/*
		//modled after retail's color for health:
		if(m_iHealth > 15){
			gHUD.getGenericOrangeColor(r, g, b);
			a = 175;
		}else{
			r = 250;
			g = 0;
			b = 0;
			a = 255;
		}
		*/
		//...Or we can just go ahead and use the retail's color.
		
		GetPainColor( r, g, b );
		//ScaleColors(r, g, b, a );   not here.

	}
}









int CHudHealth::DrawDamage(float flTime)
{
	int r, g, b, a = 0;
	DAMAGE_IMAGE *pdmg;



	//MODDD - can't draw damage without a suit (these are part of the suit's UI)
	if (!(gHUD.m_iWeaponBits & (1<<(WEAPON_SUIT)) ))
		return 1;

	//MODDD
	//if (!m_bitsDamage)
	if (!m_bitsDamage && ! m_bitsDamageMod)
		return 1;

	//MODDD
	if(gHUD.m_fPlayerDead && global2_timedDamageDeathRemoveMode == 1){
		//do not render anymore.
		return 1;
	}

	if(!gHUD.canDrawBottomStats){
		//do not draw damage indicators if showing the weapon-select.
		return 1;
	}

	//MODDD - if the damage-type indicator were used, it would probably use the same color scheme as the health?
	/*
	UnpackRGB(r,g,b, RGB_YELLOWISH);
	
	a = (int)( fabs(sin(flTime*2)) * 256.0);
	ScaleColors(r, g, b, a);
	*/
	//deriveColorFromHealth(r, g, b, a);

	// Draw all the items
	int i;
	//MODDD - removed.
	

	if( (global2_hud_version == 1 && global2_E3ShowsDamageIcons == 1) || (global2_hud_version == 0 && global2_preE3ShowsDamageIcons == 1)  ){

		int extraYOffset = 0;

		if(global2_hud_version == 0){
			extraYOffset = -41;
		}

		//UnpackRGB(r,g,b, RGB_YELLOWISH);
		//Not working out so well for whatever reason, using this instead:
		
		//gHUD.getGenericOrangeColor(r, g, b);
		deriveColorFromHealth(r, g, b);


		a = (int)( fabs(sin(flTime*2)) * 256.0);
		ScaleColors(r, g, b, a);

		
		for (i = 0; i < NUM_DMG_TYPES; i++){

			int imaoffset = 0;
			//if not loading the TF2 textures, it will throw the image-index offset off.   Adjust as necessary.
			if(global2_skipTFDamageTextures == 1){
				if(i > 7){
					imaoffset = -4;
				}
			}

			int* m_rgbTimeBasedDamgeRef = 0;
			if(i <= 11){
				//use the old bitmask.
				m_rgbTimeBasedDamgeRef = &m_bitsDamage;
			}else{
				//use the new bitmask.
				m_rgbTimeBasedDamgeRef = &m_bitsDamageMod;
			}



			if ((*m_rgbTimeBasedDamgeRef) & giDmgFlags[i])
			{
				pdmg = &m_dmg[i];
				//MODDD - slighlty different position.
				//SPR_DrawAdditive(0, pdmg->x, pdmg->y, &gHUD.GetSpriteRect(m_HUD_dmg_bio + i));
				
				wrect_t* tempRect = &gHUD.GetSpriteRect(m_HUD_dmg_bio + i + imaoffset);
				gHUD.attemptDrawBrokenTrans(pdmg->x - 2, pdmg->y + 5 + extraYOffset, tempRect);

				SPR_Set(gHUD.GetSprite(m_HUD_dmg_bio + i + imaoffset), r, g, b );
				SPR_DrawAdditive(0, pdmg->x - 2, pdmg->y + 5 + extraYOffset, tempRect );
			
			}

		}
	}




	// check for bits that should be expired
	for ( i = 0; i < NUM_DMG_TYPES; i++ )
	{
		DAMAGE_IMAGE *pdmg = &m_dmg[i];


		int* m_rgbTimeBasedDamgeRef = 0;
		if(i <= 11){
			//use the old bitmask.
			m_rgbTimeBasedDamgeRef = &m_bitsDamage;
		}else{
			//use the new bitmask.
			m_rgbTimeBasedDamgeRef = &m_bitsDamageMod;
		}


		if ( (*m_rgbTimeBasedDamgeRef) & giDmgFlags[i] )
		{
			pdmg->fExpire = min( flTime + DMG_IMAGE_LIFE, pdmg->fExpire );

			if ( pdmg->fExpire <= flTime		// when the time has expired
				&& a < 40 )						// and the flash is at the low point of the cycle
			{
				pdmg->fExpire = 0;

				int y = pdmg->y;
				pdmg->x = pdmg->y = 0;

				// move everyone above down
				for (int j = 0; j < NUM_DMG_TYPES; j++)
				{
					pdmg = &m_dmg[j];
					if ((pdmg->y) && (pdmg->y < y))
						pdmg->y += giDmgHeight;

				}

				(*m_rgbTimeBasedDamgeRef)  &= ~giDmgFlags[i];  // clear the bits
			}
		}
	}

	return 1;
}
 
//MODDD - includes "long bitsDamageMod".
//void CHudHealth::UpdateTiles(float flTime, long bitsDamage)
void CHudHealth::UpdateTiles(float flTime, long bitsDamage, long bitsDamageMod)
{	
	DAMAGE_IMAGE *pdmg;


	

	// Which types are new?
	long bitsOn = ~m_bitsDamage & bitsDamage;
	long bitsOnMod = ~m_bitsDamageMod & bitsDamageMod;
	
	for (int i = 0; i < NUM_DMG_TYPES; i++)
	{
		pdmg = &m_dmg[i];


		int* m_rgbTimeBasedDamgeRef = 0;
		long* bitsOnRef = 0;
		long* bitsModern = 0;
		if(i <= 11){
			//use the old bitmask.
			m_rgbTimeBasedDamgeRef = &m_bitsDamage;
			bitsOnRef = &bitsOn;
			bitsModern = &bitsDamage;
		}else{
			//use the new bitmask.
			m_rgbTimeBasedDamgeRef = &m_bitsDamageMod;
			bitsOnRef = &bitsOnMod;
			bitsModern = &bitsDamageMod;
		}




		// Is this one already on?
		if ( (*bitsModern) & giDmgFlags[i])
		{
			
			pdmg->fExpire = flTime + DMG_IMAGE_LIFE; // extend the duration
			if (!pdmg->fBaseline)
				pdmg->fBaseline = flTime;
				
		}

		// Are we just turning it on?
		if ( (*bitsOnRef) & giDmgFlags[i])
		{
			// put this one at the bottom
			pdmg->x = giDmgWidth/8;
			pdmg->y = ScreenHeight - giDmgHeight * 2;
			pdmg->fExpire=flTime + DMG_IMAGE_LIFE;
			
			// move everyone else up
			for (int j = 0; j < NUM_DMG_TYPES; j++)
			{
				if (j == i)
					continue;

				pdmg = &m_dmg[j];
				if (pdmg->y)
					pdmg->y -= giDmgHeight;

			}
			pdmg = &m_dmg[i];
		}	
	}	

	// damage bits are only turned on here;  they are turned off when the draw time has expired (in DrawDamage())
	m_bitsDamage |= bitsDamage;
	//MODDD - new
	m_bitsDamageMod |= bitsDamageMod;
}

void CHudHealth::setUniformDamage(float damageAmount){
	
	m_fAttackFrontDamage = m_fAttackRearDamage = m_fAttackRightDamage = m_fAttackLeftDamage = damageAmount / 4.0f;

}//END OF setUniformDamage(...)

