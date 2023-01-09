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

#include "g_local.h"
#include "b_local.h"
#include "g_functions.h"
#include "wp_saber.h"
#include "w_local.h"

extern qboolean walk_check(const gentity_t* self);
extern qboolean PM_CrouchAnim(int anim);
extern qboolean G_ControlledByPlayer(const gentity_t* self);

//---------------
//	Blaster
//---------------

//---------------------------------------------------------
void WP_FireBlasterMissile(gentity_t* ent, vec3_t start, vec3_t dir, const qboolean alt_fire)
//---------------------------------------------------------
{
	int velocity = BLASTER_VELOCITY;
	int damage = alt_fire ? weaponData[WP_BLASTER].altDamage : weaponData[WP_BLASTER].damage;

	if (ent && ent->client && ent->client->NPC_class == CLASS_VEHICLE)
	{
		damage *= 3;
		velocity = ATST_MAIN_VEL + ent->client->ps.speed;
	}
	else
	{
		// If an enemy is shooting at us, lower the velocity so you have a chance to evade
		if (ent->client && ent->client->ps.client_num != 0 && ent->client->NPC_class != CLASS_BOBAFETT && ent->client->
			NPC_class != CLASS_MANDO)
		{
			if (g_spskill->integer < 2)
			{
				velocity *= BLASTER_NPC_VEL_CUT;
			}
			else
			{
				velocity *= BLASTER_NPC_HARD_VEL_CUT;
			}
		}
	}

	WP_TraceSetStart(ent, start);
	//make sure our start point isn't on the other side of a wall

	WP_MissileTargetHint(ent, start, dir);

	gentity_t* missile = create_missile(start, dir, velocity, 10000, ent, alt_fire);

	missile->classname = "blaster_proj";
	missile->s.weapon = WP_BLASTER;

	// Do the damages
	if (ent->s.number != 0 && ent->client->NPC_class != CLASS_BOBAFETT && ent->client->NPC_class != CLASS_MANDO)
	{
		if (g_spskill->integer == 0)
		{
			damage = BLASTER_NPC_DAMAGE_EASY;
		}
		else if (g_spskill->integer == 1)
		{
			damage = BLASTER_NPC_DAMAGE_NORMAL;
		}
		else
		{
			damage = BLASTER_NPC_DAMAGE_HARD;
		}
	}

	missile->damage = damage;
	missile->dflags = DAMAGE_DEATH_KNOCKBACK | DAMAGE_EXTRA_KNOCKBACK;
	if (alt_fire)
	{
		missile->methodOfDeath = MOD_BLASTER_ALT;
	}
	else
	{
		missile->methodOfDeath = MOD_BLASTER;
	}
	missile->clipmask = MASK_SHOT;

	// we don't want it to bounce forever
	missile->bounceCount = 8;
}

