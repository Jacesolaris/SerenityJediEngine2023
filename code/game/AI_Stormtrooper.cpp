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

#include "b_local.h"
#include "g_nav.h"
#include "anims.h"
#include "g_navigator.h"
#include "../cgame/cg_local.h"
#include "g_functions.h"

extern void CG_DrawAlert(vec3_t origin, float rating);
extern void G_AddVoiceEvent(gentity_t* self, int event, int speakDebounceTime);
extern void AI_GroupUpdateSquadstates(AIGroupInfo_t* group, gentity_t* member, int newSquadState);
extern qboolean AI_GroupContainsEntNum(AIGroupInfo_t* group, int entNum);
extern void AI_GroupUpdateEnemyLastSeen(AIGroupInfo_t* group, vec3_t spot);
extern void AI_GroupUpdateClearShotTime(AIGroupInfo_t* group);
extern void NPC_TempLookTarget(gentity_t* self, int lookEntNum, int minLookTime, int maxLookTime);
extern qboolean G_ExpandPointToBBox(vec3_t point, const vec3_t mins, const vec3_t maxs, int ignore, int clipmask);
extern void ChangeWeapon(const gentity_t* ent, int newWeapon);
extern void NPC_CheckGetNewWeapon(void);
extern qboolean Q3_TaskIDPending(const gentity_t* ent, taskID_t taskType);
extern int GetTime(int lastTime);
extern void NPC_AimAdjust(int change);
extern qboolean FlyingCreature(const gentity_t* ent);
extern void NPC_EvasionSaber(void);
extern qboolean RT_Flying(gentity_t* self);
extern void NPC_CheckEvasion(void);
extern qboolean in_front(vec3_t spot, vec3_t from, vec3_t from_angles, float thresh_hold = 0.0f);
extern qboolean PM_InKnockDown(const playerState_t* ps);
extern qboolean NPC_CanUseAdvancedFighting();
extern void NPC_AngerSound(void);
extern qboolean WP_AbsorbKick(gentity_t* self, gentity_t* pusher, const vec3_t pushDir);
extern cvar_t* g_allowgunnerbash;

extern cvar_t* d_asynchronousGroupAI;

constexpr auto MAX_VIEW_DIST = 1024;
constexpr auto MAX_VIEW_SPEED = 250;
constexpr auto MAX_LIGHT_INTENSITY = 255;
constexpr auto MIN_LIGHT_THRESHOLD = 0.1;
constexpr auto ST_MIN_LIGHT_THRESHOLD = 30;
constexpr auto ST_MAX_LIGHT_THRESHOLD = 180;
constexpr auto DISTANCE_THRESHOLD = 0.075f;
#define	MIN_TURN_AROUND_DIST_SQ	(10000)	//(100 squared) don't stop running backwards if your goal is less than 100 away
constexpr auto SABER_AVOID_DIST = 128.0f; //256.0f;
#define SABER_AVOID_DIST_SQ (SABER_AVOID_DIST*SABER_AVOID_DIST)

constexpr auto DISTANCE_SCALE = 0.35f; //These first three get your base detection rating, ideally add up to 1;
constexpr auto FOV_SCALE = 0.40f; //;
constexpr auto LIGHT_SCALE = 0.25f; //;

constexpr auto SPEED_SCALE = 0.25f; //These next two are bonuses;
constexpr auto TURNING_SCALE = 0.25f; //;

constexpr auto REALIZE_THRESHOLD = 0.6f;
#define CAUTIOUS_THRESHOLD	( REALIZE_THRESHOLD * 0.75 )

qboolean NPC_CheckPlayerTeamStealth(void);

static qboolean enemyLOS;
static qboolean enemyCS;
static qboolean enemyInFOV;
static qboolean hitAlly;
static qboolean faceEnemy;
static qboolean doMove;
static qboolean shoot;
static float enemyDist;
static vec3_t impactPos;

int groupSpeechDebounceTime[TEAM_NUM_TEAMS]; //used to stop several group AI from speaking all at once

void NPC_Saboteur_Precache(void)
{
	G_SoundIndex("sound/chars/shadowtrooper/cloak.wav");
	G_SoundIndex("sound/chars/shadowtrooper/decloak.wav");
}

void Saboteur_Decloak(gentity_t* self, int uncloakTime)
{
	if (self && self->client)
	{
		if (self->client->ps.powerups[PW_CLOAKED] && TIMER_Done(self, "decloakwait"))
		{
			//Uncloak
			self->client->ps.powerups[PW_CLOAKED] = 0;
			self->client->ps.powerups[PW_UNCLOAKING] = level.time + 2000;
			//FIXME: temp sound
			G_SoundOnEnt(self, CHAN_ITEM, "sound/chars/shadowtrooper/decloak.wav");
			TIMER_Set(self, "nocloak", uncloakTime);
			NPC_SetAnim(self, SETANIM_TORSO, BOTH_FORCE_DRAIN_RELEASE, SETANIM_FLAG_OVERRIDE | SETANIM_FLAG_HOLD);
		}
	}
}

void Saboteur_Cloak(gentity_t* self)
{
	if (self && self->client && self->NPC)
	{
		if (in_camera)	// Cinematic
		{
			Saboteur_Decloak(self);
		}
		if (TIMER_Done(self, "nocloak"))
		{
			//not sitting around waiting to cloak again
			if (!(self->NPC->aiFlags & NPCAI_SHIELDS))
			{
				//not allowed to cloak, actually
				Saboteur_Decloak(self);
			}
			else if (!self->client->ps.powerups[PW_CLOAKED])
			{
				//cloak
				self->client->ps.powerups[PW_CLOAKED] = Q3_INFINITE;
				self->client->ps.powerups[PW_UNCLOAKING] = level.time + 2000;
				G_SoundOnEnt(self, CHAN_ITEM, "sound/chars/shadowtrooper/cloak.wav");
				NPC_SetAnim(self, SETANIM_TORSO, BOTH_FORCE_PROTECT_FAST, SETANIM_FLAG_OVERRIDE | SETANIM_FLAG_HOLD);
			}
		}
	}
}

//Local state enums
enum
{
	LSTATE_NONE = 0,
	LSTATE_UNDERFIRE,
	LSTATE_INVESTIGATE,
};

void ST_AggressionAdjust(gentity_t* self, int change)
{
	int upper_threshold, lower_threshold;

	self->NPC->stats.aggression += change;

	//FIXME: base this on initial NPC stats
	if (self->client->playerTeam == TEAM_PLAYER)
	{
		//good guys are less aggressive
		upper_threshold = 10;
		lower_threshold = 5;
	}
	else
	{
		//bad guys are more aggressive
		if (self->client->NPC_class == CLASS_IMPERIAL ||
			self->client->NPC_class == CLASS_RODIAN ||
			self->client->NPC_class == CLASS_TRANDOSHAN ||
			self->client->NPC_class == CLASS_TUSKEN ||
			self->client->NPC_class == CLASS_WEEQUAY ||
			self->client->NPC_class == CLASS_STORMTROOPER ||
			self->client->NPC_class == CLASS_CLONETROOPER ||
			self->client->NPC_class == CLASS_STORMCOMMANDO)
		{
			upper_threshold = 20;
			lower_threshold = 10;
		}
		else
		{
			upper_threshold = 15;
			lower_threshold = 5;
		}
	}

	if (self->NPC->stats.aggression > upper_threshold)
	{
		self->NPC->stats.aggression = upper_threshold;
	}
	else if (self->NPC->stats.aggression < lower_threshold)
	{
		self->NPC->stats.aggression = lower_threshold;
	}
}

void ST_ClearTimers(gentity_t* ent)
{
	TIMER_Set(ent, "chatter", 0);
	TIMER_Set(ent, "duck", 0);
	TIMER_Set(ent, "stand", 0);
	TIMER_Set(ent, "shuffleTime", 0);
	TIMER_Set(ent, "sleepTime", 0);
	TIMER_Set(ent, "enemyLastVisible", 0);
	TIMER_Set(ent, "roamTime", 0);
	TIMER_Set(ent, "hideTime", 0);
	TIMER_Set(ent, "attackDelay", 0); //FIXME: Slant for difficulty levels
	TIMER_Set(ent, "stick", 0);
	TIMER_Set(ent, "scoutTime", 0);
	TIMER_Set(ent, "flee", 0);
	TIMER_Set(ent, "interrogating", 0);
	TIMER_Set(ent, "verifyCP", 0);
	TIMER_Set(ent, "strafeRight", 0);
	TIMER_Set(ent, "strafeLeft", 0);
	TIMER_Set(ent, "slapattackDelay", 0);
	TIMER_Set(ent, "smackTime", 0);
}

enum
{
	SPEECH_CHASE,
	SPEECH_CONFUSED,
	SPEECH_COVER,
	SPEECH_DETECTED,
	SPEECH_GIVEUP,
	SPEECH_LOOK,
	SPEECH_LOST,
	SPEECH_OUTFLANK,
	SPEECH_ESCAPING,
	SPEECH_SIGHT,
	SPEECH_SOUND,
	SPEECH_SUSPICIOUS,
	SPEECH_YELL,
	SPEECH_PUSHED
};

qboolean NPC_IsGunner(gentity_t* self)
{
	switch (self->client->NPC_class)
	{
	case CLASS_BESPIN_COP:
	case CLASS_COMMANDO:
	case CLASS_GALAK:
	case CLASS_GRAN:
	case CLASS_IMPERIAL:
	case CLASS_IMPWORKER:
	case CLASS_JAN:
	case CLASS_LANDO:
	case CLASS_GALAKMECH:
	case CLASS_MONMOTHA:
	case CLASS_PRISONER:
	case CLASS_REBEL:
	case CLASS_REELO:
	case CLASS_RODIAN:
	case CLASS_STORMTROOPER:
	case CLASS_SWAMPTROOPER:
	case CLASS_SABOTEUR:
	case CLASS_TRANDOSHAN:
	case CLASS_UGNAUGHT:
	case CLASS_JAWA:
	case CLASS_WEEQUAY:
	case CLASS_BOBAFETT:
	case CLASS_MANDO:
	case CLASS_SBD:
	case CLASS_BATTLEDROID:
	case CLASS_STORMCOMMANDO:
	case CLASS_CLONETROOPER:
	case CLASS_TUSKEN:
	case CLASS_WOOKIE:
		return qtrue;
	default:
		break;
	}

	return qfalse;
}

static void ST_Speech(gentity_t* self, int speechType, float failChance)
{
	if (Q_flrand(0.0f, 1.0f) < failChance)
	{
		return;
	}

	if (failChance >= 0)
	{
		//a negative failChance makes it always talk
		if (self->NPC->group)
		{
			//group AI speech debounce timer
			if (self->NPC->group->speechDebounceTime > level.time)
			{
				return;
			}
		}
		else if (!TIMER_Done(self, "chatter"))
		{
			//personal timer
			return;
		}
		else if (groupSpeechDebounceTime[self->client->playerTeam] > level.time)
		{
			//for those not in group AI
			return;
		}
	}

	if (self->NPC->group)
	{
		//So they don't all speak at once...
		self->NPC->group->speechDebounceTime = level.time + Q_irand(2000, 4000);
	}
	else
	{
		TIMER_Set(self, "chatter", Q_irand(2000, 4000));
	}
	groupSpeechDebounceTime[self->client->playerTeam] = level.time + Q_irand(2000, 4000);

	if (self->NPC->blockedSpeechDebounceTime > level.time)
	{
		return;
	}

	switch (speechType)
	{
	case SPEECH_CHASE:
		G_AddVoiceEvent(self, Q_irand(EV_CHASE1, EV_CHASE3), 2000);
		break;
	case SPEECH_CONFUSED:
		G_AddVoiceEvent(self, Q_irand(EV_CONFUSE1, EV_CONFUSE3), 2000);
		break;
	case SPEECH_COVER:
		G_AddVoiceEvent(self, Q_irand(EV_COVER1, EV_COVER5), 2000);
		break;
	case SPEECH_DETECTED:
		G_AddVoiceEvent(self, Q_irand(EV_DETECTED1, EV_DETECTED5), 2000);
		break;
	case SPEECH_GIVEUP:
		G_AddVoiceEvent(self, Q_irand(EV_GIVEUP1, EV_GIVEUP4), 2000);
		break;
	case SPEECH_LOOK:
		G_AddVoiceEvent(self, Q_irand(EV_LOOK1, EV_LOOK2), 2000);
		break;
	case SPEECH_LOST:
		G_AddVoiceEvent(self, EV_LOST1, 2000);
		break;
	case SPEECH_OUTFLANK:
		G_AddVoiceEvent(self, Q_irand(EV_OUTFLANK1, EV_OUTFLANK2), 2000);
		break;
	case SPEECH_ESCAPING:
		G_AddVoiceEvent(self, Q_irand(EV_ESCAPING1, EV_ESCAPING3), 2000);
		break;
	case SPEECH_SIGHT:
		G_AddVoiceEvent(self, Q_irand(EV_SIGHT1, EV_SIGHT3), 2000);
		break;
	case SPEECH_SOUND:
		G_AddVoiceEvent(self, Q_irand(EV_SOUND1, EV_SOUND3), 2000);
		break;
	case SPEECH_SUSPICIOUS:
		G_AddVoiceEvent(self, Q_irand(EV_SUSPICIOUS1, EV_SUSPICIOUS5), 2000);
		break;
	case SPEECH_YELL:
		G_AddVoiceEvent(self, Q_irand(EV_ANGER1, EV_ANGER3), 2000);
		break;
	case SPEECH_PUSHED:
		G_AddVoiceEvent(self, Q_irand(EV_PUSHED1, EV_PUSHED3), 2000);
		break;
	default:
		break;
	}

	self->NPC->blockedSpeechDebounceTime = level.time + 2000;
}

void ST_MarkToCover(gentity_t* self)
{
	if (!self || !self->NPC)
	{
		return;
	}
	self->NPC->localState = LSTATE_UNDERFIRE;
	TIMER_Set(self, "attackDelay", Q_irand(500, 2500));
	ST_AggressionAdjust(self, -3);
	if (self->NPC->group && self->NPC->group->numGroup > 1)
	{
		ST_Speech(self, SPEECH_COVER, 0); //FIXME: flee sound?
	}
}

void ST_StartFlee(gentity_t* self, gentity_t* enemy, vec3_t dangerPoint, int dangerLevel, int minTime, int maxTime)
{
	if (!self || !self->NPC)
	{
		return;
	}
	G_StartFlee(self, enemy, dangerPoint, dangerLevel, minTime, maxTime);
	if (self->NPC->group && self->NPC->group->numGroup > 1)
	{
		ST_Speech(self, SPEECH_COVER, 0); //FIXME: flee sound?
	}
}

/*
-------------------------
NPC_ST_Pain
-------------------------
*/

void NPC_ST_Pain(gentity_t* self, gentity_t* inflictor, gentity_t* other, const vec3_t point, int damage, int mod,
	int hitLoc)
{
	self->NPC->localState = LSTATE_UNDERFIRE;

	TIMER_Set(self, "duck", -1);
	TIMER_Set(self, "hideTime", -1);
	TIMER_Set(self, "stand", 2000);

	NPC_Pain(self, inflictor, other, point, damage, mod, hitLoc);

	if (!damage && self->health > 0)
	{
		//FIXME: better way to know I was pushed
		G_AddVoiceEvent(self, Q_irand(EV_PUSHED1, EV_PUSHED3), 2000);
	}
}

/*
-------------------------
ST_HoldPosition
-------------------------
*/

static void ST_HoldPosition(void)
{
	if (NPCInfo->squadState == SQUAD_RETREAT)
	{
		TIMER_Set(NPC, "flee", -level.time);
	}
	TIMER_Set(NPC, "verifyCP", Q_irand(1000, 3000)); //don't look for another one for a few seconds
	NPC_FreeCombatPoint(NPCInfo->combatPoint, qtrue);
	//NPCInfo->combatPoint = -1;//???
	if (!Q3_TaskIDPending(NPC, TID_MOVE_NAV))
	{
		//don't have a script waiting for me to get to my point, okay to stop trying and stand
		AI_GroupUpdateSquadstates(NPCInfo->group, NPC, SQUAD_STAND_AND_SHOOT);
		NPCInfo->goalEntity = nullptr;
		if (NPC->health > 0)
		{
			//FIXME: better way to know I was pushed
			G_AddVoiceEvent(NPC, Q_irand(EV_LOOK1, EV_LOOK2), 2000);
		}
	}
}

void NPC_ST_SayMovementSpeech(void)
{
	if (!NPCInfo->movementSpeech)
	{
		return;
	}
	if (NPCInfo->group &&
		NPCInfo->group->commander &&
		NPCInfo->group->commander->client &&
		NPCInfo->group->commander->client->NPC_class == CLASS_IMPERIAL &&
		!Q_irand(0, 3))
	{
		//imperial (commander) gives the order
		ST_Speech(NPCInfo->group->commander, NPCInfo->movementSpeech, NPCInfo->movementSpeechChance);
	}
	else
	{
		//really don't want to say this unless we can actually get there...
		ST_Speech(NPC, NPCInfo->movementSpeech, NPCInfo->movementSpeechChance);
	}

	NPCInfo->movementSpeech = 0;
	NPCInfo->movementSpeechChance = 0.0f;
}

void NPC_ST_StoreMovementSpeech(int speech, float chance)
{
	NPCInfo->movementSpeech = speech;
	NPCInfo->movementSpeechChance = chance;
}

/*
-------------------------
ST_Move
-------------------------
*/
void ST_TransferMoveGoal(gentity_t* self, gentity_t* other);
extern void NAV_GetLastMove(navInfo_t& info);

static qboolean ST_Move(void)
{
	NPCInfo->combatMove = qtrue; //always move straight toward our goal

	const qboolean moved = NPC_MoveToGoal(qtrue);
	navInfo_t info;

	//Get the move info
	NAV_GetLastMove(info);

	//FIXME: if we bump into another one of our guys and can't get around him, just stop!
	//If we hit our target, then stop and fire!
	if (info.flags & NIF_COLLISION)
	{
		if (info.blocker == NPC->enemy)
		{
			ST_HoldPosition();
		}
	}

	//If our move failed, then reset
	if (moved == qfalse)
	{
		//FIXME: if we're going to a combat point, need to pick a different one
		if (!Q3_TaskIDPending(NPC, TID_MOVE_NAV))
		{
			//can't transfer movegoal or stop when a script we're running is waiting to complete
			if (info.blocker && info.blocker->NPC && NPCInfo->group != nullptr && info.blocker->NPC->group == NPCInfo->
				group) //(NPCInfo->aiFlags&NPCAI_BLOCKED) && NPCInfo->group != NULL )
			{
				//dammit, something is in our way
				//see if it's one of ours
				for (int j = 0; j < NPCInfo->group->numGroup; j++)
				{
					if (NPCInfo->group->member[j].number == NPCInfo->blockingEntNum)
					{
						//we're being blocked by one of our own, pass our goal onto them and I'll stand still
						ST_TransferMoveGoal(NPC, &g_entities[NPCInfo->group->member[j].number]);
						break;
					}
				}
			}

			ST_HoldPosition();
		}
	}
	else
	{
		//First time you successfully move, say what it is you're doing
		NPC_ST_SayMovementSpeech();
	}

	return moved;
}

/*
-------------------------
NPC_ST_SleepShuffle
-------------------------
*/

static void NPC_ST_SleepShuffle(void)
{
	//Play an awake script if we have one
	if (G_ActivateBehavior(NPC, BSET_AWAKE))
	{
		return;
	}

	//Automate some movement and noise
	if (TIMER_Done(NPC, "shuffleTime"))
	{
		TIMER_Set(NPC, "shuffleTime", 4000);
		TIMER_Set(NPC, "sleepTime", 2000);
		return;
	}

	//They made another noise while we were stirring, see if we can see them
	if (TIMER_Done(NPC, "sleepTime"))
	{
		NPC_CheckPlayerTeamStealth();
		TIMER_Set(NPC, "sleepTime", 2000);
	}
}

/*
-------------------------
NPC_ST_Sleep
-------------------------
*/

void NPC_BSST_Sleep(void)
{
	const int alertEvent = NPC_CheckAlertEvents(qfalse, qtrue); //only check sounds since we're alseep!

	//There is an event we heard
	if (alertEvent >= 0)
	{
		//See if it was enough to wake us up
		if (level.alertEvents[alertEvent].level == AEL_DISCOVERED && NPCInfo->scriptFlags & SCF_LOOK_FOR_ENEMIES)
		{
			if (&g_entities[0] && g_entities[0].health > 0)
			{
				G_SetEnemy(NPC, &g_entities[0]);
				return;
			}
		}

		//Otherwise just stir a bit
		NPC_ST_SleepShuffle();
	}
}

/*
-------------------------
NPC_CheckEnemyStealth
-------------------------
*/

