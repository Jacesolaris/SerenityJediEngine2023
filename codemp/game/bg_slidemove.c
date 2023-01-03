/*
===========================================================================
Copyright (C) 1999 - 2005, Id Software, Inc.
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

// bg_slidemove.c -- part of bg_pmove functionality
// game and cgame, NOT ui

#include "qcommon/q_shared.h"
#include "bg_public.h"
#include "bg_local.h"

#ifdef _GAME
#include "g_local.h"
#elif _CGAME
#include "cgame/cg_local.h"
#elif UI_BUILD
#include "ui/ui_local.h"
#endif

/*

input: origin, velocity, bounds, groundPlane, trace function

output: origin, velocity, impacts, stairup boolean

*/

//do vehicle impact stuff
// slight rearrangement by BTO (VV) so that we only have one namespace include
#ifdef _GAME
extern void G_FlyVehicleSurfaceDestruction(gentity_t* veh, trace_t* trace, int magnitude, qboolean force); //g_vehicle.c
extern qboolean G_CanBeEnemy(gentity_t* self, gentity_t* enemy); //w_saber.c
#endif

extern qboolean BG_UnrestrainedPitchRoll(const playerState_t* ps, Vehicle_t* p_veh);

extern bgEntity_t* pm_entSelf;
extern bgEntity_t* pm_entVeh;

//vehicle impact stuff continued...
#ifdef _GAME
extern qboolean FighterIsLanded(Vehicle_t* p_veh, playerState_t* parentPS);
extern void G_DamageFromKiller(gentity_t* pEnt, gentity_t* pVehEnt, gentity_t* attacker, vec3_t org, int damage,
                               int dflags, int mod);
#endif

#define MAX_IMPACT_TURN_ANGLE 45.0f

