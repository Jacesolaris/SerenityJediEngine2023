/*
This file is part of Serenity.
*/
// leave this line at the top of all AI_xxxx.cpp files for PCH reasons...

#include "b_local.h"
#include "g_functions.h"

constexpr auto MIN_MELEE_RANGE = 640;
#define	MIN_MELEE_RANGE_SQR	( MIN_MELEE_RANGE * MIN_MELEE_RANGE )

constexpr auto MIN_DISTANCE = 128;
#define MIN_DISTANCE_SQR	( MIN_DISTANCE * MIN_DISTANCE )

constexpr auto DROIDEKA_SHIELD_HEALTH = 250;
constexpr auto SHIELD_EFFECT_TIME = 500;
constexpr auto GENERATOR_HEALTH = 10;
constexpr auto TURN_ON = 0x00000000;
constexpr auto TURN_OFF = 0x00000100;
static vec3_t shieldMins = { -20, -20, -24 };
static vec3_t shieldMaxs = { 20, 20, 40 };
extern void NPC_SetPainEvent(gentity_t* self);
extern void G_AddVoiceEvent(const gentity_t* self, int event, int speakDebounceTime);
extern qboolean Q3_TaskIDPending(const gentity_t* ent, taskID_t taskType);

/*
-------------------------
NPC_ATST_Precache
-------------------------
*/
void npc_droideka_precache()
{
	G_SoundIndex("sound/chars/droideka/pain25");
	G_SoundIndex("sound/chars/droideka/pain100");
	G_EffectIndex("env/med_explode2");
	G_EffectIndex("env/small_explode2");
	G_EffectIndex("galak/explode");
}

void NPC_DROIDEKA_Init(gentity_t* ent)
{
	if (ent->NPC->behaviorState != BS_CINEMATIC)
	{
		ent->client->ps.stats[STAT_ARMOR] = DROIDEKA_SHIELD_HEALTH;
		ent->NPC->investigateCount = ent->NPC->investigateDebounceTime = 0;
		ent->flags |= FL_SHIELDED; //reflect normal shots
		ent->client->ps.powerups[PW_GALAK_SHIELD] = Q3_INFINITE; //temp, for effect
		ent->fx_time = level.time;
		VectorSet(ent->mins, -60, -60, -24);
		VectorSet(ent->maxs, 60, 60, 80);
		ent->flags |= FL_NO_KNOCKBACK; //don't get pushed
		TIMER_Set(ent, "attackDelay", 0); //FIXME: Slant for difficulty levels
		TIMER_Set(ent, "flee", 0);
		TIMER_Set(ent, "talkDebounce", 0);
		gi.G2API_SetSurfaceOnOff(&ent->ghoul2[ent->playerModel], "torso_shield_off", TURN_ON);
	}
	else
	{
		gi.G2API_SetSurfaceOnOff(&ent->ghoul2[ent->playerModel], "torso_shield_off", TURN_OFF);
	}
}

//-----------------------------------------------------------------
static void DROIDEKA_CreateExplosion(gentity_t* self, const int boltID, qboolean doSmall = qfalse)
{
	if (boltID >= 0)
	{
		mdxaBone_t boltMatrix;
		vec3_t org, dir;
		gi.G2API_GetBoltMatrix(self->ghoul2, self->playerModel, boltID, &boltMatrix, self->currentAngles,
			self->currentOrigin, level.time, nullptr, self->s.modelScale);
		gi.G2API_GiveMeVectorFromMatrix(boltMatrix, ORIGIN, org);
		gi.G2API_GiveMeVectorFromMatrix(boltMatrix, NEGATIVE_Y, dir);

		if (doSmall)
		{
			G_PlayEffect("env/small_explode2", org, dir);
		}
		else
		{
			G_PlayEffect("env/med_explode2", org, dir);
		}
	}
}

/*
-------------------------
GM_Dying
-------------------------
*/