qboolean NPC_CheckEnemyStealth(gentity_t* target)
{
	float minDist = 40; //any closer than 40 and we definitely notice

	//In case we aquired one some other way
	if (NPC->enemy != nullptr)
		return qtrue;

	//Ignore notarget
	if (target->flags & FL_NOTARGET)
		return qfalse;

	if (target->health <= 0)
	{
		return qfalse;
	}

	if (target->client->ps.weapon == WP_SABER && target->client->ps.SaberActive() && !target->client->ps.saberInFlight)
	{
		//if target has saber in hand and activated, we wake up even sooner even if not facing him
		minDist = 100;
	}

	float target_dist = DistanceSquared(target->currentOrigin, NPC->currentOrigin);
	//If the target is this close, then wake up regardless
	if (!(target->client->ps.pm_flags & PMF_DUCKED) //not ducking
		&& NPCInfo->scriptFlags & SCF_LOOK_FOR_ENEMIES //looking for enemies
		&& target_dist < minDist * minDist) //closer than minDist
	{
		G_SetEnemy(NPC, target);
		NPCInfo->enemyLastSeenTime = level.time;
		TIMER_Set(NPC, "attackDelay", Q_irand(500, 1500));
		return qtrue;
	}

	float maxViewDist = MAX_VIEW_DIST;

	if (NPCInfo->stats.visrange > maxViewDist)
	{
		//FIXME: should we always just set maxViewDist to this?
		maxViewDist = NPCInfo->stats.visrange;
	}

	if (target_dist > maxViewDist * maxViewDist)
	{
		//out of possible visRange
		return qfalse;
	}

	//Check FOV first
	if (InFOV(target, NPC, NPCInfo->stats.hfov, NPCInfo->stats.vfov) == qfalse)
		return qfalse;

	const qboolean clearLOS = target->client->ps.leanofs
		? NPC_ClearLOS(target->client->renderInfo.eyePoint)
		: NPC_ClearLOS(target);

	//Now check for clear line of vision
	if (clearLOS)
	{
		if (target->client->NPC_class == CLASS_ATST)
		{
			//can't miss 'em!
			G_SetEnemy(NPC, target);
			TIMER_Set(NPC, "attackDelay", Q_irand(500, 1500));
			return qtrue;
		}
		vec3_t targ_org = {
			target->currentOrigin[0], target->currentOrigin[1], target->currentOrigin[2] + target->maxs[2] - 4
		};
		float hAngle_perc = NPC_GetHFOVPercentage(targ_org, NPC->client->renderInfo.eyePoint,
			NPC->client->renderInfo.eyeAngles, NPCInfo->stats.hfov);
		float vAngle_perc = NPC_GetVFOVPercentage(targ_org, NPC->client->renderInfo.eyePoint,
			NPC->client->renderInfo.eyeAngles, NPCInfo->stats.vfov);

		//Scale them vertically some, and horizontally pretty harshly
		vAngle_perc *= vAngle_perc; //( vAngle_perc * vAngle_perc );
		hAngle_perc *= hAngle_perc * hAngle_perc;

		//Assess the player's current status
		target_dist = Distance(target->currentOrigin, NPC->currentOrigin);

		const float target_speed = VectorLength(target->client->ps.velocity);
		const int target_crouching = target->client->usercmd.upmove < 0;
		const float dist_rating = target_dist / maxViewDist;
		float speed_rating = target_speed / MAX_VIEW_SPEED;
		const float turning_rating = AngleDelta(target->client->ps.viewangles[PITCH], target->lastAngles[PITCH]) /
			180.0f + AngleDelta(target->client->ps.viewangles[YAW], target->lastAngles[YAW]) / 180.0f;
		const float light_level = target->lightLevel / MAX_LIGHT_INTENSITY;
		const float FOV_perc = 1.0f - (hAngle_perc + vAngle_perc) * 0.5f; //FIXME: Dunno about the average...
		float vis_rating = 0.0f;

		//Too dark
		if (light_level < MIN_LIGHT_THRESHOLD)
			return qfalse;

		//Too close?
		if (dist_rating < DISTANCE_THRESHOLD)
		{
			G_SetEnemy(NPC, target);
			TIMER_Set(NPC, "attackDelay", Q_irand(500, 1500));
			return qtrue;
		}

		//Out of range
		if (dist_rating > 1.0f)
			return qfalse;

		//Cap our speed checks
		if (speed_rating > 1.0f)
			speed_rating = 1.0f;

		//Calculate the distance, fov and light influences
		//...Visibilty linearly wanes over distance
		const float dist_influence = DISTANCE_SCALE * (1.0f - dist_rating);
		//...As the percentage out of the FOV increases, straight perception suffers on an exponential scale
		const float fov_influence = FOV_SCALE * (1.0f - FOV_perc);
		//...Lack of light hides, abundance of light exposes
		const float light_influence = (light_level - 0.5f) * LIGHT_SCALE;

		//Calculate our base rating
		float target_rating = dist_influence + fov_influence + light_influence;

		//Now award any final bonuses to this number
		const int contents = gi.pointcontents(targ_org, target->s.number);
		if (contents & CONTENTS_WATER)
		{
			const int myContents = gi.pointcontents(NPC->client->renderInfo.eyePoint, NPC->s.number);
			if (!(myContents & CONTENTS_WATER))
			{
				//I'm not in water
				if (NPC->client->NPC_class == CLASS_SWAMPTROOPER || NPC->client->NPC_class == CLASS_CLONETROOPER)
				{
					//these guys can see in in/through water pretty well
					vis_rating = 0.10f; //10% bonus
				}
				else
				{
					vis_rating = 0.35f; //35% bonus
				}
			}
			else
			{
				//else, if we're both in water
				if (NPC->client->NPC_class == CLASS_SWAMPTROOPER || NPC->client->NPC_class == CLASS_CLONETROOPER)
				{
					//I can see him just fine
				}
				else
				{
					vis_rating = 0.15f; //15% bonus
				}
			}
		}
		else
		{
			//not in water
			if (contents & CONTENTS_FOG)
			{
				vis_rating = 0.15f; //15% bonus
			}
		}

		target_rating *= 1.0f - vis_rating;

		//...Motion draws the eye quickly
		target_rating += speed_rating * SPEED_SCALE;
		target_rating += turning_rating * TURNING_SCALE;
		//FIXME: check to see if they're animating, too?  But can we do something as simple as frame != oldframe?

		//...Smaller targets are harder to indentify
		if (target_crouching)
		{
			target_rating *= 0.9f; //10% bonus
		}

		//If he's violated the threshold, then realize him
		float realize, cautious;
		if (NPC->client->NPC_class == CLASS_SWAMPTROOPER || NPC->client->NPC_class == CLASS_CLONETROOPER)
		{
			//swamptroopers can see much better
			realize = static_cast<float>(CAUTIOUS_THRESHOLD
				/**difficulty_scale*/)/**difficulty_scale*/;
			cautious = static_cast<float>(CAUTIOUS_THRESHOLD) * 0.75f/**difficulty_scale*/;
		}
		else
		{
			realize = static_cast<float>(REALIZE_THRESHOLD/**difficulty_scale*/)/**difficulty_scale*/;
			cautious = static_cast<float>(CAUTIOUS_THRESHOLD) * 0.75f/**difficulty_scale*/;
		}

		if (target_rating > realize && NPCInfo->scriptFlags & SCF_LOOK_FOR_ENEMIES)
		{
			G_SetEnemy(NPC, target);
			NPCInfo->enemyLastSeenTime = level.time;
			TIMER_Set(NPC, "attackDelay", Q_irand(500, 1500));
			return qtrue;
		}

		//If he's above the caution threshold, then realize him in a few seconds unless he moves to cover
		if (target_rating > cautious && !(NPCInfo->scriptFlags & SCF_IGNORE_ALERTS))
		{
			//FIXME: ambushing guys should never talk
			if (TIMER_Done(NPC, "enemyLastVisible"))
			{
				//If we haven't already, start the counter
				const int lookTime = Q_irand(4500, 8500);
				//NPCInfo->timeEnemyLastVisible = level.time + 2000;
				TIMER_Set(NPC, "enemyLastVisible", lookTime);
				//TODO: Play a sound along the lines of, "Huh?  What was that?"
				ST_Speech(NPC, SPEECH_SIGHT, 0);
				NPC_TempLookTarget(NPC, target->s.number, lookTime, lookTime);
				//FIXME: set desired yaw and pitch towards this guy?
			}
			else if (TIMER_Get(NPC, "enemyLastVisible") <= level.time + 500 && NPCInfo->scriptFlags &
				SCF_LOOK_FOR_ENEMIES) //FIXME: Is this reliable?
			{
				if (NPCInfo->rank < RANK_LT && !Q_irand(0, 2))
				{
					const int interrogateTime = Q_irand(2000, 4000);
					ST_Speech(NPC, SPEECH_SUSPICIOUS, 0);
					TIMER_Set(NPC, "interrogating", interrogateTime);
					G_SetEnemy(NPC, target);
					NPCInfo->enemyLastSeenTime = level.time;
					TIMER_Set(NPC, "attackDelay", interrogateTime);
					TIMER_Set(NPC, "stand", interrogateTime);
				}
				else
				{
					G_SetEnemy(NPC, target);
					NPCInfo->enemyLastSeenTime = level.time;
					//FIXME: ambush guys (like those popping out of water) shouldn't delay...
					TIMER_Set(NPC, "attackDelay", Q_irand(500, 1500));
					TIMER_Set(NPC, "stand", Q_irand(500, 2500));
				}
				return qtrue;
			}

			return qfalse;
		}
	}

	return qfalse;
}

qboolean NPC_CheckPlayerTeamStealth(void)
{
	for (int i = 0; i < ENTITYNUM_WORLD; i++)
	{
		if (!PInUse(i))
			continue;
		gentity_t* enemy = &g_entities[i];
		if (enemy && enemy->client && NPC_ValidEnemy(enemy))
		{
			if (NPC_CheckEnemyStealth(enemy)) //Change this pointer to assess other entities
			{
				return qtrue;
			}
		}
	}
	return qfalse;
}

qboolean NPC_CheckEnemiesInSpotlight(void)
{
	gentity_t* entityList[MAX_GENTITIES];
	gentity_t* suspect = nullptr;
	int i;
	vec3_t mins, maxs;

	for (i = 0; i < 3; i++)
	{
		mins[i] = NPC->client->renderInfo.eyePoint[i] - NPC->speed;
		maxs[i] = NPC->client->renderInfo.eyePoint[i] + NPC->speed;
	}

	const int numListedEntities = gi.EntitiesInBox(mins, maxs, entityList, MAX_GENTITIES);

	for (i = 0; i < numListedEntities; i++)
	{
		if (!PInUse(i))
			continue;

		gentity_t* enemy = entityList[i];

		if (enemy && enemy->client && NPC_ValidEnemy(enemy) && enemy->client->playerTeam == NPC->client->enemyTeam)
		{
			//valid ent & client, valid enemy, on the target team
			//check to see if they're in my FOV
			if (InFOV(enemy->currentOrigin, NPC->client->renderInfo.eyePoint, NPC->client->renderInfo.eyeAngles,
				NPCInfo->stats.hfov, NPCInfo->stats.vfov))
			{
				//in my cone
				//check to see that they're close enough
				if (DistanceSquared(NPC->client->renderInfo.eyePoint, enemy->currentOrigin) - 256
					/*fudge factor: 16 squared*/ <= NPC->speed * NPC->speed)
				{
					//within range
					//check to see if we have a clear trace to them
					if (G_ClearLOS(NPC, enemy))
					{
						//clear LOS
						G_SetEnemy(NPC, enemy);
						TIMER_Set(NPC, "attackDelay", Q_irand(500, 1500));
						return qtrue;
					}
				}
			}
			if (InFOV(enemy->currentOrigin, NPC->client->renderInfo.eyePoint, NPC->client->renderInfo.eyeAngles, 90,
				NPCInfo->stats.vfov * 3))
			{
				//one to look at if we don't get an enemy
				if (G_ClearLOS(NPC, enemy))
				{
					//clear LOS
					if (suspect == nullptr || DistanceSquared(NPC->client->renderInfo.eyePoint, enemy->currentOrigin) <
						DistanceSquared(NPC->client->renderInfo.eyePoint, suspect->currentOrigin))
					{
						//remember him
						suspect = enemy;
					}
				}
			}
		}
	}
	if (suspect && Q_flrand(0, NPCInfo->stats.visrange * NPCInfo->stats.visrange) > DistanceSquared(
		NPC->client->renderInfo.eyePoint, suspect->currentOrigin))
	{
		//hey!  who's that?
		if (TIMER_Done(NPC, "enemyLastVisible"))
		{
			//If we haven't already, start the counter
			const int lookTime = Q_irand(4500, 8500);
			TIMER_Set(NPC, "enemyLastVisible", lookTime);
			ST_Speech(NPC, SPEECH_SIGHT, 0);
			NPC_FacePosition(suspect->currentOrigin, qtrue);
		}
		else if (TIMER_Get(NPC, "enemyLastVisible") <= level.time + 500
			&& NPCInfo->scriptFlags & SCF_LOOK_FOR_ENEMIES) //FIXME: Is this reliable?
		{
			if (!Q_irand(0, 2))
			{
				const int interrogateTime = Q_irand(2000, 4000);
				ST_Speech(NPC, SPEECH_SUSPICIOUS, 0);
				TIMER_Set(NPC, "interrogating", interrogateTime);
				NPC_FacePosition(suspect->currentOrigin, qtrue);
			}
		}
	}
	return qfalse;
}

/*
-------------------------
NPC_ST_InvestigateEvent
-------------------------
*/

constexpr auto MAX_CHECK_THRESHOLD = 1;

static qboolean NPC_ST_InvestigateEvent(int eventID, bool extraSuspicious)
{
	//If they've given themselves away, just take them as an enemy
	if (NPCInfo->confusionTime < level.time && NPCInfo->insanityTime < level.time)
	{
		if (level.alertEvents[eventID].level == AEL_DISCOVERED && NPCInfo->scriptFlags & SCF_LOOK_FOR_ENEMIES)
		{
			if (!level.alertEvents[eventID].owner ||
				!level.alertEvents[eventID].owner->client ||
				level.alertEvents[eventID].owner->health <= 0 ||
				level.alertEvents[eventID].owner->client->playerTeam != NPC->client->enemyTeam)
			{
				//not an enemy
				return qfalse;
			}
			//FIXME: what if can't actually see enemy, don't know where he is... should we make them just become very alert and start looking for him?  Or just let combat AI handle this... (act as if you lost him)
			ST_Speech(NPC, SPEECH_CHASE, 0);
			G_SetEnemy(NPC, level.alertEvents[eventID].owner);
			NPCInfo->enemyLastSeenTime = level.time;
			TIMER_Set(NPC, "attackDelay", Q_irand(500, 1500));
			if (level.alertEvents[eventID].type == AET_SOUND)
			{
				//heard him, didn't see him, stick for a bit
				TIMER_Set(NPC, "roamTime", Q_irand(500, 2500));
			}
			return qtrue;
		}
	}

	//don't look at the same alert twice

	if (level.alertEvents[eventID].ID == NPCInfo->lastAlertID)
	{
		return qfalse;
	}
	NPCInfo->lastAlertID = level.alertEvents[eventID].ID;

	if (level.alertEvents[eventID].type == AET_SIGHT)
	{
		//sight alert, check the light level
		if (level.alertEvents[eventID].light < Q_irand(ST_MIN_LIGHT_THRESHOLD, ST_MAX_LIGHT_THRESHOLD))
		{
			//below my threshhold of potentially seeing
			return qfalse;
		}
	}

	//Save the position for movement (if necessary)
	VectorCopy(level.alertEvents[eventID].position, NPCInfo->investigateGoal);

	//First awareness of it
	NPCInfo->investigateCount += extraSuspicious ? 2 : 1;

	//Clamp the value
	if (NPCInfo->investigateCount > 4)
		NPCInfo->investigateCount = 4;

	//See if we should walk over and investigate
	if (level.alertEvents[eventID].level > AEL_MINOR && NPCInfo->investigateCount > 1 && NPCInfo->scriptFlags &
		SCF_CHASE_ENEMIES)
	{
		//make it so they can walk right to this point and look at it rather than having to use combatPoints
		if (G_ExpandPointToBBox(NPCInfo->investigateGoal, NPC->mins, NPC->maxs, NPC->s.number,
			NPC->clipmask & ~CONTENTS_BODY | CONTENTS_BOTCLIP))
		{
			//we were able to doMove the investigateGoal to a point in which our bbox would fit
			//drop the goal to the ground so we can get at it
			vec3_t end;
			trace_t trace;
			VectorCopy(NPCInfo->investigateGoal, end);
			end[2] -= 512; //FIXME: not always right?  What if it's even higher, somehow?
			gi.trace(&trace, NPCInfo->investigateGoal, NPC->mins, NPC->maxs, end, ENTITYNUM_NONE,
				NPC->clipmask & ~CONTENTS_BODY | CONTENTS_BOTCLIP, static_cast<EG2_Collision>(0), 0);
			if (trace.fraction >= 1.0f)
			{
				//too high to even bother
				//FIXME: look at them???
			}
			else
			{
				VectorCopy(trace.endpos, NPCInfo->investigateGoal);
				NPC_SetMoveGoal(NPC, NPCInfo->investigateGoal, 16, qtrue);
				NPCInfo->localState = LSTATE_INVESTIGATE;
			}
		}
		else
		{
			const int id = NPC_FindCombatPoint(NPCInfo->investigateGoal, NPCInfo->investigateGoal,
				NPCInfo->investigateGoal, CP_INVESTIGATE | CP_HAS_ROUTE, 0);

			if (id != -1)
			{
				NPC_SetMoveGoal(NPC, level.combatPoints[id].origin, 16, qtrue, id);
				NPCInfo->localState = LSTATE_INVESTIGATE;
			}
		}
		//Say something
		//FIXME: only if have others in group... these should be responses?
		if (NPCInfo->investigateDebounceTime + NPCInfo->pauseTime > level.time)
		{
			//was already investigating
			if (NPCInfo->group &&
				NPCInfo->group->commander &&
				NPCInfo->group->commander->client &&
				NPCInfo->group->commander->client->NPC_class == CLASS_IMPERIAL &&
				!Q_irand(0, 3))
			{
				ST_Speech(NPCInfo->group->commander, SPEECH_LOOK, 0); //FIXME: "I'll go check it out" type sounds
			}
			else
			{
				ST_Speech(NPC, SPEECH_LOOK, 0); //FIXME: "I'll go check it out" type sounds
			}
		}
		else
		{
			if (level.alertEvents[eventID].type == AET_SIGHT)
			{
				ST_Speech(NPC, SPEECH_SIGHT, 0);
			}
			else if (level.alertEvents[eventID].type == AET_SOUND)
			{
				ST_Speech(NPC, SPEECH_SOUND, 0);
			}
		}
		//Setup the debounce info
		NPCInfo->investigateDebounceTime = NPCInfo->investigateCount * 5000;
		NPCInfo->investigateSoundDebounceTime = level.time + 2000;
		NPCInfo->pauseTime = level.time;
	}
	else
	{
		//just look?
		//Say something
		if (level.alertEvents[eventID].type == AET_SIGHT)
		{
			ST_Speech(NPC, SPEECH_SIGHT, 0);
		}
		else if (level.alertEvents[eventID].type == AET_SOUND)
		{
			ST_Speech(NPC, SPEECH_SOUND, 0);
		}
		//Setup the debounce info
		NPCInfo->investigateDebounceTime = NPCInfo->investigateCount * 1000;
		NPCInfo->investigateSoundDebounceTime = level.time + 1000;
		NPCInfo->pauseTime = level.time;
		VectorCopy(level.alertEvents[eventID].position, NPCInfo->investigateGoal);

		if (NPC->client->NPC_class == CLASS_ROCKETTROOPER && !RT_Flying(NPC))
		{
			NPC_SetAnim(NPC, SETANIM_BOTH, BOTH_GUARD_LOOKAROUND1, SETANIM_FLAG_OVERRIDE | SETANIM_FLAG_HOLD);
		}
	}

	if (level.alertEvents[eventID].level >= AEL_DANGER)
	{
		NPCInfo->investigateDebounceTime = Q_irand(500, 2500);
	}

	//Start investigating
	NPCInfo->tempBehavior = BS_INVESTIGATE;
	return qtrue;
}

/*
-------------------------
ST_OffsetLook
-------------------------
*/

static void ST_OffsetLook(float offset, vec3_t out)
{
	vec3_t angles, forward, temp;

	GetAnglesForDirection(NPC->currentOrigin, NPCInfo->investigateGoal, angles);
	angles[YAW] += offset;
	AngleVectors(angles, forward, nullptr, nullptr);
	VectorMA(NPC->currentOrigin, 64, forward, out);

	CalcEntitySpot(NPC, SPOT_HEAD, temp);
	out[2] = temp[2];
}

/*
-------------------------
ST_LookAround
-------------------------
*/

static void ST_LookAround(void)
{
	vec3_t lookPos;
	const float perc = static_cast<float>(level.time - NPCInfo->pauseTime) / static_cast<float>(NPCInfo->
		investigateDebounceTime);

	//Keep looking at the spot
	if (perc < 0.25)
	{
		VectorCopy(NPCInfo->investigateGoal, lookPos);
	}
	else if (perc < 0.5f) //Look up but straight ahead
	{
		ST_OffsetLook(0.0f, lookPos);
	}
	else if (perc < 0.75f) //Look right
	{
		ST_OffsetLook(45.0f, lookPos);
	}
	else //Look left
	{
		ST_OffsetLook(-45.0f, lookPos);
	}

	NPC_FacePosition(lookPos);
}

/*
-------------------------
NPC_BSST_Investigate
-------------------------
*/

