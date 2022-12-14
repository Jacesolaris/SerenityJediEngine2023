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
#include "b_local.h"
#include "wp_saber.h"
#include "g_vehicles.h"
#include "../qcommon/tri_coll_test.h"
#include "../cgame/cg_local.h"

//////////Defines////////////////
extern qboolean PM_KickingAnim(int anim);
extern qboolean BG_SaberInNonIdleDamageMove(const playerState_t* ps);
extern qboolean in_front(vec3_t spot, vec3_t from, vec3_t from_angles, float thresh_hold = 0.0f);
extern qboolean PM_SaberInKnockaway(int move);
extern qboolean PM_SaberInBounce(int move);
extern qboolean BG_InSlowBounce(const playerState_t* ps);
extern qboolean G_ControlledByPlayer(const gentity_t* self);
extern qboolean PM_InKnockDown(const playerState_t* ps);
extern qboolean PM_SaberInTransitionAny(int move);
extern void G_AddVoiceEvent(const gentity_t* self, int event, int speakDebounceTime);
extern qboolean npc_is_dark_jedi(const gentity_t* self);
extern saberMoveName_t pm_broken_parry_for_attack(int move);
extern qboolean PM_InGetUp(const playerState_t* ps);
extern qboolean PM_InForceGetUp(const playerState_t* ps);
extern qboolean PM_SaberInParry(int move);
extern saberMoveName_t PM_KnockawayForParry(int move);
extern int G_KnockawayForParry(int move);
extern qboolean PM_VelocityForBlockedMove(const playerState_t* ps, vec3_t throwDir);
extern void PM_VelocityForSaberMove(const playerState_t* ps, vec3_t throw_dir);
extern qboolean WP_SaberLose(gentity_t* self, vec3_t throw_dir);
extern cvar_t* g_saberAutoBlocking;
extern qboolean WP_SabersCheckLock(gentity_t* ent1, gentity_t* ent2);
extern qboolean walk_check(const gentity_t* self);
extern void G_Knockdown(gentity_t* self, gentity_t* attacker, const vec3_t push_dir, float strength,
	qboolean break_saber_lock);
extern qboolean PM_SuperBreakWinAnim(int anim);
extern void PM_AddFatigue(playerState_t* ps, int Fatigue);
extern qboolean WP_SaberBlockNonRandom(gentity_t* self, vec3_t hitloc, qboolean missileBlock);
extern saberMoveName_t PM_BrokenParryForParry(int move);
extern void PM_AddBlockFatigue(playerState_t* ps, int Fatigue);
extern void G_Stagger(gentity_t* hit_ent);
extern void G_FatigueBPKnockaway(gentity_t* blocker);
extern void G_StaggerAttacker(gentity_t* atk);
extern void G_BounceAttacker(gentity_t* atk);
extern void wp_block_points_regenerate(const gentity_t* self, int override_amt);
extern saberMoveName_t PM_SaberBounceForAttack(int move);
extern cvar_t* g_SaberPerfectBlockingTimer;
extern cvar_t* g_SaberPerfectBlockingwaitTimer;
extern void WP_SaberDrop(const gentity_t* self, gentity_t* saber);
extern qboolean PM_SaberInnonblockableAttack(int anim);
extern qboolean pm_saber_in_special_attack(int anim);
extern qboolean PM_SaberInKata(saberMoveName_t saberMove);
extern void wp_saber_clear_damage_for_ent_num(gentity_t* attacker, int entity_num, int saber_num, int blade_num);
extern cvar_t* d_slowmoaction;
extern void G_StartStasisEffect(const gentity_t* ent, int me_flags = 0, int length = 1000, float time_scale = 0.0f,
	int spin_time = 0);
extern void CGCam_BlockShakeSP(float intensity, int duration);
extern int G_GetParryForBlock(int block);
extern qboolean WP_SaberDisarmed(gentity_t* self, vec3_t throw_dir);
extern void wp_block_points_regenerate_over_ride(const gentity_t* self, int override_amt);
// Saber Blocks
extern qboolean WP_SaberMBlock(gentity_t* victim, gentity_t* attacker, int saber_num, int blade_num);
extern qboolean WP_SaberParry(gentity_t* blocker, gentity_t* attacker, int saber_num, int blade_num);
extern qboolean WP_SaberBlockedBounceBlock(gentity_t* blocker, gentity_t* attacker, int saber_num, int blade_num);
extern qboolean WP_SaberFatiguedParry(gentity_t* blocker, gentity_t* attacker, int saber_num, int blade_num);
void SabBeh_AnimateHeavySlowBounceAttacker(gentity_t* attacker);
extern cvar_t* g_DebugSaberCombat;
extern qboolean PM_InSaberLock(int anim);
///////////Defines////////////////

//////////Actions////////////////

void SabBeh_SaberShouldBeDisarmedAttacker(gentity_t* attacker, const int saber_num)
{
	if (saber_num == 0)
	{
		//can only lose right-hand saber for now
		if (!(attacker->client->ps.saber[saber_num].saberFlags & SFL_NOT_DISARMABLE))
		{
			//knocked the saber right out of his hand!
			vec3_t throwDir = { 0, 0, 350 };

			G_Stagger(attacker);

			WP_SaberDisarmed(attacker, throwDir);
		}
	}
}

void SabBeh_SaberShouldBeDisarmedBlocker(gentity_t* blocker, const int saber_num)
{
	if (saber_num == 0)
	{
		//can only lose right-hand saber for now
		if (!(blocker->client->ps.saber[saber_num].saberFlags & SFL_NOT_DISARMABLE))
		{
			//knocked the saber right out of his hand!
			vec3_t throwDir = { 0, 0, 350 };

			G_Stagger(blocker);

			WP_SaberDisarmed(blocker, throwDir);
		}
	}
}