void PM_VehicleImpact(bgEntity_t* pEnt, trace_t* trace)
{
	// See if the vehicle has crashed into the ground.
	Vehicle_t* pSelfVeh = pEnt->m_pVehicle;
	float magnitude = VectorLength(pm->ps->velocity) * pSelfVeh->m_pVehicleInfo->mass / 50.0f;
	qboolean forceSurfDestruction = qfalse;
#ifdef _GAME
	gentity_t* hitEnt = trace != NULL ? &g_entities[trace->entity_num] : NULL;

	if (!hitEnt ||
		pSelfVeh && pSelfVeh->m_pPilot &&
		hitEnt && hitEnt->s.eType == ET_MISSILE && hitEnt->inuse &&
		hitEnt->r.ownerNum == pSelfVeh->m_pPilot->s.number
	)
	{
		return;
	}

	if (pSelfVeh //I have a vehicle struct
		&& pSelfVeh->m_iRemovedSurfaces) //vehicle has bits removed
	{
		//spiralling to our deaths, explode on any solid impact
		if (hitEnt->s.NPC_class == CLASS_VEHICLE)
		{
			//hit another vehicle, explode!
			//Give credit to whoever got me into this death spiral state
			G_DamageFromKiller((gentity_t*)pEnt, (gentity_t*)pSelfVeh->m_pParentEntity, hitEnt, pm->ps->origin, 999999,
			                   DAMAGE_NO_ARMOR, MOD_COLLISION);
			return;
		}
		if (!VectorCompare(trace->plane.normal, vec3_origin)
			&& (trace->entity_num == ENTITYNUM_WORLD || hitEnt->r.bmodel))
		{
			//have a valid hit plane and we hit a solid brush
			vec3_t moveDir;
			VectorCopy(pm->ps->velocity, moveDir);
			VectorNormalize(moveDir);
			const float impactDot = DotProduct(moveDir, trace->plane.normal);
			if (impactDot <= -0.7f) //hit rather head-on and hard
			{
				// Just DIE now
				G_DamageFromKiller((gentity_t*)pEnt, (gentity_t*)pSelfVeh->m_pParentEntity, hitEnt, pm->ps->origin,
				                   999999, DAMAGE_NO_ARMOR, MOD_FALLING);
				return;
			}
		}
	}

	if (trace->entity_num < ENTITYNUM_WORLD
		&& hitEnt->s.eType == ET_MOVER
		&& hitEnt->s.apos.trType != TR_STATIONARY //rotating
		&& hitEnt->spawnflags & 16 //IMPACT
		&& Q_stricmp("func_rotating", hitEnt->classname) == 0)
	{
		//hit a func_rotating that is supposed to destroy anything it touches!
		//guarantee the hit will happen, thereby taking off a piece of the ship
		forceSurfDestruction = qtrue;
	}
	else if (fabs(pm->ps->velocity[0]) + fabs(pm->ps->velocity[1]) < 100.0f
		&& pm->ps->velocity[2] > -100.0f)
#elif defined(_CGAME)
	if (fabs(pm->ps->velocity[0]) + fabs(pm->ps->velocity[1]) < 100.0f
		&& pm->ps->velocity[2] > -100.0f)
#endif
	{
		//we're landing, we're cool
#ifdef _GAME
		if (hitEnt && (hitEnt->s.eType == ET_PLAYER || hitEnt->s.eType == ET_NPC) && pSelfVeh->m_pVehicleInfo->type ==
			VH_FIGHTER)
		{
			//always smack players
		}
		else
#endif
		{
			return;
		}
	}
	if (pSelfVeh &&
		(pSelfVeh->m_pVehicleInfo->type == VH_SPEEDER || pSelfVeh->m_pVehicleInfo->type == VH_FIGHTER) &&
		//this is kind of weird on tauntauns and atst's..
		(magnitude >= 100 || forceSurfDestruction))
	{
		if (pEnt->m_pVehicle->m_iHitDebounce < pm->cmd.serverTime
			|| forceSurfDestruction)
		{
			//a bit of a hack, may conflict with getting shot, but...
			//FIXME: impact sound and effect should be gotten from g_vehicleInfo...?
			//FIXME: should pass in trace.endpos and trace.plane.normal
#ifdef _CGAME
			bgEntity_t* hitEnt;
#endif
			vec3_t vehUp;

#ifdef _GAME
			qboolean noDamage = qfalse;
#endif

			if (trace && !pSelfVeh->m_iRemovedSurfaces && !forceSurfDestruction)
			{
				qboolean turnFromImpact = qfalse, turnHitEnt = qfalse;
				float l = pm->ps->speed * 0.5f;
				vec3_t bounceDir;
#ifdef _CGAME
				bgEntity_t* hitEnt = PM_BGEntForNum(trace->entity_num);
#endif
				if ((trace->entity_num == ENTITYNUM_WORLD || hitEnt->s.solid == SOLID_BMODEL) //bounce off any brush
					&& !VectorCompare(trace->plane.normal, vec3_origin)) //have a valid plane to bounce off of
				{
					//bounce off in the opposite direction of the impact
					if (pSelfVeh->m_pVehicleInfo->type == VH_SPEEDER)
					{
						pm->ps->speed *= pml.frametime;
						VectorCopy(trace->plane.normal, bounceDir);
					}
					else if (trace->plane.normal[2] >= MIN_LANDING_SLOPE //flat enough to land on
						&& pSelfVeh->m_LandTrace.fraction < 1.0f //ground present
						&& pm->ps->speed <= MIN_LANDING_SPEED)
					{
						//could land here, don't bounce off, in fact, return altogether!
						return;
					}
					else
					{
						if (pSelfVeh->m_pVehicleInfo->type == VH_FIGHTER)
						{
							turnFromImpact = qtrue;
						}
						VectorCopy(trace->plane.normal, bounceDir);
					}
				}
				else if (pSelfVeh->m_pVehicleInfo->type == VH_FIGHTER)
				{
					//check for impact with another fighter
#ifdef _CGAME
					bgEntity_t* hitEnt = PM_BGEntForNum(trace->entity_num);
#endif
					if (hitEnt->s.NPC_class == CLASS_VEHICLE
						&& hitEnt->m_pVehicle
						&& hitEnt->m_pVehicle->m_pVehicleInfo
						&& hitEnt->m_pVehicle->m_pVehicleInfo->type == VH_FIGHTER)
					{
						//two vehicles hit each other, turn away from the impact
						turnFromImpact = qtrue;
						turnHitEnt = qtrue;
#ifdef _GAME
						VectorSubtract(pm->ps->origin, hitEnt->r.currentOrigin, bounceDir);
#else
						VectorSubtract(pm->ps->origin, hitEnt->s.origin, bounceDir);
#endif
						VectorNormalize(bounceDir);
					}
				}
				if (turnFromImpact)
				{
					//bounce off impact surf and turn away
					vec3_t push_dir = {0}, turnAwayAngles, turnDelta;
					vec3_t moveDir;
					//bounce
					if (!turnHitEnt)
					{
						//hit wall
						VectorScale(bounceDir, pm->ps->speed * 0.25f / pSelfVeh->m_pVehicleInfo->mass, push_dir);
					}
					else
					{
						//hit another fighter
#ifdef _GAME
						if (hitEnt->client)
						{
							VectorScale(bounceDir, (pm->ps->speed + hitEnt->client->ps.speed) * 0.5f, push_dir);
						}
						else
						{
							VectorScale(bounceDir, (pm->ps->speed + hitEnt->s.speed) * 0.5f, push_dir);
						}
#else
						VectorScale(bounceDir, (pm->ps->speed + hitEnt->s.speed) * 0.5f, bounceDir);
#endif
						VectorScale(push_dir, l / pSelfVeh->m_pVehicleInfo->mass, push_dir);
						VectorScale(push_dir, 0.1f, push_dir);
					}
					VectorNormalize2(pm->ps->velocity, moveDir);
					float bounceDot = DotProduct(moveDir, bounceDir) * -1;
					if (bounceDot < 0.1f)
					{
						bounceDot = 0.1f;
					}
					VectorScale(push_dir, bounceDot, push_dir);
					VectorAdd(pm->ps->velocity, push_dir, pm->ps->velocity);
					//turn
					float turnDivider = pSelfVeh->m_pVehicleInfo->mass / 400.0f;
					if (turnHitEnt)
					{
						//don't turn as much when hit another ship
						turnDivider *= 4.0f;
					}
					if (turnDivider < 0.5f)
					{
						turnDivider = 0.5f;
					}
					float turnStrength = magnitude / 2000.0f;
					if (turnStrength < 0.1f)
					{
						turnStrength = 0.1f;
					}
					else if (turnStrength > 2.0f)
					{
						turnStrength = 2.0f;
					}
					//get the angles we are going to turn towards
					vectoangles(bounceDir, turnAwayAngles);
					//get the delta from our current angles to those new angles
					AnglesSubtract(turnAwayAngles, pSelfVeh->m_vOrientation, turnDelta);
					//now do pitch
					if (!bounceDir[2])
					{
						//shouldn't be any pitch
					}
					else
					{
						float pitchTurnStrength = turnStrength * turnDelta[PITCH];
						if (pitchTurnStrength > MAX_IMPACT_TURN_ANGLE)
						{
							pitchTurnStrength = MAX_IMPACT_TURN_ANGLE;
						}
						else if (pitchTurnStrength < -MAX_IMPACT_TURN_ANGLE)
						{
							pitchTurnStrength = -MAX_IMPACT_TURN_ANGLE;
						}
						pSelfVeh->m_vFullAngleVelocity[PITCH] = AngleNormalize180(
							pSelfVeh->m_vOrientation[PITCH] + pitchTurnStrength / turnDivider * pSelfVeh->
							m_fTimeModifier);
					}
					//now do yaw
					if (!bounceDir[0]
						&& !bounceDir[1])
					{
						//shouldn't be any yaw
					}
					else
					{
						float yawTurnStrength = turnStrength * turnDelta[YAW];
						if (yawTurnStrength > MAX_IMPACT_TURN_ANGLE)
						{
							yawTurnStrength = MAX_IMPACT_TURN_ANGLE;
						}
						else if (yawTurnStrength < -MAX_IMPACT_TURN_ANGLE)
						{
							yawTurnStrength = -MAX_IMPACT_TURN_ANGLE;
						}
						pSelfVeh->m_vFullAngleVelocity[ROLL] = AngleNormalize180(
							pSelfVeh->m_vOrientation[ROLL] - yawTurnStrength / turnDivider * pSelfVeh->m_fTimeModifier);
					}
#ifdef _GAME//server-side, turn the guy we hit away from us, too
					if (turnHitEnt //make the other guy turn and get pushed
						&& hitEnt->client //must be a valid client
						&& !FighterIsLanded(hitEnt->m_pVehicle, &hitEnt->client->ps) //but not if landed
						&& !(hitEnt->spawnflags & 2)) //and not if suspended
					{
						l = hitEnt->client->ps.speed;
						//now bounce *them* away and turn them
						//flip the bounceDir
						VectorScale(bounceDir, -1, bounceDir);
						//do bounce
						VectorScale(bounceDir, (pm->ps->speed + l) * 0.5f, push_dir);
						VectorScale(push_dir, l * 0.5f / hitEnt->m_pVehicle->m_pVehicleInfo->mass, push_dir);
						VectorNormalize2(hitEnt->client->ps.velocity, moveDir);
						bounceDot = DotProduct(moveDir, bounceDir) * -1;
						if (bounceDot < 0.1f)
						{
							bounceDot = 0.1f;
						}
						VectorScale(push_dir, bounceDot, push_dir);
						VectorAdd(hitEnt->client->ps.velocity, push_dir, hitEnt->client->ps.velocity);
						//turn
						turnDivider = hitEnt->m_pVehicle->m_pVehicleInfo->mass / 400.0f;
						if (turnHitEnt)
						{
							//don't turn as much when hit another ship
							turnDivider *= 4.0f;
						}
						if (turnDivider < 0.5f)
						{
							turnDivider = 0.5f;
						}
						//get the angles we are going to turn towards
						vectoangles(bounceDir, turnAwayAngles);
						//get the delta from our current angles to those new angles
						AnglesSubtract(turnAwayAngles, hitEnt->m_pVehicle->m_vOrientation, turnDelta);
						//now do pitch
						if (!bounceDir[2])
						{
							//shouldn't be any pitch
						}
						else
						{
							float pitchTurnStrength = turnStrength * turnDelta[PITCH];
							if (pitchTurnStrength > MAX_IMPACT_TURN_ANGLE)
							{
								pitchTurnStrength = MAX_IMPACT_TURN_ANGLE;
							}
							else if (pitchTurnStrength < -MAX_IMPACT_TURN_ANGLE)
							{
								pitchTurnStrength = -MAX_IMPACT_TURN_ANGLE;
							}
							hitEnt->m_pVehicle->m_vFullAngleVelocity[PITCH] = AngleNormalize180(
								hitEnt->m_pVehicle->m_vOrientation[PITCH] + pitchTurnStrength / turnDivider * pSelfVeh->
								m_fTimeModifier);
						}
						//now do yaw
						if (!bounceDir[0]
							&& !bounceDir[1])
						{
							//shouldn't be any yaw
						}
						else
						{
							float yawTurnStrength = turnStrength * turnDelta[YAW];
							if (yawTurnStrength > MAX_IMPACT_TURN_ANGLE)
							{
								yawTurnStrength = MAX_IMPACT_TURN_ANGLE;
							}
							else if (yawTurnStrength < -MAX_IMPACT_TURN_ANGLE)
							{
								yawTurnStrength = -MAX_IMPACT_TURN_ANGLE;
							}
							hitEnt->m_pVehicle->m_vFullAngleVelocity[ROLL] = AngleNormalize180(
								hitEnt->m_pVehicle->m_vOrientation[ROLL] - yawTurnStrength / turnDivider * pSelfVeh->
								m_fTimeModifier);
						}
					}
#endif
				}
			}

#ifdef _GAME
			if (!hitEnt)
			{
				return;
			}

			AngleVectors(pSelfVeh->m_vOrientation, NULL, NULL, vehUp);
			if (pSelfVeh->m_pVehicleInfo->iImpactFX)
			{
				G_AddEvent((gentity_t*)pEnt, EV_PLAY_EFFECT_ID, pSelfVeh->m_pVehicleInfo->iImpactFX);
			}
			pEnt->m_pVehicle->m_iHitDebounce = pm->cmd.serverTime + 200;
			magnitude /= pSelfVeh->m_pVehicleInfo->toughness * 50.0f;

			if (hitEnt && (hitEnt->s.eType != ET_TERRAIN || !(hitEnt->spawnflags & 1) || pSelfVeh->m_pVehicleInfo->type
				== VH_FIGHTER))
			{
				//don't damage the vehicle from terrain that doesn't want to damage vehicles
				gentity_t* killer = NULL;

				if (pSelfVeh->m_pVehicleInfo->type == VH_FIGHTER)
				{
					//increase the damage...
					float mult = pSelfVeh->m_vOrientation[PITCH] * 0.1f;
					if (mult < 1.0f)
					{
						mult = 1.0f;
					}
					if (hitEnt->inuse && hitEnt->takedamage)
					{
						//if the other guy takes damage, don't hurt us a lot for ramming him
						//unless it's a vehicle, then we get 1.5 times damage
						if (hitEnt->s.eType == ET_NPC &&
							hitEnt->s.NPC_class == CLASS_VEHICLE &&
							hitEnt->m_pVehicle)
						{
							mult = 1.5f;
						}
						else
						{
							mult = 0.5f;
						}
					}

					magnitude *= mult;
				}
				pSelfVeh->m_iLastImpactDmg = magnitude;
				//FIXME: what about proper death credit to the guy who shot you down?
				//FIXME: actually damage part of the ship that impacted?
				if (hitEnt->s.eType == ET_MISSILE) //missile
				{
					//FIX: NEVER do or take impact damage from a missile...
					noDamage = qtrue;
					if (hitEnt->s.eFlags & EF_JETPACK_ACTIVE && hitEnt->r.ownerNum < MAX_CLIENTS) //valid client owner
					{
						//I ran into a missile and died because of the impact, give credit to the missile's owner (PROBLEM: might this ever accidentally give improper credit to client 0?)
						killer = &g_entities[hitEnt->r.ownerNum];
					}
				}
				if (!noDamage)
				{
					G_Damage((gentity_t*)pEnt, hitEnt, killer != NULL ? killer : hitEnt, NULL, pm->ps->origin,
					         magnitude * 5, DAMAGE_NO_ARMOR,
					         hitEnt->s.NPC_class == CLASS_VEHICLE ? MOD_COLLISION : MOD_FALLING);
				}

				if (pSelfVeh->m_pVehicleInfo->surfDestruction)
				{
					G_FlyVehicleSurfaceDestruction((gentity_t*)pEnt, trace, magnitude, forceSurfDestruction);
				}

				pSelfVeh->m_ulFlags |= VEH_CRASHING;
			}

			if (hitEnt &&
				hitEnt->inuse &&
				hitEnt->takedamage)
			{
				//damage this guy because we hit him
				float pmult = 1.0f;
				gentity_t* attackEnt;

				if (hitEnt->s.eType == ET_PLAYER && hitEnt->s.number < MAX_CLIENTS ||
					hitEnt->s.eType == ET_NPC && hitEnt->s.NPC_class != CLASS_VEHICLE)
				{
					//probably a humanoid, or something
					if (pSelfVeh->m_pVehicleInfo->type == VH_FIGHTER)
					{
						//player die good.. if me fighter
						pmult = 2000.0f;
					}
					else
					{
						pmult = 40.0f;
					}

					if (hitEnt->client &&
						BG_KnockDownable(&hitEnt->client->ps) &&
						G_CanBeEnemy((gentity_t*)pEnt, hitEnt))
					{
						//smash!
						if (hitEnt->client->ps.forceHandExtend != HANDEXTEND_KNOCKDOWN)
						{
							hitEnt->client->ps.forceHandExtend = HANDEXTEND_KNOCKDOWN;
							hitEnt->client->ps.forceHandExtendTime = pm->cmd.serverTime + 1100;
							hitEnt->client->ps.forceDodgeAnim = 0;
							//this toggles between 1 and 0, when it's 1 we should play the get up anim
						}

						hitEnt->client->ps.otherKiller = pEnt->s.number;
						hitEnt->client->ps.otherKillerTime = pm->cmd.serverTime + 5000;
						hitEnt->client->ps.otherKillerDebounceTime = pm->cmd.serverTime + 100;
						hitEnt->client->otherKillerMOD = MOD_COLLISION;
						hitEnt->client->otherKillerVehWeapon = 0;
						hitEnt->client->otherKillerWeaponType = WP_NONE;

						//add my velocity into his to force him along in the correct direction from impact
						VectorAdd(hitEnt->client->ps.velocity, pm->ps->velocity, hitEnt->client->ps.velocity);
						//upward thrust
						hitEnt->client->ps.velocity[2] += 200.0f;
					}
				}

				if (pSelfVeh->m_pPilot)
				{
					attackEnt = (gentity_t*)pSelfVeh->m_pPilot;
				}
				else
				{
					attackEnt = (gentity_t*)pEnt;
				}

				int finalD = magnitude * pmult;
				if (finalD < 1)
				{
					finalD = 1;
				}
				if (!noDamage)
				{
					G_Damage(hitEnt, attackEnt, attackEnt, NULL, pm->ps->origin, finalD, 0,
					         hitEnt->s.NPC_class == CLASS_VEHICLE ? MOD_COLLISION : MOD_FALLING);
				}
			}
#else	//this is gonna result in "double effects" for the client doing the prediction.
			//it doesn't look bad though. could just use predicted events, but I'm too lazy.
			hitEnt = PM_BGEntForNum(trace->entity_num);

			if (!hitEnt || hitEnt->s.owner != pEnt->s.number)
			{
				//don't hit your own missiles!
				AngleVectors(pSelfVeh->m_vOrientation, NULL, NULL, vehUp);
				pEnt->m_pVehicle->m_iHitDebounce = pm->cmd.serverTime + 200;
				trap->FX_PlayEffectID(pSelfVeh->m_pVehicleInfo->iImpactFX, pm->ps->origin, vehUp, -1, -1, qfalse);

				pSelfVeh->m_ulFlags |= VEH_CRASHING;
			}
#endif
		}
	}
}

