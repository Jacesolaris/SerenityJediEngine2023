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

#include "bg_public.h"
#include "bg_vehicles.h"

#ifdef _GAME
#include "g_local.h"
#elif _CGAME
#include "cgame/cg_local.h"
#endif

extern float DotToSpot(vec3_t spot, vec3_t from, vec3_t fromAngles);
#ifdef _GAME //SP or game side MP
extern vmCvar_t cg_thirdPersonAlpha;
extern vec3_t player_mins;
extern vec3_t player_maxs;
extern void ChangeWeapon(const gentity_t* ent, int newWeapon);
extern int PM_AnimLength(int index, animNumber_t anim);
extern void G_VehicleTrace(trace_t* results, const vec3_t start, const vec3_t tMins, const vec3_t tMaxs,
                           const vec3_t end, int pass_entity_num, int contentmask);
extern void G_DamageFromKiller(gentity_t* pEnt, gentity_t* pVehEnt, gentity_t* attacker, vec3_t org, int damage,
                               int dflags, int mod);
#endif

extern qboolean BG_UnrestrainedPitchRoll(const playerState_t* ps, Vehicle_t* p_veh);
extern void BG_SetAnim(playerState_t* ps, const animation_t* animations, int set_anim_parts, int anim,
                       int set_anim_flags);
extern int BG_GetTime(void);

//this stuff has got to be predicted, so..
qboolean BG_FighterUpdate(Vehicle_t* p_veh, const usercmd_t* pUcmd, vec3_t trMins, vec3_t trMaxs, const float gravity,
                          void (*traceFunc)(trace_t* results, const vec3_t start, const vec3_t lmins,
                                            const vec3_t lmaxs, const vec3_t end, int pass_entity_num, int content_mask))
{
	vec3_t bottom;
	playerState_t * parentPS;
#ifdef _GAME //don't do this on client

	// Make sure the riders are not visible or collide able.
	p_veh->m_pVehicleInfo->Ghost(p_veh, p_veh->m_pPilot);
	for (int i = 0; i < p_veh->m_pVehicleInfo->maxPassengers; i++)
	{
		p_veh->m_pVehicleInfo->Ghost(p_veh, p_veh->m_ppPassengers[i]);
	}
#endif

	parentPS = p_veh->m_pParentEntity->playerState;

	if (!parentPS)
	{
		Com_Error(ERR_DROP, "NULL PS in BG_FighterUpdate (%s)", p_veh->m_pVehicleInfo->name);
		return qfalse;
	}

	// If we have a pilot, take out gravity (it's a flying craft...).
	if (p_veh->m_pPilot)
	{
		parentPS->gravity = 0;
	}
	else
	{
		if (p_veh->m_pVehicleInfo->gravity)
		{
			parentPS->gravity = p_veh->m_pVehicleInfo->gravity;
		}
		else
		{
			//it doesn't have gravity specified apparently
			parentPS->gravity = gravity;
		}
	}

	// Check to see if the fighter has taken off yet (if it's a certain height above ground).
	VectorCopy(parentPS->origin, bottom);
	bottom[2] -= p_veh->m_pVehicleInfo->landingHeight;

	traceFunc(&p_veh->m_LandTrace, parentPS->origin, trMins, trMaxs, bottom, p_veh->m_pParentEntity->s.number,
	          MASK_NPCSOLID & ~CONTENTS_BODY);

	return qtrue;
}

#ifdef _GAME //ONLY in SP or on server, not cgame

// Like a think or move command, this updates various vehicle properties.
static qboolean Update(Vehicle_t* p_veh, const usercmd_t* pUcmd)
{
	assert(p_veh->m_pParentEntity);
	if (!BG_FighterUpdate(p_veh, pUcmd, ((gentity_t*)p_veh->m_pParentEntity)->r.mins,
	                      ((gentity_t*)p_veh->m_pParentEntity)->r.maxs,
	                      g_gravity.value,
	                      G_VehicleTrace))
	{
		return qfalse;
	}

	if (!g_vehicleInfo[VEHICLE_BASE].Update(p_veh, pUcmd))
	{
		return qfalse;
	}

	return qtrue;
}

// Board this Vehicle (get on). The first entity to board an empty vehicle becomes the Pilot.
static qboolean Board(Vehicle_t* p_veh, bgEntity_t* pEnt)
{
	if (!g_vehicleInfo[VEHICLE_BASE].Board(p_veh, pEnt))
		return qfalse;

	// Set the board wait time (they won't be able to do anything, including getting off, for this amount of time).
	p_veh->m_iBoarding = level.time + 1500;

	return qtrue;
}