void DROIDEKA_Dying(gentity_t* self)
{
	if (level.time - self->s.time < 4000)
	{
		//FIXME: need a real effect
		self->s.powerups |= 1 << PW_SHOCKED;
		self->client->ps.powerups[PW_SHOCKED] = level.time + 1000;
		if (TIMER_Done(self, "dyingExplosion"))
		{
			int newBolt;
			switch (Q_irand(1, 14))
			{
				// Find place to generate explosion
			case 1:
				if (!gi.G2API_GetSurfaceRenderStatus(&self->ghoul2[self->playerModel], "r_hand"))
				{
					//r_hand still there
					DROIDEKA_CreateExplosion(self, self->handRBolt, qtrue);
					gi.G2API_SetSurfaceOnOff(&self->ghoul2[self->playerModel], "r_hand", TURN_OFF);
				}
				else if (!gi.G2API_GetSurfaceRenderStatus(&self->ghoul2[self->playerModel], "r_arm_middle"))
				{
					//r_arm_middle still there
					newBolt = gi.G2API_AddBolt(&self->ghoul2[self->playerModel], "*r_arm_elbow");
					gi.G2API_SetSurfaceOnOff(&self->ghoul2[self->playerModel], "r_arm_middle", TURN_OFF);
				}
				break;
			case 2:
				//FIXME: do only once?
				if (!gi.G2API_GetSurfaceRenderStatus(&self->ghoul2[self->playerModel], "l_hand"))
				{
					//l_hand still there
					DROIDEKA_CreateExplosion(self, self->handLBolt);
					gi.G2API_SetSurfaceOnOff(&self->ghoul2[self->playerModel], "l_hand", TURN_OFF);
				}
				else if (!gi.G2API_GetSurfaceRenderStatus(&self->ghoul2[self->playerModel], "l_arm_wrist"))
				{
					//l_arm_wrist still there
					newBolt = gi.G2API_AddBolt(&self->ghoul2[self->playerModel], "*l_arm_cap_l_hand");
					gi.G2API_SetSurfaceOnOff(&self->ghoul2[self->playerModel], "l_arm_wrist", TURN_OFF);
				}
				else if (!gi.G2API_GetSurfaceRenderStatus(&self->ghoul2[self->playerModel], "l_arm_middle"))
				{
					//l_arm_middle still there
					newBolt = gi.G2API_AddBolt(&self->ghoul2[self->playerModel], "*l_arm_cap_l_hand");
					gi.G2API_SetSurfaceOnOff(&self->ghoul2[self->playerModel], "l_arm_middle", TURN_OFF);
				}
				else if (!gi.G2API_GetSurfaceRenderStatus(&self->ghoul2[self->playerModel], "l_arm_augment"))
				{
					//l_arm_augment still there
					newBolt = gi.G2API_AddBolt(&self->ghoul2[self->playerModel], "*l_arm_elbow");
					gi.G2API_SetSurfaceOnOff(&self->ghoul2[self->playerModel], "l_arm_augment", TURN_OFF);
				}
				break;
			case 3:
			case 4:
				newBolt = gi.G2API_AddBolt(&self->ghoul2[self->playerModel], "*hip_fr");
				DROIDEKA_CreateExplosion(self, newBolt);
				break;
			case 5:
			case 6:
				newBolt = gi.G2API_AddBolt(&self->ghoul2[self->playerModel], "*shldr_l");
				DROIDEKA_CreateExplosion(self, newBolt);
				break;
			case 7:
			case 8:
				newBolt = gi.G2API_AddBolt(&self->ghoul2[self->playerModel], "*uchest_r");
				DROIDEKA_CreateExplosion(self, newBolt);
				break;
			case 9:
			case 10:
				DROIDEKA_CreateExplosion(self, self->headBolt);
				break;
			case 11:
				newBolt = gi.G2API_AddBolt(&self->ghoul2[self->playerModel], "*l_leg_knee");
				DROIDEKA_CreateExplosion(self, newBolt, qtrue);
				break;
			case 12:
				newBolt = gi.G2API_AddBolt(&self->ghoul2[self->playerModel], "*r_leg_knee");
				DROIDEKA_CreateExplosion(self, newBolt, qtrue);
				break;
			case 13:
				newBolt = gi.G2API_AddBolt(&self->ghoul2[self->playerModel], "*l_leg_foot");
				DROIDEKA_CreateExplosion(self, newBolt, qtrue);
				break;
			case 14:
				newBolt = gi.G2API_AddBolt(&self->ghoul2[self->playerModel], "*r_leg_foot");
				DROIDEKA_CreateExplosion(self, newBolt, qtrue);
				break;
			default:;
			}

			TIMER_Set(self, "dyingExplosion", Q_irand(300, 1100));
		}
	}
	else
	{
		//one final, huge explosion
		G_PlayEffect("galak/explode", self->currentOrigin);
		self->nextthink = level.time + FRAMETIME;
		self->e_ThinkFunc = thinkF_G_FreeEntity;
	}
}

