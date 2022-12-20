/*
===========================================================================
Copyright (C) 2000 - 2013, Raven Software, Inc.
Copyright (C) 2001 - 2013, Activision, Inc.
Copyright (C) 2013 - 2015, OpenJK contributors

This file is part of the OpenJK source code.

OpenJK is free software; you can redistribute it and/or modify it
under the terms of the GNU General Public License version 2 as
published by the Free Software Foundation.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, see <http://www.gnu.org/licenses/>.
===========================================================================
*/

//or something, so the headers spew errors on these defs from the previous compile.
//this fixes that. -rww
#ifdef _JK2MP
//get rid of all the crazy defs we added for this file
#undef currentAngles
#undef currentOrigin
#undef mins
#undef maxs
#undef legsAnimTimer
#undef torsoAnimTimer
#undef bool
#undef false
#undef true

#undef sqrtf
#undef Q_flrand

#undef MOD_EXPLOSIVE
#endif

#ifdef _JK2 //SP does not have this preprocessor for game like MP does
#ifndef _JK2MP
#define _JK2MP
#endif
#endif

#ifndef _JK2MP //if single player
#ifndef QAGAME //I don't think we have a QAGAME define
#define QAGAME //but define it cause in sp we're always in the game
#endif
#endif

#ifdef QAGAME //including game headers on cgame is FORBIDDEN ^_^
#include "g_local.h"
#elif defined _JK2MP
#include "bg_public.h"
#endif

#ifndef _JK2MP
#include "g_vehicles.h"
#else
#include "bg_vehicles.h"
#endif

#ifdef _JK2MP
//this is really horrible, but it works! just be sure not to use any locals or anything
//with these names (exluding bool, false, true). -rww
#define currentAngles r.currentAngles
#define currentOrigin r.currentOrigin
#define mins r.mins
#define maxs r.maxs
#define legsAnimTimer legsTimer
#define torsoAnimTimer torsoTimer
#define bool qboolean
#define false qfalse
#define true qtrue

#define sqrtf sqrt
#define Q_flrand flrand

#define MOD_EXPLOSIVE MOD_SUICIDE
#else
#define bgEntity_t gentity_t
#endif

extern float DotToSpot(vec3_t spot, vec3_t from, vec3_t fromAngles);
#ifdef QAGAME //SP or game side MP
extern vmCvar_t cg_thirdPersonAlpha;
extern vec3_t playerMins;
extern vec3_t playerMaxs;
extern cvar_t* g_speederControlScheme;
extern void ChangeWeapon(const gentity_t* ent, int newWeapon);
extern void PM_SetAnim(const pmove_t* pm, int setAnimParts, int anim, int setAnimFlags, int blendTime);
extern int PM_AnimLength(int index, animNumber_t anim);
extern void G_VehicleTrace(trace_t* results, const vec3_t start, const vec3_t tMins, const vec3_t tMaxs,
                           const vec3_t end, int passEntityNum, int contentmask);
#endif

extern qboolean BG_UnrestrainedPitchRoll();

#ifdef _JK2MP

#include "../namespace_begin.h"

extern void BG_SetAnim(playerState_t* ps, animation_t* animations, int setAnimParts, int anim, int setAnimFlags);
extern int BG_GetTime(void);
extern void G_DamageFromKiller(gentity_t* pEnt, gentity_t* pVehEnt, gentity_t* attacker, vec3_t org, int damage, int dflags, int mod);
#endif

#include "b_local.h"

//this stuff has got to be predicted, so..
bool BG_FighterUpdate(Vehicle_t* pVeh, const usercmd_t* pUcmd, vec3_t trMins, vec3_t trMaxs, float gravity,
                      void (*traceFunc)(trace_t* results, const vec3_t start, const vec3_t lmins, const vec3_t lmaxs,
                                        const vec3_t end, int passEntityNum, int contentMask))
{
	vec3_t bottom;
	playerState_t* parentPS;
#ifdef QAGAME //don't do this on client
	// Make sure the riders are not visible.
	pVeh->m_pVehicleInfo->Ghost(pVeh, pVeh->m_pPilot);
#endif

#ifdef _JK2MP
	parentPS = pVeh->m_pParentEntity->playerState;
#else
	parentPS = &pVeh->m_pParentEntity->client->ps;
#endif

	if (!parentPS)
	{
		Com_Error(ERR_DROP, "NULL PS in BG_FighterUpdate (%s)", pVeh->m_pVehicleInfo->name);
	}

	// If we have a pilot, take out gravity (it's a flying craft...).
	if (pVeh->m_pPilot)
	{
		parentPS->gravity = 0;
#ifndef _JK2MP //don't need this flag in mp, I.. guess
		pVeh->m_pParentEntity->svFlags |= SVF_CUSTOM_GRAVITY;
#endif
	}
	else
	{
#ifndef _JK2MP //don't need this flag in mp, I.. guess
		pVeh->m_pParentEntity->svFlags &= ~SVF_CUSTOM_GRAVITY;
#else //in MP set gravity back to normal gravity
		if (pVeh->m_pVehicleInfo->gravity)
		{
			parentPS->gravity = pVeh->m_pVehicleInfo->gravity;
		}
		else
		{ //it doesn't have gravity specified apparently
			parentPS->gravity = gravity;
		}
#endif
	}

	// Check to see if the fighter has taken off yet (if it's a certain height above ground).
	VectorCopy(parentPS->origin, bottom);
	bottom[2] -= pVeh->m_pVehicleInfo->landingHeight;

	traceFunc(&pVeh->m_LandTrace, parentPS->origin, trMins, trMaxs, bottom, pVeh->m_pParentEntity->s.number,
	          MASK_NPCSOLID & ~CONTENTS_BODY);

	return true;
}

#ifdef QAGAME //ONLY in SP or on server, not cgame

// Like a think or move command, this updates various vehicle properties.
static bool update(Vehicle_t* p_veh, const usercmd_t* pUcmd)
{
	const gentity_t* parent = p_veh->m_pParentEntity;

	assert(p_veh->m_pParentEntity);

	if (!BG_FighterUpdate(p_veh, pUcmd, p_veh->m_pParentEntity->mins, p_veh->m_pParentEntity->maxs,
#ifdef _JK2MP
		g_gravity.value,
#else
	                      g_gravity->value,
#endif
	                      G_VehicleTrace))
	{
		return false;
	}

	if (!g_vehicleInfo[VEHICLE_BASE].Update(p_veh, pUcmd))
	{
		return false;
	}

	// Exhaust Effects Start And Stop When The Accelerator Is Pressed
	//----------------------------------------------------------------
	if (p_veh->m_pVehicleInfo->iExhaustFX)
	{
		// Start It On Each Exhaust Bolt
		//-------------------------------
		if (p_veh->m_ucmd.forwardmove && !(p_veh->m_ulFlags & VEH_ACCELERATORON))
		{
			p_veh->m_ulFlags |= VEH_ACCELERATORON;
			for (int i = 0; i < MAX_VEHICLE_EXHAUSTS && p_veh->m_iExhaustTag[i] != -1; i++)
			{
				G_PlayEffect(p_veh->m_pVehicleInfo->iExhaustFX, parent->playerModel, p_veh->m_iExhaustTag[i],
				             parent->s.number, parent->currentOrigin, 1, qtrue);
			}
		}

		// Stop It On Each Exhaust Bolt
		//------------------------------
		else if (!p_veh->m_ucmd.forwardmove && p_veh->m_ulFlags & VEH_ACCELERATORON)
		{
			p_veh->m_ulFlags &= ~VEH_ACCELERATORON;
			for (int i = 0; i < MAX_VEHICLE_EXHAUSTS && p_veh->m_iExhaustTag[i] != -1; i++)
			{
				G_StopEffect(p_veh->m_pVehicleInfo->iExhaustFX, parent->playerModel, p_veh->m_iExhaustTag[i],
				             parent->s.number);
			}
		}
		else
		{
			if (p_veh->m_ucmd.forwardmove && p_veh->m_ulFlags & VEH_FLYING)
			{
				for (int i = 0; i < MAX_VEHICLE_EXHAUSTS && p_veh->m_iExhaustTag[i] != -1; i++)
				{
					G_PlayEffect(p_veh->m_pVehicleInfo->iTurboFX, parent->playerModel, p_veh->m_iExhaustTag[i],
					             parent->s.number, parent->currentOrigin, 1, qtrue);
				}
			}
		}
	}

	return true;
}

// Board this Vehicle (get on). The first entity to board an empty vehicle becomes the Pilot.
static bool Board(Vehicle_t* pVeh, bgEntity_t* pEnt)
{
	if (!g_vehicleInfo[VEHICLE_BASE].Board(pVeh, pEnt))
		return false;

	// Set the board wait time (they won't be able to do anything, including getting off, for this amount of time).
	pVeh->m_iBoarding = level.time + 1500;

	return true;
}