void NPC_BSST_Investigate(void)
{
	//get group- mainly for group speech debouncing, but may use for group scouting/investigating AI, too
	AI_GetGroup(NPC);

	if (NPCInfo->scriptFlags & SCF_FIRE_WEAPON)
	{
		WeaponThink(qtrue);
	}

	if (NPCInfo->confusionTime < level.time && NPCInfo->insanityTime < level.time)
	{
		if (NPCInfo->scriptFlags & SCF_LOOK_FOR_ENEMIES)
		{
			//Look for an enemy
			if (NPC_CheckPlayerTeamStealth())
			{
				ST_Speech(NPC, SPEECH_DETECTED, 0);
				NPCInfo->tempBehavior = BS_DEFAULT;
				NPC_UpdateAngles(qtrue, qtrue);
				return;
			}
		}
	}

	if (!(NPCInfo->scriptFlags & SCF_IGNORE_ALERTS))
	{
		const int alertEvent = NPC_CheckAlertEvents(qtrue, qtrue, NPCInfo->lastAlertID);

		//There is an event to look at
		if (alertEvent >= 0)
		{
			if (NPCInfo->confusionTime < level.time && NPCInfo->insanityTime < level.time)
			{
				if (NPC_CheckForDanger(alertEvent))
				{
					//running like hell
					ST_Speech(NPC, SPEECH_COVER, 0); //FIXME: flee sound?
					return;
				}
			}

			if (level.alertEvents[alertEvent].ID != NPCInfo->lastAlertID)
			{
				NPC_ST_InvestigateEvent(alertEvent, qtrue);
			}
		}
	}

	//If we're done looking, then just return to what we were doing
	if (NPCInfo->investigateDebounceTime + NPCInfo->pauseTime < level.time)
	{
		NPCInfo->tempBehavior = BS_DEFAULT;
		NPCInfo->goalEntity = UpdateGoal();

		NPC_UpdateAngles(qtrue, qtrue);
		//Say something
		ST_Speech(NPC, SPEECH_GIVEUP, 0);
		return;
	}

	//FIXME: else, look for new alerts

	//See if we're searching for the noise's origin
	if (NPCInfo->localState == LSTATE_INVESTIGATE && NPCInfo->goalEntity != nullptr)
	{
		//See if we're there
		if (!STEER::Reached(NPC, NPCInfo->goalEntity, 32, FlyingCreature(NPC) != qfalse))
		{
			ucmd.buttons |= BUTTON_WALKING;

			//Try and doMove there
			if (NPC_MoveToGoal(qtrue))
			{
				//Bump our times
				NPCInfo->investigateDebounceTime = NPCInfo->investigateCount * 5000;
				NPCInfo->pauseTime = level.time;

				NPC_UpdateAngles(qtrue, qtrue);
				return;
			}
		}

		//Otherwise we're done or have given up
		//Say something
		ST_Speech(NPC, SPEECH_LOOK, 0.33f);
		NPCInfo->localState = LSTATE_NONE;
	}

	//Look around
	ST_LookAround();
}

/*
-------------------------
NPC_BSST_Patrol
-------------------------
*/

void NPC_BSST_Patrol(void)
{
	//FIXME: pick up on bodies of dead buddies?
	//Not a scriptflag, but...
	if (NPC->client->NPC_class == CLASS_ROCKETTROOPER && NPC->client->ps.eFlags & EF_SPOTLIGHT)
	{
		//using spotlight search mode
		vec3_t eyeFwd, end, mins = { -2, -2, -2 }, maxs = { 2, 2, 2 };
		trace_t trace;
		AngleVectors(NPC->client->renderInfo.eyeAngles, eyeFwd, nullptr, nullptr);
		VectorMA(NPC->client->renderInfo.eyePoint, NPCInfo->stats.visrange, eyeFwd, end);
		//get server-side trace impact point
		gi.trace(&trace, NPC->client->renderInfo.eyePoint, mins, maxs, end, NPC->s.number,
			MASK_OPAQUE | CONTENTS_BODY | CONTENTS_CORPSE, static_cast<EG2_Collision>(0), 0);
		NPC->speed = trace.fraction * NPCInfo->stats.visrange;
		if (NPCInfo->scriptFlags & SCF_LOOK_FOR_ENEMIES)
		{
			//FIXME: do a FOV cone check, then a trace
			if (trace.entityNum < ENTITYNUM_WORLD)
			{
				//hit something
				//try cheap check first
				gentity_t* enemy = &g_entities[trace.entityNum];
				if (enemy && enemy->client && NPC_ValidEnemy(enemy) && enemy->client->playerTeam == NPC->client->
					enemyTeam)
				{
					G_SetEnemy(NPC, enemy);
					TIMER_Set(NPC, "attackDelay", Q_irand(500, 1500));
					NPC_AngerSound();
					NPC_UpdateAngles(qtrue, qtrue);
					return;
				}
			}
			//FIXME: maybe do a quick check of ents within the spotlight's radius?
			//hmmm, look around
			if (NPC_CheckEnemiesInSpotlight())
			{
				NPC_AngerSound();
				NPC_UpdateAngles(qtrue, qtrue);
				return;
			}
		}
	}
	else
	{
		//get group- mainly for group speech debouncing, but may use for group scouting/investigating AI, too
		AI_GetGroup(NPC);

		if (NPCInfo->confusionTime < level.time && NPCInfo->insanityTime < level.time)
		{
			//Look for any enemies
			if (NPCInfo->scriptFlags & SCF_LOOK_FOR_ENEMIES)
			{
				if (NPC_CheckPlayerTeamStealth())
				{
					NPC_AngerSound();
					NPC_UpdateAngles(qtrue, qtrue);
					return;
				}
			}
		}
	}

	if (!(NPCInfo->scriptFlags & SCF_IGNORE_ALERTS))
	{
		const int alertEvent = NPC_CheckAlertEvents(qtrue, qtrue);

		//There is an event to look at
		if (alertEvent >= 0)
		{
			if (NPC_CheckForDanger(alertEvent))
			{
				//going to run?
				ST_Speech(NPC, SPEECH_COVER, 0);
				return;
			}
			if (NPC->client->NPC_class == CLASS_BOBAFETT /*|| NPC->client->NPC_class == CLASS_MANDO*/)
			{
				if (!level.alertEvents[alertEvent].owner ||
					!level.alertEvents[alertEvent].owner->client ||
					level.alertEvents[alertEvent].owner->health <= 0 ||
					level.alertEvents[alertEvent].owner->client->playerTeam != NPC->client->enemyTeam)
				{
					//not an enemy
					return;
				}
				G_SetEnemy(NPC, level.alertEvents[alertEvent].owner);
				NPCInfo->enemyLastSeenTime = level.time;
				TIMER_Set(NPC, "attackDelay", Q_irand(500, 1500));
				return;
			}
			if (NPC_ST_InvestigateEvent(alertEvent, qfalse))
			{
				//actually going to investigate it
				NPC_UpdateAngles(qtrue, qtrue);
				return;
			}
		}
	}

	//If we have somewhere to go, then do that
	if (UpdateGoal())
	{
		ucmd.buttons |= BUTTON_WALKING;
		NPC_MoveToGoal(qtrue);
	}
	else
	{
		if (TIMER_Done(NPC, "enemyLastVisible"))
		{
			//nothing suspicious, look around
			if (!Q_irand(0, 30))
			{
				NPCInfo->desiredYaw = NPC->s.angles[1] + Q_irand(-90, 90);
			}
			if (!Q_irand(0, 30))
			{
				NPCInfo->desiredPitch = Q_irand(-20, 20);
			}
		}
	}

	NPC_UpdateAngles(qtrue, qtrue);
	//TEMP hack for Imperial stand anim
	if (NPC->client->NPC_class == CLASS_IMPERIAL || NPC->client->NPC_class == CLASS_IMPWORKER)
	{
		//hack
		if (NPC->client->ps.weapon != WP_CONCUSSION)
		{
			//not Rax
			if (ucmd.forwardmove || ucmd.rightmove || ucmd.upmove)
			{
				//moving
				if (!NPC->client->ps.torsoAnimTimer || NPC->client->ps.torsoAnim == BOTH_STAND4)
				{
					if (ucmd.buttons & BUTTON_WALKING && !(NPCInfo->scriptFlags & SCF_RUNNING))
					{
						//not running, only set upper anim
						//  No longer overrides scripted anims
						NPC_SetAnim(NPC, SETANIM_TORSO, BOTH_STAND4, SETANIM_FLAG_OVERRIDE | SETANIM_FLAG_HOLD);
						NPC->client->ps.torsoAnimTimer = 200;
					}
				}
			}
			else
			{
				//standing still, set both torso and legs anim
				//  No longer overrides scripted anims
				if ((!NPC->client->ps.torsoAnimTimer || NPC->client->ps.torsoAnim == BOTH_STAND4) &&
					(!NPC->client->ps.legsAnimTimer || NPC->client->ps.legsAnim == BOTH_STAND4))
				{
					NPC_SetAnim(NPC, SETANIM_BOTH, BOTH_STAND4, SETANIM_FLAG_OVERRIDE | SETANIM_FLAG_HOLD);
					NPC->client->ps.torsoAnimTimer = NPC->client->ps.legsAnimTimer = 200;
				}
			}
			//FIXME: this is a disgusting hack that is supposed to make the Imperials start with their weapon holstered- need a better way
			if (NPC->client->ps.weapon != WP_NONE)
			{
				ChangeWeapon(NPC, WP_NONE);
				NPC->client->ps.weapon = WP_NONE;
				NPC->client->ps.weaponstate = WEAPON_READY;
				G_RemoveWeaponModels(NPC);
			}
		}
	}
}

/*
-------------------------
ST_CheckMoveState
-------------------------
*/

static void ST_CheckMoveState(void)
{
	if (Q3_TaskIDPending(NPC, TID_MOVE_NAV))
	{
		//moving toward a goal that a script is waiting on, so don't stop for anything!
		doMove = qtrue;
	}
	else if (NPC->client->NPC_class == CLASS_ROCKETTROOPER
		&& NPC->client->ps.groundEntityNum == ENTITYNUM_NONE)
	{
		//no squad stuff
		return;
	}
	else if (NPCInfo->squadState == SQUAD_SCOUT)
	{
		//If we're supposed to stay put, then stand there and fire
		if (TIMER_Done(NPC, "stick") == qfalse)
		{
			doMove = qfalse;
			return;
		}

		//Otherwise, if we can see our target, just shoot
		if (enemyLOS)
		{
			if (enemyCS)
			{
				//if we're going after our enemy, we can stop now
				if (NPCInfo->goalEntity == NPC->enemy)
				{
					AI_GroupUpdateSquadstates(NPCInfo->group, NPC, SQUAD_STAND_AND_SHOOT);
					doMove = qfalse;
					return;
				}
			}
		}
		else
		{
			//Move to find our target
			faceEnemy = qfalse;
		}
	}
	//See if we're running away
	else if (NPCInfo->squadState == SQUAD_RETREAT)
	{
		if (NPCInfo->goalEntity)
		{
			faceEnemy = qfalse;
		}
		else
		{
			//um, lost our goal?  Just stand and shoot, then
			NPCInfo->squadState = SQUAD_STAND_AND_SHOOT;
		}
	}
	//see if we're heading to some other combatPoint
	else if (NPCInfo->squadState == SQUAD_TRANSITION)
	{
		//ucmd.buttons |= BUTTON_CAREFUL;
		if (!NPCInfo->goalEntity)
		{
			//um, lost our goal?  Just stand and shoot, then
			NPCInfo->squadState = SQUAD_STAND_AND_SHOOT;
		}
	}
	//see if we're at point, duck and fire
	else if (NPCInfo->squadState == SQUAD_POINT)
	{
		if (TIMER_Done(NPC, "stick"))
		{
			AI_GroupUpdateSquadstates(NPCInfo->group, NPC, SQUAD_STAND_AND_SHOOT);
			return;
		}

		doMove = qfalse;
		return;
	}
	//see if we're just standing around
	else if (NPCInfo->squadState == SQUAD_STAND_AND_SHOOT)
	{
		//from this squadState we can transition to others?
		doMove = qfalse;
		return;
	}
	//see if we're hiding
	else if (NPCInfo->squadState == SQUAD_COVER)
	{
		//Should we duck?
		doMove = qfalse;
		return;
	}
	//see if we're just standing around
	else if (NPCInfo->squadState == SQUAD_IDLE)
	{
		if (!NPCInfo->goalEntity)
		{
			doMove = qfalse;
			return;
		}
	}
	else
	{
		doMove = qtrue;
	}
	//See if we're a scout

	//See if we're moving towards a goal, not the enemy
	if (NPCInfo->goalEntity != NPC->enemy && NPCInfo->goalEntity != nullptr)
	{
		//Did we make it?
		if (STEER::Reached(NPC, NPCInfo->goalEntity, 16, !!FlyingCreature(NPC)) ||
			enemyLOS && NPCInfo->aiFlags & NPCAI_STOP_AT_LOS && !Q3_TaskIDPending(NPC, TID_MOVE_NAV)
			)
		{
			//either hit our navgoal or our navgoal was not a crucial (scripted) one (maybe a combat point) and we're scouting and found our enemy
			int newSquadState = SQUAD_STAND_AND_SHOOT;
			//we got where we wanted to go, set timers based on why we were running
			switch (NPCInfo->squadState)
			{
			case SQUAD_RETREAT: //was running away
				//done fleeing, obviously
				TIMER_Set(NPC, "duck", (NPC->max_health - NPC->health) * 100);
				TIMER_Set(NPC, "hideTime", Q_irand(3000, 7000));
				TIMER_Set(NPC, "flee", -level.time);
				newSquadState = SQUAD_COVER;
				break;
			case SQUAD_TRANSITION: //was heading for a combat point
				TIMER_Set(NPC, "hideTime", Q_irand(2000, 4000));
				break;
			case SQUAD_SCOUT: //was running after player
				break;
			default:
				break;
			}
			AI_GroupUpdateSquadstates(NPCInfo->group, NPC, newSquadState);
			NPC_ReachedGoal();
			//don't attack right away
			TIMER_Set(NPC, "attackDelay", Q_irand(250, 500)); //FIXME: Slant for difficulty levels
			//don't do something else just yet

			// THIS IS THE ONE TRUE PLACE WHERE ROAM TIME IS SET
			TIMER_Set(NPC, "roamTime", Q_irand(8000, 15000)); //Q_irand( 1000, 4000 ) );
			if (Q_irand(0, 3) == 0)
			{
				TIMER_Set(NPC, "duck", Q_irand(5000, 10000)); // just reached our goal, chance of ducking now
			}
			return;
		}

		//keep going, hold of roamTimer until we get there
		TIMER_Set(NPC, "roamTime", Q_irand(8000, 9000));
	}
}

void ST_ResolveBlockedShot(int hit)
{
	int stuckTime;
	//figure out how long we intend to stand here, max
	if (TIMER_Get(NPC, "roamTime") > TIMER_Get(NPC, "stick"))
	{
		stuckTime = TIMER_Get(NPC, "roamTime") - level.time;
	}
	else
	{
		stuckTime = TIMER_Get(NPC, "stick") - level.time;
	}

	if (TIMER_Done(NPC, "duck"))
	{
		//we're not ducking
		if (AI_GroupContainsEntNum(NPCInfo->group, hit))
		{
			gentity_t* member = &g_entities[hit];
			if (TIMER_Done(member, "duck"))
			{
				//they aren't ducking
				if (TIMER_Done(member, "stand"))
				{
					//they're not being forced to stand
					//tell them to duck at least as long as I'm not moving
					TIMER_Set(member, "duck", stuckTime); // tell my friend to duck so I can shoot over his head
					return;
				}
			}
		}
	}
	else
	{
		//maybe we should stand
		if (TIMER_Done(NPC, "stand"))
		{
			//stand for as long as we'll be here
			TIMER_Set(NPC, "stand", stuckTime);
			return;
		}
	}
	//Hmm, can't resolve this by telling them to duck or telling me to stand
	//We need to doMove!
	TIMER_Set(NPC, "roamTime", -1);
	TIMER_Set(NPC, "stick", -1);
	TIMER_Set(NPC, "duck", -1);
	TIMER_Set(NPC, "attakDelay", Q_irand(1000, 3000));
}

/*
-------------------------
ST_CheckFireState
-------------------------
*/

static void ST_CheckFireState(void)
{
	if (enemyCS)
	{
		//if have a clear shot, always try
		return;
	}

	if (NPCInfo->squadState == SQUAD_RETREAT || NPCInfo->squadState == SQUAD_TRANSITION || NPCInfo->squadState ==
		SQUAD_SCOUT)
	{
		//runners never try to fire at the last pos
		return;
	}

	if (!VectorCompare(NPC->client->ps.velocity, vec3_origin))
	{
		//if moving at all, don't do this
		return;
	}

	//See if we should continue to fire on their last position
	if (!hitAlly //we're not going to hit an ally
		&& enemyInFOV //enemy is in our FOV //FIXME: or we don't have a clear LOS?
		&& NPCInfo->enemyLastSeenTime > 0 //we've seen the enemy
		&& NPCInfo->group //have a group
		&& (NPCInfo->group->numState[SQUAD_RETREAT] > 0 || NPCInfo->group->numState[SQUAD_TRANSITION] > 0 || NPCInfo->
			group->numState[SQUAD_SCOUT] > 0)) //laying down covering fire
	{
		if (level.time - NPCInfo->enemyLastSeenTime < 10000 && //we have seem the enemy in the last 10 seconds
			(!NPCInfo->group || level.time - NPCInfo->group->lastSeenEnemyTime < 10000))
			//we are not in a group or the group has seen the enemy in the last 10 seconds
		{
			if (!Q_irand(0, 10))
			{
				//Fire on the last known position
				vec3_t muzzle, dir, angles;
				qboolean tooClose = qfalse;
				qboolean tooFar = qfalse;

				CalcEntitySpot(NPC, SPOT_HEAD, muzzle);
				if (VectorCompare(impactPos, vec3_origin))
				{
					//never checked ShotEntity this frame, so must do a trace...
					trace_t tr;
					vec3_t forward, end;
					AngleVectors(NPC->client->ps.viewangles, forward, nullptr, nullptr);
					VectorMA(muzzle, 8192, forward, end);
					gi.trace(&tr, muzzle, vec3_origin, vec3_origin, end, NPC->s.number, MASK_SHOT,
						static_cast<EG2_Collision>(0), 0);
					VectorCopy(tr.endpos, impactPos);
				}

				//see if impact would be too close to me
				float distThreshold = 16384; //default
				switch (NPC->s.weapon)
				{
				case WP_ROCKET_LAUNCHER:
				case WP_FLECHETTE:
				case WP_THERMAL:
				case WP_TRIP_MINE:
				case WP_DET_PACK:
					distThreshold = 65536/*256*256*/;
					break;
				case WP_REPEATER:
					if (NPCInfo->scriptFlags & SCF_ALT_FIRE)
					{
						distThreshold = 65536/*256*256*/;
					}
					break;
				case WP_CONCUSSION:
					if (!(NPCInfo->scriptFlags & SCF_ALT_FIRE))
					{
						distThreshold = 65536/*256*256*/;
					}
					break;
				default:
					break;
				}

				float dist = DistanceSquared(impactPos, muzzle);

				if (dist < distThreshold)
				{
					//impact would be too close to me
					tooClose = qtrue;
				}
				else if (level.time - NPCInfo->enemyLastSeenTime > 5000 ||
					NPCInfo->group && level.time - NPCInfo->group->lastSeenEnemyTime > 5000)
				{
					//we've haven't seen them in the last 5 seconds
					//see if it's too far from where he is
					distThreshold = 65536/*256*256*/; //default
					switch (NPC->s.weapon)
					{
					case WP_ROCKET_LAUNCHER:
					case WP_FLECHETTE:
					case WP_THERMAL:
					case WP_TRIP_MINE:
					case WP_DET_PACK:
						distThreshold = 262144/*512*512*/;
						break;
					case WP_REPEATER:
						if (NPCInfo->scriptFlags & SCF_ALT_FIRE)
						{
							distThreshold = 262144/*512*512*/;
						}
						break;
					case WP_CONCUSSION:
						if (!(NPCInfo->scriptFlags & SCF_ALT_FIRE))
						{
							distThreshold = 262144/*512*512*/;
						}
						break;
					default:
						break;
					}
					dist = DistanceSquared(impactPos, NPCInfo->enemyLastSeenLocation);
					if (dist > distThreshold)
					{
						//impact would be too far from enemy
						tooFar = qtrue;
					}
				}

				if (!tooClose && !tooFar)
				{
					//okay too shoot at last pos
					VectorSubtract(NPCInfo->enemyLastSeenLocation, muzzle, dir);
					VectorNormalize(dir);
					vectoangles(dir, angles);

					NPCInfo->desiredYaw = angles[YAW];
					NPCInfo->desiredPitch = angles[PITCH];

					shoot = qtrue;
					faceEnemy = qfalse;
				}
			}
		}
	}
}

void ST_TrackEnemy(gentity_t* self, vec3_t enemyPos)
{
	//clear timers
	TIMER_Set(self, "attackDelay", Q_irand(1000, 2000));
	TIMER_Set(self, "stick", Q_irand(500, 1500));
	TIMER_Set(self, "stand", -1);
	TIMER_Set(self, "scoutTime", TIMER_Get(self, "stick") - level.time + Q_irand(5000, 10000));
	//leave my combat point
	NPC_FreeCombatPoint(self->NPC->combatPoint);
	//go after his last seen pos
	NPC_SetMoveGoal(self, enemyPos, 100.0f, qfalse);
	if (Q_irand(0, 3) == 0)
	{
		NPCInfo->aiFlags |= NPCAI_STOP_AT_LOS;
	}
}