/*
-------------------------
G_DROIDEKACheckPain

Called by NPC's and player in an DROIDEKA
-------------------------
*/
void g_droideka_check_pain(const gentity_t* self, gentity_t* other, const vec3_t point, int damage, int mod, int hit_loc)
{
	if (rand() & 1)
	{
		G_SoundOnEnt(self, CHAN_LESS_ATTEN, "sound/chars/droideka/pain25");
	}
	else
	{
		G_SoundOnEnt(self, CHAN_LESS_ATTEN, "sound/chars/droideka/pain100");
	}
}

/*
-------------------------
NPC_DROIDEKA_Pain
-------------------------
*/
void NPC_DROIDEKA_Pain(gentity_t* self, gentity_t* inflictor, gentity_t* attacker, const vec3_t point, int damage,
	int mod, int hit_loc)
{
	if (self->client->ps.powerups[PW_GALAK_SHIELD] == 0)
	{
		//shield is currently down
		if (hit_loc == HL_GENERIC1 && self->locationDamage[HL_GENERIC1] > GENERATOR_HEALTH)
		{
			gi.G2API_SetSurfaceOnOff(&self->ghoul2[self->playerModel], "torso_shield_off", TURN_OFF);
			self->client->ps.powerups[PW_GALAK_SHIELD] = 0; //temp, for effect
			self->client->ps.stats[STAT_ARMOR] = 0; //no more armor
			self->NPC->investigateDebounceTime = 0; //stop recharging

			NPC_SetAnim(self, SETANIM_BOTH, BOTH_PAIN1, SETANIM_FLAG_OVERRIDE | SETANIM_FLAG_HOLD);
			TIMER_Set(self, "attackDelay", self->client->ps.torsoAnimTimer);
			G_AddEvent(self, Q_irand(EV_DEATH1, EV_DEATH3), self->health);
		}
	}
	else
	{
		//store the point for shield impact
		if (point)
		{
			VectorCopy(point, self->pos4);
			self->client->poisonTime = level.time;
			self->client->stunTime = level.time;
		}
	}

	if (!self->lockCount && !self->client->ps.torsoAnimTimer)
	{
		//don't interrupt laser sweep attack or other special attacks/moves
		if (self->count < 4 && self->health > 100 && hit_loc != HL_GENERIC1)
		{
			if (self->delay < level.time)
			{
				int speech;
				switch (self->count)
				{
				default:
				case 0:
					speech = EV_PUSHED1;
					break;
				case 1:
					speech = EV_PUSHED2;
					break;
				case 2:
					speech = EV_PUSHED3;
					break;
				case 3:
					speech = EV_DETECTED1;
					break;
				}
				self->count++;
				self->NPC->blockedSpeechDebounceTime = 0;
				G_AddVoiceEvent(self, speech, Q_irand(3000, 5000));
				self->delay = level.time + Q_irand(5000, 7000);
			}
		}
		else
		{
			NPC_Pain(self, inflictor, attacker, point, damage, mod, hit_loc);
		}
	}
	else if (hit_loc == HL_GENERIC1)
	{
		NPC_SetPainEvent(self);
		self->s.powerups |= 1 << PW_SHOCKED;
		self->client->ps.powerups[PW_SHOCKED] = level.time + Q_irand(500, 2500);
	}
}

static void DROIDEKA_HoldPosition(void)
{
	NPC_FreeCombatPoint(NPCInfo->combatPoint, qtrue);
	if (!Q3_TaskIDPending(NPC, TID_MOVE_NAV))
	{
		//don't have a script waiting for me to get to my point, okay to stop trying and stand
		NPCInfo->goalEntity = nullptr;
	}
}