qboolean PM_GroundSlideOkay(const float z_normal)
{
	if (z_normal > 0)
	{
		if (pm->ps->velocity[2] > 0)
		{
			if (pm->ps->legsAnim == BOTH_WALL_RUN_RIGHT
				|| pm->ps->legsAnim == BOTH_WALL_RUN_LEFT
				|| pm->ps->legsAnim == BOTH_WALL_RUN_RIGHT_STOP
				|| pm->ps->legsAnim == BOTH_WALL_RUN_LEFT_STOP
				|| pm->ps->legsAnim == BOTH_FORCEWALLRUNFLIP_START
				|| pm->ps->legsAnim == BOTH_FORCELONGLEAP_START
				|| pm->ps->legsAnim == BOTH_FORCELONGLEAP_ATTACK
				|| pm->ps->legsAnim == BOTH_FORCELONGLEAP_ATTACK2
				|| pm->ps->legsAnim == BOTH_FORCELONGLEAP_LAND
				|| pm->ps->legsAnim == BOTH_FORCELONGLEAP_LAND2
				|| PM_InReboundJump(pm->ps->legsAnim))
			{
				return qfalse;
			}
		}
	}
	return qtrue;
}

/*
===============
qboolean PM_ClientImpact( trace_t *trace, qboolean damageSelf )

===============
*/
#ifdef _GAME
extern void Client_CheckImpactBBrush(gentity_t* self, gentity_t* other);
extern qboolean PM_CheckGrabWall(trace_t* trace);

