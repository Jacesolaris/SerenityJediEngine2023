/*
===========================================================================
Copyright (C) 1999 - 2005, Id Software, Inc.
Copyright (C) 2000 - 2013, Raven Software, Inc.
Copyright (C) 2001 - 2013, Activision, Inc.
Copyright (C) 2005 - 2015, ioquake3 contributors
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

/// /////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////// ///
///																																///
///																																///
///													SERENITY JEDI ENGINE														///
///										          LIGHTSABER COMBAT SYSTEM													    ///
///																																///
///						      System designed by Serenity and modded by JaceSolaris. (c) 2019 SJE   		                    ///
///								    https://www.moddb.com/mods/serenityjediengine-20											///
///																																///
/// /////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////// ///

#include "g_local.h"
#include "ghoul2/G2.h"
#include "bg_saga.h"

// g_client.c -- client functions that don't happen every frame

static vec3_t playerMins = { -15, -15, DEFAULT_MINS_2 };
static vec3_t playerMaxs = { 15, 15, DEFAULT_MAXS_2 };

extern int g_siegeRespawnCheck;

void WP_SaberAddG2Model(gentity_t* saberent, const char* saber_model, qhandle_t saber_skin);
void WP_SaberRemoveG2Model(gentity_t* saberent);
extern qboolean WP_SaberCanTurnOffSomeBlades(const saberInfo_t* saber);
extern qboolean G_ValidSaberStyle(const gentity_t* ent, int saberStyle);
extern void Player_CheckBurn(const gentity_t* self);
extern void Player_CheckFreeze(const gentity_t* self);
extern qboolean WP_SaberStyleValidForSaber(const saberInfo_t* saber1, const saberInfo_t* saber2, int saberHolstered,
	int saberAnimLevel);
extern qboolean WP_UseFirstValidSaberStyle(const saberInfo_t* saber1, const saberInfo_t* saber2, int saberHolstered,
	int* saberAnimLevel);

forcedata_t Client_Force[MAX_CLIENTS];

void g_Spectator(gentity_t* ent)
{
	if (ent->client->sess.sessionTeam != TEAM_SPECTATOR)
	{
		ent->client->tempSpectate = Q3_INFINITE;
		ent->health = ent->client->ps.stats[STAT_HEALTH] = 1;
		ent->client->ps.weapon = WP_NONE;
		ent->client->ps.stats[STAT_WEAPONS] = 0;
		ent->client->ps.stats[STAT_HOLDABLE_ITEMS] = 0;
		ent->client->ps.stats[STAT_HOLDABLE_ITEM] = 0;
		ent->takedamage = qfalse;
		trap->UnlinkEntity((sharedEntity_t*)ent);
	}
}

void SP_info_player_duel(gentity_t* ent)
{
	int i;

	G_SpawnInt("nobots", "0", &i);
	if (i)
	{
		ent->flags |= FL_NO_BOTS;
	}
	G_SpawnInt("nohumans", "0", &i);
	if (i)
	{
		ent->flags |= FL_NO_HUMANS;
	}
}

/*QUAKED info_player_duel1 (1 0 1) (-16 -16 -24) (16 16 32) initial
potential spawning position for lone duelists in powerduel.
Targets will be fired when someone spawns in on them.
"nobots" will prevent bots from using this spot.
"nohumans" will prevent non-bots from using this spot.
*/
void SP_info_player_duel1(gentity_t* ent)
{
	int i;

	G_SpawnInt("nobots", "0", &i);
	if (i)
	{
		ent->flags |= FL_NO_BOTS;
	}
	G_SpawnInt("nohumans", "0", &i);
	if (i)
	{
		ent->flags |= FL_NO_HUMANS;
	}
}

/*QUAKED info_player_duel2 (1 0 1) (-16 -16 -24) (16 16 32) initial
potential spawning position for paired duelists in powerduel.
Targets will be fired when someone spawns in on them.
"nobots" will prevent bots from using this spot.
"nohumans" will prevent non-bots from using this spot.
*/
void SP_info_player_duel2(gentity_t* ent)
{
	int i;

	G_SpawnInt("nobots", "0", &i);
	if (i)
	{
		ent->flags |= FL_NO_BOTS;
	}
	G_SpawnInt("nohumans", "0", &i);
	if (i)
	{
		ent->flags |= FL_NO_HUMANS;
	}
}

/*QUAKED info_player_deathmatch (1 0 1) (-16 -16 -24) (16 16 32) initial
potential spawning position for deathmatch games.
The first time a player enters the game, they will be at an 'initial' spot.
Targets will be fired when someone spawns in on them.
"nobots" will prevent bots from using this spot.
"nohumans" will prevent non-bots from using this spot.
*/
void SP_misc_teleporter_dest(gentity_t* ent);

void SP_info_player_deathmatch(gentity_t* ent)
{
	int i;

	SP_misc_teleporter_dest(ent);

	G_SpawnInt("nobots", "0", &i);
	if (i)
	{
		ent->flags |= FL_NO_BOTS;
	}
	G_SpawnInt("nohumans", "0", &i);
	if (i)
	{
		ent->flags |= FL_NO_HUMANS;
	}
	//make sure the spawnpoint's genericValue1 is cleared so that our magical waypoint
	//system will work.
	ent->genericValue1 = 0;
}

/*QUAKED info_player_start (1 0 0) (-16 -16 -24) (16 16 32)
Targets will be fired when someone spawns in on them.
equivelant to info_player_deathmatch
*/
void SP_info_player_start(gentity_t* ent)
{
	ent->classname = "info_player_deathmatch";
	SP_info_player_deathmatch(ent);
}

/*QUAKED info_player_start_red (1 0 0) (-16 -16 -24) (16 16 32) INITIAL
For Red Team DM starts

Targets will be fired when someone spawns in on them.
equivalent to info_player_deathmatch

INITIAL - The first time a player enters the game, they will be at an 'initial' spot.

"nobots" will prevent bots from using this spot.
"nohumans" will prevent non-bots from using this spot.
*/
void SP_info_player_start_red(gentity_t* ent)
{
	SP_info_player_deathmatch(ent);
}

/*QUAKED info_player_start_blue (1 0 0) (-16 -16 -24) (16 16 32) INITIAL
For Blue Team DM starts

Targets will be fired when someone spawns in on them.
equivalent to info_player_deathmatch

INITIAL - The first time a player enters the game, they will be at an 'initial' spot.

"nobots" will prevent bots from using this spot.
"nohumans" will prevent non-bots from using this spot.
*/
void SP_info_player_start_blue(gentity_t* ent)
{
	SP_info_player_deathmatch(ent);
}

void SiegePointUse(gentity_t* self, gentity_t* other, gentity_t* activator)
{
	//Toggle the point on/off
	if (self->genericValue1)
	{
		self->genericValue1 = 0;
	}
	else
	{
		self->genericValue1 = 1;
	}
}

/*QUAKED info_player_siegeteam1 (1 0 0) (-16 -16 -24) (16 16 32)
siege start point - team1
name and behavior of team1 depends on what is defined in the
.siege file for this level

startoff - if non-0 spawn point will be disabled until used
idealclass - if specified, this spawn point will be considered
"ideal" for players of this class name. Corresponds to the name
entry in the .scl (siege class) file.
Targets will be fired when someone spawns in on them.
*/
void SP_info_player_siegeteam1(gentity_t* ent)
{
	int soff = 0;

	if (level.gametype != GT_SIEGE)
	{
		//turn into a DM spawn if not in siege game mode
		ent->classname = "info_player_deathmatch";
		SP_info_player_deathmatch(ent);

		return;
	}

	G_SpawnInt("startoff", "0", &soff);

	if (soff)
	{
		//start disabled
		ent->genericValue1 = 0;
	}
	else
	{
		ent->genericValue1 = 1;
	}

	ent->use = SiegePointUse;
}

/*QUAKED info_player_siegeteam2 (0 0 1) (-16 -16 -24) (16 16 32)
siege start point - team2
name and behavior of team2 depends on what is defined in the
.siege file for this level

startoff - if non-0 spawn point will be disabled until used
idealclass - if specified, this spawn point will be considered
"ideal" for players of this class name. Corresponds to the name
entry in the .scl (siege class) file.
Targets will be fired when someone spawns in on them.
*/
void SP_info_player_siegeteam2(gentity_t* ent)
{
	int soff = 0;

	if (level.gametype != GT_SIEGE)
	{
		//turn into a DM spawn if not in siege game mode
		ent->classname = "info_player_deathmatch";
		SP_info_player_deathmatch(ent);

		return;
	}

	G_SpawnInt("startoff", "0", &soff);

	if (soff)
	{
		//start disabled
		ent->genericValue1 = 0;
	}
	else
	{
		ent->genericValue1 = 1;
	}

	ent->use = SiegePointUse;
}

/*QUAKED info_player_intermission (1 0 1) (-16 -16 -24) (16 16 32) RED BLUE
The intermission will be viewed from this point.  Target an info_notnull for the view direction.
RED - In a Siege game, the intermission will happen here if the Red (attacking) team wins
BLUE - In a Siege game, the intermission will happen here if the Blue (defending) team wins
*/
void SP_info_player_intermission(gentity_t* ent)
{
}

/*QUAKED info_player_intermission_red (1 0 1) (-16 -16 -24) (16 16 32)
The intermission will be viewed from this point.  Target an info_notnull for the view direction.

In a Siege game, the intermission will happen here if the Red (attacking) team wins
target - ent to look at
target2 - ents to use when this intermission point is chosen
*/
void SP_info_player_intermission_red(gentity_t* ent)
{
}

/*QUAKED info_player_intermission_blue (1 0 1) (-16 -16 -24) (16 16 32)
The intermission will be viewed from this point.  Target an info_notnull for the view direction.

In a Siege game, the intermission will happen here if the Blue (defending) team wins
target - ent to look at
target2 - ents to use when this intermission point is chosen
*/
void SP_info_player_intermission_blue(gentity_t* ent)
{
}

#define JMSABER_RESPAWN_TIME 20000 //in case it gets stuck somewhere no one can reach

void ThrowSaberToAttacker(gentity_t* self, const gentity_t* attacker)
{
	gentity_t* ent = &g_entities[self->client->ps.saberIndex];
	int altVelocity = 0;

	if (!ent || ent->enemy != self)
	{
		//something has gone very wrong (this should never happen)
		//but in case it does.. find the saber manually
#ifdef _DEBUG
		Com_Printf("Lost the saber! Attempting to use global pointer..\n");
#endif
		ent = gJMSaberEnt;

		if (!ent)
		{
#ifdef _DEBUG
			Com_Printf("The global pointer was NULL. This is a bad thing.\n");
#endif
			return;
		}

#ifdef _DEBUG
		Com_Printf("Got it (%i). Setting enemy to client %i.\n", ent->s.number, self->s.number);
#endif

		ent->enemy = self;
		self->client->ps.saberIndex = ent->s.number;
	}

	trap->SetConfigstring(CS_CLIENT_JEDIMASTER, "-1");

	if (attacker && attacker->client && self->client->ps.saberInFlight)
	{
		//someone killed us and we had the saber thrown, so actually move this saber to the saber location
		//if we killed ourselves with saber thrown, however, same suicide rules of respawning at spawn spot still
		//apply.
		const gentity_t* flyingsaber = &g_entities[self->client->ps.saberEntityNum];

		if (flyingsaber && flyingsaber->inuse)
		{
			VectorCopy(flyingsaber->s.pos.trBase, ent->s.pos.trBase);
			VectorCopy(flyingsaber->s.pos.trDelta, ent->s.pos.trDelta);
			VectorCopy(flyingsaber->s.apos.trBase, ent->s.apos.trBase);
			VectorCopy(flyingsaber->s.apos.trDelta, ent->s.apos.trDelta);

			VectorCopy(flyingsaber->r.currentOrigin, ent->r.currentOrigin);
			VectorCopy(flyingsaber->r.currentAngles, ent->r.currentAngles);
			altVelocity = 1;
		}
	}

	self->client->ps.saberInFlight = qtrue; //say he threw it anyway in order to properly remove from dead body

	WP_SaberAddG2Model(ent, self->client->saber[0].model, self->client->saber[0].skin);

	ent->s.eFlags &= ~(EF_NODRAW);
	ent->s.modelGhoul2 = 1;
	ent->s.eType = ET_MISSILE;
	ent->enemy = NULL;

	if (!attacker || !attacker->client)
	{
		VectorCopy(ent->s.origin2, ent->s.pos.trBase);
		VectorCopy(ent->s.origin2, ent->s.origin);
		VectorCopy(ent->s.origin2, ent->r.currentOrigin);
		ent->pos2[0] = 0;
		trap->LinkEntity((sharedEntity_t*)ent);
		return;
	}

	if (!altVelocity)
	{
		vec3_t a;
		VectorCopy(self->s.pos.trBase, ent->s.pos.trBase);
		VectorCopy(self->s.pos.trBase, ent->s.origin);
		VectorCopy(self->s.pos.trBase, ent->r.currentOrigin);

		VectorSubtract(attacker->client->ps.origin, ent->s.pos.trBase, a);

		VectorNormalize(a);

		ent->s.pos.trDelta[0] = a[0] * 256;
		ent->s.pos.trDelta[1] = a[1] * 256;
		ent->s.pos.trDelta[2] = 256;
	}

	trap->LinkEntity((sharedEntity_t*)ent);
}

void JMSaberThink(gentity_t* ent)
{
	gJMSaberEnt = ent;

	if (ent->enemy)
	{
		if (!ent->enemy->client || !ent->enemy->inuse)
		{
			//disconnected?
			VectorCopy(ent->enemy->s.pos.trBase, ent->s.pos.trBase);
			VectorCopy(ent->enemy->s.pos.trBase, ent->s.origin);
			VectorCopy(ent->enemy->s.pos.trBase, ent->r.currentOrigin);
			ent->s.modelindex = G_ModelIndex(DEFAULT_SABER_MODEL);
			ent->s.eFlags &= ~(EF_NODRAW);
			ent->s.modelGhoul2 = 1;
			ent->s.eType = ET_MISSILE;
			ent->enemy = NULL;

			ent->pos2[0] = 1;
			ent->pos2[1] = 0; //respawn next think
			trap->LinkEntity((sharedEntity_t*)ent);
		}
		else
		{
			ent->pos2[1] = level.time + JMSABER_RESPAWN_TIME;
		}
	}
	else if (ent->pos2[0] && ent->pos2[1] < level.time)
	{
		VectorCopy(ent->s.origin2, ent->s.pos.trBase);
		VectorCopy(ent->s.origin2, ent->s.origin);
		VectorCopy(ent->s.origin2, ent->r.currentOrigin);
		ent->pos2[0] = 0;
		trap->LinkEntity((sharedEntity_t*)ent);
	}

	ent->nextthink = level.time + 50;
	G_RunObject(ent);
}

void DetermineDodgeMax(const gentity_t* ent);

void JMSaberTouch(gentity_t* self, gentity_t* other, trace_t* trace)
{
	int i = 0;

	if (!other || !other->client || other->health < 1)
	{
		return;
	}

	if (self->enemy)
	{
		return;
	}

	if (!self->s.modelindex)
	{
		return;
	}

	if (other->client->ps.stats[STAT_WEAPONS] & 1 << WP_SABER)
	{
		return;
	}

	if (other->client->ps.isJediMaster)
	{
		return;
	}

	self->enemy = other;
	other->client->ps.stats[STAT_WEAPONS] = 1 << WP_SABER;
	other->client->ps.weapon = WP_SABER;
	other->s.weapon = WP_SABER;
	other->client->ps.zoomMode = 0;
	G_AddEvent(other, EV_BECOME_JEDIMASTER, 0);

	// Track the jedi master
	trap->SetConfigstring(CS_CLIENT_JEDIMASTER, va("%i", other->s.number));

	if (g_spawnInvulnerability.integer)
	{
		other->client->ps.eFlags |= EF_INVULNERABLE;
		other->client->invulnerableTimer = level.time + g_spawnInvulnerability.integer;
	}

	trap->SendServerCommand(-1, va("cp \"%s %s\n\"", other->client->pers.netname,
		G_GetStringEdString("MP_SVGAME", "BECOMEJM")));

	other->client->ps.isJediMaster = qtrue;
	other->client->ps.saberIndex = self->s.number;

	if (other->health < 200 && other->health > 0)
	{
		//full health when you become the Jedi Master
		other->client->ps.stats[STAT_HEALTH] = other->health = 200;
	}

	if (other->client->ps.fd.forcePower < 100)
	{
		other->client->ps.fd.forcePower = 100;
	}

	while (i < NUM_FORCE_POWERS)
	{
		other->client->ps.fd.forcePowersKnown |= 1 << i;
		other->client->ps.fd.forcePowerLevel[i] = FORCE_LEVEL_3;

		i++;
	}

	DetermineDodgeMax(other);
	//set dp to max.
	other->client->ps.stats[STAT_DODGE] = other->client->ps.stats[STAT_MAX_DODGE];

	self->pos2[0] = 1;
	self->pos2[1] = level.time + JMSABER_RESPAWN_TIME;

	self->s.modelindex = 0;
	self->s.eFlags |= EF_NODRAW;
	self->s.modelGhoul2 = 0;
	self->s.eType = ET_GENERAL;
	G_KillG2Queue(self->s.number);
}

gentity_t* gJMSaberEnt = NULL;

/*QUAKED info_jedimaster_start (1 0 0) (-16 -16 -24) (16 16 32)
"jedi master" saber spawn point
*/
void SP_info_jedimaster_start(gentity_t* ent)
{
	if (level.gametype != GT_JEDIMASTER)
	{
		gJMSaberEnt = NULL;
		G_FreeEntity(ent);
		return;
	}

	ent->enemy = NULL;

	ent->flags = FL_BOUNCE_HALF;

	ent->s.modelindex = G_ModelIndex(DEFAULT_SABER_MODEL);
	ent->s.modelGhoul2 = 1;
	ent->s.g2radius = 20;
	//ent->s.eType = ET_GENERAL;
	ent->s.eType = ET_MISSILE;
	ent->s.weapon = WP_SABER;
	ent->s.pos.trType = TR_GRAVITY;
	ent->s.pos.trTime = level.time;
	VectorSet(ent->r.maxs, 3, 3, 3);
	VectorSet(ent->r.mins, -3, -3, -3);
	ent->r.contents = CONTENTS_TRIGGER;
	ent->clipmask = MASK_SOLID;

	ent->isSaberEntity = qtrue;

	ent->bounceCount = -5;

	ent->physicsObject = qtrue;

	VectorCopy(ent->s.pos.trBase, ent->s.origin2); //remember the spawn spot

	ent->touch = JMSaberTouch;

	trap->LinkEntity((sharedEntity_t*)ent);

	ent->think = JMSaberThink;
	ent->nextthink = level.time + 50;
}

/*
=======================================================================

  SelectSpawnPoint

=======================================================================
*/

/*
================
SpotWouldTelefrag

================
*/
qboolean SpotWouldTelefrag(const gentity_t* spot)
{
	int touch[MAX_GENTITIES];
	vec3_t mins, maxs;

	VectorAdd(spot->s.origin, playerMins, mins);
	VectorAdd(spot->s.origin, playerMaxs, maxs);
	const int num = trap->EntitiesInBox(mins, maxs, touch, MAX_GENTITIES);

	for (int i = 0; i < num; i++)
	{
		const gentity_t* hit = &g_entities[touch[i]];
		//if ( hit->client && hit->client->ps.stats[STAT_HEALTH] > 0 ) {
		if (hit->client)
		{
			return qtrue;
		}
	}

	return qfalse;
}

qboolean spot_would_telefrag2(const gentity_t* mover, vec3_t dest)
{
	int touch[MAX_GENTITIES];
	vec3_t mins, maxs;

	VectorAdd(dest, mover->r.mins, mins);
	VectorAdd(dest, mover->r.maxs, maxs);
	const int num = trap->EntitiesInBox(mins, maxs, touch, MAX_GENTITIES);

	for (int i = 0; i < num; i++)
	{
		const gentity_t* hit = &g_entities[touch[i]];
		if (hit == mover)
		{
			continue;
		}

		if (hit->r.contents & mover->r.contents)
		{
			return qtrue;
		}
	}

	return qfalse;
}

/*
================
SelectNearestDeathmatchSpawnPoint

Find the spot that we DON'T want to use
================
*/
#define	MAX_SPAWN_POINTS	128

gentity_t* SelectNearestDeathmatchSpawnPoint(vec3_t from)
{
	float nearestDist = 999999;
	gentity_t* nearestSpot = NULL;
	gentity_t* spot = NULL;

	while ((spot = G_Find(spot, FOFS(classname), "info_player_deathmatch")) != NULL)
	{
		vec3_t delta;
		VectorSubtract(spot->s.origin, from, delta);
		const float dist = VectorLength(delta);
		if (dist < nearestDist)
		{
			nearestDist = dist;
			nearestSpot = spot;
		}
	}

	return nearestSpot;
}

/*
================
SelectRandomDeathmatchSpawnPoint

go to a random point that doesn't telefrag
================
*/
#define	MAX_SPAWN_POINTS	128

gentity_t* SelectRandomDeathmatchSpawnPoint(qboolean isbot)
{
	gentity_t* spots[MAX_SPAWN_POINTS];

	int count = 0;
	gentity_t* spot = NULL;

	while ((spot = G_Find(spot, FOFS(classname), "info_player_deathmatch")) != NULL && count < MAX_SPAWN_POINTS)
	{
		if (SpotWouldTelefrag(spot))
		{
			continue;
		}

		//if(((spot->flags & FL_NO_BOTS) && isbot) ||
		//   ((spot->flags & FL_NO_HUMANS) && !isbot))
		//{
		//	// spot is not for this human/bot player
		//	continue;
		//}

		spots[count] = spot;
		count++;
	}

	if (!count)
	{
		// no spots that won't telefrag
		return G_Find(NULL, FOFS(classname), "info_player_deathmatch");
	}

	const int selection = rand() % count;
	return spots[selection];
}

/*
===========
SelectRandomFurthestSpawnPoint

Chooses a player start, deathmatch start, etc
============
*/
gentity_t* SelectRandomFurthestSpawnPoint(vec3_t avoidPoint, vec3_t origin, vec3_t angles, team_t team, qboolean isbot)
{
	vec3_t delta;
	float dist;
	float list_dist[MAX_SPAWN_POINTS];
	gentity_t* list_spot[MAX_SPAWN_POINTS];
	int i, j;

	int numSpots = 0;
	gentity_t* spot = NULL;

	//in Team DM, look for a team start spot first, if any
	if (level.gametype == GT_TEAM
		&& team != TEAM_FREE
		&& team != TEAM_SPECTATOR)
	{
		const char* classname;
		if (team == TEAM_RED)
		{
			classname = "info_player_start_red";
		}
		else
		{
			classname = "info_player_start_blue";
		}
		while ((spot = G_Find(spot, FOFS(classname), classname)) != NULL)
		{
			if (SpotWouldTelefrag(spot))
			{
				continue;
			}

			if (spot->flags & FL_NO_BOTS && isbot ||
				spot->flags & FL_NO_HUMANS && !isbot)
			{
				// spot is not for this human/bot player
				continue;
			}

			VectorSubtract(spot->s.origin, avoidPoint, delta);
			dist = VectorLength(delta);
			for (i = 0; i < numSpots; i++)
			{
				if (dist > list_dist[i])
				{
					if (numSpots >= MAX_SPAWN_POINTS)
						numSpots = MAX_SPAWN_POINTS - 1;
					for (j = numSpots; j > i; j--)
					{
						list_dist[j] = list_dist[j - 1];
						list_spot[j] = list_spot[j - 1];
					}
					list_dist[i] = dist;
					list_spot[i] = spot;
					numSpots++;
					break;
				}
			}
			if (i >= numSpots && numSpots < MAX_SPAWN_POINTS)
			{
				list_dist[numSpots] = dist;
				list_spot[numSpots] = spot;
				numSpots++;
			}
		}
	}

	if (!numSpots)
	{
		//couldn't find any of the above
		while ((spot = G_Find(spot, FOFS(classname), "info_player_deathmatch")) != NULL)
		{
			if (SpotWouldTelefrag(spot))
			{
				continue;
			}

			if (spot->flags & FL_NO_BOTS && isbot ||
				spot->flags & FL_NO_HUMANS && !isbot)
			{
				// spot is not for this human/bot player
				continue;
			}

			VectorSubtract(spot->s.origin, avoidPoint, delta);
			dist = VectorLength(delta);
			for (i = 0; i < numSpots; i++)
			{
				if (dist > list_dist[i])
				{
					if (numSpots >= MAX_SPAWN_POINTS)
						numSpots = MAX_SPAWN_POINTS - 1;
					for (j = numSpots; j > i; j--)
					{
						list_dist[j] = list_dist[j - 1];
						list_spot[j] = list_spot[j - 1];
					}
					list_dist[i] = dist;
					list_spot[i] = spot;
					numSpots++;
					break;
				}
			}
			if (i >= numSpots && numSpots < MAX_SPAWN_POINTS)
			{
				list_dist[numSpots] = dist;
				list_spot[numSpots] = spot;
				numSpots++;
			}
		}
		if (!numSpots)
		{
			spot = G_Find(NULL, FOFS(classname), "info_player_deathmatch");
			if (!spot)
				trap->Error(ERR_DROP, "Couldn't find a spawn point");
			VectorCopy(spot->s.origin, origin);
			origin[2] += 9;
			VectorCopy(spot->s.angles, angles);
			return spot;
		}
	}

	// select a random spot from the spawn points furthest away
	const int rnd = Q_flrand(0.0f, 1.0f) * (numSpots / 2);

	VectorCopy(list_spot[rnd]->s.origin, origin);
	origin[2] += 9;
	VectorCopy(list_spot[rnd]->s.angles, angles);

	return list_spot[rnd];
}

gentity_t* SelectDuelSpawnPoint(int team, vec3_t avoidPoint, vec3_t origin, vec3_t angles, qboolean isbot)
{
	float list_dist[MAX_SPAWN_POINTS];
	gentity_t* list_spot[MAX_SPAWN_POINTS];
	int i;
	char* spotName;

	int numSpots = 0;
	gentity_t* spot = NULL;

	if (team == DUELTEAM_LONE)
	{
		spotName = "info_player_duel1";
	}
	else if (team == DUELTEAM_DOUBLE)
	{
		spotName = "info_player_duel2";
	}
	else if (team == DUELTEAM_SINGLE)
	{
		spotName = "info_player_duel";
	}
	else
	{
		spotName = "info_player_deathmatch";
	}
tryAgain:

	while ((spot = G_Find(spot, FOFS(classname), spotName)) != NULL)
	{
		vec3_t delta;
		if (SpotWouldTelefrag(spot))
		{
			continue;
		}

		if (spot->flags & FL_NO_BOTS && isbot ||
			spot->flags & FL_NO_HUMANS && !isbot)
		{
			// spot is not for this human/bot player
			continue;
		}

		VectorSubtract(spot->s.origin, avoidPoint, delta);
		const float dist = VectorLength(delta);
		for (i = 0; i < numSpots; i++)
		{
			if (dist > list_dist[i])
			{
				if (numSpots >= MAX_SPAWN_POINTS)
					numSpots = MAX_SPAWN_POINTS - 1;
				for (int j = numSpots; j > i; j--)
				{
					list_dist[j] = list_dist[j - 1];
					list_spot[j] = list_spot[j - 1];
				}
				list_dist[i] = dist;
				list_spot[i] = spot;
				numSpots++;
				break;
			}
		}
		if (i >= numSpots && numSpots < MAX_SPAWN_POINTS)
		{
			list_dist[numSpots] = dist;
			list_spot[numSpots] = spot;
			numSpots++;
		}
	}
	if (!numSpots)
	{
		if (Q_stricmp(spotName, "info_player_deathmatch"))
		{
			//try the loop again with info_player_deathmatch as the target if we couldn't find a duel spot
			spotName = "info_player_deathmatch";
			goto tryAgain;
		}

		//If we got here we found no free duel or DM spots, just try the first DM spot
		spot = G_Find(NULL, FOFS(classname), "info_player_deathmatch");
		if (!spot)
			trap->Error(ERR_DROP, "Couldn't find a spawn point");
		VectorCopy(spot->s.origin, origin);
		origin[2] += 9;
		VectorCopy(spot->s.angles, angles);
		return spot;
	}

	// select a random spot from the spawn points furthest away
	const int rnd = Q_flrand(0.0f, 1.0f) * (numSpots / 2);

	VectorCopy(list_spot[rnd]->s.origin, origin);
	origin[2] += 9;
	VectorCopy(list_spot[rnd]->s.angles, angles);

	return list_spot[rnd];
}

/*
===========
SelectSpawnPoint

Chooses a player start, deathmatch start, etc
============
*/
gentity_t* SelectSpawnPoint(vec3_t avoidPoint, vec3_t origin, vec3_t angles, team_t team, qboolean isbot)
{
	return SelectRandomFurthestSpawnPoint(avoidPoint, origin, angles, team, isbot);
}

/*
===========
SelectInitialSpawnPoint

Try to find a spawn point marked 'initial', otherwise
use normal spawn selection.
============
*/
gentity_t* SelectInitialSpawnPoint(vec3_t origin, vec3_t angles, team_t team, qboolean isbot)
{
	gentity_t* spot = NULL;
	while ((spot = G_Find(spot, FOFS(classname), "info_player_deathmatch")) != NULL)
	{
		if (spot->flags & FL_NO_BOTS && isbot ||
			spot->flags & FL_NO_HUMANS && !isbot)
		{
			continue;
		}

		if (spot->spawnflags & 1)
		{
			break;
		}
	}

	if (!spot || SpotWouldTelefrag(spot))
	{
		return SelectSpawnPoint(vec3_origin, origin, angles, team, isbot);
	}

	VectorCopy(spot->s.origin, origin);
	origin[2] += 9;
	VectorCopy(spot->s.angles, angles);

	return spot;
}

/*
===========
SelectSpectatorSpawnPoint

============
*/
gentity_t* SelectSpectatorSpawnPoint(vec3_t origin, vec3_t angles)
{
	FindIntermissionPoint();

	VectorCopy(level.intermission_origin, origin);
	VectorCopy(level.intermission_angle, angles);

	return NULL;
}

/*
=======================================================================

BODYQUE

=======================================================================
*/

/*
=======================================================================

BODYQUE

=======================================================================
*/

#define BODY_SINK_TIME		30000//45000

/*
===============
InitBodyQue
===============
*/
void InitBodyQue(void)
{
	level.bodyQueIndex = 0;
	for (int i = 0; i < BODY_QUEUE_SIZE; i++)
	{
		gentity_t* ent = G_Spawn();
		ent->classname = "bodyque";
		ent->neverFree = qtrue;
		level.bodyQue[i] = ent;
	}
}

/*
=============
BodySink

After sitting around for five seconds, fall into the ground and disappear
=============
*/
void BodySink(gentity_t* ent)
{
	//while we're at it, I'm making the corpse removal time be set by a cvar like in SP.
	if (g_corpseRemovalTime.integer && level.time - ent->timestamp > g_corpseRemovalTime.integer * 1000 + 2500)
	{
		G_KillG2Queue(ent->s.number);
		G_FreeEntity(ent);
		return;
	}

	G_AddEvent(ent, EV_BODYFADE, 0);
	ent->nextthink = level.time + 18000;
	ent->takedamage = qfalse;
}

/*
=============
CopyToBodyQue

A player is respawning, so make an entity that looks
just like the existing corpse to leave behind.
=============
*/
static qboolean CopyToBodyQue(gentity_t* ent)
{
	int islight = 0;

	if (level.intermissiontime)
	{
		return qfalse;
	}

	trap->UnlinkEntity((sharedEntity_t*)ent);

	// if client is in a nodrop area, don't leave the body
	const int contents = trap->PointContents(ent->s.origin, -1);
	if (contents & CONTENTS_NODROP)
	{
		return qfalse;
	}

	if (ent->client && ent->client->ps.eFlags & EF_DISINTEGRATION)
	{
		//for now, just don't spawn a body if you got disint'd
		return qfalse;
	}

	gentity_t* body = G_Spawn();

	body->classname = "body";

	trap->UnlinkEntity((sharedEntity_t*)body);
	body->s = ent->s;

	//avoid oddly angled corpses floating around
	body->s.angles[PITCH] = body->s.angles[ROLL] = body->s.apos.trBase[PITCH] = body->s.apos.trBase[ROLL] = 0;

	body->s.g2radius = 100;

	body->s.eType = ET_BODY;
	body->s.eFlags = EF_DEAD; // clear EF_TALK, etc

	if (ent->client && ent->client->ps.eFlags & EF_DISINTEGRATION)
	{
		body->s.eFlags |= EF_DISINTEGRATION;
	}

	VectorCopy(ent->client->ps.lastHitLoc, body->s.origin2);

	body->s.powerups = 0; // clear powerups
	body->s.loopSound = 0; // clear lava burning
	body->s.loopIsSoundset = qfalse;
	body->s.number = body - g_entities;
	body->timestamp = level.time;
	body->physicsObject = qtrue;
	body->physicsBounce = 0.0f; // don't bounce
	if (body->s.groundEntityNum == ENTITYNUM_NONE)
	{
		body->s.pos.trType = TR_GRAVITY;
		body->s.pos.trTime = level.time;
		VectorCopy(ent->client->ps.velocity, body->s.pos.trDelta);
	}
	else
	{
		body->s.pos.trType = TR_STATIONARY;
	}
	body->s.event = 0;

	body->s.weapon = ent->s.bolt2;

	if (body->s.weapon == WP_SABER && ent->client->ps.saberInFlight)
	{
		body->s.weapon = WP_MELEE; //lie to keep from putting a saber on the corpse, because it was thrown at death
	}

	//Now doing this through a modified version of the rcg reliable command.
	if (ent->client && ent->client->ps.fd.forceSide == FORCE_LIGHTSIDE)
	{
		islight = 1;
	}
	trap->SendServerCommand(-1, va("ircg %i %i %i %i", ent->s.number, body->s.number, body->s.weapon, islight));

	body->r.svFlags = ent->r.svFlags | SVF_BROADCAST;
	VectorCopy(ent->r.mins, body->r.mins);
	VectorCopy(ent->r.maxs, body->r.maxs);
	VectorCopy(ent->r.absmin, body->r.absmin);
	VectorCopy(ent->r.absmax, body->r.absmax);

	body->s.torsoAnim = body->s.legsAnim = ent->client->ps.legsAnim;

	body->s.customRGBA[0] = ent->client->ps.customRGBA[0];
	body->s.customRGBA[1] = ent->client->ps.customRGBA[1];
	body->s.customRGBA[2] = ent->client->ps.customRGBA[2];
	body->s.customRGBA[3] = ent->client->ps.customRGBA[3];

	body->clipmask = CONTENTS_SOLID | CONTENTS_PLAYERCLIP;
	body->r.contents = CONTENTS_CORPSE;
	body->r.ownerNum = ent->s.number;

	if (g_corpseRemovalTime.integer)
	{
		body->nextthink = level.time + g_corpseRemovalTime.integer * 1000;
		body->think = BodySink;
	}

	body->die = body_die;

	// don't take more damage if already gibbed
	if (ent->health <= GIB_HEALTH)
	{
		body->takedamage = qfalse;
	}
	else
	{
		body->takedamage = qtrue;
	}

	VectorCopy(body->s.pos.trBase, body->r.currentOrigin);
	trap->LinkEntity((sharedEntity_t*)body);

	return qtrue;
}

//======================================================================

/*
==================
SetClientViewAngle

==================
*/
void SetClientViewAngle(gentity_t* ent, vec3_t angle)
{
	// set the delta angle
	for (int i = 0; i < 3; i++)
	{
		const int cmdAngle = ANGLE2SHORT(angle[i]);
		ent->client->ps.delta_angles[i] = cmdAngle - ent->client->pers.cmd.angles[i];
	}
	VectorCopy(angle, ent->s.angles);
	VectorCopy(ent->s.angles, ent->client->ps.viewangles);
}

void MaintainBodyQueue(gentity_t* ent)
{
	//do whatever should be done taking ragdoll and dismemberment states into account.
	qboolean doRCG = qfalse;

	assert(ent && ent->client);
	if (ent->client->tempSpectate >= level.time ||
		ent->client->ps.eFlags2 & EF2_SHIP_DEATH)
	{
		ent->client->noCorpse = qtrue;
	}

	if (!ent->client->noCorpse && !ent->client->ps.fallingToDeath)
	{
		if (!CopyToBodyQue(ent))
		{
			doRCG = qtrue;
		}
	}
	else
	{
		ent->client->noCorpse = qfalse; //clear it for next time
		ent->client->ps.fallingToDeath = qfalse;
		doRCG = qtrue;
	}

	if (doRCG)
	{
		//bodyque func didn't manage to call ircg so call this to assure our limbs and ragdoll states are proper on the client.
		trap->SendServerCommand(-1, va("rcg %i", ent->s.client_num));
	}
}

/*
================
ClientRespawn
================
*/
extern int g_ffaRespawnTimerCheck;
void SiegeRespawn(gentity_t* ent);

void ClientRespawn(gentity_t* ent)
{
	MaintainBodyQueue(ent);

	if (gEscaping || level.gametype == GT_POWERDUEL)
	{
		ent->client->sess.sessionTeam = TEAM_SPECTATOR;
		ent->client->sess.spectatorState = SPECTATOR_FREE;
		ent->client->sess.spectatorClient = 0;

		ent->client->pers.teamState.state = TEAM_BEGIN;
		AddTournamentQueue(ent->client);
		ClientSpawn(ent);
		ent->client->iAmALoser = qtrue;
		return;
	}

	trap->UnlinkEntity((sharedEntity_t*)ent);

	if (g_lms.integer > 0 && ent->lives < 1 && BG_IsLMSGametype(g_gametype.integer) && LMS_EnoughPlayers())
	{
		//playing LMS and we're DEAD!  Just start chillin in tempSpec.
		g_Spectator(ent);
	}
	else if (g_gametype.integer == GT_FFA || g_gametype.integer == GT_TEAM || g_gametype.integer == GT_CTF)
	{
		if (g_ffaRespawnTimer.integer)
		{
			if (ent->client->tempSpectate <= level.time)
			{
				int minDel = g_siegeRespawn.integer * 2000;
				if (minDel < 20000)
				{
					minDel = 20000;
				}
				g_Spectator(ent);
				ent->client->tempSpectate = level.time + minDel;

				// Respawn time.
				if (ent->s.number < MAX_CLIENTS)
				{
					gentity_t* te = G_TempEntity(ent->client->ps.origin, EV_SIEGESPEC);
					te->s.time = g_ffaRespawnTimerCheck;
					te->s.owner = ent->s.number;
				}
				return;
			}
		}
		ClientSpawn(ent);

		if (g_lms.integer > 0 && BG_IsLMSGametype(g_gametype.integer) && LMS_EnoughPlayers())
		{
			//reduce our number of lives since we respawned and we're not the only player.
			ent->lives--;
		}
	}
	else if (level.gametype == GT_SIEGE)
	{
		if (g_siegeRespawn.integer)
		{
			if (ent->client->tempSpectate < level.time)
			{
				int minDel = g_siegeRespawn.integer * 2000;
				if (minDel < 20000)
				{
					minDel = 20000;
				}
				ent->client->tempSpectate = level.time + minDel;
				ent->health = ent->client->ps.stats[STAT_HEALTH] = 1;
				ent->waterlevel = ent->watertype = 0;
				ent->client->ps.weapon = WP_NONE;
				ent->client->ps.stats[STAT_WEAPONS] = 0;
				ent->client->ps.stats[STAT_HOLDABLE_ITEMS] = 0;
				ent->client->ps.stats[STAT_HOLDABLE_ITEM] = 0;
				ent->takedamage = qfalse;
				trap->LinkEntity((sharedEntity_t*)ent);

				// Respawn time.
				if (ent->s.number < MAX_CLIENTS)
				{
					gentity_t* te = G_TempEntity(ent->client->ps.origin, EV_SIEGESPEC);
					te->s.time = g_siegeRespawnCheck;
					te->s.owner = ent->s.number;
				}

				return;
			}
		}
		SiegeRespawn(ent);
	}
	else
	{
		ClientSpawn(ent);
	}
}

/*
================
TeamCount

Returns number of players on a team
================
*/
int TeamCount(int ignoreclient_num, team_t team)
{
	int count = 0;

	for (int i = 0; i < level.maxclients; i++)
	{
		if (i == ignoreclient_num)
		{
			continue;
		}
		if (level.clients[i].pers.connected == CON_DISCONNECTED)
		{
			continue;
		}
		if (level.clients[i].sess.sessionTeam == team)
		{
			count++;
		}
		else if (level.gametype == GT_SIEGE &&
			level.clients[i].sess.siegeDesiredTeam == team)
		{
			count++;
		}
	}

	return count;
}

/*
================
TeamLeader

Returns the client number of the team leader
================
*/
int TeamLeader(int team)
{
	for (int i = 0; i < level.maxclients; i++)
	{
		if (level.clients[i].pers.connected == CON_DISCONNECTED)
		{
			continue;
		}
		if (level.clients[i].sess.sessionTeam == team)
		{
			if (level.clients[i].sess.teamLeader)
				return i;
		}
	}

	return -1;
}

/*
================
PickTeam

================
*/
team_t PickTeam(int ignoreclient_num)
{
	int counts[TEAM_NUM_TEAMS];

	counts[TEAM_BLUE] = TeamCount(ignoreclient_num, TEAM_BLUE);
	counts[TEAM_RED] = TeamCount(ignoreclient_num, TEAM_RED);

	if (counts[TEAM_BLUE] > counts[TEAM_RED])
	{
		return TEAM_RED;
	}
	if (counts[TEAM_RED] > counts[TEAM_BLUE])
	{
		return TEAM_BLUE;
	}
	// equal team count, so join the team with the lowest score
	if (level.teamScores[TEAM_BLUE] > level.teamScores[TEAM_RED])
	{
		return TEAM_RED;
	}
	return TEAM_BLUE;
}

/*
===========
ForceClientSkin

Forces a client's skin (for teamplay)
===========
*/
/*
static void ForceClientSkin( gclient_t *client, char *model, const char *skin ) {
	char *p;

	if ((p = Q_strrchr(model, '/')) != 0) {
		*p = 0;
	}

	Q_strcat(model, MAX_QPATH, "/");
	Q_strcat(model, MAX_QPATH, skin);
}
*/

/*
===========
ClientCheckName
============
*/
static void client_clean_name(const char* in, char* out, int outSize)
{
	int outpos = 0, colorlessLen = 0, spaces = 0, ats = 0;

	// discard leading spaces
	// ReSharper disable once CppPossiblyErroneousEmptyStatements
	for (; *in == ' '; in++);

	// discard leading asterisk's (fail raven for using * as a skipnotify)
	// apparently .* causes the issue too so... derp
	//for(; *in == '*'; in++);

	for (; *in && outpos < outSize - 1; in++)
	{
		out[outpos] = *in;

		if (*in == ' ')
		{
			// don't allow too many consecutive spaces
			if (spaces > 2)
				continue;

			spaces++;
		}
		else if (*in == '@')
		{
			// don't allow too many consecutive at signs
			if (++ats > 2)
			{
				outpos -= 2;
				ats = 0;
				continue;
			}
		}
		else if ((byte)*in < 0x20
			|| (byte)*in == 0x81 || (byte)*in == 0x8D || (byte)*in == 0x8F || (byte)*in == 0x90 || (byte)*in == 0x9D
			|| (byte)*in == 0xA0 || (byte)*in == 0xAD)
		{
			continue;
		}
		else if (outpos > 0 && out[outpos - 1] == Q_COLOR_ESCAPE)
		{
			if (Q_IsColorStringExt(&out[outpos - 1]))
			{
				colorlessLen--;

#if 0
				if (ColorIndex(*in) == 0)
				{// Disallow color black in names to prevent players from getting advantage playing in front of black backgrounds
					outpos--;
					continue;
				}
#endif
			}
			else
			{
				spaces = ats = 0;
				colorlessLen++;
			}
		}
		else
		{
			spaces = ats = 0;
			colorlessLen++;
		}

		outpos++;
	}

	out[outpos] = '\0';

	// don't allow empty names
	if (*out == '\0' || colorlessLen == 0)
		Q_strncpyz(out, "Padawan", outSize);
}

#ifdef _DEBUG
void G_DebugWrite(const char* path, const char* text)
{
	fileHandle_t f;

	trap->FS_Open(path, &f, FS_APPEND);
	trap->FS_Write(text, strlen(text), f);
	trap->FS_Close(f);
}
#endif

qboolean G_SaberModelSetup(const gentity_t* ent)
{
	int i = 0;
	qboolean fallbackForSaber = qtrue;

	while (i < MAX_SABERS)
	{
		if (ent->client->saber[i].model[0])
		{
			//first kill it off if we've already got it
			if (ent->client->weaponGhoul2[i])
			{
				trap->G2API_CleanGhoul2Models(&ent->client->weaponGhoul2[i]);
			}
			trap->G2API_InitGhoul2Model(&ent->client->weaponGhoul2[i], ent->client->saber[i].model, 0, 0, -20, 0, 0);

			if (ent->client->weaponGhoul2[i])
			{
				int j = 0;

				if (ent->client->saber[i].skin)
				{
					trap->G2API_SetSkin(ent->client->weaponGhoul2[i], 0, ent->client->saber[i].skin,
						ent->client->saber[i].skin);
				}

				if (ent->client->saber[i].saberFlags & SFL_BOLT_TO_WRIST)
				{
					trap->G2API_SetBoltInfo(ent->client->weaponGhoul2[i], 0, 3 + i);
				}
				else
				{
					// bolt to right hand for 0, or left hand for 1
					trap->G2API_SetBoltInfo(ent->client->weaponGhoul2[i], 0, i);
				}

				//Add all the bolt points
				while (j < ent->client->saber[i].numBlades)
				{
					const char* tagName = va("*blade%i", j + 1);
					int tag_bolt = trap->G2API_AddBolt(ent->client->weaponGhoul2[i], 0, tagName);

					if (tag_bolt == -1)
					{
						if (j == 0)
						{
							//guess this is an 0ldsk3wl saber
							tag_bolt = trap->G2API_AddBolt(ent->client->weaponGhoul2[i], 0, "*flash");
							fallbackForSaber = qfalse;
							break;
						}

						if (tag_bolt == -1)
						{
							assert(0);
							break;
						}
					}
					j++;

					fallbackForSaber = qfalse; //got at least one custom saber so don't need default
				}

				//Copy it into the main instance
				trap->G2API_CopySpecificGhoul2Model(ent->client->weaponGhoul2[i], 0, ent->ghoul2, i + 1);
			}
		}
		else
		{
			break;
		}

		i++;
	}

	return fallbackForSaber;
}

/*
===========
SetupGameGhoul2Model

There are two ghoul2 model instances per player (actually three).  One is on the clientinfo (the base for the client side
player, and copied for player spawns and for corpses).  One is attached to the centity itself, which is the model acutally
animated and rendered by the system.  The final is the game ghoul2 model.  This is animated by pmove on the server, and
is used for determining where the lightsaber should be, and for per-poly collision tests.
===========
*/
void* g2SaberInstance = NULL;

qboolean BG_IsValidCharacterModel(const char* modelName, const char* skinName);
qboolean BG_ValidateSkinForTeam(const char* modelName, char* skinName, int team, float* colors);
void BG_GetVehicleModelName(char* modelName, const char* vehicleName, size_t len);

void SetupGameGhoul2Model(gentity_t* ent, char* modelname, char* skinName)
{
	int handle;
#if 0
	char		/**GLAName,*/* slash;
#endif
	char GLAName[MAX_QPATH];
	const vec3_t tempVec = { 0, 0, 0 };

	if (strlen(modelname) >= MAX_QPATH)
	{
		Com_Error(ERR_FATAL, "SetupGameGhoul2Model(%s): modelname exceeds MAX_QPATH.\n", modelname);
	}
	if (skinName && strlen(skinName) >= MAX_QPATH)
	{
		Com_Error(ERR_FATAL, "SetupGameGhoul2Model(%s): skinName exceeds MAX_QPATH.\n", skinName);
	}

	// First things first.  If this is a ghoul2 model, then let's make sure we demolish this first.
	if (ent->ghoul2 && trap->G2API_HaveWeGhoul2Models(ent->ghoul2))
	{
		trap->G2API_CleanGhoul2Models(&ent->ghoul2);
	}

	//rww - just load the "standard" model for the server"
	if (!precachedKyle)
	{
		char afilename[MAX_QPATH];
		Com_sprintf(afilename, sizeof afilename, "models/players/"DEFAULT_MODEL"/model.glm");
		handle = trap->G2API_InitGhoul2Model(&precachedKyle, afilename, 0, 0, -20, 0, 0);

		if (handle < 0)
		{
			return;
		}

		const int defSkin = trap->R_RegisterSkin("models/players/"DEFAULT_MODEL"/model_default.skin");
		trap->G2API_SetSkin(precachedKyle, 0, defSkin, defSkin);
	}

	if (precachedKyle && trap->G2API_HaveWeGhoul2Models(precachedKyle))
	{
		if (d_perPlayerGhoul2.integer || ent->s.number >= MAX_CLIENTS ||
			G_PlayerHasCustomSkeleton(ent))
		{
			//rww - allow option for perplayer models on server for collision and bolt stuff.
			char modelFullPath[MAX_QPATH];
			char truncModelName[MAX_QPATH];
			char skin[MAX_QPATH];
			char vehicleName[MAX_QPATH];
			int skinHandle = 0;

			// If this is a vehicle, get it's model name.
			if (ent->client->NPC_class == CLASS_VEHICLE)
			{
				char realModelName[MAX_QPATH];

				Q_strncpyz(vehicleName, modelname, sizeof vehicleName);
				BG_GetVehicleModelName(realModelName, modelname, sizeof realModelName);
				strcpy(truncModelName, realModelName);
				skin[0] = 0;
				if (ent->m_pVehicle
					&& ent->m_pVehicle->m_pVehicleInfo
					&& ent->m_pVehicle->m_pVehicleInfo->skin
					&& ent->m_pVehicle->m_pVehicleInfo->skin[0])
				{
					skinHandle = trap->R_RegisterSkin(va("models/players/%s/model_%s.skin", realModelName,
						ent->m_pVehicle->m_pVehicleInfo->skin));
				}
				else
				{
					skinHandle = trap->R_RegisterSkin(va("models/players/%s/model_default.skin", realModelName));
				}
			}
			else
			{
				if (skinName && skinName[0])
				{
					strcpy(skin, skinName);
					strcpy(truncModelName, modelname);
				}
				else
				{
					strcpy(skin, "default");

					strcpy(truncModelName, modelname);
					char* p = Q_strrchr(truncModelName, '/');

					if (p)
					{
						int i = 0;
						*p = 0;
						p++;

						while (p && *p)
						{
							skin[i] = *p;
							i++;
							p++;
						}
						skin[i] = 0;
						i = 0;
					}

					if (!BG_IsValidCharacterModel(truncModelName, skin))
					{
						strcpy(truncModelName, DEFAULT_MODEL);
						strcpy(skin, "default");
					}

					if (level.gametype >= GT_TEAM && level.gametype != GT_SIEGE && !g_jediVmerc.integer)
					{
						float colorOverride[3];

						colorOverride[0] = colorOverride[1] = colorOverride[2] = 0.0f;

						BG_ValidateSkinForTeam(truncModelName, skin, ent->client->sess.sessionTeam, colorOverride);
						if (colorOverride[0] != 0.0f ||
							colorOverride[1] != 0.0f ||
							colorOverride[2] != 0.0f)
						{
							ent->client->ps.customRGBA[0] = colorOverride[0] * 255.0f;
							ent->client->ps.customRGBA[1] = colorOverride[1] * 255.0f;
							ent->client->ps.customRGBA[2] = colorOverride[2] * 255.0f;
						}

						//BG_ValidateSkinForTeam( truncModelName, skin, ent->client->sess.sessionTeam, NULL );
					}
					else if (level.gametype == GT_SIEGE)
					{
						//force skin for class if appropriate
						if (ent->client->siegeClass != -1)
						{
							const siegeClass_t* scl = &bgSiegeClasses[ent->client->siegeClass];
							if (scl->forcedSkin[0])
							{
								Q_strncpyz(skin, scl->forcedSkin, sizeof skin);
							}
						}
					}
				}
			}

			if (skin[0])
			{
				char* useSkinName;

				if (strchr(skin, '|'))
				{
					//three part skin
					useSkinName = va("models/players/%s/|%s", truncModelName, skin);
				}
				else
				{
					useSkinName = va("models/players/%s/model_%s.skin", truncModelName, skin);
				}

				skinHandle = trap->R_RegisterSkin(useSkinName);
			}

			strcpy(modelFullPath, va("models/players/%s/model.glm", truncModelName));
			handle = trap->G2API_InitGhoul2Model(&ent->ghoul2, modelFullPath, 0, skinHandle, -20, 0, 0);

			if (handle < 0)
			{
				//Huh. Guess we don't have this model. Use the default.
				if (ent->ghoul2 && trap->G2API_HaveWeGhoul2Models(ent->ghoul2))
				{
					trap->G2API_CleanGhoul2Models(&ent->ghoul2);
				}
				ent->ghoul2 = NULL;
				trap->G2API_DuplicateGhoul2Instance(precachedKyle, &ent->ghoul2);
			}
			else
			{
				trap->G2API_SetSkin(ent->ghoul2, 0, skinHandle, skinHandle);

				GLAName[0] = 0;
				trap->G2API_GetGLAName(ent->ghoul2, 0, GLAName);

				if (!GLAName[0] || !strstr(GLAName, "players/_humanoid/") && ent->s.number < MAX_CLIENTS && !
					G_PlayerHasCustomSkeleton(ent))
				{
					//a bad model
					trap->G2API_CleanGhoul2Models(&ent->ghoul2);
					ent->ghoul2 = NULL;
					trap->G2API_DuplicateGhoul2Instance(precachedKyle, &ent->ghoul2);
				}

				if (ent->s.number >= MAX_CLIENTS)
				{
					ent->s.modelGhoul2 = 1; //so we know to free it on the client when we're removed.

					if (skin[0])
					{
						//append it after a *
						strcat(modelFullPath, va("*%s", skin));
					}

					if (ent->client->NPC_class == CLASS_VEHICLE)
					{
						//vehicles are tricky and send over their vehicle names as the model (the model is then retrieved based on the vehicle name)
						ent->s.modelindex = G_ModelIndex(vehicleName);
					}
					else
					{
						ent->s.modelindex = G_ModelIndex(modelFullPath);
					}
				}
			}
		}
		else
		{
			trap->G2API_DuplicateGhoul2Instance(precachedKyle, &ent->ghoul2);
		}
	}
	else
	{
		return;
	}

	//Attach the instance to this entity num so we can make use of client-server
	//shared operations if possible.
	trap->G2API_AttachInstanceToEntNum(ent->ghoul2, ent->s.number, qtrue);

	// The model is now loaded.

	GLAName[0] = 0;

	if (!BGPAFtextLoaded)
	{
		if (bg_parse_animation_file("models/players/_humanoid/animation.cfg", bgHumanoidAnimations, qtrue) == -1)
		{
			Com_Printf("Failed to load humanoid animation file\n");
			return;
		}
	}

	if (ent->s.number >= MAX_CLIENTS || G_PlayerHasCustomSkeleton(ent))
	{
		ent->localAnimIndex = -1;

		GLAName[0] = 0;
		trap->G2API_GetGLAName(ent->ghoul2, 0, GLAName);

		if (GLAName[0] &&
			!strstr(GLAName, "players/_humanoid/"))
		{
			//it doesn't use humanoid anims.
			char* slash = Q_strrchr(GLAName, '/');
			if (slash)
			{
				strcpy(slash, "/animation.cfg");

				ent->localAnimIndex = bg_parse_animation_file(GLAName, NULL, qfalse);
			}
		}
		else
		{
			//humanoid index.
			if (strstr(GLAName, "players/rockettrooper/"))
			{
				ent->localAnimIndex = 1;
			}
			else
			{
				ent->localAnimIndex = 0;
			}
		}

		if (ent->localAnimIndex == -1)
		{
			Com_Error(ERR_DROP, "NPC had an invalid GLA\n");
		}
	}
	else
	{
		GLAName[0] = 0;
		trap->G2API_GetGLAName(ent->ghoul2, 0, GLAName);

		if (strstr(GLAName, "players/rockettrooper/"))
		{
			//assert(!"Should not have gotten in here with rockettrooper skel");
			ent->localAnimIndex = 1;
		}
		else
		{
			ent->localAnimIndex = 0;
		}
	}

	if (ent->s.NPC_class == CLASS_VEHICLE &&
		ent->m_pVehicle)
	{
		//do special vehicle stuff
		char strTemp[128];

		// Setup the default first bolt
		int i;

		// Setup the droid unit.
		ent->m_pVehicle->m_iDroidUnitTag = trap->G2API_AddBolt(ent->ghoul2, 0, "*droidunit");

		// Setup the Exhausts.
		for (i = 0; i < MAX_VEHICLE_EXHAUSTS; i++)
		{
			Com_sprintf(strTemp, 128, "*exhaust%i", i + 1);
			ent->m_pVehicle->m_iExhaustTag[i] = trap->G2API_AddBolt(ent->ghoul2, 0, strTemp);
		}

		// Setup the Muzzles.
		for (i = 0; i < MAX_VEHICLE_MUZZLES; i++)
		{
			Com_sprintf(strTemp, 128, "*muzzle%i", i + 1);
			ent->m_pVehicle->m_iMuzzleTag[i] = trap->G2API_AddBolt(ent->ghoul2, 0, strTemp);
			if (ent->m_pVehicle->m_iMuzzleTag[i] == -1)
			{
				//ergh, try *flash?
				Com_sprintf(strTemp, 128, "*flash%i", i + 1);
				ent->m_pVehicle->m_iMuzzleTag[i] = trap->G2API_AddBolt(ent->ghoul2, 0, strTemp);
			}
		}

		// Setup the Turrets.
		for (i = 0; i < MAX_VEHICLE_TURRET_MUZZLES; i++)
		{
			if (ent->m_pVehicle->m_pVehicleInfo->turret[i].gunnerViewTag)
			{
				ent->m_pVehicle->m_iGunnerViewTag[i] = trap->G2API_AddBolt(
					ent->ghoul2, 0, ent->m_pVehicle->m_pVehicleInfo->turret[i].gunnerViewTag);
			}
			else
			{
				ent->m_pVehicle->m_iGunnerViewTag[i] = -1;
			}
		}
	}

	if (ent->client->ps.weapon == WP_SABER || ent->s.number < MAX_CLIENTS)
	{
		//a player or NPC saber user
		trap->G2API_AddBolt(ent->ghoul2, 0, "*r_hand");
		trap->G2API_AddBolt(ent->ghoul2, 0, "*l_hand");

		//rhand must always be first bolt. lhand always second. Whichever you want the
		//jetpack bolted to must always be third.
		trap->G2API_AddBolt(ent->ghoul2, 0, "*chestg");

		//claw bolts
		trap->G2API_AddBolt(ent->ghoul2, 0, "*r_hand_cap_r_arm");
		trap->G2API_AddBolt(ent->ghoul2, 0, "*l_hand_cap_l_arm");

		trap->G2API_SetBoneAnim(ent->ghoul2, 0, "model_root", 0, 12, BONE_ANIM_OVERRIDE_LOOP, 1.0f, level.time, -1, -1);
		trap->G2API_SetBoneAngles(ent->ghoul2, 0, "upper_lumbar", tempVec, BONE_ANGLES_POSTMULT, POSITIVE_X, NEGATIVE_Y,
			NEGATIVE_Z, NULL, 0, level.time);
		trap->G2API_SetBoneAngles(ent->ghoul2, 0, "cranium", tempVec, BONE_ANGLES_POSTMULT, POSITIVE_Z, NEGATIVE_Y,
			POSITIVE_X, NULL, 0, level.time);

		if (!g2SaberInstance)
		{
			trap->G2API_InitGhoul2Model(&g2SaberInstance, DEFAULT_SABER_MODEL, 0, 0, -20, 0, 0);

			if (g2SaberInstance)
			{
				// indicate we will be bolted to model 0 (ie the player) on bolt 0 (always the right hand) when we get copied
				trap->G2API_SetBoltInfo(g2SaberInstance, 0, 0);
				// now set up the gun bolt on it
				trap->G2API_AddBolt(g2SaberInstance, 0, "*blade1");
			}
		}

		if (G_SaberModelSetup(ent))
		{
			if (g2SaberInstance)
			{
				trap->G2API_CopySpecificGhoul2Model(g2SaberInstance, 0, ent->ghoul2, 1);
			}
		}
	}

	if (ent->s.number >= MAX_CLIENTS)
	{
		//some extra NPC stuff
		if (trap->G2API_AddBolt(ent->ghoul2, 0, "lower_lumbar") == -1)
		{
			//check now to see if we have this bone for setting anims and such
			ent->noLumbar = qtrue;
		}
		//add some extra bolts for some of the NPC classes
		if (ent->client->NPC_class == CLASS_HOWLER)
		{
			ent->NPC->genericBolt1 = trap->G2API_AddBolt(&ent->ghoul2, 0, "Tongue01"); // tongue base
			ent->NPC->genericBolt2 = trap->G2API_AddBolt(&ent->ghoul2, 0, "Tongue08"); // tongue tip
		}
	}
}

/*
===========
ClientUserInfoChanged

Called from ClientConnect when the player first connects and
directly by the server system when the player updates a userinfo variable.

The game can override any of the settings and call trap->SetUserinfo
if desired.
============
*/

qboolean G_SetSaber(gentity_t* ent, int saber_num, char* saberName, qboolean siegeOverride);
void G_ValidateSiegeClassForTeam(const gentity_t* ent, int team);

typedef struct userinfoValidate_s
{
	const char* field, * fieldClean;
	unsigned int minCount, maxCount;
} userinfoValidate_t;

#define UIF( x, _min, _max ) { STRING(\\) #x STRING(\\), STRING( x ), _min, _max }
static userinfoValidate_t userinfoFields[] = {
	UIF(cl_guid, 0, 0), // not allowed, q3fill protection
	UIF(cl_punkbuster, 0, 0), // not allowed, q3fill protection
	UIF(ip, 0, 1), // engine adds this at the end
	UIF(name, 1, 1),
	UIF(rate, 1, 1),
	UIF(snaps, 1, 1),
	UIF(model, 1, 1),
	UIF(forcepowers, 1, 1),
	UIF(color1, 1, 1),
	UIF(color2, 1, 1),
	UIF(handicap, 1, 1),
	UIF(sex, 0, 1),
	UIF(cg_predictItems, 1, 1),
	UIF(saber1, 1, 1),
	UIF(saber2, 1, 1),
	UIF(char_color_red, 1, 1),
	UIF(char_color_green, 1, 1),
	UIF(char_color_blue, 1, 1),
	UIF(teamtask, 0, 1), // optional
	UIF(password, 0, 1), // optional
	UIF(teamoverlay, 0, 1), // only registered in cgame, not sent when connecting
};
static const size_t numUserinfoFields = ARRAY_LEN(userinfoFields);

static const char* userinfoValidateExtra[USERINFO_VALIDATION_MAX] = {
	"Size", // USERINFO_VALIDATION_SIZE
	"# of slashes", // USERINFO_VALIDATION_SLASH
	"Extended ascii", // USERINFO_VALIDATION_EXTASCII
	"Control characters", // USERINFO_VALIDATION_CONTROLCHARS
};

void Svcmd_ToggleUserinfoValidation_f(void)
{
	if (trap->Argc() == 1)
	{
		int i;
		for (i = 0; i < numUserinfoFields; i++)
		{
			if (g_userinfoValidate.integer & 1 << i) trap->Print("%2d [X] %s\n", i, userinfoFields[i].fieldClean);
			else trap->Print("%2d [ ] %s\n", i, userinfoFields[i].fieldClean);
		}
		for (; i < numUserinfoFields + USERINFO_VALIDATION_MAX; i++)
		{
			if (g_userinfoValidate.integer & 1 << i)
				trap->Print("%2d [X] %s\n", i,
					userinfoValidateExtra[i - numUserinfoFields]);
			else trap->Print("%2d [ ] %s\n", i, userinfoValidateExtra[i - numUserinfoFields]);
		}
		return;
	}
	char arg[8] = { 0 };

	trap->Argv(1, arg, sizeof arg);
	const int index = atoi(arg);

	if (index < 0 || index > numUserinfoFields + USERINFO_VALIDATION_MAX - 1)
	{
		Com_Printf("ToggleUserinfoValidation: Invalid range: %i [0, %i]\n", index,
			numUserinfoFields + USERINFO_VALIDATION_MAX - 1);
		return;
	}

	trap->Cvar_Set("g_userinfoValidate",
		va("%i", (1 << index) ^ (g_userinfoValidate.integer & ((1 << (numUserinfoFields +
			USERINFO_VALIDATION_MAX)) - 1))));
	trap->Cvar_Update(&g_userinfoValidate);

	if (index < numUserinfoFields)
		Com_Printf("%s %s\n", userinfoFields[index].fieldClean,
			g_userinfoValidate.integer & 1 << index ? "Validated" : "Ignored");
	else
		Com_Printf("%s %s\n", userinfoValidateExtra[index - numUserinfoFields],
			g_userinfoValidate.integer & 1 << index ? "Validated" : "Ignored");
}

char* G_ValidateUserinfo(const char* userinfo)
{
	unsigned int i, count;
	const size_t length = strlen(userinfo);
	userinfoValidate_t* info;
	const char* s;
	unsigned int fieldCount[ARRAY_LEN(userinfoFields)] = { 0 };

	// size checks
	if (g_userinfoValidate.integer & (1 << (numUserinfoFields + USERINFO_VALIDATION_SIZE)))
	{
		if (length < 1)
			return "Userinfo too short";
		if (length >= MAX_INFO_STRING)
			return "Userinfo too long";
	}

	// slash checks
	if (g_userinfoValidate.integer & (1 << (numUserinfoFields + USERINFO_VALIDATION_SLASH)))
	{
		// there must be a leading slash
		if (userinfo[0] != '\\')
			return "Missing leading slash";

		// no trailing slashes allowed, engine will append ip\\ip:port
		if (userinfo[length - 1] == '\\')
			return "Trailing slash";

		// format for userinfo field is: \\key\\value
		// so there must be an even amount of slashes
		for (i = 0, count = 0; i < length; i++)
		{
			if (userinfo[i] == '\\')
				count++;
		}
		if (count & 1) // odd
			return "Bad number of slashes";
	}

	// extended characters are impossible to type, may want to disable
	if (g_userinfoValidate.integer & (1 << (numUserinfoFields + USERINFO_VALIDATION_EXTASCII)))
	{
		for (i = 0, count = 0; i < length; i++)
		{
			if (userinfo[i] < 0)
				count++;
		}
		if (count)
			return "Extended ASCII characters found";
	}

	// disallow \n \r ; and \"
	if (g_userinfoValidate.integer & (1 << (numUserinfoFields + USERINFO_VALIDATION_CONTROLCHARS)))
	{
		if (Q_strchrs(userinfo, "\n\r;\""))
			return "Invalid characters found";
	}

	s = userinfo;
	while (s)
	{
		char key[BIG_INFO_KEY];
		char value[BIG_INFO_VALUE];
		Info_NextPair(&s, key, value);

		if (!key[0])
			break;

		for (i = 0; i < numUserinfoFields; i++)
		{
			if (!Q_stricmp(key, userinfoFields[i].fieldClean))
				fieldCount[i]++;
		}
	}

	// count the number of fields
	for (i = 0, info = userinfoFields; i < numUserinfoFields; i++, info++)
	{
		if (g_userinfoValidate.integer & 1 << i)
		{
			if (info->minCount && !fieldCount[i])
				return va("%s field not found", info->fieldClean);
			if (fieldCount[i] > info->maxCount)
				return va("Too many %s fields (%i/%i)", info->fieldClean, fieldCount[i], info->maxCount);
		}
	}

	return NULL;
}

char lcase(char c)
{
	if (c >= 'A' && c <= 'Z')
	{
		c = c | 0x20;
	}
	return c;
}

int playermodel(char* haystack, char* needle)
{
	while (*haystack)
	{
		char* np = needle;

		while (*haystack && lcase(*haystack) != lcase(*np))
		{
			haystack++;
		}
		while (*haystack && *np && (lcase(*haystack) == lcase(*np) || *haystack == '^'))
		{
			if (*haystack == '^')
			{
				for (int i = 0; i < 2; i++)
				{
					if (*haystack)
					{
						haystack++;
					}
				}
			}
			else
			{
				if (*haystack)
				{
					*haystack++;
				}
				if (*np)
				{
					*np++;
				}
			}
		}
		if (!*np)
		{
			return 1;
		}
	}
	return 0;
}

void ScalePlayer(gentity_t* self, int scale)
{
	float fc = scale / 100.0f;

	if (!self)
		return;

	if (!self->client)
		return;

	if (!scale)
		fc = 1.0f;

	self->client->ps.iModelScale = scale;
	self->modelScale[0] = self->modelScale[1] = self->modelScale[2] = fc;
}

qboolean WinterGear = qfalse; //sets weither or not the models go for winter gear skins
qboolean client_userinfo_changed(const int client_num)
{
	gentity_t* ent = g_entities + client_num;
	gclient_t* client = ent->client;
	int team;
	int health;
	int max_health = 100;
	const char* value;
	char userinfo[MAX_INFO_STRING];
	char buf[MAX_INFO_STRING];
	char oldClientinfo[MAX_INFO_STRING];
	char model[MAX_QPATH];
	char forcePowers[DEFAULT_FORCEPOWERS_LEN];
	char oldname[MAX_NETNAME];
	char className[MAX_QPATH];
	char color1[16];
	char color2[16];
	qboolean model_changed = qfalse;
	gender_t gender;
	char rgb1[MAX_INFO_STRING];
	char rgb2[MAX_INFO_STRING];
	char script1[MAX_INFO_STRING];
	char script2[MAX_INFO_STRING];

	trap->GetUserinfo(client_num, userinfo, sizeof userinfo);

	// check for malformed or illegal info strings
	const char* s = G_ValidateUserinfo(userinfo);
	if (s && *s)
	{
		G_SecurityLogPrintf("Client %d (%s) failed userinfo validation: %s [IP: %s]\n", client_num,
			ent->client->pers.netname, s, client->sess.IP);
		trap->DropClient(client_num, va("Failed userinfo validation: %s", s));
		G_LogPrintf("Userinfo: %s\n", userinfo);
		return qfalse;
	}

	// check for local client
	s = Info_ValueForKey(userinfo, "ip");
	if (strcmp(s, "localhost") == 0 && !(ent->r.svFlags & SVF_BOT))
		client->pers.localClient = qtrue;

	// check the item prediction
	s = Info_ValueForKey(userinfo, "cg_predictItems");
	if (!atoi(s)) client->pers.predictItemPickup = qfalse;
	else client->pers.predictItemPickup = qtrue;

	// set name
	Q_strncpyz(oldname, client->pers.netname, sizeof oldname);
	s = Info_ValueForKey(userinfo, "name");
	client_clean_name(s, client->pers.netname, sizeof client->pers.netname);
	Q_strncpyz(client->pers.netname_nocolor, client->pers.netname, sizeof client->pers.netname_nocolor);
	Q_StripColor(client->pers.netname_nocolor);

	if (client->sess.sessionTeam == TEAM_SPECTATOR && client->sess.spectatorState == SPECTATOR_SCOREBOARD)
	{
		Q_strncpyz(client->pers.netname, "scoreboard", sizeof client->pers.netname);
		Q_strncpyz(client->pers.netname_nocolor, "scoreboard", sizeof client->pers.netname_nocolor);
	}

	if (client->pers.connected == CON_CONNECTED && strcmp(oldname, client->pers.netname) != 0)
	{
		if (client->pers.netnameTime > level.time)
		{
			trap->SendServerCommand(client_num, va("print \"%s\n\"", G_GetStringEdString("MP_SVGAME", "NONAMECHANGE")));

			Info_SetValueForKey(userinfo, "name", oldname);
			trap->SetUserinfo(client_num, userinfo);
			Q_strncpyz(client->pers.netname, oldname, sizeof client->pers.netname);
			Q_strncpyz(client->pers.netname_nocolor, oldname, sizeof client->pers.netname_nocolor);
			Q_StripColor(client->pers.netname_nocolor);
		}
		else
		{
			trap->SendServerCommand(-1, va("print \"%s" S_COLOR_WHITE " %s %s\n\"", oldname,
				G_GetStringEdString("MP_SVGAME", "PLRENAME"), client->pers.netname));
			G_LogPrintf("ClientRename: %i [%s] (%s) \"%s^7\" -> \"%s^7\"\n", client_num, ent->client->sess.IP,
				ent->client->pers.guid, oldname, ent->client->pers.netname);
			client->pers.netnameTime = level.time + 5000;
		}
	}

	// set model
	Q_strncpyz(model, Info_ValueForKey(userinfo, "model"), sizeof model);

	// load class system
	if (ent->s.eType != ET_NPC // no npcs,handled in npc.cfg
		&& level.gametype != GT_SIEGE)
	{
		if (playermodel(model, "model_siege")
			|| playermodel(model, "atst")
			|| playermodel(model, "gonk")
			|| playermodel(model, "hazardtrooper")
			|| playermodel(model, "lambdashuttle")
			|| playermodel(model, "marka_ragnos")
			|| playermodel(model, "probe")
			|| playermodel(model, "rocks")
			|| playermodel(model, "sand_creature")
			|| playermodel(model, "sentry")
			|| playermodel(model, "swoop")
			|| playermodel(model, "tauntaun")
			|| playermodel(model, "tie_bomber")
			|| playermodel(model, "tie_fighter")
			|| playermodel(model, "x-wing")
			|| playermodel(model, "z-95"))
		{
			// don't allow them to pick these models
			strcpy(model, DEFAULT_MODEL);
			strcpy(client->modelname, DEFAULT_MODEL);
			model_changed = qtrue;
			client->pers.botmodelscale = BOTZIZE_NORMAL;
		}

		if (playermodel(model, "aggl_dooku/main")
			|| playermodel(model, "aggl_dooku/")
			|| playermodel(model, "Jerec")
			|| playermodel(model, "darth_talon")
			|| playermodel(model, "darth_talon/")
			|| playermodel(model, "darth_talon/head_aa|torso_aa|lower_aa")
			|| playermodel(model, "chiss_jedi")
			|| playermodel(model, "chiss_jedi/")
			|| playermodel(model, "chiss_jedi/default")
			|| playermodel(model, "ani3d/main")
			|| playermodel(model, "ani3d/main2")
			|| playermodel(model, "ani3d/main3")
			|| playermodel(model, "ani3d/")
			|| playermodel(model, "Jerec2")
			|| playermodel(model, "Maula/main")
			|| playermodel(model, "Maula")
			|| playermodel(model, "maulb")
			|| playermodel(model, "maulb/main")
			|| playermodel(model, "maulsp")
			|| playermodel(model, "maulsp/main")
			|| playermodel(model, "Sith_Stalker2")
			|| playermodel(model, "Sith_Stalker2/default")
			|| playermodel(model, "Sith_Stalker")
			|| playermodel(model, "Sith_Stalker/red")
			|| playermodel(model, "Sith_Stalker/blue")
			|| playermodel(model, "Sith_Stalker/robe")
			|| playermodel(model, "Sith_Stalker/robehood")
			|| playermodel(model, "Sith_Stalker/siege"))
		{
			client->pers.nextbotclass = BCLASS_SITHWORRIOR1;
			client->pers.botmodelscale = BOTZIZE_NORMAL;
			if (!(ent->r.svFlags & SVF_BOT))
			{
				if (g_gametype.integer != GT_DUEL && g_gametype.integer != GT_POWERDUEL && g_gametype.integer !=
					GT_SIEGE)
				{
					client->ps.stats[STAT_HEALTH] = ent->health = 0;
					player_die(ent, ent, ent, 100000, MOD_TEAM_CHANGE);
					trap->UnlinkEntity((sharedEntity_t*)ent);
				}
				Com_Printf("Changes to your Class settings will take effect the next time you respawn.\n");
			}
		}
		else if (playermodel(model, "alora")
			|| playermodel(model, "alora/red")
			|| playermodel(model, "alorablue")
			|| playermodel(model, "alora2")
			|| playermodel(model, "mara")
			|| playermodel(model, "malak")
			|| playermodel(model, "malek"))
		{
			client->pers.nextbotclass = BCLASS_ALORA;
			client->pers.botmodelscale = BOTZIZE_NORMAL;
			if (!(ent->r.svFlags & SVF_BOT))
			{
				if (g_gametype.integer != GT_DUEL && g_gametype.integer != GT_POWERDUEL && g_gametype.integer !=
					GT_SIEGE)
				{
					client->ps.stats[STAT_HEALTH] = ent->health = 0;
					player_die(ent, ent, ent, 100000, MOD_TEAM_CHANGE);
					trap->UnlinkEntity((sharedEntity_t*)ent);
				}
				Com_Printf("Changes to your Class settings will take effect the next time you respawn.\n");
			}
		}
		else if (playermodel(model, "assassin_droid"))
		{
			client->pers.nextbotclass = BCLASS_ASSASSIN_DROID;
			client->pers.botmodelscale = BOTZIZE_LARGE;
			if (!(ent->r.svFlags & SVF_BOT))
			{
				if (g_gametype.integer != GT_DUEL && g_gametype.integer != GT_POWERDUEL && g_gametype.integer !=
					GT_SIEGE)
				{
					client->ps.stats[STAT_HEALTH] = ent->health = 0;
					player_die(ent, ent, ent, 100000, MOD_TEAM_CHANGE);
					trap->UnlinkEntity((sharedEntity_t*)ent);
				}
				Com_Printf("Changes to your Class settings will take effect the next time you respawn.\n");
			}
		}
		else if (playermodel(model, "biker_scout")
			|| playermodel(model, "rebel_pilot/main")
			|| playermodel(model, "bespin_cop/main")
			|| playermodel(model, "bespin_cop")
			|| playermodel(model, "bespin_cop/red")
			|| playermodel(model, "bespin_cop/blue")
			|| playermodel(model, "aurrasing/default")
			|| playermodel(model, "aurrasing"))
		{
			client->pers.nextbotclass = BCLASS_BESPIN_COP;
			client->pers.botmodelscale = BOTZIZE_NORMAL;
			if (!(ent->r.svFlags & SVF_BOT))
			{
				if (g_gametype.integer != GT_DUEL && g_gametype.integer != GT_POWERDUEL && g_gametype.integer !=
					GT_SIEGE)
				{
					client->ps.stats[STAT_HEALTH] = ent->health = 0;
					player_die(ent, ent, ent, 100000, MOD_TEAM_CHANGE);
					trap->UnlinkEntity((sharedEntity_t*)ent);
				}
				Com_Printf("Changes to your Class settings will take effect the next time you respawn.\n");
			}
		}
		else if (playermodel(model, "lahansolo/main")
			|| playermodel(model, "jynerso/default")
			|| playermodel(model, "jynerso")
			|| playermodel(model, "han_tfa")
			|| playermodel(model, "han_anh")
			|| playermodel(model, "han_anh/")
			|| playermodel(model, "han_anh/default")
			|| playermodel(model, "han_esb")
			|| playermodel(model, "han_esb/")
			|| playermodel(model, "han_esb/default")
			|| playermodel(model, "oldhan/default")
			|| playermodel(model, "oldhan")
			|| playermodel(model, "hansolo/main")
			|| playermodel(model, "hansolo/main2")
			|| playermodel(model, "sthan/main")
			|| playermodel(model, "ceclonepilot/main")
			|| playermodel(model, "stormpilot")
			|| playermodel(model, "stormpilot/blue")
			|| playermodel(model, "stormpilot/red")
			|| playermodel(model, "stormpilot/main")
			|| playermodel(model, "at/main")
			|| playermodel(model, "atp/main")
			|| playermodel(model, "st_poe")
			|| playermodel(model, "jedi_st_poe")
			|| playermodel(model, "Krennic")
			|| playermodel(model, "dash_rendar/default")
			|| playermodel(model, "dash_rendar"))
		{
			client->pers.nextbotclass = BCLASS_STORMPILOT;
			client->pers.botmodelscale = BOTZIZE_NORMAL;
			if (!(ent->r.svFlags & SVF_BOT))
			{
				if (g_gametype.integer != GT_DUEL && g_gametype.integer != GT_POWERDUEL && g_gametype.integer !=
					GT_SIEGE)
				{
					client->ps.stats[STAT_HEALTH] = ent->health = 0;
					player_die(ent, ent, ent, 100000, MOD_TEAM_CHANGE);
					trap->UnlinkEntity((sharedEntity_t*)ent);
				}
				Com_Printf("Changes to your Class settings will take effect the next time you respawn.\n");
			}
		}
		else if (playermodel(model, "trooper3/default")
			|| playermodel(model, "MandoloriansPac/default_")
			|| playermodel(model, "Bountyhunter3/default")
			|| playermodel(model, "boba_fett")
			|| playermodel(model, "boba_fett/main")
			|| playermodel(model, "boba_fett/main2")
			|| playermodel(model, "jangoJA_fett/")
			|| playermodel(model, "jangoJA_fett/main")
			|| playermodel(model, "jangoJA_fett/main2")
			|| playermodel(model, "jangoJA_fett/default"))
		{
			client->pers.nextbotclass = BCLASS_BOBAFETT;
			client->pers.botmodelscale = BOTZIZE_NORMAL;
			if (!(ent->r.svFlags & SVF_BOT))
			{
				if (g_gametype.integer != GT_DUEL && g_gametype.integer != GT_POWERDUEL && g_gametype.integer !=
					GT_SIEGE)
				{
					client->ps.stats[STAT_HEALTH] = ent->health = 0;
					player_die(ent, ent, ent, 100000, MOD_TEAM_CHANGE);
					trap->UnlinkEntity((sharedEntity_t*)ent);
				}
				Com_Printf("Changes to your Class settings will take effect the next time you respawn.\n");
			}
		}
		else if (playermodel(model, "chiss")
			|| playermodel(model, "chiss/red")
			|| playermodel(model, "chiss/blue"))
		{
			client->pers.nextbotclass = BCLASS_BARTENDER;
			client->pers.botmodelscale = BOTZIZE_NORMAL;
			if (!(ent->r.svFlags & SVF_BOT))
			{
				if (g_gametype.integer != GT_DUEL && g_gametype.integer != GT_POWERDUEL && g_gametype.integer !=
					GT_SIEGE)
				{
					client->ps.stats[STAT_HEALTH] = ent->health = 0;
					player_die(ent, ent, ent, 100000, MOD_TEAM_CHANGE);
					trap->UnlinkEntity((sharedEntity_t*)ent);
				}
				Com_Printf("Changes to your Class settings will take effect the next time you respawn.\n");
			}
		}
		else if (playermodel(model, "chewbacca")
			|| playermodel(model, "chewbacca2")
			|| playermodel(model, "chewbacca_bespin")
			|| playermodel(model, "koWookiee")
			|| playermodel(model, "koWookiee/main")
			|| playermodel(model, "chewbacca/default")
			|| playermodel(model, "chewbacca/red")
			|| playermodel(model, "chewbacca/blue")
			|| playermodel(model, "cadbane"))
		{
			client->pers.botmodelscale = BOTZIZE_MASSIVE;
			client->pers.nextbotclass = BCLASS_CHEWIE;
			if (!(ent->r.svFlags & SVF_BOT))
			{
				if (g_gametype.integer != GT_DUEL && g_gametype.integer != GT_POWERDUEL && g_gametype.integer !=
					GT_SIEGE)
				{
					client->ps.stats[STAT_HEALTH] = ent->health = 0;
					player_die(ent, ent, ent, 100000, MOD_TEAM_CHANGE);
					trap->UnlinkEntity((sharedEntity_t*)ent);
				}
				Com_Printf("Changes to your Class settings will take effect the next time you respawn.\n");
			}
		}
		else if (playermodel(model, "wookiee/default"))
		{
			client->pers.botmodelscale = BOTZIZE_MASSIVE;
			client->pers.nextbotclass = BCLASS_CHEWIE;
			if (!(ent->r.svFlags & SVF_BOT))
			{
				if (g_gametype.integer != GT_DUEL && g_gametype.integer != GT_POWERDUEL && g_gametype.integer !=
					GT_SIEGE)
				{
					client->ps.stats[STAT_HEALTH] = ent->health = 0;
					player_die(ent, ent, ent, 100000, MOD_TEAM_CHANGE);
					trap->UnlinkEntity((sharedEntity_t*)ent);
				}
				Com_Printf("Changes to your Class settings will take effect the next time you respawn.\n");
			}
		}
		else if (playermodel(model, "wookiee/blue")
			|| playermodel(model, "zaalbar"))
		{
			client->pers.botmodelscale = BOTZIZE_MASSIVE;
			client->pers.nextbotclass = BCLASS_WOOKIEMELEE;
			if (!(ent->r.svFlags & SVF_BOT))
			{
				if (g_gametype.integer != GT_DUEL && g_gametype.integer != GT_POWERDUEL && g_gametype.integer !=
					GT_SIEGE)
				{
					client->ps.stats[STAT_HEALTH] = ent->health = 0;
					player_die(ent, ent, ent, 100000, MOD_TEAM_CHANGE);
					trap->UnlinkEntity((sharedEntity_t*)ent);
				}
				Com_Printf("Changes to your Class settings will take effect the next time you respawn.\n");
			}
		}
		else if (playermodel(model, "wookiee/red"))
		{
			client->pers.botmodelscale = BOTZIZE_MASSIVE;
			client->pers.nextbotclass = BCLASS_WOOKIE;
			if (!(ent->r.svFlags & SVF_BOT))
			{
				if (g_gametype.integer != GT_DUEL && g_gametype.integer != GT_POWERDUEL && g_gametype.integer !=
					GT_SIEGE)
				{
					client->ps.stats[STAT_HEALTH] = ent->health = 0;
					player_die(ent, ent, ent, 100000, MOD_TEAM_CHANGE);
					trap->UnlinkEntity((sharedEntity_t*)ent);
				}
				Com_Printf("Changes to your Class settings will take effect the next time you respawn.\n");
			}
		}
		else if (playermodel(model, "mandalore")
			|| playermodel(model, "MandalorianBlack")
			|| playermodel(model, "MandalorianBlue")
			|| playermodel(model, "MandalorianGrey")
			|| playermodel(model, "MandalorianRed")
			|| playermodel(model, "Mandalorian"))
		{
			client->pers.botmodelscale = BOTZIZE_MASSIVE;
			client->pers.nextbotclass = BCLASS_WOOKIE;
			if (!(ent->r.svFlags & SVF_BOT))
			{
				if (g_gametype.integer != GT_DUEL && g_gametype.integer != GT_POWERDUEL && g_gametype.integer !=
					GT_SIEGE)
				{
					client->ps.stats[STAT_HEALTH] = ent->health = 0;
					player_die(ent, ent, ent, 100000, MOD_TEAM_CHANGE);
					trap->UnlinkEntity((sharedEntity_t*)ent);
				}
				Com_Printf("Changes to your Class settings will take effect the next time you respawn.\n");
			}
		}
		else if (playermodel(model, "bao_dur")
			|| playermodel(model, "bith")
			|| playermodel(model, "derrik")
			|| playermodel(model, "Duros2")
			|| playermodel(model, "Duros3")
			|| playermodel(model, "Duros4")
			|| playermodel(model, "Duros5")
			|| playermodel(model, "Duros6")
			|| playermodel(model, "mission")
			|| playermodel(model, "Rakata")
			|| playermodel(model, "Selkath")
			|| playermodel(model, "sithsoldier")
			|| playermodel(model, "talia")
			|| playermodel(model, "TwinSuns"))
		{
			client->pers.nextbotclass = BCLASS_WOOKIE;
			if (!(ent->r.svFlags & SVF_BOT))
			{
				if (g_gametype.integer != GT_DUEL && g_gametype.integer != GT_POWERDUEL && g_gametype.integer !=
					GT_SIEGE)
				{
					client->ps.stats[STAT_HEALTH] = ent->health = 0;
					player_die(ent, ent, ent, 100000, MOD_TEAM_CHANGE);
					trap->UnlinkEntity((sharedEntity_t*)ent);
				}
				Com_Printf("Changes to your Class settings will take effect the next time you respawn.\n");
			}
		}
		else if (playermodel(model, "cultist")
			|| playermodel(model, "cultist/red")
			|| playermodel(model, "cultist/blue")
			|| playermodel(model, "cultist/"))
		{
			client->pers.nextbotclass = BCLASS_CULTIST;
			client->pers.botmodelscale = BOTZIZE_NORMAL;
			if (!(ent->r.svFlags & SVF_BOT))
			{
				if (g_gametype.integer != GT_DUEL && g_gametype.integer != GT_POWERDUEL && g_gametype.integer !=
					GT_SIEGE)
				{
					client->ps.stats[STAT_HEALTH] = ent->health = 0;
					player_die(ent, ent, ent, 100000, MOD_TEAM_CHANGE);
					trap->UnlinkEntity((sharedEntity_t*)ent);
				}
				Com_Printf("Changes to your Class settings will take effect the next time you respawn.\n");
			}
		}
		else if (playermodel(model, "desann")
			|| playermodel(model, "desann/main")
			|| playermodel(model, "desann/red")
			|| playermodel(model, "desann/blue")
			|| playermodel(model, "desann/dark_robed")
			|| playermodel(model, "desann/unrobed")
			|| playermodel(model, "desann/default"))
		{
			client->pers.botmodelscale = BOTZIZE_LARGE;
			client->pers.nextbotclass = BCLASS_DESANN;
			if (!(ent->r.svFlags & SVF_BOT))
			{
				if (g_gametype.integer != GT_DUEL && g_gametype.integer != GT_POWERDUEL && g_gametype.integer !=
					GT_SIEGE)
				{
					client->ps.stats[STAT_HEALTH] = ent->health = 0;
					player_die(ent, ent, ent, 100000, MOD_TEAM_CHANGE);
					trap->UnlinkEntity((sharedEntity_t*)ent);
				}
				Com_Printf("Changes to your Class settings will take effect the next time you respawn.\n");
			}
		}
		else if (playermodel(model, "kylo_ren")
			|| playermodel(model, "kylo")
			|| playermodel(model, "kylomp")
			|| playermodel(model, "KyloRen")
			|| playermodel(model, "KyloRenK")
			|| playermodel(model, "kylo_ren/")
			|| playermodel(model, "Matt_TRT")
			|| playermodel(model, "batman_begins")
			|| playermodel(model, "kylo_ren_nomask"))
		{
			client->pers.botmodelscale = BOTZIZE_TALL;
			client->pers.nextbotclass = BCLASS_UNSTABLESABER;
			if (!(ent->r.svFlags & SVF_BOT))
			{
				if (g_gametype.integer != GT_DUEL && g_gametype.integer != GT_POWERDUEL && g_gametype.integer !=
					GT_SIEGE)
				{
					client->ps.stats[STAT_HEALTH] = ent->health = 0;
					player_die(ent, ent, ent, 100000, MOD_TEAM_CHANGE);
					trap->UnlinkEntity((sharedEntity_t*)ent);
				}
				Com_Printf("Changes to your Class settings will take effect the next time you respawn.\n");
			}
		}
		else if (playermodel(model, "prisoner")
			|| playermodel(model, "prisoner/red")
			|| playermodel(model, "prisoner/blue"))
		{
			client->pers.nextbotclass = BCLASS_ELDER;
			client->pers.botmodelscale = BOTZIZE_NORMAL;
			if (!(ent->r.svFlags & SVF_BOT))
			{
				if (g_gametype.integer != GT_DUEL && g_gametype.integer != GT_POWERDUEL && g_gametype.integer !=
					GT_SIEGE)
				{
					client->ps.stats[STAT_HEALTH] = ent->health = 0;
					player_die(ent, ent, ent, 100000, MOD_TEAM_CHANGE);
					trap->UnlinkEntity((sharedEntity_t*)ent);
				}
				Com_Printf("Changes to your Class settings will take effect the next time you respawn.\n");
			}
		}
		else if (playermodel(model, "galak")
			|| playermodel(model, "galak/red")
			|| playermodel(model, "galak/blue")
			|| playermodel(model, "hothelite/default")
			|| playermodel(model, "hothelite")
			|| playermodel(model, "kejarjar/main")
			|| playermodel(model, "kagungan/main"))
		{
			client->pers.nextbotclass = BCLASS_GALAK;
			client->pers.botmodelscale = BOTZIZE_NORMAL;
			if (!(ent->r.svFlags & SVF_BOT))
			{
				if (g_gametype.integer != GT_DUEL && g_gametype.integer != GT_POWERDUEL && g_gametype.integer !=
					GT_SIEGE)
				{
					client->ps.stats[STAT_HEALTH] = ent->health = 0;
					player_die(ent, ent, ent, 100000, MOD_TEAM_CHANGE);
					trap->UnlinkEntity((sharedEntity_t*)ent);
				}
				Com_Printf("Changes to your Class settings will take effect the next time you respawn.\n");
			}
		}
		else if (playermodel(model, "galakmech")
			|| playermodel(model, "galakmech/default")
			|| playermodel(model, "galakmech/red")
			|| playermodel(model, "galakmech/blue")
			|| playermodel(model, "galak_mech")
			|| playermodel(model, "galak_mech/red")
			|| playermodel(model, "galak_mech/blue"))
		{
			client->pers.nextbotclass = BCLASS_GALAKMECH;
			client->pers.botmodelscale = BOTZIZE_LARGER;
			if (!(ent->r.svFlags & SVF_BOT))
			{
				if (g_gametype.integer != GT_DUEL && g_gametype.integer != GT_POWERDUEL && g_gametype.integer !=
					GT_SIEGE)
				{
					client->ps.stats[STAT_HEALTH] = ent->health = 0;
					player_die(ent, ent, ent, 100000, MOD_TEAM_CHANGE);
					trap->UnlinkEntity((sharedEntity_t*)ent);
				}
				Com_Printf("Changes to your Class settings will take effect the next time you respawn.\n");
			}
		}
		else if (playermodel(model, "canderous")
			|| playermodel(model, "OldRepSold")
			|| playermodel(model, "OpoChano")
			|| playermodel(model, "republic_officer"))
		{
			client->pers.nextbotclass = BCLASS_BOUNTYHUNTER1;
			client->pers.botmodelscale = BOTZIZE_TALL;
			if (!(ent->r.svFlags & SVF_BOT))
			{
				if (g_gametype.integer != GT_DUEL && g_gametype.integer != GT_POWERDUEL && g_gametype.integer !=
					GT_SIEGE)
				{
					client->ps.stats[STAT_HEALTH] = ent->health = 0;
					player_die(ent, ent, ent, 100000, MOD_TEAM_CHANGE);
					trap->UnlinkEntity((sharedEntity_t*)ent);
				}
				Com_Printf("Changes to your Class settings will take effect the next time you respawn.\n");
			}
		}
		else if (playermodel(model, "gran")
			|| playermodel(model, "gran/red")
			|| playermodel(model, "gran/blue")
			|| playermodel(model, "gran/main")
			|| playermodel(model, "ta/main")
			|| playermodel(model, "it/main"))
		{
			client->pers.nextbotclass = BCLASS_GRAN;
			client->pers.botmodelscale = BOTZIZE_LARGE;
			if (!(ent->r.svFlags & SVF_BOT))
			{
				if (g_gametype.integer != GT_DUEL && g_gametype.integer != GT_POWERDUEL && g_gametype.integer !=
					GT_SIEGE)
				{
					client->ps.stats[STAT_HEALTH] = ent->health = 0;
					player_die(ent, ent, ent, 100000, MOD_TEAM_CHANGE);
					trap->UnlinkEntity((sharedEntity_t*)ent);
				}
				Com_Printf("Changes to your Class settings will take effect the next time you respawn.\n");
			}
		}
		else if (playermodel(model, "hazardtrooper")
			|| playermodel(model, "hazardtrooper/red")
			|| playermodel(model, "hazardtrooper/blue"))
		{
			client->pers.nextbotclass = BCLASS_HAZARDTROOPER;
			client->pers.botmodelscale = BOTZIZE_LARGER;
			if (!(ent->r.svFlags & SVF_BOT))
			{
				if (g_gametype.integer != GT_DUEL && g_gametype.integer != GT_POWERDUEL && g_gametype.integer !=
					GT_SIEGE)
				{
					client->ps.stats[STAT_HEALTH] = ent->health = 0;
					player_die(ent, ent, ent, 100000, MOD_TEAM_CHANGE);
					trap->UnlinkEntity((sharedEntity_t*)ent);
				}
				Com_Printf("Changes to your Class settings will take effect the next time you respawn.\n");
			}
		}
		else if (playermodel(model, "human_merc")
			|| playermodel(model, "human_merc/red")
			|| playermodel(model, "human_merc/blue")
			|| playermodel(model, "Swoopgang"))
		{
			client->pers.nextbotclass = BCLASS_HUMAN_MERC;
			client->pers.botmodelscale = BOTZIZE_NORMAL;
			if (!(ent->r.svFlags & SVF_BOT))
			{
				if (g_gametype.integer != GT_DUEL && g_gametype.integer != GT_POWERDUEL && g_gametype.integer !=
					GT_SIEGE)
				{
					client->ps.stats[STAT_HEALTH] = ent->health = 0;
					player_die(ent, ent, ent, 100000, MOD_TEAM_CHANGE);
					trap->UnlinkEntity((sharedEntity_t*)ent);
				}
				Com_Printf("Changes to your Class settings will take effect the next time you respawn.\n");
			}
		}
		else if (playermodel(model, "imperial")
			|| playermodel(model, "imperial/main")
			|| playermodel(model, "imperial/main2")
			|| playermodel(model, "imperial/main3"))
		{
			client->pers.nextbotclass = BCLASS_IMPERIAL;
			client->pers.botmodelscale = BOTZIZE_NORMAL;
			if (!(ent->r.svFlags & SVF_BOT))
			{
				if (g_gametype.integer != GT_DUEL && g_gametype.integer != GT_POWERDUEL && g_gametype.integer !=
					GT_SIEGE)
				{
					client->ps.stats[STAT_HEALTH] = ent->health = 0;
					player_die(ent, ent, ent, 100000, MOD_TEAM_CHANGE);
					trap->UnlinkEntity((sharedEntity_t*)ent);
				}
				Com_Printf("Changes to your Class settings will take effect the next time you respawn.\n");
			}
		}
		else if (playermodel(model, "imperial/red")
			|| playermodel(model, "imperial/blue"))
		{
			client->pers.nextbotclass = BCLASS_IPPERIALAGENT3;
			client->pers.botmodelscale = BOTZIZE_NORMAL;
			if (!(ent->r.svFlags & SVF_BOT))
			{
				if (g_gametype.integer != GT_DUEL && g_gametype.integer != GT_POWERDUEL && g_gametype.integer !=
					GT_SIEGE)
				{
					client->ps.stats[STAT_HEALTH] = ent->health = 0;
					player_die(ent, ent, ent, 100000, MOD_TEAM_CHANGE);
					trap->UnlinkEntity((sharedEntity_t*)ent);
				}
				Com_Printf("Changes to your Class settings will take effect the next time you respawn.\n");
			}
		}
		else if (playermodel(model, "imperial_worker")
			|| playermodel(model, "imperial_worker/red")
			|| playermodel(model, "imperial_worker/blue"))
		{
			client->pers.nextbotclass = BCLASS_IMPWORKER;
			client->pers.botmodelscale = BOTZIZE_NORMAL;
			if (!(ent->r.svFlags & SVF_BOT))
			{
				if (g_gametype.integer != GT_DUEL && g_gametype.integer != GT_POWERDUEL && g_gametype.integer !=
					GT_SIEGE)
				{
					client->ps.stats[STAT_HEALTH] = ent->health = 0;
					player_die(ent, ent, ent, 100000, MOD_TEAM_CHANGE);
					trap->UnlinkEntity((sharedEntity_t*)ent);
				}
				Com_Printf("Changes to your Class settings will take effect the next time you respawn.\n");
			}
		}
		else if (playermodel(model, "jan")
			|| playermodel(model, "jan/red")
			|| playermodel(model, "jan/blue")
			|| playermodel(model, "leia_TFA")
			|| playermodel(model, "leia/default")
			|| playermodel(model, "leia_anh/main")
			|| playermodel(model, "leia_anh/default")
			|| playermodel(model, "leia_esb")
			|| playermodel(model, "leia_esb/")
			|| playermodel(model, "leia_esb/default")
			|| playermodel(model, "LeiaV/main")
			|| playermodel(model, "princess/main")
			|| playermodel(model, "princess/main2")
			|| playermodel(model, "princess/main3")
			|| playermodel(model, "padme3a/main")
			|| playermodel(model, "padme3b/main")
			|| playermodel(model, "padmebt/main")
			|| playermodel(model, "st_finn")
			|| playermodel(model, "jedi_st_finn")
			|| playermodel(model, "padme/main")
			|| playermodel(model, "leia_hoth")
			|| playermodel(model, "leia_hoth/default")
			|| playermodel(model, "atton")
			|| playermodel(model, "carth")
			|| playermodel(model, "mira"))
		{
			client->pers.nextbotclass = BCLASS_JAN;
			client->pers.botmodelscale = BOTZIZE_NORMAL;
			if (!(ent->r.svFlags & SVF_BOT))
			{
				if (g_gametype.integer != GT_DUEL && g_gametype.integer != GT_POWERDUEL && g_gametype.integer !=
					GT_SIEGE)
				{
					client->ps.stats[STAT_HEALTH] = ent->health = 0;
					player_die(ent, ent, ent, 100000, MOD_TEAM_CHANGE);
					trap->UnlinkEntity((sharedEntity_t*)ent);
				}
				Com_Printf("Changes to your Class settings will take effect the next time you respawn.\n");
			}
		}
		else if (playermodel(model, "jawa")
			|| playermodel(model, "jawa/red")
			|| playermodel(model, "jawa/blue")
			|| playermodel(model, "jawa/main"))
		{
			client->pers.botmodelscale = BOTZIZE_SMALLER;
			client->pers.nextbotclass = BCLASS_JAWA;
			if (!(ent->r.svFlags & SVF_BOT))
			{
				if (g_gametype.integer != GT_DUEL && g_gametype.integer != GT_POWERDUEL && g_gametype.integer !=
					GT_SIEGE)
				{
					client->ps.stats[STAT_HEALTH] = ent->health = 0;
					player_die(ent, ent, ent, 100000, MOD_TEAM_CHANGE);
					trap->UnlinkEntity((sharedEntity_t*)ent);
				}
				Com_Printf("Changes to your Class settings will take effect the next time you respawn.\n");
			}
		}
		else if (playermodel(model, "jedi")
			|| playermodel(model, "quinlan_vos")
			|| playermodel(model, "quinlan_vos2")
			|| playermodel(model, "jedi_st_tiplee")
			|| playermodel(model, "jedi_st_tiplee/")
			|| playermodel(model, "Eeth_Koth/main")
			|| playermodel(model, "Eeth_Koth_mp")
			|| playermodel(model, "st_tiplee/default")
			|| playermodel(model, "st_tiplee")
			|| playermodel(model, "mja/")
			|| playermodel(model, "mja/main")
			|| playermodel(model, "mj/")
			|| playermodel(model, "mj/main")
			|| playermodel(model, "jedi/red")
			|| playermodel(model, "jedi/blue")
			|| playermodel(model, "obi3/main4")
			|| playermodel(model, "muwindu/main")
			|| playermodel(model, "mmKiadimundi/main")
			|| playermodel(model, "bar/main")
			|| playermodel(model, "shaak_ti/main")
			|| playermodel(model, "lu/main")
			|| playermodel(model, "cin_drallig_tm")
			|| playermodel(model, "cin_drallig_tm/")
			|| playermodel(model, "cin_drallig_tm/default")
			|| playermodel(model, "cin_drallig_tm/default_robed")
			|| playermodel(model, "eerin_bant_robe")
			|| playermodel(model, "eerin_bant_robe/")
			|| playermodel(model, "eerin_bant_robe/default")
			|| playermodel(model, "anavader")
			|| playermodel(model, "anavader/")
			|| playermodel(model, "anavader/default")
			|| playermodel(model, "agen_kolar")
			|| playermodel(model, "agen_kolar/")
			|| playermodel(model, "agen_kolar/default")
			|| playermodel(model, "mlkfisto/main")
			|| playermodel(model, "kit_fisto/default")
			|| playermodel(model, "kit_fisto")
			|| playermodel(model, "Coleman/main")
			|| playermodel(model, "coleman_trebor_vm")
			|| playermodel(model, "coleman_trebor_vm/")
			|| playermodel(model, "coleman_trebor_vm/default")
			|| playermodel(model, "saesee_tiin/main")
			|| playermodel(model, "mhplokoon/main")
			|| playermodel(model, "mkyarael/main")
			|| playermodel(model, "Anakin_JA/main")
			|| playermodel(model, "ani3/main")
			|| playermodel(model, "ani3/main2")
			|| playermodel(model, "ani3/main3")
			|| playermodel(model, "jediknight1/default")
			|| playermodel(model, "jediknight2/default")
			|| playermodel(model, "jediknight3/default")
			|| playermodel(model, "jediconsular1/default")
			|| playermodel(model, "jediconsular2/default")
			|| playermodel(model, "jediconsular3/default")
			|| playermodel(model, "Obinew2/main")
			|| playermodel(model, "jedi_spanki1a")
			|| playermodel(model, "jedi_spanki1b")
			|| playermodel(model, "jedi_spanki2")
			|| playermodel(model, "jedi_spanki2a")
			|| playermodel(model, "jedi_spanki2b")
			|| playermodel(model, "jedi_spanki3")
			|| playermodel(model, "jedi_spanki3a")
			|| playermodel(model, "jedi_spanki3b")
			|| playermodel(model, "jedi_spanki4")
			|| playermodel(model, "jedi_spanki4a")
			|| playermodel(model, "jedi_spanki4b")
			|| playermodel(model, "jedi_spanki5")
			|| playermodel(model, "jedi_spanki5a")
			|| playermodel(model, "jedi_spanki5b")
			|| playermodel(model, "jedi_spanki6")
			|| playermodel(model, "jedi_spanki6a")
			|| playermodel(model, "jedi_spanki6b")
			|| playermodel(model, "jedi_spanki")
			|| playermodel(model, "spiderman")
			|| playermodel(model, "Wolverine")
			|| playermodel(model, "SD_tmnt")
			|| playermodel(model, "jedi_hm")
			|| playermodel(model, "ExileMaleLightSide")
			|| playermodel(model, "ExileMaleLightSideUR")
			|| playermodel(model, "Kavar")
			|| playermodel(model, "kreia")
			|| playermodel(model, "Vandar")
			|| playermodel(model, "Vandar_ghost")
			|| playermodel(model, "Visas")
			|| playermodel(model, "VrookLamar")
			|| playermodel(model, "cal_kestis")
			|| playermodel(model, "cal_kestis/cape")
			|| playermodel(model, "cal_kestis/cape2")
			|| playermodel(model, "cal_kestis/cape3")
			|| playermodel(model, "cal_kestis/cape4")
			|| playermodel(model, "cal_kestis/cape5")
			|| playermodel(model, "cal_kestis/cape6")
			|| playermodel(model, "cal_kestis/cape7")
			|| playermodel(model, "cal_kestis/default2")
			|| playermodel(model, "cal_kestis/default3")
			|| playermodel(model, "cal_kestis/default4"))
		{
			client->pers.nextbotclass = BCLASS_JEDI;
			client->pers.botmodelscale = BOTZIZE_NORMAL;
			if (!(ent->r.svFlags & SVF_BOT))
			{
				if (g_gametype.integer != GT_DUEL && g_gametype.integer != GT_POWERDUEL && g_gametype.integer !=
					GT_SIEGE)
				{
					client->ps.stats[STAT_HEALTH] = ent->health = 0;
					player_die(ent, ent, ent, 100000, MOD_TEAM_CHANGE);
					trap->UnlinkEntity((sharedEntity_t*)ent);
				}
				Com_Printf("Changes to your Class settings will take effect the next time you respawn.\n");
			}
		}
		else if (playermodel(model, "Jedi_GenericFemale1")
			|| playermodel(model, "Jedi_GenericFemale1A")
			|| playermodel(model, "Jedi_GenericFemale2")
			|| playermodel(model, "Jedi_GenericFemale2A")
			|| playermodel(model, "Jedi_GenericFemale3")
			|| playermodel(model, "Jedi_GenericFemale3A")
			|| playermodel(model, "Female_Jedi")
			|| playermodel(model, "lxjade/main")
			|| playermodel(model, "jedi_hf")
			|| playermodel(model, "ExileFemaleLightSide")
			|| playermodel(model, "ExileFemaleLightSideUR")
			|| playermodel(model, "Female_Jedi"))
		{
			client->pers.botmodelscale = BOTZIZE_SMALL;
			client->pers.nextbotclass = BCLASS_JEDI;
			if (!(ent->r.svFlags & SVF_BOT))
			{
				if (g_gametype.integer != GT_DUEL && g_gametype.integer != GT_POWERDUEL && g_gametype.integer !=
					GT_SIEGE)
				{
					client->ps.stats[STAT_HEALTH] = ent->health = 0;
					player_die(ent, ent, ent, 100000, MOD_TEAM_CHANGE);
					trap->UnlinkEntity((sharedEntity_t*)ent);
				}
				Com_Printf("Changes to your Class settings will take effect the next time you respawn.\n");
			}
		}
		else if (playermodel(model, "jedi/master"))
		{
			client->pers.nextbotclass = BCLASS_JEDIMASTER;
			client->pers.botmodelscale = BOTZIZE_NORMAL;
			if (!(ent->r.svFlags & SVF_BOT))
			{
				if (g_gametype.integer != GT_DUEL && g_gametype.integer != GT_POWERDUEL && g_gametype.integer !=
					GT_SIEGE)
				{
					client->ps.stats[STAT_HEALTH] = ent->health = 0;
					player_die(ent, ent, ent, 100000, MOD_TEAM_CHANGE);
					trap->UnlinkEntity((sharedEntity_t*)ent);
				}
				Com_Printf("Changes to your Class settings will take effect the next time you respawn.\n");
			}
		}
		else if (playermodel(model, "jeditrainer")
			|| playermodel(model, "jeditrainer/red")
			|| playermodel(model, "jeditrainer/blue"))
		{
			client->pers.nextbotclass = BCLASS_JEDITRAINER;
			client->pers.botmodelscale = BOTZIZE_NORMAL;
			if (!(ent->r.svFlags & SVF_BOT))
			{
				if (g_gametype.integer != GT_DUEL && g_gametype.integer != GT_POWERDUEL && g_gametype.integer !=
					GT_SIEGE)
				{
					client->ps.stats[STAT_HEALTH] = ent->health = 0;
					player_die(ent, ent, ent, 100000, MOD_TEAM_CHANGE);
					trap->UnlinkEntity((sharedEntity_t*)ent);
				}
				Com_Printf("Changes to your Class settings will take effect the next time you respawn.\n");
			}
		}
		else if (playermodel(model, "kyle")
			|| playermodel(model, "kyle/main")
			|| playermodel(model, "kyle/red")
			|| playermodel(model, "kyle/blue")
			|| playermodel(model, "Jedi_Genericfemale")
			|| playermodel(model, "wzoldben/main3")
			|| playermodel(model, "wzoldben/main2")
			|| playermodel(model, "ntobiwan/main")
			|| playermodel(model, "wzoldben/main")
			|| playermodel(model, "Obinew2/main")
			|| playermodel(model, "Obinew2/main2")
			|| playermodel(model, "Obinew2/main3")
			|| playermodel(model, "obi3/main")
			|| playermodel(model, "obi3/main2")
			|| playermodel(model, "obi3/main4")
			|| playermodel(model, "obi3/main3")
			|| playermodel(model, "ntobiwan/main2")
			|| playermodel(model, "macewinduv3")
			|| playermodel(model, "mace_windu")
			|| playermodel(model, "mace_winduvm")
			|| playermodel(model, "mace_winduvm/default_robed")
			|| playermodel(model, "jedi_quigon")
			|| playermodel(model, "Quigon")
			|| playermodel(model, "jedi_kk")
			|| playermodel(model, "hs_kenobi_rots")
			|| playermodel(model, "jedi_kenobi")
			|| playermodel(model, "noQuiGonVM3/main")
			|| playermodel(model, "noQuiGonVM3/main2")
			|| playermodel(model, "moMace_Windu/main")
			|| playermodel(model, "muwindu/main"))
		{
			client->pers.nextbotclass = BCLASS_KYLE;
			client->pers.botmodelscale = BOTZIZE_NORMAL;
			if (!(ent->r.svFlags & SVF_BOT))
			{
				if (g_gametype.integer != GT_DUEL && g_gametype.integer != GT_POWERDUEL && g_gametype.integer !=
					GT_SIEGE)
				{
					client->ps.stats[STAT_HEALTH] = ent->health = 0;
					player_die(ent, ent, ent, 100000, MOD_TEAM_CHANGE);
					trap->UnlinkEntity((sharedEntity_t*)ent);
				}
				Com_Printf("Changes to your Class settings will take effect the next time you respawn.\n");
			}
		}
		else if (playermodel(model, "Shaaktivm")
			|| playermodel(model, "jedi_shaakti")
			|| playermodel(model, "bastila")
			|| playermodel(model, "brianna"))
		{
			client->pers.nextbotclass = BCLASS_KYLE;
			client->pers.botmodelscale = BOTZIZE_SMALL;
			if (!(ent->r.svFlags & SVF_BOT))
			{
				if (g_gametype.integer != GT_DUEL && g_gametype.integer != GT_POWERDUEL && g_gametype.integer !=
					GT_SIEGE)
				{
					client->ps.stats[STAT_HEALTH] = ent->health = 0;
					player_die(ent, ent, ent, 100000, MOD_TEAM_CHANGE);
					trap->UnlinkEntity((sharedEntity_t*)ent);
				}
				Com_Printf("Changes to your Class settings will take effect the next time you respawn.\n");
			}
		}
		else if (playermodel(model, "lando")
			|| playermodel(model, "lando/red")
			|| playermodel(model, "lando/blue")
			|| playermodel(model, "lando/main"))
		{
			client->pers.nextbotclass = BCLASS_LANDO;
			client->pers.botmodelscale = BOTZIZE_NORMAL;
			if (!(ent->r.svFlags & SVF_BOT))
			{
				if (g_gametype.integer != GT_DUEL && g_gametype.integer != GT_POWERDUEL && g_gametype.integer !=
					GT_SIEGE)
				{
					client->ps.stats[STAT_HEALTH] = ent->health = 0;
					player_die(ent, ent, ent, 100000, MOD_TEAM_CHANGE);
					trap->UnlinkEntity((sharedEntity_t*)ent);
				}
				Com_Printf("Changes to your Class settings will take effect the next time you respawn.\n");
			}
		}
		else if (playermodel(model, "luke")
			|| playermodel(model, "luke/")
			|| playermodel(model, "lukejka")
			|| playermodel(model, "lukejka/")
			|| playermodel(model, "luke_tfa")
			|| playermodel(model, "rey/head_a1|torso_a1|lower_a1")
			|| playermodel(model, "rey")
			|| playermodel(model, "st_rey")
			|| playermodel(model, "jedi_st_rey")
			|| playermodel(model, "lb/")
			|| playermodel(model, "lb/main")
			|| playermodel(model, "ld/")
			|| playermodel(model, "ld/main")
			|| playermodel(model, "lx/")
			|| playermodel(model, "lx/main")
			|| playermodel(model, "lr/")
			|| playermodel(model, "lr/main")
			|| playermodel(model, "lr/main2")
			|| playermodel(model, "lr/main3")
			|| playermodel(model, "stluke/")
			|| playermodel(model, "stluke/main")
			|| playermodel(model, "luanhluke/")
			|| playermodel(model, "luanhluke/main")
			|| playermodel(model, "luanhluke/main2")
			|| playermodel(model, "luke/red")
			|| playermodel(model, "luke/blue")
			|| playermodel(model, "lukejka/red")
			|| playermodel(model, "lukejka/blue")
			|| playermodel(model, "t_luke_rotj")
			|| playermodel(model, "jedi_luke")
			|| playermodel(model, "t_anakin")
			|| playermodel(model, "jedi_anakint")
			|| playermodel(model, "jedi_anakin")
			|| playermodel(model, "adi/main")
			|| playermodel(model, "shaak_ti/main")
			|| playermodel(model, "mgaaylasecura/")
			|| playermodel(model, "mgaaylasecura/main")
			|| playermodel(model, "jedi_anakin2")
			|| playermodel(model, "jedi_anakin")
			|| playermodel(model, "ajunta")
			|| playermodel(model, "Atris"))
		{
			client->pers.nextbotclass = BCLASS_LUKE;
			client->pers.botmodelscale = BOTZIZE_NORMAL;
			if (!(ent->r.svFlags & SVF_BOT))
			{
				if (g_gametype.integer != GT_DUEL && g_gametype.integer != GT_POWERDUEL && g_gametype.integer !=
					GT_SIEGE)
				{
					client->ps.stats[STAT_HEALTH] = ent->health = 0;
					player_die(ent, ent, ent, 100000, MOD_TEAM_CHANGE);
					trap->UnlinkEntity((sharedEntity_t*)ent);
				}
				Com_Printf("Changes to your Class settings will take effect the next time you respawn.\n");
			}
		}
		else if (playermodel(model, "ea_ep2anakin"))
		{
			client->pers.nextbotclass = BCLASS_DUELS;
			client->pers.botmodelscale = BOTZIZE_NORMAL;
			if (!(ent->r.svFlags & SVF_BOT))
			{
				if (g_gametype.integer != GT_DUEL && g_gametype.integer != GT_POWERDUEL && g_gametype.integer !=
					GT_SIEGE)
				{
					client->ps.stats[STAT_HEALTH] = ent->health = 0;
					player_die(ent, ent, ent, 100000, MOD_TEAM_CHANGE);
					trap->UnlinkEntity((sharedEntity_t*)ent);
				}
				Com_Printf("Changes to your Class settings will take effect the next time you respawn.\n");
			}
		}
		else if (playermodel(model, "monmothma")
			|| playermodel(model, "monmothma/red")
			|| playermodel(model, "monmothma/blue")
			|| playermodel(model, "ta/main")
			|| playermodel(model, "it/main"))
		{
			client->pers.nextbotclass = BCLASS_MONMOTHA;
			client->pers.botmodelscale = BOTZIZE_NORMAL;
			if (!(ent->r.svFlags & SVF_BOT))
			{
				if (g_gametype.integer != GT_DUEL && g_gametype.integer != GT_POWERDUEL && g_gametype.integer !=
					GT_SIEGE)
				{
					client->ps.stats[STAT_HEALTH] = ent->health = 0;
					player_die(ent, ent, ent, 100000, MOD_TEAM_CHANGE);
					trap->UnlinkEntity((sharedEntity_t*)ent);
				}
				Com_Printf("Changes to your Class settings will take effect the next time you respawn.\n");
			}
		}
		else if (playermodel(model, "morgan")
			|| playermodel(model, "morgan/red")
			|| playermodel(model, "morgan/blue"))
		{
			client->pers.nextbotclass = BCLASS_MORGANKATARN;
			client->pers.botmodelscale = BOTZIZE_NORMAL;
			if (!(ent->r.svFlags & SVF_BOT))
			{
				if (g_gametype.integer != GT_DUEL && g_gametype.integer != GT_POWERDUEL && g_gametype.integer !=
					GT_SIEGE)
				{
					client->ps.stats[STAT_HEALTH] = ent->health = 0;
					player_die(ent, ent, ent, 100000, MOD_TEAM_CHANGE);
					trap->UnlinkEntity((sharedEntity_t*)ent);
				}
				Com_Printf("Changes to your Class settings will take effect the next time you respawn.\n");
			}
		}
		else if (playermodel(model, "noghri")
			|| playermodel(model, "noghri/red")
			|| playermodel(model, "noghri/blue"))
		{
			client->pers.nextbotclass = BCLASS_NOGRHRI;
			client->pers.botmodelscale = BOTZIZE_LARGE;
			if (!(ent->r.svFlags & SVF_BOT))
			{
				if (g_gametype.integer != GT_DUEL && g_gametype.integer != GT_POWERDUEL && g_gametype.integer !=
					GT_SIEGE)
				{
					client->ps.stats[STAT_HEALTH] = ent->health = 0;
					player_die(ent, ent, ent, 100000, MOD_TEAM_CHANGE);
					trap->UnlinkEntity((sharedEntity_t*)ent);
				}
				Com_Printf("Changes to your Class settings will take effect the next time you respawn.\n");
			}
		}
		else if (playermodel(model, "protocol")
			|| playermodel(model, "protocol/red")
			|| playermodel(model, "protocol/blue")
			|| playermodel(model, "tc14/main")
			|| playermodel(model, "kgc3po/main"))
		{
			client->pers.nextbotclass = BCLASS_PROTOCOL;
			client->pers.botmodelscale = BOTZIZE_NORMAL;
			if (!(ent->r.svFlags & SVF_BOT))
			{
				if (g_gametype.integer != GT_DUEL && g_gametype.integer != GT_POWERDUEL && g_gametype.integer !=
					GT_SIEGE)
				{
					client->ps.stats[STAT_HEALTH] = ent->health = 0;
					player_die(ent, ent, ent, 100000, MOD_TEAM_CHANGE);
					trap->UnlinkEntity((sharedEntity_t*)ent);
				}
				Com_Printf("Changes to your Class settings will take effect the next time you respawn.\n");
			}
		}
		else if (playermodel(model, "r2d2")
			|| playermodel(model, "r5d2"))
		{
			client->pers.botmodelscale = BOTZIZE_SMALLER;
			client->pers.nextbotclass = BCLASS_R2D2;
			if (!(ent->r.svFlags & SVF_BOT))
			{
				if (g_gametype.integer != GT_DUEL && g_gametype.integer != GT_POWERDUEL && g_gametype.integer !=
					GT_SIEGE)
				{
					client->ps.stats[STAT_HEALTH] = ent->health = 0;
					player_die(ent, ent, ent, 100000, MOD_TEAM_CHANGE);
					trap->UnlinkEntity((sharedEntity_t*)ent);
				}
				Com_Printf("Changes to your Class settings will take effect the next time you respawn.\n");
			}
		}
		else if (playermodel(model, "marka_ragnos"))
		{
			client->pers.nextbotclass = BCLASS_RAGNOS;
			client->pers.botmodelscale = BOTZIZE_NORMAL;
			if (!(ent->r.svFlags & SVF_BOT))
			{
				if (g_gametype.integer != GT_DUEL && g_gametype.integer != GT_POWERDUEL && g_gametype.integer !=
					GT_SIEGE)
				{
					client->ps.stats[STAT_HEALTH] = ent->health = 0;
					player_die(ent, ent, ent, 100000, MOD_TEAM_CHANGE);
					trap->UnlinkEntity((sharedEntity_t*)ent);
				}
				Com_Printf("Changes to your Class settings will take effect the next time you respawn.\n");
			}
		}
		else if (playermodel(model, "rancor"))
		{
			client->pers.nextbotclass = BCLASS_RANCOR;
			client->pers.botmodelscale = BOTZIZE_NORMAL;
			if (!(ent->r.svFlags & SVF_BOT))
			{
				if (g_gametype.integer != GT_DUEL && g_gametype.integer != GT_POWERDUEL && g_gametype.integer !=
					GT_SIEGE)
				{
					client->ps.stats[STAT_HEALTH] = ent->health = 0;
					player_die(ent, ent, ent, 100000, MOD_TEAM_CHANGE);
					trap->UnlinkEntity((sharedEntity_t*)ent);
				}
				Com_Printf("Changes to your Class settings will take effect the next time you respawn.\n");
			}
		}
		else if (playermodel(model, "rax_joris")
			|| playermodel(model, "calo"))
		{
			client->pers.nextbotclass = BCLASS_RAX;
			client->pers.botmodelscale = BOTZIZE_NORMAL;
			if (!(ent->r.svFlags & SVF_BOT))
			{
				if (g_gametype.integer != GT_DUEL && g_gametype.integer != GT_POWERDUEL && g_gametype.integer !=
					GT_SIEGE)
				{
					client->ps.stats[STAT_HEALTH] = ent->health = 0;
					player_die(ent, ent, ent, 100000, MOD_TEAM_CHANGE);
					trap->UnlinkEntity((sharedEntity_t*)ent);
				}
				Com_Printf("Changes to your Class settings will take effect the next time you respawn.\n");
			}
		}
		else if (playermodel(model, "rebel")
			|| playermodel(model, "rebel/main")
			|| playermodel(model, "rebel/red")
			|| playermodel(model, "rebel/blue"))
		{
			client->pers.nextbotclass = BCLASS_REBEL;
			client->pers.botmodelscale = BOTZIZE_NORMAL;
			if (!(ent->r.svFlags & SVF_BOT))
			{
				if (g_gametype.integer != GT_DUEL && g_gametype.integer != GT_POWERDUEL && g_gametype.integer !=
					GT_SIEGE)
				{
					client->ps.stats[STAT_HEALTH] = ent->health = 0;
					player_die(ent, ent, ent, 100000, MOD_TEAM_CHANGE);
					trap->UnlinkEntity((sharedEntity_t*)ent);
				}
				Com_Printf("Changes to your Class settings will take effect the next time you respawn.\n");
			}
		}
		else if (playermodel(model, "reborn")
			|| playermodel(model, "reborn/red")
			|| playermodel(model, "reborn/blue")
			|| playermodel(model, "reborn/boss")
			|| playermodel(model, "reborn/fencer")
			|| playermodel(model, "reborn/forceuser")
			|| playermodel(model, "reborn/acrobat")
			|| playermodel(model, "reborn_new")
			|| playermodel(model, "reborn_twin")
			|| playermodel(model, "ExileFemaleDarkSide")
			|| playermodel(model, "ExileFemaleDarkSideUR"))
		{
			client->pers.nextbotclass = BCLASS_REBORN;
			client->pers.botmodelscale = BOTZIZE_NORMAL;
			if (!(ent->r.svFlags & SVF_BOT))
			{
				if (g_gametype.integer != GT_DUEL && g_gametype.integer != GT_POWERDUEL && g_gametype.integer !=
					GT_SIEGE)
				{
					client->ps.stats[STAT_HEALTH] = ent->health = 0;
					player_die(ent, ent, ent, 100000, MOD_TEAM_CHANGE);
					trap->UnlinkEntity((sharedEntity_t*)ent);
				}
				Com_Printf("Changes to your Class settings will take effect the next time you respawn.\n");
			}
		}
		else if (playermodel(model, "reelo")
			|| playermodel(model, "reelo/red")
			|| playermodel(model, "reelo/blue"))
		{
			client->pers.nextbotclass = BCLASS_REELO;
			client->pers.botmodelscale = BOTZIZE_NORMAL;
			if (!(ent->r.svFlags & SVF_BOT))
			{
				if (g_gametype.integer != GT_DUEL && g_gametype.integer != GT_POWERDUEL && g_gametype.integer !=
					GT_SIEGE)
				{
					client->ps.stats[STAT_HEALTH] = ent->health = 0;
					player_die(ent, ent, ent, 100000, MOD_TEAM_CHANGE);
					trap->UnlinkEntity((sharedEntity_t*)ent);
				}
				Com_Printf("Changes to your Class settings will take effect the next time you respawn.\n");
			}
		}
		else if (playermodel(model, "rockettrooper")
			|| playermodel(model, "rockettrooper/red")
			|| playermodel(model, "rockettrooper/blue"))
		{
			client->pers.nextbotclass = BCLASS_ROCKETTROOPER;
			client->pers.botmodelscale = BOTZIZE_LARGER;
			if (!(ent->r.svFlags & SVF_BOT))
			{
				if (g_gametype.integer != GT_DUEL && g_gametype.integer != GT_POWERDUEL && g_gametype.integer !=
					GT_SIEGE)
				{
					client->ps.stats[STAT_HEALTH] = ent->health = 0;
					player_die(ent, ent, ent, 100000, MOD_TEAM_CHANGE);
					trap->UnlinkEntity((sharedEntity_t*)ent);
				}
				Com_Printf("Changes to your Class settings will take effect the next time you respawn.\n");
			}
		}
		else if (playermodel(model, "rodian")
			|| playermodel(model, "rodian/red")
			|| playermodel(model, "rodian/blue")
			|| playermodel(model, "rodian/main")
			|| playermodel(model, "pb/main"))
		{
			client->pers.nextbotclass = BCLASS_RODIAN;
			client->pers.botmodelscale = BOTZIZE_NORMAL;
			if (!(ent->r.svFlags & SVF_BOT))
			{
				if (g_gametype.integer != GT_DUEL && g_gametype.integer != GT_POWERDUEL && g_gametype.integer !=
					GT_SIEGE)
				{
					client->ps.stats[STAT_HEALTH] = ent->health = 0;
					player_die(ent, ent, ent, 100000, MOD_TEAM_CHANGE);
					trap->UnlinkEntity((sharedEntity_t*)ent);
				}
				Com_Printf("Changes to your Class settings will take effect the next time you respawn.\n");
			}
		}
		else if (playermodel(model, "rosh_penin")
			|| playermodel(model, "rosh_penin/red")
			|| playermodel(model, "rosh_penin/blue"))
		{
			client->pers.nextbotclass = BCLASS_ROSH_PENIN;
			client->pers.botmodelscale = BOTZIZE_NORMAL;
			if (!(ent->r.svFlags & SVF_BOT))
			{
				if (g_gametype.integer != GT_DUEL && g_gametype.integer != GT_POWERDUEL && g_gametype.integer !=
					GT_SIEGE)
				{
					client->ps.stats[STAT_HEALTH] = ent->health = 0;
					player_die(ent, ent, ent, 100000, MOD_TEAM_CHANGE);
					trap->UnlinkEntity((sharedEntity_t*)ent);
				}
				Com_Printf("Changes to your Class settings will take effect the next time you respawn.\n");
			}
		}
		else if (playermodel(model, "saboteur")
			|| playermodel(model, "saboteur/red")
			|| playermodel(model, "saboteur/blue"))
		{
			client->pers.nextbotclass = BCLASS_SABOTEUR;
			client->pers.botmodelscale = BOTZIZE_NORMAL;
			if (!(ent->r.svFlags & SVF_BOT))
			{
				if (g_gametype.integer != GT_DUEL && g_gametype.integer != GT_POWERDUEL && g_gametype.integer !=
					GT_SIEGE)
				{
					client->ps.stats[STAT_HEALTH] = ent->health = 0;
					player_die(ent, ent, ent, 100000, MOD_TEAM_CHANGE);
					trap->UnlinkEntity((sharedEntity_t*)ent);
				}
				Com_Printf("Changes to your Class settings will take effect the next time you respawn.\n");
			}
		}
		else if (playermodel(model, "shadowtrooper")
			|| playermodel(model, "shadowtrooper/red")
			|| playermodel(model, "shadowtrooper/blue"))
		{
			client->pers.nextbotclass = BCLASS_SHADOWTROOPER;
			client->pers.botmodelscale = BOTZIZE_NORMAL;
			if (!(ent->r.svFlags & SVF_BOT))
			{
				if (g_gametype.integer != GT_DUEL && g_gametype.integer != GT_POWERDUEL && g_gametype.integer !=
					GT_SIEGE)
				{
					client->ps.stats[STAT_HEALTH] = ent->health = 0;
					player_die(ent, ent, ent, 100000, MOD_TEAM_CHANGE);
					trap->UnlinkEntity((sharedEntity_t*)ent);
				}
				Com_Printf("Changes to your Class settings will take effect the next time you respawn.\n");
			}
		}
		else if (playermodel(model, "cc/main")
			|| playermodel(model, "sdt/")
			|| playermodel(model, "bs/")
			|| playermodel(model, "snowtrooper")
			|| playermodel(model, "snowtrooper/blue")
			|| playermodel(model, "snowtrooper/red")
			|| playermodel(model, "arc")
			|| playermodel(model, "cn")
			|| playermodel(model, "cp")
			|| playermodel(model, "ccm")
			|| playermodel(model, "ccm/main")
			|| playermodel(model, "cc/")
			|| playermodel(model, "stormtrooper")
			|| playermodel(model, "TR_8R")
			|| playermodel(model, "TR_8R_normal")
			|| playermodel(model, "FN-2187")
			|| playermodel(model, "captainphasma")
			|| playermodel(model, "stormtrooper_tfa")
			|| playermodel(model, "stormtrooper/blue")
			|| playermodel(model, "stormtrooper/red")
			|| playermodel(model, "stormtrooper/main")
			|| playermodel(model, "stormtrooper/main2")
			|| playermodel(model, "bh")
			|| playermodel(model, "ct")
			|| playermodel(model, "ct/main2")
			|| playermodel(model, "ct/main3")
			|| playermodel(model, "ct/main4")
			|| playermodel(model, "ct/main5")
			|| playermodel(model, "ct/main6")
			|| playermodel(model, "cb")
			|| playermodel(model, "cbe")
			|| playermodel(model, "cdy/")
			|| playermodel(model, "cdy/main")
			|| playermodel(model, "cdy/main2")
			|| playermodel(model, "smuggler2/default")
			|| playermodel(model, "trooper1/default")
			|| playermodel(model, "trooper2/default")
			|| playermodel(model, "swamptrooper")
			|| playermodel(model, "swamptrooper/blue")
			|| playermodel(model, "swamptrooper/red")
			|| playermodel(model, "First_Order_Riot_Trooper")
			|| playermodel(model, "sullustan"))
		{
			client->pers.botmodelscale = BOTZIZE_NORMAL;
			client->pers.nextbotclass = BCLASS_STORMTROOPER;
			if (!(ent->r.svFlags & SVF_BOT))
			{
				if (g_gametype.integer != GT_DUEL && g_gametype.integer != GT_POWERDUEL && g_gametype.integer !=
					GT_SIEGE)
				{
					client->ps.stats[STAT_HEALTH] = ent->health = 0;
					player_die(ent, ent, ent, 100000, MOD_TEAM_CHANGE);
					trap->UnlinkEntity((sharedEntity_t*)ent);
				}
				Com_Printf("Changes to your Class settings will take effect the next time you respawn.\n");
			}
		}
		else if (playermodel(model, "captainphasma")
			|| playermodel(model, "CaptainPhasmaK"))
		{
			client->pers.botmodelscale = BOTZIZE_TALL;
			client->pers.nextbotclass = BCLASS_STORMTROOPER;
			if (!(ent->r.svFlags & SVF_BOT))
			{
				if (g_gametype.integer != GT_DUEL && g_gametype.integer != GT_POWERDUEL && g_gametype.integer !=
					GT_SIEGE)
				{
					client->ps.stats[STAT_HEALTH] = ent->health = 0;
					player_die(ent, ent, ent, 100000, MOD_TEAM_CHANGE);
					trap->UnlinkEntity((sharedEntity_t*)ent);
				}
				Com_Printf("Changes to your Class settings will take effect the next time you respawn.\n");
			}
		}
		else if (playermodel(model, "fbhutt/main")
			|| playermodel(model, "Itho")
			|| playermodel(model, "Itho2")
			|| playermodel(model, "Itho3")
			|| playermodel(model, "Itho4")
			|| playermodel(model, "ithorian"))
		{
			client->pers.nextbotclass = BCLASS_STORMTROOPER;
			client->pers.botmodelscale = BOTZIZE_MASSIVE;
			if (!(ent->r.svFlags & SVF_BOT))
			{
				if (g_gametype.integer != GT_DUEL && g_gametype.integer != GT_POWERDUEL && g_gametype.integer !=
					GT_SIEGE)
				{
					client->ps.stats[STAT_HEALTH] = ent->health = 0;
					player_die(ent, ent, ent, 100000, MOD_TEAM_CHANGE);
					trap->UnlinkEntity((sharedEntity_t*)ent);
				}
				Com_Printf("Changes to your Class settings will take effect the next time you respawn.\n");
			}
		}
		else if (playermodel(model, "tavion")
			|| playermodel(model, "tavion/blue")
			|| playermodel(model, "tavion/red")
			|| playermodel(model, "tavion_new")
			|| playermodel(model, "tavion_new/blue")
			|| playermodel(model, "tavion_new/red")
			|| playermodel(model, "tavion_new/main")
			|| playermodel(model, "asajj")
			|| playermodel(model, "assajv")
			|| playermodel(model, "AssajjCW"))
		{
			client->pers.nextbotclass = BCLASS_TAVION;
			client->pers.botmodelscale = BOTZIZE_NORMAL;
			if (!(ent->r.svFlags & SVF_BOT))
			{
				if (g_gametype.integer != GT_DUEL && g_gametype.integer != GT_POWERDUEL && g_gametype.integer !=
					GT_SIEGE)
				{
					client->ps.stats[STAT_HEALTH] = ent->health = 0;
					player_die(ent, ent, ent, 100000, MOD_TEAM_CHANGE);
					trap->UnlinkEntity((sharedEntity_t*)ent);
				}
				Com_Printf("Changes to your Class settings will take effect the next time you respawn.\n");
			}
		}
		else if (playermodel(model, "trandoshan")
			|| playermodel(model, "trandoshan/blue")
			|| playermodel(model, "trandoshan/red")
			|| playermodel(model, "trandoshan/main")
			|| playermodel(model, "sebulba/main")
			|| playermodel(model, "sebulba/main2")
			|| playermodel(model, "sebulba/main3")
			|| playermodel(model, "lamasu/main"))
		{
			client->pers.nextbotclass = BCLASS_TRANDOSHAN;
			client->pers.botmodelscale = BOTZIZE_NORMAL;
			if (!(ent->r.svFlags & SVF_BOT))
			{
				if (g_gametype.integer != GT_DUEL && g_gametype.integer != GT_POWERDUEL && g_gametype.integer !=
					GT_SIEGE)
				{
					client->ps.stats[STAT_HEALTH] = ent->health = 0;
					player_die(ent, ent, ent, 100000, MOD_TEAM_CHANGE);
					trap->UnlinkEntity((sharedEntity_t*)ent);
				}
				Com_Printf("Changes to your Class settings will take effect the next time you respawn.\n");
			}
		}
		else if (playermodel(model, "Bountyhunter2/default")
			|| playermodel(model, "bkzam/")
			|| playermodel(model, "bkzam/main")
			|| playermodel(model, "bkzam/main2")
			|| playermodel(model, "bkzam/main3")
			|| playermodel(model, "4lom/main")
			|| playermodel(model, "4lom/")
			|| playermodel(model, "tusken")
			|| playermodel(model, "tusken/main")
			|| playermodel(model, "tusken/blue")
			|| playermodel(model, "tusken/red")
			|| playermodel(model, "boushh/default"))
		{
			client->pers.nextbotclass = BCLASS_TUSKEN_SNIPER;
			client->pers.botmodelscale = BOTZIZE_NORMAL;
			if (!(ent->r.svFlags & SVF_BOT))
			{
				if (g_gametype.integer != GT_DUEL && g_gametype.integer != GT_POWERDUEL && g_gametype.integer !=
					GT_SIEGE)
				{
					client->ps.stats[STAT_HEALTH] = ent->health = 0;
					player_die(ent, ent, ent, 100000, MOD_TEAM_CHANGE);
					trap->UnlinkEntity((sharedEntity_t*)ent);
				}
				Com_Printf("Changes to your Class settings will take effect the next time you respawn.\n");
			}
		}
		else if (playermodel(model, "edAurra/main")
			|| playermodel(model, "ks/main"))
		{
			client->pers.nextbotclass = BCLASS_TUSKEN_RAIDER;
			client->pers.botmodelscale = BOTZIZE_SMALL;
			if (!(ent->r.svFlags & SVF_BOT))
			{
				if (g_gametype.integer != GT_DUEL && g_gametype.integer != GT_POWERDUEL && g_gametype.integer !=
					GT_SIEGE)
				{
					client->ps.stats[STAT_HEALTH] = ent->health = 0;
					player_die(ent, ent, ent, 100000, MOD_TEAM_CHANGE);
					trap->UnlinkEntity((sharedEntity_t*)ent);
				}
				Com_Printf("Changes to your Class settings will take effect the next time you respawn.\n");
			}
		}
		else if (playermodel(model, "fcgamorrean/main"))
		{
			client->pers.nextbotclass = BCLASS_TUSKEN_RAIDER;
			client->pers.botmodelscale = BOTZIZE_LARGER;
			if (!(ent->r.svFlags & SVF_BOT))
			{
				if (g_gametype.integer != GT_DUEL && g_gametype.integer != GT_POWERDUEL && g_gametype.integer !=
					GT_SIEGE)
				{
					client->ps.stats[STAT_HEALTH] = ent->health = 0;
					player_die(ent, ent, ent, 100000, MOD_TEAM_CHANGE);
					trap->UnlinkEntity((sharedEntity_t*)ent);
				}
				Com_Printf("Changes to your Class settings will take effect the next time you respawn.\n");
			}
		}
		else if (playermodel(model, "ugnaught")
			|| playermodel(model, "ew/main")
			|| playermodel(model, "ew"))
		{
			client->pers.botmodelscale = BOTZIZE_SMALLER;
			client->pers.nextbotclass = BCLASS_UGNAUGHT;
			if (!(ent->r.svFlags & SVF_BOT))
			{
				if (g_gametype.integer != GT_DUEL && g_gametype.integer != GT_POWERDUEL && g_gametype.integer !=
					GT_SIEGE)
				{
					client->ps.stats[STAT_HEALTH] = ent->health = 0;
					player_die(ent, ent, ent, 100000, MOD_TEAM_CHANGE);
					trap->UnlinkEntity((sharedEntity_t*)ent);
				}
				Com_Printf("Changes to your Class settings will take effect the next time you respawn.\n");
			}
		}
		else if (playermodel(model, "wampa"))
		{
			client->pers.nextbotclass = BCLASS_WAMPA;
			client->pers.botmodelscale = BOTZIZE_NORMAL;
			if (!(ent->r.svFlags & SVF_BOT))
			{
				if (g_gametype.integer != GT_DUEL && g_gametype.integer != GT_POWERDUEL && g_gametype.integer !=
					GT_SIEGE)
				{
					client->ps.stats[STAT_HEALTH] = ent->health = 0;
					player_die(ent, ent, ent, 100000, MOD_TEAM_CHANGE);
					trap->UnlinkEntity((sharedEntity_t*)ent);
				}
				Com_Printf("Changes to your Class settings will take effect the next time you respawn.\n");
			}
		}
		else if (playermodel(model, "weequay")
			|| playermodel(model, "weequay/blue")
			|| playermodel(model, "weequay/red")
			|| playermodel(model, "weequay/main"))
		{
			client->pers.nextbotclass = BCLASS_WEEQUAY;
			client->pers.botmodelscale = BOTZIZE_NORMAL;
			if (!(ent->r.svFlags & SVF_BOT))
			{
				if (g_gametype.integer != GT_DUEL && g_gametype.integer != GT_POWERDUEL && g_gametype.integer !=
					GT_SIEGE)
				{
					client->ps.stats[STAT_HEALTH] = ent->health = 0;
					player_die(ent, ent, ent, 100000, MOD_TEAM_CHANGE);
					trap->UnlinkEntity((sharedEntity_t*)ent);
				}
				Com_Printf("Changes to your Class settings will take effect the next time you respawn.\n");
			}
		}
		else if (playermodel(model, "lamasu/main"))
		{
			client->pers.nextbotclass = BCLASS_WEEQUAY;
			client->pers.botmodelscale = BOTZIZE_LARGER;
			if (!(ent->r.svFlags & SVF_BOT))
			{
				if (g_gametype.integer != GT_DUEL && g_gametype.integer != GT_POWERDUEL && g_gametype.integer !=
					GT_SIEGE)
				{
					client->ps.stats[STAT_HEALTH] = ent->health = 0;
					player_die(ent, ent, ent, 100000, MOD_TEAM_CHANGE);
					trap->UnlinkEntity((sharedEntity_t*)ent);
				}
				Com_Printf("Changes to your Class settings will take effect the next time you respawn.\n");
			}
		}
		else if (playermodel(model, "SBD/default")
			|| playermodel(model, "SBD")
			|| playermodel(model, "SBD2")
			|| playermodel(model, "Super_Battle_Droid")
			|| playermodel(model, "Super Battle Droid")
			|| playermodel(model, "superbattledroid")
			|| playermodel(model, "SBD/"))
		{
			client->pers.nextbotclass = BCLASS_SBD;
			client->pers.botmodelscale = BOTZIZE_MASSIVE;
			if (!(ent->r.svFlags & SVF_BOT))
			{
				if (g_gametype.integer != GT_DUEL && g_gametype.integer != GT_POWERDUEL && g_gametype.integer !=
					GT_SIEGE)
				{
					client->ps.stats[STAT_HEALTH] = ent->health = 0;
					player_die(ent, ent, ent, 100000, MOD_TEAM_CHANGE);
					trap->UnlinkEntity((sharedEntity_t*)ent);
				}
				Com_Printf("Changes to your Class settings will take effect the next time you respawn.\n");
			}
		}
		else if (playermodel(model, "battledroid")
			|| playermodel(model, "battledroid/main")
			|| playermodel(model, "battledroid/main2")
			|| playermodel(model, "battledroid/main3")
			|| playermodel(model, "battledroid/main4")
			|| playermodel(model, "battle3po/main")
			|| playermodel(model, "ra7/main")
			|| playermodel(model, "hk47")
			|| playermodel(model, "hk50")
			|| playermodel(model, "hk51")
			|| playermodel(model, "Wardroid"))
		{
			client->pers.nextbotclass = BCLASS_BATTLEDROID;
			client->pers.botmodelscale = BOTZIZE_NORMAL;
			if (!(ent->r.svFlags & SVF_BOT))
			{
				if (g_gametype.integer != GT_DUEL && g_gametype.integer != GT_POWERDUEL && g_gametype.integer !=
					GT_SIEGE)
				{
					client->ps.stats[STAT_HEALTH] = ent->health = 0;
					player_die(ent, ent, ent, 100000, MOD_TEAM_CHANGE);
					trap->UnlinkEntity((sharedEntity_t*)ent);
				}
				Com_Printf("Changes to your Class settings will take effect the next time you respawn.\n");
			}
		}
		else if (playermodel(model, "mando_hunter/default")
			|| playermodel(model, "Bountyhunter1/default")
			|| playermodel(model, "boba_fett/red")
			|| playermodel(model, "Bountyhunter1/default_kamino"))
		{
			client->pers.nextbotclass = BCLASS_MANDOLORIAN1;
			client->pers.botmodelscale = BOTZIZE_NORMAL;
			if (!(ent->r.svFlags & SVF_BOT))
			{
				if (g_gametype.integer != GT_DUEL && g_gametype.integer != GT_POWERDUEL && g_gametype.integer !=
					GT_SIEGE)
				{
					client->ps.stats[STAT_HEALTH] = ent->health = 0;
					player_die(ent, ent, ent, 100000, MOD_TEAM_CHANGE);
					trap->UnlinkEntity((sharedEntity_t*)ent);
				}
				Com_Printf("Changes to your Class settings will take effect the next time you respawn.\n");
			}
		}
		else if (playermodel(model, "jango_fett/blue")
			|| playermodel(model, "boba_fett/blue"))
		{
			client->pers.nextbotclass = BCLASS_MANDOLORIAN2;
			client->pers.botmodelscale = BOTZIZE_NORMAL;
			if (!(ent->r.svFlags & SVF_BOT))
			{
				if (g_gametype.integer != GT_DUEL && g_gametype.integer != GT_POWERDUEL && g_gametype.integer !=
					GT_SIEGE)
				{
					client->ps.stats[STAT_HEALTH] = ent->health = 0;
					player_die(ent, ent, ent, 100000, MOD_TEAM_CHANGE);
					trap->UnlinkEntity((sharedEntity_t*)ent);
				}
				Com_Printf("Changes to your Class settings will take effect the next time you respawn.\n");
			}
		}
		else if (playermodel(model, "T_yoda_MP")
			|| playermodel(model, "T_yoda_MP/default")
			|| playermodel(model, "nayodaghost/main")
			|| playermodel(model, "yoda/main")
			|| playermodel(model, "jedi_yoda")
			|| playermodel(model, "yoda")
			|| playermodel(model, "yodavm"))
		{
			client->pers.botmodelscale = BOTZIZE_SMALLEST;
			client->pers.nextbotclass = BCLASS_YODA;
			if (!(ent->r.svFlags & SVF_BOT))
			{
				if (g_gametype.integer != GT_DUEL && g_gametype.integer != GT_POWERDUEL && g_gametype.integer !=
					GT_SIEGE)
				{
					client->ps.stats[STAT_HEALTH] = ent->health = 0;
					player_die(ent, ent, ent, 100000, MOD_TEAM_CHANGE);
					trap->UnlinkEntity((sharedEntity_t*)ent);
				}
				Com_Printf("Changes to your Class settings will take effect the next time you respawn.\n");
			}
		}
		else if (playermodel(model, "youngani")
			|| playermodel(model, "youngshak")
			|| playermodel(model, "youngfem")
			|| playermodel(model, "youngling")
			|| playermodel(model, "youngling/default"))
		{
			client->pers.botmodelscale = BOTZIZE_SMALLER;
			client->pers.nextbotclass = BCLASS_JEDI;
			if (!(ent->r.svFlags & SVF_BOT))
			{
				if (g_gametype.integer != GT_DUEL && g_gametype.integer != GT_POWERDUEL && g_gametype.integer !=
					GT_SIEGE)
				{
					client->ps.stats[STAT_HEALTH] = ent->health = 0;
					player_die(ent, ent, ent, 100000, MOD_TEAM_CHANGE);
					trap->UnlinkEntity((sharedEntity_t*)ent);
				}
				Com_Printf("Changes to your Class settings will take effect the next time you respawn.\n");
			}
		}
		else if (playermodel(model, "npj_p/default"))
		{
			client->pers.botmodelscale = BOTZIZE_SMALL;
			client->pers.nextbotclass = BCLASS_JEDI;
			if (!(ent->r.svFlags & SVF_BOT))
			{
				if (g_gametype.integer != GT_DUEL && g_gametype.integer != GT_POWERDUEL && g_gametype.integer !=
					GT_SIEGE)
				{
					client->ps.stats[STAT_HEALTH] = ent->health = 0;
					player_die(ent, ent, ent, 100000, MOD_TEAM_CHANGE);
					trap->UnlinkEntity((sharedEntity_t*)ent);
				}
				Com_Printf("Changes to your Class settings will take effect the next time you respawn.\n");
			}
		}
		else if (playermodel(model, "Zaalba"))
		{
			client->pers.botmodelscale = BOTZIZE_LARGER;
			client->pers.nextbotclass = BCLASS_SMUGGLER1;
			if (!(ent->r.svFlags & SVF_BOT))
			{
				if (g_gametype.integer != GT_DUEL && g_gametype.integer != GT_POWERDUEL && g_gametype.integer !=
					GT_SIEGE)
				{
					client->ps.stats[STAT_HEALTH] = ent->health = 0;
					player_die(ent, ent, ent, 100000, MOD_TEAM_CHANGE);
					trap->UnlinkEntity((sharedEntity_t*)ent);
				}
				Com_Printf("Changes to your Class settings will take effect the next time you respawn.\n");
			}
		}
		else if (playermodel(model, "jarjar")
			|| playermodel(model, "gungan"))
		{
			client->pers.botmodelscale = BOTZIZE_LARGER;
			client->pers.nextbotclass = BCLASS_SOILDER;
			if (!(ent->r.svFlags & SVF_BOT))
			{
				if (g_gametype.integer != GT_DUEL && g_gametype.integer != GT_POWERDUEL && g_gametype.integer !=
					GT_SIEGE)
				{
					client->ps.stats[STAT_HEALTH] = ent->health = 0;
					player_die(ent, ent, ent, 100000, MOD_TEAM_CHANGE);
					trap->UnlinkEntity((sharedEntity_t*)ent);
				}
				Com_Printf("Changes to your Class settings will take effect the next time you respawn.\n");
			}
		}
		else if (playermodel(model, "khadmiral/main")
			|| playermodel(model, "khadmiral/main2")
			|| playermodel(model, "dustil"))
		{
			client->pers.nextbotclass = BCLASS_SOILDER;
			client->pers.botmodelscale = BOTZIZE_NORMAL;
			if (!(ent->r.svFlags & SVF_BOT))
			{
				if (g_gametype.integer != GT_DUEL && g_gametype.integer != GT_POWERDUEL && g_gametype.integer !=
					GT_SIEGE)
				{
					client->ps.stats[STAT_HEALTH] = ent->health = 0;
					player_die(ent, ent, ent, 100000, MOD_TEAM_CHANGE);
					trap->UnlinkEntity((sharedEntity_t*)ent);
				}
				Com_Printf("Changes to your Class settings will take effect the next time you respawn.\n");
			}
		}
		else if (playermodel(model, "jedi_maul")
			|| playermodel(model, "DT_Maul")
			|| playermodel(model, "DT_Maul/default")
			|| playermodel(model, "DT_Maul/default_hooded")
			|| playermodel(model, "DT_Maul/default_robed")
			|| playermodel(model, "maulvm")
			|| playermodel(model, "maulvms")
			|| playermodel(model, "maulvmr"))
		{
			client->pers.nextbotclass = BCLASS_STAFFDARK;
			client->pers.botmodelscale = BOTZIZE_NORMAL;
			if (!(ent->r.svFlags & SVF_BOT))
			{
				if (g_gametype.integer != GT_DUEL && g_gametype.integer != GT_POWERDUEL && g_gametype.integer !=
					GT_SIEGE)
				{
					client->ps.stats[STAT_HEALTH] = ent->health = 0;
					player_die(ent, ent, ent, 100000, MOD_TEAM_CHANGE);
					trap->UnlinkEntity((sharedEntity_t*)ent);
				}
				Com_Printf("Changes to your Class settings will take effect the next time you respawn.\n");
			}
		}
		else if (playermodel(model, "droideka")
			|| playermodel(model, "droideka/main"))
		{
			client->pers.nextbotclass = BCLASS_SOILDER;
			client->pers.botmodelscale = BOTZIZE_NORMAL;
			if (!(ent->r.svFlags & SVF_BOT))
			{
				if (g_gametype.integer != GT_DUEL && g_gametype.integer != GT_POWERDUEL && g_gametype.integer !=
					GT_SIEGE)
				{
					client->ps.stats[STAT_HEALTH] = ent->health = 0;
					player_die(ent, ent, ent, 100000, MOD_TEAM_CHANGE);
					trap->UnlinkEntity((sharedEntity_t*)ent);
				}
				Com_Printf("Changes to your Class settings will take effect the next time you respawn.\n");
			}
		}
		else if (playermodel(model, "vader")
			|| playermodel(model, "vaderVM")
			|| playermodel(model, "vader/")
			|| playermodel(model, "vader/main")
			|| playermodel(model, "vader/main2")
			|| playermodel(model, "vader/main3")
			|| playermodel(model, "vader/main4")
			|| playermodel(model, "vader/default")
			|| playermodel(model, "jedi_vader")
			|| playermodel(model, "am_vader")
			|| playermodel(model, "darthvader")
			|| playermodel(model, "vadervmm")
			|| playermodel(model, "t_vader"))
		{
			client->pers.botmodelscale = BOTZIZE_MASSIVE;
			client->pers.nextbotclass = BCLASS_LORDVADER;
			if (!(ent->r.svFlags & SVF_BOT))
			{
				if (g_gametype.integer != GT_DUEL && g_gametype.integer != GT_POWERDUEL && g_gametype.integer !=
					GT_SIEGE)
				{
					client->ps.stats[STAT_HEALTH] = ent->health = 0;
					player_die(ent, ent, ent, 100000, MOD_TEAM_CHANGE);
					trap->UnlinkEntity((sharedEntity_t*)ent);
				}
				Com_Printf("Changes to your Class settings will take effect the next time you respawn.\n");
			}
		}
		else if (playermodel(model, "jedi_palpatine")
			|| playermodel(model, "sithinquisitor1/default")
			|| playermodel(model, "sithinquisitor3/default")
			|| playermodel(model, "sithworrior2/default")
			|| playermodel(model, "darth_sidious")
			|| playermodel(model, "sidious")
			|| playermodel(model, "sidious/main")
			|| playermodel(model, "sidious/main2")
			|| playermodel(model, "sidious/")
			|| playermodel(model, "t_palpatine_sith")
			|| playermodel(model, "hs_dooku")
			|| playermodel(model, "jedi_dooku")
			|| playermodel(model, "t_palpatine")
			|| playermodel(model, "palpatine")
			|| playermodel(model, "emperor/")
			|| playermodel(model, "emperor/main")
			|| playermodel(model, "sithinquisitor2/default")
			|| playermodel(model, "sithworrior1/default")
			|| playermodel(model, "palpatine/main/")
			|| playermodel(model, "darkjedi")
			|| playermodel(model, "darthrevan")
			|| playermodel(model, "darthsion")
			|| playermodel(model, "darthtraya"))
		{
			client->pers.nextbotclass = BCLASS_SITHLORD;
			client->pers.botmodelscale = BOTZIZE_NORMAL;
			if (!(ent->r.svFlags & SVF_BOT))
			{
				if (g_gametype.integer != GT_DUEL && g_gametype.integer != GT_POWERDUEL && g_gametype.integer !=
					GT_SIEGE)
				{
					client->ps.stats[STAT_HEALTH] = ent->health = 0;
					player_die(ent, ent, ent, 100000, MOD_TEAM_CHANGE);
					trap->UnlinkEntity((sharedEntity_t*)ent);
				}
				Com_Printf("Changes to your Class settings will take effect the next time you respawn.\n");
			}
		}
		else if (playermodel(model, "snoke"))
		{
			client->pers.nextbotclass = BCLASS_SITHLORD;
			client->pers.botmodelscale = BOTZIZE_TALL;
			if (!(ent->r.svFlags & SVF_BOT))
			{
				if (g_gametype.integer != GT_DUEL && g_gametype.integer != GT_POWERDUEL && g_gametype.integer !=
					GT_SIEGE)
				{
					client->ps.stats[STAT_HEALTH] = ent->health = 0;
					player_die(ent, ent, ent, 100000, MOD_TEAM_CHANGE);
					trap->UnlinkEntity((sharedEntity_t*)ent);
				}
				Com_Printf("Changes to your Class settings will take effect the next time you respawn.\n");
			}
		}
		else if (playermodel(model, "gr")
			|| playermodel(model, "gr/")
			|| playermodel(model, "gr/main")
			|| playermodel(model, "gr/main2")
			|| playermodel(model, "grfour")
			|| playermodel(model, "grievous_utapau")
			|| playermodel(model, "grievous4")
			|| playermodel(model, "grievous")
			|| playermodel(model, "jedi_gri"))
		{
			client->pers.botmodelscale = BOTZIZE_MASSIVE;
			client->pers.nextbotclass = BCLASS_GRIEVOUS;
			if (!(ent->r.svFlags & SVF_BOT))
			{
				if (g_gametype.integer != GT_DUEL && g_gametype.integer != GT_POWERDUEL && g_gametype.integer !=
					GT_SIEGE)
				{
					client->ps.stats[STAT_HEALTH] = ent->health = 0;
					player_die(ent, ent, ent, 100000, MOD_TEAM_CHANGE);
					trap->UnlinkEntity((sharedEntity_t*)ent);
				}
				Com_Printf("Changes to your Class settings will take effect the next time you respawn.\n");
			}
		}
		else if (playermodel(model, "ma/main")
			|| playermodel(model, "acdRoyalguard")
			|| playermodel(model, "acdRoyalguard/")
			|| playermodel(model, "acdRoyalguard/main")
			|| playermodel(model, "ma/main2")
			|| playermodel(model, "ma/main3")
			|| playermodel(model, "ma/main4")
			|| playermodel(model, "ma")
			|| playermodel(model, "magnaguard"))
		{
			client->pers.botmodelscale = BOTZIZE_MASSIVE;
			client->pers.nextbotclass = BCLASS_SITH;
			if (!(ent->r.svFlags & SVF_BOT))
			{
				if (g_gametype.integer != GT_DUEL && g_gametype.integer != GT_POWERDUEL && g_gametype.integer !=
					GT_SIEGE)
				{
					client->ps.stats[STAT_HEALTH] = ent->health = 0;
					player_die(ent, ent, ent, 100000, MOD_TEAM_CHANGE);
					trap->UnlinkEntity((sharedEntity_t*)ent);
				}
				Com_Printf("Changes to your Class settings will take effect the next time you respawn.\n");
			}
		}
		else if (playermodel(model, "exile")
			|| playermodel(model, "ExileMaleDarkSideUR")
			|| playermodel(model, "nihilus")
			|| playermodel(model, "revan")
			|| playermodel(model, "sith_apprentice")
			|| playermodel(model, "Sith_Assassin")
			|| playermodel(model, "Sith_Assassin2")
			|| playermodel(model, "Sith_Assassin2Master")
			|| playermodel(model, "sith_warrior"))
		{
			client->pers.nextbotclass = BCLASS_SITH;
			if (!(ent->r.svFlags & SVF_BOT))
			{
				if (g_gametype.integer != GT_DUEL && g_gametype.integer != GT_POWERDUEL && g_gametype.integer !=
					GT_SIEGE)
				{
					client->ps.stats[STAT_HEALTH] = ent->health = 0;
					player_die(ent, ent, ent, 100000, MOD_TEAM_CHANGE);
					trap->UnlinkEntity((sharedEntity_t*)ent);
				}
				Com_Printf("Changes to your Class settings will take effect the next time you respawn.\n");
			}
		}
		else
		{
			client->pers.botmodelscale = BOTZIZE_NORMAL;
			if (!(ent->r.svFlags & SVF_BOT))
			{
				if (g_gametype.integer != GT_DUEL && g_gametype.integer != GT_POWERDUEL && g_gametype.integer !=
					GT_SIEGE)
				{
					client->ps.stats[STAT_HEALTH] = ent->health = 0;
					player_die(ent, ent, ent, 100000, MOD_TEAM_CHANGE);
					trap->UnlinkEntity((sharedEntity_t*)ent);
				}
				client->pers.botmodelscale = BOTZIZE_NORMAL;
				Com_Printf("Changes to your Class settings will take effect the next time you respawn.\n");
			}
		}
	}

	client->pers.botclass = client->pers.nextbotclass;

	if (WinterGear)
	{
		//use winter gear
		char* skinname = strstr(model, "|");
		if (skinname)
		{
			//we're using a species player model, try to use their hoth clothes.
			skinname++;
			strcpy(skinname, "torso_g1|lower_e1\0");
		}
	}

	if (d_perPlayerGhoul2.integer && Q_stricmp(model, client->modelname))
	{
		Q_strncpyz(client->modelname, model, sizeof client->modelname);
		model_changed = qtrue;
	}

	client->ps.customRGBA[0] = ((value = Info_ValueForKey(userinfo, "char_color_red")))
		? Com_Clampi(0, 255, atoi(value))
		: 255;
	client->ps.customRGBA[1] = ((value = Info_ValueForKey(userinfo, "char_color_green")))
		? Com_Clampi(0, 255, atoi(value))
		: 255;
	client->ps.customRGBA[2] = ((value = Info_ValueForKey(userinfo, "char_color_blue")))
		? Com_Clampi(0, 255, atoi(value))
		: 255;

	//Prevent skins being too dark
	if (g_charRestrictRGB.integer && client->ps.customRGBA[0] + client->ps.customRGBA[1] + client->ps.customRGBA[2] <
		100)
		client->ps.customRGBA[0] = client->ps.customRGBA[1] = client->ps.customRGBA[2] = 255;

	client->ps.customRGBA[3] = 255;

	Q_strncpyz(forcePowers, Info_ValueForKey(userinfo, "forcepowers"), sizeof forcePowers);

	// update our customRGBA for team colors.
	if (level.gametype >= GT_TEAM && level.gametype != GT_SIEGE && !g_jediVmerc.integer)
	{
		char skin[MAX_QPATH] = { 0 };
		vec3_t colorOverride = { 0.0f };

		VectorClear(colorOverride);

		BG_ValidateSkinForTeam(model, skin, client->sess.sessionTeam, colorOverride);
		if (colorOverride[0] != 0.0f || colorOverride[1] != 0.0f || colorOverride[2] != 0.0f)
			VectorScaleM(colorOverride, 255.0f, client->ps.customRGBA);
	}

	// bots set their team a few frames later
	if (level.gametype >= GT_TEAM && g_entities[client_num].r.svFlags & SVF_BOT)
	{
		s = Info_ValueForKey(userinfo, "team");
		if (!Q_stricmp(s, "red") || !Q_stricmp(s, "r"))
			team = TEAM_RED;
		else if (!Q_stricmp(s, "blue") || !Q_stricmp(s, "b"))
			team = TEAM_BLUE;
		else
			team = PickTeam(client_num); // pick the team with the least number of players
	}
	else
		team = client->sess.sessionTeam;

	//Testing to see if this fixes the problem with a bot's team getting set incorrectly.
	team = client->sess.sessionTeam;

	//Set the siege class
	if (level.gametype == GT_SIEGE)
	{
		Q_strncpyz(className, client->sess.siegeClass, sizeof className);

		//Now that the team is legal for sure, we'll go ahead and get an index for it.
		client->siegeClass = BG_SiegeFindClassIndexByName(className);
		if (client->siegeClass == -1)
		{
			// ok, get the first valid class for the team you're on then, I guess.
			BG_SiegeCheckClassLegality(team, className);
			Q_strncpyz(client->sess.siegeClass, className, sizeof client->sess.siegeClass);
			client->siegeClass = BG_SiegeFindClassIndexByName(className);
		}
		else
		{
			// otherwise, make sure the class we are using is legal.
			G_ValidateSiegeClassForTeam(ent, team);
			Q_strncpyz(className, client->sess.siegeClass, sizeof className);
		}

		if (client->siegeClass != -1)
		{
			// Set the sabers if the class dictates
			siegeClass_t* scl = &bgSiegeClasses[client->siegeClass];

			G_SetSaber(ent, 0, scl->saber1[0] ? scl->saber1 : DEFAULT_SABER, qtrue);
			G_SetSaber(ent, 1, scl->saber2[0] ? scl->saber2 : "none", qtrue);

			//make sure the saber models are updated
			G_SaberModelSetup(ent);

			if (scl->forcedModel[0])
			{
				// be sure to override the model we actually use
				Q_strncpyz(model, scl->forcedModel, sizeof model);
				if (d_perPlayerGhoul2.integer && Q_stricmp(model, client->modelname))
				{
					Q_strncpyz(client->modelname, model, sizeof client->modelname);
					model_changed = qtrue;
				}
			}

			if (G_PlayerHasCustomSkeleton(ent))
			{
				//force them to use their class model on the server, if the class dictates
				if (Q_stricmp(model, client->modelname) || ent->localAnimIndex == 0)
				{
					Q_strncpyz(client->modelname, model, sizeof client->modelname);
					model_changed = qtrue;
				}
			}
		}
	}
	else
		Q_strncpyz(className, "none", sizeof className);

	// only set the saber name on the first connect.
	//	it will be read from userinfo on ClientSpawn and stored in client->pers.saber1/2
	if (!VALIDSTRING(client->pers.saber1) || !VALIDSTRING(client->pers.saber2))
	{
		G_SetSaber(ent, 0, Info_ValueForKey(userinfo, "saber1"), qfalse);
		G_SetSaber(ent, 1, Info_ValueForKey(userinfo, "saber2"), qfalse);
	}

	// set max health
	if (level.gametype == GT_SIEGE && client->siegeClass != -1)
	{
		const siegeClass_t* scl = &bgSiegeClasses[client->siegeClass];

		if (scl->maxhealth)
			max_health = scl->maxhealth;

		health = max_health;
	}
	else
		health = Com_Clampi(1, 100, atoi(Info_ValueForKey(userinfo, "handicap")));

	client->pers.maxHealth = health;
	if (client->pers.maxHealth < 1 || client->pers.maxHealth > max_health)
		client->pers.maxHealth = 100;
	client->ps.stats[STAT_MAX_HEALTH] = client->pers.maxHealth;

	if (level.gametype >= GT_TEAM)
		client->pers.teamInfo = qtrue;
	else
	{
		s = Info_ValueForKey(userinfo, "teamoverlay");
		if (!*s || atoi(s) != 0)
			client->pers.teamInfo = qtrue;
		else
			client->pers.teamInfo = qfalse;
	}

	const int team_leader = client->sess.teamLeader;

	// colors
	Q_strncpyz(color1, Info_ValueForKey(userinfo, "color1"), sizeof color1);
	Q_strncpyz(color2, Info_ValueForKey(userinfo, "color2"), sizeof color2);

	s = Info_ValueForKey(userinfo, "SJE_clientplugin");
	if (!*s)
	{
		client->pers.SJE_clientplugin = qfalse;
	}
	else if (strcmp(CURRENT_SJE_CLIENTVERSION, s) == 0)
	{
		client->pers.SJE_clientplugin = qtrue;
	}

	Q_strncpyz(rgb1, Info_ValueForKey(userinfo, "rgb_saber1"), sizeof rgb1);
	Q_strncpyz(rgb2, Info_ValueForKey(userinfo, "rgb_saber2"), sizeof rgb2);

	Q_strncpyz(script1, Info_ValueForKey(userinfo, "rgb_script1"), sizeof script1);
	Q_strncpyz(script2, Info_ValueForKey(userinfo, "rgb_script2"), sizeof script2);

	// gender hints
	s = Info_ValueForKey(userinfo, "sex");
	if (!Q_stricmp(s, "female"))
	{
		gender = GENDER_FEMALE;
	}
	else
	{
		gender = GENDER_MALE;
	}

	s = Info_ValueForKey(userinfo, "snaps");
	if (atoi(s) < sv_fps.integer)
	{
		trap->SendServerCommand(client_num, va(
			"print \"" S_COLOR_YELLOW
			"Recommend setting /snaps %d or higher to match this server's sv_fps\n\"",
			sv_fps.integer));
	}

	// send over a subset of the userinfo keys so other clients can
	// print scoreboards, display models, and play custom sounds
	buf[0] = '\0';
	Q_strcat(buf, sizeof buf, va("n\\%s\\", client->pers.netname));
	Q_strcat(buf, sizeof buf, va("t\\%i\\", client->sess.sessionTeam));
	Q_strcat(buf, sizeof buf, va("model\\%s\\", model));

	if (gender == GENDER_FEMALE)
	{
		Q_strcat(buf, sizeof buf, va("ds\\%c\\", 'f'));
	}
	else
	{
		Q_strcat(buf, sizeof buf, va("ds\\%c\\", 'm'));
	}

	Q_strcat(buf, sizeof buf, va("st\\%s\\", client->pers.saber1));
	Q_strcat(buf, sizeof buf, va("st2\\%s\\", client->pers.saber2));
	Q_strcat(buf, sizeof buf, va("c1\\%s\\", color1));
	Q_strcat(buf, sizeof buf, va("c2\\%s\\", color2));
	Q_strcat(buf, sizeof buf, va("hc\\%i\\", client->pers.maxHealth));
	//RGBSaber STUFF
	Q_strcat(buf, sizeof buf, va("tc1\\%s\\", rgb1));
	Q_strcat(buf, sizeof buf, va("tc2\\%s\\", rgb2));
	Q_strcat(buf, sizeof buf, va("ss1\\%s\\", script1));
	Q_strcat(buf, sizeof buf, va("ss2\\%s\\", script2));

	if (ent->r.svFlags & SVF_BOT)
	{
		Q_strcat(buf, sizeof buf, va("skill\\%s\\", Info_ValueForKey(userinfo, "skill")));
	}

	if (level.gametype == GT_DUEL || level.gametype == GT_POWERDUEL)
	{
		Q_strcat(buf, sizeof buf, va("w\\%i\\", client->sess.wins));
		Q_strcat(buf, sizeof buf, va("l\\%i\\", client->sess.losses));
	}

	if (level.gametype == GT_POWERDUEL)
	{
		Q_strcat(buf, sizeof buf, va("dt\\%i\\", client->sess.duelTeam));
	}

	if (level.gametype >= GT_TEAM)
	{
		Q_strcat(buf, sizeof buf, va("tl\\%d\\", team_leader));
	}

	if (level.gametype == GT_SIEGE)
	{
		Q_strcat(buf, sizeof buf, va("siegeclass\\%s\\", className));
		Q_strcat(buf, sizeof buf, va("sdt\\%i\\", client->sess.siegeDesiredTeam));
	}

	trap->GetConfigstring(CS_PLAYERS + client_num, oldClientinfo, sizeof oldClientinfo);
	trap->SetConfigstring(CS_PLAYERS + client_num, buf);

	// only going to be true for allowable server-side custom skeleton cases
	if (model_changed)
	{
		// update the server g2 instance if appropriate
		char* modelname = Info_ValueForKey(userinfo, "model");
		SetupGameGhoul2Model(ent, modelname, NULL);

		if (ent->ghoul2 && ent->client)
			ent->client->renderInfo.lastG2 = NULL; //update the renderinfo bolts next update.

		client->torsoAnimExecute = client->legsAnimExecute = -1;
		client->torsoLastFlip = client->legsLastFlip = qfalse;
	}

	if (g_logClientInfo.integer)
	{
		if (strcmp(oldClientinfo, buf) != 0)
			G_LogPrintf("ClientUserinfoChanged: %i %s\n", client_num, buf);
		else
			G_LogPrintf("ClientUserinfoChanged: %i <no change>\n", client_num);
	}

	return qtrue;
}

/*
===========
ClientConnect

Called when a player begins connecting to the server.
Called again for every map change or tournament restart.

The session information will be valid after exit.

Return NULL if the client should be allowed, otherwise return
a string with the reason for denial.

Otherwise, the client will be sent the current gamestate
and will eventually get to ClientBegin.

firstTime will be qtrue the very first time a client connects
to the server machine, but qfalse on map changes and tournament
restarts.
============
*/

static qboolean CompareIPs(const char* ip1, const char* ip2)
{
	while (1)
	{
		if (*ip1 != *ip2)
			return qfalse;
		if (!*ip1 || *ip1 == ':')
			break;
		ip1++;
		ip2++;
	}

	return qtrue;
}

char* ClientConnect(int client_num, qboolean firstTime, qboolean isBot)
{
	char userinfo[MAX_INFO_STRING] = { 0 },
		tmpIP[NET_ADDRSTRMAXLEN] = { 0 },
		guid[33] = { 0 };

	gentity_t* ent = &g_entities[client_num];

	ent->s.number = client_num;
	ent->classname = "connecting";

	trap->GetUserinfo(client_num, userinfo, sizeof userinfo);

	if (g_lms.integer > 0 && BG_IsLMSGametype(g_gametype.integer))
	{
		//LMS mode, set up lives.
		ent->lives = g_lmslives.integer >= 1 ? g_lmslives.integer : 1;
		if (level.gametype == GT_SINGLE_PLAYER)
		{
			//LMS mode, playing missions, setup liveExp
			ent->liveExp = 0;
		}
	}

	char* value = Info_ValueForKey(userinfo, "ja_guid");
	if (value[0])
		Q_strncpyz(guid, value, sizeof guid);
	else if (isBot)
		Q_strncpyz(guid, "BOT", sizeof guid);
	else
		Q_strncpyz(guid, "NOGUID", sizeof guid);

	// check to see if they are on the banned IP list
	value = Info_ValueForKey(userinfo, "ip");
	Q_strncpyz(tmpIP, isBot ? "Bot" : value, sizeof tmpIP);
	if (G_FilterPacket(value))
	{
		return "Banned.";
	}

	if (!isBot && g_needpass.integer)
	{
		// check for a password
		value = Info_ValueForKey(userinfo, "password");
		if (g_password.string[0] && Q_stricmp(g_password.string, "none") &&
			strcmp(g_password.string, value) != 0)
		{
			static char sTemp[1024];
			Q_strncpyz(sTemp, G_GetStringEdString("MP_SVGAME", "INVALID_ESCAPE_TO_MAIN"), sizeof sTemp);
			return sTemp; // return "Invalid password";
		}
	}

	if (!isBot && firstTime)
	{
		if (g_antiFakePlayer.integer)
		{
			// patched, check for > g_maxConnPerIP connections from same IP
			int count = 0;
			for (int i = 0; i < sv_maxclients.integer; i++)
			{
#if 0
				if (level.clients[i].pers.connected != CON_DISCONNECTED && i != client_num)
				{
					if (CompareIPs(client_num, i))
					{
						if (!level.security.clientConnectionActive[i])
						{//This IP has a dead connection pending, wait for it to time out
						//	client->pers.connected = CON_DISCONNECTED;
							return "Please wait, another connection from this IP is still pending...";
						}
					}
				}
#else
				if (CompareIPs(tmpIP, level.clients[i].sess.IP))
					count++;
#endif
			}
			if (count > g_maxConnPerIP.integer)
			{
				//	client->pers.connected = CON_DISCONNECTED;
				return "Too many connections from the same IP";
			}
		}
	}

	if (ent->inuse)
	{
		// if a player reconnects quickly after a disconnect, the client disconnect may never be called, thus flag can get lost in the ether
		G_LogPrintf("Forcing disconnect on active client: %i\n", client_num);
		// so lets just fix up anything that should happen on a disconnect
		ClientDisconnect(client_num);
	}

	// they can connect
	gclient_t* client = &level.clients[client_num];
	ent->client = client;

	//assign the pointer for bg entity access
	ent->playerState = &ent->client->ps;

	memset(client, 0, sizeof * client);

	Q_strncpyz(client->pers.guid, guid, sizeof client->pers.guid);

	client->pers.connected = CON_CONNECTING;
	client->pers.connectTime = level.time;

	// read or initialize the session data
	if (firstTime || level.newSession)
	{
		G_InitSessionData(client, userinfo, isBot);
	}
	G_ReadSessionData(client);

	if (level.gametype == GT_SIEGE &&
		(firstTime || level.newSession))
	{
		//if this is the first time then auto-assign a desired siege team and show briefing for that team
		client->sess.siegeDesiredTeam = 0; //PickTeam(ent->s.number);
	}

	if (level.gametype == GT_SIEGE && client->sess.sessionTeam != TEAM_SPECTATOR)
	{
		if (firstTime || level.newSession)
		{
			//start as spec
			client->sess.siegeDesiredTeam = client->sess.sessionTeam;
			client->sess.sessionTeam = TEAM_SPECTATOR;
		}
	}
	else if (level.gametype == GT_POWERDUEL && client->sess.sessionTeam != TEAM_SPECTATOR)
	{
		client->sess.sessionTeam = TEAM_SPECTATOR;
	}

	if (isBot)
	{
		ent->r.svFlags |= SVF_BOT;
		ent->inuse = qtrue;
		if (!G_BotConnect(client_num, !firstTime))
		{
			return "BotConnectfailed";
		}
	}

	// get and distribute relevent paramters
	if (!client_userinfo_changed(client_num))
		return "Failed userinfo validation";

	if (!isBot && firstTime)
	{
		if (!tmpIP[0])
		{
			//No IP sent when connecting, probably an unban hack attempt
			client->pers.connected = CON_DISCONNECTED;
			G_SecurityLogPrintf("Client %i (%s) sent no IP when connecting.\n", client_num, client->pers.netname);
			return "Invalid userinfo detected";
		}
	}

	if (firstTime)
		Q_strncpyz(client->sess.IP, tmpIP, sizeof client->sess.IP);

	G_LogPrintf("ClientConnect: %i [%s] (%s) \"%s^7\"\n", client_num, tmpIP, guid, client->pers.netname);

	// don't do the "xxx connected" messages if they were caried over from previous level
	if (firstTime)
	{
		trap->SendServerCommand(-1, va("print \"%s" S_COLOR_WHITE " %s\n\"", client->pers.netname,
			G_GetStringEdString("MP_SVGAME", "PLCONNECT")));
	}

	if (level.gametype >= GT_TEAM &&
		client->sess.sessionTeam != TEAM_SPECTATOR)
	{
		BroadcastTeamChange(client, -1);
	}

	// count current clients and rank for scoreboard
	CalculateRanks();

	gentity_t* te = G_TempEntity(vec3_origin, EV_CLIENTJOIN);
	te->r.svFlags |= SVF_BROADCAST;
	te->s.eventParm = client_num;

	// for statistics
	//	client->areabits = areabits;
	//	if ( !client->areabits )
	//		client->areabits = G_Alloc( (trap->AAS_PointReachabilityAreaIndex( NULL ) + 7) / 8 );

	return NULL;
}

void G_WriteClientSessionData(const gclient_t* client);

void WP_SetSaber(int entNum, saberInfo_t* sabers, int saber_num, const char* saberName);

/*
===========
ClientBegin

called when a client has finished connecting, and is ready
to be placed into the level.  This will happen every level load,
and on transition between teams, but doesn't happen on respawns
============
*/
extern void PlayerPain(gentity_t* self, int damage);
extern qboolean gSiegeRoundBegun;
extern qboolean gSiegeRoundEnded;
extern qboolean g_dontPenalizeTeam; //g_cmds.c
void SetTeamQuick(const gentity_t* ent, int team, qboolean doBegin);

void ClientBegin(int client_num, qboolean allowTeamReset)
{
	char userinfo[MAX_INFO_VALUE];
	//contains the message of the day that is sent to new players.
	char motd[1024];

	gentity_t* ent = g_entities + client_num;

	if (ent->r.svFlags & SVF_BOT && level.gametype >= GT_TEAM)
	{
		if (allowTeamReset)
		{
			const char* team;

			//SetTeam(ent, "");
			ent->client->sess.sessionTeam = PickTeam(-1);
			trap->GetUserinfo(client_num, userinfo, MAX_INFO_STRING);

			if (ent->client->sess.sessionTeam == TEAM_SPECTATOR)
			{
				ent->client->sess.sessionTeam = TEAM_RED;
			}

			if (ent->client->sess.sessionTeam == TEAM_RED)
			{
				team = "Red";
			}
			else
			{
				team = "Blue";
			}

			Info_SetValueForKey(userinfo, "team", team);

			trap->SetUserinfo(client_num, userinfo);

			ent->client->ps.persistant[PERS_TEAM] = ent->client->sess.sessionTeam;

			const int preSess = ent->client->sess.sessionTeam;
			G_ReadSessionData(ent->client);
			ent->client->sess.sessionTeam = preSess;
			G_WriteClientSessionData(ent->client);
			if (!client_userinfo_changed(client_num))
				return;
			ClientBegin(client_num, qfalse);
			return;
		}
	}

	gclient_t* client = level.clients + client_num;

	if (ent->r.linked)
	{
		trap->UnlinkEntity((sharedEntity_t*)ent);
	}
	G_InitGentity(ent);
	ent->touch = 0;
	ent->pain = 0;
	ent->client = client;

	//assign the pointer for bg entity access
	ent->playerState = &ent->client->ps;

	client->pers.connected = CON_CONNECTED;
	client->pers.enterTime = level.time;
	client->pers.teamState.state = TEAM_BEGIN;

	// save eflags around this, because changing teams will
	// cause this to happen with a valid entity, and we
	// want to make sure the teleport bit is set right
	// so the viewpoint doesn't interpolate through the
	// world to the new position
	const int flags = client->ps.eFlags;
	const int spawnCount = client->ps.persistant[PERS_SPAWN_COUNT];

	int i = 0;

	while (i < NUM_FORCE_POWERS)
	{
		if (ent->client->ps.fd.forcePowersActive & 1 << i)
		{
			WP_ForcePowerStop(ent, i);
		}
		i++;
	}

	i = TRACK_CHANNEL_1;

	while (i < NUM_TRACK_CHANNELS)
	{
		if (ent->client->ps.fd.killSoundEntIndex[i - 50] && ent->client->ps.fd.killSoundEntIndex[i - 50] < MAX_GENTITIES
			&& ent->client->ps.fd.killSoundEntIndex[i - 50] > 0)
		{
			G_MuteSound(ent->client->ps.fd.killSoundEntIndex[i - 50], CHAN_VOICE);
		}
		i++;
	}
	i = 0;

	memset(&client->ps, 0, sizeof client->ps);
	client->ps.eFlags = flags;
	client->ps.persistant[PERS_SPAWN_COUNT] = spawnCount;

	client->ps.hasDetPackPlanted = qfalse;

	//first-time force power initialization
	WP_InitForcePowers(ent);

	//init saber ent
	wp_saber_init_blade_data(ent);

	// First time model setup for that player.
	trap->GetUserinfo(client_num, userinfo, sizeof userinfo);
	char* modelname = Info_ValueForKey(userinfo, "model");
	SetupGameGhoul2Model(ent, modelname, NULL);

	if (ent->ghoul2 && ent->client)
		ent->client->renderInfo.lastG2 = NULL; //update the renderinfo bolts next update.

	if (level.gametype == GT_POWERDUEL && client->sess.sessionTeam != TEAM_SPECTATOR && client->sess.duelTeam ==
		DUELTEAM_FREE)
		SetTeam(ent, "s");
	else
	{
		if (level.gametype == GT_SIEGE && (!gSiegeRoundBegun || gSiegeRoundEnded))
		{
			SetTeamQuick(ent, TEAM_SPECTATOR, qfalse);
		}

		if (g_lms.integer > 0 && ent->lives < 1 && BG_IsLMSGametype(g_gametype.integer) && LMS_EnoughPlayers() && client
			->sess.sessionTeam != TEAM_SPECTATOR)
		{
			//don't allow players to respawn in LMS by switching teams.
			g_Spectator(ent);
		}
		else
		{
			ClientSpawn(ent);
			if (client->sess.sessionTeam != TEAM_SPECTATOR && LMS_EnoughPlayers())
			{
				//costs a life to switch teams to something other than spectator.
				ent->lives--;
			}
		}
	}

	if (client->sess.sessionTeam != TEAM_SPECTATOR)
	{
		// send event
		if (g_lms.integer <= 0 || ent->lives >= 1 || !BG_IsLMSGametype(g_gametype.integer))
		{
			//don't do the "teleport in" effect if we're playing LMS and we're "out"
			gentity_t* tent = G_TempEntity(ent->client->ps.origin, EV_PLAYER_TELEPORT_IN);
			tent->s.client_num = ent->s.client_num;
		}
		if (level.gametype != GT_DUEL || level.gametype == GT_POWERDUEL)
		{
			trap->SendServerCommand(-1, va("print \"%s" S_COLOR_WHITE " %s\n\"", client->pers.netname,
				G_GetStringEdString("MP_SVGAME", "PLENTER")));
		}
	}
	G_LogPrintf("ClientBegin: %i\n", client_num);

	if (client->pers.SJE_clientplugin)
	{
		//send this client the MOTD for clients using the right version of sje.
		TextWrapCenterPrint(SJE_clientMOTD.string, motd);
	}
	else
	{
		//send this client the MOTD for clients aren't running sje or just not the right version.
		TextWrapCenterPrint(SJE_MOTD.string, motd);
	}

	trap->SendServerCommand(client_num, va("cp \"%s\n\"", motd));

	// count current clients and rank for scoreboard
	CalculateRanks();

	G_ClearClientLog(client_num);
}

qboolean AllForceDisabled(int force)
{
	if (force)
	{
		for (int i = 0; i < NUM_FORCE_POWERS; i++)
		{
			if (!(force & 1 << i))
			{
				return qfalse;
			}
		}

		return qtrue;
	}

	return qfalse;
}

//Convenient interface to set all my limb breakage stuff up -rww
void G_BreakArm(gentity_t* ent, int arm)
{
	int anim = -1;

	assert(ent && ent->client);

	if (ent->s.NPC_class == CLASS_VEHICLE || ent->localAnimIndex > 1)
	{
		//no broken limbs for vehicles and non-humanoids
		return;
	}

	if (!arm)
	{
		//repair him
		ent->client->ps.brokenLimbs = 0;
		return;
	}

	if (ent->client->ps.fd.saberAnimLevel == SS_STAFF)
	{
		//I'm too lazy to deal with this as well for now.
		return;
	}

	if (arm == BROKENLIMB_LARM)
	{
		if (ent->client->saber[1].model[0] &&
			ent->client->ps.weapon == WP_SABER &&
			!ent->client->ps.saberHolstered &&
			ent->client->saber[1].soundOff)
		{
			//the left arm shuts off its saber upon being broken
			G_Sound(ent, CHAN_AUTO, ent->client->saber[1].soundOff);
		}
	}

	ent->client->ps.brokenLimbs = 0; //make sure it's cleared out
	ent->client->ps.brokenLimbs |= 1 << arm; //this arm is now marked as broken

	//Do a pain anim based on the side. Since getting your arm broken does tend to hurt.
	if (arm == BROKENLIMB_LARM)
	{
		anim = BOTH_PAIN2;
	}
	else if (arm == BROKENLIMB_RARM)
	{
		anim = BOTH_PAIN3;
	}

	if (anim == -1)
	{
		return;
	}

	G_SetAnim(ent, &ent->client->pers.cmd, SETANIM_BOTH, anim, SETANIM_FLAG_OVERRIDE | SETANIM_FLAG_HOLD, 0);

	//This could be combined into a single event. But I guess limbs don't break often enough to
	//worry about it.
	G_EntitySound(ent, CHAN_VOICE, G_SoundIndex("*pain25.wav"));
	//FIXME: A nice bone snapping sound instead if possible
	G_Sound(ent, CHAN_AUTO, G_SoundIndex(va("sound/player/bodyfall_human%i.wav", Q_irand(1, 3))));
}

//Update the ghoul2 instance anims based on the playerstate values
qboolean PM_SaberStanceAnim(int anim);
qboolean PM_RunningAnim(int anim);
extern stringID_table_t animTable[MAX_ANIMATIONS + 1];

void G_UpdateClientAnims(gentity_t* self, float animSpeedScale)
{
	static int torsoAnim;
	static int legsAnim;
	static int firstFrame, lastFrame;
	static int aFlags;
	static float animSpeed, lAnimSpeedScale;
	qboolean setTorso = qfalse;

	torsoAnim = self->client->ps.torsoAnim;
	legsAnim = self->client->ps.legsAnim;

	if (self->client->ps.saberLockFrame)
	{
		trap->G2API_SetBoneAnim(self->ghoul2, 0, "model_root", self->client->ps.saberLockFrame,
			self->client->ps.saberLockFrame + 1, BONE_ANIM_OVERRIDE_FREEZE | BONE_ANIM_BLEND,
			animSpeedScale, level.time, -1, 150);
		trap->G2API_SetBoneAnim(self->ghoul2, 0, "lower_lumbar", self->client->ps.saberLockFrame,
			self->client->ps.saberLockFrame + 1, BONE_ANIM_OVERRIDE_FREEZE | BONE_ANIM_BLEND,
			animSpeedScale, level.time, -1, 150);
		trap->G2API_SetBoneAnim(self->ghoul2, 0, "Motion", self->client->ps.saberLockFrame,
			self->client->ps.saberLockFrame + 1, BONE_ANIM_OVERRIDE_FREEZE | BONE_ANIM_BLEND,
			animSpeedScale, level.time, -1, 150);
		return;
	}

	if (self->localAnimIndex > 1 &&
		bgAllAnims[self->localAnimIndex].anims[legsAnim].firstFrame == 0 &&
		bgAllAnims[self->localAnimIndex].anims[legsAnim].numFrames == 0)
	{
		//We'll allow this for non-humanoids.
		goto tryTorso;
	}

	if (self->client->legsAnimExecute != legsAnim || self->client->legsLastFlip != self->client->ps.legsFlip)
	{
		animSpeed = 50.0f / bgAllAnims[self->localAnimIndex].anims[legsAnim].frameLerp;
		lAnimSpeedScale = animSpeed *= animSpeedScale;

		if (bgAllAnims[self->localAnimIndex].anims[legsAnim].loopFrames != -1)
		{
			aFlags = BONE_ANIM_OVERRIDE_LOOP;
		}
		else
		{
			aFlags = BONE_ANIM_OVERRIDE_FREEZE;
		}

		if (animSpeed < 0)
		{
			lastFrame = bgAllAnims[self->localAnimIndex].anims[legsAnim].firstFrame;
			firstFrame = bgAllAnims[self->localAnimIndex].anims[legsAnim].firstFrame + bgAllAnims[self->localAnimIndex].
				anims[legsAnim].numFrames;
		}
		else
		{
			firstFrame = bgAllAnims[self->localAnimIndex].anims[legsAnim].firstFrame;
			lastFrame = bgAllAnims[self->localAnimIndex].anims[legsAnim].firstFrame + bgAllAnims[self->localAnimIndex].
				anims[legsAnim].numFrames;
		}

		aFlags |= BONE_ANIM_BLEND;
		//since client defaults to blend. Not sure if this will make much difference if any on server position, but it's here just for the sake of matching them.

		trap->G2API_SetBoneAnim(self->ghoul2, 0, "model_root", firstFrame, lastFrame, aFlags, lAnimSpeedScale,
			level.time, -1, 150);
		self->client->legsAnimExecute = legsAnim;
		self->client->legsLastFlip = self->client->ps.legsFlip;
	}

tryTorso:
	{
		if (self->localAnimIndex > 1 &&
			bgAllAnims[self->localAnimIndex].anims[torsoAnim].firstFrame == 0 &&
			bgAllAnims[self->localAnimIndex].anims[torsoAnim].numFrames == 0)

		{
			//If this fails as well just return.
			return;
		}
		if (self->s.number >= MAX_CLIENTS &&
			self->s.NPC_class == CLASS_VEHICLE)
		{
			//we only want to set the root bone for vehicles
			return;
		}
	}

	if ((self->client->torsoAnimExecute != torsoAnim || self->client->torsoLastFlip != self->client->ps.torsoFlip) &&
		!self->noLumbar)
	{
		static int f;
		aFlags = 0;
		animSpeed = 0;

		f = torsoAnim;

		pm_saber_start_trans_anim(self->s.number, self->client->ps.fd.saberAnimLevel, self->client->ps.weapon, f,
			&animSpeedScale, self->client->ps.brokenLimbs, self->client->ps.userInt3);

		animSpeed = 50.0f / bgAllAnims[self->localAnimIndex].anims[f].frameLerp;
		lAnimSpeedScale = animSpeed *= animSpeedScale;

		if (bgAllAnims[self->localAnimIndex].anims[f].loopFrames != -1)
		{
			aFlags = BONE_ANIM_OVERRIDE_LOOP;
		}
		else
		{
			aFlags = BONE_ANIM_OVERRIDE_FREEZE;
		}

		aFlags |= BONE_ANIM_BLEND;
		//since client defaults to blend. Not sure if this will make much difference if any on client position, but it's here just for the sake of matching them.

		if (animSpeed < 0)
		{
			lastFrame = bgAllAnims[self->localAnimIndex].anims[f].firstFrame;
			firstFrame = bgAllAnims[self->localAnimIndex].anims[f].firstFrame + bgAllAnims[self->localAnimIndex].anims[
				f].numFrames;
		}
		else
		{
			firstFrame = bgAllAnims[self->localAnimIndex].anims[f].firstFrame;
			lastFrame = bgAllAnims[self->localAnimIndex].anims[f].firstFrame + bgAllAnims[self->localAnimIndex].anims[f]
				.numFrames;
		}

		trap->G2API_SetBoneAnim(self->ghoul2, 0, "lower_lumbar", firstFrame, lastFrame, aFlags, lAnimSpeedScale,
			level.time, /*firstFrame why was it this before?*/-1, 150);

		self->client->torsoAnimExecute = torsoAnim;
		self->client->torsoLastFlip = self->client->ps.torsoFlip;

		setTorso = qtrue;
	}

	if (setTorso &&
		self->localAnimIndex <= 1)
	{
		//only set the motion bone for humanoids.
		trap->G2API_SetBoneAnim(self->ghoul2, 0, "Motion", firstFrame, lastFrame, aFlags, lAnimSpeedScale, level.time,
			-1, 150);
	}

#if 0 //disabled for now
	if (self->client->ps.brokenLimbs != self->client->brokenLimbs ||
		setTorso)
	{
		if (self->localAnimIndex <= 1 && self->client->ps.brokenLimbs &&
			(self->client->ps.brokenLimbs & (1 << BROKENLIMB_LARM)))
		{ //broken left arm
			char* brokenBone = "lhumerus";
			animation_t* armAnim;
			int armFirstFrame;
			int armLastFrame;
			int armFlags = 0;
			float armAnimSpeed;

			armAnim = &bgAllAnims[self->localAnimIndex].anims[BOTH_DEAD21];
			self->client->brokenLimbs = self->client->ps.brokenLimbs;

			armFirstFrame = armAnim->firstFrame;
			armLastFrame = armAnim->firstFrame + armAnim->numFrames;
			armAnimSpeed = 50.0f / armAnim->frameLerp;
			armFlags = (BONE_ANIM_OVERRIDE_LOOP | BONE_ANIM_BLEND);

			trap->G2API_SetBoneAnim(self->ghoul2, 0, brokenBone, armFirstFrame, armLastFrame, armFlags, armAnimSpeed, level.time, -1, 150);
		}
		else if (self->localAnimIndex <= 1 && self->client->ps.brokenLimbs &&
			(self->client->ps.brokenLimbs & (1 << BROKENLIMB_RARM)))
		{ //broken right arm
			char* brokenBone = "rhumerus";
			char* supportBone = "lhumerus";

			self->client->brokenLimbs = self->client->ps.brokenLimbs;

			//Only put the arm in a broken pose if the anim is such that we
			//want to allow it.
			if ((//self->client->ps.weapon == WP_MELEE ||
				self->client->ps.weapon != WP_SABER ||
				PM_SaberStanceAnim(self->client->ps.torsoAnim) ||
				PM_RunningAnim(self->client->ps.torsoAnim)) &&
				(!self->client->saber[1].model[0] || self->client->ps.weapon != WP_SABER))
			{
				int armFirstFrame;
				int armLastFrame;
				int armFlags = 0;
				float armAnimSpeed;
				animation_t* armAnim;

				if (self->client->ps.weapon == WP_MELEE ||
					self->client->ps.weapon == WP_SABER ||
					self->client->ps.weapon == WP_BRYAR_PISTOL)
				{ //don't affect this arm if holding a gun, just make the other arm support it
					armAnim = &bgAllAnims[self->localAnimIndex].anims[BOTH_ATTACK2];

					//armFirstFrame = armAnim->firstFrame;
					armFirstFrame = armAnim->firstFrame + armAnim->numFrames;
					armLastFrame = armAnim->firstFrame + armAnim->numFrames;
					armAnimSpeed = 50.0f / armAnim->frameLerp;
					armFlags = (BONE_ANIM_OVERRIDE_LOOP | BONE_ANIM_BLEND);

					trap->G2API_SetBoneAnim(self->ghoul2, 0, brokenBone, armFirstFrame, armLastFrame, armFlags, armAnimSpeed, level.time, -1, 150);
				}
				else
				{ //we want to keep the broken bone updated for some cases
					trap->G2API_SetBoneAnim(self->ghoul2, 0, brokenBone, firstFrame, lastFrame, aFlags, lAnimSpeedScale, level.time, -1, 150);
				}

				if (self->client->ps.torsoAnim != BOTH_MELEE1 &&
					self->client->ps.torsoAnim != BOTH_MELEE2 &&
					self->client->ps.torsoAnim != BOTH_MELEE3 &&
					self->client->ps.torsoAnim != BOTH_MELEE4 &&
					self->client->ps.torsoAnim != BOTH_MELEE5 &&
					self->client->ps.torsoAnim != BOTH_MELEE6 &&
					self->client->ps.torsoAnim != BOTH_MELEE_L &&
					self->client->ps.torsoAnim != BOTH_MELEE_R &&
					self->client->ps.torsoAnim != BOTH_MELEEUP &&
					self->client->ps.torsoAnim != BOTH_WOOKIE_SLAP &&
					(self->client->ps.torsoAnim == TORSO_WEAPONREADY2 || self->client->ps.torsoAnim == BOTH_ATTACK2 || self->client->ps.weapon < WP_BRYAR_PISTOL))
				{
					//Now set the left arm to "support" the right one
					armAnim = &bgAllAnims[self->localAnimIndex].anims[BOTH_STAND2];
					armFirstFrame = armAnim->firstFrame;
					armLastFrame = armAnim->firstFrame + armAnim->numFrames;
					armAnimSpeed = 50.0f / armAnim->frameLerp;
					armFlags = (BONE_ANIM_OVERRIDE_LOOP | BONE_ANIM_BLEND);

					trap->G2API_SetBoneAnim(self->ghoul2, 0, supportBone, armFirstFrame, armLastFrame, armFlags, armAnimSpeed, level.time, -1, 150);
				}
				else
				{ //we want to keep the support bone updated for some cases
					trap->G2API_SetBoneAnim(self->ghoul2, 0, supportBone, firstFrame, lastFrame, aFlags, lAnimSpeedScale, level.time, -1, 150);
				}
			}
			else
			{ //otherwise, keep it set to the same as the torso
				trap->G2API_SetBoneAnim(self->ghoul2, 0, brokenBone, firstFrame, lastFrame, aFlags, lAnimSpeedScale, level.time, -1, 150);
				trap->G2API_SetBoneAnim(self->ghoul2, 0, supportBone, firstFrame, lastFrame, aFlags, lAnimSpeedScale, level.time, -1, 150);
			}
		}
		else if (self->client->brokenLimbs)
		{ //remove the bone now so it can be set again
			char* brokenBone = NULL;
			int broken = 0;

			//Warning: Don't remove bones that you've added as bolts unless you want to invalidate your bolt index
			//(well, in theory, I haven't actually run into the problem)
			if (self->client->brokenLimbs & (1 << BROKENLIMB_LARM))
			{
				brokenBone = "lhumerus";
				broken |= (1 << BROKENLIMB_LARM);
			}
			else if (self->client->brokenLimbs & (1 << BROKENLIMB_RARM))
			{ //can only have one arm broken at once.
				brokenBone = "rhumerus";
				broken |= (1 << BROKENLIMB_RARM);

				//want to remove the support bone too then
				trap->G2API_SetBoneAnim(self->ghoul2, 0, "lhumerus", 0, 1, 0, 0, level.time, -1, 0);
				trap->G2API_RemoveBone(self->ghoul2, "lhumerus", 0);
			}

			assert(brokenBone);

			//Set the flags and stuff to 0, so that the remove will succeed
			trap->G2API_SetBoneAnim(self->ghoul2, 0, brokenBone, 0, 1, 0, 0, level.time, -1, 0);

			//Now remove it
			trap->G2API_RemoveBone(self->ghoul2, brokenBone, 0);
			self->client->brokenLimbs &= ~broken;
		}
	}
#endif
}

/*
===========
ClientSpawn

Called every time a client is placed fresh in the world:
after the first ClientBegin, and after each respawn
Initializes all non-persistant parts of playerState
============
*/
extern void UpdatePlayerScriptTarget(void);
extern qboolean UseSpawnWeapons;
extern int SpawnWeapons;
extern qboolean WP_HasForcePowers(const playerState_t* ps);

void ClientSpawn(gentity_t* ent)
{
	int i = 0, index = 0, savesaber_num = ENTITYNUM_NONE, wDisable = 0, savedSiegeIndex = 0, maxHealth = 100;
	vec3_t spawn_origin, spawn_angles;
	gentity_t* spawnPoint = NULL, * tent = NULL;
	gclient_t* client = NULL;
	clientPersistant_t saved;
	clientSession_t savedSess;
	forcedata_t savedForce;
	int savedDodgeMax;
	saberInfo_t saberSaved[MAX_SABERS];
	int persistant[MAX_PERSISTANT] = { 0 };
	int flags, gameFlags, savedPing, accuracy_hits, accuracy_shots, eventSequence;
	void* g2WeaponPtrs[MAX_SABERS];
	char userinfo[MAX_INFO_STRING] = { 0 }, * key = NULL, * value = NULL, * saber = NULL;
	qboolean changedSaber = qfalse, inSiegeWithClass = qfalse;

	index = ent - g_entities;
	client = ent->client;
	client->pers.botclass = client->pers.nextbotclass;

	//first we want the userinfo so we can see if we should update this client's saber -rww
	trap->GetUserinfo(index, userinfo, sizeof userinfo);

	for (i = 0; i < MAX_SABERS; i++)
	{
		saber = i & 1 ? ent->client->pers.saber2 : ent->client->pers.saber1;
		value = Info_ValueForKey(userinfo, va("saber%i", i + 1));
		if (saber && value &&
			(Q_stricmp(value, saber) || !saber[0] || !ent->client->saber[0].model[0]))
		{
			//doesn't match up (or our saber is BS), we want to try setting it
			if (G_SetSaber(ent, i, value, qfalse))
				changedSaber = qtrue;

			//Well, we still want to say they changed then (it means this is siege and we have some overrides)
			else if (!saber[0] || !ent->client->saber[0].model[0])
				changedSaber = qtrue;
		}
	}

	if (changedSaber)
	{
		//make sure our new info is sent out to all the other clients, and give us a valid stance
		if (!client_userinfo_changed(ent->s.number))
			return;

		//make sure the saber models are updated
		G_SaberModelSetup(ent);

		for (i = 0; i < MAX_SABERS; i++)
		{
			saber = i & 1 ? ent->client->pers.saber2 : ent->client->pers.saber1;
			key = va("saber%d", i + 1);
			value = Info_ValueForKey(userinfo, key);
			if (Q_stricmp(value, saber))
			{
				// they don't match up, force the user info
				Info_SetValueForKey(userinfo, key, saber);
				trap->SetUserinfo(ent->s.number, userinfo);
			}
		}
	}

	if (client->ps.fd.forceDoInit || ent->r.svFlags & SVF_BOT)
	{
		//force a reread of force powers
		WP_InitForcePowers(ent);
		client->ps.fd.forceDoInit = 0;
	}
	if (ent->r.svFlags & SVF_BOT
		&& ent->client->ps.fd.saberAnimLevel != SS_STAFF
		&& ent->client->ps.fd.saberAnimLevel != SS_DUAL)
	{
		//Bots randomly switch styles on respawn if not using a staff or dual
		int newLevel = Q_irand(SS_MEDIUM, SS_STAFF);

		//new validation technique.
		if (!G_ValidSaberStyle(ent, newLevel))
		{
			//had an illegal style, revert to a valid one
			int count;
			for (count = SS_MEDIUM; count < SS_STAFF; count++)
			{
				newLevel++;
				if (newLevel > SS_STAFF)
				{
					if (ent->client->saber[0].type == SABER_BACKHAND
						|| ent->client->saber[0].type == SABER_ASBACKHAND)
					{
						newLevel = SS_STAFF;
					}
					else if (ent->client->saber[0].type == SABER_STAFF
						|| ent->client->saber[0].type == SABER_STAFF_UNSTABLE
						|| ent->client->saber[0].type == SABER_STAFF_THIN)
					{
						newLevel = SS_DESANN;
					}
					else
					{
						if (ent->client->saber[0].model[0] && ent->client->saber[1].model[0]
							&& WP_SaberCanTurnOffSomeBlades(&ent->client->saber[1])
							&& ent->client->ps.fd.saberAnimLevel != SS_DUAL)
						{//using dual sabers, but not the dual style
							newLevel = SS_TAVION;
						}
						else
						{
							newLevel = SS_MEDIUM;
						}
					}
				}

				if (G_ValidSaberStyle(ent, newLevel))
				{
					break;
				}
			}
		}

		ent->client->ps.fd.saberAnimLevel = newLevel;
	}

	ent->client->ps.fd.saberAnimLevel = ent->client->ps.fd.saberDrawAnimLevel = ent->client->sess.saberLevel;

	if (g_gametype.integer != GT_SIEGE)
	{
		//let's just make sure the styles we chose are cool
		if (!G_ValidSaberStyle(ent, ent->client->ps.fd.saberAnimLevel))
		{//had an illegal style, revert to default
			if ((ent->client->saber[0].type == SABER_BACKHAND))
			{
				ent->client->ps.fd.saberAnimLevel = SS_STAFF;
			}
			else if ((ent->client->saber[0].type == SABER_ASBACKHAND))
			{
				ent->client->ps.fd.saberAnimLevel = SS_STAFF;
			}
			else if ((ent->client->saber[0].type == SABER_STAFF_MAUL))
			{
				ent->client->ps.fd.saberAnimLevel = SS_STAFF;
			}
			else if ((ent->client->saber[0].type == SABER_ELECTROSTAFF))
			{
				ent->client->ps.fd.saberAnimLevel = SS_STAFF;
			}
			else
			{
				ent->client->ps.fd.saberAnimLevel = SS_MEDIUM;
			}
			ent->client->saberCycleQueue = ent->client->ps.fd.saberAnimLevel;
		}
	}

	// find a spawn point
	// do it before setting health back up, so farthest
	// ranging doesn't count this client
	if (client->sess.sessionTeam == TEAM_SPECTATOR)
	{
		spawnPoint = SelectSpectatorSpawnPoint(spawn_origin, spawn_angles);
	}
	else if (level.gametype == GT_CTF || level.gametype == GT_CTY)
	{
		// all base oriented team games use the CTF spawn points
		spawnPoint = SelectCTFSpawnPoint(client->sess.sessionTeam, client->pers.teamState.state, spawn_origin,
			spawn_angles, !!(ent->r.svFlags & SVF_BOT));
	}
	else if (level.gametype == GT_SIEGE)
	{
		spawnPoint = SelectSiegeSpawnPoint(client->siegeClass, client->sess.sessionTeam, client->pers.teamState.state,
			spawn_origin, spawn_angles, !!(ent->r.svFlags & SVF_BOT));
	}
	else if (level.gametype == GT_SINGLE_PLAYER)
	{
		spawnPoint = SelectSPSpawnPoint(spawn_origin, spawn_angles);
	}
	else
	{
		if (level.gametype == GT_POWERDUEL)
		{
			spawnPoint = SelectDuelSpawnPoint(client->sess.duelTeam, client->ps.origin, spawn_origin, spawn_angles,
				!!(ent->r.svFlags & SVF_BOT));
		}
		else if (level.gametype == GT_DUEL)
		{
			// duel
			spawnPoint = SelectDuelSpawnPoint(DUELTEAM_SINGLE, client->ps.origin, spawn_origin, spawn_angles,
				!!(ent->r.svFlags & SVF_BOT));
		}
		else
		{
			// the first spawn should be at a good looking spot
			if (!client->pers.initialSpawn && client->pers.localClient)
			{
				client->pers.initialSpawn = qtrue;
				spawnPoint = SelectInitialSpawnPoint(spawn_origin, spawn_angles, client->sess.sessionTeam,
					!!(ent->r.svFlags & SVF_BOT));
			}
			else
			{
				// don't spawn near existing origin if possible
				spawnPoint = SelectSpawnPoint(
					client->ps.origin,
					spawn_origin, spawn_angles, client->sess.sessionTeam, !!(ent->r.svFlags & SVF_BOT));
			}
		}
	}
	client->pers.teamState.state = TEAM_ACTIVE;

	// toggle the teleport bit so the client knows to not lerp
	// and never clear the voted flag
	flags = ent->client->ps.eFlags & EF_TELEPORT_BIT;
	flags ^= EF_TELEPORT_BIT;
	gameFlags = ent->client->mGameFlags & (PSG_VOTED | PSG_TEAMVOTED);

	// clear everything but the persistant data

	saved = client->pers;
	savedSess = client->sess;
	savedPing = client->ps.ping;
	accuracy_hits = client->accuracy_hits;
	accuracy_shots = client->accuracy_shots;

	for (i = 0; i < MAX_PERSISTANT; i++)
		persistant[i] = client->ps.persistant[i];

	eventSequence = client->ps.eventSequence;

	savedForce = client->ps.fd;

	savesaber_num = client->ps.saberEntityNum;

	savedSiegeIndex = client->siegeClass;

	savedDodgeMax = client->ps.stats[STAT_MAX_DODGE];

	for (i = 0; i < MAX_SABERS; i++)
	{
		saberSaved[i] = client->saber[i];
		g2WeaponPtrs[i] = client->weaponGhoul2[i];
	}

	for (i = 0; i < HL_MAX; i++)
		ent->locationDamage[i] = 0;

	memset(client, 0, sizeof * client); // bk FIXME: Com_Memset?
	client->bodyGrabIndex = ENTITYNUM_NONE;

	//Get the skin RGB based on his userinfo
	client->ps.customRGBA[0] = ((value = Info_ValueForKey(userinfo, "char_color_red")))
		? Com_Clampi(0, 255, atoi(value))
		: 255;
	client->ps.customRGBA[1] = ((value = Info_ValueForKey(userinfo, "char_color_green")))
		? Com_Clampi(0, 255, atoi(value))
		: 255;
	client->ps.customRGBA[2] = ((value = Info_ValueForKey(userinfo, "char_color_blue")))
		? Com_Clampi(0, 255, atoi(value))
		: 255;

	//Prevent skins being too dark
	if (g_charRestrictRGB.integer && client->ps.customRGBA[0] + client->ps.customRGBA[1] + client->ps.customRGBA[2] <
		100)
		client->ps.customRGBA[0] = client->ps.customRGBA[1] = client->ps.customRGBA[2] = 255;

	client->ps.customRGBA[3] = 255;

	if (level.gametype >= GT_TEAM && level.gametype != GT_SIEGE && !g_jediVmerc.integer)
	{
		char skin[MAX_QPATH] = { 0 }, model[MAX_QPATH] = { 0 };
		vec3_t colorOverride = { 0.0f };

		VectorClear(colorOverride);
		Q_strncpyz(model, Info_ValueForKey(userinfo, "model"), sizeof model);

		BG_ValidateSkinForTeam(model, skin, savedSess.sessionTeam, colorOverride);
		if (colorOverride[0] != 0.0f || colorOverride[1] != 0.0f || colorOverride[2] != 0.0f)
			VectorScaleM(colorOverride, 255.0f, client->ps.customRGBA);
	}

	client->siegeClass = savedSiegeIndex;

	client->ps.stats[STAT_MAX_DODGE] = savedDodgeMax;

	for (i = 0; i < MAX_SABERS; i++)
	{
		client->saber[i] = saberSaved[i];
		client->weaponGhoul2[i] = g2WeaponPtrs[i];
	}

	//or the saber ent num
	client->ps.saberEntityNum = savesaber_num;
	client->saberStoredIndex = savesaber_num;

	client->ps.fd = savedForce;

	client->ps.duelIndex = ENTITYNUM_NONE;

	//spawn with 100
	client->ps.jetpackFuel = 100;
	client->ps.sprintFuel = 100;
	client->ps.cloakFuel = 100;
	client->ps.fd.blockPoints = BLOCK_POINTS_MAX;

	client->pers = saved;
	client->sess = savedSess;
	client->ps.ping = savedPing;
	client->accuracy_hits = accuracy_hits;
	client->accuracy_shots = accuracy_shots;
	client->lastkilled_client = -1;

	for (i = 0; i < MAX_PERSISTANT; i++)
		client->ps.persistant[i] = persistant[i];

	client->ps.eventSequence = eventSequence;
	// increment the spawncount so the client will detect the respawn
	client->ps.persistant[PERS_SPAWN_COUNT]++;
	client->ps.persistant[PERS_TEAM] = client->sess.sessionTeam;

	client->airOutTime = level.time + 12000;

	// set max health
	if (level.gametype == GT_SIEGE && client->siegeClass != -1)
	{
		siegeClass_t* scl = &bgSiegeClasses[client->siegeClass];
		maxHealth = 100;

		if (scl->maxhealth)
		{
			maxHealth = scl->maxhealth;
		}
	}
	else
	{
		maxHealth = Com_Clampi(1, 100, atoi(Info_ValueForKey(userinfo, "handicap")));
	}
	client->pers.maxHealth = maxHealth;

	if (client->pers.maxHealth < 1 || client->pers.maxHealth > maxHealth)
	{
		client->pers.maxHealth = 100;
	}
	// clear entity values
	client->ps.stats[STAT_MAX_HEALTH] = client->pers.maxHealth;
	client->ps.eFlags = flags;
	client->mGameFlags = gameFlags;

	client->ps.groundEntityNum = ent->s.groundEntityNum = ENTITYNUM_NONE;
	ent->client = &level.clients[index];
	ent->playerState = &ent->client->ps;
	ent->takedamage = qtrue;
	ent->inuse = qtrue;
	ent->classname = "player";
	ent->r.contents = CONTENTS_BODY;
	ent->clipmask = MASK_PLAYERSOLID;
	ent->die = player_die;
	ent->waterlevel = 0;
	ent->watertype = 0;
	ent->flags = 0;

	VectorCopy(playerMins, ent->r.mins);
	VectorCopy(playerMaxs, ent->r.maxs);
	client->ps.crouchheight = CROUCH_MAXS_2;
	client->ps.standheight = DEFAULT_MAXS_2;

	client->ps.client_num = index;
	//give default weapons
	client->ps.stats[STAT_WEAPONS] = 1 << WP_NONE;

	if (level.gametype == GT_DUEL || level.gametype == GT_POWERDUEL)
	{
		wDisable = g_duelWeaponDisable.integer;
	}
	else
	{
		wDisable = g_weaponDisable.integer;
	}

	if (ent->r.svFlags & SVF_BOT)
	{
		client->skillLevel[SK_PISTOL] = FORCE_LEVEL_3;
		client->skillLevel[SK_BLASTER] = FORCE_LEVEL_3;
		client->skillLevel[SK_THERMAL] = FORCE_LEVEL_3;
		client->skillLevel[SK_BOWCASTER] = FORCE_LEVEL_3;
		client->skillLevel[SK_DETPACK] = FORCE_LEVEL_3;
		client->skillLevel[SK_REPEATER] = FORCE_LEVEL_3;
		client->skillLevel[SK_DISRUPTOR] = FORCE_LEVEL_3;
	}

	//Give everyone fists as long as they aren't disabled.
	if (!wDisable || !(wDisable & 1 << WP_MELEE))
	{
		client->ps.stats[STAT_WEAPONS] |= 1 << WP_MELEE;
	}

	if (level.gametype != GT_HOLOCRON
		&& level.gametype != GT_JEDIMASTER
		&& !HasSetSaberOnly()
		&& !AllForceDisabled(g_forcePowerDisable.integer)
		&& g_jediVmerc.integer)
	{
		if (level.gametype >= GT_TEAM && (client->sess.sessionTeam == TEAM_BLUE || client->sess.sessionTeam ==
			TEAM_RED))
		{
			//In Team games, force one side to be merc and other to be jedi
			if (level.numPlayingClients > 0)
			{
				//already someone in the game
				int forceTeam = TEAM_SPECTATOR;
				for (i = 0; i < level.maxclients; i++)
				{
					if (level.clients[i].pers.connected == CON_DISCONNECTED)
					{
						continue;
					}
					if (level.clients[i].sess.sessionTeam == TEAM_BLUE || level.clients[i].sess.sessionTeam == TEAM_RED)
					{
						//in-game
						if (WP_HasForcePowers(&level.clients[i].ps))
						{
							//this side is using force
							forceTeam = level.clients[i].sess.sessionTeam;
						}
						else
						{
							//other team is using force
							if (level.clients[i].sess.sessionTeam == TEAM_BLUE)
							{
								forceTeam = TEAM_RED;
							}
							else
							{
								forceTeam = TEAM_BLUE;
							}
						}
						break;
					}
				}
				if (WP_HasForcePowers(&client->ps) && client->sess.sessionTeam != forceTeam)
				{
					//using force but not on right team, switch him over
					const char* teamName = TeamName(forceTeam);
					//client->sess.sessionTeam = forceTeam;
					SetTeam(ent, (char*)teamName);
					return;
				}
			}
		}

		if (WP_HasForcePowers(&client->ps))
		{
			client->ps.trueNonJedi = qfalse;
			client->ps.trueJedi = qtrue;
			//make sure they only use the saber
			client->ps.weapon = WP_SABER;
			client->ps.stats[STAT_WEAPONS] = 1 << WP_SABER;
		}
		else
		{
			//no force powers set
			client->ps.trueNonJedi = qtrue;
			client->ps.trueJedi = qfalse;
			if (!wDisable || !(wDisable & 1 << WP_BRYAR_PISTOL))
			{
				client->ps.stats[STAT_WEAPONS] |= 1 << WP_BRYAR_PISTOL;
			}
			if (!wDisable || !(wDisable & 1 << WP_BLASTER))
			{
				client->ps.stats[STAT_WEAPONS] |= 1 << WP_BLASTER;
			}
			if (!wDisable || !(wDisable & 1 << WP_BOWCASTER))
			{
				client->ps.stats[STAT_WEAPONS] |= 1 << WP_BOWCASTER;
			}
			if (!wDisable || !(wDisable & 1 << WP_REPEATER))
			{
				client->ps.stats[STAT_WEAPONS] |= 1 << WP_REPEATER;
			}
			if (!wDisable || !(wDisable & 1 << WP_DISRUPTOR)) //JRHockney
			{
				client->ps.stats[STAT_WEAPONS] |= 1 << WP_DISRUPTOR;
			}
			if (!wDisable || !(wDisable & 1 << WP_FLECHETTE))
			{
				client->ps.stats[STAT_WEAPONS] |= 1 << WP_FLECHETTE;
			}
			client->ps.stats[STAT_WEAPONS] &= ~(1 << WP_SABER);
			client->ps.stats[STAT_WEAPONS] |= 1 << WP_MELEE;
			client->ps.ammo[AMMO_POWERCELL] = ammoData[AMMO_POWERCELL].max;
			client->ps.ammo[AMMO_BLASTER] = ammoData[AMMO_BLASTER].max;
			client->ps.ammo[AMMO_METAL_BOLTS] = ammoData[AMMO_METAL_BOLTS].max;
			client->skillLevel[SK_PISTOL] = FORCE_LEVEL_3;
			client->ps.eFlags |= EF_DUAL_WEAPONS;
			client->skillLevel[SK_BLASTER] = FORCE_LEVEL_3;
			client->skillLevel[SK_THERMAL] = FORCE_LEVEL_3;
			client->skillLevel[SK_BOWCASTER] = FORCE_LEVEL_3;
			client->skillLevel[SK_DETPACK] = FORCE_LEVEL_3;
			client->skillLevel[SK_REPEATER] = FORCE_LEVEL_3;
			client->skillLevel[SK_DISRUPTOR] = FORCE_LEVEL_3;
			client->skillLevel[SK_CRYOBAN] = FORCE_LEVEL_3;
		}
	}
	else
	{
		//jediVmerc is incompatible with this gametype, turn it off!
		trap->Cvar_Set("g_jediVmerc", "0");
		trap->Cvar_Update(&g_jediVmerc);
		if (level.gametype == GT_HOLOCRON)
		{
			//always get free saber level 1 in holocron
			if (!wDisable || !(wDisable & 1 << WP_SABER))
			{
				client->ps.stats[STAT_WEAPONS] |= 1 << WP_SABER;
			}
		}
		else if (g_gametype.integer == GT_JEDIMASTER || g_gametype.integer == GT_HOLOCRON)
		{
			//don't have selectable starting weapons, but we do have max max gun skills so
			//people can actually fight well, when they get a gun.
			client->skillLevel[SK_PISTOL] = FORCE_LEVEL_2;
			client->skillLevel[SK_BLASTER] = FORCE_LEVEL_3;
			client->skillLevel[SK_THERMAL] = FORCE_LEVEL_3;
			client->skillLevel[SK_BOWCASTER] = FORCE_LEVEL_3;
			client->skillLevel[SK_DETPACK] = FORCE_LEVEL_3;
			client->skillLevel[SK_REPEATER] = FORCE_LEVEL_3;
			client->skillLevel[SK_DISRUPTOR] = FORCE_LEVEL_3;
			client->skillLevel[SK_CRYOBAN] = FORCE_LEVEL_3;
		}
		else
		{
			//New class system
			if (ent->s.eType != ET_NPC && // no npcs,handled in npc.cfg
				level.gametype != GT_SIEGE)
			{
				//use class-specified weapons
				switch (client->pers.botclass)
				{
					//botclass weapons and ammo here
				case BCLASS_CADENCE:
				case BCLASS_SERENITY:
					client->ps.stats[STAT_WEAPONS] |= 1 << WP_MELEE;
					client->ps.stats[STAT_WEAPONS] |= 1 << WP_SABER;
					client->ps.stats[STAT_HOLDABLE_ITEMS] |= (1 << HI_SEEKER);
					break;
				case BCLASS_JEDITRAINER:
				case BCLASS_ALORA:
				case BCLASS_DESANN:
				case BCLASS_KYLE:
				case BCLASS_LUKE:
				case BCLASS_JEDIMASTER:
				case BCLASS_DUELS:
				case BCLASS_CULTIST:
				case BCLASS_JEDI:
				case BCLASS_MORGANKATARN:
				case BCLASS_REBORN_TWIN:
				case BCLASS_REBORN_MASTER:
				case BCLASS_REBORN:
				case BCLASS_ROSH_PENIN:
				case BCLASS_SABER_DROID:
				case BCLASS_TAVION:
				case BCLASS_SHADOWTROOPER:
				case BCLASS_YODA:
				case BCLASS_PADAWAN:
				case BCLASS_GRIEVOUS:
				case BCLASS_SITHLORD:
				case BCLASS_LORDVADER:
				case BCLASS_SITH:
				case BCLASS_APPRENTICE:
				case BCLASS_JEDIKNIGHT1:
				case BCLASS_JEDIKNIGHT2:
				case BCLASS_JEDIKNIGHT3:
				case BCLASS_SMUGGLER1:
				case BCLASS_SMUGGLER3:
				case BCLASS_JEDICONSULAR1:
				case BCLASS_JEDICONSULAR2:
				case BCLASS_JEDICONSULAR3:
				case BCLASS_WOOKIE:
				case BCLASS_SITHWORRIOR1:
				case BCLASS_SITHWORRIOR2:
				case BCLASS_SITHWORRIOR3:
				case BCLASS_SITHINQUISITOR1:
				case BCLASS_SITHINQUISITOR2:
				case BCLASS_SITHINQUISITOR3:
				case BCLASS_STAFFDARK:
				case BCLASS_STAFF:
				case BCLASS_UNSTABLESABER:
					client->ps.stats[STAT_WEAPONS] |= 1 << WP_MELEE;
					client->ps.stats[STAT_WEAPONS] |= 1 << WP_SABER;
					break;
				case BCLASS_ASSASSIN_DROID:
					client->ps.stats[STAT_WEAPONS] |= 1 << WP_DISRUPTOR;
					client->skillLevel[SK_DISRUPTOR] = FORCE_LEVEL_3;
					client->ps.ammo[AMMO_BLASTER] = 900;
					client->ps.ammo[AMMO_POWERCELL] = 500;
					client->ps.ammo[AMMO_METAL_BOLTS] = 900;
					break;
				case BCLASS_BARTENDER:
					client->ps.stats[STAT_WEAPONS] |= 1 << WP_MELEE;
					client->ps.stats[STAT_WEAPONS] |= 1 << WP_DISRUPTOR;
					client->skillLevel[SK_DISRUPTOR] = FORCE_LEVEL_3;
					client->ps.ammo[AMMO_BLASTER] = 900;
					client->ps.ammo[AMMO_POWERCELL] = 500;
					client->ps.ammo[AMMO_METAL_BOLTS] = 900;
					client->ps.stats[STAT_WEAPONS] |= 1 << WP_DET_PACK;
					client->skillLevel[SK_DETPACK] = FORCE_LEVEL_3;
					client->ps.ammo[AMMO_DETPACK] = 3;
					break;
				case BCLASS_BESPIN_COP:
					client->ps.stats[STAT_WEAPONS] |= 1 << WP_MELEE;
					client->skillLevel[SK_ACROBATICS] = FORCE_LEVEL_3;
					client->ps.stats[STAT_WEAPONS] |= 1 << WP_BRYAR_PISTOL;
					client->skillLevel[SK_PISTOL] = FORCE_LEVEL_3;
					client->ps.eFlags |= EF_DUAL_WEAPONS;
					client->ps.ammo[AMMO_BLASTER] = 900;
					client->ps.ammo[AMMO_POWERCELL] = 500;
					client->ps.ammo[AMMO_METAL_BOLTS] = 900;
					client->ps.stats[STAT_WEAPONS] |= 1 << WP_THERMAL;
					client->ps.ammo[AMMO_THERMAL] = 4;
					break;
				case BCLASS_BOBAFETT:
					client->ps.stats[STAT_WEAPONS] |= 1 << WP_MELEE;
					client->ps.stats[STAT_WEAPONS] |= 1 << WP_BLASTER;
					client->skillLevel[SK_ACROBATICS] = FORCE_LEVEL_3;
					client->skillLevel[SK_BLASTER] = FORCE_LEVEL_3;
					client->skillLevel[SK_BLASTERRATEOFFIREUPGRADE] = FORCE_LEVEL_3;
					client->ps.stats[STAT_WEAPONS] |= 1 << WP_BRYAR_PISTOL;
					client->skillLevel[SK_PISTOL] = FORCE_LEVEL_3;
					client->ps.eFlags |= EF_DUAL_WEAPONS;
					client->ps.ammo[AMMO_BLASTER] = 900;
					client->ps.ammo[AMMO_POWERCELL] = 500;
					client->ps.ammo[AMMO_METAL_BOLTS] = 900;
					client->ps.stats[STAT_WEAPONS] |= 1 << WP_ROCKET_LAUNCHER;
					client->skillLevel[SK_ROCKET] = FORCE_LEVEL_3;
					client->ps.ammo[AMMO_ROCKETS] = 3;
					client->ps.stats[STAT_WEAPONS] |= 1 << WP_THERMAL;
					client->skillLevel[SK_THERMAL] = FORCE_LEVEL_3;
					client->skillLevel[SK_SMOKEGRENADE] = FORCE_LEVEL_3;
					client->ps.ammo[AMMO_THERMAL] = 4;
					client->skillLevel[SK_GRAPPLE] = FORCE_LEVEL_3;
					break;
				case BCLASS_CHEWIE:
					client->ps.stats[STAT_WEAPONS] |= 1 << WP_MELEE;
					client->ps.stats[STAT_WEAPONS] |= 1 << WP_BOWCASTER;
					client->skillLevel[SK_BOWCASTER] = FORCE_LEVEL_3;
					client->ps.ammo[AMMO_BLASTER] = 900;
					client->ps.ammo[AMMO_POWERCELL] = 500;
					client->ps.ammo[AMMO_METAL_BOLTS] = 900;
					client->ps.stats[STAT_WEAPONS] |= 1 << WP_TRIP_MINE;
					client->ps.ammo[AMMO_TRIPMINE] = 3;
					client->ps.stats[STAT_WEAPONS] |= 1 << WP_DET_PACK;
					client->skillLevel[SK_DETPACK] = FORCE_LEVEL_3;
					client->ps.ammo[AMMO_DETPACK] = 3;
					break;
				case BCLASS_ELDER:
					client->ps.stats[STAT_WEAPONS] |= 1 << WP_MELEE;
					break;
				case BCLASS_GALAK:
					client->ps.stats[STAT_WEAPONS] |= 1 << WP_MELEE;
					client->ps.stats[STAT_WEAPONS] |= 1 << WP_REPEATER;
					client->skillLevel[SK_REPEATER] = FORCE_LEVEL_3;
					client->skillLevel[SK_REPEATERUPGRADE] = FORCE_LEVEL_3;
					client->ps.stats[STAT_WEAPONS] |= 1 << WP_FLECHETTE;
					client->skillLevel[SK_FLECHETTE] = FORCE_LEVEL_3;
					client->ps.ammo[AMMO_BLASTER] = 900;
					client->ps.ammo[AMMO_POWERCELL] = 500;
					client->ps.ammo[AMMO_METAL_BOLTS] = 900;
					client->ps.stats[STAT_WEAPONS] |= 1 << WP_ROCKET_LAUNCHER;
					client->skillLevel[SK_ROCKET] = FORCE_LEVEL_3;
					client->ps.ammo[AMMO_ROCKETS] = 3;
					client->ps.stats[STAT_WEAPONS] |= 1 << WP_THERMAL;
					client->skillLevel[SK_THERMAL] = FORCE_LEVEL_3;
					client->skillLevel[SK_CRYOBAN] = FORCE_LEVEL_3;
					client->ps.ammo[AMMO_THERMAL] = 4;
					client->ps.stats[STAT_HOLDABLE_ITEMS] |= 1 << HI_SEEKER;
					break;
				case BCLASS_GRAN:
					client->ps.stats[STAT_WEAPONS] |= 1 << WP_MELEE;
					client->ps.stats[STAT_WEAPONS] |= 1 << WP_BLASTER;
					client->skillLevel[SK_BLASTER] = FORCE_LEVEL_3;
					client->ps.ammo[AMMO_BLASTER] = 900;
					client->ps.ammo[AMMO_POWERCELL] = 500;
					client->ps.ammo[AMMO_METAL_BOLTS] = 900;
					break;
				case BCLASS_HAZARDTROOPER:
					client->ps.stats[STAT_WEAPONS] |= 1 << WP_MELEE;
					client->ps.stats[STAT_WEAPONS] |= 1 << WP_REPEATER;
					client->skillLevel[SK_REPEATER] = FORCE_LEVEL_3;
					client->ps.ammo[AMMO_BLASTER] = 900;
					client->ps.ammo[AMMO_POWERCELL] = 500;
					client->ps.ammo[AMMO_METAL_BOLTS] = 900;
					client->ps.stats[STAT_WEAPONS] |= 1 << WP_ROCKET_LAUNCHER;
					client->skillLevel[SK_ROCKET] = FORCE_LEVEL_3;
					client->ps.ammo[AMMO_ROCKETS] = 3;
					client->ps.stats[STAT_WEAPONS] |= 1 << WP_DET_PACK;
					client->skillLevel[SK_DETPACK] = FORCE_LEVEL_3;
					client->ps.ammo[AMMO_DETPACK] = 3;
					break;
				case BCLASS_HUMAN_MERC:
					client->ps.stats[STAT_WEAPONS] |= 1 << WP_MELEE;
					client->ps.stats[STAT_WEAPONS] |= 1 << WP_FLECHETTE;
					client->skillLevel[SK_FLECHETTE] = FORCE_LEVEL_3;
					client->ps.ammo[AMMO_BLASTER] = 900;
					client->ps.ammo[AMMO_POWERCELL] = 500;
					client->ps.ammo[AMMO_METAL_BOLTS] = 900;
					client->ps.stats[STAT_WEAPONS] |= 1 << WP_THERMAL;
					client->skillLevel[SK_THERMAL] = FORCE_LEVEL_3;
					client->skillLevel[SK_SMOKEGRENADE] = FORCE_LEVEL_3;
					client->ps.ammo[AMMO_THERMAL] = 4;
					client->ps.stats[STAT_WEAPONS] |= 1 << WP_DET_PACK;
					client->skillLevel[SK_DETPACK] = FORCE_LEVEL_3;
					client->ps.ammo[AMMO_DETPACK] = 4;
					break;
				case BCLASS_IMPERIAL:
					client->ps.stats[STAT_WEAPONS] |= 1 << WP_MELEE;
					client->ps.stats[STAT_WEAPONS] |= 1 << WP_DEMP2;
					client->ps.ammo[AMMO_BLASTER] = 900;
					client->ps.ammo[AMMO_POWERCELL] = 500;
					client->ps.ammo[AMMO_METAL_BOLTS] = 900;
					break;
				case BCLASS_IMPWORKER:
					client->ps.stats[STAT_WEAPONS] |= 1 << WP_MELEE;
					client->ps.stats[STAT_WEAPONS] |= 1 << WP_BLASTER;
					client->skillLevel[SK_BLASTER] = FORCE_LEVEL_3;
					client->ps.ammo[AMMO_BLASTER] = 900;
					client->ps.ammo[AMMO_POWERCELL] = 500;
					client->ps.ammo[AMMO_METAL_BOLTS] = 900;
					break;
				case BCLASS_JAN:
					client->ps.stats[STAT_WEAPONS] |= 1 << WP_MELEE;
					client->skillLevel[SK_ACROBATICS] = FORCE_LEVEL_3;
					client->ps.stats[STAT_WEAPONS] |= 1 << WP_BLASTER;
					client->skillLevel[SK_BLASTER] = FORCE_LEVEL_3;
					client->ps.ammo[AMMO_BLASTER] = 900;
					client->ps.ammo[AMMO_POWERCELL] = 500;
					client->ps.ammo[AMMO_METAL_BOLTS] = 900;
					client->ps.stats[STAT_WEAPONS] |= 1 << WP_THERMAL;
					client->skillLevel[SK_THERMAL] = FORCE_LEVEL_3;
					client->skillLevel[SK_FLASHGRENADE] = FORCE_LEVEL_3;
					client->ps.ammo[AMMO_THERMAL] = 4;
					client->ps.stats[STAT_HOLDABLE_ITEMS] |= 1 << HI_SEEKER;
					break;
				case BCLASS_JAWA:
					client->ps.stats[STAT_WEAPONS] |= 1 << WP_BRYAR_PISTOL;
					client->ps.stats[STAT_WEAPONS] |= 1 << WP_STUN_BATON;
					client->skillLevel[SK_PISTOL] = FORCE_LEVEL_2;
					client->ps.ammo[AMMO_BLASTER] = 900;
					client->ps.ammo[AMMO_POWERCELL] = 500;
					client->ps.ammo[AMMO_METAL_BOLTS] = 900;
					break;
				case BCLASS_LANDO:
					client->ps.stats[STAT_WEAPONS] |= 1 << WP_MELEE;
					client->skillLevel[SK_ACROBATICS] = FORCE_LEVEL_3;
					client->ps.stats[STAT_WEAPONS] |= 1 << WP_BRYAR_PISTOL;
					client->skillLevel[SK_PISTOL] = FORCE_LEVEL_2;
					client->skillLevel[SK_BLASTER] = FORCE_LEVEL_3;
					client->skillLevel[SK_BLASTERRATEOFFIREUPGRADE] = FORCE_LEVEL_3;
					client->ps.ammo[AMMO_BLASTER] = 900;
					client->ps.ammo[AMMO_POWERCELL] = 500;
					client->ps.ammo[AMMO_METAL_BOLTS] = 900;
					break;
				case BCLASS_GALAKMECH:
					client->ps.stats[STAT_WEAPONS] |= 1 << WP_MELEE;
					client->ps.stats[STAT_WEAPONS] |= 1 << WP_REPEATER;
					client->skillLevel[SK_REPEATER] = FORCE_LEVEL_3;
					client->skillLevel[SK_REPEATERUPGRADE] = FORCE_LEVEL_3;
					client->ps.ammo[AMMO_BLASTER] = 900;
					client->ps.ammo[AMMO_POWERCELL] = 500;
					client->ps.ammo[AMMO_METAL_BOLTS] = 900;
					client->ps.stats[STAT_WEAPONS] |= 1 << WP_FLECHETTE;
					client->skillLevel[SK_FLECHETTE] = FORCE_LEVEL_3;
					client->ps.stats[STAT_WEAPONS] |= 1 << WP_ROCKET_LAUNCHER;
					client->skillLevel[SK_ROCKET] = FORCE_LEVEL_3;
					client->ps.ammo[AMMO_ROCKETS] = 3;
					client->ps.stats[STAT_WEAPONS] |= 1 << WP_THERMAL;
					client->skillLevel[SK_THERMAL] = FORCE_LEVEL_3;
					client->skillLevel[SK_CRYOBAN] = FORCE_LEVEL_3;
					client->ps.ammo[AMMO_THERMAL] = 4;
					client->ps.stats[STAT_HOLDABLE_ITEMS] |= 1 << HI_SEEKER;
					client->ps.powerups[PW_GALAK_SHIELD] = Q3_INFINITE;
					break;
				case BCLASS_MONMOTHA:
					client->ps.stats[STAT_WEAPONS] |= 1 << WP_MELEE;
					client->ps.stats[STAT_WEAPONS] |= 1 << WP_BRYAR_PISTOL;
					client->skillLevel[SK_PISTOL] = FORCE_LEVEL_2;
					client->ps.stats[STAT_WEAPONS] |= 1 << WP_BLASTER;
					client->skillLevel[SK_BLASTER] = FORCE_LEVEL_3;
					client->ps.ammo[AMMO_BLASTER] = 900;
					client->ps.ammo[AMMO_POWERCELL] = 500;
					client->ps.ammo[AMMO_METAL_BOLTS] = 900;
					break;
				case BCLASS_NOGRHRI:
					client->ps.stats[STAT_WEAPONS] |= 1 << WP_MELEE;
					client->ps.stats[STAT_WEAPONS] |= 1 << WP_CONCUSSION;
					client->ps.ammo[AMMO_BLASTER] = 900;
					client->ps.ammo[AMMO_POWERCELL] = 500;
					client->ps.ammo[AMMO_METAL_BOLTS] = 900;
					client->ps.stats[STAT_WEAPONS] |= 1 << WP_THERMAL;
					client->skillLevel[SK_THERMAL] = FORCE_LEVEL_3;
					client->ps.ammo[AMMO_THERMAL] = 4;
					break;
				case BCLASS_PROTOCOL:
					client->ps.stats[STAT_WEAPONS] |= 1 << WP_MELEE;
					break;
				case BCLASS_R2D2:
					client->ps.stats[STAT_WEAPONS] |= 1 << WP_STUN_BATON;
					break;
				case BCLASS_R5D2:
					client->ps.stats[STAT_WEAPONS] |= 1 << WP_STUN_BATON;
					break;
				case BCLASS_RAGNOS:
					client->ps.stats[STAT_WEAPONS] |= 1 << WP_MELEE;
					client->ps.stats[STAT_WEAPONS] |= 1 << WP_SABER;
					break;
				case BCLASS_RANCOR:
					client->ps.stats[STAT_WEAPONS] |= 1 << WP_MELEE;
					break;
				case BCLASS_RAX:
					client->ps.stats[STAT_WEAPONS] |= 1 << WP_MELEE;
					client->ps.stats[STAT_WEAPONS] |= 1 << WP_CONCUSSION;
					client->ps.ammo[AMMO_BLASTER] = 900;
					client->ps.ammo[AMMO_POWERCELL] = 500;
					client->ps.ammo[AMMO_METAL_BOLTS] = 900;
					client->ps.stats[STAT_WEAPONS] |= 1 << WP_THERMAL;
					client->skillLevel[SK_THERMAL] = FORCE_LEVEL_3;
					client->skillLevel[SK_CRYOBAN] = FORCE_LEVEL_3;
					client->ps.ammo[AMMO_THERMAL] = 4;
					break;
				case BCLASS_REBEL:
					client->ps.stats[STAT_WEAPONS] |= 1 << WP_MELEE;
					client->ps.stats[STAT_WEAPONS] |= 1 << WP_BRYAR_PISTOL;
					client->skillLevel[SK_PISTOL] = FORCE_LEVEL_2;
					client->ps.stats[STAT_WEAPONS] |= 1 << WP_BLASTER;
					client->skillLevel[SK_BLASTER] = FORCE_LEVEL_3;
					client->ps.ammo[AMMO_BLASTER] = 900;
					client->ps.ammo[AMMO_POWERCELL] = 500;
					client->ps.ammo[AMMO_METAL_BOLTS] = 900;
					break;
				case BCLASS_REELO:
					client->ps.stats[STAT_WEAPONS] |= 1 << WP_MELEE;
					client->ps.stats[STAT_WEAPONS] |= 1 << WP_BLASTER;
					client->skillLevel[SK_BLASTER] = FORCE_LEVEL_3;
					client->ps.ammo[AMMO_BLASTER] = 900;
					client->ps.ammo[AMMO_POWERCELL] = 500;
					client->ps.ammo[AMMO_METAL_BOLTS] = 900;
					client->ps.stats[STAT_WEAPONS] |= 1 << WP_FLECHETTE;
					client->skillLevel[SK_FLECHETTE] = FORCE_LEVEL_3;
					client->ps.stats[STAT_WEAPONS] |= 1 << WP_CONCUSSION;
					client->ps.stats[STAT_WEAPONS] |= 1 << WP_REPEATER;
					client->skillLevel[SK_REPEATER] = FORCE_LEVEL_3;
					client->ps.stats[STAT_WEAPONS] |= 1 << WP_THERMAL;
					client->skillLevel[SK_THERMAL] = FORCE_LEVEL_3;
					client->skillLevel[SK_FLASHGRENADE] = FORCE_LEVEL_3;
					client->ps.ammo[AMMO_THERMAL] = 4;
					break;
				case BCLASS_ROCKETTROOPER:
					client->ps.stats[STAT_WEAPONS] |= 1 << WP_MELEE;
					client->ps.stats[STAT_WEAPONS] |= 1 << WP_BLASTER;
					client->skillLevel[SK_BLASTER] = FORCE_LEVEL_3;
					client->ps.ammo[AMMO_BLASTER] = 900;
					client->ps.ammo[AMMO_POWERCELL] = 500;
					client->ps.ammo[AMMO_METAL_BOLTS] = 900;
					client->ps.stats[STAT_WEAPONS] |= 1 << WP_REPEATER;
					client->skillLevel[SK_REPEATER] = FORCE_LEVEL_3;
					client->ps.stats[STAT_WEAPONS] |= 1 << WP_ROCKET_LAUNCHER;
					client->skillLevel[SK_ROCKET] = FORCE_LEVEL_3;
					client->ps.ammo[AMMO_ROCKETS] = 5;
					break;
				case BCLASS_RODIAN:
					client->ps.stats[STAT_WEAPONS] |= 1 << WP_MELEE;
					client->ps.stats[STAT_WEAPONS] |= 1 << WP_FLECHETTE;
					client->skillLevel[SK_FLECHETTE] = FORCE_LEVEL_3;
					client->ps.ammo[AMMO_BLASTER] = 900;
					client->ps.ammo[AMMO_POWERCELL] = 500;
					client->ps.ammo[AMMO_METAL_BOLTS] = 900;
					client->ps.stats[STAT_WEAPONS] |= 1 << WP_CONCUSSION;
					client->ps.stats[STAT_WEAPONS] |= 1 << WP_THERMAL;
					client->skillLevel[SK_THERMAL] = FORCE_LEVEL_3;
					client->ps.ammo[AMMO_THERMAL] = 4;
					break;
				case BCLASS_SABOTEUR:
					client->ps.stats[STAT_WEAPONS] |= 1 << WP_MELEE;
					client->ps.stats[STAT_ARMOR] = 100;
					client->ps.stats[STAT_WEAPONS] |= 1 << WP_DISRUPTOR;
					client->skillLevel[SK_DISRUPTOR] = FORCE_LEVEL_3;
					client->ps.ammo[AMMO_BLASTER] = 900;
					client->ps.ammo[AMMO_POWERCELL] = 500;
					client->ps.ammo[AMMO_METAL_BOLTS] = 900;
					break;
				case BCLASS_STORMTROOPER:
					client->ps.stats[STAT_WEAPONS] |= 1 << WP_MELEE;
					client->ps.stats[STAT_ARMOR] = 100;
					client->ps.stats[STAT_WEAPONS] |= 1 << WP_BLASTER;
					client->skillLevel[SK_BLASTER] = FORCE_LEVEL_3;
					client->ps.ammo[AMMO_BLASTER] = 900;
					client->ps.ammo[AMMO_POWERCELL] = 500;
					client->ps.ammo[AMMO_METAL_BOLTS] = 900;
					break;
				case BCLASS_STORMPILOT:
					client->ps.stats[STAT_WEAPONS] |= 1 << WP_MELEE;
					client->ps.stats[STAT_ARMOR] = 100;
					client->ps.stats[STAT_WEAPONS] |= 1 << WP_BRYAR_PISTOL;
					client->skillLevel[SK_PISTOL] = FORCE_LEVEL_2;
					client->ps.stats[STAT_WEAPONS] |= 1 << WP_THERMAL;
					client->ps.ammo[AMMO_BLASTER] = 900;
					client->ps.ammo[AMMO_POWERCELL] = 500;
					client->ps.ammo[AMMO_METAL_BOLTS] = 900;
					break;
				case BCLASS_SWAMPTROOPER:
					client->ps.stats[STAT_WEAPONS] |= 1 << WP_MELEE;
					client->ps.stats[STAT_WEAPONS] |= 1 << WP_FLECHETTE;
					client->skillLevel[SK_FLECHETTE] = FORCE_LEVEL_3;
					client->ps.ammo[AMMO_BLASTER] = 900;
					client->ps.ammo[AMMO_POWERCELL] = 500;
					client->ps.ammo[AMMO_METAL_BOLTS] = 900;
					client->ps.stats[STAT_WEAPONS] |= 1 << WP_CONCUSSION;
					client->ps.stats[STAT_WEAPONS] |= 1 << WP_REPEATER;
					client->skillLevel[SK_REPEATER] = FORCE_LEVEL_3;
					break;
				case BCLASS_TRANDOSHAN:
					client->ps.stats[STAT_WEAPONS] |= 1 << WP_MELEE;
					client->ps.stats[STAT_ARMOR] = 100;
					client->ps.stats[STAT_WEAPONS] |= 1 << WP_BLASTER;
					client->skillLevel[SK_BLASTER] = FORCE_LEVEL_3;
					client->ps.ammo[AMMO_BLASTER] = 900;
					client->ps.ammo[AMMO_POWERCELL] = 500;
					client->ps.ammo[AMMO_METAL_BOLTS] = 900;
					client->ps.stats[STAT_WEAPONS] |= 1 << WP_THERMAL;
					client->skillLevel[SK_THERMAL] = FORCE_LEVEL_3;
					client->ps.ammo[AMMO_THERMAL] = 4;
					break;
				case BCLASS_TUSKEN_SNIPER:
					client->ps.stats[STAT_WEAPONS] |= 1 << WP_MELEE;
					client->ps.stats[STAT_ARMOR] = 100;
					client->ps.stats[STAT_WEAPONS] |= 1 << WP_DISRUPTOR;
					client->skillLevel[SK_DISRUPTOR] = FORCE_LEVEL_3;
					client->ps.ammo[AMMO_BLASTER] = 900;
					client->ps.ammo[AMMO_POWERCELL] = 500;
					client->ps.ammo[AMMO_METAL_BOLTS] = 900;
					break;
				case BCLASS_TUSKEN_RAIDER:
					client->ps.stats[STAT_WEAPONS] |= 1 << WP_MELEE;
					client->ps.stats[STAT_WEAPONS] |= 1 << WP_SABER;
					break;
				case BCLASS_UGNAUGHT:
					client->ps.stats[STAT_WEAPONS] |= 1 << WP_MELEE;
					client->ps.stats[STAT_ARMOR] = 100;
					client->ps.stats[STAT_WEAPONS] |= 1 << WP_BRYAR_PISTOL;
					client->skillLevel[SK_PISTOL] = FORCE_LEVEL_2;
					client->ps.stats[STAT_WEAPONS] |= 1 << WP_THERMAL;
					client->skillLevel[SK_THERMAL] = FORCE_LEVEL_3;
					client->ps.ammo[AMMO_THERMAL] = 4;
					client->ps.ammo[AMMO_BLASTER] = 900;
					client->ps.ammo[AMMO_POWERCELL] = 500;
					client->ps.ammo[AMMO_METAL_BOLTS] = 900;
					break;
				case BCLASS_WAMPA:
					client->ps.stats[STAT_WEAPONS] |= 1 << WP_MELEE;
					break;
				case BCLASS_WEEQUAY:
					client->ps.stats[STAT_WEAPONS] |= 1 << WP_MELEE;
					client->ps.stats[STAT_WEAPONS] |= 1 << WP_CONCUSSION;
					client->ps.ammo[AMMO_BLASTER] = 900;
					client->ps.ammo[AMMO_POWERCELL] = 500;
					client->ps.ammo[AMMO_METAL_BOLTS] = 900;
					client->ps.stats[STAT_WEAPONS] |= 1 << WP_THERMAL;
					client->skillLevel[SK_THERMAL] = FORCE_LEVEL_3;
					client->ps.ammo[AMMO_THERMAL] = 4;
					break;
				case BCLASS_MANDOLORIAN: //boba fett
					client->ps.stats[STAT_WEAPONS] |= 1 << WP_MELEE;
					client->skillLevel[SK_ACROBATICS] = FORCE_LEVEL_3;
					client->ps.stats[STAT_WEAPONS] |= 1 << WP_BLASTER;
					client->skillLevel[SK_BLASTER] = FORCE_LEVEL_3;
					client->ps.stats[STAT_WEAPONS] |= 1 << WP_BRYAR_PISTOL;
					client->skillLevel[SK_PISTOL] = FORCE_LEVEL_3;
					client->ps.eFlags |= EF_DUAL_WEAPONS;
					client->ps.ammo[AMMO_BLASTER] = 900;
					client->ps.ammo[AMMO_POWERCELL] = 500;
					client->ps.ammo[AMMO_METAL_BOLTS] = 900;
					client->ps.stats[STAT_WEAPONS] |= 1 << WP_THERMAL;
					client->ps.ammo[AMMO_THERMAL] = 4;
					client->skillLevel[SK_GRAPPLE] = FORCE_LEVEL_3;
					break;
				case BCLASS_MANDOLORIAN1:
					client->ps.stats[STAT_WEAPONS] |= 1 << WP_MELEE;
					client->skillLevel[SK_ACROBATICS] = FORCE_LEVEL_3;
					client->ps.stats[STAT_WEAPONS] |= 1 << WP_BLASTER;
					client->skillLevel[SK_BLASTER] = FORCE_LEVEL_3;
					client->ps.stats[STAT_WEAPONS] |= 1 << WP_BRYAR_PISTOL;
					client->skillLevel[SK_PISTOL] = FORCE_LEVEL_3;
					client->ps.eFlags |= EF_DUAL_WEAPONS;
					client->ps.ammo[AMMO_BLASTER] = 900;
					client->ps.ammo[AMMO_POWERCELL] = 500;
					client->ps.ammo[AMMO_METAL_BOLTS] = 900;
					client->ps.stats[STAT_WEAPONS] |= 1 << WP_REPEATER;
					client->skillLevel[SK_REPEATER] = FORCE_LEVEL_3;
					client->ps.stats[STAT_WEAPONS] |= 1 << WP_ROCKET_LAUNCHER;
					client->skillLevel[SK_ROCKET] = FORCE_LEVEL_3;
					client->ps.ammo[AMMO_ROCKETS] = 5;
					client->skillLevel[SK_GRAPPLE] = FORCE_LEVEL_3;
					break;
				case BCLASS_MANDOLORIAN2:
					client->ps.stats[STAT_WEAPONS] |= 1 << WP_MELEE;
					client->skillLevel[SK_ACROBATICS] = FORCE_LEVEL_3;
					client->ps.stats[STAT_WEAPONS] |= 1 << WP_BRYAR_PISTOL;
					client->skillLevel[SK_PISTOL] = FORCE_LEVEL_3;
					client->ps.eFlags |= EF_DUAL_WEAPONS;
					client->ps.ammo[AMMO_BLASTER] = 900;
					client->ps.ammo[AMMO_POWERCELL] = 500;
					client->ps.ammo[AMMO_METAL_BOLTS] = 900;
					client->ps.ammo[AMMO_ROCKETS] = 5;
					client->ps.stats[STAT_WEAPONS] |= 1 << WP_THERMAL;
					client->skillLevel[SK_THERMAL] = FORCE_LEVEL_3;
					client->skillLevel[SK_FLASHGRENADE] = FORCE_LEVEL_3;
					client->ps.ammo[AMMO_THERMAL] = 4;
					client->skillLevel[SK_GRAPPLE] = FORCE_LEVEL_3;
					break;
				case BCLASS_SOILDER:
					client->ps.stats[STAT_WEAPONS] |= 1 << WP_MELEE;
					client->ps.stats[STAT_WEAPONS] |= 1 << WP_BLASTER;
					client->skillLevel[SK_BLASTER] = FORCE_LEVEL_3;
					client->ps.ammo[AMMO_BLASTER] = 900;
					client->ps.ammo[AMMO_POWERCELL] = 500;
					client->ps.ammo[AMMO_METAL_BOLTS] = 900;
					client->ps.stats[STAT_WEAPONS] |= 1 << WP_ROCKET_LAUNCHER;
					client->skillLevel[SK_ROCKET] = FORCE_LEVEL_3;
					client->ps.ammo[AMMO_ROCKETS] = 3;
					client->ps.stats[STAT_WEAPONS] |= 1 << WP_THERMAL;
					client->skillLevel[SK_THERMAL] = FORCE_LEVEL_3;
					client->skillLevel[SK_SMOKEGRENADE] = FORCE_LEVEL_3;
					client->ps.ammo[AMMO_THERMAL] = 4;
					break;
				case BCLASS_BATTLEDROID:
					client->ps.stats[STAT_WEAPONS] |= 1 << WP_BLASTER;
					client->skillLevel[SK_BLASTER] = FORCE_LEVEL_3;
					client->ps.ammo[AMMO_BLASTER] = 900;
					client->ps.ammo[AMMO_POWERCELL] = 500;
					client->ps.ammo[AMMO_METAL_BOLTS] = 900;
					break;
				case BCLASS_SBD:
					client->ps.stats[STAT_WEAPONS] |= 1 << WP_BRYAR_OLD;
					client->skillLevel[SK_PISTOL] = FORCE_LEVEL_3;
					client->skillLevel[SK_BLASTER] = FORCE_LEVEL_3;
					client->ps.ammo[AMMO_BLASTER] = 900;
					client->ps.ammo[AMMO_POWERCELL] = 900;
					client->ps.ammo[AMMO_METAL_BOLTS] = 900;
					break;
				case BCLASS_WOOKIEMELEE:
					client->ps.stats[STAT_WEAPONS] |= 1 << WP_MELEE;
					client->skillLevel[SK_ACROBATICS] = FORCE_LEVEL_3;
					break;
				case BCLASS_TROOPER1:
					client->ps.stats[STAT_WEAPONS] |= 1 << WP_MELEE;
					client->ps.stats[STAT_WEAPONS] |= 1 << WP_REPEATER;
					client->skillLevel[SK_REPEATER] = FORCE_LEVEL_3;
					client->ps.ammo[AMMO_BLASTER] = 900;
					client->ps.ammo[AMMO_POWERCELL] = 500;
					client->ps.ammo[AMMO_METAL_BOLTS] = 900;
					client->ps.stats[STAT_WEAPONS] |= 1 << WP_THERMAL;
					client->skillLevel[SK_THERMAL] = FORCE_LEVEL_3;
					client->skillLevel[SK_SMOKEGRENADE] = FORCE_LEVEL_3;
					client->ps.ammo[AMMO_THERMAL] = 4;
					break;
				case BCLASS_TROOPER2:
					client->ps.stats[STAT_WEAPONS] |= 1 << WP_MELEE;
					client->ps.stats[STAT_WEAPONS] |= 1 << WP_FLECHETTE;
					client->skillLevel[SK_FLECHETTE] = FORCE_LEVEL_3;
					client->ps.ammo[AMMO_BLASTER] = 900;
					client->ps.ammo[AMMO_POWERCELL] = 500;
					client->ps.ammo[AMMO_METAL_BOLTS] = 900;
					break;
				case BCLASS_TROOPER3:
					client->ps.stats[STAT_WEAPONS] |= 1 << WP_MELEE;
					client->ps.stats[STAT_WEAPONS] |= 1 << WP_REPEATER;
					client->skillLevel[SK_REPEATER] = FORCE_LEVEL_3;
					client->ps.ammo[AMMO_BLASTER] = 900;
					client->ps.ammo[AMMO_POWERCELL] = 500;
					client->ps.ammo[AMMO_METAL_BOLTS] = 900;
					client->ps.stats[STAT_WEAPONS] |= 1 << WP_ROCKET_LAUNCHER;
					client->skillLevel[SK_ROCKET] = FORCE_LEVEL_3;
					client->ps.ammo[AMMO_ROCKETS] = 5;
					break;
				case BCLASS_SMUGGLER2:
					client->ps.stats[STAT_WEAPONS] |= 1 << WP_MELEE;
					client->ps.stats[STAT_ARMOR] = 100;
					client->ps.stats[STAT_WEAPONS] |= 1 << WP_BLASTER;
					client->skillLevel[SK_BLASTER] = FORCE_LEVEL_3;
					client->ps.ammo[AMMO_BLASTER] = 900;
					client->ps.ammo[AMMO_POWERCELL] = 500;
					client->ps.ammo[AMMO_METAL_BOLTS] = 900;
					client->ps.stats[STAT_WEAPONS] |= 1 << WP_THERMAL;
					client->skillLevel[SK_THERMAL] = FORCE_LEVEL_3;
					client->skillLevel[SK_CRYOBAN] = FORCE_LEVEL_3;
					client->ps.ammo[AMMO_THERMAL] = 4;
					client->ps.ammo[AMMO_TRIPMINE] = 3;
					break;
				case BCLASS_BOUNTYHUNTER1:
					client->ps.stats[STAT_WEAPONS] |= 1 << WP_MELEE;
					client->ps.stats[STAT_WEAPONS] |= 1 << WP_FLECHETTE;
					client->skillLevel[SK_FLECHETTE] = FORCE_LEVEL_3;
					client->ps.ammo[AMMO_BLASTER] = 900;
					client->ps.ammo[AMMO_POWERCELL] = 500;
					client->ps.ammo[AMMO_METAL_BOLTS] = 900;
					client->ps.stats[STAT_WEAPONS] |= 1 << WP_REPEATER;
					client->skillLevel[SK_REPEATER] = FORCE_LEVEL_3;
					client->ps.stats[STAT_WEAPONS] |= 1 << WP_THERMAL;
					client->ps.ammo[AMMO_THERMAL] = 4;
					client->ps.stats[STAT_WEAPONS] |= 1 << WP_DET_PACK;
					client->skillLevel[SK_DETPACK] = FORCE_LEVEL_3;
					client->ps.ammo[AMMO_DETPACK] = 3;
					break;
				case BCLASS_BOUNTYHUNTER2:
					client->ps.stats[STAT_WEAPONS] |= 1 << WP_MELEE;
					client->ps.stats[STAT_WEAPONS] |= 1 << WP_BRYAR_PISTOL;
					client->skillLevel[SK_PISTOL] = FORCE_LEVEL_3;
					client->ps.eFlags |= EF_DUAL_WEAPONS;
					client->ps.ammo[AMMO_BLASTER] = 900;
					client->ps.ammo[AMMO_POWERCELL] = 500;
					client->ps.ammo[AMMO_METAL_BOLTS] = 900;
					client->ps.stats[STAT_WEAPONS] |= 1 << WP_DET_PACK;
					client->skillLevel[SK_DETPACK] = FORCE_LEVEL_3;
					client->ps.ammo[AMMO_DETPACK] = 3;
					break;
				case BCLASS_BOUNTYHUNTER3:
					client->ps.stats[STAT_WEAPONS] |= 1 << WP_MELEE;
					client->ps.stats[STAT_WEAPONS] |= 1 << WP_BLASTER;
					client->skillLevel[SK_BLASTER] = FORCE_LEVEL_3;
					client->ps.ammo[AMMO_BLASTER] = 900;
					client->ps.ammo[AMMO_POWERCELL] = 500;
					client->ps.ammo[AMMO_METAL_BOLTS] = 900;
					client->ps.stats[STAT_WEAPONS] |= 1 << WP_REPEATER;
					client->skillLevel[SK_REPEATER] = FORCE_LEVEL_3;
					break;
				case BCLASS_IPPERIALAGENT1:
					client->ps.stats[STAT_WEAPONS] |= 1 << WP_MELEE;
					client->ps.stats[STAT_WEAPONS] |= 1 << WP_FLECHETTE;
					client->skillLevel[SK_FLECHETTE] = FORCE_LEVEL_3;
					client->ps.ammo[AMMO_METAL_BOLTS] = 675;
					client->ps.stats[STAT_WEAPONS] |= 1 << WP_BLASTER;
					client->skillLevel[SK_BLASTER] = FORCE_LEVEL_3;
					client->ps.ammo[AMMO_BLASTER] = 900;
					client->ps.ammo[AMMO_POWERCELL] = 500;
					client->ps.ammo[AMMO_METAL_BOLTS] = 900;
					client->ps.stats[STAT_WEAPONS] |= 1 << WP_REPEATER;
					client->skillLevel[SK_REPEATER] = FORCE_LEVEL_3;
					break;
				case BCLASS_IPPERIALAGENT2:
					client->ps.stats[STAT_WEAPONS] |= 1 << WP_MELEE;
					client->ps.stats[STAT_WEAPONS] |= 1 << WP_BLASTER;
					client->skillLevel[SK_BLASTER] = FORCE_LEVEL_3;
					client->ps.ammo[AMMO_BLASTER] = 900;
					client->ps.ammo[AMMO_POWERCELL] = 500;
					client->ps.ammo[AMMO_METAL_BOLTS] = 900;
					client->ps.stats[STAT_WEAPONS] |= 1 << WP_FLECHETTE;
					client->skillLevel[SK_FLECHETTE] = FORCE_LEVEL_3;
					client->ps.stats[STAT_WEAPONS] |= 1 << WP_THERMAL;
					client->skillLevel[SK_THERMAL] = FORCE_LEVEL_3;
					client->skillLevel[SK_CRYOBAN] = FORCE_LEVEL_3;
					client->ps.ammo[AMMO_THERMAL] = 4;
					break;
				case BCLASS_IPPERIALAGENT3:
					client->ps.stats[STAT_WEAPONS] |= 1 << WP_MELEE;
					client->skillLevel[SK_ACROBATICS] = FORCE_LEVEL_3;
					client->ps.stats[STAT_WEAPONS] |= 1 << WP_BRYAR_PISTOL;
					client->skillLevel[SK_PISTOL] = FORCE_LEVEL_3;
					client->ps.eFlags |= EF_DUAL_WEAPONS;
					client->ps.ammo[AMMO_BLASTER] = 900;
					client->ps.ammo[AMMO_POWERCELL] = 500;
					client->ps.ammo[AMMO_METAL_BOLTS] = 900;
					client->ps.ammo[AMMO_TRIPMINE] = 3;
					client->ps.stats[STAT_WEAPONS] |= 1 << WP_THERMAL;
					client->skillLevel[SK_THERMAL] = FORCE_LEVEL_3;
					client->skillLevel[SK_CRYOBAN] = FORCE_LEVEL_3;
					client->ps.ammo[AMMO_THERMAL] = 4;
					break;
				case BCLASS_PLAYER:
					client->ps.stats[STAT_WEAPONS] |= 1 << WP_BLASTER;
					client->skillLevel[SK_BLASTER] = FORCE_LEVEL_3;
					client->ps.ammo[AMMO_BLASTER] = 900;
					client->ps.ammo[AMMO_POWERCELL] = 500;
					client->ps.ammo[AMMO_METAL_BOLTS] = 900;
					break;
				default:
					if (g_gametype.integer != GT_SIEGE)
					{
						if (g_gametype.integer == GT_JEDIMASTER)
						{
							if (!wDisable || !(wDisable & 1 << WP_BRYAR_PISTOL))
							{
								client->ps.stats[STAT_WEAPONS] |= 1 << WP_BRYAR_PISTOL;
							}
						}
						if (client->ps.fd.forcePowerSelected == FP_SABER_OFFENSE)
						{
							client->ps.stats[STAT_WEAPONS] |= 1 << WP_SABER;
						}
						else
						{
							if (!wDisable || !(wDisable & 1 << WP_BRYAR_PISTOL))
							{
								client->ps.stats[STAT_WEAPONS] |= 1 << WP_BRYAR_PISTOL;
							}
						}
					}
					client->ps.ammo[AMMO_BLASTER] = 500;
					client->ps.ammo[AMMO_POWERCELL] = 500;
					client->ps.ammo[AMMO_METAL_BOLTS] = 500;
					client->ps.stats[STAT_WEAPONS] |= 1 << WP_MELEE;
					client->skillLevel[SK_BLASTER] = FORCE_LEVEL_3;
					client->skillLevel[SK_THERMAL] = FORCE_LEVEL_3;
					client->skillLevel[SK_BOWCASTER] = FORCE_LEVEL_3;
					client->skillLevel[SK_DETPACK] = FORCE_LEVEL_3;
					client->skillLevel[SK_REPEATER] = FORCE_LEVEL_3;
					client->skillLevel[SK_DISRUPTOR] = FORCE_LEVEL_3;
					client->skillLevel[SK_CRYOBAN] = FORCE_LEVEL_3;
					if (client->skillLevel[SK_PISTOL] >= FORCE_LEVEL_3 && ent->client->ps.weapon == WP_BRYAR_PISTOL)
					{
						client->ps.eFlags |= EF_DUAL_WEAPONS;
					}
					break;
				}
			}
			else
			{
				if (g_gametype.integer != GT_SIEGE)
				{
					if (g_gametype.integer == GT_JEDIMASTER)
					{
						if (!wDisable || !(wDisable & 1 << WP_BRYAR_PISTOL))
						{
							client->ps.stats[STAT_WEAPONS] |= 1 << WP_BRYAR_PISTOL;
						}
					}
					if (client->ps.fd.forcePowerSelected == FP_SABER_OFFENSE)
					{
						client->ps.stats[STAT_WEAPONS] |= 1 << WP_SABER;
					}
					else
					{
						if (!wDisable || !(wDisable & 1 << WP_BRYAR_PISTOL))
						{
							client->ps.stats[STAT_WEAPONS] |= 1 << WP_BRYAR_PISTOL;
						}
					}
				}
				client->ps.ammo[AMMO_BLASTER] = 500;
				client->ps.ammo[AMMO_POWERCELL] = 500;
				client->ps.ammo[AMMO_METAL_BOLTS] = 500;
				client->ps.stats[STAT_WEAPONS] |= 1 << WP_MELEE;
				client->skillLevel[SK_BLASTER] = FORCE_LEVEL_3;
				client->skillLevel[SK_THERMAL] = FORCE_LEVEL_3;
				client->skillLevel[SK_BOWCASTER] = FORCE_LEVEL_3;
				client->skillLevel[SK_DETPACK] = FORCE_LEVEL_3;
				client->skillLevel[SK_REPEATER] = FORCE_LEVEL_3;
				client->skillLevel[SK_DISRUPTOR] = FORCE_LEVEL_3;
				client->skillLevel[SK_CRYOBAN] = FORCE_LEVEL_3;
				if (client->skillLevel[SK_PISTOL] >= FORCE_LEVEL_3 && ent->client->ps.weapon == WP_BRYAR_PISTOL)
				{
					client->ps.eFlags |= EF_DUAL_WEAPONS;
				}
			}
		}
		if (wDisable == WP_ALLDISABLED)
		{
			//some joker disabled all the weapons.  Give everyone Melee
			client->ps.stats[STAT_WEAPONS] |= 1 << WP_MELEE;
			Com_Printf("ERROR:  The game doesn't like it when you disable ALL the weapons.\nReenabling Melee.\n");
			if (g_gametype.integer == GT_DUEL || g_gametype.integer == GT_POWERDUEL)
			{
				trap->Cvar_Set("g_duelWeaponDisable", va("%i", WP_MELEEONLY));
			}
			else
			{
				trap->Cvar_Set("g_weaponDisable", va("%i", WP_MELEEONLY));
			}
		}

		if (level.gametype == GT_JEDIMASTER)
		{
			client->ps.stats[STAT_WEAPONS] &= ~(1 << WP_SABER);

			if (!wDisable || !(wDisable & 1 << WP_BRYAR_PISTOL))
			{
				client->ps.stats[STAT_WEAPONS] |= 1 << WP_MELEE;
			}
		}

		if (client->ps.stats[STAT_WEAPONS] & 1 << WP_SABER)
		{
			//sabers auto selected if we have them.
			client->ps.weapon = WP_SABER;
		}
		else
		{
			int i1;
			for (i1 = LAST_USEABLE_WEAPON; i1 > WP_NONE; i1--)
			{
				if (client->ps.stats[STAT_WEAPONS] & 1 << i1)
				{
					//have this one, equip it.
					client->ps.weapon = i1;
					break;
				}
			}
		}
	}

	if (level.gametype == GT_SIEGE && client->siegeClass != -1 &&
		client->sess.sessionTeam != TEAM_SPECTATOR)
	{
		//well then, we will use a custom weaponset for our class
		int m = 0;

		client->ps.stats[STAT_WEAPONS] = bgSiegeClasses[client->siegeClass].weapons;

		if (client->ps.stats[STAT_WEAPONS] & 1 << WP_SABER)
		{
			client->ps.weapon = WP_SABER;
		}
		else if (client->ps.stats[STAT_WEAPONS] & 1 << WP_BRYAR_PISTOL)
		{
			client->ps.weapon = WP_BRYAR_PISTOL;
		}
		else
		{
			client->ps.weapon = WP_MELEE;
		}
		inSiegeWithClass = qtrue;

		while (m < WP_NUM_WEAPONS)
		{
			if (client->ps.stats[STAT_WEAPONS] & 1 << m)
			{
				if (client->ps.weapon != WP_SABER)
				{
					//try to find the highest ranking weapon we have
					if (m > client->ps.weapon)
					{
						client->ps.weapon = m;
					}
				}

				if (m >= WP_BRYAR_PISTOL)
				{
					//Max his ammo out for all the weapons he has.
					if (level.gametype == GT_SIEGE
						&& m == WP_ROCKET_LAUNCHER)
					{
						//don't give full ammo!
						//FIXME: extern this and check it when getting ammo from supplier, pickups or ammo stations!
						if (client->siegeClass != -1 &&
							bgSiegeClasses[client->siegeClass].classflags & 1 << CFL_SINGLE_ROCKET)
						{
							client->ps.ammo[weaponData[m].ammoIndex] = 1;
						}
						else
						{
							client->ps.ammo[weaponData[m].ammoIndex] = 10;
						}
					}
					else
					{
						if (level.gametype == GT_SIEGE
							&& client->siegeClass != -1
							&& bgSiegeClasses[client->siegeClass].classflags & 1 << CFL_EXTRA_AMMO)
						{
							//double ammo
							client->ps.ammo[weaponData[m].ammoIndex] = ammoData[weaponData[m].ammoIndex].max * 2;
							client->ps.eFlags |= EF_DOUBLE_AMMO;
						}
						else
						{
							client->ps.ammo[weaponData[m].ammoIndex] = ammoData[weaponData[m].ammoIndex].max;
						}
					}
				}
			}
			m++;
		}
	}

	if (level.gametype == GT_SIEGE &&
		client->siegeClass != -1 &&
		client->sess.sessionTeam != TEAM_SPECTATOR)
	{
		//use class-specified inventory
		client->ps.stats[STAT_HOLDABLE_ITEMS] = bgSiegeClasses[client->siegeClass].invenItems;
		client->ps.stats[STAT_HOLDABLE_ITEM] = 0;
	}
	else
	{
		//New class system
		switch (client->pers.botclass)
		{
			//botclass holdable stuff armour and health here
		case BCLASS_ALORA:
			client->ps.stats[STAT_ARMOR] = 200;
			client->ps.stats[STAT_MAX_HEALTH] = 100;
			client->ps.stats[STAT_HOLDABLE_ITEMS] |= 1 << HI_MEDPAC;
			break;
		case BCLASS_ASSASSIN_DROID:
			client->ps.stats[STAT_ARMOR] = 500;
			client->ps.stats[STAT_MAX_HEALTH] = 100;
			break;
		case BCLASS_BARTENDER:
			client->ps.stats[STAT_ARMOR] = 100;
			client->ps.stats[STAT_MAX_HEALTH] = 100;
			break;
		case BCLASS_BESPIN_COP:
			client->ps.stats[STAT_HOLDABLE_ITEMS] |= 1 << HI_SHIELD;
			client->ps.stats[STAT_ARMOR] = 100;
			client->ps.stats[STAT_MAX_HEALTH] = 100;
			client->ps.stats[STAT_HOLDABLE_ITEMS] |= 1 << HI_MEDPAC;
			break;
		case BCLASS_BOBAFETT:
			client->ps.stats[STAT_ARMOR] = 300;
			client->ps.stats[STAT_MAX_HEALTH] = 150;
			client->ps.stats[STAT_HOLDABLE_ITEMS] |= 1 << HI_JETPACK;
			client->ps.stats[STAT_HOLDABLE_ITEMS] |= 1 << HI_FLAMETHROWER;
			client->ps.stats[STAT_HOLDABLE_ITEMS] |= 1 << HI_MEDPAC;
			client->ps.stats[STAT_HOLDABLE_ITEMS] |= 1 << HI_SEEKER;
			client->ps.stats[STAT_HOLDABLE_ITEMS] |= 1 << HI_GRAPPLE;
			break;
		case BCLASS_CHEWIE:
			client->ps.stats[STAT_HOLDABLE_ITEMS] |= 1 << HI_SENTRY_GUN;
			client->ps.stats[STAT_ARMOR] = 300;
			client->ps.stats[STAT_MAX_HEALTH] = 150;
			client->ps.fd.forcePowerLevel[FP_RAGE] = FORCE_LEVEL_3; //chewie gets rage
			client->ps.stats[STAT_HOLDABLE_ITEMS] |= 1 << HI_MEDPAC;
			break;
		case BCLASS_CULTIST:
			client->ps.stats[STAT_ARMOR] = 100;
			client->ps.stats[STAT_MAX_HEALTH] = 100;
			client->ps.stats[STAT_HOLDABLE_ITEMS] |= 1 << HI_MEDPAC;
			break;
		case BCLASS_DESANN:
			client->ps.stats[STAT_ARMOR] = 300;
			client->ps.stats[STAT_MAX_HEALTH] = 200;
			client->ps.stats[STAT_HOLDABLE_ITEMS] |= 1 << HI_MEDPAC;
			break;
		case BCLASS_UNSTABLESABER:
			client->ps.stats[STAT_ARMOR] = 300;
			client->ps.stats[STAT_MAX_HEALTH] = 200;
			client->ps.stats[STAT_HOLDABLE_ITEMS] |= 1 << HI_MEDPAC;
			break;
		case BCLASS_ELDER:
			client->ps.stats[STAT_HOLDABLE_ITEMS] |= 1 << HI_SENTRY_GUN;
			client->ps.stats[STAT_HOLDABLE_ITEMS] |= 1 << HI_EWEB;
			client->ps.stats[STAT_HOLDABLE_ITEMS] |= 1 << HI_DROIDEKA;
			client->ps.stats[STAT_ARMOR] = 100;
			client->ps.stats[STAT_MAX_HEALTH] = 100;
			client->ps.stats[STAT_HOLDABLE_ITEMS] |= 1 << HI_MEDPAC;
			break;
		case BCLASS_GALAK:
			client->ps.stats[STAT_HOLDABLE_ITEMS] |= 1 << HI_EWEB;
			client->ps.stats[STAT_ARMOR] = 100;
			client->ps.stats[STAT_MAX_HEALTH] = 100;
			client->ps.stats[STAT_HOLDABLE_ITEMS] |= 1 << HI_MEDPAC;
			break;
		case BCLASS_GRAN:
			client->ps.stats[STAT_ARMOR] = 300;
			client->ps.stats[STAT_MAX_HEALTH] = 150;
			client->ps.stats[STAT_HOLDABLE_ITEMS] |= 1 << HI_MEDPAC;
			break;
		case BCLASS_HAZARDTROOPER:
			client->ps.stats[STAT_ARMOR] = 300;
			client->ps.stats[STAT_HOLDABLE_ITEMS] |= 1 << HI_JETPACK;
			client->ps.stats[STAT_HOLDABLE_ITEMS] |= 1 << HI_FLAMETHROWER;
			client->ps.stats[STAT_HOLDABLE_ITEMS] |= 1 << HI_SENTRY_GUN;
			client->ps.stats[STAT_MAX_HEALTH] = 100;
			client->ps.stats[STAT_HOLDABLE_ITEMS] |= 1 << HI_MEDPAC;
			break;
		case BCLASS_HUMAN_MERC:
			client->ps.stats[STAT_ARMOR] = 100;
			client->ps.stats[STAT_MAX_HEALTH] = 100;
			client->ps.stats[STAT_HOLDABLE_ITEMS] |= 1 << HI_MEDPAC;
			break;
		case BCLASS_IMPERIAL:
			client->ps.stats[STAT_HOLDABLE_ITEMS] |= 1 << HI_SENTRY_GUN;
			client->ps.stats[STAT_ARMOR] = 100;
			client->ps.stats[STAT_MAX_HEALTH] = 100;
			client->ps.stats[STAT_HOLDABLE_ITEMS] |= 1 << HI_MEDPAC;
			break;
		case BCLASS_IMPWORKER:
			client->ps.stats[STAT_ARMOR] = 100;
			client->ps.stats[STAT_MAX_HEALTH] = 100;
			client->ps.stats[STAT_HOLDABLE_ITEMS] |= 1 << HI_MEDPAC;
			break;
		case BCLASS_JAN:
			client->ps.stats[STAT_HOLDABLE_ITEMS] |= 1 << HI_SENTRY_GUN;
			client->ps.stats[STAT_HOLDABLE_ITEMS] |= 1 << HI_EWEB;
			client->ps.stats[STAT_ARMOR] = 200;
			client->ps.stats[STAT_MAX_HEALTH] = 200;
			client->ps.stats[STAT_HOLDABLE_ITEMS] |= 1 << HI_MEDPAC;
			break;
		case BCLASS_JAWA:
			client->ps.stats[STAT_ARMOR] = 100;
			client->ps.stats[STAT_MAX_HEALTH] = 100;
			client->ps.stats[STAT_HOLDABLE_ITEMS] |= 1 << HI_MEDPAC;
			break;
		case BCLASS_JEDI:
			client->ps.stats[STAT_ARMOR] = 100;
			client->ps.stats[STAT_MAX_HEALTH] = 100;
			client->ps.stats[STAT_HOLDABLE_ITEMS] |= 1 << HI_MEDPAC;
			break;
		case BCLASS_JEDIMASTER:
			client->ps.stats[STAT_ARMOR] = 125;
			client->ps.stats[STAT_MAX_HEALTH] = 125;
			client->ps.stats[STAT_HOLDABLE_ITEMS] |= 1 << HI_MEDPAC;
			break;
		case BCLASS_JEDITRAINER:
			client->ps.stats[STAT_ARMOR] = 100;
			client->ps.stats[STAT_MAX_HEALTH] = 100;
			client->ps.stats[STAT_HOLDABLE_ITEMS] |= 1 << HI_MEDPAC;
			break;
		case BCLASS_KYLE:
			client->ps.stats[STAT_ARMOR] = 100;
			client->ps.stats[STAT_MAX_HEALTH] = 100;
			client->ps.stats[STAT_HOLDABLE_ITEMS] |= 1 << HI_MEDPAC;
			break;
		case BCLASS_LANDO:
			client->ps.stats[STAT_HOLDABLE_ITEMS] |= 1 << HI_EWEB;
			client->ps.stats[STAT_ARMOR] = 100;
			client->ps.stats[STAT_MAX_HEALTH] = 100;
			break;
		case BCLASS_LUKE:
			client->ps.stats[STAT_ARMOR] = 200;
			client->ps.stats[STAT_MAX_HEALTH] = 200;
			client->ps.stats[STAT_HOLDABLE_ITEMS] |= 1 << HI_MEDPAC;
			break;
		case BCLASS_DUELS:
			client->ps.stats[STAT_ARMOR] = 100;
			client->ps.stats[STAT_MAX_HEALTH] = 100;
			client->ps.stats[STAT_HOLDABLE_ITEMS] |= 1 << HI_MEDPAC;
			break;
		case BCLASS_GALAKMECH:
			client->ps.stats[STAT_HOLDABLE_ITEMS] |= 1 << HI_EWEB;
			client->ps.stats[STAT_ARMOR] = 100;
			client->ps.stats[STAT_MAX_HEALTH] = 100;
			client->ps.stats[STAT_HOLDABLE_ITEMS] |= 1 << HI_MEDPAC;
			break;
		case BCLASS_MONMOTHA:
			client->ps.stats[STAT_HOLDABLE_ITEMS] |= 1 << HI_EWEB;
			client->ps.stats[STAT_ARMOR] = 100;
			client->ps.stats[STAT_MAX_HEALTH] = 100;
			client->ps.stats[STAT_HOLDABLE_ITEMS] |= 1 << HI_MEDPAC;
			break;
		case BCLASS_MORGANKATARN:
			client->ps.stats[STAT_ARMOR] = 100;
			client->ps.stats[STAT_MAX_HEALTH] = 100;
			client->ps.stats[STAT_HOLDABLE_ITEMS] |= 1 << HI_MEDPAC;
			break;
		case BCLASS_NOGRHRI:
			client->ps.stats[STAT_HOLDABLE_ITEMS] |= 1 << HI_EWEB;
			client->ps.stats[STAT_ARMOR] = 200;
			client->ps.stats[STAT_MAX_HEALTH] = 200;
			client->ps.stats[STAT_HOLDABLE_ITEMS] |= 1 << HI_MEDPAC;
			break;
		case BCLASS_RAX:
			client->ps.stats[STAT_HOLDABLE_ITEMS] |= 1 << HI_EWEB;
			client->ps.stats[STAT_ARMOR] = 100;
			client->ps.stats[STAT_MAX_HEALTH] = 100;
			client->ps.stats[STAT_HOLDABLE_ITEMS] |= 1 << HI_MEDPAC;
			break;
		case BCLASS_REBEL:
			client->ps.stats[STAT_HOLDABLE_ITEMS] |= 1 << HI_EWEB;
			client->ps.stats[STAT_ARMOR] = 100;
			client->ps.stats[STAT_MAX_HEALTH] = 100;
			client->ps.stats[STAT_HOLDABLE_ITEMS] |= 1 << HI_MEDPAC;
			break;
		case BCLASS_REBORN:
			client->ps.stats[STAT_ARMOR] = 100;
			client->ps.stats[STAT_MAX_HEALTH] = 100;
			client->ps.stats[STAT_HOLDABLE_ITEMS] |= 1 << HI_MEDPAC;
			break;
		case BCLASS_REBORN_TWIN:
			client->ps.stats[STAT_ARMOR] = 100;
			client->ps.stats[STAT_MAX_HEALTH] = 100;
			client->ps.stats[STAT_HOLDABLE_ITEMS] |= 1 << HI_MEDPAC;
			break;
		case BCLASS_REBORN_MASTER:
			client->ps.stats[STAT_ARMOR] = 100;
			client->ps.stats[STAT_MAX_HEALTH] = 100;
			client->ps.stats[STAT_HOLDABLE_ITEMS] |= 1 << HI_MEDPAC;
			break;
		case BCLASS_REELO:
			client->ps.stats[STAT_HOLDABLE_ITEMS] |= 1 << HI_EWEB;
			client->ps.stats[STAT_ARMOR] = 100;
			client->ps.stats[STAT_MAX_HEALTH] = 100;
			client->ps.stats[STAT_HOLDABLE_ITEMS] |= 1 << HI_MEDPAC;
			break;
		case BCLASS_ROCKETTROOPER:
			client->ps.stats[STAT_ARMOR] = 300;
			client->ps.stats[STAT_HOLDABLE_ITEMS] |= 1 << HI_JETPACK;
			client->ps.stats[STAT_HOLDABLE_ITEMS] |= 1 << HI_SEEKER;
			client->ps.stats[STAT_HOLDABLE_ITEMS] |= 1 << HI_EWEB;
			client->ps.stats[STAT_MAX_HEALTH] = 100;
			client->ps.stats[STAT_HOLDABLE_ITEMS] |= 1 << HI_MEDPAC;
			break;
		case BCLASS_RODIAN:
			client->ps.stats[STAT_HOLDABLE_ITEMS] |= 1 << HI_EWEB;
			client->ps.stats[STAT_ARMOR] = 100;
			client->ps.stats[STAT_MAX_HEALTH] = 100;
			client->ps.stats[STAT_HOLDABLE_ITEMS] |= 1 << HI_MEDPAC;
			client->ps.stats[STAT_HOLDABLE_ITEMS] |= 1 << HI_SHIELD;
			break;
		case BCLASS_ROSH_PENIN:
			client->ps.stats[STAT_ARMOR] = 150;
			client->ps.stats[STAT_MAX_HEALTH] = 150;
			client->ps.stats[STAT_HOLDABLE_ITEMS] |= 1 << HI_MEDPAC;
			break;
		case BCLASS_SABER_DROID:
			client->ps.stats[STAT_ARMOR] = 100;
			client->ps.stats[STAT_MAX_HEALTH] = 100;
			client->ps.stats[STAT_HOLDABLE_ITEMS] |= 1 << HI_MEDPAC;
			break;
		case BCLASS_SABOTEUR:
			client->ps.stats[STAT_HOLDABLE_ITEMS] |= 1 << HI_EWEB;
			client->ps.stats[STAT_ARMOR] = 100;
			client->ps.stats[STAT_MAX_HEALTH] = 100;
			client->ps.stats[STAT_HOLDABLE_ITEMS] |= 1 << HI_MEDPAC;
			break;
		case BCLASS_SHADOWTROOPER:
			client->ps.stats[STAT_ARMOR] = 175;
			client->ps.stats[STAT_MAX_HEALTH] = 175;
			client->ps.stats[STAT_HOLDABLE_ITEMS] |= 1 << HI_CLOAK;
			client->ps.stats[STAT_HOLDABLE_ITEMS] |= 1 << HI_MEDPAC;
			break;
		case BCLASS_STORMTROOPER:
			client->ps.stats[STAT_HOLDABLE_ITEMS] |= 1 << HI_SEEKER;
			client->ps.stats[STAT_HOLDABLE_ITEMS] |= 1 << HI_EWEB;
			client->ps.stats[STAT_HOLDABLE_ITEMS] |= 1 << HI_SHIELD;
			client->ps.stats[STAT_ARMOR] = 100;
			client->ps.stats[STAT_MAX_HEALTH] = 100;
			client->ps.stats[STAT_HOLDABLE_ITEMS] |= 1 << HI_MEDPAC;
			break;
		case BCLASS_STORMPILOT:
			client->ps.stats[STAT_HOLDABLE_ITEMS] |= 1 << HI_SEEKER;
			client->ps.stats[STAT_HOLDABLE_ITEMS] |= 1 << HI_EWEB;
			client->ps.stats[STAT_HOLDABLE_ITEMS] |= 1 << HI_SHIELD;
			client->ps.stats[STAT_ARMOR] = 100;
			client->ps.stats[STAT_MAX_HEALTH] = 100;
			client->ps.stats[STAT_HOLDABLE_ITEMS] |= 1 << HI_MEDPAC;
			break;
		case BCLASS_SWAMPTROOPER:
			client->ps.stats[STAT_HOLDABLE_ITEMS] |= 1 << HI_EWEB;
			client->ps.stats[STAT_ARMOR] = 100;
			client->ps.stats[STAT_MAX_HEALTH] = 100;
			client->ps.stats[STAT_HOLDABLE_ITEMS] |= 1 << HI_MEDPAC;
			break;
		case BCLASS_TAVION:
			client->ps.stats[STAT_ARMOR] = 250;
			client->ps.stats[STAT_MAX_HEALTH] = 250;
			client->ps.stats[STAT_HOLDABLE_ITEMS] |= 1 << HI_MEDPAC;
			break;
		case BCLASS_TRANDOSHAN:
			client->ps.stats[STAT_HOLDABLE_ITEMS] |= 1 << HI_EWEB;
			client->ps.stats[STAT_ARMOR] = 100;
			client->ps.stats[STAT_MAX_HEALTH] = 100;
			client->ps.stats[STAT_HOLDABLE_ITEMS] |= 1 << HI_MEDPAC;
			break;
		case BCLASS_TUSKEN_SNIPER:
			client->ps.stats[STAT_ARMOR] = 100;
			client->ps.stats[STAT_MAX_HEALTH] = 100;
			client->ps.stats[STAT_HOLDABLE_ITEMS] |= 1 << HI_MEDPAC;
			break;
		case BCLASS_TUSKEN_RAIDER:
			client->ps.stats[STAT_ARMOR] = 100;
			client->ps.stats[STAT_MAX_HEALTH] = 100;
			client->ps.stats[STAT_HOLDABLE_ITEMS] |= 1 << HI_MEDPAC;
			break;
		case BCLASS_UGNAUGHT:
			client->ps.stats[STAT_HOLDABLE_ITEMS] |= 1 << HI_EWEB;
			client->ps.stats[STAT_HOLDABLE_ITEMS] |= 1 << HI_SHIELD;
			client->ps.stats[STAT_ARMOR] = 100;
			client->ps.stats[STAT_MAX_HEALTH] = 100;
			client->ps.stats[STAT_HOLDABLE_ITEMS] |= 1 << HI_MEDPAC;
			break;
		case BCLASS_WEEQUAY:
			client->ps.stats[STAT_HOLDABLE_ITEMS] |= 1 << HI_EWEB;
			client->ps.stats[STAT_ARMOR] = 100;
			client->ps.stats[STAT_MAX_HEALTH] = 100;
			client->ps.stats[STAT_HOLDABLE_ITEMS] |= 1 << HI_MEDPAC;
			break;
		case BCLASS_SERENITY:
			client->ps.stats[STAT_ARMOR] = 500;
			client->ps.stats[STAT_MAX_HEALTH] = 500;
			client->ps.stats[STAT_HOLDABLE_ITEMS] |= 1 << HI_SEEKER;
			client->ps.stats[STAT_HOLDABLE_ITEMS] |= 1 << HI_SENTRY_GUN;
			client->ps.stats[STAT_HOLDABLE_ITEMS] |= 1 << HI_EWEB;
			client->ps.stats[STAT_HOLDABLE_ITEMS] |= 1 << HI_MEDPAC;
			client->ps.stats[STAT_HOLDABLE_ITEMS] |= 1 << HI_SHIELD;
			break;
		case BCLASS_CADENCE:
			client->ps.stats[STAT_ARMOR] = 500;
			client->ps.stats[STAT_MAX_HEALTH] = 500;
			client->ps.stats[STAT_HOLDABLE_ITEMS] |= 1 << HI_HEALTHDISP;
			client->ps.stats[STAT_HOLDABLE_ITEMS] |= 1 << HI_MEDPAC;
			client->ps.stats[STAT_HOLDABLE_ITEMS] |= 1 << HI_SHIELD;
			break;
		case BCLASS_YODA:
			client->ps.stats[STAT_ARMOR] = 500;
			client->ps.stats[STAT_MAX_HEALTH] = 250;
			client->ps.stats[STAT_HOLDABLE_ITEMS] |= 1 << HI_MEDPAC;
			break;
		case BCLASS_PADAWAN:
			client->ps.stats[STAT_ARMOR] = 100;
			client->ps.stats[STAT_MAX_HEALTH] = 100;
			client->ps.stats[STAT_HOLDABLE_ITEMS] |= 1 << HI_MEDPAC;
			break;
		case BCLASS_GRIEVOUS:
			client->ps.stats[STAT_ARMOR] = 200;
			client->ps.stats[STAT_MAX_HEALTH] = 200;
			client->ps.stats[STAT_HOLDABLE_ITEMS] |= 1 << HI_MEDPAC;
			break;
		case BCLASS_SITHLORD:
			client->ps.stats[STAT_ARMOR] = 200;
			client->ps.stats[STAT_MAX_HEALTH] = 200;
			client->ps.stats[STAT_HOLDABLE_ITEMS] |= 1 << HI_MEDPAC;
			break;
		case BCLASS_LORDVADER:
			client->ps.stats[STAT_ARMOR] = 500;
			client->ps.stats[STAT_MAX_HEALTH] = 250;
			client->ps.stats[STAT_HOLDABLE_ITEMS] |= 1 << HI_MEDPAC;
			break;
		case BCLASS_SITH:
			client->ps.stats[STAT_ARMOR] = 100;
			client->ps.stats[STAT_MAX_HEALTH] = 100;
			client->ps.stats[STAT_HOLDABLE_ITEMS] |= 1 << HI_MEDPAC;
			break;
		case BCLASS_STAFFDARK:
			client->ps.stats[STAT_ARMOR] = 100;
			client->ps.stats[STAT_MAX_HEALTH] = 100;
			client->ps.stats[STAT_HOLDABLE_ITEMS] |= 1 << HI_MEDPAC;
			break;
		case BCLASS_APPRENTICE:
			client->ps.stats[STAT_ARMOR] = 100;
			client->ps.stats[STAT_MAX_HEALTH] = 100;
			client->ps.stats[STAT_HOLDABLE_ITEMS] |= 1 << HI_MEDPAC;
			break;
		case BCLASS_MANDOLORIAN:
			client->ps.stats[STAT_ARMOR] = 300;
			client->ps.stats[STAT_MAX_HEALTH] = 100;
			client->ps.stats[STAT_HOLDABLE_ITEMS] |= 1 << HI_JETPACK;
			client->ps.stats[STAT_HOLDABLE_ITEMS] |= 1 << HI_FLAMETHROWER;
			client->ps.stats[STAT_HOLDABLE_ITEMS] |= 1 << HI_SEEKER;
			client->ps.stats[STAT_HOLDABLE_ITEMS] |= 1 << HI_EWEB;
			client->ps.stats[STAT_HOLDABLE_ITEMS] |= 1 << HI_MEDPAC;
			client->ps.stats[STAT_HOLDABLE_ITEMS] |= 1 << HI_GRAPPLE;
			break;
		case BCLASS_MANDOLORIAN1:
			client->ps.stats[STAT_ARMOR] = 300;
			client->ps.stats[STAT_MAX_HEALTH] = 100;
			client->ps.stats[STAT_HOLDABLE_ITEMS] |= 1 << HI_JETPACK;
			client->ps.stats[STAT_HOLDABLE_ITEMS] |= 1 << HI_FLAMETHROWER;
			client->ps.stats[STAT_HOLDABLE_ITEMS] |= 1 << HI_SEEKER;
			client->ps.stats[STAT_HOLDABLE_ITEMS] |= 1 << HI_EWEB;
			client->ps.stats[STAT_HOLDABLE_ITEMS] |= 1 << HI_MEDPAC;
			client->ps.stats[STAT_HOLDABLE_ITEMS] |= 1 << HI_GRAPPLE;
			break;
		case BCLASS_MANDOLORIAN2:
			client->ps.stats[STAT_ARMOR] = 300;
			client->ps.stats[STAT_MAX_HEALTH] = 150;
			client->ps.stats[STAT_HOLDABLE_ITEMS] |= 1 << HI_JETPACK;
			client->ps.stats[STAT_HOLDABLE_ITEMS] |= 1 << HI_FLAMETHROWER;
			client->ps.stats[STAT_HOLDABLE_ITEMS] |= 1 << HI_SENTRY_GUN;
			client->ps.stats[STAT_HOLDABLE_ITEMS] |= 1 << HI_EWEB;
			client->ps.stats[STAT_HOLDABLE_ITEMS] |= 1 << HI_MEDPAC;
			client->ps.stats[STAT_HOLDABLE_ITEMS] |= 1 << HI_GRAPPLE;
			break;
		case BCLASS_SOILDER:
			client->ps.stats[STAT_HOLDABLE_ITEMS] |= 1 << HI_EWEB;
			client->ps.stats[STAT_ARMOR] = 100;
			client->ps.stats[STAT_MAX_HEALTH] = 100;
			client->ps.stats[STAT_HOLDABLE_ITEMS] |= 1 << HI_MEDPAC;
			break;
		case BCLASS_BATTLEDROID:
			client->ps.stats[STAT_ARMOR] = 100;
			client->ps.stats[STAT_MAX_HEALTH] = 100;
			break;
		case BCLASS_SBD:
			client->ps.stats[STAT_ARMOR] = 500;
			client->ps.stats[STAT_MAX_HEALTH] = 500;
			break;
		case BCLASS_WOOKIE:
			client->ps.stats[STAT_ARMOR] = 500;
			client->ps.stats[STAT_MAX_HEALTH] = 250;
			client->ps.stats[STAT_HOLDABLE_ITEMS] |= 1 << HI_MEDPAC;
			break;
		case BCLASS_WOOKIEMELEE:
			client->ps.stats[STAT_ARMOR] = 500;
			client->ps.stats[STAT_MAX_HEALTH] = 250;
			client->ps.stats[STAT_HOLDABLE_ITEMS] |= 1 << HI_MEDPAC;
			break;
		case BCLASS_TROOPER1:
			client->ps.stats[STAT_HOLDABLE_ITEMS] |= 1 << HI_EWEB;
			client->ps.stats[STAT_ARMOR] = 100;
			client->ps.stats[STAT_MAX_HEALTH] = 100;
			break;
		case BCLASS_TROOPER2:
			client->ps.stats[STAT_HOLDABLE_ITEMS] |= 1 << HI_EWEB;
			client->ps.stats[STAT_HOLDABLE_ITEMS] |= 1 << HI_SEEKER;
			client->ps.stats[STAT_ARMOR] = 100;
			client->ps.stats[STAT_MAX_HEALTH] = 100;
			client->ps.stats[STAT_HOLDABLE_ITEMS] |= 1 << HI_MEDPAC;
			break;
		case BCLASS_TROOPER3:
			client->ps.stats[STAT_HOLDABLE_ITEMS] |= 1 << HI_JETPACK;
			client->ps.stats[STAT_HOLDABLE_ITEMS] |= 1 << HI_EWEB;
			client->ps.stats[STAT_ARMOR] = 100;
			client->ps.stats[STAT_MAX_HEALTH] = 100;
			client->ps.stats[STAT_HOLDABLE_ITEMS] |= 1 << HI_MEDPAC;
			break;
		case BCLASS_JEDIKNIGHT1:
			client->ps.stats[STAT_ARMOR] = 100;
			client->ps.stats[STAT_MAX_HEALTH] = 100;
			client->ps.stats[STAT_HOLDABLE_ITEMS] |= 1 << HI_MEDPAC;
			break;
		case BCLASS_JEDIKNIGHT2:
			client->ps.stats[STAT_ARMOR] = 100;
			client->ps.stats[STAT_MAX_HEALTH] = 100;
			client->ps.stats[STAT_HOLDABLE_ITEMS] |= 1 << HI_MEDPAC;
			break;
		case BCLASS_JEDIKNIGHT3:
			client->ps.stats[STAT_ARMOR] = 100;
			client->ps.stats[STAT_MAX_HEALTH] = 100;
			client->ps.stats[STAT_HOLDABLE_ITEMS] |= 1 << HI_MEDPAC;
			break;
		case BCLASS_SMUGGLER1:
			client->ps.stats[STAT_MAX_HEALTH] = 200;
			client->ps.stats[STAT_ARMOR] = 100;
			client->ps.stats[STAT_HOLDABLE_ITEMS] |= 1 << HI_MEDPAC;
			break;
		case BCLASS_SMUGGLER2:
			client->ps.stats[STAT_HOLDABLE_ITEMS] |= 1 << HI_SEEKER;
			client->ps.stats[STAT_HOLDABLE_ITEMS] |= 1 << HI_EWEB;
			client->ps.stats[STAT_ARMOR] = 100;
			client->ps.stats[STAT_MAX_HEALTH] = 100;
			client->ps.stats[STAT_HOLDABLE_ITEMS] |= 1 << HI_MEDPAC;
			break;
		case BCLASS_SMUGGLER3:
			client->ps.stats[STAT_ARMOR] = 100;
			client->ps.stats[STAT_HOLDABLE_ITEMS] |= 1 << HI_SEEKER;
			client->ps.stats[STAT_ARMOR] = 100;
			client->ps.stats[STAT_MAX_HEALTH] = 100;
			client->ps.stats[STAT_HOLDABLE_ITEMS] |= 1 << HI_MEDPAC;
			break;
		case BCLASS_JEDICONSULAR1:
			client->ps.stats[STAT_ARMOR] = 100;
			client->ps.stats[STAT_MAX_HEALTH] = 100;
			client->ps.stats[STAT_HOLDABLE_ITEMS] |= 1 << HI_MEDPAC;
			break;
		case BCLASS_JEDICONSULAR2:
			client->ps.stats[STAT_ARMOR] = 100;
			client->ps.stats[STAT_MAX_HEALTH] = 100;
			client->ps.stats[STAT_HOLDABLE_ITEMS] |= 1 << HI_MEDPAC;
			break;
		case BCLASS_JEDICONSULAR3:
			client->ps.stats[STAT_ARMOR] = 100;
			client->ps.stats[STAT_MAX_HEALTH] = 100;
			client->ps.stats[STAT_HOLDABLE_ITEMS] |= 1 << HI_MEDPAC;
			break;
		case BCLASS_BOUNTYHUNTER1:
			client->ps.stats[STAT_HOLDABLE_ITEMS] |= 1 << HI_JETPACK;
			break;
		case BCLASS_BOUNTYHUNTER2:
			client->ps.stats[STAT_HOLDABLE_ITEMS] |= 1 << HI_JETPACK;
			client->ps.stats[STAT_HOLDABLE_ITEMS] |= 1 << HI_SHIELD;
			client->ps.stats[STAT_HOLDABLE_ITEMS] |= 1 << HI_MEDPAC;
			break;
		case BCLASS_BOUNTYHUNTER3:
			client->ps.stats[STAT_HOLDABLE_ITEMS] |= 1 << HI_JETPACK;
			client->ps.stats[STAT_HOLDABLE_ITEMS] |= 1 << HI_SENTRY_GUN;
			client->ps.stats[STAT_HOLDABLE_ITEMS] |= 1 << HI_EWEB;
			client->ps.stats[STAT_HOLDABLE_ITEMS] |= 1 << HI_MEDPAC;
			break;
		case BCLASS_SITHWORRIOR1:
			client->ps.stats[STAT_MAX_HEALTH] = 100;
			client->ps.stats[STAT_ARMOR] = 200;
			client->ps.stats[STAT_HOLDABLE_ITEMS] |= 1 << HI_MEDPAC;
			break;
		case BCLASS_SITHWORRIOR2:
			client->ps.stats[STAT_ARMOR] = 100;
			client->ps.stats[STAT_MAX_HEALTH] = 100;
			client->ps.stats[STAT_HOLDABLE_ITEMS] |= 1 << HI_MEDPAC;
			break;
		case BCLASS_SITHWORRIOR3:
			client->ps.stats[STAT_ARMOR] = 100;
			client->ps.stats[STAT_MAX_HEALTH] = 100;
			client->ps.stats[STAT_HOLDABLE_ITEMS] |= 1 << HI_MEDPAC;
			break;
		case BCLASS_IPPERIALAGENT2:
			client->ps.stats[STAT_HOLDABLE_ITEMS] |= 1 << HI_SHIELD;
			client->ps.stats[STAT_HOLDABLE_ITEMS] |= 1 << HI_MEDPAC;
			break;
		case BCLASS_SITHINQUISITOR1:
			client->ps.stats[STAT_ARMOR] = 100;
			client->ps.stats[STAT_MAX_HEALTH] = 100;
			client->ps.stats[STAT_HOLDABLE_ITEMS] |= 1 << HI_MEDPAC;
			break;
		case BCLASS_SITHINQUISITOR2:
			client->ps.stats[STAT_ARMOR] = 100;
			client->ps.stats[STAT_MAX_HEALTH] = 100;
			client->ps.stats[STAT_HOLDABLE_ITEMS] |= 1 << HI_MEDPAC;
			break;
		case BCLASS_SITHINQUISITOR3:
			client->ps.stats[STAT_ARMOR] = 100;
			client->ps.stats[STAT_MAX_HEALTH] = 100;
			client->ps.stats[STAT_HOLDABLE_ITEMS] |= 1 << HI_MEDPAC;
			break;
		case BCLASS_PLAYER:
			client->ps.stats[STAT_ARMOR] = 100;
			client->ps.stats[STAT_MAX_HEALTH] = 100;
			client->ps.stats[STAT_HOLDABLE_ITEMS] |= 1 << HI_SHIELD;
			break;
		default:
			client->ps.stats[STAT_ARMOR] = 100;
			client->ps.stats[STAT_MAX_HEALTH] = 100;
			client->ps.stats[STAT_HOLDABLE_ITEMS] |= 1 << HI_SHIELD;
			break;
		}
	}

	if (level.gametype == GT_SIEGE &&
		client->siegeClass != -1 &&
		bgSiegeClasses[client->siegeClass].powerups &&
		client->sess.sessionTeam != TEAM_SPECTATOR)
	{
		//this class has some start powerups
		i = 0;
		while (i < PW_NUM_POWERUPS)
		{
			if (bgSiegeClasses[client->siegeClass].powerups & 1 << i)
			{
				client->ps.powerups[i] = Q3_INFINITE;
			}
			i++;
		}
	}
	else
	{
		//New class system
		switch (client->pers.botclass)
		{
			//botclass forcepowers and skills here
		case BCLASS_ALORA:
		case BCLASS_DESANN:
		case BCLASS_CULTIST:
		case BCLASS_JEDI:
		case BCLASS_KYLE:
		case BCLASS_JEDIMASTER:
		case BCLASS_JEDITRAINER:
		case BCLASS_DUELS:
		case BCLASS_LUKE:
		case BCLASS_MORGANKATARN:
		case BCLASS_REBORN_TWIN:
		case BCLASS_REBORN_MASTER:
		case BCLASS_REBORN:
		case BCLASS_ROSH_PENIN:
		case BCLASS_SABER_DROID:
		case BCLASS_TAVION:
		case BCLASS_SHADOWTROOPER:
		case BCLASS_SERENITY:
		case BCLASS_CADENCE:
		case BCLASS_YODA:
		case BCLASS_PADAWAN:
		case BCLASS_GRIEVOUS:
		case BCLASS_SITHLORD:
		case BCLASS_LORDVADER:
		case BCLASS_SITH:
		case BCLASS_APPRENTICE:
		case BCLASS_JEDIKNIGHT1:
		case BCLASS_JEDIKNIGHT2:
		case BCLASS_JEDIKNIGHT3:
		case BCLASS_SMUGGLER3:
		case BCLASS_JEDICONSULAR1:
		case BCLASS_JEDICONSULAR2:
		case BCLASS_JEDICONSULAR3:
		case BCLASS_SITHWORRIOR1:
		case BCLASS_SITHWORRIOR2:
		case BCLASS_SITHWORRIOR3:
		case BCLASS_SITHINQUISITOR1:
		case BCLASS_SITHINQUISITOR2:
		case BCLASS_SITHINQUISITOR3:
		case BCLASS_STAFFDARK:
		case BCLASS_STAFF:
		case BCLASS_UNSTABLESABER:
			//Read bot.txt or Player FP setup for saber users
			client->ps.fd.blockPoints = BLOCK_POINTS_MAX;
			break;
		case BCLASS_ASSASSIN_DROID:
		case BCLASS_BARTENDER:
		case BCLASS_BESPIN_COP:
		case BCLASS_ELDER:
		case BCLASS_GALAK:
		case BCLASS_GRAN:
		case BCLASS_HAZARDTROOPER:
		case BCLASS_HUMAN_MERC:
		case BCLASS_IMPERIAL:
		case BCLASS_IMPWORKER:
		case BCLASS_JAN:
		case BCLASS_JAWA:
		case BCLASS_LANDO:
		case BCLASS_GALAKMECH:
		case BCLASS_MONMOTHA:
		case BCLASS_NOGRHRI:
		case BCLASS_PROTOCOL:
		case BCLASS_R2D2:
		case BCLASS_R5D2:
		case BCLASS_RANCOR:
		case BCLASS_RAX:
		case BCLASS_REBEL:
		case BCLASS_REELO:
		case BCLASS_RODIAN:
		case BCLASS_SABOTEUR:
		case BCLASS_STORMTROOPER:
		case BCLASS_STORMPILOT:
		case BCLASS_SWAMPTROOPER:
		case BCLASS_TRANDOSHAN:
		case BCLASS_TUSKEN_SNIPER:
		case BCLASS_UGNAUGHT:
		case BCLASS_WAMPA:
		case BCLASS_WEEQUAY:
		case BCLASS_SOILDER:
		case BCLASS_BATTLEDROID:
		case BCLASS_SBD:
		case BCLASS_TROOPER1:
		case BCLASS_TROOPER2:
		case BCLASS_SMUGGLER1:
		case BCLASS_SMUGGLER2:
		case BCLASS_BOUNTYHUNTER2:
		case BCLASS_BOUNTYHUNTER3:
		case BCLASS_IPPERIALAGENT1:
		case BCLASS_IPPERIALAGENT2:
		case BCLASS_IPPERIALAGENT3:
		case BCLASS_PLAYER:
			client->ps.fd.forcePowerLevel[FP_HEAL] = FORCE_LEVEL_0;
			client->ps.fd.forcePowerLevel[FP_LEVITATION] = FORCE_LEVEL_0;
			client->ps.fd.forcePowerLevel[FP_SPEED] = FORCE_LEVEL_0;
			client->ps.fd.forcePowerLevel[FP_PUSH] = FORCE_LEVEL_0;
			client->ps.fd.forcePowerLevel[FP_PULL] = FORCE_LEVEL_0;
			client->ps.fd.forcePowerLevel[FP_TELEPATHY] = FORCE_LEVEL_0;
			client->ps.fd.forcePowerLevel[FP_GRIP] = FORCE_LEVEL_0;
			client->ps.fd.forcePowerLevel[FP_LIGHTNING] = FORCE_LEVEL_0;
			client->ps.fd.forcePowerLevel[FP_RAGE] = FORCE_LEVEL_0;
			client->ps.fd.forcePowerLevel[FP_PROTECT] = FORCE_LEVEL_0;
			client->ps.fd.forcePowerLevel[FP_ABSORB] = FORCE_LEVEL_0;
			client->ps.fd.forcePowerLevel[FP_TEAM_HEAL] = FORCE_LEVEL_0;
			client->ps.fd.forcePowerLevel[FP_TEAM_FORCE] = FORCE_LEVEL_0;
			client->ps.fd.forcePowerLevel[FP_DRAIN] = FORCE_LEVEL_0;
			client->ps.fd.forcePowerLevel[FP_SEE] = FORCE_LEVEL_0;
			client->ps.fd.forcePowerLevel[FP_SABER_OFFENSE] = FORCE_LEVEL_0;
			client->ps.fd.forcePowerLevel[FP_SABER_DEFENSE] = FORCE_LEVEL_0;
			client->ps.fd.forcePowerLevel[FP_SABERTHROW] = FORCE_LEVEL_0;
			break;
		case BCLASS_CHEWIE:
			client->ps.fd.forcePowerLevel[FP_HEAL] = FORCE_LEVEL_0;
			client->ps.fd.forcePowerLevel[FP_LEVITATION] = FORCE_LEVEL_0;
			client->ps.fd.forcePowerLevel[FP_SPEED] = FORCE_LEVEL_0;
			client->ps.fd.forcePowerLevel[FP_PUSH] = FORCE_LEVEL_0;
			client->ps.fd.forcePowerLevel[FP_PULL] = FORCE_LEVEL_0;
			client->ps.fd.forcePowerLevel[FP_TELEPATHY] = FORCE_LEVEL_0;
			client->ps.fd.forcePowerLevel[FP_GRIP] = FORCE_LEVEL_0;
			client->ps.fd.forcePowerLevel[FP_LIGHTNING] = FORCE_LEVEL_0;
			client->ps.fd.forcePowerLevel[FP_PROTECT] = FORCE_LEVEL_0;
			client->ps.fd.forcePowerLevel[FP_ABSORB] = FORCE_LEVEL_0;
			client->ps.fd.forcePowerLevel[FP_TEAM_HEAL] = FORCE_LEVEL_0;
			client->ps.fd.forcePowerLevel[FP_TEAM_FORCE] = FORCE_LEVEL_0;
			client->ps.fd.forcePowerLevel[FP_DRAIN] = FORCE_LEVEL_0;
			client->ps.fd.forcePowerLevel[FP_SEE] = FORCE_LEVEL_0;
			client->ps.fd.forcePowerLevel[FP_SABER_OFFENSE] = FORCE_LEVEL_0;
			client->ps.fd.forcePowerLevel[FP_SABER_DEFENSE] = FORCE_LEVEL_0;
			client->ps.fd.forcePowerLevel[FP_SABERTHROW] = FORCE_LEVEL_0;
			client->ps.fd.forcePowerLevel[FP_RAGE] = FORCE_LEVEL_3; //chewie gets rage
			break;
		case BCLASS_ROCKETTROOPER:
		case BCLASS_MANDOLORIAN:
		case BCLASS_MANDOLORIAN1:
		case BCLASS_MANDOLORIAN2:
		case BCLASS_TROOPER3:
		case BCLASS_BOUNTYHUNTER1:
		case BCLASS_BOBAFETT:
			client->ps.fd.forcePowerLevel[FP_HEAL] = FORCE_LEVEL_0;
			client->ps.fd.forcePowerLevel[FP_LEVITATION] = FORCE_LEVEL_3; //big jumps for the jetpack club
			client->ps.fd.forcePowerLevel[FP_SPEED] = FORCE_LEVEL_0;
			client->ps.fd.forcePowerLevel[FP_PUSH] = FORCE_LEVEL_0;
			client->ps.fd.forcePowerLevel[FP_PULL] = FORCE_LEVEL_0;
			client->ps.fd.forcePowerLevel[FP_TELEPATHY] = FORCE_LEVEL_0;
			client->ps.fd.forcePowerLevel[FP_GRIP] = FORCE_LEVEL_0;
			client->ps.fd.forcePowerLevel[FP_LIGHTNING] = FORCE_LEVEL_0;
			client->ps.fd.forcePowerLevel[FP_RAGE] = FORCE_LEVEL_0;
			client->ps.fd.forcePowerLevel[FP_PROTECT] = FORCE_LEVEL_0;
			client->ps.fd.forcePowerLevel[FP_ABSORB] = FORCE_LEVEL_0;
			client->ps.fd.forcePowerLevel[FP_TEAM_HEAL] = FORCE_LEVEL_0;
			client->ps.fd.forcePowerLevel[FP_TEAM_FORCE] = FORCE_LEVEL_0;
			client->ps.fd.forcePowerLevel[FP_DRAIN] = FORCE_LEVEL_0;
			client->ps.fd.forcePowerLevel[FP_SEE] = FORCE_LEVEL_0;
			client->ps.fd.forcePowerLevel[FP_SABER_OFFENSE] = FORCE_LEVEL_0;
			client->ps.fd.forcePowerLevel[FP_SABER_DEFENSE] = FORCE_LEVEL_0;
			client->ps.fd.forcePowerLevel[FP_SABERTHROW] = FORCE_LEVEL_0;
			break;
		case BCLASS_TUSKEN_RAIDER:
			client->ps.fd.forcePowerLevel[FP_HEAL] = FORCE_LEVEL_0;
			client->ps.fd.forcePowerLevel[FP_LEVITATION] = FORCE_LEVEL_0;
			client->ps.fd.forcePowerLevel[FP_SPEED] = FORCE_LEVEL_0;
			client->ps.fd.forcePowerLevel[FP_PUSH] = FORCE_LEVEL_0;
			client->ps.fd.forcePowerLevel[FP_PULL] = FORCE_LEVEL_0;
			client->ps.fd.forcePowerLevel[FP_TELEPATHY] = FORCE_LEVEL_0;
			client->ps.fd.forcePowerLevel[FP_GRIP] = FORCE_LEVEL_0;
			client->ps.fd.forcePowerLevel[FP_LIGHTNING] = FORCE_LEVEL_0;
			client->ps.fd.forcePowerLevel[FP_RAGE] = FORCE_LEVEL_0;
			client->ps.fd.forcePowerLevel[FP_PROTECT] = FORCE_LEVEL_0;
			client->ps.fd.forcePowerLevel[FP_ABSORB] = FORCE_LEVEL_0;
			client->ps.fd.forcePowerLevel[FP_TEAM_HEAL] = FORCE_LEVEL_0;
			client->ps.fd.forcePowerLevel[FP_TEAM_FORCE] = FORCE_LEVEL_0;
			client->ps.fd.forcePowerLevel[FP_DRAIN] = FORCE_LEVEL_0;
			client->ps.fd.forcePowerLevel[FP_SEE] = FORCE_LEVEL_0;
			client->ps.fd.forcePowerLevel[FP_SABER_OFFENSE] = FORCE_LEVEL_3;
			client->ps.fd.forcePowerLevel[FP_SABER_DEFENSE] = FORCE_LEVEL_3;
			client->ps.fd.forcePowerLevel[FP_SABERTHROW] = FORCE_LEVEL_3;
			break;
		case BCLASS_WOOKIE:
			client->ps.fd.forcePowerLevel[FP_HEAL] = FORCE_LEVEL_0;
			client->ps.fd.forcePowerLevel[FP_LEVITATION] = FORCE_LEVEL_0;
			client->ps.fd.forcePowerLevel[FP_SPEED] = FORCE_LEVEL_0;
			client->ps.fd.forcePowerLevel[FP_PUSH] = FORCE_LEVEL_0;
			client->ps.fd.forcePowerLevel[FP_PULL] = FORCE_LEVEL_0;
			client->ps.fd.forcePowerLevel[FP_TELEPATHY] = FORCE_LEVEL_0;
			client->ps.fd.forcePowerLevel[FP_GRIP] = FORCE_LEVEL_0;
			client->ps.fd.forcePowerLevel[FP_LIGHTNING] = FORCE_LEVEL_0;
			client->ps.fd.forcePowerLevel[FP_RAGE] = FORCE_LEVEL_3; //chewie gets rage
			client->ps.fd.forcePowerLevel[FP_PROTECT] = FORCE_LEVEL_0;
			client->ps.fd.forcePowerLevel[FP_ABSORB] = FORCE_LEVEL_0;
			client->ps.fd.forcePowerLevel[FP_TEAM_HEAL] = FORCE_LEVEL_0;
			client->ps.fd.forcePowerLevel[FP_TEAM_FORCE] = FORCE_LEVEL_0;
			client->ps.fd.forcePowerLevel[FP_DRAIN] = FORCE_LEVEL_0;
			client->ps.fd.forcePowerLevel[FP_SEE] = FORCE_LEVEL_0;
			client->ps.fd.forcePowerLevel[FP_SABER_OFFENSE] = FORCE_LEVEL_3;
			client->ps.fd.forcePowerLevel[FP_SABER_DEFENSE] = FORCE_LEVEL_3;
			client->ps.fd.forcePowerLevel[FP_SABERTHROW] = FORCE_LEVEL_3;
			break;
		case BCLASS_WOOKIEMELEE:
			client->ps.fd.forcePowerLevel[FP_HEAL] = FORCE_LEVEL_0;
			client->ps.fd.forcePowerLevel[FP_LEVITATION] = FORCE_LEVEL_0;
			client->ps.fd.forcePowerLevel[FP_SPEED] = FORCE_LEVEL_0;
			client->ps.fd.forcePowerLevel[FP_PUSH] = FORCE_LEVEL_0;
			client->ps.fd.forcePowerLevel[FP_PULL] = FORCE_LEVEL_0;
			client->ps.fd.forcePowerLevel[FP_TELEPATHY] = FORCE_LEVEL_0;
			client->ps.fd.forcePowerLevel[FP_GRIP] = FORCE_LEVEL_0;
			client->ps.fd.forcePowerLevel[FP_LIGHTNING] = FORCE_LEVEL_0;
			client->ps.fd.forcePowerLevel[FP_RAGE] = FORCE_LEVEL_3; //chewie gets rage
			client->ps.fd.forcePowerLevel[FP_PROTECT] = FORCE_LEVEL_0;
			client->ps.fd.forcePowerLevel[FP_ABSORB] = FORCE_LEVEL_0;
			client->ps.fd.forcePowerLevel[FP_TEAM_HEAL] = FORCE_LEVEL_0;
			client->ps.fd.forcePowerLevel[FP_TEAM_FORCE] = FORCE_LEVEL_0;
			client->ps.fd.forcePowerLevel[FP_DRAIN] = FORCE_LEVEL_0;
			client->ps.fd.forcePowerLevel[FP_SEE] = FORCE_LEVEL_0;
			client->ps.fd.forcePowerLevel[FP_SABER_OFFENSE] = FORCE_LEVEL_0;
			client->ps.fd.forcePowerLevel[FP_SABER_DEFENSE] = FORCE_LEVEL_0;
			client->ps.fd.forcePowerLevel[FP_SABERTHROW] = FORCE_LEVEL_0;
			break;
		default:
			//Read bot.txt or Player FP setup for saber users
			break;
		}
	}

	if (client->sess.sessionTeam == TEAM_SPECTATOR)
	{
		client->ps.stats[STAT_WEAPONS] = 0;
		client->ps.stats[STAT_HOLDABLE_ITEMS] = 0;
		client->ps.stats[STAT_HOLDABLE_ITEM] = 0;
	}

	// nmckenzie: DESERT_SIEGE... or well, siege generally.  This was over-writing the max value, which was NOT good for siege.
	if (inSiegeWithClass == qfalse)
	{
		client->ps.ammo[AMMO_BLASTER] = 500;
	}
	client->ps.rocketLockIndex = ENTITYNUM_NONE;
	client->ps.rocketLockTime = 0;
	client->ps.genericEnemyIndex = -1;

	G_AddEvent(ent, EV_WEAPINVCHANGE, client->ps.stats[STAT_WEAPONS]);

	client->ps.isJediMaster = qfalse;

	if (client->ps.fallingToDeath)
	{
		client->ps.fallingToDeath = 0;
		client->noCorpse = qtrue;
	}

	//Do per-spawn force power initialization
	WP_SpawnInitForcePowers(ent);

	ent->reloadTime = 0;

	//set saberAnimLevelBase
	if (ent->client->saber[0].model[0] && ent->client->saber[1].model[0])
	{
		ent->client->ps.fd.saberAnimLevelBase = SS_DUAL;
	}
	else if (ent->client->saber[0].numBlades > 1
		&& WP_SaberCanTurnOffSomeBlades(&ent->client->saber[0]))
	{
		ent->client->ps.fd.saberAnimLevelBase = SS_STAFF;
	}
	else
	{
		ent->client->ps.fd.saberAnimLevelBase = SS_MEDIUM;
	}

	//set initial saber holstered mode
	if (ent->client->saber[0].model[0] && ent->client->saber[1].model[0]
		&& WP_SaberCanTurnOffSomeBlades(&ent->client->saber[1])
		&& ent->client->ps.fd.saberAnimLevel != SS_DUAL)
	{
		//using dual sabers, but not the dual style, turn off blade
		ent->client->ps.saberHolstered = 1;
	}
	else if (ent->client->saber[0].numBlades > 1
		&& WP_SaberCanTurnOffSomeBlades(&ent->client->saber[0])
		&& ent->client->ps.fd.saberAnimLevel != SS_STAFF)
	{
		//using staff saber, but not the staff style, turn off blade
		ent->client->ps.saberHolstered = 1;
	}
	else
	{
		ent->client->ps.saberHolstered = 0;
	}

	// health will count down towards max_health
	if (level.gametype == GT_SIEGE &&
		client->siegeClass != -1 &&
		bgSiegeClasses[client->siegeClass].starthealth)
	{
		//class specifies a start health, so use it
		ent->health = client->ps.stats[STAT_HEALTH] = bgSiegeClasses[client->siegeClass].starthealth;
	}
	else if (level.gametype == GT_DUEL || level.gametype == GT_POWERDUEL)
	{
		//only start with 100 health in Duel
		if (level.gametype == GT_POWERDUEL && client->sess.duelTeam == DUELTEAM_LONE)
		{
			if (duel_fraglimit.integer)
			{
				ent->health = client->ps.stats[STAT_HEALTH] = client->ps.stats[STAT_MAX_HEALTH] =
					g_powerDuelStartHealth.integer - (g_powerDuelStartHealth.integer - g_powerDuelEndHealth.integer) *
					(float)client->sess.wins / (float)duel_fraglimit.integer;
			}
			else
			{
				ent->health = client->ps.stats[STAT_HEALTH] = client->ps.stats[STAT_MAX_HEALTH] = 150;
			}
		}
		else
		{
			ent->health = client->ps.stats[STAT_HEALTH] = client->ps.stats[STAT_MAX_HEALTH] = 100;
		}
	}
	else if (client->ps.stats[STAT_MAX_HEALTH] <= 100)
	{
		ent->health = client->ps.stats[STAT_HEALTH] = client->ps.stats[STAT_MAX_HEALTH] * 1.25;
	}
	else if (client->ps.stats[STAT_MAX_HEALTH] < 125)
	{
		ent->health = client->ps.stats[STAT_HEALTH] = 125;
	}
	else
	{
		ent->health = client->ps.stats[STAT_HEALTH] = client->ps.stats[STAT_MAX_HEALTH];
	}

	// Start with a small amount of armor as well.
	if (level.gametype == GT_SIEGE &&
		client->siegeClass != -1 /*&&
		bgSiegeClasses[client->siegeClass].startarmor*/)
	{
		//class specifies a start armor amount, so use it
		client->ps.stats[STAT_ARMOR] = bgSiegeClasses[client->siegeClass].startarmor;
	}
	else if (level.gametype == GT_DUEL || level.gametype == GT_POWERDUEL)
	{
		//no armor in duel
		client->ps.stats[STAT_ARMOR] = 0;
	}
	else
	{
		client->ps.stats[STAT_ARMOR] = client->ps.stats[STAT_MAX_HEALTH] * 0.25;
	}

	ScalePlayer(ent, ent->client->pers.botmodelscale);
	client->ps.stats[STAT_DODGE] = client->ps.stats[STAT_MAX_DODGE];
	G_SetOrigin(ent, spawn_origin);
	VectorCopy(spawn_origin, client->ps.origin);

	// the respawned flag will be cleared after the attack and jump keys come up
	client->ps.pm_flags |= PMF_RESPAWNED;

	trap->GetUsercmd(client - level.clients, &ent->client->pers.cmd);
	SetClientViewAngle(ent, spawn_angles);

	// don't allow full run speed for a bit
	client->ps.pm_flags |= PMF_TIME_KNOCKBACK;
	client->ps.pm_time = 100;

	client->respawnTime = level.time;
	client->inactivityTime = level.time + g_inactivity.integer * 1000;
	client->latched_buttons = 0;

	if (level.intermissiontime)
	{
		MoveClientToIntermission(ent);
	}
	else
	{
		if (ent->client->sess.sessionTeam != TEAM_SPECTATOR)
		{
			g_kill_box(ent);
			if (client->ps.weapon <= WP_NONE)
			{
				client->ps.weapon = WP_BRYAR_PISTOL;
			}

			client->ps.torsoTimer = client->ps.legsTimer = 0;

			if (client->ps.weapon == WP_SABER)
			{
				G_SetAnim(ent, NULL, SETANIM_BOTH, BOTH_STAND1TO2,
					SETANIM_FLAG_OVERRIDE | SETANIM_FLAG_HOLD | SETANIM_FLAG_HOLDLESS, 0);
			}
			else
			{
				G_SetAnim(ent, NULL, SETANIM_TORSO, TORSO_RAISEWEAP1,
					SETANIM_FLAG_OVERRIDE | SETANIM_FLAG_HOLD | SETANIM_FLAG_HOLDLESS, 0);

				if (client->ps.eFlags & EF_DUAL_WEAPONS)
				{
					client->ps.legsAnim = WeaponReadyAnim2[client->ps.weapon];
				}
				else
				{
					client->ps.legsAnim = WeaponReadyAnim[client->ps.weapon];
				}
			}
			client->ps.weaponstate = WEAPON_RAISING;
			client->ps.weaponTime = client->ps.torsoTimer;

			if (g_spawnInvulnerability.integer)
			{
				ent->client->ps.eFlags |= EF_INVULNERABLE;
				ent->client->invulnerableTimer = level.time + g_spawnInvulnerability.integer;
			}

			// fire the targets of the spawn point
			UpdatePlayerScriptTarget();

			// positively link the client, even if the command times are weird
			VectorCopy(ent->client->ps.origin, ent->r.currentOrigin);

			tent = G_TempEntity(ent->client->ps.origin, EV_PLAYER_TELEPORT_IN);
			tent->s.client_num = ent->s.client_num;

			trap->LinkEntity((sharedEntity_t*)ent);
		}
	}

	//set teams for NPCs to recognize
	if (level.gametype == GT_SIEGE)
	{
		//Imperial (team1) team is allied with "enemy" NPCs in this mode
		if (client->sess.sessionTeam == SIEGETEAM_TEAM1)
		{
			client->playerTeam = ent->s.teamowner = NPCTEAM_ENEMY;
			client->enemyTeam = NPCTEAM_PLAYER;
		}
		else
		{
			client->playerTeam = ent->s.teamowner = NPCTEAM_PLAYER;
			client->enemyTeam = NPCTEAM_ENEMY;
		}
	}
	else
	{
		client->playerTeam = ent->s.teamowner = NPCTEAM_PLAYER;
		client->enemyTeam = NPCTEAM_ENEMY;
	}

	// run a client frame to drop exactly to the floor,
	// initialize animations and other things
	client->ps.commandTime = level.time - 100;
	ent->client->pers.cmd.serverTime = level.time;
	ClientThink(ent - g_entities, NULL);

	// run the presend to set anything else, follow spectators wait
	// until all clients have been reconnected after map_restart
	if (ent->client->sess.spectatorState != SPECTATOR_FOLLOW)
		ClientEndFrame(ent);

	// clear entity state values
	BG_PlayerStateToEntityState(&client->ps, &ent->s, qtrue);

	//rww - make sure client has a valid icarus instance
	trap->ICARUS_FreeEnt((sharedEntity_t*)ent);
	trap->ICARUS_InitEnt((sharedEntity_t*)ent);

	if (!level.intermissiontime)
	{
		//we want to use all the spawnpoint's triggers if we're not in intermission.
		G_UseTargets(spawnPoint, ent);
		if (level.gametype == GT_SINGLE_PLAYER && spawnPoint)
		{
			//remove the target of the spawnpoint to prevent multiple target firings
			spawnPoint->target = NULL;
		}
	}

	Player_CheckBurn(ent);
	Player_CheckFreeze(ent);
}

/*
===========
ClientDisconnect

Called when a player drops from the server.
Will not be called between levels.

This should NOT be called directly by any game logic,
call trap->DropClient(), which will call this and do
server system housekeeping.
============
*/
extern void G_LeaveVehicle(gentity_t* ent, qboolean ConCheck);

void G_ClearVote(const gentity_t* ent)
{
	if (level.voteTime)
	{
		if (ent->client->mGameFlags & PSG_VOTED)
		{
			if (ent->client->pers.vote == 1)
			{
				level.voteYes--;
				trap->SetConfigstring(CS_VOTE_YES, va("%i", level.voteYes));
			}
			else if (ent->client->pers.vote == 2)
			{
				level.voteNo--;
				trap->SetConfigstring(CS_VOTE_NO, va("%i", level.voteNo));
			}
		}
		ent->client->mGameFlags &= ~(PSG_VOTED);
		ent->client->pers.vote = 0;
	}
}

void G_ClearTeamVote(const gentity_t* ent, int team)
{
	int voteteam;

	if (team == TEAM_RED) voteteam = 0;
	else if (team == TEAM_BLUE) voteteam = 1;
	else return;

	if (level.teamVoteTime[voteteam])
	{
		if (ent->client->mGameFlags & PSG_TEAMVOTED)
		{
			if (ent->client->pers.teamvote == 1)
			{
				level.teamVoteYes[voteteam]--;
				trap->SetConfigstring(CS_TEAMVOTE_YES, va("%i", level.teamVoteYes[voteteam]));
			}
			else if (ent->client->pers.teamvote == 2)
			{
				level.teamVoteNo[voteteam]--;
				trap->SetConfigstring(CS_TEAMVOTE_NO, va("%i", level.teamVoteNo[voteteam]));
			}
		}
		ent->client->mGameFlags &= ~(PSG_TEAMVOTED);
		ent->client->pers.teamvote = 0;
	}
}

void ClientDisconnect(int client_num)
{
	// cleanup if we are kicking a bot that
	// hasn't spawned yet
	G_RemoveQueuedBotBegin(client_num);

	gentity_t* ent = g_entities + client_num;
	if (!ent->client || ent->client->pers.connected == CON_DISCONNECTED)
	{
		return;
	}

	int i = 0;

	while (i < NUM_FORCE_POWERS)
	{
		if (ent->client->ps.fd.forcePowersActive & 1 << i)
		{
			WP_ForcePowerStop(ent, i);
		}
		i++;
	}

	i = TRACK_CHANNEL_1;

	while (i < NUM_TRACK_CHANNELS)
	{
		if (ent->client->ps.fd.killSoundEntIndex[i - 50] && ent->client->ps.fd.killSoundEntIndex[i - 50] < MAX_GENTITIES
			&& ent->client->ps.fd.killSoundEntIndex[i - 50] > 0)
		{
			G_MuteSound(ent->client->ps.fd.killSoundEntIndex[i - 50], CHAN_VOICE);
		}
		i++;
	}
	i = 0;

	G_LeaveVehicle(ent, qtrue);

	if (ent->client->ewebIndex)
	{
		gentity_t* eweb = &g_entities[ent->client->ewebIndex];

		ent->client->ps.emplacedIndex = 0;
		ent->client->ewebIndex = 0;
		ent->client->ewebHealth = 0;
		G_FreeEntity(eweb);
	}

	// stop any following clients
	for (i = 0; i < level.maxclients; i++)
	{
		if (level.clients[i].sess.sessionTeam == TEAM_SPECTATOR
			&& level.clients[i].sess.spectatorState == SPECTATOR_FOLLOW
			&& level.clients[i].sess.spectatorClient == client_num)
		{
			StopFollowing(&g_entities[i]);
		}
	}

	// send effect if they were completely connected
	if (ent->client->pers.connected == CON_CONNECTED
		&& ent->client->sess.sessionTeam != TEAM_SPECTATOR)
	{
		gentity_t* tent = G_TempEntity(ent->client->ps.origin, EV_PLAYER_TELEPORT_OUT);
		tent->s.client_num = ent->s.client_num;

		// They don't get to take powerups with them!
		// Especially important for stuff like CTF flags
		TossClientItems(ent);
	}

	G_LogPrintf("ClientDisconnect: %i [%s] (%s) \"%s^7\"\n", client_num, ent->client->sess.IP, ent->client->pers.guid,
		ent->client->pers.netname);

	// if we are playing in tourney mode, give a win to the other player and clear his frags for this round
	if (level.gametype == GT_DUEL && !level.intermissiontime && !level.warmupTime)
	{
		if (level.sortedClients[1] == client_num)
		{
			level.clients[level.sortedClients[0]].ps.persistant[PERS_SCORE] = 0;
			level.clients[level.sortedClients[0]].sess.wins++;
			client_userinfo_changed(level.sortedClients[0]);
		}
		else if (level.sortedClients[0] == client_num)
		{
			level.clients[level.sortedClients[1]].ps.persistant[PERS_SCORE] = 0;
			level.clients[level.sortedClients[1]].sess.wins++;
			client_userinfo_changed(level.sortedClients[1]);
		}
	}

	if (level.gametype == GT_DUEL && ent->client->sess.sessionTeam == TEAM_FREE && level.intermissiontime)
	{
		trap->SendConsoleCommand(EXEC_APPEND, "map_restart 0\n");
		level.restarted = qtrue;
		level.changemap = NULL;
		level.intermissiontime = 0;
	}

	if (ent->ghoul2 && trap->G2API_HaveWeGhoul2Models(ent->ghoul2))
	{
		trap->G2API_CleanGhoul2Models(&ent->ghoul2);
	}
	i = 0;
	while (i < MAX_SABERS)
	{
		if (ent->client->weaponGhoul2[i] && trap->G2API_HaveWeGhoul2Models(ent->client->weaponGhoul2[i]))
		{
			trap->G2API_CleanGhoul2Models(&ent->client->weaponGhoul2[i]);
		}
		i++;
	}

	G_ClearVote(ent);
	G_ClearTeamVote(ent, ent->client->sess.sessionTeam);

	trap->UnlinkEntity((sharedEntity_t*)ent);
	ent->s.modelindex = 0;
	ent->inuse = qfalse;
	ent->classname = "disconnected";
	ent->client->pers.connected = CON_DISCONNECTED;
	ent->client->ps.persistant[PERS_TEAM] = TEAM_FREE;
	ent->client->sess.sessionTeam = TEAM_FREE;
	ent->r.contents = 0;
	//============================grapplemod===================
	if (ent->client->hook)
	{
		// free it!
		Weapon_HookFree(ent->client->hook);
	}
	if (ent->client->stun)
	{
		// free it!
		Weapon_StunFree(ent->client->stun);
	}

	if (ent->client->holdingObjectiveItem > 0)
	{
		//carrying a siege objective item - make sure it updates and removes itself from us now in case this is an instant death-respawn situation
		gentity_t* objectiveItem = &g_entities[ent->client->holdingObjectiveItem];

		if (objectiveItem->inuse && objectiveItem->think)
		{
			objectiveItem->think(objectiveItem);
		}
	}

	trap->SetConfigstring(CS_PLAYERS + client_num, "");

	CalculateRanks();

	if (ent->r.svFlags & SVF_BOT)
	{
		BotAIShutdownClient(client_num);
	}

	G_ClearClientLog(client_num);
}

qboolean G_StandardHumanoid(gentity_t* self)
{
	char GLAName[MAX_QPATH];

	if (!self || !self->ghoul2)
	{
		return qfalse;
	}

	trap->G2API_GetGLAName(&self->ghoul2, 0, GLAName);

	assert(GLAName);

	if (GLAName)
	{
		if (!Q_stricmpn("models/players/_humanoid", GLAName, 24))
		{
			//only _humanoid skeleton is expected to have these
			return qtrue;
		}
		if (!Q_stricmpn("models/players/JK2anims/", GLAName, 24))
		{
			//only _humanoid skeleton is expected to have these
			return qtrue;
		}
		if (!Q_stricmpn("models/players/_humanoid_sbd", GLAName, 24)) ///_humanoid", GLAName, 36) )
		{
			//only _humanoid skeleton is expected to have these
			return qtrue;
		}
		if (!Q_stricmpn("models/players/_humanoid_yoda", GLAName, 24)) ///_humanoid", GLAName, 36) )
		{
			//only _humanoid skeleton is expected to have these
			return qtrue;
		}
		if (!Q_stricmp("models/players/protocol/protocol", GLAName))
		{
			//protocol droid duplicates many of these
			return qtrue;
		}
		if (!Q_stricmp("models/players/assassin_droid/model", GLAName))
		{
			//assassin_droid duplicates many of these
			return qtrue;
		}
		if (!Q_stricmp("models/players/saber_droid/model", GLAName))
		{
			//saber_droid duplicates many of these
			return qtrue;
		}
		if (!Q_stricmp("models/players/hazardtrooper/hazardtrooper", GLAName))
		{
			//hazardtrooper duplicates many of these
			return qtrue;
		}
		if (!Q_stricmp("models/players/rockettrooper/rockettrooper", GLAName))
		{
			//rockettrooper duplicates many of these
			return qtrue;
		}
		if (!Q_stricmp("models/players/wampa/wampa", GLAName))
		{
			//rockettrooper duplicates many of these
			return qtrue;
		}
	}
	return qfalse;
}

qboolean LMS_EnoughPlayers()
{
	//checks to see if there's enough players in game to enable LMS rules
	if (level.numNonSpectatorClients < 2)
	{
		//definitely not enough.
		return qfalse;
	}

	if (level.gametype >= GT_TEAM
		&& (TeamCount(-1, TEAM_RED) == 0 || TeamCount(-1, TEAM_BLUE) == 0))
	{
		//have to have at least one player on each team to be able to play LMS
		return qfalse;
	}

	return qtrue;
}

qboolean SJE_AllPlayersHaveClientPlugin(void)
{
	for (int i = 0; i < level.maxclients; i++)
	{
		if (g_entities[i].inuse && !g_entities[i].client->pers.SJE_clientplugin)
		{
			//a live player that doesn't have the plugin
			return qfalse;
		}
	}

	return qtrue;
}