// Eject an entity from the vehicle.
static bool Eject(Vehicle_t* pVeh, bgEntity_t* pEnt, const qboolean forceEject)
{
	if (g_vehicleInfo[VEHICLE_BASE].Eject(pVeh, pEnt, forceEject))
	{
		if (pVeh->m_pVehicleInfo->soundOff)
		{
			G_SoundIndexOnEnt(pVeh->m_pParentEntity, CHAN_AUTO, pVeh->m_pVehicleInfo->soundOff);
		}
		return true;
	}

	return false;
}

#endif //end game-side only

//method of decrementing the given angle based on the given taking variable frame times into account
static float PredictedAngularDecrement(const float scale, const float timeMod, const float originalAngle)
{
	float fixedBaseDec = originalAngle * 0.05f;
	float r = 0.0f;

	if (fixedBaseDec < 0.0f)
	{
		fixedBaseDec = -fixedBaseDec;
	}

	fixedBaseDec *= 1.0f + (1.0f - scale);

	if (fixedBaseDec < 0.1f)
	{
		//don't increment in incredibly small fractions, it would eat up unnecessary bandwidth.
		fixedBaseDec = 0.1f;
	}

	fixedBaseDec *= timeMod * 0.1f;
	if (originalAngle > 0.0f)
	{
		//subtract
		r = originalAngle - fixedBaseDec;
		if (r < 0.0f)
		{
			r = 0.0f;
		}
	}
	else if (originalAngle < 0.0f)
	{
		//add
		r = originalAngle + fixedBaseDec;
		if (r > 0.0f)
		{
			r = 0.0f;
		}
	}

	return r;
}

#ifdef QAGAME//only do this check on GAME side, because if it's CGAME, it's being predicted, and it's only predicted if the local client is the driver



qboolean FighterIsInSpace(const gentity_t* gParent)
{
	if (gParent
		&& gParent->client
		&& gParent->client->inSpaceIndex
		&& gParent->client->inSpaceIndex < ENTITYNUM_WORLD)
	{
		return qtrue;
	}
	return qfalse;
}
#endif

qboolean FighterOverValidLandingSurface(const Vehicle_t* pVeh)
{
	if (pVeh->m_LandTrace.fraction < 1.0f //ground present
		&& pVeh->m_LandTrace.plane.normal[2] >= MIN_LANDING_SLOPE) //flat enough
	//FIXME: also check for a certain surface flag ... "landing zones"?
	{
		return qtrue;
	}
	return qfalse;
}

qboolean FighterIsLanded(const Vehicle_t* pVeh, const playerState_t* parentPS)
{
	if (FighterOverValidLandingSurface(pVeh)
		&& !parentPS->speed) //stopped
	{
		return qtrue;
	}
	return qfalse;
}

qboolean FighterIsLanding(Vehicle_t* pVeh, playerState_t* parentPS)
{
	if (FighterOverValidLandingSurface(pVeh)
		&& pVeh->m_pVehicleInfo->Inhabited(pVeh) //has to have a driver in order to be capable of landing
		&& (pVeh->m_ucmd.forwardmove < 0 || pVeh->m_ucmd.upmove < 0) //decelerating or holding crouch button
		&& parentPS->speed <= MIN_LANDING_SPEED)
	//going slow enough to start landing - was using pVeh->m_pVehicleInfo->speedIdle, but that's still too fast
	{
		parentPS->speed = 0;
		return qtrue;
	}
	return qfalse;
}

qboolean FighterIsLaunching(Vehicle_t* pVeh, const playerState_t* parentPS)
{
	if (FighterOverValidLandingSurface(pVeh)
#ifdef QAGAME//only do this check on GAME side, because if it's CGAME, it's being predicted, and it's only predicted if the local client is the driver



		&& pVeh->m_pVehicleInfo->Inhabited(pVeh) //has to have a driver in order to be capable of landing
#endif
		&& pVeh->m_ucmd.upmove > 0 //trying to take off
		&& parentPS->speed <= 200.0f)
	//going slow enough to start landing - was using pVeh->m_pVehicleInfo->speedIdle, but that's still too fast
	{
		return qtrue;
	}
	return qfalse;
}

qboolean FighterSuspended(const Vehicle_t* pVeh, const playerState_t* parentPS)
{
#ifdef QAGAME//only do this check on GAME side, because if it's CGAME, it's being predicted, and it's only predicted if the local client is the driver



	if (!pVeh->m_pPilot //empty
		&& !parentPS->speed //not moving
		&& pVeh->m_ucmd.forwardmove <= 0 //not trying to go forward for whatever reason
		&& pVeh->m_pParentEntity != nullptr
		&& pVeh->m_pParentEntity->spawnflags & 2) //SUSPENDED spawn flag is on
	{
		return qtrue;
	}
	return qfalse;
#elif defined CGAME
	return qfalse;
#endif
}

#define FIGHTER_MIN_TAKEOFF_FRACTION 0.7f