qboolean g_accurate_blocking(const gentity_t* self, const gentity_t* attacker, vec3_t hit_loc)
{
	//determines if self (who is blocking) is actively blocking (parrying)
	vec3_t p_angles;
	vec3_t p_right;
	vec3_t parrier_move;
	vec3_t hit_pos;
	vec3_t hit_flat; //flatten 2D version of the hitPos.
	const qboolean in_front_of_me = in_front(attacker->client->ps.origin, self->client->ps.origin,
		self->client->ps.viewangles, 0.0f);

	if (self->s.number < MAX_CLIENTS)
	{
		if (!(self->client->ps.ManualBlockingFlags & 1 << HOLDINGBLOCK))
		{
			return qfalse;
		}
	}

	if (!in_front_of_me)
	{
		//can't parry attacks to the rear.
		return qfalse;
	}
	if (PM_SaberInKnockaway(self->client->ps.saberMove))
	{
		//already in parry move, continue parrying anything that hits us as long as
		//the attacker is in the same general area that we're facing.
		return qtrue;
	}

	if (PM_KickingAnim(self->client->ps.legsAnim))
	{
		//can't parry in kick.
		return qfalse;
	}

	if (BG_SaberInNonIdleDamageMove(&self->client->ps)
		|| PM_SaberInBounce(self->client->ps.saberMove) || BG_InSlowBounce(&self->client->ps))
	{
		//can't parry if we're transitioning into a block from an attack state.
		return qfalse;
	}

	if (self->client->ps.pm_flags & PMF_DUCKED)
	{
		//can't parry while ducked or running
		return qfalse;
	}

	if (PM_InKnockDown(&self->client->ps))
	{
		//can't block while knocked down or getting up from knockdown, or we are staggered.
		return qfalse;
	}

	//set up flatten version of the location of the incoming attack in orientation
	//to the player.
	VectorSubtract(hit_loc, self->client->ps.origin, hit_pos);
	VectorSet(p_angles, 0, self->client->ps.viewangles[YAW], 0);
	AngleVectors(p_angles, nullptr, p_right, nullptr);
	hit_flat[0] = 0;
	hit_flat[1] = DotProduct(p_right, hit_pos);

	//just bump the hit pos down for the blocking since the average left/right slice happens at about origin +10
	hit_flat[2] = hit_pos[2] - 10;
	VectorNormalize(hit_flat);

	//set up the vector for the direction the player is trying to parry in.
	parrier_move[0] = 0;
	parrier_move[1] = self->client->pers.cmd.rightmove;
	parrier_move[2] = -self->client->pers.cmd.forwardmove;
	VectorNormalize(parrier_move);

	const float block_dot = DotProduct(hit_flat, parrier_move);

	if (block_dot >= .4)
	{
		//player successfully blocked in the right direction to do a full parry.
		return qtrue;
	}
	//player didn't parry in the correct direction, do blockPoints punishment
	if (self->NPC)
	{
		//bots just randomly parry to make up for them not intelligently parrying.
		if (NPC_PARRYRATE * g_spskill->integer > Q_irand(0, 999))
		{
			return qtrue;
		}
	}
	return qfalse;
}

void SabBeh_AddMishap_Attacker(gentity_t* attacker, const int saber_num)
{
	if (attacker->client->ps.blockPoints <= MISHAPLEVEL_NONE)
	{
		attacker->client->ps.blockPoints = MISHAPLEVEL_NONE;
	}
	else if (attacker->client->ps.saberFatigueChainCount <= MISHAPLEVEL_NONE)
	{
		attacker->client->ps.saberFatigueChainCount = MISHAPLEVEL_NONE;
	}
	else
	{
		//overflowing causes a full mishap.
		const int rand_num = Q_irand(0, 2);

		switch (rand_num)
		{
		case 0:
			if (attacker->NPC && !G_ControlledByPlayer(attacker)) //NPC only
			{
				if (!Q_irand(0, 4))
				{//20% chance
					SabBeh_AnimateHeavySlowBounceAttacker(attacker);
					if (d_attackinfo->integer || g_DebugSaberCombat->integer && (attacker->NPC && !G_ControlledByPlayer(attacker)))
					{
						gi.Printf(S_COLOR_YELLOW"NPC Attacker staggering\n");
					}
				}
				else
				{
					SabBeh_SaberShouldBeDisarmedAttacker(attacker, saber_num);
					if (d_attackinfo->integer || g_DebugSaberCombat->integer && (attacker->NPC && !G_ControlledByPlayer(attacker)))
					{
						gi.Printf(S_COLOR_RED"NPC Attacker lost his saber\n");
					}
				}
			}
			else
			{
				SabBeh_SaberShouldBeDisarmedAttacker(attacker, saber_num);
				if (d_attackinfo->integer || g_DebugSaberCombat->integer && (attacker->s.number < MAX_CLIENTS || G_ControlledByPlayer(attacker)))
				{
					gi.Printf(S_COLOR_RED"Player Attacker lost his saber\n");
				}
			}
			break;
		case 1:
			SabBeh_AnimateHeavySlowBounceAttacker(attacker);
			if (d_attackinfo->integer || g_DebugSaberCombat->integer && (attacker->s.number < MAX_CLIENTS || G_ControlledByPlayer(attacker)))
			{
				gi.Printf(S_COLOR_RED"Player Attacker staggering\n");
			}
			break;
		default:;
		}
	}
}