qboolean PM_ClientImpact(const trace_t* trace, qboolean damageSelf)
{
	const int otherEntityNum = trace->entity_num;

	if (!pm_entSelf)
	{
		return qfalse;
	}

	if (otherEntityNum >= ENTITYNUM_WORLD)
	{
		return qfalse;
	}

	const gentity_t* traceEnt = &g_entities[otherEntityNum];

	if (VectorLength(pm->ps->velocity) >= 100 && pm_entSelf->s.NPC_class != CLASS_VEHICLE && pm->ps->lastOnGround + 100
		< level.time)
	{
		Client_CheckImpactBBrush((gentity_t*)pm_entSelf, &g_entities[otherEntityNum]);
		//DoImpact((gentity_t*)(pm_entSelf), &g_entities[otherEntityNum], damageSelf, trace);
	}

	if (!traceEnt
		|| !(traceEnt->r.contents & pm->tracemask))
	{
		//it's dead or not in my way anymore, don't clip against it
		return qtrue;
	}

	return qfalse;
}
#endif

/*
==================
PM_SlideMove

Returns qtrue if the velocity was clipped in some way
==================
*/
#define	MAX_CLIP_PLANES	5

qboolean PM_SlideMove(const qboolean gravity)
{
	int bumpcount;
	int numplanes;
	vec3_t normal, planes[MAX_CLIP_PLANES];
	vec3_t primal_velocity;
	int i;
	trace_t trace;
	vec3_t end_velocity;
#ifdef _GAME
	const qboolean damageSelf = qtrue;
#endif
	const int numbumps = 4;

	VectorCopy(pm->ps->velocity, primal_velocity);
	VectorCopy(pm->ps->velocity, end_velocity);

	if (gravity)
	{
		end_velocity[2] -= pm->ps->gravity * pml.frametime;
		pm->ps->velocity[2] = (pm->ps->velocity[2] + end_velocity[2]) * 0.5;
		primal_velocity[2] = end_velocity[2];
		if (pml.groundPlane)
		{
			if (PM_GroundSlideOkay(pml.groundTrace.plane.normal[2]))
			{
				// slide along the ground plane
				PM_ClipVelocity(pm->ps->velocity, pml.groundTrace.plane.normal,
				                pm->ps->velocity, OVERCLIP);
			}
		}
	}

	float time_left = pml.frametime;

	// never turn against the ground plane
	if (pml.groundPlane)
	{
		numplanes = 1;
		VectorCopy(pml.groundTrace.plane.normal, planes[0]);
		if (!PM_GroundSlideOkay(planes[0][2]))
		{
			planes[0][2] = 0;
			VectorNormalize(planes[0]);
		}
	}
	else
	{
		numplanes = 0;
	}

	// never turn against original velocity
	VectorNormalize2(pm->ps->velocity, planes[numplanes]);
	numplanes++;

	for (bumpcount = 0; bumpcount < numbumps; bumpcount++)
	{
		vec3_t end;
		// calculate position we are trying to move to
		VectorMA(pm->ps->origin, time_left, pm->ps->velocity, end);

		// see if we can make it there
		pm->trace(&trace, pm->ps->origin, pm->mins, pm->maxs, end, pm->ps->client_num, pm->tracemask);

		if (trace.allsolid)
		{
			// entity is completely trapped in another solid
			pm->ps->velocity[2] = 0; // don't build up falling damage, but allow sideways acceleration
			return qtrue;
		}

		if (trace.fraction > 0)
		{
			// actually covered some distance
			VectorCopy(trace.endpos, pm->ps->origin);
		}

		if (trace.fraction == 1)
		{
			break; // moved the entire distance
		}

		// save entity for contact
		PM_AddTouchEnt(trace.entity_num);

		if (pm->ps->client_num >= MAX_CLIENTS)
		{
			bgEntity_t* p_ent = pm_entSelf;

			if (p_ent && p_ent->s.eType == ET_NPC && p_ent->s.NPC_class == CLASS_VEHICLE &&
				p_ent->m_pVehicle)
			{
				//do vehicle impact stuff then
				PM_VehicleImpact(p_ent, &trace);
			}
		}
#ifdef _GAME
		else
		{
			if (PM_ClientImpact(&trace, damageSelf))
			{
				continue;
			}
		}
#endif

		time_left -= time_left * trace.fraction;

		if (numplanes >= MAX_CLIP_PLANES)
		{
			// this shouldn't really happen
			VectorClear(pm->ps->velocity);
			return qtrue;
		}

		VectorCopy(trace.plane.normal, normal);

		if (!PM_GroundSlideOkay(normal[2]))
		{
			//wall-running
			//never push up off a sloped wall
			normal[2] = 0;
			VectorNormalize(normal);
		}
		//
		// if this is the same plane we hit before, nudge velocity
		// out along it, which fixes some epsilon issues with
		// non-axial planes
		//
		if (!(pm->ps->pm_flags & PMF_STUCK_TO_WALL))
		{
			//no sliding if stuck to wall!
			for (i = 0; i < numplanes; i++)
			{
				if (VectorCompare(normal, planes[i]))
				{
					//DotProduct( normal, planes[i] ) > 0.99 ) {
					VectorAdd(normal, pm->ps->velocity, pm->ps->velocity);
					break;
				}
			}
			if (i < numplanes)
			{
				continue;
			}
		}
		VectorCopy(normal, planes[numplanes]);
		numplanes++;

		//
		// modify velocity so it parallels all of the clip planes
		//

		// find a plane that it enters
		for (i = 0; i < numplanes; i++)
		{
			vec3_t end_clip_velocity;
			vec3_t clip_velocity;
			const float into = DotProduct(pm->ps->velocity, planes[i]);
			if (into >= 0.1)
			{
				continue; // move doesn't interact with the plane
			}

			// see how hard we are hitting things
			if (-into > pml.impactSpeed)
			{
				pml.impactSpeed = -into;
			}

			// slide along the plane
			PM_ClipVelocity(pm->ps->velocity, planes[i], clip_velocity, OVERCLIP);

			// slide along the plane
			PM_ClipVelocity(end_velocity, planes[i], end_clip_velocity, OVERCLIP);

			// see if there is a second plane that the new move enters
			for (int j = 0; j < numplanes; j++)
			{
				vec3_t dir;
				if (j == i)
				{
					continue;
				}
				if (DotProduct(clip_velocity, planes[j]) >= 0.1)
				{
					continue; // move doesn't interact with the plane
				}

				// try clipping the move to the plane
				PM_ClipVelocity(clip_velocity, planes[j], clip_velocity, OVERCLIP);
				PM_ClipVelocity(end_clip_velocity, planes[j], end_clip_velocity, OVERCLIP);

				// see if it goes back into the first clip plane
				if (DotProduct(clip_velocity, planes[i]) >= 0)
				{
					continue;
				}

				// slide the original velocity along the crease
				CrossProduct(planes[i], planes[j], dir);
				VectorNormalize(dir);
				float d = DotProduct(dir, pm->ps->velocity);
				VectorScale(dir, d, clip_velocity);

				CrossProduct(planes[i], planes[j], dir);
				VectorNormalize(dir);
				d = DotProduct(dir, end_velocity);
				VectorScale(dir, d, end_clip_velocity);

				// see if there is a third plane the the new move enters
				for (int k = 0; k < numplanes; k++)
				{
					if (k == i || k == j)
					{
						continue;
					}
					if (DotProduct(clip_velocity, planes[k]) >= 0.1)
					{
						continue; // move doesn't interact with the plane
					}

					// stop dead at a triple plane interaction
					VectorClear(pm->ps->velocity);
					return qtrue;
				}
			}

			// if we have fixed all interactions, try another move
			VectorCopy(clip_velocity, pm->ps->velocity);
			VectorCopy(end_clip_velocity, end_velocity);
			break;
		}
	}

	if (gravity)
	{
		VectorCopy(end_velocity, pm->ps->velocity);
	}

	// don't change velocity if in a timer (FIXME: is this correct?)
	if (pm->ps->pm_time)
	{
		VectorCopy(primal_velocity, pm->ps->velocity);
	}

	return bumpcount != 0;
}