static void ProcessMoveCommands(Vehicle_t* pVeh)
{
	/************************************************************************************/
	/*	BEGIN	Here is where we move the vehicle (forward or back or whatever). BEGIN	*/
	/************************************************************************************/

	//Client sets ucmds and such for speed alterations
	float speedInc, speedIdleDec, speedIdle, speedIdleAccel, speedMin, speedMax;
	bgEntity_t* parent = pVeh->m_pParentEntity;
	qboolean isLandingOrLaunching;
#ifndef _JK2MP//SP
	int curTime = level.time;
#elif defined QAGAME//MP GAME
	int curTime = level.time;
#elif defined CGAME//MP CGAME
	//FIXME: pass in ucmd?  Not sure if this is reliable...
	int curTime = pm->cmd.serverTime;
#endif

#ifdef _JK2MP
	playerState_t* parentPS = parent->playerState;
#else
	playerState_t* parentPS = &parent->client->ps;
#endif

	if (parentPS->hyperSpaceTime && curTime - parentPS->hyperSpaceTime < HYPERSPACE_TIME)
	{
		//Going to Hyperspace
		//totally override movement
		const float timeFrac = static_cast<float>(curTime - parentPS->hyperSpaceTime) / HYPERSPACE_TIME;

		if (timeFrac < HYPERSPACE_TELEPORT_FRAC)
		{
			//for first half, instantly jump to top speed!
			if (!(parentPS->eFlags2 & EF2_HYPERSPACE))
			{
				//waiting to face the right direction, do nothing
				parentPS->speed = 0.0f;
			}
			else
			{
				if (parentPS->speed < HYPERSPACE_SPEED)
				{
					//just started hyperspace
					for (int i = 0; i < MAX_VEHICLE_EXHAUSTS && pVeh->m_iExhaustTag[i] != -1; i++)
					{
						// Start The Turbo Fx Start
						//--------------------------
						G_PlayEffect(pVeh->m_pVehicleInfo->iTurboStartFX, pVeh->m_pParentEntity->playerModel,
						             pVeh->m_iExhaustTag[i], pVeh->m_pParentEntity->s.number,
						             pVeh->m_pParentEntity->currentOrigin);

						// Start The Looping Effect
						//--------------------------
						if (pVeh->m_pVehicleInfo->iTurboFX)
						{
							G_PlayEffect(pVeh->m_pVehicleInfo->iTurboFX, pVeh->m_pParentEntity->playerModel,
							             pVeh->m_iExhaustTag[i], pVeh->m_pParentEntity->s.number,
							             pVeh->m_pParentEntity->currentOrigin, pVeh->m_pVehicleInfo->turboDuration,
							             qtrue);
						}
					}
				}

				parentPS->speed = HYPERSPACE_SPEED;
				if (pVeh->m_pVehicleInfo->soundHyper)
				{
					G_SoundIndexOnEnt(pVeh->m_pParentEntity, CHAN_AUTO, pVeh->m_pVehicleInfo->soundHyper);
				}
			}
		}
		else
		{
			//slow from top speed to 200...
			parentPS->speed = 200.0f + (1.0f - timeFrac) * (1.0f / HYPERSPACE_TELEPORT_FRAC) * (HYPERSPACE_SPEED -
				200.0f);
			//don't mess with acceleration, just pop to the high velocity
			if (VectorLength(parentPS->velocity) < parentPS->speed)
			{
				VectorScale(parentPS->moveDir, parentPS->speed, parentPS->velocity);
			}
		}
		return;
	}

	if (pVeh->m_iDropTime >= curTime)
	{
		//no speed, just drop
		parentPS->speed = 0.0f;
		parentPS->gravity = 800;
		return;
	}

	isLandingOrLaunching = static_cast<qboolean>(FighterIsLanding(pVeh, parentPS) ||
		FighterIsLaunching(pVeh, parentPS));

	// If we are hitting the ground, just allow the fighter to go up and down.
	if (isLandingOrLaunching //going slow enough to start landing
		&& (pVeh->m_ucmd.forwardmove <= 0 || pVeh->m_LandTrace.fraction <= FIGHTER_MIN_TAKEOFF_FRACTION))
	//not trying to accelerate away already (or: you are trying to, but not high enough off the ground yet)
	{
		//FIXME: if start to move forward and fly over something low while still going relatively slow, you may try to land even though you don't mean to...
		//float fInvFrac = 1.0f - pVeh->m_LandTrace.fraction;

		if (pVeh->m_ucmd.upmove > 0)
		{
			if (parentPS->velocity[2] <= 0 && pVeh->m_pVehicleInfo->soundTakeOff)
			{
				//taking off for the first time
				G_SoundIndexOnEnt(pVeh->m_pParentEntity, CHAN_AUTO, pVeh->m_pVehicleInfo->soundTakeOff);
			}
			parentPS->velocity[2] += pVeh->m_pVehicleInfo->acceleration * pVeh->m_fTimeModifier;
			// * ( /*fInvFrac **/ 1.5f );
		}
		else if (pVeh->m_ucmd.upmove < 0)
		{
			parentPS->velocity[2] -= pVeh->m_pVehicleInfo->acceleration * pVeh->m_fTimeModifier;
			// * ( /*fInvFrac **/ 1.8f );
		}
		else if (pVeh->m_ucmd.forwardmove < 0)
		{
			if (pVeh->m_LandTrace.fraction != 0.0f)
			{
				parentPS->velocity[2] -= pVeh->m_pVehicleInfo->acceleration * pVeh->m_fTimeModifier;
			}

			if (pVeh->m_LandTrace.fraction <= FIGHTER_MIN_TAKEOFF_FRACTION)
			{
				parentPS->velocity[2] = PredictedAngularDecrement(pVeh->m_LandTrace.fraction,
				                                                  pVeh->m_fTimeModifier * 5.0f, parentPS->velocity[2]);

				parentPS->speed = 0;
			}
		}

		// Make sure they don't pitch as they near the ground.
		pVeh->m_vOrientation[PITCH] = PredictedAngularDecrement(0.7f, pVeh->m_fTimeModifier * 10.0f,
		                                                        pVeh->m_vOrientation[PITCH]);

		return;
	}

	// Exhaust Effects Start And Stop When The Accelerator Is Pressed
	//----------------------------------------------------------------
	if (pVeh->m_pVehicleInfo->iExhaustFX && pVeh->m_pVehicleInfo->Inhabited(pVeh))
	{
		for (int i = 0; i < MAX_VEHICLE_EXHAUSTS && pVeh->m_iExhaustTag[i] != -1; i++)
		{
			G_PlayEffect(pVeh->m_pVehicleInfo->iExhaustFX, parent->playerModel, pVeh->m_iExhaustTag[i],
			             parent->s.number, parent->currentOrigin, 1, qtrue);

			if (pVeh->m_ucmd.forwardmove > 0)
			{
				if (pVeh->m_pVehicleInfo->iTrailFX)
				{
					G_PlayEffect(pVeh->m_pVehicleInfo->iTrailFX, pVeh->m_pParentEntity->playerModel,
					             pVeh->m_iExhaustTag[i], pVeh->m_pParentEntity->s.number,
					             pVeh->m_pParentEntity->currentOrigin, pVeh->m_pVehicleInfo->turboDuration, qtrue);
				}
			}
		}
	}
	else
	{
		for (int i = 0; i < MAX_VEHICLE_EXHAUSTS && pVeh->m_iExhaustTag[i] != -1; i++)
		{
			G_StopEffect(pVeh->m_pVehicleInfo->iExhaustFX, parent->playerModel, pVeh->m_iExhaustTag[i],
			             parent->s.number);
		}
	}

	if (pVeh->m_pPilot &&
		(pVeh->m_ucmd.upmove > 0 && pVeh->m_ucmd.forwardmove > 0 && pVeh->m_pVehicleInfo->turboSpeed))
	{
		if (curTime - pVeh->m_iTurboTime > pVeh->m_pVehicleInfo->turboRecharge)
		{
			pVeh->m_iTurboTime = curTime + pVeh->m_pVehicleInfo->turboDuration;
			if (pVeh->m_pVehicleInfo->iTurboStartFX)
			{
				for (int i = 0; i < MAX_VEHICLE_EXHAUSTS && pVeh->m_iExhaustTag[i] != -1; i++)
				{
#ifndef _JK2MP//SP
					// Start The Turbo Fx Start
					//--------------------------
					G_PlayEffect(pVeh->m_pVehicleInfo->iTurboStartFX, pVeh->m_pParentEntity->playerModel,
					             pVeh->m_iExhaustTag[i], pVeh->m_pParentEntity->s.number,
					             pVeh->m_pParentEntity->currentOrigin);

					// Start The Looping Effect
					//--------------------------
					if (pVeh->m_pVehicleInfo->iTurboFX)
					{
						G_PlayEffect(pVeh->m_pVehicleInfo->iTurboFX, pVeh->m_pParentEntity->playerModel,
						             pVeh->m_iExhaustTag[i], pVeh->m_pParentEntity->s.number,
						             pVeh->m_pParentEntity->currentOrigin, pVeh->m_pVehicleInfo->turboDuration, qtrue);
					}

#else
#ifdef QAGAME
					if (pVeh->m_pParentEntity &&
						pVeh->m_pParentEntity->ghoul2 &&
						pVeh->m_pParentEntity->playerState)
					{ //fine, I'll use a temp ent for this, but only because it's played only once at the start of a turbo.
						vec3_t boltOrg, boltDir;
						mdxaBone_t boltMatrix;

						VectorSet(boltDir, 0.0f, pVeh->m_pParentEntity->playerState->viewangles[YAW], 0.0f);

						trap_G2API_GetBoltMatrix(pVeh->m_pParentEntity->ghoul2, 0, pVeh->m_iExhaustTag[i], &boltMatrix, boltDir, pVeh->m_pParentEntity->playerState->origin, level.time, nullptr, pVeh->m_pParentEntity->modelScale);
						BG_GiveMeVectorFromMatrix(&boltMatrix, ORIGIN, boltOrg);
						BG_GiveMeVectorFromMatrix(&boltMatrix, ORIGIN, boltDir);
						G_PlayEffectID(pVeh->m_pVehicleInfo->iTurboStartFX, boltOrg, boltDir);
					}
#endif
#endif
				}
			}
			if (pVeh->m_pVehicleInfo->soundTurbo)
			{
				G_SoundIndexOnEnt(pVeh->m_pParentEntity, CHAN_AUTO, pVeh->m_pVehicleInfo->soundTurbo);
			}
			parentPS->speed = pVeh->m_pVehicleInfo->turboSpeed; // Instantly Jump To Turbo Speed
		}
	}
	speedInc = pVeh->m_pVehicleInfo->acceleration * pVeh->m_fTimeModifier;

	if (curTime < pVeh->m_iTurboTime)
	{
		//going turbo speed
		speedMax = pVeh->m_pVehicleInfo->turboSpeed;
		//double our acceleration
		speedInc *= 2.0f;
		//force us to move forward
		pVeh->m_ucmd.forwardmove = 127;
		//add flag to let cgame know to draw the iTurboFX effect
		parentPS->eFlags |= EF_JETPACK_ACTIVE;
	}
	else
	{
		//normal max speed
		speedMax = pVeh->m_pVehicleInfo->speedMax;
		if (parentPS->eFlags & EF_JETPACK_ACTIVE)
		{
			//stop cgame from playing the turbo exhaust effect
			parentPS->eFlags &= ~EF_JETPACK_ACTIVE;
		}
	}
	speedIdleDec = pVeh->m_pVehicleInfo->decelIdle * pVeh->m_fTimeModifier;
	speedIdle = pVeh->m_pVehicleInfo->speedIdle;
	speedIdleAccel = pVeh->m_pVehicleInfo->accelIdle * pVeh->m_fTimeModifier;
	speedMin = pVeh->m_pVehicleInfo->speedMin;

	if (parentPS->brokenLimbs & 1 << SHIPSURF_DAMAGE_BACK_HEAVY)
	{
		//engine has taken heavy damage
		speedMax *= 0.8f; //at 80% speed
	}
	else if (parentPS->brokenLimbs & 1 << SHIPSURF_DAMAGE_BACK_LIGHT)
	{
		//engine has taken light damage
		speedMax *= 0.6f; //at 60% speed
	}

	if (pVeh->m_iRemovedSurfaces
		|| parentPS->electrifyTime >= curTime)
	{
		//go out of control
		parentPS->speed += speedInc;
		//Why set forwardmove?  PMove code doesn't use it... does it?
		pVeh->m_ucmd.forwardmove = 127;
	}
#ifdef QAGAME //well, the thing is always going to be inhabited if it's being predicted!
	else if (FighterSuspended(pVeh, parentPS))
	{
		parentPS->speed = 0;
		pVeh->m_ucmd.forwardmove = 0;
	}
	else if (!pVeh->m_pVehicleInfo->Inhabited(pVeh)
		&& parentPS->speed > 0)
	{
		//pilot jumped out while we were moving forward (not landing or landed) so just keep the throttle locked
		//Why set forwardmove?  PMove code doesn't use it... does it?
		pVeh->m_ucmd.forwardmove = 127;
	}
#endif
	else if ((parentPS->speed || parentPS->groundEntityNum == ENTITYNUM_NONE ||
		pVeh->m_ucmd.forwardmove || pVeh->m_ucmd.upmove > 0) && pVeh->m_LandTrace.fraction >= 0.05f)
	{
		if (pVeh->m_ucmd.forwardmove > 0 && speedInc)
		{
			parentPS->speed += speedInc;
			pVeh->m_ucmd.forwardmove = 127;
		}
		else if (pVeh->m_ucmd.forwardmove < 0
			|| pVeh->m_ucmd.upmove < 0)
		{
			//decelerating or braking
			if (pVeh->m_ucmd.upmove < 0)
			{
				//braking (trying to land?), slow down faster
				if (pVeh->m_ucmd.forwardmove)
				{
					//decelerator + brakes
					speedInc += pVeh->m_pVehicleInfo->braking;
					speedIdleDec += pVeh->m_pVehicleInfo->braking;
				}
				else
				{
					//just brakes
					speedInc = speedIdleDec = pVeh->m_pVehicleInfo->braking;
				}
			}
			if (parentPS->speed > speedIdle)
			{
				parentPS->speed -= speedInc;
			}
			else if (parentPS->speed > speedMin)
			{
				if (FighterOverValidLandingSurface(pVeh))
				{
					//there's ground below us and we're trying to slow down, slow down faster
					parentPS->speed -= speedInc;
				}
				else
				{
					parentPS->speed -= speedIdleDec;
					if (parentPS->speed < MIN_LANDING_SPEED)
					{
						//unless you can land, don't drop below the landing speed!!!  This way you can't come to a dead stop in mid-air
						parentPS->speed = MIN_LANDING_SPEED;
					}
				}
			}
			if (pVeh->m_pVehicleInfo->type == VH_FIGHTER)
			{
				pVeh->m_ucmd.forwardmove = 127;
			}
			else if (speedMin >= 0)
			{
				pVeh->m_ucmd.forwardmove = 0;
			}
		}
		//else not accel, decel or braking
		else if (pVeh->m_pVehicleInfo->throttleSticks)
		{
			//we're using a throttle that sticks at current speed
			if (parentPS->speed <= MIN_LANDING_SPEED)
			{
				//going less than landing speed
				if (FighterOverValidLandingSurface(pVeh))
				{
					//close to ground and not going very fast
					//slow to a stop if within landing height and not accel/decel/braking
					if (parentPS->speed > 0)
					{
						//slow down
						parentPS->speed -= speedIdleDec;
					}
					else if (parentPS->speed < 0)
					{
						//going backwards, slow down
						parentPS->speed += speedIdleDec;
					}
				}
				else
				{
					//not over a valid landing surf, but going too slow
					//speed up to idle speed if not over a valid landing surf and not accel/decel/braking
					if (parentPS->speed < speedIdle)
					{
						parentPS->speed += speedIdleAccel;
						if (parentPS->speed > speedIdle)
						{
							parentPS->speed = speedIdle;
						}
					}
				}
			}
		}
		else
		{
			//then speed up or slow down to idle speed
			//accelerate to cruising speed only, otherwise, just coast
			// If they've launched, apply some constant motion.
			if ((pVeh->m_LandTrace.fraction >= 1.0f //no ground
					|| pVeh->m_LandTrace.plane.normal[2] < MIN_LANDING_SLOPE) //or can't land on ground below us
				&& speedIdle > 0)
			{
				//not above ground and have an idle speed
				//float fSpeed = pVeh->m_pParentEntity->client->ps.speed;
				if (parentPS->speed < speedIdle)
				{
					parentPS->speed += speedIdleAccel;
					if (parentPS->speed > speedIdle)
					{
						parentPS->speed = speedIdle;
					}
				}
				else if (parentPS->speed > 0)
				{
					//slow down
					parentPS->speed -= speedIdleDec;

					if (parentPS->speed < speedIdle)
					{
						parentPS->speed = speedIdle;
					}
				}
			}
			else //either close to ground or no idle speed
			{
				//slow to a stop if no idle speed or within landing height and not accel/decel/braking
				if (parentPS->speed > 0)
				{
					//slow down
					parentPS->speed -= speedIdleDec;
				}
				else if (parentPS->speed < 0)
				{
					//going backwards, slow down
					parentPS->speed += speedIdleDec;
				}
			}
		}
	}
	else
	{
		if (pVeh->m_ucmd.forwardmove < 0)
		{
			pVeh->m_ucmd.forwardmove = 0;
		}
		if (pVeh->m_ucmd.upmove < 0)
		{
			pVeh->m_ucmd.upmove = 0;
		}

#ifndef _JK2MP
		if (!pVeh->m_pVehicleInfo->strafePerc || !g_speederControlScheme->value && !pVeh->m_pParentEntity->s.number)
		{
			//if in a strafe-capable vehicle, clear strafing unless using alternate control scheme
			pVeh->m_ucmd.rightmove = 0;
		}
#endif
	}

#if 1//This is working now, but there are some transitional jitters... Rich?
	//STRAFING==============================================================================
	if (pVeh->m_pVehicleInfo->strafePerc
#ifdef QAGAME//only do this check on GAME side, because if it's CGAME, it's being predicted, and it's only predicted if the local client is the driver



		&& pVeh->m_pVehicleInfo->Inhabited(pVeh) //has to have a driver in order to be capable of landing
#endif
		&& !pVeh->m_iRemovedSurfaces
		&& parentPS->electrifyTime < curTime
		&& parentPS->vehTurnaroundTime < curTime
		&& (pVeh->m_LandTrace.fraction >= 1.0f //no ground
			|| pVeh->m_LandTrace.plane.normal[2] < MIN_LANDING_SLOPE //can't land here
			|| parentPS->speed > MIN_LANDING_SPEED) //going too fast to land
		&& pVeh->m_ucmd.rightmove)
	{
		//strafe
		vec3_t vAngles, vRight;
		float strafeSpeed = pVeh->m_pVehicleInfo->strafePerc * speedMax * 5.0f;
		VectorCopy(pVeh->m_vOrientation, vAngles);
		vAngles[PITCH] = vAngles[ROLL] = 0;
		AngleVectors(vAngles, nullptr, vRight, nullptr);

		if (pVeh->m_ucmd.rightmove > 0)
		{
			//strafe right
			//FIXME: this will probably make it possible to cheat and
			//		go faster than max speed if you keep turning and strafing...
			if (pVeh->m_fStrafeTime > -MAX_STRAFE_TIME)
			{
				//can strafe right for 2 seconds
				const float curStrafeSpeed = DotProduct(parentPS->velocity, vRight);
				if (curStrafeSpeed > 0.0f)
				{
					//if > 0, already strafing right
					strafeSpeed -= curStrafeSpeed; //so it doesn't add up
				}
				if (strafeSpeed > 0)
				{
					VectorMA(parentPS->velocity, strafeSpeed * pVeh->m_fTimeModifier, vRight, parentPS->velocity);
				}
				pVeh->m_fStrafeTime -= 50 * pVeh->m_fTimeModifier;
			}
		}
		else
		{
			//strafe left
			if (pVeh->m_fStrafeTime < MAX_STRAFE_TIME)
			{
				//can strafe left for 2 seconds
				const float curStrafeSpeed = DotProduct(parentPS->velocity, vRight);
				if (curStrafeSpeed < 0.0f)
				{
					//if < 0, already strafing left
					strafeSpeed += curStrafeSpeed; //so it doesn't add up
				}
				if (strafeSpeed > 0)
				{
					VectorMA(parentPS->velocity, -strafeSpeed * pVeh->m_fTimeModifier, vRight, parentPS->velocity);
				}
				pVeh->m_fStrafeTime += 50 * pVeh->m_fTimeModifier;
			}
		}
	}
	else
	{
		if (pVeh->m_fStrafeTime > 0)
		{
			pVeh->m_fStrafeTime -= 50 * pVeh->m_fTimeModifier;
			if (pVeh->m_fStrafeTime < 0)
			{
				pVeh->m_fStrafeTime = 0.0f;
			}
		}
		else if (pVeh->m_fStrafeTime < 0)
		{
			pVeh->m_fStrafeTime += 50 * pVeh->m_fTimeModifier;
			if (pVeh->m_fStrafeTime > 0)
			{
				pVeh->m_fStrafeTime = 0.0f;
			}
		}
	}
	//STRAFING==============================================================================
#endif

	if (parentPS->speed > speedMax)
	{
		parentPS->speed = speedMax;
	}
	else if (parentPS->speed < speedMin)
	{
		parentPS->speed = speedMin;
	}

#ifdef QAGAME//FIXME: get working in GAME and CGAME
	if (pVeh->m_vOrientation[PITCH] * 0.1f > 10.0f)
	{
		//pitched downward, increase speed more and more based on our tilt
		if (FighterIsInSpace(parent))
		{
			//in space, do nothing with speed base on pitch...
		}
		else
		{
			//really should only do this when on a planet
			float mult = pVeh->m_vOrientation[PITCH] * 0.1f;
			if (mult < 1.0f)
			{
				mult = 1.0f;
			}
			parentPS->speed = PredictedAngularDecrement(mult, pVeh->m_fTimeModifier * 10.0f, parentPS->speed);
		}
	}

	if (pVeh->m_iRemovedSurfaces
		|| parentPS->electrifyTime >= curTime)
	{
		//going down
		if (FighterIsInSpace(parent))
		{
			//we're in a valid trigger_space brush
			//simulate randomness
			if (!(parent->s.number & 3))
			{
				//even multiple of 3, don't do anything
				parentPS->gravity = 0;
			}
			else if (!(parent->s.number & 2))
			{
				//even multiple of 2, go up
				parentPS->gravity = -500.0f;
				parentPS->velocity[2] = 80.0f;
			}
			else
			{
				//odd number, go down
				parentPS->gravity = 500.0f;
				parentPS->velocity[2] = -80.0f;
			}
		}
		else
		{
			//over a planet
			parentPS->gravity = 500.0f;
			parentPS->velocity[2] = -80.0f;
		}
	}
	else if (FighterSuspended(pVeh, parentPS))
	{
		parentPS->gravity = 0;
	}
	else if ((!parentPS->speed || parentPS->speed < speedIdle) && pVeh->m_ucmd.upmove <= 0)
	{
		//slowing down or stopped and not trying to take off
		if (FighterIsInSpace(parent))
		{
			//we're in space, stopping doesn't make us drift downward
			if (FighterOverValidLandingSurface(pVeh))
			{
				//well, there's something below us to land on, so go ahead and lower us down to it
				parentPS->gravity = (speedIdle - parentPS->speed) / 4;
			}
		}
		else
		{
			//over a planet
			parentPS->gravity = (speedIdle - parentPS->speed) / 4;
		}
	}
	else
	{
		parentPS->gravity = 0;
	}
#else//FIXME: get above checks working in GAME and CGAME
	parentPS->gravity = 0;
#endif

	/********************************************************************************/
	/*	END Here is where we move the vehicle (forward or back or whatever). END	*/
	/********************************************************************************/
}

