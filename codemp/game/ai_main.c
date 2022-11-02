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

/*****************************************************************************
 * name:		ai_main.c
 *
 * desc:		Quake3 bot AI
 *
 * $Archive: /MissionPack/code/game/ai_main.c $
 * $Author: Osman $
 * $Revision: 1.5 $
 * $Modtime: 6/06/01 1:11p $
 * $Date: 2003/03/15 23:43:59 $
 *
 *****************************************************************************/

#include "g_local.h"
#include "qcommon/q_shared.h"
#include "botlib/botlib.h"		//bot lib interface
#include "botlib/be_aas.h"
 //
#include "ai_main.h"
#include "w_saber.h"
//

#define BOT_THINK_TIME	1000/bot_fps.integer

//bot states
bot_state_t* botstates[MAX_CLIENTS];
int walktime[MAX_CLIENTS]; //timer for walking
int blocktime[MAX_CLIENTS]; //timer for blocking
int gesturetime[MAX_CLIENTS]; //timer for taunting
int nexttaunt[MAX_CLIENTS];
#define	APEX_HEIGHT		200.0f
//data stuff
qboolean bot_will_fall[MAX_CLIENTS];
vec3_t jumpPos[MAX_CLIENTS];
int next_bot_fallcheck[MAX_CLIENTS];
int next_bot_vischeck[MAX_CLIENTS];
int last_bot_wp_vischeck_result[MAX_CLIENTS];
vec3_t safePos[MAX_CLIENTS];
int last_checkfall[MAX_GENTITIES];
//number of bots
int numbots;
//floating point time
float floattime;
//time to do a regular update
float regularupdate_time;

int BotGetWeaponRange(const bot_state_t* bs);
int PassLovedOneCheck(const bot_state_t* bs, const gentity_t* ent);
void ExitLevel(void);
qboolean WP_ForcePowerUsable(const gentity_t* self, forcePowers_t forcePower);
extern qboolean G_HeavyMelee(const gentity_t* attacker);
void Siege_DefendFromAttackers(bot_state_t* bs);
void RequestSiegeAssistance(bot_state_t* bs, int BaseClass);
qboolean switch_siege_ideal_class(const bot_state_t* bs, const char* idealclass);
extern int rebel_attackers;
extern int imperial_attackers;
void BotBehave_AttackBasic(bot_state_t* bs, const gentity_t* target);
extern void G_SoundOnEnt(gentity_t* ent, soundChannel_t channel, const char* soundPath);
extern qboolean PM_SaberInBounce(int move);
extern qboolean PM_SaberInReturn(int move);
extern qboolean PM_SaberInStart(int move);
extern qboolean PM_SaberInTransition(int move);
extern void NPC_SetAnim(gentity_t* ent, int setAnimParts, int anim, int setAnimFlags);
qboolean AttackLocalBreakable(bot_state_t* bs, vec3_t origin);
boteventtracker_t gBotEventTracker[MAX_CLIENTS];
extern qboolean G_PointInBounds(vec3_t point, vec3_t mins, vec3_t maxs);
extern siegeClass_t* BG_GetClassOnBaseClass(int team, short classIndex, short cntIndex);
extern void G_AddVoiceEvent(gentity_t* self, int event, int speakDebounceTime);
int NumberofSiegeSpecificClass(int team, const char* classname);
void BotAimLeading(bot_state_t* bs, vec3_t headlevel, float leadAmount);
float BotWeaponCanLead(const bot_state_t* bs);
int BotWeapon_Detpack(bot_state_t* bs, const gentity_t* target);
void TraceMove(bot_state_t* bs, vec3_t move_dir, int targetNum);
extern qboolean G_NameInTriggerClassList(char* list, char* str);
void BotBehave_DefendBasic(bot_state_t* bs, vec3_t defpoint);
int BotSelectChoiceWeapon(bot_state_t* bs, int weapon, int doselection);
void adjustfor_strafe(const bot_state_t* bs, vec3_t move_dir);
void BotBehave_Attack(bot_state_t* bs);
extern const gbuyable_t bg_buylist[];
extern qboolean IsSurrendering(gentity_t* self);
extern qboolean IsRespecting(gentity_t* self);
extern qboolean IsCowering(gentity_t* self);
extern qboolean IsAnimRequiresResponce(gentity_t* self);
qboolean G_ThereIsAMaster(void);
void BotBehave_AttackMove(bot_state_t* bs);

//rww - new bot cvars..
vmCvar_t bot_forcepowers;
vmCvar_t bot_forgimmick;
vmCvar_t bot_honorableduelacceptance;
vmCvar_t bot_pvstype;
vmCvar_t bot_normgpath;
#ifndef FINAL_BUILD
vmCvar_t bot_getinthecarrr;
#endif

#ifdef _DEBUG
vmCvar_t bot_nogoals;
vmCvar_t bot_debugmessages;
#endif

vmCvar_t bot_attachments;
vmCvar_t bot_camp;

vmCvar_t bot_wp_info;
vmCvar_t bot_wp_edit;
vmCvar_t bot_wp_clearweight;
vmCvar_t bot_wp_distconnect;
vmCvar_t bot_wp_visconnect;
vmCvar_t bot_fps;
//end rww

wpobject_t* flagRed;
wpobject_t* oFlagRed;
wpobject_t* flagBlue;
wpobject_t* oFlagBlue;

gentity_t* eFlagRed;
gentity_t* droppedRedFlag;
gentity_t* eFlagBlue;
gentity_t* droppedBlueFlag;

char* ctfStateNames[] = {
	"CTFSTATE_NONE",
	"CTFSTATE_ATTACKER",
	"CTFSTATE_DEFENDER",
	"CTFSTATE_RETRIEVAL",
	"CTFSTATE_GUARDCARRIER",
	"CTFSTATE_GETFLAGHOME",
	"CTFSTATE_MAXCTFSTATES"
};

char* ctfStateDescriptions[] = {
	"I'm not occupied",
	"I'm attacking the enemy's base",
	"I'm defending our base",
	"I'm getting our flag back",
	"I'm escorting our flag carrier",
	"I've got the enemy's flag"
};

char* siegeStateDescriptions[] = {
	"I'm not occupied",
	"I'm attempting to complete the current objective",
	"I'm preventing the enemy from completing their objective"
};

char* teamplayStateDescriptions[] = {
	"I'm not occupied",
	"I'm following my squad commander",
	"I'm assisting my commanding",
	"I'm attempting to regroup and form a new squad"
};

#define DEFEND_MINDISTANCE	200

int force_jump_needed(vec3_t startvect, vec3_t endvect)
{
	vec3_t tempstart;
	vec3_t tempend;

	const float heightdif = endvect[2] - startvect[2];

	VectorCopy(startvect, tempstart);
	VectorCopy(endvect, tempend);

	tempend[2] = tempstart[2] = 0;
	const float lengthdif = Distance(tempstart, tempend);

	if (heightdif < 128 && lengthdif < 128)
	{
		//no force needed
		return FORCE_LEVEL_0;
	}

	if (heightdif > 512)
	{
		//too high
		return FORCE_LEVEL_4;
	}
	if (heightdif > 350 || lengthdif > 700)
	{
		return FORCE_LEVEL_3;
	}
	if (heightdif > 256 || lengthdif > 512)
	{
		return FORCE_LEVEL_2;
	}
	return FORCE_LEVEL_1;
}

//the amount of Force Power you'll need for a given Force Jump level
int ForcePowerforJump[NUM_FORCE_POWER_LEVELS] =
{
	0,
	30,
	30,
	30
};

int MinimumAttackDistance[WP_NUM_WEAPONS] =
{
	0, //WP_NONE,
	0, //WP_STUN_BATON,
	30, //WP_MELEE,
	60, //WP_SABER,
	200, //WP_BRYAR_PISTOL,
	0, //WP_BLASTER,
	0, //WP_DISRUPTOR,
	0, //WP_BOWCASTER,
	0, //WP_REPEATER,
	0, //WP_DEMP2,
	0, //WP_FLECHETTE,
	100, //WP_ROCKET_LAUNCHER,
	100, //WP_THERMAL,
	100, //WP_TRIP_MINE,
	0, //WP_DET_PACK,
	0, //WP_CONCUSSION,
	0, //WP_BRYAR_OLD,
	0, //WP_EMPLACED_GUN,
	0 //WP_TURRET,
	//WP_NUM_WEAPONS
};

int MaximumAttackDistance[WP_NUM_WEAPONS] =
{
	9999, //WP_NONE,
	0, //WP_STUN_BATON,
	100, //WP_MELEE,
	160, //WP_SABER,
	9999, //WP_BRYAR_PISTOL,
	9999, //WP_BLASTER,
	9999, //WP_DISRUPTOR,
	9999, //WP_BOWCASTER,
	9999, //WP_REPEATER,
	9999, //WP_DEMP2,
	9999, //WP_FLECHETTE,
	9999, //WP_ROCKET_LAUNCHER,
	9999, //WP_THERMAL,
	9999, //WP_TRIP_MINE,
	9999, //WP_DET_PACK,
	9999, //WP_CONCUSSION,
	9999, //WP_BRYAR_OLD,
	9999, //WP_EMPLACED_GUN,
	9999 //WP_TURRET,
	//WP_NUM_WEAPONS
};

int IdealAttackDistance[WP_NUM_WEAPONS] =
{
	0, //WP_NONE,
	0, //WP_STUN_BATON,
	40, //WP_MELEE,
	80, //WP_SABER,
	350, //WP_BRYAR_PISTOL,
	1000, //WP_BLASTER,
	1000, //WP_DISRUPTOR,
	1000, //WP_BOWCASTER,
	1000, //WP_REPEATER,
	1000, //WP_DEMP2,
	1000, //WP_FLECHETTE,
	1000, //WP_ROCKET_LAUNCHER,
	1000, //WP_THERMAL,
	1000, //WP_TRIP_MINE,
	1000, //WP_DET_PACK,
	1000, //WP_CONCUSSION,
	1000, //WP_BRYAR_OLD,
	1000, //WP_EMPLACED_GUN,
	1000 //WP_TURRET,
	//WP_NUM_WEAPONS
};

//Find the origin location of a given entity
void FindOrigin(const gentity_t* ent, vec3_t origin)
{
	if (!ent->classname)
	{
		//some funky entity, just set vec3_origin
		VectorCopy(vec3_origin, origin);
		return;
	}

	if (ent->client)
	{
		VectorCopy(ent->client->ps.origin, origin);
	}
	else
	{
		if (strcmp(ent->classname, "func_breakable") == 0
			|| strcmp(ent->classname, "trigger_multiple") == 0
			|| strcmp(ent->classname, "trigger_once") == 0
			|| strcmp(ent->classname, "func_usable") == 0)
		{
			//func_breakable's don't have normal origin data
			origin[0] = (ent->r.absmax[0] + ent->r.absmin[0]) / 2;
			origin[1] = (ent->r.absmax[1] + ent->r.absmin[1]) / 2;
			origin[2] = (ent->r.absmax[2] + ent->r.absmin[2]) / 2;
		}
		else
		{
			VectorCopy(ent->r.currentOrigin, origin);
		}
	}
}

//find angles/viewangles for entity
void FindAngles(const gentity_t* ent, vec3_t angles)
{
	if (ent->client)
	{
		//player
		VectorCopy(ent->client->ps.viewangles, angles);
	}
	else
	{
		//other stuff
		VectorCopy(ent->s.angles, angles);
	}
}

float TargetDistance(const bot_state_t* bs, const gentity_t* target, vec3_t targetorigin)
{
	if (strcmp(target->classname, "misc_siege_item") == 0
		|| strcmp(target->classname, "func_breakable") == 0
		|| target->client && target->client->NPC_class == CLASS_RANCOR)
	{
		vec3_t enemy_origin;
		//flatten origin heights and measure
		VectorCopy(targetorigin, enemy_origin);
		if (fabs(enemy_origin[2] - bs->eye[2]) < 150)
		{
			//don't flatten unless you're on the same relative plane
			enemy_origin[2] = bs->eye[2];
		}

		if (target->client && target->client->NPC_class == CLASS_RANCOR)
		{
			//Rancors are big and stuff
			return Distance(bs->eye, enemy_origin) - 60;
		}
		if (strcmp(target->classname, "misc_siege_item") == 0)
		{
			//assume this is a misc_siege_item.  These have absolute based mins/maxs.
			//Scale for the entity's bounding box
			float adjustor;
			const float x = fabs(bs->eye[0] - enemy_origin[0]);
			const float y = fabs(bs->eye[1] - enemy_origin[1]);
			const float z = fabs(bs->eye[2] - enemy_origin[2]);

			//find the general direction of the impact to determine which bbox length to
			//scale with
			if (x > y && x > z)
			{
				//x
				adjustor = target->r.maxs[0];
			}
			else if (y > x && y > z)
			{
				//y
				adjustor = target->r.maxs[1];
			}
			else
			{
				//z
				adjustor = target->r.maxs[2];
			}

			return Distance(bs->eye, enemy_origin) - adjustor + 15;
		}
		if (strcmp(target->classname, "func_breakable") == 0)
		{
			//Scale for the entity's bounding box
			float adjustor;

			//find the smallest min/max and use that.
			if (target->r.absmax[0] - enemy_origin[0] < target->r.absmax[1] - enemy_origin[1])
			{
				adjustor = target->r.absmax[0] - enemy_origin[0];
			}
			else
			{
				adjustor = target->r.absmax[1] - enemy_origin[1];
			}

			return Distance(bs->eye, enemy_origin) - adjustor + 15;
		}
		//func_breakable
		return Distance(bs->eye, enemy_origin);
	}
	//standard stuff
	return Distance(bs->eye, targetorigin);
}

qboolean visible(const gentity_t* self, const gentity_t* other)
{
	vec3_t spot1;
	vec3_t spot2;
	trace_t tr;

	if (!self->r.currentOrigin || !other->r.currentOrigin)
		return qfalse;

	if (!other->client || !self->client)
		return qfalse;

	VectorCopy(self->r.currentOrigin, spot1);
	spot1[2] += 48;

	VectorCopy(other->r.currentOrigin, spot2);
	spot2[2] += 48;

	// And a standard pass..
	trap->Trace(&tr, spot1, NULL, NULL, other->r.currentOrigin, self->s.number, MASK_SHOT, qfalse, 0, 0);

	const gentity_t* traceEnt = &g_entities[tr.entityNum];

	if (traceEnt == other)
		return qtrue;

	return qfalse;
}

gentity_t* FindClosestHumanPlayer(vec3_t position, int enemyTeam)
{
	float bestdist = 9999;
	gentity_t* closestplayer = NULL;
	for (int i = 0; i < MAX_CLIENTS; i++)
	{
		gentity_t* player = &g_entities[i];
		if (!player || !player->client || !player->inuse)
		{
			//player not active
			continue;
		}

		if (player->r.svFlags & SVF_BOT)
		{
			//player bot.
			continue;
		}

		if (player->client->playerTeam != enemyTeam)
		{
			//this player isn't on the team I hate.
			continue;
		}

		if (player->health <= 0 || player->s.eFlags & EF_DEAD)
		{
			//player is dead, don t use
			continue;
		}

		const float dist = Distance(player->client->ps.origin, position);
		if (dist < bestdist)
		{
			closestplayer = player;
			bestdist = dist;
		}
	}

	return closestplayer;
}

float vector_distancebot(vec3_t v1, vec3_t v2)
{
	vec3_t dir;

	VectorSubtract(v2, v1, dir);
	return VectorLength(dir);
}

qboolean CheckFall_By_Vectors(vec3_t origin, vec3_t angles, const gentity_t* ent)
{
	// Check a little in front of us.
	trace_t tr;
	vec3_t down, flatAng, fwd, use;

	if (last_checkfall[ent->s.number] > level.time - 250) // Only check every 1/4 sec.
		return qtrue; //lastresult[ent->s.number]; // To speed things up.

	last_checkfall[ent->s.number] = level.time;

	VectorCopy(angles, flatAng);
	flatAng[PITCH] = 0;

	AngleVectors(flatAng, fwd, 0, 0);

	use[0] = origin[0] + fwd[0] * 96;
	use[1] = origin[1] + fwd[1] * 96;
	use[2] = origin[2] + fwd[2] * 96;

	VectorCopy(use, down);

	down[2] -= 4096;

	trap->Trace(&tr, use, ent->r.mins, ent->r.maxs, down, ent->s.clientNum, MASK_SOLID, qfalse, 0, 0);
	// Look for ground.

	VectorSubtract(use, tr.endpos, down);

	if (down[2] >= 1000)
		return qtrue; // Long way down!

	use[0] = origin[0] + fwd[0] * 64;
	use[1] = origin[1] + fwd[1] * 64;
	use[2] = origin[2] + fwd[2] * 64;

	VectorCopy(use, down);

	down[2] -= 4096;

	trap->Trace(&tr, use, ent->r.mins, ent->r.maxs, down, ent->s.clientNum, MASK_SOLID, qfalse, 0, 0);
	// Look for ground.

	VectorSubtract(use, tr.endpos, down);

	if (down[2] >= 1000)
		return qtrue; // Long way down!

	use[0] = origin[0] + fwd[0] * 32;
	use[1] = origin[1] + fwd[1] * 32;
	use[2] = origin[2] + fwd[2] * 32;

	VectorCopy(use, down);

	down[2] -= 4096;

	trap->Trace(&tr, use, ent->r.mins, ent->r.maxs, down, ent->s.clientNum, MASK_SOLID, qfalse, 0, 0);
	// Look for ground.

	VectorSubtract(use, tr.endpos, down);

	if (down[2] >= 1000)
		return qtrue; // Long way down!

	return qfalse; // All is ok!
}

void Set_Enemy_Path(bot_state_t* bs)
{
	// For aborted jumps.
	vec3_t fwd, b_angle;

	bs->goalAngles[PITCH] = 0;
	bs->goalAngles[ROLL] = 0;

	VectorCopy(bs->goalAngles, b_angle);

	AngleVectors(b_angle, fwd, NULL, NULL);

	if (!bs->currentEnemy // No enemy!
		|| CheckFall_By_Vectors(bs->origin, fwd, &g_entities[bs->entitynum]) == qtrue) // We're gonna fall!!!
	{
		if (bs->wpCurrent)
		{
			// Fall back to the old waypoint info... Should be already set.
		}
		else
		{
			bs->beStill = level.time + 1000;
		}
	}
	else
	{
		// Set enemy as our goal.
		VectorCopy(bs->currentEnemy->r.currentOrigin, bs->goalPosition);
	}
}

void AIMod_Jump(bot_state_t* bs)
{
	vec3_t dir, p1, p2, apex;
	float time, height, forward, z, xy, dist;
	float apexHeight;
	gentity_t* bot = &g_entities[bs->entitynum];

	if (!bs->currentEnemy)
	{
		bs->BOTjumpState = JS_WAITING;
		return;
	}

	const gentity_t* target = &g_entities[bs->currentEnemy->client->ps.clientNum]; // We NEED a valid enemy!

	if (!target)
	{
		//Should have task completed the navgoal
		bs->BOTjumpState = JS_WAITING;
		return;
	}

	//We don't really care about pitch here
	switch (bs->BOTjumpState)
	{
	case JS_FACING:
	case JS_CROUCHING:
		if (bs->cur_ps.stats[STAT_HOLDABLE_ITEMS] & 1 << HI_JETPACK)
		{
			// Jetpacker.. Jetpack ON!
			bs->cur_ps.eFlags |= EF_JETPACK_ACTIVE;
			bs->cur_ps.eFlags |= EF_JETPACK_FLAMING; //going up
			bs->jumpHoldTime = (bs->forceJumpChargeTime + level.time) / 2 + 50000;
		}

		if (bot->r.currentOrigin[2] > jumpPos[bot->s.number][2])
		{
			VectorCopy(bot->r.currentOrigin, p1);
			VectorCopy(jumpPos[bot->s.number], p2);
		}
		else if (bot->r.currentOrigin[2] < jumpPos[bot->s.number][2])
		{
			VectorCopy(jumpPos[bot->s.number], p1);
			VectorCopy(bot->r.currentOrigin, p2);
		}
		else
		{
			VectorCopy(bot->r.currentOrigin, p1);
			VectorCopy(jumpPos[bot->s.number], p2);
		}

		VectorSubtract(p2, p1, dir);
		dir[2] = 0;

		xy = VectorNormalize(dir);

		z = p1[2] - p2[2];

		if (xy >= 600)
		{
			bs->BOTjumpState = JS_WAITING;
			return;
		}

		apexHeight = APEX_HEIGHT / 2;
		z = sqrt(apexHeight + z) - sqrt(apexHeight);

		assert(z >= 0);

		xy -= z;
		xy *= 0.5;

		if (xy <= 0)
			xy = 1.0f;

		VectorMA(p1, xy, dir, apex);

		apex[2] += apexHeight;

		VectorCopy(apex, bot->pos1);

		//Now we have the apex, aim for it

		height = apex[2] - bot->r.currentOrigin[2];

		time = sqrt(height / (.5 * bot->client->ps.gravity));
		if (!time)
		{
			dist = vector_distancebot(jumpPos[bot->s.number], bot->r.currentOrigin);
			if (dist > 1200)
			{
				bs->BOTjumpState = JS_WAITING;
				Set_Enemy_Path(bs);
				return;
			}

			height = dist;

			time = sqrt(height / (.5 * bot->client->ps.gravity));
		}

		// set s.origin2 to the push velocity
		VectorSubtract(apex, bot->r.currentOrigin, bot->client->ps.velocity);
		bot->client->ps.velocity[2] = 0;
		dist = VectorNormalize(bot->client->ps.velocity);

		forward = dist / time;

		VectorScale(bot->client->ps.velocity, forward, bot->client->ps.velocity);

		bot->client->ps.velocity[2] = time * bot->client->ps.gravity;

		bot->flags |= FL_NO_KNOCKBACK;
		bs->BOTjumpState = JS_JUMPING;
		break;
	case JS_JUMPING:
		if (bs->cur_ps.stats[STAT_HOLDABLE_ITEMS] & 1 << HI_JETPACK)
		{
			// Jetpacker.. Jetpack ON!
			bs->cur_ps.eFlags |= EF_JETPACK_ACTIVE;
			bs->cur_ps.eFlags |= EF_JETPACK_FLAMING; //going up
			bs->jumpHoldTime = (bs->forceJumpChargeTime + level.time) / 2 + 50000;
		}
		if (bot->r.currentOrigin[0] == jumpPos[bot->s.number][0]
			&& bot->r.currentOrigin[1] == jumpPos[bot->s.number][1])
		{
			//Let's land!
			//FIXME: if the
			bot->client->ps.velocity[0] *= 0.5;
			bot->client->ps.velocity[1] *= 0.5;
			if (bot->client->ps.velocity[2] < 0)
				bot->client->ps.velocity[2] -= -16;
			else
				bot->client->ps.velocity[2] = -16;
		}
		else if (bot->s.groundEntityNum != ENTITYNUM_NONE)
		{
			//Landed, start landing anim
			//FIXME: if the
			VectorClear(bot->client->ps.velocity);
			NPC_SetAnim(bot, SETANIM_BOTH, BOTH_LAND1, SETANIM_FLAG_OVERRIDE | SETANIM_FLAG_HOLD);
			bs->BOTjumpState = JS_LANDING;
			//FIXME: landsound?
		}
		else if (bot->client->ps.legsTimer > 0)
		{
			//Still playing jumping anim
		}
		else
		{
			//still in air, but done with jump anim, play inair anim
			NPC_SetAnim(bot, SETANIM_BOTH, BOTH_INAIR1, SETANIM_FLAG_OVERRIDE);
		}
		break;
	case JS_LANDING:
		if (bs->cur_ps.stats[STAT_HOLDABLE_ITEMS] & 1 << HI_JETPACK && Q_irand(0, 5) < 2)
		{
			// Jetpacker.. Jetpack ON!
			bs->cur_ps.eFlags |= EF_JETPACK_ACTIVE;
			bs->cur_ps.eFlags |= EF_JETPACK_FLAMING;
		}

		if (bot->client->ps.legsTimer > 0)
		{
			//Still playing landing anim
			return;
		}
		bs->BOTjumpState = JS_WAITING;
		bot->client->pers.cmd.forwardmove = 0;
		bot->flags &= ~FL_NO_KNOCKBACK;
		break;
	case JS_WAITING:
		if (bs->cur_ps.stats[STAT_HOLDABLE_ITEMS] & 1 << HI_JETPACK)
		{
			// Jetpacker.. Jetpack ON!
			bs->cur_ps.eFlags |= EF_JETPACK_ACTIVE;
			bs->cur_ps.eFlags |= EF_JETPACK_HOVER;
			bs->cur_ps.eFlags |= EF_JETPACK_FLAMING;
			bs->jumpHoldTime = (bs->forceJumpChargeTime + level.time) / 2 + 50000;
		}
		break;
	default:
		if (bs->cur_ps.stats[STAT_HOLDABLE_ITEMS] & 1 << HI_JETPACK)
		{
			// Jetpacker.. Jetpack ON!
			bs->cur_ps.eFlags |= EF_JETPACK_ACTIVE;
			bs->cur_ps.eFlags |= EF_JETPACK_FLAMING; //going up
			bs->cur_ps.eFlags |= EF_JETPACK_HOVER;
			bs->jumpHoldTime = (bs->forceJumpChargeTime + level.time) / 2 + 50000;
		}
		bs->BOTjumpState = JS_FACING;
		break;
	}
}

void BotStraightTPOrderCheck(gentity_t* ent, const int ordernum, bot_state_t* bs)
{
	switch (ordernum)
	{
	case 0:
		if (bs->squadLeader == ent)
		{
			bs->teamplayState = 0;
			bs->squadLeader = NULL;
		}
		break;
	case TEAMPLAYSTATE_FOLLOWING:
		bs->teamplayState = ordernum;
		bs->isSquadLeader = 0;
		bs->squadLeader = ent;
		bs->wpDestSwitchTime = 0;
		break;
	case TEAMPLAYSTATE_ASSISTING:
		bs->teamplayState = ordernum;
		bs->isSquadLeader = 0;
		bs->squadLeader = ent;
		bs->wpDestSwitchTime = 0;
		break;
	default:
		bs->teamplayState = ordernum;
		break;
	}
}

void QDECL BotAI_Print(int type, char* fmt, ...)
{
}

void BotSelectWeapon(const int client, const int weapon)
{
	if (weapon <= WP_NONE)
	{
		return;
	}
	trap->EA_SelectWeapon(client, weapon);
}

void BotReportStatus(const bot_state_t* bs)
{
	if (level.gametype == GT_TEAM)
	{
		trap->EA_SayTeam(bs->client, teamplayStateDescriptions[bs->teamplayState]);
	}
	else if (level.gametype == GT_SIEGE)
	{
		trap->EA_SayTeam(bs->client, siegeStateDescriptions[bs->siegeState]);
	}
	else if (level.gametype == GT_CTF || level.gametype == GT_CTY)
	{
		trap->EA_SayTeam(bs->client, ctfStateDescriptions[bs->ctfState]);
	}
}

//accept a team order from a player
void BotOrder(gentity_t* ent, const int clientnum, const int ordernum)
{
	int stateMin = 0;
	int stateMax = 0;

	if (!ent || !ent->client || !ent->client->sess.teamLeader)
	{
		return;
	}

	if (clientnum != -1 && !botstates[clientnum])
	{
		return;
	}

	if (clientnum != -1 && !OnSameTeam(ent, &g_entities[clientnum]))
	{
		return;
	}

	if (level.gametype != GT_CTF && level.gametype != GT_CTY && level.gametype != GT_SIEGE && level.gametype != GT_TEAM)
	{
		return;
	}

	if (level.gametype == GT_CTF || level.gametype == GT_CTY)
	{
		stateMin = CTFSTATE_NONE;
		stateMax = CTFSTATE_MAXCTFSTATES;
	}
	else if (level.gametype == GT_SIEGE)
	{
		stateMin = SIEGESTATE_NONE;
		stateMax = SIEGESTATE_MAXSIEGESTATES;
	}
	else if (level.gametype == GT_TEAM)
	{
		stateMin = TEAMPLAYSTATE_NONE;
		stateMax = TEAMPLAYSTATE_MAXTPSTATES;
	}

	if (ordernum < stateMin && ordernum != -1 || ordernum >= stateMax)
	{
		return;
	}

	if (clientnum != -1)
	{
		if (ordernum == -1)
		{
			BotReportStatus(botstates[clientnum]);
		}
		else
		{
			BotStraightTPOrderCheck(ent, ordernum, botstates[clientnum]);
			botstates[clientnum]->state_Forced = ordernum;
			botstates[clientnum]->chatObject = ent;
			botstates[clientnum]->chatAltObject = NULL;
			if (BotDoChat(botstates[clientnum], "OrderAccepted", 1))
			{
				botstates[clientnum]->chatTeam = 1;
			}
		}
	}
	else
	{
		int i = 0;
		while (i < MAX_CLIENTS)
		{
			if (botstates[i] && OnSameTeam(ent, &g_entities[i]))
			{
				if (ordernum == -1)
				{
					BotReportStatus(botstates[i]);
				}
				else
				{
					BotStraightTPOrderCheck(ent, ordernum, botstates[i]);
					botstates[i]->state_Forced = ordernum;
					botstates[i]->chatObject = ent;
					botstates[i]->chatAltObject = NULL;
					if (BotDoChat(botstates[i], "OrderAccepted", 0))
					{
						botstates[i]->chatTeam = 1;
					}
				}
			}

			i++;
		}
	}
}

//See if bot is mind tricked by the client in question
int BotMindTricked(const int bot_client, const int enemyClient)
{
	if (!g_entities[enemyClient].client)
	{
		return 0;
	}

	const forcedata_t* fd = &g_entities[enemyClient].client->ps.fd;

	if (!fd)
	{
		return 0;
	}

	if (bot_client > 47)
	{
		if (fd->forceMindtrickTargetIndex4 & (1 << (bot_client - 48)))
		{
			return 1;
		}
	}
	else if (bot_client > 31)
	{
		if (fd->forceMindtrickTargetIndex3 & (1 << (bot_client - 32)))
		{
			return 1;
		}
	}
	else if (bot_client > 15)
	{
		if (fd->forceMindtrickTargetIndex2 & (1 << (bot_client - 16)))
		{
			return 1;
		}
	}
	else
	{
		if (fd->forceMindtrickTargetIndex & (1 << bot_client))
		{
			return 1;
		}
	}

	return 0;
}

int is_teamplay(void)
{
	if (level.gametype < GT_TEAM)
	{
		return 0;
	}

	return 1;
}

/*
==================
BotAI_GetClientState
==================
*/
int BotAI_GetClientState(const int clientNum, playerState_t* state)
{
	const gentity_t* ent = &g_entities[clientNum];
	if (!ent->inuse)
	{
		return qfalse;
	}
	if (!ent->client)
	{
		return qfalse;
	}

	memcpy(state, &ent->client->ps, sizeof(playerState_t));
	return qtrue;
}

/*
==================
BotAI_GetEntityState
==================
*/
int BotAI_GetEntityState(const int entity_num, entityState_t* state)
{
	const gentity_t* ent = &g_entities[entity_num];
	memset(state, 0, sizeof(entityState_t));
	if (!ent->inuse) return qfalse;
	if (!ent->r.linked) return qfalse;
	if (ent->r.svFlags & SVF_NOCLIENT) return qfalse;
	memcpy(state, &ent->s, sizeof(entityState_t));
	return qtrue;
}

/*
==================
BotAI_GetSnapshotEntity
==================
*/
int BotAI_GetSnapshotEntity(const int clientNum, const int sequence, entityState_t* state)
{
	const int entNum = trap->BotGetSnapshotEntity(clientNum, sequence);
	if (entNum == -1)
	{
		memset(state, 0, sizeof(entityState_t));
		return -1;
	}

	BotAI_GetEntityState(entNum, state);

	return sequence + 1;
}

/*
==============
BotEntityInfo
==============
*/
void BotEntityInfo(const int entnum, aas_entityinfo_t* info)
{
	trap->AAS_EntityInfo(entnum, info);
}

/*
==============
NumBots
==============
*/
int NumBots(void)
{
	return numbots;
}

/*
==============
AngleDifference
==============
*/
float AngleDifference(const float ang1, const float ang2)
{
	float diff = ang1 - ang2;
	if (ang1 > ang2)
	{
		if (diff > 180.0) diff -= 360.0;
	}
	else
	{
		if (diff < -180.0) diff += 360.0;
	}
	return diff;
}

/*
==============
BotChangeViewAngle
==============
*/
float BotChangeViewAngle(float angle, float ideal_angle, float speed)
{
	angle = AngleMod(angle);
	ideal_angle = AngleMod(ideal_angle);
	if (angle == ideal_angle) return angle;
	float move = ideal_angle - angle;
	if (ideal_angle > angle)
	{
		if (move > 180.0) move -= 360.0;
	}
	else
	{
		if (move < -180.0) move += 360.0;
	}
	if (move > 0)
	{
		if (move > speed) move = speed;
	}
	else
	{
		if (move < -speed) move = -speed;
	}
	return AngleMod(angle + move);
}

/*
==============
BotChangeViewAngles
==============
*/
void BotChangeViewAngles(bot_state_t* bs, const float thinktime)
{
	float factor;

	if (bs->ideal_viewangles[PITCH] > 180) bs->ideal_viewangles[PITCH] -= 360;

	if (bs->currentEnemy && bs->frame_Enemy_Vis)
	{
		if (bs->settings.skill <= 1)
		{
			factor = bs->skills.turnspeed_combat * 0.4f * bs->settings.skill;
		}
		else if (bs->settings.skill <= 2)
		{
			factor = bs->skills.turnspeed_combat * 0.6f * bs->settings.skill;
		}
		else if (bs->settings.skill <= 3)
		{
			factor = bs->skills.turnspeed_combat * 0.8f * bs->settings.skill;
		}
		else
		{
			factor = bs->skills.turnspeed_combat * bs->settings.skill;
		}
	}
	else
	{
		factor = bs->skills.turnspeed;
	}

	if (factor > 1)
	{
		factor = 1;
	}
	if (factor < 0.001)
	{
		factor = 0.001f;
	}

	float maxchange = bs->skills.maxturn;

	//if (maxchange < 240) maxchange = 240;
	maxchange *= thinktime;
	for (int i = 0; i < 2; i++)
	{
		bs->viewangles[i] = AngleMod(bs->viewangles[i]);
		bs->ideal_viewangles[i] = AngleMod(bs->ideal_viewangles[i]);
		const float diff = AngleDifference(bs->viewangles[i], bs->ideal_viewangles[i]);
		const float disired_speed = diff * factor;
		bs->viewanglespeed[i] += bs->viewanglespeed[i] - disired_speed;
		if (bs->viewanglespeed[i] > 180) bs->viewanglespeed[i] = maxchange;
		if (bs->viewanglespeed[i] < -180) bs->viewanglespeed[i] = -maxchange;
		float anglespeed = bs->viewanglespeed[i];
		if (anglespeed > maxchange) anglespeed = maxchange;
		if (anglespeed < -maxchange) anglespeed = -maxchange;
		bs->viewangles[i] += anglespeed;
		bs->viewangles[i] = AngleMod(bs->viewangles[i]);
		bs->viewanglespeed[i] *= 0.45f * (1 - factor);
	}
	if (bs->viewangles[PITCH] > 180) bs->viewangles[PITCH] -= 360;
	trap->EA_View(bs->client, bs->viewangles);
}

/*
==============
BotInputToUserCommand
==============
*/
void BotInputToUserCommand(bot_input_t* bi, usercmd_t* ucmd, int delta_angles[3], const int time, const int useTime)
{
	vec3_t angles, forward, right;

	//clear the whole structure
	memset(ucmd, 0, sizeof(usercmd_t));
	//the duration for the user command in milli seconds
	ucmd->serverTime = time;
	//
	if (bi->actionflags & ACTION_DELAYEDJUMP)
	{
		bi->actionflags |= ACTION_JUMP;
		bi->actionflags &= ~ACTION_DELAYEDJUMP;
	}
	//set the buttons
	if (bi->actionflags & ACTION_RESPAWN) ucmd->buttons = BUTTON_ATTACK;
	if (bi->actionflags & ACTION_ATTACK) ucmd->buttons |= BUTTON_ATTACK;
	if (bi->actionflags & ACTION_ALT_ATTACK) ucmd->buttons |= BUTTON_ALT_ATTACK;
	if (bi->actionflags & ACTION_GESTURE) ucmd->buttons |= BUTTON_GESTURE;
	if (bi->actionflags & ACTION_USE) ucmd->buttons |= BUTTON_USE_HOLDABLE;
	if (bi->actionflags & ACTION_WALK) ucmd->buttons |= BUTTON_WALKING;
	if (bi->actionflags & ACTION_FORCEPOWER) ucmd->buttons |= BUTTON_FORCEPOWER;
	if (bi->actionflags & ACTION_KICK) ucmd->buttons |= BUTTON_KICK;
	if (bi->actionflags & ACTION_USE) ucmd->buttons |= BUTTON_BOTUSE;
	if (bi->actionflags & ACTION_BLOCK) ucmd->buttons |= BUTTON_BLOCK;

	if (useTime < level.time && Q_irand(1, 10) < 5)
	{
		//for now just hit use randomly in case there's something useable around
		ucmd->buttons |= BUTTON_BOTUSE;
	}

	if (bi->weapon == WP_NONE)
	{
		bi->weapon = WP_BRYAR_PISTOL;
	}

	//
	ucmd->weapon = bi->weapon;
	//set the view angles
	//NOTE: the ucmd->angles are the angles WITHOUT the delta angles
	ucmd->angles[PITCH] = ANGLE2SHORT(bi->viewangles[PITCH]);
	ucmd->angles[YAW] = ANGLE2SHORT(bi->viewangles[YAW]);
	ucmd->angles[ROLL] = ANGLE2SHORT(bi->viewangles[ROLL]);
	//subtract the delta angles
	for (int j = 0; j < 3; j++)
	{
		const short temp = ucmd->angles[j] - delta_angles[j];
		ucmd->angles[j] = temp;
	}
	//NOTE: movement is relative to the REAL view angles
	//get the horizontal forward and right vector
	//get the pitch in the range [-180, 180]
	if (bi->dir[2]) angles[PITCH] = bi->viewangles[PITCH];
	else angles[PITCH] = 0;
	angles[YAW] = bi->viewangles[YAW];
	angles[ROLL] = 0;
	AngleVectors(angles, forward, right, NULL);

	//bot input speed is in the range [0, 400]
	bi->speed = bi->speed * 127 / 400;

	//set the view independent movement
	float f = DotProduct(forward, bi->dir);
	float r = DotProduct(right, bi->dir);
	float u = fabs(forward[2]) * bi->dir[2];
	float m = fabs(f);

	if (fabs(r) > m)
	{
		m = fabs(r);
	}

	if (fabs(u) > m)
	{
		m = fabs(u);
	}

	if (m > 0)
	{
		f *= bi->speed / m;
		r *= bi->speed / m;
		u *= bi->speed / m;
	}

	ucmd->forwardmove = f;
	ucmd->rightmove = r;
	ucmd->upmove = u;
	//normal keyboard movement
	if (bi->actionflags & ACTION_MOVEFORWARD) ucmd->forwardmove = 127;
	if (bi->actionflags & ACTION_MOVEBACK) ucmd->forwardmove = -127;
	if (bi->actionflags & ACTION_MOVELEFT) ucmd->rightmove = -127;
	if (bi->actionflags & ACTION_MOVERIGHT) ucmd->rightmove = 127;
	//jump/moveup
	if (bi->actionflags & ACTION_JUMP) ucmd->upmove = 127;
	//crouch/movedown
	if (bi->actionflags & ACTION_CROUCH) ucmd->upmove = -127;

	if (bi->actionflags & ACTION_WALK)
	{
		if (ucmd->forwardmove > 46)
		{
			ucmd->forwardmove = 46;
		}
		else if (ucmd->forwardmove < -46)
		{
			ucmd->forwardmove = -46;
		}

		if (ucmd->rightmove > 46)
		{
			ucmd->rightmove = 46;
		}
		else if (ucmd->rightmove < -46)
		{
			ucmd->rightmove = -46;
		}
	}
	ucmd->forcesel = bi->forcesel;
}

/*
==============
BotUpdateInput
==============
*/
void BotUpdateInput(bot_state_t* bs, const int time, const int elapsed_time)
{
	bot_input_t bi;
	int j;

	//add the delta angles to the bot's current view angles
	for (j = 0; j < 3; j++)
	{
		bs->viewangles[j] = AngleMod(bs->viewangles[j] + SHORT2ANGLE(bs->cur_ps.delta_angles[j]));
	}
	//change the bot view angles

	if (bs->cur_ps.weapon == WP_SABER)
	{
		BotChangeViewAngles(bs, (float)2 * elapsed_time / 1000);
	}
	else
	{
		BotChangeViewAngles(bs, (float)elapsed_time / 1000);
	}
	//retrieve the bot input
	trap->EA_GetInput(bs->client, (float)time / 1000, &bi);
	//respawn hack
	if (bi.actionflags & ACTION_RESPAWN)
	{
		if (bs->lastucmd.buttons & BUTTON_ATTACK) bi.actionflags &= ~(ACTION_RESPAWN | ACTION_ATTACK);
	}

	if (bs->cur_ps.weapon == WP_SABER)
	{
		if (bs->currentEnemy
			&& bs->currentEnemy->client
			&& bs->currentEnemy->health > 0
			&& bs->jumpTime <= level.time // Don't walk during jumping...
			&& (VectorDistance(g_entities[bs->cur_ps.clientNum].r.currentOrigin, bs->currentEnemy->r.currentOrigin) < 300 || walktime[bs->cur_ps.clientNum] > level.time))
		{
			if (visible(&g_entities[bs->cur_ps.clientNum], bs->currentEnemy) || walktime[bs->cur_ps.clientNum] > level.
				time)
			{
				bi.actionflags |= ACTION_WALK;
				walktime[bs->cur_ps.clientNum] = level.time + 2000;
			}
			else
			{
				// Reset.
				walktime[bs->cur_ps.clientNum] = 0;
			}
		}
		else
		{
			// Reset.
			walktime[bs->cur_ps.clientNum] = 0;
		}
	}

	if (gesturetime[bs->cur_ps.clientNum] <= level.time && bot_thinklevel.integer >= 0)
	{
		if (bs->currentEnemy
			&& bs->currentEnemy->client
			&& bs->currentEnemy->health > 0
			&& bs->jumpTime <= level.time // Don't gesture during jumping...
			&& !bs->doAttack
			&& !bs->doAltAttack
			&& bs->currentEnemy->client->ps.groundEntityNum != ENTITYNUM_NONE
			&& VectorDistance(g_entities[bs->cur_ps.clientNum].r.currentOrigin, bs->currentEnemy->r.currentOrigin) < 300
			|| gesturetime[bs->cur_ps.clientNum] > level.time)
		{
			if (visible(&g_entities[bs->cur_ps.clientNum], bs->currentEnemy) || gesturetime[bs->cur_ps.clientNum] > level.time)
			{
				const int saberRange = SABER_ATTACK_RANGE;

				if (bs->frame_Enemy_Len <= saberRange)
				{
					bi.actionflags |= ACTION_KICK;
				}
				else
				{
					bi.actionflags |= ACTION_GESTURE;
				}

				int check_val = bot_thinklevel.integer;

				if (check_val <= 0)
					check_val = 1;

				gesturetime[bs->cur_ps.clientNum] = level.time + 20000 / check_val;
			}
			else
			{
				// Reset.
				gesturetime[bs->cur_ps.clientNum] = 0;
			}
		}
		else
		{
			// Reset.
			gesturetime[bs->cur_ps.clientNum] = 0;
		}
	}

	if (bs->doWalk)
	{
		bi.actionflags |= ACTION_WALK;
	}
	if (bs->doBotKick)
	{
		bi.actionflags |= ACTION_KICK;
	}
	/*if (bs->lastucmd.buttons & BUTTON_KICK)
	{
		bi.actionflags |= ACTION_KICK;
	}*/
	if (bs->doBotGesture)
	{
		bi.actionflags |= ACTION_GESTURE;
	}
	if (bs->doBotBlock && bs->cur_ps.weapon == WP_SABER)
	{
		bi.actionflags |= ACTION_BLOCK;
	}

	const int curmove = g_entities[bs->client].client->ps.saberMove;
	if (PM_SaberInStart(curmove) || PM_SaberInTransition(curmove))
	{
		bi.actionflags |= ACTION_ATTACK;
	}

	//set up forcesel.  Doesn't use cur_ps, since cur_ps is just a copy of the real ps.
	bi.forcesel = level.clients[bs->client].ps.fd.forcePowerSelected;
	//convert the bot input to a usercmd
	BotInputToUserCommand(&bi, &bs->lastucmd, bs->cur_ps.delta_angles, time, bs->noUseTime);
	//subtract the delta angles
	for (j = 0; j < 3; j++)
	{
		bs->viewangles[j] = AngleMod(bs->viewangles[j] - SHORT2ANGLE(bs->cur_ps.delta_angles[j]));
	}
}

/*
==============
BotAIRegularUpdate
==============
*/
void BotAIRegularUpdate(void)
{
	if (regularupdate_time < FloatTime())
	{
		trap->BotUpdateEntityItems();
		regularupdate_time = FloatTime() + 0.3;
	}
}

/*
==============
RemoveColorEscapeSequences
==============
*/
void RemoveColorEscapeSequences(char* text)
{
	int l = 0;
	for (int i = 0; text[i]; i++)
	{
		if (Q_IsColorStringExt(&text[i]))
		{
			i++;
			continue;
		}
		if (text[i] > 0x7E)
			continue;
		text[l++] = text[i];
	}
	text[l] = '\0';
}

/*
==============
BotAI
==============
*/
extern vmCvar_t bot_cpu_usage;

int BotAI(int client, const float thinktime)
{
	bot_state_t* bs;
	char buf[1024], * args;
	int j;
#ifdef _DEBUG
	int start;
	int end;
#endif

	trap->EA_ResetInput(client);
	//
	bs = botstates[client];
	if (!bs || !bs->inuse)
	{
		BotAI_Print(PRT_FATAL, "BotAI: client %d is not setup\n", client);
		return qfalse;
	}

	//retrieve the current client state
	BotAI_GetClientState(client, &bs->cur_ps);

	//retrieve any waiting server commands
	while (trap->BotGetServerCommand(client, buf, sizeof buf))
	{
		//have buf point to the command and args to the command arguments
		args = strchr(buf, ' ');
		if (!args) continue;
		*args++ = '\0';

		//remove color espace sequences from the arguments
		RemoveColorEscapeSequences(args);

		if (!Q_stricmp(buf, "cp "))
		{
			/*CenterPrintf*/
		}
		else if (!Q_stricmp(buf, "cs"))
		{
			/*ConfigStringModified*/
		}
		else if (!Q_stricmp(buf, "scores"))
		{
			/*FIXME: parse scores?*/
		}
		else if (!Q_stricmp(buf, "clientLevelShot"))
		{
			/*ignore*/
		}
	}
	//add the delta angles to the bot's current view angles
	for (j = 0; j < 3; j++)
	{
		bs->viewangles[j] = AngleMod(bs->viewangles[j] + SHORT2ANGLE(bs->cur_ps.delta_angles[j]));
	}
	//increase the local time of the bot
	bs->ltime += thinktime;
	//
	bs->thinktime = thinktime;
	//origin of the bot
	VectorCopy(bs->cur_ps.origin, bs->origin);
	//eye coordinates of the bot
	VectorCopy(bs->cur_ps.origin, bs->eye);
	bs->eye[2] += bs->cur_ps.viewheight;
	//get the area the bot is in

#ifdef _DEBUG
	start = trap->Milliseconds();
#endif
	StandardBotAI(bs, thinktime);
	bs->full_thinktime = level.time + (100 - bot_cpu_usage.integer);
#ifdef _DEBUG
	end = trap->Milliseconds();

	trap->Cvar_Update(&bot_debugmessages);

	if (bot_debugmessages.integer)
	{
		Com_Printf("Single AI frametime: %i\n", end - start);
	}
#endif

	//subtract the delta angles
	for (j = 0; j < 3; j++)
	{
		bs->viewangles[j] = AngleMod(bs->viewangles[j] - SHORT2ANGLE(bs->cur_ps.delta_angles[j]));
	}
	//everything was ok
	return qtrue;
}

/*
==================
BotScheduleBotThink
==================
*/
void BotScheduleBotThink(void)
{
	int botnum = 0;

	for (int i = 0; i < MAX_CLIENTS; i++)
	{
		if (!botstates[i] || !botstates[i]->inuse)
		{
			continue;
		}
		//initialize the bot think residual time
		botstates[i]->botthink_residual = BOT_THINK_TIME * botnum / numbots;
		botnum++;
	}
}

int PlayersInGame(void)
{
	int i = 0;
	int pl = 0;

	while (i < MAX_CLIENTS)
	{
		const gentity_t* ent = &g_entities[i];

		if (ent && ent->client && ent->client->pers.connected == CON_CONNECTED)
		{
			pl++;
		}

		i++;
	}

	return pl;
}

/*
==============
BotAISetupClient
==============
*/
int BotAISetupClient(int client, const struct bot_settings_s* settings, qboolean restart)
{
	if (!botstates[client])
	{
		botstates[client] = (bot_state_t*)B_Alloc(sizeof(bot_state_t));
	}

	memset(botstates[client], 0, sizeof(bot_state_t));

	bot_state_t* bs = botstates[client];

	if (bs && bs->inuse)
	{
		BotAI_Print(PRT_FATAL, "BotAISetupClient: client %d already setup\n", client);
		return qfalse;
	}

	memcpy(&bs->settings, settings, sizeof(bot_settings_t));

	bs->client = client; //need to know the client number before doing personality stuff

	//initialize weapon weight defaults..
	bs->botWeaponWeights[WP_NONE] = 0;
	bs->botWeaponWeights[WP_STUN_BATON] = 1;
	bs->botWeaponWeights[WP_MELEE] = 1;
	bs->botWeaponWeights[WP_SABER] = 10;
	bs->botWeaponWeights[WP_BRYAR_PISTOL] = 11;
	bs->botWeaponWeights[WP_BLASTER] = 12;
	bs->botWeaponWeights[WP_DISRUPTOR] = 13;
	bs->botWeaponWeights[WP_BOWCASTER] = 14;
	bs->botWeaponWeights[WP_REPEATER] = 15;
	bs->botWeaponWeights[WP_DEMP2] = 16;
	bs->botWeaponWeights[WP_FLECHETTE] = 17;
	bs->botWeaponWeights[WP_ROCKET_LAUNCHER] = 18;
	bs->botWeaponWeights[WP_THERMAL] = 14;
	bs->botWeaponWeights[WP_TRIP_MINE] = 0;
	bs->botWeaponWeights[WP_DET_PACK] = 0;
	bs->botWeaponWeights[WP_CONCUSSION] = 15;
	bs->botWeaponWeights[WP_BRYAR_OLD] = 19;

	BotUtilizePersonality(bs);

	if (level.gametype == GT_DUEL || level.gametype == GT_POWERDUEL)
	{
		bs->botWeaponWeights[WP_SABER] = 13;
	}

	//allocate a goal state
	bs->gs = trap->BotAllocGoalState(client);

	//allocate a weapon state
	bs->ws = trap->BotAllocWeaponState();

	bs->inuse = qtrue;
	bs->entitynum = client;
	bs->setupcount = 4;
	bs->entergame_time = FloatTime();
	bs->ms = trap->BotAllocMoveState();
	numbots++;

	//NOTE: reschedule the bot thinking
	BotScheduleBotThink();

	if (PlayersInGame())
	{
		//don't talk to yourself
		BotDoChat(bs, "GeneralGreetings", 0);
	}

	return qtrue;
}

/*
==============
BotAIShutdownClient
==============
*/
int BotAIShutdownClient(const int client, qboolean restart)
{
	bot_state_t* bs = botstates[client];
	if (!bs || !bs->inuse)
	{
		//BotAI_Print(PRT_ERROR, "BotAIShutdownClient: client %d already shutdown\n", client);
		return qfalse;
	}

	trap->BotFreeMoveState(bs->ms);
	//free the goal state`
	trap->BotFreeGoalState(bs->gs);
	//free the weapon weights
	trap->BotFreeWeaponState(bs->ws);
	//
	//clear the bot state
	memset(bs, 0, sizeof(bot_state_t));
	//set the inuse flag to qfalse
	bs->inuse = qfalse;
	//there's one bot less
	numbots--;
	//everything went ok
	return qtrue;
}

/*
==============
BotResetState

called when a bot enters the intermission or observer mode and
when the level is changed
==============
*/
void BotResetState(bot_state_t* bs)
{
	bot_settings_t settings;
	playerState_t ps; //current player state

	//save some things that should not be reset here
	memcpy(&settings, &bs->settings, sizeof(bot_settings_t));
	memcpy(&ps, &bs->cur_ps, sizeof(playerState_t));
	const int inuse = bs->inuse;
	const int client = bs->client;
	const int entitynum = bs->entitynum;
	const int movestate = bs->ms;
	const int goalstate = bs->gs;
	const int weaponstate = bs->ws;
	const float entergame_time = bs->entergame_time;
	//reset the whole state
	memset(bs, 0, sizeof(bot_state_t));
	//copy back some state stuff that should not be reset
	bs->ms = movestate;
	bs->gs = goalstate;
	bs->ws = weaponstate;
	memcpy(&bs->cur_ps, &ps, sizeof(playerState_t));
	memcpy(&bs->settings, &settings, sizeof(bot_settings_t));
	bs->inuse = inuse;
	bs->client = client;
	bs->entitynum = entitynum;
	bs->entergame_time = entergame_time;
	//reset several states
	if (bs->ms) trap->BotResetMoveState(bs->ms);
	if (bs->gs) trap->BotResetGoalState(bs->gs);
	if (bs->ws) trap->BotResetWeaponState(bs->ws);
	if (bs->gs) trap->BotResetAvoidGoals(bs->gs);
	if (bs->ms) trap->BotResetAvoidReach(bs->ms);
}

/*
==============
BotAILoadMap
==============
*/
int BotAILoadMap(int restart)
{
	for (int i = 0; i < MAX_CLIENTS; i++)
	{
		if (botstates[i] && botstates[i]->inuse)
		{
			BotResetState(botstates[i]);
			botstates[i]->setupcount = 4;
		}
	}

	return qtrue;
}

//rww - bot ai

//standard visibility check
int OrgVisible(vec3_t org1, vec3_t org2, const int ignore)
{
	trace_t tr;

	trap->Trace(&tr, org1, NULL, NULL, org2, ignore, MASK_SOLID, qfalse, 0, 0);

	if (tr.fraction == 1)
	{
		return 1;
	}

	return 0;
}

//special waypoint visibility check
int WPOrgVisible(const gentity_t* bot, vec3_t org1, vec3_t org2, const int ignore)
{
	trace_t tr;

	trap->Trace(&tr, org1, NULL, NULL, org2, ignore, MASK_SOLID, qfalse, 0, 0);

	if (tr.fraction == 1)
	{
		trap->Trace(&tr, org1, NULL, NULL, org2, ignore, MASK_PLAYERSOLID, qfalse, 0, 0);

		if (tr.fraction != 1 && tr.entityNum != ENTITYNUM_NONE && g_entities[tr.entityNum].s.eType == ET_SPECIAL)
		{
			if (g_entities[tr.entityNum].parent && g_entities[tr.entityNum].parent->client)
			{
				const gentity_t* ownent = g_entities[tr.entityNum].parent;

				if (OnSameTeam(bot, ownent) || bot->s.number == ownent->s.number)
				{
					return 1;
				}
			}
			return 2;
		}

		return 1;
	}

	return 0;
}

//visibility check with hull trace
int OrgVisibleBox(vec3_t org1, vec3_t mins, vec3_t maxs, vec3_t org2, const int ignore)
{
	trace_t tr;

	if (RMG.integer)
	{
		trap->Trace(&tr, org1, NULL, NULL, org2, ignore, MASK_SOLID, qfalse, 0, 0);
	}
	else
	{
		trap->Trace(&tr, org1, mins, maxs, org2, ignore, MASK_SOLID, qfalse, 0, 0);
	}

	if (tr.fraction == 1 && !tr.startsolid && !tr.allsolid)
	{
		return 1;
	}

	return 0;
}

//see if there's a func_* ent under the given pos.
//kind of badly done, but this shouldn't happen
//often.
int CheckForFunc(vec3_t org, const int ignore)
{
	vec3_t under;
	trace_t tr;

	VectorCopy(org, under);

	under[2] -= 64;

	trap->Trace(&tr, org, NULL, NULL, under, ignore, MASK_SOLID, qfalse, 0, 0);

	if (tr.fraction == 1)
	{
		return 0;
	}

	const gentity_t* fent = &g_entities[tr.entityNum];

	if (!fent)
	{
		return 0;
	}

	if (strstr(fent->classname, "func_"))
	{
		return 1; //there's a func brush here
	}

	return 0;
}

//perform pvs check based on rmg or not
qboolean BotPVSCheck(const vec3_t p1, const vec3_t p2)
{
	if (RMG.integer && bot_pvstype.integer)
	{
		vec3_t subPoint;
		VectorSubtract(p1, p2, subPoint);

		if (VectorLength(subPoint) > 5000)
		{
			return qfalse;
		}
		return qtrue;
	}

	return trap->InPVS(p1, p2);
}

/*
=========================
START A* Pathfinding Code
=========================
*/

typedef struct node_waypoint_s
{
	int wpNum;
	float f;
	float g;
	float h;
	int pNum;
} node_waypoint_t;

//Don't use the 0 slot of this array.  It's a binary heap and it's based on 1 being the first slot.
node_waypoint_t OpenList[MAX_WPARRAY_SIZE + 1];

node_waypoint_t CloseList[MAX_WPARRAY_SIZE];

qboolean OpenListEmpty(void)
{
	//since we're using a binary heap, in theory, if the first slot is empty, the heap
	//is empty.
	if (OpenList[1].wpNum != -1)
	{
		return qfalse;
	}

	return qtrue;
}

//Scans for the given wp on the Open List and returns it's OpenList position.
//Returns -1 if not found.
int FindOpenList(const int wpNum)
{
	for (int i = 1; i < MAX_WPARRAY_SIZE + 1 && OpenList[i].wpNum != -1; i++)
	{
		if (OpenList[i].wpNum == wpNum)
		{
			return i;
		}
	}
	return -1;
}

//Scans for the given wp on the Close List and returns it's CloseList position.
//Returns -1 if not found.
int FindCloseList(const int wpNum)
{
	for (int i = 0; i < MAX_WPARRAY_SIZE && CloseList[i].wpNum != -1; i++)
	{
		if (CloseList[i].wpNum == wpNum)
		{
			return i;
		}
	}
	return -1;
}

extern float RandFloat(float min, float max);

qboolean CarryingCapObjective(const bot_state_t* bs)
{
	//Carrying the Capture Objective?
	if (level.gametype == GT_SIEGE)
	{
		if (bs->tacticEntity && bs->client == bs->tacticEntity->genericValue8)
			return qtrue;
	}
	else
	{
		if (g_entities[bs->client].client->ps.powerups[PW_REDFLAG]
			|| g_entities[bs->client].client->ps.powerups[PW_BLUEFLAG])
			return qtrue;
	}
	return qfalse;
}

float RouteRandomize(const bot_state_t* bs, const float dest_dist)
{
	//this function randomizes the h value (distance to target location) to make the
	//bots take a random path instead of always taking the shortest route.
	//This should vary based on situation to prevent the bots from taking weird routes
	//for inapproprate situations.
	if (bs->currentTactic == BOTORDER_OBJECTIVE
		&& bs->objectiveType == OT_CAPTURE
		&& !CarryingCapObjective(bs))
	{
		//trying to capture something.  Fairly random paths to mix up the defending team.
		return dest_dist * RandFloat(.5, 1.5);
	}

	//return shortest distance.
	return dest_dist;
}

//Add this wpNum to the open list.
void AddOpenList(const bot_state_t* bs, const int wpNum, const int parent, const int end)
{
	int i;
	float g;

	if (gWPArray[wpNum]->flags & WPFLAG_REDONLY
		&& g_entities[bs->client].client->sess.sessionTeam != TEAM_RED)
	{
		//red only wp, can't use
		return;
	}

	if (gWPArray[wpNum]->flags & WPFLAG_BLUEONLY
		&& g_entities[bs->client].client->sess.sessionTeam != TEAM_BLUE)
	{
		//blue only wp, can't use
		return;
	}

	if (parent != -1 && gWPArray[wpNum]->flags & WPFLAG_JUMP)
	{
		if (force_jump_needed(gWPArray[parent]->origin, gWPArray[wpNum]->origin) > bs->cur_ps.fd.forcePowerLevel[
			FP_LEVITATION])
		{
			//can't make this jump with our level of Force Jump
			return;
		}
	}

	if (parent == -1)
	{
		//This is the start point, just use zero for g
		g = 0;
	}
	else if (wpNum == parent + 1)
	{
		if (gWPArray[wpNum]->flags & WPFLAG_ONEWAY_BACK)
		{
			//can't go down this one way
			return;
		}
		i = FindCloseList(parent);
		if (i != -1)
		{
			g = CloseList[i].g + gWPArray[parent]->disttonext;
		}
		else
		{
			return;
		}
	}
	else if (wpNum == parent - 1)
	{
		if (gWPArray[wpNum]->flags & WPFLAG_ONEWAY_FWD)
		{
			//can't go down this one way
			return;
		}
		i = FindCloseList(parent);
		if (i != -1)
		{
			g = CloseList[i].g + gWPArray[wpNum]->disttonext;
		}
		else
		{
			return;
		}
	}
	else
	{
		//nonsequencal parent/wpNum
		//don't try to go thru oneways when you're doing neighbor moves
		if (gWPArray[wpNum]->flags & WPFLAG_ONEWAY_FWD || gWPArray[wpNum]->flags & WPFLAG_ONEWAY_BACK)
		{
			return;
		}

		i = FindCloseList(parent);
		if (i != -1)
		{
			g = OpenList[i].g + Distance(gWPArray[wpNum]->origin, gWPArray[parent]->origin);
		}
		else
		{
			return;
		}
	}

	//Find first open slot or the slot that this wpNum already occupies.
	for (i = 1; i < MAX_WPARRAY_SIZE + 1 && OpenList[i].wpNum != -1; i++)
	{
		if (OpenList[i].wpNum == wpNum)
		{
			break;
		}
	}

	float h = Distance(gWPArray[wpNum]->origin, gWPArray[end]->origin);

	//add a bit of a random factor to h to make the bots take a variety of paths.
	h = RouteRandomize(bs, h);

	const float f = g + h;
	OpenList[i].f = f;
	OpenList[i].g = g;
	OpenList[i].h = h;
	OpenList[i].pNum = parent;
	OpenList[i].wpNum = wpNum;

	while (OpenList[i].f <= OpenList[i / 2].f && i != 1)
	{
		//binary parent has higher f than child
		const float ftemp = OpenList[i / 2].f;
		const float gtemp = OpenList[i / 2].g;
		const float htemp = OpenList[i / 2].h;
		const int p_numtemp = OpenList[i / 2].pNum;
		const int wptemp = OpenList[i / 2].wpNum;

		OpenList[i / 2].f = OpenList[i].f;
		OpenList[i / 2].g = OpenList[i].g;
		OpenList[i / 2].h = OpenList[i].h;
		OpenList[i / 2].pNum = OpenList[i].pNum;
		OpenList[i / 2].wpNum = OpenList[i].wpNum;

		OpenList[i].f = ftemp;
		OpenList[i].g = gtemp;
		OpenList[i].h = htemp;
		OpenList[i].pNum = p_numtemp;
		OpenList[i].wpNum = wptemp;

		i = i / 2;
	}
}

//Remove the first element from the OpenList.
void RemoveFirstOpenList(void)
{
	int i;
	for (i = 1; i < MAX_WPARRAY_SIZE + 1 && OpenList[i].wpNum != -1; i++)
	{
	}

	i--;
	if (OpenList[i].wpNum == -1)
	{
		//
	}

	if (OpenList[1].wpNum == OpenList[i].wpNum)
	{
		//the first slot is the only thing on the list. blank it.
		OpenList[1].f = -1;
		OpenList[1].g = -1;
		OpenList[1].h = -1;
		OpenList[1].pNum = -1;
		OpenList[1].wpNum = -1;
		return;
	}

	//shift last entry to start
	OpenList[1].f = OpenList[i].f;
	OpenList[1].g = OpenList[i].g;
	OpenList[1].h = OpenList[i].h;
	OpenList[1].pNum = OpenList[i].pNum;
	OpenList[1].wpNum = OpenList[i].wpNum;

	OpenList[i].f = -1;
	OpenList[i].g = -1;
	OpenList[i].h = -1;
	OpenList[i].pNum = -1;
	OpenList[i].wpNum = -1;

	while (OpenList[i].f >= OpenList[i * 2].f && OpenList[i * 2].wpNum != -1
		|| OpenList[i].f >= OpenList[i * 2 + 1].f && OpenList[i * 2 + 1].wpNum != -1)
	{
		if (OpenList[i * 2].f < OpenList[i * 2 + 1].f || OpenList[i * 2 + 1].wpNum == -1)
		{
			const float ftemp = OpenList[i * 2].f;
			const float gtemp = OpenList[i * 2].g;
			const float htemp = OpenList[i * 2].h;
			const int p_numtemp = OpenList[i * 2].pNum;
			const int wptemp = OpenList[i * 2].wpNum;

			OpenList[i * 2].f = OpenList[i].f;
			OpenList[i * 2].g = OpenList[i].g;
			OpenList[i * 2].h = OpenList[i].h;
			OpenList[i * 2].pNum = OpenList[i].pNum;
			OpenList[i * 2].wpNum = OpenList[i].wpNum;

			OpenList[i].f = ftemp;
			OpenList[i].g = gtemp;
			OpenList[i].h = htemp;
			OpenList[i].pNum = p_numtemp;
			OpenList[i].wpNum = wptemp;

			i = i * 2;
		}
		else if (OpenList[i * 2 + 1].wpNum != -1)
		{
			const float ftemp = OpenList[i * 2 + 1].f;
			const float gtemp = OpenList[i * 2 + 1].g;
			const float htemp = OpenList[i * 2 + 1].h;
			const int p_numtemp = OpenList[i * 2 + 1].pNum;
			const int wptemp = OpenList[i * 2 + 1].wpNum;

			OpenList[i * 2 + 1].f = OpenList[i].f;
			OpenList[i * 2 + 1].g = OpenList[i].g;
			OpenList[i * 2 + 1].h = OpenList[i].h;
			OpenList[i * 2 + 1].pNum = OpenList[i].pNum;
			OpenList[i * 2 + 1].wpNum = OpenList[i].wpNum;

			OpenList[i].f = ftemp;
			OpenList[i].g = gtemp;
			OpenList[i].h = htemp;
			OpenList[i].pNum = p_numtemp;
			OpenList[i].wpNum = wptemp;

			i = i * 2 + 1;
		}
		else
		{
			return;
		}
	}
}

//Adds a given OpenList wp to the closed list
void AddCloseList(const int openListpos)
{
	if (OpenList[openListpos].wpNum != -1)
	{
		for (int i = 0; i < MAX_WPARRAY_SIZE; i++)
		{
			if (CloseList[i].wpNum == -1)
			{
				//open slot, fill it.  heheh.
				CloseList[i].f = OpenList[openListpos].f;
				CloseList[i].g = OpenList[openListpos].g;
				CloseList[i].h = OpenList[openListpos].h;
				CloseList[i].pNum = OpenList[openListpos].pNum;
				CloseList[i].wpNum = OpenList[openListpos].wpNum;
				return;
			}
		}
	}
}

//Clear out the Route
void ClearRoute(int Route[MAX_WPARRAY_SIZE])
{
	for (int i = 0; i < MAX_WPARRAY_SIZE; i++)
	{
		Route[i] = -1;
	}
}

void AddtoRoute(const int wpNum, int Route[MAX_WPARRAY_SIZE])
{
	int i;
	for (i = 0; i < MAX_WPARRAY_SIZE && Route[i] != -1; i++)
	{
	}

	if (Route[i] == -1 && i < MAX_WPARRAY_SIZE)
	{
		//found the first empty slot
		while (i > 0)
		{
			Route[i] = Route[i - 1];
			i--;
		}
	}
	else
	{
		return;
	}
	if (i == 0)
	{
		Route[0] = wpNum;
	}
}

//find a given wpNum on the given route and return it's address.  return -1 if not on route.
//use wpNum = -1 to find the last wp on route.
int FindOnRoute(const int wpNum, int Route[MAX_WPARRAY_SIZE])
{
	int i;
	for (i = 0; i < MAX_WPARRAY_SIZE && Route[i] != wpNum; i++)
	{
	}

	//Special find end route command stuff
	if (wpNum == -1)
	{
		i--;
		if (Route[i] != -1)
		{
			//found it
			return i;
		}

		//otherwise, this is a empty route list
		return -1;
	}

	if (wpNum == Route[i])
	{
		//Success!
		return i;
	}

	//Couldn't find it
	return -1;
}

//Copy Route
void CopyRoute(bot_route_t routesource, bot_route_t routedest)
{
	for (int i = 0; i < MAX_WPARRAY_SIZE; i++)
	{
		routedest[i] = routesource[i];
	}
}

//Find the ideal (shortest) route between the start wp and the end wp
//badwp is for situations where you need to recalc a path when you dynamically discover
//that a wp is bad (door locked, blocked, etc).
//doRoute = actually set botRoute
float FindIdealPathtoWP(bot_state_t* bs, const int start, int end, const int badwp, bot_route_t Route)
{
	int i;

	if (bs->PathFindDebounce > level.time)
	{
		//currently debouncing the path finding to prevent a massive overload of AI thinking
		//in weird situations, like when the target near a waypoint where the bot can't
		//get to (like in an area where you need a specific force power to get to).
		return -1;
	}

	if (start == end)
	{
		ClearRoute(Route);
		AddtoRoute(end, Route);
		return 0;
	}

	//reset node lists
	for (i = 0; i < MAX_WPARRAY_SIZE; i++)
	{
		OpenList[i].wpNum = -1;
		OpenList[i].f = -1;
		OpenList[i].g = -1;
		OpenList[i].h = -1;
		OpenList[i].pNum = -1;
	}

	for (i = 0; i < MAX_WPARRAY_SIZE; i++)
	{
		CloseList[i].wpNum = -1;
		CloseList[i].f = -1;
		CloseList[i].g = -1;
		CloseList[i].h = -1;
		CloseList[i].pNum = -1;
	}

	AddOpenList(bs, start, -1, end);

	while (!OpenListEmpty() && FindOpenList(end) == -1)
	{
		//open list not empty
		//we're using a binary pile so the first slot always has the lowest f score
		AddCloseList(1);
		i = OpenList[1].wpNum;
		RemoveFirstOpenList();

		//Add surrounding nodes
		if (gWPArray[i + 1] && gWPArray[i + 1]->inuse)
		{
			if (gWPArray[i]->disttonext < 1000
				&& FindCloseList(i + 1) == -1 && i + 1 != badwp)
			{
				//Add next sequential node
				AddOpenList(bs, i + 1, i, end);
			}
		}

		if (i > 0)
		{
			if (gWPArray[i - 1]->disttonext < 1000 && gWPArray[i - 1]->inuse
				&& FindCloseList(i - 1) == -1 && i - 1 != badwp)
			{
				//Add previous sequential node
				AddOpenList(bs, i - 1, i, end);
			}
		}

		if (gWPArray[i]->neighbornum)
		{
			for (int x = 0; x < gWPArray[i]->neighbornum; x++)
			{
				if (x != badwp && FindCloseList(gWPArray[i]->neighbors[x].num) == -1)
				{
					AddOpenList(bs, gWPArray[i]->neighbors[x].num, i, end);
				}
			}
		}
	}

	i = FindOpenList(end);

	if (i != -1)
	{
		//we have a valid route to the end point
		ClearRoute(Route);
		AddtoRoute(end, Route);
		const float dist = OpenList[i].g;
		i = OpenList[i].pNum;
		i = FindCloseList(i);
		while (i != -1)
		{
			AddtoRoute(CloseList[i].wpNum, Route);
			i = FindCloseList(CloseList[i].pNum);
		}
		//only have the debouncer when we fail to find a route.
		bs->PathFindDebounce = level.time;
		return dist;
	}

	if (bot_wp_edit.integer)
	{
		//print error message if in edit mode.
	}
	bs->PathFindDebounce = level.time + 3000; //try again in 3 seconds.

	return -1;
}

/*
=========================
END A* Pathfinding Code
=========================
*/

//get the index to the nearest visible waypoint in the global trail
int GetNearestVisibleWP(vec3_t org, const int ignore)
{
	float bestdist;
	vec3_t mins, maxs;

	int i = 0;
	if (RMG.integer)
	{
		bestdist = 300;
	}
	else
	{
		bestdist = 800; //99999;
		//don't trace over 800 units away to avoid GIANT HORRIBLE SPEED HITS ^_^
	}
	int bestindex = -1;

	mins[0] = -15;
	mins[1] = -15;
	mins[2] = -1;
	maxs[0] = 15;
	maxs[1] = 15;
	maxs[2] = 1;

	while (i < gWPNum)
	{
		if (gWPArray[i] && gWPArray[i]->inuse)
		{
			vec3_t a;
			VectorSubtract(org, gWPArray[i]->origin, a);
			const float flLen = VectorLength(a);

			if (flLen < bestdist && (RMG.integer || BotPVSCheck(org, gWPArray[i]->origin)) && OrgVisibleBox(
				org, mins, maxs, gWPArray[i]->origin, ignore))
			{
				bestdist = flLen;
				bestindex = i;
			}
		}

		i++;
	}

	return bestindex;
}

//visually scanning in the given direction.
void BotBehave_VisualScan(bot_state_t* bs)
{
	VectorCopy(bs->VisualScanDir, bs->goalAngles);
	bs->wpCurrent = NULL;
}

//Just stand still
void BotBeStill(bot_state_t* bs)
{
	VectorCopy(bs->origin, bs->goalPosition);
	bs->beStill = level.time + BOT_THINK_TIME;
	VectorClear(bs->DestPosition);
	bs->DestIgnore = -1;
	bs->wpCurrent = NULL;
}

//just like GetNearestVisibleWP except with a bad waypoint input
int GetNearestVisibleWPSJE(const bot_state_t* bs, vec3_t org, const int ignore, const int badwp)
{
	float bestdist;
	vec3_t mins, maxs;

	if (RMG.integer)
	{
		bestdist = 300;
	}
	else
	{
		bestdist = 800; //99999;
		//don't trace over 800 units away to avoid GIANT HORRIBLE SPEED HITS ^_^
	}
	int bestindex = -1;

	mins[0] = -15;
	mins[1] = -15;
	mins[2] = -1;
	maxs[0] = 15;
	maxs[1] = 15;
	maxs[2] = 1;

	for (int i = 0; i < gWPNum; i++)
	{
		if (gWPArray[i] && gWPArray[i]->inuse && i != badwp)
		{
			vec3_t a;
			if (bs)
			{
				//check to make sure that this bot's team can use this waypoint
				if (gWPArray[i]->flags & WPFLAG_REDONLY
					&& g_entities[bs->client].client->sess.sessionTeam != TEAM_RED)
				{
					//red only wp, can't use
					continue;
				}

				if (gWPArray[i]->flags & WPFLAG_BLUEONLY
					&& g_entities[bs->client].client->sess.sessionTeam != TEAM_BLUE)
				{
					//blue only wp, can't use
					continue;
				}
			}

			VectorSubtract(org, gWPArray[i]->origin, a);
			float flLen = VectorLength(a);

			if (gWPArray[i]->flags & WPFLAG_WAITFORFUNC
				|| gWPArray[i]->flags & WPFLAG_NOMOVEFUNC
				|| gWPArray[i]->flags & WPFLAG_DESTROY_FUNCBREAK
				|| gWPArray[i]->flags & WPFLAG_FORCEPUSH
				|| gWPArray[i]->flags & WPFLAG_FORCEPULL)
			{
				//boost the distance for these waypoints so that we will try to avoid using them
				//if at all possible
				flLen = +500;
			}

			if (flLen < bestdist && (RMG.integer || BotPVSCheck(org, gWPArray[i]->origin)) && OrgVisibleBox(
				org, mins, maxs, gWPArray[i]->origin, ignore))
			{
				bestdist = flLen;
				bestindex = i;
			}
		}
	}

	return bestindex;
}

//just like GetNearestVisibleWP except without visiblity checks
int GetNearestWP(const bot_state_t* bs, vec3_t org, const int badwp)
{
	float bestdist;

	if (RMG.integer)
	{
		bestdist = 300;
	}
	else
	{
		//We're not doing traces!
		bestdist = 99999;
	}
	int bestindex = -1;

	for (int i = 0; i < gWPNum; i++)
	{
		if (gWPArray[i] && gWPArray[i]->inuse && i != badwp)
		{
			vec3_t a;
			if (bs)
			{
				//check to make sure that this bot's team can use this waypoint
				if (gWPArray[i]->flags & WPFLAG_REDONLY
					&& g_entities[bs->client].client->sess.sessionTeam != TEAM_RED)
				{
					//red only wp, can't use
					continue;
				}

				if (gWPArray[i]->flags & WPFLAG_BLUEONLY
					&& g_entities[bs->client].client->sess.sessionTeam != TEAM_BLUE)
				{
					//blue only wp, can't use
					continue;
				}
			}

			VectorSubtract(org, gWPArray[i]->origin, a);
			float flLen = VectorLength(a);

			if (gWPArray[i]->flags & WPFLAG_WAITFORFUNC
				|| gWPArray[i]->flags & WPFLAG_NOMOVEFUNC
				|| gWPArray[i]->flags & WPFLAG_DESTROY_FUNCBREAK
				|| gWPArray[i]->flags & WPFLAG_FORCEPUSH
				|| gWPArray[i]->flags & WPFLAG_FORCEPULL)
			{
				//boost the distance for these waypoints so that we will try to avoid using them
				//if at all possible
				flLen = flLen + 500;
			}

			if (flLen < bestdist)
			{
				bestdist = flLen;
				bestindex = i;
			}
		}
	}

	return bestindex;
}

qboolean IsHeavyWeapon(const bot_state_t* bs, int weapon)
{
	//right now we only show positive for weapons that can do this in primary fire mode
	switch (weapon)
	{
	case WP_MELEE:
		if (G_HeavyMelee(&g_entities[bs->client]))
		{
			return qtrue;
		}
		break;

	case WP_SABER:
	case WP_ROCKET_LAUNCHER:
	case WP_THERMAL:
	case WP_DET_PACK:
	case WP_CONCUSSION:
	case WP_FLECHETTE:
		return qtrue;
	default:;
	}

	return qfalse;
}

//use Force push or pull on local func_doors
qboolean UseForceonLocal(bot_state_t* bs, vec3_t origin, const qboolean pull)
{
	gentity_t* test = NULL;
	vec3_t center;
	qboolean SkipPushPull = qfalse;

	if (bs->DontSpamPushPull > level.time)
	{
		//pushed/pulled for this waypoint recently
		SkipPushPull = qtrue;
	}

	if (!SkipPushPull)
	{
		//Force Push/Pull
		while ((test = G_Find(test, FOFS(classname), "func_door")) != NULL)
		{
			vec3_t pos2;
			vec3_t pos1;
			if (!(test->spawnflags & 2))
			{
				//can't use the Force to move this door, ignore
				continue;
			}

			if (test->wait < 0 && test->moverState == MOVER_POS2)
			{
				//locked in position already, ignore.
				continue;
			}

			//find center, pos1, pos2
			if (VectorCompare(vec3_origin, test->s.origin))
			{
				vec3_t size;
				//does not have an origin brush, so pos1 & pos2 are relative to world origin, need to calc center
				VectorSubtract(test->r.absmax, test->r.absmin, size);
				VectorMA(test->r.absmin, 0.5, size, center);
				if (test->spawnflags & 1 && test->moverState == MOVER_POS1)
				{
					//if at pos1 and started open, make sure we get the center where it *started* because we're going to add back in the relative values pos1 and pos2
					VectorSubtract(center, test->pos1, center);
				}
				else if (!(test->spawnflags & 1) && test->moverState == MOVER_POS2)
				{
					//if at pos2, make sure we get the center where it *started* because we're going to add back in the relative values pos1 and pos2
					VectorSubtract(center, test->pos2, center);
				}
				VectorAdd(center, test->pos1, pos1);
				VectorAdd(center, test->pos2, pos2);
			}
			else
			{
				//actually has an origin, pos1 and pos2 are absolute
				VectorCopy(test->r.currentOrigin, center);
				VectorCopy(test->pos1, pos1);
				VectorCopy(test->pos2, pos2);
			}

			if (Distance(origin, center) > 400)
			{
				//too far away
				continue;
			}

			if (Distance(pos1, bs->eye) < Distance(pos2, bs->eye))
			{
				//pos1 is closer
				if (test->moverState == MOVER_POS1)
				{
					//at the closest pos
					if (pull)
					{
						//trying to pull, but already at closest point, so screw it
						continue;
					}
				}
				else if (test->moverState == MOVER_POS2)
				{
					//at farthest pos
					if (!pull)
					{
						//trying to push, but already at farthest point, so screw it
						continue;
					}
				}
			}
			else
			{
				//pos2 is closer
				if (test->moverState == MOVER_POS1)
				{
					//at the farthest pos
					if (!pull)
					{
						//trying to push, but already at farthest point, so screw it
						continue;
					}
				}
				else if (test->moverState == MOVER_POS2)
				{
					//at closest pos
					if (pull)
					{
						//trying to pull, but already at closest point, so screw it
						continue;
					}
				}
			}
			//we have a winner!
			break;
		}

		if (test)
		{
			//have a push/pull able door
			vec3_t viewDir;
			vec3_t ang;

			//doing special wp move
			bs->wpSpecial = qtrue;

			VectorSubtract(center, bs->eye, viewDir);
			vectoangles(viewDir, ang);
			VectorCopy(ang, bs->goalAngles);

			if (InFieldOfVision(bs->viewangles, 5, ang))
			{
				//use the force
				if (pull)
				{
					bs->doForcePull = level.time + 700;
				}
				else
				{
					bs->doForcePush = level.time + 700;
				}
				//Only press the button once every 15 seconds
				//this way the bots will be able to go up/down a lift weither the elevator
				//was up or down.
				//This debounce only applies to this waypoint.
				bs->DontSpamPushPull = level.time + 15000;
			}
			return qtrue;
		}
	}

	if (bs->DontSpamButton > level.time)
	{
		//pressed a button recently
		if (bs->useTime > level.time)
		{
			//holding down button
			//update the hacking buttons to account for lag about crap
			if (g_entities[bs->client].client->isHacking)
			{
				bs->useTime = bs->cur_ps.hackingTime + 100;
				bs->DontSpamButton = bs->useTime + 15000;
				bs->wpSpecial = qtrue;
				return qtrue;
			}

			//lost hack target.  clear things out
			bs->useTime = level.time;
			return qfalse;
		}
		return qfalse;
	}

	//use button checking
	while ((test = G_Find(test, FOFS(classname), "trigger_multiple")) != NULL)
	{
		if (test->flags & FL_INACTIVE)
		{
			//set by target_deactivate
			continue;
		}

		if (!(test->spawnflags & 4))
		{
			//can't use button this trigger
			continue;
		}

		if (test->alliedTeam)
		{
			if (g_entities[bs->client].client->sess.sessionTeam != test->alliedTeam)
			{
				//not useable by this team
				continue;
			}
		}

		FindOrigin(test, center);

		if (Distance(origin, center) > 200)
		{
			//too far away
			continue;
		}

		break;
	}

	if (!test)
	{
		//not luck so far
		while ((test = G_Find(test, FOFS(classname), "trigger_once")) != NULL)
		{
			if (test->flags & FL_INACTIVE)
			{
				//set by target_deactivate
				continue;
			}

			if (!test->use)
			{
				//trigger already fired
				continue;
			}

			if (!(test->spawnflags & 4))
			{
				//can't use button this trigger
				continue;
			}

			if (test->alliedTeam)
			{
				if (g_entities[bs->client].client->sess.sessionTeam != test->alliedTeam)
				{
					//not useable by this team
					continue;
				}
			}

			FindOrigin(test, center);

			if (Distance(origin, center) > 200)
			{
				//too far away
				continue;
			}

			break;
		}
	}

	if (test)
	{
		if (level.gametype == GT_SIEGE && test->idealclass && test->idealclass[0])
		{
			if (!G_NameInTriggerClassList(bgSiegeClasses[g_entities[bs->client].client->siegeClass].name,
				test->idealclass))
			{
				//not the right class to use this button
				if (!switch_siege_ideal_class(bs, test->idealclass))
				{
					//didn't want to switch to the class that hacks this trigger, call
					//for help and then see if there's a local breakable to just
					//smash it open.
					RequestSiegeAssistance(bs, SPC_SUPPORT);
					return AttackLocalBreakable(bs, origin);
				}
				//switched classes to be able to hack this target
				return qtrue;
			}
		}

		//if(tr.entityNum == test->s.number || tr.fraction == 1.0)
		{
			vec3_t viewDir;
			vec3_t ang;
			bs->wpSpecial = qtrue;

			//you can use use
			//set view angles.
			if (test->spawnflags & 2)
			{
				//you have to face in the direction of the trigger to have it work
				vectoangles(test->movedir, ang);
			}
			else
			{
				VectorSubtract(center, bs->eye, viewDir);
				vectoangles(viewDir, ang);
			}
			VectorCopy(ang, bs->goalAngles);

			if (G_PointInBounds(bs->origin, test->r.absmin, test->r.absmax))
			{
				//inside trigger zone, press use.
				bs->useTime = level.time + test->genericValue7 + 100;
				bs->DontSpamButton = bs->useTime + 15000;
			}
			else
			{
				//need to move closer
				VectorSubtract(center, bs->origin, viewDir);

				viewDir[2] = 0;
				VectorNormalize(viewDir);

				trap->EA_Move(bs->client, viewDir, 5000);
			}
			return qtrue;
		}
	}
	return qfalse;
}

//just returns the favorite weapon.
int FavoriteWeapon(bot_state_t* bs, const gentity_t* target, const qboolean haveCheck, const qboolean ammoCheck,
	const int ignoreWeaps)
{
	int bestweight = -1;
	int bestweapon = 0;

	for (int i = 0; i < WP_NUM_WEAPONS; i++)
	{
		if (haveCheck &&
			!(bs->cur_ps.stats[STAT_WEAPONS] & 1 << i))
		{
			//check to see if we actually have this weapon.
			continue;
		}

		if (ammoCheck &&
			bs->cur_ps.ammo[weaponData[i].ammoIndex] < weaponData[i].energyPerShot)
		{
			//check to see if we have enough ammo for this weapon.
			continue;
		}

		if (ignoreWeaps & 1 << i)
		{
			//check to see if this weapon is on our ignored weapons list
			continue;
		}

		if (target && target->inuse)
		{
			//do additional weapon checks based on our target's attributes.
			if (target->flags & FL_DMG_BY_HEAVY_WEAP_ONLY)
			{
				//don't use weapons that can't damage this target
				if (!IsHeavyWeapon(bs, i))
				{
					continue;
				}
			}

			//try to use explosives on breakables if we can.
			if (strcmp(target->classname, "func_breakable") == 0)
			{
				if (i == WP_DET_PACK)
				{
					bestweight = 100;
					bestweapon = i;
				}
				else if (i == WP_THERMAL)
				{
					bestweight = 99;
					bestweapon = i;
				}
				else if (i == WP_ROCKET_LAUNCHER)
				{
					bestweight = 99;
					bestweapon = i;
				}
				else if (bs->botWeaponWeights[i] > bestweight)
				{
					bestweight = bs->botWeaponWeights[i];
					bestweapon = i;
				}
			}
			else
			{
				//no special requirements for this target.
				if (bs->botWeaponWeights[i] > bestweight)
				{
					bestweight = bs->botWeaponWeights[i];
					bestweapon = i;
				}
			}
		}
		else
		{
			//no target
			if (bs->botWeaponWeights[i] > bestweight)
			{
				bestweight = bs->botWeaponWeights[i];
				bestweapon = i;
			}
		}
	}

	if (level.gametype == GT_SIEGE
		&& bestweapon == WP_NONE
		&& target && target->flags & FL_DMG_BY_HEAVY_WEAP_ONLY)
	{
		//we don't have the weapons to damage this thingy. call for help!
		RequestSiegeAssistance(bs, SPC_DEMOLITIONIST);
	}

	return bestweapon;
}

void ResetWPTimers(bot_state_t* bs);

qboolean HaveHeavyWeapon(const int weapons)
{
	if (weapons & 1 << WP_SABER
		|| weapons & 1 << WP_ROCKET_LAUNCHER
		|| weapons & 1 << WP_THERMAL
		|| weapons & 1 << WP_TRIP_MINE
		|| weapons & 1 << WP_DET_PACK
		|| weapons & 1 << WP_CONCUSSION
		|| weapons & 1 << WP_FLECHETTE)
	{
		return qtrue;
	}
	return qfalse;
}

qboolean AttackLocalBreakable(bot_state_t* bs, vec3_t origin)
{
	gentity_t* test = NULL;
	const gentity_t* valid = NULL;
	qboolean defend = qfalse;
	vec3_t testorigin;

	while ((test = G_Find(test, FOFS(classname), "func_breakable")) != NULL)
	{
		FindOrigin(test, testorigin);

		if (TargetDistance(bs, test, testorigin) < 300)
		{
			if (test->teamnodmg && test->teamnodmg == g_entities[bs->client].client->sess.sessionTeam)
			{
				//on a team that can't damage this breakable, as such defend it from immediate harm
				defend = qtrue;
			}

			valid = test;
			break;
		}

		//reset for next check
		VectorClear(testorigin);
	}

	if (valid)
	{
		//due to crazy stack overflow issues, just attack wildly while moving towards the
		//breakable
		trace_t tr;
		const int desiredweap = FavoriteWeapon(bs, valid, qtrue, qtrue, 0);

		//visual check

		trap->Trace(&tr, bs->eye, NULL, NULL, testorigin, bs->client, MASK_PLAYERSOLID, qfalse, 0, 0);

		if (tr.entityNum == test->s.number || tr.fraction == 1.0)
		{
			//we can see the breakable
			//doing special wp move
			bs->wpSpecial = qtrue;

			if (defend)
			{
				//defend this target since we can assume that the other team is going to try to
				BotBehave_DefendBasic(bs, testorigin);
			}
			else if (test->flags & FL_DMG_BY_HEAVY_WEAP_ONLY && !IsHeavyWeapon(bs, desiredweap))
			{
				//we currently don't have a heavy weap that we can use to destroy this target
				if (HaveHeavyWeapon(bs->cur_ps.stats[STAT_WEAPONS]))
				{
					//we have a weapon that could destroy this target but we don't have ammo
					BotBehave_DefendBasic(bs, testorigin);
				}
				else
				{
					//go hunting for a weapon that can destroy this object
					BotBehave_DefendBasic(bs, testorigin);
				}
			}
			else
			{
				//ATTACK!
				//determine which weapon you want to use
				if (desiredweap != bs->virtualWeapon)
				{
					//need to switch to desired weapon
					BotSelectChoiceWeapon(bs, desiredweap, qtrue);
				}
				//set visible flag so we'll attack this.
				bs->frame_Enemy_Vis = 1;
				BotBehave_AttackBasic(bs, valid);
			}

			return qtrue;
		}
	}
	return qfalse;
}

void WPVisibleUpdate(bot_state_t* bs)
{
	if (OrgVisibleBox(bs->origin, NULL, NULL, bs->wpCurrent->origin, bs->client))
	{
		//see the waypoint hold the counter
		bs->wpSeenTime = level.time + 3000;
	}
}

qboolean CalculateJump(bot_state_t* bs, vec3_t origin, vec3_t dest)
{
	vec3_t flatorigin;
	vec3_t flatdest;
	const float heightDif = dest[2] - origin[2];

	VectorCopy(origin, flatorigin);
	VectorCopy(dest, flatdest);

	const float dist = Distance(flatdest, flatorigin);

	if (heightDif > 30 && dist < 100)
	{
		return qtrue;
	}
	return qfalse;
}

void bot_move(bot_state_t* bs, vec3_t dest, const qboolean wptravel, qboolean strafe)
{
	vec3_t moveDir;
	vec3_t viewDir;
	vec3_t ang;
	qboolean movetrace = qtrue;

	VectorSubtract(dest, bs->eye, viewDir);
	vectoangles(viewDir, ang);
	VectorCopy(ang, bs->goalAngles);

	if (wptravel)
	{
		//if we're traveling between waypoints, don't bob the view up and down.
		bs->goalAngles[PITCH] = 0;
	}

	VectorSubtract(dest, bs->origin, moveDir);
	moveDir[2] = 0;
	VectorNormalize(moveDir);

	if (wptravel && !bs->wpCurrent)
	{
		return;
	}
	if (wptravel)
	{
		//special wp moves
		if (bs->wpCurrent->flags & WPFLAG_DESTROY_FUNCBREAK)
		{
			//look for nearby func_breakable and break them if we can before we continue
			if (AttackLocalBreakable(bs, bs->wpCurrent->origin))
			{
				//found a breakable that we can destroy
				bs->wpSeenTime = level.time + 3000;
				return;
			}
		}

		if (bs->wpCurrent->flags & WPFLAG_FORCEPUSH)
		{
			if (UseForceonLocal(bs, bs->wpCurrent->origin, qfalse))
			{
				//found something we can Force Push
				bs->wpSeenTime = level.time + 3000;
				return;
			}
		}

		if (bs->wpCurrent->flags & WPFLAG_FORCEPULL)
		{
			if (UseForceonLocal(bs, bs->wpCurrent->origin, qtrue))
			{
				//found something we can Force Pull
				WPVisibleUpdate(bs);
				return;
			}
		}

		if (bs->wpCurrent->flags & WPFLAG_JUMP)
		{
			//jump while travelling to this point
			vec3_t viewang;
			vec3_t velocity;
			vec3_t flatorigin, flatstart, flatend;

			VectorCopy(bs->origin, flatorigin);
			VectorCopy(bs->wpCurrentLoc, flatstart);
			VectorCopy(bs->wpCurrent->origin, flatend);

			flatorigin[2] = flatstart[2] = flatend[2] = 0;

			const float diststart = Distance(flatorigin, flatstart);
			const float distend = Distance(flatorigin, flatend);

			VectorSubtract(dest, bs->origin, viewang);
			vectoangles(viewang, ang);

			//never strafe during when jumping somewhere
			strafe = qfalse;

			if (bs->cur_ps.groundEntityNum != ENTITYNUM_NONE &&
				(diststart < distend || bs->origin[2] < bs->wpCurrent->origin[2]))
			{
				//before jump attempt
				if (ForcePowerforJump[force_jump_needed(bs->origin, bs->wpCurrent->origin)] > bs->cur_ps.fd.forcePower)
				{
					//we don't have enough energy to make our jump.  wait here.
					bs->wpSpecial = qtrue;
					return;
				}
			}

			//velocity analysis
			viewang[2] = 0;
			VectorNormalize(viewang);
			VectorCopy(bs->cur_ps.velocity, velocity);
			velocity[2] = 0;
			const float hor_velo = VectorNormalize(velocity);

			//make sure we're stopped or moving towards our goal before jumping
			if (diststart < distend && (VectorCompare(vec3_origin, velocity) || DotProduct(velocity, viewang) > .7)
				|| bs->cur_ps.groundEntityNum == ENTITYNUM_NONE)
			{
				//moving towards to our jump target or not moving at all or already on route and not already near the target.
				//hold down jump until we're pretty sure that we'll hit our target by just falling onto it.
				vec3_t toDestFlat;
				qboolean holdJump = qtrue;

				VectorSubtract(flatend, flatorigin, toDestFlat);
				VectorNormalize(toDestFlat);

				const float velo_scaler = DotProduct(toDestFlat, velocity);

				//figure out how long it will take make it to the target with our current horizontal velocity.
				if (hor_velo)
				{
					//can't check when not moving
					const float timeToEnd = distend / (hor_velo * velo_scaler);
					//assumes we're moving fully in the correct direction

					//calculate our estimated vertical position if we just let go of the jump now.
					const float estVert = bs->origin[2] + bs->cur_ps.velocity[2] * timeToEnd - g_gravity.value *
						timeToEnd *
						timeToEnd;

					if (estVert >= bs->wpCurrent->origin[2])
					{
						//we're going to make it, let go of jump
						holdJump = qfalse;
					}
				}

				if (holdJump)
				{
					//jump
					bs->jumpTime = level.time + 100;
					bs->wpSpecial = qtrue;
					WPVisibleUpdate(bs);
					trap->EA_Move(bs->client, moveDir, 5000);
					return;
				}
			}
		}

		//not doing a special wp move so clear that flag.
		bs->wpSpecial = qfalse;

		if (bs->wpCurrent->flags & WPFLAG_WAITFORFUNC)
		{
			if (!CheckForFunc(bs->wpCurrent->origin, bs->client))
			{
				WPVisibleUpdate(bs);
				if (!bs->AltRouteCheck && bs->wpTravelTime - level.time < 20000)
				{
					//been waiting for 10 seconds, try looking for alt route if we haven't
					//already
					bot_route_t routeTest;
					int newwp = GetNearestVisibleWPSJE(bs, bs->origin, bs->client,
						bs->wpCurrent->index);
					bs->AltRouteCheck = qtrue;

					if (newwp == -1)
					{
						newwp = GetNearestWP(bs, bs->origin, bs->wpCurrent->index);
					}
					if (FindIdealPathtoWP(bs, newwp, bs->wpDestination->index, bs->wpCurrent->index, routeTest) != -1)
					{
						//found a new route
						bs->wpCurrent = gWPArray[newwp];
						CopyRoute(routeTest, bs->botRoute);
						ResetWPTimers(bs);
					}
				}
				return;
			}
		}
		if (bs->wpCurrent->flags & WPFLAG_NOMOVEFUNC)
		{
			if (CheckForFunc(bs->wpCurrent->origin, bs->client))
			{
				WPVisibleUpdate(bs);
				if (!bs->AltRouteCheck && bs->wpTravelTime - level.time < 20000)
				{
					//been waiting for 10 seconds, try looking for alt route if we haven't
					//already
					bot_route_t routeTest;
					int newwp = GetNearestVisibleWPSJE(bs, bs->origin, bs->client, bs->wpCurrent->index);
					bs->AltRouteCheck = qtrue;

					if (newwp == -1)
					{
						newwp = GetNearestWP(bs, bs->origin, bs->wpCurrent->index);
					}
					if (FindIdealPathtoWP(bs, newwp, bs->wpDestination->index, bs->wpCurrent->index, routeTest) != -1)
					{
						//found a new route
						bs->wpCurrent = gWPArray[newwp];
						CopyRoute(routeTest, bs->botRoute);
						ResetWPTimers(bs);
					}
				}
				return;
			}
		}

		if (bs->wpCurrent->flags & WPFLAG_DUCK)
		{
			//duck while travelling to this point
			bs->duckTime = level.time + 100;
		}

		//visual check
		if (!(bs->wpCurrent->flags & WPFLAG_NOVIS))
		{
			//do visual check
			WPVisibleUpdate(bs);
		}
		else
		{
			movetrace = qfalse;
			strafe = qfalse;
		}
	}
	else
	{
		//jump to dest if we need to.
		if (CalculateJump(bs, bs->origin, dest))
		{
			bs->jumpTime = level.time + 100;
		}
	}

	//set strafing.
	if (strafe)
	{
		if (bs->meleeStrafeTime < level.time)
		{
			//select a new strafing direction, since we're actively navigating, switch strafe
			//directions more often
			//0 = no strafe
			//1 = strafe right
			//2 = strafe left
			bs->meleeStrafeDir = Q_irand(0, 2);
			bs->meleeStrafeTime = level.time + Q_irand(500, 1000);
		}

		//adjust the moveDir to do strafing
		adjustfor_strafe(bs, moveDir);
	}

	if (movetrace)
	{
		TraceMove(bs, moveDir, bs->DestIgnore);
	}

	if (DistanceHorizontal(bs->origin, dest) > 10)
	{
		//move if we're not in touch range.
		trap->EA_Move(bs->client, moveDir, 5000);
	}
}

qboolean DontBlockAllies(bot_state_t* bs)
{
	for (int i = 0; i < level.maxclients; i++)
	{
		if (i != bs->client //not the bot
			&& g_entities[i].inuse && g_entities[i].client //valid player
			&& g_entities[i].client->pers.connected == CON_CONNECTED //who is connected
			&& !(g_entities[i].s.eFlags & EF_DEAD) //and isn't dead
			&& g_entities[i].client->sess.sessionTeam == g_entities[bs->client].client->sess.sessionTeam
			//is on our team
			&& Distance(g_entities[i].client->ps.origin, bs->origin) < 50) //and we're too close to them.
		{
			//on your team and too close
			vec3_t moveDir, DestOrigin;
			VectorSubtract(bs->origin, g_entities[i].client->ps.origin, moveDir);
			VectorAdd(bs->origin, moveDir, DestOrigin);
			bot_move(bs, DestOrigin, qfalse, qfalse);
			return qtrue;
		}
	}
	return qfalse;
}

void WPTouch(bot_state_t* bs)
{
	//Touched the target WP
	int i = FindOnRoute(bs->wpCurrent->index, bs->botRoute);

	if (i == -1)
	{
		//This wp isn't on the route
		if (FindIdealPathtoWP(bs, bs->wpCurrent->index, bs->wpDestination->index, -2, bs->botRoute) == -1)
		{
			//couldn't find new path to destination!
			bs->wpCurrent = NULL;
			bot_move(bs, bs->DestPosition, qfalse, qfalse);
			return;
		}
		//set wp timers
		ResetWPTimers(bs);
	}

	i = FindOnRoute(bs->wpCurrent->index, bs->botRoute);
	i++;

	if (i >= MAX_WPARRAY_SIZE || bs->botRoute[i] == -1)
	{
		//at destination wp
		bs->wpCurrent = bs->wpDestination;
		VectorCopy(bs->origin, bs->wpCurrentLoc);
		ResetWPTimers(bs);
		return;
	}
	if (!gWPArray[i] || !gWPArray[i]->inuse)
	{
		return;
	}

	bs->wpCurrent = gWPArray[bs->botRoute[i]];
	VectorCopy(bs->origin, bs->wpCurrentLoc);
	ResetWPTimers(bs);
}

void ResetWPTimers(bot_state_t* bs)
{
	if (bs->wpCurrent->flags & WPFLAG_WAITFORFUNC
		|| bs->wpCurrent->flags & WPFLAG_NOMOVEFUNC
		|| bs->wpCurrent->flags & WPFLAG_DESTROY_FUNCBREAK
		|| bs->wpCurrent->flags & WPFLAG_FORCEPUSH
		|| bs->wpCurrent->flags & WPFLAG_FORCEPULL)
	{
		//it's an elevator or something waypoint time needs to be longer.
		bs->wpSeenTime = level.time + 30000;
		bs->wpTravelTime = level.time + 30000;
	}
	else if (bs->wpCurrent->flags & WPFLAG_NOVIS)
	{
		//10 sec
		bs->wpSeenTime = level.time + 10000;
		bs->wpTravelTime = level.time + 10000;
	}
	else
	{
		//3 sec visual time
		bs->wpSeenTime = level.time + 3000;

		//10 sec move time
		bs->wpTravelTime = level.time + 10000;
	}

	if (bs->wpCurrent->index == bs->wpDestination->index)
	{
		//just touched our destination node
		bs->wpTouchedDest = qtrue;
	}
	else
	{
		//reset the final node touched flag
		bs->wpTouchedDest = qfalse;
	}

	bs->AltRouteCheck = qfalse;
	bs->DontSpamPushPull = 0;
}

//wpDirection
//0 == FORWARD
//1 == BACKWARD

//see if this is a valid waypoint to pick up in our
//current state (whatever that may be)
int PassWayCheck(const bot_state_t* bs, const int windex)
{
	if (!gWPArray[windex] || !gWPArray[windex]->inuse)
	{
		//bad point index
		return 0;
	}

	if (RMG.integer)
	{
		if (gWPArray[windex]->flags & WPFLAG_RED_FLAG ||
			gWPArray[windex]->flags & WPFLAG_BLUE_FLAG)
		{
			//red or blue flag, we'd like to get here
			return 1;
		}
	}

	if (bs->wpDirection && gWPArray[windex]->flags & WPFLAG_ONEWAY_FWD)
	{
		//we're not travelling in a direction on the trail that will allow us to pass this point
		return 0;
	}
	if (!bs->wpDirection && gWPArray[windex]->flags & WPFLAG_ONEWAY_BACK)
	{
		//we're not travelling in a direction on the trail that will allow us to pass this point
		return 0;
	}

	if (bs->wpCurrent && gWPArray[windex]->forceJumpTo &&
		gWPArray[windex]->origin[2] > bs->wpCurrent->origin[2] + 64 &&
		bs->cur_ps.fd.forcePowerLevel[FP_LEVITATION] < gWPArray[windex]->forceJumpTo)
	{
		//waypoint requires force jump level greater than our current one to pass
		return 0;
	}

	return 1;
}

//tally up the distance between two waypoints
float TotalTrailDistance(const int start, const int end, bot_state_t* bs)
{
	int beginat;
	int endat;

	float distancetotal = 0;

	if (start > end)
	{
		beginat = end;
		endat = start;
	}
	else
	{
		beginat = start;
		endat = end;
	}

	while (beginat < endat)
	{
		if (beginat >= gWPNum || !gWPArray[beginat] || !gWPArray[beginat]->inuse)
		{
			//invalid waypoint index
			return -1;
		}

		if (!RMG.integer)
		{
			if (end > start && gWPArray[beginat]->flags & WPFLAG_ONEWAY_BACK ||
				start > end && gWPArray[beginat]->flags & WPFLAG_ONEWAY_FWD)
			{
				//a one-way point, this means this path cannot be travelled to the final point
				return -1;
			}
		}

#if 0 //disabled force jump checks for now
		if (gWPArray[beginat]->forceJumpTo)
		{
			if (gWPArray[beginat - 1] && gWPArray[beginat - 1]->origin[2] + 64 < gWPArray[beginat]->origin[2])
			{
				gdif = gWPArray[beginat]->origin[2] - gWPArray[beginat - 1]->origin[2];
			}

			if (gdif)
			{
				if (bs && bs->cur_ps.fd.forcePowerLevel[FP_LEVITATION] < gWPArray[beginat]->forceJumpTo)
				{
					return -1;
				}
			}
		}

		if (bs->wpCurrent && gWPArray[windex]->forceJumpTo &&
			gWPArray[windex]->origin[2] > (bs->wpCurrent->origin[2] + 64) &&
			bs->cur_ps.fd.forcePowerLevel[FP_LEVITATION] < gWPArray[windex]->forceJumpTo)
		{
			return -1;
		}
#endif

		distancetotal += gWPArray[beginat]->disttonext;

		beginat++;
	}

	return distancetotal;
}

//see if there's a route shorter than our current one to get
//to the final destination we currently desire
void CheckForShorterRoutes(bot_state_t* bs, const int newwpindex)
{
	int i = 0;
	int fj = 0;

	if (!bs->wpDestination)
	{
		return;
	}

	//set our traversal direction based on the index of the point
	if (newwpindex < bs->wpDestination->index)
	{
		bs->wpDirection = 0;
	}
	else if (newwpindex > bs->wpDestination->index)
	{
		bs->wpDirection = 1;
	}

	//can't switch again yet
	if (bs->wpSwitchTime > level.time)
	{
		return;
	}

	//no neighboring points to check off of
	if (!gWPArray[newwpindex]->neighbornum)
	{
		return;
	}

	//get the trail distance for our wp
	int bestindex = newwpindex;
	float bestlen = TotalTrailDistance(newwpindex, bs->wpDestination->index, bs);

	while (i < gWPArray[newwpindex]->neighbornum)
	{
		//now go through the neighbors and check the distance to the desired point from each neighbor
		const float checklen = TotalTrailDistance(gWPArray[newwpindex]->neighbors[i].num, bs->wpDestination->index, bs);

		if (checklen < bestlen - 64 || bestlen == -1)
		{
			//this path covers less distance, let's take it instead
			if (bs->cur_ps.fd.forcePowerLevel[FP_LEVITATION] >= gWPArray[newwpindex]->neighbors[i].forceJumpTo)
			{
				bestlen = checklen;
				bestindex = gWPArray[newwpindex]->neighbors[i].num;

				if (gWPArray[newwpindex]->neighbors[i].forceJumpTo)
				{
					fj = gWPArray[newwpindex]->neighbors[i].forceJumpTo;
				}
				else
				{
					fj = 0;
				}
			}
		}

		i++;
	}

	if (bestindex != newwpindex && bestindex != -1)
	{
		//we found a path we want to switch to, let's do it
		bs->wpCurrent = gWPArray[bestindex];
		bs->wpSwitchTime = level.time + 3000;

		if (fj)
		{
			//do we have to force jump to get to this neighbor?
#ifndef FORCEJUMP_INSTANTMETHOD
			bs->forceJumpChargeTime = level.time + 1000;
			bs->beStill = level.time + 1000;
			bs->forceJumping = bs->forceJumpChargeTime;
#else
			bs->beStill = level.time + 500;
			bs->jumpTime = level.time + fj * 1200;
			bs->jDelay = level.time + 200;
			bs->forceJumping = bs->jumpTime;
#endif
		}
	}
}

//Find the origin location of a given entity
void FindOrigins(const gentity_t* ent, vec3_t origin)
{
	if (!ent->classname)
	{
		//some funky entity, just set vec3_origin
		VectorCopy(vec3_origin, origin);
		return;
	}

	if (ent->client)
	{
		VectorCopy(ent->client->ps.origin, origin);
	}
	else
	{
		if (strcmp(ent->classname, "func_breakable") == 0
			|| strcmp(ent->classname, "trigger_multiple") == 0
			|| strcmp(ent->classname, "trigger_once") == 0
			|| strcmp(ent->classname, "func_usable") == 0)
		{
			//func_breakable's don't have normal origin data
			origin[0] = (ent->r.absmax[0] + ent->r.absmin[0]) / 2;
			origin[1] = (ent->r.absmax[1] + ent->r.absmin[1]) / 2;
			origin[2] = (ent->r.absmax[2] + ent->r.absmin[2]) / 2;
		}
		else
		{
			VectorCopy(ent->r.currentOrigin, origin);
		}
	}
}

float TargetDistances(const bot_state_t* bs, const gentity_t* target, vec3_t targetorigin)
{
	if (strcmp(target->classname, "misc_siege_item") == 0
		|| strcmp(target->classname, "func_breakable") == 0
		|| target->client && target->client->NPC_class == CLASS_RANCOR)
	{
		vec3_t enemy_origin;
		//flatten origin heights and measure
		VectorCopy(targetorigin, enemy_origin);
		if (fabs(enemy_origin[2] - bs->eye[2]) < 150)
		{
			//don't flatten unless you're on the same relative plane
			enemy_origin[2] = bs->eye[2];
		}

		if (target->client && target->client->NPC_class == CLASS_RANCOR)
		{
			//Rancors are big and stuff
			return Distance(bs->eye, enemy_origin) - 60;
		}
		if (strcmp(target->classname, "misc_siege_item") == 0)
		{
			//assume this is a misc_siege_item.  These have absolute based mins/maxs.
			//Scale for the entity's bounding box
			float adjustor;
			const float x = fabs(bs->eye[0] - enemy_origin[0]);
			const float y = fabs(bs->eye[1] - enemy_origin[1]);
			const float z = fabs(bs->eye[2] - enemy_origin[2]);

			//find the general direction of the impact to determine which bbox length to
			//scale with
			if (x > y && x > z)
			{
				//x
				adjustor = target->r.maxs[0];
			}
			else if (y > x && y > z)
			{
				//y
				adjustor = target->r.maxs[1];
			}
			else
			{
				//z
				adjustor = target->r.maxs[2];
			}

			return Distance(bs->eye, enemy_origin) - adjustor + 15;
		}
		if (strcmp(target->classname, "func_breakable") == 0)
		{
			//Scale for the entity's bounding box
			float adjustor;

			//find the smallest min/max and use that.
			if (target->r.absmax[0] - enemy_origin[0] < target->r.absmax[1] - enemy_origin[1])
			{
				adjustor = target->r.absmax[0] - enemy_origin[0];
			}
			else
			{
				adjustor = target->r.absmax[1] - enemy_origin[1];
			}

			return Distance(bs->eye, enemy_origin) - adjustor + 15;
		}
		//func_breakable
		return Distance(bs->eye, enemy_origin);
	}
	//standard stuff
	return Distance(bs->eye, targetorigin);
}

qboolean IsaHeavyWeapon(const bot_state_t* bs, const int weapon)
{
	//right now we only show positive for weapons that can do this in primary fire mode
	switch (weapon)
	{
	case WP_MELEE:
		if (G_HeavyMelee(&g_entities[bs->client]))
		{
			return qtrue;
		}
		break;

	case WP_SABER:
	case WP_ROCKET_LAUNCHER:
	case WP_THERMAL:
	case WP_DET_PACK:
	case WP_CONCUSSION:
	case WP_FLECHETTE:
		return qtrue;
	default:;
	}

	return qfalse;
}

int FavoriteWeapons(const bot_state_t* bs, const gentity_t* target, const qboolean haveCheck, const qboolean ammoCheck,
	const int ignoreWeaps)
{
	int bestweight = -1;
	int bestweapon = 0;

	for (int i = 0; i < WP_NUM_WEAPONS; i++)
	{
		if (haveCheck &&
			!(bs->cur_ps.stats[STAT_WEAPONS] & 1 << i))
		{
			//check to see if we actually have this weapon.
			continue;
		}

		if (ammoCheck &&
			bs->cur_ps.ammo[weaponData[i].ammoIndex] < weaponData[i].energyPerShot)
		{
			//check to see if we have enough ammo for this weapon.
			continue;
		}

		if (ignoreWeaps & 1 << i)
		{
			//check to see if this weapon is on our ignored weapons list
			continue;
		}

		if (target && target->inuse)
		{
			//do additional weapon checks based on our target's attributes.
			if (target->flags & FL_DMG_BY_HEAVY_WEAP_ONLY)
			{
				//don't use weapons that can't damage this target
				if (!IsaHeavyWeapon(bs, i))
				{
					continue;
				}
			}

			//try to use explosives on breakables if we can.
			if (strcmp(target->classname, "func_breakable") == 0)
			{
				if (i == WP_DET_PACK)
				{
					bestweight = 100;
					bestweapon = i;
				}
				else if (i == WP_THERMAL)
				{
					bestweight = 99;
					bestweapon = i;
				}
				else if (i == WP_ROCKET_LAUNCHER)
				{
					bestweight = 99;
					bestweapon = i;
				}
				else if (bs->botWeaponWeights[i] > bestweight)
				{
					bestweight = bs->botWeaponWeights[i];
					bestweapon = i;
				}
			}
			else
			{
				//no special requirements for this target.
				if (bs->botWeaponWeights[i] > bestweight)
				{
					bestweight = bs->botWeaponWeights[i];
					bestweapon = i;
				}
			}
		}
		else
		{
			//no target
			if (bs->botWeaponWeights[i] > bestweight)
			{
				bestweight = bs->botWeaponWeights[i];
				bestweapon = i;
			}
		}
	}

	return bestweapon;
}

//Find the number of players useing this basic class on team.
int NumberofSiegeBasicClasses(const int team, const int BaseClass)
{
	const siegeClass_t* holdClass = BG_GetClassOnBaseClass(team, BaseClass, 0);
	int NumPlayers = 0;

	for (int i = 0; i < MAX_CLIENTS; i++)
	{
		const gentity_t* ent = &g_entities[i];
		if (ent && ent->client && ent->client->pers.connected == CON_CONNECTED
			&& ent->client->sess.siegeClass && ent->client->sess.sessionTeam == team)
		{
			if (strcmp(ent->client->sess.siegeClass, holdClass->name) == 0)
			{
				NumPlayers++;
			}
		}
	}
	return NumPlayers;
}

qboolean HaveHeavyWeapons(const int weapons)
{
	if (weapons & 1 << WP_SABER
		|| weapons & 1 << WP_ROCKET_LAUNCHER
		|| weapons & 1 << WP_THERMAL
		|| weapons & 1 << WP_TRIP_MINE
		|| weapons & 1 << WP_DET_PACK
		|| weapons & 1 << WP_CONCUSSION
		|| weapons & 1 << WP_FLECHETTE)
	{
		return qtrue;
	}
	return qfalse;
}

int FindHeavyWeaponClassse(const int team, const int index, const qboolean saber)
{
	int NumHeavyWeapClasses = 0;

	const siegeTeam_t* stm = BG_SiegeFindThemeForTeam(team);
	if (!stm)
	{
		return -1;
	}

	// Loop through all the classes for this team
	for (int i = 0; i < stm->numClasses; i++)
	{
		if (!saber)
		{
			if (HaveHeavyWeapons(stm->classes[i]->weapons))
			{
				if (index == NumHeavyWeapClasses)
				{
					return stm->classes[i]->playerClass;
				}
				NumHeavyWeapClasses++;
			}
		}
		else
		{
			//look for saber
			if (stm->classes[i]->weapons & 1 << WP_SABER)
			{
				if (index == NumHeavyWeapClasses)
				{
					return stm->classes[i]->playerClass;
				}
				NumHeavyWeapClasses++;
			}
		}
	}

	//no heavy weapons/saber carrying units at this index
	return -1;
}

void RequestSiegeAssistance(bot_state_t* bs, const int BaseClass)
{
	if (bs->vchatTime > level.time)
	{
		//recently did vchat, don't do it now.
		return;
	}
	switch (BaseClass)
	{
	case SPC_DEMOLITIONIST:
		trap->EA_Command(bs->client, "voice_cmd req_demo");
		bs->vchatTime = level.time + Q_irand(25000, 45000);
		break;
	case SPC_SUPPORT:
		trap->EA_Command(bs->client, "voice_cmd req_tech");
		bs->vchatTime = level.time + Q_irand(25000, 45000);
		break;
	case SPOTTED_SNIPER:
		trap->EA_Command(bs->client, "voice_cmd spot_sniper");
		bs->vchatTime = level.time + Q_irand(25000, 45000);
		break;
	case SPOTTED_TROOPS:
		trap->EA_Command(bs->client, "voice_cmd spot_troops");
		bs->vchatTime = level.time + Q_irand(12000, 30000);
		break;
	case REQUEST_SUPPLIES:
		trap->EA_Command(bs->client, "voice_cmd req_sup");
		bs->vchatTime = level.time + Q_irand(60000, 75000);
		break;
	case REQUEST_ASSISTANCE:
		trap->EA_Command(bs->client, "voice_cmd req_assist");
		bs->vchatTime = level.time + Q_irand(15000, 30000);
		break;
	case REQUEST_MEDIC:
		trap->EA_Command(bs->client, "voice_cmd req_medic");
		bs->vchatTime = level.time + Q_irand(5000, 10000);
		break;
	case TACTICAL_COVERME:
		trap->EA_Command(bs->client, "voice_cmd tac_cover");
		bs->vchatTime = level.time + Q_irand(10000, 25000);
		break;
	case TACTICAL_HOLDPOSITION:
		trap->EA_Command(bs->client, "voice_cmd tac_hold");
		bs->vchatTime = level.time + Q_irand(5000, 12000);
		break;
	case TACTICAL_FOLLOW:
		trap->EA_Command(bs->client, "voice_cmd tac_follow");
		bs->vchatTime = level.time + Q_irand(25000, 45000);
		break;
	case REPLY_YES:
		trap->EA_Command(bs->client, "voice_cmd reply_yes");
		bs->vchatTime = level.time + Q_irand(2000, 4000);
		break;
	case REPLY_COMING:
		trap->EA_Command(bs->client, "voice_cmd reply_coming");
		bs->vchatTime = level.time + Q_irand(2000, 4000);
		break;
	default:;
	}
}

//Should we switch classes to destroy this breakable or just call for help?
//saber = saber only destroyable?
void ShouldSwitchaSiegeClasses(bot_state_t* bs, const qboolean saber)
{
	int i = 0;

	int classNum = FindHeavyWeaponClassse(g_entities[bs->client].client->sess.siegeDesiredTeam, i, saber);
	while (classNum != -1)
	{
		const int x = NumberofSiegeBasicClasses(g_entities[bs->client].client->sess.siegeDesiredTeam, classNum);
		if (x)
		{
			//request assistance for this class since we already have someone
			//playing that class
			RequestSiegeAssistance(bs, SPC_DEMOLITIONIST);
			return;
		}

		//ok, noone is using that class check for the next
		//indexed heavy weapon class
		i++;
		classNum = FindHeavyWeaponClassse(g_entities[bs->client].client->sess.siegeDesiredTeam, i, saber);
	}

	//ok, noone else is using a siege class with a heavyweapon.  Switch to
	//one ourselves
	i--;
	classNum = FindHeavyWeaponClassse(g_entities[bs->client].client->sess.siegeDesiredTeam, i, saber);

	if (classNum == -1)
	{
		//what the?!
		//G_Printf("couldn't find a siege class with a heavy weapon in ShouldSwitchaSiegeClasses().\n");
	}
	else
	{
		//switch to this class
		siegeClass_t* holdClass = BG_GetClassOnBaseClass(g_entities[bs->client].client->sess.siegeDesiredTeam, classNum,
			0);
		trap->EA_Command(bs->client, va("siegeclass \"%s\"\n", holdClass->name));
	}
}

//check/select the chosen weapon
int BotSelectChoiceWeapon(bot_state_t* bs, const int weapon, const int doselection)
{
	int hasit = 0;

	int i = 0;

	while (i < WP_NUM_WEAPONS)
	{
		if (bs->cur_ps.ammo[weaponData[i].ammoIndex] >= weaponData[i].energyPerShot &&
			i == weapon &&
			bs->cur_ps.stats[STAT_WEAPONS] & 1 << i)
		{
			hasit = 1;
			break;
		}

		i++;
	}

	if (hasit && bs->cur_ps.weapon != weapon && doselection && bs->virtualWeapon != weapon)
	{
		bs->virtualWeapon = weapon;
		BotSelectWeapon(bs->client, weapon);
		return 2;
	}

	if (hasit)
	{
		return 1;
	}

	return 0;
}

qboolean AttackLocalBreakables(bot_state_t* bs, vec3_t origin)
{
	gentity_t* test = NULL;
	const gentity_t* valid = NULL;
	qboolean defend = qfalse;
	vec3_t testorigin;

	while ((test = G_Find(test, FOFS(classname), "func_breakable")) != NULL)
	{
		FindOrigins(test, testorigin);

		if (TargetDistances(bs, test, testorigin) < 300)
		{
			if (test->teamnodmg && test->teamnodmg == g_entities[bs->client].client->sess.sessionTeam)
			{
				//on a team that can't damage this breakable, as such defend it from immediate harm
				defend = qtrue;
			}

			valid = test;
			break;
		}

		//reset for next check
		VectorClear(testorigin);
	}

	if (valid)
	{
		//due to crazy stack overflow issues, just attack wildly while moving towards the
		//breakable
		trace_t tr;
		const int desiredweap = FavoriteWeapons(bs, valid, qtrue, qtrue, 0);

		//visual check
		trap->Trace(&tr, bs->eye, NULL, NULL, testorigin, bs->client, MASK_SOLID, qfalse, 0, 0);

		if (tr.entityNum == test->s.number || tr.fraction == 1.0)
		{
			//we can see the breakable
			//doing special wp move
			bs->wpSpecial = qtrue;

			if (defend)
			{
				//defend this target since we can assume that the other team is going to try to
				//destroy this thingy
				Siege_DefendFromAttackers(bs);
			}
			else if (test->flags & FL_DMG_BY_HEAVY_WEAP_ONLY && !IsaHeavyWeapon(bs, desiredweap))
			{
				//we currently don't have a heavy weap that we can use to destroy this target
				if (HaveHeavyWeapons(bs->cur_ps.stats[STAT_WEAPONS]))
				{
					//we have a weapon that could destroy this target but we don't have ammo
					//RAFIXME:  at this point we should have the bot go look for some ammo
					//but for now just defend this area.
					Siege_DefendFromAttackers(bs);
				}
				else if (level.gametype == GT_SIEGE)
				{
					//ok, check to see if we should switch classes if noone else can blast this
					ShouldSwitchaSiegeClasses(bs, qfalse);
					Siege_DefendFromAttackers(bs);
				}
				else
				{
					//go hunting for a weapon that can destroy this object
					//RAFIXME:  Add this code
					Siege_DefendFromAttackers(bs);
				}
			}
			else if (test->flags & FL_DMG_BY_SABER_ONLY && !(bs->cur_ps.stats[STAT_WEAPONS] & 1 << WP_SABER))
			{
				//This is only damaged by sabers and we don't have a saber
				ShouldSwitchaSiegeClasses(bs, qtrue);
				Siege_DefendFromAttackers(bs);
			}
			else
			{
				//ATTACK!
				//determine which weapon you want to use
				if (desiredweap != bs->virtualWeapon)
				{
					//need to switch to desired weapon
					BotSelectChoiceWeapon(bs, desiredweap, qtrue);
				}
				//set visible flag so we'll attack this.
				bs->frame_Enemy_Vis = 1;
				trap->EA_Attack(bs->client);
			}

			return qtrue;
		}
	}
	return qfalse;
}

qboolean UseForceonLocals(bot_state_t* bs, vec3_t origin, const qboolean pull)
{
	gentity_t* test = NULL;
	vec3_t center;
	qboolean SkipPushPull = qfalse;

	if (bs->DontSpamPushPull > level.time)
	{
		//pushed/pulled for this waypoint recently
		SkipPushPull = qtrue;
	}

	if (!SkipPushPull)
	{
		//Force Push/Pull
		while ((test = G_Find(test, FOFS(classname), "func_door")) != NULL)
		{
			vec3_t pos2;
			vec3_t pos1;
			if (!(test->spawnflags & 2))
			{
				//can't use the Force to move this door, ignore
				continue;
			}

			if (test->wait < 0 && test->moverState == MOVER_POS2)
			{
				//locked in position already, ignore.
				continue;
			}

			//find center, pos1, pos2
			if (VectorCompare(vec3_origin, test->s.origin))
			{
				vec3_t size;
				//does not have an origin brush, so pos1 & pos2 are relative to world origin, need to calc center
				VectorSubtract(test->r.absmax, test->r.absmin, size);
				VectorMA(test->r.absmin, 0.5, size, center);
				if (test->spawnflags & 1 && test->moverState == MOVER_POS1)
				{
					//if at pos1 and started open, make sure we get the center where it *started* because we're going to add back in the relative values pos1 and pos2
					VectorSubtract(center, test->pos1, center);
				}
				else if (!(test->spawnflags & 1) && test->moverState == MOVER_POS2)
				{
					//if at pos2, make sure we get the center where it *started* because we're going to add back in the relative values pos1 and pos2
					VectorSubtract(center, test->pos2, center);
				}
				VectorAdd(center, test->pos1, pos1);
				VectorAdd(center, test->pos2, pos2);
			}
			else
			{
				//actually has an origin, pos1 and pos2 are absolute
				VectorCopy(test->r.currentOrigin, center);
				VectorCopy(test->pos1, pos1);
				VectorCopy(test->pos2, pos2);
			}

			if (Distance(origin, center) > 400)
			{
				//too far away
				continue;
			}

			if (Distance(pos1, bs->eye) < Distance(pos2, bs->eye))
			{
				//pos1 is closer
				if (test->moverState == MOVER_POS1)
				{
					//at the closest pos
					if (pull)
					{
						//trying to pull, but already at closest point, so screw it
						continue;
					}
				}
				else if (test->moverState == MOVER_POS2)
				{
					//at farthest pos
					if (!pull)
					{
						//trying to push, but already at farthest point, so screw it
						continue;
					}
				}
			}
			else
			{
				//pos2 is closer
				if (test->moverState == MOVER_POS1)
				{
					//at the farthest pos
					if (!pull)
					{
						//trying to push, but already at farthest point, so screw it
						continue;
					}
				}
				else if (test->moverState == MOVER_POS2)
				{
					//at closest pos
					if (pull)
					{
						//trying to pull, but already at closest point, so screw it
						continue;
					}
				}
			}
			//we have a winner!
			break;
		}

		if (test)
		{
			//have a push/pull able door
			vec3_t viewDir;
			vec3_t ang;

			//doing special wp move
			bs->wpSpecial = qtrue;

			VectorSubtract(center, bs->eye, viewDir);
			vectoangles(viewDir, ang);
			VectorCopy(ang, bs->goalAngles);

			if (InFieldOfVision(bs->viewangles, 5, ang))
			{
				//use the force
				if (pull)
				{
					bs->doForcePull = level.time + 700;
				}
				else
				{
					bs->doForcePush = level.time + 700;
				}
				//Only press the button once every 15 seconds
				//this way the bots will be able to go up/down a lift weither the elevator
				//was up or down.
				//This debounce only applies to this waypoint.
				bs->DontSpamPushPull = level.time + 15000;
			}
			return qtrue;
		}
	}

	if (bs->DontSpamButton > level.time)
	{
		//pressed a button recently
		if (bs->useTime > level.time)
		{
			//holding down button
			//update the hacking buttons to account for lag about crap
			if (g_entities[bs->client].client->isHacking)
			{
				bs->useTime = bs->cur_ps.hackingTime + 100;
				bs->DontSpamButton = bs->useTime + 15000;
				bs->wpSpecial = qtrue;
				return qtrue;
			}

			//lost hack target.  clear things out
			bs->useTime = level.time;
			return qfalse;
		}
		return qfalse;
	}

	//use button checking
	while ((test = G_Find(test, FOFS(classname), "trigger_multiple")) != NULL)
	{
		if (test->flags & FL_INACTIVE)
		{
			//set by target_deactivate
			continue;
		}

		if (!(test->spawnflags & 4))
		{
			//can't use button this trigger
			continue;
		}

		if (test->alliedTeam)
		{
			if (g_entities[bs->client].client->sess.sessionTeam != test->alliedTeam)
			{
				//not useable by this team
				continue;
			}
		}

		FindOrigins(test, center);

		if (Distance(origin, center) > 200)
		{
			//too far away
			continue;
		}

		break;
	}

	if (!test)
	{
		//not luck so far
		while ((test = G_Find(test, FOFS(classname), "trigger_once")) != NULL)
		{
			if (test->flags & FL_INACTIVE)
			{
				//set by target_deactivate
				continue;
			}

			if (!test->use)
			{
				//trigger already fired
				continue;
			}

			if (!(test->spawnflags & 4))
			{
				//can't use button this trigger
				continue;
			}

			if (test->alliedTeam)
			{
				if (g_entities[bs->client].client->sess.sessionTeam != test->alliedTeam)
				{
					//not useable by this team
					continue;
				}
			}

			FindOrigins(test, center);

			if (Distance(origin, center) > 200)
			{
				//too far away
				continue;
			}

			break;
		}
	}

	if (test)
	{
		vec3_t viewDir;
		vec3_t ang;
		bs->wpSpecial = qtrue;

		//you can use use
		//set view angles.
		if (test->spawnflags & 2)
		{
			//you have to face in the direction of the trigger to have it work
			vectoangles(test->movedir, ang);
		}
		else
		{
			VectorSubtract(center, bs->eye, viewDir);
			vectoangles(viewDir, ang);
		}
		VectorCopy(ang, bs->goalAngles);

		if (G_PointInBounds(bs->origin, test->r.absmin, test->r.absmax))
		{
			//inside trigger zone, press use.
			bs->useTime = level.time + test->genericValue7 + 100;
			bs->DontSpamButton = bs->useTime + 15000;
		}
		else
		{
			//need to move closer
			VectorSubtract(center, bs->origin, viewDir);

			viewDir[2] = 0;
			VectorNormalize(viewDir);

			trap->EA_Move(bs->client, viewDir, 5000);
		}
		return qtrue;
	}
	return qfalse;
}

void WPVisibleUpdates(bot_state_t* bs)
{
	if (OrgVisibleBox(bs->origin, NULL, NULL, bs->wpCurrent->origin, bs->client))
	{
		//see the waypoint hold the counter
		bs->wpSeenTime = level.time + 3000;
	}
}

//check for flags on the waypoint we're currently traveling to
//and perform the desired behavior based on the flag
void WPConstantRoutine(bot_state_t* bs)
{
	if (!bs->wpCurrent)
	{
		return;
	}

	if (bs->wpCurrent->flags & WPFLAG_DUCK)
	{
		//duck while traveling to this point
		bs->duckTime = level.time + 100;
	}

	if (bs->wpCurrent->flags & WPFLAG_FORCEPUSH)
	{
		if (UseForceonLocals(bs, bs->wpCurrent->origin, qfalse))
		{
			//found something we can Force Push
			bs->wpSeenTime = level.time + 1000;
			return;
		}
	}
	if (bs->wpCurrent->flags & WPFLAG_FORCEPULL)
	{
		if (UseForceonLocals(bs, bs->wpCurrent->origin, qtrue))
		{
			//found something we can Force Pull
			WPVisibleUpdates(bs);
			return;
		}
	}

#ifndef FORCEJUMP_INSTANTMETHOD
	if (bs->wpCurrent->flags & WPFLAG_JUMP)
	{
		//jump while traveling to this point
		float heightDif = bs->wpCurrent->origin[2] - bs->origin[2] + 16;

		if (bs->origin[2] + 16 >= bs->wpCurrent->origin[2])
		{
			//don't need to jump, we're already higher than this point
			heightDif = 0;
		}
		if (heightDif > 128 && bs->cur_ps.stats[STAT_HOLDABLE_ITEMS] & 1 << HI_JETPACK)
		{
			// Jet packer.. Jetpack ON!
			bs->cur_ps.eFlags = PM_JETPACK;
			bs->cur_ps.eFlags |= EF_JETPACK_ACTIVE;
			bs->cur_ps.eFlags |= EF_JETPACK_FLAMING;
			bs->cur_ps.eFlags |= EF_JETPACK_HOVER;
			bs->jumpHoldTime = (bs->forceJumpChargeTime + level.time) / 2 + 50000;
		}

		if (heightDif > 40 && bs->cur_ps.fd.forcePowersKnown & 1 << FP_LEVITATION && (bs->cur_ps.fd.forceJumpCharge
			< forceJumpStrength[bs->cur_ps.fd.forcePowerLevel[FP_LEVITATION]] - 100 || bs->cur_ps.groundEntityNum ==
			ENTITYNUM_NONE))
		{
			//alright, let's jump
			bs->forceJumpChargeTime = level.time + 1000;
			if (bs->cur_ps.groundEntityNum != ENTITYNUM_NONE && bs->jumpPrep < level.time - 300)
			{
				bs->jumpPrep = level.time + 700;
			}
			bs->beStill = level.time + 300;
			bs->jumpTime = 0;

			if (bs->wpSeenTime < level.time + 600)
			{
				bs->wpSeenTime = level.time + 600;
			}
		}
		else if (heightDif > 64 && !(bs->cur_ps.fd.forcePowersKnown & 1 << FP_LEVITATION))
		{
			//this point needs force jump to reach and we don't have it
			//Kill the current point and turn around
			bs->wpCurrent = NULL;
			if (bs->wpDirection)
			{
				bs->wpDirection = 0;
			}
			else
			{
				bs->wpDirection = 1;
			}

			return;
		}
	}
#endif

	if (bs->wpCurrent->forceJumpTo)
	{
#ifdef FORCEJUMP_INSTANTMETHOD
		if (bs->cur_ps.stats[STAT_HOLDABLE_ITEMS] & (1 << HI_JETPACK))
		{
			client->jetPackOn = qtrue;
			bs->cur_ps.eFlags = PM_JETPACK;
			bs->cur_ps.eFlags |= EF_JETPACK_ACTIVE;
			bs->cur_ps.eFlags |= EF_JETPACK_FLAMING;
			bs->cur_ps.eFlags |= EF_JETPACK_HOVER;
			bs->jumpHoldTime = ((bs->forceJumpChargeTime + level.time) / 2) + 50000;
		}
		if (bs->origin[2] + 16 < bs->wpCurrent->origin[2])
		{
			bs->jumpTime = level.time + 100;
		}
#else

		if (bs->cur_ps.fd.forceJumpCharge < forceJumpStrength[bs->cur_ps.fd.forcePowerLevel[FP_LEVITATION]] - 100)
		{
			bs->forceJumpChargeTime = level.time + 200;
		}
#endif
	}
}

//check if our ctf state is to guard the base
qboolean BotCTFGuardDuty(const bot_state_t* bs)
{
	if (level.gametype != GT_CTF && level.gametype != GT_CTY)
	{
		return qfalse;
	}

	if (bs->ctfState == CTFSTATE_DEFENDER)
	{
		return qtrue;
	}

	return qfalse;
}

//when we reach the waypoint we are travelling to,
//this function will be called. We will perform any
//checks for flags on the current wp and activate
//any "touch" events based on that.
void WPTouchRoutine(bot_state_t* bs)
{
	if (!bs->wpCurrent)
	{
		return;
	}

	bs->wpTravelTime = level.time + 10000;

	if (bs->wpCurrent->flags & WPFLAG_NOMOVEFUNC)
	{
		//don't try to use any nearby map objects for a little while
		bs->noUseTime = level.time + 4000;
	}

#ifdef FORCEJUMP_INSTANTMETHOD
	if ((bs->wpCurrent->flags & WPFLAG_JUMP) && bs->wpCurrent->forceJumpTo)
	{ //jump if we're flagged to but not if this indicates a force jump point. Force jumping is
	  //handled elsewhere.
		if (bs->cur_ps.stats[STAT_HOLDABLE_ITEMS] & (1 << HI_JETPACK))
		{
			bs->cur_ps.eFlags = PM_JETPACK;
			bs->cur_ps.eFlags |= EF_JETPACK_ACTIVE;
			bs->cur_ps.eFlags |= EF_JETPACK_FLAMING;
			bs->cur_ps.eFlags |= EF_JETPACK_HOVER;
			bs->jumpHoldTime = ((bs->forceJumpChargeTime + level.time) / 2) + 50000;
		}
		bs->jumpTime = level.time + 100;
	}
#else
	if (bs->wpCurrent->flags & WPFLAG_JUMP && !bs->wpCurrent->forceJumpTo)
	{
		//jump if we're flagged to but not if this indicates a force jump point. Force jumping is
		//handled elsewhere.
		if (bs->cur_ps.stats[STAT_HOLDABLE_ITEMS] & 1 << HI_JETPACK)
		{
			bs->cur_ps.eFlags = PM_JETPACK;
			bs->cur_ps.eFlags |= EF_JETPACK_ACTIVE;
			bs->cur_ps.eFlags |= EF_JETPACK_FLAMING;
			bs->cur_ps.eFlags |= EF_JETPACK_HOVER;
			bs->jumpHoldTime = 50000;
		}
		bs->jumpTime = level.time + 100;
	}
#endif

	if (bs->isCamper && bot_camp.integer && (BotIsAChickenWuss(bs) || BotCTFGuardDuty(bs) || bs->isCamper == 2) && (bs
		->wpCurrent->flags & WPFLAG_SNIPEORCAMP || bs->wpCurrent->flags & WPFLAG_SNIPEORCAMPSTAND) &&
		bs->cur_ps.weapon != WP_SABER && bs->cur_ps.weapon != WP_MELEE && bs->cur_ps.weapon != WP_STUN_BATON)
	{
		int last_num;
		//if we're a camper and a chicken then camp
		if (bs->wpDirection)
		{
			last_num = bs->wpCurrent->index + 1;
		}
		else
		{
			last_num = bs->wpCurrent->index - 1;
		}

		if (gWPArray[last_num] && gWPArray[last_num]->inuse && gWPArray[last_num]->index && bs->isCamping < level.time)
		{
			bs->isCamping = level.time + rand() % 15000 + 30000;
			bs->wpCamping = bs->wpCurrent;
			bs->wpCampingTo = gWPArray[last_num];

			if (bs->wpCurrent->flags & WPFLAG_SNIPEORCAMPSTAND)
			{
				bs->campStanding = qtrue;
			}
			else
			{
				bs->campStanding = qfalse;
			}
		}
	}
	else if ((bs->cur_ps.weapon == WP_SABER || bs->cur_ps.weapon == WP_STUN_BATON || bs->cur_ps.weapon == WP_MELEE) &&
		bs->isCamping > level.time)
	{
		//don't snipe/camp with a melee weapon, that would be silly
		bs->isCamping = 0;
		bs->wpCampingTo = NULL;
		bs->wpCamping = NULL;
	}

	if (bs->wpDestination)
	{
		if (bs->wpCurrent->index == bs->wpDestination->index)
		{
			bs->wpDestination = NULL;

			if (bs->runningLikeASissy)
			{
				//this obviously means we're scared and running, so we'll want to keep our navigational priorities less delayed
				bs->destinationGrabTime = level.time + 500;
			}
			else
			{
				bs->destinationGrabTime = level.time + 3500;
			}
		}
		else
		{
			CheckForShorterRoutes(bs, bs->wpCurrent->index);
		}
	}
}

//could also slowly lerp toward, but for now
//just copying straight over.
void MoveTowardIdealAngles(bot_state_t* bs)
{
	VectorCopy(bs->goalAngles, bs->ideal_viewangles);
}

#define BOT_STRAFE_AVOIDANCE

#ifdef BOT_STRAFE_AVOIDANCE
#define STRAFEAROUND_RIGHT			1
#define STRAFEAROUND_LEFT			2

//do some trace checks for strafing to get an idea of where we
//are and if we should move to avoid obstacles.
int BotTrace_Strafe(const bot_state_t* bs, vec3_t traceto)
{
	const vec3_t playerMins = { -15, -15, /*DEFAULT_MINS_2*/-8 };
	const vec3_t playerMaxs = { 15, 15, DEFAULT_MAXS_2 };
	vec3_t from, to;
	vec3_t dirAng, dirDif;
	vec3_t forward, right;
	trace_t tr;

	if (bs->cur_ps.groundEntityNum == ENTITYNUM_NONE)
	{
		//don't do this in the air, it can be.. dangerous.
		return 0;
	}

	VectorSubtract(traceto, bs->origin, dirAng);
	VectorNormalize(dirAng);
	vectoangles(dirAng, dirAng);

	if (AngleDifference(bs->viewangles[YAW], dirAng[YAW]) > 60 ||
		AngleDifference(bs->viewangles[YAW], dirAng[YAW]) < -60)
	{
		//If we aren't facing the direction we're going here, then we've got enough excuse to be too stupid to strafe around anyway
		return 0;
	}

	VectorCopy(bs->origin, from);
	VectorCopy(traceto, to);

	VectorSubtract(to, from, dirDif);
	VectorNormalize(dirDif);
	vectoangles(dirDif, dirDif);

	AngleVectors(dirDif, forward, 0, 0);

	to[0] = from[0] + forward[0] * 32;
	to[1] = from[1] + forward[1] * 32;
	to[2] = from[2] + forward[2] * 32;

	trap->Trace(&tr, from, playerMins, playerMaxs, to, bs->client, MASK_PLAYERSOLID, qfalse, 0, 0);

	if (tr.fraction == 1)
	{
		return 0;
	}

	AngleVectors(dirAng, 0, right, 0);

	from[0] += right[0] * 32;
	from[1] += right[1] * 32;
	from[2] += right[2] * 16;

	to[0] += right[0] * 32;
	to[1] += right[1] * 32;
	to[2] += right[2] * 32;

	trap->Trace(&tr, from, playerMins, playerMaxs, to, bs->client, MASK_PLAYERSOLID, qfalse, 0, 0);

	if (tr.fraction == 1)
	{
		return STRAFEAROUND_RIGHT;
	}

	from[0] -= right[0] * 64;
	from[1] -= right[1] * 64;
	from[2] -= right[2] * 64;

	to[0] -= right[0] * 64;
	to[1] -= right[1] * 64;
	to[2] -= right[2] * 64;

	trap->Trace(&tr, from, playerMins, playerMaxs, to, bs->client, MASK_PLAYERSOLID, qfalse, 0, 0);

	if (tr.fraction == 1)
	{
		return STRAFEAROUND_LEFT;
	}

	return 0;
}
#endif

//Similar to the trace check, but we want to trace to see
//if there's anything we can jump over.
int BotTrace_Jump(bot_state_t* bs, vec3_t traceto)
{
	vec3_t mins, maxs, a, fwd, traceto_mod, tracefrom_mod;
	trace_t tr;

	VectorSubtract(traceto, bs->origin, a);
	vectoangles(a, a);

	AngleVectors(a, fwd, NULL, NULL);

	traceto_mod[0] = bs->origin[0] + fwd[0] * 4;
	traceto_mod[1] = bs->origin[1] + fwd[1] * 4;
	traceto_mod[2] = bs->origin[2] + fwd[2] * 4;

	mins[0] = -15;
	mins[1] = -15;
	mins[2] = -18;
	maxs[0] = 15;
	maxs[1] = 15;
	maxs[2] = 32;

	trap->Trace(&tr, bs->origin, mins, maxs, traceto_mod, bs->client, MASK_PLAYERSOLID, qfalse, 0, 0);

	if (tr.fraction == 1)
	{
		return 0;
	}

	const int orTr = tr.entityNum;

	VectorCopy(bs->origin, tracefrom_mod);

	tracefrom_mod[2] += 41;
	traceto_mod[2] += 41;

	mins[0] = -15;
	mins[1] = -15;
	mins[2] = 0;
	maxs[0] = 15;
	maxs[1] = 15;
	maxs[2] = 8;

	trap->Trace(&tr, tracefrom_mod, mins, maxs, traceto_mod, bs->client, MASK_PLAYERSOLID, qfalse, 0, 0);

	if (tr.fraction == 1)
	{
		if (orTr >= 0 && orTr < MAX_CLIENTS && botstates[orTr] && botstates[orTr]->jumpTime > level.time)
		{
			return 0; //so bots don't try to jump over each other at the same time
		}
		if (bs->cur_ps.stats[STAT_HOLDABLE_ITEMS] & 1 << HI_JETPACK)
		{
			bs->cur_ps.eFlags = PM_JETPACK;
			bs->cur_ps.eFlags |= EF_JETPACK_ACTIVE;
			bs->cur_ps.eFlags |= EF_JETPACK_FLAMING;
			bs->cur_ps.eFlags |= EF_JETPACK_HOVER;
			bs->jumpHoldTime = (bs->forceJumpChargeTime + level.time) / 2 + 50000;
		}

		if (bs->currentEnemy && bs->currentEnemy->s.number == orTr && (BotGetWeaponRange(bs) == BWEAPONRANGE_SABER ||
			BotGetWeaponRange(bs) == BWEAPONRANGE_MELEE))
		{
			if (bs->cur_ps.stats[STAT_HOLDABLE_ITEMS] & 1 << HI_JETPACK)
			{
				bs->cur_ps.eFlags = PM_JETPACK;
				bs->cur_ps.eFlags |= EF_JETPACK_ACTIVE;
				bs->cur_ps.eFlags |= EF_JETPACK_FLAMING;
				bs->cur_ps.eFlags |= EF_JETPACK_HOVER;
				bs->jumpHoldTime = (bs->forceJumpChargeTime + level.time) / 2 + 50000;
			}
			else
			{
				return 0;
			}
		}

		return 1;
	}

	return 0;
}

//And yet another check to duck under any obstacles.
int BotTrace_Duck(const bot_state_t* bs, vec3_t traceto)
{
	vec3_t mins, maxs, a, fwd, traceto_mod, tracefrom_mod;
	trace_t tr;

	VectorSubtract(traceto, bs->origin, a);
	vectoangles(a, a);

	AngleVectors(a, fwd, NULL, NULL);

	traceto_mod[0] = bs->origin[0] + fwd[0] * 4;
	traceto_mod[1] = bs->origin[1] + fwd[1] * 4;
	traceto_mod[2] = bs->origin[2] + fwd[2] * 4;

	mins[0] = -15;
	mins[1] = -15;
	mins[2] = -23;
	maxs[0] = 15;
	maxs[1] = 15;
	maxs[2] = 8;

	trap->Trace(&tr, bs->origin, mins, maxs, traceto_mod, bs->client, MASK_PLAYERSOLID, qfalse, 0, 0);

	if (tr.fraction != 1)
	{
		return 0;
	}

	VectorCopy(bs->origin, tracefrom_mod);

	tracefrom_mod[2] += 31; //33;
	traceto_mod[2] += 31; //33;

	mins[0] = -15;
	mins[1] = -15;
	mins[2] = 0;
	maxs[0] = 15;
	maxs[1] = 15;
	maxs[2] = 32;

	trap->Trace(&tr, tracefrom_mod, mins, maxs, traceto_mod, bs->client, MASK_PLAYERSOLID, qfalse, 0, 0);

	if (tr.fraction != 1)
	{
		return 1;
	}

	return 0;
}

//check of the potential enemy is a valid one
int PassStandardEnemyChecks(const bot_state_t* bs, gentity_t* en)
{
	if (!bs || !en)
	{
		//shouldn't happen
		return 0;
	}

	if (!en->client)
	{
		//not a client, don't care about him
		return 0;
	}

	if (en->health < 1)
	{
		//he's already dead
		return 0;
	}

	if (!en->takedamage)
	{
		//a client that can't take damage?
		return 0;
	}

	if (IsSurrendering(en))
	{
		// Ignore players surrendering
		return 0;
	}

	if (IsRespecting(en))
	{
		// Ignore players surrendering
		return 0;
	}

	if (IsCowering(en))
	{
		// Ignore players surrendering
		return 0;
	}

	if (bs->doingFallback &&
		gLevelFlags & LEVELFLAG_IGNOREINFALLBACK)
	{
		return 0;
	}

	if (en->client->ps.pm_type == PM_INTERMISSION ||
		en->client->ps.pm_type == PM_SPECTATOR ||
		en->client->sess.sessionTeam == TEAM_SPECTATOR)
	{
		//don't attack spectators
		return 0;
	}

	if (!en->client->pers.connected)
	{
		//a "zombie" client?
		return 0;
	}

	if (!en->s.solid)
	{
		//shouldn't happen
		return 0;
	}

	if (bs->client == en->s.number)
	{
		//don't attack yourself
		return 0;
	}

	if (OnSameTeam(&g_entities[bs->client], en))
	{
		//don't attack teammates
		return 0;
	}

	if (BotMindTricked(bs->client, en->s.number))
	{
		if (bs->currentEnemy && bs->currentEnemy->s.number == en->s.number)
		{
			//if mindtricked by this enemy, then be less "aware" of them, even though
			//we know they're there.
			vec3_t vs;

			VectorSubtract(bs->origin, en->client->ps.origin, vs);
			const float v_len = VectorLength(vs);

			if (v_len > 64)
			{
				return 0;
			}
		}
	}

	if (en->client->ps.duelInProgress && en->client->ps.duelIndex != bs->client)
	{
		//don't attack duelists unless you're dueling them
		return 0;
	}

	if (bs->cur_ps.duelInProgress && en->s.number != bs->cur_ps.duelIndex)
	{
		//ditto, the other way around
		return 0;
	}

	if (level.gametype == GT_JEDIMASTER && !en->client->ps.isJediMaster && !bs->cur_ps.isJediMaster &&
		G_ThereIsAMaster())
	{
		//rules for attacking non-JM in JM mode
		vec3_t vs;

		if (!g_friendlyFire.integer)
		{
			//can't harm non-JM in JM mode if FF is off
			return 0;
		}

		VectorSubtract(bs->origin, en->client->ps.origin, vs);
		const float v_len = VectorLength(vs);

		if (v_len > 350)
		{
			return 0;
		}
	}

	return 1;
}

//Notifies the bot that he has taken damage from "attacker".
void BotDamageNotification(const gclient_t* bot, gentity_t* attacker)
{
	int i;

	if (!bot || !attacker || !attacker->client)
	{
		return;
	}

	if (bot->ps.clientNum >= MAX_CLIENTS)
	{
		//an NPC.. do nothing for them.
		return;
	}

	if (attacker->s.number >= MAX_CLIENTS)
	{
		//if attacker is an npc also don't care I suppose.
		return;
	}

	bot_state_t* bs_a = botstates[attacker->s.number];

	if (bs_a)
	{
		//if the client attacking us is a bot as well
		bs_a->lastAttacked = &g_entities[bot->ps.clientNum];
		i = 0;

		while (i < MAX_CLIENTS)
		{
			if (botstates[i] &&
				i != bs_a->client &&
				botstates[i]->lastAttacked == &g_entities[bot->ps.clientNum])
			{
				botstates[i]->lastAttacked = NULL;
			}

			i++;
		}
	}
	else //got attacked by a real client, so no one gets rights to lastAttacked
	{
		i = 0;

		while (i < MAX_CLIENTS)
		{
			if (botstates[i] &&
				botstates[i]->lastAttacked == &g_entities[bot->ps.clientNum])
			{
				botstates[i]->lastAttacked = NULL;
			}

			i++;
		}
	}

	bot_state_t* bs = botstates[bot->ps.clientNum];

	if (!bs)
	{
		return;
	}

	bs->lastHurt = attacker;

	if (bs->currentEnemy)
	{
		//we don't care about the guy attacking us if we have an enemy already
		return;
	}

	if (!PassStandardEnemyChecks(bs, attacker))
	{
		//the person that hurt us is not a valid enemy
		return;
	}

	if (PassLovedOneCheck(bs, attacker))
	{
		//the person that hurt us is the one we love!
		bs->currentEnemy = attacker;
		bs->enemySeenTime = level.time + ENEMY_FORGET_MS;
	}
}

//perform cheap "hearing" checks based on the event catching
//system
int BotCanHear(const bot_state_t* bs, const gentity_t* en, const float endist)
{
	float minlen;

	if (!en || !en->client)
	{
		return 0;
	}

	if (en && en->client && en->client->ps.otherSoundTime > level.time)
	{
		//they made a noise in recent time
		minlen = en->client->ps.otherSoundLen;
		goto checkStep;
	}

	if (en && en->client && en->client->ps.footstepTime > level.time)
	{
		//they made a footstep
		minlen = 256;
		goto checkStep;
	}

	if (gBotEventTracker[en->s.number].eventTime < level.time)
	{
		//no recent events to check
		return 0;
	}

	switch (gBotEventTracker[en->s.number].events[gBotEventTracker[en->s.number].eventSequence & MAX_PS_EVENTS - 1])
	{
		//did the last event contain a sound?
	case EV_GLOBAL_SOUND:
		minlen = 256;
		break;
	case EV_FIRE_WEAPON:
	case EV_ALT_FIRE:
	case EV_SABER_ATTACK:
		minlen = 512;
		break;
	case EV_STEP_4:
	case EV_STEP_8:
	case EV_STEP_12:
	case EV_STEP_16:
	case EV_FOOTSTEP:
	case EV_FOOTSTEP_METAL:
	case EV_FOOTWADE:
		minlen = 256;
		break;
	case EV_JUMP:
	case EV_ROLL:
		minlen = 256;
		break;
	default:
		minlen = 999999;
		break;
	}
checkStep:
	if (BotMindTricked(bs->client, en->s.number))
	{
		//if mindtricked by this person, cut down on the minlen so they can't "hear" as well
		minlen /= 4;
	}

	if (endist <= minlen)
	{
		//we heard it
		return 1;
	}

	return 0;
}

//check for new events
void UpdateEventTracker(void)
{
	int i = 0;

	while (i < MAX_CLIENTS)
	{
		if (gBotEventTracker[i].eventSequence != level.clients[i].ps.eventSequence)
		{
			//updated event
			gBotEventTracker[i].eventSequence = level.clients[i].ps.eventSequence;
			gBotEventTracker[i].events[0] = level.clients[i].ps.events[0];
			gBotEventTracker[i].events[1] = level.clients[i].ps.events[1];
			gBotEventTracker[i].eventTime = level.time + 0.5;
		}

		i++;
	}
}

//check if said angles are within our fov
int InFieldOfVision(vec3_t viewangles, const float fov, vec3_t angles)
{
	for (int i = 0; i < 2; i++)
	{
		const float angle = AngleMod(viewangles[i]);
		angles[i] = AngleMod(angles[i]);
		float diff = angles[i] - angle;
		if (angles[i] > angle)
		{
			if (diff > 180.0)
			{
				diff -= 360.0;
			}
		}
		else
		{
			if (diff < -180.0)
			{
				diff += 360.0;
			}
		}
		if (diff > 0)
		{
			if (diff > fov * 0.5)
			{
				return 0;
			}
		}
		else
		{
			if (diff < -fov * 0.5)
			{
				return 0;
			}
		}
	}
	return 1;
}

//We cannot hurt the ones we love. Unless of course this
//function says we can.
int PassLovedOneCheck(const bot_state_t* bs, const gentity_t* ent)
{
	if (!bs->lovednum)
	{
		return 1;
	}

	if (level.gametype == GT_DUEL || level.gametype == GT_POWERDUEL)
	{
		//There is no love in 1-on-1
		return 1;
	}

	int i = 0;

	if (!botstates[ent->s.number])
	{
		//not a bot
		return 1;
	}

	if (!bot_attachments.integer)
	{
		return 1;
	}

	const bot_state_t* loved = botstates[ent->s.number];

	while (i < bs->lovednum)
	{
		if (strcmp(level.clients[loved->client].pers.netname, bs->loved[i].name) == 0)
		{
			if (!is_teamplay() && bs->loved[i].level < 2)
			{
				//if FFA and level of love is not greater than 1, just don't care
				return 1;
			}
			if (is_teamplay() && !OnSameTeam(&g_entities[bs->client], &g_entities[loved->client]) && bs->loved[i].level < 2)
			{
				//is teamplay, but not on same team and level < 2
				return 1;
			}
			return 0;
		}

		i++;
	}

	return 1;
}

//update the currentEnemy visual data for the current enemy.
void EnemyVisualUpdate(bot_state_t* bs)
{
	vec3_t a;
	vec3_t enemyOrigin;
	vec3_t enemyAngles;
	trace_t tr;

	if (!bs->currentEnemy)
	{
		//bad!  This should never happen
		return;
	}

	FindOrigin(bs->currentEnemy, enemyOrigin);
	FindAngles(bs->currentEnemy, enemyAngles);

	VectorSubtract(enemyOrigin, bs->eye, a);
	const float dist = VectorLength(a);
	vectoangles(a, a);
	a[PITCH] = a[ROLL] = 0;

	trap->Trace(&tr, bs->eye, NULL, NULL, enemyOrigin, bs->client, MASK_PLAYERSOLID, qfalse, 0, 0);

	if (tr.entityNum == bs->currentEnemy->s.number && InFieldOfVision(bs->viewangles, 90, a) && !BotMindTricked(
		bs->client, bs->currentEnemy->s.number)
		|| BotCanHear(bs, bs->currentEnemy, dist))
	{
		//spotted him
		bs->frame_Enemy_Len = TargetDistance(bs, bs->currentEnemy, enemyOrigin);
		bs->frame_Enemy_Vis = 1;
		VectorCopy(enemyOrigin, bs->lastEnemySpotted);
		VectorCopy(bs->currentEnemy->s.angles, bs->lastEnemyAngles);
		bs->enemySeenTime = level.time + 2000;
	}
	else
	{
		//can't see him
		bs->frame_Enemy_Vis = 0;
	}
}

//standard check to find a new enemy.
int ScanForEnemies(bot_state_t* bs)
{
	float hasEnemyDist = 0;
	qboolean noAttackNonJM = qfalse;

	float closest = 999999;
	int i = 0;
	int bestindex = -1;

	if (bs->currentEnemy)
	{
		if (!PassStandardEnemyChecks(bs, bs->currentEnemy))
		{
			//target became invalid, move to next target
			bs->currentEnemy = NULL;
		}
		else
		{
			//only switch to a new enemy if he's significantly closer
			hasEnemyDist = bs->frame_Enemy_Len;
		}
	}

	if (bs->currentEnemy && bs->currentEnemy->client &&
		bs->currentEnemy->client->ps.isJediMaster)
	{
		//The Jedi Master must die.
		EnemyVisualUpdate(bs);

		//override the last seen locations because we can always see them
		FindOrigin(bs->currentEnemy, bs->lastEnemySpotted);
		FindAngles(bs->currentEnemy, bs->lastEnemyAngles);
		return -1;
	}
	if (bs->currentTactic == BOTORDER_SEARCHANDDESTROY
		&& bs->tacticEntity)
	{
		//currently going after search and destroy target
		if (bs->tacticEntity->s.number == bs->currentEnemy->s.number)
		{
			EnemyVisualUpdate(bs);
			return -1;
		}
	}
	else if (bs->currentTactic == BOTORDER_OBJECTIVE
		&& bs->tacticEntity)
	{
		if (bs->tacticEntity->s.number == bs->currentEnemy->s.number)
		{
			EnemyVisualUpdate(bs);
			return -1;
		}
	}

	if (level.gametype == GT_JEDIMASTER)
	{
		if (G_ThereIsAMaster() && !bs->cur_ps.isJediMaster)
		{
			//if friendly fire is on in jedi master we can attack people that bug us
			if (!g_friendlyFire.integer)
			{
				noAttackNonJM = qtrue;
			}
			else
			{
				closest = 128; //only get mad at people if they get close enough to you to anger you, or hurt you
			}
		}
	}

	while (i <= MAX_CLIENTS)
	{
		if (i != bs->client && g_entities[i].client && !OnSameTeam(&g_entities[bs->client], &g_entities[i]) &&
			PassStandardEnemyChecks(bs, &g_entities[i]) && BotPVSCheck(g_entities[i].client->ps.origin, bs->eye) &&
			PassLovedOneCheck(bs, &g_entities[i]))
		{
			vec3_t a;
			VectorSubtract(g_entities[i].client->ps.origin, bs->eye, a);
			float distcheck = VectorLength(a);
			vectoangles(a, a);

			if (g_entities[i].client->ps.isJediMaster)
			{
				//make us think the Jedi Master is close so we'll attack him above all
				distcheck = 1;
			}
			if (g_entities[bs->client].health < 75)
			{
				//I need a doctor!
				RequestSiegeAssistance(bs, REQUEST_MEDIC);
			}
			else if (g_entities[bs->client].health < 30)
			{
				//Sniper!
				RequestSiegeAssistance(bs, REQUEST_MEDIC);
			}
			else if (g_entities[i].client->ps.weapon == WP_DISRUPTOR &&
				g_entities[i].client->sess.sessionTeam != g_entities[bs->client].client->sess.sessionTeam)
			{
				//Sniper!
				RequestSiegeAssistance(bs, SPOTTED_SNIPER);
			}
			else if ((g_entities[i].client->ps.weapon == WP_REPEATER ||
				g_entities[i].client->ps.weapon == WP_BOWCASTER ||
				g_entities[i].client->ps.weapon == WP_FLECHETTE ||
				g_entities[i].client->ps.weapon == WP_THERMAL) &&
				(g_entities[bs->client].client->ps.weapon == WP_REPEATER ||
					g_entities[bs->client].client->ps.weapon == WP_BOWCASTER ||
					g_entities[bs->client].client->ps.weapon == WP_FLECHETTE))
			{
				//Troops, incoming
				RequestSiegeAssistance(bs, SPOTTED_TROOPS);
			}
			else if (g_entities[i].client->ps.weapon == WP_ROCKET_LAUNCHER ||
				g_entities[i].client->ps.weapon == WP_CONCUSSION)
			{
				//Crap, he has a bigger gun than us.
				RequestSiegeAssistance(bs, REQUEST_ASSISTANCE);
			}
			else if (g_entities[bs->client].client->ps.weapon == WP_BLASTER ||
				g_entities[bs->client].client->ps.weapon == WP_BRYAR_PISTOL ||
				g_entities[bs->client].client->ps.weapon == WP_STUN_BATON ||
				g_entities[bs->client].client->ps.weapon == WP_DEMP2)
			{
				//Cover me, I have a shitty weapon.
				RequestSiegeAssistance(bs, TACTICAL_COVERME);

				if (botstates[i] && g_entities[i].client->sess.sessionTeam == g_entities[bs->client].client->sess.
					sessionTeam)

					RequestSiegeAssistance(bs, Q_irand(REPLY_YES, REPLY_COMING));
			}
			else if (g_entities[i].client->ps.weapon == WP_BLASTER ||
				g_entities[i].client->ps.weapon == WP_BRYAR_PISTOL ||
				g_entities[i].client->ps.weapon == WP_STUN_BATON ||
				g_entities[i].client->ps.weapon == WP_DEMP2)
			{
				//You have a shitty weapon. Follow me.
				RequestSiegeAssistance(bs, TACTICAL_FOLLOW);

				if (botstates[i] && g_entities[i].client->sess.sessionTeam == g_entities[bs->client].client->sess.
					sessionTeam)

					RequestSiegeAssistance(bs, Q_irand(REPLY_YES, REPLY_COMING));
			}
			else if (g_entities[i].client->ps.weapon == WP_DISRUPTOR)
			{
				//You have a sniper. Stay where you're at.
				RequestSiegeAssistance(bs, TACTICAL_HOLDPOSITION);
				if (botstates[i] && g_entities[i].client->sess.sessionTeam == g_entities[bs->client].client->sess.
					sessionTeam)

					RequestSiegeAssistance(bs, Q_irand(REPLY_YES, REPLY_COMING));
			}
			else if (level.gametype != GT_FFA && g_entities[i].client->ps.weapon == g_entities[bs->client].client->ps.
				weapon)
			{
				//We have the same weapon. Split up.
				RequestSiegeAssistance(bs, TACTICAL_SPLIT);

				if (botstates[i] && g_entities[i].client->sess.sessionTeam == g_entities[bs->client].client->sess.
					sessionTeam)

					RequestSiegeAssistance(bs, REPLY_YES);
			}

			if (distcheck < closest && (InFieldOfVision(bs->viewangles, 90, a) && !BotMindTricked(bs->client, i) ||
				BotCanHear(bs, &g_entities[i], distcheck)) && OrgVisible(bs->eye, g_entities[i].client->ps.origin, -1))
			{
				if (BotMindTricked(bs->client, i))
				{
					if (distcheck < 256 || level.time - g_entities[i].client->dangerTime < 100)
					{
						if (!hasEnemyDist || distcheck < hasEnemyDist - 128)
						{
							//if we have an enemy, only switch to closer if he is 128+ closer to avoid flipping out
							if (!noAttackNonJM || g_entities[i].client->ps.isJediMaster)
							{
								closest = distcheck;
								bestindex = i;
							}
						}
					}
				}
				else
				{
					if (!hasEnemyDist || distcheck < hasEnemyDist - 128)
					{
						//if we have an enemy, only switch to closer if he is 128+ closer to avoid flipping out
						if (!noAttackNonJM || g_entities[i].client->ps.isJediMaster)
						{
							closest = distcheck;
							bestindex = i;
						}
					}
				}
			}
		}
		i++;
	}

	return bestindex;
}

//the main scan regular scan for enemies for the TAB Bot
void Advanced_ScanforEnemies(bot_state_t* bs)
{
	vec3_t a, EnemyOrigin;
	int closeEnemyNum = -1;
	float closestdist = 99999;
	int i;
	float distcheck;
	float hasEnemyDist = 0;
	int currentEnemyNum = -1;

	if (bs->currentEnemy)
	{
		if (bs->currentEnemy->health < 1
			|| bs->currentEnemy->client && bs->currentEnemy->client->sess.sessionTeam == TEAM_SPECTATOR
			|| bs->currentEnemy->s.eFlags & EF_DEAD
			|| bs->currentEnemy->client && bs->currentEnemy->client->ps.eFlags & EF_DEAD
			|| OnSameTeam(bs->currentEnemy, &g_entities[bs->cur_ps.clientNum]))
		{
			//target died or because invalid, move to next target
			bs->currentEnemy = NULL;
		}
		else
		{
			return;
		}
	}

	bs->enemyScanTime = level.time + 5000;

	if (bs->currentEnemy)
	{
		//we're already locked onto an enemy
		if (bs->currentTactic == BOTORDER_SEARCHANDDESTROY
			&& bs->tacticEntity)
		{
			//currently going after search and destroy target
			if (bs->tacticEntity->s.number == bs->currentEnemy->s.number)
			{
				EnemyVisualUpdate(bs);
				return;
			}
		}
		//If you're locked onto an objective, don't lose it.
		else if (bs->currentTactic == BOTORDER_OBJECTIVE && bs->tacticEntity)
		{
			if (bs->tacticEntity->s.number == bs->currentEnemy->s.number)
			{
				EnemyVisualUpdate(bs);
				return;
			}
		}
		else if (bs->frame_Enemy_Vis || bs->enemySeenTime > level.time - 2000)
		{
			return;
		}

		hasEnemyDist = bs->frame_Enemy_Len;
		currentEnemyNum = bs->currentEnemy->s.number;
	}

	for (i = 0; i <= level.num_entities; i++)
	{
		if (i != bs->client
			&& (g_entities[i].client && !OnSameTeam(&g_entities[bs->client], &g_entities[i]) &&
				PassStandardEnemyChecks(bs, &g_entities[i]) && PassLovedOneCheck(bs, &g_entities[i])
				|| g_entities[i].s.eType == ET_NPC && g_entities[i].s.NPC_class != CLASS_VEHICLE && g_entities[i].
				health > 0 && !(g_entities[i].s.eFlags & EF_DEAD)
				|| g_entities[i].s.weapon == WP_EMPLACED_GUN
				&& g_entities[i].teamnodmg != g_entities[bs->client].client->sess.sessionTeam
				&& g_entities[i].s.pos.trType == TR_STATIONARY))
		{
			FindOrigin(&g_entities[i], EnemyOrigin);

			VectorSubtract(EnemyOrigin, bs->eye, a);
			distcheck = VectorLength(a);
			vectoangles(a, a);
			a[PITCH] = a[ROLL] = 0;

			if (distcheck < closestdist && (InFieldOfVision(bs->viewangles, 90, a)
				&& !BotMindTricked(bs->client, i)
				|| BotCanHear(bs, &g_entities[i], distcheck))
				&& OrgVisible(bs->eye, EnemyOrigin, -1))
			{
				if (!hasEnemyDist || distcheck < hasEnemyDist - 128 || currentEnemyNum == i)
				{
					//if we have an enemy, only switch to closer if he is 128+ closer to avoid flipping out
					closestdist = distcheck;
					closeEnemyNum = i;
				}
			}
		}
	}

	if (closeEnemyNum == -1)
	{
		if (bs->currentEnemy)
		{
			//no enemies in the area but we were locked on previously.  Clear frame visual data so
			//we know that we should go go try to find them again.
			bs->frame_Enemy_Vis = 0;
		}
		else
		{
			//we're all along.  No real need to update stuff in this case.
		}
	}
	else
	{
		//have a target, update their data
		vec3_t enemy_angles;
		FindOrigin(&g_entities[closeEnemyNum], EnemyOrigin);
		FindAngles(&g_entities[closeEnemyNum], enemy_angles);

		bs->frame_Enemy_Len = closestdist;
		bs->frame_Enemy_Vis = 1;
		bs->currentEnemy = &g_entities[closeEnemyNum];
		VectorCopy(EnemyOrigin, bs->lastEnemySpotted);
		VectorCopy(enemy_angles, bs->lastEnemyAngles);
		bs->lastEnemyAngles[PITCH] = bs->lastEnemyAngles[ROLL] = 0;
		bs->enemySeenTime = level.time + 2000;
	}

	while (i <= MAX_CLIENTS)
	{
		if (i != bs->client && g_entities[i].client && !OnSameTeam(&g_entities[bs->client], &g_entities[i]) &&
			PassStandardEnemyChecks(bs, &g_entities[i]) && BotPVSCheck(g_entities[i].client->ps.origin, bs->eye) &&
			PassLovedOneCheck(bs, &g_entities[i]))
		{
			VectorSubtract(g_entities[i].client->ps.origin, bs->eye, a);
			distcheck = VectorLength(a);
			vectoangles(a, a);

			if (g_entities[i].client->ps.isJediMaster)
			{
				//make us think the Jedi Master is close so we'll attack him above all
				distcheck = 1;
			}
			if (g_entities[bs->client].health < 75)
			{
				//I need a doctor!
				RequestSiegeAssistance(bs, REQUEST_MEDIC);
			}
			else if (g_entities[bs->client].health < 30)
			{
				//Sniper!
				RequestSiegeAssistance(bs, REQUEST_MEDIC);
			}
			else if (g_entities[i].client->ps.weapon == WP_DISRUPTOR &&
				g_entities[i].client->sess.sessionTeam != g_entities[bs->client].client->sess.sessionTeam)
			{
				//Sniper!
				RequestSiegeAssistance(bs, SPOTTED_SNIPER);
			}
			else if ((g_entities[i].client->ps.weapon == WP_REPEATER ||
				g_entities[i].client->ps.weapon == WP_BOWCASTER ||
				g_entities[i].client->ps.weapon == WP_FLECHETTE ||
				g_entities[i].client->ps.weapon == WP_THERMAL) &&
				(g_entities[bs->client].client->ps.weapon == WP_REPEATER ||
					g_entities[bs->client].client->ps.weapon == WP_BOWCASTER ||
					g_entities[bs->client].client->ps.weapon == WP_FLECHETTE))
			{
				//Troops, incoming
				RequestSiegeAssistance(bs, SPOTTED_TROOPS);
			}
			else if (g_entities[i].client->ps.weapon == WP_ROCKET_LAUNCHER ||
				g_entities[i].client->ps.weapon == WP_CONCUSSION)
			{
				//Crap, he has a bigger gun than us.
				RequestSiegeAssistance(bs, REQUEST_ASSISTANCE);
			}
			else if (g_entities[bs->client].client->ps.weapon == WP_BLASTER ||
				g_entities[bs->client].client->ps.weapon == WP_BRYAR_PISTOL ||
				g_entities[bs->client].client->ps.weapon == WP_STUN_BATON ||
				g_entities[bs->client].client->ps.weapon == WP_DEMP2)
			{
				//Cover me, I have a shitty weapon.
				RequestSiegeAssistance(bs, TACTICAL_COVERME);

				if (botstates[i] && g_entities[i].client->sess.sessionTeam == g_entities[bs->client].client->sess.
					sessionTeam)

					RequestSiegeAssistance(bs, Q_irand(REPLY_YES, REPLY_COMING));
			}
			else if (g_entities[i].client->ps.weapon == WP_BLASTER ||
				g_entities[i].client->ps.weapon == WP_BRYAR_PISTOL ||
				g_entities[i].client->ps.weapon == WP_STUN_BATON ||
				g_entities[i].client->ps.weapon == WP_DEMP2)
			{
				//You have a shitty weapon. Follow me.
				RequestSiegeAssistance(bs, TACTICAL_FOLLOW);

				if (botstates[i] && g_entities[i].client->sess.sessionTeam == g_entities[bs->client].client->sess.
					sessionTeam)

					RequestSiegeAssistance(bs, Q_irand(REPLY_YES, REPLY_COMING));
			}
			else if (g_entities[i].client->ps.weapon == WP_DISRUPTOR)
			{
				//You have a sniper. Stay where you're at.
				RequestSiegeAssistance(bs, TACTICAL_HOLDPOSITION);
				if (botstates[i] && g_entities[i].client->sess.sessionTeam == g_entities[bs->client].client->sess.
					sessionTeam)

					RequestSiegeAssistance(bs, Q_irand(REPLY_YES, REPLY_COMING));
			}
			else if (level.gametype != GT_FFA && g_entities[i].client->ps.weapon == g_entities[bs->client].client->ps.
				weapon)
			{
				//We have the same weapon. Split up.
				RequestSiegeAssistance(bs, TACTICAL_SPLIT);

				if (botstates[i] && g_entities[i].client->sess.sessionTeam == g_entities[bs->client].client->sess.
					sessionTeam)

					RequestSiegeAssistance(bs, REPLY_YES);
			}

			if (distcheck < closestdist && (InFieldOfVision(bs->viewangles, 90, a) && !BotMindTricked(bs->client, i)
				|| BotCanHear(bs, &g_entities[i], distcheck)) && OrgVisible(
					bs->eye, g_entities[i].client->ps.origin, -1))
			{
				if (BotMindTricked(bs->client, i))
				{
					if (distcheck < 256 || level.time - g_entities[i].client->dangerTime < 100)
					{
						if (!hasEnemyDist || distcheck < hasEnemyDist - 128)
						{
							//if we have an enemy, only switch to closer if he is 128+ closer to avoid flipping out
							if (g_entities[i].client->ps.isJediMaster)
							{
								closestdist = distcheck;
							}
						}
					}
				}
				else
				{
					if (!hasEnemyDist || distcheck < hasEnemyDist - 128)
					{
						//if we have an enemy, only switch to closer if he is 128+ closer to avoid flipping out
						if (g_entities[i].client->ps.isJediMaster)
						{
							closestdist = distcheck;
						}
					}
				}
			}
		}
		i++;
	}
}

int WaitingForNow(bot_state_t* bs, vec3_t goalpos)
{
	vec3_t xybot;
	vec3_t xywp;
	vec3_t a;

	if (!bs->wpCurrent)
	{
		return 0;
	}

	if ((int)goalpos[0] != (int)bs->wpCurrent->origin[0] ||
		(int)goalpos[1] != (int)bs->wpCurrent->origin[1] ||
		(int)goalpos[2] != (int)bs->wpCurrent->origin[2])
	{
		return 0;
	}

	VectorCopy(bs->origin, xybot);
	VectorCopy(bs->wpCurrent->origin, xywp);

	xybot[2] = 0;
	xywp[2] = 0;

	VectorSubtract(xybot, xywp, a);

	if (VectorLength(a) < 16 && bs->frame_Waypoint_Len > 100)
	{
		if (CheckForFunc(bs->origin, bs->client))
		{
			return 1; //we're probably standing on an elevator and riding up/down. Or at least we hope so.
		}
	}
	else if (VectorLength(a) < 64 && bs->frame_Waypoint_Len > 64 &&
		CheckForFunc(bs->origin, bs->client))
	{
		bs->noUseTime = level.time + 2000;
	}

	return 0;
}

//get an ideal distance for us to be at in relation to our opponent
//based on our weapon.
int BotGetWeaponRange(const bot_state_t* bs)
{
	switch (bs->cur_ps.weapon)
	{
	case WP_STUN_BATON:
	case WP_MELEE:
		return BWEAPONRANGE_MELEE;
	case WP_SABER:
		return BWEAPONRANGE_SABER;
	case WP_BRYAR_PISTOL:
	case WP_BLASTER:
	case WP_DISRUPTOR:
	case WP_REPEATER:
		return BWEAPONRANGE_MID;
	case WP_BOWCASTER:
	case WP_DEMP2:
	case WP_FLECHETTE:
	case WP_ROCKET_LAUNCHER:
	case WP_THERMAL:
	case WP_TRIP_MINE:
	case WP_DET_PACK:
		return BWEAPONRANGE_LONG;
	default:
		return BWEAPONRANGE_MID;
	}
}

//see if we want to run away from the opponent for whatever reason
int BotIsAChickenWuss(bot_state_t* bs)
{
	if (gLevelFlags & LEVELFLAG_IMUSTNTRUNAWAY)
	{
		//The level says we mustn't run away!
		return 0;
	}

	if (level.gametype == GT_SINGLE_PLAYER)
	{
		//"missions" (not really)
		return 0;
	}

	if (level.gametype == GT_JEDIMASTER && !bs->cur_ps.isJediMaster)
	{
		//Then you may know no fear.
		//Well, unless he's strong.
		if (bs->currentEnemy && bs->currentEnemy->client &&
			bs->currentEnemy->client->ps.isJediMaster &&
			bs->currentEnemy->health > 40 &&
			bs->cur_ps.weapon < WP_ROCKET_LAUNCHER)
		{
			//explosive weapons are most effective against the Jedi Master
			goto jmPass;
		}
		return 0;
	}

	if (level.gametype == GT_CTF && bs->currentEnemy && bs->currentEnemy->client)
	{
		if (bs->currentEnemy->client->ps.powerups[PW_REDFLAG] ||
			bs->currentEnemy->client->ps.powerups[PW_BLUEFLAG])
		{
			//don't be afraid of flag carriers, they must die!
			return 0;
		}
	}

jmPass:
	if (bs->chickenWussCalculationTime > level.time)
	{
		return 2; //don't want to keep going between two points...
	}

	if (bs->cur_ps.fd.forcePowersActive & 1 << FP_RAGE)
	{
		//don't run while raging
		return 0;
	}

	if (level.gametype == GT_JEDIMASTER && !bs->cur_ps.isJediMaster)
	{
		//be frightened of the jedi master? I guess in this case.
		return 1;
	}

	bs->chickenWussCalculationTime = level.time + MAX_CHICKENWUSS_TIME;

	if (g_entities[bs->client].health < BOT_RUN_HEALTH)
	{
		//we're low on health, let's get away
		return 1;
	}

	const int bWRange = BotGetWeaponRange(bs);

	if (bWRange == BWEAPONRANGE_MELEE || bWRange == BWEAPONRANGE_SABER)
	{
		if (bWRange != BWEAPONRANGE_SABER || !bs->saberSpecialist)
		{
			//run away if we're using melee, or if we're using a saber and not a "saber specialist"
			return 1;
		}
	}

	if (bs->cur_ps.weapon == WP_BRYAR_PISTOL)
	{
		//the bryar is a weak weapon, so just try to find a new one if it's what you're having to use
		return 1;
	}

	if (bs->currentEnemy && bs->currentEnemy->client &&
		bs->currentEnemy->client->ps.weapon == WP_SABER &&
		bs->frame_Enemy_Len < 512 && bs->cur_ps.weapon != WP_SABER)
	{
		//if close to an enemy with a saber and not using a saber, then try to back off
		return 1;
	}

	if (level.time - bs->cur_ps.electrifyTime < 16000)
	{
		//lightning is dangerous.
		return 1;
	}

	//didn't run, reset the timer
	bs->chickenWussCalculationTime = 0;

	return 0;
}

//look for "bad things". bad things include detpacks, thermal detonators,
//and other dangerous explodey items.
gentity_t* GetNearestBadThing(bot_state_t* bs)
{
	int i = 0;
	float glen;
	int bestindex = 0;
	float bestdist = 800; //if not within a radius of 800, it's no threat anyway
	int foundindex = 0;
	float factor;
	trace_t tr;

	while (i < level.num_entities)
	{
		vec3_t hold;
		const gentity_t* ent = &g_entities[i];

		if (ent &&
			!ent->client &&
			ent->inuse &&
			ent->damage &&
			/*(ent->s.weapon == WP_THERMAL || ent->s.weapon == WP_FLECHETTE)*/
			ent->s.weapon &&
			ent->splashDamage ||
			ent &&
			ent->genericValue5 == 1000 &&
			ent->inuse &&
			ent->health > 0 &&
			ent->genericValue3 != bs->client &&
			g_entities[ent->genericValue3].client && !OnSameTeam(&g_entities[bs->client],
				&g_entities[ent->genericValue3]))
		{
			//try to escape from anything with a non-0 s.weapon and non-0 damage. This hopefully only means dangerous projectiles.
			//Or a sentry gun if bolt_Head == 1000. This is a terrible hack, yes.
			VectorSubtract(bs->origin, ent->r.currentOrigin, hold);
			glen = VectorLength(hold);

			if (ent->s.weapon != WP_THERMAL && ent->s.weapon != WP_FLECHETTE &&
				ent->s.weapon != WP_DET_PACK && ent->s.weapon != WP_TRIP_MINE)
			{
				factor = 0.5;

				if (ent->s.weapon && glen <= 256 && bs->settings.skill > 2)
				{
					//it's a projectile so push it away
					bs->doForcePush = level.time + 700;
					//trap->Print("PUSH PROJECTILE\n");
				}
			}
			else
			{
				factor = 1;
			}

			if (ent->s.weapon == WP_ROCKET_LAUNCHER &&
				(ent->r.ownerNum == bs->client ||
					ent->r.ownerNum > 0 && ent->r.ownerNum < MAX_CLIENTS &&
					g_entities[ent->r.ownerNum].client && OnSameTeam(
						&g_entities[bs->client], &g_entities[ent->r.ownerNum])))
			{
				//don't be afraid of your own rockets or your teammates' rockets
				factor = 0;
			}

			if (ent->s.weapon == WP_DET_PACK &&
				(ent->r.ownerNum == bs->client ||
					ent->r.ownerNum > 0 && ent->r.ownerNum < MAX_CLIENTS &&
					g_entities[ent->r.ownerNum].client && OnSameTeam(
						&g_entities[bs->client], &g_entities[ent->r.ownerNum])))
			{
				//don't be afraid of your own detpacks or your teammates' detpacks
				factor = 0;
			}

			if (ent->s.weapon == WP_TRIP_MINE &&
				(ent->r.ownerNum == bs->client ||
					ent->r.ownerNum > 0 && ent->r.ownerNum < MAX_CLIENTS &&
					g_entities[ent->r.ownerNum].client && OnSameTeam(
						&g_entities[bs->client], &g_entities[ent->r.ownerNum])))
			{
				//don't be afraid of your own trip mines or your teammates' trip mines
				factor = 0;
			}

			if (ent->s.weapon == WP_THERMAL &&
				(ent->r.ownerNum == bs->client ||
					ent->r.ownerNum > 0 && ent->r.ownerNum < MAX_CLIENTS &&
					g_entities[ent->r.ownerNum].client && OnSameTeam(
						&g_entities[bs->client], &g_entities[ent->r.ownerNum])))
			{
				//don't be afraid of your own thermals or your teammates' thermals
				factor = 0;
			}

			if (glen < bestdist * factor && BotPVSCheck(bs->origin, ent->s.pos.trBase))
			{
				trap->Trace(&tr, bs->origin, NULL, NULL, ent->s.pos.trBase, bs->client, MASK_SOLID, qfalse, 0, 0);

				if (tr.fraction == 1 || tr.entityNum == ent->s.number)
				{
					bestindex = i;
					bestdist = glen;
					foundindex = 1;
				}
			}
		}

		if (ent && !ent->client && ent->inuse && ent->damage && ent->s.weapon && ent->r.ownerNum < MAX_CLIENTS && ent->r
			.ownerNum >= 0)
		{
			//if we're in danger of a projectile belonging to someone and don't have an enemy, set the enemy to them
			gentity_t* projOwner = &g_entities[ent->r.ownerNum];

			if (projOwner && projOwner->inuse && projOwner->client)
			{
				if (!bs->currentEnemy)
				{
					if (PassStandardEnemyChecks(bs, projOwner))
					{
						if (PassLovedOneCheck(bs, projOwner))
						{
							VectorSubtract(bs->origin, ent->r.currentOrigin, hold);
							glen = VectorLength(hold);

							if (glen < 512)
							{
								bs->currentEnemy = projOwner;
								bs->enemySeenTime = level.time + ENEMY_FORGET_MS;
							}
						}
					}
				}
			}
		}

		i++;
	}

	if (foundindex)
	{
		bs->dontGoBack = level.time + 1500;
		return &g_entities[bestindex];
	}
	return NULL;
}

//Keep our CTF priorities on defending our team's flag
int BotDefendFlag(bot_state_t* bs)
{
	wpobject_t* flagPoint;
	vec3_t a;

	if (level.clients[bs->client].sess.sessionTeam == TEAM_RED)
	{
		flagPoint = flagRed;
	}
	else if (level.clients[bs->client].sess.sessionTeam == TEAM_BLUE)
	{
		flagPoint = flagBlue;
	}
	else
	{
		return 0;
	}

	if (!flagPoint)
	{
		return 0;
	}

	VectorSubtract(bs->origin, flagPoint->origin, a);

	if (VectorLength(a) > BASE_GUARD_DISTANCE)
	{
		bs->wpDestination = flagPoint;
	}

	return 1;
}

//Keep our CTF priorities on getting the other team's flag
int BotGetEnemyFlag(bot_state_t* bs)
{
	wpobject_t* flagPoint;
	vec3_t a;

	if (level.clients[bs->client].sess.sessionTeam == TEAM_RED)
	{
		flagPoint = flagBlue;
	}
	else if (level.clients[bs->client].sess.sessionTeam == TEAM_BLUE)
	{
		flagPoint = flagRed;
	}
	else
	{
		return 0;
	}

	if (!flagPoint)
	{
		return 0;
	}

	VectorSubtract(bs->origin, flagPoint->origin, a);

	if (VectorLength(a) > BASE_GETENEMYFLAG_DISTANCE)
	{
		bs->wpDestination = flagPoint;
	}

	return 1;
}

//Our team's flag is gone, so try to get it back
int BotGetFlagBack(bot_state_t* bs)
{
	int i = 0;
	int my_flag;
	int foundCarrier = 0;
	const gentity_t* ent = NULL;

	if (level.clients[bs->client].sess.sessionTeam == TEAM_RED)
	{
		my_flag = PW_REDFLAG;
	}
	else
	{
		my_flag = PW_BLUEFLAG;
	}

	while (i < MAX_CLIENTS)
	{
		ent = &g_entities[i];

		if (ent && ent->client && ent->client->ps.powerups[my_flag] && !OnSameTeam(&g_entities[bs->client], ent))
		{
			foundCarrier = 1;
			break;
		}

		i++;
	}

	if (!foundCarrier)
	{
		return 0;
	}

	if (!ent)
	{
		return 0;
	}

	if (bs->wpDestSwitchTime < level.time)
	{
		vec3_t usethisvec;
		if (ent->client)
		{
			VectorCopy(ent->client->ps.origin, usethisvec);
		}
		else
		{
			VectorCopy(ent->s.origin, usethisvec);
		}

		const int temp_int = GetNearestVisibleWP(usethisvec, 0);

		if (temp_int != -1 && TotalTrailDistance(bs->wpCurrent->index, temp_int, bs) != -1)
		{
			bs->wpDestination = gWPArray[temp_int];
			bs->wpDestSwitchTime = level.time + Q_irand(1000, 5000);
		}
	}

	return 1;
}

void DetermineCTFGoal(bot_state_t* bs)
{
	int NumOffence = 0; //number of bots on offence
	int NumDefense = 0; //number of bots on defence

	//clear out the current tactic
	bs->currentTactic = BOTORDER_OBJECTIVE;
	bs->tacticEntity = NULL;

	for (int i = 0; i < MAX_CLIENTS; i++)
	{
		const bot_state_t* tempbot = botstates[i];

		if (!tempbot || !tempbot->inuse || tempbot->client == bs->client)
		{
			//this bot isn't in use or this is the current bot
			continue;
		}

		if (g_entities[tempbot->client].client->sess.sessionTeam
			!= g_entities[bs->client].client->sess.sessionTeam)
		{
			continue;
		}

		if (tempbot->currentTactic == BOTORDER_OBJECTIVE)
		{
			//this bot is going for/defending the flag
			if (tempbot->objectiveType == OT_CAPTURE)
			{
				NumOffence++;
			}
			else
			{
				//it's on defense
				NumDefense++;
			}
		}
	}

	if (NumDefense < NumOffence)
	{
		//we have less defenders than attackers.  Go on the defense.
		bs->objectiveType = OT_DEFENDCAPTURE;
	}
	else
	{
		//go on the attack
		bs->objectiveType = OT_CAPTURE;
	}
}

//Someone else on our team has the enemy flag, so try to get
//to their assistance
int BotGuardFlagCarrier(bot_state_t* bs)
{
	int i = 0;
	int enemy_flag;
	int foundCarrier = 0;
	const gentity_t* ent = NULL;

	if (level.clients[bs->client].sess.sessionTeam == TEAM_RED)
	{
		enemy_flag = PW_BLUEFLAG;
	}
	else
	{
		enemy_flag = PW_REDFLAG;
	}

	while (i < MAX_CLIENTS)
	{
		ent = &g_entities[i];

		if (ent && ent->client && ent->client->ps.powerups[enemy_flag] && OnSameTeam(&g_entities[bs->client], ent))
		{
			foundCarrier = 1;
			break;
		}

		i++;
	}

	if (!foundCarrier)
	{
		return 0;
	}

	if (!ent)
	{
		return 0;
	}

	if (bs->wpDestSwitchTime < level.time)
	{
		vec3_t usethisvec;
		if (ent->client)
		{
			VectorCopy(ent->client->ps.origin, usethisvec);
		}
		else
		{
			VectorCopy(ent->s.origin, usethisvec);
		}

		const int temp_int = GetNearestVisibleWP(usethisvec, 0);

		if (temp_int != -1 && TotalTrailDistance(bs->wpCurrent->index, temp_int, bs) != -1)
		{
			bs->wpDestination = gWPArray[temp_int];
			bs->wpDestSwitchTime = level.time + Q_irand(1000, 5000);
		}
	}

	return 1;
}

//We have the flag, let's get it home.
int BotGetFlagHome(bot_state_t* bs)
{
	wpobject_t* flagPoint;
	vec3_t a;

	if (level.clients[bs->client].sess.sessionTeam == TEAM_RED)
	{
		flagPoint = flagRed;
	}
	else if (level.clients[bs->client].sess.sessionTeam == TEAM_BLUE)
	{
		flagPoint = flagBlue;
	}
	else
	{
		return 0;
	}

	if (!flagPoint)
	{
		return 0;
	}

	VectorSubtract(bs->origin, flagPoint->origin, a);

	if (VectorLength(a) > BASE_FLAGWAIT_DISTANCE)
	{
		bs->wpDestination = flagPoint;
	}

	return 1;
}

void GetNewFlagPoint(const wpobject_t* wp, const gentity_t* flagEnt, const int team)
{
	//get the nearest possible waypoint to the flag since it's not in its original position
	int i = 0;
	vec3_t a, mins, maxs;
	int bestindex = 0;
	int foundindex = 0;
	trace_t tr;

	mins[0] = -15;
	mins[1] = -15;
	mins[2] = -5;
	maxs[0] = 15;
	maxs[1] = 15;
	maxs[2] = 5;

	VectorSubtract(wp->origin, flagEnt->s.pos.trBase, a);

	float bestdist = VectorLength(a);

	if (bestdist <= WP_KEEP_FLAG_DIST)
	{
		trap->Trace(&tr, wp->origin, mins, maxs, flagEnt->s.pos.trBase, flagEnt->s.number, MASK_SOLID, qfalse, 0, 0);

		if (tr.fraction == 1)
		{
			//this point is good
			return;
		}
	}

	while (i < gWPNum)
	{
		VectorSubtract(gWPArray[i]->origin, flagEnt->s.pos.trBase, a);
		const float testdist = VectorLength(a);

		if (testdist < bestdist)
		{
			trap->Trace(&tr, gWPArray[i]->origin, mins, maxs, flagEnt->s.pos.trBase, flagEnt->s.number, MASK_SOLID,
				qfalse, 0, 0);

			if (tr.fraction == 1)
			{
				foundindex = 1;
				bestindex = i;
				bestdist = testdist;
			}
		}

		i++;
	}

	if (foundindex)
	{
		if (team == TEAM_RED)
		{
			flagRed = gWPArray[bestindex];
		}
		else
		{
			flagBlue = gWPArray[bestindex];
		}
	}
}

//See if our CTF state should take priority in our nav routines
int CTFTakesPriority(bot_state_t* bs)
{
	gentity_t* ent;
	int enemy_flag;
	int my_flag;
	int enemyHasOurFlag = 0;
	//int weHaveEnemyFlag = 0;
	int numOnMyTeam = 0;
	int numOnEnemyTeam = 0;
	int numAttackers = 0;
	int numDefenders = 0;
	int i;
	int dosw = 0;
	wpobject_t* dest_sw = NULL;
#ifdef BOT_CTF_DEBUG
	vec3_t t;

	trap->Print("CTFSTATE: %s\n", ctfStateNames[bs->ctfState]);
#endif

	if (level.gametype != GT_CTF && level.gametype != GT_CTY)
	{
		return 0;
	}

	if (bs->cur_ps.weapon == WP_BRYAR_PISTOL &&
		level.time - bs->lastDeadTime < BOT_MAX_WEAPON_GATHER_TIME)
	{
		//get the nearest weapon laying around base before heading off for battle
		int idleWP = GetBestIdleGoal(bs);

		if (idleWP != -1 && gWPArray[idleWP] && gWPArray[idleWP]->inuse)
		{
			if (bs->wpDestSwitchTime < level.time)
			{
				bs->wpDestination = gWPArray[idleWP];
			}
			return 1;
		}
	}
	else if (bs->cur_ps.weapon == WP_BRYAR_PISTOL &&
		level.time - bs->lastDeadTime < BOT_MAX_WEAPON_CHASE_CTF &&
		bs->wpDestination && bs->wpDestination->weight)
	{
		dest_sw = bs->wpDestination;
		dosw = 1;
	}

	if (level.clients[bs->client].sess.sessionTeam == TEAM_RED)
	{
		my_flag = PW_REDFLAG;
	}
	else
	{
		my_flag = PW_BLUEFLAG;
	}

	if (level.clients[bs->client].sess.sessionTeam == TEAM_RED)
	{
		enemy_flag = PW_BLUEFLAG;
	}
	else
	{
		enemy_flag = PW_REDFLAG;
	}

	if (!flagRed || !flagBlue ||
		!flagRed->inuse || !flagBlue->inuse ||
		!eFlagRed || !eFlagBlue)
	{
		return 0;
	}

#ifdef BOT_CTF_DEBUG
	VectorCopy(flagRed->origin, t);
	t[2] += 128;
	G_TestLine(flagRed->origin, t, 0x0000ff, 500);

	VectorCopy(flagBlue->origin, t);
	t[2] += 128;
	G_TestLine(flagBlue->origin, t, 0x0000ff, 500);
#endif

	if (droppedRedFlag && droppedRedFlag->flags & FL_DROPPED_ITEM)
	{
		GetNewFlagPoint(flagRed, droppedRedFlag, TEAM_RED);
	}
	else
	{
		flagRed = oFlagRed;
	}

	if (droppedBlueFlag && droppedBlueFlag->flags & FL_DROPPED_ITEM)
	{
		GetNewFlagPoint(flagBlue, droppedBlueFlag, TEAM_BLUE);
	}
	else
	{
		flagBlue = oFlagBlue;
	}

	if (!bs->ctfState)
	{
		return 0;
	}

	i = 0;

	while (i < MAX_CLIENTS)
	{
		ent = &g_entities[i];

		if (ent && ent->client)
		{
			/*if (ent->client->ps.powerups[enemyFlag] && OnSameTeam(&g_entities[bs->client], ent))
			{
				weHaveEnemyFlag = 1;
			}
			else */
			if (ent->client->ps.powerups[my_flag] && !OnSameTeam(&g_entities[bs->client], ent))
			{
				enemyHasOurFlag = 1;
			}

			if (OnSameTeam(&g_entities[bs->client], ent))
			{
				numOnMyTeam++;
			}
			else
			{
				numOnEnemyTeam++;
			}

			if (botstates[ent->s.number])
			{
				if (botstates[ent->s.number]->ctfState == CTFSTATE_ATTACKER ||
					botstates[ent->s.number]->ctfState == CTFSTATE_RETRIEVAL)
				{
					numAttackers++;
				}
				else
				{
					numDefenders++;
				}
			}
			else
			{
				//assume real players to be attackers in our logic
				numAttackers++;
			}
		}
		i++;
	}

	if (bs->cur_ps.powerups[enemy_flag])
	{
		if ((numOnMyTeam < 2 || !numAttackers) && enemyHasOurFlag)
		{
			bs->ctfState = CTFSTATE_RETRIEVAL;
		}
		else
		{
			bs->ctfState = CTFSTATE_GETFLAGHOME;
		}
	}
	else if (bs->ctfState == CTFSTATE_GETFLAGHOME)
	{
		bs->ctfState = 0;
	}

	if (bs->state_Forced)
	{
		bs->ctfState = bs->state_Forced;
	}

	if (bs->ctfState == CTFSTATE_DEFENDER)
	{
		if (BotDefendFlag(bs))
		{
			goto success;
		}
	}

	if (bs->ctfState == CTFSTATE_ATTACKER)
	{
		if (BotGetEnemyFlag(bs))
		{
			goto success;
		}
	}

	if (bs->ctfState == CTFSTATE_RETRIEVAL)
	{
		if (BotGetFlagBack(bs))
		{
			goto success;
		}
		//can't find anyone on another team being a carrier, so ignore this priority
		bs->ctfState = 0;
	}

	if (bs->ctfState == CTFSTATE_GUARDCARRIER)
	{
		if (BotGuardFlagCarrier(bs))
		{
			goto success;
		}
		//can't find anyone on our team being a carrier, so ignore this priority
		bs->ctfState = 0;
	}

	if (bs->ctfState == CTFSTATE_GETFLAGHOME)
	{
		if (BotGetFlagHome(bs))
		{
			goto success;
		}
	}

	return 0;

success:
	if (dosw)
	{
		//allow ctf code to run, but if after a particular item then keep going after it
		bs->wpDestination = dest_sw;
	}

	return 1;
}

int EntityVisibleBox(vec3_t org1, vec3_t mins, vec3_t maxs, vec3_t org2, const int ignore, const int ignore2)
{
	trace_t tr;

	trap->Trace(&tr, org1, mins, maxs, org2, ignore, MASK_SOLID, qfalse, 0, 0);

	if (tr.fraction == 1 && !tr.startsolid && !tr.allsolid)
	{
		return 1;
	}
	if (tr.entityNum != ENTITYNUM_NONE && tr.entityNum == ignore2)
	{
		return 1;
	}

	return 0;
}

//Get the closest objective for siege and go after it
int Siege_TargetClosestObjective(bot_state_t* bs, const int flag)
{
	int i = 0;
	int bestindex = -1;
	float testdistance;
	float bestdistance = 999999999.9f;
	vec3_t a, dif;
	vec3_t mins, maxs;
	gentity_t* goalent = &g_entities[bs->wpDestination->associated_entity];

	mins[0] = -1;
	mins[1] = -1;
	mins[2] = -1;

	maxs[0] = 1;
	maxs[1] = 1;
	maxs[2] = 1;

	if (bs->wpDestination && bs->wpDestination->flags & flag && bs->wpDestination->associated_entity != ENTITYNUM_NONE
		&&
		&g_entities[bs->wpDestination->associated_entity] && g_entities[bs->wpDestination->associated_entity].use)
	{
		goto hasPoint;
	}

	while (i < gWPNum)
	{
		if (gWPArray[i] && gWPArray[i]->inuse && gWPArray[i]->flags & flag && gWPArray[i]->associated_entity !=
			ENTITYNUM_NONE &&
			&g_entities[gWPArray[i]->associated_entity] && g_entities[gWPArray[i]->associated_entity].use)
		{
			VectorSubtract(gWPArray[i]->origin, bs->origin, a);
			testdistance = VectorLength(a);

			if (testdistance < bestdistance)
			{
				bestdistance = testdistance;
				bestindex = i;
			}
		}

		i++;
	}

	if (bestindex != -1)
	{
		bs->wpDestination = gWPArray[bestindex];
	}
	else
	{
		return 0;
	}
hasPoint:

	if (!goalent)
	{
		return 0;
	}

	VectorSubtract(bs->origin, bs->wpDestination->origin, a);

	testdistance = VectorLength(a);

	dif[0] = (goalent->r.absmax[0] + goalent->r.absmin[0]) / 2;
	dif[1] = (goalent->r.absmax[1] + goalent->r.absmin[1]) / 2;
	dif[2] = (goalent->r.absmax[2] + goalent->r.absmin[2]) / 2;
	//brush models can have tricky origins, so this is our hacky method of getting the center point

	if (goalent->takedamage && testdistance < BOT_MIN_SIEGE_GOAL_SHOOT &&
		EntityVisibleBox(bs->origin, mins, maxs, dif, bs->client, goalent->s.number))
	{
		bs->shootGoal = goalent;
		bs->touchGoal = NULL;
	}
	else if (goalent->use && testdistance < BOT_MIN_SIEGE_GOAL_TRAVEL)
	{
		bs->shootGoal = NULL;
		bs->touchGoal = goalent;
	}
	else
	{
		//don't know how to handle this goal object!
		bs->shootGoal = NULL;
		bs->touchGoal = NULL;
	}

	if (BotGetWeaponRange(bs) == BWEAPONRANGE_MELEE ||
		BotGetWeaponRange(bs) == BWEAPONRANGE_SABER)
	{
		bs->shootGoal = NULL; //too risky
	}

	if (bs->touchGoal)
	{
		//trap->Print("Please, master, let me touch it!\n");
		VectorCopy(dif, bs->goalPosition);
	}

	return 1;
}

void Siege_DefendFromAttackers(bot_state_t* bs)
{
	//this may be a little cheap, but the best way to find our defending point is probably
	int i = 0;
	int bestindex = -1;
	float bestdist = 999999;

	while (i < MAX_CLIENTS)
	{
		const gentity_t* ent = &g_entities[i];

		if (ent && ent->client && ent->client->sess.sessionTeam != g_entities[bs->client].client->sess.sessionTeam &&
			ent->health > 0 && ent->client->sess.sessionTeam != TEAM_SPECTATOR)
		{
			vec3_t a;
			VectorSubtract(ent->client->ps.origin, bs->origin, a);

			const float testdist = VectorLength(a);

			if (testdist < bestdist)
			{
				bestindex = i;
				bestdist = testdist;
			}
		}

		i++;
	}

	if (bestindex == -1)
	{
		return;
	}

	const int wp_close = GetNearestVisibleWP(g_entities[bestindex].client->ps.origin, -1);

	if (wp_close != -1 && gWPArray[wp_close] && gWPArray[wp_close]->inuse)
	{
		bs->wpDestination = gWPArray[wp_close];
		bs->destinationGrabTime = level.time + 10000;
	}
}

//how many defenders on our team?
int Siege_CountDefenders(const bot_state_t* bs)
{
	int i = 0;
	int num = 0;

	while (i < MAX_CLIENTS)
	{
		const gentity_t* ent = &g_entities[i];
		const bot_state_t* bot = botstates[i];

		if (ent && ent->client && bot)
		{
			if (bot->siegeState == SIEGESTATE_DEFENDER &&
				ent->client->sess.sessionTeam == g_entities[bs->client].client->sess.sessionTeam)
			{
				num++;
			}
		}

		i++;
	}

	return num;
}

//how many other players on our team?
int Siege_CountTeammates(const bot_state_t* bs)
{
	int i = 0;
	int num = 0;

	while (i < MAX_CLIENTS)
	{
		const gentity_t* ent = &g_entities[i];

		if (ent && ent->client)
		{
			if (ent->client->sess.sessionTeam == g_entities[bs->client].client->sess.sessionTeam)
			{
				num++;
			}
		}

		i++;
	}

	return num;
}

//see if siege objective completion should take priority in our
//nav routines.
int SiegeTakesPriority(bot_state_t* bs)
{
	int attacker;
	int flag_for_attackable_objective;
	wpobject_t* dest_sw = NULL;
	int dosw = 0;
	vec3_t dif;
	trace_t tr;

	if (level.gametype != GT_SIEGE)
	{
		return 0;
	}

	const gclient_t* bcl = g_entities[bs->client].client;

	if (!bcl)
	{
		return 0;
	}

	if (bs->cur_ps.weapon == WP_BRYAR_PISTOL &&
		level.time - bs->lastDeadTime < BOT_MAX_WEAPON_GATHER_TIME)
	{
		//get the nearest weapon laying around base before heading off for battle
		const int idleWP = GetBestIdleGoal(bs);

		if (idleWP != -1 && gWPArray[idleWP] && gWPArray[idleWP]->inuse)
		{
			if (bs->wpDestSwitchTime < level.time)
			{
				bs->wpDestination = gWPArray[idleWP];
			}
			return 1;
		}
	}
	else if (bs->cur_ps.weapon == WP_BRYAR_PISTOL &&
		level.time - bs->lastDeadTime < BOT_MAX_WEAPON_CHASE_TIME &&
		bs->wpDestination && bs->wpDestination->weight)
	{
		dest_sw = bs->wpDestination;
		dosw = 1;
	}

	if (bcl->sess.sessionTeam == SIEGETEAM_TEAM1)
	{
		attacker = imperial_attackers;
		flag_for_attackable_objective = WPFLAG_SIEGE_IMPERIALOBJ;
	}
	else
	{
		attacker = rebel_attackers;
		flag_for_attackable_objective = WPFLAG_SIEGE_REBELOBJ;
	}

	if (attacker)
	{
		bs->siegeState = SIEGESTATE_ATTACKER;
	}
	else
	{
		bs->siegeState = SIEGESTATE_DEFENDER;
		const int defenders = Siege_CountDefenders(bs);
		const int teammates = Siege_CountTeammates(bs);

		if (defenders > teammates / 3 && teammates > 1)
		{
			//devote around 1/4 of our team to completing our own side goals even if we're a defender.
			//If we have no side goals we will realize that later on and join the defenders
			bs->siegeState = SIEGESTATE_ATTACKER;
		}
	}

	if (bs->state_Forced)
	{
		bs->siegeState = bs->state_Forced;
	}

	if (bs->siegeState == SIEGESTATE_ATTACKER)
	{
		if (!Siege_TargetClosestObjective(bs, flag_for_attackable_objective))
		{
			//looks like we have no goals other than to keep the other team from completing objectives
			Siege_DefendFromAttackers(bs);
			if (bs->shootGoal)
			{
				dif[0] = (bs->shootGoal->r.absmax[0] + bs->shootGoal->r.absmin[0]) / 2;
				dif[1] = (bs->shootGoal->r.absmax[1] + bs->shootGoal->r.absmin[1]) / 2;
				dif[2] = (bs->shootGoal->r.absmax[2] + bs->shootGoal->r.absmin[2]) / 2;

				if (!BotPVSCheck(bs->origin, dif))
				{
					bs->shootGoal = NULL;
				}
				else
				{
					trap->Trace(&tr, bs->origin, NULL, NULL, dif, bs->client, MASK_SOLID, qfalse, 0, 0);

					if (tr.fraction != 1 && tr.entityNum != bs->shootGoal->s.number)
					{
						bs->shootGoal = NULL;
					}
				}
			}
		}
	}
	else if (bs->siegeState == SIEGESTATE_DEFENDER)
	{
		Siege_DefendFromAttackers(bs);
		if (bs->shootGoal)
		{
			dif[0] = (bs->shootGoal->r.absmax[0] + bs->shootGoal->r.absmin[0]) / 2;
			dif[1] = (bs->shootGoal->r.absmax[1] + bs->shootGoal->r.absmin[1]) / 2;
			dif[2] = (bs->shootGoal->r.absmax[2] + bs->shootGoal->r.absmin[2]) / 2;

			if (!BotPVSCheck(bs->origin, dif))
			{
				bs->shootGoal = NULL;
			}
			else
			{
				trap->Trace(&tr, bs->origin, NULL, NULL, dif, bs->client, MASK_SOLID, qfalse, 0, 0);

				if (tr.fraction != 1 && tr.entityNum != bs->shootGoal->s.number)
				{
					bs->shootGoal = NULL;
				}
			}
		}
	}
	else
	{
		//get busy!
		Siege_TargetClosestObjective(bs, flag_for_attackable_objective);
		if (bs->shootGoal)
		{
			dif[0] = (bs->shootGoal->r.absmax[0] + bs->shootGoal->r.absmin[0]) / 2;
			dif[1] = (bs->shootGoal->r.absmax[1] + bs->shootGoal->r.absmin[1]) / 2;
			dif[2] = (bs->shootGoal->r.absmax[2] + bs->shootGoal->r.absmin[2]) / 2;

			if (!BotPVSCheck(bs->origin, dif))
			{
				bs->shootGoal = NULL;
			}
			else
			{
				trap->Trace(&tr, bs->origin, NULL, NULL, dif, bs->client, MASK_SOLID, qfalse, 0, 0);

				if (tr.fraction != 1 && tr.entityNum != bs->shootGoal->s.number)
				{
					bs->shootGoal = NULL;
				}
			}
		}
	}

	if (dosw)
	{
		//allow siege objective code to run, but if after a particular item then keep going after it
		bs->wpDestination = dest_sw;
	}

	return 1;
}

//see if jedi master priorities should take priority in our nav
//routines.
int JMTakesPriority(bot_state_t* bs)
{
	int i = 0;
	gentity_t* the_important_entity;

	if (level.gametype != GT_JEDIMASTER)
	{
		return 0;
	}

	if (bs->cur_ps.isJediMaster)
	{
		return 0;
	}

	//jmState becomes the index for the one who carries the saber. If jmState is -1 then the saber is currently
	//without an owner
	bs->jmState = -1;

	while (i < MAX_CLIENTS)
	{
		if (g_entities[i].client && g_entities[i].inuse &&
			g_entities[i].client->ps.isJediMaster)
		{
			bs->jmState = i;
			break;
		}

		i++;
	}

	if (bs->jmState != -1)
	{
		the_important_entity = &g_entities[bs->jmState];
	}
	else
	{
		the_important_entity = gJMSaberEnt;
	}

	if (the_important_entity && the_important_entity->inuse && bs->destinationGrabTime < level.time)
	{
		int wp_close;
		if (the_important_entity->client)
		{
			wp_close = GetNearestVisibleWP(the_important_entity->client->ps.origin, the_important_entity->s.number);
		}
		else
		{
			wp_close = GetNearestVisibleWP(the_important_entity->r.currentOrigin, the_important_entity->s.number);
		}

		if (wp_close != -1 && gWPArray[wp_close] && gWPArray[wp_close]->inuse)
		{
			bs->wpDestination = gWPArray[wp_close];
			bs->destinationGrabTime = level.time + 4000;
		}
	}

	return 1;
}

//see if we already have an item/powerup/etc. that is associated
//with this waypoint.
int BotHasAssociated(const bot_state_t* bs, const wpobject_t* wp)
{
	if (wp->associated_entity == ENTITYNUM_NONE)
	{
		//make it think this is an item we have so we don't go after nothing
		return 1;
	}

	const gentity_t* as = &g_entities[wp->associated_entity];

	if (!as || !as->item)
	{
		return 0;
	}

	if (as->item->giType == IT_WEAPON)
	{
		if (bs->cur_ps.stats[STAT_WEAPONS] & 1 << as->item->giTag)
		{
			return 1;
		}

		return 0;
	}
	if (as->item->giType == IT_HOLDABLE)
	{
		if (bs->cur_ps.stats[STAT_HOLDABLE_ITEMS] & 1 << as->item->giTag)
		{
			return 1;
		}

		return 0;
	}
	if (as->item->giType == IT_POWERUP)
	{
		if (bs->cur_ps.powerups[as->item->giTag])
		{
			return 1;
		}

		return 0;
	}
	if (as->item->giType == IT_AMMO)
	{
		if (bs->cur_ps.ammo[as->item->giTag] > 10) //hack
		{
			return 1;
		}

		return 0;
	}

	return 0;
}

//we don't really have anything we want to do right now,
//let's just find the best thing to do given the current
//situation.
int GetBestIdleGoal(bot_state_t* bs)
{
	int i = 0;
	int highestweight = 0;
	int desiredindex = -1;

	if (!bs->wpCurrent)
	{
		return -1;
	}

	if (bs->isCamper != 2)
	{
		if (bs->randomNavTime < level.time)
		{
			if (Q_irand(1, 10) < 5)
			{
				bs->randomNav = 1;
			}
			else
			{
				bs->randomNav = 0;
			}

			bs->randomNavTime = level.time + Q_irand(5000, 15000);
		}
	}

	if (bs->randomNav)
	{
		//stop looking for items and/or camping on them
		return -1;
	}

	while (i < gWPNum)
	{
		if (gWPArray[i] &&
			gWPArray[i]->inuse &&
			gWPArray[i]->flags & WPFLAG_GOALPOINT &&
			gWPArray[i]->weight > highestweight &&
			!BotHasAssociated(bs, gWPArray[i]))
		{
			const int traildist = TotalTrailDistance(bs->wpCurrent->index, i, bs);

			if (traildist != -1)
			{
				int dist_to_weight = traildist / 10000;
				dist_to_weight = gWPArray[i]->weight - dist_to_weight;

				if (dist_to_weight > highestweight)
				{
					highestweight = dist_to_weight;
					desiredindex = i;
				}
			}
		}

		i++;
	}

	return desiredindex;
}

//go through the list of possible priorities for navigating
//and work out the best destination point.
void GetIdealDestination(bot_state_t* bs)
{
	int tempInt, idleWP;
	float distChange, plusLen, minusLen;
	vec3_t usethisvec, a;
	gentity_t* badthing;

#ifdef _DEBUG
	trap->Cvar_Update(&bot_nogoals);

	if (bot_nogoals.integer)
	{
		return;
	}
#endif

	if (!bs->wpCurrent)
	{
		return;
	}

	if (level.time - bs->escapeDirTime > 4000)
	{
		badthing = GetNearestBadThing(bs);
	}
	else
	{
		badthing = NULL;
	}

	if (badthing && badthing->inuse &&
		badthing->health > 0 && badthing->takedamage)
	{
		bs->dangerousObject = badthing;
	}
	else
	{
		bs->dangerousObject = NULL;
	}

	if (!badthing && bs->wpDestIgnoreTime > level.time)
	{
		return;
	}

	if (!badthing && bs->dontGoBack > level.time)
	{
		if (bs->wpDestination)
		{
			bs->wpStoreDest = bs->wpDestination;
		}
		bs->wpDestination = NULL;
		return;
	}
	if (!badthing && bs->wpStoreDest)
	{
		//after we finish running away, switch back to our original destination
		bs->wpDestination = bs->wpStoreDest;
		bs->wpStoreDest = NULL;
	}

	if (badthing && bs->wpCamping)
	{
		bs->wpCamping = NULL;
	}

	if (bs->wpCamping)
	{
		bs->wpDestination = bs->wpCamping;
		return;
	}

	if (!badthing && CTFTakesPriority(bs))
	{
		if (bs->ctfState)
		{
			bs->runningToEscapeThreat = 1;
		}
		return;
	}
	if (!badthing && SiegeTakesPriority(bs))
	{
		if (bs->siegeState)
		{
			bs->runningToEscapeThreat = 1;
		}
		return;
	}
	if (!badthing && JMTakesPriority(bs))
	{
		bs->runningToEscapeThreat = 1;
	}

	if (badthing)
	{
		bs->runningLikeASissy = level.time + 100;

		if (bs->wpDestination)
		{
			bs->wpStoreDest = bs->wpDestination;
		}
		bs->wpDestination = NULL;

		if (bs->wpDirection)
		{
			tempInt = bs->wpCurrent->index + 1;
		}
		else
		{
			tempInt = bs->wpCurrent->index - 1;
		}

		if (gWPArray[tempInt] && gWPArray[tempInt]->inuse && bs->escapeDirTime < level.time)
		{
			VectorSubtract(badthing->s.pos.trBase, bs->wpCurrent->origin, a);
			plusLen = VectorLength(a);
			VectorSubtract(badthing->s.pos.trBase, gWPArray[tempInt]->origin, a);
			minusLen = VectorLength(a);

			if (plusLen < minusLen)
			{
				if (bs->wpDirection)
				{
					bs->wpDirection = 0;
				}
				else
				{
					bs->wpDirection = 1;
				}

				bs->wpCurrent = gWPArray[tempInt];

				bs->escapeDirTime = level.time + Q_irand(500, 1000);
			}
		}
		return;
	}

	distChange = 0; //keep the compiler from complaining

	tempInt = BotGetWeaponRange(bs);

	if (tempInt == BWEAPONRANGE_MELEE)
	{
		distChange = 1;
	}
	else if (tempInt == BWEAPONRANGE_SABER)
	{
		distChange = 1;
	}
	else if (tempInt == BWEAPONRANGE_MID)
	{
		distChange = 128;
	}
	else if (tempInt == BWEAPONRANGE_LONG)
	{
		distChange = 300;
	}

	if (bs->revengeEnemy && bs->revengeEnemy->health > 0 &&
		bs->revengeEnemy->client && bs->revengeEnemy->client->pers.connected == CON_CONNECTED)
	{
		//if we hate someone, always try to get to them
		if (bs->wpDestSwitchTime < level.time)
		{
			if (bs->revengeEnemy->client)
			{
				VectorCopy(bs->revengeEnemy->client->ps.origin, usethisvec);
			}
			else
			{
				VectorCopy(bs->revengeEnemy->s.origin, usethisvec);
			}

			tempInt = GetNearestVisibleWP(usethisvec, 0);

			if (tempInt != -1 && TotalTrailDistance(bs->wpCurrent->index, tempInt, bs) != -1)
			{
				bs->wpDestination = gWPArray[tempInt];
				bs->wpDestSwitchTime = level.time + Q_irand(5000, 10000);
			}
		}
	}
	else if (bs->squadLeader && bs->squadLeader->health > 0 &&
		bs->squadLeader->client && bs->squadLeader->client->pers.connected == CON_CONNECTED)
	{
		if (bs->wpDestSwitchTime < level.time)
		{
			if (bs->squadLeader->client)
			{
				VectorCopy(bs->squadLeader->client->ps.origin, usethisvec);
			}
			else
			{
				VectorCopy(bs->squadLeader->s.origin, usethisvec);
			}

			tempInt = GetNearestVisibleWP(usethisvec, 0);

			if (tempInt != -1 && TotalTrailDistance(bs->wpCurrent->index, tempInt, bs) != -1)
			{
				bs->wpDestination = gWPArray[tempInt];
				bs->wpDestSwitchTime = level.time + Q_irand(5000, 10000);
			}
		}
	}
	else if (bs->currentEnemy)
	{
		if (bs->currentEnemy->client)
		{
			VectorCopy(bs->currentEnemy->client->ps.origin, usethisvec);
		}
		else
		{
			VectorCopy(bs->currentEnemy->s.origin, usethisvec);
		}

		int bChicken = BotIsAChickenWuss(bs);
		bs->runningToEscapeThreat = bChicken;

		if (bs->frame_Enemy_Len < distChange || bChicken && bChicken != 2)
		{
			const int c_wp_index = bs->wpCurrent->index;

			if (bs->frame_Enemy_Len > 400)
			{
				//good distance away, start running toward a good place for an item or powerup or whatever
				idleWP = GetBestIdleGoal(bs);

				if (idleWP != -1 && gWPArray[idleWP] && gWPArray[idleWP]->inuse)
				{
					bs->wpDestination = gWPArray[idleWP];
				}
			}
			else if (gWPArray[c_wp_index - 1] && gWPArray[c_wp_index - 1]->inuse &&
				gWPArray[c_wp_index + 1] && gWPArray[c_wp_index + 1]->inuse)
			{
				VectorSubtract(gWPArray[c_wp_index + 1]->origin, usethisvec, a);
				plusLen = VectorLength(a);
				VectorSubtract(gWPArray[c_wp_index - 1]->origin, usethisvec, a);
				minusLen = VectorLength(a);

				if (minusLen > plusLen)
				{
					bs->wpDestination = gWPArray[c_wp_index - 1];
				}
				else
				{
					bs->wpDestination = gWPArray[c_wp_index + 1];
				}
			}
		}
		else if (bChicken != 2 && bs->wpDestSwitchTime < level.time)
		{
			tempInt = GetNearestVisibleWP(usethisvec, 0);

			if (tempInt != -1 && TotalTrailDistance(bs->wpCurrent->index, tempInt, bs) != -1)
			{
				bs->wpDestination = gWPArray[tempInt];

				if (level.gametype == GT_SINGLE_PLAYER)
				{
					//be more aggressive
					bs->wpDestSwitchTime = level.time + Q_irand(300, 1000);
				}
				else
				{
					bs->wpDestSwitchTime = level.time + Q_irand(1000, 5000);
				}
			}
		}
	}

	if (!bs->wpDestination && bs->wpDestSwitchTime < level.time)
	{
		//trap->Print("I need something to do\n");
		idleWP = GetBestIdleGoal(bs);

		if (idleWP != -1 && gWPArray[idleWP] && gWPArray[idleWP]->inuse)
		{
			bs->wpDestination = gWPArray[idleWP];
		}
	}
}

//commander CTF AI - tell other bots in the so-called
//"squad" what to do.
void CommanderBotCTFAI(const bot_state_t* bs)
{
	int i = 0;
	gentity_t* ent;
	int squadmates = 0;
	gentity_t* squad[MAX_CLIENTS];
	int defendAttackPriority = 0; //0 == attack, 1 == defend
	int guardDefendPriority = 0; //0 == defend, 1 == guard
	int attackRetrievePriority = 0; //0 == retrieve, 1 == attack
	int my_flag;
	int enemy_flag;
	int enemyHasOurFlag = 0;
	int weHaveEnemyFlag = 0;
	int numOnMyTeam = 0;
	int numOnEnemyTeam = 0;
	int numAttackers = 0;
	int numDefenders = 0;

	if (level.clients[bs->client].sess.sessionTeam == TEAM_RED)
	{
		my_flag = PW_REDFLAG;
	}
	else
	{
		my_flag = PW_BLUEFLAG;
	}

	if (level.clients[bs->client].sess.sessionTeam == TEAM_RED)
	{
		enemy_flag = PW_BLUEFLAG;
	}
	else
	{
		enemy_flag = PW_REDFLAG;
	}

	while (i < MAX_CLIENTS)
	{
		ent = &g_entities[i];

		if (ent && ent->client)
		{
			if (ent->client->ps.powerups[enemy_flag] && OnSameTeam(&g_entities[bs->client], ent))
			{
				weHaveEnemyFlag = 1;
			}
			else if (ent->client->ps.powerups[my_flag] && !OnSameTeam(&g_entities[bs->client], ent))
			{
				enemyHasOurFlag = 1;
			}

			if (OnSameTeam(&g_entities[bs->client], ent))
			{
				numOnMyTeam++;
			}
			else
			{
				numOnEnemyTeam++;
			}

			if (botstates[ent->s.number])
			{
				if (botstates[ent->s.number]->ctfState == CTFSTATE_ATTACKER ||
					botstates[ent->s.number]->ctfState == CTFSTATE_RETRIEVAL)
				{
					numAttackers++;
				}
				else
				{
					numDefenders++;
				}
			}
			else
			{
				//assume real players to be attackers in our logic
				numAttackers++;
			}
		}
		i++;
	}

	i = 0;

	while (i < MAX_CLIENTS)
	{
		ent = &g_entities[i];

		if (ent && ent->client && botstates[i] && botstates[i]->squadLeader && botstates[i]->squadLeader->s.number == bs
			->client && i != bs->client)
		{
			squad[squadmates] = ent;
			squadmates++;
		}

		i++;
	}

	squad[squadmates] = &g_entities[bs->client];
	squadmates++;

	i = 0;

	if (enemyHasOurFlag && !weHaveEnemyFlag)
	{
		//start off with an attacker instead of a retriever if we don't have the enemy flag yet so that they can't capture it first.
		//after that we focus on getting our flag back.
		attackRetrievePriority = 1;
	}

	while (i < squadmates)
	{
		if (squad[i] && squad[i]->client && botstates[squad[i]->s.number])
		{
			if (botstates[squad[i]->s.number]->ctfState != CTFSTATE_GETFLAGHOME)
			{
				//never tell a bot to stop trying to bring the flag to the base
				if (defendAttackPriority)
				{
					if (weHaveEnemyFlag)
					{
						if (guardDefendPriority)
						{
							botstates[squad[i]->s.number]->ctfState = CTFSTATE_GUARDCARRIER;
							guardDefendPriority = 0;
						}
						else
						{
							botstates[squad[i]->s.number]->ctfState = CTFSTATE_DEFENDER;
							guardDefendPriority = 1;
						}
					}
					else
					{
						botstates[squad[i]->s.number]->ctfState = CTFSTATE_DEFENDER;
					}
					defendAttackPriority = 0;
				}
				else
				{
					if (enemyHasOurFlag)
					{
						if (attackRetrievePriority)
						{
							botstates[squad[i]->s.number]->ctfState = CTFSTATE_ATTACKER;
							attackRetrievePriority = 0;
						}
						else
						{
							botstates[squad[i]->s.number]->ctfState = CTFSTATE_RETRIEVAL;
							attackRetrievePriority = 1;
						}
					}
					else
					{
						botstates[squad[i]->s.number]->ctfState = CTFSTATE_ATTACKER;
					}
					defendAttackPriority = 1;
				}
			}
			else if ((numOnMyTeam < 2 || !numAttackers) && enemyHasOurFlag)
			{
				//I'm the only one on my team who will attack and the enemy has my flag, I have to go after him
				botstates[squad[i]->s.number]->ctfState = CTFSTATE_RETRIEVAL;
			}
		}

		i++;
	}
}

//similar to ctf ai, for siege
void CommanderBotSiegeAI(const bot_state_t* bs)
{
	int i = 0;
	int squadmates = 0;
	int commanded = 0;
	int teammates = 0;
	gentity_t* squad[MAX_CLIENTS];
	bot_state_t* bst;

	while (i < MAX_CLIENTS)
	{
		gentity_t* ent = &g_entities[i];

		if (ent && ent->client && OnSameTeam(&g_entities[bs->client], ent) && botstates[ent->s.number])
		{
			bst = botstates[ent->s.number];

			if (bst && !bst->isSquadLeader && !bst->state_Forced)
			{
				squad[squadmates] = ent;
				squadmates++;
			}
			else if (bst && !bst->isSquadLeader && bst->state_Forced)
			{
				//count them as commanded
				commanded++;
			}
		}

		if (ent && ent->client && OnSameTeam(&g_entities[bs->client], ent))
		{
			teammates++;
		}

		i++;
	}

	if (!squadmates)
	{
		return;
	}

	//tell squad mates to do what I'm doing, up to half of team, let the other half make their own decisions
	i = 0;

	while (i < squadmates && squad[i])
	{
		bst = botstates[squad[i]->s.number];

		if (commanded > teammates / 2)
		{
			break;
		}

		if (bst)
		{
			bst->state_Forced = bs->siegeState;
			bst->siegeState = bs->siegeState;
			commanded++;
		}

		i++;
	}
}

//teamplay ffa squad ai
void BotDoTeamplayAI(bot_state_t* bs)
{
	if (bs->state_Forced)
	{
		bs->teamplayState = bs->state_Forced;
	}

	if (bs->teamplayState == TEAMPLAYSTATE_REGROUP)
	{
		//force to find a new leader
		bs->squadLeader = NULL;
		bs->isSquadLeader = 0;
	}
}

//like ctf and siege commander ai, instruct the squad
void CommanderBotTeamplayAI(bot_state_t* bs)
{
	int i = 0;
	int squadmates = 0;
	int teammates = 0;
	int teammate_indanger = -1;
	int teammate_helped = 0;
	int foundsquadleader = 0;
	int worsthealth = 50;
	gentity_t* squad[MAX_CLIENTS];
	bot_state_t* bst;

	while (i < MAX_CLIENTS)
	{
		gentity_t* ent = &g_entities[i];

		if (ent && ent->client && OnSameTeam(&g_entities[bs->client], ent) && botstates[ent->s.number])
		{
			bst = botstates[ent->s.number];

			if (foundsquadleader && bst && bst->isSquadLeader)
			{
				//never more than one squad leader
				bst->isSquadLeader = 0;
			}

			if (bst && !bst->isSquadLeader)
			{
				squad[squadmates] = ent;
				squadmates++;
			}
			else if (bst)
			{
				foundsquadleader = 1;
			}
		}

		if (ent && ent->client && OnSameTeam(&g_entities[bs->client], ent))
		{
			teammates++;

			if (ent->health < worsthealth)
			{
				teammate_indanger = ent->s.number;
				worsthealth = ent->health;
			}
		}

		i++;
	}

	if (!squadmates)
	{
		return;
	}

	i = 0;

	while (i < squadmates && squad[i])
	{
		bst = botstates[squad[i]->s.number];

		if (bst && !bst->state_Forced)
		{
			//only order if this guy is not being ordered directly by the real player team leader
			if (teammate_indanger >= 0 && !teammate_helped)
			{
				//send someone out to help whoever needs help most at the moment
				bst->teamplayState = TEAMPLAYSTATE_ASSISTING;
				bst->squadLeader = &g_entities[teammate_indanger];
				teammate_helped = 1;
			}
			else if ((teammate_indanger == -1 || teammate_helped) && bst->teamplayState == TEAMPLAYSTATE_ASSISTING)
			{
				//no teammates need help badly, but this guy is trying to help them anyway, so stop
				bst->teamplayState = TEAMPLAYSTATE_FOLLOWING;
				bst->squadLeader = &g_entities[bs->client];
			}

			if (bs->squadRegroupInterval < level.time && Q_irand(1, 10) < 5)
			{
				//every so often tell the squad to regroup for the sake of variation
				if (bst->teamplayState == TEAMPLAYSTATE_FOLLOWING)
				{
					bst->teamplayState = TEAMPLAYSTATE_REGROUP;
				}

				bs->isSquadLeader = 0;
				bs->squadCannotLead = level.time + 500;
				bs->squadRegroupInterval = level.time + Q_irand(45000, 65000);
			}
		}

		i++;
	}
}

//pick which commander ai to use based on gametype
void CommanderBotAI(bot_state_t* bs)
{
	if (level.gametype == GT_CTF || level.gametype == GT_CTY)
	{
		CommanderBotCTFAI(bs);
	}
	else if (level.gametype == GT_SIEGE)
	{
		CommanderBotSiegeAI(bs);
	}
	else if (level.gametype == GT_TEAM)
	{
		CommanderBotTeamplayAI(bs);
	}
}

//close range combat routines
void MeleeCombatHandling(bot_state_t* bs)
{
	vec3_t usethisvec;
	vec3_t downvec;
	vec3_t midorg;
	vec3_t a;
	vec3_t fwd;
	vec3_t mins, maxs;
	trace_t tr;

	if (!bs->currentEnemy)
	{
		return;
	}

	if (bs->currentEnemy->client)
	{
		VectorCopy(bs->currentEnemy->client->ps.origin, usethisvec);
	}
	else
	{
		VectorCopy(bs->currentEnemy->s.origin, usethisvec);
	}

	if (bs->meleeStrafeTime < level.time)
	{
		if (bs->meleeStrafeDir)
		{
			bs->meleeStrafeDir = 0;
		}
		else
		{
			bs->meleeStrafeDir = 1;
		}

		bs->meleeStrafeTime = level.time + Q_irand(500, 1800);
	}

	mins[0] = -15;
	mins[1] = -15;
	mins[2] = -24;
	maxs[0] = 15;
	maxs[1] = 15;
	maxs[2] = 32;

	VectorCopy(usethisvec, downvec);
	downvec[2] -= 4096;

	trap->Trace(&tr, usethisvec, mins, maxs, downvec, -1, MASK_SOLID, qfalse, 0, 0);

	const int en_down = (int)tr.endpos[2];

	VectorCopy(bs->origin, downvec);
	downvec[2] -= 4096;

	trap->Trace(&tr, bs->origin, mins, maxs, downvec, -1, MASK_SOLID, qfalse, 0, 0);

	const int me_down = (int)tr.endpos[2];

	VectorSubtract(usethisvec, bs->origin, a);
	vectoangles(a, a);
	AngleVectors(a, fwd, NULL, NULL);

	midorg[0] = bs->origin[0] + fwd[0] * bs->frame_Enemy_Len / 2;
	midorg[1] = bs->origin[1] + fwd[1] * bs->frame_Enemy_Len / 2;
	midorg[2] = bs->origin[2] + fwd[2] * bs->frame_Enemy_Len / 2;

	VectorCopy(midorg, downvec);
	downvec[2] -= 4096;

	trap->Trace(&tr, midorg, mins, maxs, downvec, -1, MASK_SOLID, qfalse, 0, 0);

	const int mid_down = (int)tr.endpos[2];

	if (me_down == en_down &&
		en_down == mid_down)
	{
		VectorCopy(usethisvec, bs->goalPosition);
	}

	if (gesturetime[bs->cur_ps.clientNum] <= level.time && bot_thinklevel.integer >= 0)
	{
		if (bs->currentEnemy
			&& bs->currentEnemy->client
			&& bs->currentEnemy->health > 0
			&& bs->jumpTime <= level.time // Don't gesture during jumping...
			&& !bs->doAttack
			&& !bs->doAltAttack
			&& bs->currentEnemy->client->ps.groundEntityNum != ENTITYNUM_NONE
			&& VectorDistance(g_entities[bs->cur_ps.clientNum].r.currentOrigin, bs->currentEnemy->r.currentOrigin) <
			300 || gesturetime[bs->cur_ps.clientNum] > level.time)
		{
			if (visible(&g_entities[bs->cur_ps.clientNum], bs->currentEnemy) || gesturetime[bs->cur_ps.clientNum] >
				level.time)
			{
				bot_input_t bi;
				bi.actionflags |= ACTION_GESTURE;

				if (BotGetWeaponRange(bs) == BWEAPONRANGE_MELEE || BotGetWeaponRange(bs) == BWEAPONRANGE_SABER)
				{
					bi.actionflags |= ACTION_KICK;
				}
				int check_val = bot_thinklevel.integer;

				if (check_val <= 0)
					check_val = 1;

				gesturetime[bs->cur_ps.clientNum] = level.time + 20000 / check_val;
			}
			else
			{
				// Reset.
				gesturetime[bs->cur_ps.clientNum] = 0;
			}
		}
		else
		{
			// Reset.
			gesturetime[bs->cur_ps.clientNum] = 0;
		}
	}
}

void adjustfor_strafe(const bot_state_t* bs, vec3_t move_dir)
{
	vec3_t right;

	if (!bs->meleeStrafeDir || bs->meleeStrafeTime < level.time)
	{
		//no strafe
		return;
	}

	AngleVectors(g_entities[bs->client].client->ps.viewangles, NULL, right, NULL);

	//flaten up/down
	right[2] = 0;

	if (bs->meleeStrafeDir == 2)
	{
		//strafing left
		VectorScale(right, -1, right);
	}

	//We assume that moveDir has been normalized before this function.
	VectorAdd(move_dir, right, move_dir);
	VectorNormalize(move_dir);
}

void MoveforAttackQuad(const bot_state_t* bs, vec3_t move_dir, const int Quad)
{
	//set the moveDir to set our attack direction to be towards this Quad.
	vec3_t forward, right;

	AngleVectors(bs->viewangles, forward, right, NULL);

	switch (Quad)
	{
	case Q_B: //down strike.
		VectorCopy(forward, move_dir);
		break;
	case Q_BR: //down right strike
		VectorAdd(forward, right, move_dir);
		VectorNormalize(move_dir);
		break;
	case Q_R: //right strike
		VectorCopy(right, move_dir);
		break;
	case Q_TR: //up right strike
		VectorScale(forward, -1, forward);
		VectorAdd(forward, right, move_dir);
		VectorNormalize(move_dir);
		break;
	case Q_T: //up strike
		VectorScale(forward, -1, forward);
		VectorCopy(forward, move_dir);
		break;
	case Q_TL: //up left strike
		VectorScale(forward, -1, forward);
		VectorScale(right, -1, right);
		VectorAdd(forward, right, move_dir);
		VectorNormalize(move_dir);
		break;
	case Q_L: //left strike
		VectorScale(right, -1, right);
		VectorCopy(right, move_dir);
		break;
	case Q_BL: //down left strike.
		VectorScale(right, -1, right);
		VectorAdd(forward, right, move_dir);
		VectorNormalize(move_dir);
		break;
	default:
		break;
	}
}

qboolean BotBehave_CheckBackstab(bot_state_t* bs)
{
	// Check if there is an enemy behind us that we can backstab...
	vec3_t forward, back_org, cur_org, moveDir;
	trace_t tr;

	if (bs->cur_ps.weapon != WP_SABER)
		return qfalse;

	VectorCopy(bs->cur_ps.origin, cur_org);
	cur_org[2] += 24;
	AngleVectors(bs->cur_ps.viewangles, forward, NULL, NULL);
	VectorMA(cur_org, -64, forward, back_org);

	trap->Trace(&tr, cur_org, NULL, NULL, back_org, bs->client, MASK_SHOT, 0, 0, 0);

	if (tr.entityNum < 0 || tr.entityNum > ENTITYNUM_MAX_NORMAL)
		return qfalse;

	const gentity_t* enemy = &g_entities[tr.entityNum];

	if (!enemy
		|| enemy->s.eType != ET_PLAYER && enemy->s.eType != ET_NPC
		|| !enemy->client
		|| enemy->health < 1
		|| enemy->client->ps.eFlags & EF_DEAD)
		return qfalse;

	if (OnSameTeam(&g_entities[bs->client], enemy))
		return qfalse;

	// OK, let's stab away! bwahahaha!
	VectorCopy(bs->cur_ps.origin, cur_org);
	VectorMA(cur_org, -64, forward, back_org);

	VectorSubtract(cur_org, bs->origin, moveDir);

	moveDir[2] = 0;
	VectorNormalize(moveDir);

	//adjust the moveDir to do strafing
	adjustfor_strafe(bs, moveDir);
	TraceMove(bs, moveDir, tr.entityNum);
	trap->EA_Move(bs->client, moveDir, 5000);
	trap->EA_Attack(bs->client);

	return qtrue;
}

qboolean BotBehave_CheckUseKata(const bot_state_t* bs)
{
	// Check if there is an in front of us that we can use our kata on...
	vec3_t forward, back_org, cur_org;
	trace_t tr;

	if (bs->cur_ps.weapon != WP_SABER)
	{
		return qfalse;
	}

	VectorCopy(bs->cur_ps.origin, cur_org);
	cur_org[2] += 24;
	AngleVectors(bs->cur_ps.viewangles, forward, NULL, NULL);
	VectorMA(cur_org, 64, forward, back_org);

	trap->Trace(&tr, cur_org, NULL, NULL, back_org, bs->client, MASK_SHOT, 0, 0, 0);

	if (tr.entityNum < 0 || tr.entityNum > ENTITYNUM_MAX_NORMAL)
		return qfalse;

	const gentity_t* enemy = &g_entities[tr.entityNum];

	if (!enemy
		|| enemy->s.eType != ET_PLAYER && enemy->s.eType != ET_NPC
		|| !enemy->client
		|| enemy->health < 1
		|| enemy->client->ps.eFlags & EF_DEAD)
		return qfalse;

	if (OnSameTeam(&g_entities[bs->client], enemy))
		return qfalse;

	// OK. We have a target. First make it not happen constantly with some randomization!
	if (irand(0, 200) > 50)
		return qfalse;

	// OK, let's stab away! bwahahaha!
	trap->EA_Attack(bs->client);
	trap->EA_Alt_Attack(bs->client);

	return qtrue;
}

qboolean BotBehave_CheckUseCrouchAttack(bot_state_t* bs)
{
	// Check if there is an in front of us that we can use our special crouch attack on...
	vec3_t forward, back_org, cur_org, moveDir;
	trace_t tr;

	if (bs->cur_ps.weapon != WP_SABER)
		return qfalse;

	VectorCopy(bs->cur_ps.origin, cur_org);
	cur_org[2] += 24;
	AngleVectors(bs->cur_ps.viewangles, forward, NULL, NULL);
	VectorMA(cur_org, 64, forward, back_org);

	trap->Trace(&tr, cur_org, NULL, NULL, back_org, bs->client, MASK_SHOT, 0, 0, 0);

	if (tr.entityNum < 0 || tr.entityNum > ENTITYNUM_MAX_NORMAL)
		return qfalse;

	const gentity_t* enemy = &g_entities[tr.entityNum];

	if (!enemy
		|| enemy->s.eType != ET_PLAYER && enemy->s.eType != ET_NPC
		|| !enemy->client
		|| enemy->health < 1
		|| enemy->client->ps.eFlags & EF_DEAD)
		return qfalse;

	if (OnSameTeam(&g_entities[bs->client], enemy))
		return qfalse;

	// OK. We have a target. First make it not happen constantly with some randomization!
	if (irand(0, 200) > 10)
		return qfalse;

	// OK, let's stab away! bwahahaha!
	VectorCopy(bs->cur_ps.origin, cur_org);
	VectorMA(cur_org, 64, forward, back_org);

	VectorSubtract(cur_org, bs->origin, moveDir);

	moveDir[2] = 0;
	VectorNormalize(moveDir);

	//adjust the moveDir to do strafing
	adjustfor_strafe(bs, moveDir);
	TraceMove(bs, moveDir, tr.entityNum);
	trap->EA_Crouch(bs->client);
	trap->EA_Move(bs->client, moveDir, 5000);
	trap->EA_Attack(bs->client);
	trap->EA_Alt_Attack(bs->client);

	return qtrue;
}

void BotBehave_AttackBasic(bot_state_t* bs, const gentity_t* target)
{
	vec3_t enemyOrigin, viewDir, ang, moveDir;

	FindOrigin(target, enemyOrigin);

	const float dist = TargetDistance(bs, target, enemyOrigin);

	//adjust angle for target leading.
	const float leadamount = BotWeaponCanLead(bs);

	BotAimLeading(bs, enemyOrigin, leadamount);

	//face enemy
	VectorSubtract(enemyOrigin, bs->eye, viewDir);
	vectoangles(viewDir, ang);
	ang[PITCH] = 0;
	ang[ROLL] = 0;
	VectorCopy(ang, bs->goalAngles);

	//check to see if there's a detpack in the immediate area of the target.
	if (bs->cur_ps.stats[STAT_WEAPONS] & 1 << WP_DET_PACK)
	{
		//only check if you got det packs.
		BotWeapon_Detpack(bs, target);
	}

	if (!PM_SaberInKata(bs->cur_ps.saberMove) && bs->cur_ps.fd.forcePower > 80 &&
		bs->cur_ps.weapon == WP_SABER && dist < 128 && InFieldOfVision(bs->viewangles, 90, ang))
	{
		//KATA!
		trap->EA_Attack(bs->client);
		trap->EA_Alt_Attack(bs->client);
		return;
	}

	if (bs->meleeStrafeTime < level.time)
	{
		//select a new strafing direction
		//0 = no strafe
		//1 = strafe right
		//2 = strafe left
		bs->meleeStrafeDir = Q_irand(0, 2);
		bs->meleeStrafeTime = level.time + Q_irand(500, 1800);
	}

	VectorSubtract(enemyOrigin, bs->origin, moveDir);

	if (dist < MinimumAttackDistance[bs->virtualWeapon])
	{
		//move back
		VectorScale(moveDir, -1, moveDir);
	}
	else if (dist < IdealAttackDistance[bs->virtualWeapon])
	{
		//we're close enough, quit moving closer
		VectorClear(moveDir);
	}

	moveDir[2] = 0;
	VectorNormalize(moveDir);

	if (bs->virtualWeapon == WP_SABER)
	{
		// Added, backstab check...
		if (BotBehave_CheckBackstab(bs))
		{
			return;
		}

		// Added, kata check...
		if (BotBehave_CheckUseKata(bs))
		{
			return;
		}

		// Added, special crouch attack check...
		if (BotBehave_CheckUseCrouchAttack(bs))
		{
			return;
		}
	}

	//adjust the moveDir to do strafing
	adjustfor_strafe(bs, moveDir);

	if (bs->cur_ps.weapon == bs->virtualWeapon
		&& bs->virtualWeapon == WP_SABER && InFieldOfVision(bs->viewangles, 100, ang))
	{
		//we're using a lightsaber
		if (PM_SaberInIdle(bs->cur_ps.saberMove)
			|| PM_SaberInBounce(bs->cur_ps.saberMove)
			|| PM_SaberInReturn(bs->cur_ps.saberMove))
		{
			//we want to attack, and we need to choose a new attack swing, pick randomly.
			MoveforAttackQuad(bs, moveDir, Q_irand(Q_BR, Q_B));
		}
		else if (bs->cur_ps.userInt3 & 1 << FLAG_ATTACKFAKE)
		{
			//successfully started an attack fake, don't do it again for a while.
			bs->saberBFTime = level.time + Q_irand(3000, 5000); //every 3-5 secs
		}
		else if (bs->saberBFTime < level.time
			&& (PM_SaberInTransition(bs->cur_ps.saberMove)
				|| PM_SaberInStart(bs->cur_ps.saberMove)))
		{
			//we can and want to do a saber attack fake.
			int fakeQuad = Q_irand(Q_BR, Q_B);
			while (fakeQuad == saberMoveData[bs->cur_ps.saberMove].endQuad)
			{
				//can't fake in the direction we're already trying to attack in
				fakeQuad = Q_irand(Q_BR, Q_B);
			}
			//start trying to fake
			MoveforAttackQuad(bs, moveDir, fakeQuad);
			trap->EA_Alt_Attack(bs->client);
		}
	}

	if (!VectorCompare(vec3_origin, moveDir))
	{
		TraceMove(bs, moveDir, target->s.clientNum);
		trap->EA_Move(bs->client, moveDir, 5000);
	}

	if (bs->frame_Enemy_Vis && bs->cur_ps.weapon == bs->virtualWeapon
		&& (InFieldOfVision(bs->viewangles, 30, ang)
			|| bs->virtualWeapon == WP_SABER && InFieldOfVision(bs->viewangles, 100, ang)))
	{
		//not switching weapons so attack
		trap->EA_Attack(bs->client);
		if (bs->virtualWeapon == WP_SABER)
		{
			//only walk while attacking with the saber.
			bs->doWalk = qtrue;
		}
	}
}

//saber combat routines (it's simple, but it works)
void SaberCombatHandling(bot_state_t* bs)
{
	vec3_t usethisvec;
	vec3_t downvec;
	vec3_t midorg;
	vec3_t a;
	vec3_t fwd, ang, moveDir;
	vec3_t mins, maxs;
	trace_t tr;
	int me_down;

	if (!bs->currentEnemy)
	{
		return;
	}

	if (bs->currentEnemy->client)
	{
		VectorCopy(bs->currentEnemy->client->ps.origin, usethisvec);
	}
	else
	{
		VectorCopy(bs->currentEnemy->s.origin, usethisvec);
	}

	if (bs->meleeStrafeTime < level.time)
	{
		if (bs->meleeStrafeDir)
		{
			bs->meleeStrafeDir = 0;
		}
		else
		{
			bs->meleeStrafeDir = 1;
		}

		bs->meleeStrafeTime = level.time + Q_irand(500, 1800);
	}

	mins[0] = -15;
	mins[1] = -15;
	mins[2] = -24;
	maxs[0] = 15;
	maxs[1] = 15;
	maxs[2] = 32;

	VectorCopy(usethisvec, downvec);
	downvec[2] -= 4096;

	trap->Trace(&tr, usethisvec, mins, maxs, downvec, -1, MASK_SOLID, qfalse, 0, 0);

	int en_down = (int)tr.endpos[2];

	if (tr.startsolid || tr.allsolid)
	{
		en_down = 1;
		me_down = 2;
	}
	else
	{
		VectorCopy(bs->origin, downvec);
		downvec[2] -= 4096;

		trap->Trace(&tr, bs->origin, mins, maxs, downvec, -1, MASK_SOLID, qfalse, 0, 0);

		me_down = (int)tr.endpos[2];

		if (tr.startsolid || tr.allsolid)
		{
			en_down = 1;
			me_down = 2;
		}
	}

	VectorSubtract(usethisvec, bs->origin, a);
	vectoangles(a, a);
	AngleVectors(a, fwd, NULL, NULL);

	midorg[0] = bs->origin[0] + fwd[0] * bs->frame_Enemy_Len / 2;
	midorg[1] = bs->origin[1] + fwd[1] * bs->frame_Enemy_Len / 2;
	midorg[2] = bs->origin[2] + fwd[2] * bs->frame_Enemy_Len / 2;

	VectorCopy(midorg, downvec);
	downvec[2] -= 4096;

	trap->Trace(&tr, midorg, mins, maxs, downvec, -1, MASK_SOLID, qfalse, 0, 0);

	const int mid_down = (int)tr.endpos[2];

	if (me_down == en_down && en_down == mid_down)// - Both over the same level of ground
	{
		if (bs->frame_Enemy_Len > 128)
		{
			//be ready to attack
			//this should be an attack while moving function but for now we'll just use moveto
			vec3_t enemyOrigin;
			FindOrigin(bs->currentEnemy, enemyOrigin);
			VectorCopy(enemyOrigin, bs->DestPosition);
			bs->DestIgnore = bs->currentEnemy->s.number;
			BotBehave_AttackMove(bs);
			return;
		}
		if (bs->saberDefendDecideTime < level.time)
		{
			if (bs->saberDefending)
			{
				bs->saberDefending = 0;
			}
			else
			{
				bs->saberDefending = 1;
			}

			bs->saberDefendDecideTime = level.time + Q_irand(500, 2000);
		}

		if (bs->frame_Enemy_Len < 54) // (How far away you are from him)
		{
			VectorCopy(bs->origin, bs->goalPosition);
			bs->saberBFTime = 0;
		}

		if (bs->currentEnemy && bs->currentEnemy->client)
		{
			if (!PM_SaberInSpecial(bs->currentEnemy->client->ps.saberMove)
				&& bs->frame_Enemy_Len > 90
				&& bs->saberBFTime > level.time
				&& bs->saberBTime > level.time
				&& bs->beStill < level.time
				&& bs->saberSTime < level.time)
			{
				bs->beStill = level.time + Q_irand(500, 1000);
				bs->saberSTime = level.time + Q_irand(1200, 1800);
			}
			else if (bs->currentEnemy->client->ps.weapon == WP_SABER
				&& bs->frame_Enemy_Len < 80.0f
				&& (Q_irand(1, 10) < 8
					&& bs->saberBFTime < level.time || bs->saberBTime > level.time
					|| PM_SaberInKata(bs->currentEnemy->client->ps.saberMove)
					|| bs->currentEnemy->client->ps.saberMove == LS_SPINATTACK
					|| bs->currentEnemy->client->ps.saberMove == LS_SPINATTACK_GRIEV
					|| bs->currentEnemy->client->ps.saberMove == LS_SPINATTACK_DUAL))
			{
				vec3_t vs;
				vec3_t groundcheck;
				int idealDist;
				int checkIncr = 0;

				VectorSubtract(bs->origin, bs->goalPosition, vs);
				VectorNormalize(vs);

				if (PM_SaberInKata(bs->currentEnemy->client->ps.saberMove)
					|| bs->currentEnemy->client->ps.saberMove == LS_SPINATTACK
					|| bs->currentEnemy->client->ps.saberMove == LS_SPINATTACK_GRIEV
					|| bs->currentEnemy->client->ps.saberMove == LS_SPINATTACK_DUAL)
				{
					idealDist = 256;
				}
				else
				{
					idealDist = 64;
				}

				while (checkIncr < idealDist)
				{
					bs->goalPosition[0] = bs->origin[0] + vs[0] * checkIncr;
					bs->goalPosition[1] = bs->origin[1] + vs[1] * checkIncr;
					bs->goalPosition[2] = bs->origin[2] + vs[2] * checkIncr;

					if (bs->saberBTime < level.time)
					{
						bs->saberBFTime = level.time + Q_irand(900, 1300);
						bs->saberBTime = level.time + Q_irand(300, 700);
					}

					VectorCopy(bs->goalPosition, groundcheck);

					groundcheck[2] -= 64;

					trap->Trace(&tr, bs->goalPosition, NULL, NULL, groundcheck, bs->client, MASK_SOLID, qfalse, 0, 0);

					if (tr.fraction == 1.0f)
					{
						//don't back off of a ledge
						VectorCopy(usethisvec, bs->goalPosition);
						break;
					}
					checkIncr += 64;
				}
			}
			else if (bs->currentEnemy->client->ps.weapon == WP_SABER && bs->frame_Enemy_Len >= 75)
			{
				bs->saberBFTime = level.time + Q_irand(700, 1300);
				bs->saberBTime = 0;
			}
		}
	}
	else if (bs->frame_Enemy_Len <= 56)
	{
		BotBehave_Attack(bs);
		bs->saberDefending = 0;
	}

	if (!VectorCompare(vec3_origin, moveDir))
	{
		trap->EA_Move(bs->client, moveDir, 5000);
	}

	if (bs->frame_Enemy_Vis && bs->cur_ps.weapon == bs->virtualWeapon
		&& (InFieldOfVision(bs->viewangles, 30, ang)
			|| bs->virtualWeapon == WP_SABER && InFieldOfVision(bs->viewangles, 100, ang)))
	{
		//not switching weapons so attack
		trap->EA_Attack(bs->client);

		if (bs->cur_ps.weapon == WP_SABER)
		{
			//only walk while attacking with the saber.
			bs->doWalk = qtrue;
		}
	}
}

//should we be "leading" our aim with this weapon? And if
//so, by how much?
float BotWeaponCanLead(const bot_state_t* bs)
{
	switch (bs->cur_ps.weapon)
	{
	case WP_BRYAR_PISTOL:
		return 0.5f;
	case WP_BLASTER:
		return 0.35f;
	case WP_BOWCASTER:
		return 0.5f;
	case WP_REPEATER:
		return 0.45f;
	case WP_THERMAL:
		return 0.5f;
	case WP_DEMP2:
		return 0.35f;
	case WP_ROCKET_LAUNCHER:
		return 0.7f;
	default:
		return 0.0f;
	}
}

//offset the desired view angles with aim leading in mind
void BotAimLeading(bot_state_t* bs, vec3_t headlevel, const float leadAmount)
{
	vec3_t predictedSpot;
	vec3_t movementVector;
	vec3_t a, ang;

	if (!bs->currentEnemy ||
		!bs->currentEnemy->client)
	{
		return;
	}

	if (!bs->frame_Enemy_Len)
	{
		return;
	}

	float vtotal = 0;

	if (bs->currentEnemy->client->ps.velocity[0] < 0)
	{
		vtotal += -bs->currentEnemy->client->ps.velocity[0];
	}
	else
	{
		vtotal += bs->currentEnemy->client->ps.velocity[0];
	}

	if (bs->currentEnemy->client->ps.velocity[1] < 0)
	{
		vtotal += -bs->currentEnemy->client->ps.velocity[1];
	}
	else
	{
		vtotal += bs->currentEnemy->client->ps.velocity[1];
	}

	if (bs->currentEnemy->client->ps.velocity[2] < 0)
	{
		vtotal += -bs->currentEnemy->client->ps.velocity[2];
	}
	else
	{
		vtotal += bs->currentEnemy->client->ps.velocity[2];
	}

	VectorCopy(bs->currentEnemy->client->ps.velocity, movementVector);

	VectorNormalize(movementVector);

	float x; //hardly calculated with an exact science, but it works

	if (vtotal > 400)
	{
		vtotal = 400;
	}

	if (vtotal)
	{
		x = bs->frame_Enemy_Len * 0.9 * leadAmount * (vtotal * 0.0012);
		//hardly calculated with an exact science, but it works
	}
	else
	{
		x = bs->frame_Enemy_Len * 0.9 * leadAmount; //hardly calculated with an exact science, but it works
	}

	predictedSpot[0] = headlevel[0] + movementVector[0] * x;
	predictedSpot[1] = headlevel[1] + movementVector[1] * x;
	predictedSpot[2] = headlevel[2] + movementVector[2] * x;

	VectorSubtract(predictedSpot, bs->eye, a);
	vectoangles(a, ang);
	VectorCopy(ang, bs->goalAngles);
}

//wobble our aim around based on our sk1llz
void BotAimOffsetGoalAngles(bot_state_t* bs)
{
	if (bs->skills.perfectaim)
	{
		return;
	}

	if (bs->aimOffsetTime > level.time)
	{
		int i = 0;
		if (bs->aimOffsetAmtYaw)
		{
			bs->goalAngles[YAW] += bs->aimOffsetAmtYaw;
		}

		if (bs->aimOffsetAmtPitch)
		{
			bs->goalAngles[PITCH] += bs->aimOffsetAmtPitch;
		}

		while (i <= 2)
		{
			if (bs->goalAngles[i] > 360)
			{
				bs->goalAngles[i] -= 360;
			}

			if (bs->goalAngles[i] < 0)
			{
				bs->goalAngles[i] += 360;
			}

			i++;
		}
		return;
	}

	float acc_val = bs->skills.accuracy / bs->settings.skill;

	if (bs->currentEnemy && BotMindTricked(bs->client, bs->currentEnemy->s.number))
	{
		//having to judge where they are by hearing them, so we should be quite inaccurate here
		acc_val *= 7;

		if (acc_val < 30)
		{
			acc_val = 30;
		}
	}

	if (bs->revengeEnemy && bs->revengeHateLevel &&
		bs->currentEnemy == bs->revengeEnemy)
	{
		//bot becomes more skilled as anger level raises
		acc_val = acc_val / bs->revengeHateLevel;
	}

	if (bs->currentEnemy && bs->frame_Enemy_Vis)
	{
		//assume our goal is aiming at the enemy, seeing as he's visible and all
		if (!bs->currentEnemy->s.pos.trDelta[0] &&
			!bs->currentEnemy->s.pos.trDelta[1] &&
			!bs->currentEnemy->s.pos.trDelta[2])
		{
			acc_val = 0; //he's not even moving, so he shouldn't really be hard to hit.
		}
		else
		{
			acc_val += acc_val * 0.25; //if he's moving he's this much harder to hit
		}

		if (g_entities[bs->client].s.pos.trDelta[0] ||
			g_entities[bs->client].s.pos.trDelta[1] ||
			g_entities[bs->client].s.pos.trDelta[2])
		{
			acc_val += acc_val * 0.15; //make it somewhat harder to aim if we're moving also
		}
	}

	if (acc_val > 90)
	{
		acc_val = 90;
	}
	if (acc_val < 1)
	{
		acc_val = 0;
	}

	if (!acc_val)
	{
		bs->aimOffsetAmtYaw = 0;
		bs->aimOffsetAmtPitch = 0;
		return;
	}

	if (rand() % 10 <= 5)
	{
		bs->aimOffsetAmtYaw = rand() % (int)acc_val;
	}
	else
	{
		bs->aimOffsetAmtYaw = -(rand() % (int)acc_val);
	}

	if (rand() % 10 <= 5)
	{
		bs->aimOffsetAmtPitch = rand() % (int)acc_val;
	}
	else
	{
		bs->aimOffsetAmtPitch = -(rand() % (int)acc_val);
	}

	bs->aimOffsetTime = level.time + rand() % 500 + 200;
}

//do we want to alt fire with this weapon?
int ShouldSecondaryFire(const bot_state_t* bs)
{
	const int weap = bs->cur_ps.weapon;

	if (bs->cur_ps.ammo[weaponData[weap].ammoIndex] < weaponData[weap].altEnergyPerShot)
	{
		return 0;
	}

	if (bs->cur_ps.weaponstate == WEAPON_CHARGING_ALT && bs->cur_ps.weapon == WP_ROCKET_LAUNCHER)
	{
		const float heldTime = level.time - bs->cur_ps.weaponChargeTime;

		float rTime = bs->cur_ps.rocketLockTime;

		if (rTime < 1)
		{
			rTime = bs->cur_ps.rocketLastValidTime;
		}

		if (heldTime > 5000)
		{
			//just give up and release it if we can't manage a lock in 5 seconds
			return 2;
		}

		if (rTime > 0)
		{
			const int dif = (level.time - rTime) / (1200.0f / 16.0f);

			if (dif >= 10)
			{
				return 2;
			}
			if (bs->frame_Enemy_Len > 250)
			{
				return 1;
			}
		}
		else if (bs->frame_Enemy_Len > 250)
		{
			return 1;
		}
	}
	else if (bs->cur_ps.weaponstate == WEAPON_CHARGING_ALT && level.time - bs->cur_ps.weaponChargeTime > bs->
		altChargeTime)
	{
		return 2;
	}
	else if (bs->cur_ps.weaponstate == WEAPON_CHARGING_ALT)
	{
		return 1;
	}

	if (weap == WP_BRYAR_PISTOL && bs->frame_Enemy_Len < 300)
	{
		return 1;
	}
	if (weap == WP_BOWCASTER && bs->frame_Enemy_Len > 300)
	{
		return 1;
	}
	if (weap == WP_REPEATER && bs->frame_Enemy_Len < 600 && bs->frame_Enemy_Len > 250)
	{
		return 1;
	}
	if (weap == WP_BLASTER && bs->frame_Enemy_Len < 300)
	{
		return 1;
	}
	if (weap == WP_ROCKET_LAUNCHER && bs->frame_Enemy_Len > 250)
	{
		return 1;
	}

	return 0;
}

//standard weapon combat routines
int CombatBotAI(bot_state_t* bs, float thinktime)
{
	vec3_t eorg, a;

	if (!bs->currentEnemy)
	{
		return 0;
	}

	if (bs->currentEnemy->client)
	{
		VectorCopy(bs->currentEnemy->client->ps.origin, eorg);
	}
	else
	{
		VectorCopy(bs->currentEnemy->s.origin, eorg);
	}

	VectorSubtract(eorg, bs->eye, a);
	vectoangles(a, a);

	if (BotGetWeaponRange(bs) == BWEAPONRANGE_SABER)
	{
		if (bs->frame_Enemy_Len <= SABER_ATTACK_RANGE)
		{
			bs->doAttack = 1;
		}
	}
	else if (BotGetWeaponRange(bs) == BWEAPONRANGE_SABER)
	{
		if (bs->frame_Enemy_Len <= SABER_KICK_RANGE)
		{
			bs->doBotKick = 1;
		}
	}
	else if (BotGetWeaponRange(bs) == BWEAPONRANGE_MELEE)
	{
		if (bs->frame_Enemy_Len <= MELEE_ATTACK_RANGE)
		{
			bs->doAttack = 1;
		}
	}
	else
	{
		float fovcheck;
		if (bs->cur_ps.weapon == WP_THERMAL || bs->cur_ps.weapon == WP_ROCKET_LAUNCHER)
		{
			//be careful with the hurty weapons
			fovcheck = 40;

			if (bs->cur_ps.weaponstate == WEAPON_CHARGING_ALT &&
				bs->cur_ps.weapon == WP_ROCKET_LAUNCHER)
			{
				//if we're charging the weapon up then we can hold fire down within a normal fov
				fovcheck = 60;
			}
		}
		else
		{
			fovcheck = 60;
		}

		if (bs->cur_ps.weaponstate == WEAPON_CHARGING ||
			bs->cur_ps.weaponstate == WEAPON_CHARGING_ALT)
		{
			fovcheck = 160;
		}

		if (bs->frame_Enemy_Len < 128)
		{
			fovcheck *= 2;
		}

		if (InFieldOfVision(bs->viewangles, fovcheck, a))
		{
			if (bs->cur_ps.weapon == WP_THERMAL)
			{
				if (level.time - bs->cur_ps.weaponChargeTime < bs->frame_Enemy_Len * 2 &&
					level.time - bs->cur_ps.weaponChargeTime < 4000 &&
					bs->frame_Enemy_Len > 64 ||
					bs->cur_ps.weaponstate != WEAPON_CHARGING &&
					bs->cur_ps.weaponstate != WEAPON_CHARGING_ALT)
				{
					if (bs->cur_ps.weaponstate != WEAPON_CHARGING && bs->cur_ps.weaponstate != WEAPON_CHARGING_ALT)
					{
						if (bs->frame_Enemy_Len > 512 && bs->frame_Enemy_Len < 800)
						{
							bs->doAltAttack = 1;
						}
						else
						{
							bs->doAttack = 1;
						}
					}

					if (bs->cur_ps.weaponstate == WEAPON_CHARGING)
					{
						bs->doAttack = 1;
					}
					else if (bs->cur_ps.weaponstate == WEAPON_CHARGING_ALT)
					{
						bs->doAltAttack = 1;
					}
				}
			}
			else
			{
				const int secFire = ShouldSecondaryFire(bs);

				if (bs->cur_ps.weaponstate != WEAPON_CHARGING_ALT &&
					bs->cur_ps.weaponstate != WEAPON_CHARGING)
				{
					bs->altChargeTime = Q_irand(500, 1000);
				}

				if (secFire == 1)
				{
					bs->doAltAttack = 1;
				}
				else if (!secFire)
				{
					if (bs->cur_ps.weapon != WP_THERMAL)
					{
						if (bs->cur_ps.weaponstate != WEAPON_CHARGING ||
							bs->altChargeTime > level.time - bs->cur_ps.weaponChargeTime)
						{
							bs->doAttack = 1;
						}
					}
					else
					{
						bs->doAttack = 1;
					}
				}

				if (secFire == 2)
				{
					//released a charge
					return 1;
				}
			}
		}
	}

	return 0;
}

int next_point[MAX_CLIENTS];
//we messed up and got off the normal path, let's fall
//back to jumping around and turning in random
//directions off walls to see if we can get back to a
//good place.
int Gunner_BotFallbackNavigation(bot_state_t* bs)
{
	vec3_t b_angle, fwd, trto, mins, maxs;
	trace_t tr;

	if (bs->currentEnemy && bs->frame_Enemy_Vis)
	{
		trap->EA_MoveForward(bs->client);
	}

	mins[0] = -15;
	mins[1] = -15;
	mins[2] = 0;
	maxs[0] = 15;
	maxs[1] = 15;
	maxs[2] = 32;

	bs->goalAngles[PITCH] = 0;
	bs->goalAngles[ROLL] = 0;

	VectorCopy(bs->goalAngles, b_angle);

	AngleVectors(b_angle, fwd, NULL, NULL);

	trto[0] = bs->origin[0] + fwd[0] * 16;
	trto[1] = bs->origin[1] + fwd[1] * 16;
	trto[2] = bs->origin[2] + fwd[2] * 16;

	trap->Trace(&tr, bs->origin, mins, maxs, trto, ENTITYNUM_NONE, MASK_SOLID, qfalse, 0, 0);

	if (tr.fraction == 1)
	{
		trap->EA_MoveForward(bs->client);
	}
	else
	{
		bs->goalAngles[YAW] = rand() % 360;
	}

	return 0;
}

int Saber_BotFallbackNavigation(bot_state_t* bs)
{
	vec3_t b_angle, fwd, trto, mins, maxs;
	trace_t tr;
	vec3_t a, ang;

	if (bs->currentEnemy && bs->frame_Enemy_Vis)
	{
		trap->EA_MoveForward(bs->client);
	}

	mins[0] = -15;
	mins[1] = -15;
	mins[2] = 0;
	maxs[0] = 15;
	maxs[1] = 15;
	maxs[2] = 32;

	bs->goalAngles[PITCH] = 0;
	bs->goalAngles[ROLL] = 0;

	VectorCopy(bs->goalAngles, b_angle);

	AngleVectors(b_angle, fwd, NULL, NULL);

	if (bs->currentEnemy && bs->currentEnemy->health > 0
		&& bs->currentEnemy->client->ps.fallingToDeath == 0
		&& bs->frame_Enemy_Vis
		&& bs->currentEnemy->health > 0)
	{
		// head toward any enimies.
		VectorCopy(bs->currentEnemy->r.currentOrigin, trto);
	}
	else
	{
		// No current enemy. Randomize. Fallback code.
		if (next_point[bs->entitynum] < level.time
			|| VectorDistance(bs->goalPosition, bs->origin) < 64
			|| !(bs->velocity[0] < 32
				&& bs->velocity[1] < 32
				&& bs->velocity[2] < 32
				&& bs->velocity[0] > -32
				&& bs->velocity[1] > -32
				&& bs->velocity[2] > -32))
		{
			// Ready for a new point.
			const int choice = rand() % 4;
			qboolean found = qfalse;

			while (found == qfalse)
			{
				if (choice == 2)
				{
					trto[0] = bs->origin[0] - rand() % 1000;
					trto[1] = bs->origin[1] + rand() % 1000;
				}
				else if (choice == 3)
				{
					trto[0] = bs->origin[0] + rand() % 1000;
					trto[1] = bs->origin[1] - rand() % 1000;
				}
				else if (choice == 4)
				{
					trto[0] = bs->origin[0] - rand() % 1000;
					trto[1] = bs->origin[1] - rand() % 1000;
				}
				else
				{
					trto[0] = bs->origin[0] + rand() % 1000;
					trto[1] = bs->origin[1] + rand() % 1000;
				}

				trto[2] = bs->origin[2];

				if (OrgVisible(bs->origin, trto, -1))
					found = qtrue;
			}

			next_point[bs->entitynum] = level.time + 2000 + rand() % 5 * 1000;
		}
		else
		{
			// Still moving to the last point.
			trap->EA_MoveForward(bs->client);
		}
	}

	// Set the angles forward for checks.
	VectorSubtract(trto, bs->origin, a);
	vectoangles(a, ang);
	VectorCopy(ang, bs->goalAngles);

	trap->Trace(&tr, bs->origin, mins, maxs, trto, ENTITYNUM_NONE, MASK_SOLID, qfalse, 0, 0);

	if (tr.fraction == 1)
	{
		// Visible point.
		if (CheckFall_By_Vectors(bs->origin, ang, &g_entities[bs->entitynum]) == qtrue)
		{
			// We would fall.
			VectorCopy(bs->origin, bs->goalPosition); // Stay still.
		}
		else
		{
			// Try to walk to "trto" if we won't fall.
			VectorCopy(trto, bs->goalPosition); // Original.

			VectorSubtract(bs->goalPosition, bs->origin, a);
			vectoangles(a, ang);
			VectorCopy(ang, bs->goalAngles);

			BotChangeViewAngles(bs, bs->goalAngles[YAW]);
		}
	}
	else
	{
		int tries = 0;

		while (CheckFall_By_Vectors(bs->origin, ang, &g_entities[bs->entitynum]) == qtrue && tries <= bot_thinklevel.
			integer)
		{
			// Keep trying until we get something valid? Too much CPU maybe?
			bs->goalAngles[YAW] = rand() % 360; // Original.
			BotChangeViewAngles(bs, bs->goalAngles[YAW]);
			tries++;

			bs->goalAngles[PITCH] = 0;
			bs->goalAngles[ROLL] = 0;

			VectorCopy(bs->goalAngles, b_angle);

			AngleVectors(b_angle, fwd, NULL, NULL);

			VectorCopy(b_angle, ang);

			trto[0] = bs->origin[0] + rand() % 4 * 64;
			trto[1] = bs->origin[1] + rand() % 4 * 64;
			trto[2] = bs->origin[2];

			VectorCopy(trto, bs->goalPosition); // Move to new goal.
		}

		if (tries > bot_thinklevel.integer)
		{
			// Ran out of random attempts.
			Gunner_BotFallbackNavigation(bs);
		}
	}

	return 1; // Success!
}

int BotTryAnotherWeapon(bot_state_t* bs)
{
	int i = 1;

	while (i < WP_NUM_WEAPONS)
	{
		if (bs->cur_ps.ammo[weaponData[i].ammoIndex] >= weaponData[i].energyPerShot &&
			bs->cur_ps.stats[STAT_WEAPONS] & 1 << i)
		{
			bs->virtualWeapon = i;
			BotSelectWeapon(bs->client, i);
			return 1;
		}

		i++;
	}

	if (bs->cur_ps.weapon != 1 && bs->virtualWeapon != 1)
	{
		//should always have this.. shouldn't we?
		bs->virtualWeapon = 1;
		BotSelectWeapon(bs->client, 1);
		return 1;
	}

	return 0;
}

//is this weapon available to us?
qboolean BotWeaponSelectable(const bot_state_t* bs, const int weapon)
{
	if (weapon == WP_NONE)
	{
		return qfalse;
	}

	if (bs->cur_ps.ammo[weaponData[weapon].ammoIndex] >= weaponData[weapon].energyPerShot &&
		bs->cur_ps.stats[STAT_WEAPONS] & 1 << weapon)
	{
		return qtrue;
	}

	return qfalse;
}

//select the best weapon we can
int BotSelectIdealWeapon(bot_state_t* bs)
{
	int bestweight = -1;
	int bestweapon = 0;

	int i = 0;

	while (i < WP_NUM_WEAPONS)
	{
		if (bs->cur_ps.ammo[weaponData[i].ammoIndex] >= weaponData[i].energyPerShot &&
			bs->botWeaponWeights[i] > bestweight &&
			bs->cur_ps.stats[STAT_WEAPONS] & 1 << i)
		{
			if (i == WP_THERMAL)
			{
				//special case..
				if (bs->currentEnemy && bs->frame_Enemy_Len < 700)
				{
					bestweight = bs->botWeaponWeights[i];
					bestweapon = i;
				}
			}
			else
			{
				bestweight = bs->botWeaponWeights[i];
				bestweapon = i;
			}
		}

		i++;
	}

	if (bs->currentEnemy && bs->frame_Enemy_Len < 300 &&
		(bestweapon == WP_BRYAR_PISTOL || bestweapon == WP_BLASTER || bestweapon == WP_BOWCASTER) &&
		bs->cur_ps.stats[STAT_WEAPONS] & 1 << WP_SABER)
	{
		bestweapon = WP_SABER;
		bestweight = 1;
	}

	if (bs->currentEnemy && bs->frame_Enemy_Len > 300 &&
		bs->currentEnemy->client && bs->currentEnemy->client->ps.weapon != WP_SABER &&
		bestweapon == WP_SABER)
	{
		//if the enemy is far away, and we have our saber selected, see if we have any good distance weapons instead
		if (BotWeaponSelectable(bs, WP_DISRUPTOR))
		{
			bestweapon = WP_DISRUPTOR;
			bestweight = 1;
		}
		else if (BotWeaponSelectable(bs, WP_ROCKET_LAUNCHER))
		{
			bestweapon = WP_ROCKET_LAUNCHER;
			bestweight = 1;
		}
		else if (BotWeaponSelectable(bs, WP_BOWCASTER))
		{
			bestweapon = WP_BOWCASTER;
			bestweight = 1;
		}
		else if (BotWeaponSelectable(bs, WP_BLASTER))
		{
			bestweapon = WP_BLASTER;
			bestweight = 1;
		}
		else if (BotWeaponSelectable(bs, WP_REPEATER))
		{
			bestweapon = WP_REPEATER;
			bestweight = 1;
		}
		else if (BotWeaponSelectable(bs, WP_DEMP2))
		{
			bestweapon = WP_DEMP2;
			bestweight = 1;
		}
	}

	//assert(bs->cur_ps.weapon > 0 && bestweapon > 0);

	if (bestweight != -1 && bs->cur_ps.weapon != bestweapon && bs->virtualWeapon != bestweapon)
	{
		bs->virtualWeapon = bestweapon;
		BotSelectWeapon(bs->client, bestweapon);
		//bs->cur_ps.weapon = bestweapon;
		//level.clients[bs->client].ps.weapon = bestweapon;
		return 1;
	}

	//assert(bs->cur_ps.weapon > 0);

	return 0;
}

//override our standard weapon choice with a melee weapon
int BotSelectMelee(bot_state_t* bs)
{
	if (bs->cur_ps.weapon != 1 && bs->virtualWeapon != 1)
	{
		bs->virtualWeapon = 1;
		BotSelectWeapon(bs->client, 1);
		return 1;
	}

	return 0;
}

//See if we our in love with the potential bot.
int GetLoveLevel(const bot_state_t* bs, const bot_state_t* love)
{
	int i = 0;

	if (level.gametype == GT_DUEL || level.gametype == GT_POWERDUEL)
	{
		//There is no love in 1-on-1
		return 0;
	}

	if (!bs || !love || !g_entities[love->client].client)
	{
		return 0;
	}

	if (!bs->lovednum)
	{
		return 0;
	}

	if (!bot_attachments.integer)
	{
		return 1;
	}

	const char* lname = g_entities[love->client].client->pers.netname;

	if (!lname)
	{
		return 0;
	}

	while (i < bs->lovednum)
	{
		if (strcmp(bs->loved[i].name, lname) == 0)
		{
			return bs->loved[i].level;
		}

		i++;
	}

	return 0;
}

//Our loved one was killed. We must become infuriated!
void BotLovedOneDied(bot_state_t* bs, const bot_state_t* loved, const int lovelevel)
{
	if (!loved->lastHurt || !loved->lastHurt->client ||
		loved->lastHurt->s.number == loved->client)
	{
		return;
	}

	if (level.gametype == GT_DUEL || level.gametype == GT_POWERDUEL)
	{
		//There is no love in 1-on-1
		return;
	}

	if (!is_teamplay())
	{
		if (lovelevel < 2)
		{
			return;
		}
	}
	else if (OnSameTeam(&g_entities[bs->client], loved->lastHurt))
	{
		//don't hate teammates no matter what
		return;
	}

	if (loved->client == loved->lastHurt->s.number)
	{
		return;
	}

	if (bs->client == loved->lastHurt->s.number)
	{
		//oops!
		return;
	}

	if (!bot_attachments.integer)
	{
		return;
	}

	if (!PassLovedOneCheck(bs, loved->lastHurt))
	{
		//a loved one killed a loved one.. you cannot hate them
		bs->chatObject = loved->lastHurt;
		bs->chatAltObject = &g_entities[loved->client];
		BotDoChat(bs, "LovedOneKilledLovedOne", 0);
		return;
	}

	if (bs->revengeEnemy == loved->lastHurt)
	{
		if (bs->revengeHateLevel < bs->loved_death_thresh)
		{
			bs->revengeHateLevel++;

			if (bs->revengeHateLevel == bs->loved_death_thresh)
			{
				//broke into the highest anger level
				//CHAT: Hatred section
				bs->chatObject = loved->lastHurt;
				bs->chatAltObject = NULL;
				BotDoChat(bs, "Hatred", 1);
			}
		}
	}
	else if (bs->revengeHateLevel < bs->loved_death_thresh - 1)
	{
		//only switch hatred if we don't hate the existing revenge-enemy too much
		//CHAT: BelovedKilled section
		bs->chatObject = &g_entities[loved->client];
		bs->chatAltObject = loved->lastHurt;
		BotDoChat(bs, "BelovedKilled", 0);
		bs->revengeHateLevel = 0;
		bs->revengeEnemy = loved->lastHurt;
	}
}

void BotDeathNotify(const bot_state_t* bs)
{
	//in case someone has an emotional attachment to us, we'll notify them
	int i = 0;

	while (i < MAX_CLIENTS)
	{
		if (botstates[i] && botstates[i]->lovednum)
		{
			int ltest = 0;
			while (ltest < botstates[i]->lovednum)
			{
				if (strcmp(level.clients[bs->client].pers.netname, botstates[i]->loved[ltest].name) == 0)
				{
					BotLovedOneDied(botstates[i], bs, botstates[i]->loved[ltest].level);
					break;
				}

				ltest++;
			}
		}

		i++;
	}
}

//perform strafe trace checks
void StrafeTracing(bot_state_t* bs)
{
	vec3_t mins, maxs;
	vec3_t right, rorg, drorg;
	trace_t tr;

	mins[0] = -15;
	mins[1] = -15;
	//mins[2] = -24;
	mins[2] = -22;
	maxs[0] = 15;
	maxs[1] = 15;
	maxs[2] = 32;

	AngleVectors(bs->viewangles, NULL, right, NULL);

	if (bs->meleeStrafeDir)
	{
		rorg[0] = bs->origin[0] - right[0] * 32;
		rorg[1] = bs->origin[1] - right[1] * 32;
		rorg[2] = bs->origin[2] - right[2] * 32;
	}
	else
	{
		rorg[0] = bs->origin[0] + right[0] * 32;
		rorg[1] = bs->origin[1] + right[1] * 32;
		rorg[2] = bs->origin[2] + right[2] * 32;
	}

	trap->Trace(&tr, bs->origin, mins, maxs, rorg, bs->client, MASK_SOLID, qfalse, 0, 0);

	if (tr.fraction != 1)
	{
		bs->meleeStrafeDisable = level.time + Q_irand(500, 1500);
	}

	VectorCopy(rorg, drorg);

	drorg[2] -= 32;

	trap->Trace(&tr, rorg, NULL, NULL, drorg, bs->client, MASK_SOLID, qfalse, 0, 0);

	if (tr.fraction == 1)
	{
		//this may be a dangerous ledge, so don't strafe over it just in case
		bs->meleeStrafeDisable = level.time + Q_irand(500, 1500);
	}
}

//doing primary weapon fire
int PrimFiring(const bot_state_t* bs)
{
	if (bs->cur_ps.weaponstate != WEAPON_CHARGING &&
		bs->doAttack)
	{
		return 1;
	}

	if (bs->cur_ps.weaponstate == WEAPON_CHARGING &&
		!bs->doAttack)
	{
		return 1;
	}

	return 0;
}

//should we keep our primary weapon from firing?
int KeepPrimFromFiring(bot_state_t* bs)
{
	if (bs->cur_ps.weaponstate != WEAPON_CHARGING &&
		bs->doAttack)
	{
		bs->doAttack = 0;
	}

	if (bs->cur_ps.weaponstate == WEAPON_CHARGING &&
		!bs->doAttack)
	{
		bs->doAttack = 1;
	}

	return 0;
}

//doing secondary weapon fire
int AltFiring(const bot_state_t* bs)
{
	if (bs->cur_ps.weaponstate != WEAPON_CHARGING_ALT &&
		bs->doAltAttack)
	{
		return 1;
	}

	if (bs->cur_ps.weaponstate == WEAPON_CHARGING_ALT &&
		!bs->doAltAttack)
	{
		return 1;
	}

	return 0;
}

//should we keep our alt from firing?
int KeepAltFromFiring(bot_state_t* bs)
{
	if (bs->cur_ps.weaponstate != WEAPON_CHARGING_ALT &&
		bs->doAltAttack)
	{
		bs->doAltAttack = 0;
	}

	if (bs->cur_ps.weaponstate == WEAPON_CHARGING_ALT &&
		!bs->doAltAttack)
	{
		bs->doAltAttack = 1;
	}

	return 0;
}

//Try not to shoot our friends in the back. Or in the face. Or anywhere, really.
gentity_t* CheckForFriendInLOF(const bot_state_t* bs)
{
	vec3_t fwd;
	vec3_t trfrom, trto;
	vec3_t mins, maxs;
	trace_t tr;

	mins[0] = -3;
	mins[1] = -3;
	mins[2] = -3;

	maxs[0] = 3;
	maxs[1] = 3;
	maxs[2] = 3;

	AngleVectors(bs->viewangles, fwd, NULL, NULL);

	VectorCopy(bs->eye, trfrom);

	trto[0] = trfrom[0] + fwd[0] * 2048;
	trto[1] = trfrom[1] + fwd[1] * 2048;
	trto[2] = trfrom[2] + fwd[2] * 2048;

	trap->Trace(&tr, trfrom, mins, maxs, trto, bs->client, MASK_PLAYERSOLID, qfalse, 0, 0);

	if (tr.fraction != 1 && tr.entityNum <= MAX_CLIENTS)
	{
		gentity_t* trent = &g_entities[tr.entityNum];

		if (trent && trent->client)
		{
			if (is_teamplay() && OnSameTeam(&g_entities[bs->client], trent))
			{
				return trent;
			}

			if (botstates[trent->s.number] && GetLoveLevel(bs, botstates[trent->s.number]) > 1)
			{
				return trent;
			}
		}
	}

	return NULL;
}

void BotScanForLeader(bot_state_t* bs)
{
	//bots will only automatically obtain a leader if it's another bot using this method.
	int i = 0;

	if (bs->isSquadLeader)
	{
		return;
	}

	while (i < MAX_CLIENTS)
	{
		gentity_t* ent = &g_entities[i];

		if (ent && ent->client && botstates[i] && botstates[i]->isSquadLeader && bs->client != i)
		{
			if (OnSameTeam(&g_entities[bs->client], ent))
			{
				bs->squadLeader = ent;
				break;
			}
			if (GetLoveLevel(bs, botstates[i]) > 1 && !is_teamplay())
			{
				//ignore love status regarding squad leaders if we're in teamplay
				bs->squadLeader = ent;
				break;
			}
		}

		i++;
	}
}

//w3rd to the p33pz.
void BotReplyGreetings(const bot_state_t* bs)
{
	int i = 0;
	int numhello = 0;

	while (i < MAX_CLIENTS)
	{
		if (botstates[i] &&
			botstates[i]->canChat &&
			i != bs->client)
		{
			botstates[i]->chatObject = &g_entities[bs->client];
			botstates[i]->chatAltObject = NULL;
			if (BotDoChat(botstates[i], "ResponseGreetings", 0))
			{
				numhello++;
			}
		}

		if (numhello > 3)
		{
			//don't let more than 4 bots say hello at once
			return;
		}

		i++;
	}
}

//try to move in to grab a nearby flag
void CTFFlagMovement(bot_state_t* bs)
{
	const gentity_t* desiredDrop = NULL;
	vec3_t a, mins, maxs;
	trace_t tr;

	mins[0] = -15;
	mins[1] = -15;
	mins[2] = -7;
	maxs[0] = 15;
	maxs[1] = 15;
	maxs[2] = 7;

	if (bs->wantFlag && bs->wantFlag->flags & FL_DROPPED_ITEM)
	{
		if (bs->staticFlagSpot[0] == bs->wantFlag->s.pos.trBase[0] &&
			bs->staticFlagSpot[1] == bs->wantFlag->s.pos.trBase[1] &&
			bs->staticFlagSpot[2] == bs->wantFlag->s.pos.trBase[2])
		{
			VectorSubtract(bs->origin, bs->wantFlag->s.pos.trBase, a);

			if (VectorLength(a) <= BOT_FLAG_GET_DISTANCE)
			{
				VectorCopy(bs->wantFlag->s.pos.trBase, bs->goalPosition);
				return;
			}
			bs->wantFlag = NULL;
		}
		else
		{
			bs->wantFlag = NULL;
		}
	}
	else if (bs->wantFlag)
	{
		bs->wantFlag = NULL;
	}

	if (flagRed && flagBlue)
	{
		if (bs->wpDestination == flagRed ||
			bs->wpDestination == flagBlue)
		{
			int diddrop = 0;
			if (bs->wpDestination == flagRed && droppedRedFlag && droppedRedFlag->flags & FL_DROPPED_ITEM &&
				droppedRedFlag->classname && strcmp(droppedRedFlag->classname, "freed") != 0)
			{
				desiredDrop = droppedRedFlag;
				diddrop = 1;
			}
			if (bs->wpDestination == flagBlue && droppedBlueFlag && droppedBlueFlag->flags & FL_DROPPED_ITEM &&
				droppedBlueFlag->classname && strcmp(droppedBlueFlag->classname, "freed") != 0)
			{
				desiredDrop = droppedBlueFlag;
				diddrop = 1;
			}

			if (diddrop && desiredDrop)
			{
				VectorSubtract(bs->origin, desiredDrop->s.pos.trBase, a);

				if (VectorLength(a) <= BOT_FLAG_GET_DISTANCE)
				{
					trap->Trace(&tr, bs->origin, mins, maxs, desiredDrop->s.pos.trBase, bs->client, MASK_SOLID, qfalse,
						0, 0);

					if (tr.fraction == 1 || tr.entityNum == desiredDrop->s.number)
					{
						VectorCopy(desiredDrop->s.pos.trBase, bs->goalPosition);
						VectorCopy(desiredDrop->s.pos.trBase, bs->staticFlagSpot);
					}
				}
			}
		}
	}
}

//see if we want to make our detpacks blow up
void BotCheckDetPacks(bot_state_t* bs)
{
	gentity_t* dp = NULL;
	gentity_t* myDet = NULL;
	vec3_t a;

	while ((dp = G_Find(dp, FOFS(classname), "detpack")) != NULL)
	{
		if (dp && dp->parent && dp->parent->s.number == bs->client)
		{
			myDet = dp;
			break;
		}
	}

	if (!myDet)
	{
		return;
	}

	if (!bs->currentEnemy || !bs->currentEnemy->client || !bs->frame_Enemy_Vis)
	{
		//require the enemy to be visilbe just to be fair..
		//unless..
		if (bs->currentEnemy && bs->currentEnemy->client &&
			level.time - bs->plantContinue < 5000)
		{
			//it's a fresh plant (within 5 seconds) so we should be able to guess
			goto stillmadeit;
		}
		return;
	}

stillmadeit:

	VectorSubtract(bs->currentEnemy->client->ps.origin, myDet->s.pos.trBase, a);
	const float enLen = VectorLength(a);

	VectorSubtract(bs->origin, myDet->s.pos.trBase, a);
	const float myLen = VectorLength(a);

	if (enLen > myLen)
	{
		return;
	}

	if (enLen < BOT_PLANT_BLOW_DISTANCE && OrgVisible(bs->currentEnemy->client->ps.origin, myDet->s.pos.trBase,
		bs->currentEnemy->s.number))
	{
		//we could just call the "blow all my detpacks" function here, but I guess that's cheating.
		bs->plantKillEmAll = level.time + 500;
	}
}

/*
==================
BotBuyItem
==================
*/
qboolean bot_buy_item(bot_state_t* bs, const char* msg)
{
	int j;

	if (!bs)
	{
		return qfalse;
	}

	if (!g_AllowBotBuyItem.integer)
	{
		return qtrue;
	}

	if (level.time < bs->nextPurchase)
	{
		return qfalse;
	}

	bs->nextPurchase = level.time + 10000; //ten seconds

	for (int i = 0; bg_buylist[i].text; i++)
	{
		// allow partial match
		if (Q_stricmpn(msg, bg_buylist[i].text, strlen(msg)) == 0)
		{
			if (bs->cur_ps.persistant[PERS_SCORE] >= bg_buylist[i].price)
			{
				int tag = bg_buylist[i].giTag;
				const int type = bg_buylist[i].giType;
				int quantity = bg_buylist[i].quantity;
				int cost = bg_buylist[i].price;

				switch (type)
				{
				case IT_WEAPON:

					if (tag <= WP_NONE || tag >= WP_NUM_WEAPONS)
					{
						return qtrue; // invalid weapon
					}

					if (bs->cur_ps.stats[STAT_WEAPONS] & 1 << tag)
					{
						return qtrue; // already holding weapon
					}
					bs->cur_ps.stats[STAT_WEAPONS] |= 1 << tag;
					tag = weaponData[tag].ammoIndex;
				case IT_AMMO:
					// buy ammo for the primary gun
					if (tag == AMMO_NONE)
					{
						const int weapon = bs->cur_ps.weapon;

						if (weapon > WP_NONE && weapon < WP_NUM_WEAPONS)
						{
							const int ammo = weaponData[weapon].ammoIndex;

							if (ammo > AMMO_NONE && ammo < AMMO_MAX)
							{
								for (int i1 = 0; bg_buylist[i1].text; i1++)
								{
									if (bg_buylist[i1].giType == IT_AMMO && bg_buylist[i1].giTag == ammo)
									{
										tag = ammo;
										cost = bg_buylist[i1].giType;
										quantity = bg_buylist[i1].quantity;
									}
								}
							}
							else
							{
								return qfalse; //  weapon doesn't use ammo
							}
						}
						else
						{
							return qfalse; // invalid weapon
						}
					}

					if (tag <= AMMO_NONE || tag >= AMMO_MAX)
					{
						return qfalse; // invald ammo type
					}

					if (bs->cur_ps.ammo[tag] < ammoData[tag].max)
					{
						bs->cur_ps.ammo[tag] += quantity;
						if (bs->cur_ps.ammo[tag] > ammoData[tag].max)
						{
							bs->cur_ps.ammo[tag] = ammoData[tag].max;
						}
					}
					else
					{
						return qfalse; // ammo full
					}

					break;
				case IT_ARMOR:
				{
					int value = bs->cur_ps.stats[STAT_ARMOR];
					const int max = 100 + 25 * tag;

					if (value + 2 >= max)
					{
						return qfalse;
					}

					value += quantity;

					if (value > max)
					{
						value = max;
					}
					bs->cur_ps.stats[STAT_ARMOR] = value;
				}
				break;
				case IT_HEALTH:
				{
					int value = bs->cur_ps.stats[STAT_MAX_HEALTH];
					const int max = bs->cur_ps.stats[STAT_MAX_HEALTH] + 25 * tag;

					if (value + 2 >= max)
					{
						return qfalse;
					}

					value += quantity;

					if (value > max)
					{
						value = max;
					}
					bs->cur_ps.stats[STAT_HEALTH] = g_entities[bs->client].health = value;
				}
				break;
				case IT_POWERUP:
					break;
				case IT_HOLDABLE:
				{
					if (bs->cur_ps.stats[STAT_HOLDABLE_ITEMS] & 1 << tag)
					{
						return qfalse; // already have
					}
					bs->cur_ps.stats[STAT_HOLDABLE_ITEM] = BG_GetItemIndexByTag(tag, IT_HOLDABLE);
					bs->cur_ps.stats[STAT_HOLDABLE_ITEMS] |= 1 << tag;
				}
				break;
				case IT_PERSISTANT_POWERUP:
					break;
				default:
					return qfalse;
				}

				if (cost > 0)
				{
					bs->cur_ps.persistant[PERS_SCORE] -= cost;
				}

				// one frigged-up trick to fake an item pickup
				for (j = 0; j < bg_numItems; j++)
				{
					if (bg_itemlist[j].giType != bg_buylist[i].giType)
					{
						continue;
					}

					switch (bg_buylist[i].giType)
					{
					case IT_WEAPON:
					case IT_AMMO:
					case IT_POWERUP:
					case IT_HOLDABLE:
					case IT_PERSISTANT_POWERUP:
					case IT_TEAM:
						if (bg_itemlist[j].giTag != bg_buylist[i].giTag)
						{
							continue;
						}
						break;
					case IT_HEALTH:
					case IT_ARMOR:
						break;
					default:
						continue;
					}
					break;
				}

				if (j > 0 && j < bg_numItems)
				{
					gentity_t* te = G_TempEntity(g_entities[bs->client].s.pos.trBase, EV_ITEM_PICKUP);
					te->s.eventParm = te - g_entities;
					te->s.modelindex = j;
					te->s.number = bs->client;
				}
				return qtrue;
			}
			bs->nextPurchase = level.time + 2000;
			// we can make another purchase sooner in case we tried to buy something expensive
			return qfalse; // not enough cash
		}
	}

	return qfalse; // couldn't find item
}

//see if it would be beneficial at this time to use one of our inv items
int BotUseInventoryItem(bot_state_t* bs)
{
	if (bs->cur_ps.stats[STAT_HOLDABLE_ITEMS] & 1 << HI_MEDPAC)
	{
		if (g_entities[bs->client].health <= 75)
		{
			bs->cur_ps.stats[STAT_HOLDABLE_ITEM] = BG_GetItemIndexByTag(HI_MEDPAC, IT_HOLDABLE);
			goto wantuseitem;
		}
	}
	if (bs->cur_ps.stats[STAT_HOLDABLE_ITEMS] & 1 << HI_MEDPAC_BIG)
	{
		if (g_entities[bs->client].health <= 50)
		{
			bs->cur_ps.stats[STAT_HOLDABLE_ITEM] = BG_GetItemIndexByTag(HI_MEDPAC_BIG, IT_HOLDABLE);
			goto wantuseitem;
		}
	}
	if (bs->cur_ps.stats[STAT_HOLDABLE_ITEMS] & 1 << HI_SEEKER)
	{
		if (bs->currentEnemy && bs->frame_Enemy_Vis)
		{
			bs->cur_ps.stats[STAT_HOLDABLE_ITEM] = BG_GetItemIndexByTag(HI_SEEKER, IT_HOLDABLE);
			goto wantuseitem;
		}
	}
	if (bs->cur_ps.stats[STAT_HOLDABLE_ITEMS] & 1 << HI_SENTRY_GUN)
	{
		if (bs->currentEnemy && bs->frame_Enemy_Vis)
		{
			bs->cur_ps.stats[STAT_HOLDABLE_ITEM] = BG_GetItemIndexByTag(HI_SENTRY_GUN, IT_HOLDABLE);
			goto wantuseitem;
		}
	}
	if (bs->cur_ps.stats[STAT_HOLDABLE_ITEMS] & 1 << HI_SHIELD)
	{
		if (bs->currentEnemy && bs->frame_Enemy_Vis && bs->runningToEscapeThreat)
		{
			//this will (hopefully) result in the bot placing the shield down while facing
			bs->cur_ps.stats[STAT_HOLDABLE_ITEM] = BG_GetItemIndexByTag(HI_SHIELD, IT_HOLDABLE);
			goto wantuseitem;
		}
	}
	if (bs->cur_ps.stats[STAT_HOLDABLE_ITEMS] & 1 << HI_JETPACK && bs->cur_ps.jetpackFuel > 50)
	{
		if (bs->currentEnemy && bs->frame_Enemy_Vis && bs->runningToEscapeThreat)
		{
			bs->cur_ps.stats[STAT_HOLDABLE_ITEM] = BG_GetItemIndexByTag(HI_JETPACK, IT_HOLDABLE);
			goto wantuseitem;
		}
		if (bs->currentEnemy && bs->frame_Enemy_Vis)
		{
			bs->cur_ps.stats[STAT_HOLDABLE_ITEM] = BG_GetItemIndexByTag(HI_JETPACK, IT_HOLDABLE);
			goto wantuseitem;
		}
		if (bs->iHaveNoIdeaWhereIAmGoing)
		{
			bs->cur_ps.stats[STAT_HOLDABLE_ITEM] = BG_GetItemIndexByTag(HI_JETPACK, IT_HOLDABLE);
			goto wantuseitem;
		}
		if (bs->cur_ps.groundEntityNum == ENTITYNUM_NONE)
		{
			bs->cur_ps.stats[STAT_HOLDABLE_ITEM] = BG_GetItemIndexByTag(HI_JETPACK, IT_HOLDABLE);
			goto wantuseitem;
		}
		if (g_entities[bs->client].health <= 75)
		{
			bs->cur_ps.stats[STAT_HOLDABLE_ITEM] = BG_GetItemIndexByTag(HI_JETPACK, IT_HOLDABLE);
			goto wantuseitem;
		}
	}
	else if (bs->cur_ps.stats[STAT_HOLDABLE_ITEMS] & 1 << HI_FLAMETHROWER)
	{
		if (bs->currentEnemy && bs->frame_Enemy_Len <= MELEE_ATTACK_RANGE)
		{
			bs->cur_ps.stats[STAT_HOLDABLE_ITEM] = BG_GetItemIndexByTag(HI_FLAMETHROWER, IT_HOLDABLE);
			goto wantuseitem;
		}
		if (bs->currentEnemy && bs->frame_Enemy_Len <= MELEE_ATTACK_RANGE && bs->cur_ps.groundEntityNum ==
			ENTITYNUM_NONE)
		{
			bs->cur_ps.stats[STAT_HOLDABLE_ITEM] = BG_GetItemIndexByTag(IT_HOLDABLE, IT_HOLDABLE);
			goto wantuseitem;
		}
		if (bs->currentEnemy && bs->frame_Enemy_Len <= MELEE_ATTACK_RANGE && g_entities[bs->client].health <= 50)
		{
			bs->cur_ps.stats[STAT_HOLDABLE_ITEM] = BG_GetItemIndexByTag(IT_HOLDABLE, IT_HOLDABLE);
			goto wantuseitem;
		}
	}
	if (bs->cur_ps.stats[STAT_HOLDABLE_ITEMS] & 1 << HI_CLOAK
		&& !PM_SaberInAttack(bs->cur_ps.saberMove)
		&& (!bs->cur_ps.powerups[PW_CLOAKED])
		&& (!(bs->cur_ps.communicatingflags & 1 << CLOAK_CHARGE_RESTRICTION)))
	{
		if (bs->currentEnemy && bs->frame_Enemy_Vis && bs->frame_Enemy_Len <= 1000)
		{
			bs->cur_ps.stats[STAT_HOLDABLE_ITEM] = BG_GetItemIndexByTag(HI_CLOAK, IT_HOLDABLE);
			goto wantuseitem;
		}
	}

	return 0;

wantuseitem:
	level.clients[bs->client].ps.stats[STAT_HOLDABLE_ITEM] = bs->cur_ps.stats[STAT_HOLDABLE_ITEM];

	return 1;
}

//trace forward to see if we can plant a detpack or something
int BotSurfaceNear(const bot_state_t* bs)
{
	trace_t tr;
	vec3_t fwd;

	AngleVectors(bs->viewangles, fwd, NULL, NULL);

	fwd[0] = bs->origin[0] + fwd[0] * 64;
	fwd[1] = bs->origin[1] + fwd[1] * 64;
	fwd[2] = bs->origin[2] + fwd[2] * 64;

	trap->Trace(&tr, bs->origin, NULL, NULL, fwd, bs->client, MASK_SOLID, qfalse, 0, 0);

	if (tr.fraction != 1)
	{
		return 1;
	}

	return 0;
}

//could we block projectiles from the weapon potentially with a light saber?
int bot_weapon_blockable(const int weapon)
{
	switch (weapon)
	{
	case WP_STUN_BATON:
	case WP_MELEE:
		return 0;
	case WP_DISRUPTOR:
		return 0;
	case WP_DEMP2:
		return 0;
	case WP_ROCKET_LAUNCHER:
		return 0;
	case WP_THERMAL:
		return 0;
	case WP_TRIP_MINE:
		return 0;
	case WP_DET_PACK:
		return 0;
	default:
		return 1;
	}
}

void Cmd_EngageDuel_f(gentity_t* ent);
void Cmd_ToggleSaber_f(gentity_t* ent);

//movement overrides
void Bot_SetForcedMovement(const int bot, const int forward, const int right, const int up)
{
	bot_state_t* bs = botstates[bot];

	if (!bs)
	{
		//not a bot
		return;
	}

	if (forward != -1)
	{
		if (bs->forceMove_Forward)
		{
			bs->forceMove_Forward = 0;
		}
		else
		{
			bs->forceMove_Forward = forward;
		}
	}
	if (right != -1)
	{
		if (bs->forceMove_Right)
		{
			bs->forceMove_Right = 0;
		}
		else
		{
			bs->forceMove_Right = right;
		}
	}
	if (up != -1)
	{
		if (bs->forceMove_Up)
		{
			bs->forceMove_Up = 0;
		}
		else
		{
			bs->forceMove_Up = up;
		}
	}
}

//check to see if the bot should switch player
qboolean switch_siege_ideal_class(const bot_state_t* bs, const char* idealclass)
{
	char cmp[MAX_STRING_CHARS];
	int i = 0;

	while (idealclass[i])
	{
		int j = 0;
		while (idealclass[i] && idealclass[i] != '|')
		{
			cmp[j] = idealclass[i];
			i++;
			j++;
		}
		cmp[j] = 0;

		if (NumberofSiegeSpecificClass(g_entities[bs->client].client->sess.siegeDesiredTeam, cmp))
		{
			//found a player that can hack this trigger
			return qfalse;
		}

		if (idealclass[i] != '|')
		{
			//reached the end, so, switch to a unit that can hack this trigger.
			siegeClass_t* holdClass = BG_SiegeFindClassByName(cmp);
			if (holdClass)
			{
				trap->EA_Command(bs->client, va("siegeclass \"%s\"\n", holdClass->name));
				return qtrue;
			}
			return qfalse;
		}
		i++;
	}

	return qfalse;
}

//find a numberof a specific class on a team.
int NumberofSiegeSpecificClass(const int team, const char* classname)
{
	int NumPlayers = 0;

	for (int i = 0; i < MAX_CLIENTS; i++)
	{
		const gentity_t* ent = &g_entities[i];
		if (ent && ent->client && ent->client->pers.connected == CON_CONNECTED
			&& ent->client->sess.siegeClass && ent->client->sess.siegeDesiredTeam == team)
		{
			if (strcmp(ent->client->sess.siegeClass, classname) == 0)
			{
				NumPlayers++;
			}
		}
	}
	return NumPlayers;
}

//has the bot select the siege class with the fewest number of players on this team.
void SelectBestSiegeClass(const int ClientNum, const qboolean ForceJoin)
{
	int bestNum = MAX_CLIENTS;
	int bestBaseClass = -1;

	if (ClientNum < 0 || ClientNum > MAX_CLIENTS)
	{
		//bad ClientNum
		return;
	}

	for (int i = 0; i < SPC_MAX; i++)
	{
		const int x = NumberofSiegeBasicClasses(g_entities[ClientNum].client->sess.siegeDesiredTeam, i);
		if (x < bestNum)
		{
			//this class has fewer players.
			bestNum = x;
			bestBaseClass = i;
		}
	}

	if (bestBaseClass != -1)
	{
		//found one, join that class
		siegeClass_t* holdClass = BG_GetClassOnBaseClass(g_entities[ClientNum].client->sess.siegeDesiredTeam,
			bestBaseClass, 0);
		if (Q_stricmp(g_entities[ClientNum].client->sess.siegeClass, holdClass->name)
			|| ForceJoin)
		{
			//we're not already this class.
			trap->EA_Command(ClientNum, va("siegeclass \"%s\"\n", holdClass->name));
		}
	}
}

//the main AI loop.
//please don't be too frightened.
void StandardBotAI(bot_state_t* bs, float thinktime)
{
	const int saberNum = 0;
	int doingFallback = 0;
	int fjHalt;
	vec3_t a;
	vec3_t ang;
	vec3_t a_fo;
	float reaction;
	int meleestrafe = 0;
	int useTheForce = 0;
	int forceHostile = 0;
	gentity_t* friendInLOF = 0;
	vec3_t preFrameGAngles;
	vec3_t moveDir;

	//Reset the action states
	bs->doAttack = qfalse;
	bs->doAltAttack = qfalse;
	bs->doSaberThrow = qfalse;
	bs->doBotKick = qfalse;
	bs->doWalk = qfalse;
	bs->virtualWeapon = bs->cur_ps.weapon;

	if (gDeactivated || g_entities[bs->client].client->tempSpectate > level.time)
	{
		bs->wpCurrent = NULL;
		bs->currentEnemy = NULL;
		bs->wpDestination = NULL;
		bs->wpDirection = 0;
		ClearRoute(bs->botRoute);
		VectorClear(bs->lastDestPosition);
		bs->wpSpecial = qfalse;

		//reset tactical stuff
		bs->tacticEntity = NULL;
		bs->objectiveType = 0;
		bs->MiscBotFlags = 0;
		return;
	}

	if (g_entities[bs->client].inuse &&
		g_entities[bs->client].client &&
		g_entities[bs->client].client->sess.sessionTeam == TEAM_SPECTATOR)
	{
		bs->wpCurrent = NULL;
		bs->currentEnemy = NULL;
		bs->wpDestination = NULL;
		bs->wpDirection = 0;
		return;
	}

#ifndef FINAL_BUILD
	if (bot_getinthecarrr.integer)
	{ //stupid vehicle debug, I tire of having to connect another client to test passengers.
		gentity_t* botEnt = &g_entities[bs->client];

		if (botEnt->inuse && botEnt->client && botEnt->client->ps.m_iVehicleNum)
		{ //in a vehicle, so...
			bs->noUseTime = level.time + 5000;

			if (bot_getinthecarrr.integer != 2)
			{
				trap->EA_MoveForward(bs->client);

				if (bot_getinthecarrr.integer == 3)
				{ //use alt fire
					trap->EA_Alt_Attack(bs->client);
				}
			}
		}
		else
		{ //find one, get in
			int i = 0;
			gentity_t* vehicle = NULL;
			//find the nearest, manned vehicle
			while (i < MAX_GENTITIES)
			{
				vehicle = &g_entities[i];

				if (vehicle->inuse && vehicle->client && vehicle->s.eType == ET_NPC &&
					vehicle->s.NPC_class == CLASS_VEHICLE && vehicle->m_pVehicle &&
					(vehicle->client->ps.m_iVehicleNum || bot_getinthecarrr.integer == 2))
				{ //ok, this is a vehicle, and it has a pilot/passengers
					break;
				}
				i++;
			}
			if (i != MAX_GENTITIES && vehicle)
			{ //broke before end so we must've found something
				vec3_t v;

				VectorSubtract(vehicle->client->ps.origin, bs->origin, v);
				VectorNormalize(v);
				vectoangles(v, bs->goalAngles);
				MoveTowardIdealAngles(bs);
				trap->EA_Move(bs->client, v, 5000.0f);

				if (bs->noUseTime < (level.time - 400))
				{
					bs->noUseTime = level.time + 500;
				}
			}
		}

		return;
	}
#endif

	if (bot_forgimmick.integer)
	{
		bs->wpCurrent = NULL;
		bs->currentEnemy = NULL;
		bs->wpDestination = NULL;
		bs->wpDirection = 0;

		if (bot_forgimmick.integer == 2)
		{
			//for debugging saber stuff, this is handy
			trap->EA_Attack(bs->client);
		}

		if (bot_forgimmick.integer == 3)
		{
			//for testing cpu usage moving around rmg terrain without AI
			vec3_t mdir;

			VectorSubtract(bs->origin, vec3_origin, mdir);
			VectorNormalize(mdir);
			trap->EA_Attack(bs->client);
			trap->EA_Move(bs->client, mdir, 5000);
		}

		if (bot_forgimmick.integer == 4)
		{
			//constantly move toward client 0
			if (g_entities[0].client && g_entities[0].inuse)
			{
				vec3_t mdir;

				VectorSubtract(g_entities[0].client->ps.origin, bs->origin, mdir);
				VectorNormalize(mdir);
				trap->EA_Move(bs->client, mdir, 5000);
				}
			}

		if (bs->forceMove_Forward)
		{
			if (bs->forceMove_Forward > 0)
			{
				trap->EA_MoveForward(bs->client);
			}
			else
			{
				trap->EA_MoveBack(bs->client);
			}
		}
		if (bs->forceMove_Right)
		{
			if (bs->forceMove_Right > 0)
			{
				trap->EA_MoveRight(bs->client);
			}
			else
			{
				trap->EA_MoveLeft(bs->client);
			}
		}
		if (bs->forceMove_Up)
		{
			trap->EA_Jump(bs->client);
		}
		return;
		}

	if (level.gametype == GT_SIEGE && level.time - level.startTime < 10000)
	{
		//make sure that the bots aren't all on the same team after map changes.
		SelectBestSiegeClass(bs->client, qfalse);
	}

	if (bs->cur_ps.pm_type == PM_INTERMISSION
		|| g_entities[bs->client].client->sess.sessionTeam == TEAM_SPECTATOR)
	{
		//in intermission
		//Mash the button to prevent the game from sticking on one level.
		if (level.gametype == GT_SIEGE)
		{
			//hack to get the bots to spawn into seige games after the game has started
			if (g_entities[bs->client].client->sess.siegeDesiredTeam != SIEGETEAM_TEAM1
				&& g_entities[bs->client].client->sess.siegeDesiredTeam != SIEGETEAM_TEAM2)
			{
				//we're not on a team, go onto the best team available.
				g_entities[bs->client].client->sess.siegeDesiredTeam = PickTeam(bs->client);
			}

			SelectBestSiegeClass(bs->client, qtrue);
		}

		if (!(g_entities[bs->client].client->pers.cmd.buttons & BUTTON_ATTACK))
		{
			//only tap the button if it's not currently being pressed
			trap->EA_Attack(bs->client);
		}
		return;
	}

	if (!bs->lastDeadTime)
	{
		//just spawned in?
		bs->lastDeadTime = level.time;
		bs->MiscBotFlags = 0;
		bs->orderEntity = NULL;
		bs->ordererNum = bs->client;
		VectorClear(bs->DestPosition);
		bs->DestIgnore = -1;
	}

	if (g_entities[bs->client].health < 1 || g_entities[bs->client].client->ps.pm_type == PM_DEAD)
	{
		bs->lastDeadTime = level.time;

		if (!bs->deathActivitiesDone && bs->lastHurt && bs->lastHurt->client && bs->lastHurt->s.number != bs->client)
		{
			BotDeathNotify(bs);
			if (PassLovedOneCheck(bs, bs->lastHurt))
			{
				//CHAT: Died
				bs->chatObject = bs->lastHurt;
				bs->chatAltObject = NULL;
				BotDoChat(bs, "Died", 0);
			}
			else if (!PassLovedOneCheck(bs, bs->lastHurt) &&
				botstates[bs->lastHurt->s.number] &&
				PassLovedOneCheck(botstates[bs->lastHurt->s.number], &g_entities[bs->client]))
			{
				//killed by a bot that I love, but that does not love me
				bs->chatObject = bs->lastHurt;
				bs->chatAltObject = NULL;
				BotDoChat(bs, "KilledOnPurposeByLove", 0);
			}

			bs->deathActivitiesDone = 1;
		}

		bs->wpCurrent = NULL;
		bs->currentEnemy = NULL;
		bs->wpDestination = NULL;
		bs->wpCamping = NULL;
		bs->wpCampingTo = NULL;
		bs->wpStoreDest = NULL;
		bs->wpDestIgnoreTime = 0;
		bs->wpDestSwitchTime = 0;
		bs->wpSeenTime = 0;
		bs->wpDirection = 0;
		VectorClear(bs->lastDestPosition);
		ClearRoute(bs->botRoute);
		bs->tacticEntity = NULL;
		bs->objectiveType = 0;
		bs->MiscBotFlags = 0;

		if (rand() % 10 < 5 &&
			(!bs->doChat || bs->chatTime < level.time))
		{
			trap->EA_Attack(bs->client);
			Cmd_SaberAttackCycle_f(&g_entities[bs->client]);
			bs->changeStyleDebounce = level.time + 10000;
		}

		return;
	}

	if (PM_InLedgeMove(bs->cur_ps.legsAnim))
	{
		//we're in a ledge move, just pull up for now
		trap->EA_MoveForward(bs->client);
		return;
	}

	if (g_entities[bs->client].client->ps.fd.saberAnimLevel == SS_STAFF
		&& !(g_entities[bs->client].client->saber[saberNum].type == SABER_STAFF
			|| g_entities[bs->client].client->saber[saberNum].type == SABER_STAFF_THIN
			|| g_entities[bs->client].client->saber[saberNum].type == SABER_STAFF_UNSTABLE
			|| g_entities[bs->client].client->saber[saberNum].type == SABER_ELECTROSTAFF))
	{
		if (bs->changeStyleDebounce < level.time)
		{
			// Switch stance every so often...
			bs->changeStyleDebounce = level.time + 10000;
			Cmd_SaberAttackCycle_f(&g_entities[bs->client]);
		}
	}

	if (g_entities[bs->client].client->ps.fd.saberAnimLevel == SS_DUAL
		|| g_entities[bs->client].client->ps.fd.saberAnimLevel != SS_MEDIUM)
	{
		if (bs->changeStyleDebounce < level.time)
		{
			// Switch stance every so often...
			bs->changeStyleDebounce = level.time + 10000;
			Cmd_SaberAttackCycle_f(&g_entities[bs->client]);
		}
	}

	if (bs->cur_ps.saberLockTime > level.time)
	{
		//bot is in a saber lock
		//bots cheat by knowing their enemy's fp level, if they're low on fP, try to super break finish them.
		if (g_entities[bs->cur_ps.saberLockEnemy].client->ps.fd.forcePower < 50)
		{
			trap->EA_Attack(bs->client);
		}
		if (g_entities[bs->cur_ps.saberLockEnemy].client->ps.fd.blockPoints < BLOCKPOINTS_HALF)
		{
			trap->EA_Attack(bs->client);
		}
		return;
	}

	VectorCopy(bs->goalAngles, preFrameGAngles);

	bs->doAttack = 0;
	bs->doAltAttack = 0;
	Advanced_ScanforEnemies(bs);

	//determine which tactic we want to use.
	if (CarryingCapObjective(bs))
	{
		//we're carrying the objective, always go into capture mode.
		bs->currentTactic = BOTORDER_OBJECTIVE;
		bs->objectiveType = OT_CAPTURE;
	}
	else
	{
		//otherwise, just pick our tactic based on current situation.
		if (bs->botOrder == BOTORDER_NONE)
		{
			//we don't have a higher level order, use the default for the current situation
			if (bs->currentTactic)
			{
				//already have a tactic, use it.
			}
			else if (level.gametype == GT_SIEGE)
			{
				//hack do objectives
				bs->currentTactic = BOTORDER_OBJECTIVE;
			}
			else if (level.gametype == GT_CTF || level.gametype == GT_CTY)
			{
				DetermineCTFGoal(bs);
			}
			else if (level.gametype == GT_SINGLE_PLAYER)
			{
				gentity_t* player = FindClosestHumanPlayer(bs->origin, NPCTEAM_PLAYER);
				if (player)
				{
					//a player on our team
					bs->currentTactic = BOTORDER_DEFEND;
					bs->tacticEntity = player;
				}
				else
				{
					//just run around and kill enemies
					bs->currentTactic = BOTORDER_SEARCHANDDESTROY;
					bs->tacticEntity = NULL;
				}
			}
			else if (level.gametype == GT_JEDIMASTER)
			{
				bs->currentTactic = BOTORDER_JEDIMASTER;
			}
			else
			{
				if (bs->isSquadLeader)
				{
					CommanderBotAI(bs);
				}
				else
				{
					BotDoTeamplayAI(bs);
				}
			}
		}
		else
		{
			if (bs->isSquadLeader)
			{
				CommanderBotAI(bs);
			}
			else
			{
				BotDoTeamplayAI(bs);
			}
		}
	}

	if (!bs->currentEnemy)
	{
		bs->frame_Enemy_Vis = 0;
	}

	if (bs->revengeEnemy && bs->revengeEnemy->client &&
		bs->revengeEnemy->client->pers.connected != CON_CONNECTED && bs->revengeEnemy->client->pers.connected !=
		CON_CONNECTING)
	{
		bs->revengeEnemy = NULL;
		bs->revengeHateLevel = 0;
	}

	if (bs->currentEnemy && bs->currentEnemy->client &&
		bs->currentEnemy->client->pers.connected != CON_CONNECTED && bs->currentEnemy->client->pers.connected !=
		CON_CONNECTING)
	{
		bs->currentEnemy = NULL;
	}

	fjHalt = 0;

#ifndef FORCEJUMP_INSTANTMETHOD
	if (bs->forceJumpChargeTime > level.time)
	{
		useTheForce = 1;
		forceHostile = 0;
	}

	if (bs->currentEnemy && bs->currentEnemy->client && bs->frame_Enemy_Vis && bs->forceJumpChargeTime < level.time)
#else
	if (bs->currentEnemy && bs->currentEnemy->client && bs->frame_Enemy_Vis)
#endif
	{
		VectorSubtract(bs->currentEnemy->client->ps.origin, bs->eye, a_fo);
		vectoangles(a_fo, a_fo);

		//do this above all things
		if (bs->cur_ps.fd.forcePowersKnown & 1 << FP_PUSH && (bs->doForcePush > level.time || bs->cur_ps.fd.
			forceGripBeingGripped > level.time) && level.clients[bs->client].ps.fd.forcePower > forcePowerNeeded[
				level.clients[bs->client].ps.fd.forcePowerLevel[FP_PUSH]][FP_PUSH])
		{
			level.clients[bs->client].ps.fd.forcePowerSelected = FP_PUSH;
			useTheForce = 1;
			forceHostile = 1;
		}
		else if (bs->cur_ps.fd.forceSide == FORCE_DARKSIDE)
		{
			//try dark side powers
			//in order of priority top to bottom
			if (bs->cur_ps.fd.forcePowersKnown & 1 << FP_GRIP && bs->cur_ps.fd.forcePowersActive & 1 << FP_GRIP
				&& InFieldOfVision(bs->viewangles, 50, a_fo))
			{
				//already gripping someone, so hold it
				level.clients[bs->client].ps.fd.forcePowerSelected = FP_GRIP;
				useTheForce = 1;
				forceHostile = 1;
			}
			else if (bs->cur_ps.fd.forcePowersKnown & 1 << FP_LIGHTNING && bs->frame_Enemy_Len <
				FORCE_LIGHTNING_RADIUS && level.clients[bs->client].ps.fd.forcePower > 50 && InFieldOfVision(
					bs->viewangles, 50, a_fo))
			{
				level.clients[bs->client].ps.fd.forcePowerSelected = FP_LIGHTNING;
				useTheForce = 1;
				forceHostile = 1;
			}
			else if (bs->cur_ps.fd.forcePowersKnown & 1 << FP_GRIP && bs->frame_Enemy_Len < MAX_GRIP_DISTANCE &&
				level.clients[bs->client].ps.fd.forcePower > forcePowerNeeded[level.clients[bs->client].ps.fd.
				forcePowerLevel[FP_GRIP]][FP_GRIP] && InFieldOfVision(bs->viewangles, 50, a_fo))
			{
				level.clients[bs->client].ps.fd.forcePowerSelected = FP_GRIP;
				useTheForce = 1;
				forceHostile = 1;
			}
			else if (bs->cur_ps.fd.forcePowersKnown & 1 << FP_RAGE && g_entities[bs->client].health < 25 && level.
				clients[bs->client].ps.fd.forcePower > forcePowerNeeded[level.clients[bs->client].ps.fd.forcePowerLevel[
					FP_RAGE]][FP_RAGE])
			{
				level.clients[bs->client].ps.fd.forcePowerSelected = FP_RAGE;
				useTheForce = 1;
				forceHostile = 0;
			}
			else if (bs->cur_ps.weapon == WP_BOWCASTER && bs->cur_ps.fd.forcePowersKnown & 1 << FP_RAGE &&
				g_entities[bs->client].health < 75 && level.clients[bs->client].ps.fd.forcePower > forcePowerNeeded[
					level.clients[bs->client].ps.fd.forcePowerLevel[FP_RAGE]][FP_RAGE])
			{
				level.clients[bs->client].ps.fd.forcePowerSelected = FP_RAGE;
				useTheForce = 1;
				forceHostile = 0;
			}
			else if (bs->cur_ps.fd.forcePowersKnown & 1 << FP_DRAIN && bs->frame_Enemy_Len < MAX_DRAIN_DISTANCE &&
				level.clients[bs->client].ps.fd.forcePower > 50 && InFieldOfVision(bs->viewangles, 50, a_fo) && bs->
				currentEnemy->client->ps.fd.forcePower > 10 && bs->currentEnemy->client->ps.fd.forceSide ==
				FORCE_LIGHTSIDE)
			{
				level.clients[bs->client].ps.fd.forcePowerSelected = FP_DRAIN;
				useTheForce = 1;
				forceHostile = 1;
			}
		}
		else if (bs->cur_ps.fd.forceSide == FORCE_LIGHTSIDE)
		{
			//try light side powers
			if (bs->cur_ps.fd.forcePowersKnown & 1 << FP_ABSORB && bs->cur_ps.fd.forceGripCripple &&
				level.clients[bs->client].ps.fd.forcePower > forcePowerNeeded[level.clients[bs->client].ps.fd.
				forcePowerLevel[FP_ABSORB]][FP_ABSORB])
			{
				//absorb to get out
				level.clients[bs->client].ps.fd.forcePowerSelected = FP_ABSORB;
				useTheForce = 1;
				forceHostile = 0;
			}
			else if (bs->cur_ps.fd.forcePowersKnown & 1 << FP_ABSORB && bs->cur_ps.electrifyTime >= level.time &&
				level.clients[bs->client].ps.fd.forcePower > forcePowerNeeded[level.clients[bs->client].ps.fd.
				forcePowerLevel[FP_ABSORB]][FP_ABSORB])
			{
				//absorb lightning
				level.clients[bs->client].ps.fd.forcePowerSelected = FP_ABSORB;
				useTheForce = 1;
				forceHostile = 0;
			}
			else if (bs->cur_ps.fd.forcePowersKnown & 1 << FP_TELEPATHY && bs->frame_Enemy_Len < MAX_TRICK_DISTANCE
				&& level.clients[bs->client].ps.fd.forcePower > forcePowerNeeded[level.clients[bs->client].ps.fd.
				forcePowerLevel[FP_TELEPATHY]][FP_TELEPATHY] && InFieldOfVision(bs->viewangles, 50, a_fo) && !(bs->
					currentEnemy->client->ps.fd.forcePowersActive & 1 << FP_SEE))
			{
				level.clients[bs->client].ps.fd.forcePowerSelected = FP_TELEPATHY;
				useTheForce = 1;
				forceHostile = 1;
			}
			else if (bs->cur_ps.fd.forcePowersKnown & 1 << FP_ABSORB && g_entities[bs->client].health < 75 && bs->
				currentEnemy->client->ps.fd.forceSide == FORCE_DARKSIDE && level.clients[bs->client].ps.fd.forcePower >
				forcePowerNeeded[level.clients[bs->client].ps.fd.forcePowerLevel[FP_ABSORB]][FP_ABSORB])
			{
				level.clients[bs->client].ps.fd.forcePowerSelected = FP_ABSORB;
				useTheForce = 1;
				forceHostile = 0;
			}
			else if (bs->cur_ps.fd.forcePowersKnown & 1 << FP_PROTECT && g_entities[bs->client].health < 35 && level
				.clients[bs->client].ps.fd.forcePower > forcePowerNeeded[level.clients[bs->client].ps.fd.forcePowerLevel
				[FP_PROTECT]][FP_PROTECT])
			{
				level.clients[bs->client].ps.fd.forcePowerSelected = FP_PROTECT;
				useTheForce = 1;
				forceHostile = 0;
			}
		}
		else if (bs->cur_ps.saberInFlight && !bs->cur_ps.saberEntityNum)
		{
			//we've lost our saber.
			//check to see if we can get the saber back yet.
			if (g_entities[g_entities[bs->client].client->saberStoredIndex].s.pos.trType == TR_STATIONARY)
			{
				//saber is ready to be pulled back
				level.clients[bs->client].ps.fd.forcePowerSelected = FP_SABERTHROW;
				useTheForce = 1;
				forceHostile = 0;
			}
		}

		if (!useTheForce)
		{
			//try neutral powers
			if (bs->cur_ps.fd.forcePowersKnown & 1 << FP_PUSH && bs->cur_ps.fd.forceGripBeingGripped > level.time &&
				level.clients[bs->client].ps.fd.forcePower > forcePowerNeeded[level.clients[bs->client].ps.fd.
				forcePowerLevel[FP_PUSH]][FP_PUSH] && InFieldOfVision(bs->viewangles, 50, a_fo))
			{
				level.clients[bs->client].ps.fd.forcePowerSelected = FP_PUSH;
				useTheForce = 1;
				forceHostile = 1;
			}
			else if (bs->cur_ps.fd.forcePowersKnown & 1 << FP_SPEED && g_entities[bs->client].health < 25 && level.
				clients[bs->client].ps.fd.forcePower > forcePowerNeeded[level.clients[bs->client].ps.fd.forcePowerLevel[
					FP_SPEED]][FP_SPEED])
			{
				level.clients[bs->client].ps.fd.forcePowerSelected = FP_SPEED;
				useTheForce = 1;
				forceHostile = 0;
			}
			else if (bs->cur_ps.fd.forcePowersKnown & 1 << FP_SEE &&
				BotMindTricked(bs->client, bs->currentEnemy->s.number) && level.clients[bs->client].ps.fd.forcePower >
				forcePowerNeeded[level.clients[bs->client].ps.fd.forcePowerLevel[FP_SEE]][FP_SEE])
			{
				level.clients[bs->client].ps.fd.forcePowerSelected = FP_SEE;
				useTheForce = 1;
				forceHostile = 0;
			}
			else
			{
				if (rand() % 10 < 5 && bs->doForcePushPullSpamTime > level.time)
				{
					if (bs->cur_ps.fd.forcePowersKnown & 1 << FP_PULL && bs->frame_Enemy_Len < 256 && level.clients[
						bs->client].ps.fd.forcePower > 75 && InFieldOfVision(bs->viewangles, 50, a_fo))
					{
						level.clients[bs->client].ps.fd.forcePowerSelected = FP_PULL;
						useTheForce = 1;
						forceHostile = 1;
					}

						bs->doForcePushPullSpamTime = level.time + 500;
				}
				else
				{
					if (bs->cur_ps.fd.forcePowersKnown & 1 << FP_PUSH && bs->frame_Enemy_Len < 256 && level.clients[
						bs->client].ps.fd.forcePower > 75 && InFieldOfVision(bs->viewangles, 50, a_fo))
					{
						level.clients[bs->client].ps.fd.forcePowerSelected = FP_PUSH;
						useTheForce = 1;
						forceHostile = 1;
					}

						bs->doForcePushPullSpamTime = level.time + 500;
				}
			}
		}
	}

	if (!useTheForce)
	{
		//try powers that we don't care if we have an enemy for
		if (bs->cur_ps.fd.forcePowersKnown & 1 << FP_HEAL && g_entities[bs->client].health < 50 && level.clients[bs
			->client].ps.fd.forcePower > forcePowerNeeded[level.clients[bs->client].ps.fd.forcePowerLevel[FP_HEAL]][
				FP_HEAL] && bs->cur_ps.fd.forcePowerLevel[FP_HEAL] > FORCE_LEVEL_1)
		{
			level.clients[bs->client].ps.fd.forcePowerSelected = FP_HEAL;
			useTheForce = 1;
			forceHostile = 0;
		}
		else if (bs->cur_ps.fd.forcePowersKnown & 1 << FP_HEAL && g_entities[bs->client].health < 50 && level.
			clients[bs->client].ps.fd.forcePower > forcePowerNeeded[level.clients[bs->client].ps.fd.forcePowerLevel[
				FP_HEAL]][FP_HEAL] && !bs->currentEnemy && bs->isCamping > level.time)
		{
			//only meditate and heal if we're camping
			level.clients[bs->client].ps.fd.forcePowerSelected = FP_HEAL;
			useTheForce = 1;
			forceHostile = 0;
		}
	}

	if (useTheForce && forceHostile)
	{
		if (bs->currentEnemy && bs->currentEnemy->client &&
			!ForcePowerUsableOn(&g_entities[bs->client], bs->currentEnemy,
				level.clients[bs->client].ps.fd.forcePowerSelected))
		{
			useTheForce = 0;
			forceHostile = 0;
		}
	}

	doingFallback = 0;

	bs->deathActivitiesDone = 0;

	if (BotUseInventoryItem(bs))
	{
		if (rand() % 10 < 5)
		{
			trap->EA_Use(bs->client);
		}
	}

	if (g_AllowBotBuyItem.integer)
	{
		if (g_entities[bs->client].health <= 50)
		{
			bot_buy_item(bs, "health");
		}
		else if (bs->cur_ps.stats[STAT_ARMOR] <= 50)
		{
			bot_buy_item(bs, "shield");
		}
		else
		{
			switch (rand() % 35)
			{
			case 5: bot_buy_item(bs, "seeker");
			case 10: bot_buy_item(bs, "blaster");
			case 15: bot_buy_item(bs, "launcher");
			case 16: bot_buy_item(bs, "concussion");
			case 20: bot_buy_item(bs, "bowcaster");
			default:
				bot_buy_item(bs, "sentry");
			}
		}
	}

	if (bs->cur_ps.ammo[weaponData[bs->cur_ps.weapon].ammoIndex] < weaponData[bs->cur_ps.weapon].energyPerShot)
	{
		if (g_AllowBotBuyItem.integer)
		{
			if (bot_buy_item(bs, "ammo"))
			{
				return;
			}
		}
		else
		{
			if (BotTryAnotherWeapon(bs))
			{
				return;
			}
		}
	}
	else
	{
		int sel_result = 0;
		if (bs->currentEnemy && bs->lastVisibleEnemyIndex == bs->currentEnemy->s.number &&
			bs->frame_Enemy_Vis && bs->forceWeaponSelect)
		{
			bs->forceWeaponSelect = 0;
		}

		if (bs->plantContinue > level.time)
		{
			bs->doAttack = 1;
			bs->destinationGrabTime = 0;
		}

		if (!bs->forceWeaponSelect && bs->cur_ps.hasDetPackPlanted && bs->plantKillEmAll > level.time)
		{
			bs->forceWeaponSelect = WP_DET_PACK;
		}

		if (bs->forceWeaponSelect)
		{
			sel_result = BotSelectChoiceWeapon(bs, bs->forceWeaponSelect, 1);
		}

		if (sel_result)
		{
			if (sel_result == 2)
			{
				//newly selected
				return;
			}
		}
		else if (BotSelectIdealWeapon(bs))
		{
			return;
		}
	}

	reaction = bs->skills.reflex / bs->settings.skill;

	if (reaction < 0)
	{
		reaction = 0;
	}
	if (reaction > 2000)
	{
		reaction = 2000;
	}

	if (!bs->currentEnemy)
	{
		bs->timeToReact = level.time + reaction;
	}

	if (bs->cur_ps.weapon == WP_DET_PACK && bs->cur_ps.hasDetPackPlanted && bs->plantKillEmAll > level.time)
	{
		bs->doAltAttack = 1;
	}

	if (bs->wpCamping)
	{
		if (bs->isCamping < level.time)
		{
			bs->wpCamping = NULL;
			bs->isCamping = 0;
		}

		if (bs->currentEnemy && bs->frame_Enemy_Vis)
		{
			bs->wpCamping = NULL;
			bs->isCamping = 0;
		}
	}

	if (bs->wpCurrent &&
		(bs->wpSeenTime < level.time || bs->wpTravelTime < level.time))
	{
		bs->wpCurrent = NULL;
	}

	if (bs->currentEnemy)
	{
		if (bs->enemySeenTime < level.time ||
			!PassStandardEnemyChecks(bs, bs->currentEnemy))
		{
			if (bs->revengeEnemy == bs->currentEnemy &&
				bs->currentEnemy->health < 1 &&
				bs->lastAttacked && bs->lastAttacked == bs->currentEnemy)
			{
				bs->chatObject = bs->revengeEnemy;
				bs->chatAltObject = NULL;
				BotDoChat(bs, "KilledHatedOne", 1);
				bs->revengeEnemy = NULL;
				bs->revengeHateLevel = 0;
			}
			else if (bs->currentEnemy->health < 1 && PassLovedOneCheck(bs, bs->currentEnemy) &&
				bs->lastAttacked && bs->lastAttacked == bs->currentEnemy)
			{
				bs->chatObject = bs->currentEnemy;
				bs->chatAltObject = NULL;
				BotDoChat(bs, "Killed", 0);
			}

			bs->currentEnemy = NULL;
		}
	}

	if (bot_honorableduelacceptance.integer)
	{
		if (bs->currentEnemy && bs->currentEnemy->client &&
			g_privateDuel.integer &&
			bs->frame_Enemy_Vis &&
			bs->frame_Enemy_Len < 400)

		{
			vec3_t e_ang_vec;

			VectorSubtract(bs->currentEnemy->client->ps.origin, bs->eye, e_ang_vec);

			if (InFieldOfVision(bs->viewangles, 100, e_ang_vec))
			{
				if (bs->currentEnemy->client->ps.duelIndex == bs->client &&
					bs->currentEnemy->client->ps.duelTime > level.time &&
					!bs->cur_ps.duelInProgress)
				{
					Cmd_EngageDuel_f(&g_entities[bs->client]);
				}

				bs->doAttack = 0;
				bs->doAltAttack = 0;
				bs->doBotKick = 0;
				bs->botChallengingTime = level.time + 100;
			}
		}
	}

	if (!bs->wpCurrent)
	{
		int wp;
		wp = GetNearestVisibleWP(bs->origin, bs->client);

		if (wp != -1)
		{
			bs->wpCurrent = gWPArray[wp];
			bs->wpSeenTime = level.time + 1500;
			bs->wpTravelTime = level.time + 10000; //never take more than 10 seconds to travel to a waypoint
		}
	}

	if (bs->enemySeenTime < level.time || !bs->frame_Enemy_Vis || !bs->currentEnemy ||
		bs->currentEnemy)
	{
		int enemy;
		enemy = ScanForEnemies(bs);

		if (enemy != -1)
		{
			bs->currentEnemy = &g_entities[enemy];
			bs->enemySeenTime = level.time + ENEMY_FORGET_MS;
		}
	}

	if (!bs->squadLeader && !bs->isSquadLeader)
	{
		BotScanForLeader(bs);
	}

	if (!bs->squadLeader && bs->squadCannotLead < level.time)
	{
		//if still no leader after scanning, then become a squad leader
		bs->isSquadLeader = 1;
	}

	if (bs->isSquadLeader && bs->squadLeader)
	{
		//we don't follow anyone if we are a leader
		bs->squadLeader = NULL;
	}

	//ESTABLISH VISIBILITIES AND DISTANCES FOR THE WHOLE FRAME HERE
	if (bs->wpCurrent)
	{
		int vis_result = 0;
		if (RMG.integer)
		{
			//this is somewhat hacky, but in RMG we don't really care about vertical placement because points are scattered across only the terrain.
			vec3_t vecB, vecC;

			vecB[0] = bs->origin[0];
			vecB[1] = bs->origin[1];
			vecB[2] = bs->origin[2];

			vecC[0] = bs->wpCurrent->origin[0];
			vecC[1] = bs->wpCurrent->origin[1];
			vecC[2] = vecB[2];

			VectorSubtract(vecC, vecB, a);
		}
		else
		{
			VectorSubtract(bs->wpCurrent->origin, bs->origin, a);
		}
		bs->frame_Waypoint_Len = VectorLength(a);

		vis_result = WPOrgVisible(&g_entities[bs->client], bs->origin, bs->wpCurrent->origin, bs->client);

		if (vis_result == 2)
		{
			bs->frame_Waypoint_Vis = 0;
			bs->wpSeenTime = 0;
			bs->wpDestination = NULL;
			bs->wpDestIgnoreTime = level.time + 5000;

			if (bs->wpDirection)
			{
				bs->wpDirection = 0;
			}
			else
			{
				bs->wpDirection = 1;
			}
		}
		else if (vis_result)
		{
			bs->frame_Waypoint_Vis = 1;
		}
		else
		{
			bs->frame_Waypoint_Vis = 0;
		}
	}

	if (bs->currentEnemy)
	{
		vec3_t eorg;
		if (bs->currentEnemy->client)
		{
			VectorCopy(bs->currentEnemy->client->ps.origin, eorg);
			eorg[2] += bs->currentEnemy->client->ps.viewheight;
		}
		else
		{
			VectorCopy(bs->currentEnemy->s.origin, eorg);
		}

		VectorSubtract(eorg, bs->eye, a);
		bs->frame_Enemy_Len = VectorLength(a);

		if (OrgVisible(bs->eye, eorg, bs->client))
		{
			bs->frame_Enemy_Vis = 1;
			VectorCopy(eorg, bs->lastEnemySpotted);
			VectorCopy(bs->origin, bs->hereWhenSpotted);
			bs->lastVisibleEnemyIndex = bs->currentEnemy->s.number;
			bs->hitSpotted = 0;
		}
		else
		{
			bs->frame_Enemy_Vis = 0;
		}
	}
	else
	{
		bs->lastVisibleEnemyIndex = ENTITYNUM_NONE;
	}
	//END

	if (bs->frame_Enemy_Vis)
	{
		bs->enemySeenTime = level.time + ENEMY_FORGET_MS;
	}

	if (bs->wpCurrent)
	{
		int goalWPIndex;
		int wpTouchDist = BOT_WPTOUCH_DISTANCE;
		WPConstantRoutine(bs);

		if (!bs->wpCurrent)
		{
			//WPConstantRoutine has the ability to nullify the waypoint if it fails certain checks, so..
			return;
		}

		if (bs->wpCurrent->flags & WPFLAG_WAITFORFUNC)
		{
			if (!CheckForFunc(bs->wpCurrent->origin, -1))
			{
				bs->beStill = level.time + 500; //no func brush under.. wait
			}
		}
		if (bs->wpCurrent->flags & WPFLAG_NOMOVEFUNC)
		{
			if (CheckForFunc(bs->wpCurrent->origin, -1))
			{
				bs->beStill = level.time + 500; //func brush under.. wait
			}
		}

		if (bs->frame_Waypoint_Vis || bs->wpCurrent->flags & WPFLAG_NOVIS)
		{
			if (RMG.integer)
			{
				bs->wpSeenTime = level.time + 5000;
				//if we lose sight of the point, we have 1.5 seconds to regain it before we drop it
			}
			else
			{
				bs->wpSeenTime = level.time + 1500;
				//if we lose sight of the point, we have 1.5 seconds to regain it before we drop it
			}
		}
		VectorCopy(bs->wpCurrent->origin, bs->goalPosition);
		if (bs->wpDirection)
		{
			goalWPIndex = bs->wpCurrent->index - 1;
		}
		else
		{
			goalWPIndex = bs->wpCurrent->index + 1;
		}

		if (bs->wpCamping)
		{
			VectorSubtract(bs->wpCampingTo->origin, bs->origin, a);
			vectoangles(a, ang);
			VectorCopy(ang, bs->goalAngles);

			VectorSubtract(bs->origin, bs->wpCamping->origin, a);
			if (VectorLength(a) < 64)
			{
				VectorCopy(bs->wpCamping->origin, bs->goalPosition);
				bs->beStill = level.time + 1000;

				if (!bs->campStanding)
				{
					bs->duckTime = level.time + 1000;
				}
			}
		}
		else if (gWPArray[goalWPIndex] && gWPArray[goalWPIndex]->inuse &&
			!(gLevelFlags & LEVELFLAG_NOPOINTPREDICTION))
		{
			VectorSubtract(gWPArray[goalWPIndex]->origin, bs->origin, a);
			vectoangles(a, ang);
			VectorCopy(ang, bs->goalAngles);
		}
		else
		{
			VectorSubtract(bs->wpCurrent->origin, bs->origin, a);
			vectoangles(a, ang);
			VectorCopy(ang, bs->goalAngles);
		}

		if (bs->destinationGrabTime < level.time)
		{
			GetIdealDestination(bs);
		}

		if (bs->wpCurrent && bs->wpDestination)
		{
			if (TotalTrailDistance(bs->wpCurrent->index, bs->wpDestination->index, bs) == -1)
			{
				bs->wpDestination = NULL;
				bs->destinationGrabTime = level.time + 10000;
			}
		}

		if (RMG.integer)
		{
			if (bs->frame_Waypoint_Vis)
			{
				if (bs->wpCurrent && !bs->wpCurrent->flags)
				{
					wpTouchDist *= 3;
				}
			}
		}

		if (bs->frame_Waypoint_Len < wpTouchDist || RMG.integer && bs->frame_Waypoint_Len < wpTouchDist * 2)
		{
			int desiredIndex;
			WPTouchRoutine(bs);

			if (!bs->wpDirection)
			{
				desiredIndex = bs->wpCurrent->index + 1;
			}
			else
			{
				desiredIndex = bs->wpCurrent->index - 1;
			}

			if (gWPArray[desiredIndex] &&
				gWPArray[desiredIndex]->inuse &&
				desiredIndex < gWPNum &&
				desiredIndex >= 0 &&
				PassWayCheck(bs, desiredIndex))
			{
				bs->wpCurrent = gWPArray[desiredIndex];
			}
			else
			{
				if (bs->wpDestination)
				{
					bs->wpDestination = NULL;
					bs->destinationGrabTime = level.time + 10000;
				}

				if (bs->wpDirection)
				{
					bs->wpDirection = 0;
				}
				else
				{
					bs->wpDirection = 1;
				}
			}
		}
	}
	else //We can't find a waypoint, going to need a fallback routine.
	{
		if (bs->cur_ps.weapon == WP_SABER)
		{
			doingFallback = Saber_BotFallbackNavigation(bs);
		}
		else
		{
			if (bs->cur_ps.stats[STAT_HOLDABLE_ITEMS] & 1 << HI_JETPACK)
			{
				// Jetpacker.. Jetpack ON
				bs->jumpTime = level.time + 1500;
				bs->jDelay = 0;
				bs->cur_ps.eFlags |= EF_JETPACK_ACTIVE;
				bs->cur_ps.eFlags |= EF_JETPACK_FLAMING; //going up
				bs->jumpHoldTime = (bs->forceJumpChargeTime + level.time) / 2 + 50000;
			}
			doingFallback = Gunner_BotFallbackNavigation(bs);
		}
	}

	if (bs->currentEnemy && bs->entitynum < MAX_CLIENTS)
	{
		if (g_entities[bs->entitynum].health <= 0 || bs->currentEnemy->health <= 0)
		{
			//dont do it if there dead...doh
		}
		else
		{
			vec3_t dir;
			float xy;
			qboolean willFall = qfalse;

			if (next_bot_fallcheck[bs->entitynum] < level.time)
			{
				vec3_t fwd;
				if (CheckFall_By_Vectors(bs->origin, fwd, &g_entities[bs->entitynum]) == qtrue)
				{
					willFall = qtrue;
				}
				if (bot_will_fall[bs->entitynum])
				{
					next_bot_fallcheck[bs->entitynum] = level.time + 50;
					bot_will_fall[bs->entitynum] = willFall;
				}
			}
			else
			{
				vec3_t p2;
				vec3_t p1;
				willFall = bot_will_fall[bs->entitynum];
				if (bs->origin[2] > bs->currentEnemy->r.currentOrigin[2])
				{
					VectorCopy(bs->origin, p1);
					VectorCopy(bs->currentEnemy->r.currentOrigin, p2);
				}
				else if (bs->origin[2] < bs->currentEnemy->r.currentOrigin[2])
				{
					VectorCopy(bs->currentEnemy->r.currentOrigin, p1);
					VectorCopy(bs->origin, p2);
				}
				else
				{
					VectorCopy(bs->origin, p1);
					VectorCopy(bs->currentEnemy->r.currentOrigin, p2);
				}
				VectorSubtract(p2, p1, dir);
				dir[2] = 0;
			}
			//Get xy and z diffs
			xy = VectorNormalize(dir);

			// Jumping Stuff.
			if (bs->cur_ps.weapon == WP_SABER
				&& bs->BOTjumpState <= JS_WAITING // Not in a jump right now.
				&& xy < 300
				&& bs->currentEnemy->r.currentOrigin[2] > bs->origin[2] + 32
				&& bs->currentEnemy->health > 0
				&& bs->currentEnemy->client
				&& bs->currentEnemy->client->ps.groundEntityNum != ENTITYNUM_NONE)
			{
				// Jump to enemy. NPC style! They are above us. Do a Jump.
				bs->BOTjumpState = JS_FACING;
				AIMod_Jump(bs);

				VectorCopy(bs->currentEnemy->r.currentOrigin, jumpPos[bs->cur_ps.clientNum]);
			}
			else if (bs->cur_ps.weapon == WP_SABER
				&& bs->BOTjumpState <= JS_WAITING // Not in a jump right now.
				&& xy > 1000
				&& bs->currentEnemy->r.currentOrigin[2] < bs->origin[2] - 32
				&& bs->currentEnemy->health > 0
				&& bs->currentEnemy->client
				&& bs->currentEnemy->client->ps.groundEntityNum != ENTITYNUM_NONE)
			{
				// Jump to enemy. NPC style! They are below us.
				bs->BOTjumpState = JS_FACING;
				AIMod_Jump(bs);

				VectorCopy(bs->currentEnemy->r.currentOrigin, jumpPos[bs->cur_ps.clientNum]);
			}
			else if (bs->cur_ps.stats[STAT_HOLDABLE_ITEMS] & 1 << HI_JETPACK
				&& bs->BOTjumpState <= JS_WAITING // Not in a jump right now.
				&& xy < 300
				&& bs->currentEnemy->r.currentOrigin[2] > bs->origin[2] + 32
				&& bs->currentEnemy->health > 0
				&& bs->currentEnemy->client
				&& bs->currentEnemy->client->ps.groundEntityNum != ENTITYNUM_NONE)
			{
				// Jump to enemy. NPC style! They are above us. Do a Jump.
				bs->BOTjumpState = JS_FACING;
				AIMod_Jump(bs);

				VectorCopy(bs->currentEnemy->r.currentOrigin, jumpPos[bs->cur_ps.clientNum]);
			}
			else if (bs->cur_ps.stats[STAT_HOLDABLE_ITEMS] & 1 << HI_JETPACK
				&& bs->BOTjumpState <= JS_WAITING // Not in a jump right now.
				&& xy > 1400
				&& bs->currentEnemy->r.currentOrigin[2] < bs->origin[2] - 32
				&& bs->currentEnemy->health > 0
				&& bs->currentEnemy->client
				&& bs->currentEnemy->client->ps.groundEntityNum != ENTITYNUM_NONE)
			{
				// Jump to enemy. NPC style! They are below us.
				bs->BOTjumpState = JS_FACING;
				AIMod_Jump(bs);

				VectorCopy(bs->currentEnemy->r.currentOrigin, jumpPos[bs->cur_ps.clientNum]);
			}
			else if (bs->BOTjumpState >= JS_CROUCHING)
			{
				// Continue any jumps.
				AIMod_Jump(bs);
			}
		}
	}

	if (!VectorCompare(vec3_origin, moveDir))
	{
		trap->EA_Move(bs->client, moveDir, 5000);
	}

	if (RMG.integer)
	{
		//for RMG if the bot sticks around an area too long, jump around randomly some to spread to a new area (horrible hacky method)
		vec3_t vSubDif;

		VectorSubtract(bs->origin, bs->lastSignificantAreaChange, vSubDif);
		if (VectorLength(vSubDif) > 1500)
		{
			VectorCopy(bs->origin, bs->lastSignificantAreaChange);
			bs->lastSignificantChangeTime = level.time + 20000;
		}

		if (bs->lastSignificantChangeTime < level.time)
		{
			bs->iHaveNoIdeaWhereIAmGoing = level.time + 17000;
		}
	}

	if (bs->iHaveNoIdeaWhereIAmGoing > level.time && !bs->currentEnemy)
	{
		VectorCopy(preFrameGAngles, bs->goalAngles);
		bs->wpCurrent = NULL;
		bs->wpSwitchTime = level.time + 150;
		if (bs->cur_ps.weapon == WP_SABER)
		{
			doingFallback = Saber_BotFallbackNavigation(bs);
		}
		else
		{
			if (bs->cur_ps.stats[STAT_HOLDABLE_ITEMS] & 1 << HI_JETPACK)
			{
				// Jetpacker.. Jetpack ON
				bs->jumpTime = level.time + 1500;
				bs->jDelay = 0;
				bs->cur_ps.eFlags |= EF_JETPACK_ACTIVE;
				bs->cur_ps.eFlags |= EF_JETPACK_FLAMING; //going up
				bs->jumpHoldTime = (bs->forceJumpChargeTime + level.time) / 2 + 50000;
			}
			doingFallback = Gunner_BotFallbackNavigation(bs);
		}
		bs->jumpTime = level.time + 150;
		bs->jumpHoldTime = level.time + 150;
		bs->jDelay = 0;
		bs->lastSignificantChangeTime = level.time + 25000;
	}

	if (bs->wpCurrent && RMG.integer)
	{
		qboolean do_j = qfalse;

		if (bs->wpCurrent->origin[2] - 192 > bs->origin[2])
		{
			do_j = qtrue;
		}
		else if (bs->wpTravelTime - level.time < 5000 && bs->wpCurrent->origin[2] - 64 > bs->origin[2])
		{
			do_j = qtrue;
		}
		else if (bs->wpTravelTime - level.time < 7000 && bs->wpCurrent->flags & WPFLAG_RED_FLAG)
		{
			if (level.time - bs->jumpTime > 200)
			{
				bs->jumpTime = level.time + 100;
				bs->jumpHoldTime = level.time + 100;
				bs->jDelay = 0;
			}
		}
		else if (bs->wpTravelTime - level.time < 7000 && bs->wpCurrent->flags & WPFLAG_BLUE_FLAG)
		{
			if (level.time - bs->jumpTime > 200)
			{
				bs->jumpTime = level.time + 100;
				bs->jumpHoldTime = level.time + 100;
				bs->jDelay = 0;
			}
		}
		else if (bs->wpCurrent->index > 0)
		{
			if (bs->wpTravelTime - level.time < 7000)
			{
				if (gWPArray[bs->wpCurrent->index - 1]->flags & WPFLAG_RED_FLAG ||
					gWPArray[bs->wpCurrent->index - 1]->flags & WPFLAG_BLUE_FLAG)
				{
					if (level.time - bs->jumpTime > 200)
					{
						bs->jumpTime = level.time + 100;
						bs->jumpHoldTime = level.time + 100;
						bs->jDelay = 0;
					}
				}
			}
		}

		if (do_j)
		{
			bs->jumpTime = level.time + 1500;
			bs->jumpHoldTime = level.time + 1500;
			bs->jDelay = 0;
		}
	}

	if (doingFallback)
	{
		bs->doingFallback = qtrue;
	}
	else
	{
		bs->doingFallback = qfalse;
	}

	if (bs->timeToReact < level.time && bs->currentEnemy && bs->enemySeenTime > level.time + (ENEMY_FORGET_MS -
		ENEMY_FORGET_MS * 0.2))
	{
		vec3_t headlevel;
		if (bs->frame_Enemy_Vis)
		{
			CombatBotAI(bs, thinktime);
		}
		else if (bs->cur_ps.weaponstate == WEAPON_CHARGING_ALT)
		{
			//keep charging in case we see him again before we lose track of him
			bs->doAltAttack = 1;
		}
		else if (bs->cur_ps.weaponstate == WEAPON_CHARGING)
		{
			//keep charging in case we see him again before we lose track of him
			bs->doAttack = 1;
		}

		if (bs->destinationGrabTime > level.time + 100)
		{
			bs->destinationGrabTime = level.time + 100;
			//assures that we will continue staying within a general area of where we want to be in a combat situation
		}

		if (bs->currentEnemy->client)
		{
			VectorCopy(bs->currentEnemy->client->ps.origin, headlevel);
			headlevel[2] += bs->currentEnemy->client->ps.viewheight;
		}
		else
		{
			VectorCopy(bs->currentEnemy->client->ps.origin, headlevel);
		}

		if (!bs->frame_Enemy_Vis)
		{
			if (OrgVisible(bs->eye, bs->lastEnemySpotted, -1))
			{
				VectorCopy(bs->lastEnemySpotted, headlevel);
				VectorSubtract(headlevel, bs->eye, a);
				vectoangles(a, ang);
				VectorCopy(ang, bs->goalAngles);

				if (bs->cur_ps.weapon == WP_FLECHETTE &&
					bs->cur_ps.weaponstate == WEAPON_READY &&
					bs->currentEnemy && bs->currentEnemy->client)
				{
					float mLen;
					mLen = VectorLength(a) > 128;
					if (mLen > 128 && mLen < 1024)
					{
						VectorSubtract(bs->currentEnemy->client->ps.origin, bs->lastEnemySpotted, a);

						if (VectorLength(a) < 300)
						{
							bs->doAltAttack = 1;
						}
					}
				}
			}
		}
		else
		{
			float bLeadAmount;
			bLeadAmount = BotWeaponCanLead(bs);
			if (bs->skills.accuracy / bs->settings.skill <= 8 &&
				bLeadAmount)
			{
				BotAimLeading(bs, headlevel, bLeadAmount);
			}
			else
			{
				VectorSubtract(headlevel, bs->eye, a);
				vectoangles(a, ang);
				VectorCopy(ang, bs->goalAngles);
			}

			BotAimOffsetGoalAngles(bs);
		}
	}

	if (bs->cur_ps.saberInFlight)
	{
		bs->saberThrowTime = level.time + Q_irand(4000, 10000);
	}

	if (bs->currentEnemy)
	{
		if (BotGetWeaponRange(bs) == BWEAPONRANGE_SABER)
		{
			int saberRange = SABER_ATTACK_RANGE;

			VectorSubtract(bs->currentEnemy->client->ps.origin, bs->eye, a_fo);
			vectoangles(a_fo, a_fo);

			if (bs->saberPowerTime < level.time)
			{
				//Don't just use strong attacks constantly, switch around a bit
				if (Q_irand(1, 10) <= 5)
				{
					bs->saberPower = qtrue;
				}
				else
				{
					bs->saberPower = qfalse;
				}

				bs->saberPowerTime = level.time + Q_irand(3000, 15000);
			}

			if (bs->currentEnemy->health > 40
				&& g_entities[bs->client].client->ps.fd.forcePowerLevel[FP_SABER_OFFENSE] > 1)
			{
				if (g_entities[bs->client].client->ps.fd.saberAnimLevel != SS_MEDIUM)
				{
					//they're down on health a little, use level 2 if we can
					Cmd_SaberAttackCycle_f(&g_entities[bs->client]);
				}
			}
			else
			{
				if (g_entities[bs->client].client->ps.fd.saberAnimLevel != SS_FAST)
				{
					//they've gone below 40 health, go at them with quick attacks
					Cmd_SaberAttackCycle_f(&g_entities[bs->client]);
				}
			}

			if (level.gametype == GT_SINGLE_PLAYER)
			{
				saberRange *= 3;
			}

			if (bs->frame_Enemy_Len <= saberRange)
			{
				SaberCombatHandling(bs);

				if (bs->frame_Enemy_Len < 80)
				{
					meleestrafe = 1;
				}
			}
			else if (bs->saberThrowTime < level.time && !bs->cur_ps.saberInFlight &&
				bs->cur_ps.fd.forcePowersKnown & 1 << FP_SABERTHROW &&
				InFieldOfVision(bs->viewangles, 30, a_fo) &&
				(bs->frame_Enemy_Len > 512 && bs->frame_Enemy_Len < BOT_SABER_THROW_RANGE))
			{
				bs->doAltAttack = 1;
				bs->doAttack = 0;
			}
			else if (bs->cur_ps.saberInFlight && bs->frame_Enemy_Len > 300 && bs->frame_Enemy_Len <
				BOT_SABER_THROW_RANGE)
			{
				bs->doAltAttack = 1;
				bs->doAttack = 0;
			}
		}
		else if (BotGetWeaponRange(bs) == BWEAPONRANGE_MELEE)
		{
			if (bs->frame_Enemy_Len <= MELEE_ATTACK_RANGE)
			{
				MeleeCombatHandling(bs);
				meleestrafe = 1;
			}
		}
	}

	if (doingFallback && bs->currentEnemy) //just stand and fire if we have no idea where we are
	{
		VectorCopy(bs->origin, bs->goalPosition);
	}

	if (bs->forceJumping > level.time)
	{
		vec3_t noz_y;
		vec3_t noz_x;
		VectorCopy(bs->origin, noz_x);
		VectorCopy(bs->goalPosition, noz_y);

		noz_x[2] = noz_y[2];

		VectorSubtract(noz_x, noz_y, noz_x);

		if (VectorLength(noz_x) < 32)
		{
			fjHalt = 1;
		}
	}

	if (bs->cur_ps.lastOnGround + 300 < level.time //haven't been on the ground for a while
		&& (!bs->cur_ps.fd.forceJumpZStart || bs->origin[2] < bs->cur_ps.fd.forceJumpZStart))
		//and not safely landing from a jump
	{
		//been off the ground for a little while
		float speed = VectorLength(bs->cur_ps.velocity);
		if (speed >= 100 + g_entities[bs->client].health && bs->virtualWeapon != WP_SABER || speed >= 700)
		{
			//moving fast enough to get hurt get ready to crouch roll
			if (bs->virtualWeapon != WP_SABER)
			{
				//try switching to saber
				if (!BotSelectChoiceWeapon(bs, WP_SABER, 1))
				{
					//ok, try switching to melee
					BotSelectChoiceWeapon(bs, WP_MELEE, 1);
				}
			}

			if (bs->virtualWeapon == WP_MELEE || bs->virtualWeapon == WP_SABER)
			{
				//in or switching to a weapon that allows us to do roll landings
				bs->duckTime = level.time + 300;
				if (!bs->lastucmd.forwardmove && !bs->lastucmd.rightmove)
				{
					//not trying to move at all so we should at least attempt to move
					trap->EA_MoveForward(bs->client);
				}
			}
		}
	}

	if (bs->doChat && bs->chatTime > level.time && (!bs->currentEnemy || !bs->frame_Enemy_Vis))
	{
		return;
	}
	if (bs->doChat && bs->currentEnemy && bs->frame_Enemy_Vis)
	{
		bs->doChat = 0; //do we want to keep the bot waiting to chat until after the enemy is gone?
		bs->chatTeam = 0;
	}
	else if (bs->doChat && bs->chatTime <= level.time)
	{
		if (bs->chatTeam)
		{
			trap->EA_SayTeam(bs->client, bs->currentChat);
			bs->chatTeam = 0;
		}
		else
		{
			trap->EA_Say(bs->client, bs->currentChat);
		}
		if (bs->doChat == 2)
		{
			BotReplyGreetings(bs);
		}
		bs->doChat = 0;
	}

	CTFFlagMovement(bs);

	if (bs->shootGoal &&
		bs->shootGoal->health > 0 && bs->shootGoal->takedamage)
	{
		vec3_t dif;
		dif[0] = (bs->shootGoal->r.absmax[0] + bs->shootGoal->r.absmin[0]) / 2;
		dif[1] = (bs->shootGoal->r.absmax[1] + bs->shootGoal->r.absmin[1]) / 2;
		dif[2] = (bs->shootGoal->r.absmax[2] + bs->shootGoal->r.absmin[2]) / 2;

		if (!bs->currentEnemy || bs->frame_Enemy_Len > 256)
		{
			//if someone is close then don't stop shooting them for this
			VectorSubtract(dif, bs->eye, a);
			vectoangles(a, a);
			VectorCopy(a, bs->goalAngles);

			if (InFieldOfVision(bs->viewangles, 30, a) &&
				EntityVisibleBox(bs->origin, NULL, NULL, dif, bs->client, bs->shootGoal->s.number))
			{
				bs->doAttack = 1;
			}
		}
	}

	if (bs->cur_ps.hasDetPackPlanted)
	{
		//check if our enemy gets near it and detonate if he does
		BotCheckDetPacks(bs);
	}
	else if (bs->currentEnemy && bs->lastVisibleEnemyIndex == bs->currentEnemy->s.number && !bs->frame_Enemy_Vis && bs->
		plantTime < level.time &&
		!bs->doAttack && !bs->doAltAttack)
	{
		VectorSubtract(bs->origin, bs->hereWhenSpotted, a);

		if (bs->plantDecided > level.time || bs->frame_Enemy_Len < BOT_PLANT_DISTANCE * 2 && VectorLength(a) <
			BOT_PLANT_DISTANCE)
		{
			int detSelect = 0;
			int mineSelect = 0;
			mineSelect = BotSelectChoiceWeapon(bs, WP_TRIP_MINE, 0);
			detSelect = BotSelectChoiceWeapon(bs, WP_DET_PACK, 0);
			if (bs->cur_ps.hasDetPackPlanted)
			{
				detSelect = 0;
			}

			if (bs->plantDecided > level.time && bs->forceWeaponSelect &&
				bs->cur_ps.weapon == bs->forceWeaponSelect)
			{
				bs->doAttack = 1;
				bs->plantDecided = 0;
				bs->plantTime = level.time + BOT_PLANT_INTERVAL;
				bs->plantContinue = level.time + 500;
				bs->beStill = level.time + 500;
			}
			else if (mineSelect || detSelect)
			{
				if (BotSurfaceNear(bs))
				{
					if (!mineSelect)
					{
						//if no mines use detpacks, otherwise use mines
						mineSelect = WP_DET_PACK;
					}
					else
					{
						mineSelect = WP_TRIP_MINE;
					}

					detSelect = BotSelectChoiceWeapon(bs, mineSelect, 1);

					if (detSelect && detSelect != 2)
					{
						//We have it and it is now our weapon
						bs->plantDecided = level.time + 1000;
						bs->forceWeaponSelect = mineSelect;
						return;
					}
					if (detSelect == 2)
					{
						bs->forceWeaponSelect = mineSelect;
						return;
					}
				}
			}
		}
	}
	else if (bs->plantContinue < level.time)
	{
		bs->forceWeaponSelect = 0;
	}

	if (level.gametype == GT_JEDIMASTER && !bs->cur_ps.isJediMaster && bs->jmState == -1 && gJMSaberEnt && gJMSaberEnt->
		inuse)
	{
		vec3_t saberLen;
		float fSaberLen = 0;

		VectorSubtract(bs->origin, gJMSaberEnt->r.currentOrigin, saberLen);
		fSaberLen = VectorLength(saberLen);

		if (fSaberLen < 256)
		{
			if (OrgVisible(bs->origin, gJMSaberEnt->r.currentOrigin, bs->client))
			{
				VectorCopy(gJMSaberEnt->r.currentOrigin, bs->goalPosition);
			}
		}
	}

	if (bs->beStill < level.time && !WaitingForNow(bs, bs->goalPosition) && !fjHalt)
	{
		VectorSubtract(bs->goalPosition, bs->origin, bs->goalMovedir);
		VectorNormalize(bs->goalMovedir);

		if (bs->jumpTime > level.time && bs->jDelay < level.time &&
			level.clients[bs->client].pers.cmd.upmove > 0)
		{
			bs->beStill = level.time + 200;
		}
		else
		{
			trap->EA_Move(bs->client, bs->goalMovedir, 5000);
		}

		if (meleestrafe && bs->meleeStrafeDisable < level.time)
		{
			StrafeTracing(bs);

			//StrafeTracing() can boost this level
			if (bs->meleeStrafeDisable < level.time)
			{
				if (bs->meleeStrafeDir)
				{
					trap->EA_MoveRight(bs->client);
				}
				else
				{
					trap->EA_MoveLeft(bs->client);
				}
			}
		}

		if (BotTrace_Jump(bs, bs->goalPosition))
		{
			bs->jumpTime = level.time + 100;
		}
		else if (BotTrace_Duck(bs, bs->goalPosition))
		{
			bs->duckTime = level.time + 100;
		}
#ifdef BOT_STRAFE_AVOIDANCE
		else
		{
			int strafeAround = BotTrace_Strafe(bs, bs->goalPosition);

			if (strafeAround == STRAFEAROUND_RIGHT)
			{
				trap->EA_MoveRight(bs->client);
			}
			else if (strafeAround == STRAFEAROUND_LEFT)
			{
				trap->EA_MoveLeft(bs->client);
			}
		}
#endif
	}

#ifndef FORCEJUMP_INSTANTMETHOD
	if (bs->forceJumpChargeTime > level.time)
	{
		bs->jumpTime = 0;
	}
#endif

	if (bs->jumpPrep > level.time)
	{
		bs->forceJumpChargeTime = 0;
	}

	if (bs->forceJumpChargeTime > level.time)
	{
		bs->jumpHoldTime = (bs->forceJumpChargeTime - level.time) / 2 + level.time;
		bs->forceJumpChargeTime = 0;
	}

	if (bs->jumpHoldTime > level.time)
	{
		bs->jumpTime = bs->jumpHoldTime;
	}

	if (bs->jumpTime > level.time && bs->jDelay < level.time)
	{
		if (bs->jumpHoldTime > level.time)
		{
			trap->EA_Jump(bs->client);
			if (bs->wpCurrent)
			{
				if (bs->wpCurrent->origin[2] - bs->origin[2] < 64)
				{
					trap->EA_MoveForward(bs->client);
				}
			}
			else
			{
				trap->EA_MoveForward(bs->client);
			}
			if (g_entities[bs->client].client->ps.groundEntityNum == ENTITYNUM_NONE)
			{
				g_entities[bs->client].client->ps.pm_flags |= PMF_JUMP_HELD;
			}
		}
		else if (!(bs->cur_ps.pm_flags & PMF_JUMP_HELD))
		{
			trap->EA_Jump(bs->client);
		}
	}

	if (bs->duckTime > level.time)
	{
		trap->EA_Crouch(bs->client);
	}

	if (bs->dangerousObject && bs->dangerousObject->inuse && bs->dangerousObject->health > 0 &&
		bs->dangerousObject->takedamage && (!bs->frame_Enemy_Vis || !bs->currentEnemy) &&
		(BotGetWeaponRange(bs) == BWEAPONRANGE_MID || BotGetWeaponRange(bs) == BWEAPONRANGE_LONG) &&
		bs->cur_ps.weapon != WP_DET_PACK && bs->cur_ps.weapon != WP_TRIP_MINE &&
		!bs->shootGoal)
	{
		float danLen;

		VectorSubtract(bs->dangerousObject->r.currentOrigin, bs->eye, a);

		danLen = VectorLength(a);

		if (danLen > 256)
		{
			vectoangles(a, a);
			VectorCopy(a, bs->goalAngles);

			if (Q_irand(1, 10) < 5)
			{
				bs->goalAngles[YAW] += Q_irand(0, 3);
				bs->goalAngles[PITCH] += Q_irand(0, 3);
			}
			else
			{
				bs->goalAngles[YAW] -= Q_irand(0, 3);
				bs->goalAngles[PITCH] -= Q_irand(0, 3);
			}

			if (InFieldOfVision(bs->viewangles, 30, a) &&
				EntityVisibleBox(bs->origin, NULL, NULL, bs->dangerousObject->r.currentOrigin, bs->client,
					bs->dangerousObject->s.number))
			{
				bs->doAttack = 1;
			}
		}
	}

	if (PrimFiring(bs) ||
		AltFiring(bs))
	{
		friendInLOF = CheckForFriendInLOF(bs);

		if (friendInLOF)
		{
			if (PrimFiring(bs))
			{
				KeepPrimFromFiring(bs);
			}
			if (AltFiring(bs))
			{
				KeepAltFromFiring(bs);
			}
			if (useTheForce && forceHostile)
			{
				useTheForce = 0;
			}

			if (!useTheForce && friendInLOF->client)
			{
				//we have a friend here and are not currently using force powers, see if we can help them out
				if (friendInLOF->health <= 50 && level.clients[bs->client].ps.fd.forcePower > forcePowerNeeded[level.
					clients[bs->client].ps.fd.forcePowerLevel[FP_TEAM_HEAL]][FP_TEAM_HEAL])
				{
					level.clients[bs->client].ps.fd.forcePowerSelected = FP_TEAM_HEAL;
					useTheForce = 1;
					forceHostile = 0;
				}
				else if (friendInLOF->client->ps.fd.forcePower <= 50 && level.clients[bs->client].ps.fd.forcePower >
					forcePowerNeeded[level.clients[bs->client].ps.fd.forcePowerLevel[FP_TEAM_FORCE]][FP_TEAM_FORCE])
				{
					level.clients[bs->client].ps.fd.forcePowerSelected = FP_TEAM_FORCE;
					useTheForce = 1;
					forceHostile = 0;
				}
			}
		}
	}
	else if (level.gametype >= GT_TEAM)
	{
		//still check for anyone to help..
		friendInLOF = CheckForFriendInLOF(bs);

		if (!useTheForce && friendInLOF)
		{
			if (friendInLOF->health <= 50 && level.clients[bs->client].ps.fd.forcePower > forcePowerNeeded[level.clients
				[bs->client].ps.fd.forcePowerLevel[FP_TEAM_HEAL]][FP_TEAM_HEAL])
			{
				level.clients[bs->client].ps.fd.forcePowerSelected = FP_TEAM_HEAL;
				useTheForce = 1;
				forceHostile = 0;
			}
			else if (friendInLOF->client->ps.fd.forcePower <= 50 && level.clients[bs->client].ps.fd.forcePower >
				forcePowerNeeded[level.clients[bs->client].ps.fd.forcePowerLevel[FP_TEAM_FORCE]][FP_TEAM_FORCE])
			{
				level.clients[bs->client].ps.fd.forcePowerSelected = FP_TEAM_FORCE;
				useTheForce = 1;
				forceHostile = 0;
			}
		}
	}

	if (bs->doAttack && bs->cur_ps.weapon == WP_DET_PACK &&
		bs->cur_ps.hasDetPackPlanted)
	{
		//maybe a bit hackish, but bots only want to plant one of these at any given time to avoid complications
		bs->doAttack = 0;
	}

	if (bs->doAttack && bs->cur_ps.weapon == WP_SABER &&
		bs->saberDefending && bs->currentEnemy && bs->currentEnemy->client &&
		bot_weapon_blockable(bs->currentEnemy->client->ps.weapon))
	{
		bs->doAttack = 0;
	}

	if (bs->cur_ps.saberLockTime > level.time && bs->saberLockDebounce < level.time)
	{
		if (rand() % 10 < 5)
		{
			bs->doAttack = 1;
		}
		else
		{
			bs->doAttack = 0;
		}
		bs->saberLockDebounce = level.time + 50;
	}

	if (bs->botChallengingTime > level.time)
	{
		bs->doAttack = 0;
		bs->doAltAttack = 0;
		bs->doBotKick = 0;
	}

	if (bs->cur_ps.weapon == WP_SABER &&
		bs->cur_ps.saberInFlight &&
		!bs->cur_ps.saberEntityNum)
	{
		//saber knocked away, keep trying to get it back
		bs->doAttack = 1;
		bs->doAltAttack = 0;
	}

	if (bs->doAttack)
	{
		trap->EA_Attack(bs->client);
	}
	else if (bs->doAltAttack)
	{
		trap->EA_Alt_Attack(bs->client);
	}

	if (useTheForce && forceHostile && bs->botChallengingTime > level.time)
	{
		useTheForce = qfalse;
	}

	if (useTheForce)
	{
#ifndef FORCEJUMP_INSTANTMETHOD
		if (bs->forceJumpChargeTime > level.time)
		{
			level.clients[bs->client].ps.fd.forcePowerSelected = FP_LEVITATION;
			trap->EA_ForcePower(bs->client);
		}
		else
		{
#endif
			if (bot_forcepowers.integer && !g_forcePowerDisable.integer)
			{
				trap->EA_ForcePower(bs->client);
			}
#ifndef FORCEJUMP_INSTANTMETHOD
		}
#endif
	}

	MoveTowardIdealAngles(bs);
	}

void MovementCommand(bot_state_t* bs, const int command, vec3_t move_dir)
{
	if (!command)
	{
		//don't need to do anything
		return;
	}
	if (command == 1)
	{
		//Force Jump
		bs->jumpTime = level.time + 100;
		return;
	}
	if (command == 2)
	{
		//crouch
		bs->duckTime = level.time + 100;
		return;
	}
	//can't move!
	VectorCopy(vec3_origin, move_dir);
}

void AdjustMoveDirection(const bot_state_t* bs, vec3_t move_dir, const int Quad)
{
	vec3_t fwd, right;
	vec3_t addvect;

	AngleVectors(bs->cur_ps.viewangles, fwd, right, NULL);
	fwd[2] = 0;
	right[2] = 0;

	VectorNormalize(fwd);
	VectorNormalize(right);

	switch (Quad)
	{
	case 0:
		VectorCopy(fwd, addvect);
		break;
	case 1:
		VectorAdd(fwd, right, addvect);
		VectorNormalize(addvect);
		break;
	case 2:
		VectorCopy(right, addvect);
		break;
	case 3:
		VectorScale(fwd, -1, fwd);
		VectorAdd(fwd, right, addvect);
		VectorNormalize(addvect);
		break;
	case 4:
		VectorScale(fwd, -1, addvect);
		break;
	case 5:
		VectorScale(fwd, -1, fwd);
		VectorScale(right, -1, right);
		VectorAdd(fwd, right, addvect);
		VectorNormalize(addvect);
		break;
	case 6:
		VectorScale(right, -1, addvect);
		break;
	case 7:
		VectorScale(right, -1, right);
		VectorAdd(fwd, right, addvect);
		VectorNormalize(addvect);
		break;
	default:
		return;
	}

	VectorCopy(addvect, move_dir);
	VectorNormalize(move_dir);
}

int AdjustQuad(const int Quad)
{
	int dir = Quad;
	while (dir > 7)
	{
		//shift
		dir -= 8;
	}
	while (dir < 0)
	{
		//shift
		dir += 8;
	}

	return dir;
}

int FindMovementQuad(const playerState_t* ps, vec3_t move_dir)
{
	vec3_t viewfwd, viewright;
	vec3_t move;

	AngleVectors(ps->viewangles, viewfwd, viewright, NULL);

	VectorCopy(move_dir, move);

	viewfwd[2] = 0;
	viewright[2] = 0;
	move[2] = 0;

	VectorNormalize(viewfwd);
	VectorNormalize(viewright);
	VectorNormalize(move);

	const float x = DotProduct(viewright, move);
	const float y = DotProduct(viewfwd, move);

	if (x > .8)
	{
		//right
		return 2;
	}
	if (x < -0.8)
	{
		//left
		return 6;
	}
	if (x > .2)
	{
		//right forward/back
		if (y < 0)
		{
			//right back
			return 3;
		}
		//right forward
		return 1;
	}
	if (x < -0.2)
	{
		//left forward/back
		if (y < 0)
		{
			//left back
			return 5;
		}
		//left forward
		return 7;
	}
	//forward/back
	if (y < 0)
	{
		//back
		return 4;
	}
	//forward
	return 0;
}

int TraceJumpCrouchFall(const bot_state_t* bs, vec3_t move_dir, const int targetNum, vec3_t hit_normal)
{
	vec3_t mins;
	vec3_t maxs;
	vec3_t traceto_mod;
	vec3_t tracefrom_mod;
	vec3_t saveNormal;
	trace_t tr;
	int moveCommand = -1;

	VectorClear(saveNormal);

	//set the mins/maxs for the standard obstruction checks.
	VectorCopy(g_entities[bs->client].r.maxs, maxs);
	VectorCopy(g_entities[bs->client].r.mins, mins);

	//boost up the trace box as much as we can normally step up
	mins[2] += STEPSIZE;

	//Ok, check for obstructions then.
	traceto_mod[0] = bs->origin[0] + move_dir[0] * 20;
	traceto_mod[1] = bs->origin[1] + move_dir[1] * 20;
	traceto_mod[2] = bs->origin[2] + move_dir[2] * 20;

	//obstruction trace
	trap->Trace(&tr, bs->origin, mins, maxs, traceto_mod, bs->client, MASK_PLAYERSOLID, qfalse, 0, 0);

	if (tr.fraction == 1 //trace is clear
		|| tr.entityNum == targetNum //is our ignore target
		|| bs->currentEnemy && bs->currentEnemy->s.number == tr.entityNum) //is our current enemy
	{
		//nothing blocking our path
		moveCommand = 0;
	}
	else if (tr.entityNum == ENTITYNUM_WORLD)
	{
		//world object, check to see if we can walk on it.
		if (tr.plane.normal[2] >= 0.7f)
		{
			//you're probably moving up a steep ledge that's still walkable
			moveCommand = 0;
		}
	}
	//check to see if this is another player.  If so, we should be able to jump over them easily.
	//RAFIXME - add force power/force jump skill check?
	else if (tr.entityNum < MAX_CLIENTS
		//not a bot or a bot that isn't jumping.
		&& (!botstates[tr.entityNum] || !botstates[tr.entityNum]->inuse
			|| botstates[tr.entityNum]->jumpTime < level.time)
		&& bs->cur_ps.fd.forcePowerLevel[FP_LEVITATION] >= FORCE_LEVEL_1)
	{
		//another player who isn't our objective and isn't our current enemy.  Hop over them.  Don't hop
		//over bots who are already hopping.
		moveCommand = 1;
	}

	if (moveCommand == -1)
	{
		//our initial path is blocked. Try other methods to get around it.
		//Save the normal of the object so we can move around it if we can't jump/duck around it.
		VectorCopy(tr.plane.normal, saveNormal);

		//Check to see if we can successfully hop over this obsticle.
		VectorCopy(bs->origin, tracefrom_mod);

		//RAFIXME - make this based on Force jump skill level/force power availible.
		tracefrom_mod[2] += 20;
		traceto_mod[2] += 20;

		trap->Trace(&tr, tracefrom_mod, mins, maxs, traceto_mod, bs->client, MASK_PLAYERSOLID, qfalse, 0, 0);

		if (tr.fraction == 1 //trace is clear
			|| tr.entityNum == targetNum //is our ignore target
			|| bs->currentEnemy && bs->currentEnemy->s.number == tr.entityNum) //is our current enemy
		{
			//the hop check was clean.
			moveCommand = 1;
		}
		//check the slope of the thing blocking us
		else if (tr.entityNum == ENTITYNUM_WORLD)
		{
			//world object
			if (tr.plane.normal[2] >= 0.7f)
			{
				//you could hop to this, which is a steep ledge that's still walkable
				moveCommand = 1;
			}
		}

		if (moveCommand == -1)
		{
			//our hop would be blocked by something.  let's try crawling under obsticle.
			//just move the traceto_mod down from the hop trace position.  This is faster
			//than redoing it from scratch.
			traceto_mod[2] -= 20;

			maxs[2] = CROUCH_MAXS_2; //set our trace box to be the size of a crouched player.

			trap->Trace(&tr, bs->origin, mins, maxs, traceto_mod, bs->client, MASK_PLAYERSOLID, qfalse, 0, 0);

			if (tr.fraction == 1 //trace is clear
				|| tr.entityNum == targetNum //is our ignore target
				|| bs->currentEnemy && bs->currentEnemy->s.number == tr.entityNum) //is our current enemy
			{
				//we can duck under this object.
				moveCommand = 2;
			}
			//check the slope of the thing blocking us
			else if (tr.entityNum == ENTITYNUM_WORLD)
			{
				//world object
				if (tr.plane.normal[2] >= 0.7f)
				{
					//you could hop to this, which is a steep ledge that's still walkable
					moveCommand = 2;
				}
			}
		}
	}

	if (moveCommand != -1)
	{
		//we found a way around our current obsticle, check to make sure we're not going to go off a cliff.
		traceto_mod[0] = bs->origin[0] + move_dir[0] * 45;
		traceto_mod[1] = bs->origin[1] + move_dir[1] * 45;
		traceto_mod[2] = bs->origin[2] + move_dir[2] * 45;

		VectorCopy(traceto_mod, tracefrom_mod);

		//check for 50+ feet drops
		traceto_mod[2] -= 532;

		trap->Trace(&tr, tracefrom_mod, mins, maxs, traceto_mod, bs->client, MASK_PLAYERSOLID, qfalse, 0, 0);
		if (tr.fraction == 1 && !tr.startsolid)
		{
			//CLIFF!
			moveCommand = -1;
		}

		//RAFIXME - This might cause bots to freeze after the apex of a jump
		//over lava and such.  Keep an eye out for this possible behavior.
		if (bs->jumpTime < level.time)
		{
			//we're not actively jumping, so check to make sure we're not going to move
			//into slime/lava/fall to death areas.

			const int contents = trap->PointContents(tr.endpos, -1);
			if (contents & (CONTENTS_SLIME | CONTENTS_LAVA))
			{
				//the fall point is inside something we don't want to move into
				moveCommand = -1;
			}
		}
	}

	if (moveCommand == -1)
	{
		//couldn't find a way to move in this direction.  Save the normal vector so we can use it to move around
		//this object.  Note, we even do this when we can hop/crawl around something but the fall check fails
		//so we can follow the railing or whatever this is.
		VectorCopy(saveNormal, hit_normal);
	}
	return moveCommand;
}

qboolean try_move_around_obsticle(bot_state_t* bs, vec3_t move_dir, const int targetNum, vec3_t hit_normal,
	const int tryNum,
	const qboolean CheckBothWays)
{
	//attempt to move around objects.

	if (tryNum > 3)
	{
		//ok, don't get stuff in a loop
		return qfalse;
	}

	if (!VectorCompare(hit_normal, vec3_origin))
	{
		vec3_t cross;

		//find the perpendicular vector to the normal.
		PerpendicularVector(cross, hit_normal);
		cross[2] = 0; //flatten the cross vector to 2d.
		VectorNormalize(cross);

		//determine initial movement direction.
		if (bs->evadeTime > level.time)
		{
			//already going a set direction, keep going that direction.
			if (bs->evadeDir > 3)
			{
				//going "left".  switch directions.
				VectorScale(cross, -1, cross);
			}
		}
		else
		{
			//otherwise, try to move in the direction that takes us in the direction we're
			//trying to move.
			const float Dot = DotProduct(cross, move_dir);
			if (Dot < 0)
			{
				//going in the wrong initial direction, switch!
				VectorScale(cross, -1, cross);
			}
		}

		VectorCopy(cross, move_dir);
		int movecom = TraceJumpCrouchFall(bs, move_dir, targetNum, hit_normal);

		if (movecom != -1)
		{
			MovementCommand(bs, movecom, move_dir);
			return qtrue;
		}

		if (!VectorCompare(hit_normal, vec3_origin))
		{
			//hit another surface while trying to trace along this one, try to move along
			//it instead.
			if (try_move_around_obsticle(bs, move_dir, targetNum, hit_normal, tryNum + 1, qfalse))
			{
				//set the evade timer because this is often where we can get stuck if
				//tracing the wall sends us in some weird direction.
				bs->evadeTime = level.time + 10000;
				return qtrue;
			}
		}

		if (CheckBothWays)
		{
			//try the other direction
			VectorScale(cross, -1, cross);

			//toggle the evadeDir since we want to continue moving this direction.
			if (bs->evadeDir > 3)
			{
				//was left, try right
				bs->evadeDir = 0;
			}
			else
			{
				//was right, try left
				bs->evadeDir = 7;
			}

			VectorCopy(cross, move_dir);
			movecom = TraceJumpCrouchFall(bs, move_dir, targetNum, hit_normal);

			if (movecom != -1)
			{
				MovementCommand(bs, movecom, move_dir);
				//set the evade timer because this is often where we can get stuck if
				//tracing the wall sends us in some weird direction.
				bs->evadeTime = level.time + 10000;
				return qtrue;
			}

			//try recursively dealing with this.
			return try_move_around_obsticle(bs, move_dir, targetNum, hit_normal, tryNum + 1, qfalse);
		}
	}

	return qfalse;
}

void TraceMove(bot_state_t* bs, vec3_t move_dir, const int targetNum)
{
	vec3_t dir;
	vec3_t hitNormal;
	int i = 7;
	int Quad;
	VectorClear(hitNormal);
	int movecom = TraceJumpCrouchFall(bs, move_dir, targetNum, hitNormal);

	VectorCopy(move_dir, dir);

	if (movecom != -1)
	{
		MovementCommand(bs, movecom, move_dir);
		return;
	}

	//try moving around the edge of the blocking object if that's the problem.
	if (try_move_around_obsticle(bs, dir, targetNum, hitNormal, 0, qtrue))
	{
		//found a way.
		VectorCopy(dir, move_dir);
		return;
	}

	//restore the original moveDir incase our obsticle code choked.
	VectorCopy(move_dir, dir);

	if (bs->evadeTime > level.time)
	{
		//try the current evade direction to prevent spazing
		AdjustMoveDirection(bs, dir, bs->evadeDir);
		movecom = TraceJumpCrouchFall(bs, dir, targetNum, hitNormal);
		if (movecom != -1)
		{
			MovementCommand(bs, movecom, dir);
			VectorCopy(dir, move_dir);
			bs->evadeTime = level.time + 500;
			return;
		}
		i--;
	}

	//Since our default direction didn't work we need to switch melee strafe directions if
	//we are melee strafing.
	//0 = no strafe
	//1 = strafe right
	//2 = strafe left
	if (bs->meleeStrafeTime > level.time)
	{
		bs->meleeStrafeDir = Q_irand(0, 2);
		bs->meleeStrafeTime = level.time + Q_irand(500, 1800);
	}

	const int fwdstrafe = FindMovementQuad(&bs->cur_ps, move_dir);

	if (Q_irand(0, 1))
	{
		//try strafing left
		Quad = fwdstrafe - 2;
	}
	else
	{
		Quad = fwdstrafe + 2;
	}

	Quad = AdjustQuad(Quad);

	//reset Dir to original moveDir
	VectorCopy(move_dir, dir);

	//shift movedir for quad
	AdjustMoveDirection(bs, dir, Quad);

	movecom = TraceJumpCrouchFall(bs, dir, targetNum, hitNormal);

	if (movecom != -1)
	{
		MovementCommand(bs, movecom, dir);
		VectorCopy(dir, move_dir);
		bs->evadeDir = Quad;
		bs->evadeTime = level.time + 100;
		return;
	}
	i--;

	//no luck, try the other full strafe direction
	Quad += 4;
	Quad = AdjustQuad(Quad);

	//reset Dir to original moveDir
	VectorCopy(move_dir, dir);

	//shift movedir for quad
	AdjustMoveDirection(bs, dir, Quad);

	movecom = TraceJumpCrouchFall(bs, dir, targetNum, hitNormal);

	if (movecom != -1)
	{
		MovementCommand(bs, movecom, dir);
		VectorCopy(dir, move_dir);
		bs->evadeDir = Quad;
		bs->evadeTime = level.time + 100;
		return;
	}
	i--;

	//still no dice
	for (; i > 0; i--)
	{
		Quad++;
		Quad = AdjustQuad(Quad);

		if (Quad == fwdstrafe || Quad == AdjustQuad(fwdstrafe - 2) || Quad == AdjustQuad(fwdstrafe + 2)
			|| bs->evadeTime > level.time && Quad == bs->evadeDir)
		{
			//Already did those directions
			continue;
		}

		VectorCopy(move_dir, dir);

		//shift movedir for quad
		AdjustMoveDirection(bs, dir, Quad);
		movecom = TraceJumpCrouchFall(bs, dir, targetNum, hitNormal);
		if (movecom != -1)
		{
			//find a good direction
			MovementCommand(bs, movecom, dir);
			VectorCopy(dir, move_dir);
			bs->evadeDir = Quad;
			bs->evadeTime = level.time + 100;
			return;
		}
	}
}

extern void NPC_ConversationAnimation(gentity_t* NPC);

void BOT_CheckSpeak(gentity_t* bot, const qboolean moving)
{
	char filename[256];
	char botSoundDir[256];
	fileHandle_t f;

	if (bot->bot_check_speach_time > level.time) return; // not yet...

	bot->bot_check_speach_time = level.time + 5000 + irand(0, 15000);

	strcpy(botSoundDir, bot->client->botSoundDir);

	if (botstates[bot->s.number]->currentEnemy && botstates[bot->s.number]->enemySeenTime >= level.time)
	{
		// Have enemy...
		const int rand_choice = irand(0, 50);

		switch (rand_choice)
		{
		case 0:
			strcpy(filename, va("sound/chars/%s/misc/anger1.mp3", botSoundDir));
			break;
		case 1:
			strcpy(filename, va("sound/chars/%s/misc/anger2.mp3", botSoundDir));
			break;
		case 2:
			strcpy(filename, va("sound/chars/%s/misc/anger3.mp3", botSoundDir));
			break;
		case 3:
			strcpy(filename, va("sound/chars/%s/misc/chase1.mp3", botSoundDir));
			break;
		case 4:
			strcpy(filename, va("sound/chars/%s/misc/chase2.mp3", botSoundDir));
			break;
		case 5:
			strcpy(filename, va("sound/chars/%s/misc/chase3.mp3", botSoundDir));
			break;
		case 6:
			strcpy(filename, va("sound/chars/%s/misc/combat1.mp3", botSoundDir));
			break;
		case 7:
			strcpy(filename, va("sound/chars/%s/misc/combat2.mp3", botSoundDir));
			break;
		case 8:
			strcpy(filename, va("sound/chars/%s/misc/combat3.mp3", botSoundDir));
			break;
		case 9:
			strcpy(filename, va("sound/chars/%s/misc/confuse1.mp3", botSoundDir));
			break;
		case 10:
			strcpy(filename, va("sound/chars/%s/misc/confuse2.mp3", botSoundDir));
			break;
		case 11:
			strcpy(filename, va("sound/chars/%s/misc/confuse3.mp3", botSoundDir));
			break;
		case 12:
			strcpy(filename, va("sound/chars/%s/misc/cover1.mp3", botSoundDir));
			break;
		case 13:
			strcpy(filename, va("sound/chars/%s/misc/cover2.mp3", botSoundDir));
			break;
		case 14:
			strcpy(filename, va("sound/chars/%s/misc/cover3.mp3", botSoundDir));
			break;
		case 15:
			strcpy(filename, va("sound/chars/%s/misc/cover4.mp3", botSoundDir));
			break;
		case 16:
			strcpy(filename, va("sound/chars/%s/misc/cover5.mp3", botSoundDir));
			break;
		case 17:
			strcpy(filename, va("sound/chars/%s/misc/deflect1.mp3", botSoundDir));
			break;
		case 18:
			strcpy(filename, va("sound/chars/%s/misc/deflect2.mp3", botSoundDir));
			break;
		case 19:
			strcpy(filename, va("sound/chars/%s/misc/deflect3.mp3", botSoundDir));
			break;
		case 20:
			strcpy(filename, va("sound/chars/%s/misc/detected1.mp3", botSoundDir));
			break;
		case 21:
			strcpy(filename, va("sound/chars/%s/misc/detected2.mp3", botSoundDir));
			break;
		case 22:
			strcpy(filename, va("sound/chars/%s/misc/detected3.mp3", botSoundDir));
			break;
		case 23:
			strcpy(filename, va("sound/chars/%s/misc/detected4.mp3", botSoundDir));
			break;
		case 24:
			strcpy(filename, va("sound/chars/%s/misc/detected5.mp3", botSoundDir));
			break;
		case 25:
			strcpy(filename, va("sound/chars/%s/misc/escaping1.mp3", botSoundDir));
			break;
		case 26:
			strcpy(filename, va("sound/chars/%s/misc/escaping2.mp3", botSoundDir));
			break;
		case 27:
			strcpy(filename, va("sound/chars/%s/misc/escaping3.mp3", botSoundDir));
			break;
		case 28:
			strcpy(filename, va("sound/chars/%s/misc/giveup1.mp3", botSoundDir));
			break;
		case 29:
			strcpy(filename, va("sound/chars/%s/misc/giveup2.mp3", botSoundDir));
			break;
		case 30:
			strcpy(filename, va("sound/chars/%s/misc/giveup3.mp3", botSoundDir));
			break;
		case 31:
			strcpy(filename, va("sound/chars/%s/misc/giveup4.mp3", botSoundDir));
			break;
		case 32:
			strcpy(filename, va("sound/chars/%s/misc/gloat1.mp3", botSoundDir));
			break;
		case 33:
			strcpy(filename, va("sound/chars/%s/misc/gloat2.mp3", botSoundDir));
			break;
		case 34:
			strcpy(filename, va("sound/chars/%s/misc/gloat3.mp3", botSoundDir));
			break;
		case 35:
			strcpy(filename, va("sound/chars/%s/misc/jchase1.mp3", botSoundDir));
			break;
		case 36:
			strcpy(filename, va("sound/chars/%s/misc/jchase2.mp3", botSoundDir));
			break;
		case 37:
			strcpy(filename, va("sound/chars/%s/misc/jchase3.mp3", botSoundDir));
			break;
		case 38:
			strcpy(filename, va("sound/chars/%s/misc/jdetected1.mp3", botSoundDir));
			break;
		case 39:
			strcpy(filename, va("sound/chars/%s/misc/jdetected2.mp3", botSoundDir));
			break;
		case 40:
			strcpy(filename, va("sound/chars/%s/misc/jdetected3.mp3", botSoundDir));
			break;
		case 41:
			strcpy(filename, va("sound/chars/%s/misc/jlost1.mp3", botSoundDir));
			break;
		case 42:
			strcpy(filename, va("sound/chars/%s/misc/jlost2.mp3", botSoundDir));
			break;
		case 43:
			strcpy(filename, va("sound/chars/%s/misc/jlost3.mp3", botSoundDir));
			break;
		case 44:
			strcpy(filename, va("sound/chars/%s/misc/outflank1.mp3", botSoundDir));
			break;
		case 45:
			strcpy(filename, va("sound/chars/%s/misc/outflank2.mp3", botSoundDir));
			break;
		case 46:
			strcpy(filename, va("sound/chars/%s/misc/outflank3.mp3", botSoundDir));
			break;
		case 47:
			strcpy(filename, va("sound/chars/%s/misc/taunt.mp3", botSoundDir));
			break;
		case 48:
			strcpy(filename, va("sound/chars/%s/misc/taunt1.mp3", botSoundDir));
			break;
		case 49:
			strcpy(filename, va("sound/chars/%s/misc/taunt2.mp3", botSoundDir));
			break;
		default:
			strcpy(filename, va("sound/chars/%s/misc/taunt3.mp3", botSoundDir));
			break;
		}
	}
	else
	{
		// No enemy...
		const int rand_choice = irand(0, 10);

		switch (rand_choice)
		{
		case 0:
			strcpy(filename, va("sound/chars/%s/misc/sight1.mp3", botSoundDir));
			break;
		case 1:
			strcpy(filename, va("sound/chars/%s/misc/sight2.mp3", botSoundDir));
			break;
		case 2:
			strcpy(filename, va("sound/chars/%s/misc/sight3.mp3", botSoundDir));
			break;
		case 3:
			strcpy(filename, va("sound/chars/%s/misc/look1.mp3", botSoundDir));
			break;
		case 4:
			strcpy(filename, va("sound/chars/%s/misc/look2.mp3", botSoundDir));
			break;
		case 5:
			strcpy(filename, va("sound/chars/%s/misc/look3.mp3", botSoundDir));
			break;
		case 6:
			strcpy(filename, va("sound/chars/%s/misc/suspicious1.mp3", botSoundDir));
			break;
		case 7:
			strcpy(filename, va("sound/chars/%s/misc/suspicious2.mp3", botSoundDir));
			break;
		case 8:
			strcpy(filename, va("sound/chars/%s/misc/suspicious3.mp3", botSoundDir));
			break;
		case 9:
			strcpy(filename, va("sound/chars/%s/misc/suspicious4.mp3", botSoundDir));
			break;
		default:
			strcpy(filename, va("sound/chars/%s/misc/suspicious5.mp3", botSoundDir));
			break;
		}
	}

	trap->FS_Open(filename, &f, FS_READ);

	if (!f)
	{
		trap->FS_Close(f);
		return;
	}

	trap->FS_Close(f);

	trap->Print("Bot sound file [%s] played.\n", filename);

	// Play a taunt/etc...
	G_SoundOnEnt(bot, CHAN_VOICE_ATTEN, filename);

	if (!moving)
	{
		NPC_ConversationAnimation(bot);
	}
}

//Behavior to move to the given DestPosition
//strafe = do some strafing while moving to this location
void BotMoveto(bot_state_t* bs, const qboolean strafe)
{
	qboolean recalcroute = qfalse;
	qboolean findwp = qfalse;
	int badwp = -2;

	if (!bs->wpCurrent)
	{
		////ok, we just did something other than wp navigation.  find the closest wp.
		findwp = qtrue;
	}
	else if (bs->wpSeenTime < level.time)
	{
		//lost track of the waypoint
		findwp = qtrue;
		badwp = bs->wpCurrent->index;
		bs->wpDestination = NULL;
		recalcroute = qtrue;
	}
	else if (bs->wpTravelTime < level.time)
	{
		//spent too much time traveling to this point or lost sight for too long.
		//recheck everything
		findwp = qtrue;
		badwp = bs->wpCurrent->index;

		bs->wpDestination = NULL;
		recalcroute = qtrue;
	}
	//Check to make sure we didn't get knocked way off course.
	else if (!bs->wpSpecial)
	{
		const float distthen = Distance(bs->wpCurrentLoc, bs->wpCurrent->origin);
		const float distnow = Distance(bs->wpCurrent->origin, bs->origin);
		if (2 * distthen < distnow)
		{
			//we're pretty far off the path, check to make sure we didn't get knocked way off course.
			findwp = qtrue;
		}
	}

	if (!VectorCompare(bs->DestPosition, bs->lastDestPosition) || !bs->wpDestination)
	{
		//The goal position has moved from last frame.  make sure it's not closer to a different
		//destination WP
		int destwp = GetNearestVisibleWPSJE(bs, bs->DestPosition, bs->client, badwp);

		if (destwp == -1)
		{
			//ok, don't have a wappoint that can see that point...ok, go to the closest wp and
			//and try from there.
			destwp = GetNearestWP(bs, bs->DestPosition, badwp);

			if (destwp == -1)
			{
				//crap, this map has no wps.  try just autonaving it then
				bot_move(bs, bs->DestPosition, qfalse, strafe);
				return;
			}
		}

		if (!bs->wpDestination || bs->wpDestination->index != destwp)
		{
			bs->wpDestination = gWPArray[destwp];
			recalcroute = qtrue;
		}
	}

	BOT_CheckSpeak(&g_entities[bs->client], qtrue);

	if (findwp)
	{
		int wp = GetNearestVisibleWPSJE(bs, bs->origin, bs->client, badwp);
		if (wp == -1)
		{
			//can't find a visible
			wp = GetNearestWP(bs, bs->origin, badwp);
			if (wp == -1)
			{
				//no waypoints
				bot_move(bs, bs->DestPosition, qfalse, strafe);
				return;
			}
		}

		//got a waypoint, lock on and move towards it
		bs->wpCurrent = gWPArray[wp];
		ResetWPTimers(bs);
		VectorCopy(bs->origin, bs->wpCurrentLoc);
		if (!recalcroute && FindOnRoute(bs->wpCurrent->index, bs->botRoute) == -1)
		{
			//recalc route
			recalcroute = qtrue;
		}
	}

	if (recalcroute)
	{
		if (FindIdealPathtoWP(bs, bs->wpCurrent->index, bs->wpDestination->index, badwp, bs->botRoute) == -1)
		{
			//can't get to destination wp from current wp, wing it
			bs->wpCurrent = NULL;
			ClearRoute(bs->botRoute);
			bot_move(bs, bs->DestPosition, qfalse, strafe);
			return;
		}
		//set wp timers
		ResetWPTimers(bs);
	}

	//travelling to a waypoint
	if ((bs->wpCurrent->index != bs->wpDestination->index || !bs->wpTouchedDest) && Distance(
		bs->origin, bs->wpCurrent->origin) < BOT_WPTOUCH_DISTANCE
		&& !bs->wpSpecial)
	{
		WPTouch(bs);
	}

	//if you're closer to your bs->DestPosition than you are to your next waypoint, just
	//move to your bs->DestPosition.  This is to prevent the bots from backstepping when
	//very close to their target
	if (!bs->wpSpecial && (Distance(bs->origin, bs->wpCurrent->origin) > Distance(bs->origin, bs->DestPosition)
		//closer to our destination than the next waypoint
		|| bs->wpCurrent->index == bs->wpDestination->index && bs->wpTouchedDest))
		//We've touched our final waypoint and should head towards the destination
	{
		//move to DestPosition
		bot_move(bs, bs->DestPosition, qfalse, strafe);
	}
	else
	{
		//move to next waypoint
		bot_move(bs, bs->wpCurrent->origin, qtrue, strafe);
	}
}

void BotBehave_DefendBasic(bot_state_t* bs, vec3_t defpoint)
{
	const float dist = Distance(bs->origin, defpoint);

	if (bs->currentEnemy)
	{
		//see an enemy
		BotBehave_AttackBasic(bs, bs->currentEnemy);
		if (dist > 500)
		{
			//attack move back into the defend range
			bot_move(bs, defpoint, qfalse, qfalse);
		}
		else if (dist > 500 * .9)
		{
			//nearing max distance hold here and attack
			trap->EA_Move(bs->client, vec3_origin, 0);
		}
		else
		{
			//just attack them
		}
	}
	else
	{
		//don't see an enemy
		if (DontBlockAllies(bs))
		{
		}
		else if (dist < 200)
		{
			//just stand around and wait
		}
		else
		{
			//move closer to defend target
			bot_move(bs, defpoint, qfalse, qfalse);
		}
	}
}

void BotBehave_AttackMove(bot_state_t* bs)
{
	vec3_t viewDir;
	vec3_t ang;
	vec3_t enemyOrigin;

	if (!bs->frame_Enemy_Vis && bs->enemySeenTime < level.time)
	{
		//lost track of enemy
		bs->currentEnemy = NULL;
		return;
	}

	FindOrigin(bs->currentEnemy, enemyOrigin);

	const float range = TargetDistance(bs, bs->currentEnemy, enemyOrigin);

	//move towards DestPosition
	if (VectorLength(bs->velocity) <= 16)
	{
		BotMoveto(bs, qtrue);
	}
	else
	{
		BotMoveto(bs, qfalse);
	}

	if (bs->wpSpecial)
	{
		//in special wp move, don't do interrupt it.
		return;
	}

	//adjust angle for target leading.
	const float leadamount = BotWeaponCanLead(bs);

	BotAimLeading(bs, enemyOrigin, leadamount);

	//set viewangle
	VectorSubtract(enemyOrigin, bs->eye, viewDir);

	vectoangles(viewDir, ang);
	VectorCopy(ang, bs->goalAngles);

	if (bs->frame_Enemy_Vis && bs->cur_ps.weapon == bs->virtualWeapon && range < MaximumAttackDistance[bs->
		virtualWeapon]
		&& range > MinimumAttackDistance[bs->virtualWeapon]
		&& (InFieldOfVision(bs->viewangles, 30, ang)
			|| bs->virtualWeapon == WP_SABER && InFieldOfVision(bs->viewangles, 100, ang)))
	{
		//don't attack unless you're inside your AttackDistance band and actually pointing at your enemy.
		if (bs->virtualWeapon != WP_SABER && bs->cur_ps.BlasterAttackChainCount >= MISHAPLEVEL_HEAVY)
		{
			//don't shoot like a retard if you're not going to hit anything
			return;
		}

		//This is to prevent the bots from attackmoving with the saber @ 500 meters. :)
		trap->EA_Attack(bs->client);

		if (bs->virtualWeapon == WP_SABER)
		{
			//only walk while attacking with the saber.
			bs->doWalk = qtrue;
		}
	}
}

void BotBehave_Attack(bot_state_t* bs)
{
	const int desiredweap = FavoriteWeapon(bs, bs->currentEnemy, qtrue, qtrue, 0);

	if (bs->frame_Enemy_Len > MaximumAttackDistance[desiredweap])
	{
		//this should be an attack while moving function but for now we'll just use moveto
		vec3_t enemyOrigin;
		FindOrigin(bs->currentEnemy, enemyOrigin);
		VectorCopy(enemyOrigin, bs->DestPosition);
		bs->DestIgnore = bs->currentEnemy->s.number;
		BotBehave_AttackMove(bs);
		return;
	}

	//we're going to go get in close so null out the wpCurrent so it will update when we're
	//done.
	bs->wpCurrent = NULL;

	//use basic attack
	BotBehave_AttackBasic(bs, bs->currentEnemy);
}

int BotWeapon_Detpack(bot_state_t* bs, const gentity_t* target)
{
	gentity_t* dp = NULL;
	const gentity_t* bestDet = NULL;
	vec3_t TargOrigin;
	float bestDistance = 9999;

	FindOrigin(target, TargOrigin);

	while ((dp = G_Find(dp, FOFS(classname), "detpack")) != NULL)
	{
		if (dp && dp->parent && dp->parent->s.number == bs->client)
		{
			const float tempDist = Distance(TargOrigin, dp->s.pos.trBase);
			if (tempDist < bestDistance)
			{
				bestDistance = tempDist;
				bestDet = dp;
			}
		}
	}

	if (!bestDet || bestDistance > 500)
	{
		return qfalse;
	}

	//check to see if the bot knows that the det is near the target.

	//found the closest det to the target and it is in blow distance.
	if (WP_DET_PACK != bs->cur_ps.weapon)
	{
		//need to switch to desired weapon
		BotSelectChoiceWeapon(bs, WP_DET_PACK, qtrue);
	}
	else
	{
		//blast it!
		bs->doAltAttack = 1;
	}

	return qtrue;
}

int gUpdateVars = 0;

/*
==================
BotAIStartFrame
==================
*/
int BotAIStartFrame(const int time)
{
	int i;
	int thinktime;
	static int local_time;
	static int lastbotthink_time;

	if (gUpdateVars < level.time)
	{
		trap->Cvar_Update(&bot_pvstype);
		trap->Cvar_Update(&bot_camp);
		trap->Cvar_Update(&bot_attachments);
		trap->Cvar_Update(&bot_forgimmick);
		trap->Cvar_Update(&bot_honorableduelacceptance);
#ifndef FINAL_BUILD
		trap->Cvar_Update(&bot_getinthecarrr);
#endif
		trap->Cvar_Update(&bot_fps);

		gUpdateVars = level.time + 1000;
	}

	G_CheckBotSpawn();

	//rww - addl bot frame functions
	if (gBotEdit)
	{
		trap->Cvar_Update(&bot_wp_info);
		BotWaypointRender();
	}

	UpdateEventTracker();
	//end rww

	//cap the bot think time
	//if the bot think time changed we should reschedule the bots
	if (BOT_THINK_TIME != lastbotthink_time)
	{
		lastbotthink_time = BOT_THINK_TIME;
		BotScheduleBotThink();
	}

	const int elapsed_time = time - local_time;
	local_time = time;

	if (elapsed_time > BOT_THINK_TIME) thinktime = elapsed_time;
	else thinktime = BOT_THINK_TIME;

	// execute scheduled bot AI
	for (i = 0; i < MAX_CLIENTS; i++)
	{
		if (!botstates[i] || !botstates[i]->inuse)
		{
			continue;
		}
		botstates[i]->botthink_residual += elapsed_time;

		if (botstates[i]->botthink_residual >= thinktime)
		{
			botstates[i]->botthink_residual -= thinktime;

			if (g_entities[i].client->pers.connected == CON_CONNECTED)
			{
				BotAI(i, (float)thinktime / 1000);
			}
		}
	}

	// execute bot user commands every frame
	for (i = 0; i < MAX_CLIENTS; i++)
	{
		if (!botstates[i] || !botstates[i]->inuse)
		{
			continue;
		}
		if (g_entities[i].client->pers.connected != CON_CONNECTED)
		{
			continue;
		}

		BotUpdateInput(botstates[i], time, elapsed_time);
		trap->BotUserCommand(botstates[i]->client, &botstates[i]->lastucmd);
	}

	return qtrue;
}

/*
==============
BotAISetup
==============
*/
int BotAISetup(const int restart)
{
	//rww - new bot cvars..
	trap->Cvar_Register(&bot_forcepowers, "bot_forcepowers", "1", CVAR_CHEAT);
	trap->Cvar_Register(&bot_forgimmick, "bot_forgimmick", "0", CVAR_CHEAT);
	trap->Cvar_Register(&bot_honorableduelacceptance, "bot_honorableduelacceptance", "1", CVAR_CHEAT);
	trap->Cvar_Register(&bot_pvstype, "bot_pvstype", "1", CVAR_CHEAT);
#ifndef FINAL_BUILD
	trap->Cvar_Register(&bot_getinthecarrr, "bot_getinthecarrr", "0", 0);
#endif

#ifdef _DEBUG
	trap->Cvar_Register(&bot_nogoals, "bot_nogoals", "0", CVAR_CHEAT);
	trap->Cvar_Register(&bot_debugmessages, "bot_debugmessages", "0", CVAR_CHEAT);
#endif

	trap->Cvar_Register(&bot_attachments, "bot_attachments", "1", 0);
	trap->Cvar_Register(&bot_camp, "bot_camp", "1", 0);

	trap->Cvar_Register(&bot_wp_info, "bot_wp_info", "1", 0);
	trap->Cvar_Register(&bot_wp_edit, "bot_wp_edit", "0", CVAR_CHEAT);
	trap->Cvar_Register(&bot_wp_clearweight, "bot_wp_clearweight", "1", 0);
	trap->Cvar_Register(&bot_wp_distconnect, "bot_wp_distconnect", "1", 0);
	trap->Cvar_Register(&bot_wp_visconnect, "bot_wp_visconnect", "1", 0);
	trap->Cvar_Register(&bot_fps, "bot_fps", "20", CVAR_ARCHIVE);

	trap->Cvar_Update(&bot_forcepowers);
	//end rww

	//if the game is restarted for a tournament
	if (restart)
	{
		return qtrue;
	}

	//initialize the bot states
	memset(botstates, 0, sizeof botstates);

	if (!trap->BotLibSetup())
	{
		return qfalse; //wts?!
	}

	return qtrue;
}

/*
==============
BotAIShutdown
==============
*/
int BotAIShutdown(const int restart)
{
	//if the game is restarted for a tournament
	if (restart)
	{
		//shutdown all the bots in the botlib
		for (int i = 0; i < MAX_CLIENTS; i++)
		{
			if (botstates[i] && botstates[i]->inuse)
			{
				BotAIShutdownClient(botstates[i]->client, restart);
			}
		}
		//don't shutdown the bot library
	}
	else
	{
		trap->BotLibShutdown();
	}
	return qtrue;
}