int ST_ApproachEnemy(gentity_t* self)
{
	TIMER_Set(self, "attackDelay", Q_irand(250, 500));
	TIMER_Set(self, "stick", Q_irand(1000, 2000));
	TIMER_Set(self, "stand", -1);
	TIMER_Set(self, "scoutTime", TIMER_Get(self, "stick") - level.time + Q_irand(5000, 10000));
	//leave my combat point
	NPC_FreeCombatPoint(self->NPC->combatPoint);
	//return the relevant combat point flags
	return CP_CLEAR | CP_CLOSEST;
}

void ST_HuntEnemy(gentity_t* self)
{
	TIMER_Set(NPC, "stick", Q_irand(250, 1000));
	TIMER_Set(NPC, "stand", -1);
	TIMER_Set(NPC, "scoutTime", TIMER_Get(NPC, "stick") - level.time + Q_irand(5000, 10000));
	//leave my combat point
	NPC_FreeCombatPoint(NPCInfo->combatPoint);
	//go directly after the enemy
	if (NPCInfo->scriptFlags & SCF_CHASE_ENEMIES)
	{
		self->NPC->goalEntity = NPC->enemy;
	}
}

void ST_TransferTimers(gentity_t* self, gentity_t* other)
{
	TIMER_Set(other, "attackDelay", TIMER_Get(self, "attackDelay") - level.time);
	TIMER_Set(other, "duck", TIMER_Get(self, "duck") - level.time);
	TIMER_Set(other, "stick", TIMER_Get(self, "stick") - level.time);
	TIMER_Set(other, "scoutTime", TIMER_Get(self, "scoutTime") - level.time);
	TIMER_Set(other, "roamTime", TIMER_Get(self, "roamTime") - level.time);
	TIMER_Set(other, "stand", TIMER_Get(self, "stand") - level.time);
	TIMER_Set(self, "attackDelay", -1);
	TIMER_Set(self, "duck", -1);
	TIMER_Set(self, "stick", -1);
	TIMER_Set(self, "scoutTime", -1);
	TIMER_Set(self, "roamTime", -1);
	TIMER_Set(self, "stand", -1);
}

void ST_TransferMoveGoal(gentity_t* self, gentity_t* other)
{
	if (Q3_TaskIDPending(self, TID_MOVE_NAV))
	{
		//can't transfer movegoal when a script we're running is waiting to complete
		return;
	}
	if (self->NPC->combatPoint != -1)
	{
		//I've got a combatPoint I'm going to, give it to him
		self->NPC->lastFailedCombatPoint = other->NPC->combatPoint = self->NPC->combatPoint;
		self->NPC->combatPoint = -1;
	}
	else
	{
		//I must be going for a goal, give that to him instead
		if (self->NPC->goalEntity == self->NPC->tempGoal)
		{
			NPC_SetMoveGoal(other, self->NPC->tempGoal->currentOrigin, self->NPC->goalRadius,
				static_cast<qboolean>((self->NPC->tempGoal->svFlags & SVF_NAVGOAL) != 0));
		}
		else
		{
			other->NPC->goalEntity = self->NPC->goalEntity;
		}
	}
	//give him my squadstate
	AI_GroupUpdateSquadstates(self->NPC->group, other, NPCInfo->squadState);

	//give him my timers and clear mine
	ST_TransferTimers(self, other);

	//now make me stand around for a second or two at least
	AI_GroupUpdateSquadstates(self->NPC->group, self, SQUAD_STAND_AND_SHOOT);
	TIMER_Set(self, "stand", Q_irand(1000, 3000));
}

int ST_GetCPFlags(void)
{
	int cpFlags = 0;
	if (NPC && NPCInfo->group)
	{
		if (NPC == NPCInfo->group->commander && NPC->client->NPC_class == CLASS_IMPERIAL)
		{
			//imperials hang back and give orders
			if (NPCInfo->group->numGroup > 1 && Q_irand(-3, NPCInfo->group->numGroup) > 1)
			{
				//FIXME: make sure he;s giving orders with these lines
				if (Q_irand(0, 1))
				{
					ST_Speech(NPC, SPEECH_CHASE, 0.5);
				}
				else
				{
					ST_Speech(NPC, SPEECH_YELL, 0.5);
				}
			}
			cpFlags = CP_CLEAR | CP_COVER | CP_AVOID | CP_SAFE | CP_RETREAT;
		}
		else if (NPCInfo->group->morale < 0)
		{
			//hide
			cpFlags = CP_COVER | CP_AVOID | CP_SAFE | CP_RETREAT;
		}
		else if (NPCInfo->group->morale < NPCInfo->group->numGroup)
		{
			//morale is low for our size
			const int moraleDrop = NPCInfo->group->numGroup - NPCInfo->group->morale;
			if (moraleDrop < -6)
			{
				//flee (no clear shot needed)
				cpFlags = CP_FLEE | CP_RETREAT | CP_COVER | CP_AVOID | CP_SAFE;
			}
			else if (moraleDrop < -3)
			{
				//retreat (no clear shot needed)
				cpFlags = CP_RETREAT | CP_COVER | CP_AVOID | CP_SAFE;
			}
			else if (moraleDrop < 0)
			{
				//cover (no clear shot needed)
				cpFlags = CP_COVER | CP_AVOID | CP_SAFE;
			}
		}
		else
		{
			const int moraleBoost = NPCInfo->group->morale - NPCInfo->group->numGroup;
			if (moraleBoost > 20)
			{
				//charge to any one and outflank (no cover needed)
				cpFlags = CP_CLEAR | CP_FLANK | CP_APPROACH_ENEMY;
				//Saboteur_Decloak( NPC );
			}
			else if (moraleBoost > 15)
			{
				//charge to closest one (no cover needed)
				cpFlags = CP_CLEAR | CP_CLOSEST | CP_APPROACH_ENEMY;
			}
			else if (moraleBoost > 10)
			{
				//charge closer (no cover needed)
				cpFlags = CP_CLEAR | CP_APPROACH_ENEMY;
			}
		}
	}
	if (!cpFlags)
	{
		//at some medium level of morale
		switch (Q_irand(0, 3))
		{
		case 0: //just take the nearest one
			cpFlags = CP_CLEAR | CP_COVER | CP_NEAREST;
			break;
		case 1: //take one closer to the enemy
			cpFlags = CP_CLEAR | CP_COVER | CP_APPROACH_ENEMY;
			break;
		case 2: //take the one closest to the enemy
			cpFlags = CP_CLEAR | CP_COVER | CP_CLOSEST | CP_APPROACH_ENEMY;
			break;
		case 3: //take the one on the other side of the enemy
			cpFlags = CP_CLEAR | CP_COVER | CP_FLANK | CP_APPROACH_ENEMY;
			break;
		default:;
		}
	}
	if (NPC && NPCInfo->scriptFlags & SCF_USE_CP_NEAREST)
	{
		cpFlags &= ~(CP_FLANK | CP_APPROACH_ENEMY | CP_CLOSEST);
		cpFlags |= CP_NEAREST;
	}
	return cpFlags;
}

/*
-------------------------
ST_Commander

  Make decisions about who should go where, etc.

FIXME: leader (group-decision-making) AI?
FIXME: need alternate routes!
FIXME: more group voice interaction
FIXME: work in pairs?

-------------------------
*/
void ST_Commander(void)
{
	int i, j;
	int cp, cpFlags;
	AIGroupInfo_t* group = NPCInfo->group;
	gentity_t* member; //, *buddy;
	qboolean runner = qfalse;
	qboolean enemyLost = qfalse;
	qboolean enemyProtected = qfalse;
	float avoidDist;
	int squadState;

	group->processed = qtrue;

	if (group->enemy == nullptr || group->enemy->client == nullptr)
	{
		//hmm, no enemy...?!
		return;
	}

	//FIXME: have this group commander check the enemy group (if any) and see if they have
	//		superior numbers.  If they do, fall back rather than advance.  If you have
	//		superior numbers, advance on them.
	//FIXME: find the group commander and have him occasionally give orders when there is speech
	//FIXME: start fleeing when only a couple of you vs. a lightsaber, possibly give up if the only one left

	SaveNPCGlobals();

	if (group->lastSeenEnemyTime < level.time - 180000)
	{
		//dissolve the group
		ST_Speech(NPC, SPEECH_LOST, 0.0f);
		group->enemy->waypoint = NAV::GetNearestNode(group->enemy);
		for (i = 0; i < group->numGroup; i++)
		{
			member = &g_entities[group->member[i].number];
			SetNPCGlobals(member);
			if (Q3_TaskIDPending(NPC, TID_MOVE_NAV))
			{
				//running somewhere that a script requires us to go, don't break from that
				continue;
			}
			if (!(NPCInfo->scriptFlags & SCF_CHASE_ENEMIES))
			{
				//not allowed to doMove on my own
				continue;
			}
			//Lost enemy for three minutes?  go into search mode?
			G_ClearEnemy(NPC);
			NPC->waypoint = NAV::GetNearestNode(group->enemy);
			if (NPC->waypoint == WAYPOINT_NONE)
			{
				NPCInfo->behaviorState = BS_DEFAULT; //BS_PATROL;
			}
			else if (group->enemy->waypoint == WAYPOINT_NONE || NAV::EstimateCostToGoal(
				NPC->waypoint, group->enemy->waypoint) >= Q3_INFINITE)
			{
				NPC_BSSearchStart(NPC->waypoint, BS_SEARCH);
			}
			else
			{
				NPC_BSSearchStart(group->enemy->waypoint, BS_SEARCH);
			}
		}
		group->enemy = nullptr;
		RestoreNPCGlobals();
		return;
	}

	if (group->numState[SQUAD_SCOUT] > 0 ||
		group->numState[SQUAD_TRANSITION] > 0 ||
		group->numState[SQUAD_RETREAT] > 0)
	{
		//someone is running
		runner = qtrue;
	}

	if (group->lastSeenEnemyTime > level.time - 32000 && group->lastSeenEnemyTime < level.time - 30000)
	{
		//no-one has seen the enemy for 30 seconds// and no-one is running after him
		if (group->commander && !Q_irand(0, 1))
		{
			ST_Speech(group->commander, SPEECH_ESCAPING, 0.0f);
		}
		else
		{
			ST_Speech(NPC, SPEECH_ESCAPING, 0.0f);
		}
		//don't say this again
		NPCInfo->blockedSpeechDebounceTime = level.time + 3000;
	}

	if (group->lastSeenEnemyTime < level.time - 7000)
	{
		//no-one has seen the enemy for at least 10 seconds!  Should send a scout
		enemyLost = qtrue;
	}

	if (group->lastClearShotTime < level.time - 5000)
	{
		//no-one has had a clear shot for 5 seconds!
		enemyProtected = qtrue;
	}

	//Go through the list:

	//Everyone should try to get to a combat point if possible
	int curMemberNum, lastMemberNum;
	if (d_asynchronousGroupAI->integer)
	{
		//do one member a turn
		group->activeMemberNum++;
		if (group->activeMemberNum >= group->numGroup)
		{
			group->activeMemberNum = 0;
		}
		curMemberNum = group->activeMemberNum;
		lastMemberNum = curMemberNum + 1;
	}
	else
	{
		curMemberNum = 0;
		lastMemberNum = group->numGroup;
	}
	for (i = curMemberNum; i < lastMemberNum; i++)
	{
		//reset combat point flags
		cp = -1;
		cpFlags = 0;
		squadState = SQUAD_IDLE;
		avoidDist = 0;
		avoidDist = 0;

		//get the next guy
		member = &g_entities[group->member[i].number];
		if (!member->enemy)
		{
			//don't include guys that aren't angry
			continue;
		}
		SetNPCGlobals(member);

		if (!TIMER_Done(NPC, "flee"))
		{
			//running away
			continue;
		}

		if (Q3_TaskIDPending(NPC, TID_MOVE_NAV))
		{
			//running somewhere that a script requires us to go
			continue;
		}

		if (NPC->s.weapon == WP_NONE
			&& NPCInfo->goalEntity
			&& NPCInfo->goalEntity == NPCInfo->tempGoal
			&& NPCInfo->goalEntity->s.eType == ET_ITEM)
		{
			//running to pick up a gun, don't do other logic
			continue;
		}

		//see if this member should start running (only if have no officer... FIXME: should always run from AEL_DANGER_GREAT?)
		if (!group->commander || group->commander->NPC->rank < RANK_ENSIGN)
		{
			if (NPC_CheckForDanger(NPC_CheckAlertEvents(qtrue, qtrue, -1, qfalse, AEL_DANGER)))
			{
				//going to run
				ST_Speech(NPC, SPEECH_COVER, 0);
				continue;
			}
		}

		if (!(NPCInfo->scriptFlags & SCF_CHASE_ENEMIES))
		{
			//not allowed to do combat-movement
			continue;
		}

		if (NPCInfo->squadState != SQUAD_RETREAT)
		{
			//not already retreating
			if (NPC->client->ps.weapon == WP_NONE)
			{
				//weaponless, should be hiding
				if (NPCInfo->goalEntity == nullptr || NPCInfo->goalEntity->enemy == nullptr || NPCInfo->goalEntity->
					enemy->s.eType != ET_ITEM)
				{
					//not running after a pickup
					if (TIMER_Done(NPC, "hideTime") || DistanceSquared(group->enemy->currentOrigin, NPC->currentOrigin)
						< 65536 && NPC_ClearLOS(NPC->enemy))
					{
						//done hiding or enemy near and can see us
						//er, start another flee I guess?
						NPC_StartFlee(NPC->enemy, NPC->enemy->currentOrigin, AEL_DANGER_GREAT, 5000, 10000);
					} //else, just hang here
				}
				continue;
			}
			if (TIMER_Done(NPC, "roamTime") && TIMER_Done(NPC, "hideTime") && NPC->health > 10 && !gi.inPVS(
				group->enemy->currentOrigin, NPC->currentOrigin))
			{
				//cant even see enemy
				//better go after him
				cpFlags |= CP_CLEAR | CP_COVER;
			}
			else if (NPCInfo->localState == LSTATE_UNDERFIRE)
			{
				//we've been shot
				switch (group->enemy->client->ps.weapon)
				{
				case WP_SABER:
					if (DistanceSquared(group->enemy->currentOrigin, NPC->currentOrigin) < 65536) //256 squared
					{
						cpFlags |= CP_AVOID_ENEMY | CP_COVER | CP_AVOID | CP_RETREAT;
						if (!group->commander || group->commander->NPC->rank < RANK_ENSIGN)
						{
							squadState = SQUAD_RETREAT;
						}
						avoidDist = 256;
					}
					break;
				default:
				case WP_BLASTER:
					cpFlags |= CP_COVER;
					break;
				}
				if (NPC->health <= 10)
				{
					if (!group->commander || group->commander->NPC && group->commander->NPC->rank < RANK_ENSIGN)
					{
						cpFlags |= CP_FLEE | CP_AVOID | CP_RETREAT;
						squadState = SQUAD_RETREAT;
					}
				}
			}
			else
			{
				//not hit, see if there are other reasons we should run
				if (gi.inPVS(NPC->currentOrigin, group->enemy->currentOrigin))
				{
					//in the same room as enemy
					if (NPC->client->ps.weapon == WP_ROCKET_LAUNCHER &&
						DistanceSquared(group->enemy->currentOrigin, NPC->currentOrigin) < MIN_ROCKET_DIST_SQUARED &&
						NPCInfo->squadState != SQUAD_TRANSITION)
					{
						//too close for me to fire my weapon and I'm not already on the move
						cpFlags |= CP_AVOID_ENEMY | CP_CLEAR | CP_AVOID;
						avoidDist = 256;
					}
					else
					{
						switch (group->enemy->client->ps.weapon)
						{
						case WP_SABER:
							if (group->enemy->client->ps.SaberLength() > 0)
							{
								if (DistanceSquared(group->enemy->currentOrigin, NPC->currentOrigin) < 65536)
								{
									if (TIMER_Done(NPC, "hideTime"))
									{
										if (NPCInfo->squadState != SQUAD_TRANSITION)
										{
											//not already moving: FIXME: we need to see if where we're going is good now?
											cpFlags |= CP_AVOID_ENEMY | CP_CLEAR | CP_AVOID;
											avoidDist = 256;
										}
									}
								}
							}
						default:
							break;
						}
					}
				}
			}
		}

		if (!cpFlags)
		{
			//okay, we have no new enemy-driven reason to run... let's use tactics now
			if (runner && NPCInfo->combatPoint != -1)
			{
				//someone is running and we have a combat point already
				if (NPCInfo->squadState != SQUAD_SCOUT &&
					NPCInfo->squadState != SQUAD_TRANSITION &&
					NPCInfo->squadState != SQUAD_RETREAT)
				{
					//it's not us
					if (TIMER_Done(NPC, "verifyCP") && DistanceSquared(NPC->currentOrigin,
						level.combatPoints[NPCInfo->combatPoint].origin)
								> 64 * 64)
					{
						//1 - 3 seconds have passed since you chose a CP, see if you're there since, for some reason, you've stopped running...
						//uh, WTF, we're not on our combat point?
						//er, try again, I guess?
						cp = NPCInfo->combatPoint;
						cpFlags |= ST_GetCPFlags();
					}
					else
					{
						//cover them
						//stop ducking
						TIMER_Set(NPC, "duck", -1);
						//start shooting
						TIMER_Set(NPC, "attackDelay", -1);
						//AI should take care of the rest - fire at enemy
					}
				}
				else
				{
					//we're running
					//see if we're blocked
					if (NPCInfo->aiFlags & NPCAI_BLOCKED)
					{
						//dammit, something is in our way
						//see if it's one of ours
						for (j = 0; j < group->numGroup; j++)
						{
							if (group->member[j].number == NPCInfo->blockingEntNum)
							{
								//we're being blocked by one of our own, pass our goal onto them and I'll stand still
								ST_TransferMoveGoal(NPC, &g_entities[group->member[j].number]);
								break;
							}
						}
					}
					//we don't need to do anything else
					continue;
				}
			}
			else
			{
				//okay no-one is running, use some tactics
				if (NPCInfo->combatPoint != -1)
				{
					//we have a combat point we're supposed to be running to
					if (NPCInfo->squadState != SQUAD_SCOUT &&
						NPCInfo->squadState != SQUAD_TRANSITION &&
						NPCInfo->squadState != SQUAD_RETREAT)
					{
						//but we're not running
						if (TIMER_Done(NPC, "verifyCP"))
						{
							//1 - 3 seconds have passed since you chose a CP, see if you're there since, for some reason, you've stopped running...
							if (DistanceSquared(NPC->currentOrigin,
								level.combatPoints[NPCInfo->combatPoint].origin) > 64 * 64)
							{
								//uh, WTF, we're not on our combat point?
								//er, try again, I guess?
								cp = NPCInfo->combatPoint;
								cpFlags |= ST_GetCPFlags();
							}
						}
					}
				}
				if (enemyLost)
				{
					//if no-one has seen the enemy for a while, send a scout
					//ask where he went
					if (group->numState[SQUAD_SCOUT] <= 0)
					{
						//scouting = qtrue;
						NPC_ST_StoreMovementSpeech(SPEECH_CHASE, 0.0f);
					}
					//Since no-one else has done this, I should be the closest one, so go after him...
					ST_TrackEnemy(NPC, group->enemyLastSeenPos);
					//set me into scout mode
					AI_GroupUpdateSquadstates(group, NPC, SQUAD_SCOUT);
					//we're not using a cp, so we need to set runner to true right here
					runner = qtrue;
				}
				else if (enemyProtected)
				{
					//if no-one has a clear shot at the enemy, someone should go after him
					//FIXME: if I'm in an area where no safe combat points have a clear shot at me, they don't come after me... they should anyway, though after some extra hesitation.
					//ALSO: seem to give up when behind an area portal?
					//since no-one else here has done this, I should be the closest one
					if (TIMER_Done(NPC, "roamTime") && !Q_irand(0, group->numGroup))
					{
						//only do this if we're ready to move again and we feel like it
						cpFlags |= ST_ApproachEnemy(NPC);
						//set me into scout mode
						AI_GroupUpdateSquadstates(group, NPC, SQUAD_SCOUT);
					}
				}
				else
				{
					//group can see and has been shooting at the enemy
					//see if we should do something fancy?

					{
						//we're ready to move
						if (NPCInfo->combatPoint == -1)
						{
							//we're not on a combat point
							if (true) //!Q_irand( 0, 2 ) )
							{
								//we should go for a combat point
								cpFlags |= ST_GetCPFlags();
							}
						}
						else if (TIMER_Done(NPC, "roamTime"))
						{
							//we are already on a combat point
							if (i == 0)
							{
								//we're the closest
								if (group->morale - group->numGroup > 0 && !Q_irand(0, 4))
								{
									//try to outflank him
									cpFlags |= CP_CLEAR | CP_COVER | CP_FLANK | CP_APPROACH_ENEMY;
								}
								else if (group->morale - group->numGroup < 0)
								{
									//better move!
									cpFlags |= ST_GetCPFlags();
								}
								else
								{
									//If we're point, then get down
									TIMER_Set(NPC, "roamTime", Q_irand(2000, 5000));
									TIMER_Set(NPC, "stick", Q_irand(2000, 5000));
									//FIXME: what if we can't shoot from a ducked pos?
									TIMER_Set(NPC, "duck", Q_irand(3000, 4000));
									AI_GroupUpdateSquadstates(group, NPC, SQUAD_POINT);
								}
							}
							else if (i == group->numGroup - 1)
							{
								//farthest from the enemy
								if (group->morale - group->numGroup < 0)
								{
									//low morale, just hang here
									TIMER_Set(NPC, "roamTime", Q_irand(2000, 5000));
									TIMER_Set(NPC, "stick", Q_irand(2000, 5000));
								}
								else if (group->morale - group->numGroup > 0)
								{
									//try to move in on the enemy
									cpFlags |= ST_ApproachEnemy(NPC);
									//set me into scout mode
									AI_GroupUpdateSquadstates(group, NPC, SQUAD_SCOUT);
								}
								else
								{
									//use normal decision making process
									cpFlags |= ST_GetCPFlags();
								}
							}
							else
							{
								//someone in-between
								if (group->morale - group->numGroup < 0 || !Q_irand(0, 4))
								{
									//do something
									cpFlags |= ST_GetCPFlags();
								}
								else
								{
									TIMER_Set(NPC, "stick", Q_irand(2000, 4000));
									TIMER_Set(NPC, "roamTime", Q_irand(2000, 4000));
								}
							}
						}
					}
					if (!cpFlags)
					{
						//still not moving
						//see if we should do other fun stuff
						if (NPC->attackDebounceTime < level.time - 2000)
						{
							//we, personally, haven't shot for 2 seconds
							//maybe yell at the enemy?
							ST_Speech(NPC, SPEECH_LOOK, 0.9f);
						}
						//toy with ducking
						if (TIMER_Done(NPC, "duck"))
						{
							//not ducking
							if (TIMER_Done(NPC, "stand"))
							{
								//don't have to keep standing
								if (NPCInfo->combatPoint == -1 || level.combatPoints[NPCInfo->combatPoint].flags &
									CPF_DUCK)
								{
									//okay to duck here
									if (!Q_irand(0, 3))
									{
										TIMER_Set(NPC, "duck", Q_irand(1000, 3000));
									}
								}
							}
						}
					}
				}
			}
		}

		if (enemyLost && NAV::InSameRegion(NPC, NPC->enemy->currentOrigin))
		{
			ST_TrackEnemy(NPC, NPC->enemy->currentOrigin);
			continue;
		}

		if (!NPC->enemy)
		{
			continue;
		}

		// Check To See We Have A Clear Shot To The Enemy Every Couple Seconds
		//---------------------------------------------------------------------
		if (TIMER_Done(NPC, "checkGrenadeTooCloseDebouncer"))
		{
			TIMER_Set(NPC, "checkGrenadeTooCloseDebouncer", Q_irand(300, 600));

			vec3_t mins;
			vec3_t maxs;
			bool fled = false;
			gentity_t* ent;

			gentity_t* entityList[MAX_GENTITIES];

			for (int i1 = 0; i1 < 3; i1++)
			{
				mins[i1] = NPC->currentOrigin[i1] - 200;
				maxs[i1] = NPC->currentOrigin[i1] + 200;
			}

			int numListedEntities = gi.EntitiesInBox(mins, maxs, entityList, MAX_GENTITIES);

			for (int e = 0; e < numListedEntities; e++)
			{
				ent = entityList[e];

				if (ent == NPC)
					continue;
				if (ent->owner == NPC)
					continue;
				if (!ent->inuse)
					continue;
				if (ent->s.eType == ET_MISSILE)
				{
					if (ent->s.weapon == WP_THERMAL)
					{
						//a thermal
						if (ent->has_bounced && (!ent->owner || !OnSameTeam(ent->owner, NPC)))
						{
							//bounced and an enemy thermal
							ST_Speech(NPC, SPEECH_COVER, 0); //FIXME: flee sound?
							NPC_StartFlee(NPC->enemy, ent->currentOrigin, AEL_DANGER_GREAT, 1000, 2000);
							fled = true;
							//							cpFlags |= (CP_CLEAR|CP_COVER);	// NOPE, Can't See The Enemy, So Find A New Combat Point
							TIMER_Set(NPC, "checkGrenadeTooCloseDebouncer", Q_irand(2000, 4000));
							break;
						}
					}
				}
			}
			if (fled)
			{
				continue;
			}
		}

		// Check To See We Have A Clear Shot To The Enemy Every Couple Seconds
		//---------------------------------------------------------------------
		if (TIMER_Done(NPC, "checkEnemyVisDebouncer"))
		{
			TIMER_Set(NPC, "checkEnemyVisDebouncer", Q_irand(3000, 7000));
			if (!NPC_ClearLOS(NPC->enemy))
			{
				cpFlags |= CP_CLEAR | CP_COVER; // NOPE, Can't See The Enemy, So Find A New Combat Point
			}
		}

		// Check To See If The Enemy Is Too Close For Comfort
		//----------------------------------------------------
		if (NPC->client->NPC_class != CLASS_ASSASSIN_DROID)
		{
			if (TIMER_Done(NPC, "checkEnemyTooCloseDebouncer"))
			{
				TIMER_Set(NPC, "checkEnemyTooCloseDebouncer", Q_irand(1000, 6000));

				float distThreshold = 16384/*128*128*/; //default
				switch (NPC->s.weapon)
				{
				case WP_ROCKET_LAUNCHER:
				case WP_FLECHETTE:
				case WP_THERMAL:
				case WP_TRIP_MINE:
				case WP_DET_PACK:
					distThreshold = 65536/*256*256*/;
					break;
				case WP_REPEATER:
					if (NPCInfo->scriptFlags & SCF_ALT_FIRE)
					{
						distThreshold = 65536/*256*256*/;
					}
					break;
				case WP_CONCUSSION:
					if (!(NPCInfo->scriptFlags & SCF_ALT_FIRE))
					{
						distThreshold = 65536/*256*256*/;
					}
					break;
				default:
					break;
				}

				if (DistanceSquared(group->enemy->currentOrigin, NPC->currentOrigin) < distThreshold)
				{
					cpFlags |= CP_CLEAR | CP_COVER;
				}
			}
		}

		//clear the local state
		NPCInfo->localState = LSTATE_NONE;

		cpFlags &= ~CP_NEAREST;
		//Assign combat points
		if (cpFlags)
		{
			//we want to run to a combat point
			//always avoid enemy when picking combat points, and we always want to be able to get there

			if (group->enemy->client->ps.weapon == WP_SABER && group->enemy->client->ps.SaberLength() > 0)
			{
				//we obviously want to avoid the enemy if he has a saber
				cpFlags |= CP_AVOID_ENEMY;
				avoidDist = 256;
			}
			else
			{
				cpFlags |= CP_AVOID_ENEMY | CP_HAS_ROUTE | CP_TRYFAR;
				avoidDist = 200;
			}

			//now get a combat point
			if (cp == -1)
			{
				//may have had sone set above
				cp = NPC_FindCombatPointRetry(NPC->currentOrigin, NPC->currentOrigin, NPC->currentOrigin, &cpFlags,
					avoidDist, NPCInfo->lastFailedCombatPoint);
			}
			while (cp == -1 && cpFlags != CP_ANY)
			{
				//start "OR"ing out certain flags to see if we can find *any* point
				if (cpFlags & CP_INVESTIGATE)
				{
					//don't need to investigate
					cpFlags &= ~CP_INVESTIGATE;
				}
				else if (cpFlags & CP_SQUAD)
				{
					//don't need to stick to squads
					cpFlags &= ~CP_SQUAD;
				}
				else if (cpFlags & CP_DUCK)
				{
					//don't need to duck
					cpFlags &= ~CP_DUCK;
				}
				else if (cpFlags & CP_NEAREST)
				{
					//don't need closest one to me
					cpFlags &= ~CP_NEAREST;
				}
				else if (cpFlags & CP_FLANK)
				{
					//don't need to flank enemy
					cpFlags &= ~CP_FLANK;
				}
				else if (cpFlags & CP_SAFE)
				{
					//don't need one that hasn't been shot at recently
					cpFlags &= ~CP_SAFE;
				}
				else if (cpFlags & CP_CLOSEST)
				{
					//don't need to get closest to enemy
					cpFlags &= ~CP_CLOSEST;
					//but let's try to approach at least
					cpFlags |= CP_APPROACH_ENEMY;
				}
				else if (cpFlags & CP_APPROACH_ENEMY)
				{
					//don't need to approach enemy
					cpFlags &= ~CP_APPROACH_ENEMY;
				}
				else if (cpFlags & CP_COVER)
				{
					//don't need cover
					cpFlags &= ~CP_COVER;
					//but let's pick one that makes us duck
					cpFlags |= CP_DUCK;
				}
				else if (cpFlags & CP_CLEAR)
				{
					//don't need a clear shot to enemy
					cpFlags &= ~CP_CLEAR;
				}
				else if (cpFlags & CP_AVOID_ENEMY)
				{
					//don't need to avoid enemy
					cpFlags &= ~CP_AVOID_ENEMY;
				}
				else if (cpFlags & CP_RETREAT)
				{
					//don't need to retreat
					cpFlags &= ~CP_RETREAT;
				}
				else if (cpFlags & CP_FLEE)
				{
					//don't need to flee
					cpFlags &= ~CP_FLEE;
					//but at least avoid enemy and pick one that gives cover
					cpFlags |= CP_COVER | CP_AVOID_ENEMY;
				}
				else if (cpFlags & CP_AVOID)
				{
					//okay, even pick one right by me
					cpFlags &= ~CP_AVOID;
				}
				else
				{
					cpFlags = CP_ANY;
				}
				//now try again
				cp = NPC_FindCombatPoint(NPC->currentOrigin, NPC->currentOrigin, group->enemy->currentOrigin,
					cpFlags | CP_HAS_ROUTE, avoidDist);
			}

			//see if we got a valid one
			if (cp != -1)
			{
				//found a combat point
				//let others know that someone is now running
				runner = qtrue;
				//let others know that someone is now running
				//don't change course again until we get to where we're going
				TIMER_Set(NPC, "roamTime", Q3_INFINITE);
				TIMER_Set(NPC, "verifyCP", Q_irand(1000, 3000)); //don't make sure you're in your CP for 1 - 3 seconds

				NPC_SetCombatPoint(cp);
				NPC_SetMoveGoal(NPC, level.combatPoints[cp].origin, 8, qtrue, cp);

				if (squadState != SQUAD_IDLE)
				{
					AI_GroupUpdateSquadstates(group, NPC, squadState);
				}
				else if (cpFlags & CP_FLEE)
				{
					//outright running for your life
					AI_GroupUpdateSquadstates(group, NPC, SQUAD_RETREAT);
				}
				else
				{
					//any other kind of transition between combat points
					AI_GroupUpdateSquadstates(group, NPC, SQUAD_TRANSITION);
				}
				// If Successfully
				if (cpFlags & CP_COVER && cpFlags & CP_CLEAR)
				{
					if (group->numGroup > 1)
					{
						NPC_ST_StoreMovementSpeech(SPEECH_OUTFLANK, -1);
					}
				} //okay, try a doMove right now to see if we can even get there
				else if (cpFlags & CP_FLANK)
				{
					if (group->numGroup > 1)
					{
						NPC_ST_StoreMovementSpeech(SPEECH_OUTFLANK, -1);
					}
				}
				else if (cpFlags & CP_COVER && !(cpFlags & CP_CLEAR))
				{
					//going into hiding
					NPC_ST_StoreMovementSpeech(SPEECH_COVER, -1);
				}
				else if (Q_irand(0, 3) == 0)
				{
					NPCInfo->aiFlags |= NPCAI_STOP_AT_LOS;
				}
				else
				{
					if (group->numGroup > 1)
					{
						float dot = 1.0f;
						if (!Q_irand(0, 3))
						{
							//25% of the time, see if we're flanking the enemy
							vec3_t eDir2Me, eDir2CP;

							VectorSubtract(NPC->currentOrigin, group->enemy->currentOrigin, eDir2Me);
							VectorNormalize(eDir2Me);

							VectorSubtract(level.combatPoints[NPCInfo->combatPoint].origin, group->enemy->currentOrigin,
								eDir2CP);
							VectorNormalize(eDir2CP);

							dot = DotProduct(eDir2Me, eDir2CP);
						}

						if (dot < 0.4)
						{
							//flanking!
							NPC_ST_StoreMovementSpeech(SPEECH_OUTFLANK, -1);
						}
						else if (!Q_irand(0, 10))
						{
							//regular movement
							NPC_ST_StoreMovementSpeech(SPEECH_YELL, 0.2f); //was SPEECH_COVER
						}
					}
					else if (cpFlags & CP_CLOSEST || cpFlags & CP_APPROACH_ENEMY)
					{
						if (group->numGroup > 1)
						{
							NPC_ST_StoreMovementSpeech(SPEECH_CHASE, 0.4f);
						}
					}
					else if (!Q_irand(0, 20))
					{
						//hell, we're loading the sounds, use them every now and then!
						if (Q_irand(0, 1))
						{
							NPC_ST_StoreMovementSpeech(SPEECH_OUTFLANK, -1);
						}
						else
						{
							NPC_ST_StoreMovementSpeech(SPEECH_ESCAPING, -1);
						}
					}
				}
			}
			else if (NPCInfo->squadState == SQUAD_SCOUT)
			{
				//we couldn't find a combatPoint by the player, so just go after him directly
				ST_HuntEnemy(NPC);
				//set me into scout mode
				AI_GroupUpdateSquadstates(group, NPC, SQUAD_SCOUT);
				//AI should take care of rest
			}
		}
	}

	RestoreNPCGlobals();
}