extern void BG_VehicleTurnRateForSpeed(const Vehicle_t* p_veh, float speed, float* m_pitch_override, float* m_yaw_override);

static void FighterWingMalfunctionCheck(Vehicle_t* pVeh, const playerState_t* parentPS)
{
	float mPitchOverride = 1.0f;
	float mYawOverride = 1.0f;
	BG_VehicleTurnRateForSpeed(pVeh, parentPS->speed, &mPitchOverride, &mYawOverride);
	//check right wing damage
	if (parentPS->brokenLimbs & 1 << SHIPSURF_DAMAGE_RIGHT_HEAVY)
	{
		//right wing has taken heavy damage
		pVeh->m_vOrientation[ROLL] += (sin(pVeh->m_ucmd.serverTime * 0.001) + 1.0f) * pVeh->m_fTimeModifier *
			mYawOverride * 50.0f;
	}
	else if (parentPS->brokenLimbs & 1 << SHIPSURF_DAMAGE_RIGHT_LIGHT)
	{
		//right wing has taken light damage
		pVeh->m_vOrientation[ROLL] += (sin(pVeh->m_ucmd.serverTime * 0.001) + 1.0f) * pVeh->m_fTimeModifier *
			mYawOverride * 12.5f;
	}

	//check left wing damage
	if (parentPS->brokenLimbs & 1 << SHIPSURF_DAMAGE_LEFT_HEAVY)
	{
		//left wing has taken heavy damage
		pVeh->m_vOrientation[ROLL] -= (sin(pVeh->m_ucmd.serverTime * 0.001) + 1.0f) * pVeh->m_fTimeModifier *
			mYawOverride * 50.0f;
	}
	else if (parentPS->brokenLimbs & 1 << SHIPSURF_DAMAGE_LEFT_LIGHT)
	{
		//left wing has taken light damage
		pVeh->m_vOrientation[ROLL] -= (sin(pVeh->m_ucmd.serverTime * 0.001) + 1.0f) * pVeh->m_fTimeModifier *
			mYawOverride * 12.5f;
	}
}