// Eject an entity from the vehicle.
static qboolean Eject(Vehicle_t* p_veh, bgEntity_t* pEnt, const qboolean forceEject)
{
	if (g_vehicleInfo[VEHICLE_BASE].Eject(p_veh, pEnt, forceEject))
	{
		return qtrue;
	}

	return qfalse;
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

#ifdef _GAME//only do this check on game side, because if it's cgame, it's being predicted, and it's only predicted if the local client is the driver



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

qboolean FighterOverValidLandingSurface(const Vehicle_t* p_veh)
{
	if (p_veh->m_LandTrace.fraction < 1.0f //ground present
		&& p_veh->m_LandTrace.plane.normal[2] >= MIN_LANDING_SLOPE) //flat enough
	//FIXME: also check for a certain surface flag ... "landing zones"?
	{
		return qtrue;
	}
	return qfalse;
}

qboolean FighterIsLanded(const Vehicle_t* p_veh, const playerState_t* parentPS)
{
	if (FighterOverValidLandingSurface(p_veh)
		&& !parentPS->speed) //stopped
	{
		return qtrue;
	}
	return qfalse;
}

qboolean FighterIsLanding(Vehicle_t* p_veh, const playerState_t* parentPS)
{
	if (FighterOverValidLandingSurface(p_veh)
#ifdef _GAME//only do this check on game side, because if it's cgame, it's being predicted, and it's only predicted if the local client is the driver



		&& p_veh->m_pVehicleInfo->Inhabited(p_veh) //has to have a driver in order to be capable of landing
#endif
		&& (p_veh->m_ucmd.forwardmove < 0 || p_veh->m_ucmd.upmove < 0) //decelerating or holding crouch button
		&& parentPS->speed <= MIN_LANDING_SPEED)
	//going slow enough to start landing - was using p_veh->m_pVehicleInfo->speedIdle, but that's still too fast
	{
		return qtrue;
	}
	return qfalse;
}

qboolean FighterIsLaunching(Vehicle_t* p_veh, const playerState_t* parentPS)
{
	if (FighterOverValidLandingSurface(p_veh)
#ifdef _GAME//only do this check on game side, because if it's cgame, it's being predicted, and it's only predicted if the local client is the driver



		&& p_veh->m_pVehicleInfo->Inhabited(p_veh) //has to have a driver in order to be capable of landing
#endif
		&& p_veh->m_ucmd.upmove > 0 //trying to take off
		&& parentPS->speed <= 200.0f)
	//going slow enough to start landing - was using p_veh->m_pVehicleInfo->speedIdle, but that's still too fast
	{
		return qtrue;
	}
	return qfalse;
}

qboolean FighterSuspended(const Vehicle_t* p_veh, const playerState_t* parentPS)
{
#ifdef _GAME//only do this check on game side, because if it's cgame, it's being predicted, and it's only predicted if the local client is the driver



	if (!p_veh->m_pPilot //empty
		&& !parentPS->speed //not moving
		&& p_veh->m_ucmd.forwardmove <= 0 //not trying to go forward for whatever reason
		&& p_veh->m_pParentEntity != NULL
		&& ((gentity_t*)p_veh->m_pParentEntity)->spawnflags & 2) //SUSPENDED spawnflag is on
	{
		return qtrue;
	}
	return qfalse;
#elif _CGAME
	return qfalse;
#endif
}

//MP RULE - ALL PROCESSMOVECOMMANDS FUNCTIONS MUST BE BG-COMPATIBLE!!!
//If you really need to violate this rule for SP, then use ifdefs.
//By BG-compatible, I mean no use of game-specific data - ONLY use
//stuff available in the MP bgEntity (in SP, the bgEntity is #defined
//as a gentity, but the MP-compatible access restrictions are based
//on the bgEntity structure in the MP codebase) -rww
// ProcessMoveCommands the Vehicle.
#define FIGHTER_MIN_TAKEOFF_FRACTION 0.7f

static void ProcessMoveCommands(Vehicle_t* p_veh)
{
	/************************************************************************************/
	/*	BEGIN	Here is where we move the vehicle (forward or back or whatever). BEGIN	*/
	/************************************************************************************/

	//Client sets ucmds and such for speed alterations
	float speedMax;
	bgEntity_t* parent = p_veh->m_pParentEntity;
	//this function should only be called from pmove.. if it gets called else where,
	//obviously this will explode.
	const int curTime = pm->cmd.serverTime;

	playerState_t * parentPS = parent->playerState;

	if (parentPS->hyperSpaceTime
		&& curTime - parentPS->hyperSpaceTime < HYPERSPACE_TIME)
	{
		//Going to Hyperspace
		//totally override movement
		const float timeFrac = (float)(curTime - parentPS->hyperSpaceTime) / HYPERSPACE_TIME;
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
					//MIKE: This is going to play the sound twice for the predicting client, I suggest using
					//a predicted event or only doing it game-side. -rich
#ifdef _GAME
					//G_EntitySound( ((gentity_t *)(p_veh->m_pParentEntity)), CHAN_LOCAL, p_veh->m_pVehicleInfo->soundHyper );
#elif _CGAME
					trap->S_StartSound(NULL, pm->ps->client_num, CHAN_LOCAL, p_veh->m_pVehicleInfo->soundHyper);
#endif
				}

				parentPS->speed = HYPERSPACE_SPEED;
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

	if (p_veh->m_iDropTime >= curTime)
	{
		//no speed, just drop
		parentPS->speed = 0.0f;
		parentPS->gravity = 800;
		return;
	}

	const qboolean isLandingOrLaunching = FighterIsLanding(p_veh, parentPS) || FighterIsLaunching(p_veh, parentPS);

	// If we are hitting the ground, just allow the fighter to go up and down.
	if (isLandingOrLaunching //going slow enough to start landing
		&& (p_veh->m_ucmd.forwardmove <= 0 || p_veh->m_LandTrace.fraction <= FIGHTER_MIN_TAKEOFF_FRACTION))
	//not trying to accelerate away already (or: you are trying to, but not high enough off the ground yet)
	{
		//FIXME: if start to move forward and fly over something low while still going relatively slow, you may try to land even though you don't mean to...
		//float fInvFrac = 1.0f - p_veh->m_LandTrace.fraction;

		if (p_veh->m_ucmd.upmove > 0)
		{
			if (parentPS->velocity[2] <= 0
				&& p_veh->m_pVehicleInfo->soundTakeOff)
			{
				//taking off for the first time
#ifdef _GAME//MP GAME-side
				G_EntitySound((gentity_t*)p_veh->m_pParentEntity, CHAN_AUTO, p_veh->m_pVehicleInfo->soundTakeOff);
#endif
			}
			parentPS->velocity[2] += p_veh->m_pVehicleInfo->acceleration * p_veh->m_fTimeModifier;
			// * ( /*fInvFrac **/ 1.5f );
		}
		else if (p_veh->m_ucmd.upmove < 0)
		{
			parentPS->velocity[2] -= p_veh->m_pVehicleInfo->acceleration * p_veh->m_fTimeModifier;
			// * ( /*fInvFrac **/ 1.8f );
		}
		else if (p_veh->m_ucmd.forwardmove < 0)
		{
			if (p_veh->m_LandTrace.fraction != 0.0f)
			{
				parentPS->velocity[2] -= p_veh->m_pVehicleInfo->acceleration * p_veh->m_fTimeModifier;
			}

			if (p_veh->m_LandTrace.fraction <= FIGHTER_MIN_TAKEOFF_FRACTION)
			{
				parentPS->velocity[2] = PredictedAngularDecrement(p_veh->m_LandTrace.fraction,
				                                                  p_veh->m_fTimeModifier * 5.0f, parentPS->velocity[2]);

				parentPS->speed = 0;
			}
		}

		// Make sure they don't pitch as they near the ground.
		//p_veh->m_vOrientation[PITCH] *= 0.7f;
		p_veh->m_vOrientation[PITCH] = PredictedAngularDecrement(0.7f, p_veh->m_fTimeModifier * 10.0f,
		                                                        p_veh->m_vOrientation[PITCH]);

		return;
	}

	if (p_veh->m_ucmd.upmove > 0 && p_veh->m_pVehicleInfo->turboSpeed)
	{
		if (curTime - p_veh->m_iTurboTime > p_veh->m_pVehicleInfo->turboRecharge)
		{
			p_veh->m_iTurboTime = curTime + p_veh->m_pVehicleInfo->turboDuration;

#ifdef _GAME//MP GAME-side
			//NOTE: turbo sound can't be part of effect if effect is played on every muzzle!
			if (p_veh->m_pVehicleInfo->soundTurbo)
			{
				G_EntitySound((gentity_t*)p_veh->m_pParentEntity, CHAN_AUTO, p_veh->m_pVehicleInfo->soundTurbo);
			}
#endif
		}
	}
	float speedInc = p_veh->m_pVehicleInfo->acceleration * p_veh->m_fTimeModifier;
	if (curTime < p_veh->m_iTurboTime)
	{
		//going turbo speed
		speedMax = p_veh->m_pVehicleInfo->turboSpeed;
		speedInc = p_veh->m_pVehicleInfo->acceleration * 2.0f * p_veh->m_fTimeModifier;

		//force us to move forward
		p_veh->m_ucmd.forwardmove = 127;
		//add flag to let cgame know to draw the iTurboFX effect
		parentPS->eFlags |= EF_JETPACK_ACTIVE;
	}
	else
	{
		//normal max speed
		speedMax = p_veh->m_pVehicleInfo->speedMax;
		if (parentPS->eFlags & EF_JETPACK_ACTIVE)
		{
			//stop cgame from playing the turbo exhaust effect
			parentPS->eFlags &= ~EF_JETPACK_ACTIVE;
		}
	}
	float speedIdleDec = p_veh->m_pVehicleInfo->decelIdle * p_veh->m_fTimeModifier;
	const float speedIdle = p_veh->m_pVehicleInfo->speedIdle;
	const float speedIdleAccel = p_veh->m_pVehicleInfo->accelIdle * p_veh->m_fTimeModifier;
	const float speedMin = p_veh->m_pVehicleInfo->speedMin;

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

	if (p_veh->m_iRemovedSurfaces
		|| parentPS->electrifyTime >= curTime)
	{
		//go out of control
		parentPS->speed += speedInc;
		//Why set forwardmove?  PMove code doesn't use it... does it?
		p_veh->m_ucmd.forwardmove = 127;
	}
#ifdef _GAME //well, the thing is always going to be inhabited if it's being predicted!
	else if (FighterSuspended(p_veh, parentPS))
	{
		parentPS->speed = 0;
		p_veh->m_ucmd.forwardmove = 0;
	}
	else if (!p_veh->m_pVehicleInfo->Inhabited(p_veh)
		&& parentPS->speed > 0)
	{
		//pilot jumped out while we were moving forward (not landing or landed) so just keep the throttle locked
		//Why set forwardmove?  PMove code doesn't use it... does it?
		p_veh->m_ucmd.forwardmove = 127;
	}
#endif
	else if ((parentPS->speed || parentPS->groundEntityNum == ENTITYNUM_NONE ||
		p_veh->m_ucmd.forwardmove || p_veh->m_ucmd.upmove > 0) && p_veh->m_LandTrace.fraction >= 0.05f)
	{
		if (p_veh->m_ucmd.forwardmove > 0 && speedInc)
		{
			parentPS->speed += speedInc;
			p_veh->m_ucmd.forwardmove = 127;
		}
		else if (p_veh->m_ucmd.forwardmove < 0
			|| p_veh->m_ucmd.upmove < 0)
		{
			//decelerating or braking
			if (p_veh->m_ucmd.upmove < 0)
			{
				//braking (trying to land?), slow down faster
				if (p_veh->m_ucmd.forwardmove)
				{
					//decelerator + brakes
					speedInc += p_veh->m_pVehicleInfo->braking;
					speedIdleDec += p_veh->m_pVehicleInfo->braking;
				}
				else
				{
					//just brakes
					speedInc = speedIdleDec = p_veh->m_pVehicleInfo->braking;
				}
			}
			if (parentPS->speed > speedIdle)
			{
				parentPS->speed -= speedInc;
			}
			else if (parentPS->speed > speedMin)
			{
				if (FighterOverValidLandingSurface(p_veh))
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
			if (p_veh->m_pVehicleInfo->type == VH_FIGHTER)
			{
				p_veh->m_ucmd.forwardmove = 127;
			}
			else if (speedMin >= 0)
			{
				p_veh->m_ucmd.forwardmove = 0;
			}
		}
		//else not accel, decel or braking
		else if (p_veh->m_pVehicleInfo->throttleSticks)
		{
			//we're using a throttle that sticks at current speed
			if (parentPS->speed <= MIN_LANDING_SPEED)
			{
				//going less than landing speed
				if (FighterOverValidLandingSurface(p_veh))
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
			if ((p_veh->m_LandTrace.fraction >= 1.0f //no ground
					|| p_veh->m_LandTrace.plane.normal[2] < MIN_LANDING_SLOPE) //or can't land on ground below us
				&& speedIdle > 0)
			{
				//not above ground and have an idle speed
				//float fSpeed = p_veh->m_pParentEntity->client->ps.speed;
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
		if (p_veh->m_ucmd.forwardmove < 0)
		{
			p_veh->m_ucmd.forwardmove = 0;
		}
		if (p_veh->m_ucmd.upmove < 0)
		{
			p_veh->m_ucmd.upmove = 0;
		}
	}

#if 1//This is working now, but there are some transitional jitters... Rich?
	//STRAFING==============================================================================
	if (p_veh->m_pVehicleInfo->strafePerc
#ifdef _GAME//only do this check on game side, because if it's cgame, it's being predicted, and it's only predicted if the local client is the driver



		&& p_veh->m_pVehicleInfo->Inhabited(p_veh) //has to have a driver in order to be capable of landing
#endif
		&& !p_veh->m_iRemovedSurfaces
		&& parentPS->electrifyTime < curTime
		&& parentPS->vehTurnaroundTime < curTime
		&& (p_veh->m_LandTrace.fraction >= 1.0f //no ground
			|| p_veh->m_LandTrace.plane.normal[2] < MIN_LANDING_SLOPE //can't land here
			|| parentPS->speed > MIN_LANDING_SPEED) //going too fast to land
		&& p_veh->m_ucmd.rightmove)
	{
		//strafe
		vec3_t vAngles, vRight;
		float strafeSpeed = p_veh->m_pVehicleInfo->strafePerc * speedMax * 5.0f;
		VectorCopy(p_veh->m_vOrientation, vAngles);
		vAngles[PITCH] = vAngles[ROLL] = 0;
		AngleVectors(vAngles, NULL, vRight, NULL);

		if (p_veh->m_ucmd.rightmove > 0)
		{
			//strafe right
			//FIXME: this will probably make it possible to cheat and
			//		go faster than max speed if you keep turning and strafing...
			if (parentPS->hackingTime > -MAX_STRAFE_TIME)
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
					VectorMA(parentPS->velocity, strafeSpeed * p_veh->m_fTimeModifier, vRight, parentPS->velocity);
				}
				parentPS->hackingTime -= 50 * p_veh->m_fTimeModifier;
			}
		}
		else
		{
			//strafe left
			if (parentPS->hackingTime < MAX_STRAFE_TIME)
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
					VectorMA(parentPS->velocity, -strafeSpeed * p_veh->m_fTimeModifier, vRight, parentPS->velocity);
				}
				parentPS->hackingTime += 50 * p_veh->m_fTimeModifier;
			}
		}
		//strafing takes away from forward speed?  If so, strafePerc above should use speedMax
		//parentPS->speed *= (1.0f-p_veh->m_pVehicleInfo->strafePerc);
	}
	else
	{
		if (parentPS->hackingTime > 0)
		{
			parentPS->hackingTime -= 50 * p_veh->m_fTimeModifier;
			if (parentPS->hackingTime < 0)
			{
				parentPS->hackingTime = 0.0f;
			}
		}
		else if (parentPS->hackingTime < 0)
		{
			parentPS->hackingTime += 50 * p_veh->m_fTimeModifier;
			if (parentPS->hackingTime > 0)
			{
				parentPS->hackingTime = 0.0f;
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

#ifdef _GAME//FIXME: get working in game and cgame
	if (p_veh->m_vOrientation[PITCH] * 0.1f > 10.0f)
	{
		//pitched downward, increase speed more and more based on our tilt
		if (FighterIsInSpace((gentity_t*)parent))
		{
			//in space, do nothing with speed base on pitch...
		}
		else
		{
			//really should only do this when on a planet
			float mult = p_veh->m_vOrientation[PITCH] * 0.1f;
			if (mult < 1.0f)
			{
				mult = 1.0f;
			}
			parentPS->speed = PredictedAngularDecrement(mult, p_veh->m_fTimeModifier * 10.0f, parentPS->speed);
		}
	}

	if (p_veh->m_iRemovedSurfaces
		|| parentPS->electrifyTime >= curTime)
	{
		//going down
		if (FighterIsInSpace((gentity_t*)parent))
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
	else if (FighterSuspended(p_veh, parentPS))
	{
		parentPS->gravity = 0;
	}
	else if ((!parentPS->speed || parentPS->speed < speedIdle) && p_veh->m_ucmd.upmove <= 0)
	{
		//slowing down or stopped and not trying to take off
		if (FighterIsInSpace((gentity_t*)parent))
		{
			//we're in space, stopping doesn't make us drift downward
			if (FighterOverValidLandingSurface(p_veh))
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
#else//FIXME: get above checks working in game and cgame
	parentPS->gravity = 0;
#endif

	/********************************************************************************/
	/*	END Here is where we move the vehicle (forward or back or whatever). END	*/
	/********************************************************************************/
}

extern void BG_VehicleTurnRateForSpeed(const Vehicle_t* p_veh, float speed, float* mPitchOverride, float* mYawOverride);

static void FighterWingMalfunctionCheck(const Vehicle_t* p_veh, const playerState_t* parentPS)
{
	float mPitchOverride = 1.0f;
	float mYawOverride = 1.0f;
	BG_VehicleTurnRateForSpeed(p_veh, parentPS->speed, &mPitchOverride, &mYawOverride);
	//check right wing damage
	if (parentPS->brokenLimbs & 1 << SHIPSURF_DAMAGE_RIGHT_HEAVY)
	{
		//right wing has taken heavy damage
		p_veh->m_vOrientation[ROLL] += (sin(p_veh->m_ucmd.serverTime * 0.001) + 1.0f) * p_veh->m_fTimeModifier *
			mYawOverride * 50.0f;
	}
	else if (parentPS->brokenLimbs & 1 << SHIPSURF_DAMAGE_RIGHT_LIGHT)
	{
		//right wing has taken light damage
		p_veh->m_vOrientation[ROLL] += (sin(p_veh->m_ucmd.serverTime * 0.001) + 1.0f) * p_veh->m_fTimeModifier *
			mYawOverride * 12.5f;
	}

	//check left wing damage
	if (parentPS->brokenLimbs & 1 << SHIPSURF_DAMAGE_LEFT_HEAVY)
	{
		//left wing has taken heavy damage
		p_veh->m_vOrientation[ROLL] -= (sin(p_veh->m_ucmd.serverTime * 0.001) + 1.0f) * p_veh->m_fTimeModifier *
			mYawOverride * 50.0f;
	}
	else if (parentPS->brokenLimbs & 1 << SHIPSURF_DAMAGE_LEFT_LIGHT)
	{
		//left wing has taken light damage
		p_veh->m_vOrientation[ROLL] -= (sin(p_veh->m_ucmd.serverTime * 0.001) + 1.0f) * p_veh->m_fTimeModifier *
			mYawOverride * 12.5f;
	}
}

static void FighterNoseMalfunctionCheck(const Vehicle_t* p_veh, const playerState_t* parentPS)
{
	float mPitchOverride = 1.0f;
	float mYawOverride = 1.0f;
	BG_VehicleTurnRateForSpeed(p_veh, parentPS->speed, &mPitchOverride, &mYawOverride);
	//check nose damage
	if (parentPS->brokenLimbs & 1 << SHIPSURF_DAMAGE_FRONT_HEAVY)
	{
		//nose has taken heavy damage
		//pitch up and down over time
		p_veh->m_vOrientation[PITCH] += sin(p_veh->m_ucmd.serverTime * 0.001) * p_veh->m_fTimeModifier * mPitchOverride *
			50.0f;
	}
	else if (parentPS->brokenLimbs & 1 << SHIPSURF_DAMAGE_FRONT_LIGHT)
	{
		//nose has taken heavy damage
		//pitch up and down over time
		p_veh->m_vOrientation[PITCH] += sin(p_veh->m_ucmd.serverTime * 0.001) * p_veh->m_fTimeModifier * mPitchOverride *
			20.0f;
	}
}

static void FighterDamageRoutine(Vehicle_t* p_veh, bgEntity_t* parent, const playerState_t* parentPS,
                                 const playerState_t* riderPS,
                                 const qboolean isDead)
{
	if (!p_veh->m_iRemovedSurfaces)
	{
		//still in one piece
		if (p_veh->m_pParentEntity && isDead)
		{
			//death spiral
			p_veh->m_ucmd.upmove = 0;
			//FIXME: don't bias toward pitching down when not in space
			/*
			if ( FighterIsInSpace( p_veh->m_pParentEntity ) )
			{
			}
			else
			*/
			if (!(p_veh->m_pParentEntity->s.number % 3))
			{
				//NOT everyone should do this
				p_veh->m_vOrientation[PITCH] += p_veh->m_fTimeModifier;
				if (!BG_UnrestrainedPitchRoll(riderPS, p_veh))
				{
					if (p_veh->m_vOrientation[PITCH] > 60.0f)
					{
						p_veh->m_vOrientation[PITCH] = 60.0f;
					}
				}
			}
			else if (!(p_veh->m_pParentEntity->s.number % 2))
			{
				p_veh->m_vOrientation[PITCH] -= p_veh->m_fTimeModifier;
				if (!BG_UnrestrainedPitchRoll(riderPS, p_veh))
				{
					if (p_veh->m_vOrientation[PITCH] > -60.0f)
					{
						p_veh->m_vOrientation[PITCH] = -60.0f;
					}
				}
			}
			if (p_veh->m_pParentEntity->s.number % 2)
			{
				p_veh->m_vOrientation[YAW] += p_veh->m_fTimeModifier;
				p_veh->m_vOrientation[ROLL] += p_veh->m_fTimeModifier * 4.0f;
			}
			else
			{
				p_veh->m_vOrientation[YAW] -= p_veh->m_fTimeModifier;
				p_veh->m_vOrientation[ROLL] -= p_veh->m_fTimeModifier * 4.0f;
			}
		}
		return;
	}

	//if we get into here we have at least one broken piece
	p_veh->m_ucmd.upmove = 0;

	//if you're off the ground and not suspended, pitch down
	//FIXME: not in space!
	if (p_veh->m_LandTrace.fraction >= 0.1f)
	{
		if (!FighterSuspended(p_veh, parentPS))
		{
			//p_veh->m_ucmd.forwardmove = 0;
			//FIXME: don't bias towards pitching down when in space...
			if (!(p_veh->m_pParentEntity->s.number % 3))
			{
				//NOT everyone should do this
				p_veh->m_vOrientation[PITCH] += p_veh->m_fTimeModifier;
				if (!BG_UnrestrainedPitchRoll(riderPS, p_veh))
				{
					if (p_veh->m_vOrientation[PITCH] > 60.0f)
					{
						p_veh->m_vOrientation[PITCH] = 60.0f;
					}
				}
			}
			else if (!(p_veh->m_pParentEntity->s.number % 4))
			{
				p_veh->m_vOrientation[PITCH] -= p_veh->m_fTimeModifier;
				if (!BG_UnrestrainedPitchRoll(riderPS, p_veh))
				{
					if (p_veh->m_vOrientation[PITCH] > -60.0f)
					{
						p_veh->m_vOrientation[PITCH] = -60.0f;
					}
				}
			}
			//else: just keep going forward
		}
	}
#ifdef _GAME
	if (p_veh->m_LandTrace.fraction < 1.0f)
	{
		//if you land at all when pieces of your ship are missing, then die
		gentity_t* vparent = (gentity_t*)p_veh->m_pParentEntity;

#ifdef _JK2MP//only have this info in MP...
		G_DamageFromKiller(vparent, vparent, NULL, vparent->client->ps.origin, 999999, DAMAGE_NO_ARMOR, MOD_SUICIDE);
#else
		gentity_t* killer = vparent;
		G_Damage(vparent, killer, killer, vec3_origin, vparent->client->ps.origin, 99999, DAMAGE_NO_ARMOR, MOD_SUICIDE);
#endif
	}
#endif

	if ((p_veh->m_iRemovedSurfaces & SHIPSURF_BROKEN_C ||
			p_veh->m_iRemovedSurfaces & SHIPSURF_BROKEN_D) &&
		(p_veh->m_iRemovedSurfaces & SHIPSURF_BROKEN_E ||
			p_veh->m_iRemovedSurfaces & SHIPSURF_BROKEN_F))
	{
		//wings on both side broken
		float factor = 2.0f;
		if (p_veh->m_iRemovedSurfaces & SHIPSURF_BROKEN_E &&
			p_veh->m_iRemovedSurfaces & SHIPSURF_BROKEN_F &&
			p_veh->m_iRemovedSurfaces & SHIPSURF_BROKEN_C &&
			p_veh->m_iRemovedSurfaces & SHIPSURF_BROKEN_D)
		{
			//all wings broken
			factor *= 2.0f;
		}

		if (!(p_veh->m_pParentEntity->s.number % 2) || !(p_veh->m_pParentEntity->s.number % 6))
		{
			//won't yaw, so increase roll factor
			factor *= 4.0f;
		}

		p_veh->m_vOrientation[ROLL] += p_veh->m_fTimeModifier * factor;
	}
	else if (p_veh->m_iRemovedSurfaces & SHIPSURF_BROKEN_C ||
		p_veh->m_iRemovedSurfaces & SHIPSURF_BROKEN_D)
	{
		//left wing broken
		float factor = 2.0f;
		if (p_veh->m_iRemovedSurfaces & SHIPSURF_BROKEN_C &&
			p_veh->m_iRemovedSurfaces & SHIPSURF_BROKEN_D)
		{
			//if both are broken..
			factor *= 2.0f;
		}

		if (!(p_veh->m_pParentEntity->s.number % 2) || !(p_veh->m_pParentEntity->s.number % 6))
		{
			//won't yaw, so increase roll factor
			factor *= 4.0f;
		}

		p_veh->m_vOrientation[ROLL] += factor * p_veh->m_fTimeModifier;
	}
	else if (p_veh->m_iRemovedSurfaces & SHIPSURF_BROKEN_E ||
		p_veh->m_iRemovedSurfaces & SHIPSURF_BROKEN_F)
	{
		//right wing broken
		float factor = 2.0f;
		if (p_veh->m_iRemovedSurfaces & SHIPSURF_BROKEN_E &&
			p_veh->m_iRemovedSurfaces & SHIPSURF_BROKEN_F)
		{
			//if both are broken..
			factor *= 2.0f;
		}

		if (!(p_veh->m_pParentEntity->s.number % 2) || !(p_veh->m_pParentEntity->s.number % 6))
		{
			//won't yaw, so increase roll factor
			factor *= 4.0f;
		}

		p_veh->m_vOrientation[ROLL] -= factor * p_veh->m_fTimeModifier;
	}
}

#ifdef VEH_CONTROL_SCHEME_4

#define FIGHTER_TURNING_MULTIPLIER 0.8f//was 1.6f //magic number hackery
#define FIGHTER_TURNING_DEADZONE 0.25f//no turning if offset is this much
void FighterRollAdjust(Vehicle_t* p_veh, playerState_t* riderPS, playerState_t* parentPS)
{
	float angDif = AngleSubtract(p_veh->m_vPrevRiderViewAngles[YAW], riderPS->viewangles[YAW]);///2.0f;//AngleSubtract(p_veh->m_vPrevRiderViewAngles[YAW], riderPS->viewangles[YAW]);

	angDif *= 0.5f;
	if (angDif > 0.0f)
	{
		angDif *= angDif;
	}
	else if (angDif < 0.0f)
	{
		angDif *= -angDif;
	}

	if (parentPS && parentPS->speed)
	{
		float maxDif = p_veh->m_pVehicleInfo->turningSpeed * FIGHTER_TURNING_MULTIPLIER;

		if (p_veh->m_pVehicleInfo->speedDependantTurning)
		{
			float speedFrac = 1.0f;
			if (p_veh->m_LandTrace.fraction >= 1.0f
				|| p_veh->m_LandTrace.plane.normal[2] < MIN_LANDING_SLOPE)
			{
				float s = parentPS->speed;
				if (s < 0.0f)
				{
					s = -s;
				}
				speedFrac = (s / (p_veh->m_pVehicleInfo->speedMax * 0.75f));
				if (speedFrac < 0.25f)
				{
					speedFrac = 0.25f;
				}
				else if (speedFrac > 1.0f)
				{
					speedFrac = 1.0f;
				}
			}
			angDif *= speedFrac;
		}
		if (angDif > maxDif)
		{
			angDif = maxDif;
		}
		else if (angDif < -maxDif)
		{
			angDif = -maxDif;
		}
		p_veh->m_vOrientation[ROLL] = AngleNormalize180(p_veh->m_vOrientation[ROLL] + angDif * (p_veh->m_fTimeModifier * 0.2f));
	}
}

void FighterYawAdjust(Vehicle_t* p_veh, playerState_t* riderPS, playerState_t* parentPS)
{
	float angDif = AngleSubtract(p_veh->m_vPrevRiderViewAngles[YAW], riderPS->viewangles[YAW]);///2.0f;//AngleSubtract(p_veh->m_vPrevRiderViewAngles[YAW], riderPS->viewangles[YAW]);
	if (fabs(angDif) < FIGHTER_TURNING_DEADZONE)
	{
		angDif = 0.0f;
	}
	else if (angDif >= FIGHTER_TURNING_DEADZONE)
	{
		angDif -= FIGHTER_TURNING_DEADZONE;
	}
	else if (angDif <= -FIGHTER_TURNING_DEADZONE)
	{
		angDif += FIGHTER_TURNING_DEADZONE;
	}

	angDif *= 0.5f;
	if (angDif > 0.0f)
	{
		angDif *= angDif;
	}
	else if (angDif < 0.0f)
	{
		angDif *= -angDif;
	}

	if (parentPS && parentPS->speed)
	{
		float maxDif = p_veh->m_pVehicleInfo->turningSpeed * FIGHTER_TURNING_MULTIPLIER;

		if (p_veh->m_pVehicleInfo->speedDependantTurning)
		{
			float speedFrac = 1.0f;
			if (p_veh->m_LandTrace.fraction >= 1.0f
				|| p_veh->m_LandTrace.plane.normal[2] < MIN_LANDING_SLOPE)
			{
				float s = parentPS->speed;
				if (s < 0.0f)
				{
					s = -s;
				}
				speedFrac = (s / (p_veh->m_pVehicleInfo->speedMax * 0.75f));
				if (speedFrac < 0.25f)
				{
					speedFrac = 0.25f;
				}
				else if (speedFrac > 1.0f)
				{
					speedFrac = 1.0f;
				}
			}
			angDif *= speedFrac;
		}
		if (angDif > maxDif)
		{
			angDif = maxDif;
		}
		else if (angDif < -maxDif)
		{
			angDif = -maxDif;
		}
		p_veh->m_vOrientation[YAW] = AngleNormalize180(p_veh->m_vOrientation[YAW] - (angDif * (p_veh->m_fTimeModifier * 0.2f)));
	}
}

void FighterPitchAdjust(Vehicle_t* p_veh, playerState_t* riderPS, playerState_t* parentPS)
{
	float angDif = AngleSubtract(0, riderPS->viewangles[PITCH]);//AngleSubtract(p_veh->m_vPrevRiderViewAngles[PITCH], riderPS->viewangles[PITCH]);
	if (fabs(angDif) < FIGHTER_TURNING_DEADZONE)
	{
		angDif = 0.0f;
	}
	else if (angDif >= FIGHTER_TURNING_DEADZONE)
	{
		angDif -= FIGHTER_TURNING_DEADZONE;
	}
	else if (angDif <= -FIGHTER_TURNING_DEADZONE)
	{
		angDif += FIGHTER_TURNING_DEADZONE;
	}

	angDif *= 0.5f;
	if (angDif > 0.0f)
	{
		angDif *= angDif;
	}
	else if (angDif < 0.0f)
	{
		angDif *= -angDif;
	}

	if (parentPS && parentPS->speed)
	{
		float maxDif = p_veh->m_pVehicleInfo->turningSpeed * FIGHTER_TURNING_MULTIPLIER;

		if (p_veh->m_pVehicleInfo->speedDependantTurning)
		{
			float speedFrac = 1.0f;
			if (p_veh->m_LandTrace.fraction >= 1.0f
				|| p_veh->m_LandTrace.plane.normal[2] < MIN_LANDING_SLOPE)
			{
				float s = parentPS->speed;
				if (s < 0.0f)
				{
					s = -s;
				}
				speedFrac = (s / (p_veh->m_pVehicleInfo->speedMax * 0.75f));
				if (speedFrac < 0.25f)
				{
					speedFrac = 0.25f;
				}
				else if (speedFrac > 1.0f)
				{
					speedFrac = 1.0f;
				}
			}
			angDif *= speedFrac;
		}
		if (angDif > maxDif)
		{
			angDif = maxDif;
		}
		else if (angDif < -maxDif)
		{
			angDif = -maxDif;
		}
		p_veh->m_vOrientation[PITCH] = AngleNormalize180(p_veh->m_vOrientation[PITCH] - (angDif * (p_veh->m_fTimeModifier * 0.2f)));
	}
}

#else// VEH_CONTROL_SCHEME_4

void FighterYawAdjust(const Vehicle_t* p_veh, const playerState_t* riderPS, const playerState_t* parentPS)
{
	float angDif = AngleSubtract(p_veh->m_vOrientation[YAW], riderPS->viewangles[YAW]);

	if (parentPS && parentPS->speed)
	{
		float s = parentPS->speed;
		const float maxDif = p_veh->m_pVehicleInfo->turningSpeed * 0.8f; //magic number hackery

		if (s < 0.0f)
		{
			s = -s;
		}
		angDif *= s / p_veh->m_pVehicleInfo->speedMax;
		if (angDif > maxDif)
		{
			angDif = maxDif;
		}
		else if (angDif < -maxDif)
		{
			angDif = -maxDif;
		}
		p_veh->m_vOrientation[YAW] = AngleNormalize180(
			p_veh->m_vOrientation[YAW] - angDif * (p_veh->m_fTimeModifier * 0.2f));
	}
}

void FighterPitchAdjust(const Vehicle_t* p_veh, const playerState_t* riderPS, const playerState_t* parentPS)
{
	float angDif = AngleSubtract(p_veh->m_vOrientation[PITCH], riderPS->viewangles[PITCH]);

	if (parentPS && parentPS->speed)
	{
		float s = parentPS->speed;
		const float maxDif = p_veh->m_pVehicleInfo->turningSpeed * 0.8f; //magic number hackery

		if (s < 0.0f)
		{
			s = -s;
		}
		angDif *= s / p_veh->m_pVehicleInfo->speedMax;
		if (angDif > maxDif)
		{
			angDif = maxDif;
		}
		else if (angDif < -maxDif)
		{
			angDif = -maxDif;
		}
		p_veh->m_vOrientation[PITCH] = AngleNormalize360(
			p_veh->m_vOrientation[PITCH] - angDif * (p_veh->m_fTimeModifier * 0.2f));
	}
}
#endif// VEH_CONTROL_SCHEME_4

void FighterPitchClamp(Vehicle_t* p_veh, const playerState_t* riderPS, const playerState_t* parentPS, const int curTime)
{
	if (!BG_UnrestrainedPitchRoll(riderPS, p_veh))
	{
		//cap pitch reasonably
		if (p_veh->m_pVehicleInfo->pitchLimit != -1
			&& !p_veh->m_iRemovedSurfaces
			&& parentPS->electrifyTime < curTime)
		{
			if (p_veh->m_vOrientation[PITCH] > p_veh->m_pVehicleInfo->pitchLimit)
			{
				p_veh->m_vOrientation[PITCH] = p_veh->m_pVehicleInfo->pitchLimit;
			}
			else if (p_veh->m_vOrientation[PITCH] < -p_veh->m_pVehicleInfo->pitchLimit)
			{
				p_veh->m_vOrientation[PITCH] = -p_veh->m_pVehicleInfo->pitchLimit;
			}
		}
	}
}

//MP RULE - ALL PROCESSORIENTCOMMANDS FUNCTIONS MUST BE BG-COMPATIBLE!!!
//If you really need to violate this rule for SP, then use ifdefs.
//By BG-compatible, I mean no use of game-specific data - ONLY use
//stuff available in the MP bgEntity (in SP, the bgEntity is #defined
//as a gentity, but the MP-compatible access restrictions are based
//on the bgEntity structure in the MP codebase) -rww
// ProcessOrientCommands the Vehicle.
static void ProcessOrientCommands(Vehicle_t* p_veh)
{
	/********************************************************************************/
	/*	BEGIN	Here is where make sure the vehicle is properly oriented.	BEGIN	*/
	/********************************************************************************/

	bgEntity_t* parent = p_veh->m_pParentEntity;
	playerState_t * parentPS, *riderPS;
	float angleTimeMod;
#ifdef _GAME
	const float groundFraction = 0.1f;
#endif
	float curRoll;
	qboolean isDead;
	qboolean isLandingOrLanded;
#ifdef _GAME
	int curTime = level.time;
#elif _CGAME
	//FIXME: pass in ucmd?  Not sure if this is reliable...
	int curTime = pm->cmd.serverTime;
#endif

	const bgEntity_t* rider = NULL;
	if (parent->s.owner != ENTITYNUM_NONE)
	{
		rider = PM_BGEntForNum(parent->s.owner); //&g_entities[parent->r.ownerNum];
	}

	if (!rider)
	{
		rider = parent;
	}

	parentPS = parent->playerState;
	riderPS = rider->playerState;
	isDead = (qboolean)((parentPS->eFlags & EF_DEAD) != 0);

#ifdef VEH_CONTROL_SCHEME_4
	if (parentPS->hyperSpaceTime
		&& (curTime - parentPS->hyperSpaceTime) < HYPERSPACE_TIME)
	{//Going to Hyperspace
		//VectorCopy( riderPS->viewangles, p_veh->m_vOrientation );
		//VectorCopy( riderPS->viewangles, parentPS->viewangles );
		VectorCopy(parentPS->viewangles, p_veh->m_vOrientation);
		VectorClear(p_veh->m_vPrevRiderViewAngles);
		p_veh->m_vPrevRiderViewAngles[YAW] = AngleNormalize180(riderPS->viewangles[YAW]);
		FighterPitchClamp(p_veh, riderPS, parentPS, curTime);
		return;
	}
	else if (parentPS->vehTurnaroundIndex
		&& parentPS->vehTurnaroundTime > curTime)
	{//in turn-around brush
		//VectorCopy( riderPS->viewangles, p_veh->m_vOrientation );
		//VectorCopy( riderPS->viewangles, parentPS->viewangles );
		VectorCopy(parentPS->viewangles, p_veh->m_vOrientation);
		VectorClear(p_veh->m_vPrevRiderViewAngles);
		p_veh->m_vPrevRiderViewAngles[YAW] = AngleNormalize180(riderPS->viewangles[YAW]);
		FighterPitchClamp(p_veh, riderPS, parentPS, curTime);
		return;
	}

#else// VEH_CONTROL_SCHEME_4

	if (parentPS->hyperSpaceTime
		&& curTime - parentPS->hyperSpaceTime < HYPERSPACE_TIME)
	{
		//Going to Hyperspace
		VectorCopy(riderPS->viewangles, p_veh->m_vOrientation);
		VectorCopy(riderPS->viewangles, parentPS->viewangles);
		return;
	}
#endif// VEH_CONTROL_SCHEME_4

	if (p_veh->m_iDropTime >= curTime)
	{
		//you can only YAW during this
		parentPS->viewangles[YAW] = p_veh->m_vOrientation[YAW] = riderPS->viewangles[YAW];
#ifdef VEH_CONTROL_SCHEME_4
		VectorClear(p_veh->m_vPrevRiderViewAngles);
		p_veh->m_vPrevRiderViewAngles[YAW] = AngleNormalize180(riderPS->viewangles[YAW]);
#endif// VEH_CONTROL_SCHEME_4
		return;
	}

	angleTimeMod = p_veh->m_fTimeModifier;

	if (isDead || parentPS->electrifyTime >= curTime ||
		p_veh->m_pVehicleInfo->surfDestruction &&
		p_veh->m_iRemovedSurfaces &&
		p_veh->m_iRemovedSurfaces & SHIPSURF_BROKEN_C &&
		p_veh->m_iRemovedSurfaces & SHIPSURF_BROKEN_D &&
		p_veh->m_iRemovedSurfaces & SHIPSURF_BROKEN_E &&
		p_veh->m_iRemovedSurfaces & SHIPSURF_BROKEN_F)
	{
		//do some special stuff for when all the wings are torn off
		FighterDamageRoutine(p_veh, parent, parentPS, riderPS, isDead);
		p_veh->m_vOrientation[ROLL] = AngleNormalize180(p_veh->m_vOrientation[ROLL]);
		return;
	}

	if (!BG_UnrestrainedPitchRoll(riderPS, p_veh))
	{
		p_veh->m_vOrientation[ROLL] = PredictedAngularDecrement(0.95f, angleTimeMod * 2.0f, p_veh->m_vOrientation[ROLL]);
	}

	isLandingOrLanded = FighterIsLanding(p_veh, parentPS) || FighterIsLanded(p_veh, parentPS);

	if (!isLandingOrLanded)
	{
		//don't do this stuff while landed.. I guess. I don't want ships spinning in place, looks silly.
		int m = 0;

		FighterWingMalfunctionCheck(p_veh, parentPS);

		while (m < 3)
		{
			const float aVelDif = p_veh->m_vFullAngleVelocity[m];

			if (aVelDif != 0.0f)
			{
				const float dForVel = aVelDif * 0.1f * p_veh->m_fTimeModifier;
				if (dForVel > 1.0f || dForVel < -1.0f)
				{
					p_veh->m_vOrientation[m] += dForVel;
					p_veh->m_vOrientation[m] = AngleNormalize180(p_veh->m_vOrientation[m]);
					if (m == PITCH)
					{
						//don't pitch downward into ground even more.
						if (p_veh->m_vOrientation[m] > 90.0f && p_veh->m_vOrientation[m] - dForVel < 90.0f)
						{
							p_veh->m_vOrientation[m] = 90.0f;
							p_veh->m_vFullAngleVelocity[m] = -p_veh->m_vFullAngleVelocity[m];
						}
					}
					p_veh->m_vFullAngleVelocity[m] -= dForVel;
				}
				else
				{
					p_veh->m_vFullAngleVelocity[m] = 0.0f;
				}
			}

			m++;
		}
	}
	else
	{
		//clear decr/incr angles once landed.
		VectorClear(p_veh->m_vFullAngleVelocity);
	}

	curRoll = p_veh->m_vOrientation[ROLL];

	// If we're landed, we shouldn't be able to do anything but take off.
	if (isLandingOrLanded //going slow enough to start landing
		&& !p_veh->m_iRemovedSurfaces
		&& parentPS->electrifyTime < curTime) //not spiraling out of control
	{
		if (parentPS->speed > 0.0f)
		{
			//Uh... what?  Why?
			if (p_veh->m_LandTrace.fraction < 0.3f)
			{
				p_veh->m_vOrientation[PITCH] = 0.0f;
			}
			else
			{
				p_veh->m_vOrientation[PITCH] = PredictedAngularDecrement(0.83f, angleTimeMod * 10.0f,
				                                                        p_veh->m_vOrientation[PITCH]);
			}
		}
		if (p_veh->m_LandTrace.fraction > 0.1f
			|| p_veh->m_LandTrace.plane.normal[2] < MIN_LANDING_SLOPE)
		{
			//off the ground, at least (or not on a valid landing surf)
			// Dampen the turn rate based on the current height.
			FighterYawAdjust(p_veh, riderPS, parentPS);
		}
#ifdef VEH_CONTROL_SCHEME_4
		else
		{
			VectorClear(p_veh->m_vPrevRiderViewAngles);
			p_veh->m_vPrevRiderViewAngles[YAW] = AngleNormalize180(riderPS->viewangles[YAW]);
		}
#endif// VEH_CONTROL_SCHEME_4
	}
	else if ((p_veh->m_iRemovedSurfaces || parentPS->electrifyTime >= curTime) //spiralling out of control
		&& (!(p_veh->m_pParentEntity->s.number % 2) || !(p_veh->m_pParentEntity->s.number % 6)))
	{
		//no yaw control
	}
	else if (p_veh->m_pPilot && p_veh->m_pPilot->s.number < MAX_CLIENTS && parentPS->speed > 0.0f)
	//&& !( p_veh->m_ucmd.forwardmove > 0 && p_veh->m_LandTrace.fraction != 1.0f ) )
	{
#ifdef VEH_CONTROL_SCHEME_4
		if (0)
#else// VEH_CONTROL_SCHEME_4
		if (BG_UnrestrainedPitchRoll(riderPS, p_veh))
#endif// VEH_CONTROL_SCHEME_4
		{
			VectorCopy(riderPS->viewangles, p_veh->m_vOrientation);
			VectorCopy(riderPS->viewangles, parentPS->viewangles);

			curRoll = p_veh->m_vOrientation[ROLL];

			FighterNoseMalfunctionCheck(p_veh, parentPS);

			//VectorCopy( p_veh->m_vOrientation, parentPS->viewangles );
		}
		else
		{
			FighterYawAdjust(p_veh, riderPS, parentPS);

			// If we are not hitting the ground, allow the fighter to pitch up and down.
			if (!FighterOverValidLandingSurface(p_veh)
				|| parentPS->speed > MIN_LANDING_SPEED)
			//if ( ( p_veh->m_LandTrace.fraction >= 1.0f || p_veh->m_ucmd.forwardmove != 0 ) && p_veh->m_LandTrace.fraction >= 0.0f )
			{
				float fYawDelta;

				FighterPitchAdjust(p_veh, riderPS, parentPS);
#ifdef VEH_CONTROL_SCHEME_4
				FighterPitchClamp(p_veh, riderPS, parentPS, curTime);
#endif// VEH_CONTROL_SCHEME_4

				FighterNoseMalfunctionCheck(p_veh, parentPS);

				// Adjust the roll based on the turn amount and dampen it a little.
				fYawDelta = AngleSubtract(p_veh->m_vOrientation[YAW], p_veh->m_vPrevOrientation[YAW]);
				//p_veh->m_vOrientation[YAW] - p_veh->m_vPrevOrientation[YAW];
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
#ifdef VEH_CONTROL_SCHEME_4
				if (!BG_UnrestrainedPitchRoll(riderPS, p_veh))
				{
					if (p_veh->m_pVehicleInfo->rollLimit != -1)
					{
						if (curRoll > p_veh->m_pVehicleInfo->rollLimit)
						{
							curRoll = p_veh->m_pVehicleInfo->rollLimit;
						}
						else if (curRoll < -p_veh->m_pVehicleInfo->rollLimit)
						{
							curRoll = -p_veh->m_pVehicleInfo->rollLimit;
						}
					}
				}
#else// VEH_CONTROL_SCHEME_4
				if (p_veh->m_pVehicleInfo->rollLimit != -1)
				{
					if (curRoll > p_veh->m_pVehicleInfo->rollLimit)
					{
						curRoll = p_veh->m_pVehicleInfo->rollLimit;
					}
					else if (curRoll < -p_veh->m_pVehicleInfo->rollLimit)
					{
						curRoll = -p_veh->m_pVehicleInfo->rollLimit;
					}
				}
#endif// VEH_CONTROL_SCHEME_4
			}
		}
	}

	// If you are directly impacting the ground, even out your pitch.
	if (isLandingOrLanded)
	{
		//only if capable of landing
		if (!isDead
			&& parentPS->electrifyTime < curTime
			&& (!p_veh->m_pVehicleInfo->surfDestruction || !p_veh->m_iRemovedSurfaces))
		{
			//not crashing or spiraling out of control...
			if (p_veh->m_vOrientation[PITCH] > 0)
			{
				p_veh->m_vOrientation[PITCH] = PredictedAngularDecrement(0.2f, angleTimeMod * 10.0f,
				                                                        p_veh->m_vOrientation[PITCH]);
			}
			else
			{
				p_veh->m_vOrientation[PITCH] = PredictedAngularDecrement(0.75f, angleTimeMod * 10.0f,
				                                                        p_veh->m_vOrientation[PITCH]);
			}
		}
	}
	// If no one is in this vehicle and it's up in the sky, pitch it forward as it comes tumbling down.
#ifdef _GAME //never gonna happen on client anyway, we can't be getting predicted unless the predicting client is boarded



	if (!p_veh->m_pVehicleInfo->Inhabited(p_veh)
		&& p_veh->m_LandTrace.fraction >= groundFraction
		&& !FighterIsInSpace((gentity_t*)parent)
		&& !FighterSuspended(p_veh, parentPS))
	{
		p_veh->m_ucmd.upmove = 0;
		//p_veh->m_ucmd.forwardmove = 0;
		p_veh->m_vOrientation[PITCH] += p_veh->m_fTimeModifier;
		if (!BG_UnrestrainedPitchRoll(riderPS, p_veh))
		{
			if (p_veh->m_vOrientation[PITCH] > 60.0f)
			{
				p_veh->m_vOrientation[PITCH] = 60.0f;
			}
		}
	}
#endif

	if (!parentPS->hackingTime)
	{
		//use that roll
		p_veh->m_vOrientation[ROLL] = curRoll;
		//NOTE: this seems really backwards...
		if (p_veh->m_vOrientation[ROLL])
		{
			//continually adjust the yaw based on the roll..
			if ((p_veh->m_iRemovedSurfaces || parentPS->electrifyTime >= curTime) //spiralling out of control
				&& (!(p_veh->m_pParentEntity->s.number % 2) || !(p_veh->m_pParentEntity->s.number % 6)))
			{
				//leave YAW alone
			}
			else
			{
				if (!BG_UnrestrainedPitchRoll(riderPS, p_veh))
				{
					p_veh->m_vOrientation[YAW] -= p_veh->m_vOrientation[ROLL] * 0.05f * p_veh->m_fTimeModifier;
				}
			}
		}
	}
	else
	{
		//add in strafing roll
		const float strafeRoll = parentPS->hackingTime / MAX_STRAFE_TIME * p_veh->m_pVehicleInfo->rollLimit;
		//p_veh->m_pVehicleInfo->bankingSpeed*
		const float strafeDif = AngleSubtract(strafeRoll, p_veh->m_vOrientation[ROLL]);
		p_veh->m_vOrientation[ROLL] += strafeDif * 0.1f * p_veh->m_fTimeModifier;
		if (!BG_UnrestrainedPitchRoll(riderPS, p_veh))
		{
			//cap it reasonably
			if (p_veh->m_pVehicleInfo->rollLimit != -1
				&& !p_veh->m_iRemovedSurfaces
				&& parentPS->electrifyTime < curTime)
			{
				if (p_veh->m_vOrientation[ROLL] > p_veh->m_pVehicleInfo->rollLimit)
				{
					p_veh->m_vOrientation[ROLL] = p_veh->m_pVehicleInfo->rollLimit;
				}
				else if (p_veh->m_vOrientation[ROLL] < -p_veh->m_pVehicleInfo->rollLimit)
				{
					p_veh->m_vOrientation[ROLL] = -p_veh->m_pVehicleInfo->rollLimit;
				}
			}
		}
	}

	if (p_veh->m_pVehicleInfo->surfDestruction)
	{
		FighterDamageRoutine(p_veh, parent, parentPS, riderPS, isDead);
	}
	p_veh->m_vOrientation[ROLL] = AngleNormalize180(p_veh->m_vOrientation[ROLL]);

	/********************************************************************************/
	/*	END	Here is where make sure the vehicle is properly oriented.	END			*/
	/********************************************************************************/
}

#ifdef _GAME //ONLY on server, not cgame

// This function makes sure that the vehicle is properly animated.
static void AnimateVehicle(Vehicle_t* p_veh)
{
	int Anim = -1;
	const playerState_t* parentPS = p_veh->m_pParentEntity->playerState;
	const int curTime = level.time;

	if (parentPS->hyperSpaceTime
		&& curTime - parentPS->hyperSpaceTime < HYPERSPACE_TIME)
	{
		//Going to Hyperspace
		//close the wings (FIXME: makes sense on X-Wing, not Shuttle?)
		if (p_veh->m_ulFlags & VEH_WINGSOPEN)
		{
			p_veh->m_ulFlags &= ~VEH_WINGSOPEN;
			Anim = BOTH_WINGS_CLOSE;
		}
	}
	else
	{
		const qboolean isLanding = FighterIsLanding(p_veh, parentPS);
		const qboolean isLanded = FighterIsLanded(p_veh, parentPS);

		// if we're above launch height (way up in the air)...
		if (!isLanding && !isLanded)
		{
			if (!(p_veh->m_ulFlags & VEH_WINGSOPEN))
			{
				p_veh->m_ulFlags |= VEH_WINGSOPEN;
				p_veh->m_ulFlags &= ~VEH_GEARSOPEN;
				Anim = BOTH_WINGS_OPEN;
			}
		}
		// otherwise we're below launch height and still taking off.
		else
		{
			if ((p_veh->m_ucmd.forwardmove < 0 || p_veh->m_ucmd.upmove < 0 || isLanded)
				&& p_veh->m_LandTrace.fraction <= 0.4f
				&& p_veh->m_LandTrace.plane.normal[2] >= MIN_LANDING_SLOPE)
			{
				//already landed or trying to land and close to ground
				// Open gears.
				if (!(p_veh->m_ulFlags & VEH_GEARSOPEN))
				{
					if (p_veh->m_pVehicleInfo->soundLand)
					{
						//just landed?
						G_EntitySound((gentity_t*)p_veh->m_pParentEntity, CHAN_AUTO, p_veh->m_pVehicleInfo->soundLand);
					}
					p_veh->m_ulFlags |= VEH_GEARSOPEN;
					Anim = BOTH_GEARS_OPEN;
				}
			}
			else
			{
				//trying to take off and almost halfway off the ground
				// Close gears (if they're open).
				if (p_veh->m_ulFlags & VEH_GEARSOPEN)
				{
					p_veh->m_ulFlags &= ~VEH_GEARSOPEN;
					Anim = BOTH_GEARS_CLOSE;
					//iFlags = SETANIM_FLAG_OVERRIDE | SETANIM_FLAG_HOLD;
				}
				// If gears are closed, and we are below launch height, close the wings.
				else
				{
					if (p_veh->m_ulFlags & VEH_WINGSOPEN)
					{
						p_veh->m_ulFlags &= ~VEH_WINGSOPEN;
						Anim = BOTH_WINGS_CLOSE;
					}
				}
			}
		}
	}

	if (Anim != -1)
	{
		const int iFlags = SETANIM_FLAG_NORMAL;
		BG_SetAnim(p_veh->m_pParentEntity->playerState, bgAllAnims[p_veh->m_pParentEntity->localAnimIndex].anims,
		           SETANIM_BOTH, Anim, iFlags);
	}
}

// This function makes sure that the rider's in this vehicle are properly animated.
static void AnimateRiders(Vehicle_t* p_veh)
{
}

#endif //game-only

#ifdef _CGAME
void AttachRidersGeneric(const Vehicle_t* p_veh);
#endif

void G_SetFighterVehicleFunctions(vehicleInfo_t* pVehInfo)
{
#ifdef _GAME //ONLY in SP or on server, not cgame
	pVehInfo->AnimateVehicle = AnimateVehicle;
	pVehInfo->AnimateRiders = AnimateRiders;
	//	pVehInfo->ValidateBoard				=		ValidateBoard;
	//	pVehInfo->SetParent					=		SetParent;
	//	pVehInfo->SetPilot					=		SetPilot;
	//	pVehInfo->AddPassenger				=		AddPassenger;
	//	pVehInfo->Animate					=		Animate;
	pVehInfo->Board = Board;
	pVehInfo->Eject = Eject;
	//	pVehInfo->EjectAll					=		EjectAll;
	//	pVehInfo->StartDeathDelay			=		StartDeathDelay;
	//	pVehInfo->DeathUpdate				=		DeathUpdate;
	//	pVehInfo->RegisterAssets			=		RegisterAssets;
	//	pVehInfo->Initialize				=		Initialize;
	pVehInfo->Update = Update;
	//	pVehInfo->UpdateRider				=		UpdateRider;
#endif //game-only
	pVehInfo->ProcessMoveCommands = ProcessMoveCommands;
	pVehInfo->ProcessOrientCommands = ProcessOrientCommands;

#ifdef _CGAME //cgame prediction attachment func
	pVehInfo->AttachRiders = AttachRidersGeneric;
#endif
	//	pVehInfo->AttachRiders				=		AttachRiders;
	//	pVehInfo->Ghost						=		Ghost;
	//	pVehInfo->UnGhost					=		UnGhost;
	//	pVehInfo->Inhabited					=		Inhabited;
}

// Following is only in game, not in namespace
#ifdef _GAME
extern void G_AllocateVehicleObject(Vehicle_t** p_veh);
#endif

// Create/Allocate a new Animal Vehicle (initializing it as well).
void G_CreateFighterNPC(Vehicle_t** p_veh, const char* strType)
{
	// Allocate the Vehicle.
#ifdef _GAME
	//these will remain on entities on the client once allocated because the pointer is
	//never stomped. on the server, however, when an ent is freed, the entity struct is
	//memset to 0, so this memory would be lost..
	G_AllocateVehicleObject(p_veh);
#else
	if (!*p_veh)
	{
		//only allocate a new one if we really have to
		*p_veh = (Vehicle_t*)BG_Alloc(sizeof(Vehicle_t));
	}
#endif
	memset(*p_veh, 0, sizeof(Vehicle_t));
	(*p_veh)->m_pVehicleInfo = &g_vehicleInfo[BG_VehicleGetIndex(strType)];
}