void SabBeh_AddMishap_Blocker(gentity_t* blocker, const int saber_num)
{
	if (blocker->client->ps.blockPoints <= MISHAPLEVEL_NONE)
	{
		blocker->client->ps.blockPoints = MISHAPLEVEL_NONE;
	}
	else if (blocker->client->ps.saberFatigueChainCount <= MISHAPLEVEL_NONE)
	{
		blocker->client->ps.saberFatigueChainCount = MISHAPLEVEL_NONE;
	}
	else
	{
		//overflowing causes a full mishap.
		const int rand_num = Q_irand(0, 2);

		switch (rand_num)
		{
		case 0:
			G_Stagger(blocker);
			if (d_blockinfo->integer || g_DebugSaberCombat->integer)
			{
				gi.Printf(S_COLOR_RED"blocker staggering\n");
			}
			break;
		case 1:
			if (blocker->NPC && !G_ControlledByPlayer(blocker)) //NPC only
			{
				if (!Q_irand(0, 4))
				{//20% chance
					G_Stagger(blocker);
					if (d_blockinfo->integer || g_DebugSaberCombat->integer)
					{
						gi.Printf(S_COLOR_RED"NPC blocker staggering\n");
					}
				}
				else
				{
					SabBeh_SaberShouldBeDisarmedBlocker(blocker, saber_num);
					wp_block_points_regenerate_over_ride(blocker, BLOCKPOINTS_FATIGUE);
					if (d_blockinfo->integer || g_DebugSaberCombat->integer)
					{
						gi.Printf(S_COLOR_RED"NPC blocker lost his saber\n");
					}
				}
			}
			else
			{
				SabBeh_SaberShouldBeDisarmedBlocker(blocker, saber_num);
				if (d_blockinfo->integer || g_DebugSaberCombat->integer)
				{
					gi.Printf(S_COLOR_RED"blocker lost his saber\n");
				}
			}
			break;
		default:;
		}
	}
}

////////Attacker Bounces//////////

void SabBeh_AnimateHeavySlowBounceAttacker(gentity_t* attacker)
{
	attacker->client->ps.userInt3 |= 1 << FLAG_SLOWBOUNCE;
	attacker->client->ps.userInt3 |= 1 << FLAG_OLDSLOWBOUNCE;
	G_StaggerAttacker(attacker);
}

void SabBeh_AnimateSmallBounce(gentity_t* attacker)
{
	if (attacker->NPC && !G_ControlledByPlayer(attacker)) //NPC only
	{
		attacker->client->ps.userInt3 |= 1 << FLAG_SLOWBOUNCE;
		attacker->client->ps.userInt3 |= 1 << FLAG_OLDSLOWBOUNCE;
		G_BounceAttacker(attacker);
	}
	else
	{
		attacker->client->ps.userInt3 |= 1 << FLAG_SLOWBOUNCE;
		attacker->client->ps.saberBounceMove = LS_D1_BR + (saberMoveData[attacker->client->ps.saberMove].startQuad - Q_BR);
		attacker->client->ps.saberBlocked = BLOCKED_ATK_BOUNCE;
	}
}

////////Blocker Bounces//////////

void SabBeh_AnimateSlowBounceBlocker(gentity_t* blocker)
{
	blocker->client->ps.userInt3 |= 1 << FLAG_SLOWBOUNCE;
	blocker->client->ps.userInt3 |= 1 << FLAG_OLDSLOWBOUNCE;

	G_AddEvent(blocker, Q_irand(EV_PUSHED1, EV_PUSHED3), 0);

	blocker->client->ps.saberBounceMove = PM_BrokenParryForParry(G_GetParryForBlock(blocker->client->ps.saberBlocked));
	blocker->client->ps.saberBlocked = BLOCKED_PARRY_BROKEN;
}

////////Bounces//////////