extern void G_Knockdown(gentity_t* self, gentity_t* attacker, const vec3_t pushDir, float strength,
	qboolean breakSaberLock);

void Noghri_StickTrace(void)
{
	if (!NPC->ghoul2.size()
		|| NPC->weaponModel[0] <= 0)
	{
		return;
	}

	const int boltIndex = gi.G2API_AddBolt(&NPC->ghoul2[NPC->weaponModel[0]], "*weapon");
	if (boltIndex != -1)
	{
		const int curTime = cg.time ? cg.time : level.time;
		qboolean hit = qfalse;
		int lastHit = ENTITYNUM_NONE;
		for (int time = curTime - 25; time <= curTime + 25 && !hit; time += 25)
		{
			mdxaBone_t boltMatrix;
			vec3_t tip, dir, base, angles = { 0, NPC->currentAngles[YAW], 0 };
			const vec3_t mins = { -2, -2, -2 }, maxs = { 2, 2, 2 };
			trace_t trace;

			gi.G2API_GetBoltMatrix(NPC->ghoul2, NPC->weaponModel[0],
				boltIndex,
				&boltMatrix, angles, NPC->currentOrigin, time,
				nullptr, NPC->s.modelScale);
			gi.G2API_GiveMeVectorFromMatrix(boltMatrix, ORIGIN, base);
			gi.G2API_GiveMeVectorFromMatrix(boltMatrix, POSITIVE_Y, dir);
			VectorMA(base, 48, dir, tip);
#ifndef FINAL_BUILD
			if (d_saberCombat->integer > 1)
			{
				G_DebugLine(base, tip, FRAMETIME, 0x000000ff, qtrue);
			}
#endif
			gi.trace(&trace, base, mins, maxs, tip, NPC->s.number, MASK_SHOT, G2_RETURNONHIT, 10);
			if (trace.fraction < 1.0f && trace.entityNum != lastHit)
			{
				//hit something
				gentity_t* traceEnt = &g_entities[trace.entityNum];
				if (traceEnt->takedamage
					&& (!traceEnt->client || traceEnt == NPC->enemy || traceEnt->client->NPC_class != NPC->client->
						NPC_class))
				{
					//smack
					const int dmg = Q_irand(12, 20); //FIXME: base on skill!
					//FIXME: debounce?
					G_Sound(traceEnt, G_SoundIndex(va("sound/weapons/tusken_staff/stickhit%d.wav", Q_irand(1, 4))));
					G_Damage(traceEnt, NPC, NPC, vec3_origin, trace.endpos, dmg, DAMAGE_NO_KNOCKBACK, MOD_MELEE);
					if (traceEnt->health > 0 && dmg > 17)
					{
						//do pain on enemy
						G_Knockdown(traceEnt, NPC, dir, 300, qtrue);
					}
					lastHit = trace.entityNum;
					hit = qtrue;
				}
			}
		}
	}
}

void Noghri_StickTracenew(gentity_t* self)
{
	if (!self->ghoul2.size()
		|| self->weaponModel[0] <= 0)
	{
		return;
	}

	const int boltIndex = gi.G2API_AddBolt(&self->ghoul2[self->weaponModel[0]], "*weapon");
	if (boltIndex != -1)
	{
		const int curTime = cg.time ? cg.time : level.time;
		qboolean hit = qfalse;
		int lastHit = ENTITYNUM_NONE;
		for (int time = curTime - 25; time <= curTime + 25 && !hit; time += 25)
		{
			mdxaBone_t boltMatrix;
			vec3_t tip, dir, base, angles = { 0, self->currentAngles[YAW], 0 };
			const vec3_t mins = { -2, -2, -2 }, maxs = { 2, 2, 2 };
			trace_t trace;

			gi.G2API_GetBoltMatrix(self->ghoul2, self->weaponModel[0],
				boltIndex,
				&boltMatrix, angles, self->currentOrigin, time,
				nullptr, self->s.modelScale);
			gi.G2API_GiveMeVectorFromMatrix(boltMatrix, ORIGIN, base);
			gi.G2API_GiveMeVectorFromMatrix(boltMatrix, POSITIVE_Y, dir);
			VectorMA(base, 48, dir, tip);
#ifndef FINAL_BUILD
			if (d_saberCombat->integer > 1)
			{
				G_DebugLine(base, tip, FRAMETIME, 0x000000ff, qtrue);
			}
#endif
			gi.trace(&trace, base, mins, maxs, tip, self->s.number, MASK_SHOT, G2_RETURNONHIT, 10);
			if (trace.fraction < 1.0f && trace.entityNum != lastHit)
			{
				//hit something
				gentity_t* traceEnt = &g_entities[trace.entityNum];
				if (traceEnt->takedamage
					&& (!traceEnt->client || traceEnt == self->enemy || traceEnt->client->NPC_class != self->client->
						NPC_class))
				{
					//smack
					const int dmg = Q_irand(12, 20); //FIXME: base on skill!
					//FIXME: debounce?
					G_Sound(traceEnt, G_SoundIndex(va("sound/weapons/tusken_staff/stickhit%d.wav", Q_irand(1, 4))));
					G_Damage(traceEnt, self, self, vec3_origin, trace.endpos, dmg, DAMAGE_NO_KNOCKBACK, MOD_MELEE);
					if (traceEnt->health > 0 && dmg > 17)
					{
						//do pain on enemy
						G_Knockdown(traceEnt, self, dir, 300, qtrue);
					}
					lastHit = trace.entityNum;
					hit = qtrue;
				}
			}
		}
	}
}

/*
-------------------------
NPC_BSST_Attack
-------------------------
*/

extern qboolean PM_CrouchAnim(int anim);
constexpr auto MELEE_DIST_SQUARED = 6400;
extern qboolean PM_InOnGroundAnims(int anim);

qboolean Melee_CanDoGrab(void)
{
	if (NPC->client->NPC_class == CLASS_STORMTROOPER || NPC->client->NPC_class == CLASS_CLONETROOPER)
	{
		if (NPC->enemy && NPC->enemy->client)
		{
			//have a valid enemy
			if (TIMER_Done(NPC, "grabEnemyDebounce"))
			{
				//okay to grab again
				if (NPC->client->ps.groundEntityNum != ENTITYNUM_NONE
					&& NPC->enemy->client->ps.groundEntityNum != ENTITYNUM_NONE)
				{
					//me and enemy are on ground
					if (!PM_InOnGroundAnims(NPC->enemy->client->ps.legsAnim))
					{
						if ((NPC->client->ps.weaponTime <= 200 || NPC->client->ps.torsoAnim == BOTH_KYLE_GRAB)
							&& !NPC->client->ps.saberInFlight)
						{
							if (fabs(NPC->enemy->currentOrigin[2] - NPC->currentOrigin[2]) <= 8.0f)
							{
								//close to same level of ground
								if (DistanceSquared(NPC->enemy->currentOrigin, NPC->currentOrigin) <= 10000.0f)
								{
									return qtrue;
								}
							}
						}
					}
				}
			}
		}
	}
	return qfalse;
}

extern float NPC_EnemyRangeFromBolt(int boltIndex);

void Melee_GrabEnemy(void)
{
	TIMER_Set(NPC, "grabEnemyDebounce", NPC->client->ps.torsoAnimTimer + Q_irand(4000, 20000));
}

