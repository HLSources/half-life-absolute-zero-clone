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
//  hud_update.cpp
//

#include "external_lib_include.h"
//#include <stdlib.h>

#include <math.h>
#include "hud.h"
#include "cl_util.h"
#include <memory.h>

int CL_ButtonBits( int );
void CL_ResetButtonBits( int bits );

extern float v_idlescale;
float in_fov;

//MODDD - no implementation anywhere, as-is codebase?  whoopsie.
//extern void HUD_SetCmdBits( int bits );



extern void RenderFog();
extern void RenderFog(float currentTime);




int CHud::UpdateClientData(client_data_t *cdata, float time)
{
	

	//THANK YOU SPIRIT OF HALF LIFE 1 POINT NINE
	RenderFog(time);


	memcpy(m_vecOrigin, cdata->origin, sizeof(vec3_t));
	memcpy(m_vecAngles, cdata->viewangles, sizeof(vec3_t));
	
	m_iKeyBits = CL_ButtonBits( 0 );
	m_iWeaponBits = cdata->iWeaponBits;


	//MODDD - update CVars!
	updateClientCVarRefs();



	in_fov = cdata->fov;

	Think();

	cdata->fov = m_iPlayerFOV;
	
	v_idlescale = m_iConcussionEffect;

	CL_ResetButtonBits( m_iKeyBits );

	
	// return 1 if in anything in the client_data struct has been changed, 0 otherwise
	return 1;
}