qboolean SabBeh_Attack_Blocked(gentity_t* attacker, gentity_t* blocker, const int saber_num, const qboolean force_mishap)
{
	//if the attack is blocked -(Im the attacker)
	const qboolean m_blocking = blocker->client->ps.ManualBlockingFlags & 1 << PERFECTBLOCKING ? qtrue : qfalse;	//perfect Blocking (Timed Block)

	if (attacker->client->ps.saberFatigueChainCount >= MISHAPLEVEL_MAX)
	{
		//hard mishap.

		if (attacker->NPC && !G_ControlledByPlayer(attacker)) //NPC only
		{
			if (!Q_irand(0, 4))
			{//20% chance
				SabBeh_AddMishap_Attacker(attacker, saber_num);
			}
			else
			{
				SabBeh_AnimateHeavySlowBounceAttacker(attacker);
			}
			if (d_attackinfo->integer || g_DebugSaberCombat->integer)
			{
				gi.Printf(S_COLOR_GREEN"Attacker npc is fatigued\n");
			}

			attacker->client->ps.saberFatigueChainCount = MISHAPLEVEL_MIN;
		}
		else
		{
			if (d_attackinfo->integer || g_DebugSaberCombat->integer)
			{
				gi.Printf(S_COLOR_GREEN"Attacker player is fatigued\n");
			}
			SabBeh_AddMishap_Attacker(attacker, saber_num);
		}
		return qtrue;
	}
	if (attacker->client->ps.saberFatigueChainCount >= MISHAPLEVEL_HUDFLASH)
	{
		//slow bounce
		if (attacker->s.number < MAX_CLIENTS || G_ControlledByPlayer(attacker))
		{
			SabBeh_AnimateHeavySlowBounceAttacker(attacker);
		}
		else
		{
			SabBeh_AnimateSmallBounce(attacker);
		}

		if (attacker->NPC && !G_ControlledByPlayer(attacker)) //NPC only
		{
			attacker->client->ps.saberFatigueChainCount = MISHAPLEVEL_LIGHT;
		}

		if (d_attackinfo->integer || g_DebugSaberCombat->integer)
		{
			if (attacker->s.number < MAX_CLIENTS || G_ControlledByPlayer(attacker))
			{
				gi.Printf(S_COLOR_GREEN"player attack stagger\n");
			}
			else
			{
				gi.Printf(S_COLOR_GREEN"npc attack stagger\n");
			}
		}
		return qtrue;
	}
	if (attacker->client->ps.saberFatigueChainCount >= MISHAPLEVEL_LIGHT)
	{
		//slow bounce
		SabBeh_AnimateSmallBounce(attacker);

		if (d_attackinfo->integer || g_DebugSaberCombat->integer)
		{
			if (attacker->s.number < MAX_CLIENTS || G_ControlledByPlayer(attacker))
			{
				gi.Printf(S_COLOR_GREEN"player light blocked bounce\n");
			}
			else
			{
				gi.Printf(S_COLOR_GREEN"npc light blocked bounce\n");
			}
		}
		return qtrue;
	}
	if (force_mishap)
	{
		//two attacking sabers bouncing off each other
		SabBeh_AnimateSmallBounce(attacker);
		SabBeh_AnimateSmallBounce(blocker);

		if (d_attackinfo->integer || g_DebugSaberCombat->integer)
		{
			if (attacker->s.number < MAX_CLIENTS || G_ControlledByPlayer(attacker))
			{
				gi.Printf(S_COLOR_GREEN"player two attacking sabers bouncing off each other\n");
			}
			else
			{
				gi.Printf(S_COLOR_GREEN"npc two attacking sabers bouncing off each other\n");
			}
		}
		return qtrue;
	}
	if (!m_blocking)
	{
		if (d_attackinfo->integer || g_DebugSaberCombat->integer)
		{
			if (attacker->s.number < MAX_CLIENTS || G_ControlledByPlayer(attacker))
			{
				gi.Printf(S_COLOR_GREEN"player blocked bounce\n");
			}
			else
			{
				gi.Printf(S_COLOR_GREEN"npc blocked bounce\n");
			}
		}
		SabBeh_AnimateSmallBounce(attacker);
	}
	return qtrue;
}

void SabBeh_AddBalance(const gentity_t* self, int amount)
{
	if (!walk_check(self))
	{
		//running or moving very fast, can't balance as well
		if (amount > 0)
		{
			amount *= 2;
		}
		else
		{
			amount = amount * .5f;
		}
	}

	self->client->ps.saberFatigueChainCount += amount;

	if (self->client->ps.saberFatigueChainCount < MISHAPLEVEL_NONE)
	{
		self->client->ps.saberFatigueChainCount = MISHAPLEVEL_NONE;
	}
	else if (self->client->ps.saberFatigueChainCount >= MISHAPLEVEL_OVERLOAD)
	{
		self->client->ps.saberFatigueChainCount = MISHAPLEVEL_MAX;
	}
}

//////////Actions////////////////

/////////Functions//////////////

qboolean SabBeh_AttackVsAttack(gentity_t* attacker, gentity_t* blocker, const int saber_num)
{
	//set the saber behavior for two attacking blades hitting each other
	const qboolean atkfake = attacker->client->ps.userInt3 & 1 << FLAG_ATTACKFAKE ? qtrue : qfalse;
	const qboolean otherfake = blocker->client->ps.userInt3 & 1 << FLAG_ATTACKFAKE ? qtrue : qfalse;

	if (atkfake && !otherfake)
	{
		//self is solo faking
		//set self
		SabBeh_AddBalance(attacker, 1);
		//set otherOwner

		if (WP_SabersCheckLock(attacker, blocker))
		{
			attacker->client->ps.userInt3 |= 1 << FLAG_LOCKWINNER;
			attacker->client->ps.saberBlocked = BLOCKED_NONE;
			blocker->client->ps.saberBlocked = BLOCKED_NONE;
		}
		SabBeh_AddBalance(blocker, -1);
	}
	else if (!atkfake && otherfake)
	{
		//only otherOwner is faking
		//set self
		if (WP_SabersCheckLock(blocker, attacker))
		{
			attacker->client->ps.saberBlocked = BLOCKED_NONE;
			blocker->client->ps.userInt3 |= 1 << FLAG_LOCKWINNER;
			blocker->client->ps.saberBlocked = BLOCKED_NONE;
		}
		SabBeh_AddBalance(attacker, -1);
		//set otherOwner
		SabBeh_AddBalance(blocker, 1);
	}
	else if (PM_SaberInKata(static_cast<saberMoveName_t>(attacker->client->ps.saberMove)))
	{
		SabBeh_AddBalance(attacker, 1);
		//set otherOwner
		SabBeh_AddBalance(blocker, -1);

		if (blocker->client->ps.blockPoints < BLOCKPOINTS_TEN)
		{
			//Low points = bad blocks
			SabBeh_SaberShouldBeDisarmedBlocker(blocker, saber_num);
			wp_block_points_regenerate_over_ride(blocker, BLOCKPOINTS_FATIGUE);
		}
		else
		{
			//Low points = bad blocks
			G_Stagger(blocker);
			PM_AddBlockFatigue(&blocker->client->ps, BLOCKPOINTS_TEN);
		}
	}
	else if (PM_SaberInKata(static_cast<saberMoveName_t>(blocker->client->ps.saberMove)))
	{
		SabBeh_AddBalance(attacker, -1);
		//set otherOwner
		SabBeh_AddBalance(blocker, 1);

		if (attacker->client->ps.blockPoints < BLOCKPOINTS_TEN)
		{
			//Low points = bad blocks
			SabBeh_SaberShouldBeDisarmedAttacker(attacker, saber_num);
			wp_block_points_regenerate_over_ride(attacker, BLOCKPOINTS_FATIGUE);
		}
		else
		{
			//Low points = bad blocks
			G_Stagger(attacker);
			PM_AddBlockFatigue(&attacker->client->ps, BLOCKPOINTS_TEN);
		}
	}
	else
	{
		//either both are faking or neither is faking.  Either way, it's canceled out
		//set self
		SabBeh_AddBalance(attacker, 1);
		//set otherOwner
		SabBeh_AddBalance(blocker, 1);

		SabBeh_Attack_Blocked(attacker, blocker, saber_num, qtrue);

		SabBeh_Attack_Blocked(blocker, attacker, saber_num, qtrue);
	}
	return qtrue;
}