static qboolean DROIDEKA_Move(void)
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
			DROIDEKA_HoldPosition();
		}
	}

	//If our move failed, then reset
	if (moved == qfalse)
	{
		//FIXME: if we're going to a combat point, need to pick a different one
		if (!Q3_TaskIDPending(NPC, TID_MOVE_NAV))
		{
			//can't transfer movegoal or stop when a script we're running is waiting to complete
			DROIDEKA_HoldPosition();
		}
	}

	return moved;
}

/*
-------------------------
DROIDEKA_Hunt
-------------------------`
*/
void DROIDEKA_Hunt(qboolean visible, qboolean advance)
{
	//If we're not supposed to stand still, pursue the player
	if (NPCInfo->standTime < level.time)
	{
		//Move towards our goal
		NPCInfo->goalEntity = NPC->enemy;
		NPCInfo->goalRadius = 12;
		NPC_MoveToGoal(qtrue);
	}

	if (NPCInfo->goalEntity == nullptr)
	{
		//hunt
		NPCInfo->goalEntity = NPC->enemy;
	}
	NPCInfo->combatMove = qtrue;

	//If we have somewhere to go, then do that
	if (UpdateGoal())
	{
		ucmd.buttons |= BUTTON_WALKING;
		NPC_MoveToGoal(qtrue);
	}
	NPC_FaceEnemy();
	//Update our angles regardless
	NPC_UpdateAngles(qtrue, qtrue);
}

/*
-------------------------
DROIDEKA_Ranged
-------------------------
*/
void DROIDEKA_Ranged(qboolean visible, qboolean advance, qboolean altAttack)
{
	if (TIMER_Done(NPC, "atkDelay") && visible) // Attack?
	{
		TIMER_Set(NPC, "atkDelay", Q_irand(100, 500));
		ucmd.buttons |= BUTTON_ATTACK;
	}

	if (NPCInfo->scriptFlags & SCF_CHASE_ENEMIES)
	{
		DROIDEKA_Hunt(visible, advance);
	}
}

/*
-------------------------
DROIDEKA_Attack
-------------------------
*/
void droideka_attack()
{
	constexpr qboolean altAttack = qfalse;

	// Rate our distance to the target, and our visibilty
	const float distance = static_cast<int>(DistanceHorizontalSquared(NPC->currentOrigin, NPC->enemy->currentOrigin));
	const distance_e distRate = distance > MIN_MELEE_RANGE_SQR ? DIST_LONG : DIST_MELEE;
	const qboolean visible = NPC_ClearLOS(NPC->enemy);
	const auto advance = static_cast<qboolean>(distance > MIN_DISTANCE_SQR);

	if (NPC_CheckEnemyExt() == qfalse)
	{
		NPC->enemy = nullptr;
		DROIDEKA_Hunt(visible, advance);
		return;
	}

	// If we cannot see our target, move to see it
	if (visible == qfalse)
	{
		if (NPCInfo->scriptFlags & SCF_CHASE_ENEMIES)
		{
			DROIDEKA_Hunt(visible, advance);
			return;
		}
	}

	//If we're too far away, then keep walking forward
	if (distRate != DIST_MELEE)
	{
		DROIDEKA_Hunt(visible, advance);
		return;
	}

	NPC_FaceEnemy(qtrue);

	DROIDEKA_Ranged(visible, advance, altAttack);

	if (NPCInfo->touchedByPlayer != nullptr && NPCInfo->touchedByPlayer == NPC->enemy)
	{
		//touched enemy
		if (NPC->client->ps.powerups[PW_GALAK_SHIELD] > 0)
		{
			//zap him!
			//animate me
			NPC_SetAnim(NPC, SETANIM_BOTH, BOTH_ATTACK1, SETANIM_FLAG_OVERRIDE | SETANIM_FLAG_HOLD);
			TIMER_Set(NPC, "attackDelay", NPC->client->ps.torsoAnimTimer);
			NPCInfo->touchedByPlayer = nullptr;
			NPC->client->ps.powerups[PW_BATTLESUIT] = level.time + SHIELD_EFFECT_TIME;
			vec3_t smack_dir;
			VectorSubtract(NPC->enemy->currentOrigin, NPC->currentOrigin, smack_dir);
			smack_dir[2] += 30;
			VectorNormalize(smack_dir);
			G_Damage(NPC->enemy, NPC, NPC, smack_dir, NPC->currentOrigin, (g_spskill->integer + 1) * Q_irand(2, 5),
				DAMAGE_NO_KNOCKBACK, MOD_ELECTROCUTE);
			g_throw(NPC->enemy, smack_dir, 50);
			NPC->enemy->s.powerups |= 1 << PW_SHOCKED;
			if (NPC->enemy->client)
			{
				NPC->enemy->client->ps.powerups[PW_SHOCKED] = level.time + 1000;
			}
			//stop any attacks
			ucmd.buttons = 0;
		}
	}
}

