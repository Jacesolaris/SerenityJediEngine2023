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
#include "wp_saber.h"
#include "w_local.h"

//---------------
//	Bryar Pistol
//---------------

extern qboolean walk_check(const gentity_t* self);
extern qboolean PM_CrouchAnim(int anim);
extern qboolean G_ControlledByPlayer(const gentity_t* self);

//---------------------------------------------------------
void WP_FireBryarPistol(gentity_t* ent, qboolean alt_fire)
//---------------------------------------------------------
{
	vec3_t start;
	int damage = !alt_fire ? weaponData[WP_BLASTER_PISTOL].damage : weaponData[WP_BLASTER_PISTOL].altDamage;

	VectorCopy(muzzle, start);
	WP_TraceSetStart(ent, start, vec3_origin, vec3_origin);
	//make sure our start point isn't on the other side of a wall

	if (ent->client && ent->client->NPC_class == CLASS_VEHICLE)
	{
		//no inherent aim screw up
	}
	else if (ent->client && (!(ent->client->ps.forcePowersActive & 1 << FP_SEE) || ent->client->ps.forcePowerLevel[
		FP_SEE] < FORCE_LEVEL_2))
	{
		//force sight 2+ gives perfect aim
		vec3_t angs;

		vectoangles(forwardVec, angs);

		if (alt_fire)
		{
			// add some slop to the alt-fire direction
			if (!walk_check(ent) && (ent->s.number < MAX_CLIENTS || G_ControlledByPlayer(ent))) //if running aim is shit
			{
				angs[PITCH] += Q_flrand(-1.0f, 1.0f) * BLASTER_ALT_SPREAD;
				angs[YAW] += Q_flrand(-1.0f, 1.0f) * BLASTER_ALT_SPREAD;
			}
			else if (ent->client->ps.BlasterAttackChainCount >= BLASTERMISHAPLEVEL_FULL)
			{
				// add some slop to the fire direction
				angs[PITCH] += Q_flrand(-3.0f, 3.0f) * BLASTER_ALT_SPREAD;
				angs[YAW] += Q_flrand(-3.0f, 3.0f) * BLASTER_ALT_SPREAD;
			}
			else if (ent->client->ps.BlasterAttackChainCount >= BLASTERMISHAPLEVEL_HEAVY)
			{
				// add some slop to the fire direction
				angs[PITCH] += Q_flrand(-2.0f, 2.0f) * BLASTER_ALT_SPREAD;
				angs[YAW] += Q_flrand(-2.0f, 2.0f) * BLASTER_ALT_SPREAD;
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
				angs[PITCH] += Q_flrand(-1.0f, 1.0f) * BLASTER_MAIN_SPREAD;
				angs[YAW] += Q_flrand(-1.0f, 1.0f) * BLASTER_MAIN_SPREAD;
			}
			else if (ent->client->ps.BlasterAttackChainCount >= BLASTERMISHAPLEVEL_FULL)
			{
				// add some slop to the fire direction
				angs[PITCH] += Q_flrand(-3.0f, 3.0f) * BLASTER_MAIN_SPREAD;
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

		AngleVectors(angs, forwardVec, nullptr, nullptr);
	}

		WP_MissileTargetHint(ent, start, forwardVec);

		gentity_t* missile = create_missile(start, forwardVec, BRYAR_PISTOL_VEL, 10000, ent, alt_fire);

		missile->classname = "bryar_proj";

		if (ent->s.weapon == WP_BRYAR_PISTOL
			|| ent->s.weapon == WP_JAWA)
		{
			//*SIGH*... I hate our weapon system...
			missile->s.weapon = ent->s.weapon;
		}
		else
		{
			missile->s.weapon = WP_BLASTER_PISTOL;
		}

		if (alt_fire)
		{
			int count = (level.time - ent->client->ps.weaponChargeTime) / BRYAR_CHARGE_UNIT;

			if (count < 1)
			{
				count = 1;
			}
			else if (count > 5)
			{
				count = 5;
			}

			damage *= count;
			missile->count = count; // this will get used in the projectile rendering code to make a beefier effect
		}

		missile->damage = damage;
		missile->dflags = DAMAGE_DEATH_KNOCKBACK;

		if (alt_fire)
		{
			missile->methodOfDeath = MOD_BRYAR_ALT;
		}
		else
		{
			missile->methodOfDeath = MOD_BRYAR;
		}

		missile->clipmask = MASK_SHOT;

		// we don't want it to bounce forever
		missile->bounceCount = 8;

		if (ent->weaponModel[1] > 0)
		{
			//dual pistols, toggle the muzzle point back and forth between the two pistols each time he fires
			ent->count = ent->count ? 0 : 1;
		}
}

//---------------------------------------------------------
void WP_FireBryarPistolDuals(gentity_t* ent, qboolean alt_fire, qboolean secondPistol)
//---------------------------------------------------------
{
	vec3_t start;
	int damage = !alt_fire ? weaponData[WP_BLASTER_PISTOL].damage : weaponData[WP_BLASTER_PISTOL].altDamage;

	if (secondPistol)
	{
		VectorCopy(muzzle2, start);
	}
	else
	{
		VectorCopy(muzzle, start);
	}

	WP_TraceSetStart(ent, start, vec3_origin, vec3_origin);
	//make sure our start point isn't on the other side of a wall

	if (ent->client && ent->client->NPC_class == CLASS_VEHICLE)
	{
		//no inherent aim screw up
	}
	else if (ent->client && (!(ent->client->ps.forcePowersActive & 1 << FP_SEE) || ent->client->ps.forcePowerLevel[
		FP_SEE] < FORCE_LEVEL_2))
	{
		//force sight 2+ gives perfect aim
		vec3_t angs;

		vectoangles(forwardVec, angs);

		if (alt_fire)
		{
			// add some slop to the alt-fire direction
			if (!walk_check(ent) && (ent->s.number < MAX_CLIENTS || G_ControlledByPlayer(ent))) //if running aim is shit
			{
				angs[PITCH] += Q_flrand(-1.0f, 1.0f) * BLASTER_ALT_SPREAD;
				angs[YAW] += Q_flrand(-1.0f, 1.0f) * BLASTER_ALT_SPREAD;
			}
			else if (ent->client->ps.BlasterAttackChainCount >= BLASTERMISHAPLEVEL_FULL)
			{
				// add some slop to the fire direction
				angs[PITCH] += Q_flrand(-3.0f, 3.0f) * BLASTER_ALT_SPREAD;
				angs[YAW] += Q_flrand(-3.0f, 3.0f) * BLASTER_ALT_SPREAD;
			}
			else if (ent->client->ps.BlasterAttackChainCount >= BLASTERMISHAPLEVEL_HEAVY)
			{
				// add some slop to the fire direction
				angs[PITCH] += Q_flrand(-2.0f, 2.0f) * BLASTER_ALT_SPREAD;
				angs[YAW] += Q_flrand(-2.0f, 2.0f) * BLASTER_ALT_SPREAD;
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
				angs[PITCH] += Q_flrand(-1.0f, 1.0f) * BLASTER_MAIN_SPREAD;
				angs[YAW] += Q_flrand(-1.0f, 1.0f) * BLASTER_MAIN_SPREAD;
			}
			else if (ent->client->ps.BlasterAttackChainCount >= BLASTERMISHAPLEVEL_FULL)
			{
				// add some slop to the fire direction
				angs[PITCH] += Q_flrand(-3.0f, 3.0f) * BLASTER_MAIN_SPREAD;
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

		AngleVectors(angs, forwardVec, nullptr, nullptr);
	}

		WP_MissileTargetHint(ent, start, forwardVec);

		gentity_t* missile = create_missile(start, forwardVec, BRYAR_PISTOL_VEL, 10000, ent, alt_fire);

		missile->classname = "bryar_proj";

		if (ent->s.weapon == WP_BRYAR_PISTOL
			|| ent->s.weapon == WP_JAWA)
		{
			//*SIGH*... I hate our weapon system...
			missile->s.weapon = ent->s.weapon;
		}
		else
		{
			missile->s.weapon = WP_BLASTER_PISTOL;
		}

		if (alt_fire)
		{
			int count = (level.time - ent->client->ps.weaponChargeTime) / BRYAR_CHARGE_UNIT;

			if (count < 1)
			{
				count = 1;
			}
			else if (count > 5)
			{
				count = 5;
			}

			damage *= count;
			missile->count = count; // this will get used in the projectile rendering code to make a beefier effect
		}

		missile->damage = damage;
		missile->dflags = DAMAGE_DEATH_KNOCKBACK;

		if (alt_fire)
		{
			missile->methodOfDeath = MOD_BRYAR_ALT;
		}
		else
		{
			missile->methodOfDeath = MOD_BRYAR;
		}

		missile->clipmask = MASK_SHOT;

		// we don't want it to bounce forever
		missile->bounceCount = 8;

		if (ent->weaponModel[1] > 0)
		{
			//dual pistols, toggle the muzzle point back and forth between the two pistols each time he fires
			ent->count = ent->count ? 0 : 1;
		}
}

//---------------
//	sbdBlaster
//---------------

//---------------------------------------------------------
void WP_FireBryarsbdMissile(gentity_t* ent, vec3_t start, vec3_t dir, qboolean alt_fire)
//---------------------------------------------------------
{
	const int velocity = BRYAR_PISTOL_VEL;
	int damage = alt_fire ? weaponData[WP_SBD_PISTOL].altDamage : weaponData[WP_SBD_PISTOL].damage;

	WP_TraceSetStart(ent, start, vec3_origin, vec3_origin);
	//make sure our start point isn't on the other side of a wall

	WP_MissileTargetHint(ent, start, dir);

	gentity_t* missile = create_missile(start, dir, velocity, 10000, ent, alt_fire);

	missile->classname = "bryar_proj";
	missile->s.weapon = WP_SBD_PISTOL;

	// Do the damages
	if (alt_fire)
	{
		int count = (level.time - ent->client->ps.weaponChargeTime) / BRYAR_CHARGE_UNIT;

		if (count < 1)
		{
			count = 1;
		}
		else if (count > 5)
		{
			count = 5;
		}

		damage *= count;
		missile->count = count; // this will get used in the projectile rendering code to make a beefier effect
	}

	missile->damage = damage;
	missile->dflags = DAMAGE_DEATH_KNOCKBACK;

	if (alt_fire)
	{
		missile->methodOfDeath = MOD_PISTOL_ALT;
	}
	else
	{
		missile->methodOfDeath = MOD_PISTOL;
	}

	missile->clipmask = MASK_SHOT;

	// we don't want it to bounce forever
	missile->bounceCount = 8;
}

//---------------------------------------------------------
void WP_FireBryarsbdPistol(gentity_t* ent, qboolean alt_fire)
//---------------------------------------------------------
{
	vec3_t dir, angs;

	vectoangles(forwardVec, angs);

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
				angs[PITCH] += Q_flrand(-1.0f, 1.0f) * BLASTER_ALT_SPREAD;
				angs[YAW] += Q_flrand(-1.0f, 1.0f) * BLASTER_ALT_SPREAD;
			}
			else if (ent->client->ps.BlasterAttackChainCount >= BLASTERMISHAPLEVEL_FULL)
			{
				// add some slop to the fire direction
				angs[PITCH] += Q_flrand(-3.0f, 3.0f) * BLASTER_ALT_SPREAD;
				angs[YAW] += Q_flrand(-3.0f, 3.0f) * BLASTER_ALT_SPREAD;
			}
			else if (ent->client->ps.BlasterAttackChainCount >= BLASTERMISHAPLEVEL_HEAVY)
			{
				// add some slop to the fire direction
				angs[PITCH] += Q_flrand(-2.0f, 2.0f) * BLASTER_ALT_SPREAD;
				angs[YAW] += Q_flrand(-2.0f, 2.0f) * BLASTER_ALT_SPREAD;
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
				angs[PITCH] += Q_flrand(-1.0f, 1.0f) * BLASTER_MAIN_SPREAD;
				angs[YAW] += Q_flrand(-1.0f, 1.0f) * BLASTER_MAIN_SPREAD;
			}
			else if (ent->client->ps.BlasterAttackChainCount >= BLASTERMISHAPLEVEL_FULL)
			{
				// add some slop to the fire direction
				angs[PITCH] += Q_flrand(-3.0f, 3.0f) * BLASTER_MAIN_SPREAD;
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

		WP_FireBryarsbdMissile(ent, muzzle, dir, alt_fire);
}

//---------------------------------------------------------
void WP_FireJawaPistol(gentity_t* ent, qboolean alt_fire)
//---------------------------------------------------------
{
	vec3_t start;
	int damage = !alt_fire ? weaponData[WP_JAWA].damage : weaponData[WP_JAWA].altDamage;

	VectorCopy(muzzle, start);
	WP_TraceSetStart(ent, start, vec3_origin, vec3_origin);
	//make sure our start point isn't on the other side of a wall

	if (!(ent->client->ps.forcePowersActive & 1 << FP_SEE) || ent->client->ps.forcePowerLevel[FP_SEE] < FORCE_LEVEL_2)
	{
		//force sight 2+ gives perfect aim
		if (ent->NPC && ent->NPC->currentAim < 5)
		{
			vec3_t angs;

			vectoangles(forwardVec, angs);

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
			else if (!walk_check(ent) && (ent->s.number < MAX_CLIENTS || G_ControlledByPlayer(ent)))
				//if running aim is shit
			{
				angs[PITCH] += Q_flrand(-2.0f, 2.0f) * (RUNNING_SPREAD + 1.5f);
				angs[YAW] += Q_flrand(-2.0f, 2.0f) * (RUNNING_SPREAD + 1.5f);
			}
			else if (ent->client && ent->client->ps.BlasterAttackChainCount >= BLASTERMISHAPLEVEL_FULL)
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
				// add some slop to the main-fire direction
				angs[PITCH] += Q_flrand(-1.0f, 1.0f) * BLASTER_MAIN_SPREAD;
				angs[YAW] += Q_flrand(-1.0f, 1.0f) * BLASTER_MAIN_SPREAD;
			}
			else
			{
				// add some slop to the main-fire direction
				angs[PITCH] += Q_flrand(-1.0f, 1.0f) * BLASTER_MAIN_SPREAD;
				angs[YAW] += Q_flrand(-1.0f, 1.0f) * BLASTER_MAIN_SPREAD;
			}

			AngleVectors(angs, forwardVec, nullptr, nullptr);
		}
	}

	WP_MissileTargetHint(ent, start, forwardVec);

	gentity_t* missile = create_missile(start, forwardVec, BRYAR_PISTOL_VEL, 10000, ent, alt_fire);

	missile->classname = "bryar_proj";
	if (ent->s.weapon == WP_BLASTER_PISTOL
		|| ent->s.weapon == WP_SBD_PISTOL
		|| ent->s.weapon == WP_BRYAR_PISTOL)
	{
		//*SIGH*... I hate our weapon system...
		missile->s.weapon = ent->s.weapon;
	}
	else
	{
		missile->s.weapon = WP_JAWA;
	}

	if (alt_fire)
	{
		int count = (level.time - ent->client->ps.weaponChargeTime) / BRYAR_CHARGE_UNIT;

		if (count < 1)
		{
			count = 1;
		}
		else if (count > 5)
		{
			count = 5;
		}

		damage *= count;
		missile->count = count; // this will get used in the projectile rendering code to make a beefier effect
	}

	missile->damage = damage;
	missile->dflags = DAMAGE_DEATH_KNOCKBACK;

	if (alt_fire)
	{
		missile->methodOfDeath = MOD_BRYAR_ALT;
	}
	else
	{
		missile->methodOfDeath = MOD_BRYAR_ALT;
	}

	missile->clipmask = MASK_SHOT;

	// we don't want it to bounce forever
	missile->bounceCount = 8;

	if (ent->weaponModel[1] > 0)
	{
		//dual pistols, toggle the muzzle point back and forth between the two pistols each time he fires
		ent->count = ent->count ? 0 : 1;
	}
}

//---------------------------------------------------------
void WP_FireBryarPistolold(gentity_t* ent, qboolean alt_fire)
//---------------------------------------------------------
{
	vec3_t start;
	int damage = !alt_fire ? weaponData[WP_BRYAR_PISTOL].damage : weaponData[WP_BRYAR_PISTOL].altDamage;

	VectorCopy(muzzle, start);
	WP_TraceSetStart(ent, start, vec3_origin, vec3_origin);
	//make sure our start point isn't on the other side of a wall

	if (ent->client && ent->client->NPC_class == CLASS_VEHICLE)
	{
		//no inherent aim screw up
	}
	else if (ent->client && (!(ent->client->ps.forcePowersActive & 1 << FP_SEE) || ent->client->ps.forcePowerLevel[
		FP_SEE] < FORCE_LEVEL_2))
	{
		//force sight 2+ gives perfect aim
		vec3_t angs;

		vectoangles(forwardVec, angs);

		if (alt_fire)
		{
			// add some slop to the alt-fire direction
			if (!walk_check(ent) && (ent->s.number < MAX_CLIENTS || G_ControlledByPlayer(ent))) //if running aim is shit
			{
				angs[PITCH] += Q_flrand(-1.0f, 1.0f) * BLASTER_ALT_SPREAD;
				angs[YAW] += Q_flrand(-1.0f, 1.0f) * BLASTER_ALT_SPREAD;
			}
			else if (ent->client->ps.BlasterAttackChainCount >= BLASTERMISHAPLEVEL_FULL)
			{
				// add some slop to the fire direction
				angs[PITCH] += Q_flrand(-3.0f, 3.0f) * BLASTER_ALT_SPREAD;
				angs[YAW] += Q_flrand(-3.0f, 3.0f) * BLASTER_ALT_SPREAD;
			}
			else if (ent->client->ps.BlasterAttackChainCount >= BLASTERMISHAPLEVEL_HEAVY)
			{
				// add some slop to the fire direction
				angs[PITCH] += Q_flrand(-2.0f, 2.0f) * BLASTER_ALT_SPREAD;
				angs[YAW] += Q_flrand(-2.0f, 2.0f) * BLASTER_ALT_SPREAD;
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
				angs[PITCH] += Q_flrand(-1.0f, 1.0f) * BLASTER_MAIN_SPREAD;
				angs[YAW] += Q_flrand(-1.0f, 1.0f) * BLASTER_MAIN_SPREAD;
			}
			else if (ent->client->ps.BlasterAttackChainCount >= BLASTERMISHAPLEVEL_FULL)
			{
				// add some slop to the fire direction
				angs[PITCH] += Q_flrand(-3.0f, 3.0f) * BLASTER_MAIN_SPREAD;
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

		AngleVectors(angs, forwardVec, nullptr, nullptr);
	}

		WP_MissileTargetHint(ent, start, forwardVec);

		gentity_t* missile = create_missile(start, forwardVec, BRYAR_PISTOL_VEL, 10000, ent, alt_fire);

		missile->classname = "bryar_proj";
		if (ent->s.weapon == WP_BLASTER_PISTOL
			|| ent->s.weapon == WP_JAWA)
		{
			//*SIGH*... I hate our weapon system...
			missile->s.weapon = ent->s.weapon;
		}
		else
		{
			missile->s.weapon = WP_BRYAR_PISTOL;
		}

		if (alt_fire)
		{
			int count = (level.time - ent->client->ps.weaponChargeTime) / BRYAR_CHARGE_UNIT;

			if (count < 1)
			{
				count = 1;
			}
			else if (count > 5)
			{
				count = 5;
			}

			damage *= count;
			missile->count = count; // this will get used in the projectile rendering code to make a beefier effect
		}

		missile->damage = damage;
		missile->dflags = DAMAGE_DEATH_KNOCKBACK;

		if (alt_fire)
		{
			missile->methodOfDeath = MOD_PISTOL_ALT;
		}
		else
		{
			missile->methodOfDeath = MOD_PISTOL;
		}

		missile->clipmask = MASK_SHOT | CONTENTS_LIGHTSABER;

		// we don't want it to bounce forever
		missile->bounceCount = 8;

		if (ent->weaponModel[1] > 0)
		{
			//dual pistols, toggle the muzzle point back and forth between the two pistols each time he fires
			ent->count = ent->count ? 0 : 1;
		}
}