void NPC_BSST_Attack(void)
{
	//Don't do anything if we're hurt
	if (NPC->painDebounceTime > level.time)
	{
		NPC_UpdateAngles(qtrue, qtrue);

		if (NPC->client->ps.torsoAnim == BOTH_KYLE_GRAB)
		{
			//see if we grabbed enemy
			if (NPC->client->ps.torsoAnimTimer <= 200)
			{
				if (Melee_CanDoGrab()
					&& NPC_EnemyRangeFromBolt(0) <= 88.0f)
				{
					//grab him!
					Melee_GrabEnemy();
					return;
				}
				NPC_SetAnim(NPC, SETANIM_BOTH, BOTH_KYLE_MISS, SETANIM_FLAG_OVERRIDE | SETANIM_FLAG_HOLD);
				NPC->client->ps.weaponTime = NPC->client->ps.torsoAnimTimer;
				return;
			}
		}
		else
		{
			ST_Speech(NPC, SPEECH_COVER, 0);

			NPC_CheckEvasion();
		}
		return;
	}
	if (!in_camera && (TIMER_Done(NPC, "flee") && NPC_CheckForDanger(
		NPC_CheckAlertEvents(qtrue, qtrue, -1, qfalse, AEL_DANGER))))
	{
		ST_Speech(NPC, SPEECH_COVER, 0);

		NPC_CheckEvasion();
	}

	//If we don't have an enemy, just idle
	if (NPC_CheckEnemyExt() == qfalse)
	{
		NPC->enemy = nullptr;
		if (NPC->client->playerTeam == TEAM_PLAYER)
		{
			NPC_BSPatrol();
		}
		else
		{
			NPC_BSST_Patrol();
		}
		return;
	}

	// now if this timer is done and the enemy is no longer in our line of sight, try to get a new enemy later
	if (TIMER_Done(NPC, "sje_check_enemy"))
	{
		TIMER_Set(NPC, "sje_check_enemy", Q_irand(5000, 10000));

		if (NPC->enemy && !NPC_ClearLOS(NPC->enemy) && NPC->health > 0)
		{
			// if enemy cant be seen, try getting one later
			if (NPC->client)
			{
				NPC->enemy = nullptr;
				if (NPC->client->playerTeam == TEAM_PLAYER)
				{
					NPC_BSPatrol();
				}
				else
				{
					NPC_BSST_Patrol();
				}
				return;
			}
			if (NPC->client)
			{
				// guardians have a different way to find enemies. He tries to find the quest player and his allies
				for (int sje_it = 0; sje_it < level.maxclients; sje_it++)
				{
					gentity_t* allied_player = &g_entities[sje_it];

					if (allied_player && allied_player->client &&
						NPC_ClearLOS(allied_player))
					{
						// the quest player or one of his allies. If one of them is in line of sight, choose him as enemy
						NPC->enemy = allied_player;
					}
				}
			}
		}
	}

	//Get our group info
	if (TIMER_Done(NPC, "interrogating"))
	{
		AI_GetGroup(NPC); //, 45, 512, NPC->enemy );
	}
	else
	{
		ST_Speech(NPC, SPEECH_YELL, 0);
	}

	if (NPCInfo->group)
	{
		//I belong to a squad of guys - we should *always* have a group
		if (!NPCInfo->group->processed)
		{
			//I'm the first ent in my group, I'll make the command decisions
#if	AI_TIMERS
			int	startTime = GetTime(0);
#endif//	AI_TIMERS
			ST_Commander();
#if	AI_TIMERS
			int commTime = GetTime(startTime);
			if (commTime > 20)
			{
				gi.Printf(S_COLOR_RED"ERROR: Commander time: %d\n", commTime);
			}
			else if (commTime > 10)
			{
				gi.Printf(S_COLOR_YELLOW"WARNING: Commander time: %d\n", commTime);
			}
			else if (commTime > 2)
			{
				gi.Printf(S_COLOR_GREEN"Commander time: %d\n", commTime);
			}
#endif//	AI_TIMERS
		}
	}
	else if (TIMER_Done(NPC, "flee") && NPC_CheckForDanger(NPC_CheckAlertEvents(qtrue, qtrue, -1, qfalse, AEL_DANGER)))
	{
		//not already fleeing, and going to run
		ST_Speech(NPC, SPEECH_COVER, 0);
		NPC_UpdateAngles(qtrue, qtrue);
		return;
	}

	if (!NPC->enemy)
	{
		//WTF?  somehow we lost our enemy?
		NPC_BSST_Patrol(); //FIXME: or patrol?
		return;
	}

	if (NPCInfo->goalEntity && NPCInfo->goalEntity != NPC->enemy)
	{
		NPCInfo->goalEntity = UpdateGoal();
	}

	enemyLOS = enemyCS = enemyInFOV = qfalse;
	doMove = qtrue;
	faceEnemy = qfalse;
	shoot = qfalse;
	hitAlly = qfalse;
	VectorClear(impactPos);
	enemyDist = DistanceSquared(NPC->currentOrigin, NPC->enemy->currentOrigin);

	vec3_t enemyDir, shootDir;
	VectorSubtract(NPC->enemy->currentOrigin, NPC->currentOrigin, enemyDir);
	VectorNormalize(enemyDir);
	AngleVectors(NPC->client->ps.viewangles, shootDir, nullptr, nullptr);
	const float dot = DotProduct(enemyDir, shootDir);

	if (dot > 0.5f || enemyDist * (1.0f - dot) < 10000)
	{
		//enemy is in front of me or they're very close and not behind me
		enemyInFOV = qtrue;
	}

	if (enemyDist < MIN_ROCKET_DIST_SQUARED) //128
	{
		//enemy within 128
		if ((NPC->client->ps.weapon == WP_FLECHETTE || NPC->client->ps.weapon == WP_REPEATER) &&
			NPCInfo->scriptFlags & SCF_ALT_FIRE)
		{
			//shooting an explosive, but enemy too close, switch to primary fire
			NPCInfo->scriptFlags &= ~SCF_ALT_FIRE;
		}
	}
	else if (enemyDist > 65536) //256 squared
	{
		if (NPC->client->ps.weapon == WP_DISRUPTOR)
		{
			//sniping...
			if (!(NPCInfo->scriptFlags & SCF_ALT_FIRE))
			{
				//use primary fire
				NPCInfo->scriptFlags |= SCF_ALT_FIRE;
				//reset fire-timing variables
				NPC_ChangeWeapon(WP_DISRUPTOR);
				NPC_UpdateAngles(qtrue, qtrue);
				return;
			}
		}
	}

	//can we see our target?
	if (NPC_ClearLOS(NPC->enemy))
	{
		AI_GroupUpdateEnemyLastSeen(NPCInfo->group, NPC->enemy->currentOrigin);
		NPCInfo->enemyLastSeenTime = level.time;
		enemyLOS = qtrue;

		if (NPC->client->ps.weapon == WP_NONE)
		{
			enemyCS = qfalse; //not true, but should stop us from firing
			NPC_AimAdjust(-1); //adjust aim worse longer we have no weapon
		}
		else
		{
			//can we shoot our target?
			if (enemyDist < MIN_ROCKET_DIST_SQUARED &&
				level.time - NPC->lastMoveTime < 5000 &&
				(NPC->client->ps.weapon == WP_ROCKET_LAUNCHER
					|| NPC->client->ps.weapon == WP_CONCUSSION && !(NPCInfo->scriptFlags & SCF_ALT_FIRE)
					|| NPC->client->ps.weapon == WP_FLECHETTE && NPCInfo->scriptFlags & SCF_ALT_FIRE))
			{
				enemyCS = qfalse; //not true, but should stop us from firing
				hitAlly = qtrue; //us!
			}
			else if (enemyInFOV)
			{
				//if enemy is FOV, go ahead and check for shooting
				const int hit = NPC_ShotEntity(NPC->enemy, impactPos);
				const gentity_t* hitEnt = &g_entities[hit];

				if (hit == NPC->enemy->s.number
					|| hitEnt && hitEnt->client && hitEnt->client->playerTeam == NPC->client->enemyTeam
					|| hitEnt && hitEnt->takedamage && (hitEnt->svFlags & SVF_GLASS_BRUSH || hitEnt->health < 40 || NPC
						->s.weapon == WP_EMPLACED_GUN))
				{
					//can hit enemy or enemy ally or will hit glass or other minor breakable (or in emplaced gun), so shoot anyway
					AI_GroupUpdateClearShotTime(NPCInfo->group);
					enemyCS = qtrue;
					NPC_AimAdjust(2); //adjust aim better longer we have clear shot at enemy
					VectorCopy(NPC->enemy->currentOrigin, NPCInfo->enemyLastSeenLocation);
				}
				else
				{
					//Hmm, have to get around this bastard
					NPC_AimAdjust(1); //adjust aim better longer we can see enemy
					ST_ResolveBlockedShot(hit);
					if (hitEnt && hitEnt->client && hitEnt->client->playerTeam == NPC->client->playerTeam)
					{
						//would hit an ally, don't fire!!!
						hitAlly = qtrue;
					}
					else
					{
						//Check and see where our shot *would* hit... if it's not close to the enemy (within 256?), then don't fire
					}
				}
			}
			else
			{
				enemyCS = qfalse; //not true, but should stop us from firing
			}
		}
	}
	else if (gi.inPVS(NPC->enemy->currentOrigin, NPC->currentOrigin))
	{
		NPCInfo->enemyLastSeenTime = level.time;
		faceEnemy = qtrue;
		NPC_AimAdjust(-1); //adjust aim worse longer we cannot see enemy
	}

	if (NPC->client->ps.weapon == WP_NONE)
	{
		faceEnemy = qfalse;
		shoot = qfalse;
	}
	else
	{
		if (enemyLOS)
		{
			//FIXME: no need to face enemy if we're moving to some other goal and he's too far away to shoot?
			faceEnemy = qtrue;
		}
		if (enemyCS)
		{
			shoot = qtrue;
		}
	}

	//Check for movement to take care of
	ST_CheckMoveState();

	//See if we should override shooting decision with any special considerations
	ST_CheckFireState();

	if (faceEnemy)
	{
		//face the enemy
		NPC_FaceEnemy(qtrue);
	}

	if (!(NPCInfo->scriptFlags & SCF_CHASE_ENEMIES))
	{
		//not supposed to chase my enemies
		if (NPCInfo->goalEntity == NPC->enemy)
		{
			//goal is my entity, so don't doMove
			doMove = qfalse;
		}
	}
	else if (NPC->NPC->scriptFlags & SCF_NO_GROUPS)
	{
		NPCInfo->goalEntity = enemyLOS ? nullptr : NPC->enemy;
	}

	if (NPC->client->fireDelay && NPC->s.weapon == WP_ROCKET_LAUNCHER)
	{
		doMove = qfalse;
	}

	if (!ucmd.rightmove)
	{
		//only if not already strafing for some strange reason...?
		if (!TIMER_Done(NPC, "strafeLeft"))
		{
			ucmd.rightmove = -127;
			//re-check the duck as we might want to be rolling
			VectorClear(NPC->client->ps.moveDir);
			doMove = qfalse;
		}
		else if (!TIMER_Done(NPC, "strafeRight"))
		{
			ucmd.rightmove = 127;
			VectorClear(NPC->client->ps.moveDir);
			doMove = qfalse;
		}
	}

	if (NPC->client->ps.legsAnim == BOTH_GUARD_LOOKAROUND1)
	{
		//don't doMove when doing silly look around thing
		doMove = qfalse;
	}
	if (doMove)
	{
		//doMove toward goal
		if (NPCInfo->goalEntity)
		{
			doMove = ST_Move();

			if ((NPC->client->NPC_class != CLASS_ROCKETTROOPER || NPC->s.weapon != WP_ROCKET_LAUNCHER || enemyDist <
				MIN_ROCKET_DIST_SQUARED)
				//rockettroopers who use rocket launchers turn around and run if you get too close (closer than 128)
				&& ucmd.forwardmove <= -32)
			{
				//moving backwards at least 45 degrees
				if (NPCInfo->goalEntity
					&& DistanceSquared(NPCInfo->goalEntity->currentOrigin, NPC->currentOrigin) >
					MIN_TURN_AROUND_DIST_SQ)
				{
					//don't stop running backwards if your goal is less than 100 away
					if (TIMER_Done(NPC, "runBackwardsDebounce"))
					{
						//not already waiting for next run backwards
						if (!TIMER_Exists(NPC, "runningBackwards"))
						{
							//start running backwards
							TIMER_Set(NPC, "runningBackwards", Q_irand(500, 1000)); //Q_irand( 2000, 3500 ) );
						}
						else if (TIMER_Done2(NPC, "runningBackwards", qtrue))
						{
							//done running backwards
							TIMER_Set(NPC, "runBackwardsDebounce", Q_irand(3000, 5000));
						}
					}
				}
			}
			else
			{
				//not running backwards
				//TIMER_Remove( NPC, "runningBackwards" );
			}
		}
		else
		{
			doMove = qfalse;
		}
	}

	if (!doMove)
	{
		if (NPC->client->NPC_class != CLASS_ASSASSIN_DROID)
		{
			if (!TIMER_Done(NPC, "duck"))
			{
				ucmd.upmove = -127;
			}
		}
	}
	else
	{
		//stop ducking!
		TIMER_Set(NPC, "duck", -1);
	}

	if (g_allowgunnerbash->integer)
	{
		// slap
		if (NPC->client->NPC_class == CLASS_STORMCOMMANDO
			&& NPC->client->playerTeam == TEAM_ENEMY
			&& !PM_InKnockDown(&NPC->client->ps))
		{
			if (enemyDist < MELEE_DIST_SQUARED
				&& !NPC->client->ps.weaponTime //not firing
				&& !PM_InKnockDown(&NPC->client->ps) //not knocked down
				&& in_front(NPC->enemy->currentOrigin, NPC->currentOrigin, NPC->client->ps.viewangles, 0.3f))
				//within 80 and in front
			{
				//enemy within 80, if very close, use melee attack to slap away
				if (TIMER_Done(NPC, "slapattackDelay"))
				{
					//animate me
					const int swingAnim = BOTH_TUSKENLUNGE1;
					G_Sound(NPC->enemy, G_SoundIndex("sound/chars/stofficer1/misc/victory1.mp3"));
					NPC_SetAnim(NPC, SETANIM_BOTH, swingAnim, SETANIM_FLAG_OVERRIDE | SETANIM_FLAG_HOLD);
					TIMER_Set(NPC, "slapattackDelay", NPC->client->ps.torsoAnimTimer + Q_irand(15000, 20000));
					//delay the hurt until the proper point in the anim
					TIMER_Set(NPC, "smackTime", 300);
					NPCInfo->blockedDebounceTime = 0;
				}
			}
		}

		// SLAP
		if ((NPC->client->NPC_class == CLASS_STORMTROOPER || NPC->client->NPC_class == CLASS_REBEL)
			&& NPC->client->playerTeam == TEAM_ENEMY
			&& !PM_InKnockDown(&NPC->client->ps))
		{
			if (NPC->client->ps.torsoAnim == BOTH_TUSKENATTACK2 || NPC->client->ps.torsoAnim == BOTH_A7_HILT)
			{
				shoot = qfalse;
				if (TIMER_Done(NPC, "smackTime") && !NPCInfo->blockedDebounceTime)
				{
					//time to smack
					//recheck enemyDist and InFront
					if (enemyDist < MELEE_DIST_SQUARED
						&& !NPC->client->ps.weaponTime //not firing
						&& !PM_InKnockDown(&NPC->client->ps) //not knocked down
						&& in_front(NPC->enemy->currentOrigin, NPC->currentOrigin, NPC->client->ps.viewangles, 0.3f))
					{
						vec3_t smackDir;
						VectorSubtract(NPC->enemy->currentOrigin, NPC->currentOrigin, smackDir);
						smackDir[2] += 30;
						VectorNormalize(smackDir);
						//hurt them
						G_Sound(NPC->enemy, G_SoundIndex(va("sound/weapons/melee/punch%d", Q_irand(1, 4))));
						G_Damage(NPC->enemy, NPC, NPC, smackDir, NPC->currentOrigin,
							(g_spskill->integer + 1) * Q_irand(2, 5), DAMAGE_NO_KNOCKBACK, MOD_MELEE);

						WP_AbsorbKick(NPC->enemy, NPC, smackDir);
						//done with the damage
						NPCInfo->blockedDebounceTime = 1;
					}
				}
			}
			else
			{
				if (enemyDist < MELEE_DIST_SQUARED
					&& !NPC->client->ps.weaponTime //not firing
					&& !PM_InKnockDown(&NPC->client->ps) //not knocked down
					&& in_front(NPC->enemy->currentOrigin, NPC->currentOrigin, NPC->client->ps.viewangles, 0.3f))
					//within 80 and in front
				{
					//enemy within 80, if very close, use melee attack to slap away
					if (TIMER_Done(NPC, "slapattackDelay"))
					{
						//animate me
						int swingAnim;
						if (NPC->health > BLOCKPOINTS_THIRTY)
						{
							swingAnim = BOTH_TUSKENATTACK2;
						}
						else
						{
							swingAnim = BOTH_A7_HILT;
						}
						G_AddVoiceEvent(NPC, Q_irand(EV_OUTFLANK1, EV_OUTFLANK2), 2000);
						NPC_SetAnim(NPC, SETANIM_BOTH, swingAnim, SETANIM_FLAG_OVERRIDE | SETANIM_FLAG_HOLD);
						if (NPC->health > BLOCKPOINTS_THIRTY)
						{
							TIMER_Set(NPC, "slapattackDelay", NPC->client->ps.torsoAnimTimer + Q_irand(15000, 20000));
						}
						else
						{
							TIMER_Set(NPC, "slapattackDelay", NPC->client->ps.torsoAnimTimer + Q_irand(5000, 10000));
						}
						//delay the hurt until the proper point in the anim
						TIMER_Set(NPC, "smackTime", 300);
						NPCInfo->blockedDebounceTime = 0;
					}
				}
			}
		}

		if ((NPC->client->NPC_class == CLASS_STORMTROOPER || NPC->client->NPC_class == CLASS_REBEL)
			&& NPC->client->playerTeam == TEAM_PLAYER
			&& !PM_InKnockDown(&NPC->client->ps))
		{
			if (NPC->client->ps.torsoAnim == BOTH_TUSKENATTACK2 || NPC->client->ps.torsoAnim == BOTH_A7_HILT)
			{
				shoot = qfalse;
				if (TIMER_Done(NPC, "smackTime") && !NPCInfo->blockedDebounceTime)
				{
					//time to smack
					//recheck enemyDist and InFront
					if (enemyDist < MELEE_DIST_SQUARED
						&& !NPC->client->ps.weaponTime //not firing
						&& !PM_InKnockDown(&NPC->client->ps) //not knocked down
						&& in_front(NPC->enemy->currentOrigin, NPC->currentOrigin, NPC->client->ps.viewangles, 0.3f))
					{
						vec3_t smackDir;
						VectorSubtract(NPC->enemy->currentOrigin, NPC->currentOrigin, smackDir);
						smackDir[2] += 30;
						VectorNormalize(smackDir);
						//hurt them
						G_Sound(NPC->enemy, G_SoundIndex(va("sound/weapons/melee/punch%d", Q_irand(1, 4))));
						G_Damage(NPC->enemy, NPC, NPC, smackDir, NPC->currentOrigin,
							(g_spskill->integer + 1) * Q_irand(2, 5), DAMAGE_NO_KNOCKBACK, MOD_MELEE);

						WP_AbsorbKick(NPC->enemy, NPC, smackDir);
						//done with the damage
						NPCInfo->blockedDebounceTime = 1;
					}
				}
			}
			else
			{
				if (enemyDist < MELEE_DIST_SQUARED
					&& !NPC->client->ps.weaponTime //not firing
					&& !PM_InKnockDown(&NPC->client->ps) //not knocked down
					&& in_front(NPC->enemy->currentOrigin, NPC->currentOrigin, NPC->client->ps.viewangles, 0.3f))
					//within 80 and in front
				{
					//enemy within 80, if very close, use melee attack to slap away
					if (TIMER_Done(NPC, "slapattackDelay"))
					{
						//animate me
						int swingAnim;
						if (NPC->health > BLOCKPOINTS_THIRTY)
						{
							swingAnim = BOTH_TUSKENATTACK2;
						}
						else
						{
							swingAnim = BOTH_A7_HILT;
						}
						G_AddVoiceEvent(NPC, Q_irand(EV_OUTFLANK1, EV_OUTFLANK2), 2000);
						NPC_SetAnim(NPC, SETANIM_BOTH, swingAnim, SETANIM_FLAG_OVERRIDE | SETANIM_FLAG_HOLD);
						if (NPC->health > BLOCKPOINTS_THIRTY)
						{
							TIMER_Set(NPC, "slapattackDelay", NPC->client->ps.torsoAnimTimer + Q_irand(15000, 20000));
						}
						else
						{
							TIMER_Set(NPC, "slapattackDelay", NPC->client->ps.torsoAnimTimer + Q_irand(5000, 10000));
						}
						//delay the hurt until the proper point in the anim
						TIMER_Set(NPC, "smackTime", 300);
						NPCInfo->blockedDebounceTime = 0;
					}
				}
			}
		}

		///////////////////////////////////////////////////////////////////////////////////////////////////////////////////
		if (NPC->client->NPC_class == CLASS_CLONETROOPER
			&& NPC->client->playerTeam == TEAM_ENEMY
			&& !PM_InKnockDown(&NPC->client->ps))
		{
			if (NPC->client->ps.torsoAnim == BOTH_TUSKENATTACK1)
			{
				shoot = qfalse;
				if (TIMER_Done(NPC, "smackTime") && !NPCInfo->blockedDebounceTime)
				{
					//time to smack
					//recheck enemyDist and InFront
					if (enemyDist < MELEE_DIST_SQUARED
						&& !NPC->client->ps.weaponTime //not firing
						&& !PM_InKnockDown(&NPC->client->ps) //not knocked down
						&& in_front(NPC->enemy->currentOrigin, NPC->currentOrigin, NPC->client->ps.viewangles, 0.3f))
					{
						vec3_t smackDir;
						VectorSubtract(NPC->enemy->currentOrigin, NPC->currentOrigin, smackDir);
						smackDir[2] += 30;
						VectorNormalize(smackDir);
						//hurt them
						G_Sound(NPC->enemy, G_SoundIndex(va("sound/weapons/melee/punch%d", Q_irand(1, 4))));
						G_Damage(NPC->enemy, NPC, NPC, smackDir, NPC->currentOrigin,
							(g_spskill->integer + 1) * Q_irand(2, 5), DAMAGE_NO_KNOCKBACK, MOD_MELEE);

						WP_AbsorbKick(NPC->enemy, NPC, smackDir);
						//done with the damage
						NPCInfo->blockedDebounceTime = 1;
					}
				}
			}
			else
			{
				if (enemyDist < MELEE_DIST_SQUARED
					&& !NPC->client->ps.weaponTime //not firing
					&& !PM_InKnockDown(&NPC->client->ps) //not knocked down
					&& in_front(NPC->enemy->currentOrigin, NPC->currentOrigin, NPC->client->ps.viewangles, 0.3f))
					//within 80 and in front
				{
					//enemy within 80, if very close, use melee attack to slap away
					if (TIMER_Done(NPC, "slapattackDelay"))
					{
						//animate me
						const int swingAnim = BOTH_TUSKENATTACK1;
						G_AddVoiceEvent(NPC, Q_irand(EV_OUTFLANK1, EV_OUTFLANK2), 2000);
						NPC_SetAnim(NPC, SETANIM_BOTH, swingAnim, SETANIM_FLAG_OVERRIDE | SETANIM_FLAG_HOLD);
						if (NPC->health > BLOCKPOINTS_THIRTY)
						{
							TIMER_Set(NPC, "slapattackDelay", NPC->client->ps.torsoAnimTimer + Q_irand(15000, 20000));
						}
						else
						{
							TIMER_Set(NPC, "slapattackDelay", NPC->client->ps.torsoAnimTimer + Q_irand(5000, 10000));
						}
						//delay the hurt until the proper point in the anim
						TIMER_Set(NPC, "smackTime", 300);
						NPCInfo->blockedDebounceTime = 0;
					}
				}
			}
		}

		// SLAP
		if (NPC->client->NPC_class == CLASS_CLONETROOPER
			&& NPC->client->playerTeam == TEAM_PLAYER
			&& !PM_InKnockDown(&NPC->client->ps))
		{
			if (NPC->client->ps.torsoAnim == BOTH_TUSKENATTACK1)
			{
				shoot = qfalse;
				if (TIMER_Done(NPC, "smackTime") && !NPCInfo->blockedDebounceTime)
				{
					//time to smack
					//recheck enemyDist and InFront
					if (enemyDist < MELEE_DIST_SQUARED
						&& !NPC->client->ps.weaponTime //not firing
						&& !PM_InKnockDown(&NPC->client->ps) //not knocked down
						&& in_front(NPC->enemy->currentOrigin, NPC->currentOrigin, NPC->client->ps.viewangles, 0.3f))
					{
						vec3_t smackDir;
						VectorSubtract(NPC->enemy->currentOrigin, NPC->currentOrigin, smackDir);
						smackDir[2] += 30;
						VectorNormalize(smackDir);
						//hurt them
						G_Sound(NPC->enemy, G_SoundIndex(va("sound/weapons/melee/punch%d", Q_irand(1, 4))));
						G_Damage(NPC->enemy, NPC, NPC, smackDir, NPC->currentOrigin,
							(g_spskill->integer + 1) * Q_irand(2, 5), DAMAGE_NO_KNOCKBACK, MOD_MELEE);

						WP_AbsorbKick(NPC->enemy, NPC, smackDir);
						//done with the damage
						NPCInfo->blockedDebounceTime = 1;
					}
				}
			}
			else
			{
				if (enemyDist < MELEE_DIST_SQUARED
					&& !NPC->client->ps.weaponTime //not firing
					&& !PM_InKnockDown(&NPC->client->ps) //not knocked down
					&& in_front(NPC->enemy->currentOrigin, NPC->currentOrigin, NPC->client->ps.viewangles, 0.3f))
					//within 80 and in front
				{
					//enemy within 80, if very close, use melee attack to slap away
					if (TIMER_Done(NPC, "slapattackDelay"))
					{
						//animate me
						const int swingAnim = BOTH_TUSKENATTACK1;
						G_AddVoiceEvent(NPC, Q_irand(EV_OUTFLANK1, EV_OUTFLANK2), 2000);
						NPC_SetAnim(NPC, SETANIM_BOTH, swingAnim, SETANIM_FLAG_OVERRIDE | SETANIM_FLAG_HOLD);
						if (NPC->health > BLOCKPOINTS_THIRTY)
						{
							TIMER_Set(NPC, "slapattackDelay", NPC->client->ps.torsoAnimTimer + Q_irand(15000, 20000));
						}
						else
						{
							TIMER_Set(NPC, "slapattackDelay", NPC->client->ps.torsoAnimTimer + Q_irand(5000, 10000));
						}
						//delay the hurt until the proper point in the anim
						TIMER_Set(NPC, "smackTime", 300);
						NPCInfo->blockedDebounceTime = 0;
					}
				}
			}
		}
		if (NPC->client->NPC_class == CLASS_SBD
			&& NPC->client->playerTeam == TEAM_ENEMY
			&& !PM_InKnockDown(&NPC->client->ps))
		{
			if (NPC->client->ps.torsoAnim == BOTH_A7_SLAP_R || NPC->client->ps.torsoAnim == BOTH_A7_SLAP_L)
			{
				shoot = qfalse;
				if (TIMER_Done(NPC, "smackTime") && !NPCInfo->blockedDebounceTime)
				{
					//time to smack
					//recheck enemyDist and InFront
					if (enemyDist < MELEE_DIST_SQUARED
						&& !NPC->client->ps.weaponTime //not firing
						&& !PM_InKnockDown(&NPC->client->ps) //not knocked down
						&& in_front(NPC->enemy->currentOrigin, NPC->currentOrigin, NPC->client->ps.viewangles, 0.3f))
					{
						vec3_t smackDir;
						VectorSubtract(NPC->enemy->currentOrigin, NPC->currentOrigin, smackDir);
						smackDir[2] += 30;
						VectorNormalize(smackDir);
						//hurt them
						G_Sound(NPC->enemy, G_SoundIndex(va("sound/weapons/melee/punch%d", Q_irand(1, 4))));
						G_Damage(NPC->enemy, NPC, NPC, smackDir, NPC->currentOrigin,
							(g_spskill->integer + 1) * Q_irand(2, 5), DAMAGE_NO_KNOCKBACK, MOD_MELEE);

						WP_AbsorbKick(NPC->enemy, NPC, smackDir);
						//done with the damage
						NPCInfo->blockedDebounceTime = 1;
					}
				}
			}
			else
			{
				if (enemyDist < MELEE_DIST_SQUARED
					&& !NPC->client->ps.weaponTime //not firing
					&& !PM_InKnockDown(&NPC->client->ps) //not knocked down
					&& in_front(NPC->enemy->currentOrigin, NPC->currentOrigin, NPC->client->ps.viewangles, 0.3f))
					//within 80 and in front
				{
					//enemy within 80, if very close, use melee attack to slap away
					if (TIMER_Done(NPC, "slapattackDelay"))
					{
						//animate me
						int swingAnim;
						if (NPC->health > BLOCKPOINTS_THIRTY)
						{
							swingAnim = BOTH_A7_SLAP_R;
						}
						else
						{
							swingAnim = BOTH_A7_SLAP_L;
						}
						G_AddVoiceEvent(NPC, Q_irand(EV_OUTFLANK1, EV_OUTFLANK2), 2000);
						NPC_SetAnim(NPC, SETANIM_BOTH, swingAnim, SETANIM_FLAG_OVERRIDE | SETANIM_FLAG_HOLD);
						if (NPC->health > BLOCKPOINTS_THIRTY)
						{
							TIMER_Set(NPC, "slapattackDelay", NPC->client->ps.torsoAnimTimer + Q_irand(15000, 20000));
						}
						else
						{
							TIMER_Set(NPC, "slapattackDelay", NPC->client->ps.torsoAnimTimer + Q_irand(5000, 10000));
						}
						//delay the hurt until the proper point in the anim
						TIMER_Set(NPC, "smackTime", 300);
						NPCInfo->blockedDebounceTime = 0;
					}
				}
			}
		}
		if (NPC->client->NPC_class == CLASS_SBD
			&& NPC->client->playerTeam == TEAM_PLAYER
			&& !PM_InKnockDown(&NPC->client->ps))
		{
			if (NPC->client->ps.torsoAnim == BOTH_A7_SLAP_R || NPC->client->ps.torsoAnim == BOTH_A7_SLAP_L)
			{
				shoot = qfalse;
				if (TIMER_Done(NPC, "smackTime") && !NPCInfo->blockedDebounceTime)
				{
					//time to smack
					//recheck enemyDist and InFront
					if (enemyDist < MELEE_DIST_SQUARED
						&& !NPC->client->ps.weaponTime //not firing
						&& !PM_InKnockDown(&NPC->client->ps) //not knocked down
						&& in_front(NPC->enemy->currentOrigin, NPC->currentOrigin, NPC->client->ps.viewangles, 0.3f))
					{
						vec3_t smackDir;
						VectorSubtract(NPC->enemy->currentOrigin, NPC->currentOrigin, smackDir);
						smackDir[2] += 30;
						VectorNormalize(smackDir);
						//hurt them
						G_Sound(NPC->enemy, G_SoundIndex(va("sound/weapons/melee/punch%d", Q_irand(1, 4))));
						G_Damage(NPC->enemy, NPC, NPC, smackDir, NPC->currentOrigin,
							(g_spskill->integer + 1) * Q_irand(2, 5), DAMAGE_NO_KNOCKBACK, MOD_MELEE);

						WP_AbsorbKick(NPC->enemy, NPC, smackDir);
						//done with the damage
						NPCInfo->blockedDebounceTime = 1;
					}
				}
			}
			else
			{
				if (enemyDist < MELEE_DIST_SQUARED
					&& !NPC->client->ps.weaponTime //not firing
					&& !PM_InKnockDown(&NPC->client->ps) //not knocked down
					&& in_front(NPC->enemy->currentOrigin, NPC->currentOrigin, NPC->client->ps.viewangles, 0.3f))
					//within 80 and in front
				{
					//enemy within 80, if very close, use melee attack to slap away
					if (TIMER_Done(NPC, "slapattackDelay"))
					{
						//animate me
						int swingAnim;
						if (NPC->health > BLOCKPOINTS_THIRTY)
						{
							swingAnim = BOTH_A7_SLAP_R;
						}
						else
						{
							swingAnim = BOTH_A7_SLAP_L;
						}
						G_AddVoiceEvent(NPC, Q_irand(EV_OUTFLANK1, EV_OUTFLANK2), 2000);
						NPC_SetAnim(NPC, SETANIM_BOTH, swingAnim, SETANIM_FLAG_OVERRIDE | SETANIM_FLAG_HOLD);
						if (NPC->health > BLOCKPOINTS_THIRTY)
						{
							TIMER_Set(NPC, "slapattackDelay", NPC->client->ps.torsoAnimTimer + Q_irand(15000, 20000));
						}
						else
						{
							TIMER_Set(NPC, "slapattackDelay", NPC->client->ps.torsoAnimTimer + Q_irand(5000, 10000));
						}
						//delay the hurt until the proper point in the anim
						TIMER_Set(NPC, "smackTime", 300);
						NPCInfo->blockedDebounceTime = 0;
					}
				}
			}
		}
		if (NPC->client->NPC_class == CLASS_NOGHRI
			&& NPC->client->playerTeam == TEAM_ENEMY
			&& !PM_InKnockDown(&NPC->client->ps))
		{
			if (NPC->client->ps.torsoAnim == BOTH_TUSKENATTACK2 || NPC->client->ps.torsoAnim == BOTH_A7_HILT)
			{
				shoot = qfalse;
				if (TIMER_Done(NPC, "smackTime") && !NPCInfo->blockedDebounceTime)
				{
					//time to smack
					//recheck enemyDist and InFront
					if (enemyDist < MELEE_DIST_SQUARED
						&& !NPC->client->ps.weaponTime //not firing
						&& !PM_InKnockDown(&NPC->client->ps) //not knocked down
						&& in_front(NPC->enemy->currentOrigin, NPC->currentOrigin, NPC->client->ps.viewangles, 0.3f))
					{
						vec3_t smackDir;
						VectorSubtract(NPC->enemy->currentOrigin, NPC->currentOrigin, smackDir);
						smackDir[2] += 30;
						VectorNormalize(smackDir);
						//hurt them
						G_Sound(NPC->enemy, G_SoundIndex(va("sound/weapons/melee/punch%d", Q_irand(1, 4))));
						G_Damage(NPC->enemy, NPC, NPC, smackDir, NPC->currentOrigin,
							(g_spskill->integer + 1) * Q_irand(2, 5), DAMAGE_NO_KNOCKBACK, MOD_MELEE);

						WP_AbsorbKick(NPC->enemy, NPC, smackDir);
						//done with the damage
						NPCInfo->blockedDebounceTime = 1;
					}
				}
			}
			else
			{
				if (enemyDist < MELEE_DIST_SQUARED
					&& !NPC->client->ps.weaponTime //not firing
					&& !PM_InKnockDown(&NPC->client->ps) //not knocked down
					&& in_front(NPC->enemy->currentOrigin, NPC->currentOrigin, NPC->client->ps.viewangles, 0.3f))
					//within 80 and in front
				{
					//enemy within 80, if very close, use melee attack to slap away
					if (TIMER_Done(NPC, "slapattackDelay"))
					{
						//animate me
						int swingAnim;
						if (NPC->health > BLOCKPOINTS_THIRTY)
						{
							swingAnim = BOTH_TUSKENATTACK2;
						}
						else
						{
							swingAnim = BOTH_A7_HILT;
						}
						G_AddVoiceEvent(NPC, Q_irand(EV_OUTFLANK1, EV_OUTFLANK2), 2000);
						NPC_SetAnim(NPC, SETANIM_BOTH, swingAnim, SETANIM_FLAG_OVERRIDE | SETANIM_FLAG_HOLD);
						if (NPC->health > BLOCKPOINTS_THIRTY)
						{
							TIMER_Set(NPC, "slapattackDelay", NPC->client->ps.torsoAnimTimer + Q_irand(15000, 20000));
						}
						else
						{
							TIMER_Set(NPC, "slapattackDelay", NPC->client->ps.torsoAnimTimer + Q_irand(5000, 10000));
						}
						//delay the hurt until the proper point in the anim
						TIMER_Set(NPC, "smackTime", 300);
						NPCInfo->blockedDebounceTime = 0;
					}
				}
			}
		}
		if (NPC->client->NPC_class == CLASS_NOGHRI
			&& NPC->client->playerTeam == TEAM_PLAYER
			&& !PM_InKnockDown(&NPC->client->ps))
		{
			if (NPC->client->ps.torsoAnim == BOTH_TUSKENATTACK2 || NPC->client->ps.torsoAnim == BOTH_A7_HILT)
			{
				shoot = qfalse;
				if (TIMER_Done(NPC, "smackTime") && !NPCInfo->blockedDebounceTime)
				{
					//time to smack
					//recheck enemyDist and InFront
					if (enemyDist < MELEE_DIST_SQUARED
						&& !NPC->client->ps.weaponTime //not firing
						&& !PM_InKnockDown(&NPC->client->ps) //not knocked down
						&& in_front(NPC->enemy->currentOrigin, NPC->currentOrigin, NPC->client->ps.viewangles, 0.3f))
					{
						vec3_t smackDir;
						VectorSubtract(NPC->enemy->currentOrigin, NPC->currentOrigin, smackDir);
						smackDir[2] += 30;
						VectorNormalize(smackDir);
						//hurt them
						G_Sound(NPC->enemy, G_SoundIndex(va("sound/weapons/melee/punch%d", Q_irand(1, 4))));
						G_Damage(NPC->enemy, NPC, NPC, smackDir, NPC->currentOrigin,
							(g_spskill->integer + 1) * Q_irand(2, 5), DAMAGE_NO_KNOCKBACK, MOD_MELEE);

						WP_AbsorbKick(NPC->enemy, NPC, smackDir);
						//done with the damage
						NPCInfo->blockedDebounceTime = 1;
					}
				}
			}
			else
			{
				if (enemyDist < MELEE_DIST_SQUARED
					&& !NPC->client->ps.weaponTime //not firing
					&& !PM_InKnockDown(&NPC->client->ps) //not knocked down
					&& in_front(NPC->enemy->currentOrigin, NPC->currentOrigin, NPC->client->ps.viewangles, 0.3f))
					//within 80 and in front
				{
					//enemy within 80, if very close, use melee attack to slap away
					if (TIMER_Done(NPC, "slapattackDelay"))
					{
						//animate me
						int swingAnim;
						if (NPC->health > BLOCKPOINTS_THIRTY)
						{
							swingAnim = BOTH_TUSKENATTACK2;
						}
						else
						{
							swingAnim = BOTH_A7_HILT;
						}
						G_AddVoiceEvent(NPC, Q_irand(EV_OUTFLANK1, EV_OUTFLANK2), 2000);
						NPC_SetAnim(NPC, SETANIM_BOTH, swingAnim, SETANIM_FLAG_OVERRIDE | SETANIM_FLAG_HOLD);
						if (NPC->health > BLOCKPOINTS_THIRTY)
						{
							TIMER_Set(NPC, "slapattackDelay", NPC->client->ps.torsoAnimTimer + Q_irand(15000, 20000));
						}
						else
						{
							TIMER_Set(NPC, "slapattackDelay", NPC->client->ps.torsoAnimTimer + Q_irand(5000, 10000));
						}
						//delay the hurt until the proper point in the anim
						TIMER_Set(NPC, "smackTime", 300);
						NPCInfo->blockedDebounceTime = 0;
					}
				}
			}
		}

		//slap end
		//==
		if (NPC->client->NPC_class == CLASS_IMPERIAL ||
			NPC->client->NPC_class == CLASS_REELO ||
			NPC->client->NPC_class == CLASS_SABOTEUR
			&& NPC->client->playerTeam == TEAM_ENEMY
			&& !NPC->client->ps.weaponTime
			&& !PM_InKnockDown(&NPC->client->ps))
		{
			if (NPC->client->ps.torsoAnim == BOTH_A7_KICK_F
				|| NPC->client->ps.torsoAnim == BOTH_A7_KICK_F2
				|| NPC->client->ps.torsoAnim == BOTH_A7_KICK_B2
				|| NPC->client->ps.torsoAnim == BOTH_A7_KICK_B)
			{
				shoot = qfalse;
				if (TIMER_Done(NPC, "smackTime") && !NPCInfo->blockedDebounceTime)
				{
					//time to smack
					//recheck enemyDist and InFront
					if (enemyDist < MELEE_DIST_SQUARED
						&& !NPC->client->ps.weaponTime //not firing
						&& !PM_InKnockDown(&NPC->client->ps) //not knocked down
						&& in_front(NPC->enemy->currentOrigin, NPC->currentOrigin, NPC->client->ps.viewangles, 0.3f))
					{
						vec3_t smackDir;
						VectorSubtract(NPC->enemy->currentOrigin, NPC->currentOrigin, smackDir);
						smackDir[2] += 30;
						VectorNormalize(smackDir);
						//hurt them
						G_Sound(NPC->enemy, G_SoundIndex("sound/chars/%s/misc/pain0%d"));
						G_Damage(NPC->enemy, NPC, NPC, smackDir, NPC->currentOrigin,
							(g_spskill->integer + 1) * Q_irand(1, 3), DAMAGE_NO_KNOCKBACK, MOD_MELEE);

						WP_AbsorbKick(NPC->enemy, NPC, smackDir);
						//done with the damage
						NPCInfo->blockedDebounceTime = 1;
					}
				}
			}
		}

		//slap
		if (NPC->client->NPC_class == CLASS_IMPERIAL ||
			NPC->client->NPC_class == CLASS_REELO ||
			NPC->client->NPC_class == CLASS_SABOTEUR
			&& NPC->client->playerTeam == TEAM_ENEMY
			&& !NPC->client->ps.weaponTime
			&& !PM_InKnockDown(&NPC->client->ps))
		{
			if (enemyDist < MELEE_DIST_SQUARED
				&& !NPC->client->ps.weaponTime //not firing
				&& !PM_InKnockDown(&NPC->client->ps) //not knocked down
				&& in_front(NPC->enemy->currentOrigin, NPC->currentOrigin, NPC->client->ps.viewangles, 0.3f))
				//within 80 and in front
			{
				//enemy within 80, if very close, use melee attack to slap away
				if (TIMER_Done(NPC, "attackDelay"))
				{
					//animate me
					const int swingAnim = Q_irand(BOTH_A7_KICK_F, BOTH_A7_KICK_F2);
					G_Sound(NPC->enemy, G_SoundIndex("sound/weapons/melee/kick2.mp3"));
					NPC_SetAnim(NPC, SETANIM_BOTH, swingAnim, SETANIM_FLAG_OVERRIDE | SETANIM_FLAG_HOLD);
					TIMER_Set(NPC, "attackDelay", NPC->client->ps.torsoAnimTimer + Q_irand(15000, 20000));
					//delay the hurt until the proper point in the anim
					TIMER_Set(NPC, "smackTime", 300);
					NPCInfo->blockedDebounceTime = 0;
				}
			}
			else if (enemyDist < MELEE_DIST_SQUARED
				&& !NPC->client->ps.weaponTime //not firing
				&& !PM_InKnockDown(&NPC->client->ps) //not knocked down
				&& !in_front(NPC->enemy->currentOrigin, NPC->currentOrigin, NPC->client->ps.viewangles, -0.25f))
				//within 80 and generally behind
			{
				//enemy within 80, if very close, use melee attack to slap away
				if (TIMER_Done(NPC, "attackDelay"))
				{
					//animate me
					const int swingAnim = Q_irand(BOTH_A7_KICK_B2, BOTH_A7_KICK_B);
					G_Sound(NPC->enemy, G_SoundIndex("sound/weapons/melee/kick1.mp3"));
					NPC_SetAnim(NPC, SETANIM_BOTH, swingAnim, SETANIM_FLAG_OVERRIDE | SETANIM_FLAG_HOLD);
					TIMER_Set(NPC, "attackDelay", NPC->client->ps.torsoAnimTimer + Q_irand(15000, 20000));
					//delay the hurt until the proper point in the anim
					TIMER_Set(NPC, "smackTime", 300);
					NPCInfo->blockedDebounceTime = 0;
				}
			}
		}
		//===================================================
		if (NPC->client->NPC_class == CLASS_TRANDOSHAN ||
			NPC->client->NPC_class == CLASS_WEEQUAY ||
			NPC->client->NPC_class == CLASS_RODIAN
			&& NPC->client->playerTeam == TEAM_ENEMY
			&& !NPC->client->ps.weaponTime
			&& !PM_InKnockDown(&NPC->client->ps))
		{
			if (NPC->client->ps.torsoAnim == BOTH_MELEE_L
				|| NPC->client->ps.torsoAnim == BOTH_MELEE_R
				|| NPC->client->ps.torsoAnim == BOTH_MELEEUP
				|| NPC->client->ps.torsoAnim == BOTH_MELEE3)
			{
				shoot = qfalse;
				if (TIMER_Done(NPC, "smackTime") && !NPCInfo->blockedDebounceTime)
				{
					//time to smack
					//recheck enemyDist and InFront
					if (enemyDist < MELEE_DIST_SQUARED
						&& !NPC->client->ps.weaponTime //not firing
						&& !PM_InKnockDown(&NPC->client->ps) //not knocked down
						&& in_front(NPC->enemy->currentOrigin, NPC->currentOrigin, NPC->client->ps.viewangles, 0.3f))
					{
						vec3_t smackDir;
						VectorSubtract(NPC->enemy->currentOrigin, NPC->currentOrigin, smackDir);
						smackDir[2] += 10;
						VectorNormalize(smackDir);
						//hurt them
						G_Sound(NPC->enemy, G_SoundIndex("sound/chars/%s/misc/pain0%d"));
						G_Damage(NPC->enemy, NPC, NPC, smackDir, NPC->currentOrigin, (g_spskill->integer + 1) * 1,
							DAMAGE_NO_KNOCKBACK, MOD_MELEE);

						WP_AbsorbKick(NPC->enemy, NPC, smackDir);
						//done with the damage
						NPCInfo->blockedDebounceTime = 1;
					}
				}
			}
		}

		//slap
		if (NPC->client->NPC_class == CLASS_TRANDOSHAN ||
			NPC->client->NPC_class == CLASS_WEEQUAY ||
			NPC->client->NPC_class == CLASS_RODIAN
			&& NPC->client->playerTeam == TEAM_ENEMY
			&& !NPC->client->ps.weaponTime
			&& !PM_InKnockDown(&NPC->client->ps))
		{
			if (enemyDist < MELEE_DIST_SQUARED
				&& !NPC->client->ps.weaponTime //not firing
				&& !PM_InKnockDown(&NPC->client->ps) //not knocked down
				&& in_front(NPC->enemy->currentOrigin, NPC->currentOrigin, NPC->client->ps.viewangles, 0.3f))
				//within 80 and in front
			{
				//enemy within 80, if very close, use melee attack to slap away
				if (TIMER_Done(NPC, "attackDelay"))
				{
					//animate me
					int swingAnim;
					if (NPC->health > 50)
					{
						swingAnim = Q_irand(BOTH_MELEEUP, BOTH_MELEE3); //kick
					}
					else
					{
						swingAnim = Q_irand(BOTH_MELEE_R, BOTH_MELEE_L); //kick
					}
					G_Sound(NPC->enemy, G_SoundIndex("sound/weapons/melee/kick2.mp3"));
					NPC_SetAnim(NPC, SETANIM_BOTH, swingAnim, SETANIM_FLAG_OVERRIDE | SETANIM_FLAG_HOLD);
					TIMER_Set(NPC, "attackDelay", NPC->client->ps.torsoAnimTimer + Q_irand(15000, 20000));
					//delay the hurt until the proper point in the anim
					TIMER_Set(NPC, "smackTime", 300);
					NPCInfo->blockedDebounceTime = 0;
				}
			}
		}

		if (NPC->client->NPC_class == CLASS_BESPIN_COP ||
			NPC->client->NPC_class == CLASS_JAN ||
			NPC->client->NPC_class == CLASS_PRISONER ||
			NPC->client->NPC_class == CLASS_REBEL ||
			NPC->client->NPC_class == CLASS_WOOKIE
			&& NPC->client->playerTeam == TEAM_PLAYER
			&& !PM_InKnockDown(&NPC->client->ps)) //not knocked down ) )
		{
			if (NPC->client->ps.torsoAnim == BOTH_A7_KICK_F
				|| NPC->client->ps.torsoAnim == BOTH_A7_KICK_F2
				|| NPC->client->ps.torsoAnim == BOTH_A7_KICK_B2
				|| NPC->client->ps.torsoAnim == BOTH_A7_KICK_B)
			{
				shoot = qfalse;
				if (TIMER_Done(NPC, "smackTime") && !NPCInfo->blockedDebounceTime)
				{
					//time to smack
					//recheck enemyDist and InFront
					if (enemyDist < MELEE_DIST_SQUARED
						&& !NPC->client->ps.weaponTime //not firing
						&& !PM_InKnockDown(&NPC->client->ps) //not knocked down
						&& in_front(NPC->enemy->currentOrigin, NPC->currentOrigin, NPC->client->ps.viewangles, 0.3f))
					{
						vec3_t smackDir;
						VectorSubtract(NPC->enemy->currentOrigin, NPC->currentOrigin, smackDir);
						smackDir[2] += 30;
						VectorNormalize(smackDir);
						//hurt them
						G_Sound(NPC->enemy, G_SoundIndex("sound/chars/%s/misc/pain0%d"));
						G_Damage(NPC->enemy, NPC, NPC, smackDir, NPC->currentOrigin,
							(g_spskill->integer + 1) * Q_irand(5, 10), DAMAGE_NO_KNOCKBACK, MOD_MELEE);

						WP_AbsorbKick(NPC->enemy, NPC, smackDir);
						//done with the damage
						NPCInfo->blockedDebounceTime = 1;
					}
				}
			}
		}

		//slap
		if (NPC->client->NPC_class == CLASS_BESPIN_COP ||
			NPC->client->NPC_class == CLASS_JAN ||
			NPC->client->NPC_class == CLASS_PRISONER ||
			NPC->client->NPC_class == CLASS_REBEL ||
			NPC->client->NPC_class == CLASS_WOOKIE
			&& NPC->client->playerTeam == TEAM_PLAYER
			&& !PM_InKnockDown(&NPC->client->ps))
		{
			if (enemyDist < MELEE_DIST_SQUARED
				&& !NPC->client->ps.weaponTime //not firing
				&& !PM_InKnockDown(&NPC->client->ps) //not knocked down
				&& in_front(NPC->enemy->currentOrigin, NPC->currentOrigin, NPC->client->ps.viewangles, 0.3f))
				//within 80 and in front
			{
				//enemy within 80, if very close, use melee attack to slap away
				if (TIMER_Done(NPC, "attackDelay"))
				{
					//animate me
					const int swingAnim = Q_irand(BOTH_A7_KICK_F, BOTH_A7_KICK_F2);
					G_Sound(NPC->enemy, G_SoundIndex("sound/weapons/melee/kick3.mp3"));
					NPC_SetAnim(NPC, SETANIM_BOTH, swingAnim, SETANIM_FLAG_OVERRIDE | SETANIM_FLAG_HOLD);
					TIMER_Set(NPC, "attackDelay", NPC->client->ps.torsoAnimTimer + Q_irand(15000, 20000));
					//delay the hurt until the proper point in the anim
					TIMER_Set(NPC, "smackTime", 300);
					NPCInfo->blockedDebounceTime = 0;
				}
			}
			else if (enemyDist < MELEE_DIST_SQUARED
				&& !NPC->client->ps.weaponTime //not firing
				&& !PM_InKnockDown(&NPC->client->ps) //not knocked down
				&& !in_front(NPC->enemy->currentOrigin, NPC->currentOrigin, NPC->client->ps.viewangles, -0.25f))
				//within 80 and generally behind
			{
				//enemy within 80, if very close, use melee attack to slap away
				if (TIMER_Done(NPC, "attackDelay"))
				{
					//animate me
					const int swingAnim = Q_irand(BOTH_A7_KICK_B2, BOTH_A7_KICK_B);
					G_Sound(NPC->enemy, G_SoundIndex("sound/weapons/melee/kick1.mp3"));
					NPC_SetAnim(NPC, SETANIM_BOTH, swingAnim, SETANIM_FLAG_OVERRIDE | SETANIM_FLAG_HOLD);
					TIMER_Set(NPC, "attackDelay", NPC->client->ps.torsoAnimTimer + Q_irand(15000, 20000));
					//delay the hurt until the proper point in the anim
					TIMER_Set(NPC, "smackTime", 300);
					NPCInfo->blockedDebounceTime = 0;
				}
			}
		}

		if (NPC->client->NPC_class == CLASS_BESPIN_COP ||
			NPC->client->NPC_class == CLASS_JAN ||
			NPC->client->NPC_class == CLASS_PRISONER ||
			NPC->client->NPC_class == CLASS_REBEL ||
			NPC->client->NPC_class == CLASS_WOOKIE
			&& !PM_InKnockDown(&NPC->client->ps)) //not knocked down ) )
		{
			if (NPC->client->ps.torsoAnim == BOTH_A7_KICK_F
				|| NPC->client->ps.torsoAnim == BOTH_A7_KICK_F2
				|| NPC->client->ps.torsoAnim == BOTH_A7_KICK_B2
				|| NPC->client->ps.torsoAnim == BOTH_A7_KICK_B)
			{
				shoot = qfalse;
				if (TIMER_Done(NPC, "smackTime") && !NPCInfo->blockedDebounceTime)
				{
					//time to smack
					//recheck enemyDist and InFront
					if (enemyDist < MELEE_DIST_SQUARED
						&& !NPC->client->ps.weaponTime //not firing
						&& !PM_InKnockDown(&NPC->client->ps) //not knocked down
						&& in_front(NPC->enemy->currentOrigin, NPC->currentOrigin, NPC->client->ps.viewangles, 0.3f))
					{
						vec3_t smackDir;
						VectorSubtract(NPC->enemy->currentOrigin, NPC->currentOrigin, smackDir);
						smackDir[2] += 30;
						VectorNormalize(smackDir);
						//hurt them
						G_Sound(NPC->enemy, G_SoundIndex("sound/chars/%s/misc/pain0%d"));
						G_Damage(NPC->enemy, NPC, NPC, smackDir, NPC->currentOrigin,
							(g_spskill->integer + 1) * Q_irand(5, 10), DAMAGE_NO_KNOCKBACK, MOD_MELEE);

						WP_AbsorbKick(NPC->enemy, NPC, smackDir);
						//done with the damage
						NPCInfo->blockedDebounceTime = 1;
					}
				}
			}
		}

		//slap
		if (NPC->client->NPC_class == CLASS_BESPIN_COP ||
			NPC->client->NPC_class == CLASS_JAN ||
			NPC->client->NPC_class == CLASS_PRISONER ||
			NPC->client->NPC_class == CLASS_REBEL ||
			NPC->client->NPC_class == CLASS_WOOKIE
			&& !PM_InKnockDown(&NPC->client->ps))
		{
			if (enemyDist < MELEE_DIST_SQUARED
				&& !NPC->client->ps.weaponTime //not firing
				&& !PM_InKnockDown(&NPC->client->ps) //not knocked down
				&& in_front(NPC->enemy->currentOrigin, NPC->currentOrigin, NPC->client->ps.viewangles, 0.3f))
				//within 80 and in front
			{
				//enemy within 80, if very close, use melee attack to slap away
				if (TIMER_Done(NPC, "attackDelay"))
				{
					//animate me
					const int swingAnim = Q_irand(BOTH_A7_KICK_F, BOTH_A7_KICK_F2);
					G_Sound(NPC->enemy, G_SoundIndex("sound/weapons/melee/kick3.mp3"));
					NPC_SetAnim(NPC, SETANIM_BOTH, swingAnim, SETANIM_FLAG_OVERRIDE | SETANIM_FLAG_HOLD);
					TIMER_Set(NPC, "attackDelay", NPC->client->ps.torsoAnimTimer + Q_irand(15000, 20000));
					//delay the hurt until the proper point in the anim
					TIMER_Set(NPC, "smackTime", 300);
					NPCInfo->blockedDebounceTime = 0;
				}
			}
			else if (enemyDist < MELEE_DIST_SQUARED
				&& !NPC->client->ps.weaponTime //not firing
				&& !PM_InKnockDown(&NPC->client->ps) //not knocked down
				&& !in_front(NPC->enemy->currentOrigin, NPC->currentOrigin, NPC->client->ps.viewangles, -0.25f))
				//within 80 and generally behind
			{
				//enemy within 80, if very close, use melee attack to slap away
				if (TIMER_Done(NPC, "attackDelay"))
				{
					//animate me
					const int swingAnim = Q_irand(BOTH_A7_KICK_B2, BOTH_A7_KICK_B);
					G_Sound(NPC->enemy, G_SoundIndex("sound/weapons/melee/kick1.mp3"));
					NPC_SetAnim(NPC, SETANIM_BOTH, swingAnim, SETANIM_FLAG_OVERRIDE | SETANIM_FLAG_HOLD);
					TIMER_Set(NPC, "attackDelay", NPC->client->ps.torsoAnimTimer + Q_irand(15000, 20000));
					//delay the hurt until the proper point in the anim
					TIMER_Set(NPC, "smackTime", 300);
					NPCInfo->blockedDebounceTime = 0;
				}
			}
		}
	}

	if (NPC->client->NPC_class == CLASS_REBORN //cultist using a gun
		&& NPCInfo->rank >= RANK_LT_COMM //commando or better
		&& NPC->enemy->s.weapon == WP_SABER) //fighting a saber-user
	{
		//commando saboteur vs. jedi/reborn
		//see if we need to avoid their saber
		NPC_EvasionSaber();
	}

	if ( //!TIMER_Done( NPC, "flee" ) ||
		doMove && !TIMER_Done(NPC, "runBackwardsDebounce"))
	{
		//running away
		faceEnemy = qfalse;
	}

	//FIXME: check scf_face_move_dir here?

	if (!faceEnemy)
	{
		//we want to face in the dir we're running
		if (!doMove)
		{
			//if we haven't moved, we should look in the direction we last looked?
			VectorCopy(NPC->client->ps.viewangles, NPCInfo->lastPathAngles);
		}
		NPCInfo->desiredYaw = NPCInfo->lastPathAngles[YAW];
		NPCInfo->desiredPitch = 0;
		NPC_UpdateAngles(qtrue, qtrue);
		if (doMove)
		{
			//don't run away and shoot
			shoot = qfalse;
		}
	}

	if (NPCInfo->scriptFlags & SCF_DONT_FIRE)
	{
		shoot = qfalse;
	}

	if (NPC->enemy && NPC->enemy->enemy)
	{
		if (NPC->enemy->s.weapon == WP_SABER && NPC->enemy->enemy->s.weapon == WP_SABER)
		{
			//don't shoot at an enemy jedi who is fighting another jedi, for fear of injuring one or causing rogue blaster deflections (a la Obi Wan/Vader duel at end of ANH)
			shoot = qfalse;
		}
	}
	//FIXME: don't shoot right away!
	if (NPC->client->fireDelay)
	{
		if (NPC->client->NPC_class == CLASS_SABOTEUR)
		{
			Saboteur_Decloak(NPC);
		}
		if (NPC->s.weapon == WP_ROCKET_LAUNCHER
			|| NPC->s.weapon == WP_CONCUSSION && !(NPCInfo->scriptFlags & SCF_ALT_FIRE))
		{
			if (!enemyLOS || !enemyCS)
			{
				//cancel it
				NPC->client->fireDelay = 0;
			}
			else
			{
				//delay our next attempt
				TIMER_Set(NPC, "attackDelay", Q_irand(3000, 5000));
			}
		}
	}
	else if (shoot)
	{
		//try to shoot if it's time
		if (NPC->client->NPC_class == CLASS_SABOTEUR)
		{
			Saboteur_Decloak(NPC);
		}
		if (TIMER_Done(NPC, "attackDelay"))
		{
			if (!(NPCInfo->scriptFlags & SCF_FIRE_WEAPON)) // we've already fired, no need to do it again here
			{
				WeaponThink(qtrue);
			}
			//NASTY
			if (NPC->s.weapon == WP_ROCKET_LAUNCHER)
			{
				if (ucmd.buttons & BUTTON_ATTACK
					&& !doMove
					&& g_spskill->integer > 1
					&& !Q_irand(0, 3))
				{
					//every now and then, shoot a homing rocket
					ucmd.buttons &= ~BUTTON_ATTACK;
					ucmd.buttons |= BUTTON_ALT_ATTACK;
					NPC->client->fireDelay = Q_irand(1000, 2500);
				}
			}
			else if (NPC->s.weapon == WP_NOGHRI_STICK
				&& enemyDist < 48 * 48) //?
			{
				ucmd.buttons &= ~BUTTON_ATTACK;
				ucmd.buttons |= BUTTON_ALT_ATTACK;
				NPC->client->fireDelay = Q_irand(1500, 2000);
			}
		}
	}
	else
	{
		if (NPC->attackDebounceTime < level.time)
		{
			if (NPC->client->NPC_class == CLASS_SABOTEUR)
			{
				Saboteur_Cloak(NPC);
			}
		}
	}
}