/*
-------------------------
DROIDEKA_Patrol
-------------------------
*/
void DROIDEKA_Patrol(void)
{
	if (NPC_CheckPlayerTeamStealth())
	{
		NPC_UpdateAngles(qtrue, qtrue);
		return;
	}
	//If we have somewhere to go, then do that
	if (!NPC->enemy)
	{
		if (UpdateGoal())
		{
			ucmd.buttons |= BUTTON_WALKING;
			NPC_MoveToGoal(qtrue);
			NPC_UpdateAngles(qtrue, qtrue);
		}
	}
}

/*
-------------------------
NPC_BSDROIDEKA_Default
-------------------------
*/
void NPC_BSDROIDEKA_Default(void)
{
	if (NPCInfo->scriptFlags & SCF_FIRE_WEAPON)
	{
		WeaponThink(qtrue);
	}

	if (NPC->client->ps.stats[STAT_ARMOR] <= 0)
	{
		//armor gone
		if (!NPCInfo->investigateDebounceTime)
		{
			//start regenerating the armor
			gi.G2API_SetSurfaceOnOff(&NPC->ghoul2[NPC->playerModel], "torso_shield_off", TURN_OFF);
			NPC->flags &= ~FL_SHIELDED; //no more reflections
			VectorSet(NPC->mins, -10, -10, -14);
			VectorSet(NPC->maxs, 10, 10, 34);
			NPC->client->crouchheight = NPC->client->standheight = 64;
			if (NPC->locationDamage[HL_GENERIC1] < GENERATOR_HEALTH)
			{
				//still have the generator bolt-on
				if (NPCInfo->investigateCount < 12)
				{
					NPCInfo->investigateCount++;
				}
				NPCInfo->investigateDebounceTime = level.time + NPCInfo->investigateCount * 5000;
			}
		}
		else if (NPCInfo->investigateDebounceTime < level.time)
		{
			//armor regenerated, turn shield back on
			//do a trace and make sure we can turn this back on?
			trace_t tr;
			gi.trace(&tr, NPC->currentOrigin, shieldMins, shieldMaxs, NPC->currentOrigin, NPC->s.number, NPC->clipmask,
				G2_NOCOLLIDE, 0);
			if (!tr.startsolid)
			{
				VectorCopy(shieldMins, NPC->mins);
				VectorCopy(shieldMaxs, NPC->maxs);
				NPC->client->crouchheight = NPC->client->standheight = shieldMaxs[2];
				NPC->client->ps.stats[STAT_ARMOR] = DROIDEKA_SHIELD_HEALTH;
				NPCInfo->investigateDebounceTime = 0;
				NPC->flags |= FL_SHIELDED; //reflect normal shots
				NPC->fx_time = level.time;
				gi.G2API_SetSurfaceOnOff(&NPC->ghoul2[NPC->playerModel], "torso_shield_off", TURN_ON);
			}
		}
	}
	if (NPC->client->ps.stats[STAT_ARMOR] > 0)
	{
		//armor present
		NPC->client->ps.powerups[PW_GALAK_SHIELD] = Q3_INFINITE; //temp, for effect
		gi.G2API_SetSurfaceOnOff(&NPC->ghoul2[NPC->playerModel], "torso_shield_off", TURN_ON);
	}
	else
	{
		gi.G2API_SetSurfaceOnOff(&NPC->ghoul2[NPC->playerModel], "torso_shield_off", TURN_OFF);
	}

	if (!NPC->enemy)
	{
		//don't have an enemy, look for one
		DROIDEKA_Patrol();
	}
	else
	{
		if (NPCInfo->scriptFlags & SCF_CHASE_ENEMIES)
		{
			NPCInfo->goalEntity = NPC->enemy;
		}
		droideka_attack();
	}
}