static void FighterNoseMalfunctionCheck(Vehicle_t* pVeh, const playerState_t* parentPS)
{
	float mPitchOverride = 1.0f;
	float mYawOverride = 1.0f;
	BG_VehicleTurnRateForSpeed(pVeh, parentPS->speed, &mPitchOverride, &mYawOverride);
	//check nose damage
	if (parentPS->brokenLimbs & 1 << SHIPSURF_DAMAGE_FRONT_HEAVY)
	{
		//nose has taken heavy damage
		//pitch up and down over time
		pVeh->m_vOrientation[PITCH] += sin(pVeh->m_ucmd.serverTime * 0.001) * pVeh->m_fTimeModifier * mPitchOverride *
			50.0f;
	}
	else if (parentPS->brokenLimbs & 1 << SHIPSURF_DAMAGE_FRONT_LIGHT)
	{
		//nose has taken heavy damage
		//pitch up and down over time
		pVeh->m_vOrientation[PITCH] += sin(pVeh->m_ucmd.serverTime * 0.001) * pVeh->m_fTimeModifier * mPitchOverride *
			20.0f;
	}
}

static void FighterDamageRoutine(Vehicle_t* pVeh, bgEntity_t* parent, const playerState_t* parentPS,
                                 playerState_t* riderPS,
                                 const qboolean isDead)
{
	if (!pVeh->m_iRemovedSurfaces)
	{
		//still in one piece
		if (pVeh->m_pParentEntity && isDead)
		{
			//death spiral
			pVeh->m_ucmd.upmove = 0;

			if (!(pVeh->m_pParentEntity->s.number % 3))
			{
				//NOT everyone should do this
				pVeh->m_vOrientation[PITCH] += pVeh->m_fTimeModifier;
				if (!BG_UnrestrainedPitchRoll())
				{
					if (pVeh->m_vOrientation[PITCH] > 60.0f)
					{
						pVeh->m_vOrientation[PITCH] = 60.0f;
					}
				}
			}
			else if (!(pVeh->m_pParentEntity->s.number % 2))
			{
				pVeh->m_vOrientation[PITCH] -= pVeh->m_fTimeModifier;
				if (!BG_UnrestrainedPitchRoll())
				{
					if (pVeh->m_vOrientation[PITCH] > -60.0f)
					{
						pVeh->m_vOrientation[PITCH] = -60.0f;
					}
				}
			}
			if (pVeh->m_pParentEntity->s.number % 2)
			{
				pVeh->m_vOrientation[YAW] += pVeh->m_fTimeModifier;
				pVeh->m_vOrientation[ROLL] += pVeh->m_fTimeModifier * 4.0f;
			}
			else
			{
				pVeh->m_vOrientation[YAW] -= pVeh->m_fTimeModifier;
				pVeh->m_vOrientation[ROLL] -= pVeh->m_fTimeModifier * 4.0f;
			}
		}
		return;
	}

	//if we get into here we have at least one broken piece
	pVeh->m_ucmd.upmove = 0;

	//if you're off the ground and not suspended, pitch down
	//FIXME: not in space!
	if (pVeh->m_LandTrace.fraction >= 0.1f)
	{
		if (!FighterSuspended(pVeh, parentPS))
		{
			//pVeh->m_ucmd.forwardmove = 0;
			//FIXME: don't bias towards pitching down when in space...
			if (!(pVeh->m_pParentEntity->s.number % 3))
			{
				//NOT everyone should do this
				pVeh->m_vOrientation[PITCH] += pVeh->m_fTimeModifier;
				if (!BG_UnrestrainedPitchRoll())
				{
					if (pVeh->m_vOrientation[PITCH] > 60.0f)
					{
						pVeh->m_vOrientation[PITCH] = 60.0f;
					}
				}
			}
			else if (!(pVeh->m_pParentEntity->s.number % 4))
			{
				pVeh->m_vOrientation[PITCH] -= pVeh->m_fTimeModifier;
				if (!BG_UnrestrainedPitchRoll())
				{
					if (pVeh->m_vOrientation[PITCH] > -60.0f)
					{
						pVeh->m_vOrientation[PITCH] = -60.0f;
					}
				}
			}
			//else: just keep going forward
		}
	}
#ifdef QAGAME
	if (pVeh->m_LandTrace.fraction < 1.0f)
	{
		//if you land at all when pieces of your ship are missing, then die
		auto gentity_s = pVeh->m_pParentEntity;
#ifdef _JK2MP//only have this info in MP...
		G_DamageFromKiller(parent, parent, nullptr, gentity_s->client->ps.origin, 999999, DAMAGE_NO_ARMOR, MOD_SUICIDE);
#else
		gentity_t* killer = gentity_s;
		G_Damage(gentity_s, killer, killer, vec3_origin, gentity_s->client->ps.origin, 99999, DAMAGE_NO_ARMOR,
		         MOD_SUICIDE);
#endif
	}
#endif

	if ((pVeh->m_iRemovedSurfaces & SHIPSURF_BROKEN_C ||
			pVeh->m_iRemovedSurfaces & SHIPSURF_BROKEN_D) &&
		(pVeh->m_iRemovedSurfaces & SHIPSURF_BROKEN_E ||
			pVeh->m_iRemovedSurfaces & SHIPSURF_BROKEN_F))
	{
		//wings on both side broken
		float factor = 2.0f;
		if (pVeh->m_iRemovedSurfaces & SHIPSURF_BROKEN_E &&
			pVeh->m_iRemovedSurfaces & SHIPSURF_BROKEN_F &&
			pVeh->m_iRemovedSurfaces & SHIPSURF_BROKEN_C &&
			pVeh->m_iRemovedSurfaces & SHIPSURF_BROKEN_D)
		{
			//all wings broken
			factor *= 2.0f;
		}

		if (!(pVeh->m_pParentEntity->s.number % 4) || !(pVeh->m_pParentEntity->s.number % 5))
		{
			//won't yaw, so increase roll factor
			factor *= 4.0f;
		}

		pVeh->m_vOrientation[ROLL] += pVeh->m_fTimeModifier * factor; //do some spiraling
	}
	else if (pVeh->m_iRemovedSurfaces & SHIPSURF_BROKEN_C ||
		pVeh->m_iRemovedSurfaces & SHIPSURF_BROKEN_D)
	{
		//left wing broken
		float factor = 2.0f;
		if (pVeh->m_iRemovedSurfaces & SHIPSURF_BROKEN_C &&
			pVeh->m_iRemovedSurfaces & SHIPSURF_BROKEN_D)
		{
			//if both are broken..
			factor *= 2.0f;
		}

		if (!(pVeh->m_pParentEntity->s.number % 2) || !(pVeh->m_pParentEntity->s.number % 6))
		{
			//won't yaw, so increase roll factor
			factor *= 4.0f;
		}

		pVeh->m_vOrientation[ROLL] += factor * pVeh->m_fTimeModifier;
	}
	else if (pVeh->m_iRemovedSurfaces & SHIPSURF_BROKEN_E ||
		pVeh->m_iRemovedSurfaces & SHIPSURF_BROKEN_F)
	{
		//right wing broken
		float factor = 2.0f;
		if (pVeh->m_iRemovedSurfaces & SHIPSURF_BROKEN_E &&
			pVeh->m_iRemovedSurfaces & SHIPSURF_BROKEN_F)
		{
			//if both are broken..
			factor *= 2.0f;
		}

		if (!(pVeh->m_pParentEntity->s.number % 2) || !(pVeh->m_pParentEntity->s.number % 6))
		{
			//won't yaw, so increase roll factor
			factor *= 4.0f;
		}

		pVeh->m_vOrientation[ROLL] -= factor * pVeh->m_fTimeModifier;
	}
}