/*
==================
PM_StepSlideMove

==================
*/
void PM_StepSlideMove(qboolean gravity)
{
	vec3_t start_o, start_v;
	vec3_t down_o, down_v;
	trace_t trace;
	vec3_t up, down;
	qboolean is_giant = qfalse;
	qboolean skip_step = qfalse;

	VectorCopy(pm->ps->origin, start_o);
	VectorCopy(pm->ps->velocity, start_v);

	if (PM_InReboundHold(pm->ps->legsAnim))
	{
		gravity = qfalse;
	}

	if (PM_SlideMove(gravity) == 0)
	{
		return; // we got exactly where we wanted to go first try
	}

	const bgEntity_t* p_ent = pm_entSelf;

	if (pm->ps->client_num >= MAX_CLIENTS)
	{
		if (p_ent && p_ent->s.NPC_class == CLASS_VEHICLE &&
			p_ent->m_pVehicle && p_ent->m_pVehicle->m_pVehicleInfo->hoverHeight > 0)
		{
			return;
		}
	}

	VectorCopy(start_o, down);
	down[2] -= STEPSIZE;
	pm->trace(&trace, start_o, pm->mins, pm->maxs, down, pm->ps->client_num, pm->tracemask);
	VectorSet(up, 0, 0, 1);
	// never step up when you still have up velocity
	if (pm->ps->velocity[2] > 0 && (trace.fraction == 1.0 || DotProduct(trace.plane.normal, up) < 0.7))
	{
		return;
	}

	VectorCopy(pm->ps->origin, down_o);
	VectorCopy(pm->ps->velocity, down_v);

	VectorCopy(start_o, up);

	if (pm->ps->client_num >= MAX_CLIENTS)
	{
		// apply ground friction, even if on ladder
		if (p_ent &&
			(p_ent->s.NPC_class == CLASS_ATST ||
				p_ent->s.NPC_class == CLASS_VEHICLE && p_ent->m_pVehicle && p_ent->m_pVehicle->m_pVehicleInfo->type ==
				VH_WALKER))
		{
			//AT-STs can step high
			up[2] += 66.0f;
			is_giant = qtrue;
		}
		else if (p_ent && p_ent->s.NPC_class == CLASS_RANCOR)
		{
			//also can step up high
			up[2] += 64.0f;
			is_giant = qtrue;
		}
		else
		{
			up[2] += STEPSIZE;
		}
	}
	else
	{
#ifdef _GAME
		if (g_entities[pm->ps->client_num].r.svFlags & SVF_BOT || pm_entSelf->s.eType == ET_NPC)
		{
			// BOTs get off easy - for the sake of lower CPU usage (routing) and looking better in general...
			up[2] += STEPSIZE * 2;
		}
		else
		{
			up[2] += STEPSIZE;
		}
#else //!_GAME
		up[2] += STEPSIZE;
#endif //_GAME
	}

	// test the player position if they were a step height higher
	pm->trace(&trace, start_o, pm->mins, pm->maxs, up, pm->ps->client_num, pm->tracemask);
	if (trace.allsolid)
	{
		if (pm->debugLevel)
		{
			Com_Printf("%i:bend can't step\n", c_pmove);
		}
		return; // can't step up
	}

	const float step_size = trace.endpos[2] - start_o[2];
	// try slidemove from this position
	VectorCopy(trace.endpos, pm->ps->origin);
	VectorCopy(start_v, pm->ps->velocity);

	PM_SlideMove(gravity);

	// push down the final amount
	VectorCopy(pm->ps->origin, down);
	down[2] -= step_size;
	pm->trace(&trace, pm->ps->origin, pm->mins, pm->maxs, down, pm->ps->client_num, pm->tracemask);

	if (pm->stepSlideFix)
	{
		if (pm->ps->client_num < MAX_CLIENTS
			&& trace.plane.normal[2] < MIN_WALK_NORMAL)
		{
			//normal players cannot step up slopes that are too steep to walk on!
			vec3_t step_vec;
			//okay, the step up ends on a slope that it too steep to step up onto,
			//BUT:
			//If the step looks like this:
			//  (B)\__
			//        \_____(A)
			//Then it might still be okay, so we figure out the slope of the entire move
			//from (A) to (B) and if that slope is walk-upabble, then it's okay
			VectorSubtract(trace.endpos, down_o, step_vec);
			VectorNormalize(step_vec);
			if (step_vec[2] > 1.0f - MIN_WALK_NORMAL)
			{
				skip_step = qtrue;
			}
		}
	}

	if (!trace.allsolid
		&& !skip_step) //normal players cannot step up slopes that are too steep to walk on!
	{
		if (pm->ps->client_num >= MAX_CLIENTS //NPC
			&& is_giant
			&& trace.entity_num < MAX_CLIENTS
			&& p_ent
			&& p_ent->s.NPC_class == CLASS_RANCOR)
		{
			//Rancor don't step on clients
			if (pm->stepSlideFix)
			{
				VectorCopy(down_o, pm->ps->origin);
				VectorCopy(down_v, pm->ps->velocity);
			}
			else
			{
				VectorCopy(start_o, pm->ps->origin);
				VectorCopy(start_v, pm->ps->velocity);
			}
		}
		else
		{
			VectorCopy(trace.endpos, pm->ps->origin);
			if (pm->stepSlideFix)
			{
				if (trace.fraction < 1.0)
				{
					PM_ClipVelocity(pm->ps->velocity, trace.plane.normal, pm->ps->velocity, OVERCLIP);
				}
			}
		}
	}
	else
	{
		if (pm->stepSlideFix)
		{
			VectorCopy(down_o, pm->ps->origin);
			VectorCopy(down_v, pm->ps->velocity);
		}
	}
	if (!pm->stepSlideFix)
	{
		if (trace.fraction < 1.0)
		{
			PM_ClipVelocity(pm->ps->velocity, trace.plane.normal, pm->ps->velocity, OVERCLIP);
		}
	}

#if 0
	// if the down trace can trace back to the original position directly, don't step
	pm->trace(&trace, pm->ps->origin, pm->mins, pm->maxs, start_o, pm->ps->client_num, pm->tracemask);
	if (trace.fraction == 1.0) {
		// use the original move
		VectorCopy(down_o, pm->ps->origin);
		VectorCopy(down_v, pm->ps->velocity);
		if (pm->debugLevel) {
			Com_Printf("%i:bend\n", c_pmove);
		}
	}
	else
#endif
	{
		const float delta = pm->ps->origin[2] - start_o[2];
		if (delta > 2)
		{
			if (delta < 7)
			{
				PM_AddEvent(EV_STEP_4);
			}
			else if (delta < 11)
			{
				PM_AddEvent(EV_STEP_8);
			}
			else if (delta < 15)
			{
				PM_AddEvent(EV_STEP_12);
			}
			else
			{
				PM_AddEvent(EV_STEP_16);
			}
		}
		if (pm->debugLevel)
		{
			Com_Printf("%i:stepped\n", c_pmove);
		}
	}
}