qboolean sab_beh_attack_vs_block(gentity_t* attacker, gentity_t* blocker, const int saber_num, const int blade_num)
{
	//if the attack is blocked -(Im the attacker)
	const qboolean blocking = blocker->client->ps.ManualBlockingFlags & 1 << HOLDINGBLOCK ? qtrue : qfalse;	//Normal Blocking (just holding block button)
	const qboolean m_blocking = blocker->client->ps.ManualBlockingFlags & 1 << PERFECTBLOCKING ? qtrue : qfalse;	//perfect Blocking (Timed Block)
	const qboolean active_blocking = blocker->client->ps.ManualBlockingFlags & 1 << HOLDINGBLOCKANDATTACK ? qtrue : qfalse;//Active Blocking (Holding Block button + Attack button)
	const qboolean npc_blocking = blocker->client->ps.ManualBlockingFlags & 1 << MBF_NPCBLOCKING ? qtrue : qfalse;//(Npc Blocking function)

	const qboolean atkfake = attacker->client->ps.userInt3 & 1 << FLAG_ATTACKFAKE ? qtrue : qfalse;

	if (PM_SaberInnonblockableAttack(attacker->client->ps.torsoAnim))
	{   //perfect Blocking
		if (m_blocking) // A perfectly timed block
		{
			SabBeh_SaberShouldBeDisarmedAttacker(attacker, saber_num);
			//just so attacker knows that he was blocked
			attacker->client->ps.saberEventFlags |= SEF_BLOCKED;
			//since it was parried, take away any damage done
			wp_saber_clear_damage_for_ent_num(attacker, blocker->s.number, saber_num, blade_num);
			PM_AddBlockFatigue(&attacker->client->ps, BLOCKPOINTS_TEN);         //BP Punish Attacker
		}
		else
		{//This must be Unblockable
			if (d_attackinfo->integer || g_DebugSaberCombat->integer)
			{
				gi.Printf(S_COLOR_MAGENTA"Attacker must be Unblockable\n");
			}
			attacker->client->ps.saberEventFlags &= ~SEF_BLOCKED;
		}
	}
	else if (BG_SaberInNonIdleDamageMove(&blocker->client->ps))
	{//and blocker is attacking
		if (d_attackinfo->integer || g_DebugSaberCombat->integer)
		{
			gi.Printf(S_COLOR_YELLOW"Both Attacker and Blocker are now attacking\n");
		}

		SabBeh_AttackVsAttack(blocker, attacker, saber_num);
	}
	else if (PM_SuperBreakWinAnim(attacker->client->ps.torsoAnim))
	{
		//attacker was attempting a superbreak and he hit someone who could block the move, rail him for screwing up.
		SabBeh_AddBalance(attacker, 1);

		SabBeh_AnimateHeavySlowBounceAttacker(attacker);

		SabBeh_AddBalance(blocker, -1);
		if (d_attackinfo->integer || g_DebugSaberCombat->integer)
		{
			gi.Printf(S_COLOR_YELLOW"Attacker Super break win / fail\n");
		}
	}
	else if (atkfake)
	{
		//attacker faked but it was blocked here
		if (blocking || m_blocking || active_blocking || npc_blocking)
		{
			//defender parried the attack fake.
			SabBeh_AddBalance(attacker, MPCOST_PARRIED_ATTACKFAKE);

			if (m_blocking || active_blocking || npc_blocking)
			{
				attacker->client->ps.userInt3 |= 1 << FLAG_BLOCKED;
			}
			else
			{
				attacker->client->ps.userInt3 |= 1 << FLAG_PARRIED;
			}

			SabBeh_AddBalance(blocker, MPCOST_PARRYING_ATTACKFAKE);

			if ((d_attackinfo->integer || g_DebugSaberCombat->integer) && !PM_InSaberLock(attacker->client->ps.torsoAnim))
			{
				gi.Printf(S_COLOR_YELLOW"Attackers Attack Fake was Blocked\n");
			}
		}
		else
		{
			//otherwise, the defender stands a good chance of having his defensive broken.
			SabBeh_AddBalance(attacker, -1);

			if (WP_SabersCheckLock(attacker, blocker))
			{
				attacker->client->ps.userInt3 |= 1 << FLAG_LOCKWINNER;
				attacker->client->ps.saberBlocked = BLOCKED_NONE;
				blocker->client->ps.saberBlocked = BLOCKED_NONE;
			}

			if (!m_blocking)
			{
				SabBeh_Attack_Blocked(attacker, blocker, saber_num, qfalse);
			}

			if (d_attackinfo->integer || g_DebugSaberCombat->integer)
			{
				gi.Printf(S_COLOR_YELLOW"Attacker Attack Fake bounced\n");
			}
		}
	}
	else
	{//standard attack.
		if (blocking || m_blocking || active_blocking || npc_blocking) // All types of active blocking
		{
			if (m_blocking || active_blocking || npc_blocking)
			{
				if (npc_blocking && blocker->client->ps.blockPoints >= BLOCKPOINTS_MISSILE
					&& attacker->client->ps.saberFatigueChainCount >= MISHAPLEVEL_HUDFLASH
					&& !Q_irand(0, 4))
				{//20% chance
					SabBeh_AnimateHeavySlowBounceAttacker(attacker);
					attacker->client->ps.userInt3 |= 1 << FLAG_MBLOCKBOUNCE;

					if (attacker->s.number < MAX_CLIENTS || G_ControlledByPlayer(attacker))
					{
						if (d_slowmoaction->integer)
						{
							G_StartStasisEffect(attacker, MEF_NO_SPIN, 200, 0.3f, 0);
						}
						CGCam_BlockShakeSP(0.45f, 100);
					}
				}
				else
				{
					attacker->client->ps.userInt3 |= 1 << FLAG_BLOCKED;
				}
			}
			else
			{
				attacker->client->ps.userInt3 |= 1 << FLAG_PARRIED;
			}

			if (!m_blocking)
			{
				SabBeh_Attack_Blocked(attacker, blocker, saber_num, qfalse);
			}

			if (d_attackinfo->integer || g_DebugSaberCombat->integer)
			{
				gi.Printf(S_COLOR_YELLOW"Attackers Attack was Blocked\n");
			}
		}
		else
		{
			//Backup in case i missed some

			if (!m_blocking)
			{
				SabBeh_Attack_Blocked(attacker, blocker, saber_num, qtrue);
				G_Stagger(blocker);
			}

			if (d_attackinfo->integer || g_DebugSaberCombat->integer)
			{
				gi.Printf(S_COLOR_ORANGE"Attacker All the rest of the types of contact\n");
			}
		}
	}
	return qtrue;
}