#ifdef _JK2MP
void FighterYawAdjust(Vehicle_t* pVeh, playerState_t* riderPS, playerState_t* parentPS)
{
	float angDif = AngleSubtract(pVeh->m_vOrientation[YAW], riderPS->viewangles[YAW]);

	if (parentPS && parentPS->speed)
	{
		float s = parentPS->speed;
		float maxDif = pVeh->m_pVehicleInfo->turningSpeed * 0.8f; //magic number hackery

		if (s < 0.0f)
		{
			s = -s;
		}
		angDif *= s / pVeh->m_pVehicleInfo->speedMax;
		if (angDif > maxDif)
		{
			angDif = maxDif;
		}
		else if (angDif < -maxDif)
		{
			angDif = -maxDif;
		}
		pVeh->m_vOrientation[YAW] = AngleNormalize180(pVeh->m_vOrientation[YAW] - angDif * (pVeh->m_fTimeModifier * 0.2f));
	}
}

void FighterPitchAdjust(Vehicle_t* pVeh, playerState_t* riderPS, playerState_t* parentPS)
{
	float angDif = AngleSubtract(pVeh->m_vOrientation[PITCH], riderPS->viewangles[PITCH]);

	if (parentPS && parentPS->speed)
	{
		float s = parentPS->speed;
		float maxDif = pVeh->m_pVehicleInfo->turningSpeed * 0.8f; //magic number hackery

		if (s < 0.0f)
		{
			s = -s;
		}
		angDif *= s / pVeh->m_pVehicleInfo->speedMax;
		if (angDif > maxDif)
		{
			angDif = maxDif;
		}
		else if (angDif < -maxDif)
		{
			angDif = -maxDif;
		}
		pVeh->m_vOrientation[PITCH] = AngleNormalize360(pVeh->m_vOrientation[PITCH] - angDif * (pVeh->m_fTimeModifier * 0.2f));
	}
}
#endif