//---------------------------------------------------------
void WP_FireBlaster(gentity_t* ent, const qboolean alt_fire)
//---------------------------------------------------------
{
	vec3_t dir, angs;

	vectoangles(forward_vec, angs);

	if (ent->client && ent->client->NPC_class == CLASS_VEHICLE)
	{
		//no inherent aim screw up
	}
	else if (ent->client && (!(ent->client->ps.forcePowersActive & 1 << FP_SEE) || ent->client->ps.forcePowerLevel[
		FP_SEE] < FORCE_LEVEL_2))
	{
		//force sight 2+ gives perfect aim
		if (alt_fire)
		{
			// add some slop to the alt-fire direction
			if (!walk_check(ent) && (ent->s.number < MAX_CLIENTS || G_ControlledByPlayer(ent))) //if running aim is shit
			{
				angs[PITCH] += Q_flrand(-2.0f, 2.0f) * (BLASTER_ALT_SPREAD + 1.5f);
				angs[YAW] += Q_flrand(-2.0f, 2.0f) * (BLASTER_ALT_SPREAD + 1.5f);
			}
			else if (ent->client->ps.BlasterAttackChainCount >= BLASTERMISHAPLEVEL_FULL)
			{
				// add some slop to the fire direction
				angs[PITCH] += Q_flrand(-5.0f, 5.0f) * BLASTER_MAIN_SPREAD;
				angs[YAW] += Q_flrand(-3.0f, 3.0f) * BLASTER_MAIN_SPREAD;
			}
			else if (ent->client->ps.BlasterAttackChainCount >= BLASTERMISHAPLEVEL_HEAVY)
			{
				// add some slop to the fire direction
				angs[PITCH] += Q_flrand(-2.0f, 2.0f) * BLASTER_MAIN_SPREAD;
				angs[YAW] += Q_flrand(-2.0f, 2.0f) * BLASTER_MAIN_SPREAD;
			}
			else if (PM_CrouchAnim(ent->client->ps.legsAnim))
			{
				//
			}
			else
			{
				angs[PITCH] += Q_flrand(-1.0f, 1.0f) * BLASTER_ALT_SPREAD;
				angs[YAW] += Q_flrand(-1.0f, 1.0f) * BLASTER_ALT_SPREAD;
			}
		}
		else
		{
			if (ent->NPC && ent->NPC->currentAim < 5)
			{
				if (ent->client && ent->NPC &&
					(ent->client->NPC_class == CLASS_STORMTROOPER ||
						ent->client->NPC_class == CLASS_CLONETROOPER ||
						ent->client->NPC_class == CLASS_STORMCOMMANDO ||
						ent->client->NPC_class == CLASS_SWAMPTROOPER ||
						ent->client->NPC_class == CLASS_DROIDEKA ||
						ent->client->NPC_class == CLASS_SBD ||
						ent->client->NPC_class == CLASS_IMPWORKER ||
						ent->client->NPC_class == CLASS_REBEL ||
						ent->client->NPC_class == CLASS_WOOKIE ||
						ent->client->NPC_class == CLASS_BATTLEDROID))
				{
					angs[PITCH] += Q_flrand(-1.0f, 1.0f) * (BLASTER_NPC_SPREAD + (1 - ent->NPC->currentAim) * 0.25f);
					//was 0.5f
					angs[YAW] += Q_flrand(-1.0f, 1.0f) * (BLASTER_NPC_SPREAD + (1 - ent->NPC->currentAim) * 0.25f);
					//was 0.5
				}
			}
			else if (!walk_check(ent) && (ent->s.number < MAX_CLIENTS || G_ControlledByPlayer(ent)))
			//if running aim is shit
			{
				angs[PITCH] += Q_flrand(-2.0f, 2.0f) * (BLASTER_ALT_SPREAD + 1.5f);
				angs[YAW] += Q_flrand(-2.0f, 2.0f) * (RUNNING_SPREAD + 1.5f);
			}
			else if (ent->client->ps.BlasterAttackChainCount >= BLASTERMISHAPLEVEL_FULL)
			{
				// add some slop to the fire direction
				angs[PITCH] += Q_flrand(-5.0f, 5.0f) * BLASTER_MAIN_SPREAD;
				angs[YAW] += Q_flrand(-3.0f, 3.0f) * BLASTER_MAIN_SPREAD;
			}
			else if (ent->client->ps.BlasterAttackChainCount >= BLASTERMISHAPLEVEL_HEAVY)
			{
				// add some slop to the fire direction
				angs[PITCH] += Q_flrand(-2.0f, 2.0f) * BLASTER_MAIN_SPREAD;
				angs[YAW] += Q_flrand(-2.0f, 2.0f) * BLASTER_MAIN_SPREAD;
			}
			else if (PM_CrouchAnim(ent->client->ps.legsAnim))
			{
				//
			}
			else
			{
				//
			}
		}
	}

	AngleVectors(angs, dir, nullptr, nullptr);

	WP_FireBlasterMissile(ent, muzzle, dir, alt_fire);
}

//---------------------------------------------------------
void WP_FireJangoWristMissile(gentity_t* ent, vec3_t start, vec3_t dir, const qboolean alt_fire)
//---------------------------------------------------------
{
	int velocity = CLONECOMMANDO_VELOCITY;
	int damage = alt_fire ? weaponData[WP_WRIST_BLASTER].altDamage : weaponData[WP_WRIST_BLASTER].damage;

	if (ent && ent->client && ent->client->NPC_class == CLASS_VEHICLE)
	{
		damage *= 3;
		velocity = ATST_MAIN_VEL + ent->client->ps.speed;
	}
	else
	{
		// If an enemy is shooting at us, lower the velocity so you have a chance to evade
		if (ent->client && ent->client->ps.client_num != 0 && ent->client->NPC_class != CLASS_BOBAFETT && ent->client->
			NPC_class != CLASS_MANDO)
		{
			if (g_spskill->integer < 2)
			{
				velocity *= BLASTER_NPC_VEL_CUT;
			}
			else
			{
				velocity *= BLASTER_NPC_HARD_VEL_CUT;
			}
		}
	}

	WP_TraceSetStart(ent, start);
	//make sure our start point isn't on the other side of a wall

	WP_MissileTargetHint(ent, start, dir);

	gentity_t* missile = create_missile(start, dir, velocity, 10000, ent, alt_fire);

	missile->classname = "clone_proj";
	missile->s.weapon = WP_WRIST_BLASTER;

	// Do the damages
	if (ent->s.number != 0 && ent->client->NPC_class != CLASS_BOBAFETT && ent->client->NPC_class != CLASS_MANDO)
	{
		if (g_spskill->integer == 0)
		{
			damage = BLASTER_NPC_DAMAGE_EASY;
		}
		else if (g_spskill->integer == 1)
		{
			damage = BLASTER_NPC_DAMAGE_NORMAL;
		}
		else
		{
			damage = BLASTER_NPC_DAMAGE_HARD;
		}
	}

	missile->damage = damage;
	missile->dflags = DAMAGE_DEATH_KNOCKBACK | DAMAGE_EXTRA_KNOCKBACK;
	if (alt_fire)
	{
		missile->methodOfDeath = MOD_BLASTER_ALT;
	}
	else
	{
		missile->methodOfDeath = MOD_BLASTER;
	}
	missile->clipmask = MASK_SHOT | CONTENTS_LIGHTSABER;

	// we don't want it to bounce forever
	missile->bounceCount = 8;
}