qboolean sab_beh_block_vs_attack(gentity_t* blocker, gentity_t* attacker, const int saber_num, const int blade_num)
{
	//-(Im the blocker)

	const qboolean blocking = blocker->client->ps.ManualBlockingFlags & 1 << HOLDINGBLOCK ? qtrue : qfalse;	//Normal Blocking
	const qboolean m_blocking = blocker->client->ps.ManualBlockingFlags & 1 << PERFECTBLOCKING ? qtrue : qfalse;//perfect Blocking
	const qboolean active_blocking = blocker->client->ps.ManualBlockingFlags & 1 << HOLDINGBLOCKANDATTACK ? qtrue : qfalse;//Active Blocking
	const qboolean npc_blocking = blocker->client->ps.ManualBlockingFlags & 1 << MBF_NPCBLOCKING ? qtrue : qfalse; //Active NPC Blocking

	if (!PM_SaberInnonblockableAttack(attacker->client->ps.torsoAnim))
	{
		if (blocker->client->ps.blockPoints <= BLOCKPOINTS_FATIGUE) // blocker has less than 20BP
		{
			if (blocker->client->ps.blockPoints <= BLOCKPOINTS_TEN) // blocker has less than 10BP
			{
				//Low points = bad blocks
				if (blocker->NPC && !G_ControlledByPlayer(blocker)) //NPC only
				{
					SabBeh_AddMishap_Blocker(blocker, saber_num);
				}
				else
				{
					SabBeh_SaberShouldBeDisarmedBlocker(blocker, saber_num);
				}

				if (attacker->NPC && !G_ControlledByPlayer(attacker)) //NPC only
				{
					wp_block_points_regenerate(attacker, BLOCKPOINTS_FATIGUE);
				}
				else
				{
					if (!blocker->client->ps.saberInFlight)
					{
						wp_block_points_regenerate(blocker, BLOCKPOINTS_FATIGUE);
					}
				}

				if ((d_blockinfo->integer || g_DebugSaberCombat->integer) && blocker->s.number < MAX_CLIENTS || G_ControlledByPlayer(blocker))
				{
					gi.Printf(S_COLOR_CYAN"Blocker was disarmed with very low bp, recharge bp 20bp\n");
				}

				//just so blocker knows that he has parried the attacker
				blocker->client->ps.saberEventFlags |= SEF_PARRIED;
				//just so attacker knows that he was blocked
				attacker->client->ps.saberEventFlags |= SEF_BLOCKED;
				//since it was parried, take away any damage done
				wp_saber_clear_damage_for_ent_num(attacker, blocker->s.number, saber_num, blade_num);
			}
			else
			{
				//Low points = bad blocks
				G_FatigueBPKnockaway(blocker);

				PM_AddBlockFatigue(&blocker->client->ps, BLOCKPOINTS_DANGER);

				if ((d_blockinfo->integer || g_DebugSaberCombat->integer) && (blocker->s.number < MAX_CLIENTS || G_ControlledByPlayer(blocker)))
				{
					gi.Printf(S_COLOR_CYAN"Blocker stagger drain 4 bp\n");
				}

				//just so blocker knows that he has parried the attacker
				blocker->client->ps.saberEventFlags |= SEF_PARRIED;
				//just so attacker knows that he was blocked
				attacker->client->ps.saberEventFlags |= SEF_BLOCKED;
				//since it was parried, take away any damage done
				wp_saber_clear_damage_for_ent_num(attacker, blocker->s.number, saber_num, blade_num);
			}
		}
		else
		{//just block it //jacesolaris
			if (active_blocking) //Holding Block Button + attack button
			{   //perfect Blocking
				if (m_blocking) // A perfectly timed block
				{
					WP_SaberMBlock(blocker, attacker, saber_num, blade_num);

					if (blocker->client->ps.blockPoints <= BLOCKPOINTS_FATIGUE)
					{
						SabBeh_SaberShouldBeDisarmedAttacker(attacker, saber_num);
					}
					else
					{
						SabBeh_AnimateHeavySlowBounceAttacker(attacker);
						attacker->client->ps.userInt3 |= 1 << FLAG_MBLOCKBOUNCE;
					}

					if (blocker->s.number < MAX_CLIENTS || G_ControlledByPlayer(blocker))
					{
						if (d_slowmoaction->integer)
						{
							G_StartStasisEffect(blocker, MEF_NO_SPIN, 200, 0.3f, 0);
						}
						CGCam_BlockShakeSP(0.45f, 100);
					}

					G_Sound(blocker, G_SoundIndex(va("sound/weapons/saber/saber_perfectblock%d.mp3", Q_irand(1, 3))));

					if ((d_blockinfo->integer || g_DebugSaberCombat->integer) && blocker->s.number < MAX_CLIENTS || G_ControlledByPlayer(blocker))
					{
						gi.Printf(S_COLOR_CYAN"Blocker Perfect blocked reward 20\n");
					}

					//just so blocker knows that he has parried the attacker
					blocker->client->ps.saberEventFlags |= SEF_PARRIED;
					//just so attacker knows that he was blocked
					attacker->client->ps.saberEventFlags |= SEF_BLOCKED;
					//since it was parried, take away any damage done
					wp_saber_clear_damage_for_ent_num(attacker, blocker->s.number, saber_num, blade_num);

					wp_block_points_regenerate_over_ride(blocker, BLOCKPOINTS_FATIGUE); //BP Reward blocker
					blocker->client->ps.saberFatigueChainCount = MISHAPLEVEL_NONE;       //SAC Reward blocker
					PM_AddBlockFatigue(&attacker->client->ps, BLOCKPOINTS_TEN);         //BP Punish Attacker
				}
				else
				{
					//Spamming block + attack buttons
					if (blocker->client->ps.blockPoints <= BLOCKPOINTS_HALF)
					{
						WP_SaberFatiguedParry(blocker, attacker, saber_num, blade_num);
					}
					else
					{
						if (attacker->client->ps.saberAnimLevel == SS_DESANN || attacker->client->ps.saberAnimLevel == SS_STRONG)
						{
							WP_SaberFatiguedParry(blocker, attacker, saber_num, blade_num);
						}
						else
						{
							WP_SaberParry(blocker, attacker, saber_num, blade_num);
						}
					}

					if (attacker->NPC && !G_ControlledByPlayer(attacker)) //NPC only
					{
						PM_AddBlockFatigue(&attacker->client->ps, BLOCKPOINTS_THREE);
					}

					PM_AddBlockFatigue(&blocker->client->ps, BLOCKPOINTS_FIVE);

					if (blocker->s.number < MAX_CLIENTS || G_ControlledByPlayer(blocker))
					{
						if (d_slowmoaction->integer)
						{
							G_StartStasisEffect(blocker, MEF_NO_SPIN, 200, 0.3f, 0);
						}
						CGCam_BlockShakeSP(0.45f, 100);
					}

					if ((d_blockinfo->integer || g_DebugSaberCombat->integer) && blocker->s.number < MAX_CLIENTS || G_ControlledByPlayer(blocker))
					{
						gi.Printf(S_COLOR_CYAN"Blocker Spamming block + attack cost 5\n");
					}

					//just so blocker knows that he has parried the attacker
					blocker->client->ps.saberEventFlags |= SEF_PARRIED;
					//just so attacker knows that he was blocked
					attacker->client->ps.saberEventFlags |= SEF_BLOCKED;
					//since it was parried, take away any damage done
					wp_saber_clear_damage_for_ent_num(attacker, blocker->s.number, saber_num, blade_num);
				}
			}
			else if (blocking && !active_blocking) //Holding block button only (spamming block)
			{
				if (blocker->client->ps.blockPoints <= BLOCKPOINTS_HALF)
				{
					WP_SaberFatiguedParry(blocker, attacker, saber_num, blade_num);
				}
				else
				{
					if (attacker->client->ps.saberAnimLevel == SS_DESANN || attacker->client->ps.saberAnimLevel == SS_STRONG)
					{
						WP_SaberFatiguedParry(blocker, attacker, saber_num, blade_num);
					}
					else
					{
						WP_SaberBlockedBounceBlock(blocker, attacker, saber_num, blade_num);
					}
				}

				if (blocker->s.number < MAX_CLIENTS || G_ControlledByPlayer(blocker))
				{
					if (d_slowmoaction->integer)
					{
						G_StartStasisEffect(blocker, MEF_NO_SPIN, 200, 0.3f, 0);
					}
					CGCam_BlockShakeSP(0.45f, 100);
				}

				if (blocker->NPC && !G_ControlledByPlayer(blocker)) //NPC only
				{
					//
				}
				else
				{
					PM_AddBlockFatigue(&blocker->client->ps, BLOCKPOINTS_TEN);
				}
				if ((d_blockinfo->integer || g_DebugSaberCombat->integer) && blocker->s.number < MAX_CLIENTS || G_ControlledByPlayer(blocker))
				{
					gi.Printf(S_COLOR_CYAN"Blocker Holding block button only (spamming block) cost 10\n");
				}

				//just so blocker knows that he has parried the attacker
				blocker->client->ps.saberEventFlags |= SEF_PARRIED;
				//just so attacker knows that he was blocked
				attacker->client->ps.saberEventFlags |= SEF_BLOCKED;
				//since it was parried, take away any damage done
				wp_saber_clear_damage_for_ent_num(attacker, blocker->s.number, saber_num, blade_num);
			}
			else if (npc_blocking || (g_saberAutoBlocking->integer && blocker->NPC && !G_ControlledByPlayer(blocker) || blocker->client->ps.saberBlockingTime > level.time && blocker->NPC && !G_ControlledByPlayer(blocker))) //Other types and npc,s
			{
				if (blocker->NPC && !G_ControlledByPlayer(blocker)) //NPC only
				{
					if (attacker->client->ps.saberAnimLevel == SS_DESANN || attacker->client->ps.saberAnimLevel == SS_STRONG)
					{
						WP_SaberFatiguedParry(blocker, attacker, saber_num, blade_num);
					}
					else
					{
						if (blocker->client->ps.blockPoints <= BLOCKPOINTS_MISSILE)
						{
							WP_SaberParry(blocker, attacker, saber_num, blade_num);

							if ((d_blockinfo->integer || g_DebugSaberCombat->integer) && (blocker->NPC && !G_ControlledByPlayer(blocker)))
							{
								gi.Printf(S_COLOR_CYAN"NPC normal Parry\n");
							}
						}
						else if (blocker->client->ps.blockPoints <= BLOCKPOINTS_HALF)
						{
							WP_SaberFatiguedParry(blocker, attacker, saber_num, blade_num);

							if ((d_blockinfo->integer || g_DebugSaberCombat->integer) && (blocker->NPC && !G_ControlledByPlayer(blocker)))
							{
								gi.Printf(S_COLOR_CYAN"NPC Fatigued Parry\n");
							}
						}
						else
						{
							WP_SaberMBlock(blocker, attacker, saber_num, blade_num);

							if ((d_blockinfo->integer || g_DebugSaberCombat->integer) && (blocker->NPC && !G_ControlledByPlayer(blocker)))
							{
								gi.Printf(S_COLOR_CYAN"NPC good Parry\n");
							}
						}
					}
				}
				else
				{
					WP_SaberMBlock(blocker, attacker, saber_num, blade_num);
				}

				G_Sound(blocker, G_SoundIndex(va("sound/weapons/saber/saber_goodparry%d.mp3", Q_irand(1, 3))));

				PM_AddBlockFatigue(&blocker->client->ps, BLOCKPOINTS_THREE);
				if ((d_blockinfo->integer || g_DebugSaberCombat->integer) && blocker->s.number < MAX_CLIENTS || G_ControlledByPlayer(blocker))
				{
					gi.Printf(S_COLOR_CYAN"Blocker Other types of block and npc,s\n");
				}

				//just so blocker knows that he has parried the attacker
				blocker->client->ps.saberEventFlags |= SEF_PARRIED;
				//just so attacker knows that he was blocked
				attacker->client->ps.saberEventFlags |= SEF_BLOCKED;
				//since it was parried, take away any damage done
				wp_saber_clear_damage_for_ent_num(attacker, blocker->s.number, saber_num, blade_num);
			}
			else
			{
				SabBeh_AddMishap_Blocker(blocker, saber_num);

				if (blocker->NPC && !G_ControlledByPlayer(blocker)) //NPC only
				{
					//
				}
				else
				{
					PM_AddBlockFatigue(&blocker->client->ps, BLOCKPOINTS_TEN);
				}
				if ((d_blockinfo->integer || g_DebugSaberCombat->integer) && blocker->s.number < MAX_CLIENTS || G_ControlledByPlayer(blocker))
				{
					gi.Printf(S_COLOR_CYAN"Blocker Not holding block drain 10\n");
				}
			}
		}
	}
	else
	{   //perfect Blocking
		if (m_blocking) // A perfectly timed block
		{
			if (blocker->s.number < MAX_CLIENTS || G_ControlledByPlayer(blocker))
			{
				if (d_slowmoaction->integer)
				{
					G_StartStasisEffect(blocker, MEF_NO_SPIN, 200, 0.3f, 0);
				}
				CGCam_BlockShakeSP(0.45f, 100);
			}

			G_Sound(blocker, G_SoundIndex(va("sound/weapons/saber/saber_perfectblock%d.mp3", Q_irand(1, 3))));

			if ((d_blockinfo->integer || g_DebugSaberCombat->integer) && blocker->s.number < MAX_CLIENTS || G_ControlledByPlayer(blocker))
			{
				gi.Printf(S_COLOR_MAGENTA"Blocker Perfect blocked an Unblockable attack reward 20\n");
			}

			//just so blocker knows that he has parried the attacker
			blocker->client->ps.saberEventFlags |= SEF_PARRIED;

			wp_block_points_regenerate_over_ride(blocker, BLOCKPOINTS_FATIGUE); //BP Reward blocker
			blocker->client->ps.saberFatigueChainCount = MISHAPLEVEL_NONE;       //SAC Reward blocker
		}
		else
		{//This must be Unblockable
			if (blocker->client->ps.blockPoints < BLOCKPOINTS_TEN)
			{
				//Low points = bad blocks
				SabBeh_SaberShouldBeDisarmedBlocker(blocker, saber_num);
				wp_block_points_regenerate_over_ride(blocker, BLOCKPOINTS_FATIGUE);
			}
			else
			{
				//Low points = bad blocks
				G_FatigueBPKnockaway(blocker);
				PM_AddBlockFatigue(&blocker->client->ps, BLOCKPOINTS_TEN);
			}
			if (d_blockinfo->integer || g_DebugSaberCombat->integer)
			{
				gi.Printf(S_COLOR_MAGENTA"Blocker can not block Unblockable\n");
			}
			blocker->client->ps.saberEventFlags &= ~SEF_PARRIED;
		}
	}
	return qtrue;
}

/////////Functions//////////////