static void ProcessOrientCommands(Vehicle_t* pVeh)
{
	/********************************************************************************/
	/*	BEGIN	Here is where make sure the vehicle is properly oriented.	BEGIN	*/
	/********************************************************************************/

	bgEntity_t* parent = pVeh->m_pParentEntity;
	playerState_t *parentPS, *riderPS;
	float angleTimeMod;
#ifdef QAGAME
	constexpr float groundFraction = 0.1f;
#endif
	float curRoll;
	qboolean isDead;
	qboolean isLandingOrLanded;
#ifndef _JK2MP//SP
	int curTime = level.time;
#elif defined QAGAME//MP GAME
	int curTime = level.time;
#elif defined CGAME//MP CGAME
	//FIXME: pass in ucmd?  Not sure if this is reliable...
	int curTime = pm->cmd.serverTime;
#endif

#ifdef _JK2MP
	bgEntity_t* rider = nullptr;
	if (parent->s.owner != ENTITYNUM_NONE)
	{
		rider = PM_BGEntForNum(parent->s.owner); //&g_entities[parent->r.ownerNum];
	}
#else
	gentity_t* rider = parent->owner;
#endif

#ifdef _JK2MP
	if (!rider)
#else
	if (!rider || !rider->client)
#endif
	{
		rider = parent;
	}

#ifdef _JK2MP
	parentPS = parent->playerState;
	riderPS = rider->playerState;
	isDead = (qboolean)((parentPS->eFlags & EF_DEAD) != 0);
#else
	parentPS = &parent->client->ps;
	riderPS = &rider->client->ps;
	isDead = static_cast<qboolean>(parentPS->stats[STAT_HEALTH] <= 0);
#endif

	if (parentPS->hyperSpaceTime
		&& curTime - parentPS->hyperSpaceTime < HYPERSPACE_TIME)
	{
		//Going to Hyperspace
		VectorCopy(riderPS->viewangles, pVeh->m_vOrientation);
		VectorCopy(riderPS->viewangles, parentPS->viewangles);
		return;
	}

	if (pVeh->m_iDropTime >= curTime)
	{
		//you can only YAW during this
		parentPS->viewangles[YAW] = pVeh->m_vOrientation[YAW] = riderPS->viewangles[YAW];
		return;
	}

	angleTimeMod = pVeh->m_fTimeModifier;

	if (isDead || parentPS->electrifyTime >= curTime ||
		pVeh->m_pVehicleInfo->surfDestruction &&
		pVeh->m_iRemovedSurfaces &&
		pVeh->m_iRemovedSurfaces & SHIPSURF_BROKEN_C &&
		pVeh->m_iRemovedSurfaces & SHIPSURF_BROKEN_D &&
		pVeh->m_iRemovedSurfaces & SHIPSURF_BROKEN_E &&
		pVeh->m_iRemovedSurfaces & SHIPSURF_BROKEN_F)
	{
		//do some special stuff for when all the wings are torn off
		FighterDamageRoutine(pVeh, parent, parentPS, riderPS, isDead);
		pVeh->m_vOrientation[ROLL] = AngleNormalize180(pVeh->m_vOrientation[ROLL]);
		return;
	}

	if (!BG_UnrestrainedPitchRoll())
	{
		pVeh->m_vOrientation[ROLL] = PredictedAngularDecrement(0.95f, angleTimeMod * 2.0f, pVeh->m_vOrientation[ROLL]);
	}

	isLandingOrLanded = static_cast<qboolean>(FighterIsLanding(pVeh, parentPS) || FighterIsLanded(pVeh, parentPS));

	if (!isLandingOrLanded)
	{
		//don't do this stuff while landed.. I guess. I don't want ships spinning in place, looks silly.
		int m = 0;

		FighterWingMalfunctionCheck(pVeh, parentPS);

		while (m < 3)
		{
			const float aVelDif = pVeh->m_vFullAngleVelocity[m];

			if (aVelDif != 0.0f)
			{
				const float dForVel = aVelDif * 0.1f * pVeh->m_fTimeModifier;
				if (dForVel > 1.0f || dForVel < -1.0f)
				{
					pVeh->m_vOrientation[m] += dForVel;
					pVeh->m_vOrientation[m] = AngleNormalize180(pVeh->m_vOrientation[m]);
					if (m == PITCH)
					{
						//don't pitch downward into ground even more.
						if (pVeh->m_vOrientation[m] > 90.0f && pVeh->m_vOrientation[m] - dForVel < 90.0f)
						{
							pVeh->m_vOrientation[m] = 90.0f;
							pVeh->m_vFullAngleVelocity[m] = -pVeh->m_vFullAngleVelocity[m];
						}
					}
					if (riderPS)
					{
						riderPS->viewangles[m] = pVeh->m_vOrientation[m];
					}
					pVeh->m_vFullAngleVelocity[m] -= dForVel;
				}
				else
				{
					pVeh->m_vFullAngleVelocity[m] = 0.0f;
				}
			}

			m++;
		}
	}
	else
	{
		//clear angles once landed.
		VectorClear(pVeh->m_vFullAngleVelocity);
	}

	curRoll = pVeh->m_vOrientation[ROLL];

	// If we're landed, we shouldn't be able to do anything but take off.
	if (isLandingOrLanded //going slow enough to start landing
		&& !pVeh->m_iRemovedSurfaces
		&& parentPS->electrifyTime < curTime) //not spiraling out of control
	{
		if (parentPS->speed > 0.0f)
		{
			//Uh... what?  Why?
			if (pVeh->m_LandTrace.fraction < 0.3f)
			{
				pVeh->m_vOrientation[PITCH] = 0.0f;
			}
			else
			{
				pVeh->m_vOrientation[PITCH] = PredictedAngularDecrement(0.83f, angleTimeMod * 10.0f,
				                                                        pVeh->m_vOrientation[PITCH]);
			}
		}
		if (pVeh->m_LandTrace.fraction > 0.1f
			|| pVeh->m_LandTrace.plane.normal[2] < MIN_LANDING_SLOPE)
		{
			//off the ground, at least (or not on a valid landing surf)
			// Dampen the turn rate based on the current height.
#ifdef _JK2MP
			FighterYawAdjust(pVeh, riderPS, parentPS);
#else
			pVeh->m_vOrientation[YAW] = riderPS->viewangles[YAW]; //*pVeh->m_LandTrace.fraction;
#endif
		}
	}
	else if ((pVeh->m_iRemovedSurfaces || parentPS->electrifyTime >= curTime) //spiralling out of control
		&& (!(pVeh->m_pParentEntity->s.number % 2) || !(pVeh->m_pParentEntity->s.number % 6)))
	{
		//no yaw control
	}
	else if (pVeh->m_pPilot && pVeh->m_pPilot->s.number < MAX_CLIENTS && parentPS->speed > 0.0f)
	//&& !( pVeh->m_ucmd.forwardmove > 0 && pVeh->m_LandTrace.fraction != 1.0f ) )
	{
		if (BG_UnrestrainedPitchRoll())
		{
			VectorCopy(riderPS->viewangles, pVeh->m_vOrientation);
			VectorCopy(riderPS->viewangles, parentPS->viewangles);

			curRoll = pVeh->m_vOrientation[ROLL];

			FighterNoseMalfunctionCheck(pVeh, parentPS);

			//VectorCopy( pVeh->m_vOrientation, parentPS->viewangles );
		}
		else
		{
#ifdef _JK2MP
			FighterYawAdjust(pVeh, riderPS, parentPS);
#else
			pVeh->m_vOrientation[YAW] = riderPS->viewangles[YAW];
#endif

			// If we are not hitting the ground, allow the fighter to pitch up and down.
			if (!FighterOverValidLandingSurface(pVeh)
				|| parentPS->speed > MIN_LANDING_SPEED)
			//if ( ( pVeh->m_LandTrace.fraction >= 1.0f || pVeh->m_ucmd.forwardmove != 0 ) && pVeh->m_LandTrace.fraction >= 0.0f )
			{
				float fYawDelta;

#ifdef _JK2MP
				FighterPitchAdjust(pVeh, riderPS, parentPS);
#else
				pVeh->m_vOrientation[PITCH] = riderPS->viewangles[PITCH];
#endif

				FighterNoseMalfunctionCheck(pVeh, parentPS);

				// Adjust the roll based on the turn amount and dampen it a little.
				fYawDelta = AngleSubtract(pVeh->m_vOrientation[YAW], pVeh->m_vPrevOrientation[YAW]);
				//pVeh->m_vOrientation[YAW] - pVeh->m_vPrevOrientation[YAW];
				if (fYawDelta > 8.0f)
				{
					fYawDelta = 8.0f;
				}
				else if (fYawDelta < -8.0f)
				{
					fYawDelta = -8.0f;
				}
				curRoll -= fYawDelta;

				curRoll = PredictedAngularDecrement(0.93f, angleTimeMod * 2.0f, curRoll);
				//cap it reasonably
				//NOTE: was hardcoded to 40.0f, now using extern data
				if (pVeh->m_pVehicleInfo->rollLimit != -1)
				{
					if (curRoll > pVeh->m_pVehicleInfo->rollLimit)
					{
						curRoll = pVeh->m_pVehicleInfo->rollLimit;
					}
					else if (curRoll < -pVeh->m_pVehicleInfo->rollLimit)
					{
						curRoll = -pVeh->m_pVehicleInfo->rollLimit;
					}
				}
			}
		}
	}

	// If you are directly impacting the ground, even out your pitch.
	if (isLandingOrLanded)
	{
		//only if capable of landing
		if (!isDead
			&& parentPS->electrifyTime < curTime
			&& (!pVeh->m_pVehicleInfo->surfDestruction || !pVeh->m_iRemovedSurfaces))
		{
			//not crashing or spiraling out of control...
			if (pVeh->m_vOrientation[PITCH] > 0)
			{
				pVeh->m_vOrientation[PITCH] = PredictedAngularDecrement(0.2f, angleTimeMod * 10.0f,
				                                                        pVeh->m_vOrientation[PITCH]);
			}
			else
			{
				pVeh->m_vOrientation[PITCH] = PredictedAngularDecrement(0.75f, angleTimeMod * 10.0f,
				                                                        pVeh->m_vOrientation[PITCH]);
			}
		}
	}
	// If no one is in this vehicle and it's up in the sky, pitch it forward as it comes tumbling down.
#ifdef QAGAME //never gonna happen on client anyway, we can't be getting predicted unless the predicting client is boarded



	if (!pVeh->m_pVehicleInfo->Inhabited(pVeh)
		&& pVeh->m_LandTrace.fraction >= groundFraction
		&& !FighterIsInSpace(parent)
		&& !FighterSuspended(pVeh, parentPS))
	{
		pVeh->m_ucmd.upmove = 0;
		//pVeh->m_ucmd.forwardmove = 0;
		pVeh->m_vOrientation[PITCH] += pVeh->m_fTimeModifier;
		if (!BG_UnrestrainedPitchRoll())
		{
			if (pVeh->m_vOrientation[PITCH] > 60.0f)
			{
				pVeh->m_vOrientation[PITCH] = 60.0f;
			}
		}
	}
#endif

	if (!pVeh->m_fStrafeTime)
	{
		//use that roll
		pVeh->m_vOrientation[ROLL] = curRoll;
		//NOTE: this seems really backwards...
		if (pVeh->m_vOrientation[ROLL])
		{
			//continually adjust the yaw based on the roll..
			if ((pVeh->m_iRemovedSurfaces || parentPS->electrifyTime >= curTime) //spiralling out of control
				&& (!(pVeh->m_pParentEntity->s.number % 2) || !(pVeh->m_pParentEntity->s.number % 6)))
			{
				//leave YAW alone
			}
			else
			{
				if (!BG_UnrestrainedPitchRoll())
				{
					pVeh->m_vOrientation[YAW] -= pVeh->m_vOrientation[ROLL] * 0.05f * pVeh->m_fTimeModifier;
				}
			}
		}
	}
	else
	{
		//add in strafing roll
		const float strafeRoll = pVeh->m_fStrafeTime / MAX_STRAFE_TIME * pVeh->m_pVehicleInfo->rollLimit;
		//pVeh->m_pVehicleInfo->bankingSpeed*
		const float strafeDif = AngleSubtract(strafeRoll, pVeh->m_vOrientation[ROLL]);
		pVeh->m_vOrientation[ROLL] += strafeDif * 0.1f * pVeh->m_fTimeModifier;
		if (!BG_UnrestrainedPitchRoll())
		{
			//cap it reasonably
			if (pVeh->m_pVehicleInfo->rollLimit != -1
				&& !pVeh->m_iRemovedSurfaces
				&& parentPS->electrifyTime < curTime)
			{
				if (pVeh->m_vOrientation[ROLL] > pVeh->m_pVehicleInfo->rollLimit)
				{
					pVeh->m_vOrientation[ROLL] = pVeh->m_pVehicleInfo->rollLimit;
				}
				else if (pVeh->m_vOrientation[ROLL] < -pVeh->m_pVehicleInfo->rollLimit)
				{
					pVeh->m_vOrientation[ROLL] = -pVeh->m_pVehicleInfo->rollLimit;
				}
			}
		}
	}

	if (pVeh->m_pVehicleInfo->surfDestruction)
	{
		FighterDamageRoutine(pVeh, parent, parentPS, riderPS, isDead);
	}
	pVeh->m_vOrientation[ROLL] = AngleNormalize180(pVeh->m_vOrientation[ROLL]);
	/********************************************************************************/
	/*	END	Here is where make sure the vehicle is properly oriented.	END			*/
	/********************************************************************************/
}