//---------------------------------------------------------
void WP_FireWristPistol(gentity_t* ent, const qboolean alt_fire)
//---------------------------------------------------------
{
	vec3_t dir, angs;

	vectoangles(forward_vec, angs);

	if (ent->client && ent->client->NPC_class == CLASS_VEHICLE)
	{
		//no inherent aim screw up
	}
	else if (ent->client && (!(ent->client->ps.forcePowersActive & 1 << FP_SEE) || ent->client->ps.forcePowerLevel[
		FP_SEE] < FORCE_LEVEL_2))
	{
		//force sight 2+ gives perfect aim
		if (alt_fire)
		{
			// add some slop to the alt-fire direction
			if (!walk_check(ent) && (ent->s.number < MAX_CLIENTS || G_ControlledByPlayer(ent))) //if running aim is shit
			{
				angs[PITCH] += Q_flrand(-2.0f, 2.0f) * (BLASTER_ALT_SPREAD + 1.5f);
				angs[YAW] += Q_flrand(-2.0f, 2.0f) * (BLASTER_ALT_SPREAD + 1.5f);
			}
			else if (ent->client->ps.BlasterAttackChainCount >= BLASTERMISHAPLEVEL_FULL)
			{
				// add some slop to the fire direction
				angs[PITCH] += Q_flrand(-5.0f, 5.0f) * BLASTER_MAIN_SPREAD;
				angs[YAW] += Q_flrand(-3.0f, 3.0f) * BLASTER_MAIN_SPREAD;
			}
			else if (ent->client->ps.BlasterAttackChainCount >= BLASTERMISHAPLEVEL_HEAVY)
			{
				// add some slop to the fire direction
				angs[PITCH] += Q_flrand(-2.0f, 2.0f) * BLASTER_MAIN_SPREAD;
				angs[YAW] += Q_flrand(-2.0f, 2.0f) * BLASTER_MAIN_SPREAD;
			}
			else if (PM_CrouchAnim(ent->client->ps.legsAnim))
			{
				//
			}
			else
			{
				angs[PITCH] += Q_flrand(-1.0f, 1.0f) * BLASTER_ALT_SPREAD;
				angs[YAW] += Q_flrand(-1.0f, 1.0f) * BLASTER_ALT_SPREAD;
			}
		}
		else
		{
			if (ent->NPC && ent->NPC->currentAim < 5)
			{
				if (ent->client && ent->NPC &&
					(ent->client->NPC_class == CLASS_STORMTROOPER ||
						ent->client->NPC_class == CLASS_CLONETROOPER ||
						ent->client->NPC_class == CLASS_STORMCOMMANDO ||
						ent->client->NPC_class == CLASS_SWAMPTROOPER ||
						ent->client->NPC_class == CLASS_DROIDEKA ||
						ent->client->NPC_class == CLASS_SBD ||
						ent->client->NPC_class == CLASS_IMPWORKER ||
						ent->client->NPC_class == CLASS_REBEL ||
						ent->client->NPC_class == CLASS_WOOKIE ||
						ent->client->NPC_class == CLASS_BATTLEDROID))
				{
					angs[PITCH] += Q_flrand(-1.0f, 1.0f) * (BLASTER_NPC_SPREAD + (1 - ent->NPC->currentAim) * 0.25f);
					//was 0.5f
					angs[YAW] += Q_flrand(-1.0f, 1.0f) * (BLASTER_NPC_SPREAD + (1 - ent->NPC->currentAim) * 0.25f);
					//was 0.5
				}
			}
			else if (!walk_check(ent) && (ent->s.number < MAX_CLIENTS || G_ControlledByPlayer(ent)))
			//if running aim is shit
			{
				angs[PITCH] += Q_flrand(-2.0f, 2.0f) * (BLASTER_ALT_SPREAD + 1.5f);
				angs[YAW] += Q_flrand(-2.0f, 2.0f) * (RUNNING_SPREAD + 1.5f);
			}
			else if (ent->client->ps.BlasterAttackChainCount >= BLASTERMISHAPLEVEL_FULL)
			{
				// add some slop to the fire direction
				angs[PITCH] += Q_flrand(-5.0f, 5.0f) * BLASTER_MAIN_SPREAD;
				angs[YAW] += Q_flrand(-3.0f, 3.0f) * BLASTER_MAIN_SPREAD;
			}
			else if (ent->client->ps.BlasterAttackChainCount >= BLASTERMISHAPLEVEL_HEAVY)
			{
				// add some slop to the fire direction
				angs[PITCH] += Q_flrand(-2.0f, 2.0f) * BLASTER_MAIN_SPREAD;
				angs[YAW] += Q_flrand(-2.0f, 2.0f) * BLASTER_MAIN_SPREAD;
			}
			else if (PM_CrouchAnim(ent->client->ps.legsAnim))
			{
				//
			}
			else
			{
				//
			}
		}
	}

	AngleVectors(angs, dir, nullptr, nullptr);

	WP_FireJangoWristMissile(ent, muzzle, dir, alt_fire);
}