extern qboolean G_TuskenAttackAnimDamage(gentity_t* self);

void NPC_BSST_Default(void)
{
	if (NPCInfo->scriptFlags & SCF_FIRE_WEAPON)
	{
		WeaponThink(qtrue);
	}

	if (NPC->s.weapon == WP_NOGHRI_STICK)
	{
		if (G_TuskenAttackAnimDamage(NPC))
		{
			//Noghri_StickTrace();
			Noghri_StickTracenew(NPC);
		}
	}

	if (!NPC->enemy)
	{
		//don't have an enemy, look for one
		NPC_BSST_Patrol();
	}
	else //if ( NPC->enemy )
	{
		//have an enemy
		if (NPC->enemy->client //enemy is a client
			&& (NPC->enemy->client->NPC_class == CLASS_UGNAUGHT || NPC->enemy->client->NPC_class == CLASS_JAWA)
			//enemy is a lowly jawa or ugnaught
			&& NPC->enemy->enemy != NPC //enemy's enemy is not me
			&& (!NPC->enemy->enemy || !NPC->enemy->enemy->client || NPC->enemy->enemy->client->NPC_class != CLASS_RANCOR
				&& NPC->enemy->enemy->client->NPC_class != CLASS_WAMPA))
			//enemy's enemy is not a client or is not a wampa or rancor (which is scarier than me)
		{
			//they should be scared of ME and no-one else
			G_SetEnemy(NPC->enemy, NPC);
		}
		else
		{
			//have an enemy
			if (NPC_CanUseAdvancedFighting())
			{
				NPC_BSJedi_Default();
			}
			else
			{
				NPC_CheckGetNewWeapon();
				NPC_BSST_Attack();
			}

			if (TIMER_Done(NPC, "TalkTime"))
			{
				if (NPC_IsGunner(NPC))
				{
					// Do taunt...
					const int CallOut = Q_irand(0, 3);

					switch (CallOut)
					{
					case 0:
					default:
						G_AddVoiceEvent(NPC, Q_irand(EV_TAUNT1, EV_TAUNT3), 3000);
						break;
					case 1:
						G_AddVoiceEvent(NPC, Q_irand(EV_ANGER1, EV_ANGER1), 3000);
						break;
					case 2:
						G_AddVoiceEvent(NPC, Q_irand(EV_COMBAT1, EV_COMBAT3), 3000);
						break;
					case 3:
						G_AddVoiceEvent(NPC, Q_irand(EV_JCHASE1, EV_JCHASE3), 3000);
						break;
					}
				}
				TIMER_Set(NPC, "TalkTime", 5000);
			}
		}
	}
}