#ifdef QAGAME //ONLY in SP or on server, not cgame

extern void PM_SetAnim(const pmove_t* pm, int setAnimParts, int anim, int setAnimFlags, int blendTime);

// This function makes sure that the vehicle is properly animated.
static void AnimateVehicle(Vehicle_t* pVeh)
{
	int Anim = -1;
	int iFlags = SETANIM_FLAG_NORMAL;
#ifdef _JK2MP
	playerState_t* parentPS = pVeh->m_pParentEntity->playerState;
#else
	playerState_t* parentPS = &pVeh->m_pParentEntity->client->ps;
#endif
#ifndef _JK2MP//SP
	//nothing
#elif defined QAGAME//MP GAME
	int curTime = level.time;
#elif defined CGAME//MP CGAME
	//FIXME: pass in ucmd?  Not sure if this is reliable...
	int curTime = pm->cmd.serverTime;
#endif
	int curTime = level.time;

	if (parentPS->hyperSpaceTime
		&& curTime - parentPS->hyperSpaceTime < HYPERSPACE_TIME)
	{
		//Going to Hyperspace
		//close the wings (FIXME: makes sense on X-Wing, not Shuttle?)
		if (pVeh->m_ulFlags & VEH_WINGSOPEN)
		{
			pVeh->m_ulFlags &= ~VEH_WINGSOPEN;
			Anim = BOTH_WINGS_CLOSE;
			if (pVeh->m_pVehicleInfo->soundfoilsclose)
			{
				G_SoundIndexOnEnt(pVeh->m_pParentEntity, CHAN_AUTO, pVeh->m_pVehicleInfo->soundfoilsclose);
			}
		}
	}
	else
	{
		const qboolean is_landing = FighterIsLanding(pVeh, parentPS);
		const qboolean is_landed = FighterIsLanded(pVeh, parentPS);

		// if we're above launch height (way up in the air)...
		if (!is_landing && !is_landed)
		{
			if (!(pVeh->m_ulFlags & VEH_WINGSOPEN))
			{
				pVeh->m_ulFlags |= VEH_WINGSOPEN;
				pVeh->m_ulFlags &= ~VEH_GEARSOPEN;
				Anim = BOTH_WINGS_OPEN;

				if (pVeh->m_pVehicleInfo->soundfoilsopen)
				{
					G_SoundIndexOnEnt(pVeh->m_pParentEntity, CHAN_BODY, pVeh->m_pVehicleInfo->soundfoilsopen);
				}
			}
		}
		// otherwise we're below launch height and still taking off.
		else
		{
			if ((pVeh->m_ucmd.forwardmove < 0 || pVeh->m_ucmd.upmove < 0 || is_landed)
				&& pVeh->m_LandTrace.fraction <= 0.4f
				&& pVeh->m_LandTrace.plane.normal[2] >= MIN_LANDING_SLOPE)
			{
				//already landed or trying to land and close to ground
				// Open gears.
				if (!(pVeh->m_ulFlags & VEH_GEARSOPEN))
				{
					if (pVeh->m_pVehicleInfo->soundLand && pVeh->m_pVehicleInfo->Inhabited(pVeh))
					{
						//just landed?
						G_SoundIndexOnEnt(pVeh->m_pParentEntity, CHAN_AUTO, pVeh->m_pVehicleInfo->soundLand);
					}
					pVeh->m_ulFlags |= VEH_GEARSOPEN;
					Anim = BOTH_GEARS_OPEN;
				}
			}
			else
			{
				//trying to take off and almost halfway off the ground
				// Close gears (if they're open).
				if (pVeh->m_ulFlags & VEH_GEARSOPEN)
				{
					pVeh->m_ulFlags &= ~VEH_GEARSOPEN;
					Anim = BOTH_GEARS_CLOSE;
					if (pVeh->m_pVehicleInfo->soundTakeOff)
					{
						//just landed?
						G_SoundIndexOnEnt(pVeh->m_pParentEntity, CHAN_AUTO, pVeh->m_pVehicleInfo->soundTakeOff);
					}
				}
				// If gears are closed, and we are below launch height, close the wings.
				else
				{
					if (pVeh->m_ulFlags & VEH_WINGSOPEN)
					{
						pVeh->m_ulFlags &= ~VEH_WINGSOPEN;
						Anim = BOTH_WINGS_CLOSE;
						if (pVeh->m_pVehicleInfo->soundfoilsclose)
						{
							G_SoundIndexOnEnt(pVeh->m_pParentEntity, CHAN_BODY, pVeh->m_pVehicleInfo->soundfoilsclose);
						}
					}
				}
			}
		}
	}

	if (Anim != -1)
	{
		int iBlend = 300;
#ifdef _JK2MP
		BG_SetAnim(pVeh->m_pParentEntity->playerState, bgAllAnims[pVeh->m_pParentEntity->localAnimIndex].anims,
			SETANIM_BOTH, Anim, iFlags, iBlend);
#else
		NPC_SetAnim(pVeh->m_pParentEntity, SETANIM_BOTH, Anim, iFlags, iBlend);
#endif
	}
}

// This function makes sure that the rider's in this vehicle are properly animated.
static void AnimateRiders(Vehicle_t* pVeh)
{
}

#endif //game-only

#ifndef QAGAME
void AttachRidersGeneric(Vehicle_t* pVeh);
#endif

void G_SetFighterVehicleFunctions(vehicleInfo_t* pVehInfo)
{
#ifdef QAGAME //ONLY in SP or on server, not cgame
	pVehInfo->AnimateVehicle = AnimateVehicle;
	pVehInfo->AnimateRiders = AnimateRiders;
	pVehInfo->Board = Board;
	pVehInfo->Eject = Eject;
	pVehInfo->Update = update;
#endif //game-only
	pVehInfo->ProcessMoveCommands = ProcessMoveCommands;
	pVehInfo->ProcessOrientCommands = ProcessOrientCommands;

#ifndef QAGAME //cgame prediction attachment func
	pVehInfo->AttachRiders = AttachRidersGeneric;
#endif
}

// Following is only in game, not in namespace
#ifdef _JK2MP
#include "../namespace_end.h"
#endif

#ifdef QAGAME
extern void G_AllocateVehicleObject(Vehicle_t** pVeh);
#endif

#ifdef _JK2MP
#include "../namespace_begin.h"
#endif

// Create/Allocate a new Animal Vehicle (initializing it as well).
void G_CreateFighterNPC(Vehicle_t** pVeh, const char* strType)
{
	// Allocate the Vehicle.
#ifdef _JK2MP
#ifdef QAGAME
	//these will remain on entities on the client once allocated because the pointer is
	//never stomped. on the server, however, when an ent is freed, the entity struct is
	//memset to 0, so this memory would be lost..
	G_AllocateVehicleObject(pVeh);
#else
	if (!*pVeh)
	{ //only allocate a new one if we really have to
		(*pVeh) = (Vehicle_t*)BG_Alloc(sizeof(Vehicle_t));
	}
#endif
	memset(*pVeh, 0, sizeof(Vehicle_t));
#else
	*pVeh = static_cast<Vehicle_t*>(gi.Malloc(sizeof(Vehicle_t), TAG_G_ALLOC, qtrue));
#endif
	(*pVeh)->m_pVehicleInfo = &g_vehicleInfo[BG_VehicleGetIndex(strType)];
}

#ifdef _JK2MP

#include "../namespace_end.h"

//get rid of all the crazy defs we added for this file
#undef currentAngles
#undef currentOrigin
#undef mins
#undef maxs
#undef legsAnimTimer
#undef torsoAnimTimer
#undef bool
#undef false
#undef true

#undef sqrtf
#undef Q_flrand

#undef MOD_EXPLOSIVE
#endif
