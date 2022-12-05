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
#include "anims.h"
#include "b_local.h"
#include "g_functions.h"
#include "wp_saber.h"
#include "g_vehicles.h"
#include "../qcommon/tri_coll_test.h"
#include "../cgame/cg_local.h"

#define JK2_RAGDOLL_GRIPNOHEALTH

constexpr auto MAX_SABER_VICTIMS = 8192;
static int victimEntityNum[MAX_SABER_VICTIMS];
static float totalDmg[MAX_SABER_VICTIMS];
static vec3_t dmgDir[MAX_SABER_VICTIMS];
static vec3_t dmgNormal[MAX_SABER_VICTIMS];
static vec3_t dmgBladeVec[MAX_SABER_VICTIMS];
static vec3_t dmgSpot[MAX_SABER_VICTIMS];
static float dmgFraction[MAX_SABER_VICTIMS];
static int hitLoc[MAX_SABER_VICTIMS];
static qboolean hitDismember[MAX_SABER_VICTIMS];
static int hitDismemberLoc[MAX_SABER_VICTIMS];
static vec3_t saberHitLocation, saberHitNormal = { 0, 0, 1.0 };
static float saberHitFraction;
static float sabersCrossed;
static int saberHitEntity;
static int numVictims = 0;
extern cvar_t* com_outcast;
extern cvar_t* g_dismemberProbabilities;
extern cvar_t* g_sex;
extern cvar_t* g_timescale;
extern cvar_t* g_dismemberment;
extern cvar_t* g_debugSaberLock;
extern cvar_t* g_saberLockRandomNess;
extern cvar_t* d_slowmodeath;
extern cvar_t* d_slowmoaction;
extern cvar_t* g_cheats;
extern cvar_t* g_saberRestrictForce;
extern cvar_t* g_saberPickuppableDroppedSabers;
extern cvar_t* g_InvertedHolsteredSabers;
extern cvar_t* g_saberAutoBlocking;
extern cvar_t* g_IsSaberDoingAttackDamage;
extern cvar_t* g_DebugSaberCombat;
extern cvar_t* g_newgameplusJKA;
extern cvar_t* g_newgameplusJKO;
extern qboolean WP_SaberBladeUseSecondBladeStyle(const saberInfo_t* saber, int bladeNum);
extern qboolean WP_SaberBladeDoTransitionDamage(const saberInfo_t* saber, int bladeNum);
extern qboolean Q3_TaskIDPending(const gentity_t* ent, taskID_t taskType);
extern qboolean G_ClearViewEntity(gentity_t* ent);
extern void G_SetViewEntity(gentity_t* self, gentity_t* viewEntity);
extern qboolean G_ControlledByPlayer(const gentity_t* self);
extern void G_AddVoiceEvent(const gentity_t* self, int event, int speakDebounceTime);
extern void CG_ChangeWeapon(int num);
extern void cg_saber_do_weapon_hit_marks(const gclient_t* client, const gentity_t* saber_ent, gentity_t* hit_ent,
	int saber_num,
	int blade_num, vec3_t hit_pos, vec3_t hit_dir, vec3_t uaxis,
	vec3_t splash_back_dir,
	float size_time_scale);
extern void G_AngerAlert(const gentity_t* self);
extern void g_reflect_missile_auto(gentity_t* ent, gentity_t* missile, vec3_t forward);
extern void g_reflect_missile_npc(gentity_t* ent, gentity_t* missile, vec3_t forward);
extern int G_CheckLedgeDive(gentity_t* self, float checkDist, const vec3_t checkVel, qboolean tryOpposite,
	qboolean tryPerp);
extern void g_bounce_missile(gentity_t* ent, trace_t* trace);
extern qboolean G_PointInBounds(const vec3_t point, const vec3_t mins, const vec3_t maxs);
extern void NPC_UseResponse(gentity_t* self, const gentity_t* user, qboolean useWhenDone);
extern void g_missile_impacted(gentity_t* ent, gentity_t* other, vec3_t impact_pos, vec3_t normal,
	int hitLoc = HL_NONE);
extern evasionType_t jedi_saber_block_go(gentity_t* self, usercmd_t* cmd, vec3_t p_hitloc, vec3_t phit_dir,
	const gentity_t* incoming, float dist = 0.0f);
extern void Jedi_RageStop(const gentity_t* self);
extern int PM_PickAnim(const gentity_t* self, int minAnim, int maxAnim);
extern void NPC_SetPainEvent(gentity_t* self);
extern qboolean PM_SwimmingAnim(int anim);
extern qboolean PM_InAnimForSaberMove(int anim, int saberMove);
extern qboolean PM_SpinningSaberAnim(int anim);
extern qboolean pm_saber_in_special_attack(int anim);
extern qboolean PM_SaberInKillAttack(int anim);
extern qboolean PM_SaberInAttack(int move);
extern qboolean PM_SaberInAttackPure(int move);
extern qboolean PM_SaberInTransition(int move);
extern qboolean PM_SaberInStart(int move);
extern qboolean PM_SaberInTransitionAny(int move);
extern qboolean PM_SaberInReturn(int move);
extern qboolean PM_SaberInBounce(int move);
extern qboolean PM_SaberInParry(int move);
extern qboolean PM_SaberInKnockaway(int move);
extern qboolean PM_SaberInBrokenParry(int move);
extern qboolean PM_SpinningSaberAnim(int anim);
extern saberMoveName_t PM_SaberBounceForAttack(int move);
extern saberMoveName_t PM_KnockawayForParry(int move);
extern saberMoveName_t PM_AnimateOldKnockBack(int move);
extern qboolean PM_FlippingAnim(int anim);
extern qboolean PM_RollingAnim(int anim);
extern qboolean PM_CrouchAnim(int anim);
extern qboolean PM_SaberInIdle(int move);
extern qboolean PM_SaberInReflect(int move);
extern qboolean PM_InSpecialJump(int anim);
extern qboolean PM_InKnockDown(const playerState_t* ps);
extern qboolean PM_ForceUsingSaberAnim(int anim);
extern qboolean PM_SuperBreakLoseAnim(int anim);
extern qboolean PM_SuperBreakWinAnim(int anim);
extern qboolean PM_SaberLockBreakAnim(int anim);
extern qboolean PM_InOnGroundAnim(playerState_t* ps);
extern qboolean PM_KnockDownAnim(int anim);
extern qboolean PM_SaberInKata(saberMoveName_t saberMove);
extern qboolean PM_SaberInBackAttack(saberMoveName_t saberMove);
extern qboolean PM_SaberInOverHeadSlash(saberMoveName_t saberMove);
extern qboolean PM_SaberInRollStab(saberMoveName_t saberMove);
extern qboolean PM_SaberInLungeStab(saberMoveName_t saberMove);
extern qboolean PM_StabDownAnim(int anim);
extern qboolean PM_LungRollAnim(int anim);
extern int PM_PowerLevelForSaberAnim(const playerState_t* ps, int saberNum = 0);
extern void PM_VelocityForSaberMove(const playerState_t* ps, vec3_t throw_dir);
extern qboolean PM_VelocityForBlockedMove(const playerState_t* ps, vec3_t throwDir);
extern qboolean PM_SaberCanInterruptMove(int move, int anim);
extern int Jedi_ReCalcParryTime(const gentity_t* self, evasionType_t evasionType);
extern qboolean jedi_dodge_evasion(gentity_t* self, gentity_t* shooter, trace_t* tr, int hit_loc);
extern void Jedi_PlayDeflectSound(const gentity_t* self);
extern void Jedi_PlayBlockedPushSound(const gentity_t* self);
extern qboolean Jedi_WaitingAmbush(const gentity_t* self);
extern void Jedi_Ambush(gentity_t* self);
extern qboolean Jedi_SaberBusy(const gentity_t* self);
extern qboolean Jedi_CultistDestroyer(const gentity_t* self);
extern void JET_FlyStart(gentity_t* self);
extern void Mando_DoFlameThrower(gentity_t* self);
extern void Boba_StopFlameThrower(const gentity_t* self);
extern qboolean PM_InRoll(const playerState_t* ps);
int wp_saber_must_block(gentity_t* self, const gentity_t* atk, qboolean check_b_box_block, vec3_t point, int r_saber_num, int r_blade_num);
extern int PM_InGrappleMove(int anim);
extern Vehicle_t* G_IsRidingVehicle(const gentity_t* pEnt);
extern int SaberDroid_PowerLevelForSaberAnim(const gentity_t* self);
extern qboolean G_ValidEnemy(const gentity_t* self, const gentity_t* enemy);
extern void G_StartMatrixEffect(const gentity_t* ent, int meFlags = 0, int length = 1000, float timeScale = 0.0f,
	int spinTime = 0);
extern void G_StartStasisEffect(const gentity_t* ent, int meFlags = 0, int length = 1000, float timeScale = 0.0f,
	int spinTime = 0);
extern int PM_AnimLength(int index, animNumber_t anim);
extern void G_Knockdown(gentity_t* self, gentity_t* attacker, const vec3_t pushDir, float strength,
	qboolean breakSaberLock);
extern void G_KnockOffVehicle(gentity_t* pRider, const gentity_t* self, qboolean bPull);
extern qboolean PM_LockedAnim(int anim);
extern qboolean Rosh_BeingHealed(const gentity_t* self);
extern qboolean G_OkayToLean(const playerState_t* ps, const usercmd_t* cmd, qboolean interruptOkay);
extern qboolean PM_KickMove(int move);
int wp_absorb_conversion(const gentity_t* attacked, int atd_abs_level, gentity_t* attacker, int at_power, int at_power_level,
	int at_force_spent);
void WP_ForcePowerStart(gentity_t* self, forcePowers_t forcePower, int overrideAmt);
void WP_ForcePowerStop(gentity_t* self, forcePowers_t forcePower);
qboolean WP_ForcePowerUsable(const gentity_t* self, forcePowers_t forcePower, int overrideAmt);
void wp_saber_in_flight_reflect_check(gentity_t* self);
extern qboolean SaberAttacking(const gentity_t* self);
void WP_SaberDrop(const gentity_t* self, gentity_t* saber);
qboolean WP_SaberLose(gentity_t* self, vec3_t throwDir);
void WP_SaberReturn(const gentity_t* self, gentity_t* saber);
qboolean WP_SaberBlockNonRandom(gentity_t* self, vec3_t hitloc, qboolean missileBlock);
qboolean WP_ForcePowerAvailable(const gentity_t* self, forcePowers_t forcePower, int overrideAmt);
void WP_ForcePowerDrain(const gentity_t* self, forcePowers_t forcePower, int overrideAmt);
void WP_DeactivateSaber(const gentity_t* self, qboolean clearLength = qfalse);
qboolean FP_ForceDrainGrippableEnt(const gentity_t* victim);
void G_SaberBounce(const gentity_t* attacker, gentity_t* victim);
extern qboolean PM_FaceProtectAnim(int anim);
extern void G_KnockOver(gentity_t* self, gentity_t* attacker, const vec3_t pushDir, float strength,
	qboolean breakSaberLock);
extern qboolean BG_InKnockDown(int anim);
extern saberMoveName_t PM_BrokenParryForParry(int move);
extern qboolean PM_SaberInDamageMove(int move);
extern qboolean PM_SaberDoDamageAnim(int anim);
extern qboolean PM_SaberInnonblockableAttack(int anim);
extern qboolean PM_Saberinstab(int move);
extern qboolean PM_RestAnim(int anim);
extern qboolean PM_KickingAnim(int anim);
extern void PM_AddFatigue(playerState_t* ps, int Fatigue);
extern qboolean g_accurate_blocking(const gentity_t* self, const gentity_t* attacker, vec3_t hit_loc);
extern int G_KnockawayForParry(int move);
extern int G_AnimateOldKnockBack(int move);
extern cvar_t* g_saberRealisticCombat;
extern cvar_t* g_saberDamageCapping;
extern cvar_t* g_saberNewControlScheme;
extern cvar_t* g_saberRealisticImpact;
extern int g_crosshairEntNum;
extern void player_Decloak(gentity_t* self);
extern qboolean PM_InGetUp(const playerState_t* ps);
extern qboolean PM_InForceGetUp(const playerState_t* ps);
extern qboolean npc_is_dark_jedi(const gentity_t* self);
extern qboolean npc_is_light_jedi(const gentity_t* self);
extern qboolean PM_SaberInMassiveBounce(int anim);
//////////////////////////////////////////////////
extern qboolean sab_beh_block_vs_attack(gentity_t* blocker, gentity_t* attacker, int saberNum, int bladeNum, vec3_t hitLoc);
extern qboolean sab_beh_attack_vs_block(gentity_t* attacker, gentity_t* blocker, int saberNum, int bladeNum, vec3_t hitLoc);
//////////////////////////////////////////////////
void player_Freeze(const gentity_t* self);
void Player_CheckFreeze(const gentity_t* self);
extern qboolean PM_SaberInSpecial(int move);
float manual_saberblocking(const gentity_t* defender);
void wp_block_points_regenerate(const gentity_t* self, int override_amt);
void wp_force_power_regenerate(const gentity_t* self, int override_amt);
void G_Stagger(gentity_t* hit_ent);
extern qboolean PM_StabAnim(int anim);
extern qboolean NPC_IsAlive(const gentity_t* self, const gentity_t* NPC);
extern qboolean PM_InKataAnim(int anim);
extern qboolean PM_StaggerAnim(int anim);
extern qboolean BG_SaberInNonIdleDamageMove(const playerState_t* ps);
extern qboolean PM_InSaberAnim(int anim);
extern qboolean BG_InFlipBack(int anim);
extern qboolean PM_SaberInBashedAnim(int anim);
qboolean WP_SaberBouncedSaberDirection(gentity_t* self, vec3_t hitloc, qboolean missileBlock);
extern qboolean PM_DeathCinAnim(int anim);
extern qboolean PM_InWallHoldMove(int anim);
extern qboolean PM_HasAnimation(const gentity_t* ent, int animation);
extern int SabBeh_AnimateMassiveDualSlowBounce(int anim);
extern int SabBeh_AnimateMassiveStaffSlowBounce(int anim);
extern void PM_AddBlockFatigue(playerState_t* ps, int Fatigue);
qboolean WP_SaberMBlockDirection(gentity_t* self, vec3_t hitloc, qboolean missileBlock);
qboolean WP_SaberFatiguedParryDirection(gentity_t* self, vec3_t hitloc, qboolean missileBlock);
extern qboolean BG_IsAlreadyinTauntAnim(int anim);
extern qboolean BG_FullBodyTauntAnim(int anim);
void wp_block_points_regenerate_over_ride(const gentity_t* self, int override_amt);
extern qboolean PM_WalkingOrRunningAnim(int anim);
extern void CG_CubeOutline(vec3_t mins, vec3_t maxs, int time, unsigned int color);
void WP_BlockPointsDrain(const gentity_t* self, int Fatigue);
qboolean WP_SaberParryNonRandom(gentity_t* self, vec3_t hitloc, qboolean missileBlock);
extern void CGCam_BlockShakeSP(float intensity, int duration);
static qhandle_t repulseLoopSound = 0;
extern void Boba_FlyStop(gentity_t* self);
extern void Jetpack_Off(const gentity_t* ent);
extern void SabBeh_SaberShouldBeDisarmedBlocker(gentity_t* blocker, int saberNum);

qboolean g_saberNoEffects = qfalse;
qboolean g_noClashFlare = qfalse;
int g_saberFlashTime = 0;
vec3_t g_saberFlashPos = { 0, 0, 0 };

int force_power_dark_light[NUM_FORCE_POWERS] = //0 == neutral
{
	//nothing should be usable at rank 0..
	FORCE_LIGHTSIDE, //FP_HEAL,//instant
	0, //FP_LEVITATION,//hold/duration
	0, //FP_SPEED,//duration
	0, //FP_PUSH,//hold/duration
	0, //FP_PULL,//hold/duration
	FORCE_LIGHTSIDE, //FP_TELEPATHY,//instant
	FORCE_DARKSIDE, //FP_GRIP,//hold/duration
	FORCE_DARKSIDE, //FP_LIGHTNING,//hold/duration
	0, //FP_SABERATTACK,
	0, //FP_SABERDEFEND,
	0, //FP_SABERTHROW,
	//new Jedi Academy powers
	FORCE_DARKSIDE, //FP_RAGE,//duration
	FORCE_LIGHTSIDE, //FP_PROTECT,//duration
	FORCE_LIGHTSIDE, //FP_ABSORB,//duration
	FORCE_DARKSIDE, //FP_DRAIN,//hold/duration
	0, //FP_SEE,//duration
	FORCE_LIGHTSIDE, //FP_STASIS
	FORCE_DARKSIDE, //FP_DESTRUCTION

	FORCE_LIGHTSIDE, //FP_GRASP,//hold/duration
	FORCE_LIGHTSIDE, //FP_REPULSE
	FORCE_LIGHTSIDE, //FP_LIGHTNING_STRIKE,
	FORCE_DARKSIDE, //FP_FEAR,
	FORCE_DARKSIDE, //FP_DEADLYSIGHT
	FORCE_LIGHTSIDE, //FP_BLAST,//Instant
	FORCE_DARKSIDE, //FP_INSANITY
	FORCE_LIGHTSIDE, //FP_BLINDING
	//NUM_FORCE_POWERS
};

int force_power_needed[NUM_FORCE_POWERS] =
{
	0, //FP_HEAL,//instant
	10, //FP_LEVITATION,//hold/duration
	50, //FP_SPEED,//duration
	15, //FP_PUSH,//hold/duration
	15, //FP_PULL,//hold/duration
	20, //FP_TELEPATHY,//instant
	1, //FP_GRIP,//hold/duration - FIXME: 30?
	1, //FP_LIGHTNING,//hold/duration
	20, //FP_SABERTHROW,
	1, //FP_SABER_DEFENSE,
	0, //FP_SABER_OFFENSE,
	//new Jedi Academy powers
	50,
	//FP_RAGE,//duration - speed, invincibility and extra damage for short period, drains your health and leaves you weak and slow afterwards.
	30,
	//FP_PROTECT,//duration - protect against physical/energy (level 1 stops blaster/energy bolts, level 2 stops projectiles, level 3 protects against explosions)
	30, //FP_ABSORB,//duration - protect against dark force powers (grip, lightning, drain)
	1, //FP_DRAIN,//hold/duration - drain force power for health
	20, //FP_SEE,//duration - detect/see hidden enemies
	35, //FP_STASIS
	40, //FP_DESTRUCTION

	1, //FP_GRASP,//hold/duration
	30, //FP_REPULSE
	30, //FP_LIGHTNING_STRIKE,
	20, //FP_FEAR,
	90, //FP_DEADLYSIGHT
	30, //FP_BLAST,//Instant
	50, //FP_INSANITY
	20 //FP_BLINDING
	//NUM_FORCE_POWERS
};

int force_power_neededlevel1[NUM_FORCE_POWERS] =
{
	0, //FP_HEAL,//instant
	10, //FP_LEVITATION,//hold/duration
	10, //FP_SPEED,//duration
	15, //FP_PUSH,//hold/duration
	15, //FP_PULL,//hold/duration
	20, //FP_TELEPATHY,//instant
	1, //FP_GRIP,//hold/duration - FIXME: 30?
	1, //FP_LIGHTNING,//hold/duration
	20, //FP_SABERTHROW,
	1, //FP_SABER_DEFENSE,
	0, //FP_SABER_OFFENSE,
	//new Jedi Academy powers
	50,
	//FP_RAGE,//duration - speed, invincibility and extra damage for short period, drains your health and leaves you weak and slow afterwards.
	30,
	//FP_PROTECT,//duration - protect against physical/energy (level 1 stops blaster/energy bolts, level 2 stops projectiles, level 3 protects against explosions)
	30, //FP_ABSORB,//duration - protect against dark force powers (grip, lightning, drain)
	1, //FP_DRAIN,//hold/duration - drain force power for health
	20, //FP_SEE,//duration - detect/see hidden enemies
	35, //FP_STASIS
	40, //FP_DESTRUCTION

	1, //FP_GRASP,//hold/duration
	30, //FP_REPULSE
	30, //FP_LIGHTNING_STRIKE,
	20, //FP_FEAR,
	90, //FP_DEADLYSIGHT
	30, //FP_BLAST,//Instant
	50, //FP_INSANITY
	20 //FP_BLINDING
	//NUM_FORCE_POWERS
};

int force_power_neededlevel2[NUM_FORCE_POWERS] =
{
	0, //FP_HEAL,//instant
	10, //FP_LEVITATION,//hold/duration
	20, //FP_SPEED,//duration
	15, //FP_PUSH,//hold/duration
	15, //FP_PULL,//hold/duration
	20, //FP_TELEPATHY,//instant
	1, //FP_GRIP,//hold/duration - FIXME: 30?
	1, //FP_LIGHTNING,//hold/duration
	20, //FP_SABERTHROW,
	1, //FP_SABER_DEFENSE,
	0, //FP_SABER_OFFENSE,
	//new Jedi Academy powers
	50,
	//FP_RAGE,//duration - speed, invincibility and extra damage for short period, drains your health and leaves you weak and slow afterwards.
	30,
	//FP_PROTECT,//duration - protect against physical/energy (level 1 stops blaster/energy bolts, level 2 stops projectiles, level 3 protects against explosions)
	30, //FP_ABSORB,//duration - protect against dark force powers (grip, lightning, drain)
	1, //FP_DRAIN,//hold/duration - drain force power for health
	20, //FP_SEE,//duration - detect/see hidden enemies
	35, //FP_STASIS
	40, //FP_DESTRUCTION

	1, //FP_GRASP,//hold/duration
	30, //FP_REPULSE
	30, //FP_LIGHTNING_STRIKE,
	20, //FP_FEAR,
	90, //FP_DEADLYSIGHT
	30, //FP_BLAST,//Instant
	50, //FP_INSANITY
	20 //FP_BLINDING
	//NUM_FORCE_POWERS
};

float forceJumpStrength[NUM_FORCE_POWER_LEVELS] =
{
	JUMP_VELOCITY, //normal jump
	420,
	590,
	840
};

float forceJumpHeight[NUM_FORCE_POWER_LEVELS] =
{
	32, //normal jump (+stepheight+crouchdiff = 66)
	96, //(+stepheight+crouchdiff = 130)
	192, //(+stepheight+crouchdiff = 226)
	384 //(+stepheight+crouchdiff = 418)
};

float forceJumpHeightMax[NUM_FORCE_POWER_LEVELS] =
{
	66, //normal jump (32+stepheight(18)+crouchdiff(24) = 74)
	130, //(96+stepheight(18)+crouchdiff(24) = 138)
	226, //(192+stepheight(18)+crouchdiff(24) = 234)
	418 //(384+stepheight(18)+crouchdiff(24) = 426)
};

float forcePushPullRadius[NUM_FORCE_POWER_LEVELS] =
{
	0, //none
	384, //256,
	448, //384,
	512
};

float forcePushCone[NUM_FORCE_POWER_LEVELS] =
{
	1.0f, //none
	1.0f,
	0.8f,
	0.6f
};

float forcePullCone[NUM_FORCE_POWER_LEVELS] =
{
	1.0f, //none
	1.0f,
	1.0f,
	0.8f
};

float forceSpeedValue[NUM_FORCE_POWER_LEVELS] =
{
	1.0f, //none
	0.75f,
	0.5f,
	0.25f
};

float forceSpeedRangeMod[NUM_FORCE_POWER_LEVELS] =
{
	0.0f, //none
	30.0f,
	45.0f,
	60.0f
};

float forceSpeedFOVMod[NUM_FORCE_POWER_LEVELS] =
{
	0.0f, //none
	20.0f,
	30.0f,
	40.0f
};

int forceGripDamage[NUM_FORCE_POWER_LEVELS] =
{
	0, //none
	3,
	6,
	9
};

int forceGraspDamage[NUM_FORCE_POWER_LEVELS] =
{
	0, //none
	0,
	0,
	0
};

int mindTrickTime[NUM_FORCE_POWER_LEVELS] =
{
	0, //none
	10000, //5000,
	15000, //10000,
	30000 //15000
};

int insanityTime[NUM_FORCE_POWER_LEVELS] =
{
	0, //none
	5000, //5000,
	10000, //10000,
	15000 //15000
};

int stasisTime[NUM_FORCE_POWER_LEVELS] =
{
	0, //none
	5000, //5000,
	10000, //10000,
	15000 //15000
};

int stasisJediTime[NUM_FORCE_POWER_LEVELS] =
{
	0, //none
	2000, //5000,
	3000, //10000,
	5000 //15000
};

float forceStasisRadius[NUM_FORCE_POWER_LEVELS] =
{
	0, //none
	512, //256,
	1024, //384,
	2048
};

float forceStasisCone[NUM_FORCE_POWER_LEVELS] =
{
	1.0f, //none
	1.0f,
	0.8f,
	0.6f
};

//NOTE: keep in sync with table below!!!
int saberThrowDist[NUM_FORCE_POWER_LEVELS] =
{
	0, //none
	256,
	400,
	400
};

//NOTE: keep in sync with table above!!!
int saberThrowDistSquared[NUM_FORCE_POWER_LEVELS] =
{
	0, //none
	65536,
	160000,
	160000
};

int parry_debounce[NUM_FORCE_POWER_LEVELS] =
{
	500, //if don't even have defense, can't use defense!
	300,
	150,
	50
};

stringID_table_t SaberStyleTable[] =
{
	{"NULL", SS_NONE},
	ENUM2STRING(SS_FAST),
	{"fast", SS_FAST},
	ENUM2STRING(SS_MEDIUM),
	{"medium", SS_MEDIUM},
	ENUM2STRING(SS_STRONG),
	{"strong", SS_STRONG},
	ENUM2STRING(SS_DESANN),
	{"desann", SS_DESANN},
	ENUM2STRING(SS_TAVION),
	{"tavion", SS_TAVION},
	ENUM2STRING(SS_DUAL),
	{"dual", SS_DUAL},
	ENUM2STRING(SS_STAFF),
	{"staff", SS_STAFF},
	{"", 0},
};

qboolean class_is_gunner(const gentity_t* self)
{
	switch (self->client->ps.weapon)
	{
	case WP_STUN_BATON:
	case WP_BLASTER_PISTOL:
	case WP_BLASTER:
	case WP_DISRUPTOR:
	case WP_BOWCASTER:
	case WP_REPEATER:
	case WP_DEMP2:
	case WP_FLECHETTE:
	case WP_ROCKET_LAUNCHER:
	case WP_THERMAL:
	case WP_TRIP_MINE:
	case WP_DET_PACK:
	case WP_CONCUSSION:
	case WP_ATST_MAIN:
	case WP_ATST_SIDE:
	case WP_BRYAR_PISTOL:
	case WP_SBD_PISTOL:
	case WP_EMPLACED_GUN:
	case WP_BOT_LASER:
	case WP_TURRET:
	case WP_TIE_FIGHTER:
	case WP_RAPID_FIRE_CONC:
	case WP_JAWA:
	case WP_TUSKEN_RIFLE:
	case WP_TUSKEN_STAFF:
	case WP_SCEPTER:
	case WP_NOGHRI_STICK:
		// Is Gunner...
		return qtrue;
	default:
		// NOT Gunner...
		break;
	}

	return qfalse;
}

//SABER INITIALIZATION======================================================================

void g_create_g2_holstered_weapon_model(gentity_t* ent, const char* psWeaponModel, const int bolt_num,
	const int weapon_num, vec3_t angles, vec3_t offset)
{
	if (!psWeaponModel)
	{
		assert(psWeaponModel);
		return;
	}
	if (ent->playerModel == -1)
	{
		return;
	}
	if (bolt_num == -1)
	{
		return;
	}
	if (weapon_num < 0 || weapon_num >= MAX_INHAND_WEAPONS)
	{
		return;
	}

	char weapon_model[64];

	strcpy(weapon_model, psWeaponModel);
	if (char* spot = strstr(weapon_model, ".md3"))
	{
		*spot = 0;
		spot = strstr(weapon_model, "_w");
		//i'm using the in view weapon array instead of scanning the item list, so put the _w back on
		if (!spot && !strstr(weapon_model, "noweap"))
		{
			strcat(weapon_model, "_w");
		}
		strcat(weapon_model, ".glm"); //and change to ghoul2
	}

	// give us a saber model
	const int w_model_index = G_ModelIndex(weapon_model);
	if (w_model_index)
	{
		ent->holsterModel[weapon_num] = gi.G2API_InitGhoul2Model(ent->ghoul2, weapon_model, w_model_index, NULL_HANDLE,
			NULL_HANDLE, 0, 0);
		if (ent->holsterModel[weapon_num] != -1)
		{
			// attach it to the hip. need some correction of rotation first though!
			const int holster_origin = gi.G2API_AddBolt(&ent->ghoul2[ent->holsterModel[weapon_num]], "*holsterorigin");
			mdxaBone_t bolt_matrix2;
			if (holster_origin != -1)
			{
				constexpr vec3_t origin = { 0, 0, 0 };
				gi.G2API_GetBoltMatrix(ent->ghoul2, ent->holsterModel[weapon_num], holster_origin, &bolt_matrix2, origin,
					origin, 0, nullptr, ent->s.modelScale);
			}
			gi.G2API_AttachG2Model(&ent->ghoul2[ent->holsterModel[weapon_num]], &ent->ghoul2[ent->playerModel],
				bolt_num, ent->playerModel);

			if (holster_origin == -1)
			{
				if (ent->client->ps.saber[0].type == SABER_DAGGER)
				{
					//COUNT ME OUT ON THIS ONE
				}
				else
				{
					gi.G2API_SetBoneAnglesOffset(&ent->ghoul2[ent->holsterModel[weapon_num]],
						"ModView internal default", angles, BONE_ANGLES_PREMULT, POSITIVE_X,
						NEGATIVE_Y, NEGATIVE_Z, nullptr, 0, 0, offset);
				}
			}
			else
			{
				bolt_matrix2.matrix[1][3] -= 1.0f;
				gi.G2API_SetBoneAnglesMatrix(&ent->ghoul2[ent->holsterModel[weapon_num]], "ModView internal default",
					bolt_matrix2, BONE_ANGLES_PREMULT, nullptr, 0, 0);
			}
			// set up a bolt on the end so we can get where the saber muzzle is - we can assume this is always bolt 0
			gi.G2API_AddBolt(&ent->ghoul2[ent->holsterModel[weapon_num]], "*flash");
		}
	}
}

void g_create_g2_attached_weapon_model(gentity_t* ent, const char* ps_weapon_model, const int bolt_num,
	const int weapon_num)
{
	if (!ps_weapon_model)
	{
		assert(ps_weapon_model);
		return;
	}
	if (ent->playerModel == -1)
	{
		return;
	}
	if (bolt_num == -1)
	{
		return;
	}

	if (ent && ent->client && ent->client->NPC_class == CLASS_GALAKMECH)
	{
		//hack for galakmech, no weaponmodel
		ent->weaponModel[0] = ent->weaponModel[1] = -1;
		return;
	}

	if (ent && ent->client && ent->client->NPC_class == CLASS_SBD)
	{
		//hack for sbd, no weaponmodel
		ent->weaponModel[0] = ent->weaponModel[1] = -1;
		return;
	}

	if (weapon_num < 0 || weapon_num >= MAX_INHAND_WEAPONS)
	{
		return;
	}
	char weapon_model[64];

	strcpy(weapon_model, ps_weapon_model);
	if (char* spot = strstr(weapon_model, ".md3"))
	{
		*spot = 0;
		spot = strstr(weapon_model, "_w");
		//i'm using the in view weapon array instead of scanning the item list, so put the _w back on
		if (!spot && !strstr(weapon_model, "noweap"))
		{
			strcat(weapon_model, "_w");
		}
		strcat(weapon_model, ".glm"); //and change to ghoul2
	}

	// give us a saber model
	const int w_model_index = G_ModelIndex(weapon_model);
	if (w_model_index)
	{
		ent->weaponModel[weapon_num] = gi.G2API_InitGhoul2Model(ent->ghoul2, weapon_model, w_model_index, NULL_HANDLE,
			NULL_HANDLE, 0, 0);
		if (ent->weaponModel[weapon_num] != -1)
		{
			// attach it to the hand
			gi.G2API_AttachG2Model(&ent->ghoul2[ent->weaponModel[weapon_num]], &ent->ghoul2[ent->playerModel], bolt_num,
				ent->playerModel);
			// set up a bolt on the end so we can get where the sabre muzzle is - we can assume this is always bolt 0
			gi.G2API_AddBolt(&ent->ghoul2[ent->weaponModel[weapon_num]], "*flash");
		}
	}
}

void wp_saber_add_g2_saber_models(gentity_t* ent, const int specificSaberNum)
{
	int saber_num = 0, max_saber = 1;

	if (specificSaberNum != -1 && specificSaberNum <= max_saber)
	{
		saber_num = max_saber = specificSaberNum;
	}
	for (; saber_num <= max_saber; saber_num++)
	{
		if (ent->weaponModel[saber_num] > 0)
		{
			//we already have a weapon model in this slot
			//remove it
			gi.G2API_SetSkin(&ent->ghoul2[ent->weaponModel[saber_num]], -1, 0);
			gi.G2API_RemoveGhoul2Model(ent->ghoul2, ent->weaponModel[saber_num]);
			ent->weaponModel[saber_num] = -1;
		}
		if (saber_num > 0)
		{
			//second saber
			if (!ent->client->ps.dualSabers
				|| G_IsRidingVehicle(ent))
			{
				//only have one saber or riding a vehicle and can only use one saber
				return;
			}
		}
		else if (saber_num == 0)
		{
			//first saber
			if (ent->client->ps.saberInFlight)
			{
				//it's still out there somewhere, don't add it
				//FIXME: call it back?
				continue;
			}
		}

		int hand_bolt = saber_num == 0 ? ent->handRBolt : ent->handLBolt;

		if (ent->client->ps.saber[saber_num].saberFlags & SFL_BOLT_TO_WRIST)
		{
			//special case, bolt to forearm
			if (saber_num == 0)
			{
				hand_bolt = gi.G2API_AddBolt(&ent->ghoul2[ent->playerModel], "*r_hand_cap_r_arm");
			}
			else
			{
				hand_bolt = gi.G2API_AddBolt(&ent->ghoul2[ent->playerModel], "*l_hand_cap_l_arm");
			}
		}
		g_create_g2_attached_weapon_model(ent, ent->client->ps.saber[saber_num].model, hand_bolt, saber_num);

		if (ent->client->ps.saber[saber_num].skin != nullptr)
		{
			//if this saber has a customSkin, use it
			// lets see if it's out there
			const int saberSkin = gi.RE_RegisterSkin(ent->client->ps.saber[saber_num].skin);
			if (saberSkin)
			{
				gi.G2API_SetSkin(&ent->ghoul2[ent->weaponModel[saber_num]],
					G_SkinIndex(ent->client->ps.saber[saber_num].skin), saberSkin);
			}
		}
	}
}

void wp_saber_add_holstered_g2_saber_models(gentity_t* ent, const int specific_saber_num)
{
	int saber_num = 0, max_saber = 1;

	if (!(ent && ent->client && ent->client->ps.stats[STAT_WEAPONS] & 1 << WP_SABER) ||
		ent->client->ps.saber[0].type == SABER_DAGGER ||
		ent->client->ps.saber[0].type == SABER_GRIE ||
		ent->client->ps.saber[0].type == SABER_GRIE4 ||
		ent->client->ps.saber[0].type == SABER_ELECTROSTAFF ||
		ent->client->ps.saber[0].type == SABER_SITH_SWORD ||
		ent->client->NPC_class == CLASS_MANDO)
	{
		return;
	}
	if (specific_saber_num != -1 && specific_saber_num <= max_saber)
	{
		saber_num = max_saber = specific_saber_num;
	}
	for (; saber_num <= max_saber; saber_num++)
	{
		if (ent->holsterModel[saber_num] > 0)
		{
			//we already have a weapon model in this slot
			//remove it
			gi.G2API_SetSkin(&ent->ghoul2[ent->holsterModel[saber_num]], -1, 0);
			gi.G2API_RemoveGhoul2Model(ent->ghoul2, ent->holsterModel[saber_num]);
			ent->holsterModel[saber_num] = -1;
		}
		if (saber_num > 0)
		{
			//second saber
			if (!ent->client->ps.dualSabers)
			{
				//only have one saber or riding a vehicle and can only use one saber
				return;
			}
		}
		else if (saber_num == 0)
		{
			//first saber
			if (ent->client->ps.saberInFlight)
			{
				//it's still out there somewhere, don't add it
				continue;
			}
		}
		else if (ent->client->ps.saber[saber_num].holsterPlace == HOLSTER_NONE)
		{
			continue;
		}

		int hand_bolt = -1;
		const holster_locations_t holster_place = ent->client->ps.saber[saber_num].holsterPlace;
		vec3_t offset = { 0.0f, 0.0f, 0.0f };
		vec3_t angles = { 0.0f, 0.0f, 0.0f };

		if (holster_place == HOLSTER_HIPS)
		{
			angles[PITCH] = 180.0f;
			angles[ROLL] = 180.0f;

			if (g_InvertedHolsteredSabers && g_InvertedHolsteredSabers->integer > 0)
			{
				angles[YAW] = 180.0f;
			}
			else
			{
				angles[YAW] = 0.0f;
			}
			VectorSet(offset, 0.0f, -1.0f, -5.0f);
		}
		else if (holster_place == HOLSTER_LHIP)
		{
			angles[PITCH] = 180.0f;
			angles[ROLL] = 180.0f;

			if (g_InvertedHolsteredSabers && g_InvertedHolsteredSabers->integer > 0)
			{
				angles[YAW] = 180.0f;
			}
			else
			{
				angles[YAW] = 0.0f;
			}
			VectorSet(offset, 0.0f, -1.0f, -5.0f);
		}
		else if (holster_place == HOLSTER_BACK)
		{
			if (g_InvertedHolsteredSabers && g_InvertedHolsteredSabers->integer > 0)
			{
				angles[YAW] = 180.0f;
			}
			else
			{
				angles[YAW] = 0.0f;
			}
			angles[PITCH] = 22.5f;
			VectorSet(offset, 0.0f, -2.0f, 4.0f);
		}
		if (saber_num == 0)
		{
			if (holster_place == HOLSTER_LHIP)
			{
				if (ent->client->ps.saber[0].type == SABER_STAFF || ent->client->ps.saber[0].type == SABER_STAFF_MAUL ||
					ent->client->ps.saber[0].type == SABER_STAFF_SFX
					|| ent->client->ps.saber[0].type == SABER_STAFF_UNSTABLE || ent->client->ps.saber[0].type ==
					SABER_STAFF_THIN
					|| ent->client->ps.saber[0].type == SABER_BACKHAND)
				{
					hand_bolt = gi.G2API_AddBolt(&ent->ghoul2[ent->playerModel], "*hip_bl");
				}
				else if (ent->client->ps.saber[0].type == SABER_DAGGER)
				{
					hand_bolt = gi.G2API_AddBolt(&ent->ghoul2[ent->playerModel], "*l_hand_cap_l_arm");
				}
				else if (ent->client->ps.saber[0].type == SABER_YODA)
				{
					hand_bolt = gi.G2API_AddBolt(&ent->ghoul2[ent->playerModel], "*hip_fl");
				}
				else if (ent->client->ps.saber[0].type == SABER_UNSTABLE)
				{
					hand_bolt = gi.G2API_AddBolt(&ent->ghoul2[ent->playerModel], "*hip_fr");
				}
				else if (ent->client->ps.saber[0].type == SABER_OBIWAN)
				{
					if (com_outcast->integer == 10) //jko version
					{
						hand_bolt = gi.G2API_AddBolt(&ent->ghoul2[ent->playerModel], "*hip_l");
					}
					else
					{
						hand_bolt = gi.G2API_AddBolt(&ent->ghoul2[ent->playerModel], "*hip_r");
					}
				}
				else
				{
					hand_bolt = gi.G2API_AddBolt(&ent->ghoul2[ent->playerModel], "*hip_l");
				}
			}
			else if (holster_place == HOLSTER_HIPS)
			{
				if (ent->client->ps.saber[0].type == SABER_STAFF || ent->client->ps.saber[0].type == SABER_STAFF_MAUL ||
					ent->client->ps.saber[0].type == SABER_STAFF_SFX
					|| ent->client->ps.saber[0].type == SABER_STAFF_UNSTABLE || ent->client->ps.saber[0].type ==
					SABER_STAFF_THIN
					|| ent->client->ps.saber[0].type == SABER_BACKHAND)
				{
					hand_bolt = gi.G2API_AddBolt(&ent->ghoul2[ent->playerModel], "*hip_br");
				}
				else if (ent->client->ps.saber[0].type == SABER_DAGGER)
				{
					hand_bolt = gi.G2API_AddBolt(&ent->ghoul2[ent->playerModel], "*r_hand_cap_r_arm");
				}
				else if (ent->client->ps.saber[0].type == SABER_YODA)
				{
					hand_bolt = gi.G2API_AddBolt(&ent->ghoul2[ent->playerModel], "*hip_fl");
				}
				else if (ent->client->ps.saber[0].type == SABER_UNSTABLE)
				{
					hand_bolt = gi.G2API_AddBolt(&ent->ghoul2[ent->playerModel], "*hip_fr");
				}
				else if (ent->client->ps.saber[0].type == SABER_OBIWAN)
				{
					if (com_outcast->integer == 10) //jko version
					{
						hand_bolt = gi.G2API_AddBolt(&ent->ghoul2[ent->playerModel], "*hip_l");
					}
					else
					{
						hand_bolt = gi.G2API_AddBolt(&ent->ghoul2[ent->playerModel], "*hip_r");
					}
				}
				else
				{
					hand_bolt = gi.G2API_AddBolt(&ent->ghoul2[ent->playerModel], "*hip_r");
				}
			}
			else if (holster_place == HOLSTER_BACK)
			{
				hand_bolt = gi.G2API_AddBolt(&ent->ghoul2[ent->playerModel], "*back");
			}
		}
		else
		{
			if (holster_place == HOLSTER_HIPS || holster_place == HOLSTER_LHIP)
			{
				if (ent->client->ps.saber[0].holsterPlace == HOLSTER_LHIP)
				{
					if (ent->client->ps.saber[0].type == SABER_STAFF || ent->client->ps.saber[0].type ==
						SABER_STAFF_MAUL || ent->client->ps.saber[0].type == SABER_STAFF_SFX
						|| ent->client->ps.saber[0].type == SABER_STAFF_UNSTABLE || ent->client->ps.saber[0].type ==
						SABER_STAFF_THIN
						|| ent->client->ps.saber[0].type == SABER_BACKHAND)
					{
						hand_bolt = gi.G2API_AddBolt(&ent->ghoul2[ent->playerModel], "*hip_br");
					}
					else if (ent->client->ps.saber[0].type == SABER_DAGGER)
					{
						hand_bolt = gi.G2API_AddBolt(&ent->ghoul2[ent->playerModel], "*r_hand_cap_r_arm");
					}
					else if (ent->client->ps.saber[0].type == SABER_YODA)
					{
						hand_bolt = gi.G2API_AddBolt(&ent->ghoul2[ent->playerModel], "*hip_fl");
					}
					else if (ent->client->ps.saber[0].type == SABER_UNSTABLE)
					{
						hand_bolt = gi.G2API_AddBolt(&ent->ghoul2[ent->playerModel], "*hip_fr");
					}
					else if (ent->client->ps.saber[0].type == SABER_OBIWAN)
					{
						if (com_outcast->integer == 10) //jko version
						{
							hand_bolt = gi.G2API_AddBolt(&ent->ghoul2[ent->playerModel], "*hip_l");
						}
						else
						{
							hand_bolt = gi.G2API_AddBolt(&ent->ghoul2[ent->playerModel], "*hip_r");
						}
					}
					else
					{
						hand_bolt = gi.G2API_AddBolt(&ent->ghoul2[ent->playerModel], "*hip_r");
					}
				}
				else
				{
					if (ent->client->ps.saber[0].type == SABER_STAFF || ent->client->ps.saber[0].type ==
						SABER_STAFF_MAUL || ent->client->ps.saber[0].type == SABER_STAFF_SFX
						|| ent->client->ps.saber[0].type == SABER_STAFF_UNSTABLE || ent->client->ps.saber[0].type ==
						SABER_STAFF_THIN
						|| ent->client->ps.saber[0].type == SABER_BACKHAND)
					{
						hand_bolt = gi.G2API_AddBolt(&ent->ghoul2[ent->playerModel], "*hip_bl");
					}
					else if (ent->client->ps.saber[0].type == SABER_DAGGER)
					{
						hand_bolt = gi.G2API_AddBolt(&ent->ghoul2[ent->playerModel], "*l_hand_cap_l_arm");
					}
					else if (ent->client->ps.saber[0].type == SABER_YODA)
					{
						hand_bolt = gi.G2API_AddBolt(&ent->ghoul2[ent->playerModel], "*hip_fl");
					}
					else if (ent->client->ps.saber[0].type == SABER_UNSTABLE)
					{
						hand_bolt = gi.G2API_AddBolt(&ent->ghoul2[ent->playerModel], "*hip_fr");
					}
					else if (ent->client->ps.saber[0].type == SABER_OBIWAN)
					{
						if (com_outcast->integer == 10) //jko version
						{
							hand_bolt = gi.G2API_AddBolt(&ent->ghoul2[ent->playerModel], "*hip_l");
						}
						else
						{
							hand_bolt = gi.G2API_AddBolt(&ent->ghoul2[ent->playerModel], "*hip_r");
						}
					}
					else
					{
						hand_bolt = gi.G2API_AddBolt(&ent->ghoul2[ent->playerModel], "*hip_l");
					}
				}
			}
			else if (holster_place == HOLSTER_BACK)
			{
				if (ent->client->ps.saber[0].holsterPlace == HOLSTER_BACK)
				{
					continue;
				}
				hand_bolt = gi.G2API_AddBolt(&ent->ghoul2[ent->playerModel], "*back");
			}
		}
		g_create_g2_holstered_weapon_model(ent, ent->client->ps.saber[saber_num].model, hand_bolt, saber_num, angles,
			offset);

		if (ent->client->ps.saber[saber_num].skin != nullptr)
		{
			//if this saber has a customSkin, use it
			// lets see if it's out there
			const int saber_skin = gi.RE_RegisterSkin(ent->client->ps.saber[saber_num].skin);
			if (saber_skin)
			{
				// put it in the config strings and set the ghoul2 model to use it
				gi.G2API_SetSkin(&ent->ghoul2[ent->holsterModel[saber_num]],
					G_SkinIndex(ent->client->ps.saber[saber_num].skin), saber_skin);
			}
		}
	}
}

//Check to see if the player is actually walking or just standing
extern qboolean PM_RunningAnim(int anim);
extern qboolean PM_WalkingAnim(int anim);
extern qboolean PM_StandingAnim(int anim);

qboolean walk_check(const gentity_t* self)
{
	if (PM_RunningAnim(self->client->ps.legsAnim))
	{
		return qfalse;
	}

	return qtrue;
}

qboolean pm_walking_or_standing(const gentity_t* self)
{
	//mainly for checking if we can guard well
	if (PM_WalkingAnim(self->client->ps.legsAnim)
		|| PM_StandingAnim(self->client->ps.legsAnim)
		|| PM_CrouchAnim(self->client->ps.legsAnim))
	{
		return qtrue;
	}
	return qfalse;
}

void bg_reduce_blaster_mishap_level(playerState_t* ps)
{
	//reduces a player's mishap meter by one level
	if (ps->BlasterAttackChainCount >= BLASTERMISHAPLEVEL_MAX)
	{
		ps->BlasterAttackChainCount = BLASTERMISHAPLEVEL_TWENTYNINE;
	}
	else if (ps->BlasterAttackChainCount >= BLASTERMISHAPLEVEL_TWENTYNINE)
	{
		ps->BlasterAttackChainCount = BLASTERMISHAPLEVEL_TWENTYEIGHT;
	}
	else if (ps->BlasterAttackChainCount >= BLASTERMISHAPLEVEL_TWENTYEIGHT)
	{
		ps->BlasterAttackChainCount = BLASTERMISHAPLEVEL_OVERLOAD;
	}
	else if (ps->BlasterAttackChainCount >= BLASTERMISHAPLEVEL_OVERLOAD)
	{
		ps->BlasterAttackChainCount = BLASTERMISHAPLEVEL_TWENTYSIX;
	}
	else if (ps->BlasterAttackChainCount >= BLASTERMISHAPLEVEL_TWENTYSIX)
	{
		ps->BlasterAttackChainCount = BLASTERMISHAPLEVEL_FULL;
	}
	else if (ps->BlasterAttackChainCount >= BLASTERMISHAPLEVEL_FULL)
	{
		ps->BlasterAttackChainCount = BLASTERMISHAPLEVEL_HEAVYER;
	}
	else if (ps->BlasterAttackChainCount >= BLASTERMISHAPLEVEL_HEAVYER)
	{
		ps->BlasterAttackChainCount = BLASTERMISHAPLEVEL_HALF;
	}
	else if (ps->BlasterAttackChainCount >= BLASTERMISHAPLEVEL_HALF)
	{
		ps->BlasterAttackChainCount = BLASTERMISHAPLEVEL_HEAVY;
	}
	else if (ps->BlasterAttackChainCount >= BLASTERMISHAPLEVEL_HEAVY)
	{
		ps->BlasterAttackChainCount = BLASTERMISHAPLEVEL_MEDIUM;
	}
	else if (ps->BlasterAttackChainCount >= BLASTERMISHAPLEVEL_MEDIUM)
	{
		ps->BlasterAttackChainCount = BLASTERMISHAPLEVEL_LIGHT;
	}
	else if (ps->BlasterAttackChainCount >= BLASTERMISHAPLEVEL_LIGHT)
	{
		ps->BlasterAttackChainCount = BLASTERMISHAPLEVEL_RUNINACCURACY;
	}
	else if (ps->BlasterAttackChainCount >= BLASTERMISHAPLEVEL_RUNINACCURACY)
	{
		ps->BlasterAttackChainCount = BLASTERMISHAPLEVEL_MIN;
	}
	else if (ps->BlasterAttackChainCount >= BLASTERMISHAPLEVEL_MIN)
	{
		ps->BlasterAttackChainCount = BLASTERMISHAPLEVEL_NONE;
	}
	else
	{
		ps->BlasterAttackChainCount = BLASTERMISHAPLEVEL_NONE;
	}
}

void bg_reduce_blaster_mishap_level_advanced(playerState_t* ps)
{
	//reduces a player's mishap meter by one level
	if (ps->BlasterAttackChainCount > BLASTERMISHAPLEVEL_MIN)
	{
		ps->BlasterAttackChainCount = BLASTERMISHAPLEVEL_NONE;
	}
	else
	{
		ps->BlasterAttackChainCount = BLASTERMISHAPLEVEL_NONE;
	}
}

void bg_reduce_saber_mishap_level(playerState_t* ps)
{
	//reduces a player's mishap meter by one level
	if (ps->saberFatigueChainCount >= MISHAPLEVEL_MAX)
	{
		ps->saberFatigueChainCount = MISHAPLEVEL_FULL;
	}
	else if (ps->saberFatigueChainCount >= MISHAPLEVEL_FULL)
	{
		ps->saberFatigueChainCount = MISHAPLEVEL_THIRTEEN;
	}
	else if (ps->saberFatigueChainCount >= MISHAPLEVEL_THIRTEEN)
	{
		ps->saberFatigueChainCount = MISHAPLEVEL_HUDFLASH;
	}
	else if (ps->saberFatigueChainCount >= MISHAPLEVEL_HUDFLASH)
	{
		ps->saberFatigueChainCount = MISHAPLEVEL_ELEVEN;
	}
	else if (ps->saberFatigueChainCount >= MISHAPLEVEL_ELEVEN)
	{
		ps->saberFatigueChainCount = MISHAPLEVEL_TEN;
	}
	else if (ps->saberFatigueChainCount >= MISHAPLEVEL_TEN)
	{
		ps->saberFatigueChainCount = MISHAPLEVEL_NINE;
	}
	else if (ps->saberFatigueChainCount >= MISHAPLEVEL_NINE)
	{
		ps->saberFatigueChainCount = MISHAPLEVEL_HEAVY;
	}
	else if (ps->saberFatigueChainCount >= MISHAPLEVEL_HEAVY)
	{
		ps->saberFatigueChainCount = MISHAPLEVEL_SEVEN;
	}
	else if (ps->saberFatigueChainCount >= MISHAPLEVEL_SEVEN)
	{
		ps->saberFatigueChainCount = MISHAPLEVEL_SIX;
	}
	else if (ps->saberFatigueChainCount >= MISHAPLEVEL_SIX)
	{
		ps->saberFatigueChainCount = MISHAPLEVEL_LIGHT;
	}
	else if (ps->saberFatigueChainCount >= MISHAPLEVEL_LIGHT)
	{
		ps->saberFatigueChainCount = MISHAPLEVEL_FOUR;
	}
	else if (ps->saberFatigueChainCount >= MISHAPLEVEL_FOUR)
	{
		ps->saberFatigueChainCount = MISHAPLEVEL_SNIPER;
	}
	else if (ps->saberFatigueChainCount >= MISHAPLEVEL_SNIPER)
	{
		ps->saberFatigueChainCount = MISHAPLEVEL_RUNINACCURACY;
	}
	else if (ps->saberFatigueChainCount >= MISHAPLEVEL_RUNINACCURACY)
	{
		ps->saberFatigueChainCount = MISHAPLEVEL_MININACCURACY;
	}
	else if (ps->saberFatigueChainCount >= MISHAPLEVEL_MININACCURACY)
	{
		ps->saberFatigueChainCount = MISHAPLEVEL_NONE;
	}
	else
	{
		ps->saberFatigueChainCount = MISHAPLEVEL_NONE;
	}
}

//----------------------------------------------------------
void g_throw(gentity_t* targ, const vec3_t new_dir, const float push)
//----------------------------------------------------------
{
	vec3_t kvel;
	float mass;

	if (targ
		&& targ->client
		&& (targ->client->NPC_class == CLASS_ATST
			|| targ->client->NPC_class == CLASS_RANCOR
			|| targ->client->NPC_class == CLASS_SAND_CREATURE))
	{
		//much to large to *ever* throw
		return;
	}

	if (targ && targ->physicsBounce > 0) //override the mass
	{
		mass = targ->physicsBounce;
	}
	else
	{
		mass = 200;
	}

	if (g_gravity->value > 0)
	{
		VectorScale(new_dir, g_knockback->value * static_cast<float>(push) / mass * 0.8, kvel);
		if (!targ->client || targ->client->ps.groundEntityNum != ENTITYNUM_NONE)
		{
			//give them some z lift to get them off the ground
			kvel[2] = new_dir[2] * g_knockback->value * static_cast<float>(push) / mass * 1.5;
		}
	}
	else
	{
		VectorScale(new_dir, g_knockback->value * static_cast<float>(push) / mass, kvel);
	}

	if (targ && targ->client)
	{
		VectorAdd(targ->client->ps.velocity, kvel, targ->client->ps.velocity);
	}
	else if (targ->s.pos.trType != TR_STATIONARY
		&& targ->s.pos.trType != TR_LINEAR_STOP
		&& targ->s.pos.trType != TR_NONLINEAR_STOP)
	{
		VectorAdd(targ->s.pos.trDelta, kvel, targ->s.pos.trDelta);
		VectorCopy(targ->currentOrigin, targ->s.pos.trBase);
		targ->s.pos.trTime = level.time;
	}

	// set the timer so that the other client can't cancel
	// out the movement immediately
	if (targ->client && !targ->client->ps.pm_time)
	{
		int t = push * 2;

		if (t < 50)
		{
			t = 50;
		}
		if (t > 200)
		{
			t = 200;
		}
		targ->client->ps.pm_time = t;
		targ->client->ps.pm_flags |= PMF_TIME_KNOCKBACK;
	}
}

//----------------------------------------------------------
void g_kick_throw(gentity_t* targ, const vec3_t new_dir, const float push)
//----------------------------------------------------------
{
	vec3_t kvel;
	float mass;

	if (targ
		&& targ->client
		&& (targ->client->NPC_class == CLASS_ATST
			|| targ->client->NPC_class == CLASS_RANCOR
			|| targ->client->NPC_class == CLASS_SAND_CREATURE))
	{
		//much to large to *ever* throw
		return;
	}

	if (targ && targ->physicsBounce > 0) //override the mass
	{
		mass = targ->physicsBounce;
	}
	else
	{
		mass = 200;
	}

	if (g_gravity->value > 0)
	{
		VectorScale(new_dir, g_knockback->value * static_cast<float>(push) / mass * 0.8, kvel);
		if (!targ->client || targ->client->ps.groundEntityNum != ENTITYNUM_NONE)
		{
			//give them some z lift to get them off the ground
			kvel[2] = new_dir[2] * g_knockback->value * static_cast<float>(push) / mass * 1.5;
		}
	}
	else
	{
		VectorScale(new_dir, g_knockback->value * static_cast<float>(push) / mass, kvel);
	}

	if (targ && targ->client)
	{
		VectorAdd(targ->client->ps.velocity, kvel, targ->client->ps.velocity);
	}
	else if (targ->s.pos.trType != TR_STATIONARY
		&& targ->s.pos.trType != TR_LINEAR_STOP
		&& targ->s.pos.trType != TR_NONLINEAR_STOP)
	{
		VectorAdd(targ->s.pos.trDelta, kvel, targ->s.pos.trDelta);
		VectorCopy(targ->currentOrigin, targ->s.pos.trBase);
		targ->s.pos.trTime = level.time;
	}

	// set the timer so that the other client can't cancel
	// out the movement immediately
	if (targ->client && !targ->client->ps.pm_time)
	{
		int t = push * 2;

		if (t < 10)
		{
			t = 10;
		}
		if (t > 150)
		{
			t = 150;
		}
		targ->client->ps.pm_time = t;
		targ->client->ps.pm_flags |= PMF_TIME_KNOCKBACK;
	}
}

int wp_set_saber_model(gclient_t* client, const class_t npc_class)
{
	//FIXME: read from NPCs.cfg
	if (client)
	{
		switch (npc_class)
		{
		case CLASS_DESANN: //Desann
			client->ps.saber[0].model = "models/weapons2/saber_desann/saber_w.glm";
			break;
		case CLASS_VADER: //Desann
			client->ps.saber[0].model = "models/weapons2/saber_VaderEp5/saber_w.glm";
			break;
		case CLASS_LUKE: //Luke
			client->ps.saber[0].model = "models/weapons2/saber_luke/saber_w.glm";
			break;
		case CLASS_YODA: //YODA
			client->ps.saber[0].model = "models/weapons2/saber_yoda/saber_w.glm";
			break;
		case CLASS_PLAYER: //Kyle NPC and player
		case CLASS_KYLE: //Kyle NPC and player
			client->ps.saber[0].model = DEFAULT_SABER_MODEL;
			break;
		default: //reborn and tavion and everyone else
			client->ps.saber[0].model = DEFAULT_SABER_MODEL;
			break;
		}
		return G_ModelIndex(client->ps.saber[0].model);
	}
	switch (npc_class)
	{
	case CLASS_DESANN: //Desann
		return G_ModelIndex("models/weapons2/saber_desann/saber_w.glm");
	case CLASS_VADER: //Desann
		return G_ModelIndex("models/weapons2/saber_VaderEp5/saber_w.glm");
	case CLASS_LUKE: //Luke
		return G_ModelIndex("models/weapons2/saber_luke/saber_w.glm");
	case CLASS_YODA: //Luke
		return G_ModelIndex("models/weapons2/saber_yoda/saber_w.glm");
	case CLASS_PLAYER: //Kyle NPC and player
	case CLASS_KYLE: //Kyle NPC and player
		return G_ModelIndex(DEFAULT_SABER_MODEL);
	default: //reborn and tavion and everyone else
		return G_ModelIndex(DEFAULT_SABER_MODEL);
	}
}

void wp_set_saber_ent_model_skin(const gentity_t* ent, gentity_t* saberent)
{
	int saber_model;
	qboolean new_model = qfalse;

	if (!ent->client->ps.saber[0].model)
	{
		saber_model = wp_set_saber_model(ent->client, ent->client->NPC_class);
	}
	else
	{
		//got saberModel from NPCs.cfg
		saber_model = G_ModelIndex(ent->client->ps.saber[0].model);
	}
	if (saber_model && saberent->s.modelindex != saber_model)
	{
		if (saberent->playerModel >= 0)
		{
			//remove the old one, if there is one
			gi.G2API_RemoveGhoul2Model(saberent->ghoul2, saberent->playerModel);
		}
		//add the new one
		saberent->playerModel = gi.G2API_InitGhoul2Model(saberent->ghoul2, ent->client->ps.saber[0].model, saber_model,
			NULL_HANDLE, NULL_HANDLE, 0, 0);
		saberent->s.modelindex = saber_model;
		new_model = qtrue;
	}
	//set skin, too
	if (ent->client->ps.saber[0].skin == nullptr)
	{
		gi.G2API_SetSkin(&saberent->ghoul2[0], -1, 0);
	}
	else
	{
		//if this saber has a customSkin, use it
		// lets see if it's out there
		const int saber_skin = gi.RE_RegisterSkin(ent->client->ps.saber[0].skin);
		if (saber_skin && (new_model || saberent->s.modelindex2 != saber_skin))
		{
			// put it in the config strings
			// and set the ghoul2 model to use it
			gi.G2API_SetSkin(&saberent->ghoul2[0], G_SkinIndex(ent->client->ps.saber[0].skin), saber_skin);
			saberent->s.modelindex2 = saber_skin;
		}
	}
}

void wp_saber_fall_sound(const gentity_t* owner, const gentity_t* saber)
{
	if (!saber)
	{
		return;
	}
	if (owner && owner->client)
	{
		//have an owner, use their data (assume saberNum is 0 because only the 0 saber can be thrown)
		if (owner->client->ps.saber[0].fallSound[0])
		{
			//have an override
			G_Sound(saber, owner->client->ps.saber[0].fallSound[Q_irand(0, 2)]);
		}
		else if (owner->client->ps.saber[0].type == SABER_SITH_SWORD)
		{
			//is a sith sword
			G_Sound(saber, G_SoundIndex(va("sound/weapons/sword/fall%d.wav", Q_irand(1, 7))));
		}
		else
		{
			//normal saber
			G_Sound(saber, G_SoundIndex(va("sound/weapons/saber/bounce%d.wav", Q_irand(1, 3))));
		}
	}
	else if (saber->NPC_type && saber->NPC_type[0])
	{
		//have a saber name to look up
		saberInfo_t saber_info;
		if (WP_SaberParseParms(saber->NPC_type, &saber_info))
		{
			//found it
			if (saber_info.fallSound[0])
			{
				//have an override sound
				G_Sound(saber, saber_info.fallSound[Q_irand(0, 2)]);
			}
			else if (saber_info.type == SABER_SITH_SWORD)
			{
				//is a sith sword
				G_Sound(saber, G_SoundIndex(va("sound/weapons/sword/fall%d.wav", Q_irand(1, 7))));
			}
			else
			{
				//normal saber
				G_Sound(saber, G_SoundIndex(va("sound/weapons/saber/bounce%d.wav", Q_irand(1, 3))));
			}
		}
		else
		{
			//can't find it
			G_Sound(saber, G_SoundIndex(va("sound/weapons/saber/bounce%d.wav", Q_irand(1, 3))));
		}
	}
	else
	{
		//no saber name specified
		G_Sound(saber, G_SoundIndex(va("sound/weapons/saber/bounce%d.wav", Q_irand(1, 3))));
	}
}

void wp_saber_swing_sound(const gentity_t* ent, int saberNum, const swingType_t swingType)
{
	int index = 1;
	if (!ent || !ent->client)
	{
		return;
	}
	if (swingType == SWING_FAST)
	{
		index = Q_irand(1, 3);
	}
	else if (swingType == SWING_MEDIUM)
	{
		index = Q_irand(4, 6);
	}
	else if (swingType == SWING_STRONG)
	{
		index = Q_irand(7, 9);
	}

	if (ent->client->ps.saber[saberNum].swingSound[0])
	{
		G_SoundIndexOnEnt(ent, CHAN_WEAPON, ent->client->ps.saber[saberNum].swingSound[Q_irand(0, 2)]);
	}
	else if (ent->client->ps.saber[saberNum].type == SABER_SITH_SWORD)
	{
		G_SoundOnEnt(ent, CHAN_WEAPON, va("sound/weapons/sword/swing%d.wav", Q_irand(1, 4)));
	}
	else
	{
		G_SoundOnEnt(ent, CHAN_WEAPON, va("sound/weapons/saber/saberhup%d.wav", index));
	}
}

void wp_saber_hit_sound(const gentity_t* ent, const int saber_num, const int blade_num)
{
	const qboolean saber_in_stab_down = PM_StabDownAnim(ent->client->ps.torsoAnim);
	const qboolean saber_in_special = PM_SaberInKillAttack(ent->client->ps.torsoAnim);
	const qboolean saber_in_over_head_attack = PM_SaberInOverHeadSlash(
		static_cast<saberMoveName_t>(ent->client->ps.saberMove));
	const qboolean saberInKata = PM_SaberInKata(static_cast<saberMoveName_t>(ent->client->ps.saberMove));
	const qboolean saberInLockWin = PM_SuperBreakWinAnim(ent->client->ps.torsoAnim);
	const qboolean saberInBackAttack = PM_SaberInBackAttack(static_cast<saberMoveName_t>(ent->client->ps.saberMove));
	const qboolean saberInLungeRoll = PM_LungRollAnim(ent->client->ps.torsoAnim);

	if (!ent || !ent->client)
	{
		return;
	}

	const int index = Q_irand(1, 15);
	const int index_stab = Q_irand(1, 11);
	const int index_kill = Q_irand(1, 17);
	const int indexspecial = Q_irand(1, 3);

	if (!WP_SaberBladeUseSecondBladeStyle(&ent->client->ps.saber[saber_num], blade_num)
		&& ent->client->ps.saber[saber_num].hitSound[0])
	{
		G_Sound(ent, ent->client->ps.saber[saber_num].hitSound[Q_irand(0, 2)]);
	}
	else if (WP_SaberBladeUseSecondBladeStyle(&ent->client->ps.saber[saber_num], blade_num)
		&& ent->client->ps.saber[saber_num].hit2Sound[0])
	{
		G_Sound(ent, ent->client->ps.saber[saber_num].hit2Sound[Q_irand(0, 2)]);
	}
	else if (ent->client->ps.saber[saber_num].type == SABER_SITH_SWORD)
	{
		G_Sound(ent, G_SoundIndex(va("sound/weapons/sword/stab%d.wav", Q_irand(1, 4))));
	}
	else
	{
		if (saber_in_stab_down || saber_in_over_head_attack)
		{
			G_Sound(ent, G_SoundIndex(va("sound/weapons/saber/saberstabdown%d.mp3", index_stab)));
		}
		else if (saber_in_special)
		{
			G_Sound(ent, G_SoundIndex(va("sound/weapons/saber/saber_perfectblock%d.mp3", indexspecial)));
		}
		else if (ent->enemy && ent->enemy->health <= 5 || saberInLungeRoll || saberInBackAttack || saberInLockWin
			|| saberInKata && (ent->enemy && ent->enemy->health <= 15))
		{
			G_Sound(ent, G_SoundIndex(va("sound/weapons/saber/saberkill%d.mp3", index_kill)));
		}
		else
		{
			G_Sound(ent, G_SoundIndex(va("sound/weapons/saber/saberhit%d.mp3", index)));
		}
	}
}

void wp_saber_lock_sound(const gentity_t* ent, gentity_t* hitEnt, const int saber_num, const int blade_num)
{
	if (!ent || !ent->client)
	{
		return;
	}
	const int index = Q_irand(1, 9);

	if (!WP_SaberBladeUseSecondBladeStyle(&ent->client->ps.saber[saber_num], blade_num)
		&& ent->client->ps.saber[saber_num].blockSound[0])
	{
		G_Sound(ent, ent->client->ps.saber[saber_num].blockSound[Q_irand(0, 2)]);
	}
	else if (WP_SaberBladeUseSecondBladeStyle(&ent->client->ps.saber[saber_num], blade_num)
		&& ent->client->ps.saber[saber_num].block2Sound[0])
	{
		G_Sound(ent, ent->client->ps.saber[saber_num].block2Sound[Q_irand(0, 2)]);
	}
	else
	{
		G_Sound(ent, G_SoundIndex(va("sound/weapons/saber/saberlock%d.mp3", index)));
	}
}

void wp_saber_block_sound(const gentity_t* ent, gentity_t* hitEnt, const int saber_num, const int blade_num)
{
	if (!ent || !ent->client)
	{
		return;
	}
	const int index = Q_irand(1, 90);

	if (!WP_SaberBladeUseSecondBladeStyle(&ent->client->ps.saber[saber_num], blade_num)
		&& ent->client->ps.saber[saber_num].blockSound[0])
	{
		G_Sound(ent, ent->client->ps.saber[saber_num].blockSound[Q_irand(0, 2)]);
	}
	else if (WP_SaberBladeUseSecondBladeStyle(&ent->client->ps.saber[saber_num], blade_num)
		&& ent->client->ps.saber[saber_num].block2Sound[0])
	{
		G_Sound(ent, ent->client->ps.saber[saber_num].block2Sound[Q_irand(0, 2)]);
	}
	else
	{
		G_Sound(ent, G_SoundIndex(va("sound/weapons/saber/saberblock%d.mp3", index)));
	}
}

void wp_saber_knock_sound(const gentity_t* ent, gentity_t* hitEnt, const int saber_num, const int blade_num)
{
	if (!ent || !ent->client)
	{
		return;
	}
	const int index = Q_irand(1, 4);

	if (!WP_SaberBladeUseSecondBladeStyle(&ent->client->ps.saber[saber_num], blade_num)
		&& ent->client->ps.saber[saber_num].blockSound[0])
	{
		G_Sound(ent, ent->client->ps.saber[saber_num].blockSound[Q_irand(0, 2)]);
	}
	else if (WP_SaberBladeUseSecondBladeStyle(&ent->client->ps.saber[saber_num], blade_num)
		&& ent->client->ps.saber[saber_num].block2Sound[0])
	{
		G_Sound(ent, ent->client->ps.saber[saber_num].block2Sound[Q_irand(0, 2)]);
	}
	else
	{
		G_Sound(ent, G_SoundIndex(va("sound/weapons/saber/saberknock%d.wav", index)));
	}
}

void wp_saber_bounce_on_wall_sound(const gentity_t* ent, const int saber_num, const int blade_num)
{
	if (!ent || !ent->client)
	{
		return;
	}
	const int index = Q_irand(1, 90);

	if (!WP_SaberBladeUseSecondBladeStyle(&ent->client->ps.saber[saber_num], blade_num)
		&& ent->client->ps.saber[saber_num].bounceSound[0])
	{
		G_Sound(ent, ent->client->ps.saber[saber_num].bounceSound[Q_irand(0, 2)]);
	}
	else if (WP_SaberBladeUseSecondBladeStyle(&ent->client->ps.saber[saber_num], blade_num)
		&& ent->client->ps.saber[saber_num].bounce2Sound[0])
	{
		G_Sound(ent, ent->client->ps.saber[saber_num].bounce2Sound[Q_irand(0, 2)]);
	}
	else if (!WP_SaberBladeUseSecondBladeStyle(&ent->client->ps.saber[saber_num], blade_num)
		&& ent->client->ps.saber[saber_num].blockSound[0])
	{
		G_Sound(ent, ent->client->ps.saber[saber_num].blockSound[Q_irand(0, 2)]);
	}
	else if (WP_SaberBladeUseSecondBladeStyle(&ent->client->ps.saber[saber_num], blade_num)
		&& ent->client->ps.saber[saber_num].block2Sound[0])
	{
		G_Sound(ent, ent->client->ps.saber[saber_num].block2Sound[Q_irand(0, 2)]);
	}
	else
	{
		G_Sound(ent, G_SoundIndex(va("sound/weapons/saber/saberblock%d.mp3", index)));
	}
}

void wp_saber_bounce_sound(const gentity_t* ent, gentity_t* hitEnt, const gentity_t* play_on_ent, const int saber_num,
	const int blade_num)
{
	if (!ent || !ent->client)
	{
		return;
	}
	const int index = Q_irand(1, 3);

	if (!play_on_ent)
	{
		play_on_ent = ent;
	}
	if (!WP_SaberBladeUseSecondBladeStyle(&ent->client->ps.saber[saber_num], blade_num)
		&& ent->client->ps.saber[saber_num].blockSound[0])
	{
		G_Sound(play_on_ent, ent->client->ps.saber[saber_num].blockSound[Q_irand(0, 2)]);
	}
	else if (WP_SaberBladeUseSecondBladeStyle(&ent->client->ps.saber[saber_num], blade_num)
		&& ent->client->ps.saber[saber_num].block2Sound[0])
	{
		G_Sound(play_on_ent, ent->client->ps.saber[saber_num].block2Sound[Q_irand(0, 2)]);
	}
	else
	{
		G_Sound(play_on_ent, G_SoundIndex(va("sound/weapons/saber/saberbounce%d.wav", index)));
	}
}

int wp_saber_init_blade_data(gentity_t* ent)
{
	if (!ent->client)
	{
		return 0;
	}
	if (true)
	{
		VectorClear(ent->client->renderInfo.muzzlePoint);
		VectorClear(ent->client->renderInfo.muzzlePointOld);
		VectorClear(ent->client->renderInfo.muzzleDir);
		VectorClear(ent->client->renderInfo.muzzleDirOld);

		for (auto& saber_num : ent->client->ps.saber)
		{
			for (auto& blade_num : saber_num.blade)
			{
				VectorClear(blade_num.muzzlePoint);
				VectorClear(blade_num.muzzlePointOld);
				VectorClear(blade_num.muzzleDir);
				VectorClear(blade_num.muzzleDirOld);
				blade_num.lengthOld = blade_num.length = 0;
				if (!blade_num.lengthMax)
				{
					if (ent->client->NPC_class == CLASS_DESANN)
					{
						//longer saber
						blade_num.lengthMax = 48;
					}
					else if (ent->client->NPC_class == CLASS_VADER)
					{
						//shorter saber
						blade_num.lengthMax = 48;
					}
					else if (ent->client->NPC_class == CLASS_REBORN)
					{
						//shorter saber
						blade_num.lengthMax = 38;
					}
					else if (ent->client->NPC_class == CLASS_YODA)
					{
						//shorter saber
						blade_num.lengthMax = 28;
					}
					else
					{
						//standard saber length
						blade_num.lengthMax = 40;
					}
				}
			}
		}
		ent->client->ps.saberLockEnemy = ENTITYNUM_NONE;
		ent->client->ps.saberLockTime = 0;
		if (ent->s.number)
		{
			if (!ent->client->ps.saberAnimLevel)
			{
				if (ent->client->NPC_class == CLASS_PLAYER)
				{
					ent->client->ps.saberAnimLevel = g_entities[0].client->ps.saberAnimLevel;
				}
				else
				{
					//?
					ent->client->ps.saberAnimLevel = Q_irand(SS_FAST, SS_TAVION);
				}
			}
		}
		else
		{
			if (!ent->client->ps.saberAnimLevel)
			{
				//initialize, but don't reset
				if (ent->s.number < MAX_CLIENTS)
				{
					if (!ent->client->ps.saberStylesKnown)
					{
						ent->client->ps.saberStylesKnown = 1 << SS_MEDIUM;
					}

					if (ent->client->ps.saberStylesKnown & 1 << SS_FAST)
					{
						ent->client->ps.saberAnimLevel = SS_FAST;
					}
					else if (ent->client->ps.saberStylesKnown & 1 << SS_STRONG)
					{
						ent->client->ps.saberAnimLevel = SS_STRONG;
					}
					else if (ent->client->ps.saberStylesKnown & 1 << SS_DESANN)
					{
						ent->client->ps.saberAnimLevel = SS_DESANN;
					}
					else if (ent->client->ps.saberStylesKnown & 1 << SS_TAVION)
					{
						ent->client->ps.saberAnimLevel = SS_TAVION;
					}
					else
					{
						ent->client->ps.saberAnimLevel = SS_MEDIUM;
					}
				}
				else
				{
					ent->client->ps.saberAnimLevel = SS_MEDIUM;
				}
			}

			cg.saberAnimLevelPending = ent->client->ps.saberAnimLevel;
			if (ent->client->sess.missionStats.weaponUsed[WP_SABER] <= 0)
			{
				//let missionStats know that we actually do have the saber, even if we never use it
				ent->client->sess.missionStats.weaponUsed[WP_SABER] = 1;
			}
		}
		ent->client->ps.saberAttackChainCount = MISHAPLEVEL_NONE;

		if (ent->client->ps.saberEntityNum <= 0 || ent->client->ps.saberEntityNum >= ENTITYNUM_WORLD)
		{
			//FIXME: if you do have a saber already, be sure to re-set the model if it's changed (say, via a script).
			gentity_t* saberent = G_Spawn();
			ent->client->ps.saberEntityNum = saberent->s.number;
			saberent->classname = "lightsaber";

			saberent->s.eType = ET_GENERAL;
			saberent->svFlags = SVF_USE_CURRENT_ORIGIN;
			saberent->s.weapon = WP_SABER;
			saberent->owner = ent;
			saberent->s.otherEntityNum = ent->s.number;
			//clear the enemy
			saberent->enemy = nullptr;

			saberent->clipmask = MASK_SOLID | CONTENTS_LIGHTSABER;
			saberent->contents = CONTENTS_LIGHTSABER; //|CONTENTS_SHOTCLIP;

			VectorSet(saberent->mins, -3.0f, -3.0f, -3.0f);
			VectorSet(saberent->maxs, 3.0f, 3.0f, 3.0f);
			saberent->mass = 10; //necc?

			saberent->s.eFlags |= EF_NODRAW;
			saberent->svFlags |= SVF_NOCLIENT;
			/*
			Ghoul2 Insert Start
			*/
			saberent->playerModel = -1;
			wp_set_saber_ent_model_skin(ent, saberent);

			// set up a bolt on the end so we can get where the sabre muzzle is - we can assume this is always bolt 0
			gi.G2API_AddBolt(&saberent->ghoul2[0], "*flash");
			if (ent->client->ps.dualSabers)
			{
				//int saber2 =
				G_ModelIndex(ent->client->ps.saber[1].model);
			}

			ent->client->ps.saberInFlight = qfalse;
			ent->client->ps.saberEntityDist = 0;
			ent->client->ps.saberEntityState = SES_LEAVING;

			ent->client->ps.saberMove = ent->client->ps.saberMoveNext = LS_NONE;

			//FIXME: need a think function to create alerts when turned on or is on, etc.
		}
		else
		{
			//already have one, might just be changing sabers, register the model and skin and use them if different from what we're using now.
			wp_set_saber_ent_model_skin(ent, &g_entities[ent->client->ps.saberEntityNum]);
		}
	}

	if (ent->client->ps.dualSabers)
	{
		return 2;
	}

	return 1;
}

void wp_saber_update_old_blade_data(gentity_t* ent)
{
	if (ent->client)
	{
		qboolean did_event = qfalse;
		for (auto& saber_num : ent->client->ps.saber)
		{
			for (int blade_num = 0; blade_num < saber_num.numBlades; blade_num++)
			{
				VectorCopy(saber_num.blade[blade_num].muzzlePoint, saber_num.blade[blade_num].muzzlePointOld);
				VectorCopy(saber_num.blade[blade_num].muzzleDir, saber_num.blade[blade_num].muzzleDirOld);
				if (!did_event)
				{
					if (saber_num.blade[blade_num].lengthOld <= 0 && saber_num.blade[blade_num].length > 0)
					{
						//just turned on
						//do sound event
						vec3_t saberOrg;
						VectorCopy(g_entities[ent->client->ps.saberEntityNum].currentOrigin, saberOrg);
						if (!ent->client->ps.saberInFlight && ent->client->ps.groundEntityNum == ENTITYNUM_WORLD
							//holding saber and on ground
							|| g_entities[ent->client->ps.saberEntityNum].s.pos.trType == TR_STATIONARY)
							//saber out there somewhere and on ground
						{
							//a ground alert
							AddSoundEvent(ent, saberOrg, 256, AEL_SUSPICIOUS, qfalse, qtrue);
						}
						else
						{
							//an in-air alert
							AddSoundEvent(ent, saberOrg, 256, AEL_SUSPICIOUS);
						}
						did_event = qtrue;
					}
				}
				saber_num.blade[blade_num].lengthOld = saber_num.blade[blade_num].length;
			}
		}
		VectorCopy(ent->client->renderInfo.muzzlePoint, ent->client->renderInfo.muzzlePointOld);
		VectorCopy(ent->client->renderInfo.muzzleDir, ent->client->renderInfo.muzzleDirOld);
	}
}

//SABER DAMAGE==============================================================================
//SABER DAMAGE==============================================================================
//SABER DAMAGE==============================================================================
//SABER DAMAGE==============================================================================
//SABER DAMAGE==============================================================================
//SABER DAMAGE==============================================================================
int wp_debug_saber_colour(const saber_colors_t saber_color)
{
	switch (static_cast<int>(saber_color))
	{
	case SABER_RED:
		return 0x000000ff;
	case SABER_ORANGE:
		return 0x000088ff;
	case SABER_YELLOW:
		return 0x0000ffff;
	case SABER_GREEN:
		return 0x0000ff00;
	case SABER_BLUE:
		return 0x00ff0000;
	case SABER_PURPLE:
		return 0x00ff00ff;
	case SABER_LIME:
		return 0x0000ff00;
	default:
		return 0x00ffffff; //white
	}
}

//This function gets the attack power which is used to decide broken parries,
//knockaways, and numerous other things. It is not directly related to the
//actual amount of damage done, however. -rww
static int g_saber_attack_power(const gentity_t* ent, const qboolean attacking)
{
	int base_level = ent->client->ps.saberAnimLevel;

	//Give "medium" strength for the two special stances.
	if (base_level == SS_DUAL)
	{
		base_level = 2;
	}
	else if (base_level == SS_STAFF)
	{
		base_level = 2;
	}

	if (attacking)
	{
		//the attacker gets a boost to help penetrate defense.
		//General boost up so the individual levels make a bigger difference.
		base_level *= 2;

		base_level++;

		vec3_t v_sub;
		int tolerance_amt;

		//We want different "tolerance" levels for adding in the distance of the last swing
		//to the base power level depending on which stance we are using. Otherwise fast
		//would have more advantage than it should since the animations are all much faster.
		switch (ent->client->ps.saberAnimLevel)
		{
		case SS_STRONG:
			tolerance_amt = 8;
			break;
		case SS_MEDIUM:
			tolerance_amt = 16;
			break;
		case SS_FAST:
			tolerance_amt = 24;
			break;
		default: //dual, staff, etc.
			tolerance_amt = 16;
			break;
		}

		VectorSubtract(ent->client->renderInfo.muzzlePoint, ent->client->renderInfo.muzzlePointOld, v_sub);
		int swing_dist = static_cast<int>(VectorLength(v_sub));

		while (swing_dist > 0)
		{
			//I would like to do something more clever. But I suppose this works, at least for now.
			base_level++;
			swing_dist -= tolerance_amt;
		}
	}

	//Cap at reasonable values now.
	if (base_level < 1)
	{
		base_level = 1;
	}
	else if (base_level > 16)
	{
		base_level = 16;
	}

	return base_level;
}

extern int pm_saber_deflection_for_quad(int quad);

qboolean wp_get_saber_deflection_angle(const gentity_t* attacker, const gentity_t* defender)
{
	vec3_t temp, att_saber_base, att_start_pos, saber_mid_next, att_hit_dir, att_hit_pos, def_blade_dir;

	if (!attacker || !attacker->client || attacker->client->ps.saberInFlight || attacker->client->ps.SaberLength() <= 0)
	{
		return qfalse;
	}
	if (!defender || !defender->client || defender->client->ps.saberInFlight || defender->client->ps.SaberLength() <= 0)
	{
		return qfalse;
	}
	if (PM_SuperBreakLoseAnim(attacker->client->ps.torsoAnim) || PM_SuperBreakWinAnim(attacker->client->ps.torsoAnim))
	{
		return qfalse;
	}

	attacker->client->ps.saberBounceMove = BLOCKED_BOUNCE_MOVE;

	//get the attacker's saber base pos at time of impact
	VectorSubtract(attacker->client->renderInfo.muzzlePoint, attacker->client->renderInfo.muzzlePointOld, temp);
	VectorMA(attacker->client->renderInfo.muzzlePointOld, saberHitFraction, temp, att_saber_base);

	//get the position along the length of the blade where the hit occurred
	const float att_saber_hit_length = Distance(saberHitLocation, att_saber_base) / attacker->client->ps.SaberLength();

	//now get the start of that midpoint in the swing and the actual impact point in the swing (shouldn't the latter just be saberHitLocation?)
	VectorMA(attacker->client->renderInfo.muzzlePointOld, att_saber_hit_length,
		attacker->client->renderInfo.muzzleDirOld, att_start_pos);
	VectorMA(attacker->client->renderInfo.muzzlePoint, att_saber_hit_length, attacker->client->renderInfo.muzzleDir,
		saber_mid_next);
	VectorSubtract(saber_mid_next, att_start_pos, att_hit_dir);
	VectorMA(att_start_pos, saberHitFraction, att_hit_dir, att_hit_pos);
	VectorNormalize(att_hit_dir);

	//get the defender's saber dir at time of impact
	VectorSubtract(defender->client->renderInfo.muzzleDirOld, defender->client->renderInfo.muzzleDir, temp);
	VectorMA(defender->client->renderInfo.muzzleDirOld, saberHitFraction, temp, def_blade_dir);

	//now compare
	const float hit_dot = DotProduct(att_hit_dir, def_blade_dir);

	const int att_saber_level = g_saber_attack_power(attacker, SaberAttacking(attacker));
	const int def_saber_level = g_saber_attack_power(defender, SaberAttacking(defender));

	if (defender->client->ps.ManualBlockingFlags & 1 << HOLDINGBLOCK || defender->client->ps.saberBlockingTime > level.
		time)
	{
		//Hmm, let's try just basing it off the anim
		const int att_quad_start = saberMoveData[attacker->client->ps.saberMove].startQuad;
		const int att_quad_end = saberMoveData[attacker->client->ps.saberMove].endQuad;
		int def_quad = saberMoveData[defender->client->ps.saberMove].endQuad;
		int quad_diff = fabs(static_cast<float>(def_quad - att_quad_start));

		if (defender->client->ps.saberMove == LS_READY)
		{
			return qfalse;
		}

		//reverse the left/right of the defQuad because of the mirrored nature of facing each other in combat
		switch (def_quad)
		{
		case Q_BR:
			def_quad = Q_BL;
			break;
		case Q_R:
			def_quad = Q_L;
			break;
		case Q_TR:
			def_quad = Q_TL;
			break;
		case Q_TL:
			def_quad = Q_TR;
			break;
		case Q_L:
			def_quad = Q_R;
			break;
		case Q_BL:
			def_quad = Q_BR;
			break;
		default:;
		}

		if (quad_diff > 4)
		{
			//wrap around so diff is never greater than 180 (4 * 45)
			quad_diff = 4 - (quad_diff - 4);
		}
		//have the quads, find a good anim to use
		if ((!quad_diff || quad_diff == 1 && Q_irand(0, 1))
			//defender pretty much stopped the attack at a 90 degree angle
			&& (def_saber_level == att_saber_level || Q_irand(0, def_saber_level - att_saber_level) >= 0))
			//and the defender's style is stronger
		{
			//bounce straight back
			attacker->client->ps.saberMove = PM_SaberBounceForAttack(attacker->client->ps.saberMove);
			attacker->client->ps.saberBlocked = BLOCKED_ATK_BOUNCE;
			return qfalse;
		}
		//attack hit at an angle, figure out what angle it should bounce off att
		quad_diff = def_quad - att_quad_end;
		//add half the diff of between the defense and attack end to the attack end
		if (quad_diff > 4)
		{
			quad_diff = 4 - (quad_diff - 4);
		}
		else if (quad_diff < -4)
		{
			quad_diff = -4 + (quad_diff + 4);
		}
		int newQuad = att_quad_end + ceil(static_cast<float>(quad_diff) / 2.0f);
		if (newQuad < Q_BR)
		{
			//less than zero wraps around
			newQuad = Q_B + newQuad;
		}
		if (newQuad == att_quad_start)
		{
			//never come off at the same angle that we would have if the attack was not interrupted
			if (Q_irand(0, 1))
			{
				newQuad--;
			}
			else
			{
				newQuad++;
			}
			if (newQuad < Q_BR)
			{
				newQuad = Q_B;
			}
			else if (newQuad > Q_B)
			{
				newQuad = Q_BR;
			}
		}
		if (newQuad == def_quad)
		{
			//bounce straight back
			attacker->client->ps.saberMove = PM_SaberBounceForAttack(attacker->client->ps.saberMove);
			attacker->client->ps.saberBlocked = BLOCKED_ATK_BOUNCE;
			return qfalse;
		}
		//else, pick a deflection
		attacker->client->ps.saberMove = pm_saber_deflection_for_quad(newQuad);
		attacker->client->ps.saberBlocked = BLOCKED_BOUNCE_MOVE;
		return qtrue;
	}
	if (hit_dot < 0.25f && hit_dot > -0.25f)
	{
		//hit pretty much perpendicular, pop straight back
		attacker->client->ps.saberBounceMove = PM_SaberBounceForAttack(attacker->client->ps.saberMove);
		attacker->client->ps.saberBlocked = BLOCKED_ATK_BOUNCE;
		return qfalse;
	}
	//a deflection
	vec3_t att_right, att_up, att_deflection_dir;

	//get the direction of the deflection
	VectorScale(def_blade_dir, hit_dot, att_deflection_dir);
	//get our bounce straight back direction
	VectorScale(att_hit_dir, -1.0f, temp);
	//add the bounce back and deflection
	VectorAdd(att_deflection_dir, temp, att_deflection_dir);
	//normalize the result to determine what direction our saber should bounce back toward
	VectorNormalize(att_deflection_dir);

	//need to know the direction of the deflection relative to the attacker's facing
	VectorSet(temp, 0, attacker->client->ps.viewangles[YAW], 0); //presumes no pitch!
	AngleVectors(temp, nullptr, att_right, att_up);
	const float swing_r_dot = DotProduct(att_right, att_deflection_dir);
	const float swing_u_dot = DotProduct(att_up, att_deflection_dir);

	if (swing_r_dot > 0.25f)
	{
		//deflect to right
		if (swing_u_dot > 0.25f)
		{
			//deflect to top
			attacker->client->ps.saberBounceMove = LS_D1_TR;
		}
		else if (swing_u_dot < -0.25f)
		{
			//deflect to bottom
			attacker->client->ps.saberBounceMove = LS_D1_BR;
		}
		else
		{
			//deflect horizontally
			attacker->client->ps.saberBounceMove = LS_D1__R;
		}
	}
	else if (swing_r_dot < -0.25f)
	{
		//deflect to left
		if (swing_u_dot > 0.25f)
		{
			//deflect to top
			attacker->client->ps.saberBounceMove = LS_D1_TL;
		}
		else if (swing_u_dot < -0.25f)
		{
			//deflect to bottom
			attacker->client->ps.saberBounceMove = LS_D1_BL;
		}
		else
		{
			//deflect horizontally
			attacker->client->ps.saberBounceMove = LS_D1__L;
		}
	}
	else
	{
		//deflect in middle
		if (swing_u_dot > 0.25f)
		{
			//deflect to top
			attacker->client->ps.saberBounceMove = LS_D1_T_;
		}
		else if (swing_u_dot < -0.25f)
		{
			//deflect to bottom
			attacker->client->ps.saberBounceMove = LS_D1_B_;
		}
		else
		{
			//deflect horizontally?  Well, no such thing as straight back in my face, so use top
			if (swing_r_dot > 0)
			{
				attacker->client->ps.saberBounceMove = LS_D1_TR;
			}
			else if (swing_r_dot < 0)
			{
				attacker->client->ps.saberBounceMove = LS_D1_TL;
			}
			else
			{
				attacker->client->ps.saberBounceMove = LS_D1_T_;
			}
		}
	}
	attacker->client->ps.saberBlocked = BLOCKED_BOUNCE_MOVE;
	return qtrue;
}

void wp_saber_clear_damage_for_ent_num(gentity_t* attacker, const int entity_num, const int saber_num,
	const int blade_num)
{
	if (d_saberCombat->integer || g_DebugSaberCombat->integer)
	{
		if (entity_num)
		{
			Com_Printf("clearing damage for entnum %d\n", entity_num);
		}
	}

	float knock_back_scale = 0.0f;
	if (attacker && attacker->client)
	{
		if (!WP_SaberBladeUseSecondBladeStyle(&attacker->client->ps.saber[saber_num], blade_num)
			&& attacker->client->ps.saber[saber_num].knockbackScale > 0.0f)
		{
			knock_back_scale = attacker->client->ps.saber[saber_num].knockbackScale;
		}
		else if (WP_SaberBladeUseSecondBladeStyle(&attacker->client->ps.saber[saber_num], blade_num)
			&& attacker->client->ps.saber[saber_num].knockbackScale2 > 0.0f)
		{
			knock_back_scale = attacker->client->ps.saber[saber_num].knockbackScale2;
		}
	}

	for (int i = 0; i < numVictims; i++)
	{
		if (victimEntityNum[i] == entity_num)
		{
			//hold on a sec, let's still do any accumulated knockback
			if (knock_back_scale)
			{
				gentity_t* victim = &g_entities[victimEntityNum[i]];
				if (victim && victim->client)
				{
					vec3_t center, dir_to_center;
					float knock_down_thresh_hold;
					const float knockback = knock_back_scale * totalDmg[i] * 0.5f;

					VectorAdd(victim->absmin, victim->absmax, center);
					VectorScale(center, 0.5, center);
					VectorSubtract(victim->currentOrigin, saberHitLocation, dir_to_center);
					VectorNormalize(dir_to_center);
					g_throw(victim, dir_to_center, knockback);
					if (victim->client->ps.groundEntityNum != ENTITYNUM_NONE
						&& dir_to_center[2] <= 0)
					{
						//hit downward on someone who is standing on firm ground, so more likely to knock them down
						knock_down_thresh_hold = Q_irand(25, 50);
					}
					else
					{
						knock_down_thresh_hold = Q_irand(75, 125);
					}

					if (knockback > knock_down_thresh_hold)
					{
						G_Knockdown(victim, attacker, dir_to_center, 350, qtrue);
					}
				}
			}
			//now clear everything
			totalDmg[i] = 0; //no damage
			hitLoc[i] = HL_NONE;
			hitDismemberLoc[i] = HL_NONE;
			hitDismember[i] = qfalse;
			victimEntityNum[i] = ENTITYNUM_NONE; //like we never hit him
		}
	}
}

extern void pm_saber_start_trans_anim(int saber_anim_level, int anim, float* anim_speed, const gentity_t* gent, int fatigued);
extern float PM_GetTimeScaleMod(const gentity_t* gent);

int g_get_attack_damage(const gentity_t* self, const int min_dmg, const int max_dmg, const float mult_point)
{
	int total_damage = max_dmg;
	float attack_anim_length = PM_AnimLength(self->client->clientInfo.animFileIndex,
		static_cast<animNumber_t>(self->client->ps.torsoAnim));
	constexpr float anim_speed_factor = 1.0f;
	float time_scale_mod = PM_GetTimeScaleMod(self);

	//Be sure to scale by the proper anim speed just as if we were going to play the animation

	pm_saber_start_trans_anim(self->client->ps.saberAnimLevel, self->client->ps.torsoAnim, &time_scale_mod, self, self->userInt3);

	const int speed_dif = attack_anim_length - attack_anim_length * anim_speed_factor;
	attack_anim_length += speed_dif;
	float peak_point = attack_anim_length;
	peak_point -= attack_anim_length * mult_point;

	//we treat torsoTimer as the point in the animation (closer it is to attackAnimLength, closer it is to beginning)
	const float current_point = self->client->ps.torsoAnimTimer;

	float damageFactor = current_point / peak_point;
	if (damageFactor > 1)
	{
		damageFactor = 2.0f - damageFactor;
		if (d_combatinfo->integer || g_DebugSaberCombat->integer)
		{
			gi.Printf(S_COLOR_RED"new damage system calculating extra damage %4.2f\n", damageFactor);
		}
	}

	total_damage *= damageFactor;

	if (total_damage < min_dmg)
	{
		total_damage = min_dmg;
	}
	if (total_damage > max_dmg)
	{
		total_damage = max_dmg;
	}

	return total_damage;
}

float g_get_anim_point(const gentity_t* self)
{
	float attack_anim_length = PM_AnimLength(self->client->clientInfo.animFileIndex,
		static_cast<animNumber_t>(self->client->ps.torsoAnim));
	float anim_speed_factor = 1.0f;

	//Be sure to scale by the proper anim speed just as if we were going to play the animation
	pm_saber_start_trans_anim(self->client->ps.saberAnimLevel, self->client->ps.torsoAnim, &anim_speed_factor, self, self->userInt3);

	const int speed_dif = attack_anim_length - attack_anim_length * anim_speed_factor;
	attack_anim_length += speed_dif;

	const float current_point = self->client->ps.torsoAnimTimer;

	const float anim_percentage = current_point / attack_anim_length;

	return anim_percentage;
}

extern float damageModifier[];
extern float hitLocHealthPercentage[];
extern qboolean BG_SaberInTransitionDamageMove(const playerState_t* ps);
qboolean BG_SaberInPartialDamageMove(gentity_t* self);

qboolean wp_saber_apply_damage(gentity_t* ent, const float base_damage, const int base_d_flags,
	const qboolean broken_parry, const int saber_num, const int blade_num,
	const qboolean thrown_saber)
{
	qboolean did_damage = qfalse;
	const saberType_t saber_type = ent->client->ps.saber[saber_num].type;
	float max_dmg;

	const qboolean active_blocking = ent->client->ps.ManualBlockingFlags & 1 << HOLDINGBLOCKANDATTACK ? qtrue : qfalse;
	const qboolean holding_block = ent->client->ps.ManualBlockingFlags & 1 << HOLDINGBLOCK ? qtrue : qfalse;
	//Normal Blocking
	const qboolean m_blocking = ent->client->ps.ManualBlockingFlags & 1 << PERFECTBLOCKING ? qtrue : qfalse;
	//Perfect Blocking

	if (!numVictims)
	{
		return qfalse;
	}

	if (ent->client->buttons & BUTTON_BLOCK && (ent->s.number < MAX_CLIENTS || G_ControlledByPlayer(ent)))
		//jacesolaris 2019 test for idlekill
	{
		return qfalse;
	}

	if ((holding_block || active_blocking || m_blocking || ent->client->ps.saberBlockingTime > level.time) && (ent->s.
		number < MAX_CLIENTS || G_ControlledByPlayer(ent))) //jacesolaris 2019 test for idlekill
	{
		return qfalse;
	}

	if (!(BG_SaberInNonIdleDamageMove(&ent->client->ps) || ent->client->ps.saberInFlight))
		//if not in a damage move like this dont do damage
	{
		return qfalse;
	}

	if (BG_SaberInTransitionDamageMove(&ent->client->ps)) //if in a transition dont do damage
	{
		return qfalse;
	}

	if (in_camera && !PM_SaberInAttack(ent->client->ps.saberMove) && g_saberRealisticCombat->integer > 1 && (ent->s.
		number >= MAX_CLIENTS && !G_ControlledByPlayer(ent)))
	{
		return qfalse;
	}

	if (!(ent->client->ps.forcePowersActive & 1 << FP_SPEED))
	{
		if (BG_SaberInPartialDamageMove(ent)) //if this is true dont do damage
		{
			return qfalse;
		}
	}

	if (PM_KickMove(ent->client->ps.saberMove))
	{
		return qfalse;
	}

	for (int i = 0; i < numVictims; i++)
	{
		int d_flags = base_d_flags | DAMAGE_DEATH_KNOCKBACK | DAMAGE_NO_HIT_LOC;

		if (victimEntityNum[i] != ENTITYNUM_NONE && &g_entities[victimEntityNum[i]] != nullptr)
		{
			// Don't bother with this damage if the fraction is higher than the saber's fraction
			if (dmgFraction[i] < saberHitFraction || broken_parry)
			{
				gentity_t* victim = &g_entities[victimEntityNum[i]];
				if (!victim)
				{
					continue;
				}

				if (victim->e_DieFunc == dieF_maglock_die)
				{
					//*sigh*, special check for maglocks
					vec3_t test_from;
					if (ent->client->ps.saberInFlight)
					{
						VectorCopy(g_entities[ent->client->ps.saberEntityNum].currentOrigin, test_from);
					}
					else
					{
						VectorCopy(ent->currentOrigin, test_from);
					}
					test_from[2] = victim->currentOrigin[2];
					trace_t test_trace;
					gi.trace(&test_trace, test_from, vec3_origin, vec3_origin, victim->currentOrigin, ent->s.number,
						MASK_SHOT, static_cast<EG2_Collision>(0), 0);
					if (test_trace.entityNum != victim->s.number)
					{
						//can only damage maglocks if have a clear trace to the thing's origin
						continue;
					}
				}

				if (totalDmg[i] > 0)
				{
					//actually want to do *some* damage here
					if (victim->client
						&& victim->client->NPC_class == CLASS_WAMPA
						&& victim->activator == ent)
					{
						//
					}
					else if (PM_SuperBreakWinAnim(ent->client->ps.torsoAnim))
					{
						//never cap the superbreak wins
					}
					else
					{
						const qboolean saber_in_special = pm_saber_in_special_attack(ent->client->ps.torsoAnim);
						const qboolean saber_in_stab_down = PM_StabDownAnim(ent->client->ps.torsoAnim);
						const qboolean saber_in_kata = PM_SaberInKata(
							static_cast<saberMoveName_t>(ent->client->ps.saberMove));
						const qboolean saber_in_back_attack = PM_SaberInBackAttack(
							static_cast<saberMoveName_t>(ent->client->ps.saberMove));
						const qboolean saber_in_over_head_attack = PM_SaberInOverHeadSlash(
							static_cast<saberMoveName_t>(ent->client->ps.saberMove));
						const qboolean saber_in_roll_stab = PM_SaberInRollStab(
							static_cast<saberMoveName_t>(ent->client->ps.saberMove));
						const qboolean saber_in_lunge_stab = PM_SaberInLungeStab(
							static_cast<saberMoveName_t>(ent->client->ps.saberMove));

						if (victim->client
							&& (victim->s.weapon == WP_SABER || victim->client->NPC_class == CLASS_REBORN || victim->
								client->NPC_class == CLASS_WAMPA)
							&& !g_saberRealisticCombat->integer)
						{
							//dmg vs other saber fighters is modded by hitloc and capped
							totalDmg[i] *= damageModifier[hitLoc[i]];

							if (hitLoc[i] == HL_NONE)
							{
								max_dmg = 33 * base_damage;
							}
							else
							{
								max_dmg = 50 * hitLocHealthPercentage[hitLoc[i]] * base_damage;
							}
							if (max_dmg < totalDmg[i])
							{
								totalDmg[i] = max_dmg;
							}
						}

						if (!saber_in_special) // not doing a special move
						{
							if (victim->s.weapon != WP_SABER || !Q_stricmp("func_breakable", victim->classname))
							{
								// my enemy has a gun
								if (g_saberRealisticCombat->integer < 3)
								{
									// enemy has more than 30hp
									if (g_saberRealisticCombat->integer == 2 && victim->NPC && victim->health >= 30)
									{
										d_flags |= DAMAGE_NO_KILL;
										totalDmg[i] = 30;
									}
									else if (g_saberRealisticCombat->integer == 1 && victim->NPC && victim->health >=
										30)
									{
										d_flags |= DAMAGE_NO_KILL;
										totalDmg[i] = 25;
									}
									else if (g_saberRealisticCombat->integer == 0 && victim->NPC && victim->health >=
										30)
									{
										d_flags |= DAMAGE_NO_KILL;
										totalDmg[i] = 10;
									}
									else
									{
										// enemy has less than 30 hp
										if (g_saberRealisticCombat->integer == 2)
										{
											if (totalDmg[i] < 25)
											{
												totalDmg[i] = 25;
											}
											if (totalDmg[i] > 25)
											{
												//clamp using same adjustment as in NPC_Begin
												totalDmg[i] = 25;
											}
										}
										else if (g_saberRealisticCombat->integer == 1)
										{
											if (totalDmg[i] < 15)
											{
												totalDmg[i] = 15;
											}
											if (totalDmg[i] > 15)
											{
												//clamp using same adjustment as in NPC_Begin
												totalDmg[i] = 15;
											}
										}
										else if (g_saberRealisticCombat->integer == 0)
										{
											if (totalDmg[i] < 5)
											{
												totalDmg[i] = 5;
											}
											if (totalDmg[i] > 5)
											{
												//clamp using same adjustment as in NPC_Begin
												totalDmg[i] = 5;
											}
										}
									}
								}
								else
								{
									// need to add better damage control here for saber v gunner at high damage
									if (totalDmg[i] < 50)
									{
										totalDmg[i] = 50;
									}
									if (totalDmg[i] > 100)
									{
										//clamp using same adjustment as in NPC_Begin
										totalDmg[i] = 100;
									}
								}
							}
							else
							{
								// my enemy has a saber
								if (g_saberRealisticCombat->integer < 3)
								{
									// enemy has more than 30hp
									if (g_saberRealisticCombat->integer == 2 && victim->NPC && victim->health >= 30)
									{
										d_flags |= DAMAGE_NO_KILL;
										totalDmg[i] = 30;
									}
									else if (g_saberRealisticCombat->integer == 1 && victim->NPC && victim->health >=
										30)
									{
										d_flags |= DAMAGE_NO_KILL;
										totalDmg[i] = 25;
									}
									else if (g_saberRealisticCombat->integer == 0 && victim->NPC && victim->health >=
										30)
									{
										d_flags |= DAMAGE_NO_KILL;
										totalDmg[i] = 10;
									}
									else
									{
										// enemy has less than 30 hp
										if (g_saberRealisticCombat->integer == 2)
										{
											if (totalDmg[i] < 25)
											{
												totalDmg[i] = 25;
											}
											if (totalDmg[i] > 25)
											{
												//clamp using same adjustment as in NPC_Begin
												totalDmg[i] = 25;
											}
										}
										else if (g_saberRealisticCombat->integer == 1)
										{
											if (totalDmg[i] < 15)
											{
												totalDmg[i] = 15;
											}
											if (totalDmg[i] > 15)
											{
												//clamp using same adjustment as in NPC_Begin
												totalDmg[i] = 15;
											}
										}
										else if (g_saberRealisticCombat->integer == 0)
										{
											if (totalDmg[i] < 5)
											{
												totalDmg[i] = 5;
											}
											if (totalDmg[i] > 5)
											{
												//clamp using same adjustment as in NPC_Begin
												totalDmg[i] = 5;
											}
										}
									}
								}
								else
								{
									// need to add better damage control here for saber v gunner at high damage
									if (totalDmg[i] < 50)
									{
										totalDmg[i] = 50;
									}
									if (totalDmg[i] > 100)
									{
										//clamp using same adjustment as in NPC_Begin
										totalDmg[i] = 100;
									}
								}
							}
						}
						else
						{
							// doing a special move
							if (victim->s.weapon != WP_SABER || !Q_stricmp("func_breakable", victim->classname))
							{
								//clamp the dmg between 25 and maxhealth
								if (totalDmg[i] < 25)
								{
									totalDmg[i] = 25;
								}
								if (totalDmg[i] > 100)
								{
									//clamp using same adjustment as in NPC_Begin
									totalDmg[i] = 100;
								}
							}
							else
							{
								if (saber_in_stab_down)
								{
									if (g_DebugSaberCombat->integer)
									{
										gi.Printf(S_COLOR_RED"saberInStabDown\n");
									}
									if (totalDmg[i] < 100)
									{
										totalDmg[i] = 100;
									}
								}
								else if (saber_in_kata)
								{
									if (d_combatinfo->integer || g_DebugSaberCombat->integer)
									{
										gi.Printf(S_COLOR_RED"saber_in_kata\n");
									}
									totalDmg[i] = g_get_attack_damage(ent, 150, 250, 0.65f);
								}
								else if (saber_in_back_attack)
								{
									if (d_combatinfo->integer || g_DebugSaberCombat->integer)
									{
										gi.Printf(S_COLOR_RED"saberInBackAttack\n");
									}
									if (ent->client->ps.saberAnimLevel == SS_DESANN || ent->client->ps.saberAnimLevel ==
										SS_STRONG)
									{
										totalDmg[i] = g_get_attack_damage(ent, 50, 100, 0.5f);
									}
									else if (ent->client->ps.saberAnimLevel == SS_MEDIUM)
									{
										totalDmg[i] = g_get_attack_damage(ent, 30, 60, 0.5f);
									}
									else if (ent->client->ps.saberAnimLevel == SS_FAST || ent->client->ps.saberAnimLevel
										== SS_TAVION)
									{
										totalDmg[i] = g_get_attack_damage(ent, 20, 40, 0.5f);
									}
									else // SS_STAFF // SS_DUAL
									{
										totalDmg[i] = g_get_attack_damage(ent, 60, 70, 0.5f);
									}
								}
								else if (saber_in_over_head_attack)
								{
									if (d_combatinfo->integer || g_DebugSaberCombat->integer)
									{
										gi.Printf(S_COLOR_RED"saberInOverHeadAttack\n");
									}
									if (ent->client->ps.saberAnimLevel == SS_DESANN || ent->client->ps.saberAnimLevel ==
										SS_STRONG)
									{
										totalDmg[i] = g_get_attack_damage(ent, 50, 100, 0.5f);
									}
									else if (ent->client->ps.saberAnimLevel == SS_MEDIUM)
									{
										totalDmg[i] = g_get_attack_damage(ent, 20, 80, 0.5f);
									}
									else if (ent->client->ps.saberAnimLevel == SS_FAST || ent->client->ps.saberAnimLevel
										== SS_TAVION)
									{
										totalDmg[i] = g_get_attack_damage(ent, 10, 40, 0.5f);
									}
									else // SS_STAFF // SS_DUAL
									{
										totalDmg[i] = g_get_attack_damage(ent, 40, 70, 0.5f);
									}
								}
								else if (saber_in_roll_stab)
								{
									if (d_combatinfo->integer || g_DebugSaberCombat->integer)
									{
										gi.Printf(S_COLOR_YELLOW"SaberInRollStab\n");
									}
									totalDmg[i] = g_get_attack_damage(ent, 50, 80, 0.5f);
								}
								else if (saber_in_lunge_stab)
								{
									if (d_combatinfo->integer || g_DebugSaberCombat->integer)
									{
										gi.Printf(S_COLOR_YELLOW"SaberInLungeStab\n");
									}
									totalDmg[i] = g_get_attack_damage(ent, 40, 80, 0.5f);
								}
								else
								{
									if (d_combatinfo->integer || g_DebugSaberCombat->integer)
									{
										gi.Printf(S_COLOR_RED"saberInSpecial\n");
									}
									totalDmg[i] = g_get_attack_damage(ent, 75, 180, 0.65f);
								}
							}
						}
					}

					if (totalDmg[i] > 0)
					{
						gentity_t* inflictor = ent;
						did_damage = qtrue;
						qboolean vic_was_dismembered = qtrue;
						const auto vic_was_alive = static_cast<qboolean>(victim->health > 0);

						if (base_damage <= 0.1f)
						{
							//just get their attention?
							d_flags |= DAMAGE_NO_DAMAGE;
							G_SaberBounce(ent, victim);
						}

						if (victim->client)
						{
							if (victim->client->ps.pm_time > 0 && victim->client->ps.pm_flags & PMF_TIME_KNOCKBACK &&
								victim->client->ps.velocity[2] > 0)
							{
								//already being knocked around
								d_flags |= DAMAGE_NO_KNOCKBACK;
								G_SaberBounce(ent, victim);
							}
							if (!WP_SaberBladeUseSecondBladeStyle(&ent->client->ps.saber[saber_num], blade_num)
								&& ent->client->ps.saber[saber_num].saberFlags2 & SFL2_NO_DISMEMBERMENT)
							{
								//no dismemberment! (blunt/stabbing weapon?)
								G_SaberBounce(ent, victim);
							}
							else if (WP_SaberBladeUseSecondBladeStyle(&ent->client->ps.saber[saber_num], blade_num)
								&& ent->client->ps.saber[saber_num].saberFlags2 & SFL2_NO_DISMEMBERMENT2)
							{
								//no dismemberment! (blunt/stabbing weapon?)
								G_SaberBounce(ent, victim);
							}
							else
							{
								if (g_dismemberProbabilities->value > 0.0f)
								{
									d_flags |= DAMAGE_DISMEMBER;
									if (hitDismember[i])
									{
										victim->client->dismembered = false;
										G_SaberBounce(ent, victim);
									}
								}
								else if (hitDismember[i])
								{
									d_flags |= DAMAGE_DISMEMBER;
								}
								if (!victim->client->dismembered)
								{
									vic_was_dismembered = qfalse;
									G_SaberBounce(ent, victim);
								}
							}
							if (base_damage <= 1.0f)
							{
								//very mild damage
								if (victim->s.number == 0 || victim->client->ps.weapon == WP_SABER || victim->client->
									NPC_class == CLASS_GALAKMECH)
								{
									//if it's the player or a saber-user, don't kill them with this blow
									//dFlags |= DAMAGE_NO_KILL;
									G_SaberBounce(ent, victim);

									if (victim->health > 20)
									{
										d_flags |= DAMAGE_NO_KILL;
									}
								}
							}
						}
						else
						{
							if (victim->takedamage)
							{
								//some other breakable thing//create a flash here
								if (!g_noClashFlare)
								{
									g_saberFlashTime = level.time - 50;
									VectorCopy(dmgSpot[i], g_saberFlashPos);
									G_SaberBounce(ent, victim);
								}
							}
						}

						if (!PM_SuperBreakWinAnim(ent->client->ps.torsoAnim)
							&& !PM_StabDownAnim(ent->client->ps.torsoAnim)
							&& !g_saberRealisticCombat->integer ///// low damage (training)
							&& g_saberDamageCapping->integer)
						{
							//never cap the superbreak wins
							if (victim->client && victim->s.number >= MAX_CLIENTS)
							{
								if (victim->client->NPC_class == CLASS_SHADOWTROOPER
									|| victim->NPC && victim->NPC->aiFlags & NPCAI_BOSS_CHARACTER)
								{
									//hit a boss character
									const int dmg = (3 - g_spskill->integer) * 5 + 10;

									if (totalDmg[i] > dmg)
									{
										totalDmg[i] = dmg;
									}
								}
								else if (victim->client->ps.weapon == WP_SABER
									|| victim->client->NPC_class == CLASS_REBORN
									|| victim->client->NPC_class == CLASS_JEDI)
								{
									//hit a non-boss saber-user
									const int dmg = (3 - g_spskill->integer) * 15 + 30;
									if (totalDmg[i] > dmg)
									{
										totalDmg[i] = dmg;
									}
								}
							}
							if (victim->s.number < MAX_CLIENTS && ent->NPC)
							{
								if (ent->NPC->aiFlags & NPCAI_BOSS_CHARACTER
									|| ent->NPC->aiFlags & NPCAI_SUBBOSS_CHARACTER
									|| ent->client->NPC_class == CLASS_SHADOWTROOPER)
								{
									//player hit by a boss character
									const int dmg = (g_spskill->integer + 1) * 4 + 3;
									if (totalDmg[i] > dmg)
									{
										totalDmg[i] = dmg;
									}
								}
								else if (g_spskill->integer < 3) //was < 2
								{
									//player hit by any enemy //on easy or medium?
									const int dmg = (g_spskill->integer + 1) * 10 + 20;
									if (totalDmg[i] > dmg)
									{
										totalDmg[i] = dmg;
									}
								}
							}
						}

						d_flags |= DAMAGE_NO_KNOCKBACK; //okay, let's try no knockback whatsoever...
						d_flags &= ~DAMAGE_DEATH_KNOCKBACK;
						if (g_saberRealisticCombat->integer)
						{
							d_flags |= DAMAGE_NO_KNOCKBACK;
							d_flags &= ~DAMAGE_DEATH_KNOCKBACK;
							d_flags &= ~DAMAGE_NO_KILL;
							G_SaberBounce(ent, victim);
						}
						if (ent->client && !ent->s.number)
						{
							switch (hitLoc[i])
							{
							case HL_FOOT_RT:
							case HL_FOOT_LT:
							case HL_LEG_RT:
							case HL_LEG_LT:
								ent->client->sess.missionStats.legAttacksCnt++;
								break;
							case HL_WAIST:
							case HL_BACK_RT:
							case HL_BACK_LT:
							case HL_BACK:
							case HL_CHEST_RT:
							case HL_CHEST_LT:
							case HL_CHEST:
								ent->client->sess.missionStats.torsoAttacksCnt++;
								break;
							case HL_ARM_RT:
							case HL_ARM_LT:
							case HL_HAND_RT:
							case HL_HAND_LT:
								ent->client->sess.missionStats.armAttacksCnt++;
								break;
							default:
								ent->client->sess.missionStats.otherAttacksCnt++;
								break;
							}
						}

						if (saber_type == SABER_SITH_SWORD)
						{
							//do knockback
							d_flags &= ~(DAMAGE_NO_KNOCKBACK | DAMAGE_DEATH_KNOCKBACK);
							G_SaberBounce(ent, victim);
						}
						if (!WP_SaberBladeUseSecondBladeStyle(&ent->client->ps.saber[saber_num], blade_num)
							&& ent->client->ps.saber[saber_num].knockbackScale > 0.0f)
						{
							d_flags &= ~(DAMAGE_NO_KNOCKBACK | DAMAGE_DEATH_KNOCKBACK);
							if (saber_num < 1)
							{
								d_flags |= DAMAGE_SABER_KNOCKBACK1;
							}
							else
							{
								d_flags |= DAMAGE_SABER_KNOCKBACK2;
							}
						}
						else if (WP_SaberBladeUseSecondBladeStyle(&ent->client->ps.saber[saber_num], blade_num)
							&& ent->client->ps.saber[saber_num].knockbackScale2 > 0.0f)
						{
							d_flags &= ~(DAMAGE_NO_KNOCKBACK | DAMAGE_DEATH_KNOCKBACK);
							if (saber_num < 1)
							{
								d_flags |= DAMAGE_SABER_KNOCKBACK1_B2;
							}
							else
							{
								d_flags |= DAMAGE_SABER_KNOCKBACK2_B2;
							}
						}
						if (thrown_saber)
						{
							inflictor = &g_entities[ent->client->ps.saberEntityNum];
						}
						int damage;
						if (!WP_SaberBladeUseSecondBladeStyle(&ent->client->ps.saber[saber_num], blade_num)
							&& ent->client->ps.saber[saber_num].damageScale != 1.0f)
						{
							damage = ceil(totalDmg[i] * ent->client->ps.saber[saber_num].damageScale);
						}
						else if (WP_SaberBladeUseSecondBladeStyle(&ent->client->ps.saber[saber_num], blade_num)
							&& ent->client->ps.saber[saber_num].damageScale2 != 1.0f)
						{
							damage = ceil(totalDmg[i] * ent->client->ps.saber[saber_num].damageScale2);
						}
						else
						{
							damage = ceil(totalDmg[i]);
						}
						G_Damage(victim, inflictor, ent, dmgDir[i], dmgSpot[i], damage, d_flags, MOD_SABER,
							hitDismemberLoc[i]);
						if (damage > 0 && cg.time)
						{
							float size_time_scale = 1.0f;
							if (vic_was_alive
								&& victim->health <= 0
								|| !vic_was_dismembered
								&& victim->client->dismembered
								&& hitDismemberLoc[i] != HL_NONE
								&& hitDismember[i])
							{
								size_time_scale = 3.0f;
							}

							if (damage > 10) //only if doing damage move
							{
								cg_saber_do_weapon_hit_marks(ent->client,
									ent->client->ps.saberInFlight
									? &g_entities[ent->client->ps.saberEntityNum]
									: nullptr,
									victim,
									saber_num,
									blade_num,
									dmgSpot[i],
									dmgDir[i],
									dmgBladeVec[i],
									dmgNormal[i],
									size_time_scale);
							}
						}
						if (d_saberCombat->integer || g_DebugSaberCombat->integer)
						{
							if (d_flags & DAMAGE_NO_DAMAGE)
							{
								gi.Printf(S_COLOR_RED"damage: fake, hitLoc %d\n", hitLoc[i]);
							}
							else
							{
								gi.Printf(S_COLOR_RED"damage: %4.2f, hitLoc %d\n", totalDmg[i], hitLoc[i]);
							}
						}
						//do the effect
						if (ent->s.number == 0)
						{
							AddSoundEvent(victim->owner, dmgSpot[i], 256, AEL_DISCOVERED);
							AddSightEvent(victim->owner, dmgSpot[i], 512, AEL_DISCOVERED, 50);
						}
						if (ent->client)
						{
							if (ent->enemy && ent->enemy == victim)
							{
								//just so Jedi knows that he hit his enemy
								ent->client->ps.saberEventFlags |= SEF_HITENEMY;
							}
							else
							{
								ent->client->ps.saberEventFlags |= SEF_HITOBJECT;
							}
						}
					}
				}
			}
		}
	}
	return did_damage;
}

void wp_saber_damage_add(const float tr_dmg, const int tr_victim_entity_num, vec3_t tr_dmg_dir, vec3_t tr_dmg_blade_vec,
	vec3_t tr_dmg_normal, vec3_t tr_dmg_spot, const float dmg, const float fraction,
	const int tr_hit_loc, const qboolean tr_dismember, const int tr_dismember_loc)
{
	if (tr_victim_entity_num < 0 || tr_victim_entity_num >= ENTITYNUM_WORLD)
	{
		return;
	}
	if (tr_dmg * dmg < 10.0f)
	{
		//too piddly an amount of damage to really count?
	}
	if (tr_dmg)
	{
		int i;
		int cur_victim = 0;
		//did some damage to something
		for (i = 0; i < numVictims; i++)
		{
			if (victimEntityNum[i] == tr_victim_entity_num)
			{
				//already hit this guy before
				cur_victim = i;
				break;
			}
		}
		if (i == numVictims)
		{
			//haven't hit his guy before
			if (numVictims + 1 >= MAX_SABER_VICTIMS)
			{
				//can't add another victim at this time
				return;
			}
			//add a new victim to the list
			cur_victim = numVictims;
			victimEntityNum[numVictims++] = tr_victim_entity_num;
		}

		const float add_dmg = tr_dmg * dmg;
		if (tr_hit_loc != HL_NONE && (hitLoc[cur_victim] == HL_NONE || hitLocHealthPercentage[tr_hit_loc] >
			hitLocHealthPercentage[hitLoc[cur_victim]]))
		{
			//this hitLoc is more critical than the previous one this frame
			hitLoc[cur_victim] = tr_hit_loc;
		}

		totalDmg[cur_victim] += add_dmg;
		if (!VectorLengthSquared(dmgDir[cur_victim]))
		{
			VectorCopy(tr_dmg_dir, dmgDir[cur_victim]);
		}
		if (!VectorLengthSquared(dmgBladeVec[cur_victim]))
		{
			VectorCopy(tr_dmg_blade_vec, dmgBladeVec[cur_victim]);
		}
		if (!VectorLengthSquared(dmgNormal[cur_victim]))
		{
			VectorCopy(tr_dmg_normal, dmgNormal[cur_victim]);
		}
		if (!VectorLengthSquared(dmgSpot[cur_victim]))
		{
			VectorCopy(tr_dmg_spot, dmgSpot[cur_victim]);
		}

		// Make sure we keep track of the fraction.  Why?
		// Well, if the saber hits something that stops it, the damage isn't done past that point.
		dmgFraction[cur_victim] = fraction;
		if (tr_dismember_loc != HL_NONE && hitDismemberLoc[cur_victim] == HL_NONE
			|| !hitDismember[cur_victim] && tr_dismember)
		{
			//either this is the first dismember loc we got or we got a loc before, but it wasn't a dismember loc, so take the new one
			hitDismemberLoc[cur_victim] = tr_dismember_loc;
		}
		if (tr_dismember)
		{
			//we scored a dismemberment hit...
			hitDismember[cur_victim] = tr_dismember;
		}
	}
}

/*
WP_SabersIntersect

Breaks the two saber paths into 2 tris each and tests each tri for the first saber path against each of the other saber path's tris

FIXME: subdivide the arc into a consistent increment
FIXME: test the intersection to see if the sabers really did intersect (weren't going in the same direction and/or passed through same point at different times)?
*/
extern qboolean tri_tri_intersect(vec3_t V0, vec3_t V1, vec3_t V2, vec3_t U0, vec3_t U1, vec3_t U2);
extern void G_TestLine(vec3_t start, vec3_t end, int color, int time);

qboolean wp_sabers_intersect(const gentity_t* ent1, const int ent1_saber_num, const int ent1_blade_num,
	const gentity_t* ent2, const qboolean check_dir)
{
	if (!ent1 || !ent2)
	{
		return qfalse;
	}
	if (!ent1->client || !ent2->client)
	{
		return qfalse;
	}
	if (ent1->client->ps.SaberLength() <= 0 || ent2->client->ps.SaberLength() <= 0)
	{
		return qfalse;
	}

	for (const auto& ent2_saber_num : ent2->client->ps.saber)
	{
		for (int ent2_blade_num = 0; ent2_blade_num < ent2_saber_num.numBlades; ent2_blade_num
			++)
		{
			if (ent2_saber_num.type != SABER_NONE
				&& ent2_saber_num.blade[ent2_blade_num].length > 0)
			{
				vec3_t dir;
				vec3_t saber_tip_next2;
				vec3_t saber_base_next2;
				vec3_t saber_tip2;
				vec3_t saber_base2;
				vec3_t saber_tip_next1;
				vec3_t saber_base_next1;
				vec3_t saber_tip1;
				vec3_t saber_base1;
				//valid saber and this blade is on
				VectorCopy(ent1->client->ps.saber[ent1_saber_num].blade[ent1_blade_num].muzzlePointOld, saber_base1);
				VectorCopy(ent1->client->ps.saber[ent1_saber_num].blade[ent1_blade_num].muzzlePoint, saber_base_next1);
				VectorSubtract(ent1->client->ps.saber[ent1_saber_num].blade[ent1_blade_num].muzzlePoint,
					ent1->client->ps.saber[ent1_saber_num].blade[ent1_blade_num].muzzlePointOld, dir);
				VectorNormalize(dir);
				VectorMA(saber_base_next1, SABER_EXTRAPOLATE_DIST, dir, saber_base_next1);
				VectorMA(saber_base1, ent1->client->ps.saber[ent1_saber_num].blade[ent1_blade_num].length,
					ent1->client->ps.saber[ent1_saber_num].blade[ent1_blade_num].muzzleDirOld, saber_tip1);
				VectorMA(saber_base_next1, ent1->client->ps.saber[ent1_saber_num].blade[ent1_blade_num].length,
					ent1->client->ps.saber[ent1_saber_num].blade[ent1_blade_num].muzzleDir, saber_tip_next1);
				VectorSubtract(saber_tip_next1, saber_tip1, dir);
				VectorNormalize(dir);
				VectorMA(saber_tip_next1, SABER_EXTRAPOLATE_DIST, dir, saber_tip_next1);

				VectorCopy(ent2_saber_num.blade[ent2_blade_num].muzzlePointOld, saber_base2);
				VectorCopy(ent2_saber_num.blade[ent2_blade_num].muzzlePoint, saber_base_next2);
				VectorSubtract(ent2_saber_num.blade[ent2_blade_num].muzzlePoint,
					ent2_saber_num.blade[ent2_blade_num].muzzlePointOld, dir);
				VectorNormalize(dir);
				VectorMA(saber_base_next2, SABER_EXTRAPOLATE_DIST, dir, saber_base_next2);
				VectorMA(saber_base2, ent2_saber_num.blade[ent2_blade_num].length,
					ent2_saber_num.blade[ent2_blade_num].muzzleDirOld, saber_tip2);
				VectorMA(saber_base_next2, ent2_saber_num.blade[ent2_blade_num].length,
					ent2_saber_num.blade[ent2_blade_num].muzzleDir, saber_tip_next2);
				VectorSubtract(saber_tip_next2, saber_tip2, dir);
				VectorNormalize(dir);
				VectorMA(saber_tip_next2, SABER_EXTRAPOLATE_DIST, dir, saber_tip_next2);

				if (check_dir)
				{
					//check the direction of the two swings to make sure the sabers are swinging towards each other
					vec3_t saberDir1, saberDir2;

					VectorSubtract(saber_tip_next1, saber_tip1, saberDir1);
					VectorSubtract(saber_tip_next2, saber_tip2, saberDir2);
					VectorNormalize(saberDir1);
					VectorNormalize(saberDir2);
					if (DotProduct(saberDir1, saberDir2) > 0.6f)
					{
						//sabers moving in same dir, probably didn't actually hit
						continue;
					}
					//now check orientation of sabers, make sure they're not parallel or close to it
					const float dot = DotProduct(ent1->client->ps.saber[ent1_saber_num].blade[ent1_blade_num].muzzleDir,
						ent2_saber_num.blade[ent2_blade_num].
						muzzleDir);
					if (dot > 0.9f || dot < -0.9f)
					{
						//too parallel to really block effectively?
						continue;
					}
				}

				if (tri_tri_intersect(saber_base1, saber_tip1, saber_base_next1, saber_base2, saber_tip2,
					saber_base_next2))
				{
					return qtrue;
				}
				if (tri_tri_intersect(saber_base1, saber_tip1, saber_base_next1, saber_base2, saber_tip2,
					saber_tip_next2))
				{
					return qtrue;
				}
				if (tri_tri_intersect(saber_base1, saber_tip1, saber_tip_next1, saber_base2, saber_tip2,
					saber_base_next2))
				{
					return qtrue;
				}
				if (tri_tri_intersect(saber_base1, saber_tip1, saber_tip_next1, saber_base2, saber_tip2,
					saber_tip_next2))
				{
					return qtrue;
				}
			}
		}
	}
	return qfalse;
}

float wp_sabers_distance(const gentity_t* ent1, const gentity_t* ent2)
{
	vec3_t saber_base_next1;
	vec3_t saber_tip_next1;
	vec3_t saber_point1;
	vec3_t saber_base_next2;
	vec3_t saber_tip_next2;
	vec3_t saber_point2;

	if (!ent1 || !ent2)
	{
		return qfalse;
	}
	if (!ent1->client || !ent2->client)
	{
		return qfalse;
	}
	if (ent1->client->ps.SaberLength() <= 0 || ent2->client->ps.SaberLength() <= 0)
	{
		return qfalse;
	}

	VectorCopy(ent1->client->ps.saber[0].blade[0].muzzlePoint, saber_base_next1);
	VectorMA(saber_base_next1, ent1->client->ps.saber[0].blade[0].length, ent1->client->ps.saber[0].blade[0].muzzleDir,
		saber_tip_next1);

	VectorCopy(ent2->client->ps.saber[0].blade[0].muzzlePoint, saber_base_next2);
	VectorMA(saber_base_next2, ent2->client->ps.saber[0].blade[0].length, ent2->client->ps.saber[0].blade[0].muzzleDir,
		saber_tip_next2);

	const float sabers_dist = ShortestLineSegBewteen2LineSegs(saber_base_next1, saber_tip_next1, saber_base_next2,
		saber_tip_next2, saber_point1, saber_point2);

	return sabers_dist;
}

qboolean wp_sabers_intersection(const gentity_t* ent1, const gentity_t* ent2, vec3_t intersect)
{
	float best_line_seg_length = Q3_INFINITE;

	if (!ent1 || !ent2)
	{
		return qfalse;
	}
	if (!ent1->client || !ent2->client)
	{
		return qfalse;
	}
	if (ent1->client->ps.SaberLength() <= 0 || ent2->client->ps.SaberLength() <= 0)
	{
		return qfalse;
	}

	//UGH, had to make this work for multiply-bladed sabers
	for (const auto& saber_num1 : ent1->client->ps.saber)
	{
		for (int blade_num1 = 0; blade_num1 < saber_num1.numBlades; blade_num1++)
		{
			if (saber_num1.type != SABER_NONE
				&& saber_num1.blade[blade_num1].length > 0)
			{
				//valid saber and this blade is on
				for (const auto& saber_num2 : ent2->client->ps.saber)
				{
					for (int blade_num2 = 0; blade_num2 < saber_num2.numBlades; blade_num2++)
					{
						if (saber_num2.type != SABER_NONE
							&& saber_num2.blade[blade_num2].length > 0)
						{
							vec3_t saber_point2;
							vec3_t saber_tip_next2;
							vec3_t saber_base_next2;
							vec3_t saber_point1;
							vec3_t saber_tip_next1;
							vec3_t saber_base_next1;
							//valid saber and this blade is on
							VectorCopy(saber_num1.blade[blade_num1].muzzlePoint,
								saber_base_next1);
							VectorMA(saber_base_next1, saber_num1.blade[blade_num1].length,
								saber_num1.blade[blade_num1].muzzleDir, saber_tip_next1);

							VectorCopy(saber_num2.blade[blade_num2].muzzlePoint,
								saber_base_next2);
							VectorMA(saber_base_next2, saber_num2.blade[blade_num2].length,
								saber_num2.blade[blade_num2].muzzleDir, saber_tip_next2);

							const float line_seg_length = ShortestLineSegBewteen2LineSegs(
								saber_base_next1, saber_tip_next1, saber_base_next2, saber_tip_next2, saber_point1,
								saber_point2);
							if (line_seg_length < best_line_seg_length)
							{
								best_line_seg_length = line_seg_length;
								VectorAdd(saber_point1, saber_point2, intersect);
								VectorScale(intersect, 0.5, intersect);
							}
						}
					}
				}
			}
		}
	}
	return qtrue;
}

const char* hit_blood_sparks = "sparks/blood_sparks2";
// could have changed this effect directly, but this is just safer in case anyone anywhere else is using the old one for something?
const char* hit_sparks = "saber/saber_cut";
const char* hit_sparks2 = "saber/saber_bodyhit";
const char* hit_saber_cut_droid = "saber/saber_cut_droid";
const char* hit_saber_touch_droid = "saber/saber_touch_droid";

qboolean wp_saber_damage_effects(trace_t* tr, const vec3_t start, const float length, const float dmg, vec3_t dmg_dir,
	vec3_t blade_vec,
	const int enemy_team, const saberType_t saber_type, const saberInfo_t* saber,
	const int blade_num)
{
	int hit_ent_num[MAX_G2_COLLISIONS];
	for (int& hen : hit_ent_num)
	{
		hen = ENTITYNUM_NONE;
	}
	//NOTE: = {0} does NOT work on anything but bytes?
	float hit_ent_dmg_add[MAX_G2_COLLISIONS] = { 0 };
	float hit_ent_dmg_sub[MAX_G2_COLLISIONS] = { 0 };
	vec3_t hit_ent_point[MAX_G2_COLLISIONS];
	vec3_t hit_ent_normal[MAX_G2_COLLISIONS];
	vec3_t blade_dir;
	float hit_ent_start_frac[MAX_G2_COLLISIONS] = { 0 };
	int tr_hit_loc[MAX_G2_COLLISIONS] = { HL_NONE }; //same as 0
	int tr_dismember_loc[MAX_G2_COLLISIONS] = { HL_NONE }; //same as 0
	qboolean tr_dismember[MAX_G2_COLLISIONS] = { qfalse }; //same as 0
	int i;
	int num_hit_ents = 0;
	int hit_effect;
	gentity_t* hit_ent;

	VectorNormalize2(blade_vec, blade_dir);

	for (auto& z : tr->G2CollisionMap)
	{
		if (z.mEntityNum == -1)
		{
			//actually, completely break out of this for loop since nothing after this in the aray should ever be valid either
			continue; //break;//
		}

		CCollisionRecord& coll = z;
		const float dist_from_start = coll.mDistance;

		for (i = 0; i < num_hit_ents; i++)
		{
			if (hit_ent_num[i] == coll.mEntityNum)
			{
				//we hit this ent before
				//we'll want to add this dist
				hit_ent_dmg_add[i] = dist_from_start;
				break;
			}
		}
		if (i == num_hit_ents)
		{
			//first time we hit this ent
			if (num_hit_ents == MAX_G2_COLLISIONS)
			{
				//hit too many damn ents!
				continue;
			}
			hit_ent_num[num_hit_ents] = coll.mEntityNum;
			if (!coll.mFlags)
			{
				//hmm, we came out first, so we must have started inside
				//we'll want to subtract this dist
				hit_ent_dmg_add[num_hit_ents] = dist_from_start;
			}
			else
			{
				//we're entering the model
				//we'll want to subtract this dist
				hit_ent_dmg_sub[num_hit_ents] = dist_from_start;
			}
			//keep track of how far in the damage was done
			hit_ent_start_frac[num_hit_ents] = hit_ent_dmg_sub[num_hit_ents] / length;
			//remember the entrance point
			VectorCopy(coll.mCollisionPosition, hit_ent_point[num_hit_ents]);
			//remember the normal of the face we hit
			VectorCopy(coll.mCollisionNormal, hit_ent_normal[num_hit_ents]);
			VectorNormalize(hit_ent_normal[num_hit_ents]);

			//do the effect

			//FIXME: check material rather than team?
			hit_ent = &g_entities[hit_ent_num[num_hit_ents]];

			if (hit_ent
				&& hit_ent->client
				&& coll.mModelIndex > 0)
			{
				//hit a submodel on the enemy, not their actual body!
				if (!WP_SaberBladeUseSecondBladeStyle(saber, blade_num)
					&& saber->hitOtherEffect)
				{
					hit_effect = saber->hitOtherEffect;
				}
				else if (WP_SaberBladeUseSecondBladeStyle(saber, blade_num)
					&& saber->hitOtherEffect2)
				{
					hit_effect = saber->hitOtherEffect2;
				}
				else
				{
					if (hit_ent && hit_ent->client &&
						(hit_ent->client->NPC_class == CLASS_ATST
							|| hit_ent->client->NPC_class == CLASS_GONK
							|| hit_ent->client->NPC_class == CLASS_INTERROGATOR
							|| hit_ent->client->NPC_class == CLASS_MARK1
							|| hit_ent->client->NPC_class == CLASS_MARK2
							|| hit_ent->client->NPC_class == CLASS_MOUSE
							|| hit_ent->client->NPC_class == CLASS_PROBE
							|| hit_ent->client->NPC_class == CLASS_PROTOCOL
							|| hit_ent->client->NPC_class == CLASS_R2D2
							|| hit_ent->client->NPC_class == CLASS_R5D2
							|| hit_ent->client->NPC_class == CLASS_SEEKER
							|| hit_ent->client->NPC_class == CLASS_SENTRY
							|| hit_ent->client->NPC_class == CLASS_SBD
							|| hit_ent->client->NPC_class == CLASS_BATTLEDROID
							|| hit_ent->client->NPC_class == CLASS_DROIDEKA
							|| hit_ent->client->NPC_class == CLASS_OBJECT
							|| hit_ent->client->NPC_class == CLASS_ASSASSIN_DROID
							|| hit_ent->client->NPC_class == CLASS_SABER_DROID))
					{
						hit_effect = G_EffectIndex(hit_saber_cut_droid);
					}
					else
					{
						hit_effect = G_EffectIndex(hit_sparks);
					}
				}
			}
			else
			{
				if (!WP_SaberBladeUseSecondBladeStyle(saber, blade_num)
					&& saber->hitPersonEffect)
				{
					hit_effect = saber->hitPersonEffect;
				}
				else if (WP_SaberBladeUseSecondBladeStyle(saber, blade_num)
					&& saber->hitPersonEffect2)
				{
					hit_effect = saber->hitPersonEffect2;
				}
				else
				{
					if (hit_ent && hit_ent->client &&
						(hit_ent->client->NPC_class == CLASS_ATST
							|| hit_ent->client->NPC_class == CLASS_GONK
							|| hit_ent->client->NPC_class == CLASS_INTERROGATOR
							|| hit_ent->client->NPC_class == CLASS_MARK1
							|| hit_ent->client->NPC_class == CLASS_MARK2
							|| hit_ent->client->NPC_class == CLASS_MOUSE
							|| hit_ent->client->NPC_class == CLASS_PROBE
							|| hit_ent->client->NPC_class == CLASS_PROTOCOL
							|| hit_ent->client->NPC_class == CLASS_R2D2
							|| hit_ent->client->NPC_class == CLASS_R5D2
							|| hit_ent->client->NPC_class == CLASS_SEEKER
							|| hit_ent->client->NPC_class == CLASS_SENTRY
							|| hit_ent->client->NPC_class == CLASS_SBD
							|| hit_ent->client->NPC_class == CLASS_BATTLEDROID
							|| hit_ent->client->NPC_class == CLASS_DROIDEKA
							|| hit_ent->client->NPC_class == CLASS_OBJECT
							|| hit_ent->client->NPC_class == CLASS_ASSASSIN_DROID
							|| hit_ent->client->NPC_class == CLASS_SABER_DROID))
					{
						hit_effect = G_EffectIndex(hit_saber_cut_droid);
					}
					else
					{
						hit_effect = G_EffectIndex(hit_blood_sparks);
					}
				}
			}
			if (hit_ent != nullptr)
			{
				if (hit_ent->client)
				{
					if (hit_ent->client->NPC_class == CLASS_ATST
						|| hit_ent->client->NPC_class == CLASS_GONK
						|| hit_ent->client->NPC_class == CLASS_INTERROGATOR
						|| hit_ent->client->NPC_class == CLASS_MARK1
						|| hit_ent->client->NPC_class == CLASS_MARK2
						|| hit_ent->client->NPC_class == CLASS_MOUSE
						|| hit_ent->client->NPC_class == CLASS_PROBE
						|| hit_ent->client->NPC_class == CLASS_PROTOCOL
						|| hit_ent->client->NPC_class == CLASS_R2D2
						|| hit_ent->client->NPC_class == CLASS_R5D2
						|| hit_ent->client->NPC_class == CLASS_SEEKER
						|| hit_ent->client->NPC_class == CLASS_SENTRY
						|| hit_ent->client->NPC_class == CLASS_SBD
						|| hit_ent->client->NPC_class == CLASS_BATTLEDROID
						|| hit_ent->client->NPC_class == CLASS_DROIDEKA
						|| hit_ent->client->NPC_class == CLASS_OBJECT
						|| hit_ent->client->NPC_class == CLASS_ASSASSIN_DROID
						|| hit_ent->client->NPC_class == CLASS_SABER_DROID)
					{
						// special droid only behaviors
						if (!WP_SaberBladeUseSecondBladeStyle(saber, blade_num)
							&& saber->hitOtherEffect)
						{
							hit_effect = saber->hitOtherEffect;
						}
						else if (WP_SaberBladeUseSecondBladeStyle(saber, blade_num)
							&& saber->hitOtherEffect2)
						{
							hit_effect = saber->hitOtherEffect2;
						}
						else
						{
							hit_effect = G_EffectIndex(hit_saber_touch_droid);
						}
					}
				}
				else
				{
					// So sue me, this is the easiest way to check to see if this is the turbo laser from t2_wedge,
					// in which case I don't want the saber effects going off on it.
					if (hit_ent->flags & FL_DMG_BY_HEAVY_WEAP_ONLY
						&& hit_ent->takedamage == qfalse
						&& Q_stricmp(hit_ent->classname, "misc_turret") == 0)
					{
						continue;
					}
					if (dmg)
					{
						//only do these effects if actually trying to damage the thing...
						if (hit_ent->svFlags & SVF_BBRUSH //a breakable brush
							&& (hit_ent->spawnflags & 1 //INVINCIBLE
								|| hit_ent->flags & FL_DMG_BY_HEAVY_WEAP_ONLY)) //HEAVY weapon damage only
						{
							//no hit effect (besides regular client-side one)
							hit_effect = 0;
						}
						else
						{
							if (!WP_SaberBladeUseSecondBladeStyle(saber, blade_num)
								&& saber->hitOtherEffect)
							{
								hit_effect = saber->hitOtherEffect;
							}
							else if (WP_SaberBladeUseSecondBladeStyle(saber, blade_num)
								&& saber->hitOtherEffect2)
							{
								hit_effect = saber->hitOtherEffect2;
							}
							else
							{
								if (hit_ent && hit_ent->client &&
									(hit_ent->client->NPC_class == CLASS_ATST
										|| hit_ent->client->NPC_class == CLASS_GONK
										|| hit_ent->client->NPC_class == CLASS_INTERROGATOR
										|| hit_ent->client->NPC_class == CLASS_MARK1
										|| hit_ent->client->NPC_class == CLASS_MARK2
										|| hit_ent->client->NPC_class == CLASS_MOUSE
										|| hit_ent->client->NPC_class == CLASS_PROBE
										|| hit_ent->client->NPC_class == CLASS_PROTOCOL
										|| hit_ent->client->NPC_class == CLASS_R2D2
										|| hit_ent->client->NPC_class == CLASS_R5D2
										|| hit_ent->client->NPC_class == CLASS_SEEKER
										|| hit_ent->client->NPC_class == CLASS_SENTRY
										|| hit_ent->client->NPC_class == CLASS_SBD
										|| hit_ent->client->NPC_class == CLASS_BATTLEDROID
										|| hit_ent->client->NPC_class == CLASS_DROIDEKA
										|| hit_ent->client->NPC_class == CLASS_OBJECT
										|| hit_ent->client->NPC_class == CLASS_ASSASSIN_DROID
										|| hit_ent->client->NPC_class == CLASS_SABER_DROID))
								{
									hit_effect = G_EffectIndex(hit_saber_cut_droid);
								}
								else
								{
									hit_effect = G_EffectIndex(hit_sparks2);
								}
							}
						}
					}
				}
			}
			if (!g_saberNoEffects)
			{
				if (hit_effect != 0)
				{
					G_PlayEffect(hit_effect, coll.mCollisionPosition, coll.mCollisionNormal);
				}
			}

			//Get the hit location based on surface name
			if (hitLoc[num_hit_ents] == HL_NONE && tr_hit_loc[num_hit_ents] == HL_NONE
				|| hitDismemberLoc[num_hit_ents] == HL_NONE && tr_dismember_loc[num_hit_ents] == HL_NONE
				|| !hitDismember[num_hit_ents] && !tr_dismember[num_hit_ents])
			{
				//no hit loc set for this ent this damage cycle yet
				//FIXME: find closest impact surf *first* (per ent), then call G_GetHitLocFromSurfName?
				//FIXED: if hit multiple ents in this collision record, these trSurfName, trDismember and trDismemberLoc will get stomped/confused over the multiple ents I hit
				const char* tr_surf_name = gi.G2API_GetSurfaceName(
					&g_entities[coll.mEntityNum].ghoul2[coll.mModelIndex],
					coll.mSurfaceIndex);
				tr_dismember[num_hit_ents] = G_GetHitLocFromSurfName(&g_entities[coll.mEntityNum], tr_surf_name,
					&tr_hit_loc[num_hit_ents], coll.mCollisionPosition,
					dmg_dir, blade_dir, MOD_SABER, saber_type);
				if (tr_dismember[num_hit_ents])
				{
					tr_dismember_loc[num_hit_ents] = tr_hit_loc[num_hit_ents];
				}
			}
			num_hit_ents++;
		}
	}

	//now go through all the ents we hit and do the damage
	for (i = 0; i < num_hit_ents; i++)
	{
		float do_dmg = dmg;
		if (hit_ent_num[i] != ENTITYNUM_NONE)
		{
			if (do_dmg < 10)
			{
				//base damage is less than 10
				if (hit_ent_num[i] != 0)
				{
					//not the player
					hit_ent = &g_entities[hit_ent_num[i]];
					if (!hit_ent->client || hit_ent->client->ps.weapon != WP_SABER && hit_ent->client->NPC_class !=
						CLASS_GALAKMECH && hit_ent->client->playerTeam == enemy_team)
					{
						//did *not* hit a jedi and did *not* hit the player
						//make sure the base damage is high against non-jedi, feels better
						do_dmg = 10;
					}
				}
			}
			if (!hit_ent_dmg_add[i] && !hit_ent_dmg_sub[i])
			{
				//spent entire time in model
				//NOTE: will we even get a collision then?
				do_dmg *= length;
			}
			else if (hit_ent_dmg_add[i] && hit_ent_dmg_sub[i])
			{
				//we did enter and exit
				do_dmg *= hit_ent_dmg_add[i] - hit_ent_dmg_sub[i];
			}
			else if (!hit_ent_dmg_add[i])
			{
				//we didn't exit, just entered
				do_dmg *= length - hit_ent_dmg_sub[i];
			}
			else if (!hit_ent_dmg_sub[i])
			{
				//we didn't enter, only exited
				do_dmg *= hit_ent_dmg_add[i];
			}
			if (do_dmg > 0)
			{
				wp_saber_damage_add(1.0, hit_ent_num[i], dmg_dir, blade_vec, hit_ent_normal[i], hit_ent_point[i],
					ceil(do_dmg),
					hit_ent_start_frac[i], tr_hit_loc[i], tr_dismember[i], tr_dismember_loc[i]);
			}
		}
	}
	return static_cast<qboolean>(num_hit_ents > 0);
}

void wp_saber_block_effect(const gentity_t* attacker, const int saber_num, const int blade_num, vec3_t position,
	vec3_t normal,
	const qboolean cut_not_block)
{
	const saberInfo_t* saber = nullptr;

	if (attacker && attacker->client)
	{
		saber = &attacker->client->ps.saber[saber_num];
	}

	if (saber
		&& !WP_SaberBladeUseSecondBladeStyle(saber, blade_num)
		&& saber->blockEffect)
	{
		if (normal)
		{
			G_PlayEffect(saber->blockEffect, position, normal);
		}
		else
		{
			G_PlayEffect(saber->blockEffect, position);
		}
	}
	else if (saber
		&& WP_SaberBladeUseSecondBladeStyle(saber, blade_num)
		&& saber->blockEffect2)
	{
		if (normal)
		{
			G_PlayEffect(saber->blockEffect2, position, normal);
		}
		else
		{
			G_PlayEffect(saber->blockEffect2, position);
		}
	}
	else if (cut_not_block)
	{
		if (normal)
		{
			G_PlayEffect("saber/saber_cut", position, normal);
		}
		else
		{
			G_PlayEffect("saber/saber_bodyhit", position);
		}
	}
	else
	{
		if (normal)
		{
			G_PlayEffect("saber/saber_block", position, normal);
		}
		else
		{
			G_PlayEffect("saber/saber_badparry", position);
		}
	}
}

void wp_saber_m_block_effect(const gentity_t* attacker, const int saber_num, const int blade_num, vec3_t position,
	vec3_t normal,
	const qboolean cut_not_block)
{
	const saberInfo_t* saber = nullptr;

	if (attacker && attacker->client)
	{
		saber = &attacker->client->ps.saber[saber_num];
	}

	if (saber
		&& !WP_SaberBladeUseSecondBladeStyle(saber, blade_num)
		&& saber->blockEffect)
	{
		if (normal)
		{
			G_PlayEffect(saber->blockEffect, position, normal);
		}
		else
		{
			G_PlayEffect(saber->blockEffect, position);
		}
	}
	else if (saber
		&& WP_SaberBladeUseSecondBladeStyle(saber, blade_num)
		&& saber->blockEffect2)
	{
		if (normal)
		{
			G_PlayEffect(saber->blockEffect2, position, normal);
		}
		else
		{
			G_PlayEffect(saber->blockEffect2, position);
		}
	}
	else if (cut_not_block)
	{
		if (normal)
		{
			G_PlayEffect("saber/saber_cut", position, normal);
		}
		else
		{
			G_PlayEffect("saber/saber_bodyhit", position);
		}
	}
	else
	{
		if (normal)
		{
			G_PlayEffect("saber/saber_deflect", position, normal);
		}
		else
		{
			G_PlayEffect("saber/saber/saber_goodparry", position);
		}
	}
}

void wp_saber_knockaway(const gentity_t* attacker, trace_t* tr)
{
	WP_SaberDrop(attacker, &g_entities[attacker->client->ps.saberEntityNum]);
	wp_saber_knock_sound(attacker, nullptr, 0, 0);
	wp_saber_block_effect(attacker, 0, 0, tr->endpos, nullptr, qfalse);
	saberHitFraction = tr->fraction;

	if (d_saberCombat->integer || g_DebugSaberCombat->integer)
	{
		gi.Printf(S_COLOR_MAGENTA"WP_SaberKnockaway: saberHitFraction %4.2f\n", saberHitFraction);
	}
	VectorCopy(tr->endpos, saberHitLocation);
	saberHitEntity = tr->entityNum;
	if (!g_noClashFlare)
	{
		g_saberFlashTime = level.time - 50;
		VectorCopy(saberHitLocation, g_saberFlashPos);
	}
}

qboolean g_in_cinematic_saber_anim(const gentity_t* self)
{
	if (self->NPC
		&& self->NPC->behaviorState == BS_CINEMATIC
		&& (self->client->ps.torsoAnim == BOTH_CIN_16 || self->client->ps.torsoAnim == BOTH_CIN_17))
	{
		return qtrue;
	}
	return qfalse;
}

qboolean g_in_cinematic_lightning_anim(const gentity_t* self)
{
	if (self->NPC && self->NPC->behaviorState == BS_CINEMATIC)
	{
		return qtrue;
	}
	return qfalse;
}

constexpr auto SABER_COLLISION_DIST = 6; //was 2//was 4//was 8//was 16;
constexpr auto SABER_RADIUS_DAMAGE_DIST = 2;
constexpr auto SABER_COLLISION_BLOCKING_DIST = 8; //was 2//was 4//was 8//was 16;
extern qboolean in_front(vec3_t spot, vec3_t from, vec3_t from_angles, float thresh_hold = 0.0f);

qboolean wp_saber_damage_for_trace(const int ignore, vec3_t start, vec3_t end, float dmg, vec3_t blade_dir,
	const qboolean no_ghoul,
	int attack_strength, const saberType_t saber_type, const qboolean extrapolate,
	const int saber_num,
	const int blade_num)
{
	trace_t tr;
	constexpr int mask = MASK_SHOT | CONTENTS_LIGHTSABER;
	const gentity_t* attacker = &g_entities[ignore];

	vec3_t end2;
	VectorCopy(end, end2);
	if (extrapolate)
	{
		//since we can no longer use the predicted point, extrapolate the trace some.
		vec3_t diff;
		VectorSubtract(end, start, diff);
		VectorNormalize(diff);
		VectorMA(end2, SABER_EXTRAPOLATE_DIST, diff, end2);
	}

	if (!no_ghoul)
	{
		float use_radius_for_damage = 0;

		if (attacker
			&& attacker->client)
		{
			//see if we're not drawing the blade, if so, do a trace based on radius of blade (because the radius is being used to simulate a larger/smaller piece of a solid weapon)...
			if (!WP_SaberBladeUseSecondBladeStyle(&attacker->client->ps.saber[saber_num], blade_num)
				&& attacker->client->ps.saber[saber_num].saberFlags2 & SFL2_NO_BLADE)
			{
				//not drawing blade
				use_radius_for_damage = attacker->client->ps.saber[saber_num].blade[blade_num].radius;
			}
			else if (WP_SaberBladeUseSecondBladeStyle(&attacker->client->ps.saber[saber_num], blade_num)
				&& attacker->client->ps.saber[saber_num].saberFlags2 & SFL2_NO_BLADE2)
			{
				//not drawing blade
				use_radius_for_damage = attacker->client->ps.saber[saber_num].blade[blade_num].radius;
			}
		}
		if (!use_radius_for_damage)
		{
			//do normal check for larger-size saber traces
			if (!attacker->s.number
				|| attacker->client
				&& (attacker->client->playerTeam == TEAM_PLAYER
					|| attacker->client->NPC_class == CLASS_SHADOWTROOPER
					|| attacker->client->NPC_class == CLASS_ALORA
					|| attacker->NPC && attacker->NPC->aiFlags & NPCAI_BOSS_CHARACTER))
			{
				use_radius_for_damage = SABER_RADIUS_DAMAGE_DIST;
			}
		}

		if (use_radius_for_damage > 0)
		{
			//player,. player allies, shadow troopers, Tavion and desann use larger traces
			const vec3_t trace_mins = {
							 -use_radius_for_damage, -use_radius_for_damage, -use_radius_for_damage
			}, traceMaxs = {
				use_radius_for_damage, use_radius_for_damage, use_radius_for_damage
			};
			gi.trace(&tr, start, trace_mins, traceMaxs, end2, ignore, mask, G2_COLLIDE, 10);
		}
		else
		{
			constexpr vec3_t trace_mins = {
								 -SABER_RADIUS_DAMAGE_DIST, -SABER_RADIUS_DAMAGE_DIST, -SABER_RADIUS_DAMAGE_DIST
			},
				trace_maxs = {
					SABER_RADIUS_DAMAGE_DIST, SABER_RADIUS_DAMAGE_DIST, SABER_RADIUS_DAMAGE_DIST
			};
			gi.trace(&tr, start, trace_mins, trace_maxs, end2, ignore, mask, G2_COLLIDE, 10);
		}
	}
	else
	{
		constexpr vec3_t trace_mins = { -SABER_RADIUS_DAMAGE_DIST, -SABER_RADIUS_DAMAGE_DIST, -SABER_RADIUS_DAMAGE_DIST },
			trace_maxs = { SABER_RADIUS_DAMAGE_DIST, SABER_RADIUS_DAMAGE_DIST, SABER_RADIUS_DAMAGE_DIST };
		gi.trace(&tr, start, trace_mins, trace_maxs, end2, ignore, mask, G2_COLLIDE, 10);
	}

	if (tr.entityNum == ENTITYNUM_NONE)
	{
		return qfalse;
	}

	if (tr.entityNum == ENTITYNUM_WORLD)
	{
		if (attacker && attacker->client && attacker->client->ps.saber[saber_num].saberFlags & SFL_BOUNCE_ON_WALLS)
		{
			VectorCopy(tr.endpos, saberHitLocation);
			VectorCopy(tr.plane.normal, saberHitNormal);
		}
		return qtrue;
	}

	if (&g_entities[tr.entityNum])
	{
		gentity_t* hit_ent = &g_entities[tr.entityNum];
		const gentity_t* owner = g_entities[tr.entityNum].owner;

		if (wp_saber_must_block(hit_ent, owner, qfalse, tr.endpos, -1, -1))
		{
			//hit victim is able to block, block!
			if (wp_saber_must_block(&g_entities[tr.entityNum], owner, qfalse, tr.endpos, -1, -1))
			{
				//hit victim is able to block, block!
				hit_ent = &g_entities[tr.entityNum];

				const qboolean other_holding_block = hit_ent->client->ps.ManualBlockingFlags & 1 << HOLDINGBLOCK ? qtrue : qfalse; //Normal Blocking
				const qboolean other_active_blocking = hit_ent->client->ps.ManualBlockingFlags & 1 << HOLDINGBLOCKANDATTACK ? qtrue : qfalse; //Active Blocking
				const qboolean other_m_blocking = hit_ent->client->ps.ManualBlockingFlags & 1 << PERFECTBLOCKING ? qtrue : qfalse; //Perfect Blocking

				if (hit_ent->s.number >= MAX_CLIENTS && !G_ControlledByPlayer(hit_ent))
				{
					WP_SaberParryNonRandom(hit_ent, tr.endpos, qfalse);
				}
				else
				{
					if (other_holding_block || other_active_blocking || other_m_blocking || manual_saberblocking(hit_ent) || hit_ent->client->ps.saberBlockingTime > level.time)
					{
						WP_SaberParryNonRandom(hit_ent, tr.endpos, qfalse);

						if (d_blockinfo->integer || g_DebugSaberCombat->integer)
						{
							gi.Printf(S_COLOR_MAGENTA"Blocker Blocked a thrown saber reward 10\n");
						}

						wp_block_points_regenerate_over_ride(hit_ent, BLOCKPOINTS_TEN); //BP Reward blocker
					}
				}
			}
			else
			{
				//hit something that can actually take damage
				hit_ent = nullptr;
			}
		}
		else if (hit_ent->contents & CONTENTS_LIGHTSABER)
		{
			if (attacker && attacker->client && attacker->client->ps.saberInFlight)
			{
				//thrown saber hit something
				if (owner
					&& owner->s.number
					&& owner->client
					&& owner->NPC
					&& owner->health > 0)
				{
					if (owner->client->NPC_class == CLASS_ALORA)
					{
						//alora takes less damage
						dmg *= 0.25f;
					}
					else if (owner->client->NPC_class == CLASS_PLAYER)
					{
						//player takes less damage
						dmg *= 0.20f;
					}
					else if (owner->client->NPC_class == CLASS_TAVION)
					{
						//Tavion can toss a blocked thrown saber aside
						wp_saber_knockaway(attacker, &tr);
						Jedi_PlayDeflectSound(owner);
						return qfalse;
					}
					else if (owner->client->NPC_class == CLASS_VADER)
					{
						//vader can toss a blocked thrown saber aside
						wp_saber_knockaway(attacker, &tr);
						Jedi_PlayDeflectSound(owner);
						return qfalse;
					}
				}
				else if (owner
					&& owner->s.number
					&& owner->client
					&& owner->health > 0)
				{
					if (owner->client->NPC_class == CLASS_PLAYER)
					{
						//player takes less damage
						dmg *= 0.20f;
					}
					else if (owner->client->ps.ManualBlockingFlags & 1 << HOLDINGBLOCK)
					{
						//player takes less damage
						dmg *= 0.20f;
					}
				}
			}
			qboolean sabers_intersect = wp_sabers_intersect(attacker, saber_num, blade_num, owner, qfalse);

			float saber_dist;

			if (attacker && attacker->client && attacker->client->ps.saberInFlight  //attacker is throwing
				&& owner && owner->s.number == 0  // defender
				&& (g_saberAutoBlocking->integer && owner->NPC && !G_ControlledByPlayer(owner)//all npc,s
					|| owner->client->ps.ManualBlockingFlags & 1 << HOLDINGBLOCK //player
					|| owner->client->ps.ManualBlockingFlags & 1 << MBF_NPCBLOCKING && owner->NPC && !G_ControlledByPlayer(owner)
					|| owner->client->ps.saberBlockingTime > level.time))  //anybody already doing blocking
			{
				//players have g_saberAutoBlocking, do the more generous check against flying sabers
				saber_dist = 0;
			}
			else
			{
				//sabers must actually collide with the attacking saber
				saber_dist = wp_sabers_distance(attacker, owner);
				if (attacker && attacker->client && attacker->client->ps.saberInFlight)
				{
					saber_dist /= 2.0f;
					if (saber_dist <= 16.0f)
					{
						sabers_intersect = qtrue;
					}
				}
			}
			if (sabersCrossed == -1 || sabersCrossed > saber_dist)
			{
				sabersCrossed = saber_dist;
			}

			float collision_dist;

			if (g_saberRealisticCombat->integer > 1)
			{
				if (manual_saberblocking(owner))
				{
					collision_dist = SABER_COLLISION_BLOCKING_DIST;
				}
				else
				{
					collision_dist = SABER_COLLISION_DIST;
				}
			}
			else
			{
				if (manual_saberblocking(owner))
				{
					collision_dist = SABER_COLLISION_BLOCKING_DIST + 6 + g_spskill->integer * 4;
				}
				else
				{
					collision_dist = SABER_COLLISION_DIST + 6 + g_spskill->integer * 4;
				}
			}
			if (g_in_cinematic_saber_anim(owner) && g_in_cinematic_saber_anim(attacker))
			{
				sabers_intersect = qtrue;
			}
			if (owner && owner->client && attacker != nullptr
				&& saber_dist > collision_dist
				&& !sabers_intersect) //was qtrue, but missed too much?
			{
				//swing came from behind and/or was not stopped by a lightsaber
				gi.trace(&tr, start, nullptr, nullptr, end2, ignore, mask & ~CONTENTS_LIGHTSABER, G2_NOCOLLIDE, 10);
				if (tr.entityNum == ENTITYNUM_WORLD)
				{
					return qtrue;
				}
				if (tr.entityNum == ENTITYNUM_NONE || &g_entities[tr.entityNum] == nullptr)
				{
					//didn't hit the owner
					return qfalse; // Exit, but we didn't hit the wall.
				}

				if (d_saberCombat->integer > 1 || g_DebugSaberCombat->integer)
				{
					if (!attacker->s.number)
					{
						gi.Printf(S_COLOR_MAGENTA"%d saber hit owner through saber %4.2f, dist = %4.2f\n", level.time,
							saberHitFraction, saber_dist);
					}
				}
				hit_ent = &g_entities[tr.entityNum];
				owner = g_entities[tr.entityNum].owner;
			}
			else
			{
				//hit a lightsaber
				if ((tr.fraction < saberHitFraction || tr.startsolid)
					&& saber_dist < (8.0f + g_spskill->value) * 4.0f
					&& (sabers_intersect || saber_dist < (4.0f + g_spskill->value) * 2.0f))
				{
					// This saber hit closer than the last one.
					if ((tr.allsolid || tr.startsolid) && owner && owner->client)
					{
						//tr.fraction will be 0, unreliable... so calculate actual
						const float dist = Distance(start, end2);
						if (dist)
						{
							float hit_frac = wp_sabers_distance(attacker, owner) / dist;
							if (hit_frac > 1.0f)
							{
								//umm... minimum distance between sabers was longer than trace...?
								hit_frac = 1.0f;
							}
							if (hit_frac < saberHitFraction)
							{
								saberHitFraction = hit_frac;
							}
						}
						else
						{
							saberHitFraction = 0.0f;
						}
					}
					VectorCopy(tr.endpos, saberHitLocation);
					saberHitEntity = tr.entityNum;
				}
				return qfalse; // Exit, but we didn't hit the wall.
			}
		}
		else
		{
			if (d_saberCombat->integer > 1 || g_DebugSaberCombat->integer)
			{
				if (!attacker->s.number)
				{
					gi.Printf(S_COLOR_RED"%d saber hit owner directly %4.2f\n", level.time, saberHitFraction);
				}
			}
		}

		if (attacker && attacker->client && attacker->client->ps.saberInFlight)
		{
			//thrown saber hit something
			if (hit_ent && hit_ent->client && hit_ent->health > 0
				&& (hit_ent->client->NPC_class == CLASS_DESANN
					|| hit_ent->client->NPC_class == CLASS_SITHLORD
					|| hit_ent->client->NPC_class == CLASS_VADER
					|| !Q_stricmp("Yoda", hit_ent->NPC_type)
					|| !Q_stricmp("RebornBoss", hit_ent->NPC_type)
					|| hit_ent->client->NPC_class == CLASS_LUKE
					|| hit_ent->client->NPC_class == CLASS_BOBAFETT
					|| hit_ent->client->NPC_class == CLASS_MANDO
					|| hit_ent->client->NPC_class == CLASS_GALAKMECH && hit_ent->client->ps.powerups[PW_GALAK_SHIELD] >
					0
					|| hit_ent->client->ps.powerups[PW_GALAK_SHIELD] > 0)
				|| owner && owner->client && owner->health > 0
				&& (owner->client->NPC_class == CLASS_DESANN
					|| owner->client->NPC_class == CLASS_SITHLORD
					|| owner->client->NPC_class == CLASS_VADER
					|| !Q_stricmp("Yoda", owner->NPC_type)
					|| !Q_stricmp("RebornBoss", owner->NPC_type)
					|| owner->client->NPC_class == CLASS_LUKE
					|| owner->client->NPC_class == CLASS_GALAKMECH && owner->client->ps.powerups[PW_GALAK_SHIELD] > 0
					|| owner->client->ps.powerups[PW_GALAK_SHIELD] > 0))
			{
				//Luke and Desann slap thrown sabers aside
				wp_saber_knockaway(attacker, &tr);
				if (hit_ent->client)
				{
					Jedi_PlayDeflectSound(hit_ent);
				}
				else
				{
					Jedi_PlayDeflectSound(owner);
				}
				return qfalse; // Exit, but we didn't hit the wall.
			}
		}

		if (hit_ent->takedamage)
		{
			{
				vec3_t dir;
				vec3_t blade_vec = { 0 };
				if (attacker && attacker->client)
				{
					VectorScale(blade_dir, attacker->client->ps.saber[saber_num].blade[blade_num].length, blade_vec);
				}
				//multiply the damage by the total distance of the swipe
				VectorSubtract(end2, start, dir);
				const float len = VectorNormalize(dir); //VectorLength( dir );
				if (no_ghoul || !hit_ent->ghoul2.size())
				{
					//we weren't doing a ghoul trace
					int hit_effect = 0;
					if (dmg >= 1.0 && hit_ent->bmodel)
					{
						dmg = 1.0;
					}
					if (len > 1)
					{
						dmg *= len;
					}
					if (d_saberCombat->integer > 1 || g_DebugSaberCombat->integer)
					{
						if (!(hit_ent->contents & CONTENTS_LIGHTSABER))
						{
							gi.Printf(S_COLOR_GREEN"Hit ent, but no ghoul collisions\n");
						}
					}
					float tr_frac;
					float dmg_frac;
					if (tr.allsolid)
					{
						//totally inside them
						tr_frac = 1.0;
						dmg_frac = 0.0;
					}
					else if (tr.startsolid)
					{
						//started inside them
						//we don't know how much was inside, we know it's less than all, so use half?
						tr_frac = 0.5;
						dmg_frac = 0.0;
					}
					else
					{
						//started outside them and hit them
						//yeah. this doesn't account for coming out the other wide, but we can worry about that later (use ghoul2)
						tr_frac = 1.0f - tr.fraction;
						dmg_frac = tr.fraction;
					}
					vec3_t backdir;
					VectorScale(dir, -1, backdir);
					wp_saber_damage_add(tr_frac, tr.entityNum, dir, blade_vec, backdir, tr.endpos, dmg, dmg_frac,
						HL_NONE,
						qfalse, HL_NONE);
					if (!tr.allsolid && !tr.startsolid)
					{
						VectorScale(dir, -1, dir);
					}
					if (hit_ent != nullptr)
					{
						if (hit_ent->client)
						{
							//don't do blood sparks on non-living things
							if (hit_ent && hit_ent->client &&
								(hit_ent->client->NPC_class == CLASS_ATST
									|| hit_ent->client->NPC_class == CLASS_GONK
									|| hit_ent->client->NPC_class == CLASS_INTERROGATOR
									|| hit_ent->client->NPC_class == CLASS_MARK1
									|| hit_ent->client->NPC_class == CLASS_MARK2
									|| hit_ent->client->NPC_class == CLASS_MOUSE
									|| hit_ent->client->NPC_class == CLASS_PROBE
									|| hit_ent->client->NPC_class == CLASS_PROTOCOL
									|| hit_ent->client->NPC_class == CLASS_R2D2
									|| hit_ent->client->NPC_class == CLASS_R5D2
									|| hit_ent->client->NPC_class == CLASS_SEEKER
									|| hit_ent->client->NPC_class == CLASS_SENTRY
									|| hit_ent->client->NPC_class == CLASS_SBD
									|| hit_ent->client->NPC_class == CLASS_BATTLEDROID
									|| hit_ent->client->NPC_class == CLASS_DROIDEKA
									|| hit_ent->client->NPC_class == CLASS_OBJECT
									|| hit_ent->client->NPC_class == CLASS_ASSASSIN_DROID
									|| hit_ent->client->NPC_class == CLASS_SABER_DROID))
							{
								// special droid only behaviors
								if (!WP_SaberBladeUseSecondBladeStyle(&attacker->client->ps.saber[saber_num], blade_num)
									&& attacker->client->ps.saber[saber_num].hitOtherEffect)
								{
									hit_effect = attacker->client->ps.saber[saber_num].hitOtherEffect;
								}
								else if (WP_SaberBladeUseSecondBladeStyle(
									&attacker->client->ps.saber[saber_num], blade_num)
									&& attacker->client->ps.saber[saber_num].hitOtherEffect2)
								{
									hit_effect = attacker->client->ps.saber[saber_num].hitOtherEffect2;
								}
								else
								{
									hit_effect = G_EffectIndex(hit_sparks);
								}
								G_SaberBounce(attacker, hit_ent);
							}
						}
						else
						{
							if (dmg)
							{
								//only do these effects if actually trying to damage the thing...
								if (hit_ent->svFlags & SVF_BBRUSH //a breakable brush
									&& (hit_ent->spawnflags & 1 //INVINCIBLE
										|| hit_ent->flags & FL_DMG_BY_HEAVY_WEAP_ONLY //HEAVY weapon damage only
										|| hit_ent->NPC_targetname && attacker && attacker->targetname && Q_stricmp(
											attacker->targetname, hit_ent->NPC_targetname)))
									//only breakable by an entity who is not the attacker
								{
									//
								}
								else
								{
									if (attacker->client->ps.saber[saber_num].hitOtherEffect && !
										WP_SaberBladeUseSecondBladeStyle(
											&attacker->client->ps.saber[saber_num], blade_num))
									{
										hit_effect = attacker->client->ps.saber[saber_num].hitOtherEffect;
									}
									else if (attacker->client->ps.saber[saber_num].hitOtherEffect2 &&
										WP_SaberBladeUseSecondBladeStyle(
											&attacker->client->ps.saber[saber_num], blade_num))
									{
										hit_effect = attacker->client->ps.saber[saber_num].hitOtherEffect2;
									}
									else
									{
										hit_effect = G_EffectIndex(hit_sparks2);
									}
								}
								G_SaberBounce(attacker, hit_ent);
							}
						}
					}

					if (!g_saberNoEffects && hit_effect != 0)
					{
						G_PlayEffect(hit_effect, tr.endpos, dir);
						G_SaberBounce(attacker, hit_ent);
					}
				}
				else
				{
					//we were doing a ghoul trace
					if (!attacker
						|| !attacker->client
						|| attacker->client->ps.saberLockTime < level.time)
					{
						if (!wp_saber_damage_effects(&tr, start, len, dmg, dir, blade_vec, attacker->client->enemyTeam,
							saber_type, &attacker->client->ps.saber[saber_num], blade_num))
						{
							G_SaberBounce(attacker, hit_ent);
						}
					}
				}
			}
		}
	}

	return qfalse;
}

constexpr auto LOCK_IDEAL_DIST_TOP = 32.0f;
constexpr auto LOCK_IDEAL_DIST_CIRCLE = 48.0f;
constexpr auto LOCK_IDEAL_DIST_JKA = 46.0f;
//all of the new saberlocks are 46.08 from each other because Richard Lico is da MAN;
extern void PM_SetAnimFrame(gentity_t* gent, int frame, qboolean torso, qboolean legs);
extern qboolean ValidAnimFileIndex(int index);

int g_saber_lock_anim(const int attacker_saber_style, const int defender_saber_style, const int top_or_side,
	const int lock_or_break_or_super_break,
	const int win_or_lose)
{
	int base_anim = -1;
	if (lock_or_break_or_super_break == SABER_LOCK_LOCK)
	{
		//special case: if we're using the same style and locking
		if (attacker_saber_style == defender_saber_style
			|| attacker_saber_style >= SS_FAST && attacker_saber_style <= SS_TAVION && defender_saber_style >= SS_FAST
			&&
			defender_saber_style <= SS_TAVION)
		{
			//using same style
			if (win_or_lose == SABER_LOCK_LOSE)
			{
				//you want the defender's stance...
				switch (defender_saber_style)
				{
				case SS_DUAL:
					if (top_or_side == SABER_LOCK_TOP)
					{
						base_anim = BOTH_LK_DL_DL_T_L_2;
					}
					else
					{
						base_anim = BOTH_LK_DL_DL_S_L_2;
					}
					break;
				case SS_STAFF:
					if (top_or_side == SABER_LOCK_TOP)
					{
						base_anim = BOTH_LK_ST_ST_T_L_2;
					}
					else
					{
						base_anim = BOTH_LK_ST_ST_S_L_2;
					}
					break;
				default:
					if (top_or_side == SABER_LOCK_TOP)
					{
						base_anim = BOTH_LK_S_S_T_L_2;
					}
					else
					{
						base_anim = BOTH_LK_S_S_S_L_2;
					}
					break;
				}
			}
		}
	}
	if (base_anim == -1)
	{
		switch (attacker_saber_style)
		{
		case SS_DUAL:
			switch (defender_saber_style)
			{
			case SS_DUAL:
				base_anim = BOTH_LK_DL_DL_S_B_1_L;
				break;
			case SS_STAFF:
				base_anim = BOTH_LK_DL_ST_S_B_1_L;
				break;
			default: //single
				base_anim = BOTH_LK_DL_S_S_B_1_L;
				break;
			}
			break;
		case SS_STAFF:
			switch (defender_saber_style)
			{
			case SS_DUAL:
				base_anim = BOTH_LK_ST_DL_S_B_1_L;
				break;
			case SS_STAFF:
				base_anim = BOTH_LK_ST_ST_S_B_1_L;
				break;
			default: //single
				base_anim = BOTH_LK_ST_S_S_B_1_L;
				break;
			}
			break;
		default: //single
			switch (defender_saber_style)
			{
			case SS_DUAL:
				base_anim = BOTH_LK_S_DL_S_B_1_L;
				break;
			case SS_STAFF:
				base_anim = BOTH_LK_S_ST_S_B_1_L;
				break;
			default: //single
				base_anim = BOTH_LK_S_S_S_B_1_L;
				break;
			}
			break;
		}
		//side lock or top lock?
		if (top_or_side == SABER_LOCK_TOP)
		{
			base_anim += 5;
		}
		//lock, break or superbreak?
		if (lock_or_break_or_super_break == SABER_LOCK_LOCK)
		{
			base_anim += 2;
		}
		else
		{
			//a break or superbreak
			if (lock_or_break_or_super_break == SABER_LOCK_SUPER_BREAK)
			{
				base_anim += 3;
			}
			//winner or loser?
			if (win_or_lose == SABER_LOCK_WIN)
			{
				base_anim += 1;
			}
		}
	}
	return base_anim;
}

qboolean g_check_increment_lock_anim(int anim, int winOrLose)
{
	qboolean increment = qfalse; //???
	//RULE: if you are the first style in the lock anim, you advance from LOSING position to WINNING position
	//		if you are the second style in the lock anim, you advance from WINNING position to LOSING position
	switch (anim)
	{
		//increment to win:
	case BOTH_LK_DL_DL_S_L_1: //lock if I'm using dual vs. dual and I initiated
	case BOTH_LK_DL_DL_S_L_2: //lock if I'm using dual vs. dual and other initiated
	case BOTH_LK_DL_DL_T_L_1: //lock if I'm using dual vs. dual and I initiated
	case BOTH_LK_DL_DL_T_L_2: //lock if I'm using dual vs. dual and other initiated
	case BOTH_LK_DL_S_S_L_1: //lock if I'm using dual vs. a single
	case BOTH_LK_DL_S_T_L_1: //lock if I'm using dual vs. a single
	case BOTH_LK_DL_ST_S_L_1: //lock if I'm using dual vs. a staff
	case BOTH_LK_DL_ST_T_L_1: //lock if I'm using dual vs. a staff
	case BOTH_LK_S_S_S_L_1: //lock if I'm using single vs. a single and I initiated
	case BOTH_LK_S_S_T_L_2: //lock if I'm using single vs. a single and other initiated
	case BOTH_LK_ST_S_S_L_1: //lock if I'm using staff vs. a single
	case BOTH_LK_ST_S_T_L_1: //lock if I'm using staff vs. a single
	case BOTH_LK_ST_ST_T_L_1: //lock if I'm using staff vs. a staff and I initiated
	case BOTH_LK_ST_ST_T_L_2: //lock if I'm using staff vs. a staff and other initiated
		if (winOrLose == SABER_LOCK_WIN)
		{
			increment = qtrue;
		}
		else
		{
			increment = qfalse;
		}
		break;

		//decrement to win:
	case BOTH_LK_S_DL_S_L_1: //lock if I'm using single vs. a dual
	case BOTH_LK_S_DL_T_L_1: //lock if I'm using single vs. a dual
	case BOTH_LK_S_S_S_L_2: //lock if I'm using single vs. a single and other initiated
	case BOTH_LK_S_S_T_L_1: //lock if I'm using single vs. a single and I initiated
	case BOTH_LK_S_ST_S_L_1: //lock if I'm using single vs. a staff
	case BOTH_LK_S_ST_T_L_1: //lock if I'm using single vs. a staff
	case BOTH_LK_ST_DL_S_L_1: //lock if I'm using staff vs. dual
	case BOTH_LK_ST_DL_T_L_1: //lock if I'm using staff vs. dual
	case BOTH_LK_ST_ST_S_L_1: //lock if I'm using staff vs. a staff and I initiated
	case BOTH_LK_ST_ST_S_L_2: //lock if I'm using staff vs. a staff and other initiated
		if (winOrLose == SABER_LOCK_WIN)
		{
			increment = qfalse;
		}
		else
		{
			increment = qtrue;
		}
		break;
	default:
		break;
	}
	return increment;
}

qboolean WP_SabersCheckLock2(gentity_t* attacker, gentity_t* defender, sabersLockMode_t lockMode)
{
	animation_t* anim;
	int attAnim, defAnim, advance = 0;
	float attStart = 0.5f, defStart = 0.5f;
	float idealDist = 48.0f;
	//FIXME: this distances need to be modified by the lengths of the sabers involved...
	//MATCH ANIMS
	if (lockMode == LOCK_KYLE_GRAB1
		|| lockMode == LOCK_KYLE_GRAB2
		|| lockMode == LOCK_KYLE_GRAB3)
	{
		float numSpins = 1.0f;
		idealDist = 46.0f; //42.0f;
		attStart = defStart = 0.0f;

		switch (lockMode)
		{
		default:
		case LOCK_KYLE_GRAB1:
			attAnim = BOTH_KYLE_PA_1;
			defAnim = BOTH_PLAYER_PA_1;
			numSpins = 2.0f;
			break;
		case LOCK_KYLE_GRAB2:
			attAnim = BOTH_KYLE_PA_3;
			defAnim = BOTH_PLAYER_PA_3;
			numSpins = 1.0f;
			break;
		case LOCK_KYLE_GRAB3:
			attAnim = BOTH_KYLE_PA_2;
			defAnim = BOTH_PLAYER_PA_2;
			defender->forcePushTime = level.time + PM_AnimLength(defender->client->clientInfo.animFileIndex,
				BOTH_PLAYER_PA_2);
			numSpins = 3.0f;
			break;
		}
		attacker->client->ps.SaberDeactivate();
		defender->client->ps.SaberDeactivate();
		if (d_slowmoaction->integer > 3
			&& (defender->s.number < MAX_CLIENTS
				|| attacker->s.number < MAX_CLIENTS))
		{
			if (ValidAnimFileIndex(attacker->client->clientInfo.animFileIndex))
			{
				int effectTime = PM_AnimLength(attacker->client->clientInfo.animFileIndex,
					static_cast<animNumber_t>(attAnim));
				int spinTime = floor(static_cast<float>(effectTime) / numSpins);
				int meFlags = MEF_MULTI_SPIN; //MEF_NO_TIMESCALE|MEF_NO_VERTBOB|
				if (Q_irand(0, 1))
				{
					meFlags |= MEF_REVERSE_SPIN;
				}
				G_StartMatrixEffect(attacker, meFlags, effectTime, 0.75f, spinTime);
			}
		}
		else if (d_slowmodeath->integer
			&& (defender->s.number < MAX_CLIENTS
				|| attacker->s.number < MAX_CLIENTS))
		{
			if (ValidAnimFileIndex(attacker->client->clientInfo.animFileIndex))
			{
				int effectTime = PM_AnimLength(attacker->client->clientInfo.animFileIndex,
					static_cast<animNumber_t>(attAnim));
				int spinTime = floor(static_cast<float>(effectTime) / numSpins);
				int meFlags = MEF_MULTI_SPIN;
				if (Q_irand(0, 1))
				{
					meFlags |= MEF_REVERSE_SPIN;
				}
				if (d_slowmoaction->integer && (attacker->s.number < MAX_CLIENTS || G_ControlledByPlayer(attacker)))
				{
					G_StartStasisEffect(attacker, meFlags, effectTime, 0.75f, spinTime);
				}
			}
		}
	}
	else if (lockMode == LOCK_FORCE_DRAIN)
	{
		idealDist = 46.0f; //42.0f;
		attStart = defStart = 0.0f;

		attAnim = BOTH_FORCE_DRAIN_GRAB_START;
		defAnim = BOTH_FORCE_DRAIN_GRABBED;
		attacker->client->ps.SaberDeactivate();
		defender->client->ps.SaberDeactivate();
	}
	else
	{
		if (lockMode == LOCK_RANDOM)
		{
			lockMode = static_cast<sabersLockMode_t>(Q_irand(LOCK_FIRST, static_cast<int>(LOCK_RANDOM) - 1));
		}
		//FIXME: attStart% and idealDist will change per saber lock anim pairing... do we need a big table like in bg_panimate.cpp?
		if (attacker->client->ps.saberAnimLevel >= SS_FAST
			&& attacker->client->ps.saberAnimLevel <= SS_TAVION
			&& defender->client->ps.saberAnimLevel >= SS_FAST
			&& defender->client->ps.saberAnimLevel <= SS_TAVION)
		{
			//2 single sabers?  Just do it the old way...
			switch (lockMode)
			{
			case LOCK_TOP:
				attAnim = BOTH_BF2LOCK;
				defAnim = BOTH_BF1LOCK;
				attStart = defStart = 0.5f;
				idealDist = LOCK_IDEAL_DIST_TOP;
				break;
			case LOCK_DIAG_TR:
				attAnim = BOTH_CCWCIRCLELOCK;
				defAnim = BOTH_CWCIRCLELOCK;
				attStart = defStart = 0.5f;
				idealDist = LOCK_IDEAL_DIST_CIRCLE;
				break;
			case LOCK_DIAG_TL:
				attAnim = BOTH_CWCIRCLELOCK;
				defAnim = BOTH_CCWCIRCLELOCK;
				attStart = defStart = 0.5f;
				idealDist = LOCK_IDEAL_DIST_CIRCLE;
				break;
			case LOCK_DIAG_BR:
				attAnim = BOTH_CCWCIRCLELOCK;
				defAnim = BOTH_CWCIRCLELOCK;
				attStart = defStart = 0.15f;
				idealDist = LOCK_IDEAL_DIST_CIRCLE;
				break;
			case LOCK_DIAG_BL:
				attAnim = BOTH_CWCIRCLELOCK;
				defAnim = BOTH_CCWCIRCLELOCK;
				attStart = defStart = 0.15f;
				idealDist = LOCK_IDEAL_DIST_CIRCLE;
				break;
			case LOCK_R:
				attAnim = BOTH_CCWCIRCLELOCK;
				defAnim = BOTH_CWCIRCLELOCK;
				attStart = defStart = 0.25f;
				idealDist = LOCK_IDEAL_DIST_CIRCLE;
				break;
			case LOCK_L:
				attAnim = BOTH_CWCIRCLELOCK;
				defAnim = BOTH_CCWCIRCLELOCK;
				attStart = defStart = 0.25f;
				idealDist = LOCK_IDEAL_DIST_CIRCLE;
				break;
			default:
				return qfalse;
			}
		}
		else
		{
			//use the new system
			idealDist = LOCK_IDEAL_DIST_JKA;
			//all of the new saberlocks are 46.08 from each other because Richard Lico is da MAN
			if (lockMode == LOCK_TOP)
			{
				//top lock
				attAnim = g_saber_lock_anim(attacker->client->ps.saberAnimLevel, defender->client->ps.saberAnimLevel,
					SABER_LOCK_TOP, SABER_LOCK_LOCK, SABER_LOCK_WIN);
				defAnim = g_saber_lock_anim(defender->client->ps.saberAnimLevel, attacker->client->ps.saberAnimLevel,
					SABER_LOCK_TOP, SABER_LOCK_LOCK, SABER_LOCK_LOSE);
				attStart = defStart = 0.5f;
			}
			else
			{
				//side lock
				switch (lockMode)
				{
				case LOCK_DIAG_TR:
					attAnim = g_saber_lock_anim(attacker->client->ps.saberAnimLevel,
						defender->client->ps.saberAnimLevel,
						SABER_LOCK_SIDE, SABER_LOCK_LOCK, SABER_LOCK_WIN);
					defAnim = g_saber_lock_anim(defender->client->ps.saberAnimLevel,
						attacker->client->ps.saberAnimLevel,
						SABER_LOCK_SIDE, SABER_LOCK_LOCK, SABER_LOCK_LOSE);
					attStart = defStart = 0.5f;
					break;
				case LOCK_DIAG_TL:
					attAnim = g_saber_lock_anim(attacker->client->ps.saberAnimLevel,
						defender->client->ps.saberAnimLevel,
						SABER_LOCK_SIDE, SABER_LOCK_LOCK, SABER_LOCK_LOSE);
					defAnim = g_saber_lock_anim(defender->client->ps.saberAnimLevel,
						attacker->client->ps.saberAnimLevel,
						SABER_LOCK_SIDE, SABER_LOCK_LOCK, SABER_LOCK_WIN);
					attStart = defStart = 0.5f;
					break;
				case LOCK_DIAG_BR:
					attAnim = g_saber_lock_anim(attacker->client->ps.saberAnimLevel,
						defender->client->ps.saberAnimLevel,
						SABER_LOCK_SIDE, SABER_LOCK_LOCK, SABER_LOCK_WIN);
					defAnim = g_saber_lock_anim(defender->client->ps.saberAnimLevel,
						attacker->client->ps.saberAnimLevel,
						SABER_LOCK_SIDE, SABER_LOCK_LOCK, SABER_LOCK_LOSE);
					if (g_check_increment_lock_anim(attAnim, SABER_LOCK_WIN))
					{
						attStart = 0.85f; //move to end of anim
					}
					else
					{
						attStart = 0.15f; //start at beginning of anim
					}
					if (g_check_increment_lock_anim(defAnim, SABER_LOCK_LOSE))
					{
						defStart = 0.85f; //start at end of anim
					}
					else
					{
						defStart = 0.15f; //start at beginning of anim
					}
					break;
				case LOCK_DIAG_BL:
					attAnim = g_saber_lock_anim(attacker->client->ps.saberAnimLevel,
						defender->client->ps.saberAnimLevel,
						SABER_LOCK_SIDE, SABER_LOCK_LOCK, SABER_LOCK_LOSE);
					defAnim = g_saber_lock_anim(defender->client->ps.saberAnimLevel,
						attacker->client->ps.saberAnimLevel,
						SABER_LOCK_SIDE, SABER_LOCK_LOCK, SABER_LOCK_WIN);
					if (g_check_increment_lock_anim(attAnim, SABER_LOCK_WIN))
					{
						attStart = 0.85f; //move to end of anim
					}
					else
					{
						attStart = 0.15f; //start at beginning of anim
					}
					if (g_check_increment_lock_anim(defAnim, SABER_LOCK_LOSE))
					{
						defStart = 0.85f; //start at end of anim
					}
					else
					{
						defStart = 0.15f; //start at beginning of anim
					}
					break;
				case LOCK_R:
					attAnim = g_saber_lock_anim(attacker->client->ps.saberAnimLevel,
						defender->client->ps.saberAnimLevel,
						SABER_LOCK_SIDE, SABER_LOCK_LOCK, SABER_LOCK_LOSE);
					defAnim = g_saber_lock_anim(defender->client->ps.saberAnimLevel,
						attacker->client->ps.saberAnimLevel,
						SABER_LOCK_SIDE, SABER_LOCK_LOCK, SABER_LOCK_WIN);
					if (g_check_increment_lock_anim(attAnim, SABER_LOCK_WIN))
					{
						attStart = 0.75f; //move to end of anim
					}
					else
					{
						attStart = 0.25f; //start at beginning of anim
					}
					if (g_check_increment_lock_anim(defAnim, SABER_LOCK_LOSE))
					{
						defStart = 0.75f; //start at end of anim
					}
					else
					{
						defStart = 0.25f; //start at beginning of anim
					}
					break;
				case LOCK_L:
					attAnim = g_saber_lock_anim(attacker->client->ps.saberAnimLevel,
						defender->client->ps.saberAnimLevel,
						SABER_LOCK_SIDE, SABER_LOCK_LOCK, SABER_LOCK_WIN);
					defAnim = g_saber_lock_anim(defender->client->ps.saberAnimLevel,
						attacker->client->ps.saberAnimLevel,
						SABER_LOCK_SIDE, SABER_LOCK_LOCK, SABER_LOCK_LOSE);
					//attacker starts with advantage
					if (g_check_increment_lock_anim(attAnim, SABER_LOCK_WIN))
					{
						attStart = 0.75f; //move to end of anim
					}
					else
					{
						attStart = 0.25f; //start at beginning of anim
					}
					if (g_check_increment_lock_anim(defAnim, SABER_LOCK_LOSE))
					{
						defStart = 0.75f; //start at end of anim
					}
					else
					{
						defStart = 0.25f; //start at beginning of anim
					}
					break;
				default:
					return qfalse;
				}
			}
		}
	}
	//set the proper anims
	NPC_SetAnim(attacker, SETANIM_BOTH, attAnim, SETANIM_FLAG_OVERRIDE | SETANIM_FLAG_HOLD);
	NPC_SetAnim(defender, SETANIM_BOTH, defAnim, SETANIM_FLAG_OVERRIDE | SETANIM_FLAG_HOLD);
	//don't let them store a kick for the whole saberlock....
	attacker->client->ps.saberMoveNext = defender->client->ps.saberMoveNext = LS_NONE;
	//
	if (attStart > 0.0f)
	{
		if (ValidAnimFileIndex(attacker->client->clientInfo.animFileIndex))
		{
			anim = &level.knownAnimFileSets[attacker->client->clientInfo.animFileIndex].animations[attAnim];
			advance = floor(anim->numFrames * attStart);
			PM_SetAnimFrame(attacker, anim->firstFrame + advance, qtrue, qtrue);
			if (d_saberCombat->integer || g_DebugSaberCombat->integer)
			{
				Com_Printf("%s starting saber lock, anim = %s, %d frames to go!\n", attacker->NPC_type,
					animTable[attAnim].name, anim->numFrames - advance);
			}
		}
	}
	if (defStart > 0.0f)
	{
		if (ValidAnimFileIndex(defender->client->clientInfo.animFileIndex))
		{
			anim = &level.knownAnimFileSets[defender->client->clientInfo.animFileIndex].animations[defAnim];
			advance = ceil(anim->numFrames * defStart);
			PM_SetAnimFrame(defender, anim->firstFrame + advance, qtrue, qtrue);
			//was anim->firstFrame + anim->numFrames - advance, but that's wrong since they are matched anims
			if (d_saberCombat->integer || g_DebugSaberCombat->integer)
			{
				Com_Printf("%s starting saber lock, anim = %s, %d frames to go!\n", defender->NPC_type,
					animTable[defAnim].name, advance);
			}
		}
	}
	VectorClear(attacker->client->ps.velocity);
	VectorClear(attacker->client->ps.moveDir);
	VectorClear(defender->client->ps.velocity);
	VectorClear(defender->client->ps.moveDir);

	if (lockMode == LOCK_KYLE_GRAB1
		|| lockMode == LOCK_KYLE_GRAB2
		|| lockMode == LOCK_KYLE_GRAB3
		|| lockMode == LOCK_FORCE_DRAIN)
	{
		//not a real lock, just freeze them both in place
		//can't move or attack
		attacker->client->ps.pm_time = attacker->client->ps.weaponTime = attacker->client->ps.legsAnimTimer;
		attacker->client->ps.pm_flags |= PMF_TIME_KNOCKBACK;
		attacker->painDebounceTime = level.time + attacker->client->ps.pm_time;
		if (lockMode != LOCK_FORCE_DRAIN)
		{
			defender->client->ps.torsoAnimTimer += 200;
			defender->client->ps.legsAnimTimer += 200;
		}
		defender->client->ps.pm_time = defender->client->ps.weaponTime = defender->client->ps.legsAnimTimer;
		defender->client->ps.pm_flags |= PMF_TIME_KNOCKBACK;
		if (lockMode != LOCK_FORCE_DRAIN)
		{
			attacker->aimDebounceTime = level.time + attacker->client->ps.pm_time;
		}
	}
	else
	{
		attacker->client->ps.saberLockTime = defender->client->ps.saberLockTime = level.time + SABER_LOCK_TIME;
		attacker->client->ps.legsAnimTimer = attacker->client->ps.torsoAnimTimer = defender->client->ps.legsAnimTimer =
			defender->client->ps.torsoAnimTimer = SABER_LOCK_TIME;
		//attacker->client->ps.weaponTime = defender->client->ps.weaponTime = SABER_LOCK_TIME;
		attacker->client->ps.saberLockEnemy = defender->s.number;
		defender->client->ps.saberLockEnemy = attacker->s.number;
		attacker->client->ps.userInt3 &= ~(1 << FLAG_ATTACKFAKE);
		defender->client->ps.userInt3 &= ~(1 << FLAG_ATTACKFAKE);
	}

	//MATCH ANGLES
	if (lockMode == LOCK_KYLE_GRAB1
		|| lockMode == LOCK_KYLE_GRAB2
		|| lockMode == LOCK_KYLE_GRAB3)
	{
		//not a real lock, just set pitch to 0
		attacker->client->ps.viewangles[PITCH] = defender->client->ps.viewangles[PITCH] = 0;
	}
	else
	{
		//FIXME: if zDiff in elevation, make lower look up and upper look down and move them closer?
		float defPitchAdd = 0, zDiff = attacker->currentOrigin[2] + attacker->client->standheight - (defender->
			currentOrigin[2] + defender->client->standheight);
		if (zDiff > 24)
		{
			defPitchAdd = -30;
		}
		else if (zDiff < -24)
		{
			defPitchAdd = 30;
		}
		else
		{
			defPitchAdd = zDiff / 24.0f * -30.0f;
		}
		if (attacker->NPC && defender->NPC)
		{
			//if 2 NPCs, just set pitch to 0
			attacker->client->ps.viewangles[PITCH] = -defPitchAdd;
			defender->client->ps.viewangles[PITCH] = defPitchAdd;
		}
		else
		{
			//if a player is involved, clamp player's pitch and match NPC's to player
			if (!attacker->s.number)
			{
				//clamp to defPitch
				if (attacker->client->ps.viewangles[PITCH] > -defPitchAdd + 10)
				{
					attacker->client->ps.viewangles[PITCH] = -defPitchAdd + 10;
				}
				else if (attacker->client->ps.viewangles[PITCH] < -defPitchAdd - 10)
				{
					attacker->client->ps.viewangles[PITCH] = -defPitchAdd - 10;
				}
				//clamp to sane numbers
				if (attacker->client->ps.viewangles[PITCH] > 50)
				{
					attacker->client->ps.viewangles[PITCH] = 50;
				}
				else if (attacker->client->ps.viewangles[PITCH] < -50)
				{
					attacker->client->ps.viewangles[PITCH] = -50;
				}
				defender->client->ps.viewangles[PITCH] = attacker->client->ps.viewangles[PITCH] * -1;
				defPitchAdd = defender->client->ps.viewangles[PITCH];
			}
			else if (!defender->s.number)
			{
				//clamp to defPitch
				if (defender->client->ps.viewangles[PITCH] > defPitchAdd + 10)
				{
					defender->client->ps.viewangles[PITCH] = defPitchAdd + 10;
				}
				else if (defender->client->ps.viewangles[PITCH] < defPitchAdd - 10)
				{
					defender->client->ps.viewangles[PITCH] = defPitchAdd - 10;
				}
				//clamp to sane numbers
				if (defender->client->ps.viewangles[PITCH] > 50)
				{
					defender->client->ps.viewangles[PITCH] = 50;
				}
				else if (defender->client->ps.viewangles[PITCH] < -50)
				{
					defender->client->ps.viewangles[PITCH] = -50;
				}
				defPitchAdd = defender->client->ps.viewangles[PITCH];
				attacker->client->ps.viewangles[PITCH] = defender->client->ps.viewangles[PITCH] * -1;
			}
		}
	}
	vec3_t attAngles, defAngles, defDir;
	VectorSubtract(defender->currentOrigin, attacker->currentOrigin, defDir);
	VectorCopy(attacker->client->ps.viewangles, attAngles);
	attAngles[YAW] = vectoyaw(defDir);
	SetClientViewAngle(attacker, attAngles);
	defAngles[PITCH] = attAngles[PITCH] * -1;
	defAngles[YAW] = AngleNormalize180(attAngles[YAW] + 180);
	defAngles[ROLL] = 0;
	SetClientViewAngle(defender, defAngles);

	//MATCH POSITIONS
	vec3_t newOrg;
	float scale = (attacker->s.modelScale[0] + attacker->s.modelScale[1]) * 0.5f;
	if (scale && scale != 1.0f)
	{
		idealDist += 8 * (scale - 1.0f);
	}
	scale = (defender->s.modelScale[0] + defender->s.modelScale[1]) * 0.5f;
	if (scale && scale != 1.0f)
	{
		idealDist += 8 * (scale - 1.0f);
	}

	float diff = VectorNormalize(defDir) - idealDist; //diff will be the total error in dist
	//try to move attacker half the diff towards the defender
	VectorMA(attacker->currentOrigin, diff * 0.5f, defDir, newOrg);
	trace_t trace;
	gi.trace(&trace, attacker->currentOrigin, attacker->mins, attacker->maxs, newOrg, attacker->s.number,
		attacker->clipmask, static_cast<EG2_Collision>(0), 0);
	if (!trace.startsolid && !trace.allsolid)
	{
		G_SetOrigin(attacker, trace.endpos);
		gi.linkentity(attacker);
	}
	//now get the defender's dist and do it for him too
	vec3_t attDir;
	VectorSubtract(attacker->currentOrigin, defender->currentOrigin, attDir);
	diff = VectorNormalize(attDir) - idealDist; //diff will be the total error in dist
	//try to move defender all of the remaining diff towards the attacker
	VectorMA(defender->currentOrigin, diff, attDir, newOrg);
	gi.trace(&trace, defender->currentOrigin, defender->mins, defender->maxs, newOrg, defender->s.number,
		defender->clipmask, static_cast<EG2_Collision>(0), 0);
	if (!trace.startsolid && !trace.allsolid)
	{
		G_SetOrigin(defender, trace.endpos);
		gi.linkentity(defender);
	}

	//DONE!

	return qtrue;
}

qboolean WP_SabersCheckLock(gentity_t* ent1, gentity_t* ent2)
{
	if (ent1->client->playerTeam == ent2->client->playerTeam)
	{
		return qfalse;
	}
	if (ent1->client->NPC_class == CLASS_SABER_DROID
		|| ent2->client->NPC_class == CLASS_SABER_DROID)
	{
		//they don't have saberlock anims
		return qfalse;
	}
	if (ent1->client->ps.groundEntityNum == ENTITYNUM_NONE ||
		ent2->client->ps.groundEntityNum == ENTITYNUM_NONE)
	{
		return qfalse;
	}
	if (ent1->client->ps.saber[0].saberFlags & SFL_NOT_LOCKABLE
		|| ent2->client->ps.saber[0].saberFlags & SFL_NOT_LOCKABLE)
	{
		//one of these sabers cannot lock (like a lance)
		return qfalse;
	}
	if (ent1->client->ps.dualSabers
		&& ent1->client->ps.saber[1].Active()
		&& ent1->client->ps.saber[1].saberFlags & SFL_NOT_LOCKABLE)
	{
		//one of these sabers cannot lock (like a lance)
		return qfalse;
	}
	if (ent2->client->ps.dualSabers
		&& ent2->client->ps.saber[1].Active()
		&& ent2->client->ps.saber[1].saberFlags & SFL_NOT_LOCKABLE)
	{
		//one of these sabers cannot lock (like a lance)
		return qfalse;
	}
	if (ent1->painDebounceTime > level.time - 1000 || ent2->painDebounceTime > level.time - 1000)
	{
		//can't saberlock if you're not ready
		return qfalse;
	}
	if (fabs(ent1->currentOrigin[2] - ent2->currentOrigin[2]) > 18)
	{
		return qfalse;
	}
	const float dist = DistanceSquared(ent1->currentOrigin, ent2->currentOrigin);

	if (dist < 64 || dist > 6400) //( dist < 128 || dist > 2304 )
	{
		//between 8 and 80 from each other//was 16 and 48
		return qfalse;
	}
	if (!InFOV(ent1, ent2, 40, 180) || !InFOV(ent2, ent1, 40, 180))
	{
		return qfalse;
	}
	if (ent1->client->ps.torsoAnim == BOTH_A2_STABBACK1 && ent1->client->ps.torsoAnimTimer > 300)
	{
		//can't lock when saber is behind you
		return qfalse;
	}
	if (ent1->client->ps.torsoAnim == BOTH_A2_STABBACK1B && ent1->client->ps.torsoAnimTimer > 300)
	{
		//can't lock when saber is behind you
		return qfalse;
	}
	if (ent2->client->ps.torsoAnim == BOTH_A2_STABBACK1 && ent2->client->ps.torsoAnimTimer > 300)
	{
		//can't lock when saber is behind you
		return qfalse;
	}
	if (ent2->client->ps.torsoAnim == BOTH_A2_STABBACK1B && ent2->client->ps.torsoAnimTimer > 300)
	{
		//can't lock when saber is behind you
		return qfalse;
	}
	if (PM_LockedAnim(ent1->client->ps.torsoAnim)
		|| PM_LockedAnim(ent2->client->ps.torsoAnim))
	{
		//stuck doing something else
		return qfalse;
	}
	if (PM_SaberLockBreakAnim(ent1->client->ps.torsoAnim)
		|| PM_SaberLockBreakAnim(ent2->client->ps.torsoAnim))
	{
		//still finishing the last lock break!
		return qfalse;
	}
	//BR to TL lock
	if (ent1->client->ps.torsoAnim == BOTH_A1_BR_TL ||
		ent1->client->ps.torsoAnim == BOTH_A2_BR_TL ||
		ent1->client->ps.torsoAnim == BOTH_A3_BR_TL ||
		ent1->client->ps.torsoAnim == BOTH_A4_BR_TL ||
		ent1->client->ps.torsoAnim == BOTH_A5_BR_TL ||
		ent1->client->ps.torsoAnim == BOTH_A6_BR_TL ||
		ent1->client->ps.torsoAnim == BOTH_A7_BR_TL)
	{
		//ent1 is attacking in the opposite diagonal
		return WP_SabersCheckLock2(ent1, ent2, LOCK_DIAG_BR);
	}
	if (ent2->client->ps.torsoAnim == BOTH_A1_BR_TL ||
		ent2->client->ps.torsoAnim == BOTH_A2_BR_TL ||
		ent2->client->ps.torsoAnim == BOTH_A3_BR_TL ||
		ent2->client->ps.torsoAnim == BOTH_A4_BR_TL ||
		ent2->client->ps.torsoAnim == BOTH_A5_BR_TL ||
		ent2->client->ps.torsoAnim == BOTH_A6_BR_TL ||
		ent2->client->ps.torsoAnim == BOTH_A7_BR_TL)
	{
		//ent2 is attacking in the opposite diagonal
		return WP_SabersCheckLock2(ent2, ent1, LOCK_DIAG_BR);
	}
	//BL to TR lock
	if (ent1->client->ps.torsoAnim == BOTH_A1_BL_TR ||
		ent1->client->ps.torsoAnim == BOTH_A2_BL_TR ||
		ent1->client->ps.torsoAnim == BOTH_A3_BL_TR ||
		ent1->client->ps.torsoAnim == BOTH_A4_BL_TR ||
		ent1->client->ps.torsoAnim == BOTH_A5_BL_TR ||
		ent1->client->ps.torsoAnim == BOTH_A6_BL_TR ||
		ent1->client->ps.torsoAnim == BOTH_A7_BL_TR)
	{
		//ent1 is attacking in the opposite diagonal
		return WP_SabersCheckLock2(ent1, ent2, LOCK_DIAG_BL);
	}
	if (ent2->client->ps.torsoAnim == BOTH_A1_BL_TR ||
		ent2->client->ps.torsoAnim == BOTH_A2_BL_TR ||
		ent2->client->ps.torsoAnim == BOTH_A3_BL_TR ||
		ent2->client->ps.torsoAnim == BOTH_A4_BL_TR ||
		ent2->client->ps.torsoAnim == BOTH_A5_BL_TR ||
		ent2->client->ps.torsoAnim == BOTH_A6_BL_TR ||
		ent2->client->ps.torsoAnim == BOTH_A7_BL_TR)
	{
		//ent2 is attacking in the opposite diagonal
		return WP_SabersCheckLock2(ent2, ent1, LOCK_DIAG_BL);
	}
	//L to R lock
	if (ent1->client->ps.torsoAnim == BOTH_A1__L__R ||
		ent1->client->ps.torsoAnim == BOTH_A2__L__R ||
		ent1->client->ps.torsoAnim == BOTH_A3__L__R ||
		ent1->client->ps.torsoAnim == BOTH_A4__L__R ||
		ent1->client->ps.torsoAnim == BOTH_A5__L__R ||
		ent1->client->ps.torsoAnim == BOTH_A6__L__R ||
		ent1->client->ps.torsoAnim == BOTH_A7__L__R)
	{
		//ent1 is attacking l to r
		return WP_SabersCheckLock2(ent1, ent2, LOCK_L);
	}
	if (ent2->client->ps.torsoAnim == BOTH_A1__L__R ||
		ent2->client->ps.torsoAnim == BOTH_A2__L__R ||
		ent2->client->ps.torsoAnim == BOTH_A3__L__R ||
		ent2->client->ps.torsoAnim == BOTH_A4__L__R ||
		ent2->client->ps.torsoAnim == BOTH_A5__L__R ||
		ent2->client->ps.torsoAnim == BOTH_A6__L__R ||
		ent2->client->ps.torsoAnim == BOTH_A7__L__R)
	{
		//ent2 is attacking l to r
		return WP_SabersCheckLock2(ent2, ent1, LOCK_L);
	}
	//R to L lock
	if (ent1->client->ps.torsoAnim == BOTH_A1__R__L ||
		ent1->client->ps.torsoAnim == BOTH_A2__R__L ||
		ent1->client->ps.torsoAnim == BOTH_A3__R__L ||
		ent1->client->ps.torsoAnim == BOTH_A4__R__L ||
		ent1->client->ps.torsoAnim == BOTH_A5__R__L ||
		ent1->client->ps.torsoAnim == BOTH_A6__R__L ||
		ent1->client->ps.torsoAnim == BOTH_A7__R__L)
	{
		//ent1 is attacking r to l
		return WP_SabersCheckLock2(ent1, ent2, LOCK_R);
	}
	if (ent2->client->ps.torsoAnim == BOTH_A1__R__L ||
		ent2->client->ps.torsoAnim == BOTH_A2__R__L ||
		ent2->client->ps.torsoAnim == BOTH_A3__R__L ||
		ent2->client->ps.torsoAnim == BOTH_A4__R__L ||
		ent2->client->ps.torsoAnim == BOTH_A5__R__L ||
		ent2->client->ps.torsoAnim == BOTH_A6__R__L ||
		ent2->client->ps.torsoAnim == BOTH_A7__R__L)
	{
		//ent2 is attacking r to l
		return WP_SabersCheckLock2(ent2, ent1, LOCK_R);
	}
	//TR to BL lock
	if (ent1->client->ps.torsoAnim == BOTH_A1_TR_BL ||
		ent1->client->ps.torsoAnim == BOTH_A2_TR_BL ||
		ent1->client->ps.torsoAnim == BOTH_A3_TR_BL ||
		ent1->client->ps.torsoAnim == BOTH_A4_TR_BL ||
		ent1->client->ps.torsoAnim == BOTH_A5_TR_BL ||
		ent1->client->ps.torsoAnim == BOTH_A6_TR_BL ||
		ent1->client->ps.torsoAnim == BOTH_A7_TR_BL)
	{
		//ent1 is attacking diagonally
		return WP_SabersCheckLock2(ent1, ent2, LOCK_DIAG_TR);
	}

	if (ent2->client->ps.torsoAnim == BOTH_A1_TR_BL ||
		ent2->client->ps.torsoAnim == BOTH_A2_TR_BL ||
		ent2->client->ps.torsoAnim == BOTH_A3_TR_BL ||
		ent2->client->ps.torsoAnim == BOTH_A4_TR_BL ||
		ent2->client->ps.torsoAnim == BOTH_A5_TR_BL ||
		ent2->client->ps.torsoAnim == BOTH_A6_TR_BL ||
		ent2->client->ps.torsoAnim == BOTH_A7_TR_BL)
	{
		//ent2 is attacking diagonally
		return WP_SabersCheckLock2(ent2, ent1, LOCK_DIAG_TR);
	}

	//TL to BR lock
	if (ent1->client->ps.torsoAnim == BOTH_A1_TL_BR ||
		ent1->client->ps.torsoAnim == BOTH_A2_TL_BR ||
		ent1->client->ps.torsoAnim == BOTH_A3_TL_BR ||
		ent1->client->ps.torsoAnim == BOTH_A4_TL_BR ||
		ent1->client->ps.torsoAnim == BOTH_A5_TL_BR ||
		ent1->client->ps.torsoAnim == BOTH_A6_TL_BR ||
		ent1->client->ps.torsoAnim == BOTH_A7_TL_BR)
	{
		//ent1 is attacking diagonally
		return WP_SabersCheckLock2(ent1, ent2, LOCK_DIAG_TL);
	}

	if (ent2->client->ps.torsoAnim == BOTH_A1_TL_BR ||
		ent2->client->ps.torsoAnim == BOTH_A2_TL_BR ||
		ent2->client->ps.torsoAnim == BOTH_A3_TL_BR ||
		ent2->client->ps.torsoAnim == BOTH_A4_TL_BR ||
		ent2->client->ps.torsoAnim == BOTH_A5_TL_BR ||
		ent2->client->ps.torsoAnim == BOTH_A6_TL_BR ||
		ent2->client->ps.torsoAnim == BOTH_A7_TL_BR)
	{
		//ent2 is attacking diagonally
		return WP_SabersCheckLock2(ent2, ent1, LOCK_DIAG_TL);
	}

	//T to B lock
	if (ent1->client->ps.torsoAnim == BOTH_A1_T__B_ ||
		ent1->client->ps.torsoAnim == BOTH_A2_T__B_ ||
		ent1->client->ps.torsoAnim == BOTH_A3_T__B_ ||
		ent1->client->ps.torsoAnim == BOTH_A4_T__B_ ||
		ent1->client->ps.torsoAnim == BOTH_A5_T__B_ ||
		ent1->client->ps.torsoAnim == BOTH_A6_T__B_ ||
		ent1->client->ps.torsoAnim == BOTH_A7_T__B_)
	{
		//ent1 is attacking top-down
		return WP_SabersCheckLock2(ent1, ent2, LOCK_TOP);
	}

	if (ent2->client->ps.torsoAnim == BOTH_A1_T__B_ ||
		ent2->client->ps.torsoAnim == BOTH_A2_T__B_ ||
		ent2->client->ps.torsoAnim == BOTH_A3_T__B_ ||
		ent2->client->ps.torsoAnim == BOTH_A4_T__B_ ||
		ent2->client->ps.torsoAnim == BOTH_A5_T__B_ ||
		ent2->client->ps.torsoAnim == BOTH_A6_T__B_ ||
		ent2->client->ps.torsoAnim == BOTH_A7_T__B_)
	{
		//ent2 is attacking top-down
		return WP_SabersCheckLock2(ent2, ent1, LOCK_TOP);
	}

	return qfalse;
}

int G_GetParryForBlock(const int block)
{
	switch (block)
	{
	case BLOCKED_UPPER_RIGHT:
		return LS_PARRY_UR;
	case BLOCKED_UPPER_RIGHT_PROJ:
		return LS_K1_TR;
	case BLOCKED_UPPER_LEFT:
		return LS_PARRY_UL;
	case BLOCKED_UPPER_LEFT_PROJ:
		return LS_K1_TL;
	case BLOCKED_LOWER_RIGHT:
		return LS_PARRY_LR;
	case BLOCKED_LOWER_RIGHT_PROJ:
		return LS_REFLECT_LR;
	case BLOCKED_LOWER_LEFT:
		return LS_PARRY_LL;
	case BLOCKED_LOWER_LEFT_PROJ:
		return LS_REFLECT_LL;
	case BLOCKED_TOP:
		return LS_PARRY_UP;
	case BLOCKED_TOP_PROJ:
		return LS_REFLECT_UP;
	case BLOCKED_FRONT:
		return LS_PARRY_FRONT;
	case BLOCKED_FRONT_PROJ:
		return LS_REFLECT_FRONT;
	default:
		break;
	}

	return LS_NONE;
}

qboolean WP_SaberMBlock(gentity_t* blocker, gentity_t* attacker, int saberNum, int bladeNum)
{
	const qboolean OtherHoldingBlock = blocker->client->ps.ManualBlockingFlags & 1 << HOLDINGBLOCK ? qtrue : qfalse;	//Normal Blocking
	const qboolean OtherActiveBlocking = blocker->client->ps.ManualBlockingFlags & 1 << HOLDINGBLOCKANDATTACK ? qtrue : qfalse; //Active Blocking
	const qboolean OtherMBlocking = blocker->client->ps.ManualBlockingFlags & 1 << PERFECTBLOCKING ? qtrue : qfalse; //Perfect Blocking
	const qboolean NPCBlocking = blocker->client->ps.ManualBlockingFlags & 1 << MBF_NPCBLOCKING ? qtrue : qfalse;//Normal Blocking

	if (!blocker || !blocker->client || !attacker)
	{
		return qfalse;
	}
	if (Rosh_BeingHealed(blocker))
	{
		return qfalse;
	}
	if (g_in_cinematic_saber_anim(blocker))
	{
		return qfalse;
	}
	if (PM_SuperBreakLoseAnim(blocker->client->ps.torsoAnim)
		|| PM_SuperBreakWinAnim(blocker->client->ps.torsoAnim))
	{
		return qfalse;
	}

	if (blocker->s.number
		|| OtherHoldingBlock
		|| OtherActiveBlocking
		|| OtherMBlocking
		|| NPCBlocking
		|| g_saberAutoBlocking->integer && blocker->NPC && !G_ControlledByPlayer(blocker)
		|| manual_saberblocking(blocker)  // already in a block position it will override
		|| blocker->client->ps.saberBlockingTime > level.time)
	{
		//either an NPC or a player who is blocking
		if (!PM_SaberInTransitionAny(blocker->client->ps.saberMove)
			&& !PM_SaberInBounce(blocker->client->ps.saberMove)
			&& !PM_SaberInKnockaway(blocker->client->ps.saberMove)
			&& !BG_InKnockDown(blocker->client->ps.legsAnim)
			&& !BG_InKnockDown(blocker->client->ps.torsoAnim)
			&& !PM_InKnockDown(&blocker->client->ps))
		{
			//I'm not attacking, in transition or in a bounce or knockaway, so play a parry
			WP_SaberMBlockDirection(blocker, saberHitLocation, qfalse);
		}

		//just so blocker knows that he has parried the attacker
		blocker->client->ps.saberEventFlags |= SEF_PARRIED;

		//since it was parried, take away any damage done
		wp_saber_clear_damage_for_ent_num(attacker, blocker->s.number, saberNum, bladeNum);

		//tell the victim to get mad at me
		if (blocker->enemy != attacker && blocker->client->playerTeam != attacker->client->playerTeam)
		{
			//they're not mad at me and they're not on my team
			G_ClearEnemy(blocker);
			G_SetEnemy(blocker, attacker);
		}
		return qtrue;
	}
	return qfalse;
}

qboolean WP_SaberParry(gentity_t* blocker, gentity_t* attacker, int saberNum, int bladeNum)
{
	const qboolean OtherHoldingBlock = blocker->client->ps.ManualBlockingFlags & 1 << HOLDINGBLOCK ? qtrue : qfalse;	//Normal Blocking
	const qboolean OtherActiveBlocking = blocker->client->ps.ManualBlockingFlags & 1 << HOLDINGBLOCKANDATTACK ? qtrue : qfalse; //Active Blocking
	const qboolean OtherMBlocking = blocker->client->ps.ManualBlockingFlags & 1 << PERFECTBLOCKING ? qtrue : qfalse; //Perfect Blocking
	const qboolean NPCBlocking = blocker->client->ps.ManualBlockingFlags & 1 << MBF_NPCBLOCKING ? qtrue : qfalse;//Normal Blocking

	if (!blocker || !blocker->client || !attacker)
	{
		return qfalse;
	}
	if (Rosh_BeingHealed(blocker))
	{
		return qfalse;
	}
	if (g_in_cinematic_saber_anim(blocker))
	{
		return qfalse;
	}
	if (PM_SuperBreakLoseAnim(blocker->client->ps.torsoAnim)
		|| PM_SuperBreakWinAnim(blocker->client->ps.torsoAnim))
	{
		return qfalse;
	}

	if (blocker->s.number
		|| OtherHoldingBlock
		|| OtherActiveBlocking
		|| OtherMBlocking
		|| NPCBlocking
		|| g_saberAutoBlocking->integer && blocker->NPC && !G_ControlledByPlayer(blocker)
		|| manual_saberblocking(blocker)  // already in a block position it will override
		|| blocker->client->ps.saberBlockingTime > level.time)
	{
		//either an NPC or a player who is blocking
		if (!PM_SaberInTransitionAny(blocker->client->ps.saberMove)
			&& !PM_SaberInBounce(blocker->client->ps.saberMove)
			&& !PM_SaberInKnockaway(blocker->client->ps.saberMove)
			&& !BG_InKnockDown(blocker->client->ps.legsAnim)
			&& !BG_InKnockDown(blocker->client->ps.torsoAnim)
			&& !PM_InKnockDown(&blocker->client->ps))
		{
			//I'm not attacking, in transition or in a bounce or knockaway, so play a parry
			WP_SaberBlockNonRandom(blocker, saberHitLocation, qfalse);
		}

		//just so blocker knows that he has parried the attacker
		blocker->client->ps.saberEventFlags |= SEF_PARRIED;

		//since it was parried, take away any damage done
		wp_saber_clear_damage_for_ent_num(attacker, blocker->s.number, saberNum, bladeNum);

		//tell the victim to get mad at me
		if (blocker->enemy != attacker && blocker->client->playerTeam != attacker->client->playerTeam)
		{
			//they're not mad at me and they're not on my team
			G_ClearEnemy(blocker);
			G_SetEnemy(blocker, attacker);
		}
		return qtrue;
	}
	return qfalse;
}

qboolean WP_SaberBlockedBounceBlock(gentity_t* blocker, gentity_t* attacker, int saberNum, int bladeNum)
{
	const qboolean OtherHoldingBlock = blocker->client->ps.ManualBlockingFlags & 1 << HOLDINGBLOCK ? qtrue : qfalse;	//Normal Blocking
	const qboolean OtherActiveBlocking = blocker->client->ps.ManualBlockingFlags & 1 << HOLDINGBLOCKANDATTACK ? qtrue : qfalse; //Active Blocking
	const qboolean OtherMBlocking = blocker->client->ps.ManualBlockingFlags & 1 << PERFECTBLOCKING ? qtrue : qfalse; //Perfect Blocking
	const qboolean NPCBlocking = blocker->client->ps.ManualBlockingFlags & 1 << MBF_NPCBLOCKING ? qtrue : qfalse;//Normal Blocking

	if (!blocker || !blocker->client || !attacker)
	{
		return qfalse;
	}
	if (Rosh_BeingHealed(blocker))
	{
		return qfalse;
	}
	if (g_in_cinematic_saber_anim(blocker))
	{
		return qfalse;
	}
	if (PM_SuperBreakLoseAnim(blocker->client->ps.torsoAnim)
		|| PM_SuperBreakWinAnim(blocker->client->ps.torsoAnim))
	{
		return qfalse;
	}

	if (blocker->s.number
		|| OtherHoldingBlock
		|| OtherActiveBlocking
		|| OtherMBlocking
		|| NPCBlocking
		|| g_saberAutoBlocking->integer && blocker->NPC && !G_ControlledByPlayer(blocker)
		|| manual_saberblocking(blocker)  // already in a block position it will override
		|| blocker->client->ps.saberBlockingTime > level.time)
	{
		//either an NPC or a player who is blocking
		if (!PM_SaberInTransitionAny(blocker->client->ps.saberMove)
			&& !PM_SaberInBounce(blocker->client->ps.saberMove)
			&& !PM_SaberInKnockaway(blocker->client->ps.saberMove)
			&& !BG_InKnockDown(blocker->client->ps.legsAnim)
			&& !BG_InKnockDown(blocker->client->ps.torsoAnim)
			&& !PM_InKnockDown(&blocker->client->ps))
		{
			//I'm not attacking, in transition or in a bounce or knockaway, so play a parry
			WP_SaberBouncedSaberDirection(blocker, saberHitLocation, qfalse);
		}

		//just so blocker knows that he has parried the attacker
		blocker->client->ps.saberEventFlags |= SEF_PARRIED;

		//since it was parried, take away any damage done
		wp_saber_clear_damage_for_ent_num(attacker, blocker->s.number, saberNum, bladeNum);

		//tell the victim to get mad at me
		if (blocker->enemy != attacker && blocker->client->playerTeam != attacker->client->playerTeam)
		{
			//they're not mad at me and they're not on my team
			G_ClearEnemy(blocker);
			G_SetEnemy(blocker, attacker);
		}
		return qtrue;
	}

	return qfalse;
}

qboolean WP_SaberFatiguedParry(gentity_t* blocker, gentity_t* attacker, int saberNum, int bladeNum)
{
	const qboolean OtherHoldingBlock = blocker->client->ps.ManualBlockingFlags & 1 << HOLDINGBLOCK ? qtrue : qfalse;	//Normal Blocking
	const qboolean OtherActiveBlocking = blocker->client->ps.ManualBlockingFlags & 1 << HOLDINGBLOCKANDATTACK ? qtrue : qfalse; //Active Blocking
	const qboolean OtherMBlocking = blocker->client->ps.ManualBlockingFlags & 1 << PERFECTBLOCKING ? qtrue : qfalse; //Perfect Blocking
	const qboolean NPCBlocking = blocker->client->ps.ManualBlockingFlags & 1 << MBF_NPCBLOCKING ? qtrue : qfalse;//Normal Blocking

	if (!blocker || !blocker->client || !attacker)
	{
		return qfalse;
	}
	if (Rosh_BeingHealed(blocker))
	{
		return qfalse;
	}
	if (g_in_cinematic_saber_anim(blocker))
	{
		return qfalse;
	}
	if (PM_SuperBreakLoseAnim(blocker->client->ps.torsoAnim)
		|| PM_SuperBreakWinAnim(blocker->client->ps.torsoAnim))
	{
		return qfalse;
	}

	if (blocker->s.number
		|| OtherHoldingBlock
		|| OtherActiveBlocking
		|| OtherMBlocking
		|| NPCBlocking
		|| g_saberAutoBlocking->integer && blocker->NPC && !G_ControlledByPlayer(blocker)
		|| manual_saberblocking(blocker)  // already in a block position it will override
		|| blocker->client->ps.saberBlockingTime > level.time)
	{
		//either an NPC or a player who is blocking
		if (!PM_SaberInTransitionAny(blocker->client->ps.saberMove)
			&& !PM_SaberInBounce(blocker->client->ps.saberMove)
			&& !PM_SaberInKnockaway(blocker->client->ps.saberMove)
			&& !BG_InKnockDown(blocker->client->ps.legsAnim)
			&& !BG_InKnockDown(blocker->client->ps.torsoAnim)
			&& !PM_InKnockDown(&blocker->client->ps))
		{
			//I'm not attacking, in transition or in a bounce or knockaway, so play a parry
			WP_SaberFatiguedParryDirection(blocker, saberHitLocation, qfalse);
		}

		//just so blocker knows that he has parried the attacker
		blocker->client->ps.saberEventFlags |= SEF_PARRIED;

		//since it was parried, take away any damage done
		wp_saber_clear_damage_for_ent_num(attacker, blocker->s.number, saberNum, bladeNum);

		//tell the victim to get mad at me
		if (blocker->enemy != attacker && blocker->client->playerTeam != attacker->client->playerTeam)
		{
			//they're not mad at me and they're not on my team
			G_ClearEnemy(blocker);
			G_SetEnemy(blocker, attacker);
		}
		return qtrue;
	}
	return qfalse;
}
qboolean saberShotOutOfHand(gentity_t* self, vec3_t throwDir);

qboolean WP_BrokenBoltBlockKnockBack(gentity_t* victim, gentity_t* attacker, qboolean allowAnyMove)
{
	if (!victim || !victim->client)
	{
		return qfalse;
	}

	if (victim->s.weapon == WP_SABER && victim->client->ps.SaberActive() && victim->client->ps.blockPoints <=
		BLOCKPOINTS_TEN
		|| victim->s.weapon == WP_SABER && victim->client->ps.SaberActive() && victim->client->ps.forcePower <=
		BLOCKPOINTS_TEN)
	{
		//knock their asses down!!
		G_Stagger(victim);

		vec3_t throwDir = { 0, 0, 350 };

		saberShotOutOfHand(victim, throwDir);

		G_AddEvent(victim, EV_PAIN, victim->health);
		return qtrue;
	}
	G_Stagger(victim);
	return qtrue;
}

qboolean G_TryingKataAttack(gentity_t* self, const usercmd_t* cmd)
{
	if (g_saberNewControlScheme->integer)
	{
		//use the new control scheme: force focus button
		if (cmd->buttons & BUTTON_FORCE_FOCUS)
		{
			return qtrue;
		}
		return qfalse;
	}
	//use the old control scheme
	if (cmd->buttons & BUTTON_ALT_ATTACK)
	{
		//pressing alt-attack
		if (cmd->buttons & BUTTON_ATTACK)
		{
			//pressing attack
			return qtrue;
		}
	}
	return qfalse;
}

qboolean G_TryingPullAttack(const gentity_t* self, const usercmd_t* cmd, qboolean amPulling)
{
	if (g_saberNewControlScheme->integer)
	{
		//use the new control scheme: force focus button
		if (cmd->buttons & BUTTON_FORCE_FOCUS)
		{
			if (self && self->client)
			{
				if (self->client->ps.forcePowerLevel[FP_PULL] >= FORCE_LEVEL_3)
				{
					//force pull 3
					if (amPulling
						|| self->client->ps.forcePowersActive & 1 << FP_PULL
						|| self->client->ps.forcePowerDebounce[FP_PULL] > level.time) //force-pulling
					{
						//pulling
						return qtrue;
					}
				}
			}
		}
		else
		{
			return qfalse;
		}
	}
	else
	{
		//use the old control scheme
		if (cmd->buttons & BUTTON_ATTACK)
		{
			//pressing attack
			if (self && self->client)
			{
				if (self->client->ps.forcePowerLevel[FP_PULL] >= FORCE_LEVEL_3)
				{
					//force pull 3
					if (amPulling
						|| self->client->ps.forcePowersActive & 1 << FP_PULL
						|| self->client->ps.forcePowerDebounce[FP_PULL] > level.time) //force-pulling
					{
						//pulling
						return qtrue;
					}
				}
			}
		}
	}
	return qfalse;
}

qboolean G_TryingCartwheel(const gentity_t* self, const usercmd_t* cmd)
{
	if (g_saberNewControlScheme->integer)
	{
		//use the new control scheme: force focus button
		if (cmd->buttons & BUTTON_FORCE_FOCUS)
		{
			return qtrue;
		}
		return qfalse;
	}
	//use the old control scheme
	if (cmd->buttons & BUTTON_ATTACK)
	{
		//pressing attack
		if (cmd->rightmove)
		{
			if (self && self->client)
			{
				if (cmd->upmove > 0 //)
					&& self->client->ps.groundEntityNum != ENTITYNUM_NONE)
				{
					//on ground, pressing jump
					return qtrue;
				}
				//just jumped?
				if (self->client->ps.groundEntityNum == ENTITYNUM_NONE
					&& level.time - self->client->ps.lastOnGround <= 50 //250
					&& self->client->ps.pm_flags & PMF_JUMPING) //jumping
				{
					//just jumped this or last frame
					return qtrue;
				}
			}
		}
	}
	return qfalse;
}

qboolean G_TryingJumpAttack(const gentity_t* self, const usercmd_t* cmd)
{
	if (g_saberNewControlScheme->integer)
	{
		//use the new control scheme: force focus button
		if (cmd->buttons & BUTTON_FORCE_FOCUS)
		{
			return qtrue;
		}
		return qfalse;
	}
	//use the old control scheme
	if (cmd->buttons & BUTTON_ATTACK)
	{
		//pressing attack
		if (cmd->upmove > 0)
		{
			//pressing jump
			return qtrue;
		}
		if (self && self->client)
		{
			//just jumped?
			if (self->client->ps.groundEntityNum == ENTITYNUM_NONE
				&& level.time - self->client->ps.lastOnGround <= 250
				&& self->client->ps.pm_flags & PMF_JUMPING) //jumping
			{
				//jumped within the last quarter second
				return qtrue;
			}
		}
	}
	return qfalse;
}

qboolean G_TryingJumpForwardAttack(const gentity_t* self, const usercmd_t* cmd)
{
	if (g_saberNewControlScheme->integer)
	{
		//use the new control scheme: force focus button
		if (cmd->buttons & BUTTON_FORCE_FOCUS)
		{
			return qtrue;
		}
		return qfalse;
	}
	//use the old control scheme
	if (cmd->buttons & BUTTON_ATTACK)
	{
		//pressing attack
		if (cmd->forwardmove > 0)
		{
			//moving forward
			if (self && self->client)
			{
				if (cmd->upmove > 0
					&& self->client->ps.groundEntityNum != ENTITYNUM_NONE)
				{
					//pressing jump
					return qtrue;
				}
				//no slop on forward jumps - must be precise!
				if (self->client->ps.groundEntityNum == ENTITYNUM_NONE
					&& level.time - self->client->ps.lastOnGround <= 50
					&& self->client->ps.pm_flags & PMF_JUMPING) //jumping
				{
					//just jumped this or last frame
					return qtrue;
				}
			}
		}
	}
	return qfalse;
}

qboolean G_TryingLungeAttack(const gentity_t* self, const usercmd_t* cmd)
{
	if (g_saberNewControlScheme->integer)
	{
		//use the new control scheme: force focus button
		if (cmd->buttons & BUTTON_FORCE_FOCUS)
		{
			return qtrue;
		}
		return qfalse;
	}
	//use the old control scheme
	if (cmd->buttons & BUTTON_ATTACK)
	{
		//pressing attack
		if (cmd->upmove < 0)
		{
			//pressing crouch
			return qtrue;
		}
		if (self && self->client)
		{
			//just unducked?
			if (self->client->ps.pm_flags & PMF_DUCKED)
			{
				//just unducking
				return qtrue;
			}
		}
	}
	return qfalse;
}

//FIXME: for these below funcs, maybe in the old control scheme some moves should still cost power... if so, pass in the saberMove and use a switch statement
qboolean G_EnoughPowerForSpecialMove(int forcePower, int cost, qboolean kataMove)
{
	if (g_saberNewControlScheme->integer || kataMove)
	{
		//special moves cost power
		if (forcePower >= cost)
		{
			return qtrue;
		}
		cg.forceHUDTotalFlashTime = level.time + 1000;
		cg.mishapHUDTotalFlashTime = level.time + 500;
		cg.blockHUDTotalFlashTime = level.time + 500;
		return qfalse;
	}
	//old control scheme: uses no power, so just do it
	return qtrue;
}

void G_DrainPowerForSpecialMove(const gentity_t* self, forcePowers_t fp, int cost, qboolean kataMove)
{
	if (!self || !self->client || self->s.number >= MAX_CLIENTS)
	{
		return;
	}
	if (g_saberNewControlScheme->integer || kataMove)
	{
		//special moves cost power
		WP_ForcePowerDrain(self, fp, cost); //drain the required force power
	}
	else
	{
		//old control scheme: uses no power, so just do it
	}
}

int G_CostForSpecialMove(int cost, qboolean kataMove)
{
	if (g_saberNewControlScheme->integer || kataMove)
	{
		//special moves cost power
		return cost;
	}
	//old control scheme: uses no power, so just do it
	return 0;
}

extern qboolean G_EntIsBreakable(int entityNum, const gentity_t* breaker);

void WP_SaberRadiusDamage(gentity_t* ent, vec3_t point, float radius, int damage, float knockBack)
{
	if (!ent || !ent->client)
	{
		return;
	}
	if (radius <= 0.0f || damage <= 0 && knockBack <= 0)
	{
		return;
	}
	vec3_t mins, maxs, entDir;
	gentity_t* radiusEnts[128];
	int i;

	//Setup the bbox to search in
	for (i = 0; i < 3; i++)
	{
		mins[i] = point[i] - radius;
		maxs[i] = point[i] + radius;
	}

	//Get the number of entities in a given space
	const int numEnts = gi.EntitiesInBox(mins, maxs, radiusEnts, 128);

	for (i = 0; i < numEnts; i++)
	{
		if (!radiusEnts[i]->inuse)
		{
			continue;
		}

		if (radiusEnts[i] == ent)
		{
			//Skip myself
			continue;
		}

		if (radiusEnts[i]->client == nullptr)
		{
			//must be a client
			if (G_EntIsBreakable(radiusEnts[i]->s.number, ent))
			{
				//damage breakables within range, but not as much
				G_Damage(radiusEnts[i], ent, ent, vec3_origin, radiusEnts[i]->currentOrigin, 10, 0,
					MOD_EXPLOSIVE_SPLASH);
			}
			continue;
		}

		if (radiusEnts[i]->client->ps.eFlags & EF_HELD_BY_RANCOR
			|| radiusEnts[i]->client->ps.eFlags & EF_HELD_BY_WAMPA)
		{
			//can't be one being held
			continue;
		}

		VectorSubtract(radiusEnts[i]->currentOrigin, point, entDir);
		const float dist = VectorNormalize(entDir);
		if (dist <= radius)
		{
			//in range
			if (damage > 0)
			{
				//do damage
				const int points = ceil(static_cast<float>(damage) * dist / radius);
				G_Damage(radiusEnts[i], ent, ent, vec3_origin, radiusEnts[i]->currentOrigin, points,
					DAMAGE_NO_KNOCKBACK, MOD_EXPLOSIVE_SPLASH);
			}
			if (knockBack > 0)
			{
				//do knockback
				if (radiusEnts[i]->client
					&& radiusEnts[i]->client->NPC_class != CLASS_RANCOR
					&& radiusEnts[i]->client->NPC_class != CLASS_ATST
					&& !(radiusEnts[i]->flags & FL_NO_KNOCKBACK)) //don't throw them back
				{
					const float knockbackStr = knockBack * dist / radius;
					entDir[2] += 0.1f;
					VectorNormalize(entDir);
					g_throw(radiusEnts[i], entDir, knockbackStr);
					if (radiusEnts[i]->health > 0)
					{
						//still alive
						if (knockbackStr > 50)
						{
							//close enough and knockback high enough to possibly knock down
							if (dist < radius * 0.5f
								|| radiusEnts[i]->client->ps.groundEntityNum != ENTITYNUM_NONE)
							{
								//within range of my fist or within ground-shaking range and not in the air
								G_Knockdown(radiusEnts[i], ent, entDir, 500, qtrue);
							}
						}
					}
				}
			}
		}
	}
}

void G_Stagger(gentity_t* hit_ent)
{
	if (PM_InGetUp(&hit_ent->client->ps) || PM_InForceGetUp(&hit_ent->client->ps))
	{
		return;
	}

	const int anim_choice = irand(0, 6);
	// this could possibly be based on animation done when the clash happend, but this should do for now.
	int use_anim;

	switch (anim_choice)
	{
	default:
	case 0:
		use_anim = BOTH_BASHED1;
		break;
	case 1:
		use_anim = BOTH_H1_S1_T_;
		break;
	case 2:
		use_anim = BOTH_H1_S1_TR;
		break;
	case 3:
		use_anim = BOTH_H1_S1_TL;
		break;
	case 4:
		use_anim = BOTH_H1_S1_BL;
		break;
	case 5:
		use_anim = BOTH_H1_S1_B_;
		break;
	case 6:
		use_anim = BOTH_H1_S1_BR;
		break;
	}

	NPC_SetAnim(hit_ent, SETANIM_TORSO, use_anim, SETANIM_AFLAG_PACE);

	if (PM_SaberInMassiveBounce(hit_ent->client->ps.torsoAnim))
	{
		hit_ent->client->ps.saberMove = LS_NONE;
		hit_ent->client->ps.saberBlocked = BLOCKED_NONE;
		hit_ent->client->ps.weaponTime = hit_ent->client->ps.torsoAnimTimer;
		hit_ent->client->MassiveBounceAnimTime = hit_ent->client->ps.torsoAnimTimer + level.time;
	}
	else
	{
		hit_ent->client->ps.saberMove = LS_READY;
	}

	if (hit_ent->client->ps.saberAnimLevel == SS_DUAL)
	{
		SabBeh_AnimateMassiveDualSlowBounce(use_anim);
	}
	else if (hit_ent->client->ps.saberAnimLevel == SS_STAFF)
	{
		SabBeh_AnimateMassiveStaffSlowBounce(use_anim);
	}
}

void G_FatigueBPKnockaway(gentity_t* Blocker)
{
	if (PM_InGetUp(&Blocker->client->ps) || PM_InForceGetUp(&Blocker->client->ps))
	{
		return;
	}

	const int anim_choice = irand(0, 5);

	if (Blocker->client->ps.saberAnimLevel == SS_DUAL)
	{
		switch (anim_choice)
		{
		default:
		case 0:
			NPC_SetAnim(Blocker, SETANIM_TORSO, BOTH_K6_S6_T_, SETANIM_AFLAG_PACE);
			break;
		case 1:
			NPC_SetAnim(Blocker, SETANIM_TORSO, BOTH_K6_S6_TR, SETANIM_AFLAG_PACE);
			break;
		case 2:
			NPC_SetAnim(Blocker, SETANIM_TORSO, BOTH_K6_S6_TL, SETANIM_AFLAG_PACE);
			break;
		case 3:
			NPC_SetAnim(Blocker, SETANIM_TORSO, BOTH_K6_S6_BL, SETANIM_AFLAG_PACE);
			break;
		case 4:
			NPC_SetAnim(Blocker, SETANIM_TORSO, BOTH_K6_S6_B_, SETANIM_AFLAG_PACE);
			break;
		case 5:
			NPC_SetAnim(Blocker, SETANIM_TORSO, BOTH_K6_S6_BR, SETANIM_AFLAG_PACE);
			break;
		}
	}
	else if (Blocker->client->ps.saberAnimLevel == SS_STAFF)
	{
		switch (anim_choice)
		{
		default:
		case 0:
			NPC_SetAnim(Blocker, SETANIM_TORSO, BOTH_K7_S7_T_, SETANIM_AFLAG_PACE);
			break;
		case 1:
			NPC_SetAnim(Blocker, SETANIM_TORSO, BOTH_K7_S7_TR, SETANIM_AFLAG_PACE);
			break;
		case 2:
			NPC_SetAnim(Blocker, SETANIM_TORSO, BOTH_K7_S7_TL, SETANIM_AFLAG_PACE);
			break;
		case 3:
			NPC_SetAnim(Blocker, SETANIM_TORSO, BOTH_K7_S7_BL, SETANIM_AFLAG_PACE);
			break;
		case 4:
			NPC_SetAnim(Blocker, SETANIM_TORSO, BOTH_K7_S7_B_, SETANIM_AFLAG_PACE);
			break;
		case 5:
			NPC_SetAnim(Blocker, SETANIM_TORSO, BOTH_K7_S7_BR, SETANIM_AFLAG_PACE);
			break;
		}
	}
	else
	{
		switch (anim_choice)
		{
		default:
		case 0:
			NPC_SetAnim(Blocker, SETANIM_TORSO, BOTH_K1_S1_T_, SETANIM_AFLAG_PACE);
			break;
		case 1:
			NPC_SetAnim(Blocker, SETANIM_TORSO, BOTH_K1_S1_TR_OLD, SETANIM_AFLAG_PACE);
			break;
		case 2:
			NPC_SetAnim(Blocker, SETANIM_TORSO, BOTH_K1_S1_TL_OLD, SETANIM_AFLAG_PACE);
			break;
		case 3:
			NPC_SetAnim(Blocker, SETANIM_TORSO, BOTH_K1_S1_BL, SETANIM_AFLAG_PACE);
			break;
		case 4:
			NPC_SetAnim(Blocker, SETANIM_TORSO, BOTH_K1_S1_B_, SETANIM_AFLAG_PACE);
			break;
		case 5:
			NPC_SetAnim(Blocker, SETANIM_TORSO, BOTH_K1_S1_BR, SETANIM_AFLAG_PACE);
			break;
		}
	}

	if (PM_SaberInMassiveBounce(Blocker->client->ps.torsoAnim))
	{
		Blocker->client->ps.saberMove = LS_NONE;
		Blocker->client->ps.saberBlocked = BLOCKED_NONE;
		Blocker->client->ps.weaponTime = Blocker->client->ps.torsoAnimTimer;
		Blocker->client->MassiveBounceAnimTime = Blocker->client->ps.torsoAnimTimer + level.time;
	}
	else
	{
		if (!in_camera)
		{
			Blocker->client->ps.saberMove = LS_READY;
		}
	}
}

// Attack stagger
void G_StaggerAttacker(gentity_t* atk)
{
	if (PM_InGetUp(&atk->client->ps) || PM_InForceGetUp(&atk->client->ps))
	{
		return;
	}

	const int anim_choice = irand(0, 7);

	if (atk->client->ps.saberAnimLevel == SS_DUAL)
	{
		switch (anim_choice)
		{
		default:
		case 0:
			NPC_SetAnim(atk, SETANIM_TORSO, BOTH_V6_BR_S6, SETANIM_AFLAG_PACE);
			break;
		case 1:
			NPC_SetAnim(atk, SETANIM_TORSO, BOTH_V6__R_S6, SETANIM_AFLAG_PACE);
			break;
		case 2:
			NPC_SetAnim(atk, SETANIM_TORSO, BOTH_V6_TR_S6, SETANIM_AFLAG_PACE);
			break;
		case 3:
			NPC_SetAnim(atk, SETANIM_TORSO, BOTH_V6_T__S6, SETANIM_AFLAG_PACE);
			break;
		case 4:
			NPC_SetAnim(atk, SETANIM_TORSO, BOTH_V6_TL_S6, SETANIM_AFLAG_PACE);
			break;
		case 5:
			NPC_SetAnim(atk, SETANIM_TORSO, BOTH_V6__L_S6, SETANIM_AFLAG_PACE);
			break;
		case 6:
			NPC_SetAnim(atk, SETANIM_TORSO, BOTH_V6_BL_S6, SETANIM_AFLAG_PACE);
			break;
		case 7:
			NPC_SetAnim(atk, SETANIM_TORSO, BOTH_V6_B__S6, SETANIM_AFLAG_PACE);
			break;
		}
	}
	else if (atk->client->ps.saberAnimLevel == SS_STAFF)
	{
		switch (anim_choice)
		{
		default:
		case 0:
			NPC_SetAnim(atk, SETANIM_TORSO, BOTH_V7_BR_S7, SETANIM_AFLAG_PACE);
			break;
		case 1:
			NPC_SetAnim(atk, SETANIM_TORSO, BOTH_V7__R_S7, SETANIM_AFLAG_PACE);
			break;
		case 2:
			NPC_SetAnim(atk, SETANIM_TORSO, BOTH_V7_TR_S7, SETANIM_AFLAG_PACE);
			break;
		case 3:
			NPC_SetAnim(atk, SETANIM_TORSO, BOTH_V7_T__S7, SETANIM_AFLAG_PACE);
			break;
		case 4:
			NPC_SetAnim(atk, SETANIM_TORSO, BOTH_V7_TL_S7, SETANIM_AFLAG_PACE);
			break;
		case 5:
			NPC_SetAnim(atk, SETANIM_TORSO, BOTH_V7__L_S7, SETANIM_AFLAG_PACE);
			break;
		case 6:
			NPC_SetAnim(atk, SETANIM_TORSO, BOTH_V7_BL_S7, SETANIM_AFLAG_PACE);
			break;
		case 7:
			NPC_SetAnim(atk, SETANIM_TORSO, BOTH_V7_B__S7, SETANIM_AFLAG_PACE);
			break;
		}
	}
	else
	{
		switch (anim_choice)
		{
		default:
		case 0:
			NPC_SetAnim(atk, SETANIM_TORSO, BOTH_V1_BR_S1, SETANIM_AFLAG_PACE);
			break;
		case 1:
			NPC_SetAnim(atk, SETANIM_TORSO, BOTH_V1__R_S1, SETANIM_AFLAG_PACE);
			break;
		case 2:
			NPC_SetAnim(atk, SETANIM_TORSO, BOTH_V1_TR_S1, SETANIM_AFLAG_PACE);
			break;
		case 3:
			NPC_SetAnim(atk, SETANIM_TORSO, BOTH_V1_T__S1, SETANIM_AFLAG_PACE);
			break;
		case 4:
			NPC_SetAnim(atk, SETANIM_TORSO, BOTH_V1_TL_S1, SETANIM_AFLAG_PACE);
			break;
		case 5:
			NPC_SetAnim(atk, SETANIM_TORSO, BOTH_V1__L_S1, SETANIM_AFLAG_PACE);
			break;
		case 6:
			NPC_SetAnim(atk, SETANIM_TORSO, BOTH_V1_BL_S1, SETANIM_AFLAG_PACE);
			break;
		case 7:
			NPC_SetAnim(atk, SETANIM_TORSO, BOTH_V1_B__S1, SETANIM_AFLAG_PACE);
			break;
		}
	}

	if (PM_SaberInMassiveBounce(atk->client->ps.torsoAnim))
	{
		atk->client->ps.saberMove = LS_NONE;
		atk->client->ps.saberBlocked = BLOCKED_NONE;
		atk->client->ps.weaponTime = atk->client->ps.torsoAnimTimer;
		atk->client->MassiveBounceAnimTime = atk->client->ps.torsoAnimTimer + level.time;
	}
	else
	{
		if (!in_camera)
		{
			atk->client->ps.saberMove = LS_READY;
		}
	}
}

// Attack bounce
void G_BounceAttacker(gentity_t* atk)
{
	if (PM_InGetUp(&atk->client->ps) || PM_InForceGetUp(&atk->client->ps))
	{
		return;
	}

	const int anim_choice = irand(0, 6);

	if (atk->client->ps.saberAnimLevel == SS_DUAL)
	{
		switch (anim_choice)
		{
		default:
		case 0:
			NPC_SetAnim(atk, SETANIM_TORSO, BOTH_B6_BL___, SETANIM_AFLAG_PACE);
			break;
		case 1:
			NPC_SetAnim(atk, SETANIM_TORSO, BOTH_B6_BR___, SETANIM_AFLAG_PACE);
			break;
		case 2:
			NPC_SetAnim(atk, SETANIM_TORSO, BOTH_B6_TL___, SETANIM_AFLAG_PACE);
			break;
		case 3:
			NPC_SetAnim(atk, SETANIM_TORSO, BOTH_B6_TR___, SETANIM_AFLAG_PACE);
			break;
		case 4:
			NPC_SetAnim(atk, SETANIM_TORSO, BOTH_B6_T____, SETANIM_AFLAG_PACE);
			break;
		case 5:
			NPC_SetAnim(atk, SETANIM_TORSO, BOTH_B6__L___, SETANIM_AFLAG_PACE);
			break;
		case 6:
			NPC_SetAnim(atk, SETANIM_TORSO, BOTH_B6__R___, SETANIM_AFLAG_PACE);
			break;
		}
	}
	else if (atk->client->ps.saberAnimLevel == SS_STAFF)
	{
		switch (anim_choice)
		{
		default:
		case 0:
			NPC_SetAnim(atk, SETANIM_TORSO, BOTH_B7_BL___, SETANIM_AFLAG_PACE);
			break;
		case 1:
			NPC_SetAnim(atk, SETANIM_TORSO, BOTH_B7_BR___, SETANIM_AFLAG_PACE);
			break;
		case 2:
			NPC_SetAnim(atk, SETANIM_TORSO, BOTH_B7_TL___, SETANIM_AFLAG_PACE);
			break;
		case 3:
			NPC_SetAnim(atk, SETANIM_TORSO, BOTH_B7_TR___, SETANIM_AFLAG_PACE);
			break;
		case 4:
			NPC_SetAnim(atk, SETANIM_TORSO, BOTH_B7_T____, SETANIM_AFLAG_PACE);
			break;
		case 5:
			NPC_SetAnim(atk, SETANIM_TORSO, BOTH_B7__L___, SETANIM_AFLAG_PACE);
			break;
		case 6:
			NPC_SetAnim(atk, SETANIM_TORSO, BOTH_B7__R___, SETANIM_AFLAG_PACE);
			break;
		}
	}
	else if (atk->client->ps.saberAnimLevel == SS_FAST)
	{
		switch (anim_choice)
		{
		default:
		case 0:
			NPC_SetAnim(atk, SETANIM_TORSO, BOTH_B1_BL___, SETANIM_AFLAG_PACE);
			break;
		case 1:
			NPC_SetAnim(atk, SETANIM_TORSO, BOTH_B1_BR___, SETANIM_AFLAG_PACE);
			break;
		case 2:
			NPC_SetAnim(atk, SETANIM_TORSO, BOTH_B1_TL___, SETANIM_AFLAG_PACE);
			break;
		case 3:
			NPC_SetAnim(atk, SETANIM_TORSO, BOTH_B1_TR___, SETANIM_AFLAG_PACE);
			break;
		case 4:
			NPC_SetAnim(atk, SETANIM_TORSO, BOTH_B1_T____, SETANIM_AFLAG_PACE);
			break;
		case 5:
			NPC_SetAnim(atk, SETANIM_TORSO, BOTH_B1__L___, SETANIM_AFLAG_PACE);
			break;
		case 6:
			NPC_SetAnim(atk, SETANIM_TORSO, BOTH_B1__R___, SETANIM_AFLAG_PACE);
			break;
		}
	}
	else if (atk->client->ps.saberAnimLevel == SS_MEDIUM)
	{
		switch (anim_choice)
		{
		default:
		case 0:
			NPC_SetAnim(atk, SETANIM_TORSO, BOTH_B2_BL___, SETANIM_AFLAG_PACE);
			break;
		case 1:
			NPC_SetAnim(atk, SETANIM_TORSO, BOTH_B2_BR___, SETANIM_AFLAG_PACE);
			break;
		case 2:
			NPC_SetAnim(atk, SETANIM_TORSO, BOTH_B2_TL___, SETANIM_AFLAG_PACE);
			break;
		case 3:
			NPC_SetAnim(atk, SETANIM_TORSO, BOTH_B2_TR___, SETANIM_AFLAG_PACE);
			break;
		case 4:
			NPC_SetAnim(atk, SETANIM_TORSO, BOTH_B2_T____, SETANIM_AFLAG_PACE);
			break;
		case 5:
			NPC_SetAnim(atk, SETANIM_TORSO, BOTH_B2__L___, SETANIM_AFLAG_PACE);
			break;
		case 6:
			NPC_SetAnim(atk, SETANIM_TORSO, BOTH_B2__R___, SETANIM_AFLAG_PACE);
			break;
		}
	}
	else if (atk->client->ps.saberAnimLevel == SS_STRONG)
	{
		switch (anim_choice)
		{
		default:
		case 0:
			NPC_SetAnim(atk, SETANIM_TORSO, BOTH_B3_BL___, SETANIM_AFLAG_PACE);
			break;
		case 1:
			NPC_SetAnim(atk, SETANIM_TORSO, BOTH_B3_BR___, SETANIM_AFLAG_PACE);
			break;
		case 2:
			NPC_SetAnim(atk, SETANIM_TORSO, BOTH_B3_TL___, SETANIM_AFLAG_PACE);
			break;
		case 3:
			NPC_SetAnim(atk, SETANIM_TORSO, BOTH_B3_TR___, SETANIM_AFLAG_PACE);
			break;
		case 4:
			NPC_SetAnim(atk, SETANIM_TORSO, BOTH_B3_T____, SETANIM_AFLAG_PACE);
			break;
		case 5:
			NPC_SetAnim(atk, SETANIM_TORSO, BOTH_B3__L___, SETANIM_AFLAG_PACE);
			break;
		case 6:
			NPC_SetAnim(atk, SETANIM_TORSO, BOTH_B3__R___, SETANIM_AFLAG_PACE);
			break;
		}
	}
	else if (atk->client->ps.saberAnimLevel == SS_DESANN)
	{
		switch (anim_choice)
		{
		default:
		case 0:
			NPC_SetAnim(atk, SETANIM_TORSO, BOTH_B4_BL___, SETANIM_AFLAG_PACE);
			break;
		case 1:
			NPC_SetAnim(atk, SETANIM_TORSO, BOTH_B4_BR___, SETANIM_AFLAG_PACE);
			break;
		case 2:
			NPC_SetAnim(atk, SETANIM_TORSO, BOTH_B4_TL___, SETANIM_AFLAG_PACE);
			break;
		case 3:
			NPC_SetAnim(atk, SETANIM_TORSO, BOTH_B4_TR___, SETANIM_AFLAG_PACE);
			break;
		case 4:
			NPC_SetAnim(atk, SETANIM_TORSO, BOTH_B4_T____, SETANIM_AFLAG_PACE);
			break;
		case 5:
			NPC_SetAnim(atk, SETANIM_TORSO, BOTH_B4__L___, SETANIM_AFLAG_PACE);
			break;
		case 6:
			NPC_SetAnim(atk, SETANIM_TORSO, BOTH_B4__R___, SETANIM_AFLAG_PACE);
			break;
		}
	}
	else if (atk->client->ps.saberAnimLevel == SS_TAVION)
	{
		switch (anim_choice)
		{
		default:
		case 0:
			NPC_SetAnim(atk, SETANIM_TORSO, BOTH_B5_BL___, SETANIM_AFLAG_PACE);
			break;
		case 1:
			NPC_SetAnim(atk, SETANIM_TORSO, BOTH_B5_BR___, SETANIM_AFLAG_PACE);
			break;
		case 2:
			NPC_SetAnim(atk, SETANIM_TORSO, BOTH_B5_TL___, SETANIM_AFLAG_PACE);
			break;
		case 3:
			NPC_SetAnim(atk, SETANIM_TORSO, BOTH_B5_TR___, SETANIM_AFLAG_PACE);
			break;
		case 4:
			NPC_SetAnim(atk, SETANIM_TORSO, BOTH_B5_T____, SETANIM_AFLAG_PACE);
			break;
		case 5:
			NPC_SetAnim(atk, SETANIM_TORSO, BOTH_B5__L___, SETANIM_AFLAG_PACE);
			break;
		case 6:
			NPC_SetAnim(atk, SETANIM_TORSO, BOTH_B5__R___, SETANIM_AFLAG_PACE);
			break;
		}
	}

	if (PM_SaberInMassiveBounce(atk->client->ps.torsoAnim))
	{
		atk->client->ps.saberMove = LS_NONE;
		atk->client->ps.saberBlocked = BLOCKED_NONE;
		atk->client->ps.weaponTime = atk->client->ps.torsoAnimTimer;
		atk->client->MassiveBounceAnimTime = atk->client->ps.torsoAnimTimer + level.time;
	}
	else
	{
		if (!in_camera)
		{
			atk->client->ps.saberMove = LS_READY;
		}
	}
}

void G_Stumble(gentity_t* hit_ent)
{
	if (PM_SaberInBashedAnim(hit_ent->client->ps.torsoAnim))
	{
		return;
	}

	if (PM_InGetUp(&hit_ent->client->ps) || PM_InForceGetUp(&hit_ent->client->ps))
	{
		return;
	}

	const int anim_choice = irand(0, 6);
	// this could possibly be based on animation done when the clash happend, but this should do for now.
	int use_anim;

	switch (anim_choice)
	{
	default:
	case 0:
		use_anim = BOTH_BASHED1;
		break;
	case 1:
		use_anim = BOTH_PAIN3;
		break;
	case 2:
		use_anim = BOTH_PAIN2;
		break;
	case 3:
		use_anim = BOTH_PAIN15;
		break;
	case 4:
		use_anim = BOTH_PAIN12;
		break;
	case 5:
		use_anim = BOTH_PAIN5;
		break;
	case 6:
		use_anim = BOTH_PAIN7;
		break;
	}

	NPC_SetAnim(hit_ent, SETANIM_TORSO, use_anim, SETANIM_AFLAG_PACE);

	if (PM_SaberInBashedAnim(hit_ent->client->ps.torsoAnim))
	{
		hit_ent->client->ps.saberMove = LS_NONE;
		hit_ent->client->ps.saberBlocked = BLOCKED_NONE;
		hit_ent->client->ps.weaponTime = hit_ent->client->ps.torsoAnimTimer;
		hit_ent->client->MassiveBounceAnimTime = hit_ent->client->ps.torsoAnimTimer + level.time;
	}
}

/*
---------------------------------------------------------
void WP_SaberDamageTrace( gentity_t *ent, int saberNum, int bladeNum )

  Constantly trace from the old blade pos to new, down the saber beam and do damage

  FIXME: if the dot product of the old muzzle dir and the new muzzle dir is < 0.75, subdivide it and do multiple traces so we don't flatten out the arc!
---------------------------------------------------------
*/
constexpr auto MAX_SABER_SWING_INC = 0.33f;

void WP_SaberDamageTrace(gentity_t* ent, int saber_num, int blade_num)
{
	vec3_t mp1;
	vec3_t mp2;
	vec3_t md1;
	vec3_t md2;
	vec3_t base_old;
	vec3_t base_new;
	vec3_t end_old;
	vec3_t end_new;
	float base_damage;
	int base_d_flags = 0;
	qboolean hit_wall = qfalse;
	qboolean broken_parry = qfalse;
	qboolean saber_in_special = pm_saber_in_special_attack(ent->client->ps.torsoAnim);

	for (int& ven : victimEntityNum)
	{
		ven = ENTITYNUM_NONE;
	}
	memset(totalDmg, 0, sizeof totalDmg);
	memset(dmgDir, 0, sizeof dmgDir);
	memset(dmgNormal, 0, sizeof dmgNormal);
	memset(dmgSpot, 0, sizeof dmgSpot);
	memset(dmgFraction, 0, sizeof dmgFraction);
	memset(hitLoc, HL_NONE, sizeof hitLoc);
	memset(hitDismemberLoc, HL_NONE, sizeof hitDismemberLoc);
	memset(hitDismember, qfalse, sizeof hitDismember);
	numVictims = 0;
	VectorClear(saberHitLocation);
	VectorClear(saberHitNormal);
	saberHitFraction = 1.0; // Closest saber hit.  The saber can do no damage past this point.
	saberHitEntity = ENTITYNUM_NONE;
	sabersCrossed = -1;

	if (!ent->client)
	{
		return;
	}

	if (!ent->s.number)
	{
		ent->client->ps.saberEventFlags &= ~SEF_EVENTS;
	}

	if (ent->client->ps.saber[saber_num].blade[blade_num].length <= 1) //can get down to 1 when in a wall
	{
		//saber is not on
		return;
	}

	if (VectorCompare(ent->client->renderInfo.muzzlePointOld, vec3_origin) || VectorCompare(
		ent->client->renderInfo.muzzleDirOld, vec3_origin))
	{
		//just started up the saber?
		return;
	}

	int saber_contents = 0;

	if (!(ent->client->ps.saber[saber_num].saberFlags & SFL_ON_IN_WATER))
	{
		//saber can't stay on underwater
		saber_contents = gi.pointcontents(ent->client->renderInfo.muzzlePoint, ent->client->ps.saberEntityNum);
	}
	if (saber_contents & CONTENTS_WATER ||
		saber_contents & CONTENTS_SLIME ||
		saber_contents & CONTENTS_LAVA)
	{
		//um... turn off?  Or just set length to 1?
		ent->client->ps.saber[saber_num].blade[blade_num].active = qfalse;
		return;
	}
	if (!g_saberNoEffects && gi.WE_IsOutside(ent->client->renderInfo.muzzlePoint))
	{
		float chance_of_fizz = gi.WE_GetChanceOfSaberFizz();
		if (chance_of_fizz > 0 && Q_flrand(0.0f, 1.0f) < chance_of_fizz)
		{
			vec3_t end;
			VectorMA(ent->client->ps.saber[saber_num].blade[blade_num].muzzlePoint,
				ent->client->ps.saber[saber_num].blade[blade_num].length * Q_flrand(0, 1),
				ent->client->ps.saber[saber_num].blade[blade_num].muzzleDir, end);
			G_PlayEffect("saber/fizz", end);
		}
	}

	int attacker_power_level = 0;
	// ReSharper disable once CppEntityAssignedButNoRead
	int style_power_modifier = 0;
	int style_power_level = PM_PowerLevelForSaberAnim(&ent->client->ps, saber_num);

	switch (style_power_level) // check what the saber style is and set power modifier accordingly
	{
	case 1:
		style_power_modifier = -2;
		break;
	case 2:
		style_power_modifier = -1;
		break;
	case 3:
		style_power_modifier = 0;
		break;
	case 4:
		style_power_modifier = 1;
		break;
	case 5: //special attacks count as this in bg_panimate
		style_power_modifier = 2;
		break;
	default:;
	}

	if (ent->client->NPC_class == CLASS_SABER_DROID)
	{
		attacker_power_level = 3 * SaberDroid_PowerLevelForSaberAnim(ent);
	}
	else if (!ent->s.number && ent->client->ps.forcePowersActive & 1 << FP_SPEED)
	{
		//min power bonus is +1, max bonus is +3
		attacker_power_level += 1 * ent->client->ps.forcePowerLevel[FP_SPEED];
	}
	else
	{
		attacker_power_level = PM_PowerLevelForSaberAnim(&ent->client->ps, saber_num);
	}

	if (attacker_power_level)
	{
		if (ent->client->ps.forceRageRecoveryTime > level.time)
		{
			attacker_power_level -= 2;
		}
		else if (ent->client->ps.forcePowersActive & 1 << FP_RAGE)
		{
			//min power bonus is +2, max is +4, slightly more than Speed 3
			attacker_power_level += ent->client->ps.forcePowerLevel[FP_RAGE] + 1;
		}
	}

	if (ent->client->ps.saberInFlight)
	{
		//flying sabers are much more deadly//unless you're dead
		if (ent->health <= 0 && g_saberRealisticCombat->integer < 2)
		{
			//so enemies don't keep trying to block it
			return;
		}
		//or unless returning
		if (ent->client->ps.saberEntityState == SES_RETURNING
			&& !(ent->client->ps.saber[0].saberFlags & SFL_RETURN_DAMAGE))
		{
			//special case, since we're returning, chances are if we hit something
			base_damage = 0.1f;
		}
		else
		{
			if (!ent->s.number)
			{
				//cheat for player
				base_damage = 5.0f;
			}
			else
			{
				base_damage = 2.5f;
			}
		}
		//Use old to current since can't predict it
		VectorCopy(ent->client->ps.saber[saber_num].blade[blade_num].muzzlePointOld, mp1);
		VectorCopy(ent->client->ps.saber[saber_num].blade[blade_num].muzzleDirOld, md1);
		VectorCopy(ent->client->ps.saber[saber_num].blade[blade_num].muzzlePoint, mp2);
		VectorCopy(ent->client->ps.saber[saber_num].blade[blade_num].muzzleDir, md2);
	}
	else
	{
		if (ent->client->ps.torsoAnim == BOTH_A7_HILT ||
			ent->client->ps.torsoAnim == BOTH_SMACK_L ||
			ent->client->ps.torsoAnim == BOTH_SMACK_R)
		{
			return;
		}
		if (g_in_cinematic_saber_anim(ent))
		{
			base_damage = 0.1f;
		}
		else if (ent->client->ps.saberMove == LS_READY && !pm_saber_in_special_attack(ent->client->ps.torsoAnim))
		{
			//just do effects
			if (g_saberRealisticCombat->integer < 2)
			{
				//don't kill with this hit
				//baseDFlags = DAMAGE_NO_KILL;
			}
			if (!WP_SaberBladeUseSecondBladeStyle(&ent->client->ps.saber[saber_num], blade_num)
				&& ent->client->ps.saber[saber_num].saberFlags2 & SFL2_NO_IDLE_EFFECT
				|| WP_SaberBladeUseSecondBladeStyle(&ent->client->ps.saber[saber_num], blade_num)
				&& ent->client->ps.saber[saber_num].saberFlags2 & SFL2_NO_IDLE_EFFECT2)
			{
				//do nothing at all when idle
				return;
			}
			base_damage = 0;
		}
		else if (ent->client->ps.saberLockTime > level.time)
		{
			//just do effects
			if (!WP_SaberBladeUseSecondBladeStyle(&ent->client->ps.saber[saber_num], blade_num)
				&& ent->client->ps.saber[saber_num].saberFlags2 & SFL2_NO_IDLE_EFFECT
				|| WP_SaberBladeUseSecondBladeStyle(&ent->client->ps.saber[saber_num], blade_num)
				&& ent->client->ps.saber[saber_num].saberFlags2 & SFL2_NO_IDLE_EFFECT2)
			{
				//do nothing at all when idle
				return;
			}
			base_damage = 0;
		}
		else if (!WP_SaberBladeUseSecondBladeStyle(&ent->client->ps.saber[saber_num], blade_num)
			&& ent->client->ps.saber[saber_num].damageScale <= 0.0f
			&& ent->client->ps.saber[saber_num].knockbackScale <= 0.0f)
		{
			//this blade does no damage and no knockback (only for blocking?)
			base_damage = 0;
		}
		else if (WP_SaberBladeUseSecondBladeStyle(&ent->client->ps.saber[saber_num], blade_num)
			&& ent->client->ps.saber[saber_num].damageScale2 <= 0.0f
			&& ent->client->ps.saber[saber_num].knockbackScale2 <= 0.0f)
		{
			//this blade does no damage and no knockback (only for blocking?)
			base_damage = 0;
		}
		else if (ent->client->ps.saberBlocked > BLOCKED_NONE
			|| !PM_SaberInAttack(ent->client->ps.saberMove)
			&& !pm_saber_in_special_attack(ent->client->ps.torsoAnim)
			&& !PM_SaberInTransitionAny(ent->client->ps.saberMove))
		{
			//don't do damage if parrying/reflecting/bouncing/deflecting or not actually attacking or in a transition to/from/between attacks
			if (!WP_SaberBladeUseSecondBladeStyle(&ent->client->ps.saber[saber_num], blade_num)
				&& ent->client->ps.saber[saber_num].saberFlags2 & SFL2_NO_IDLE_EFFECT
				|| WP_SaberBladeUseSecondBladeStyle(&ent->client->ps.saber[saber_num], blade_num)
				&& ent->client->ps.saber[saber_num].saberFlags2 & SFL2_NO_IDLE_EFFECT2)
			{
				//do nothing at all when idle
				return;
			}
			base_damage = 0;
		}
		else
		{
			//okay, in a saberMove that does damage//make sure we're in the right anim
			if (!pm_saber_in_special_attack(ent->client->ps.torsoAnim)
				&& !PM_InAnimForSaberMove(ent->client->ps.torsoAnim, ent->client->ps.saberMove))
			{
				//forced into some other animation somehow, like a pain or death?
				if (!WP_SaberBladeUseSecondBladeStyle(&ent->client->ps.saber[saber_num], blade_num)
					&& ent->client->ps.saber[saber_num].saberFlags2 & SFL2_NO_IDLE_EFFECT
					|| WP_SaberBladeUseSecondBladeStyle(&ent->client->ps.saber[saber_num], blade_num)
					&& ent->client->ps.saber[saber_num].saberFlags2 & SFL2_NO_IDLE_EFFECT2)
				{
					//do nothing at all when idle
					return;
				}
				base_damage = 0;
			}
			else if (ent->client->ps.weaponstate == WEAPON_FIRING
				&& ent->client->ps.saberBlocked == BLOCKED_NONE
				&& (PM_SaberInAttack(ent->client->ps.saberMove)
					|| pm_saber_in_special_attack(ent->client->ps.torsoAnim)
					|| PM_SpinningSaberAnim(ent->client->ps.torsoAnim)
					|| attacker_power_level > FORCE_LEVEL_2
					|| WP_SaberBladeDoTransitionDamage(&ent->client->ps.saber[saber_num], blade_num)))
			{
				//normal attack swing swinging/spinning (or if using strong set), do normal damage
				if (g_saberRealisticCombat->integer > 1)
				{
					switch (attacker_power_level)
					{
					default:
					case FORCE_LEVEL_5:
						base_damage = 7.5f;
						break;
					case FORCE_LEVEL_4: //Staff, medium, duals all do same damage
					case FORCE_LEVEL_3:
						base_damage = 5.0f;
						break;
					case FORCE_LEVEL_2:
						base_damage = 2.0f;
						break;
					case FORCE_LEVEL_1: //Fast damage is so low...
					case FORCE_LEVEL_0:
						base_damage = 1.5f;
						break;
					}
				}
				else
				{
					if (g_spskill->integer > 0
						&& ent->s.number < MAX_CLIENTS
						&& (ent->client->ps.torsoAnim == BOTH_ROLL_STAB
							|| ent->client->ps.torsoAnim == BOTH_SPINATTACK6
							|| ent->client->ps.torsoAnim == BOTH_SPINATTACK7
							|| ent->client->ps.torsoAnim == BOTH_SPINATTACKGRIEVOUS
							|| ent->client->ps.torsoAnim == BOTH_LUNGE2_B__T_))
					{
						//*sigh*, these anim do less damage since they're so easy to do
						base_damage = 1.5f;
					}
					else
					{
						switch (attacker_power_level)
						{
						default:
						case FORCE_LEVEL_5:
							base_damage = 2.5f * static_cast<float>(attacker_power_level);
						case FORCE_LEVEL_4: //Staff, medium, duals all do same damage
						case FORCE_LEVEL_3:
							base_damage = 2.0f * static_cast<float>(attacker_power_level);
							break;
						case FORCE_LEVEL_2:
							base_damage = 1.5f * static_cast<float>(attacker_power_level);
							break;
						case FORCE_LEVEL_1: // Fast damage is so low...
						case FORCE_LEVEL_0:
							base_damage = 1.0f * static_cast<float>(attacker_power_level);
							break;
						}
					}
				}
			}
			else
			{
				//saber is transitioning, defending or idle, don't do as much damage
				if (g_timescale->value < 1.0)
				{
					//in slow mo or force speed, we need to do damage during the transitions
					if (g_saberRealisticCombat->integer > 1)
					{
						switch (attacker_power_level)
						{
						case FORCE_LEVEL_5:
							base_damage = 7.5f;
							break;
						case FORCE_LEVEL_4:
						case FORCE_LEVEL_3:
							base_damage = 5.0f;
							break;
						case FORCE_LEVEL_2:
							base_damage = 2.0f;
							break;
						default:
						case FORCE_LEVEL_1:
						case FORCE_LEVEL_0:
							base_damage = 1.5f;
							break;
						}
					}
					else
					{
						base_damage = 1.5f * static_cast<float>(attacker_power_level);
					}
				}
				else
				{
					//I have to do *some* damage in transitions or else you feel like a total gimp
					base_damage = 0.1f;
				}
			}
		}
		//prediction was causing gaps in swing (G2 problem) so *don't* predict
		if (ent->client->ps.saberDamageDebounceTime > level.time)
		{
			//really only used when a saber attack start anim starts, not actually for stopping damage
			//we just want to not use the old position to trace the attack from...
			VectorCopy(ent->client->ps.saber[saber_num].blade[blade_num].muzzlePoint,
				ent->client->ps.saber[saber_num].blade[blade_num].muzzlePointOld);
			VectorCopy(ent->client->ps.saber[saber_num].blade[blade_num].muzzleDir,
				ent->client->ps.saber[saber_num].blade[blade_num].muzzleDirOld);
		}
		//do the damage trace from the last position...
		VectorCopy(ent->client->ps.saber[saber_num].blade[blade_num].muzzlePointOld, mp1);
		VectorCopy(ent->client->ps.saber[saber_num].blade[blade_num].muzzleDirOld, md1);
		//...to the current one.
		VectorCopy(ent->client->ps.saber[saber_num].blade[blade_num].muzzlePoint, mp2);
		VectorCopy(ent->client->ps.saber[saber_num].blade[blade_num].muzzleDir, md2);

		//see if anyone is so close that they're within the dist from my origin to the start of the saber
		if (ent->health > 0
			&& !ent->client->ps.saberLockTime
			&& saber_num == 0
			&& blade_num == 0
			&& !g_in_cinematic_saber_anim(ent))
		{
			//only do once - for first blade
			trace_t trace;

			gi.trace(&trace, ent->currentOrigin, vec3_origin, vec3_origin, mp1, ent->s.number, MASK_SHOT & ~(CONTENTS_CORPSE | CONTENTS_ITEM), static_cast<EG2_Collision>(0), 0);

			if (trace.entityNum < ENTITYNUM_WORLD
				&& (trace.entityNum > 0
					|| ent->client->NPC_class == CLASS_DESANN
					|| ent->client->NPC_class == CLASS_VADER
					|| ent->client->NPC_class == CLASS_SITHLORD
					|| ent->client->NPC_class == CLASS_LUKE))
			{
				//a valid ent
				gentity_t* traceEnt = &g_entities[trace.entityNum];

				if (traceEnt
					&& traceEnt->client
					&& traceEnt->client->NPC_class != CLASS_RANCOR
					&& traceEnt->client->NPC_class != CLASS_ATST
					&& traceEnt->client->NPC_class != CLASS_WAMPA
					&& traceEnt->client->NPC_class != CLASS_SAND_CREATURE
					&& traceEnt->health > 0
					&& traceEnt->client->playerTeam != ent->client->playerTeam
					&& !(traceEnt->client->buttons & BUTTON_BLOCK)
					&& !PM_SuperBreakLoseAnim(traceEnt->client->ps.legsAnim)
					&& !PM_SuperBreakLoseAnim(traceEnt->client->ps.torsoAnim)
					&& !PM_SuperBreakWinAnim(traceEnt->client->ps.legsAnim)
					&& !PM_SuperBreakWinAnim(traceEnt->client->ps.torsoAnim)
					&& !PM_InKnockDown(&traceEnt->client->ps)
					&& !PM_LockedAnim(traceEnt->client->ps.legsAnim)
					&& !PM_LockedAnim(traceEnt->client->ps.torsoAnim)
					&& !g_in_cinematic_saber_anim(traceEnt))
				{
					//enemy client, push them away
					if (!traceEnt->client->ps.saberLockTime
						&& !traceEnt->message
						&& !(traceEnt->flags & FL_NO_KNOCKBACK)
						&& (!traceEnt->NPC || traceEnt->NPC->jumpState != JS_JUMPING))
					{
						//don't push people in saberlock or with security keys or who are in BS_JUMP
						vec3_t hitDir;
						VectorSubtract(trace.endpos, ent->currentOrigin, hitDir);
						float totalDist = Distance(mp1, ent->currentOrigin);
						float knockback = (totalDist - VectorNormalize(hitDir)) / totalDist * 200.0f;

						if (ent->s.number >= MAX_CLIENTS && !G_ControlledByPlayer(ent))
						{
							NPC_SetAnim(ent, SETANIM_TORSO, BOTH_A7_SLAP_L, SETANIM_AFLAG_PACE);
							G_Stagger(traceEnt);
							G_SoundOnEnt(traceEnt, CHAN_WEAPON, "sound/weapons/melee/punch1.wav");
						}

						hitDir[2] = 0;
						VectorMA(traceEnt->client->ps.velocity, knockback, hitDir, traceEnt->client->ps.velocity);
						traceEnt->client->ps.pm_time = 200;
						traceEnt->client->ps.pm_flags |= PMF_TIME_NOFRICTION;
						if (d_combatinfo->integer || g_DebugSaberCombat->integer)
						{
							gi.Printf("%s pushing away %s at %s\n", ent->NPC_type, traceEnt->NPC_type,
								vtos(traceEnt->client->ps.velocity));
						}
					}
				}
			}
		}
	}
	//the thicker the blade, the more damage... the thinner, the less damage
	base_damage *= ent->client->ps.saber[saber_num].blade[blade_num].radius / SABER_RADIUS_STANDARD;

	if (g_saberRealisticCombat->integer > 1)
	{//always do damage, and lots of it
		if (g_saberRealisticCombat->integer > 2)
		{//always do damage, and lots of it
			base_damage = 25.0f;
		}
		else if (base_damage > 0.1f)
		{//only do super damage if we would have done damage according to normal rules
			base_damage = 25.0f;
		}
	}
	else if ((!ent->s.number && ent->client->ps.forcePowersActive & 1 << FP_SPEED || ent->client->ps.forcePowersActive & 1 << FP_RAGE)
		&& g_timescale->value < 1.0f)
	{
		base_damage *= 1.0f - g_timescale->value;
	}
	if (base_damage > 0.1f)
	{
		if (ent->client->ps.forcePowersActive & 1 << FP_RAGE)
		{//add some damage if raged
			base_damage += ent->client->ps.forcePowerLevel[FP_RAGE] * 5.0f;
		}
		else if (ent->client->ps.forceRageRecoveryTime)
		{//halve it if recovering
			base_damage *= 0.5f;
		}
	}
	// Get the old state of the blade
	VectorCopy(mp1, base_old);
	VectorMA(base_old, ent->client->ps.saber[saber_num].blade[blade_num].length, md1, end_old);
	// Get the future state of the blade
	VectorCopy(mp2, base_new);
	VectorMA(base_new, ent->client->ps.saber[saber_num].blade[blade_num].length, md2, end_new);

	sabersCrossed = -1;

	if (VectorCompare2(base_old, base_new) && VectorCompare2(end_old, end_new))
	{
		hit_wall = wp_saber_damage_for_trace(ent->s.number, mp2, end_new, base_damage * 4, md2, qfalse,
			attacker_power_level,
			ent->client->ps.saber[saber_num].type, qfalse, saber_num, blade_num);
	}
	else
	{
		float tip_dmg_mod = 1.0f;
		vec3_t base_diff;
		float aveLength, step = 8, stepsize = 8;
		vec3_t ma1, ma2, md2ang, curBase2;
		int xx;
		//do the trace at the base first
		hit_wall = wp_saber_damage_for_trace(ent->s.number, base_old, base_new, base_damage, md2, qfalse,
			attacker_power_level,
			ent->client->ps.saber[saber_num].type, qtrue, saber_num, blade_num);

		//if hit a saber, shorten rest of traces to match
		if (saberHitFraction < 1.0)
		{
			//adjust muzzleDir...
			vectoangles(md1, ma1);
			vectoangles(md2, ma2);
			for (xx = 0; xx < 3; xx++)
			{
				md2ang[xx] = LerpAngle(ma1[xx], ma2[xx], saberHitFraction);
			}
			AngleVectors(md2ang, md2, nullptr, nullptr);
			//shorten the base pos
			VectorSubtract(mp2, mp1, base_diff);
			VectorMA(mp1, saberHitFraction, base_diff, base_new);
			VectorMA(base_new, ent->client->ps.saber[saber_num].blade[blade_num].length, md2, end_new);
		}

		//If the angle diff in the blade is high, need to do it in chunks of 33 to avoid flattening of the arc
		float dirInc, curDirFrac;
		if (PM_SaberInAttack(ent->client->ps.saberMove)
			|| pm_saber_in_special_attack(ent->client->ps.torsoAnim)
			|| PM_SpinningSaberAnim(ent->client->ps.torsoAnim)
			|| PM_InSpecialJump(ent->client->ps.torsoAnim)
			|| g_timescale->value < 1.0f && PM_SaberInTransitionAny(ent->client->ps.saberMove))
		{
			curDirFrac = DotProduct(md1, md2);
		}
		else
		{
			curDirFrac = 1.0f;
		}
		//if saber spun at least 180 degrees since last damage trace, this is not reliable...!
		if (fabs(curDirFrac) < 1.0f - MAX_SABER_SWING_INC)
		{
			//the saber blade spun more than 33 degrees since the last damage trace
			curDirFrac = dirInc = 1.0f / ((1.0f - curDirFrac) / MAX_SABER_SWING_INC);
		}
		else
		{
			curDirFrac = 1.0f;
			dirInc = 0.0f;
		}

		qboolean hit_saber = qfalse;

		vectoangles(md1, ma1);
		vectoangles(md2, ma2);

		vec3_t curMD2;
		VectorCopy(md1, curMD2);
		VectorCopy(base_old, curBase2);

		while (true)
		{
			vec3_t cur_md1;
			vec3_t curBase1;
			VectorCopy(curMD2, cur_md1);
			VectorCopy(curBase2, curBase1);
			if (curDirFrac >= 1.0f)
			{
				VectorCopy(md2, curMD2);
				VectorCopy(base_new, curBase2);
			}
			else
			{
				for (xx = 0; xx < 3; xx++)
				{
					md2ang[xx] = LerpAngle(ma1[xx], ma2[xx], curDirFrac);
				}
				AngleVectors(md2ang, curMD2, nullptr, nullptr);
				VectorSubtract(base_new, base_old, base_diff);
				VectorMA(base_old, curDirFrac, base_diff, curBase2);
			}
			// Move up the blade in intervals of stepsize
			for (step = stepsize; step < ent->client->ps.saber[saber_num].blade[blade_num].length && step < ent->client->
				ps.saber[saber_num].blade[blade_num].lengthOld; step += 12)
			{
				vec3_t blade_point_new;
				vec3_t blade_point_old;
				VectorMA(curBase1, step, cur_md1, blade_point_old);
				VectorMA(curBase2, step, curMD2, blade_point_new);
				if (wp_saber_damage_for_trace(ent->s.number, blade_point_old, blade_point_new, base_damage, curMD2,
					qfalse,
					attacker_power_level, ent->client->ps.saber[saber_num].type, qtrue, saber_num,
					blade_num))
				{
					hit_wall = qtrue;
				}

				//if hit a saber, shorten rest of traces to match
				if (saberHitFraction < 1.0)
				{
					//adjust muzzle endpoint
					VectorSubtract(mp2, mp1, base_diff);
					VectorMA(mp1, saberHitFraction, base_diff, base_new);
					VectorMA(base_new, ent->client->ps.saber[saber_num].blade[blade_num].length, curMD2, end_new);
					//adjust muzzleDir...
					vec3_t curMA1, curMA2;
					vectoangles(cur_md1, curMA1);
					vectoangles(curMD2, curMA2);
					for (xx = 0; xx < 3; xx++)
					{
						md2ang[xx] = LerpAngle(curMA1[xx], curMA2[xx], saberHitFraction);
					}
					AngleVectors(md2ang, curMD2, nullptr, nullptr);
					hit_saber = qtrue;
				}
				if (hit_wall)
				{
					break;
				}
			}
			if (hit_wall || hit_saber)
			{
				break;
			}
			if (curDirFrac >= 1.0f)
			{
				break;
			}
			curDirFrac += dirInc;
			if (curDirFrac >= 1.0f)
			{
				curDirFrac = 1.0f;
			}
		}

		//do the trace at the end last
		aveLength = (ent->client->ps.saber[saber_num].blade[blade_num].lengthOld + ent->client->ps.saber[saber_num].blade[
			blade_num].length) / 2;
		if (step > aveLength)
		{
			//less dmg if the last interval was not stepsize
			tip_dmg_mod = (stepsize - (step - aveLength)) / stepsize;
		}
		//since this is the tip, we do not extrapolate the extra 16
		if (wp_saber_damage_for_trace(ent->s.number, end_old, end_new, tip_dmg_mod * base_damage, md2, qfalse,
			attacker_power_level, ent->client->ps.saber[saber_num].type, qfalse, saber_num,
			blade_num))
		{
			hit_wall = qtrue;
		}
	}

	if ((saberHitFraction < 1.0f || sabersCrossed >= 0 && sabersCrossed <= 32.0f) && (ent->client->ps.weaponstate ==
		WEAPON_FIRING || ent->client->ps.saberInFlight || g_in_cinematic_saber_anim(ent)))
	{
		// The saber (in-hand) hit another saber, mano.
		qboolean inFlightSaberBlocked = qfalse;
		qboolean collisionResolved = qfalse;
		qboolean deflected = qfalse;
		gentity_t* hitEnt = &g_entities[saberHitEntity];
		gentity_t* hitOwner = nullptr;
		int BlockerPowerLevel = FORCE_LEVEL_0;

		if (hitEnt)
		{
			hitOwner = hitEnt->owner;
		}
		if (hitOwner && hitOwner->client)
		{
			BlockerPowerLevel = 2 * hitOwner->client->ps.forcePowerLevel[FP_SABER_DEFENSE] + PM_PowerLevelForSaberAnim(
				&hitOwner->client->ps);
		}

		if (ent->client->ps.saberInFlight && saber_num == 0 &&
			ent->client->ps.saber[saber_num].blade[blade_num].active &&
			ent->client->ps.saberEntityNum != ENTITYNUM_NONE &&
			ent->client->ps.saberEntityState != SES_RETURNING)
		{
			//saber was blocked, return it
			inFlightSaberBlocked = qtrue;
		}

		//Check deflections and broken parries
		if (hitOwner && hitOwner->health > 0 && ent->health > 0 //both are alive
			&& !inFlightSaberBlocked
			&& hitOwner->client
			&& !hitOwner->client->ps.saberInFlight
			&& !ent->client->ps.saberInFlight //both have sabers in-hand
			&& ent->client->ps.saberBlocked != BLOCKED_PARRY_BROKEN
			&& ent->client->ps.saberLockTime < level.time
			&& hitOwner->client->ps.saberLockTime < level.time)
		{
			//2 in-hand sabers hit
			if (base_damage)
			{
				//there is damage involved, not just effects
				qboolean entAttacking = qfalse;
				qboolean hitOwnerAttacking = qfalse;
				qboolean entDefending = qfalse;
				qboolean hitOwnerDefending = qfalse;
				qboolean forceLock = qfalse;
				qboolean atkfake = ent->client->ps.userInt3 & 1 << FLAG_ATTACKFAKE ? qtrue : qfalse;
				qboolean otherfake = hitOwner->client->ps.userInt3 & 1 << FLAG_ATTACKFAKE ? qtrue : qfalse;

				if (ent->client->NPC_class == CLASS_KYLE && ent->spawnflags & 1 && hitOwner->s.number < MAX_CLIENTS
					|| hitOwner->client->NPC_class == CLASS_KYLE && hitOwner->spawnflags & 1 && ent->s.number <
					MAX_CLIENTS)
				{
					//Player vs. Kyle Boss == lots of saberlocks
					if (!Q_irand(0, 2))
					{
						forceLock = qtrue;
					}
				}

				if (PM_SaberInAttack(ent->client->ps.saberMove) ||
					PM_SaberInDamageMove(ent->client->ps.saberMove) ||
					pm_saber_in_special_attack(ent->client->ps.torsoAnim) ||
					PM_SaberDoDamageAnim(ent->client->ps.torsoAnim))
				{
					entAttacking = qtrue;
				}

				if (PM_SaberInParry(ent->client->ps.saberMove)
					|| ent->client->ps.ManualBlockingFlags & 1 << HOLDINGBLOCK
					|| ent->client->ps.ManualBlockingFlags & 1 << HOLDINGBLOCKANDATTACK
					|| ent->client->ps.ManualBlockingFlags & 1 << PERFECTBLOCKING
					|| g_saberAutoBlocking->integer && ent->NPC && !G_ControlledByPlayer(ent)
					|| ent->client->ps.ManualBlockingFlags & 1 << MBF_NPCBLOCKING && ent->NPC && !G_ControlledByPlayer(ent)
					|| ent->client->ps.saberBlockingTime > level.time
					|| ent->client->ps.saberMove == LS_READY)
				{
					entDefending = qtrue;
				}

				if (ent->client->ps.torsoAnim == BOTH_A1_SPECIAL
					|| ent->client->ps.torsoAnim == BOTH_A2_SPECIAL
					|| ent->client->ps.torsoAnim == BOTH_A3_SPECIAL
					|| ent->client->ps.torsoAnim == BOTH_A4_SPECIAL
					|| ent->client->ps.torsoAnim == BOTH_A5_SPECIAL
					|| ent->client->ps.torsoAnim == BOTH_A7_SOULCAL
					|| ent->client->ps.torsoAnim == BOTH_YODA_SPECIAL
					|| ent->client->ps.torsoAnim == BOTH_GRIEVOUS_SPIN
					|| ent->client->ps.torsoAnim == BOTH_A6_SABERPROTECT
					|| ent->client->ps.torsoAnim == BOTH_GRIEVOUS_PROTECT)
				{
					//parry/block/break-parry bonus for single-style kata moves
					attacker_power_level++;
				}
				if (entAttacking)
				{
					//add twoHanded bonus and breakParryBonus to AttackerPowerLevel here
					if (!WP_SaberBladeUseSecondBladeStyle(&ent->client->ps.saber[saber_num], blade_num))
					{
						attacker_power_level += ent->client->ps.saber[saber_num].breakParryBonus;
					}
					else if (WP_SaberBladeUseSecondBladeStyle(&ent->client->ps.saber[saber_num], blade_num))
					{
						attacker_power_level += ent->client->ps.saber[saber_num].breakParryBonus2;
					}
				}
				else if (entDefending)
				{
					//add twoHanded bonus and dualSaber bonus and parryBonus to AttackerPowerLevel here
					attacker_power_level += Q_irand(ent->client->ps.saber[saber_num].parryBonus,
						ent->client->ps.forcePowerLevel[FP_SABER_DEFENSE]);
				}

				if (PM_SaberInAttack(hitOwner->client->ps.saberMove) ||
					PM_SaberInDamageMove(hitOwner->client->ps.saberMove) ||
					pm_saber_in_special_attack(hitOwner->client->ps.torsoAnim) ||
					PM_SaberDoDamageAnim(hitOwner->client->ps.torsoAnim))
				{
					hitOwnerAttacking = qtrue;
				}

				if (PM_SaberInParry(hitOwner->client->ps.saberMove)
					|| hitOwner->client->ps.ManualBlockingFlags & 1 << HOLDINGBLOCK
					|| hitOwner->client->ps.ManualBlockingFlags & 1 << HOLDINGBLOCKANDATTACK
					|| hitOwner->client->ps.ManualBlockingFlags & 1 << PERFECTBLOCKING
					|| g_saberAutoBlocking->integer && hitOwner->NPC && !G_ControlledByPlayer(hitOwner)
					|| hitOwner->client->ps.ManualBlockingFlags & 1 << MBF_NPCBLOCKING && hitOwner->NPC && !G_ControlledByPlayer(hitOwner)
					|| hitOwner->client->ps.saberBlockingTime > level.time
					|| hitOwner->client->ps.saberMove == LS_READY)
				{
					hitOwnerDefending = qtrue;
				}

				if (hitOwner->client->ps.torsoAnim == BOTH_A1_SPECIAL
					|| hitOwner->client->ps.torsoAnim == BOTH_A2_SPECIAL
					|| hitOwner->client->ps.torsoAnim == BOTH_A3_SPECIAL
					|| hitOwner->client->ps.torsoAnim == BOTH_A4_SPECIAL
					|| hitOwner->client->ps.torsoAnim == BOTH_A5_SPECIAL
					|| hitOwner->client->ps.torsoAnim == BOTH_A7_SOULCAL
					|| hitOwner->client->ps.torsoAnim == BOTH_YODA_SPECIAL
					|| hitOwner->client->ps.torsoAnim == BOTH_GRIEVOUS_SPIN
					|| hitOwner->client->ps.torsoAnim == BOTH_A6_SABERPROTECT
					|| hitOwner->client->ps.torsoAnim == BOTH_GRIEVOUS_PROTECT)
				{
					//parry/block/break-parry bonus for single-style kata moves
					BlockerPowerLevel++;
				}
				if (hitOwnerAttacking)
				{
					//add twoHanded bonus and breakParryBonus to AttackerPowerLevel here
					BlockerPowerLevel += hitOwner->client->ps.saber[0].breakParryBonus;

					if (hitOwner->client->ps.dualSabers && Q_irand(0, 1))
					{
						// assumes both sabers are hitting at same time...?
						BlockerPowerLevel += 1 + hitOwner->client->ps.saber[1].breakParryBonus;
					}
				}
				else if (hitOwnerDefending)
				{
					//add twoHanded bonus and dualSaber bonus and parryBonus to AttackerPowerLevel here
					BlockerPowerLevel += Q_irand(hitOwner->client->ps.saber[saber_num].parryBonus,
						hitOwner->client->ps.forcePowerLevel[FP_SABER_DEFENSE]);

					if (hitOwner->client->ps.dualSabers && Q_irand(0, 1))
					{
						//assumes both sabers are defending at same time...?
						BlockerPowerLevel += 1 + Q_irand(hitOwner->client->ps.saber[saber_num].parryBonus,
							hitOwner->client->ps.forcePowerLevel[FP_SABER_DEFENSE]);
					}
				}

				if (PM_SuperBreakLoseAnim(ent->client->ps.torsoAnim)
					|| PM_SuperBreakWinAnim(ent->client->ps.torsoAnim)
					|| PM_SuperBreakLoseAnim(hitOwner->client->ps.torsoAnim)
					|| PM_SuperBreakWinAnim(hitOwner->client->ps.torsoAnim))
				{
					//don't mess with this
					collisionResolved = qtrue;
				}
				else if (entAttacking && hitOwnerAttacking && attacker_power_level == BlockerPowerLevel
					&& g_saberLockRandomNess->integer > 1 && WP_SabersCheckLock(ent, hitOwner))
				{
					collisionResolved = qtrue;
				}
				else if (hitOwnerAttacking && entDefending && BlockerPowerLevel > attacker_power_level
					&& g_saberLockRandomNess->integer > 0 && WP_SabersCheckLock(ent, hitOwner))
				{
					collisionResolved = qtrue;
				}
				else if (entAttacking && hitOwnerDefending && attacker_power_level > BlockerPowerLevel
					&& g_saberLockRandomNess->integer > 0 && WP_SabersCheckLock(ent, hitOwner))
				{
					collisionResolved = qtrue;
				}
				else if (atkfake && !otherfake && WP_SabersCheckLock(ent, hitOwner))
				{
					collisionResolved = qtrue;
				}
				else if (otherfake && !atkfake && WP_SabersCheckLock(hitOwner, ent))
				{
					collisionResolved = qtrue;
				}
				else if (otherfake && atkfake && WP_SabersCheckLock(hitOwner, ent))
				{
					collisionResolved = qtrue;
				}
				else if (forceLock && WP_SabersCheckLock(ent, hitOwner))
				{
					collisionResolved = qtrue;
				}
				else if (forceLock && WP_SabersCheckLock(hitOwner, ent))
				{
					collisionResolved = qtrue;
				}
				else if (entAttacking && hitOwnerDefending)
				{
					if (saberHitFraction < 1.0f)
					{//an actual collision
						if (d_blockinfo->integer || g_DebugSaberCombat->integer)
						{
							gi.Printf(S_COLOR_YELLOW"Blocking starts here 1\n");
						}

						//make me parry	-(Im the blocker)
						sab_beh_block_vs_attack(hitOwner, ent, saber_num, blade_num, saberHitLocation);

						//make me bounce -(Im the attacker)
						sab_beh_attack_vs_block(ent, hitOwner, saber_num, blade_num, saberHitLocation);

						collisionResolved = qtrue;
					}
				}
				else
				{
					//some other kind of in-hand saber collision bounce the attacker off it
				}
			}
		}
		else
		{
			//some kind of in-flight collision
		}

		if (saberHitFraction < 1.0f)
		{
			auto active_defenseentagain = static_cast<qboolean>
				(ent->client->ps.ManualBlockingFlags & 1 << MBF_NPCBLOCKING || ent->client->ps.ManualBlockingFlags & 1 << HOLDINGBLOCK);

			if (!collisionResolved && base_damage)
			{
				//some other kind of in-hand saber collision
				//handle my reaction
				if (!ent->client->ps.saberInFlight
					&& ent->client->ps.saberLockTime < level.time)
				{
					//my saber is in hand
					if (ent->client->ps.saberBlocked != BLOCKED_PARRY_BROKEN)
					{
						if (PM_SaberInAttack(ent->client->ps.saberMove) || pm_saber_in_special_attack(ent->client->ps.torsoAnim)
							&& !PM_SaberInIdle(ent->client->ps.saberMove)
							&& !PM_SaberInParry(ent->client->ps.saberMove)
							&& !PM_SaberInReflect(ent->client->ps.saberMove)
							&& !PM_SaberInBounce(ent->client->ps.saberMove)
							&& !PM_SaberInMassiveBounce(ent->client->ps.saberMove))
						{
							//in the middle of attacking
							if (hitOwner->health > 0)
							{
								//don't deflect/bounce in strong attack or when enemy is dead
								wp_get_saber_deflection_angle(ent, hitOwner);
								ent->client->ps.saberEventFlags |= SEF_BLOCKED;
								wp_saber_clear_damage_for_ent_num(ent, hitOwner->s.number, saber_num, blade_num);

								if (d_blockinfo->integer || g_DebugSaberCombat->integer)
								{
									gi.Printf(S_COLOR_CYAN"Saber Throw ent Swing blocked\n");
								}
							}
						}
						else
						{
							if (active_defenseentagain)
							{
								if (!PM_SaberInTransitionAny(ent->client->ps.saberMove) && !PM_SaberInBounce(ent->client->ps.saberMove))
								{
									WP_SaberParry(ent, hitOwner, saber_num, blade_num);
									wp_saber_clear_damage_for_ent_num(ent, hitOwner->s.number, saber_num, blade_num);
									ent->client->ps.saberEventFlags |= SEF_PARRIED;

									if (d_blockinfo->integer || g_DebugSaberCombat->integer)
									{
										gi.Printf(S_COLOR_CYAN"Saber throw ent manual blocked\n");
									}
								}
							}
						}
					}
					else
					{
						wp_saber_clear_damage_for_ent_num(ent, hitOwner->s.number, saber_num, blade_num);

						if (d_blockinfo->integer || g_DebugSaberCombat->integer)
						{
							gi.Printf(S_COLOR_CYAN"Saber throw ent auto cleared damage?\n");
						}
					}
				}
				else
				{
					//nothing happens to *me* when my inFlight saber hits something
				}

				//handle their reaction
				if (hitOwner
					&& hitOwner->health > 0
					&& hitOwner->client
					&& !hitOwner->client->ps.saberInFlight
					&& hitOwner->client->ps.saberLockTime < level.time)
				{
					//their saber is in hand
					if (PM_SaberInAttack(hitOwner->client->ps.saberMove) || pm_saber_in_special_attack(hitOwner->client->ps.torsoAnim)
						&& !PM_SaberInIdle(hitOwner->client->ps.saberMove)
						&& !PM_SaberInParry(hitOwner->client->ps.saberMove)
						&& !PM_SaberInReflect(hitOwner->client->ps.saberMove)
						&& !PM_SaberInBounce(hitOwner->client->ps.saberMove)
						&& !PM_SaberInMassiveBounce(hitOwner->client->ps.saberMove))
					{
						//in the middle of attacking
						if (d_blockinfo->integer || g_DebugSaberCombat->integer)
						{
							gi.Printf(S_COLOR_CYAN"Saber throw Hit owner blank\n");
						}
					}
					else
					{
						//saber collided when not attacking, parry it
						if (!PM_SaberInBrokenParry(hitOwner->client->ps.saberMove))
						{
							if (!WP_SaberParry(hitOwner, ent, saber_num, blade_num))
							{
								if (hitOwner->client->ps.blockPoints < BLOCKPOINTS_FATIGUE)
								{
									//Low points = bad blocks
									SabBeh_SaberShouldBeDisarmedBlocker(hitOwner, saber_num);
									wp_block_points_regenerate_over_ride(hitOwner, BLOCKPOINTS_TEN);
								}
								else
								{
									G_FatigueBPKnockaway(hitOwner);
									PM_AddBlockFatigue(&hitOwner->client->ps, BLOCKPOINTS_TEN);
								}

								if (d_blockinfo->integer || g_DebugSaberCombat->integer)
								{
									gi.Printf(S_COLOR_CYAN"Saber throw hit owner Did not parry\n");
								}
							}
						}
					}
				}
				else
				{
					//nothing happens to *hitOwner* when their inFlight saber hits something
				}
			}

			//collision must have been handled by now
			//Set the blocked attack bounce value in saberBlocked so we actually play our saberBounceMove anim
			if (ent->client->ps.saberEventFlags & SEF_BLOCKED)
			{
				if (ent->client->ps.saberBlocked != BLOCKED_PARRY_BROKEN)
				{
					ent->client->ps.saberBlocked = BLOCKED_ATK_BOUNCE;
					ent->client->ps.userInt3 |= 1 << FLAG_SLOWBOUNCE;
				}
			}
		}
		// End of saber behaviour code jacesolaris

		/////////////////////////////////////Sound effects //////////////////////////////////////
		if (saberHitFraction < 1.0f || collisionResolved)
		{
			//either actually hit or locked
			if (ent->client->ps.saberLockTime < level.time)
			{
				if (inFlightSaberBlocked)
				{
					//FIXME: never hear this sound
					wp_saber_bounce_sound(ent, hitOwner, &g_entities[ent->client->ps.saberEntityNum], 0, 0);
				}
				else
				{
					if (deflected)
					{
						wp_saber_bounce_sound(ent, hitOwner, nullptr, saber_num, blade_num);
					}
					else
					{
						if (ent->client->ps.blockPoints < BLOCKPOINTS_HALF)
						{
							wp_saber_knock_sound(ent, hitOwner, saber_num, blade_num);
						}
						else
						{
							wp_saber_block_sound(ent, hitOwner, saber_num, blade_num);
						}
					}
				}
				if (!g_saberNoEffects)
				{
					if (ent->client->ps.blockPoints < BLOCKPOINTS_HALF)
					{
						wp_saber_block_effect(ent, saber_num, blade_num, saberHitLocation, saberHitNormal, qfalse);
					}
					else
					{
						wp_saber_m_block_effect(ent, saber_num, blade_num, saberHitLocation, saberHitNormal, qfalse);
					}
				}
			}
			if (!g_noClashFlare)
			{
				g_saberFlashTime = level.time - 50;
				VectorCopy(saberHitLocation, g_saberFlashPos);
			}
		}

		if (saberHitFraction < 1.0f)
		{
			if (inFlightSaberBlocked)
			{
				//we threw a saber and it was blocked, do any effects, etc.
				int knockAway = 5;
				if (hitEnt
					&& hitOwner
					&& hitOwner->client
					&& (PM_SaberInAttack(hitOwner->client->ps.saberMove) ||
						pm_saber_in_special_attack(hitOwner->client->ps.torsoAnim) || PM_SpinningSaberAnim(
							hitOwner->client->ps.torsoAnim)))
				{
					//if hit someone who was in an attack or spin anim, more likely to have in-flight saber knocked away
					if (BlockerPowerLevel > FORCE_LEVEL_2)
					{
						//strong attacks almost always knock it aside!
						knockAway = 1;
					}
					else
					{
						//33% chance
						knockAway = 2;
					}
					knockAway -= hitOwner->client->ps.SaberDisarmBonus(0);
				}
				if (Q_irand(0, knockAway) <= 0 ||
					hitOwner
					&& hitOwner->client
					&& hitOwner->NPC
					&& hitOwner->NPC->aiFlags & NPCAI_BOSS_CHARACTER)
				{
					//knock it aside and turn it off
					if (!g_saberNoEffects)
					{
						if (!WP_SaberBladeUseSecondBladeStyle(&ent->client->ps.saber[saber_num], blade_num)
							&& ent->client->ps.saber[saber_num].hitOtherEffect)
						{
							G_PlayEffect(ent->client->ps.saber[saber_num].hitOtherEffect, saberHitLocation,
								saberHitNormal);
						}
						else if (WP_SaberBladeUseSecondBladeStyle(&ent->client->ps.saber[saber_num], blade_num)
							&& ent->client->ps.saber[saber_num].hitOtherEffect2)
						{
							G_PlayEffect(ent->client->ps.saber[saber_num].hitOtherEffect2, saberHitLocation,
								saberHitNormal);
						}
						else
						{
							G_PlayEffect("saber/saber_bodyhit", saberHitLocation, saberHitNormal);
						}
					}
					if (hitEnt)
					{
						vec3_t newDir;

						VectorSubtract(g_entities[ent->client->ps.saberEntityNum].currentOrigin, hitEnt->currentOrigin,
							newDir);
						VectorNormalize(newDir);

						if (ent->s.number >= MAX_CLIENTS && !G_ControlledByPlayer(ent))
						{
							g_reflect_missile_npc(ent, &g_entities[ent->client->ps.saberEntityNum], newDir);
						}
						else
						{
							g_reflect_missile_auto(ent, &g_entities[ent->client->ps.saberEntityNum], newDir);
						}
					}
					Jedi_PlayDeflectSound(hitOwner);
					WP_SaberDrop(ent, &g_entities[ent->client->ps.saberEntityNum]);
				}
				else
				{
					if (!Q_irand(0, 2) && hitEnt)
					{
						vec3_t newDir;
						VectorSubtract(g_entities[ent->client->ps.saberEntityNum].currentOrigin, hitEnt->currentOrigin,
							newDir);
						VectorNormalize(newDir);

						if (ent->s.number >= MAX_CLIENTS && !G_ControlledByPlayer(ent))
						{
							g_reflect_missile_npc(ent, &g_entities[ent->client->ps.saberEntityNum], newDir);
						}
						else
						{
							g_reflect_missile_auto(ent, &g_entities[ent->client->ps.saberEntityNum], newDir);
						}
					}
					WP_SaberDrop(ent, &g_entities[ent->client->ps.saberEntityNum]);
				}
			}
		}
	}

	if (ent->client->ps.saberLockTime > level.time)
	{
		if (ent->s.number < ent->client->ps.saberLockEnemy
			&& !Q_irand(0, 3))
		{
			//need to make some kind of effect
			vec3_t hitNorm = { 0, 0, 1 };
			if (wp_sabers_intersection(ent, &g_entities[ent->client->ps.saberLockEnemy], g_saberFlashPos))
			{
				int index = 1;
				index = Q_irand(1, 9);

				if (Q_irand(0, 10))
				{
					if (!g_saberNoEffects)
					{
						wp_saber_block_effect(ent, saber_num, blade_num, g_saberFlashPos, hitNorm, qfalse);
					}
				}
				else
				{
					if (!g_noClashFlare)
					{
						g_saberFlashTime = level.time - 50;
					}
					if (!g_saberNoEffects)
					{
						G_PlayEffect("saber/saber_friction.efx", g_saberFlashPos, hitNorm);
					}
				}
				G_Sound(ent, G_SoundIndex(va("sound/weapons/saber/saberlock%d.mp3", index)));
			}
		}
	}
	else
	{
		if (hit_wall
			&& ent->client->ps.saber[saber_num].saberFlags & SFL_BOUNCE_ON_WALLS
			&& (PM_SaberInAttackPure(ent->client->ps.saberMove) //only in a normal attack anim
				|| ent->client->ps.saberMove == LS_A_JUMP_T__B_ || ent->client->ps.saberMove == LS_A_JUMP_PALP_))
			//or in the strong jump-fwd-attack "death from above" move
		{
			//bounce off walls
			ent->client->ps.saberBlocked = BLOCKED_ATK_BOUNCE;
			ent->client->ps.saberBounceMove = LS_D1_BR + (saberMoveData[ent->client->ps.saberMove].startQuad - Q_BR);
			//do bounce sound & force feedback
			wp_saber_bounce_on_wall_sound(ent, saber_num, blade_num);
			//do hit effect
			if (!g_saberNoEffects)
			{
				if (!WP_SaberBladeUseSecondBladeStyle(&ent->client->ps.saber[saber_num], blade_num)
					&& ent->client->ps.saber[saber_num].hitOtherEffect)
				{
					G_PlayEffect(ent->client->ps.saber[saber_num].hitOtherEffect, saberHitLocation, saberHitNormal);
				}
				else if (WP_SaberBladeUseSecondBladeStyle(&ent->client->ps.saber[saber_num], blade_num)
					&& ent->client->ps.saber[saber_num].hitOtherEffect2)
				{
					G_PlayEffect(ent->client->ps.saber[saber_num].hitOtherEffect2, saberHitLocation, saberHitNormal);
				}
				else
				{
					G_PlayEffect("saber/saber_bodyhit", saberHitLocation, saberHitNormal);
				}
			}
			//do radius damage/knockback, if any
			if (!WP_SaberBladeUseSecondBladeStyle(&ent->client->ps.saber[saber_num], blade_num))
			{
				WP_SaberRadiusDamage(ent, saberHitLocation, ent->client->ps.saber[saber_num].splashRadius,
					ent->client->ps.saber[saber_num].splashDamage,
					ent->client->ps.saber[saber_num].splashKnockback);
			}
			else
			{
				WP_SaberRadiusDamage(ent, saberHitLocation, ent->client->ps.saber[saber_num].splashRadius2,
					ent->client->ps.saber[saber_num].splashDamage2,
					ent->client->ps.saber[saber_num].splashKnockback2);
			}
		}
		else if (hit_wall &&
			!PM_SaberInAttackPure(ent->client->ps.saberMove) &&
			!PM_CrouchAnim(ent->client->ps.legsAnim) &&
			!PM_WalkingAnim(ent->client->ps.legsAnim) &&
			!PM_RunningAnim(ent->client->ps.legsAnim) &&
			ent->client->buttons & BUTTON_WALKING &&
			ent->client->ps.ManualBlockingFlags & 1 << HOLDINGBLOCK &&
			(ent->s.number < MAX_CLIENTS || G_ControlledByPlayer(ent)))
		{
			//reflect from wall
			//do bounce sound & force feedback
			ent->client->ps.saberBlocked = BLOCKED_ATK_BOUNCE;
			ent->client->ps.saberBounceMove = LS_D1_BR + (saberMoveData[ent->client->ps.saberMove].startQuad - Q_BR);

			//do hit effect
			if (!g_saberNoEffects)
			{
				if (!WP_SaberBladeUseSecondBladeStyle(&ent->client->ps.saber[saber_num], blade_num)
					&& ent->client->ps.saber[saber_num].hitOtherEffect)
				{
					G_PlayEffect(ent->client->ps.saber[saber_num].hitOtherEffect, saberHitLocation, saberHitNormal);
				}
				else if (WP_SaberBladeUseSecondBladeStyle(&ent->client->ps.saber[saber_num], blade_num)
					&& ent->client->ps.saber[saber_num].hitOtherEffect2)
				{
					G_PlayEffect(ent->client->ps.saber[saber_num].hitOtherEffect2, saberHitLocation, saberHitNormal);
				}
				else
				{
					G_PlayEffect("saber/saber_cut", saberHitLocation, saberHitNormal);
				}
			}
			//do radius damage/knockback, if any
			if (!WP_SaberBladeUseSecondBladeStyle(&ent->client->ps.saber[saber_num], blade_num))
			{
				WP_SaberRadiusDamage(ent, saberHitLocation, ent->client->ps.saber[saber_num].splashRadius,
					ent->client->ps.saber[saber_num].splashDamage,
					ent->client->ps.saber[saber_num].splashKnockback);
			}
			else
			{
				WP_SaberRadiusDamage(ent, saberHitLocation, ent->client->ps.saber[saber_num].splashRadius2,
					ent->client->ps.saber[saber_num].splashDamage2,
					ent->client->ps.saber[saber_num].splashKnockback2);
			}
		}
	}

	if (wp_saber_apply_damage(ent, base_damage, base_d_flags, broken_parry, saber_num, blade_num,
		static_cast<qboolean>(saber_num == 0 && ent->client->ps.saberInFlight)))
	{
		//actually did damage to something
		wp_saber_hit_sound(ent, saber_num, blade_num);
		if (!saber_in_special && (d_combatinfo->integer || g_DebugSaberCombat->integer))
		{
			if (g_saberRealisticCombat->integer == 3)
			{
				gi.Printf("Very High Damage. Min damage done was %4.2f\n", base_damage);
			}
			else if (g_saberRealisticCombat->integer == 2)
			{
				gi.Printf("High Damage. Min damage done was %4.2f\n", base_damage);
			}
			else if (g_saberRealisticCombat->integer == 1)
			{
				gi.Printf("Medium Damage. Min damage done was %4.2f\n", base_damage);
			}
			else if (g_saberRealisticCombat->integer == 0)
			{
				gi.Printf("Low Damage. Min damage done was %4.2f\n", base_damage);
			}
		}
	}

	if (hit_wall)
	{
		//just so Jedi knows that he hit a wall
		ent->client->ps.saberEventFlags |= SEF_HITWALL;
		if (ent->s.number == 0)
		{
			AddSoundEvent(ent, ent->currentOrigin, 128, AEL_DISCOVERED, qfalse, qtrue);
			AddSightEvent(ent, ent->currentOrigin, 256, AEL_DISCOVERED, 50);
		}
	}
}

void WP_SabersDamageTrace(gentity_t* ent, qboolean noEffects)
{
	if (!ent->client)
	{
		return;
	}
	if (PM_SuperBreakLoseAnim(ent->client->ps.torsoAnim))
	{
		return;
	}
	// Saber 1.
	g_saberNoEffects = noEffects;
	for (int i = 0; i < ent->client->ps.saber[0].numBlades; i++)
	{
		// If the Blade is not active and the length is 0, don't trace it, try the next blade...
		if (!ent->client->ps.saber[0].blade[i].active && ent->client->ps.saber[0].blade[i].length == 0)
			continue;

		if (i != 0)
		{
			//not first blade
			if (ent->client->ps.saber[0].type == SABER_BROAD ||
				ent->client->ps.saber[0].type == SABER_SAI ||
				ent->client->ps.saber[0].type == SABER_CLAW)
			{
				g_saberNoEffects = qtrue;
			}
		}
		g_noClashFlare = qfalse;
		if (!WP_SaberBladeUseSecondBladeStyle(&ent->client->ps.saber[0], i) && ent->client->ps.saber[0].saberFlags2 &
			SFL2_NO_CLASH_FLARE
			|| WP_SaberBladeUseSecondBladeStyle(&ent->client->ps.saber[0], i) && ent->client->ps.saber[0].saberFlags2 &
			SFL2_NO_CLASH_FLARE2)
		{
			g_noClashFlare = qtrue;
		}

		WP_SaberDamageTrace(ent, 0, i);
	}
	// Saber 2.
	g_saberNoEffects = noEffects;
	if (ent->client->ps.dualSabers)
	{
		for (int i = 0; i < ent->client->ps.saber[1].numBlades; i++)
		{
			// If the Blade is not active and the length is 0, don't trace it, try the next blade...
			if (!ent->client->ps.saber[1].blade[i].active && ent->client->ps.saber[1].blade[i].length == 0)
				continue;

			if (i != 0)
			{
				//not first blade
				if (ent->client->ps.saber[1].type == SABER_BROAD ||
					ent->client->ps.saber[1].type == SABER_SAI ||
					ent->client->ps.saber[1].type == SABER_CLAW)
				{
					g_saberNoEffects = qtrue;
				}
			}
			g_noClashFlare = qfalse;
			if (!WP_SaberBladeUseSecondBladeStyle(&ent->client->ps.saber[1], i) && ent->client->ps.saber[1].saberFlags2
				& SFL2_NO_CLASH_FLARE
				|| WP_SaberBladeUseSecondBladeStyle(&ent->client->ps.saber[1], i) && ent->client->ps.saber[1].
				saberFlags2 & SFL2_NO_CLASH_FLARE2)
			{
				g_noClashFlare = qtrue;
			}

			WP_SaberDamageTrace(ent, 1, i);
		}
	}
	g_saberNoEffects = qfalse;
	g_noClashFlare = qfalse;
}

//SABER THROWING============================================================================
//SABER THROWING============================================================================
//SABER THROWING============================================================================
//SABER THROWING============================================================================
//SABER THROWING============================================================================
//SABER THROWING============================================================================

/*
================
WP_SaberImpact

================
*/
void WP_SaberImpact(gentity_t* owner, gentity_t* saber, trace_t* trace)
{
	gentity_t* other = &g_entities[trace->entityNum];

	if (other->takedamage && other->svFlags & SVF_BBRUSH)
	{
		//a breakable brush?  break it!
		if (other->spawnflags & 1 //INVINCIBLE
			|| other->flags & FL_DMG_BY_HEAVY_WEAP_ONLY) //HEAVY weapon damage only
		{
			//can't actually break it
			//no hit effect (besides regular client-side one)
		}
		else if (other->NPC_targetname &&
			(!owner || !owner->targetname || Q_stricmp(owner->targetname, other->NPC_targetname)))
		{
			//only breakable by an entity who is not the attacker
			//no hit effect (besides regular client-side one)
		}
		else
		{
			vec3_t dir;
			VectorCopy(saber->s.pos.trDelta, dir);
			VectorNormalize(dir);

			int dmg = other->health * 2;
			if (other->health > 50 && dmg > 20 && !(other->svFlags & SVF_GLASS_BRUSH))
			{
				dmg = 20;
			}
			G_Damage(other, saber, owner, dir, trace->endpos, dmg, 0, MOD_SABER);
			if (owner
				&& owner->client
				&& owner->client->ps.saber[0].hitOtherEffect)
			{
				G_PlayEffect(owner->client->ps.saber[0].hitOtherEffect, trace->endpos, dir);
			}
			else
			{
				G_PlayEffect("saber/saber_cut", trace->endpos, dir);
			}
			if (owner->s.number == 0)
			{
				AddSoundEvent(owner, trace->endpos, 256, AEL_DISCOVERED);
				AddSightEvent(owner, trace->endpos, 512, AEL_DISCOVERED, 50);
			}
			return;
		}
	}

	if (saber->s.pos.trType == TR_LINEAR)
	{
		//hit a wall?
		WP_SaberDrop(saber->owner, saber);
	}

	if (other && !other->client && other->contents & CONTENTS_LIGHTSABER)
	{
		//2 in-flight sabers collided!
		wp_saber_knock_sound(saber->owner, nullptr, 0, 0);
		wp_saber_block_effect(saber->owner, 0, 0, trace->endpos, nullptr, qfalse);
		qboolean noFlare = qfalse;
		if (saber->owner
			&& saber->owner->client
			&& saber->owner->client->ps.saber[0].saberFlags2 & SFL2_NO_CLASH_FLARE)
		{
			noFlare = qtrue;
		}
		if (!noFlare)
		{
			g_saberFlashTime = level.time - 50;
			VectorCopy(trace->endpos, g_saberFlashPos);
		}
	}

	if (owner && owner->s.number == 0 && owner->client)
	{
		//Add the event
		if (owner->client->ps.SaberLength() > 0)
		{
			//saber is on, very suspicious
			if (!owner->client->ps.saberInFlight && owner->client->ps.groundEntityNum == ENTITYNUM_WORLD
				//holding saber and on ground
				|| saber->s.pos.trType == TR_STATIONARY) //saber out there somewhere and on ground
			{
				//an on-ground alert
				AddSoundEvent(owner, saber->currentOrigin, 128, AEL_DISCOVERED, qfalse, qtrue);
			}
			else
			{
				//an in-air alert
				AddSoundEvent(owner, saber->currentOrigin, 128, AEL_DISCOVERED);
			}
			AddSightEvent(owner, saber->currentOrigin, 256, AEL_DISCOVERED, 50);
		}
		else
		{
			//saber is off, not as suspicious
			AddSoundEvent(owner, saber->currentOrigin, 128, AEL_SUSPICIOUS);
			AddSightEvent(owner, saber->currentOrigin, 256, AEL_SUSPICIOUS);
		}
	}

	// check for bounce
	if (!other->takedamage && saber->s.eFlags & (EF_BOUNCE | EF_BOUNCE_HALF))
	{
		// Check to see if there is a bounce count
		if (saber->bounceCount)
		{
			// decrement number of bounces and then see if it should be done bouncing
			if (--saber->bounceCount <= 0)
			{
				// He (or she) will bounce no more (after this current bounce, that is).
				saber->s.eFlags &= ~(EF_BOUNCE | EF_BOUNCE_HALF);
				if (saber->s.pos.trType == TR_LINEAR && owner && owner->client && owner->client->ps.saberEntityState ==
					SES_RETURNING)
				{
					WP_SaberDrop(saber->owner, saber);
				}
				return;
			}
			//bounced and still have bounces left
			if (saber->s.pos.trType == TR_LINEAR && owner && owner->client && owner->client->ps.saberEntityState ==
				SES_RETURNING)
			{
				//under telekinetic control
				if (!gi.inPVS(saber->currentOrigin, owner->client->renderInfo.handRPoint))
				{
					//not in the PVS of my master
					saber->bounceCount -= 25;
				}
			}
		}

		if (saber->s.pos.trType == TR_LINEAR && owner && owner->client && owner->client->ps.saberEntityState ==
			SES_RETURNING)
		{
			//don't home for a few frames so we can get around this thing
			trace_t bounceTr;
			vec3_t end;
			const float owner_dist = Distance(owner->client->renderInfo.handRPoint, saber->currentOrigin);

			VectorMA(saber->currentOrigin, 10, trace->plane.normal, end);
			gi.trace(&bounceTr, saber->currentOrigin, saber->mins, saber->maxs, end, owner->s.number, saber->clipmask,
				static_cast<EG2_Collision>(0), 0);
			VectorCopy(bounceTr.endpos, saber->currentOrigin);
			if (owner_dist > 0)
			{
				if (owner_dist > 50)
				{
					owner->client->ps.saberEntityDist = owner_dist - 50;
				}
				else
				{
					owner->client->ps.saberEntityDist = 0;
				}
			}
			return;
		}

		g_bounce_missile(saber, trace);

		if (saber->s.pos.trType == TR_GRAVITY)
		{
			//bounced
			//play a bounce sound
			wp_saber_fall_sound(owner, saber);
			//change rotation
			VectorCopy(saber->currentAngles, saber->s.apos.trBase);
			saber->s.apos.trType = TR_LINEAR;
			saber->s.apos.trTime = level.time;
			VectorSet(saber->s.apos.trDelta, Q_irand(-300, 300), Q_irand(-300, 300), Q_irand(-300, 300));
		}
		//see if we stopped
		else if (saber->s.pos.trType == TR_STATIONARY)
		{
			//stopped
			//play a bounce sound
			wp_saber_fall_sound(owner, saber);
			//stop rotation
			VectorClear(saber->s.apos.trDelta);
			pitch_roll_for_slope(saber, trace->plane.normal, saber->currentAngles);
			saber->currentAngles[0] += SABER_PITCH_HACK;
			VectorCopy(saber->currentAngles, saber->s.apos.trBase);
			//remember when it fell so it can return auto-magically
			saber->aimDebounceTime = level.time;
		}
	}
	else if (other->client && other->health > 0
		&& (other->NPC && other->NPC->aiFlags & NPCAI_BOSS_CHARACTER
			|| other->client->NPC_class == CLASS_BOBAFETT
			|| other->client->ps.powerups[PW_GALAK_SHIELD] > 0))
	{
		//Luke, Desann and Tavion slap thrown sabers aside
		WP_SaberDrop(owner, saber);

		if (other->client && owner->client->ps.blockPoints < BLOCKPOINTS_HALF)
		{
			wp_saber_knock_sound(owner, other, 0, 0);
		}
		else
		{
			wp_saber_block_sound(owner, other, 0, 0);
		}
		wp_saber_block_effect(owner, 0, 0, trace->endpos, nullptr, qfalse);
		qboolean no_flare = qfalse;
		if (owner
			&& owner->client
			&& owner->client->ps.saber[0].saberFlags2 & SFL2_NO_CLASH_FLARE)
		{
			no_flare = qtrue;
		}
		if (!no_flare)
		{
			g_saberFlashTime = level.time - 50;
			VectorCopy(trace->endpos, g_saberFlashPos);
		}
		//FIXME: make Luke/Desann/Tavion play an attack anim or some other special anim when this happens
		Jedi_PlayDeflectSound(other);
	}
}

extern float G_PointDistFromLineSegment(const vec3_t start, const vec3_t end, const vec3_t from);

void wp_saber_in_flight_reflect_check(gentity_t* self)
{
	gentity_t* entityList[MAX_GENTITIES];
	gentity_t* missile_list[MAX_GENTITIES];
	vec3_t mins, maxs;
	int ent_count = 0;
	vec3_t center;
	vec3_t up = { 0, 0, 1 };

	if (self->NPC && self->NPC->scriptFlags & SCF_IGNORE_ALERTS)
	{
		//don't react to things flying at me...
		return;
	}
	//sanity checks: make sure we actually have a saberent
	if (self->client->ps.weapon != WP_SABER)
	{
		return;
	}
	if (!self->client->ps.saberInFlight)
	{
		return;
	}
	if (!self->client->ps.SaberLength())
	{
		return;
	}
	if (self->client->ps.saberEntityNum == ENTITYNUM_NONE)
	{
		return;
	}
	gentity_t* saberent = &g_entities[self->client->ps.saberEntityNum];
	if (!saberent)
	{
		return;
	}
	//okay, enough damn sanity checks

	VectorCopy(saberent->currentOrigin, center);

	for (int i = 0; i < 3; i++)
	{
		constexpr int radius = 180;
		mins[i] = center[i] - radius;
		maxs[i] = center[i] + radius;
	}

	const int num_listed_entities = gi.EntitiesInBox(mins, maxs, entityList, MAX_GENTITIES);

	//FIXME: check visibility?
	for (int e = 0; e < num_listed_entities; e++)
	{
		gentity_t* ent = entityList[e];

		if (ent == self)
			continue;
		if (ent->owner == self)
			continue;
		if (!ent->inuse)
			continue;
		if (ent->s.eType != ET_MISSILE)
		{
			if (ent->client || ent->s.weapon != WP_SABER)
			{
				//FIXME: wake up bad guys?
				continue;
			}
			if (ent->s.eFlags & EF_NODRAW)
			{
				continue;
			}
			if (Q_stricmp("lightsaber", ent->classname) != 0)
			{
				//not a lightsaber
				continue;
			}
		}
		else
		{
			//FIXME: make exploding missiles explode?
			if (ent->s.pos.trType == TR_STATIONARY)
			{
				//nothing you can do with a stationary missile
				continue;
			}
			if (ent->splashDamage || ent->splashRadius)
			{
				//can't deflect exploding missiles
				if (DistanceSquared(ent->currentOrigin, center) < 256) //16 squared
				{
					g_missile_impacted(ent, saberent, ent->currentOrigin, up);
				}
				continue;
			}
		}

		//don't deflect it if it's not within 16 units of the blade
		//do this for all blades
		qboolean will_hit = qfalse;
		int num_sabers = 1;
		if (self->client->ps.dualSabers)
		{
			num_sabers = 2;
		}
		for (int saber_num = 0; saber_num < num_sabers; saber_num++)
		{
			for (int blade_num = 0; blade_num < self->client->ps.saber[saber_num].numBlades; blade_num++)
			{
				vec3_t tip;
				VectorMA(self->client->ps.saber[saber_num].blade[blade_num].muzzlePoint,
					self->client->ps.saber[saber_num].blade[blade_num].length,
					self->client->ps.saber[saber_num].blade[blade_num].muzzleDir, tip);

				if (G_PointDistFromLineSegment(self->client->ps.saber[saber_num].blade[blade_num].muzzlePoint, tip,
					ent->currentOrigin) <= 32)
				{
					will_hit = qtrue;
					break;
				}
			}
			if (will_hit)
			{
				break;
			}
		}
		if (!will_hit)
		{
			continue;
		}
		// ok, we are within the radius, add us to the incoming list
		missile_list[ent_count] = ent;
		ent_count++;
	}

	if (ent_count)
	{
		// we are done, do we have any to deflect?
		if (ent_count)
		{
			for (int x = 0; x < ent_count; x++)
			{
				vec3_t fx_dir;
				if (missile_list[x]->s.weapon == WP_SABER)
				{
					//just send it back
					if (missile_list[x]->owner && missile_list[x]->owner->client && missile_list[x]->owner->client->ps.
						saber[0].Active() && missile_list[x]->s.pos.trType == TR_LINEAR && missile_list[x]->owner->
						client->ps.saberEntityState != SES_RETURNING)
					{
						//it's on and being controlled
						if (missile_list[x]->owner->NPC && !G_ControlledByPlayer(missile_list[x]->owner))
						{
							WP_SaberDrop(missile_list[x]->owner, missile_list[x]);
						}
						else
						{
							WP_SaberReturn(missile_list[x]->owner, missile_list[x]);
						}

						VectorNormalize2(missile_list[x]->s.pos.trDelta, fx_dir);
						wp_saber_block_effect(self, 0, 0, missile_list[x]->currentOrigin, fx_dir, qfalse);
						if (missile_list[x]->owner->client->ps.saberInFlight && self->client->ps.saberInFlight)
						{
							wp_saber_block_sound(self, missile_list[x]->owner, 0, 0);
							qboolean no_flare = qfalse;
							if (missile_list[x]->owner->client->ps.saber[0].saberFlags2 & SFL2_NO_CLASH_FLARE
								&& self->client->ps.saber[0].saberFlags2 & SFL2_NO_CLASH_FLARE)
							{
								no_flare = qtrue;
							}
							if (!no_flare)
							{
								g_saberFlashTime = level.time - 50;
								const gentity_t* saber = &g_entities[self->client->ps.saberEntityNum];
								vec3_t org;
								VectorSubtract(missile_list[x]->currentOrigin, saber->currentOrigin, org);
								VectorMA(saber->currentOrigin, 0.5, org, org);
								VectorCopy(org, g_saberFlashPos);
							}
						}
					}
				}
				else
				{
					//bounce it
					vec3_t reflect_angle, forward;
					if (self->client && !self->s.number)
					{
						self->client->sess.missionStats.saberBlocksCnt++;
					}
					VectorCopy(saberent->s.apos.trBase, reflect_angle);
					reflect_angle[PITCH] = Q_flrand(-90, 90);
					AngleVectors(reflect_angle, forward, nullptr, nullptr);

					if (self->s.number >= MAX_CLIENTS && !G_ControlledByPlayer(self))
					{
						g_reflect_missile_npc(self, missile_list[x], forward);
					}
					else
					{
						g_reflect_missile_auto(self, missile_list[x], forward);
					}
					//do an effect
					VectorNormalize2(missile_list[x]->s.pos.trDelta, fx_dir);
					G_PlayEffect("blaster/deflect_passthrough", missile_list[x]->currentOrigin, fx_dir);
				}
			}
		}
	}
}

qboolean wp_saber_validate_enemy(gentity_t* self, gentity_t* enemy)
{
	if (!enemy)
	{
		return qfalse;
	}

	if (!enemy || enemy == self || !enemy->inuse || !enemy->client)
	{
		//not valid
		return qfalse;
	}

	if (enemy->health <= 0)
	{
		//corpse
		return qfalse;
	}

	if (enemy->s.number >= MAX_CLIENTS)
	{
		//NPCs can cheat and use the homing saber throw 3 on the player
		if (enemy->client->ps.forcePowersKnown)
		{
			//not other jedi?
			return qfalse;
		}
	}

	if (DistanceSquared(self->client->renderInfo.handRPoint, enemy->currentOrigin) > saberThrowDistSquared[self->client
		->ps.forcePowerLevel[FP_SABERTHROW]])
	{
		//too far
		return qfalse;
	}

	if ((!in_front(enemy->currentOrigin, self->currentOrigin, self->client->ps.viewangles, 0.0f) || !G_ClearLOS(
		self, self->client->renderInfo.eyePoint, enemy))
		&& (DistanceHorizontalSquared(enemy->currentOrigin, self->currentOrigin) > 65536 || fabs(
			enemy->currentOrigin[2] - self->currentOrigin[2]) > 384))
	{
		//(not in front or not clear LOS) & greater than 256 away
		return qfalse;
	}

	if (enemy->client->playerTeam == self->client->playerTeam && (enemy->client->playerTeam != TEAM_SOLO && self->client
		->playerTeam != TEAM_SOLO))
	{
		//on same team
		return qfalse;
	}

	//LOS?

	return qtrue;
}

float WP_SaberRateEnemy(const gentity_t* enemy, vec3_t center, vec3_t forward, float radius)
{
	vec3_t dir;

	VectorSubtract(enemy->currentOrigin, center, dir);
	float rating = 1.0f - VectorNormalize(dir) / radius;
	rating *= DotProduct(forward, dir);
	return rating;
}

gentity_t* wp_saber_find_enemy(gentity_t* self, const gentity_t* saber)
{
	//FIXME: should be a more intelligent way of doing this, like auto aim?
	//closest, most in front... did damage to... took damage from?  How do we know who the player is focusing on?
	gentity_t* bestEnt = nullptr;
	gentity_t* entityList[MAX_GENTITIES];
	vec3_t center, mins, maxs, fwdangles, forward;
	constexpr float radius = 400;
	float bestRating = 0.0f;

	//FIXME: no need to do this in 1st person?
	fwdangles[1] = self->client->ps.viewangles[1];
	AngleVectors(fwdangles, forward, nullptr, nullptr);

	VectorCopy(saber->currentOrigin, center);

	for (int i = 0; i < 3; i++)
	{
		mins[i] = center[i] - radius;
		maxs[i] = center[i] + radius;
	}

	//if the saber has an enemy from the last time it looked, init to that one
	if (wp_saber_validate_enemy(self, saber->enemy))
	{
		if (gi.inPVS(self->currentOrigin, saber->enemy->currentOrigin))
		{
			//potentially visible
			if (G_ClearLOS(self, self->client->renderInfo.eyePoint, saber->enemy))
			{
				//can see him
				bestEnt = saber->enemy;
				bestRating = WP_SaberRateEnemy(bestEnt, center, forward, radius);
			}
		}
	}

	//If I have an enemy, see if that's even better
	if (wp_saber_validate_enemy(self, self->enemy))
	{
		const float myEnemyRating = WP_SaberRateEnemy(self->enemy, center, forward, radius);
		if (myEnemyRating > bestRating)
		{
			if (gi.inPVS(self->currentOrigin, self->enemy->currentOrigin))
			{
				//potentially visible
				if (G_ClearLOS(self, self->client->renderInfo.eyePoint, self->enemy))
				{
					//can see him
					bestEnt = self->enemy;
					bestRating = myEnemyRating;
				}
			}
		}
	}

	const int numListedEntities = gi.EntitiesInBox(mins, maxs, entityList, MAX_GENTITIES);

	if (!numListedEntities)
	{
		//should we clear the enemy?
		return bestEnt;
	}

	for (int e = 0; e < numListedEntities; e++)
	{
		gentity_t* ent = entityList[e];

		if (ent == self || ent == saber || ent == bestEnt)
		{
			continue;
		}
		if (!wp_saber_validate_enemy(self, ent))
		{
			//doesn't meet criteria of valid look enemy (don't check current since we would have done that before this func's call
			continue;
		}
		if (!gi.inPVS(self->currentOrigin, ent->currentOrigin))
		{
			//not even potentially visible
			continue;
		}
		if (!G_ClearLOS(self, self->client->renderInfo.eyePoint, ent))
		{
			//can't see him
			continue;
		}
		//rate him based on how close & how in front he is
		const float rating = WP_SaberRateEnemy(ent, center, forward, radius);
		if (rating > bestRating)
		{
			bestEnt = ent;
			bestRating = rating;
		}
	}
	return bestEnt;
}

void WP_RunSaber(gentity_t* self, gentity_t* saber)
{
	vec3_t origin, oldOrg;
	trace_t tr;

	VectorCopy(saber->currentOrigin, oldOrg);
	// get current position
	EvaluateTrajectory(&saber->s.pos, level.time, origin);
	// get current angles
	EvaluateTrajectory(&saber->s.apos, level.time, saber->currentAngles);

	// trace a line from the previous position to the current position,
	// ignoring interactions with the missile owner
	int clipmask = saber->clipmask;
	if (!self || !self->client || self->client->ps.SaberLength() <= 0)
	{
		//don't keep hitting other sabers when turned off
		clipmask &= ~CONTENTS_LIGHTSABER;
	}
	gi.trace(&tr, saber->currentOrigin, saber->mins, saber->maxs, origin,
		saber->owner ? saber->owner->s.number : ENTITYNUM_NONE, clipmask, static_cast<EG2_Collision>(0), 0);

	VectorCopy(tr.endpos, saber->currentOrigin);

	if (self->client->ps.SaberActive())
	{
		if (self->client->ps.saberInFlight || self->client->ps.weaponTime && !Q_irand(0, 100))
		{
			//make enemies run from a lit saber in flight or from me when I'm attacking
			if (!Q_irand(0, 10))
			{
				//not so often...
				AddSightEvent(self, saber->currentOrigin, self->client->ps.SaberLength() * 3, AEL_DANGER, 100);
			}
		}
	}

	if (tr.startsolid)
	{
		tr.fraction = 0;
	}

	gi.linkentity(saber);

	//touch push triggers?

	if (tr.fraction != 1)
	{
		WP_SaberImpact(self, saber, &tr);
	}

	if (saber->s.pos.trType == TR_LINEAR)
	{
		//home
		//figure out where saber should be
		vec3_t fwdangles = { 0 };

		VectorCopy(self->client->ps.viewangles, fwdangles);
		if (self->s.number)
		{
			fwdangles[0] -= 8;
		}
		else if (cg.renderingThirdPerson)
		{
			fwdangles[0] -= 5;
		}

		if (self->client->ps.forcePowerLevel[FP_SABERTHROW] > FORCE_LEVEL_1
			|| self->client->ps.saberEntityState == SES_RETURNING
			|| VectorCompare(saber->s.pos.trDelta, vec3_origin))
		{
			vec3_t saberDest;
			vec3_t saberHome;
			vec3_t forward;
			//control if it's returning or just starting
			float saberSpeed = 500; //FIXME: based on force level?
			gentity_t* enemy = nullptr;

			AngleVectors(fwdangles, forward, nullptr, nullptr);

			if (self->client->ps.saberEntityDist < 100)
			{
				//make the saber head to my hand- the bolt it was attached to
				VectorCopy(self->client->renderInfo.handRPoint, saberHome);
			}
			else
			{
				//aim saber from eyes
				VectorCopy(self->client->renderInfo.eyePoint, saberHome);
			}
			VectorMA(saberHome, self->client->ps.saberEntityDist, forward, saberDest);

			if (self->client->ps.forcePowerLevel[FP_SABERTHROW] > FORCE_LEVEL_2
				&& self->client->ps.saberEntityState == SES_LEAVING)
			{
				//max level
				if (self->enemy && !wp_saber_validate_enemy(self, self->enemy))
				{
					//if my enemy isn't valid to auto-aim at, don't autoaim
				}
				else
				{
					//pick an enemy
					enemy = wp_saber_find_enemy(self, saber);
					if (enemy)
					{
						//home in on enemy
						const float enemyDist = Distance(self->client->renderInfo.handRPoint, enemy->currentOrigin);
						VectorCopy(enemy->currentOrigin, saberDest);
						saberDest[2] += enemy->maxs[2] / 2.0f;
						//FIXME: when in a knockdown anim, the saber float above them... do we care?
						self->client->ps.saberEntityDist = enemyDist;
						//once you pick an enemy, stay with it!
						saber->enemy = enemy;
						//FIXME: lock onto that enemy for a minimum amount of time (unless they become invalid?)
					}
				}
			}

			//Make the saber head there
			VectorSubtract(saberDest, saber->currentOrigin, saber->s.pos.trDelta);
			const float dist = VectorNormalize(saber->s.pos.trDelta);
			if (self->client->ps.forcePowerLevel[FP_SABERTHROW] > FORCE_LEVEL_2 && self->client->ps.saberEntityState ==
				SES_LEAVING && !enemy)
			{
				if (dist < 200)
				{
					saberSpeed = 400 - dist * 2;
				}
			}
			else if (self->client->ps.saberEntityState == SES_LEAVING && dist < 50)
			{
				saberSpeed = dist * 2 + 30;
				if (enemy && dist > enemy->maxs[0] || !enemy && dist > 24)
				{
					//auto-tracking an enemy and we can't hit him
					if (saberSpeed < 120)
					{
						//clamp to a minimum speed
						saberSpeed = 120;
					}
				}
			}
			VectorScale(saber->s.pos.trDelta, saberSpeed, saber->s.pos.trDelta);
			VectorCopy(saber->currentOrigin, saber->s.pos.trBase);
			saber->s.pos.trTime = level.time;
			saber->s.pos.trType = TR_LINEAR;
		}
		else
		{
			VectorCopy(saber->currentOrigin, saber->s.pos.trBase);
			saber->s.pos.trTime = level.time;
			saber->s.pos.trType = TR_LINEAR;
		}

		//if it's heading back, point it's base at us
		if (self->client->ps.saberEntityState == SES_RETURNING
			&& !(self->client->ps.saber[0].saberFlags & SFL_RETURN_DAMAGE)) //type != SABER_STAR )
		{
			fwdangles[0] += SABER_PITCH_HACK;
			VectorCopy(fwdangles, saber->s.apos.trBase);
			saber->s.apos.trTime = level.time;
			saber->s.apos.trType = TR_INTERPOLATE;
			VectorClear(saber->s.apos.trDelta);
		}
	}
}

qboolean WP_SaberLaunch(gentity_t* self, gentity_t* saber, qboolean thrown, qboolean noFail = qfalse)
{
	constexpr vec3_t saberMins = { -3.0f, -3.0f, -3.0f };
	constexpr vec3_t saberMaxs = { 3.0f, 3.0f, 3.0f };
	trace_t trace;

	if (self->client->NPC_class == CLASS_SABER_DROID)
	{
		//saber droids can't drop their saber
		return qfalse;
	}
	if (!noFail)
	{
		if (thrown)
		{
			//this is a regular throw, so see if it's legal
			if (self->client->ps.forcePowerLevel[FP_SABERTHROW] > FORCE_LEVEL_2)
			{
				if (!WP_ForcePowerUsable(self, FP_SABERTHROW, 20))
				{
					return qfalse;
				}
			}
			else
			{
				if (!WP_ForcePowerUsable(self, FP_SABERTHROW, 0))
				{
					return qfalse;
				}
			}
		}
		if (!self->s.number && (cg.zoomMode || in_camera))
		{
			//can't saber throw when zoomed in or in cinematic
			return qfalse;
		}
		//make sure it won't start in solid
		gi.trace(&trace, self->client->renderInfo.handRPoint, saberMins, saberMaxs, self->client->renderInfo.handRPoint,
			saber->s.number, MASK_SOLID, static_cast<EG2_Collision>(0), 0);
		if (trace.startsolid || trace.allsolid)
		{
			return qfalse;
		}
		//make sure I'm not throwing it on the other side of a door or wall or whatever
		gi.trace(&trace, self->currentOrigin, vec3_origin, vec3_origin, self->client->renderInfo.handRPoint,
			self->s.number, MASK_SOLID, static_cast<EG2_Collision>(0), 0);
		if (trace.startsolid || trace.allsolid || trace.fraction < 1.0f)
		{
			return qfalse;
		}

		if (thrown)
		{
			//this is a regular throw, so take force power
			if (self->client->ps.forcePowerLevel[FP_SABERTHROW] > FORCE_LEVEL_2)
			{
				//at max skill, the cost increases as keep it out
				WP_ForcePowerStart(self, FP_SABERTHROW, 10);
			}
			else
			{
				WP_ForcePowerStart(self, FP_SABERTHROW, 0);
			}
		}
	}
	//clear the enemy
	saber->enemy = nullptr;

	//===FIXME!!!==============================================================================================
	//We should copy the right-hand saber's g2 instance to the thrown saber
	//Then back again when you catch it!!!
	//===FIXME!!!==============================================================================================

	//draw it
	saber->s.eFlags &= ~EF_NODRAW;
	saber->svFlags |= SVF_BROADCAST;
	saber->svFlags &= ~SVF_NOCLIENT;

	//place it
	VectorCopy(self->client->renderInfo.handRPoint, saber->currentOrigin); //muzzlePoint
	VectorCopy(saber->currentOrigin, saber->s.pos.trBase);
	saber->s.pos.trTime = level.time;
	saber->s.pos.trType = TR_LINEAR;
	VectorClear(saber->s.pos.trDelta);
	gi.linkentity(saber);

	//spin it
	VectorClear(saber->s.apos.trBase);
	saber->s.apos.trTime = level.time;
	saber->s.apos.trType = TR_LINEAR;
	if (self->health > 0 && thrown)
	{
		//throwing it
		saber->s.apos.trBase[1] = self->client->ps.viewangles[1];
		saber->s.apos.trBase[0] = SABER_PITCH_HACK;
	}
	else
	{
		//dropping it
		vectoangles(self->client->renderInfo.muzzleDir, saber->s.apos.trBase);
	}
	VectorClear(saber->s.apos.trDelta);

	if (self->client->ps.saberAnimLevel == SS_STAFF)
	{
		saber->s.apos.trDelta[1] = 800;
	}
	else
	{
		saber->s.apos.trDelta[0] = 600;
	}

	//Take it out of my hand
	self->client->ps.saberInFlight = qtrue;
	self->client->ps.saberEntityState = SES_LEAVING;
	self->client->ps.saberEntityDist = saberThrowDist[self->client->ps.forcePowerLevel[FP_SABERTHROW]];
	self->client->ps.saberThrowTime = level.time;
	self->client->ps.forcePowerDebounce[FP_SABERTHROW] = level.time + 1000;
	//so we can keep it out for a minimum amount of time

	if (thrown)
	{
		//this is a regular throw, so turn the saber on
		//turn saber on
		if (self->client->ps.saber[0].saberFlags & SFL_SINGLE_BLADE_THROWABLE)
		{
			//only first blade can be on
			if (!self->client->ps.saber[0].blade[0].active)
			{
				//turn on first one
				self->client->ps.SaberBladeActivate(0, 0);
			}
			for (int i = 1; i < self->client->ps.saber[0].numBlades; i++)
			{
				//turn off all others
				if (self->client->ps.saber[0].blade[i].active)
				{
					self->client->ps.SaberBladeActivate(0, i, qfalse);
				}
			}
		}
		else
		{
			//turn the sabers, all blades...?
			self->client->ps.saber[0].Activate();
			//self->client->ps.SaberActivate();
		}
		//turn on the saber trail
		self->client->ps.saber[0].ActivateTrail(150);
	}

	//reset the mins
	VectorCopy(saberMins, saber->mins);
	VectorCopy(saberMaxs, saber->maxs);
	saber->contents = 0; //CONTENTS_LIGHTSABER;
	saber->clipmask = MASK_SOLID | CONTENTS_LIGHTSABER;

	// remove the ghoul2 right-hand saber model on the player
	if (self->weaponModel[0] > 0)
	{
		gi.G2API_RemoveGhoul2Model(self->ghoul2, self->weaponModel[0]);
		self->weaponModel[0] = -1;
	}

	return qtrue;
}

void WP_SaberKnockedOutOfHand(const gentity_t* self, gentity_t* saber)
{
	//clear the enemy
	saber->enemy = nullptr;
	saber->s.eFlags &= ~EF_BOUNCE;
	saber->bounceCount = 0;
	//make it fall
	saber->s.pos.trType = TR_GRAVITY;
	//make it bounce some
	saber->s.eFlags |= EF_BOUNCE_HALF;
	//make it spin
	VectorCopy(saber->currentAngles, saber->s.apos.trBase);
	saber->s.apos.trType = TR_LINEAR;
	saber->s.apos.trTime = level.time;

	VectorSet(saber->s.apos.trDelta, Q_irand(-300, 300), saber->s.apos.trDelta[1], Q_irand(-300, 300));

	if (!saber->s.apos.trDelta[1])
	{
		saber->s.apos.trDelta[1] = Q_irand(-300, 300);
	}
	//force it to be ready to return
	self->client->ps.saberEntityDist = 0;

	if (self->client && level.time - self->client->ps.saberThrowTime <= MAX_DISARM_TIME)
	{
		self->client->ps.saberEntityState = SES_LEAVING;
	}
	else
	{
		self->client->ps.saberEntityState = SES_RETURNING;
	}
	//turn it off
	self->client->ps.saber[0].Deactivate();
	//turn off the saber trail
	self->client->ps.saber[0].DeactivateTrail(75);
	//play the saber turning off sound
	G_SoundIndexOnEnt(saber, CHAN_AUTO, self->client->ps.saber[0].soundOff);

	if (self->health <= 0)
	{
		//owner is dead!
		saber->s.time = level.time; //will make us free ourselves after a time
	}
}

qboolean saberShotOutOfHand(gentity_t* self, vec3_t throwDir)
{
	if (!self || !self->client || self->client->ps.saberEntityNum <= 0)
	{
		//WTF?!!  We lost it already?
		return qfalse;
	}

	if (self->client->NPC_class == CLASS_SABER_DROID)
	{
		//saber droids can't drop their saber
		return qfalse;
	}

	gentity_t* dropped = &g_entities[self->client->ps.saberEntityNum];

	if (!self->client->ps.saberInFlight)
	{
		//not already in air
		//throw it
		if (!WP_SaberLaunch(self, dropped, qfalse))
		{
			//couldn't throw it
			return qfalse;
		}
	}

	if (self->client->ps.saber[0].Active())
	{//drop it instantly
		WP_SaberKnockedOutOfHand(self, dropped);
		self->client->ps.ManualBlockingFlags &= ~(1 << HOLDINGBLOCK);
		self->client->ps.ManualBlockingFlags &= ~(1 << HOLDINGBLOCKANDATTACK);
		self->client->ps.ManualBlockingFlags &= ~(1 << PERFECTBLOCKING);
		self->client->ps.ManualBlockingFlags &= ~(1 << MBF_BLOCKWALKING);
		self->client->ps.userInt3 &= ~(1 << FLAG_BLOCKING);
		self->client->ps.ManualBlockingTime = 0; //Blocking time 1 on
		self->client->ps.ManualMBlockingTime = 0;
	}
	//optionally give it some thrown velocity
	if (throwDir && !VectorCompare(throwDir, vec3_origin))
	{
		VectorCopy(throwDir, dropped->s.pos.trDelta);
	}
	//don't pull it back on the next frame
	if (self->client && level.time - self->client->ps.saberThrowTime <= MAX_LEAVE_TIME)
	{
		self->client->usercmd.buttons &= ~BUTTON_ATTACK;
	}

	wp_block_points_regenerate_over_ride(self, BLOCKPOINTS_FATIGUE); //BP Reward blocker
	return qtrue;
}

qboolean WP_SaberDisarmed(gentity_t* self, vec3_t throwDir)
{
	if (!self || !self->client || self->client->ps.saberEntityNum <= 0)
	{
		//WTF?!!  We lost it already?
		return qfalse;
	}

	if (self->client->NPC_class == CLASS_SABER_DROID)
	{
		//saber droids can't drop their saber
		return qfalse;
	}

	gentity_t* dropped = &g_entities[self->client->ps.saberEntityNum];

	if (!self->client->ps.saberInFlight)
	{
		//not already in air
		//throw it
		if (!WP_SaberLaunch(self, dropped, qfalse))
		{
			//couldn't throw it
			return qfalse;
		}
	}

	if (self->client->ps.saber[0].Active())
	{//drop it instantly
		WP_SaberKnockedOutOfHand(self, dropped);
		self->client->ps.ManualBlockingFlags &= ~(1 << HOLDINGBLOCK);
		self->client->ps.ManualBlockingFlags &= ~(1 << HOLDINGBLOCKANDATTACK);
		self->client->ps.ManualBlockingFlags &= ~(1 << PERFECTBLOCKING);
		self->client->ps.ManualBlockingFlags &= ~(1 << MBF_BLOCKWALKING);
		self->client->ps.userInt3 &= ~(1 << FLAG_BLOCKING);
		self->client->ps.ManualBlockingTime = 0; //Blocking time 1 on
		self->client->ps.ManualMBlockingTime = 0;
	}
	//optionally give it some thrown velocity
	if (throwDir && !VectorCompare(throwDir, vec3_origin))
	{
		VectorCopy(throwDir, dropped->s.pos.trDelta);
	}
	//don't pull it back on the next frame
	if (self->client && level.time - self->client->ps.saberThrowTime <= MAX_LEAVE_TIME)
	{
		self->client->usercmd.buttons &= ~BUTTON_ATTACK;
	}
	return qtrue;
}

qboolean WP_SaberLose(gentity_t* self, vec3_t throwDir)
{
	if (!self || !self->client || self->client->ps.saberEntityNum <= 0)
	{
		//WTF?!!  We lost it already?
		return qfalse;
	}
	if (self->client->NPC_class == CLASS_SABER_DROID)
	{
		//saber droids can't drop their saber
		return qfalse;
	}
	if (self->client->ps.SaberActive())
	{
		self->client->ps.SaberDeactivate();
	}
	gentity_t* dropped = &g_entities[self->client->ps.saberEntityNum];

	if (!self->client->ps.saberInFlight)
	{
		//not already in air
		//throw it
		if (!WP_SaberLaunch(self, dropped, qfalse))
		{
			//couldn't throw it
			return qfalse;
		}
	}
	if (self->client->ps.saber[0].Active())
	{
		//on
		//drop it instantly
		WP_SaberDrop(self, dropped);
		//turn it off
		if (self->client->ps.SaberActive())
		{
			self->client->ps.SaberDeactivate();
		}
	}
	//optionally give it some thrown velocity
	if (throwDir && !VectorCompare(throwDir, vec3_origin))
	{
		VectorCopy(throwDir, dropped->s.pos.trDelta);
	}
	//don't pull it back on the next frame
	if (self->NPC)
	{
		self->NPC->last_ucmd.buttons &= ~BUTTON_ATTACK;
	}
	return qtrue;
}

void WP_SetSaberOrigin(gentity_t* self, vec3_t newOrg)
{
	if (!self || !self->client)
	{
		return;
	}
	if (self->client->ps.saberEntityNum <= 0 || self->client->ps.saberEntityNum >= ENTITYNUM_WORLD)
	{
		//no saber ent to reposition
		return;
	}
	if (self->client->NPC_class == CLASS_SABER_DROID)
	{
		//saber droids can't drop their saber
		return;
	}
	if (self->client->NPC_class == CLASS_SBD)
	{
		//saber droids can't drop their saber
		return;
	}
	gentity_t* dropped = &g_entities[self->client->ps.saberEntityNum];
	if (!self->client->ps.saberInFlight)
	{
		//not already in air
		qboolean noForceThrow = qfalse;
		//make it so we can throw it
		self->client->ps.forcePowersKnown |= 1 << FP_SABERTHROW;
		if (self->client->ps.forcePowerLevel[FP_SABERTHROW] < FORCE_LEVEL_1)
		{
			noForceThrow = qtrue;
			self->client->ps.forcePowerLevel[FP_SABERTHROW] = FORCE_LEVEL_1;
		}
		//throw it
		if (!WP_SaberLaunch(self, dropped, qfalse, qtrue))
		{
			//couldn't throw it
			return;
		}
		if (noForceThrow)
		{
			self->client->ps.forcePowerLevel[FP_SABERTHROW] = FORCE_LEVEL_0;
		}
	}
	VectorCopy(newOrg, dropped->s.origin);
	VectorCopy(newOrg, dropped->currentOrigin);
	VectorCopy(newOrg, dropped->s.pos.trBase);
	//drop it instantly
	WP_SaberDrop(self, dropped);
	//don't pull it back on the next frame
	if (self->NPC)
	{
		self->NPC->last_ucmd.buttons &= ~BUTTON_ATTACK;
	}
}

void WP_SaberCatch(gentity_t* self, gentity_t* saber, const qboolean switchToSaber)
{
	if (PM_SaberInBrokenParry(self->client->ps.saberMove) || self->client->ps.saberBlocked == BLOCKED_PARRY_BROKEN || self->NPC && level.time - self->client->ps.saberThrowTime < MAX_DISARM_TIME)
	{
		return;
	}

	if (self->health > 0
		&& !PM_SaberInBrokenParry(self->client->ps.saberMove)
		&& self->client->ps.saberBlocked != BLOCKED_PARRY_BROKEN)
	{
		//clear the enemy
		saber->enemy = nullptr;
		//===FIXME!!!==============================================================================================
		//We should copy the thrown saber's g2 instance to the right-hand saber
		//When you catch it, and vice-versa when you throw it!!!
		//===FIXME!!!==============================================================================================
		//don't draw it
		saber->s.eFlags |= EF_NODRAW;
		saber->svFlags &= SVF_BROADCAST;
		saber->svFlags |= SVF_NOCLIENT;

		//take off any gravity stuff if we'd dropped it
		saber->s.pos.trType = TR_LINEAR;
		saber->s.eFlags &= ~EF_BOUNCE_HALF;

		//Put it in my hand
		self->client->ps.saberInFlight = qfalse;
		self->client->ps.saberEntityState = SES_LEAVING;

		//turn off the saber trail
		self->client->ps.saber[0].DeactivateTrail(75);

		//reset its contents/clipmask
		saber->contents = CONTENTS_LIGHTSABER; // | CONTENTS_SHOTCLIP;
		saber->clipmask = MASK_SHOT | CONTENTS_LIGHTSABER;

		//play catch sound
		G_Sound(saber, G_SoundIndex("sound/weapons/saber/saber_catch.wav"));

		if (self->client->ps.weapon == WP_SABER)
		{
			//only do the first saber since we only throw the first one
			wp_saber_add_g2_saber_models(self, qfalse);

			if (self->s.number >= MAX_CLIENTS && !G_ControlledByPlayer(self)) //NPC only
			{
				if (self->client->ps.saberFatigueChainCount >= MISHAPLEVEL_TEN)
				{
					self->client->ps.saberFatigueChainCount = MISHAPLEVEL_LIGHT;
				}
				wp_block_points_regenerate(self, BLOCKPOINTS_TWENTYFIVE);
				wp_force_power_regenerate(self, BLOCKPOINTS_TWENTYFIVE);
			}
			else
			{
				if (self->client->ps.saberFatigueChainCount >= MISHAPLEVEL_HUDFLASH)
				{
					self->client->ps.saberFatigueChainCount = MISHAPLEVEL_LIGHT;
				}
			}
		}

		if (switchToSaber)
		{
			if (self->client->ps.weapon != WP_SABER)
			{
				CG_ChangeWeapon(WP_SABER);
			}
			else
			{
				//if it's not active, turn it on
				if (self->client->ps.saber[0].saberFlags & SFL_SINGLE_BLADE_THROWABLE)
				{
					//only first blade can be on
					if (!self->client->ps.saber[0].blade[0].active)
					{
						//only turn it on if first blade is off, otherwise, leave as-is
						if (self->NPC && level.time - self->client->ps.saberThrowTime < MAX_DISARM_TIME)
						{
							//
						}
						else
						{
							self->client->ps.saber[0].Activate();
						}
					}
				}
				else
				{
					//turn all blades on
					if (self->NPC && level.time - self->client->ps.saberThrowTime < MAX_DISARM_TIME)
					{
						//
					}
					else
					{
						self->client->ps.saber[0].Activate();
					}
				}
			}
		}
	}
}

void WP_SaberReturn(const gentity_t* self, gentity_t* saber)
{
	const qboolean ActiveBlocking = self->client->ps.ManualBlockingFlags & 1 << HOLDINGBLOCKANDATTACK ? qtrue : qfalse;
	const qboolean HoldingBlock = self->client->ps.ManualBlockingFlags & 1 << HOLDINGBLOCK ? qtrue : qfalse;
	//Normal Blocking

	if (PM_SaberInBrokenParry(self->client->ps.saberMove) || self->client->ps.saberBlocked == BLOCKED_PARRY_BROKEN || self->NPC && level.time - self->client->ps.saberThrowTime < MAX_DISARM_TIME)
	{
		return;
	}
	if (self->s.number < MAX_CLIENTS || G_ControlledByPlayer(self))
	{
		if (/*self->client->ps.forcePower < BLOCKPOINTS_FIVE || self->client->ps.blockPoints < BLOCKPOINTS_TWELVE ||*/
			HoldingBlock || ActiveBlocking || self->client->buttons & BUTTON_BLOCK)
		{
			WP_SaberDrop(self, saber);
			return;
		}
	}
	if (self && self->client)
	{
		//still alive and stuff
		self->client->ps.saberEntityState = SES_RETURNING;
		//turn down the saber trail
		if (!(self->client->ps.saber[0].saberFlags & SFL_RETURN_DAMAGE))
		{
			self->client->ps.saber[0].DeactivateTrail(75);
		}
	}
	if (!(saber->s.eFlags & EF_BOUNCE))
	{
		saber->s.eFlags |= EF_BOUNCE;
		saber->bounceCount = 300;
	}
}

void WP_SaberDrop(const gentity_t* self, gentity_t* saber)
{
	//clear the enemy
	saber->enemy = nullptr;
	saber->s.eFlags &= ~EF_BOUNCE;
	saber->bounceCount = 0;
	//make it fall
	saber->s.pos.trType = TR_GRAVITY;
	//make it bounce some
	saber->s.eFlags |= EF_BOUNCE_HALF;
	//make it spin
	VectorCopy(saber->currentAngles, saber->s.apos.trBase);
	saber->s.apos.trType = TR_LINEAR;
	saber->s.apos.trTime = level.time;
	VectorSet(saber->s.apos.trDelta, Q_irand(-300, 300), saber->s.apos.trDelta[1], Q_irand(-300, 300));
	if (!saber->s.apos.trDelta[1])
	{
		saber->s.apos.trDelta[1] = Q_irand(-300, 300);
	}
	//force it to be ready to return
	self->client->ps.saberEntityDist = 0;
	self->client->ps.saberEntityState = SES_RETURNING;
	//turn it off
	if (self->client->ps.SaberActive())
	{
		self->client->ps.SaberDeactivate();
	}
	//turn off the saber trail
	self->client->ps.saber[0].DeactivateTrail(75);
	//play the saber turning off sound
	G_SoundIndexOnEnt(saber, CHAN_AUTO, self->client->ps.saber[0].soundOff);

	if (self->health <= 0)
	{
		//owner is dead!
		saber->s.time = level.time; //will make us free ourselves after a time
	}
}

void WP_SaberPull(const gentity_t* self, gentity_t* saber)
{
	if (PM_SaberInBrokenParry(self->client->ps.saberMove) || self->client->ps.saberBlocked == BLOCKED_PARRY_BROKEN || self->NPC && level.time - self->client->ps.saberThrowTime < MAX_DISARM_TIME)
	{
		return;
	}

	if (self->s.number >= MAX_CLIENTS && !G_ControlledByPlayer(self)) //NPC only
	{
		if (self->client->ps.saberFatigueChainCount >= MISHAPLEVEL_LIGHT)
		{
			self->client->ps.saberFatigueChainCount = MISHAPLEVEL_MIN;
		}
	}
	else
	{
		if (self->client->ps.saberFatigueChainCount >= MISHAPLEVEL_HUDFLASH)
		{
			self->client->ps.saberFatigueChainCount = MISHAPLEVEL_LIGHT;
		}
	}
	if (self->health > 0)
	{
		//take off gravity
		saber->s.pos.trType = TR_LINEAR;
		//take off bounce
		saber->s.eFlags &= EF_BOUNCE_HALF;
		//play sound
		G_Sound(self, G_SoundIndex("sound/weapons/force/pull.wav"));
	}
}

void WP_SaberGrab(const gentity_t* self, gentity_t* saber)
{
	const qboolean ActiveBlocking = self->client->ps.ManualBlockingFlags & 1 << HOLDINGBLOCKANDATTACK ? qtrue : qfalse;
	const qboolean HoldingBlock = self->client->ps.ManualBlockingFlags & 1 << HOLDINGBLOCK ? qtrue : qfalse;
	//Normal Blocking

	if (PM_SaberInBrokenParry(self->client->ps.saberMove) || self->client->ps.saberBlocked == BLOCKED_PARRY_BROKEN)
	{
		return;
	}
	if (self->s.number < MAX_CLIENTS || G_ControlledByPlayer(self))
	{
		if (HoldingBlock || ActiveBlocking)
		{
			return;
		}
	}

	if (self->health > 0)
	{
		//take off gravity
		saber->s.pos.trType = TR_LINEAR;
		//take off bounce
		saber->s.eFlags &= EF_BOUNCE_HALF;
		//play sound
		G_Sound(self, G_SoundIndex("sound/weapons/force/pull.wav"));
	}
}

const char* saberColorStringForColor[SABER_LIME + 1] =
{
	"red", //SABER_RED
	"orange", //SABER_ORANGE
	"yellow", //SABER_YELLOW
	"green", //SABER_GREEN
	"blue", //SABER_BLUE
	"purple", //SABER_PURPLE
	// Custom saber glow, blade & dlight color code
	"custom", //SABER_CUSTOM
	"lime" //SABER_LIME
};

// Check if we are throwing it, launch it if needed, update position if needed.
void WP_SaberThrow(gentity_t* self, const usercmd_t* ucmd)
{
	vec3_t saber_diff;
	trace_t tr;

	if (self->client->ps.saberEntityNum <= 0 || self->client->ps.saberEntityNum >= ENTITYNUM_WORLD)
	{
		//WTF?!!  We lost it?
		return;
	}

	if (self->client->ps.torsoAnim == BOTH_LOSE_SABER || PM_SaberInMassiveBounce(self->client->ps.torsoAnim))
	{
		//can't catch it while it's being yanked from your hand!
		return;
	}

	if (!g_saberNewControlScheme->integer)
	{
		if (PM_SaberInKata(static_cast<saberMoveName_t>(self->client->ps.saberMove)))
		{
			//don't throw saber when in special attack (alt+attack)
			return;
		}
		if (ucmd->buttons & BUTTON_ATTACK
			&& ucmd->buttons & BUTTON_ALT_ATTACK
			&& !self->client->ps.saberInFlight)
		{
			//trying to do special attack, don't throw it
			return;
		}
		if (self->client->ps.torsoAnim == BOTH_A1_SPECIAL
			|| self->client->ps.torsoAnim == BOTH_A2_SPECIAL
			|| self->client->ps.torsoAnim == BOTH_A3_SPECIAL
			|| self->client->ps.torsoAnim == BOTH_A4_SPECIAL
			|| self->client->ps.torsoAnim == BOTH_A5_SPECIAL
			|| self->client->ps.torsoAnim == BOTH_GRIEVOUS_SPIN
			|| self->client->ps.torsoAnim == BOTH_YODA_SPECIAL)
		{
			//don't throw in these anims!
			return;
		}
	}
	gentity_t* saberent = &g_entities[self->client->ps.saberEntityNum];

	VectorSubtract(self->client->renderInfo.handRPoint, saberent->currentOrigin, saber_diff);

	//is our saber in flight?
	if (!self->client->ps.saberInFlight)
	{
		//saber is not in flight right now
		if (self->client->ps.weapon != WP_SABER)
		{
			//don't even have it out
			return;
		}
		if (ucmd->buttons & BUTTON_ALT_ATTACK && !(self->client->ps.pm_flags & PMF_ALT_ATTACK_HELD))
		{
			//still holding it, not still holding attack from a previous throw, so throw it.
			if (!(self->client->ps.saberEventFlags & SEF_INWATER) && WP_SaberLaunch(self, saberent, qtrue))
			{
				if (self->client && !self->s.number)
				{
					self->client->sess.missionStats.saberThrownCnt++;
				}
				//need to recalc this because we just moved it
				VectorSubtract(self->client->renderInfo.handRPoint, saberent->currentOrigin, saber_diff);
			}
			else
			{
				//couldn't throw it
				return;
			}
		}
		else
		{
			//holding it, don't want to throw it, go away.
			return;
		}
	}
	else
	{
		//inflight
		//is our saber currently on it's way back to us?
		if (self->client->ps.saberEntityState == SES_RETURNING)
		{
			//see if we're close enough to pick it up
			if (VectorLengthSquared(saber_diff) <= 256)
			{
				//caught it
				vec3_t axis_point;
				trace_t trace;
				VectorCopy(self->currentOrigin, axis_point);
				axis_point[2] = self->client->renderInfo.handRPoint[2];
				gi.trace(&trace, axis_point, vec3_origin, vec3_origin, self->client->renderInfo.handRPoint,
					self->s.number, MASK_SOLID, static_cast<EG2_Collision>(0), 0);

				if (!trace.startsolid && trace.fraction >= 1.0f)
				{
					//our hand isn't through a wall
					WP_SaberCatch(self, saberent, qtrue);
				}
				return;
			}
		}

		if (saberent->s.pos.trType != TR_STATIONARY)
		{
			//saber is in flight, lerp it
			if (self->health <= 0)
			{
				//make us free ourselves after a time
				if (g_saberPickuppableDroppedSabers->integer
					&& G_DropSaberItem(self->client->ps.saber[0].name, self->client->ps.saber[0].blade[0].color,
						saberent->currentOrigin, saberent->s.pos.trDelta,
						saberent->currentAngles) != nullptr)
				{
					//dropped it
					G_FreeEntity(saberent);
					//forget it
					self->client->ps.saberEntityNum = ENTITYNUM_NONE;
					return;
				}
			}
			WP_RunSaber(self, saberent);
		}
		else
		{
			//it fell on the ground
			if (self->health <= 0)
			{
				//make us free ourselves after a time
				if (g_saberPickuppableDroppedSabers->integer)
				{
					//spawn an item
					G_DropSaberItem(self->client->ps.saber[0].name, self->client->ps.saber[0].blade[0].color,
						saberent->currentOrigin, saberent->s.pos.trDelta, saberent->currentAngles);
				}
				//free it
				G_FreeEntity(saberent);
				//forget it
				self->client->ps.saberEntityNum = ENTITYNUM_NONE;
				return;
			}
			if (!self->s.number && level.time - saberent->aimDebounceTime > 15000
				|| self->s.number && level.time - saberent->aimDebounceTime > 5000)
			{
				//(only for player) been missing for 15 seconds, automatically return
				WP_SaberCatch(self, saberent, qfalse);
				return;
			}
		}
	}

	//are we still trying to use the saber?
	if (self->client->ps.weapon != WP_SABER)
	{
		//switched away
		if (!self->client->ps.saberInFlight)
		{
			//wasn't throwing saber
			return;
		}
		if (saberent->s.pos.trType == TR_LINEAR)
		{
			//switched away while controlling it, just drop the saber
			WP_SaberDrop(self, saberent);
			return;
		}
		//it's on the ground, see if it's inside us (touching)
		if (G_PointInBounds(saberent->currentOrigin, self->absmin, self->absmax))
		{
			//it's in us, pick it up automatically
			if (self->NPC && !G_ControlledByPlayer(self))
			{
				if (level.time - self->client->ps.saberThrowTime >= MAX_LEAVE_TIME &&
					!PM_SaberInBrokenParry(self->client->ps.saberMove)
					&& self->client->ps.saberBlocked != BLOCKED_PARRY_BROKEN)
				{
					WP_SaberPull(self, saberent);
				}
			}
			else
			{
				if (level.time - self->client->ps.saberThrowTime >= MAX_RETURN_TIME
					&& !PM_SaberInBrokenParry(self->client->ps.saberMove)
					&& self->client->ps.saberBlocked != BLOCKED_PARRY_BROKEN
					&& !PM_SaberInMassiveBounce(self->client->ps.torsoAnim)
					&& !PM_SaberInBashedAnim(self->client->ps.torsoAnim))
				{
					WP_SaberGrab(self, saberent);
				}
			}
		}
	}
	else if (saberent->s.pos.trType != TR_LINEAR)
	{
		//weapon is saber and not flying
		if (self->client->ps.saberInFlight)
		{
			//we dropped it
			if (ucmd->buttons & BUTTON_ATTACK)
			{
				//we actively want to pick it up or we just switched to it, so pull it back
				gi.trace(&tr, saberent->currentOrigin, saberent->mins, saberent->maxs,
					self->client->renderInfo.handRPoint, self->s.number, MASK_SOLID, static_cast<EG2_Collision>(0),
					0);

				if (tr.allsolid || tr.startsolid || tr.fraction < 1.0f)
				{
					//can't pick it up yet, no LOS
					return;
				}
				//clear LOS, pick it up
				if (self->NPC && !G_ControlledByPlayer(self))
				{
					if (level.time - self->client->ps.saberThrowTime >= MAX_LEAVE_TIME &&
						!PM_SaberInBrokenParry(self->client->ps.saberMove) && self->client->ps.saberBlocked !=
						BLOCKED_PARRY_BROKEN)
					{
						WP_SaberPull(self, saberent);
					}
				}
				else
				{
					if (level.time - self->client->ps.saberThrowTime >= MAX_RETURN_TIME
						&& !PM_SaberInBrokenParry(self->client->ps.saberMove)
						&& self->client->ps.saberBlocked != BLOCKED_PARRY_BROKEN
						&& !PM_SaberInMassiveBounce(self->client->ps.torsoAnim)
						&& !PM_SaberInBashedAnim(self->client->ps.torsoAnim))
					{
						WP_SaberGrab(self, saberent);
						if (self->client->ps.blockPoints < BLOCKPOINTS_TWELVE)
						{
							wp_block_points_regenerate(self, BLOCKPOINTS_FATIGUE);
						}
					}
				}
			}
			else
			{
				//see if it's inside us (touching)
				if (G_PointInBounds(saberent->currentOrigin, self->absmin, self->absmax))
				{
					//it's in us, pick it up automatically
					if (self->NPC && !G_ControlledByPlayer(self))
					{
						if (level.time - self->client->ps.saberThrowTime >= MAX_LEAVE_TIME &&
							!PM_SaberInBrokenParry(self->client->ps.saberMove) && self->client->ps.saberBlocked !=
							BLOCKED_PARRY_BROKEN)
						{
							WP_SaberPull(self, saberent);
						}
					}
					else
					{
						if (level.time - self->client->ps.saberThrowTime >= MAX_RETURN_TIME
							&& !PM_SaberInBrokenParry(self->client->ps.saberMove)
							&& self->client->ps.saberBlocked != BLOCKED_PARRY_BROKEN
							&& !PM_SaberInMassiveBounce(self->client->ps.torsoAnim)
							&& !PM_SaberInBashedAnim(self->client->ps.torsoAnim))
						{
							WP_SaberGrab(self, saberent);
						}
					}
				}
			}
		}
	}
	else if (self->health <= 0 && self->client->ps.saberInFlight)
	{
		//we died, drop it
		WP_SaberDrop(self, saberent);
		return;
	}
	else if (!self->client->ps.saber[0].Active() && self->client->ps.saberEntityState != SES_RETURNING)
	{
		//we turned it off, drop it
		WP_SaberDrop(self, saberent);
		return;
	}

	if (saberent->s.pos.trType != TR_LINEAR)
	{
		//don't home
		return;
	}

	const float saberDist = VectorLength(saber_diff);

	if (self->client->ps.saberEntityState == SES_LEAVING)
	{
		//saber still flying forward
		if (self->client->ps.forcePowerLevel[FP_SABERTHROW] > FORCE_LEVEL_2)
		{
			//still holding it out
			if (!(ucmd->buttons & BUTTON_ALT_ATTACK) && !(ucmd->buttons & BUTTON_BLOCK) && self->client->ps.
				forcePowerDebounce[FP_SABERTHROW] < level.time)
			{
				//done throwing, return to me
				if (self->client->ps.saber[0].Active())
				{
					//still on
					WP_SaberReturn(self, saberent);
				}
			}
			else if (level.time - self->client->ps.saberThrowTime >= 100)
			{
				if (WP_ForcePowerAvailable(self, FP_SABERTHROW, 10))
				{
					WP_ForcePowerDrain(self, FP_SABERTHROW, 1);
					self->client->ps.saberThrowTime = level.time;
				}
				else
				{
					//out of force power, drop it
					WP_SaberDrop(self, saberent);
				}
			}
		}
		else
		{
			if (!(ucmd->buttons & BUTTON_ALT_ATTACK) && self->client->ps.forcePowerDebounce[FP_SABERTHROW] < level.time)
			{
				//not holding button and has been out at least 1 second, drop it
				if (self->client->ps.saber[0].Active())
				{
					//still on
					WP_SaberDrop(self, saberent);
				}
			}
			else if (level.time - self->client->ps.saberThrowTime > 3000
				|| self->client->ps.forcePowerLevel[FP_SABERTHROW] == FORCE_LEVEL_1 && saberDist >= self->client->ps.
				saberEntityDist)
			{
				//been out too long, or saber throw 1 went too far, return to me
				if (self->client->ps.saber[0].Active())
				{
					//still on
					WP_SaberDrop(self, saberent);
				}
			}
		}
	}
	if (self->client->ps.saberEntityState == SES_RETURNING)
	{
		if (self->client->ps.saberEntityDist > 0)
		{
			self->client->ps.saberEntityDist -= 25;
		}
		if (self->client->ps.saberEntityDist < 0)
		{
			self->client->ps.saberEntityDist = 0;
		}
		else if (saberDist < self->client->ps.saberEntityDist)
		{
			//if it's coming back to me, never push it away
			self->client->ps.saberEntityDist = saberDist;
		}
	}
}

//SABER BLOCKING============================================================================
//SABER BLOCKING============================================================================
//SABER BLOCKING============================================================================
//SABER BLOCKING============================================================================
//SABER BLOCKING============================================================================
int WP_MissileBlockForBlock(int saber_block)
{
	switch (saber_block)
	{
	case BLOCKED_UPPER_RIGHT:
		return BLOCKED_UPPER_RIGHT_PROJ;
	case BLOCKED_UPPER_LEFT:
		return BLOCKED_UPPER_LEFT_PROJ;
	case BLOCKED_LOWER_RIGHT:
		return BLOCKED_LOWER_RIGHT_PROJ;
	case BLOCKED_LOWER_LEFT:
		return BLOCKED_LOWER_LEFT_PROJ;
	case BLOCKED_TOP:
		return BLOCKED_TOP_PROJ;
	case BLOCKED_FRONT:
		return BLOCKED_FRONT_PROJ;
	case BLOCKED_BACK:
		return BLOCKED_BACK;
	default:;
	}
	return saber_block;
}

float VectorBlockDistance(vec3_t v1, vec3_t v2)
{
	//returns the distance between the two points.
	vec3_t dir;

	VectorSubtract(v2, v1, dir);
	return VectorLength(dir);
}

int BasicWeaponBlockCosts[NUM_MODS] =
{
	-1, //MOD_UNKNOWN,
	20, //MOD_SABER,
	20, //MOD_BRYAR,
	10, //MOD_BRYAR_ALT,
	20, //MOD_BLASTER,
	20, //MOD_BLASTER_ALT,
	20, //MOD_DISRUPTOR,
	20, //MOD_SNIPER,
	20, //MOD_BOWCASTER,
	20, //MOD_BOWCASTER_ALT,
	20, //MOD_REPEATER,
	20, //MOD_REPEATER_ALT,
	20, //MOD_DEMP2,
	20, //MOD_DEMP2_ALT,
	20, //MOD_FLECHETTE,
	20, //MOD_FLECHETTE_ALT,
	20, //MOD_ROCKET,
	20, //MOD_ROCKET_ALT,
	20, //MOD_CONC,
	20, //MOD_CONC_ALT,
	20, //MOD_THERMAL,
	20, //MOD_THERMAL_ALT,
	20, //MOD_DETPACK,
	20, //MOD_LASERTRIP,
	20, //MOD_LASERTRIP_ALT,
	-1, //MOD_MELEE,
	20, //MOD_SEEKER,
	-1, //MOD_FORCE_GRIP,
	20, //MOD_FORCE_LIGHTNING,
	-1, //MOD_FORCE_DRAIN,
	20, //MOD_EMPLACED,
	20, //MOD_ELECTROCUTE,
	20, //MOD_EXPLOSIVE,
	20, //MOD_EXPLOSIVE_SPLASH,
	-1, //MOD_KNOCKOUT,
	-1, //MOD_ENERGY,
	-1, //MOD_ENERGY_SPLASH,
	-1, //MOD_WATER,
	-1, //MOD_SLIME,
	-1, //MOD_LAVA,
	-1, //MOD_CRUSH,
	-1, //MOD_IMPACT,
	-1, //MOD_FALLING,
	-1, //MOD_SUICIDE,
	-1, //MOD_TRIGGER_HURT,
	-1, //MOD_GAS,
	20, //MOD_HEADSHOT,
	20, //MOD_BODYSHOT,
	-1, //MOD_TEAM_CHANGE,
	-1, //MOD_DESTRUCTION,
	-1, //MOD_BURNING,
	20, //MOD_PISTOL,
	10, //MOD_PISTOL_ALT,
	20, //MOD_LIGHTNING_STRIKE,

	//NUM_MODS,
};

int BasicSaberBlockCost(int attackerStyle)
{
	//returns the basic saber block cost of blocking an attack from the given saber style.
	switch (attackerStyle)
	{
	case SS_DUAL:
		return 13;
	case SS_STAFF:
		return 13;
	case SS_TAVION:
		return 14;
	case SS_FAST:
		return 12;
	case SS_MEDIUM:
		return 15;
	case SS_DESANN:
		return 16;
	case SS_STRONG:
		return 17;
	default:
		return 0;
	}
}

qboolean IsMoving(const gentity_t* ent)
{
	if (!ent || !ent->client)
		return qfalse;

	if (ent->client->pers.cmd.upmove == 0
		&& ent->client->pers.cmd.forwardmove == 0
		&& ent->client->pers.cmd.rightmove == 0)
		return qfalse;

	return qtrue;
}

int WP_SaberBlockCost(gentity_t* defender, const gentity_t* attacker, vec3_t hit_loc)
{
	//returns the cost to block this attack for this attacker/defender combo.
	float saber_block_cost;
	//===========================
	// Determine Base Block Cost
	//===========================

	if (!attacker //don't have attacker
		|| !attacker->client //attacker isn't a NPC/player
		|| attacker->client->ps.weapon != WP_SABER) //or the player that is attacking isn't using a saber
	{
		//standard bolt block!
		saber_block_cost = DODGE_BOLTBLOCK;

		if (attacker && attacker->client && attacker->activator && attacker->activator->s.weapon == WP_SBD_PISTOL)
		{
			saber_block_cost = 4;
		}

		if (attacker && attacker->client && attacker->activator && attacker->activator->s.weapon == WP_BRYAR_PISTOL)
		{
			saber_block_cost = 4;
		}
		if (attacker->client && attacker->client->ps.weapon != WP_FLECHETTE)
		{
			saber_block_cost += 2;
		}

		if (defender->client->ps.forcePowersActive & 1 << FP_SPEED)
		{
			saber_block_cost += 10; //Using force speed
		}
		if (PM_RunningAnim(defender->client->ps.legsAnim))
		{
			saber_block_cost += 5; //Running
		}
		if (walk_check(defender) && IsMoving(defender))
		{
			saber_block_cost++; //Walking
		}
		if (defender->client->ps.groundEntityNum == ENTITYNUM_NONE)
		{
			saber_block_cost += 10;
		}

		if (attacker->activator && attacker->activator->s.weapon == WP_BRYAR_PISTOL
			|| attacker->activator && attacker->activator->s.weapon == WP_SBD_PISTOL
			|| attacker->activator && attacker->activator->s.weapon == WP_BLASTER_PISTOL
			|| attacker->activator && attacker->activator->s.weapon == WP_REPEATER
			|| attacker->activator && attacker->activator->s.weapon == WP_BOWCASTER
			|| attacker->activator && attacker->activator->s.weapon == WP_DISRUPTOR
			|| attacker->activator && attacker->activator->s.weapon == WP_EMPLACED_GUN
			|| attacker->activator && attacker->activator->s.weapon == WP_FLECHETTE)
		{
			if (attacker->activator->s.weapon == WP_FLECHETTE)
			{
				const float distance = VectorBlockDistance(attacker->activator->currentOrigin, defender->currentOrigin);

				if (walk_check(defender))
				{
					saber_block_cost = 2;
				}
				else
				{
					saber_block_cost = 4;
				}

				if (defender->client->ps.forcePowerLevel[FP_SABER_OFFENSE] >= FORCE_LEVEL_3 && defender->client->ps.
					saberAnimLevel != SS_MEDIUM)
				{
					saber_block_cost++;
				}

				if (distance >= 200.0f)
				{
					saber_block_cost += 1.5;
				}
				saber_block_cost += 1;
			}
			else if (attacker->activator->s.weapon != WP_BOWCASTER)
			{
				const float distance = VectorBlockDistance(attacker->activator->currentOrigin, defender->currentOrigin);

				if (distance <= 125.0f)
				{
					saber_block_cost = DODGE_BOLTBLOCK * 3;
				}
				else if (distance <= 300.0f)
				{
					saber_block_cost = DODGE_BOLTBLOCK * 2;
				}
				else
				{
					saber_block_cost = DODGE_BOLTBLOCK;
				}

				if (attacker->activator->s.weapon == WP_REPEATER)
				{
					saber_block_cost = DODGE_REPEATERBLOCK;
				}
			}
			else if (attacker->activator->s.weapon == WP_DISRUPTOR)
			{
				saber_block_cost = 10;
			}
			else if (attacker->activator->s.weapon == WP_DEMP2)
			{
				saber_block_cost = DODGE_TUSKENBLOCK * 3;
			}
			else if (attacker->activator->s.weapon == WP_TUSKEN_RIFLE)
			{
				saber_block_cost = DODGE_TUSKENBLOCK * 3;
			}
			else
			{
				saber_block_cost = 5;
			}

			if (PM_SaberInAttack(defender->client->ps.saberMove) || PM_SaberInStart(defender->client->ps.saberMove))
			{
				if (attacker->activator->s.weapon == WP_FLECHETTE)
				{
					saber_block_cost = saber_block_cost * 1.5;
				}
				else
				{
					saber_block_cost = saber_block_cost * 2;
				}
			}

			if (attacker->activator->s.weapon == WP_BOWCASTER)
			{
				saber_block_cost = 3;
			}

			if (defender->client->ps.saberAnimLevel == SS_FAST)
			{
				saber_block_cost--;
			}
		}
		else if (attacker->activator && attacker->activator->s.weapon == WP_BLASTER)
		{
			saber_block_cost = 2;
		}
		else
		{
			saber_block_cost = DODGE_BOLTBLOCK;
		}
	}
	else if (attacker->client->ps.saberMove == LS_A_LUNGE
		|| attacker->client->ps.saberMove == LS_SPINATTACK
		|| attacker->client->ps.saberMove == LS_SPINATTACK_DUAL
		|| attacker->client->ps.saberMove == LS_SPINATTACK_GRIEV
		|| attacker->client->ps.saberMove == LS_GRIEVOUS_SPECIAL)
	{
		//lunge attacks
		saber_block_cost = .75 * BasicSaberBlockCost(attacker->client->ps.saberAnimLevel);
	}
	else if (attacker->client->ps.saberMove == LS_ROLL_STAB)
	{
		//roll stab
		saber_block_cost = 2 * BasicSaberBlockCost(attacker->client->ps.saberAnimLevel);
	}
	else if (attacker->client->ps.saberMove == LS_A_JUMP_T__B_)
	{
		//DFA moves
		saber_block_cost = 4 * BasicSaberBlockCost(attacker->client->ps.saberAnimLevel);
	}
	else if (attacker->client->ps.saberMove == LS_A_JUMP_PALP_)
	{
		//DFA moves
		saber_block_cost = 4 * BasicSaberBlockCost(attacker->client->ps.saberAnimLevel);
	}
	else if (attacker->client->ps.saberMove == LS_A_FLIP_STAB
		|| attacker->client->ps.saberMove == LS_A_FLIP_SLASH)
	{
		//flip stabs do more DP
		saber_block_cost = 2 * BasicSaberBlockCost(attacker->client->ps.saberAnimLevel);
	}
	else
	{
		//"normal" swing moves
		if (attacker->client->ps.userInt3 & 1 << FLAG_ATTACKFAKE)
		{
			//attacker is in an attack fake
			if (attacker->client->ps.saberAnimLevel == SS_STRONG
				&& !g_accurate_blocking(defender, attacker, hit_loc))
			{
				//Red does additional DP damage with attack fakes if they aren't parried.
				saber_block_cost = BasicSaberBlockCost(attacker->client->ps.saberAnimLevel) * 1.35;
			}
			else
			{
				saber_block_cost = BasicSaberBlockCost(attacker->client->ps.saberAnimLevel) * 1.25;
			}
		}
		else
		{
			//normal saber block
			saber_block_cost = BasicSaberBlockCost(attacker->client->ps.saberAnimLevel);
		}

		//add running damage bonus to normal swings but don't apply if the defender is slowbouncing
		if (!walk_check(attacker)
			&& !(defender->client->ps.userInt3 & 1 << FLAG_SLOWBOUNCE)
			&& !(defender->client->ps.userInt3 & 1 << FLAG_OLDSLOWBOUNCE))
		{
			if (attacker->client->ps.saberAnimLevel == SS_DUAL)
			{
				saber_block_cost *= 3.0;
			}
			else
			{
				saber_block_cost *= 1.5;
			}
		}
	}

	//======================
	// Block Cost Modifiers
	//======================

	if (attacker && attacker->client)
	{
		//attacker is a player so he must have just hit you with a saber blow.
		if (g_accurate_blocking(defender, attacker, hit_loc))
		{
			//parried this attack, cost is less
			if (defender->client->ps.saberAnimLevel == SS_FAST)
			{
				//blue parries cheaper
				saber_block_cost = saber_block_cost / 3.25;
			}
			else
			{
				saber_block_cost = saber_block_cost / 3;
			}
		}

		if (!in_front(attacker->client->ps.origin, defender->client->ps.origin, defender->client->ps.viewangles, -0.7f))
		{
			//player is behind us, costs more to block
			//staffs back block at normal cost.
			if (defender->client->ps.saberAnimLevel == SS_STAFF && defender->client->ps.forcePowerLevel[
				FP_SABER_DEFENSE] == FORCE_LEVEL_3)
			{
				// Having both staff and defense 3 allow no extra back hit damage
				saber_block_cost *= 1;
			}
			else if (defender->client->ps.saberAnimLevel == SS_STAFF && defender->client->ps.forcePowerLevel[
				FP_SABER_DEFENSE] == FORCE_LEVEL_2)
			{
				//level 2 defense lowers back damage more
				saber_block_cost *= 1.50;
			}
			else if (defender->client->ps.saberAnimLevel == SS_STAFF && defender->client->ps.forcePowerLevel[
				FP_SABER_DEFENSE] == FORCE_LEVEL_1)
			{
				//level 1 defense lowers back damage a bit
				saber_block_cost *= 1.75;
			}
			else
			{
				saber_block_cost *= 2;
			}
		}

		//clamp to body dodge cost since it wouldn't be fair to cost more than that.
		if (saber_block_cost > BasicWeaponBlockCosts[MOD_SABER])
		{
			saber_block_cost = BasicWeaponBlockCosts[MOD_SABER];
		}
	}
	if (PM_SaberInBrokenParry(defender->client->ps.saberMove))
	{
		//we're stunned/stumbling, increase DP cost
		saber_block_cost *= 1.5;
	}

	if (PM_KickingAnim(defender->client->ps.legsAnim))
	{
		//kicking
		saber_block_cost *= 1.5;
	}

	if (!walk_check(defender))
	{
		if (defender->NPC)
		{
			saber_block_cost *= 1.0;
		}
		else
		{
			saber_block_cost *= 2.5;
		}
	}
	if (defender->client->ps.groundEntityNum == ENTITYNUM_NONE)
	{
		//in mid-air
		if (defender->client->ps.saberAnimLevel == SS_DUAL) //Ataru's other perk much less cost for air hit
		{
			saber_block_cost *= .5;
		}
		else
		{
			saber_block_cost *= 2;
		}
	}
	if (defender->client->ps.saberBlockingTime > level.time)
	{
		//attempting to block something too soon after a saber bolt block
		saber_block_cost *= 2;
	}
	return static_cast<int>(saber_block_cost);
}

int WP_SaberBoltBlockCost(gentity_t* defender, const gentity_t* attacker)
{
	//returns the cost to block this attack for this attacker/defender combo.
	float saber_block_cost;
	//===========================
	// Determine Base Block Cost
	//===========================

	if (!attacker //don't have attacker
		|| !attacker->client //attacker isn't a NPC/player
		|| attacker->client->ps.weapon != WP_SABER) //or the player that is attacking isn't using a saber
	{
		//standard bolt block!
		saber_block_cost = DODGE_BOLTBLOCK;

		if (attacker && attacker->client && attacker->activator && attacker->activator->s.weapon == WP_SBD_PISTOL)
		{
			saber_block_cost = 4;
		}

		if (attacker && attacker->client && attacker->activator && attacker->activator->s.weapon == WP_BRYAR_PISTOL)
		{
			saber_block_cost = 4;
		}
		if (attacker->client && attacker->client->ps.weapon != WP_FLECHETTE)
		{
			saber_block_cost += 2;
		}

		if (defender->client->ps.forcePowersActive & 1 << FP_SPEED)
		{
			saber_block_cost += 10; //Using force speed
		}
		if (PM_RunningAnim(defender->client->ps.legsAnim))
		{
			saber_block_cost += 5; //Running
		}
		if (walk_check(defender) && IsMoving(defender))
		{
			saber_block_cost++; //Walking
		}
		if (defender->client->ps.groundEntityNum == ENTITYNUM_NONE)
		{
			saber_block_cost += 10;
		}

		if (attacker->activator && attacker->activator->s.weapon == WP_BRYAR_PISTOL
			|| attacker->activator && attacker->activator->s.weapon == WP_SBD_PISTOL
			|| attacker->activator && attacker->activator->s.weapon == WP_BLASTER_PISTOL
			|| attacker->activator && attacker->activator->s.weapon == WP_REPEATER
			|| attacker->activator && attacker->activator->s.weapon == WP_BOWCASTER
			|| attacker->activator && attacker->activator->s.weapon == WP_DISRUPTOR
			|| attacker->activator && attacker->activator->s.weapon == WP_EMPLACED_GUN
			|| attacker->activator && attacker->activator->s.weapon == WP_FLECHETTE)
		{
			if (attacker->activator->s.weapon == WP_FLECHETTE)
			{
				const float distance = VectorBlockDistance(attacker->activator->currentOrigin, defender->currentOrigin);

				if (walk_check(defender))
				{
					saber_block_cost = 2;
				}
				else
				{
					saber_block_cost = 4;
				}

				if (defender->client->ps.forcePowerLevel[FP_SABER_OFFENSE] >= FORCE_LEVEL_3 && defender->client->ps.
					saberAnimLevel != SS_MEDIUM)
				{
					saber_block_cost++;
				}

				if (distance >= 200.0f)
				{
					saber_block_cost += 1.5;
				}
				saber_block_cost += 1;
			}
			else if (attacker->activator->s.weapon != WP_BOWCASTER)
			{
				const float distance = VectorBlockDistance(attacker->activator->currentOrigin, defender->currentOrigin);

				if (distance <= 125.0f)
				{
					saber_block_cost = DODGE_BOLTBLOCK * 3;
				}
				else if (distance <= 300.0f)
				{
					saber_block_cost = DODGE_BOLTBLOCK * 2;
				}
				else
				{
					saber_block_cost = DODGE_BOLTBLOCK;
				}

				if (attacker->activator->s.weapon == WP_REPEATER)
				{
					saber_block_cost = DODGE_REPEATERBLOCK;
				}
			}
			else if (attacker->activator->s.weapon == WP_DISRUPTOR)
			{
				saber_block_cost = 10;
			}
			else if (attacker->activator->s.weapon == WP_DEMP2)
			{
				saber_block_cost = DODGE_TUSKENBLOCK * 3;
			}
			else if (attacker->activator->s.weapon == WP_TUSKEN_RIFLE)
			{
				saber_block_cost = DODGE_TUSKENBLOCK * 3;
			}
			else
			{
				saber_block_cost = 5;
			}

			if (PM_SaberInAttack(defender->client->ps.saberMove) || PM_SaberInStart(defender->client->ps.saberMove))
			{
				if (attacker->activator->s.weapon == WP_FLECHETTE)
				{
					saber_block_cost = saber_block_cost * 1.5;
				}
				else
				{
					saber_block_cost = saber_block_cost * 2;
				}
			}

			if (attacker->activator->s.weapon == WP_BOWCASTER)
			{
				saber_block_cost = 3;
			}

			if (defender->client->ps.saberAnimLevel == SS_FAST)
			{
				saber_block_cost--;
			}
		}
		else if (attacker->activator && attacker->activator->s.weapon == WP_BLASTER)
		{
			saber_block_cost = 2;
		}
		else
		{
			saber_block_cost = DODGE_BOLTBLOCK;
		}
	}
	else if (attacker->client->ps.saberMove == LS_A_LUNGE
		|| attacker->client->ps.saberMove == LS_SPINATTACK
		|| attacker->client->ps.saberMove == LS_SPINATTACK_DUAL
		|| attacker->client->ps.saberMove == LS_SPINATTACK_GRIEV
		|| attacker->client->ps.saberMove == LS_GRIEVOUS_SPECIAL)
	{
		//lunge attacks
		saber_block_cost = .75 * BasicSaberBlockCost(attacker->client->ps.saberAnimLevel);
	}
	else if (attacker->client->ps.saberMove == LS_ROLL_STAB)
	{
		//roll stab
		saber_block_cost = 2 * BasicSaberBlockCost(attacker->client->ps.saberAnimLevel);
	}
	else if (attacker->client->ps.saberMove == LS_A_JUMP_T__B_)
	{
		//DFA moves
		saber_block_cost = 4 * BasicSaberBlockCost(attacker->client->ps.saberAnimLevel);
	}
	else if (attacker->client->ps.saberMove == LS_A_JUMP_PALP_)
	{
		//DFA moves
		saber_block_cost = 4 * BasicSaberBlockCost(attacker->client->ps.saberAnimLevel);
	}
	else if (attacker->client->ps.saberMove == LS_A_FLIP_STAB
		|| attacker->client->ps.saberMove == LS_A_FLIP_SLASH)
	{
		//flip stabs do more DP
		saber_block_cost = 2 * BasicSaberBlockCost(attacker->client->ps.saberAnimLevel);
	}
	else
	{
		//"normal" swing moves
		if (attacker->client->ps.userInt3 & 1 << FLAG_ATTACKFAKE)
		{
			//attacker is in an attack fake
			saber_block_cost = BasicSaberBlockCost(attacker->client->ps.saberAnimLevel) * 1.25;
		}
		else
		{
			//normal saber block
			saber_block_cost = BasicSaberBlockCost(attacker->client->ps.saberAnimLevel);
		}

		//add running damage bonus to normal swings but don't apply if the defender is slowbouncing
		if (!walk_check(attacker)
			&& !(defender->client->ps.userInt3 & 1 << FLAG_SLOWBOUNCE)
			&& !(defender->client->ps.userInt3 & 1 << FLAG_OLDSLOWBOUNCE))
		{
			if (attacker->client->ps.saberAnimLevel == SS_DUAL)
			{
				saber_block_cost *= 3.0;
			}
			else
			{
				saber_block_cost *= 1.5;
			}
		}
	}

	//======================
	// Block Cost Modifiers
	//======================

	if (attacker && attacker->client)
	{
		//attacker is a player so he must have just hit you with a saber blow.
		if (!in_front(attacker->client->ps.origin, defender->client->ps.origin, defender->client->ps.viewangles, -0.7f))
		{
			//player is behind us, costs more to block
			//staffs back block at normal cost.
			if (defender->client->ps.saberAnimLevel == SS_STAFF && defender->client->ps.forcePowerLevel[
				FP_SABER_DEFENSE] == FORCE_LEVEL_3)
			{
				// Having both staff and defense 3 allow no extra back hit damage
				saber_block_cost *= 1;
			}
			else if (defender->client->ps.saberAnimLevel == SS_STAFF && defender->client->ps.forcePowerLevel[
				FP_SABER_DEFENSE] == FORCE_LEVEL_2)
			{
				//level 2 defense lowers back damage more
				saber_block_cost *= 1.50;
			}
			else if (defender->client->ps.saberAnimLevel == SS_STAFF && defender->client->ps.forcePowerLevel[
				FP_SABER_DEFENSE] == FORCE_LEVEL_1)
			{
				//level 1 defense lowers back damage a bit
				saber_block_cost *= 1.75;
			}
			else
			{
				saber_block_cost *= 2;
			}
		}

		//clamp to body dodge cost since it wouldn't be fair to cost more than that.
		if (saber_block_cost > BasicWeaponBlockCosts[MOD_SABER])
		{
			saber_block_cost = BasicWeaponBlockCosts[MOD_SABER];
		}
	}
	if (PM_SaberInBrokenParry(defender->client->ps.saberMove))
	{
		//we're stunned/stumbling, increase DP cost
		saber_block_cost *= 1.5;
	}

	if (PM_KickingAnim(defender->client->ps.legsAnim))
	{
		//kicking
		saber_block_cost *= 1.5;
	}

	if (!walk_check(defender))
	{
		if (defender->NPC)
		{
			saber_block_cost *= 1.0;
		}
		else
		{
			saber_block_cost *= 2.5;
		}
	}
	if (defender->client->ps.groundEntityNum == ENTITYNUM_NONE)
	{
		//in mid-air
		if (defender->client->ps.saberAnimLevel == SS_DUAL) //Ataru's other perk much less cost for air hit
		{
			saber_block_cost *= .5;
		}
		else
		{
			saber_block_cost *= 2;
		}
	}
	if (defender->client->ps.saberBlockingTime > level.time)
	{
		//attempting to block something too soon after a saber bolt block
		saber_block_cost *= 2;
	}
	return static_cast<int>(saber_block_cost);
}

int wp_saber_must_block(gentity_t* self, const gentity_t* atk, const qboolean check_b_box_block, vec3_t point, const int r_saber_num, const int r_blade_num)
{
	if (!self || !self->client || !atk)
	{
		return 0;
	}

	if (atk && atk->s.eType == ET_MISSILE
		&& (atk->s.weapon == WP_ROCKET_LAUNCHER ||
			atk->s.weapon == WP_THERMAL ||
			atk->s.weapon == WP_TRIP_MINE ||
			atk->s.weapon == WP_DET_PACK ||
			atk->methodOfDeath == MOD_REPEATER_ALT ||
			atk->methodOfDeath == MOD_CONC ||
			atk->methodOfDeath == MOD_CONC_ALT ||
			atk->methodOfDeath == MOD_REPEATER_ALT ||
			atk->methodOfDeath == MOD_FLECHETTE_ALT))
	{
		//can't block this stuff with a saber
		return 0;
	}

	if (self && self->client && self->client->MassiveBounceAnimTime > level.time)
	{
		return 0;
	}

	if (PM_SaberInMassiveBounce(self->client->ps.torsoAnim) || PM_SaberInBashedAnim(self->client->ps.torsoAnim))
	{
		return 0;
	}

	if (PM_InGrappleMove(self->client->ps.torsoAnim))
	{
		//you can't block while doing a melee move.
		return 0;
	}

	if (PM_SaberInBrokenParry(self->client->ps.saberMove))
	{
		//you've been stunned from a broken parry
		return 0;
	}

	if (PM_KickMove(self->client->ps.saberMove))
	{
		return 0;
	}

	if (BG_FullBodyTauntAnim(self->client->ps.torsoAnim))
	{
		return 0;
	}

	if (self->client->ps.weapon != WP_SABER
		|| self->client->ps.weapon == WP_NONE
		|| self->client->ps.weapon == WP_MELEE) //saber not here
	{
		return 0;
	}

	if (!self->client->ps.saberEntityNum || self->client->ps.saberInFlight)
	{
		//saber not currently in use or available, attempt to use our hands instead.
		return 0;
	}

	if (self->client->ps.weaponstate == WEAPON_RAISING || !(self->client->ps.ManualBlockingFlags & 1 << HOLDINGBLOCK))
	{
		if (self->s.number >= MAX_CLIENTS && !G_ControlledByPlayer(self))
		{
			//bots just randomly parry to make up for them not intelligently parrying.
			if (self->client->ps.weapon == WP_SABER && self->client->ps.SaberActive() && !self->client->ps.saberInFlight)
			{
				return 1;
			}
			return 0;
		}
		return 0;
	}

	if (!walk_check(self) && self->client->ps.forcePowersActive & 1 << FP_SPEED)
	{
		//can't block while running in force speed.
		return 0;
	}

	if (PM_InKnockDown(&self->client->ps))
	{
		//can't block while knocked down or getting up from knockdown.
		return 0;
	}

	if (atk && atk->client && atk->client->ps.weapon == WP_SABER)
	{
		//player is attacking with saber
		if (!BG_SaberInNonIdleDamageMove(&atk->client->ps))
		{
			//saber attacker isn't in a real damaging move
			if (self->s.number >= MAX_CLIENTS && !G_ControlledByPlayer(self))
			{
				//bots just randomly parry to make up for them not intelligently parrying.
				if (self->client->ps.weapon == WP_SABER && self->client->ps.SaberActive() && !self->client->ps.
					saberInFlight)
				{
					return 1;
				}
				return 0;
			}
			return 0;
		}

		if ((atk->client->ps.saberMove == LS_A_LUNGE
			|| atk->client->ps.saberMove == LS_SPINATTACK
			|| atk->client->ps.saberMove == LS_SPINATTACK_DUAL
			|| atk->client->ps.saberMove == LS_SPINATTACK_GRIEV
			|| atk->client->ps.saberMove == LS_GRIEVOUS_SPECIAL)
			&& self->client->ps.userInt3 & 1 << FLAG_FATIGUED)
		{
			//saber attacker, we can't block lunge attacks while fatigued.
			return 0;
		}

		if (PM_SuperBreakWinAnim(atk->client->ps.torsoAnim) && self->client->ps.blockPoints < BLOCKPOINTS_THIRTY)
		{
			//can't block super breaks when in critical fp.
			return 0;
		}

		if (!walk_check(self)
			&& (!in_front(atk->client->ps.origin, self->client->ps.origin, self->client->ps.viewangles, -0.9f)
				|| PM_SaberInAttack(self->client->ps.saberMove)
				|| PM_SaberInStart(self->client->ps.saberMove)))
		{
			//can't block saber swings while running and hit from behind or in swing
			if (self->s.number >= MAX_CLIENTS && !G_ControlledByPlayer(self))
			{
				//bots just randomly parry to make up for them not intelligently parrying.
				if (self->client->ps.weapon == WP_SABER && self->client->ps.SaberActive() && !self->client->ps.
					saberInFlight)
				{
					return 1;
				}
				return 0;
			}
			return 0;
		}
	}

	//check to see if we have the BLOCKPOINTS to do this.
	if (self->client->ps.blockPoints < WP_SaberBlockCost(self, atk, point))
	{
		if (self->s.number >= MAX_CLIENTS && !G_ControlledByPlayer(self))
		{
			//bots just randomly parry to make up for them not intelligently parrying.
			if (self->client->ps.weapon == WP_SABER && self->client->ps.SaberActive() && !self->client->ps.
				saberInFlight)
			{
				return 1;
			}
			return 0;
		}
		return 0;
	}

	// allow for blocking behind our backs
	if (!in_front(point, self->client->ps.origin, self->client->ps.viewangles, -0.9f))
	{
		return 1;
	}

	if (!check_b_box_block)
	{
		//don't do the additional checkBBoxBlock checks.  As such, we're safe to saber block.
		return 1;
	}

	if (atk && atk->client && atk->client->ps.weapon == WP_SABER && PM_SuperBreakWinAnim(atk->client->ps.torsoAnim))
	{
		//never box block saberlock super break wins, it looks weird.
		return 0;
	}

	if (VectorCompare(point, vec3_origin))
	{
		//no hit position given, can't do blade movement check.
		return 0;
	}

	if (atk && atk->client && r_saber_num != -1 && r_blade_num != -1)
	{
		vec3_t body_min;
		vec3_t body_max;
		vec3_t closest_body_point;
		vec3_t dir_to_body;
		vec3_t saber_move_dir;

		//player attacker, if they are here they're using their saber to attack.
		//Check to make sure that we only block the blade if it is moving towards the player

		//create a line seqment thru the center of the player.
		VectorCopy(self->client->ps.origin, body_min);
		VectorCopy(self->client->ps.origin, body_max);

		body_max[2] += self->maxs[2];
		body_min[2] -= self->mins[2];

		//find dirToBody
		G_FindClosestPointOnLineSegment(body_min, body_max, point, closest_body_point);

		VectorSubtract(closest_body_point, point, dir_to_body);

		//find current saber movement direction of the attacker
		VectorSubtract(atk->client->ps.saber[r_saber_num].blade[r_blade_num].muzzlePoint,
			atk->client->ps.saber[r_saber_num].blade[r_blade_num].muzzlePointOld, saber_move_dir);

		if (DotProduct(dir_to_body, saber_move_dir) < 0)
		{
			//saber is moving away from defender
			return 0;
		}
	}

	return 1;
}

qboolean Manual_Saberreadyanim(int anim)
{
	//check for saber block animation
	switch (anim)
	{
		//special case anims
	case BOTH_STAND1: //not really a saberstance anim, actually... "saber off" stance
	case BOTH_STAND2: //single-saber, medium style
	case BOTH_SABERFAST_STANCE: //single-saber, fast style
	case BOTH_SABERSLOW_STANCE: //single-saber, strong style
	case BOTH_SABERSTAFF_STANCE: //saber staff style
	case BOTH_SABERSTAFF_STANCE_IDLE: //saber staff style
	case BOTH_SABERDUAL_STANCE: //dual saber style
	case BOTH_SABERDUAL_STANCE_GRIEVOUS: //dual saber style
	case BOTH_SABERDUAL_STANCE_IDLE: //dual saber style
	case BOTH_SABERTAVION_STANCE: //tavion saberstyle
	case BOTH_SABERDESANN_STANCE: //desann saber style
	case BOTH_SABERSTANCE_STANCE: //similar to stand2
	case BOTH_SABERYODA_STANCE: //YODA saber style
	case BOTH_SABERBACKHAND_STANCE: //BACKHAND saber style
	case BOTH_SABERDUAL_STANCE_ALT: //alt dual
	case BOTH_SABERSTAFF_STANCE_ALT: //alt staff
	case BOTH_SABERSTANCE_STANCE_ALT: //alt idle
	case BOTH_SABEROBI_STANCE: //obiwan
	case BOTH_SABEREADY_STANCE: //ready
	case BOTH_SABER_REY_STANCE: //rey
		return qtrue;
	default:;
	}
	return qfalse;
}

qboolean is_holding_block_button(const gentity_t* defender)
{
	if (defender->client->ps.pm_flags & PMF_BLOCK_HELD)
	{
		return qtrue;
	}
	return qfalse;
}

float manual_saberblocking(const gentity_t* defender)
{
	if (defender->s.number >= MAX_CLIENTS && !G_ControlledByPlayer(defender)
		&& defender->client->ps.weapon != WP_SABER)
	{
		return qfalse;
	}

	if (PM_RestAnim(defender->client->ps.legsAnim))
	{
		return qfalse;
	}

	if (defender->s.eFlags & EF_FORCE_DRAINED || defender->s.eFlags & EF_FORCE_GRIPPED || defender->s.eFlags &
		EF_FORCE_GRABBED)
	{
		return qfalse;
	}

	if (!(defender->client->ps.forcePowersKnown & 1 << FP_SABER_DEFENSE))
	{
		//doesn't have saber defense
		return qfalse;
	}

	if (defender->client->ps.weapon != WP_SABER
		|| defender->client->ps.weapon == WP_NONE
		|| defender->client->ps.weapon == WP_MELEE) //saber not here
	{
		return qfalse;
	}

	if (defender->client->ps.weapon == WP_SABER
		&& defender->client->ps.saberInFlight)
	{
		//saber not currently in use or available.
		return qfalse;
	}

	if (defender->client->ps.weapon == WP_SABER
		&& !defender->client->ps.SaberActive())
	{
		//saber not currently in use or available.
		return qfalse;
	}

	if (defender->health <= 1
		|| BG_InKnockDown(defender->client->ps.legsAnim)
		|| BG_InKnockDown(defender->client->ps.torsoAnim)
		|| PM_InKnockDown(&defender->client->ps)
		|| BG_InFlipBack(defender->client->ps.torsoAnim)
		|| PM_InRoll(&defender->client->ps)
		|| PM_SaberInAttack(defender->client->ps.saberMove)
		|| pm_saber_in_special_attack(defender->client->ps.torsoAnim)
		|| PM_SpinningSaberAnim(defender->client->ps.torsoAnim)
		|| PM_SaberInReturn(defender->client->ps.saberMove)
		|| PM_SuperBreakLoseAnim(defender->client->ps.torsoAnim)
		|| PM_SuperBreakWinAnim(defender->client->ps.torsoAnim)
		|| pm_saber_in_special_attack(defender->client->ps.torsoAnim)
		|| PM_InSpecialJump(defender->client->ps.torsoAnim)
		|| PM_SaberInBounce(defender->client->ps.saberMove)
		|| PM_SaberInKnockaway(defender->client->ps.saberMove)
		|| PM_SaberInMassiveBounce(defender->client->ps.torsoAnim)
		|| PM_SaberInBashedAnim(defender->client->ps.torsoAnim)
		|| PM_SaberInBrokenParry(defender->client->ps.saberMove)
		|| defender->client->ps.groundEntityNum == ENTITYNUM_NONE
		|| defender->client->ps.blockPoints < BLOCKPOINTS_FIVE
		|| defender->client->ps.forcePower < BLOCKPOINTS_FIVE)
	{
		return qfalse;
	}

	if (defender->client->buttons & BUTTON_ALT_ATTACK ||
		defender->client->buttons & BUTTON_FORCE_LIGHTNING ||
		defender->client->buttons & BUTTON_USE_FORCE ||
		defender->client->buttons & BUTTON_FORCE_DRAIN ||
		defender->client->buttons & BUTTON_FORCEGRIP ||
		defender->client->buttons & BUTTON_DASH)
	{
		return qfalse;
	}

	if (SaberAttacking(defender) && (defender->s.number >= MAX_CLIENTS && !G_ControlledByPlayer(defender)))
	{
		//bots just randomly parry to make up for them not intelligently parrying.
		return qtrue;
	}

	if (!(defender->client->buttons & BUTTON_BLOCK))
	{
		if (defender->s.number >= MAX_CLIENTS && !G_ControlledByPlayer(defender) && defender->client->ps.weapon == WP_SABER)
		{
			//bots just randomly parry to make up for them not intelligently parrying.
			return qtrue;
		}
		return qfalse;
	}
	return qtrue;
}

float manual_running_and_saberblocking(const gentity_t* defender)
{
	if (defender->s.number >= MAX_CLIENTS && !G_ControlledByPlayer(defender)
		&& defender->client->ps.weapon != WP_SABER)
	{
		return qfalse;
	}

	if (!PM_RunningAnim(defender->client->ps.legsAnim))
	{
		return qfalse;
	}

	if (defender->client->buttons & BUTTON_ATTACK)
	{
		return qfalse;
	}

	if (defender->client->buttons & BUTTON_WALKING)
	{
		return qfalse;
	}

	if (PM_RestAnim(defender->client->ps.legsAnim))
	{
		return qfalse;
	}

	if (defender->s.eFlags & EF_FORCE_DRAINED || defender->s.eFlags & EF_FORCE_GRIPPED || defender->s.eFlags &
		EF_FORCE_GRABBED)
	{
		return qfalse;
	}

	if (!(defender->client->ps.forcePowersKnown & 1 << FP_SABER_DEFENSE))
	{
		//doesn't have saber defense
		return qfalse;
	}

	if (defender->health <= 1
		|| BG_InKnockDown(defender->client->ps.legsAnim)
		|| BG_InKnockDown(defender->client->ps.torsoAnim)
		|| PM_InRoll(&defender->client->ps)
		|| PM_SuperBreakLoseAnim(defender->client->ps.torsoAnim)
		|| PM_SuperBreakWinAnim(defender->client->ps.torsoAnim)
		|| pm_saber_in_special_attack(defender->client->ps.torsoAnim)
		|| PM_InSpecialJump(defender->client->ps.torsoAnim)
		|| PM_SaberInBounce(defender->client->ps.saberMove)
		|| PM_SaberInKnockaway(defender->client->ps.saberMove)
		|| PM_SaberInMassiveBounce(defender->client->ps.torsoAnim)
		|| PM_SaberInBashedAnim(defender->client->ps.torsoAnim)
		|| PM_SaberInBrokenParry(defender->client->ps.saberMove)
		|| defender->client->ps.groundEntityNum == ENTITYNUM_NONE
		|| defender->client->ps.blockPoints < BLOCKPOINTS_FIVE
		|| defender->client->ps.forcePower < BLOCKPOINTS_FIVE)
	{
		return qfalse;
	}

	if (defender->client->ps.weapon != WP_SABER
		|| defender->client->ps.weapon == WP_NONE
		|| defender->client->ps.weapon == WP_MELEE) //saber not here
	{
		return qfalse;
	}

	if (defender->client->ps.weapon == WP_SABER && defender->client->ps.saberInFlight)
	{
		//saber not currently in use or available, attempt to use our hands instead.
		return qfalse;
	}

	if (defender->client->buttons & BUTTON_ALT_ATTACK ||
		defender->client->buttons & BUTTON_FORCE_LIGHTNING ||
		defender->client->buttons & BUTTON_USE_FORCE ||
		defender->client->buttons & BUTTON_FORCE_DRAIN ||
		defender->client->buttons & BUTTON_FORCEGRIP)
	{
		return qfalse;
	}

	if (SaberAttacking(defender) && (defender->s.number >= MAX_CLIENTS && !G_ControlledByPlayer(defender)))
	{
		//bots just randomly parry to make up for them not intelligently parrying.
		return qtrue;
	}

	if (!(defender->client->buttons & BUTTON_BLOCK))
	{
		if (defender->s.number >= MAX_CLIENTS && !G_ControlledByPlayer(defender) && defender->client->ps.weapon ==
			WP_SABER)
		{
			//bots just randomly parry to make up for them not intelligently parrying.
			return qtrue;
		}
		return qfalse;
	}
	return qtrue;
}

qboolean manual_meleeblocking(const gentity_t* defender) //Is this guy blocking or not?
{
	if (defender->client->ps.weapon == WP_MELEE
		&& defender->client->buttons & BUTTON_WALKING
		&& defender->client->buttons & BUTTON_BLOCK
		&& !PM_KickMove(defender->client->ps.saberMove)
		&& !PM_KickingAnim(defender->client->ps.torsoAnim)
		&& !PM_KickingAnim(defender->client->ps.legsAnim)
		&& !PM_InRoll(&defender->client->ps)
		&& !PM_InKnockDown(&defender->client->ps)
		&& !(defender->client->ps.pm_flags & PMF_DUCKED))
	{
		return qtrue;
	}
	return qfalse;
}

qboolean manual_melee_dodging(const gentity_t* defender) //Is this guy dodgeing or not?
{
	if (defender->client->NPC_class == CLASS_BOBAFETT || defender->client->NPC_class == CLASS_MANDO)
	{
		return qfalse;
	}
	if (!PM_WalkingOrRunningAnim(defender->client->ps.legsAnim)
		&& defender->client->buttons & BUTTON_USE
		&& !(defender->client->buttons & BUTTON_WALKING)
		&& !(defender->client->buttons & BUTTON_BLOCK)
		&& !PM_KickMove(defender->client->ps.saberMove)
		&& !PM_KickingAnim(defender->client->ps.torsoAnim)
		&& !PM_KickingAnim(defender->client->ps.legsAnim)
		&& !PM_InRoll(&defender->client->ps)
		&& !PM_InKnockDown(&defender->client->ps)
		&& !(defender->client->ps.pm_flags & PMF_DUCKED))
	{
		return qtrue;
	}
	return qfalse;
}

float wp_block_force_chance(const gentity_t* defender)
{
	float block_factor = 1.0f;

	if (defender->s.number >= MAX_CLIENTS)
	{
		//bots just randomly parry to make up for them not intelligently parrying.
		if (defender->client->ps.forcePower <= BLOCKPOINTS_HALF)
		{
			switch (defender->client->ps.forcePowerLevel[FP_ABSORB])
			{
				// These actually make more sense to be separate from SABER blocking arcs.
			case FORCE_LEVEL_3:
				block_factor = 0.6f;
				break;
			case FORCE_LEVEL_2:
				block_factor = 0.5f;
				break;
			case FORCE_LEVEL_1:
				block_factor = 0.4f;
				break;
			default:
				block_factor = 0.1f;
				break;
			}
		}
		else
		{
			switch (defender->client->ps.forcePowerLevel[FP_ABSORB])
			{
				// These actually make more sense to be separate from SABER blocking arcs.
			case FORCE_LEVEL_3:
				block_factor = 0.9f;
				break;
			case FORCE_LEVEL_2:
				block_factor = 0.8f;
				break;
			case FORCE_LEVEL_1:
				block_factor = 0.7f;
				break;
			default:
				block_factor = 0.1f;
				break;
			}
		}
	}

	return block_factor;
}

float manual_forceblocking(const gentity_t* defender)
{
	const float block_factor = wp_block_force_chance(defender);

	if (defender->client->ps.forcePower <= BLOCKPOINTS_FATIGUE)
	{
		return qfalse;
	}

	if (!(defender->client->ps.forcePowersKnown & 1 << FP_ABSORB))
	{
		//doesn't have absorb
		return qfalse;
	}

	if (defender->health <= 1
		|| PM_InKnockDown(&defender->client->ps)
		|| PM_InRoll(&defender->client->ps)
		|| PM_SuperBreakLoseAnim(defender->client->ps.torsoAnim)
		|| PM_SuperBreakWinAnim(defender->client->ps.torsoAnim)
		|| pm_saber_in_special_attack(defender->client->ps.torsoAnim)
		|| PM_InSpecialJump(defender->client->ps.torsoAnim)
		|| defender->client->ps.groundEntityNum == ENTITYNUM_NONE
		|| !walk_check(defender)
		|| in_camera)
	{
		return qfalse;
	}

	if (!(defender->client->buttons & BUTTON_BLOCK))
	{
		if (defender->s.number >= MAX_CLIENTS && !G_ControlledByPlayer(defender))
		{
			//bots just randomly parry to make up for them not intelligently parrying.
			return block_factor;
		}
		return qfalse;
	}
	return qtrue;
}

float manual_npc_saberblocking(const gentity_t* defender)
{
	if (defender->s.number < MAX_CLIENTS || G_ControlledByPlayer(defender))
	{
		return qfalse;
	}

	if (BG_IsAlreadyinTauntAnim(defender->client->ps.torsoAnim))
	{
		return qfalse;
	}

	if (defender->NPC && !G_ControlledByPlayer(defender) && defender->client->ps.weapon != WP_SABER)
	{
		return qfalse;
	}

	if (PM_SaberInKata(static_cast<saberMoveName_t>(defender->client->ps.saberMove)))
	{
		return qfalse;
	}

	if (defender->s.eFlags & EF_FORCE_DRAINED || defender->s.eFlags & EF_FORCE_GRIPPED || defender->s.eFlags &
		EF_FORCE_GRABBED)
	{
		return qfalse;
	}

	if (defender->health <= 1
		|| BG_InKnockDown(defender->client->ps.legsAnim)
		|| BG_InKnockDown(defender->client->ps.torsoAnim)
		|| PM_InRoll(&defender->client->ps)
		|| PM_SuperBreakLoseAnim(defender->client->ps.torsoAnim)
		|| PM_SuperBreakWinAnim(defender->client->ps.torsoAnim)
		|| pm_saber_in_special_attack(defender->client->ps.torsoAnim)
		|| PM_InSpecialJump(defender->client->ps.torsoAnim)
		|| PM_SaberInBounce(defender->client->ps.saberMove)
		|| PM_SaberInKnockaway(defender->client->ps.saberMove)
		|| PM_SaberInMassiveBounce(defender->client->ps.torsoAnim)
		|| PM_SaberInBashedAnim(defender->client->ps.torsoAnim)
		|| PM_SaberInBrokenParry(defender->client->ps.saberMove)
		|| defender->client->ps.groundEntityNum == ENTITYNUM_NONE
		|| defender->client->ps.blockPoints < BLOCKPOINTS_FIVE)
	{
		return qfalse;
	}

	if (defender->client->ps.weapon != WP_SABER
		|| defender->client->ps.weapon == WP_NONE
		|| defender->client->ps.weapon == WP_MELEE) //saber not here
	{
		return qfalse;
	}

	if (defender->client->ps.weapon == WP_SABER
		&& defender->client->ps.saberInFlight)
	{
		//saber not currently in use or available, attempt to use our hands instead.
		return qfalse;
	}

	if (defender->client->ps.weapon == WP_SABER && !defender->client->ps.SaberActive())
	{
		//saber not currently in use or available, attempt to use our hands instead.
		return qfalse;
	}

	if (defender->s.number >= MAX_CLIENTS && !G_ControlledByPlayer(defender))
	{
		//bots just randomly parry to make up for them not intelligently parrying.
		return qtrue;
	}

	return qtrue;
}

int PlayerCanAbsorbKick(const gentity_t* defender, const vec3_t pushDir) //Can the player absorb a kick
{
	vec3_t pLAngles, pLFwd;

	const qboolean HoldingBlock = defender->client->ps.ManualBlockingFlags & 1 << HOLDINGBLOCK ? qtrue : qfalse;
	//Normal Blocking

	if (!defender || !defender->client)
	{
		//non-humanoids can't absorb kicks.
		return qfalse;
	}

	if (!HoldingBlock) // Must be holding Block button
	{
		// Not doing any blocking can't absorb kicks.
		return qfalse;
	}

	if (!walk_check(defender))
	{
		// runners cant absorb kick
		return qfalse;
	}
	// Cant absorb kick if
	if (defender->health <= 1 // low health (so dead people dont do it)
		|| BG_InKnockDown(defender->client->ps.legsAnim) // In a Knockdown (legs)
		|| BG_InKnockDown(defender->client->ps.torsoAnim) // In a Knockdown (torso)
		|| PM_InRoll(&defender->client->ps) // In a roll
		|| BG_InFlipBack(defender->client->ps.torsoAnim) // Flipping back
		|| PM_SuperBreakLoseAnim(defender->client->ps.torsoAnim) // lost a saber lock
		|| PM_SuperBreakWinAnim(defender->client->ps.torsoAnim) // won a saber lock
		|| pm_saber_in_special_attack(defender->client->ps.torsoAnim) // A special saber attack
		|| PM_InSpecialJump(defender->client->ps.torsoAnim) // A Force jump
		|| PM_SaberInBounce(defender->client->ps.saberMove) // Saber is bouncing
		|| PM_SaberInKnockaway(defender->client->ps.saberMove) // Saber is being knocked away
		|| PM_SaberInBrokenParry(defender->client->ps.saberMove) // Your parry got smashed open
		|| PM_KickMove(defender->client->ps.saberMove) // If you are doing a kick / melee / slap
		|| SaberAttacking(defender) // you are saber attacking
		|| PM_InGrappleMove(defender->client->ps.torsoAnim) // Trying to grab
		|| defender->client->ps.forcePowerLevel[FP_SABER_DEFENSE] < FORCE_LEVEL_1
		// No force saber deference (Gunners cant do it at all)
		|| defender->client->moveType == MT_FLYSWIM // Bobafett flying cant do it
		|| defender->client->ps.groundEntityNum == ENTITYNUM_NONE // Your in the air (jumping).
		|| defender->client->ps.blockPoints < FATIGUE_DODGEING // Less than 35 Block points
		|| defender->client->ps.forcePower < FATIGUE_DODGEING // Less than 35 Force points
		|| defender->client->ps.saberFatigueChainCount >= MISHAPLEVEL_TEN) // Your saber fatigued
	{
		return qfalse;
	}

	VectorSet(pLAngles, 0, defender->client->ps.viewangles[YAW], 0);
	AngleVectors(pLAngles, pLFwd, nullptr, nullptr);

	if (DotProduct(pLFwd, pushDir) > 0.2f)
	{
		//not hit in the front, can't absorb kick.
		return qfalse;
	}

	return qtrue; // If all that stuff above is clear then you can convert a knockdown in to a stagger
}

int BotCanAbsorbKick(const gentity_t* defender, const vec3_t pushDir) //Can the player absorb a kick
{
	vec3_t pLAngles, pLFwd;

	if (!defender || !defender->client)
	{
		//non-humanoids can't absorb kicks.
		return qfalse;
	}

	if (!walk_check(defender))
	{
		// runners cant absorb kick
		return qfalse;
	}

	if (in_camera)
	{
		return qfalse;
	}

	// Cant absorb kick if
	if (defender->health <= 1 // low health (so dead people dont do it)
		|| BG_InKnockDown(defender->client->ps.legsAnim) // In a Knockdown (legs)
		|| BG_InKnockDown(defender->client->ps.torsoAnim) // In a Knockdown (torso)
		|| PM_InRoll(&defender->client->ps) // In a roll
		|| BG_InFlipBack(defender->client->ps.torsoAnim) // Flipping back
		|| PM_SuperBreakLoseAnim(defender->client->ps.torsoAnim) // lost a saber lock
		|| PM_SuperBreakWinAnim(defender->client->ps.torsoAnim) // won a saber lock
		|| pm_saber_in_special_attack(defender->client->ps.torsoAnim) // A special saber attack
		|| PM_InSpecialJump(defender->client->ps.torsoAnim) // A Force jump
		|| PM_SaberInBounce(defender->client->ps.saberMove) // Saber is bouncing
		|| PM_SaberInKnockaway(defender->client->ps.saberMove) // Saber is being knocked away
		|| PM_SaberInBrokenParry(defender->client->ps.saberMove) // Your parry got smashed open
		|| PM_KickMove(defender->client->ps.saberMove) // If you are doing a kick / melee / slap
		|| SaberAttacking(defender) // you are saber attacking
		|| PM_InGrappleMove(defender->client->ps.torsoAnim) // Trying to grab
		|| defender->client->ps.forcePowerLevel[FP_SABER_DEFENSE] < FORCE_LEVEL_1
		// No force saber defend (Gunners cant do it at all)
		|| defender->client->moveType == MT_FLYSWIM // Bobafett flying cant do it
		|| defender->client->ps.groundEntityNum == ENTITYNUM_NONE // Your in the air (jumping).
		|| defender->client->ps.blockPoints < FATIGUE_DODGEING // Less than 35 Block points
		|| defender->client->ps.forcePower < FATIGUE_DODGEING // Less than 35 Force points
		|| defender->client->ps.saberFatigueChainCount >= MISHAPLEVEL_TEN) // Your saber fatigued
	{
		return qfalse;
	}

	VectorSet(pLAngles, 0, defender->client->ps.viewangles[YAW], 0);
	AngleVectors(pLAngles, pLFwd, nullptr, nullptr);

	if (DotProduct(pLFwd, pushDir) > 0.2f)
	{
		//not hit in the front, can't absorb kick.
		return qfalse;
	}

	return qtrue; // If all that stuff above is clear then you can convert a knockdown in to a stagger
}

float manual_npc_kick_absorbing(const gentity_t* defender)
{
	if (defender->s.number < MAX_CLIENTS)
	{
		return qfalse;
	}

	if (defender->client->ps.saberFatigueChainCount > MISHAPLEVEL_TEN && (defender->client->ps.forcePower <=
		BLOCKPOINTS_HALF || defender->client->ps.blockPoints <= BLOCKPOINTS_HALF))
	{
		return qfalse;
	}

	if (defender->s.eFlags & EF_FORCE_DRAINED || defender->s.eFlags & EF_FORCE_GRIPPED || defender->s.eFlags &
		EF_FORCE_GRABBED)
	{
		return qfalse;
	}

	if (defender->health <= 1
		|| BG_InKnockDown(defender->client->ps.legsAnim)
		|| BG_InKnockDown(defender->client->ps.torsoAnim)
		|| PM_InRoll(&defender->client->ps)
		|| PM_SuperBreakLoseAnim(defender->client->ps.torsoAnim)
		|| PM_SuperBreakWinAnim(defender->client->ps.torsoAnim)
		|| pm_saber_in_special_attack(defender->client->ps.torsoAnim)
		|| PM_InSpecialJump(defender->client->ps.torsoAnim)
		|| PM_SaberInBounce(defender->client->ps.saberMove)
		|| PM_SaberInReturn(defender->client->ps.saberMove)
		|| PM_SaberInKnockaway(defender->client->ps.saberMove)
		|| PM_SaberInBrokenParry(defender->client->ps.saberMove)
		|| PM_SaberInMassiveBounce(defender->client->ps.saberMove)
		|| PM_SaberInBashedAnim(defender->client->ps.saberMove)
		|| defender->client->ps.groundEntityNum == ENTITYNUM_NONE
		|| defender->client->ps.blockPoints < BLOCKPOINTS_THIRTY
		|| defender->client->ps.forcePower < BLOCKPOINTS_THIRTY
		|| defender->client->ps.userInt3 & 1 << FLAG_FATIGUED)
	{
		return qfalse;
	}

	if (defender->client->ps.weapon != WP_SABER
		|| defender->client->ps.weapon == WP_NONE
		|| defender->client->ps.weapon == WP_MELEE) //saber not here
	{
		return qfalse;
	}

	if (defender->client->ps.weapon == WP_SABER
		&& defender->client->ps.saberInFlight)
	{
		//saber not currently in use or available, attempt to use our hands instead.
		return qfalse;
	}

	if (defender->client->ps.weapon == WP_SABER
		&& !defender->client->ps.SaberActive())
	{
		//saber not currently in use or available, attempt to use our hands instead.
		return qfalse;
	}

	if (!walk_check(defender))
	{
		//can't block while running.
		return qfalse;
	}

	if (defender->s.number >= MAX_CLIENTS && !G_ControlledByPlayer(defender))
	{
		//bots just randomly parry to make up for them not intelligently parrying.
		return qtrue;
	}

	return qtrue;
}

extern qboolean BG_FullBodyEmoteAnim(int anim);
extern qboolean BG_FullBodyCowerstartAnim(int anim);

qboolean IsSurrendering(const gentity_t* self)
{
	if (self->s.number >= MAX_CLIENTS && !G_ControlledByPlayer(self))
	{
		return qfalse;
	}

	if (self->client->ps.weapon != WP_SABER)
	{
		return qfalse;
	}

	if (!BG_FullBodyEmoteAnim(self->client->ps.torsoAnim))
	{
		// Ignore players surrendering
		return qfalse;
	}

	return qtrue;
}

qboolean IsCowering(const gentity_t* self)
{
	if (self->s.number >= MAX_CLIENTS && !G_ControlledByPlayer(self))
	{
		return qfalse;
	}

	if (!BG_FullBodyCowerstartAnim(self->client->ps.torsoAnim))
	{
		// Ignore players surrendering
		return qfalse;
	}

	return qtrue;
}

extern qboolean BG_FullBodyRespectAnim(int anim);

qboolean IsRespecting(const gentity_t* self)
{
	if (self->s.number >= MAX_CLIENTS && !G_ControlledByPlayer(self))
	{
		return qfalse;
	}

	if (!BG_FullBodyRespectAnim(self->client->ps.torsoAnim))
	{
		// Ignore players surrendering
		return qfalse;
	}

	return qtrue;
}

extern qboolean BG_AnimRequiresResponce(int anim);

qboolean IsAnimRequiresResponce(const gentity_t* self)
{
	if (self->s.number >= MAX_CLIENTS && !G_ControlledByPlayer(self))
	{
		return qfalse;
	}

	if (!BG_AnimRequiresResponce(self->client->ps.torsoAnim))
	{
		// Ignore players surrendering
		return qfalse;
	}

	return qtrue;
}

extern qboolean BG_AnimIsSurrenderingandRequiresResponce(int anim);

qboolean IsSurrenderingAnimRequiresResponce(const gentity_t* self)
{
	if (self->s.number >= MAX_CLIENTS && !G_ControlledByPlayer(self))
	{
		return qfalse;
	}

	if (!BG_AnimIsSurrenderingandRequiresResponce(self->client->ps.torsoAnim))
	{
		// Ignore players surrendering
		return qfalse;
	}

	return qtrue;
}

qboolean wp_saber_block_check_random(gentity_t* self, vec3_t hitloc)
{
	vec3_t diff, fwdangles = { 0, 0, 0 }, right;
	const qboolean inFront = in_front(hitloc, self->client->ps.origin, self->client->ps.viewangles, -0.7f);

	VectorSubtract(hitloc, self->client->renderInfo.eyePoint, diff);
	diff[2] = 0;
	VectorNormalize(diff);
	fwdangles[1] = self->client->ps.viewangles[1];
	// Ultimately we might care if the shot was ahead or behind, but for now, just quadrant is fine.
	AngleVectors(fwdangles, nullptr, right, nullptr);

	const float rightdot = DotProduct(right, diff);
	const float zdiff = hitloc[2] - self->client->renderInfo.eyePoint[2];

	if (self->client->ps.weaponstate == WEAPON_DROPPING ||
		self->client->ps.weaponstate == WEAPON_RAISING)
	{
		return qfalse;
	}
	if (PM_SuperBreakLoseAnim(self->client->ps.torsoAnim)
		|| PM_SuperBreakWinAnim(self->client->ps.torsoAnim))
	{
		return qfalse;
	}

	if (PM_SaberInMassiveBounce(self->client->ps.torsoAnim) || PM_SaberInBashedAnim(self->client->ps.torsoAnim))
	{
		return qfalse;
	}

	if (!inFront && self->client->ps.forcePowerLevel[FP_SABER_DEFENSE] >= FORCE_LEVEL_1)
	{
		switch (self->client->ps.saberAnimLevel)
		{
		case SS_STAFF:
			NPC_SetAnim(self, SETANIM_TORSO, BOTH_P7_S1_B_, SETANIM_AFLAG_PACE);
			break;
		case SS_DUAL:
			NPC_SetAnim(self, SETANIM_TORSO, BOTH_P6_S1_B_, SETANIM_AFLAG_PACE);
			break;
		default:
			NPC_SetAnim(self, SETANIM_TORSO, BOTH_P1_S1_B_, SETANIM_AFLAG_PACE);
			break;
		}
	}
	else if (zdiff > -5)
	{
		if (rightdot > 0.3)
		{
			self->client->ps.saberBlocked = BLOCKED_UPPER_RIGHT;
		}
		else if (rightdot < -0.3)
		{
			self->client->ps.saberBlocked = BLOCKED_UPPER_LEFT;
		}
		else
		{
			self->client->ps.saberBlocked = BLOCKED_TOP;
		}
	}
	else if (zdiff > -22)
	{
		if (zdiff < -10)
		{
			//NPC should duck, but NPC should never get here
		}
		if (rightdot > 0.1)
		{
			self->client->ps.saberBlocked = BLOCKED_UPPER_RIGHT;
		}
		else if (rightdot < -0.1)
		{
			self->client->ps.saberBlocked = BLOCKED_UPPER_LEFT;
		}
		else
		{
			self->client->ps.saberBlocked = BLOCKED_TOP;
		}
	}
	else
	{
		if (rightdot >= 0)
		{
			self->client->ps.saberBlocked = BLOCKED_LOWER_RIGHT;
		}
		else
		{
			self->client->ps.saberBlocked = BLOCKED_LOWER_LEFT;
		}
	}

	if (self->s.number >= MAX_CLIENTS && !G_ControlledByPlayer(self) && self->client->ps.saberBlocked != BLOCKED_NONE)
	{
		const int parry_re_calc_time = Jedi_ReCalcParryTime(self, EVASION_PARRY);
		if (self->client->ps.forcePowerDebounce[FP_SABER_DEFENSE] < level.time + parry_re_calc_time)
		{
			self->client->ps.forcePowerDebounce[FP_SABER_DEFENSE] = level.time + parry_re_calc_time;
		}
	}

	self->client->ps.userInt3 &= ~(1 << FLAG_PREBLOCK);
	return qtrue;
}

qboolean WP_SaberMBlockDirection(gentity_t* self, vec3_t hitloc, const qboolean missileBlock)
{
	vec3_t diff, fwdangles = { 0, 0, 0 }, right;
	const qboolean inFront = in_front(hitloc, self->client->ps.origin, self->client->ps.viewangles, -0.7f);

	VectorSubtract(hitloc, self->client->renderInfo.eyePoint, diff);
	diff[2] = 0;
	VectorNormalize(diff);
	fwdangles[1] = self->client->ps.viewangles[1];
	// Ultimately we might care if the shot was ahead or behind, but for now, just quadrant is fine.
	AngleVectors(fwdangles, nullptr, right, nullptr);

	const float rightdot = DotProduct(right, diff);
	const float zdiff = hitloc[2] - self->client->renderInfo.eyePoint[2];

	if (self->client->ps.weaponstate == WEAPON_DROPPING ||
		self->client->ps.weaponstate == WEAPON_RAISING)
	{
		return qfalse;
	}

	if (PM_SaberInAttack(self->client->ps.saberMove) ||
		PM_SuperBreakLoseAnim(self->client->ps.torsoAnim) ||
		PM_SuperBreakWinAnim(self->client->ps.torsoAnim) ||
		PM_SaberInBrokenParry(self->client->ps.saberMove) ||
		PM_SaberInKnockaway(self->client->ps.saberMove) ||
		PM_InRoll(&self->client->ps) ||
		BG_InKnockDown(self->client->ps.legsAnim) ||
		BG_InKnockDown(self->client->ps.torsoAnim) ||
		PM_InKnockDown(&self->client->ps))
	{
		return qfalse;
	}

	if (PM_SaberInMassiveBounce(self->client->ps.torsoAnim) || PM_SaberInBashedAnim(self->client->ps.torsoAnim))
	{
		return qfalse;
	}

	if (self->client->ps.blockPoints <= BLOCKPOINTS_FAIL || self->client->ps.forcePower <= BLOCKPOINTS_DANGER)
	{
		return qfalse;
	}

	if (!inFront && self->client->ps.forcePowerLevel[FP_SABER_DEFENSE] >= FORCE_LEVEL_1)
	{
		switch (self->client->ps.saberAnimLevel)
		{
		case SS_STAFF:
			NPC_SetAnim(self, SETANIM_TORSO, BOTH_P7_S1_B_, SETANIM_AFLAG_PACE);
			break;
		case SS_DUAL:
			NPC_SetAnim(self, SETANIM_TORSO, BOTH_P6_S1_B_, SETANIM_AFLAG_PACE);
			break;
		default:
			NPC_SetAnim(self, SETANIM_TORSO, BOTH_P1_S1_B_, SETANIM_AFLAG_PACE);
			break;
		}
		self->client->ps.weaponTime = Q_irand(300, 600);
	}
	else if (zdiff > -5)
	{
		if (rightdot > 0.3)
		{
			switch (self->client->ps.saberAnimLevel)
			{
			case SS_STAFF:
				NPC_SetAnim(self, SETANIM_TORSO, BOTH_B7_TR___, SETANIM_AFLAG_PACE);
				break;
			case SS_DUAL:
				NPC_SetAnim(self, SETANIM_TORSO, BOTH_B6_TR___, SETANIM_AFLAG_PACE);
				break;
			default:
				NPC_SetAnim(self, SETANIM_TORSO, BOTH_K1_S1_TR_ALT, SETANIM_AFLAG_PACE);
				break;
			}
			self->client->ps.weaponTime = Q_irand(300, 600);
		}
		else if (rightdot < -0.3)
		{
			switch (self->client->ps.saberAnimLevel)
			{
			case SS_STAFF:
				NPC_SetAnim(self, SETANIM_TORSO, BOTH_B7_TL___, SETANIM_AFLAG_PACE);
				break;
			case SS_DUAL:
				NPC_SetAnim(self, SETANIM_TORSO, BOTH_B6_TL___, SETANIM_AFLAG_PACE);
				break;
			default:
				NPC_SetAnim(self, SETANIM_TORSO, BOTH_K1_S1_TL_ALT, SETANIM_AFLAG_PACE);
				break;
			}
			self->client->ps.weaponTime = Q_irand(300, 600);
		}
		else
		{
			switch (self->client->ps.saberAnimLevel)
			{
			case SS_STAFF:
				NPC_SetAnim(self, SETANIM_TORSO, BOTH_K7_S7_T_, SETANIM_AFLAG_PACE);
				break;
			case SS_DUAL:
				NPC_SetAnim(self, SETANIM_TORSO, BOTH_K6_S6_T_, SETANIM_AFLAG_PACE);
				break;
			default:
				NPC_SetAnim(self, SETANIM_TORSO, BOTH_K1_S1_T_, SETANIM_AFLAG_PACE);
				break;
			}
			self->client->ps.weaponTime = Q_irand(300, 600);
		}
	}
	else if (zdiff > -20)
	{
		if (zdiff < -10)
		{
			//hmm, pretty low, but not low enough to use the low block
		}
		if (rightdot > 0.1)
		{
			switch (self->client->ps.saberAnimLevel)
			{
			case SS_STAFF:
				NPC_SetAnim(self, SETANIM_TORSO, BOTH_B7_TR___, SETANIM_AFLAG_PACE);
				break;
			case SS_DUAL:
				NPC_SetAnim(self, SETANIM_TORSO, BOTH_B6_TR___, SETANIM_AFLAG_PACE);
				break;
			default:
				NPC_SetAnim(self, SETANIM_TORSO, BOTH_K1_S1_TR_ALT, SETANIM_AFLAG_PACE);
				break;
			}
			self->client->ps.weaponTime = Q_irand(300, 600);
		}
		else if (rightdot < -0.1)
		{
			switch (self->client->ps.saberAnimLevel)
			{
			case SS_STAFF:
				NPC_SetAnim(self, SETANIM_TORSO, BOTH_B7_TL___, SETANIM_AFLAG_PACE);
				break;
			case SS_DUAL:
				NPC_SetAnim(self, SETANIM_TORSO, BOTH_B6_TL___, SETANIM_AFLAG_PACE);
				break;
			default:
				NPC_SetAnim(self, SETANIM_TORSO, BOTH_K1_S1_TL_ALT, SETANIM_AFLAG_PACE);
				break;
			}
			self->client->ps.weaponTime = Q_irand(300, 600);
		}
		else
		{
			switch (self->client->ps.saberAnimLevel)
			{
			case SS_STAFF:
				NPC_SetAnim(self, SETANIM_TORSO, BOTH_K7_S7_T_, SETANIM_AFLAG_PACE);
				break;
			case SS_DUAL:
				NPC_SetAnim(self, SETANIM_TORSO, BOTH_K6_S6_T_, SETANIM_AFLAG_PACE);
				break;
			default:
				NPC_SetAnim(self, SETANIM_TORSO, BOTH_K1_S1_T_, SETANIM_AFLAG_PACE);
				break;
			}
			self->client->ps.weaponTime = Q_irand(300, 600);
		}
	}
	else
	{
		if (rightdot >= 0)
		{
			switch (self->client->ps.saberAnimLevel)
			{
				//BOTTOM RIGHT
			case SS_STAFF:
				NPC_SetAnim(self, SETANIM_TORSO, BOTH_B7_BR___, SETANIM_AFLAG_PACE);
				break;
			case SS_DUAL:
				NPC_SetAnim(self, SETANIM_TORSO, BOTH_B6_BR___, SETANIM_AFLAG_PACE);
				break;
			default:
				NPC_SetAnim(self, SETANIM_TORSO, BOTH_B1_BR___, SETANIM_AFLAG_PACE);
				break;
			}
			self->client->ps.weaponTime = Q_irand(300, 600);
		}
		else
		{
			switch (self->client->ps.saberAnimLevel)
			{
				//BOTTOM LEFT
			case SS_STAFF:
				NPC_SetAnim(self, SETANIM_TORSO, BOTH_B7_BL___, SETANIM_AFLAG_PACE);
				break;
			case SS_DUAL:
				NPC_SetAnim(self, SETANIM_TORSO, BOTH_B6_BL___, SETANIM_AFLAG_PACE);
				break;
			default:
				NPC_SetAnim(self, SETANIM_TORSO, BOTH_B1_BL___, SETANIM_AFLAG_PACE);
				break;
			}
			self->client->ps.weaponTime = Q_irand(300, 600);
		}
	}

	if (missileBlock)
	{
		self->client->ps.saberBlocked = WP_MissileBlockForBlock(self->client->ps.saberBlocked);
		self->client->ps.weaponTime = Q_irand(300, 600);
	}

	if (self->s.number >= MAX_CLIENTS && !G_ControlledByPlayer(self) && self->client->ps.saberBlocked != BLOCKED_NONE)
	{
		const int parryReCalcTime = Jedi_ReCalcParryTime(self, EVASION_PARRY);
		if (self->client->ps.forcePowerDebounce[FP_SABER_DEFENSE] < level.time + parryReCalcTime)
		{
			self->client->ps.forcePowerDebounce[FP_SABER_DEFENSE] = level.time + parryReCalcTime;
		}
	}

	self->client->ps.userInt3 &= ~(1 << FLAG_PREBLOCK);
	return qtrue;
}

qboolean WP_SaberBlockNonRandom(gentity_t* self, vec3_t hitloc, const qboolean missileBlock)
{
	vec3_t diff, fwdangles = { 0, 0, 0 }, right;
	const qboolean inFront = in_front(hitloc, self->client->ps.origin, self->client->ps.viewangles, -0.7f);

	VectorSubtract(hitloc, self->client->renderInfo.eyePoint, diff);
	diff[2] = 0;
	VectorNormalize(diff);
	fwdangles[1] = self->client->ps.viewangles[1];
	// Ultimately we might care if the shot was ahead or behind, but for now, just quadrant is fine.
	AngleVectors(fwdangles, nullptr, right, nullptr);

	const float rightdot = DotProduct(right, diff);
	const float zdiff = hitloc[2] - self->client->renderInfo.eyePoint[2];

	if (self->client->ps.weaponstate == WEAPON_DROPPING ||
		self->client->ps.weaponstate == WEAPON_RAISING)
	{
		return qfalse;
	}

	if (PM_SaberInAttack(self->client->ps.saberMove) ||
		PM_SuperBreakLoseAnim(self->client->ps.torsoAnim) ||
		PM_SuperBreakWinAnim(self->client->ps.torsoAnim) ||
		PM_SaberInBrokenParry(self->client->ps.saberMove) ||
		PM_SaberInKnockaway(self->client->ps.saberMove) ||
		PM_InRoll(&self->client->ps) ||
		BG_InKnockDown(self->client->ps.legsAnim) ||
		BG_InKnockDown(self->client->ps.torsoAnim) ||
		PM_InKnockDown(&self->client->ps))
	{
		return qfalse;
	}

	if (PM_SaberInMassiveBounce(self->client->ps.torsoAnim) || PM_SaberInBashedAnim(self->client->ps.torsoAnim))
	{
		return qfalse;
	}

	if (self->client->ps.blockPoints <= BLOCKPOINTS_FAIL || self->client->ps.forcePower <= BLOCKPOINTS_DANGER)
	{
		return qfalse;
	}

	if (!inFront && self->client->ps.forcePowerLevel[FP_SABER_DEFENSE] >= FORCE_LEVEL_1)
	{
		switch (self->client->ps.saberAnimLevel)
		{
		case SS_STAFF:
			NPC_SetAnim(self, SETANIM_TORSO, BOTH_P7_S1_B_, SETANIM_AFLAG_PACE);
			break;
		case SS_DUAL:
			NPC_SetAnim(self, SETANIM_TORSO, BOTH_P6_S1_B_, SETANIM_AFLAG_PACE);
			break;
		default:
			NPC_SetAnim(self, SETANIM_TORSO, BOTH_P1_S1_B_, SETANIM_AFLAG_PACE);
			break;
		}
		self->client->ps.weaponTime = Q_irand(300, 600);
	}
	else if (zdiff > -5)
	{
		if (rightdot > 0.3)
		{
			switch (self->client->ps.saberAnimLevel)
			{
			case SS_STAFF:
				NPC_SetAnim(self, SETANIM_TORSO, BOTH_P7_S7_TR, SETANIM_AFLAG_PACE);
				break;
			case SS_DUAL:
				NPC_SetAnim(self, SETANIM_TORSO, BOTH_P6_S6_TR, SETANIM_AFLAG_PACE);
				break;
			default:
				if (self->client->NPC_class == CLASS_DESANN)
				{
					NPC_SetAnim(self, SETANIM_TORSO, BOTH_K1_S1_TR_ALT, SETANIM_AFLAG_PACE);
				}
				else
				{
					NPC_SetAnim(self, SETANIM_TORSO, BOTH_P1_S1_TR, SETANIM_AFLAG_PACE);
				}
				break;
			}
			self->client->ps.weaponTime = Q_irand(300, 600);
		}
		else if (rightdot < -0.3)
		{
			switch (self->client->ps.saberAnimLevel)
			{
			case SS_STAFF:
				NPC_SetAnim(self, SETANIM_TORSO, BOTH_P7_S7_TL, SETANIM_AFLAG_PACE);
				break;
			case SS_DUAL:
				NPC_SetAnim(self, SETANIM_TORSO, BOTH_P6_S6_TL, SETANIM_AFLAG_PACE);
				break;
			default:
				if (self->client->NPC_class == CLASS_DESANN)
				{
					NPC_SetAnim(self, SETANIM_TORSO, BOTH_K1_S1_TL_ALT, SETANIM_AFLAG_PACE);
				}
				else
				{
					NPC_SetAnim(self, SETANIM_TORSO, BOTH_P1_S1_TL, SETANIM_AFLAG_PACE);
				}
				break;
			}
			self->client->ps.weaponTime = Q_irand(300, 600);
		}
		else
		{
			switch (self->client->ps.saberAnimLevel)
			{
			case SS_STAFF:
				NPC_SetAnim(self, SETANIM_TORSO, BOTH_P7_S7_T_, SETANIM_AFLAG_PACE);
				break;
			case SS_DUAL:
				NPC_SetAnim(self, SETANIM_TORSO, BOTH_P6_S6_T_, SETANIM_AFLAG_PACE);
				break;
			default:
				NPC_SetAnim(self, SETANIM_TORSO, BOTH_P1_S1_T_, SETANIM_AFLAG_PACE);
				break;
			}
			self->client->ps.weaponTime = Q_irand(300, 600);
		}
	}
	else if (zdiff > -20)
	{
		if (zdiff < -10)
		{
			//hmm, pretty low, but not low enough to use the low block, so we need to duck
		}
		if (rightdot > 0.1)
		{
			switch (self->client->ps.saberAnimLevel)
			{
			case SS_STAFF:
				NPC_SetAnim(self, SETANIM_TORSO, BOTH_P7_S7_TR, SETANIM_AFLAG_PACE);
				break;
			case SS_DUAL:
				NPC_SetAnim(self, SETANIM_TORSO, BOTH_P6_S6_TR, SETANIM_AFLAG_PACE);
				break;
			default:
				NPC_SetAnim(self, SETANIM_TORSO, BOTH_P1_S1_TR, SETANIM_AFLAG_PACE);
				break;
			}
			self->client->ps.weaponTime = Q_irand(300, 600);
		}
		else if (rightdot < -0.1)
		{
			switch (self->client->ps.saberAnimLevel)
			{
			case SS_STAFF:
				NPC_SetAnim(self, SETANIM_TORSO, BOTH_P7_S7_TL, SETANIM_AFLAG_PACE);
				break;
			case SS_DUAL:
				NPC_SetAnim(self, SETANIM_TORSO, BOTH_P6_S6_TL, SETANIM_AFLAG_PACE);
				break;
			default:
				NPC_SetAnim(self, SETANIM_TORSO, BOTH_P1_S1_TL, SETANIM_AFLAG_PACE);
				break;
			}
			self->client->ps.weaponTime = Q_irand(300, 600);
		}
		else
		{
			switch (self->client->ps.saberAnimLevel)
			{
			case SS_STAFF:
				NPC_SetAnim(self, SETANIM_TORSO, BOTH_P7_S7_T_, SETANIM_AFLAG_PACE);
				break;
			case SS_DUAL:
				NPC_SetAnim(self, SETANIM_TORSO, BOTH_P6_S6_T_, SETANIM_AFLAG_PACE);
				break;
			default:
				NPC_SetAnim(self, SETANIM_TORSO, BOTH_P1_S1_T_, SETANIM_AFLAG_PACE);
				break;
			}
			self->client->ps.weaponTime = Q_irand(300, 600);
		}
	}
	else
	{
		if (rightdot >= 0)
		{
			switch (self->client->ps.saberAnimLevel)
			{
			case SS_STAFF:
				NPC_SetAnim(self, SETANIM_TORSO, BOTH_P7_S7_BR, SETANIM_AFLAG_PACE);
				break;
			case SS_DUAL:
				NPC_SetAnim(self, SETANIM_TORSO, BOTH_P6_S6_BR, SETANIM_AFLAG_PACE);
				break;
			default:
				NPC_SetAnim(self, SETANIM_TORSO, BOTH_P1_S1_BR, SETANIM_AFLAG_PACE);
				break;
			}
			self->client->ps.weaponTime = Q_irand(300, 600);
		}
		else
		{
			switch (self->client->ps.saberAnimLevel)
			{
			case SS_STAFF:
				NPC_SetAnim(self, SETANIM_TORSO, BOTH_P7_S7_BL, SETANIM_AFLAG_PACE);
				break;
			case SS_DUAL:
				NPC_SetAnim(self, SETANIM_TORSO, BOTH_P6_S6_BL, SETANIM_AFLAG_PACE);
				break;
			default:
				NPC_SetAnim(self, SETANIM_TORSO, BOTH_P1_S1_BL, SETANIM_AFLAG_PACE);
				break;
			}
			self->client->ps.weaponTime = Q_irand(300, 600);
		}
	}

	if (missileBlock)
	{
		self->client->ps.saberBlocked = WP_MissileBlockForBlock(self->client->ps.saberBlocked);
		self->client->ps.weaponTime = Q_irand(300, 600);
	}

	if (self->s.number >= MAX_CLIENTS && !G_ControlledByPlayer(self) && self->client->ps.saberBlocked != BLOCKED_NONE)
	{
		const int parryReCalcTime = Jedi_ReCalcParryTime(self, EVASION_PARRY);
		if (self->client->ps.forcePowerDebounce[FP_SABER_DEFENSE] < level.time + parryReCalcTime)
		{
			self->client->ps.forcePowerDebounce[FP_SABER_DEFENSE] = level.time + parryReCalcTime;
		}
	}

	self->client->ps.userInt3 &= ~(1 << FLAG_PREBLOCK);
	return qtrue;
}

qboolean WP_SaberBouncedSaberDirection(gentity_t* self, vec3_t hitloc, const qboolean missileBlock)
{
	vec3_t diff, fwdangles = { 0, 0, 0 }, right;
	const qboolean inFront = in_front(hitloc, self->client->ps.origin, self->client->ps.viewangles, -0.7f);

	VectorSubtract(hitloc, self->client->renderInfo.eyePoint, diff);
	diff[2] = 0;
	VectorNormalize(diff);
	fwdangles[1] = self->client->ps.viewangles[1];
	// Ultimately we might care if the shot was ahead or behind, but for now, just quadrant is fine.
	AngleVectors(fwdangles, nullptr, right, nullptr);

	const float rightdot = DotProduct(right, diff);
	const float zdiff = hitloc[2] - self->client->renderInfo.eyePoint[2];

	if (self->client->ps.weaponstate == WEAPON_DROPPING ||
		self->client->ps.weaponstate == WEAPON_RAISING)
	{
		return qfalse;
	}

	if (PM_SaberInAttack(self->client->ps.saberMove) ||
		PM_SuperBreakLoseAnim(self->client->ps.torsoAnim) ||
		PM_SuperBreakWinAnim(self->client->ps.torsoAnim) ||
		PM_SaberInBrokenParry(self->client->ps.saberMove) ||
		PM_SaberInKnockaway(self->client->ps.saberMove) ||
		PM_InRoll(&self->client->ps) ||
		BG_InKnockDown(self->client->ps.legsAnim) ||
		BG_InKnockDown(self->client->ps.torsoAnim) ||
		PM_InKnockDown(&self->client->ps))
	{
		return qfalse;
	}

	if (PM_SaberInMassiveBounce(self->client->ps.torsoAnim) || PM_SaberInBashedAnim(self->client->ps.torsoAnim))
	{
		return qfalse;
	}

	if (self->client->ps.blockPoints <= BLOCKPOINTS_FAIL || self->client->ps.forcePower <= BLOCKPOINTS_DANGER)
	{
		return qfalse;
	}

	if (!inFront && self->client->ps.forcePowerLevel[FP_SABER_DEFENSE] >= FORCE_LEVEL_1)
	{
		switch (self->client->ps.saberAnimLevel)
		{
		case SS_STAFF:
			NPC_SetAnim(self, SETANIM_TORSO, BOTH_P7_S1_B_, SETANIM_AFLAG_PACE);
			break;
		case SS_DUAL:
			NPC_SetAnim(self, SETANIM_TORSO, BOTH_P6_S1_B_, SETANIM_AFLAG_PACE);
			break;
		default:
			NPC_SetAnim(self, SETANIM_TORSO, BOTH_P1_S1_B_, SETANIM_AFLAG_PACE);
			break;
		}
		self->client->ps.weaponTime = Q_irand(300, 600);
	}
	else if (zdiff > -5)
	{
		if (rightdot > 0.3)
		{
			switch (self->client->ps.saberAnimLevel)
			{
			case SS_STAFF:
				NPC_SetAnim(self, SETANIM_TORSO, BOTH_R7_TR_S7, SETANIM_AFLAG_PACE);
				break;
			case SS_DUAL:
				NPC_SetAnim(self, SETANIM_TORSO, BOTH_R6_TR_S6, SETANIM_AFLAG_PACE);
				break;
			default:
				NPC_SetAnim(self, SETANIM_TORSO, BOTH_R1_TR_S1, SETANIM_AFLAG_PACE);
				break;
			}
			self->client->ps.weaponTime = Q_irand(300, 600);
		}
		else if (rightdot < -0.3)
		{
			switch (self->client->ps.saberAnimLevel)
			{
			case SS_STAFF:
				NPC_SetAnim(self, SETANIM_TORSO, BOTH_R7_TL_S7, SETANIM_AFLAG_PACE);
				break;
			case SS_DUAL:
				NPC_SetAnim(self, SETANIM_TORSO, BOTH_R6_TL_S6, SETANIM_AFLAG_PACE);
				break;
			default:
				NPC_SetAnim(self, SETANIM_TORSO, BOTH_R1_TL_S1, SETANIM_AFLAG_PACE);
				break;
			}
			self->client->ps.weaponTime = Q_irand(300, 600);
		}
		else
		{
			switch (self->client->ps.saberAnimLevel)
			{
			case SS_STAFF:
				NPC_SetAnim(self, SETANIM_TORSO, BOTH_K7_S7_T_, SETANIM_AFLAG_PACE);
				break;
			case SS_DUAL:
				NPC_SetAnim(self, SETANIM_TORSO, BOTH_K6_S6_T_, SETANIM_AFLAG_PACE);
				break;
			default:
				NPC_SetAnim(self, SETANIM_TORSO, BOTH_K1_S1_T_, SETANIM_AFLAG_PACE);
				break;
			}
			self->client->ps.weaponTime = Q_irand(300, 600);
		}
	}
	else if (zdiff > -20)
	{
		if (zdiff < -10)
		{
			//hmm, pretty low, but not low enough to use the low block
		}
		if (rightdot > 0.1)
		{
			switch (self->client->ps.saberAnimLevel)
			{
			case SS_STAFF:
				NPC_SetAnim(self, SETANIM_TORSO, BOTH_R7_TR_S7, SETANIM_AFLAG_PACE);
				break;
			case SS_DUAL:
				NPC_SetAnim(self, SETANIM_TORSO, BOTH_R6_TR_S6, SETANIM_AFLAG_PACE);
				break;
			default:
				NPC_SetAnim(self, SETANIM_TORSO, BOTH_R1_TR_S1, SETANIM_AFLAG_PACE);
				break;
			}
			self->client->ps.weaponTime = Q_irand(300, 600);
		}
		else if (rightdot < -0.1)
		{
			switch (self->client->ps.saberAnimLevel)
			{
			case SS_STAFF:
				NPC_SetAnim(self, SETANIM_TORSO, BOTH_R7_TL_S7, SETANIM_AFLAG_PACE);
				break;
			case SS_DUAL:
				NPC_SetAnim(self, SETANIM_TORSO, BOTH_R6_TL_S6, SETANIM_AFLAG_PACE);
				break;
			default:
				NPC_SetAnim(self, SETANIM_TORSO, BOTH_R1_TL_S1, SETANIM_AFLAG_PACE);
				break;
			}
			self->client->ps.weaponTime = Q_irand(300, 600);
		}
		else
		{
			switch (self->client->ps.saberAnimLevel)
			{
			case SS_STAFF:
				NPC_SetAnim(self, SETANIM_TORSO, BOTH_K7_S7_T_, SETANIM_AFLAG_PACE);
				break;
			case SS_DUAL:
				NPC_SetAnim(self, SETANIM_TORSO, BOTH_K6_S6_T_, SETANIM_AFLAG_PACE);
				break;
			default:
				NPC_SetAnim(self, SETANIM_TORSO, BOTH_K1_S1_T_, SETANIM_AFLAG_PACE);
				break;
			}
			self->client->ps.weaponTime = Q_irand(300, 600);
		}
	}
	else
	{
		if (rightdot >= 0)
		{
			switch (self->client->ps.saberAnimLevel)
			{
			case SS_STAFF:
				NPC_SetAnim(self, SETANIM_TORSO, BOTH_P7_S7_BR, SETANIM_AFLAG_PACE);
				break;
			case SS_DUAL:
				NPC_SetAnim(self, SETANIM_TORSO, BOTH_P6_S6_BR, SETANIM_AFLAG_PACE);
				break;
			default:
				NPC_SetAnim(self, SETANIM_TORSO, BOTH_P1_S1_BR, SETANIM_AFLAG_PACE);
				break;
			}
			self->client->ps.weaponTime = Q_irand(300, 600);
		}
		else
		{
			switch (self->client->ps.saberAnimLevel)
			{
			case SS_STAFF:
				NPC_SetAnim(self, SETANIM_TORSO, BOTH_P7_S7_BL, SETANIM_AFLAG_PACE);
				break;
			case SS_DUAL:
				NPC_SetAnim(self, SETANIM_TORSO, BOTH_P6_S6_BL, SETANIM_AFLAG_PACE);
				break;
			default:
				NPC_SetAnim(self, SETANIM_TORSO, BOTH_P1_S1_BL, SETANIM_AFLAG_PACE);
				break;
			}
			self->client->ps.weaponTime = Q_irand(300, 600);
		}
	}

	if (missileBlock)
	{
		self->client->ps.saberBlocked = WP_MissileBlockForBlock(self->client->ps.saberBlocked);
		self->client->ps.weaponTime = Q_irand(300, 600);
	}

	if (self->s.number >= MAX_CLIENTS && !G_ControlledByPlayer(self) && self->client->ps.saberBlocked != BLOCKED_NONE)
	{
		const int parryReCalcTime = Jedi_ReCalcParryTime(self, EVASION_PARRY);
		if (self->client->ps.forcePowerDebounce[FP_SABER_DEFENSE] < level.time + parryReCalcTime)
		{
			self->client->ps.forcePowerDebounce[FP_SABER_DEFENSE] = level.time + parryReCalcTime;
		}
	}

	self->client->ps.userInt3 &= ~(1 << FLAG_PREBLOCK);
	return qtrue;
}

qboolean WP_SaberFatiguedParryDirection(gentity_t* self, vec3_t hitloc, const qboolean missileBlock)
{
	vec3_t diff, fwdangles = { 0, 0, 0 }, right;
	const qboolean inFront = in_front(hitloc, self->client->ps.origin, self->client->ps.viewangles, -0.7f);

	VectorSubtract(hitloc, self->client->renderInfo.eyePoint, diff);
	diff[2] = 0;
	VectorNormalize(diff);
	fwdangles[1] = self->client->ps.viewangles[1];
	// Ultimately we might care if the shot was ahead or behind, but for now, just quadrant is fine.
	AngleVectors(fwdangles, nullptr, right, nullptr);

	const float rightdot = DotProduct(right, diff);
	const float zdiff = hitloc[2] - self->client->renderInfo.eyePoint[2];

	if (self->client->ps.weaponstate == WEAPON_DROPPING ||
		self->client->ps.weaponstate == WEAPON_RAISING)
	{
		return qfalse;
	}

	if (PM_SaberInAttack(self->client->ps.saberMove) ||
		PM_SuperBreakLoseAnim(self->client->ps.torsoAnim) ||
		PM_SuperBreakWinAnim(self->client->ps.torsoAnim) ||
		PM_SaberInBrokenParry(self->client->ps.saberMove) ||
		PM_SaberInKnockaway(self->client->ps.saberMove) ||
		PM_InRoll(&self->client->ps) ||
		BG_InKnockDown(self->client->ps.legsAnim) ||
		BG_InKnockDown(self->client->ps.torsoAnim) ||
		PM_InKnockDown(&self->client->ps))
	{
		return qfalse;
	}

	if (PM_SaberInMassiveBounce(self->client->ps.torsoAnim) || PM_SaberInBashedAnim(self->client->ps.torsoAnim))
	{
		return qfalse;
	}

	if (self->client->ps.blockPoints <= BLOCKPOINTS_FAIL || self->client->ps.forcePower <= BLOCKPOINTS_DANGER)
	{
		return qfalse;
	}

	if (!inFront && self->client->ps.forcePowerLevel[FP_SABER_DEFENSE] >= FORCE_LEVEL_1)
	{
		switch (self->client->ps.saberAnimLevel)
		{
		case SS_STAFF:
			NPC_SetAnim(self, SETANIM_TORSO, BOTH_P7_S1_B_, SETANIM_AFLAG_PACE);
			break;
		case SS_DUAL:
			NPC_SetAnim(self, SETANIM_TORSO, BOTH_P6_S1_B_, SETANIM_AFLAG_PACE);
			break;
		default:
			NPC_SetAnim(self, SETANIM_TORSO, BOTH_P1_S1_B_, SETANIM_AFLAG_PACE);
			break;
		}
		self->client->ps.weaponTime = Q_irand(300, 600);
	}
	else if (zdiff > -5)
	{
		if (rightdot > 0.3)
		{
			switch (self->client->ps.saberAnimLevel)
			{
			case SS_STAFF:
				NPC_SetAnim(self, SETANIM_TORSO, BOTH_V7_TR_S7, SETANIM_AFLAG_PACE); //TOP RIGHT
				break;
			case SS_DUAL:
				NPC_SetAnim(self, SETANIM_TORSO, BOTH_V6_TR_S6, SETANIM_AFLAG_PACE);
				break;
			default:
				NPC_SetAnim(self, SETANIM_TORSO, BOTH_V1_TR_S1, SETANIM_AFLAG_PACE);
				break;
			}
			self->client->ps.weaponTime = Q_irand(300, 600);
		}
		else if (rightdot < -0.3)
		{
			switch (self->client->ps.saberAnimLevel)
			{
			case SS_STAFF:
				NPC_SetAnim(self, SETANIM_TORSO, BOTH_V7_TL_S7, SETANIM_AFLAG_PACE); //TOP LEFT
				break;
			case SS_DUAL:
				NPC_SetAnim(self, SETANIM_TORSO, BOTH_V6_TL_S6, SETANIM_AFLAG_PACE);
				break;
			default:
				NPC_SetAnim(self, SETANIM_TORSO, BOTH_V1_TL_S1, SETANIM_AFLAG_PACE);
				break;
			}
			self->client->ps.weaponTime = Q_irand(300, 600);
		}
		else
		{
			switch (self->client->ps.saberAnimLevel)
			{
			case SS_STAFF:
				NPC_SetAnim(self, SETANIM_TORSO, BOTH_V7_T__S7, SETANIM_AFLAG_PACE); //TOP TOP
				break;
			case SS_DUAL:
				NPC_SetAnim(self, SETANIM_TORSO, BOTH_V6_T__S6, SETANIM_AFLAG_PACE);
				break;
			default:
				NPC_SetAnim(self, SETANIM_TORSO, BOTH_V1_T__S1, SETANIM_AFLAG_PACE);
				break;
			}
			self->client->ps.weaponTime = Q_irand(300, 600);
		}
	}
	else if (zdiff > -20)
	{
		if (zdiff < -10)
		{
			//hmm, pretty low, but not low enough to use the low block
		}
		if (rightdot > 0.1)
		{
			switch (self->client->ps.saberAnimLevel)
			{
			case SS_STAFF:
				NPC_SetAnim(self, SETANIM_TORSO, BOTH_V7__R_S7, SETANIM_AFLAG_PACE); //TOP RIGHT
				break;
			case SS_DUAL:
				NPC_SetAnim(self, SETANIM_TORSO, BOTH_V6__R_S6, SETANIM_AFLAG_PACE);
				break;
			default:
				NPC_SetAnim(self, SETANIM_TORSO, BOTH_V1__R_S1, SETANIM_AFLAG_PACE);
				break;
			}
			self->client->ps.weaponTime = Q_irand(300, 600);
		}
		else if (rightdot < -0.1)
		{
			switch (self->client->ps.saberAnimLevel)
			{
			case SS_STAFF:
				NPC_SetAnim(self, SETANIM_TORSO, BOTH_V7__L_S7, SETANIM_AFLAG_PACE); //TOP LEFT
				break;
			case SS_DUAL:
				NPC_SetAnim(self, SETANIM_TORSO, BOTH_V6__L_S6, SETANIM_AFLAG_PACE);
				break;
			default:
				NPC_SetAnim(self, SETANIM_TORSO, BOTH_V1_TL_S1, SETANIM_AFLAG_PACE);
				break;
			}
			self->client->ps.weaponTime = Q_irand(300, 600);
		}
		else
		{
			switch (self->client->ps.saberAnimLevel)
			{
			case SS_STAFF:
				NPC_SetAnim(self, SETANIM_TORSO, BOTH_V7_T__S7, SETANIM_AFLAG_PACE); //TOP TOP
				break;
			case SS_DUAL:
				NPC_SetAnim(self, SETANIM_TORSO, BOTH_V6_T__S6, SETANIM_AFLAG_PACE);
				break;
			default:
				NPC_SetAnim(self, SETANIM_TORSO, BOTH_V1_T__S1, SETANIM_AFLAG_PACE);
				break;
			}
			self->client->ps.weaponTime = Q_irand(300, 600);
		}
	}
	else
	{
		if (rightdot >= 0)
		{
			switch (self->client->ps.saberAnimLevel)
			{
			case SS_STAFF:
				NPC_SetAnim(self, SETANIM_TORSO, BOTH_V7_BR_S7, SETANIM_AFLAG_PACE); //BOTTOM RIGHT
				break;
			case SS_DUAL:
				NPC_SetAnim(self, SETANIM_TORSO, BOTH_V6_BR_S6, SETANIM_AFLAG_PACE);
				break;
			default:
				NPC_SetAnim(self, SETANIM_TORSO, BOTH_V1_BR_S1, SETANIM_AFLAG_PACE);
				break;
			}
			self->client->ps.weaponTime = Q_irand(300, 600);
		}
		else
		{
			switch (self->client->ps.saberAnimLevel)
			{
			case SS_STAFF:
				NPC_SetAnim(self, SETANIM_TORSO, BOTH_V7_BL_S7, SETANIM_AFLAG_PACE); //BOTTOM LEFT
				break;
			case SS_DUAL:
				NPC_SetAnim(self, SETANIM_TORSO, BOTH_V6_BL_S6, SETANIM_AFLAG_PACE);
				break;
			default:
				NPC_SetAnim(self, SETANIM_TORSO, BOTH_V1_BL_S1, SETANIM_AFLAG_PACE);
				break;
			}
			self->client->ps.weaponTime = Q_irand(300, 600);
		}
	}

	if (missileBlock)
	{
		self->client->ps.saberBlocked = WP_MissileBlockForBlock(self->client->ps.saberBlocked);
		self->client->ps.weaponTime = Q_irand(300, 600);
	}

	if (self->s.number >= MAX_CLIENTS && !G_ControlledByPlayer(self) && self->client->ps.saberBlocked != BLOCKED_NONE)
	{
		const int parryReCalcTime = Jedi_ReCalcParryTime(self, EVASION_PARRY);
		if (self->client->ps.forcePowerDebounce[FP_SABER_DEFENSE] < level.time + parryReCalcTime)
		{
			self->client->ps.forcePowerDebounce[FP_SABER_DEFENSE] = level.time + parryReCalcTime;
		}
	}

	self->client->ps.userInt3 &= ~(1 << FLAG_PREBLOCK);
	return qtrue;
}

//missile blocks

qboolean wp_saber_block_non_random_missile(gentity_t* self, vec3_t hitloc, const qboolean missileBlock)
{
	vec3_t diff, fwdangles = { 0, 0, 0 }, right;
	const qboolean inFront = in_front(hitloc, self->client->ps.origin, self->client->ps.viewangles, -0.7f);

	VectorSubtract(hitloc, self->client->renderInfo.eyePoint, diff);
	diff[2] = 0;
	VectorNormalize(diff);
	fwdangles[1] = self->client->ps.viewangles[1];
	// Ultimately we might care if the shot was ahead or behind, but for now, just quadrant is fine.
	AngleVectors(fwdangles, nullptr, right, nullptr);

	const float rightdot = DotProduct(right, diff);
	const float zdiff = hitloc[2] - self->client->renderInfo.eyePoint[2];

	if (self->client->ps.weaponstate == WEAPON_DROPPING ||
		self->client->ps.weaponstate == WEAPON_RAISING)
	{
		return qfalse;
	}

	if (PM_SaberInAttack(self->client->ps.saberMove) ||
		PM_SuperBreakLoseAnim(self->client->ps.torsoAnim) ||
		PM_SuperBreakWinAnim(self->client->ps.torsoAnim) ||
		PM_SaberInBrokenParry(self->client->ps.saberMove) ||
		PM_SaberInKnockaway(self->client->ps.saberMove) ||
		PM_InRoll(&self->client->ps) ||
		BG_InKnockDown(self->client->ps.legsAnim) ||
		BG_InKnockDown(self->client->ps.torsoAnim) ||
		PM_InKnockDown(&self->client->ps))
	{
		return qfalse;
	}

	if (PM_SaberInMassiveBounce(self->client->ps.torsoAnim) || PM_SaberInBashedAnim(self->client->ps.torsoAnim))
	{
		return qfalse;
	}

	if (self->client->ps.blockPoints <= BLOCKPOINTS_FAIL || self->client->ps.forcePower <= BLOCKPOINTS_DANGER)
	{
		return qfalse;
	}

	if (!inFront && self->client->ps.forcePowerLevel[FP_SABER_DEFENSE] >= FORCE_LEVEL_1)
	{
		switch (self->client->ps.saberAnimLevel)
		{
		case SS_STAFF:
			NPC_SetAnim(self, SETANIM_TORSO, BOTH_P7_S1_B_, SETANIM_AFLAG_PACE);
			break;
		case SS_DUAL:
			NPC_SetAnim(self, SETANIM_TORSO, BOTH_P6_S1_B_, SETANIM_AFLAG_PACE);
			break;
		default:
			NPC_SetAnim(self, SETANIM_TORSO, BOTH_P1_S1_B_, SETANIM_AFLAG_PACE);
			break;
		}
		self->client->ps.weaponTime = Q_irand(300, 600);
	}
	else if (zdiff > -5)
	{
		if (rightdot > 0.3)
		{
			switch (self->client->ps.saberAnimLevel)
			{
			case SS_STAFF:
				NPC_SetAnim(self, SETANIM_TORSO, BOTH_P7_S7_TR, SETANIM_AFLAG_PACE);
				break;
			case SS_DUAL:
				NPC_SetAnim(self, SETANIM_TORSO, BOTH_P6_S6_TR, SETANIM_AFLAG_PACE);
				break;
			default:
				NPC_SetAnim(self, SETANIM_TORSO, BOTH_P1_S1_TR, SETANIM_AFLAG_PACE);
				break;
			}
			self->client->ps.weaponTime = Q_irand(300, 600);
		}
		else if (rightdot < -0.3)
		{
			switch (self->client->ps.saberAnimLevel)
			{
			case SS_STAFF:
				NPC_SetAnim(self, SETANIM_TORSO, BOTH_P7_S7_TL, SETANIM_AFLAG_PACE);
				break;
			case SS_DUAL:
				NPC_SetAnim(self, SETANIM_TORSO, BOTH_P6_S6_TL, SETANIM_AFLAG_PACE);
				break;
			default:
				NPC_SetAnim(self, SETANIM_TORSO, BOTH_P1_S1_TL, SETANIM_AFLAG_PACE);
				break;
			}
			self->client->ps.weaponTime = Q_irand(300, 600);
		}
		else
		{
			switch (self->client->ps.saberAnimLevel)
			{
			case SS_STAFF:
				NPC_SetAnim(self, SETANIM_TORSO, BOTH_P7_S7_T_, SETANIM_AFLAG_PACE);
				break;
			case SS_DUAL:
				NPC_SetAnim(self, SETANIM_TORSO, BOTH_P6_S6_T_, SETANIM_AFLAG_PACE);
				break;
			default:
				NPC_SetAnim(self, SETANIM_TORSO, BOTH_P1_S1_T_, SETANIM_AFLAG_PACE);
				break;
			}
			self->client->ps.weaponTime = Q_irand(300, 600);
		}
	}
	else if (zdiff > -20)
	{
		if (zdiff < -10)
		{
			//hmm, pretty low, but not low enough to use the low block
		}
		if (rightdot > 0.1)
		{
			switch (self->client->ps.saberAnimLevel)
			{
			case SS_STAFF:
				NPC_SetAnim(self, SETANIM_TORSO, BOTH_P7_S7_TR, SETANIM_AFLAG_PACE);
				break;
			case SS_DUAL:
				NPC_SetAnim(self, SETANIM_TORSO, BOTH_P6_S6_TR, SETANIM_AFLAG_PACE);
				break;
			default:
				NPC_SetAnim(self, SETANIM_TORSO, BOTH_P1_S1_TR, SETANIM_AFLAG_PACE);
				break;
			}
			self->client->ps.weaponTime = Q_irand(300, 600);
		}
		else if (rightdot < -0.1)
		{
			switch (self->client->ps.saberAnimLevel)
			{
			case SS_STAFF:
				NPC_SetAnim(self, SETANIM_TORSO, BOTH_P7_S7_TL, SETANIM_AFLAG_PACE);
				break;
			case SS_DUAL:
				NPC_SetAnim(self, SETANIM_TORSO, BOTH_P6_S6_TL, SETANIM_AFLAG_PACE);
				break;
			default:
				NPC_SetAnim(self, SETANIM_TORSO, BOTH_P1_S1_TL, SETANIM_AFLAG_PACE);
				break;
			}
			self->client->ps.weaponTime = Q_irand(300, 600);
		}
		else
		{
			switch (self->client->ps.saberAnimLevel)
			{
			case SS_STAFF:
				NPC_SetAnim(self, SETANIM_TORSO, BOTH_P7_S7_T_, SETANIM_AFLAG_PACE);
				break;
			case SS_DUAL:
				NPC_SetAnim(self, SETANIM_TORSO, BOTH_P6_S6_T_, SETANIM_AFLAG_PACE);
				break;
			default:
				NPC_SetAnim(self, SETANIM_TORSO, BOTH_P1_S1_T_, SETANIM_AFLAG_PACE);
				break;
			}
			self->client->ps.weaponTime = Q_irand(300, 600);
		}
	}
	else
	{
		if (rightdot >= 0)
		{
			switch (self->client->ps.saberAnimLevel)
			{
			case SS_STAFF:
				NPC_SetAnim(self, SETANIM_TORSO, BOTH_P7_S7_BR, SETANIM_AFLAG_PACE);
				break;
			case SS_DUAL:
				NPC_SetAnim(self, SETANIM_TORSO, BOTH_P6_S6_BR, SETANIM_AFLAG_PACE);
				break;
			default:
				NPC_SetAnim(self, SETANIM_TORSO, BOTH_P1_S1_BR, SETANIM_AFLAG_PACE);
				break;
			}
			self->client->ps.weaponTime = Q_irand(300, 600);
		}
		else
		{
			switch (self->client->ps.saberAnimLevel)
			{
			case SS_STAFF:
				NPC_SetAnim(self, SETANIM_TORSO, BOTH_P7_S7_BL, SETANIM_AFLAG_PACE);
				break;
			case SS_DUAL:
				NPC_SetAnim(self, SETANIM_TORSO, BOTH_P6_S6_BL, SETANIM_AFLAG_PACE);
				break;
			default:
				NPC_SetAnim(self, SETANIM_TORSO, BOTH_P1_S1_BL, SETANIM_AFLAG_PACE);
				break;
			}
			self->client->ps.weaponTime = Q_irand(300, 600);
		}
	}

	if (missileBlock)
	{
		self->client->ps.saberBlocked = WP_MissileBlockForBlock(self->client->ps.saberBlocked);
		self->client->ps.weaponTime = Q_irand(300, 600);
	}

	if (self->s.number >= MAX_CLIENTS && !G_ControlledByPlayer(self) && self->client->ps.saberBlocked != BLOCKED_NONE)
	{
		const int parryReCalcTime = Jedi_ReCalcParryTime(self, EVASION_PARRY);
		if (self->client->ps.forcePowerDebounce[FP_SABER_DEFENSE] < level.time + parryReCalcTime)
		{
			self->client->ps.forcePowerDebounce[FP_SABER_DEFENSE] = level.time + parryReCalcTime;
		}
	}

	self->client->ps.userInt3 &= ~(1 << FLAG_PREBLOCK);
	return qtrue;
}

qboolean WP_SaberBlockBolt(gentity_t* self, vec3_t hitloc, const qboolean missileBlock)
{
	vec3_t diff, fwdangles = { 0, 0, 0 }, right;
	const qboolean inFront = in_front(hitloc, self->client->ps.origin, self->client->ps.viewangles, -0.6f);

	VectorSubtract(hitloc, self->client->renderInfo.eyePoint, diff);
	diff[2] = 0;
	VectorNormalize(diff);
	fwdangles[1] = self->client->ps.viewangles[1];
	// Ultimately we might care if the shot was ahead or behind, but for now, just quadrant is fine.
	AngleVectors(fwdangles, nullptr, right, nullptr);

	const float rightdot = DotProduct(right, diff);
	const float zdiff = hitloc[2] - self->client->renderInfo.eyePoint[2];

	if (!inFront && self->client->ps.forcePowerLevel[FP_SABER_DEFENSE] >= FORCE_LEVEL_1)
	{
		switch (self->client->ps.saberAnimLevel)
		{
		case SS_STAFF:
			NPC_SetAnim(self, SETANIM_TORSO, BOTH_P7_S1_B_, SETANIM_AFLAG_PACE);
			break;
		case SS_DUAL:
			NPC_SetAnim(self, SETANIM_TORSO, BOTH_P6_S1_B_, SETANIM_AFLAG_PACE);
			break;
		default:
			NPC_SetAnim(self, SETANIM_TORSO, BOTH_P1_S1_B_, SETANIM_AFLAG_PACE);
			break;
		}
		self->client->ps.weaponTime = Q_irand(300, 600);
	}
	else if (zdiff > -5)
	{
		if (rightdot > 0.3)
		{
			switch (self->client->ps.saberAnimLevel)
			{
			case SS_STAFF:
				NPC_SetAnim(self, SETANIM_TORSO, BOTH_R7_TR_S7, SETANIM_AFLAG_PACE);
				break;
			case SS_DUAL:
				NPC_SetAnim(self, SETANIM_TORSO, BOTH_R6_TR_S6, SETANIM_AFLAG_PACE);
				break;
			default:
				NPC_SetAnim(self, SETANIM_TORSO, BOTH_R1_TR_S1, SETANIM_AFLAG_PACE);
				break;
			}
		}
		else if (rightdot < -0.3)
		{
			switch (self->client->ps.saberAnimLevel)
			{
			case SS_STAFF:
				NPC_SetAnim(self, SETANIM_TORSO, BOTH_R7_TL_S7, SETANIM_AFLAG_PACE);
				break;
			case SS_DUAL:
				NPC_SetAnim(self, SETANIM_TORSO, BOTH_R6_TL_S6, SETANIM_AFLAG_PACE);
				break;
			default:
				NPC_SetAnim(self, SETANIM_TORSO, BOTH_R1_TL_S1, SETANIM_AFLAG_PACE);
				break;
			}
		}
		else
		{
			switch (self->client->ps.saberAnimLevel)
			{
			case SS_STAFF:
				NPC_SetAnim(self, SETANIM_TORSO, BOTH_K7_S7_T_, SETANIM_AFLAG_PACE);
				break;
			case SS_DUAL:
				NPC_SetAnim(self, SETANIM_TORSO, BOTH_K6_S6_T_, SETANIM_AFLAG_PACE);
				break;
			default:
				NPC_SetAnim(self, SETANIM_TORSO, BOTH_SABER_BLOCKBOLT, SETANIM_AFLAG_PACE);
				break;
			}
		}
	}
	else if (zdiff > -20)
	{
		if (zdiff < -10)
		{
			//hmm, pretty low, but not low enough to use the low block
		}
		if (rightdot > 0.1)
		{
			switch (self->client->ps.saberAnimLevel)
			{
			case SS_STAFF:
				NPC_SetAnim(self, SETANIM_TORSO, BOTH_R7_TR_S7, SETANIM_AFLAG_PACE);
				break;
			case SS_DUAL:
				NPC_SetAnim(self, SETANIM_TORSO, BOTH_R6_TR_S6, SETANIM_AFLAG_PACE);
				break;
			default:
				NPC_SetAnim(self, SETANIM_TORSO, BOTH_R1_TR_S1, SETANIM_AFLAG_PACE);
				break;
			}
		}
		else if (rightdot < -0.1)
		{
			switch (self->client->ps.saberAnimLevel)
			{
			case SS_STAFF:
				NPC_SetAnim(self, SETANIM_TORSO, BOTH_R7_TL_S7, SETANIM_AFLAG_PACE);
				break;
			case SS_DUAL:
				NPC_SetAnim(self, SETANIM_TORSO, BOTH_R6_TL_S6, SETANIM_AFLAG_PACE);
				break;
			default:
				NPC_SetAnim(self, SETANIM_TORSO, BOTH_R1_TL_S1, SETANIM_AFLAG_PACE);
				break;
			}
		}
		else
		{
			switch (self->client->ps.saberAnimLevel)
			{
			case SS_STAFF:
				NPC_SetAnim(self, SETANIM_TORSO, BOTH_K7_S7_T_, SETANIM_AFLAG_PACE);
				break;
			case SS_DUAL:
				NPC_SetAnim(self, SETANIM_TORSO, BOTH_K6_S6_T_, SETANIM_AFLAG_PACE);
				break;
			default:
				NPC_SetAnim(self, SETANIM_TORSO, BOTH_SABER_BLOCKBOLT, SETANIM_AFLAG_PACE);
				break;
			}
		}
	}
	else
	{
		if (rightdot >= 0)
		{
			switch (self->client->ps.saberAnimLevel)
			{
			case SS_STAFF:
				NPC_SetAnim(self, SETANIM_TORSO, BOTH_P7_S7_BR, SETANIM_AFLAG_PACE);
				break;
			case SS_DUAL:
				NPC_SetAnim(self, SETANIM_TORSO, BOTH_P6_S6_BR, SETANIM_AFLAG_PACE);
				break;
			default:
				NPC_SetAnim(self, SETANIM_TORSO, BOTH_P1_S1_BR, SETANIM_AFLAG_PACE);
				break;
			}
		}
		else
		{
			switch (self->client->ps.saberAnimLevel)
			{
			case SS_STAFF:
				NPC_SetAnim(self, SETANIM_TORSO, BOTH_P7_S7_BL, SETANIM_AFLAG_PACE);
				break;
			case SS_DUAL:
				NPC_SetAnim(self, SETANIM_TORSO, BOTH_P6_S6_BL, SETANIM_AFLAG_PACE);
				break;
			default:
				NPC_SetAnim(self, SETANIM_TORSO, BOTH_P1_S1_BL, SETANIM_AFLAG_PACE);
				break;
			}
		}
	}

	if (missileBlock)
	{
		self->client->ps.saberBlocked = WP_MissileBlockForBlock(self->client->ps.saberBlocked);
		self->client->ps.weaponTime = Q_irand(300, 600);
	}

	if (self->s.number >= MAX_CLIENTS && !G_ControlledByPlayer(self) && self->client->ps.saberBlocked != BLOCKED_NONE)
	{
		const int parryReCalcTime = Jedi_ReCalcParryTime(self, EVASION_PARRY);
		if (self->client->ps.forcePowerDebounce[FP_SABER_DEFENSE] < level.time + parryReCalcTime)
		{
			self->client->ps.forcePowerDebounce[FP_SABER_DEFENSE] = level.time + parryReCalcTime;
		}
	}

	self->client->ps.userInt3 &= ~(1 << FLAG_PREBLOCK);
	return qtrue;
}

qboolean WP_SaberParryNonRandom(gentity_t* self, vec3_t hitloc, const qboolean missileBlock)
{
	vec3_t diff, fwdangles = { 0, 0, 0 }, right;
	const qboolean inFront = in_front(hitloc, self->client->ps.origin, self->client->ps.viewangles, -0.6f);

	VectorSubtract(hitloc, self->client->renderInfo.eyePoint, diff);
	diff[2] = 0;
	VectorNormalize(diff);
	fwdangles[1] = self->client->ps.viewangles[1];
	// Ultimately we might care if the shot was ahead or behind, but for now, just quadrant is fine.
	AngleVectors(fwdangles, nullptr, right, nullptr);

	const float rightdot = DotProduct(right, diff);
	const float zdiff = hitloc[2] - self->client->renderInfo.eyePoint[2];

	if (!inFront && self->client->ps.forcePowerLevel[FP_SABER_DEFENSE] >= FORCE_LEVEL_1)
	{
		switch (self->client->ps.saberAnimLevel)
		{
		case SS_STAFF:
			NPC_SetAnim(self, SETANIM_TORSO, BOTH_P7_S1_B_, SETANIM_AFLAG_PACE);
			break;
		case SS_DUAL:
			NPC_SetAnim(self, SETANIM_TORSO, BOTH_P6_S1_B_, SETANIM_AFLAG_PACE);
			break;
		default:
			NPC_SetAnim(self, SETANIM_TORSO, BOTH_P1_S1_B_, SETANIM_AFLAG_PACE);
			break;
		}
		self->client->ps.weaponTime = Q_irand(300, 600);
	}
	else if (zdiff > -5)
	{
		if (rightdot > 0.3)
		{
			switch (self->client->ps.saberAnimLevel)
			{
			case SS_STAFF:
				NPC_SetAnim(self, SETANIM_TORSO, BOTH_P7_S7_TR, SETANIM_AFLAG_PACE);
				break;
			case SS_DUAL:
				NPC_SetAnim(self, SETANIM_TORSO, BOTH_P6_S6_TR, SETANIM_AFLAG_PACE);
				break;
			default:
				NPC_SetAnim(self, SETANIM_TORSO, BOTH_P1_S1_TR, SETANIM_AFLAG_PACE);
				break;
			}
			self->client->ps.weaponTime = Q_irand(300, 600);
		}
		else if (rightdot < -0.3)
		{
			switch (self->client->ps.saberAnimLevel)
			{
			case SS_STAFF:
				NPC_SetAnim(self, SETANIM_TORSO, BOTH_P7_S7_TL, SETANIM_AFLAG_PACE);
				break;
			case SS_DUAL:
				NPC_SetAnim(self, SETANIM_TORSO, BOTH_P6_S6_TL, SETANIM_AFLAG_PACE);
				break;
			default:
				NPC_SetAnim(self, SETANIM_TORSO, BOTH_P1_S1_TL, SETANIM_AFLAG_PACE);
				break;
			}
			self->client->ps.weaponTime = Q_irand(300, 600);
		}
		else
		{
			switch (self->client->ps.saberAnimLevel)
			{
			case SS_STAFF:
				NPC_SetAnim(self, SETANIM_TORSO, BOTH_P7_S7_T_, SETANIM_AFLAG_PACE);
				break;
			case SS_DUAL:
				NPC_SetAnim(self, SETANIM_TORSO, BOTH_P6_S6_T_, SETANIM_AFLAG_PACE);
				break;
			default:
				NPC_SetAnim(self, SETANIM_TORSO, BOTH_P1_S1_T_, SETANIM_AFLAG_PACE);
				break;
			}
			self->client->ps.weaponTime = Q_irand(300, 600);
		}
	}
	else if (zdiff > -20)
	{
		if (zdiff < -10)
		{
			//hmm, pretty low, but not low enough to use the low block, so we need to duck
		}
		if (rightdot > 0.1)
		{
			switch (self->client->ps.saberAnimLevel)
			{
			case SS_STAFF:
				NPC_SetAnim(self, SETANIM_TORSO, BOTH_P7_S7_TR, SETANIM_AFLAG_PACE);
				break;
			case SS_DUAL:
				NPC_SetAnim(self, SETANIM_TORSO, BOTH_P6_S6_TR, SETANIM_AFLAG_PACE);
				break;
			default:
				NPC_SetAnim(self, SETANIM_TORSO, BOTH_P1_S1_TR, SETANIM_AFLAG_PACE);
				break;
			}
			self->client->ps.weaponTime = Q_irand(300, 600);
		}
		else if (rightdot < -0.1)
		{
			switch (self->client->ps.saberAnimLevel)
			{
			case SS_STAFF:
				NPC_SetAnim(self, SETANIM_TORSO, BOTH_P7_S7_TL, SETANIM_AFLAG_PACE);
				break;
			case SS_DUAL:
				NPC_SetAnim(self, SETANIM_TORSO, BOTH_P6_S6_TL, SETANIM_AFLAG_PACE);
				break;
			default:
				NPC_SetAnim(self, SETANIM_TORSO, BOTH_P1_S1_TL, SETANIM_AFLAG_PACE);
				break;
			}
			self->client->ps.weaponTime = Q_irand(300, 600);
		}
		else
		{
			switch (self->client->ps.saberAnimLevel)
			{
			case SS_STAFF:
				NPC_SetAnim(self, SETANIM_TORSO, BOTH_P7_S7_T_, SETANIM_AFLAG_PACE);
				break;
			case SS_DUAL:
				NPC_SetAnim(self, SETANIM_TORSO, BOTH_P6_S6_T_, SETANIM_AFLAG_PACE);
				break;
			default:
				NPC_SetAnim(self, SETANIM_TORSO, BOTH_P1_S1_T_, SETANIM_AFLAG_PACE);
				break;
			}
			self->client->ps.weaponTime = Q_irand(300, 600);
		}
	}
	else
	{
		if (rightdot >= 0)
		{
			switch (self->client->ps.saberAnimLevel)
			{
			case SS_STAFF:
				NPC_SetAnim(self, SETANIM_TORSO, BOTH_P7_S7_BR, SETANIM_AFLAG_PACE);
				break;
			case SS_DUAL:
				NPC_SetAnim(self, SETANIM_TORSO, BOTH_P6_S6_BR, SETANIM_AFLAG_PACE);
				break;
			default:
				NPC_SetAnim(self, SETANIM_TORSO, BOTH_P1_S1_BR, SETANIM_AFLAG_PACE);
				break;
			}
			self->client->ps.weaponTime = Q_irand(300, 600);
		}
		else
		{
			switch (self->client->ps.saberAnimLevel)
			{
			case SS_STAFF:
				NPC_SetAnim(self, SETANIM_TORSO, BOTH_P7_S7_BL, SETANIM_AFLAG_PACE);
				break;
			case SS_DUAL:
				NPC_SetAnim(self, SETANIM_TORSO, BOTH_P6_S6_BL, SETANIM_AFLAG_PACE);
				break;
			default:
				NPC_SetAnim(self, SETANIM_TORSO, BOTH_P1_S1_BL, SETANIM_AFLAG_PACE);
				break;
			}
			self->client->ps.weaponTime = Q_irand(300, 600);
		}
	}

	if (missileBlock)
	{
		self->client->ps.saberBlocked = WP_MissileBlockForBlock(self->client->ps.saberBlocked);
		self->client->ps.weaponTime = Q_irand(300, 600);
	}

	if (self->s.number >= MAX_CLIENTS && !G_ControlledByPlayer(self) && self->client->ps.saberBlocked != BLOCKED_NONE)
	{
		const int parryReCalcTime = Jedi_ReCalcParryTime(self, EVASION_PARRY);
		if (self->client->ps.forcePowerDebounce[FP_SABER_DEFENSE] < level.time + parryReCalcTime)
		{
			self->client->ps.forcePowerDebounce[FP_SABER_DEFENSE] = level.time + parryReCalcTime;
		}
	}

	self->client->ps.userInt3 &= ~(1 << FLAG_PREBLOCK);
	return qtrue;
}

extern int blockedfor_quad(int quad);
extern int invert_quad(int quad);

void wp_saber_start_missile_block_check(gentity_t* self, const usercmd_t* ucmd)
{
	qboolean closest_swing_block = qfalse; //default setting makes the compiler happy.
	int swing_block_quad = Q_T;
	int closest_swing_quad = Q_T;
	gentity_t* incoming = nullptr;
	gentity_t* entityList[MAX_GENTITIES];
	vec3_t mins, maxs;
	constexpr float radius = 256;
	vec3_t forward, fwdangles = { 0 };
	trace_t trace;
	vec3_t trace_to, ent_dir;
	qboolean dodge_only_sabers = qfalse;
	qboolean do_full_routine = qtrue;

	if (self->client->ps.weapon != WP_SABER) //saber not here
	{
		//saber not currently in use or available, attempt to use our hands instead.
		do_full_routine = qfalse;
	}

	if (self->client->ps.weapon == WP_SABER && !self->client->ps.SaberActive())
	{
		//saber not currently in use or available, attempt to use our hands instead.
		do_full_routine = qfalse;
	}

	if (self->client->ps.weapon == WP_SABER && self->client->ps.saberInFlight)
	{
		//saber not currently in use or available, attempt to use our hands instead.
		do_full_routine = qfalse;
	}

	if ((self->s.number < MAX_CLIENTS || G_ControlledByPlayer(self)) && !(self->client->ps.ManualBlockingFlags & 1 << HOLDINGBLOCK))
	{
		return;
	}

	if (self->client->ps.SaberLength() <= 0)
	{
		//not on
		return;
	}

	if (!walk_check(self)
		&& (PM_SaberInAttack(self->client->ps.saberMove)
			|| PM_SaberInStart(self->client->ps.saberMove)))
	{
		//this was put in to help bolts stop swings a bit. I dont know why it helps but it does :p
		do_full_routine = qfalse;
	}

	if (self->client->ps.weaponTime > 0 && !PM_SaberInParry(self->client->ps.saberMove))
	{
		//don't autoblock while busy with stuff
		return;
	}

	if (self->NPC && self->NPC->scriptFlags & SCF_IGNORE_ALERTS)
	{
		//don't react to things flying at me...
		return;
	}
	if (self->health <= 0)
	{
		//dead don't try to block (NOTE: actual deflection happens in missile code)
		return;
	}

	if (PM_InKnockDown(&self->client->ps))
	{
		//can't block when knocked down
		return;
	}

	if (PM_SuperBreakLoseAnim(self->client->ps.torsoAnim)
		|| PM_SuperBreakWinAnim(self->client->ps.torsoAnim))
	{
		//can't block while in break anim
		return;
	}

	if (Rosh_BeingHealed(self))
	{
		return;
	}

	if (self->client->NPC_class == CLASS_ROCKETTROOPER)
	{
		//rockettrooper
		if (self->client->ps.groundEntityNum != ENTITYNUM_NONE)
		{
			//must be in air
			return;
		}
		if (Q_irand(0, 4 - g_spskill->integer * 2))
		{
			//easier level guys do this less
			return;
		}
		if (Q_irand(0, 3))
		{
			//base level: 25% chance of looking for something to dodge
			if (Q_irand(0, 1))
			{
				//dodge sabers twice as frequently as other projectiles
				dodge_only_sabers = qtrue;
			}
			else
			{
				return;
			}
		}
	}

	if (self->client->NPC_class == CLASS_BOBAFETT || self->client->NPC_class == CLASS_MANDO)
	{
		//Boba doesn't dodge quite as much
		if (Q_irand(0, 2 - g_spskill->integer))
		{
			//easier level guys do this less
			return;
		}
	}

	if (self->client->NPC_class != CLASS_BOBAFETT && self->client->NPC_class != CLASS_MANDO
		&& (self->client->NPC_class != CLASS_REBORN || self->s.weapon == WP_SABER)
		&& (self->client->NPC_class != CLASS_ROCKETTROOPER || !self->NPC || self->NPC->rank < RANK_LT))
	{
		if (ucmd->buttons & BUTTON_USE && !(ucmd->buttons & BUTTON_WALKING)
			&& cg.renderingThirdPerson
			&& G_OkayToLean(&self->client->ps, ucmd, qfalse))
		{
			//HES DOING DODGING INSTEAD OF BLOCKING
		}
		else
		{
			if (self->client->ps.weapon != WP_SABER)
			{
				return;
			}

			if (self->client->ps.saberInFlight)
			{
				return;
			}

			if (self->s.number < MAX_CLIENTS)
			{
				if (!self->client->ps.SaberLength())
				{
					//player doesn't auto-activate
					return;
				}

				if (!(self->client->ps.ManualBlockingFlags & 1 << HOLDINGBLOCK) && self->client->ps.saberBlockingTime <
					level.time)
				{
					return;
				}
			}

			if (self->client->ps.saber[0].saberFlags & SFL_NOT_ACTIVE_BLOCKING)
			{
				//can't actively block with this saber type
				return;
			}
		}

		if (!self->s.number)
		{
			//don't do this if already attacking!
			if (ucmd->buttons & BUTTON_ATTACK
				|| PM_SaberInAttack(self->client->ps.saberMove)
				|| pm_saber_in_special_attack(self->client->ps.torsoAnim)
				|| PM_SaberInTransitionAny(self->client->ps.saberMove))
			{
				return;
			}
		}

		if (self->client->ps.forcePowerLevel[FP_SABER_DEFENSE] < FORCE_LEVEL_1)
		{
			//you have not the SKILLZ
			return;
		}

		if (self->client->ps.forcePowerDebounce[FP_SABER_DEFENSE] > level.time)
		{
			//can't block while already blocking
			do_full_routine = qfalse;
		}

		if (self->client->ps.forcePowersActive & 1 << FP_LIGHTNING)
		{
			//can't block while zapping
			do_full_routine = qfalse;
		}

		if (self->client->ps.forcePowersActive & 1 << FP_DRAIN)
		{
			//can't block while draining
			do_full_routine = qfalse;
		}

		if (self->client->ps.forcePowersActive & 1 << FP_PUSH)
		{
			//can't block while shoving
			do_full_routine = qfalse;
		}

		if (self->client->ps.forcePowersActive & 1 << FP_GRIP)
		{
			//can't block while gripping (FIXME: or should it break the grip?  Pain should break the grip, I think...)
			do_full_routine = qfalse;
		}

		if (self->client->ps.weaponTime > 0 && !PM_SaberInParry(self->client->ps.saberMove))
		{
			//don't auto block while busy with stuff
			return;
		}

		if (self->client->ps.SaberLength() <= 0)
		{
			//not on
			return;
		}
	}

	fwdangles[1] = self->client->ps.viewangles[1];
	AngleVectors(fwdangles, forward, nullptr, nullptr);

	for (int i = 0; i < 3; i++)
	{
		mins[i] = self->currentOrigin[i] - radius;
		maxs[i] = self->currentOrigin[i] + radius;
	}

	const int num_listed_entities = gi.EntitiesInBox(mins, maxs, entityList, MAX_GENTITIES);

	float closest_dist = radius;

	for (int e = 0; e < num_listed_entities; e++)
	{
		vec3_t dir;
		gentity_t* ent = entityList[e];
		qboolean swing_block = qfalse;

		if (ent == self)
			continue;
		if (ent->owner == self)
			continue;
		if (!ent->inuse)
			continue;
		if (dodge_only_sabers)
		{
			//only care about thrown sabers
			if (ent->client
				|| ent->s.weapon != WP_SABER
				|| !ent->classname
				|| !ent->classname[0]
				|| Q_stricmp("lightsaber", ent->classname))
			{
				//not a lightsaber, ignore it
				continue;
			}
		}
		if (ent->s.eType != ET_MISSILE && !(ent->s.eFlags & EF_MISSILE_STICK))
		{
			//not a normal projectile
			if (ent->client || ent->s.weapon != WP_SABER)
			{
				//FIXME: wake up bad guys?
				continue;
			}
			if (ent->s.eFlags & EF_NODRAW)
			{
				continue;
			}
			if (Q_stricmp("lightsaber", ent->classname) != 0)
			{
				//not a lightsaber
				//FIXME: what about general objects that are small in size- like rocks, etc...
				continue;
			}
			//a lightsaber.. make sure it's on and inFlight
			if (!ent->owner || !ent->owner->client)
			{
				continue;
			}
			if (ent->owner->client->ps.SaberLength() <= 0)
			{
				//not on
				continue;
			}
			if (ent->owner->health <= 0 && g_saberRealisticCombat->integer < 1)
			{
				//it's not doing damage, so ignore it
				continue;
			}

			if (!ent->owner->client->ps.saberInFlight)
			{
				//active saber blade, treat differently.//allow the blocking of normal saber swings
				swing_block = qtrue;
				if (BG_SaberInNonIdleDamageMove(&self->client->ps))
				{
					//attacking
					swing_block_quad = invert_quad(saberMoveData[self->client->ps.saberMove].startQuad);
				}
				else if (PM_SaberInStart(self->client->ps.saberMove) ||
					PM_SaberInTransition(self->client->ps.saberMove))
				{
					//preparing to attack
					swing_block_quad = invert_quad(saberMoveData[self->client->ps.saberMove].endQuad);
				}
				else
				{
					//not attacking
					continue;
				}
			}
		}
		else
		{
			if (ent->s.pos.trType == TR_STATIONARY && !self->s.number)
			{
				//nothing you can do with a stationary missile if you're the player
				continue;
			}
		}

		// ReSharper disable once CppEntityAssignedButNoRead
		float dot1;
		//see if they're in front of me
		VectorSubtract(ent->currentOrigin, self->currentOrigin, dir);
		const float dist = VectorNormalize(dir);

		if (dist > 150 && swing_block)
		{
			//don't block swings that are too far away.
			continue;
		}

		//handle detpacks, proximity mines and trip mines
		if (ent->s.weapon == WP_THERMAL)
		{
			//thermal detonator!
			if (self->NPC && dist < ent->splashRadius)
			{
				if (dist < ent->splashRadius &&
					ent->nextthink < level.time + 600 &&
					ent->count &&
					self->client->ps.groundEntityNum != ENTITYNUM_NONE &&
					(ent->s.pos.trType == TR_STATIONARY ||
						ent->s.pos.trType == TR_INTERPOLATE ||
						(dot1 = DotProduct(dir, forward)) < SABER_REFLECT_MISSILE_CONE))
				{
					//TD is close enough to hurt me, I'm on the ground and the thing is at rest or behind me and about to blow up, or I don't have force-push so force-jump!
					if (self->client->NPC_class == CLASS_BOBAFETT
						|| self->client->NPC_class == CLASS_MANDO
						|| self->client->NPC_class == CLASS_ROCKETTROOPER)
					{
						//jump out of the way
						self->client->ps.forceJumpCharge = 480;
						PM_AddFatigue(&self->client->ps, FORCE_LONGJUMP_POWER);
					}
				}
				else if (self->client->ps.ManualBlockingFlags & 1 << HOLDINGBLOCK
					&& self->client->NPC_class != CLASS_BOBAFETT
					&& self->client->NPC_class != CLASS_MANDO
					&& self->client->NPC_class != CLASS_ROCKETTROOPER)
				{
					if (!ent->owner || !OnSameTeam(self, ent->owner))
					{
						ForceThrow(self, qfalse);
						PM_AddFatigue(&self->client->ps, FORCE_DEFLECT_PUSH);
					}
				}
			}
			continue;
		}
		if (ent->splashDamage && ent->splashRadius && !(ent->s.powerups & 1 << PW_FORCE_PROJECTILE))
		{
			//exploding missile
			if (ent->s.pos.trType == TR_STATIONARY && ent->s.eFlags & EF_MISSILE_STICK)
			{
				//a placed explosive like a trip mine or detpacks
				if (InFOV(ent->currentOrigin, self->client->renderInfo.eyePoint, self->client->ps.viewangles, 90, 90))
				{
					//in front of me
					if (G_ClearLOS(self, ent))
					{
						//can see it
						vec3_t throw_dir;
						//make the gesture
						if (self->client->ps.ManualBlockingFlags & 1 << HOLDINGBLOCK
							&& self->client->NPC_class != CLASS_BOBAFETT
							&& self->client->NPC_class != CLASS_MANDO
							&& self->client->NPC_class != CLASS_ROCKETTROOPER)
						{
							ForceThrow(self, qfalse);
							PM_AddFatigue(&self->client->ps, FORCE_DEFLECT_PUSH);
						}
						//take it off the wall and toss it
						ent->s.pos.trType = TR_GRAVITY;
						ent->s.eType = ET_MISSILE;
						ent->s.eFlags &= ~EF_MISSILE_STICK;
						ent->s.eFlags |= EF_BOUNCE_HALF;
						AngleVectors(ent->currentAngles, throw_dir, nullptr, nullptr);
						VectorMA(ent->currentOrigin, ent->maxs[0] + 4, throw_dir, ent->currentOrigin);
						VectorCopy(ent->currentOrigin, ent->s.pos.trBase);
						VectorScale(throw_dir, 300, ent->s.pos.trDelta);
						ent->s.pos.trDelta[2] += 150;
						VectorMA(ent->s.pos.trDelta, 800, dir, ent->s.pos.trDelta);
						ent->s.pos.trTime = level.time; // move a bit on the very first frame
						VectorCopy(ent->currentOrigin, ent->s.pos.trBase);
						ent->owner = self;
						// make it explode, but with less damage
						ent->splashDamage /= 3;
						ent->splashRadius /= 3;
						ent->e_ThinkFunc = thinkF_WP_Explode;
						ent->nextthink = level.time + Q_irand(500, 3000);
					}
				}
			}
			else if (self->s.number
				&& dist < ent->splashRadius
				&& self->client->ps.groundEntityNum != ENTITYNUM_NONE
				&& DotProduct(dir, forward) < SABER_REFLECT_MISSILE_CONE)
			{
				//try to evade it
				if (self->client->NPC_class == CLASS_BOBAFETT
					|| self->client->NPC_class == CLASS_MANDO
					|| self->client->NPC_class == CLASS_ROCKETTROOPER)
				{
					//jump out of the way
					self->client->ps.forceJumpCharge = 480;
					PM_AddFatigue(&self->client->ps, FORCE_LONGJUMP_POWER);
				}
			}
			else if (self->client->ps.ManualBlockingFlags & 1 << HOLDINGBLOCK
				&& self->client->NPC_class != CLASS_BOBAFETT
				&& self->client->NPC_class != CLASS_MANDO
				&& self->client->NPC_class != CLASS_ROCKETTROOPER)
			{
				if (!ent->owner || !OnSameTeam(self, ent->owner))
				{
					if (!self->s.number && self->client->ps.forcePowerLevel[FP_PUSH] == 1 && dist >= 192)
					{
						//player with push 1 has to wait until it's closer otherwise the push misses
					}
					else
					{
						ForceThrow(self, qfalse);
						PM_AddFatigue(&self->client->ps, FORCE_DEFLECT_PUSH);
					}
				}
			}
			//otherwise, can't block it, so we're screwed
			continue;
		}

		if (!do_full_routine)
		{
			//don't care about the rest then
			continue;
		}

		if (ent->s.weapon != WP_SABER)
		{
			//only block shots coming from behind
			if ((dot1 = DotProduct(dir, forward)) < SABER_REFLECT_MISSILE_CONE)
				continue;
		}

		//see if they're heading towards me
		if (!swing_block)
		{
			vec3_t missile_dir;
			// ReSharper disable once CppEntityAssignedButNoRead
			float dot2;
			VectorCopy(ent->s.pos.trDelta, missile_dir);
			VectorNormalize(missile_dir);
			if ((dot2 = DotProduct(dir, missile_dir)) > 0)
				continue;
		}

		//FIXME: must have a clear trace to me, too...
		if (dist < closest_dist)
		{
			VectorCopy(self->currentOrigin, trace_to);
			trace_to[2] = self->absmax[2] - 4;
			gi.trace(&trace, ent->currentOrigin, ent->mins, ent->maxs, trace_to, ent->s.number, ent->clipmask,
				static_cast<EG2_Collision>(0), 0);
			if (trace.allsolid || trace.startsolid || trace.fraction < 1.0f && trace.entityNum != self->s.number &&
				trace.entityNum != self->client->ps.saberEntityNum)
			{
				//okay, try one more check
				VectorNormalize2(ent->s.pos.trDelta, ent_dir);
				VectorMA(ent->currentOrigin, radius, ent_dir, trace_to);
				gi.trace(&trace, ent->currentOrigin, ent->mins, ent->maxs, trace_to, ent->s.number, ent->clipmask,
					static_cast<EG2_Collision>(0), 0);
				if (trace.allsolid || trace.startsolid || trace.fraction < 1.0f && trace.entityNum != self->s.number &&
					trace.entityNum != self->client->ps.saberEntityNum)
				{
					//can't hit me, ignore it
					continue;
				}
			}
			if (self->s.number != 0)
			{
				//An NPC
				if (self->NPC && !self->enemy && ent->owner)
				{
					if (ent->owner->health >= 0 && (!ent->owner->client || ent->owner->client->playerTeam != self->
						client->playerTeam))
					{
						G_SetEnemy(self, ent->owner);
					}
				}
			}
			closest_dist = dist;
			incoming = ent;
			closest_swing_block = swing_block;
			closest_swing_quad = swing_block_quad;
		}
	}

	if (!do_full_routine)
	{
		//then we're done now
		return;
	}

	if (incoming)
	{
		if (self->s.number >= MAX_CLIENTS && !G_ControlledByPlayer(self))
		{
			if (Jedi_WaitingAmbush(self))
			{
				Jedi_Ambush(self);
			}
			if ((self->client->NPC_class == CLASS_BOBAFETT ||
				self->client->NPC_class == CLASS_MANDO || self->client->NPC_class == CLASS_ROCKETTROOPER)
				&& self->client->moveType == MT_FLYSWIM
				&& incoming->methodOfDeath != MOD_ROCKET_ALT)
			{
				//a hovering Boba Fett, not a tracking rocket
				if (!Q_irand(0, 1))
				{
					//strafe
					self->NPC->standTime = 0;
					self->client->ps.forcePowerDebounce[FP_SABER_DEFENSE] = level.time + Q_irand(1000, 2000);
				}
				if (!Q_irand(0, 1))
				{
					//go up/down
					TIMER_Set(self, "heightChange", Q_irand(1000, 3000));
					self->client->ps.forcePowerDebounce[FP_SABER_DEFENSE] = level.time + Q_irand(1000, 2000);
				}
			}
			else if (jedi_saber_block_go(self, &self->NPC->last_ucmd, nullptr, nullptr, incoming) != EVASION_NONE
				&& self->client->NPC_class != CLASS_ROCKETTROOPER
				&& self->client->NPC_class != CLASS_BOBAFETT
				&& self->client->NPC_class != CLASS_MANDO)
			{
				//make sure to turn on your saber if it's not on
				if (self->client->NPC_class != CLASS_REBORN || self->s.weapon == WP_SABER)
				{
					self->client->ps.SaberActivate();
				}
			}
		}
		else //player
		{
			gentity_t* blocker = &g_entities[incoming->ownerNum];

			if (!self->client->ps.SaberActive())
			{
				self->client->ps.SaberActivate();
			}
			if (closest_swing_block && blocker->health > 0)
			{
				blocker->client->ps.saberBlocked = blockedfor_quad(closest_swing_quad);
				blocker->client->ps.userInt3 |= 1 << FLAG_PREBLOCK;
			}
			else if (blocker->client->ps.ManualBlockingFlags & 1 << HOLDINGBLOCK)
			{
				wp_saber_block_non_random_missile(blocker, incoming->currentOrigin, qtrue);
			}
			else
			{
				vec3_t diff, start, end;
				VectorSubtract(incoming->currentOrigin, self->currentOrigin, diff);
				const float scale = VectorLength(diff);
				VectorNormalize2(incoming->s.pos.trDelta, ent_dir);
				VectorMA(incoming->currentOrigin, scale, ent_dir, start);
				VectorCopy(self->currentOrigin, end);
				end[2] += self->maxs[2] * 0.75f;
				gi.trace(&trace, start, incoming->mins, incoming->maxs, end, incoming->s.number, MASK_SHOT, G2_COLLIDE,
					10);

				jedi_dodge_evasion(self, incoming->owner, &trace, HL_NONE);
			}
			if (incoming->owner && incoming->owner->client && (!self->enemy || self->enemy->s.weapon != WP_SABER))
			{
				self->enemy = incoming->owner;
				NPC_SetLookTarget(self, incoming->owner->s.number, level.time + 1000);
			}
		}
	}
}

//GENERAL SABER============================================================================
//GENERAL SABER============================================================================
//GENERAL SABER============================================================================
//GENERAL SABER============================================================================
//GENERAL SABER============================================================================

void WP_SetSaberMove(const gentity_t* self, const short blocked)
{
	self->client->ps.saberBlocked = blocked;
}

void wp_saber_update(gentity_t* self, const usercmd_t* ucmd)
{
	float minsize = 16;

	if (!self->client)
	{
		return;
	}

	if (self->client->ps.saberEntityNum < 0 || self->client->ps.saberEntityNum >= ENTITYNUM_WORLD)
	{
		//never got one
		return;
	}

	// Check if we are throwing it, launch it if needed, update position if needed.
	WP_SaberThrow(self, ucmd);

	if (self->client->ps.saberEntityNum <= 0)
	{
		//WTF?!!  We lost it?
		return;
	}

	gentity_t* saberent = &g_entities[self->client->ps.saberEntityNum];

	if (self->client->ps.saberBlocked != BLOCKED_NONE)
	{
		//we're blocking, increase min size
		minsize = 32;
	}

	if (g_in_cinematic_saber_anim(self))
	{
		//fake some blocking
		self->client->ps.saberBlocking = BLK_TIGHT;
		if (self->client->ps.saber[0].Active())
		{
			self->client->ps.saber[0].ActivateTrail(150);
		}
		if (self->client->ps.saber[1].Active())
		{
			self->client->ps.saber[1].ActivateTrail(150);
		}
	}

	//is our saber in flight?
	if (!self->client->ps.saberInFlight)
	{
		// It isn't, which means we can update its position as we will.
		qboolean always_block[MAX_SABERS][MAX_BLADES];
		qboolean force_block = qfalse;
		qboolean no_blocking = qfalse;

		//clear out last frame's numbers
		VectorClear(saberent->mins);
		VectorClear(saberent->maxs);

		const Vehicle_t* p_veh = G_IsRidingVehicle(self);
		if (!self->client->ps.SaberActive()
			|| !self->client->ps.saberBlocking
			|| (self->s.number < MAX_CLIENTS || G_ControlledByPlayer(self)) && !manual_saberblocking(self)
			|| PM_InKnockDown(&self->client->ps)
			|| PM_SuperBreakLoseAnim(self->client->ps.torsoAnim)
			|| p_veh && p_veh->m_pVehicleInfo && p_veh->m_pVehicleInfo->type != VH_ANIMAL && p_veh->m_pVehicleInfo->type
			!= VH_FLIER) //riding a vehicle that you cannot block shots on
		{
			//can't block if saber isn't on
			int j;
			for (int i = 0; i < MAX_SABERS; i++)
			{
				//initialize to not blocking
				for (j = 0; j < MAX_BLADES; j++)
				{
					always_block[i][j] = qfalse;
				}
				if (i > 0 && !self->client->ps.dualSabers)
				{
					//not using a second saber, leave it not blocking
				}
				else
				{
					if (self->client->ps.saber[i].saberFlags2 & SFL2_ALWAYS_BLOCK)
					{
						for (j = 0; j < self->client->ps.saber[i].numBlades; j++)
						{
							always_block[i][j] = qtrue;
							force_block = qtrue;
						}
					}
					if (self->client->ps.saber[i].bladeStyle2Start > 0)
					{
						for (j = self->client->ps.saber[i].bladeStyle2Start; j < self->client->ps.saber[i].numBlades; j
							++)
						{
							if (self->client->ps.saber[i].saberFlags2 & SFL2_ALWAYS_BLOCK2)
							{
								always_block[i][j] = qtrue;
								force_block = qtrue;
							}
							else
							{
								always_block[i][j] = qfalse;
							}
						}
					}
				}
			}
			if (!force_block)
			{
				no_blocking = qtrue;
			}
			else if (!self->client->ps.saberBlocking)
			{
				//turn blocking on!
				self->client->ps.saberBlocking = BLK_TIGHT;
			}
		}
		if (no_blocking)
		{
			G_SetOrigin(saberent, self->currentOrigin);
		}
		else if (self->client->ps.saberBlocking == BLK_TIGHT || self->client->ps.saberBlocking == BLK_WIDE)
		{
			//keep bbox in front of player, even when wide?
			vec3_t saber_org;

			if (!force_block
				&& (self->s.number && !Jedi_SaberBusy(self) && !g_saberRealisticCombat->integer
					|| self->s.number == 0 && self->client->ps.saberBlocking == BLK_WIDE
					&& (self->client->ps.ManualBlockingFlags & 1 << HOLDINGBLOCK
						|| self->client->ps.ManualBlockingFlags & 1 << MBF_NPCBLOCKING && self->NPC && !G_ControlledByPlayer(self)
						|| g_saberAutoBlocking->integer && self->NPC && !G_ControlledByPlayer(self)
						|| self->client->ps.saberBlockingTime > level.time))
				&& self->client->ps.weaponTime <= 0
				&& !g_in_cinematic_saber_anim(self))
			{
				//full-size blocking for non-attacking player with ManualBlockingFlags & (1 << HOLDINGBLOCK) on
				vec3_t saberang = { 0, 0, 0 }, fwd;
				constexpr vec3_t sabermaxs = { 8, 8, 8 };
				constexpr vec3_t sabermins = { -8, -8, -8 };

				saberang[YAW] = self->client->ps.viewangles[YAW];
				AngleVectors(saberang, fwd, nullptr, nullptr);

				VectorMA(self->currentOrigin, 12, fwd, saber_org);

				VectorAdd(self->mins, sabermins, saberent->mins);
				VectorAdd(self->maxs, sabermaxs, saberent->maxs);

				saberent->contents = CONTENTS_LIGHTSABER;

				G_SetOrigin(saberent, saber_org);
			}
			else
			{
				int num_sabers = 1;
				if (self->client->ps.dualSabers)
				{
					num_sabers = 2;
				}
				for (int saber_num = 0; saber_num < num_sabers; saber_num++)
				{
					for (int blade_num = 0; blade_num < self->client->ps.saber[saber_num].numBlades; blade_num++)
					{
						vec3_t saber_tip;
						vec3_t saber_base;
						if (self->client->ps.saber[saber_num].blade[blade_num].length <= 0.0f)
						{
							//don't include blades that are not on...
							continue;
						}
						if (force_block)
						{
							//doing blade-specific bbox-sizing only, see if this blade should be counted
							if (!always_block[saber_num][blade_num])
							{
								//this blade doesn't count right now
								continue;
							}
						}
						VectorCopy(self->client->ps.saber[saber_num].blade[blade_num].muzzlePoint, saber_base);
						VectorMA(saber_base, self->client->ps.saber[saber_num].blade[blade_num].length,
							self->client->ps.saber[saber_num].blade[blade_num].muzzleDir, saber_tip);
						VectorMA(saber_base, self->client->ps.saber[saber_num].blade[blade_num].length * 0.5,
							self->client->ps.saber[saber_num].blade[blade_num].muzzleDir, saber_org);
						for (int i = 0; i < 3; i++)
						{
							float new_size_tip = saber_tip[i] - saber_org[i];
							new_size_tip += new_size_tip >= 0 ? 8 : -8;
							float new_size_base = saber_base[i] - saber_org[i];
							new_size_base += new_size_base >= 0 ? 8 : -8;
							if (new_size_tip > saberent->maxs[i])
							{
								saberent->maxs[i] = new_size_tip;
							}
							if (new_size_base > saberent->maxs[i])
							{
								saberent->maxs[i] = new_size_base;
							}
							if (new_size_tip < saberent->mins[i])
							{
								saberent->mins[i] = new_size_tip;
							}
							if (new_size_base < saberent->mins[i])
							{
								saberent->mins[i] = new_size_base;
							}
						}
					}
				}
				if (!force_block)
				{
					//not doing special "alwaysBlock" bbox
					if (self->client->ps.weaponTime > 0
						|| self->s.number
						|| g_saberAutoBlocking->integer && self->NPC && !G_ControlledByPlayer(self)
						|| self->client->ps.ManualBlockingFlags & 1 << MBF_NPCBLOCKING && self->NPC && !G_ControlledByPlayer(self)
						|| self->client->ps.ManualBlockingFlags & 1 << HOLDINGBLOCK
						|| self->client->ps.saberBlockingTime > level.time)
					{
						//if attacking or blocking (or an NPC), inflate to a minimum size
						for (int i = 0; i < 3; i++)
						{
							if (saberent->maxs[i] < minsize)
							{
								saberent->maxs[i] = minsize;
							}
							if (saberent->mins[i] > -minsize)
							{
								saberent->mins[i] = -minsize;
							}
						}
					}
				}
				saberent->contents = CONTENTS_LIGHTSABER;
				G_SetOrigin(saberent, saber_org);
			}
		}
		else
		{
			// Otherwise there is no blocking possible.
			G_SetOrigin(saberent, self->currentOrigin);
		}
		saberent->clipmask = MASK_SHOT | CONTENTS_LIGHTSABER;
		gi.linkentity(saberent);
	}
	else
	{
		wp_saber_in_flight_reflect_check(self);
	}

	if ((d_saberInfo->integer || g_DebugSaberCombat->integer) && !PM_SaberInAttack(self->client->ps.saberMove))
	{
		if (self->NPC && !G_ControlledByPlayer(self)) //NPC only
		{
			CG_CubeOutline(saberent->absmin, saberent->absmax, 40,
				wp_debug_saber_colour(self->client->ps.saber[0].blade[0].color));
		}
		else
		{
			CG_CubeOutline(saberent->absmin, saberent->absmax, 10,
				wp_debug_saber_colour(self->client->ps.saber[0].blade[0].color));
		}
	}
}

constexpr auto MAX_RADIUS_ENTS = 256; //NOTE: This can cause entities to be lost;
qboolean G_CheckEnemyPresence(const gentity_t* ent, int dir, float radius, float tolerance)
{
	gentity_t* radiusEnts[MAX_RADIUS_ENTS];
	vec3_t mins, maxs;
	vec3_t checkDir;
	int i;

	switch (dir)
	{
	case DIR_RIGHT:
		AngleVectors(ent->currentAngles, nullptr, checkDir, nullptr);
		break;
	case DIR_LEFT:
		AngleVectors(ent->currentAngles, nullptr, checkDir, nullptr);
		VectorScale(checkDir, -1, checkDir);
		break;
	case DIR_FRONT:
		AngleVectors(ent->currentAngles, checkDir, nullptr, nullptr);
		break;
	case DIR_BACK:
		AngleVectors(ent->currentAngles, checkDir, nullptr, nullptr);
		VectorScale(checkDir, -1, checkDir);
		break;
	default:;
	}
	//Get all ents in range, see if they're living clients and enemies, then check dot to them...

	//Setup the bbox to search in
	for (i = 0; i < 3; i++)
	{
		mins[i] = ent->currentOrigin[i] - radius;
		maxs[i] = ent->currentOrigin[i] + radius;
	}

	//Get a number of entities in a given space
	const int numEnts = gi.EntitiesInBox(mins, maxs, radiusEnts, MAX_RADIUS_ENTS);

	for (i = 0; i < numEnts; i++)
	{
		vec3_t dir2checkEnt;
		//Don't consider self
		if (radiusEnts[i] == ent)
			continue;

		//Must be valid
		if (G_ValidEnemy(ent, radiusEnts[i]) == qfalse)
			continue;

		VectorSubtract(radiusEnts[i]->currentOrigin, ent->currentOrigin, dir2checkEnt);
		const float dist = VectorNormalize(dir2checkEnt);
		if (dist <= radius
			&& DotProduct(dir2checkEnt, checkDir) >= tolerance)
		{
			//stop on the first one
			return qtrue;
		}
	}

	return qfalse;
}

//OTHER JEDI POWERS=========================================================================
//OTHER JEDI POWERS=========================================================================
//OTHER JEDI POWERS=========================================================================
//OTHER JEDI POWERS=========================================================================
//OTHER JEDI POWERS=========================================================================
extern gentity_t* TossClientItems(gentity_t* self);
extern void ChangeWeapon(const gentity_t* ent, int newWeapon);

void WP_DropWeapon(gentity_t* dropper, vec3_t velocity)
{
	if (!dropper || !dropper->client)
	{
		return;
	}
	int replaceWeap = WP_NONE;
	const int oldWeap = dropper->s.weapon;
	gentity_t* weapon = TossClientItems(dropper);
	if (oldWeap == WP_THERMAL && dropper->NPC)
	{
		//Hmm, maybe all NPCs should go into melee?  Not too many, though, or they mob you and look silly
		replaceWeap = WP_MELEE;
	}
	if (dropper->ghoul2.IsValid())
	{
		if (dropper->weaponModel[0] > 0)
		{
			//NOTE: guess you never drop the left-hand weapon, eh?
			gi.G2API_RemoveGhoul2Model(dropper->ghoul2, dropper->weaponModel[0]);
			dropper->weaponModel[0] = -1;
		}
	}
	//FIXME: does this work on the player?
	dropper->client->ps.stats[STAT_WEAPONS] |= 1 << replaceWeap;
	if (!dropper->s.number)
	{
		if (oldWeap == WP_THERMAL)
		{
			dropper->client->ps.ammo[weaponData[oldWeap].ammoIndex] -= weaponData[oldWeap].energyPerShot;
		}
		else
		{
			dropper->client->ps.stats[STAT_WEAPONS] &= ~(1 << oldWeap);
		}
		CG_ChangeWeapon(replaceWeap);
	}
	else
	{
		dropper->client->ps.stats[STAT_WEAPONS] &= ~(1 << oldWeap);
	}
	ChangeWeapon(dropper, replaceWeap);
	dropper->s.weapon = replaceWeap;
	if (dropper->NPC)
	{
		dropper->NPC->last_ucmd.weapon = replaceWeap;
	}
	if (weapon != nullptr && velocity && !VectorCompare(velocity, vec3_origin))
	{
		//weapon should have a direction to it's throw
		VectorScale(velocity, 3, weapon->s.pos.trDelta); //NOTE: Presumes it is moving already...?
		if (weapon->s.pos.trDelta[2] < 150)
		{
			//this is presuming you don't want them to drop the weapon down on you...
			weapon->s.pos.trDelta[2] = 150;
		}
		//FIXME: gets stuck inside it's former owner...
		weapon->forcePushTime = level.time + 600; // let the push effect last for 600 ms
	}
}

void WP_KnockdownTurret(gentity_t* self, gentity_t* pas)
{
	//knock it over
	VectorCopy(pas->currentOrigin, pas->s.pos.trBase);
	pas->s.pos.trType = TR_LINEAR_STOP;
	pas->s.pos.trDuration = 250;
	pas->s.pos.trTime = level.time;
	pas->s.pos.trDelta[2] = 12.0f / (pas->s.pos.trDuration * 0.001f);

	VectorCopy(pas->currentAngles, pas->s.apos.trBase);
	pas->s.apos.trType = TR_LINEAR_STOP;
	pas->s.apos.trDuration = 250;
	pas->s.apos.trTime = level.time;
	//FIXME: pick pitch/roll that always tilts it directly away from pusher
	pas->s.apos.trDelta[PITCH] = 100.0f / (pas->s.apos.trDuration * 0.001f);

	//kill it
	pas->count = 0;
	pas->nextthink = -1;
	G_Sound(pas, G_SoundIndex("sound/chars/turret/shutdown.wav"));
	//push effect?
	pas->forcePushTime = level.time + 600; // let the push effect last for 600 ms
}

void WP_ForceThrowHazardTrooper(gentity_t* self, gentity_t* trooper, qboolean pull)
{
	if (!self || !self->client)
	{
		return;
	}
	if (!trooper || !trooper->client)
	{
		return;
	}

	//all levels: see effect on them, they notice us
	trooper->forcePushTime = level.time + 600; // let the push effect last for 600 ms

	if (pull && self->client->ps.forcePowerLevel[FP_PULL] > FORCE_LEVEL_1
		|| !pull && self->client->ps.forcePowerLevel[FP_PUSH] > FORCE_LEVEL_1)
	{
		//level 2: they stop for a couple seconds and make a sound
		trooper->painDebounceTime = level.time + Q_irand(1500, 2500);
		G_AddVoiceEvent(trooper, Q_irand(EV_PUSHED1, EV_PUSHED3), Q_irand(1000, 3000));
		GEntity_PainFunc(trooper, self, self, trooper->currentOrigin, 0, MOD_MELEE);

		if (pull && self->client->ps.forcePowerLevel[FP_PULL] > FORCE_LEVEL_2
			|| !pull && self->client->ps.forcePowerLevel[FP_PUSH] > FORCE_LEVEL_2)
		{
			//level 3: they actually play a pushed anim and stumble a bit
			vec3_t hazAngles = { 0, trooper->currentAngles[YAW], 0 };
			int anim;
			if (in_front(self->currentOrigin, trooper->currentOrigin, hazAngles))
			{
				//I'm on front of him
				if (pull)
				{
					anim = BOTH_PAIN4;
				}
				else
				{
					anim = BOTH_PAIN1;
				}
			}
			else
			{
				//I'm behind him
				if (pull)
				{
					anim = BOTH_PAIN1;
				}
				else
				{
					anim = BOTH_PAIN4;
				}
			}
			if (anim != -1)
			{
				if (anim == BOTH_PAIN1)
				{
					//make them take a couple steps back
					AngleVectors(hazAngles, trooper->client->ps.velocity, nullptr, nullptr);
					VectorScale(trooper->client->ps.velocity, -40.0f, trooper->client->ps.velocity);
					trooper->client->ps.pm_flags |= PMF_TIME_NOFRICTION;
				}
				else if (anim == BOTH_PAIN4)
				{
					//make them stumble forward
					AngleVectors(hazAngles, trooper->client->ps.velocity, nullptr, nullptr);
					VectorScale(trooper->client->ps.velocity, 80.0f, trooper->client->ps.velocity);
					trooper->client->ps.pm_flags |= PMF_TIME_NOFRICTION;
				}
				NPC_SetAnim(trooper, SETANIM_BOTH, anim, SETANIM_FLAG_OVERRIDE | SETANIM_FLAG_HOLD);
				trooper->painDebounceTime += trooper->client->ps.torsoAnimTimer;
				trooper->client->ps.pm_time = trooper->client->ps.torsoAnimTimer;
			}
		}
		if (trooper->NPC)
		{
			if (trooper->NPC->shotTime < trooper->painDebounceTime)
			{
				trooper->NPC->shotTime = trooper->painDebounceTime;
			}
		}
		trooper->client->ps.weaponTime = trooper->painDebounceTime - level.time;
	}
	else
	{
		//level 1: no pain reaction, but they should still notice
		if (trooper->enemy == nullptr //not mad at anyone
			&& trooper->client->playerTeam != self->client->playerTeam //not on our team
			&& !(trooper->svFlags & SVF_LOCKEDENEMY) //not locked on an enemy
			&& !(trooper->svFlags & SVF_IGNORE_ENEMIES) //not ignoring enemies
			&& !(self->flags & FL_NOTARGET)) //I'm not in notarget
		{
			//not already mad at them and can get mad at them, do so
			G_SetEnemy(trooper, self);
		}
	}
}

void WP_ResistForcePush(gentity_t* self, const gentity_t* pusher, qboolean noPenalty)
{
	int parts;
	qboolean runningResist = qfalse;

	if (!self || self->health <= 0 || !self->client || !pusher || !pusher->client)
	{
		return;
	}

	if (PM_InKnockDown(&self->client->ps))
	{
		return;
	}

	if (PM_KnockDownAnim(self->client->ps.legsAnim) || PM_KnockDownAnim(self->client->ps.torsoAnim) || PM_StaggerAnim(
		self->client->ps.torsoAnim))
	{
		return;
	}

	if (PM_SaberInKata(static_cast<saberMoveName_t>(self->client->ps.saberMove)) || PM_InKataAnim(
		self->client->ps.torsoAnim))
	{
		//don't throw saber when in special attack (alt+attack)
		return;
	}

	if (!PM_SaberCanInterruptMove(self->client->ps.saberMove, self->client->ps.torsoAnim))
	{
		//can't interrupt my current torso anim/sabermove with this, so ignore it entirely!
		return;
	}

	if ((!self->s.number
		|| self->NPC && self->NPC->aiFlags & NPCAI_BOSS_CHARACTER
		|| self->client && self->client->NPC_class == CLASS_SHADOWTROOPER)
		&& (VectorLengthSquared(self->client->ps.velocity) > 10000 || self->client->ps.forcePowerLevel[FP_PUSH] >=
			FORCE_LEVEL_3 || self->client->ps.forcePowerLevel[FP_PULL] >= FORCE_LEVEL_3))
	{
		runningResist = qtrue;
	}

	if (!runningResist
		&& self->client->ps.groundEntityNum != ENTITYNUM_NONE
		&& !PM_SpinningSaberAnim(self->client->ps.legsAnim)
		&& !PM_FlippingAnim(self->client->ps.legsAnim)
		&& !PM_RollingAnim(self->client->ps.legsAnim)
		&& !PM_InKnockDown(&self->client->ps)
		&& !PM_CrouchAnim(self->client->ps.legsAnim))
	{
		//if on a surface and not in a spin or flip, play full body resist
		parts = SETANIM_BOTH;
	}
	else
	{
		//play resist just in torso
		parts = SETANIM_TORSO;
	}

	if (self->client->ps.forcePowerLevel[FP_ABSORB] > FORCE_LEVEL_2)
	{
		NPC_SetAnim(self, parts, BOTH_RESISTPUSH, SETANIM_FLAG_OVERRIDE | SETANIM_FLAG_HOLD);
	}
	else if (self->client->ps.saber[0].type == SABER_YODA)
	{
		NPC_SetAnim(self, parts, BOTH_YODA_RESISTFORCE, SETANIM_FLAG_OVERRIDE | SETANIM_FLAG_HOLD);
	}
	else
	{
		NPC_SetAnim(self, parts, BOTH_RESISTPUSH2, SETANIM_FLAG_OVERRIDE | SETANIM_FLAG_HOLD);
	}
	self->client->ps.powerups[PW_MEDITATE] = level.time + self->client->ps.torsoAnimTimer + 1000;

	if (!noPenalty)
	{
		if (!runningResist)
		{
			VectorClear(self->client->ps.velocity);
			self->client->ps.weaponTime = 1000;
			if (self->client->ps.forcePowersActive & 1 << FP_SPEED)
			{
				self->client->ps.weaponTime = floor(self->client->ps.weaponTime * g_timescale->value);
			}
			self->client->ps.pm_time = self->client->ps.weaponTime;
			self->client->ps.pm_flags |= PMF_TIME_KNOCKBACK;
			//play the full body push effect on me
			self->forcePushTime = level.time + 600; // let the push effect last for 600 ms
		}
		else
		{
			self->client->ps.weaponTime = 600;
			if (self->client->ps.forcePowersActive & 1 << FP_SPEED)
			{
				self->client->ps.weaponTime = floor(self->client->ps.weaponTime * g_timescale->value);
			}
		}
	}

	if (!pusher //???
		|| pusher == self->enemy //my enemy tried to push me
		|| pusher->client && pusher->client->playerTeam != self->client->playerTeam)
		//someone not on my team tried to push me
	{
		Jedi_PlayBlockedPushSound(self);
	}
}

extern qboolean Boba_StopKnockdown(gentity_t* self, const gentity_t* pusher, const vec3_t pushDir, qboolean forceKnockdown);
extern qboolean Jedi_StopKnockdown(gentity_t* self, const vec3_t push_dir);

void WP_ForceKnockdown(gentity_t* self, gentity_t* pusher, qboolean pull, qboolean strongKnockdown,
	qboolean breakSaberLock)
{
	if (!self || !self->client || !pusher || !pusher->client)
	{
		return;
	}

	if (self->client->NPC_class == CLASS_ROCKETTROOPER)
	{
		return;
	}
	if (PM_LockedAnim(self->client->ps.legsAnim))
	{
		//stuck doing something else
		return;
	}
	if (Rosh_BeingHealed(self))
	{
		return;
	}

	//break out of a saberLock?
	if (self->client->ps.saberLockTime > level.time)
	{
		if (breakSaberLock
			|| pusher && self->client->ps.saberLockEnemy == pusher->s.number)
		{
			self->client->ps.saberLockTime = 0;
			self->client->ps.saberLockEnemy = ENTITYNUM_NONE;
		}
		else
		{
			return;
		}
	}

	if (self->health > 0)
	{
		if (!self->s.number)
		{
			NPC_SetPainEvent(self);
		}
		else
		{
			GEntity_PainFunc(self, pusher, pusher, self->currentOrigin, 0, MOD_MELEE);
		}

		vec3_t pushDir;
		if (pull)
		{
			VectorSubtract(pusher->currentOrigin, self->currentOrigin, pushDir);
		}
		else
		{
			VectorSubtract(self->currentOrigin, pusher->currentOrigin, pushDir);
		}

		if (Boba_StopKnockdown(self, pusher, pushDir, qtrue))
		{
			//He can backflip instead of be knocked down
			return;
		}
		if (Jedi_StopKnockdown(self, pushDir))
		{
			//They can backflip instead of be knocked down
			return;
		}

		G_CheckLedgeDive(self, 72, pushDir, qfalse, qfalse);

		if (!PM_RollingAnim(self->client->ps.legsAnim)
			&& !PM_InKnockDown(&self->client->ps))
		{
			int knock_anim; //default knockdown
			if (pusher->client->NPC_class == CLASS_DESANN || pusher->client->NPC_class == CLASS_SITHLORD || pusher->
				client->NPC_class == CLASS_VADER && self->client->NPC_class != CLASS_LUKE)
			{
				//desann always knocks down, unless you're Luke
				strongKnockdown = qtrue;
			}
			if (!self->s.number
				&& !strongKnockdown
				&& (!pull && (self->client->ps.forcePowerLevel[FP_PUSH] > FORCE_LEVEL_1 || !g_spskill->integer) || pull
					&& (self->client->ps.forcePowerLevel[FP_PULL] > FORCE_LEVEL_1 || !g_spskill->integer)))
			{
				//player only knocked down if pushed *hard*
				if (self->s.weapon == WP_SABER)
				{
					//temp HACK: these are the only 2 pain anims that look good when holding a saber
					knock_anim = PM_PickAnim(self, BOTH_PAIN2, BOTH_PAIN3);
				}
				else
				{
					knock_anim = PM_PickAnim(self, BOTH_PAIN1, BOTH_PAIN18);
				}
			}
			else if (PM_CrouchAnim(self->client->ps.legsAnim))
			{
				//crouched knockdown
				knock_anim = BOTH_KNOCKDOWN4;
			}
			else
			{
				//plain old knockdown
				vec3_t pLFwd;
				const vec3_t pLAngles = { 0, self->client->ps.viewangles[YAW], 0 };
				vec3_t sFwd;
				const vec3_t sAngles = { 0, pusher->client->ps.viewangles[YAW], 0 };
				AngleVectors(pLAngles, pLFwd, nullptr, nullptr);
				AngleVectors(sAngles, sFwd, nullptr, nullptr);
				if (DotProduct(sFwd, pLFwd) > 0.2f)
				{
					//pushing him from behind
					//FIXME: check to see if we're aiming below or above the waist?
					if (pull)
					{
						knock_anim = BOTH_KNOCKDOWN1;
					}
					else
					{
						knock_anim = BOTH_KNOCKDOWN3;
					}
				}
				else
				{
					//pushing him from front
					if (pull)
					{
						knock_anim = BOTH_KNOCKDOWN3;
					}
					else
					{
						knock_anim = BOTH_KNOCKDOWN1;
					}
				}
			}
			if (knock_anim == BOTH_KNOCKDOWN1 && strongKnockdown)
			{
				//push *hard*
				knock_anim = BOTH_KNOCKDOWN2;
			}
			NPC_SetAnim(self, SETANIM_BOTH, knock_anim, SETANIM_FLAG_OVERRIDE | SETANIM_FLAG_HOLD);
			if (self->s.number >= MAX_CLIENTS)
			{
				//randomize getup times - but not for boba
				int addTime;
				if (self->client->NPC_class == CLASS_BOBAFETT)
				{
					addTime = Q_irand(-500, 0);
				}
				else
				{
					addTime = Q_irand(-300, 300);
				}
				self->client->ps.legsAnimTimer += addTime;
				self->client->ps.torsoAnimTimer += addTime;
			}
			else
			{
				//player holds extra long so you have more time to decide to do the quick getup
				if (PM_KnockDownAnim(self->client->ps.legsAnim))
				{
					self->client->ps.legsAnimTimer += PLAYER_KNOCKDOWN_HOLD_EXTRA_TIME;
					self->client->ps.torsoAnimTimer += PLAYER_KNOCKDOWN_HOLD_EXTRA_TIME;
				}
			}
			//
			if (pusher->NPC && pusher->enemy == self)
			{
				//pushed pushed down his enemy
				G_AddVoiceEvent(pusher, Q_irand(EV_GLOAT1, EV_GLOAT3), 3000);
				pusher->NPC->blockedSpeechDebounceTime = level.time + 3000;
			}
		}
	}
	self->forcePushTime = level.time + 600; // let the push effect last for 600 ms
}

qboolean WP_ForceThrowable(gentity_t* ent, const gentity_t* forwardEnt, const gentity_t* self, qboolean pull, float cone,
	float radius, vec3_t forward)
{
	if (ent == self)
		return qfalse;
	if (ent->owner == self && ent->s.weapon != WP_THERMAL) //can push your own thermals
		return qfalse;
	if (!ent->inuse)
		return qfalse;
	if (ent->NPC && ent->NPC->scriptFlags & SCF_NO_FORCE)
	{
		if (!in_camera && ent->NPC && ent->NPC->scriptFlags & SCF_NO_FORCE)
		{
			if (PM_DeathCinAnim(ent->client->ps.legsAnim))
			{
			}
			else
			{
				if (ent->s.weapon == WP_SABER)
				{
					//Hmm, should jedi do the resist behavior?  If this is on, perhaps it's because of a cinematic?
					WP_ResistForcePush(ent, self, qtrue);
				}
				else
				{
					NPC_SetAnim(ent, SETANIM_TORSO, BOTH_WIND, SETANIM_AFLAG_PACE);
				}
				ent->client->ps.powerups[PW_MEDITATE] = level.time + ent->client->ps.torsoAnimTimer + 5000;
			}
			return qfalse;
		}
	}
	if (ent->flags & FL_FORCE_PULLABLE_ONLY && !pull)
	{
		//simple HACK: cannot force-push ammo rack items (because they may start in solid)
		return qfalse;
	}
	//FIXME: don't push it if I already pushed it a little while ago
	if (ent->s.eType != ET_MISSILE)
	{
		if (ent->client)
		{
			if (ent->client->ps.pullAttackTime > level.time)
			{
				return qfalse;
			}
		}
		if (cone >= 1.0f)
		{
			//must be pointing right at them
			if (ent != forwardEnt)
			{
				//must be the person I'm looking right at
				if (ent->client && !pull
					&& ent->client->ps.forceGripEntityNum == self->s.number
					&& (self->s.eFlags & EF_FORCE_GRIPPED || self->s.eFlags & EF_FORCE_GRABBED))
				{
					//this is the guy that's force-gripping me, use a wider cone regardless of force power level
				}
				else
				{
					if (ent->client && !pull
						&& ent->client->ps.forceDrainEntityNum == self->s.number
						&& self->s.eFlags & EF_FORCE_DRAINED)
					{
						//this is the guy that's force-draining me, use a wider cone regardless of force power level
					}
					else
					{
						return qfalse;
					}
				}
			}
		}
		if (ent->s.eType != ET_ITEM && ent->e_ThinkFunc != thinkF_G_RunObject)
		{
			//FIXME: need pushable objects
			if (ent->s.eFlags & EF_NODRAW)
			{
				return qfalse;
			}
			if (!ent->client)
			{
				if (Q_stricmp("lightsaber", ent->classname) != 0)
				{
					//not a lightsaber
					if (!(ent->svFlags & SVF_GLASS_BRUSH))
					{
						//and not glass
						if (Q_stricmp("func_door", ent->classname) != 0 || !(ent->spawnflags & 2))
						{
							//not a force-usable door
							if (Q_stricmp("func_static", ent->classname) != 0 || !(ent->spawnflags & 1) && !(ent->
								spawnflags & 2) || ent->spawnflags & 32)
							{
								//not a force-usable func_static or, it is one, but it's solitary, so you only press it when looking right at it
								if (Q_stricmp("limb", ent->classname))
								{
									//not a limb
									if (ent->s.weapon == WP_TURRET && !Q_stricmp("PAS", ent->classname) && ent->s.apos.
										trType == TR_STATIONARY)
									{
										//can knock over placed turrets
										if (!self->s.number || self->enemy != ent)
										{
											//only NPCs who are actively mad at this turret can push it over
											return qfalse;
										}
									}
									else
									{
										return qfalse;
									}
								}
							}
						}
						else if (ent->moverState != MOVER_POS1 && ent->moverState != MOVER_POS2)
						{
							//not at rest
							return qfalse;
						}
					}
				}
				//return qfalse;
			}
			else if (ent->client->NPC_class == CLASS_MARK1)
			{
				//can't push Mark1 unless push 3
				if (pull || self->client->ps.forcePowerLevel[FP_PUSH] < FORCE_LEVEL_3)
				{
					return qfalse;
				}
			}
			else if (ent->client->NPC_class == CLASS_GALAKMECH
				|| ent->client->NPC_class == CLASS_ATST
				|| ent->client->NPC_class == CLASS_SBD
				|| ent->client->NPC_class == CLASS_RANCOR
				|| ent->client->NPC_class == CLASS_WAMPA
				|| ent->client->NPC_class == CLASS_SAND_CREATURE)
			{
				//can't push ATST or Galak or Rancor or Wampa
				return qfalse;
			}
			else if (ent->s.weapon == WP_EMPLACED_GUN)
			{
				//FIXME: maybe can pull them out?
				return qfalse;
			}
			else if (ent->client->playerTeam == self->client->playerTeam && self->enemy && self->enemy != ent)
			{
				//can't accidently push a teammate while in combat
				return qfalse;
			}
			else if (G_IsRidingVehicle(ent)
				&& ent->s.eFlags & EF_NODRAW)
			{
				//can't push/pull anyone riding *inside* vehicle
				return qfalse;
			}
		}
		else if (ent->s.eType == ET_ITEM)
		{
			if (ent->flags & FL_NO_KNOCKBACK)
			{
				return qfalse;
			}
			if (ent->item
				&& ent->item->giType == IT_HOLDABLE
				&& ent->item->giTag == INV_SECURITY_KEY)
			{
				//dropped security keys can't be pushed?  But placed ones can...?  does this make any sense?
				if (!pull && !g_pushitems->integer)
					return qfalse; // Can't push, cvar disabled
				if (pull && !g_pullitems->integer)
					return qfalse; // Can't pull, cvar disabled
				if (!pull || self->s.number)
				{
					//can't push, NPC's can't do anything to it
					return qfalse;
				}
				if (g_crosshairEntNum != ent->s.number)
				{
					//player can pull it if looking *right* at it
					if (cone >= 1.0f)
					{
						//we did a forwardEnt trace
						if (forwardEnt != ent)
						{
							//must be pointing right at them
							return qfalse;
						}
					}
					else if (forward)
					{
						//do a forwardEnt trace
						trace_t tr;
						vec3_t end;
						VectorMA(self->client->renderInfo.eyePoint, radius, forward, end);
						gi.trace(&tr, self->client->renderInfo.eyePoint, vec3_origin, vec3_origin, end, self->s.number,
							MASK_OPAQUE | CONTENTS_SOLID | CONTENTS_BODY | CONTENTS_ITEM | CONTENTS_CORPSE,
							static_cast<EG2_Collision>(0), 0); //was MASK_SHOT, changed to match crosshair trace
						if (tr.entityNum != ent->s.number)
						{
							//last chance
							return qfalse;
						}
					}
				}
			}
			else
			{
				if (!pull && !g_pushitems->integer)
					return qfalse; // Pushing items disabled by cvar.
				if (pull && !g_pullitems->integer)
					return qfalse; // Pulling items disabled by cvar.
			}
		}
	}
	else
	{
		switch (ent->s.weapon)
		{
			//only missiles with mass are force-pushable
		case WP_CONCUSSION:
		case WP_ROCKET_LAUNCHER:
			//Don't push Destruction projectiles
			if (ent->s.powerups & 1 << PW_FORCE_PROJECTILE)
			{
				return qfalse;
			}
			break;
		case WP_SABER:
		case WP_FLECHETTE:
		case WP_THERMAL:
		case WP_TRIP_MINE:
		case WP_DET_PACK:
			break;
			//only alt-fire of this weapon is force-pushable
		case WP_REPEATER:
			if (ent->methodOfDeath != MOD_REPEATER_ALT)
			{
				//not an alt-fire missile
				return qfalse;
			}
			break;
			//everything else cannot be pushed
		case WP_ATST_SIDE:
			if (ent->methodOfDeath != MOD_EXPLOSIVE)
			{
				//not a rocket
				return qfalse;
			}
			break;
		default:
			return qfalse;
		}

		if (ent->s.pos.trType == TR_STATIONARY && ent->s.eFlags & EF_MISSILE_STICK)
		{
			//can't force-push/pull stuck missiles (detpacks, tripmines)
			return qfalse;
		}
		if (ent->s.pos.trType == TR_STATIONARY && ent->s.weapon != WP_THERMAL)
		{
			//only thermal detonators can be pushed once stopped
			return qfalse;
		}
	}
	return qtrue;
}

static qboolean playeris_resisting_force_throw(const gentity_t* player, gentity_t* attacker, qboolean pull)
{
	if (player->health <= 0)
	{
		return qfalse;
	}

	if (!player->client)
	{
		return qfalse;
	}

	if (player->client->ps.forcePowerLevel[FP_SABER_DEFENSE] < FORCE_LEVEL_1)
	{
		return qfalse;
	}

	if (player->client->ps.forceRageRecoveryTime >= level.time)
	{
		return qfalse;
	}

	//wasn't trying to grip/drain anyone
	if (player->client->ps.torsoAnim == BOTH_FORCEGRIP_HOLD ||
		player->client->ps.torsoAnim == BOTH_FORCE_DRAIN_GRAB_START ||
		player->client->ps.torsoAnim == BOTH_FORCE_DRAIN_GRAB_HOLD)
	{
		return qfalse;
	}

	//on the ground
	if (player->client->ps.groundEntityNum == ENTITYNUM_NONE)
	{
		return qfalse;
	}

	//not knocked down already
	if (PM_InKnockDown(&player->client->ps))
	{
		return qfalse;
	}

	if (PM_KnockDownAnim(player->client->ps.legsAnim) || PM_KnockDownAnim(player->client->ps.torsoAnim) ||
		PM_StaggerAnim(player->client->ps.torsoAnim))
	{
		return qfalse;
	}

	if (PM_SaberInKata(static_cast<saberMoveName_t>(player->client->ps.saberMove)) || PM_InKataAnim(
		player->client->ps.torsoAnim))
	{
		//don't throw saber when in special attack (alt+attack)
		return qfalse;
	}

	//not involved in a saberLock
	if (player->client->ps.saberLockTime >= level.time)
	{
		return qfalse;
	}

	//not attacking or otherwise busy
	if (player->client->ps.weaponTime >= level.time)
	{
		return qfalse;
	}

	if (manual_forceblocking(player))
	{
		// player was pushing, or player's force push/pull is high enough to try to stop me
		if (in_front(attacker->currentOrigin, player->client->renderInfo.eyePoint, player->client->ps.viewangles, 0.3f))
		{
			//I'm in front of player
			return qtrue;
		}
	}

	return qfalse;
}

static qboolean ShouldPlayerResistForceThrow(const gentity_t* player, gentity_t* attacker, qboolean pull)
{
	if (player->health <= 0)
	{
		return qfalse;
	}

	if (!player->client)
	{
		return qfalse;
	}

	if (player->client->ps.forceRageRecoveryTime >= level.time)
	{
		return qfalse;
	}

	//wasn't trying to grip/drain anyone
	if (player->client->ps.torsoAnim == BOTH_FORCEGRIP_HOLD ||
		player->client->ps.torsoAnim == BOTH_FORCE_DRAIN_GRAB_START ||
		player->client->ps.torsoAnim == BOTH_FORCE_DRAIN_GRAB_HOLD)
	{
		return qfalse;
	}

	//only 30% chance of resisting a master
	if ((attacker->client->NPC_class == CLASS_DESANN
		|| attacker->client->NPC_class == CLASS_SITHLORD
		|| attacker->client->NPC_class == CLASS_VADER
		|| Q_stricmp("Yoda", attacker->NPC_type)
		|| Q_stricmp("T_Yoda", attacker->NPC_type)
		|| Q_stricmp("jedi_kdm1", attacker->NPC_type)
		|| Q_stricmp("RebornBoss", attacker->NPC_type)
		|| Q_stricmp("T_Palpatine_sith", attacker->NPC_type) == 0)
		&& Q_irand(0, 2) > 0)
	{
		return qfalse;
	}

	//on the ground
	if (player->client->ps.groundEntityNum == ENTITYNUM_NONE)
	{
		return qfalse;
	}

	//not knocked down already
	if (PM_InKnockDown(&player->client->ps))
	{
		return qfalse;
	}

	if (PM_KnockDownAnim(player->client->ps.legsAnim) || PM_KnockDownAnim(player->client->ps.torsoAnim) ||
		PM_StaggerAnim(player->client->ps.torsoAnim))
	{
		return qfalse;
	}

	if (PM_SaberInKata(static_cast<saberMoveName_t>(player->client->ps.saberMove)) || PM_InKataAnim(
		player->client->ps.torsoAnim))
	{
		//don't throw saber when in special attack (alt+attack)
		return qfalse;
	}

	//not involved in a saberLock
	if (player->client->ps.saberLockTime >= level.time)
	{
		return qfalse;
	}

	//not attacking or otherwise busy
	if (player->client->ps.weaponTime >= level.time)
	{
		return qfalse;
	}

	//using saber or fists
	if (player->client->ps.weapon != WP_SABER && player->client->ps.weapon != WP_MELEE)
	{
		return qfalse;
	}

	const forcePowers_t forcePower = pull ? FP_PULL : FP_PUSH;
	const int attackingForceLevel = attacker->client->ps.forcePowerLevel[forcePower];
	const int defendingForceLevel = player->client->ps.forcePowerLevel[forcePower];

	if (player->client->ps.powerups[PW_FORCE_PUSH] > level.time ||
		Q_irand(0, Q_max(0, defendingForceLevel - attackingForceLevel) * 2 + 1) > 0)
	{
		// player was pushing, or player's force push/pull is high enough to try to stop me
		if (in_front(attacker->currentOrigin, player->client->renderInfo.eyePoint, player->client->ps.viewangles, 0.3f))
		{
			//I'm in front of player
			return qtrue;
		}
	}

	return qfalse;
}

void RepulseDamage(gentity_t* self, gentity_t* enemy, vec3_t location, int damageLevel)
{
	switch (damageLevel)
	{
	case FORCE_LEVEL_1:
		G_Damage(enemy, self, self, nullptr, location, 20, DAMAGE_DEATH_KNOCKBACK | DAMAGE_EXTRA_KNOCKBACK, MOD_SNIPER);
		break;
	case FORCE_LEVEL_2:
		G_Damage(enemy, self, self, nullptr, location, 40, DAMAGE_DEATH_KNOCKBACK | DAMAGE_EXTRA_KNOCKBACK, MOD_SNIPER);
		break;
	case FORCE_LEVEL_3:
		G_Damage(enemy, self, self, nullptr, location, 60, DAMAGE_DEATH_KNOCKBACK | DAMAGE_EXTRA_KNOCKBACK, MOD_SNIPER);
		break;
	default:
		break;
	}

	if (enemy->client->ps.stats[STAT_HEALTH] <= 0) // if we are dead
	{
		vec3_t spot;

		VectorCopy(enemy->currentOrigin, spot);

		enemy->flags |= FL_DISINTEGRATED;
		enemy->svFlags |= SVF_BROADCAST;
		gentity_t* tent = G_TempEntity(spot, EV_DISINTEGRATION);
		tent->s.eventParm = PW_DISRUPTION;
		tent->svFlags |= SVF_BROADCAST;
		tent->owner = enemy;

		if (enemy->playerModel >= 0)
		{
			// don't let 'em animate
			gi.G2API_PauseBoneAnimIndex(&enemy->ghoul2[self->playerModel], enemy->rootBone, cg.time);
			gi.G2API_PauseBoneAnimIndex(&enemy->ghoul2[self->playerModel], enemy->motionBone, cg.time);
			gi.G2API_PauseBoneAnimIndex(&enemy->ghoul2[self->playerModel], enemy->lowerLumbarBone, cg.time);
		}

		//not solid anymore
		enemy->contents = 0;
		enemy->maxs[2] = -8;

		//need to pad deathtime some to stick around long enough for death effect to play
		enemy->NPC->timeOfDeath = level.time + 4000;
	}
}

void PushDamage(gentity_t* self, gentity_t* enemy, vec3_t location, int damageLevel)
{
	switch (damageLevel)
	{
	case FORCE_LEVEL_1:
		G_Damage(enemy, self, self, nullptr, location, 10, DAMAGE_DEATH_KNOCKBACK | DAMAGE_EXTRA_KNOCKBACK,
			MOD_UNKNOWN);
		break;
	case FORCE_LEVEL_2:
		G_Damage(enemy, self, self, nullptr, location, 15, DAMAGE_DEATH_KNOCKBACK | DAMAGE_EXTRA_KNOCKBACK,
			MOD_UNKNOWN);
		break;
	case FORCE_LEVEL_3:
		G_Damage(enemy, self, self, nullptr, location, 20, DAMAGE_DEATH_KNOCKBACK | DAMAGE_EXTRA_KNOCKBACK,
			MOD_UNKNOWN);
		break;
	default:
		break;
	}
}

void ForceThrow(gentity_t* self, qboolean pull, qboolean fake)
{
	//shove things in front of you away
	float dist;
	gentity_t* ent, * forwardEnt = nullptr;
	gentity_t* entityList[MAX_GENTITIES];
	gentity_t* push_list[MAX_GENTITIES];
	int numListedEntities = 0;
	vec3_t mins, maxs;
	vec3_t v;
	int i, e;
	int ent_count = 0;
	int radius;
	vec3_t center, ent_org, size, forward, right, end, dir, fwdangles = { 0 };
	// ReSharper disable once CppEntityAssignedButNoRead
	float dot1;
	float cone;
	trace_t tr;
	int anim, hold, soundIndex, cost;
	qboolean noResist = qfalse;
	int damageLevel = FORCE_LEVEL_0;

	if (self->health <= 0)
	{
		return;
	}
	if (self->client->ps.leanofs)
	{
		//can't force-throw while leaning
		return;
	}

	if (self->client->ps.pullAttackTime > level.time)
	{
		//already pull-attacking
		return;
	}

	if (PM_InLedgeMove(self->client->ps.legsAnim))
	{
		return;
	}

	if (PM_SaberInKata(static_cast<saberMoveName_t>(self->client->ps.saberMove)))
	{
		return;
	}

	if (!self->s.number && (cg.zoomMode || in_camera))
	{
		//can't force throw/pull when zoomed in or in cinematic
		return;
	}
	if (self->client->ps.saberLockTime > level.time)
	{
		if (pull || self->client->ps.forcePowerLevel[FP_PUSH] < FORCE_LEVEL_3)
		{
			//this can be a way to break out
			return;
		}
		//else, I'm breaking my half of the saberlock
		self->client->ps.saberLockTime = 0;
		self->client->ps.saberLockEnemy = ENTITYNUM_NONE;
	}

	if (self->client->ps.groundEntityNum == ENTITYNUM_NONE && self->client->ps.forcePowerLevel[FP_PUSH] > FORCE_LEVEL_2
		&& (self->s.weapon == WP_MELEE ||
			self->s.weapon == WP_NONE ||
			self->s.weapon == WP_SABER && !self->client->ps.SaberActive()))
	{
		damageLevel = FORCE_LEVEL_3;
	}
	else if (self->client->ps.groundEntityNum == ENTITYNUM_NONE && self->client->ps.forcePowerLevel[FP_PUSH] <
		FORCE_LEVEL_3
		&& (self->s.weapon == WP_MELEE ||
			self->s.weapon == WP_NONE ||
			self->s.weapon == WP_SABER && !self->client->ps.SaberActive()))
	{
		damageLevel = FORCE_LEVEL_2;
	}
	else
	{
		damageLevel = FORCE_LEVEL_1;
	}

	if (self->s.number >= MAX_CLIENTS && !G_ControlledByPlayer(self) && self->client->ps.weapon == WP_SABER)
		//npc force use limit
	{
		if (self->client->ps.blockPoints < 75 || self->client->ps.forcePower < 75)
		{
			return;
		}
	}

	if (self->client->ps.legsAnim == BOTH_KNOCKDOWN3
		|| self->client->ps.torsoAnim == BOTH_FORCE_GETUP_F1 && self->client->ps.torsoAnimTimer > 400
		|| self->client->ps.torsoAnim == BOTH_FORCE_GETUP_F2 && self->client->ps.torsoAnimTimer > 900
		|| self->client->ps.torsoAnim == BOTH_GETUP3 && self->client->ps.torsoAnimTimer > 500
		|| self->client->ps.torsoAnim == BOTH_GETUP4 && self->client->ps.torsoAnimTimer > 300
		|| self->client->ps.torsoAnim == BOTH_GETUP5 && self->client->ps.torsoAnimTimer > 500)
	{
		//we're face-down, so we'd only be force-push/pulling the floor
		return;
	}

	if (pull)
	{
		radius = forcePushPullRadius[self->client->ps.forcePowerLevel[FP_PULL]];
	}
	else
	{
		radius = forcePushPullRadius[self->client->ps.forcePowerLevel[FP_PUSH]];
	}

	if (!radius)
	{
		//no ability to do this yet
		return;
	}

	if (pull)
	{
		cost = force_power_needed[FP_PULL];
		if (!WP_ForcePowerUsable(self, FP_PULL, cost))
		{
			return;
		}
		//make sure this plays and that you cannot press fire for about 200ms after this
		anim = BOTH_FORCEPULL;
		soundIndex = G_SoundIndex("sound/weapons/force/pull.wav");
		hold = 200;
	}
	else
	{
		cost = force_power_needed[FP_PUSH];
		if (!WP_ForcePowerUsable(self, FP_PUSH, cost))
		{
			return;
		}
		//make sure this plays and that you cannot press fire for about 1 second after this
		if (self->s.weapon == WP_MELEE ||
			self->s.weapon == WP_NONE ||
			self->s.weapon == WP_SABER && !self->client->ps.SaberActive())
		{
			//2-handed PUSH
			if (self->client->ps.groundEntityNum == ENTITYNUM_NONE)
			{
				anim = BOTH_SUPERPUSH;
			}
			else
			{
				if (self->s.eFlags & EF_FORCE_DRAINED || self->s.eFlags & EF_FORCE_GRIPPED || self->s.eFlags &
					EF_FORCE_GRABBED)
				{
					anim = BOTH_FORCEPUSH;
				}
				else
				{
					anim = BOTH_2HANDPUSH;
				}
			}
		}
		else
		{
			anim = BOTH_FORCEPUSH;
		}

		hold = 650;

		if (self->s.weapon == WP_MELEE ||
			self->s.weapon == WP_NONE ||
			self->s.weapon == WP_SABER && !self->client->ps.SaberActive())
		{
			//2-handed PUSH
			if (self->client->ps.groundEntityNum == ENTITYNUM_NONE)
			{
				soundIndex = G_SoundIndex("sound/weapons/force/repulsepush.wav");
			}
			else
			{
				soundIndex = G_SoundIndex("sound/weapons/force/pushhard.mp3");
			}
		}
		else if (self->client->ps.saber[0].type == SABER_YODA) //saber yoda
		{
			soundIndex = G_SoundIndex("sound/weapons/force/pushyoda.mp3");
		}
		else if (self->client->ps.saber[0].type == SABER_UNSTABLE //saber kylo
			|| self->client->ps.saber[0].type == SABER_STAFF_UNSTABLE
			|| self->client->ps.saber[0].type == SABER_STAFF_MAUL
			|| self->client->ps.saber[0].type == SABER_BACKHAND
			|| self->client->ps.saber[0].type == SABER_ASBACKHAND
			|| self->client->ps.saber[0].type == SABER_ANAKIN
			|| self->client->ps.saber[0].type == SABER_PALP
			|| self->client->ps.saber[0].type == SABER_DOOKU) //saber yoda
		{
			soundIndex = G_SoundIndex("sound/weapons/force/push.mp3");
		}
		else if (self->client->ps.forcePower < 30 || PM_InKnockDown(&self->client->ps))
		{
			soundIndex = G_SoundIndex("sound/weapons/force/pushyoda.mp3");
		}
		else
		{
			soundIndex = G_SoundIndex("sound/weapons/force/pushlow.mp3");
		}
	}

	int parts = SETANIM_TORSO;

	if (!PM_InKnockDown(&self->client->ps))
	{
		if (self->client->ps.saberLockTime > level.time)
		{
			self->client->ps.saberLockTime = 0;
			self->painDebounceTime = level.time + 2000;
			hold += 1000;
			parts = SETANIM_BOTH;
		}
		else if (!VectorLengthSquared(self->client->ps.velocity) && !(self->client->ps.pm_flags & PMF_DUCKED))
		{
			parts = SETANIM_BOTH;
		}
	}

	NPC_SetAnim(self, parts, anim, SETANIM_FLAG_OVERRIDE | SETANIM_FLAG_HOLD | SETANIM_FLAG_RESTART);
	self->client->ps.saberMove = self->client->ps.saberBounceMove = LS_READY;
	//don't finish whatever saber anim you may have been in
	self->client->ps.saberBlocked = BLOCKED_NONE;

	if (self->client->ps.forcePowersActive & 1 << FP_SPEED)
	{
		hold = floor(hold * g_timescale->value);
	}

	self->client->ps.weaponTime = hold;
	self->client->ps.powerups[PW_FORCE_PUSH] = level.time + self->client->ps.torsoAnimTimer + 500;
	self->client->pushEffectFadeTime = 0;

	G_Sound(self, soundIndex);

	if (!pull && self->client->ps.forcePowersForced & 1 << FP_PUSH
		|| pull && self->client->ps.forcePowersForced & 1 << FP_PULL
		|| pull && self->client->NPC_class == CLASS_KYLE && self->spawnflags & 1 && TIMER_Done(self, "kyleTakesSaber"))
	{
		noResist = qtrue;
	}

	VectorCopy(self->client->ps.viewangles, fwdangles);
	AngleVectors(fwdangles, forward, right, nullptr);
	VectorCopy(self->currentOrigin, center);

	if (pull)
	{
		cone = forcePullCone[self->client->ps.forcePowerLevel[FP_PULL]];
	}
	else
	{
		cone = forcePushCone[self->client->ps.forcePowerLevel[FP_PUSH]];
	}

	if (self->client->ps.groundEntityNum != ENTITYNUM_NONE)
	{
		//must be pointing right at them
		VectorMA(self->client->renderInfo.eyePoint, radius, forward, end);
		gi.trace(&tr, self->client->renderInfo.eyePoint, vec3_origin, vec3_origin, end, self->s.number,
			MASK_OPAQUE | CONTENTS_SOLID | CONTENTS_BODY | CONTENTS_ITEM | CONTENTS_CORPSE,
			static_cast<EG2_Collision>(0), 0); //was MASK_SHOT, changed to match crosshair trace
		if (tr.entityNum < ENTITYNUM_WORLD)
		{
			//found something right in front of self,
			forwardEnt = &g_entities[tr.entityNum];
			if (!forwardEnt->client && !Q_stricmp("func_static", forwardEnt->classname))
			{
				if (forwardEnt->spawnflags & 1 || forwardEnt->spawnflags & 2)
				{
					//push/pullable
					if (forwardEnt->spawnflags & 32)
					{
						//can only push/pull ME, ignore all others
						if (forwardEnt->NPC_targetname == nullptr
							|| self->targetname && Q_stricmp(forwardEnt->NPC_targetname, self->targetname) == 0)
						{
							//anyone can push it or only 1 person can push it and it's me
							push_list[0] = forwardEnt;
							ent_count = numListedEntities = 1;
						}
					}
				}
			}
		}
	}

	if (forwardEnt)
	{
		if (G_TryingPullAttack(self, &self->client->usercmd, qtrue))
		{
			//we're going to try to do a pull attack on our forwardEnt
			if (WP_ForceThrowable(forwardEnt, forwardEnt, self, pull, cone, radius, forward))
			{
				//we will actually pull-attack him, so don't pull him or anything else here
				//activate the power, here, though, so the later check that actually does the pull attack knows we tried to pull
				self->client->ps.forcePowersActive |= 1 << FP_PULL;
				self->client->ps.forcePowerDebounce[FP_PULL] = level.time + 100; //force-pulling
				return;
			}
		}
	}

	//REPULSE ########################################################################## IN THE AIR PUSH
	if (self->client->ps.groundEntityNum == ENTITYNUM_NONE
		&& (self->s.weapon == WP_MELEE || self->s.weapon == WP_NONE || self->s.weapon == WP_SABER && !self->client->ps.
			SaberActive()))
	{
		if (pull)
		{
			if (!numListedEntities)
			{
				for (i = 0; i < 3; i++)
				{
					mins[i] = center[i] - radius;
					maxs[i] = center[i] + radius;
				}

				numListedEntities = gi.EntitiesInBox(mins, maxs, entityList, MAX_GENTITIES);

				for (e = 0; e < numListedEntities; e++)
				{
					ent = entityList[e];

					if (!WP_ForceThrowable(ent, forwardEnt, self, pull, cone, radius, forward))
					{
						continue;
					}

					//this is all to see if we need to start a saber attack, if it's in flight, this doesn't matter
					// find the distance from the edge of the bounding box
					for (i = 0; i < 3; i++)
					{
						if (center[i] < ent->absmin[i])
						{
							v[i] = ent->absmin[i] - center[i];
						}
						else if (center[i] > ent->absmax[i])
						{
							v[i] = center[i] - ent->absmax[i];
						}
						else
						{
							v[i] = 0;
						}
					}

					VectorSubtract(ent->absmax, ent->absmin, size);
					VectorMA(ent->absmin, 0.5, size, ent_org);

					//see if they're in front of me
					VectorSubtract(ent_org, center, dir);
					VectorNormalize(dir);
					if (cone < 1.0f)
					{
						//must be within the forward cone
						if (ent->client && !pull
							&& ent->client->ps.forceGripEntityNum == self->s.number
							&& (self->s.eFlags & EF_FORCE_GRIPPED || self->s.eFlags & EF_FORCE_GRABBED))
						{
							//this is the guy that's force-gripping me, use a wider cone regardless of force power level
							if ((dot1 = DotProduct(dir, forward)) < cone - 0.3f)
								continue;
						}
						else if (ent->client && !pull
							&& ent->client->ps.forceDrainEntityNum == self->s.number
							&& self->s.eFlags & EF_FORCE_DRAINED)
						{
							//this is the guy that's force-draining me, use a wider cone regardless of force power level
							if ((dot1 = DotProduct(dir, forward)) < cone - 0.3f)
								continue;
						}
						else if (ent->s.eType == ET_MISSILE)
						{
							//missiles are easier to force-push, never require direct trace (FIXME: maybe also items and general physics objects)
							if ((dot1 = DotProduct(dir, forward)) < cone - 0.3f)
								continue;
						}
						else if ((dot1 = DotProduct(dir, forward)) < cone)
						{
							continue;
						}
					}
					else if (ent->s.eType == ET_MISSILE)
					{
						//a missile and we're at force level 1... just use a small cone, but not ridiculously small
						if ((dot1 = DotProduct(dir, forward)) < 0.75f)
						{
							continue;
						}
					} //else is an NPC or brush entity that our forward trace would have to hit

					dist = VectorLength(v);

					//Now check and see if we can actually deflect it
					//method1
					//if within a certain range, deflect it
					if (ent->s.eType == ET_MISSILE && cone >= 1.0f)
					{
						//smaller radius on missile checks at force push 1
						if (dist >= 192)
						{
							continue;
						}
					}
					else if (dist >= radius)
					{
						continue;
					}

					//in PVS?
					if (!ent->bmodel && !gi.inPVS(ent_org, self->client->renderInfo.eyePoint))
					{
						//must be in PVS
						continue;
					}

					if (ent != forwardEnt)
					{
						//don't need to trace against forwardEnt again
						//really should have a clear LOS to this thing...
						gi.trace(&tr, self->client->renderInfo.eyePoint, vec3_origin, vec3_origin, ent_org,
							self->s.number, MASK_FORCE_PUSH, static_cast<EG2_Collision>(0), 0);
						//was MASK_SHOT, but changed to match above trace and crosshair trace
						if (tr.fraction < 1.0f && tr.entityNum != ent->s.number)
						{
							//must have clear LOS
							continue;
						}
					}

					// ok, we are within the radius, add us to the incoming list
					push_list[ent_count] = ent;
					ent_count++;
				}
			}
		}
		else
		{
			if (!numListedEntities)
			{
				for (i = 0; i < 3; i++)
				{
					mins[i] = center[i] - radius;
					maxs[i] = center[i] + radius;
				}

				numListedEntities = gi.EntitiesInBox(mins, maxs, entityList, MAX_GENTITIES);

				for (e = 0; e < numListedEntities; e++)
				{
					ent = entityList[e];

					if (!WP_ForceThrowable(ent, forwardEnt, self, pull, cone, radius, forward))
					{
						continue;
					}

					// find the distance from the edge of the bounding box
					for (i = 0; i < 3; i++)
					{
						if (center[i] < ent->absmin[i])
						{
							v[i] = ent->absmin[i] - center[i];
						}
						else if (center[i] > ent->absmax[i])
						{
							v[i] = center[i] - ent->absmax[i];
						}
						else
						{
							v[i] = 0;
						}
					}

					VectorSubtract(ent->absmax, ent->absmin, size);
					VectorMA(ent->absmin, 0.5, size, ent_org);

					//see if they're in front of me
					VectorSubtract(ent_org, center, dir);
					VectorNormalize(dir);

					if (self->client->ps.groundEntityNum != ENTITYNUM_NONE)
					{
						if (cone < 1.0f)
						{
							//must be within the forward cone
							if (ent->client && !pull
								&& ent->client->ps.forceGripEntityNum == self->s.number
								&& (self->s.eFlags & EF_FORCE_GRIPPED || self->s.eFlags & EF_FORCE_GRABBED))
							{
								//this is the guy that's force-gripping me, use a wider cone regardless of force power level
								if ((dot1 = DotProduct(dir, forward)) < cone - 0.3f)
									continue;
							}
							else if (ent->client && !pull
								&& ent->client->ps.forceDrainEntityNum == self->s.number
								&& self->s.eFlags & EF_FORCE_DRAINED)
							{
								//this is the guy that's force-draining me, use a wider cone regardless of force power level
								if ((dot1 = DotProduct(dir, forward)) < cone - 0.3f)
									continue;
							}
							else if (ent->s.eType == ET_MISSILE)
							{
								//missiles are easier to force-push, never require direct trace (FIXME: maybe also items and general physics objects)
								if ((dot1 = DotProduct(dir, forward)) < cone - 0.3f)
									continue;
							}
							else if ((dot1 = DotProduct(dir, forward)) < cone)
							{
								continue;
							}
						}
						else if (ent->s.eType == ET_MISSILE)
						{
							//a missile and we're at force level 1... just use a small cone, but not ridiculously small
							if ((dot1 = DotProduct(dir, forward)) < 0.75f)
							{
								continue;
							}
						} //else is an NPC or brush entity that our forward trace would have to hit
					}
					else if (ent->s.eType == ET_MISSILE)
					{
						//a missile and we're at force level 1... just use a small cone, but not ridiculously small
						if ((dot1 = DotProduct(dir, forward)) < 0.75f)
						{
							continue;
						}
					}

					dist = VectorLength(v);

					if (self->client->ps.groundEntityNum != ENTITYNUM_NONE)
					{
						//if within a certain range, deflect it
						if (ent->s.eType == ET_MISSILE && cone >= 1.0f)
						{
							//smaller radius on missile checks at force push 1
							if (dist >= 192)
							{
								continue;
							}
						}
						else if (dist >= radius)
						{
							continue;
						}
					}
					else if (dist >= radius)
					{
						continue;
					}

					//in PVS?
					if (!ent->bmodel && !gi.inPVS(ent_org, self->client->renderInfo.eyePoint))
					{
						//must be in PVS
						continue;
					}

					if (ent != forwardEnt)
					{
						gi.trace(&tr, self->client->renderInfo.eyePoint, vec3_origin, vec3_origin, ent_org,
							self->s.number, MASK_FORCE_PUSH, static_cast<EG2_Collision>(0), 0);
						if (tr.fraction < 1.0f && tr.entityNum != ent->s.number)
						{
							//must have clear LOS
							continue;
						}
					}

					// ok, we are within the radius, add us to the incoming list
					push_list[ent_count] = ent;
					ent_count++;
				}
			}
		}
	}
	else // ON THE GROUND ################################## NORMAL PUSH
	{
		if (!numListedEntities)
		{
			for (i = 0; i < 3; i++)
			{
				mins[i] = center[i] - radius;
				maxs[i] = center[i] + radius;
			}

			numListedEntities = gi.EntitiesInBox(mins, maxs, entityList, MAX_GENTITIES);

			for (e = 0; e < numListedEntities; e++)
			{
				ent = entityList[e];

				if (!WP_ForceThrowable(ent, forwardEnt, self, pull, cone, radius, forward))
				{
					continue;
				}

				//this is all to see if we need to start a saber attack, if it's in flight, this doesn't matter
				// find the distance from the edge of the bounding box
				for (i = 0; i < 3; i++)
				{
					if (center[i] < ent->absmin[i])
					{
						v[i] = ent->absmin[i] - center[i];
					}
					else if (center[i] > ent->absmax[i])
					{
						v[i] = center[i] - ent->absmax[i];
					}
					else
					{
						v[i] = 0;
					}
				}

				VectorSubtract(ent->absmax, ent->absmin, size);
				VectorMA(ent->absmin, 0.5, size, ent_org);

				//see if they're in front of me
				VectorSubtract(ent_org, center, dir);
				VectorNormalize(dir);

				if (cone < 1.0f)
				{
					//must be within the forward cone
					if (ent->client && !pull
						&& ent->client->ps.forceGripEntityNum == self->s.number
						&& (self->s.eFlags & EF_FORCE_GRIPPED || self->s.eFlags & EF_FORCE_GRABBED))
					{
						//this is the guy that's force-gripping me, use a wider cone regardless of force power level
						if ((dot1 = DotProduct(dir, forward)) < cone - 0.3f)
							continue;
					}
					else if (ent->client && !pull
						&& ent->client->ps.forceDrainEntityNum == self->s.number
						&& self->s.eFlags & EF_FORCE_DRAINED)
					{
						//this is the guy that's force-draining me, use a wider cone regardless of force power level
						if ((dot1 = DotProduct(dir, forward)) < cone - 0.3f)
							continue;
					}
					else if (ent->s.eType == ET_MISSILE)
						//&& ent->s.eType != ET_ITEM && ent->e_ThinkFunc != thinkF_G_RunObject )
					{
						//missiles are easier to force-push, never require direct trace (FIXME: maybe also items and general physics objects)
						if ((dot1 = DotProduct(dir, forward)) < cone - 0.3f)
							continue;
					}
					else if ((dot1 = DotProduct(dir, forward)) < cone)
					{
						continue;
					}
				}
				else if (ent->s.eType == ET_MISSILE)
				{
					//a missile and we're at force level 1... just use a small cone, but not ridiculously small
					if ((dot1 = DotProduct(dir, forward)) < 0.75f)
					{
						continue;
					}
				} //else is an NPC or brush entity that our forward trace would have to hit

				dist = VectorLength(v);

				//if within a certain range, deflect it
				if (ent->s.eType == ET_MISSILE && cone >= 1.0f)
				{
					//smaller radius on missile checks at force push 1
					if (dist >= 192)
					{
						continue;
					}
				}
				else if (dist >= radius)
				{
					continue;
				}

				//in PVS?
				if (!ent->bmodel && !gi.inPVS(ent_org, self->client->renderInfo.eyePoint))
				{
					//must be in PVS
					continue;
				}

				if (ent != forwardEnt)
				{
					//don't need to trace against forwardEnt again
					//really should have a clear LOS to this thing...
					gi.trace(&tr, self->client->renderInfo.eyePoint, vec3_origin, vec3_origin, ent_org, self->s.number,
						MASK_FORCE_PUSH, static_cast<EG2_Collision>(0), 0);
					//was MASK_SHOT, but changed to match above trace and crosshair trace
					if (tr.fraction < 1.0f && tr.entityNum != ent->s.number)
					{
						//must have clear LOS
						continue;
					}
				}

				// ok, we are within the radius, add us to the incoming list
				push_list[ent_count] = ent;
				ent_count++;
			}
		}
	}

	if (ent_count)
	{
		int actualCost;
		for (int x = 0; x < ent_count; x++)
		{
			if (push_list[x]->client)
			{
				float knockback = pull ? 0 : 200;

				//SIGH band-aid...
				if (push_list[x]->s.number >= MAX_CLIENTS
					&& self->s.number < MAX_CLIENTS)
				{
					if (push_list[x]->client->ps.forcePowersActive & 1 << FP_GRIP
						&& push_list[x]->client->ps.forceGripEntityNum == self->s.number)
					{
						WP_ForcePowerStop(push_list[x], FP_GRIP);
					}
					if (push_list[x]->client->ps.forcePowersActive & 1 << FP_DRAIN
						&& push_list[x]->client->ps.forceDrainEntityNum == self->s.number)
					{
						WP_ForcePowerStop(push_list[x], FP_DRAIN);
					}
				}

				if (Rosh_BeingHealed(push_list[x]))
				{
					continue;
				}
				if (push_list[x]->client->NPC_class == CLASS_HAZARD_TROOPER
					&& push_list[x]->health > 0)
				{
					//living hazard troopers resist push/pull
					WP_ForceThrowHazardTrooper(self, push_list[x], pull);
					continue;
				}
				if (fake)
				{
					//always resist
					WP_ResistForcePush(push_list[x], self, qfalse);
					continue;
				}
				int powerLevel, powerUse;
				if (pull)
				{
					powerLevel = self->client->ps.forcePowerLevel[FP_PULL];
					powerUse = FP_PULL;
				}
				else
				{
					powerLevel = self->client->ps.forcePowerLevel[FP_PUSH];
					powerUse = FP_PUSH;
				}

				int modPowerLevel = wp_absorb_conversion(push_list[x],
					push_list[x]->client->ps.forcePowerLevel[FP_ABSORB], self,
					powerUse, powerLevel,
					force_power_needed[self->client->ps.forcePowerLevel[
						powerUse]]);

				if (push_list[x]->client->NPC_class == CLASS_ASSASSIN_DROID ||
					push_list[x]->client->NPC_class == CLASS_DROIDEKA ||
					push_list[x]->client->NPC_class == CLASS_SBD ||
					push_list[x]->client->NPC_class == CLASS_HAZARD_TROOPER)
				{
					modPowerLevel = 0; // divides throw by 10
				}

				//First, if this is the player we're push/pulling, see if he can counter it
				if (modPowerLevel != -1
					&& !noResist
					&& in_front(self->currentOrigin, push_list[x]->client->renderInfo.eyePoint,
						push_list[x]->client->ps.viewangles, 0.3f))
				{
					//absorbed and I'm in front of them
					//counter it
					if (push_list[x]->client->ps.forcePowerLevel[FP_ABSORB] > FORCE_LEVEL_2)
					{
						//no reaction at all
					}
					else
					{
						WP_ResistForcePush(push_list[x], self, qfalse);
						push_list[x]->client->ps.saberMove = push_list[x]->client->ps.saberBounceMove = LS_READY;
						//don't finish whatever saber anim you may have been in
						push_list[x]->client->ps.saberBlocked = BLOCKED_NONE;
					}
					continue;
				}
				if (!push_list[x]->s.number)
				{
					//player
					if (!noResist && (ShouldPlayerResistForceThrow(push_list[x], self, pull)
						|| playeris_resisting_force_throw(push_list[x], self, pull))
						&& push_list[x]->client->ps.saberFatigueChainCount < MISHAPLEVEL_HEAVY)
					{
						WP_ResistForcePush(push_list[x], self, qfalse);
						push_list[x]->client->ps.saberMove = push_list[x]->client->ps.saberBounceMove = LS_READY;
						//don't finish whatever saber anim you may have been in
						push_list[x]->client->ps.saberBlocked = BLOCKED_NONE;
						continue;
					}
				}
				else if (push_list[x]->client && Jedi_WaitingAmbush(push_list[x]))
				{
					WP_ForceKnockdown(push_list[x], self, pull, qtrue, qfalse);
					soundIndex = G_SoundIndex("sound/weapons/force/pushed.mp3");

					if (self->s.weapon == WP_MELEE || self->s.weapon == WP_NONE && self->client->ps.groundEntityNum ==
						ENTITYNUM_NONE
						|| self->s.weapon == WP_SABER && !self->client->ps.SaberActive() && self->client->ps.
						groundEntityNum == ENTITYNUM_NONE)
					{
						RepulseDamage(self, push_list[x], tr.endpos, damageLevel);
					}
					else
					{
						if (pull)
						{
							//
						}
						else
						{
							PushDamage(self, push_list[x], tr.endpos, damageLevel);
						}
					}
					continue;
				}
				else if (PM_SaberInBrokenParry(push_list[x]->client->ps.saberMove))
				{
					//do a knockdown if fairly close
					WP_ForceKnockdown(push_list[x], self, pull, qtrue, qfalse);
					soundIndex = G_SoundIndex("sound/weapons/force/pushed.mp3");

					if (self->s.weapon == WP_MELEE || self->s.weapon == WP_NONE && self->client->ps.groundEntityNum ==
						ENTITYNUM_NONE
						|| self->s.weapon == WP_SABER && !self->client->ps.SaberActive() && self->client->ps.
						groundEntityNum == ENTITYNUM_NONE)
					{
						RepulseDamage(self, push_list[x], tr.endpos, damageLevel);
					}
					else
					{
						if (pull)
						{
							//
						}
						else
						{
							PushDamage(self, push_list[x], tr.endpos, damageLevel);
						}
					}
					continue;
				}

				G_KnockOffVehicle(push_list[x], self, pull);

				if (!pull
					&& push_list[x]->client->ps.forceDrainEntityNum == self->s.number
					&& self->s.eFlags & EF_FORCE_DRAINED)
				{
					//stop them from draining me now, dammit!
					WP_ForcePowerStop(push_list[x], FP_DRAIN);
				}

				//okay, everyone else (or player who couldn't resist it)...
				if ((self->s.number == 0 && Q_irand(0, 2) || Q_irand(0, 2)) && push_list[x]->client && push_list[x]->
					health > 0 //a living client
					&& push_list[x]->client->ps.weapon == WP_SABER //Jedi
					&& push_list[x]->health > 0 //alive
					&& push_list[x]->client->ps.forceRageRecoveryTime < level.time //not recovering from rage
					&& (self->client->NPC_class != CLASS_DESANN && self->client->NPC_class != CLASS_SITHLORD && self->
						client->NPC_class != CLASS_VADER && Q_stricmp("Yoda", self->NPC_type) || !Q_irand(0, 2))
					//only 30% chance of resisting a Desann push
					&& push_list[x]->client->ps.groundEntityNum != ENTITYNUM_NONE //on the ground
					&& in_front(self->currentOrigin, push_list[x]->currentOrigin, push_list[x]->client->ps.viewangles,
						0.3f) //I'm in front of him
					&& (push_list[x]->client->ps.powerups[PW_FORCE_PUSH] > level.time || //he's pushing too
						push_list[x]->s.number != 0 && push_list[x]->client->ps.weaponTime < level.time))
					//not the player and not attacking (NPC jedi auto-defend against pushes)
				{
					//Jedi don't get pushed, they resist as long as they aren't already attacking and are on the ground
					if (push_list[x]->client->ps.saberLockTime > level.time)
					{
						//they're in a lock
						if (push_list[x]->client->ps.saberLockEnemy != self->s.number)
						{
							//they're not in a lock with me
							continue;
						}
						if (pull || self->client->ps.forcePowerLevel[FP_PUSH] < FORCE_LEVEL_3 ||
							push_list[x]->client->ps.forcePowerLevel[FP_PUSH] > FORCE_LEVEL_2)
						{
							//they're in a lock with me, but my push is too weak
							continue;
						}
						//we will knock them down
						self->painDebounceTime = 0;
						self->client->ps.weaponTime = 500;
						if (self->client->ps.forcePowersActive & 1 << FP_SPEED)
						{
							self->client->ps.weaponTime = floor(self->client->ps.weaponTime * g_timescale->value);
						}
						soundIndex = G_SoundIndex("sound/weapons/force/pushed.mp3");
					}
					int resistChance = Q_irand(0, 2);
					if (!push_list[x]->s.number && self->client->ps.ManualBlockingFlags & 1 << HOLDINGBLOCK)
					{
						resistChance = 1;
					}
					else if (push_list[x]->s.number >= MAX_CLIENTS)
					{
						//NPC
						if (g_spskill->integer == 1)
						{
							//stupid tweak for graham
							resistChance = Q_irand(0, 3);
						}
					}
					if (noResist ||
						!pull
						&& modPowerLevel == -1
						&& self->client->ps.forcePowerLevel[FP_PUSH] > FORCE_LEVEL_2
						&& !resistChance
						&& push_list[x]->client->ps.forcePowerLevel[FP_PUSH] < FORCE_LEVEL_3)
					{
						//a level 3 push can even knock down a jedi
						if (PM_InKnockDown(&push_list[x]->client->ps))
						{
							//can't knock them down again
							continue;
						}
						WP_ForceKnockdown(push_list[x], self, pull, qfalse, qtrue);
						soundIndex = G_SoundIndex("sound/weapons/force/pushed.mp3");

						if (self->s.weapon == WP_MELEE || self->s.weapon == WP_NONE && self->client->ps.groundEntityNum
							== ENTITYNUM_NONE
							|| self->s.weapon == WP_SABER && !self->client->ps.SaberActive() && self->client->ps.
							groundEntityNum == ENTITYNUM_NONE)
						{
							RepulseDamage(self, push_list[x], tr.endpos, damageLevel);
						}
						else
						{
							if (pull)
							{
								//
							}
							else
							{
								PushDamage(self, push_list[x], tr.endpos, damageLevel);
							}
						}
					}
					else
					{
						WP_ResistForcePush(push_list[x], self, qfalse);
					}
				}
				else
				{
					vec3_t pushDir;
					//shove them
					if (push_list[x]->NPC
						&& push_list[x]->NPC->jumpState == JS_JUMPING)
					{
						//don't interrupt a scripted jump
						push_list[x]->forcePushTime = level.time + 600; // let the push effect last for 600 ms
						continue;
					}

					if (push_list[x]->s.number
						&& (push_list[x]->message || push_list[x]->flags & FL_NO_KNOCKBACK))
					{
						//an NPC who has a key
						WP_ForceKnockdown(push_list[x], self, pull, qfalse, qfalse);
						soundIndex = G_SoundIndex("sound/weapons/force/pushed.mp3");

						if (self->s.weapon == WP_MELEE || self->s.weapon == WP_NONE && self->client->ps.groundEntityNum
							== ENTITYNUM_NONE
							|| self->s.weapon == WP_SABER && !self->client->ps.SaberActive() && self->client->ps.
							groundEntityNum == ENTITYNUM_NONE)
						{
							RepulseDamage(self, push_list[x], tr.endpos, damageLevel);
						}
						else
						{
							if (pull)
							{
								//
							}
							else
							{
								PushDamage(self, push_list[x], tr.endpos, damageLevel);
							}
						}
						continue;
					}
					if (pull)
					{
						VectorSubtract(self->currentOrigin, push_list[x]->currentOrigin, pushDir);
						if (self->client->ps.forcePowerLevel[FP_PULL] >= FORCE_LEVEL_3
							&& self->client->NPC_class == CLASS_KYLE
							&& self->spawnflags & 1
							&& TIMER_Done(self, "kyleTakesSaber")
							&& push_list[x]->client
							&& push_list[x]->client->ps.weapon == WP_SABER
							&& !push_list[x]->client->ps.saberInFlight
							&& push_list[x]->client->ps.saberEntityNum < ENTITYNUM_WORLD
							&& !PM_InOnGroundAnim(&push_list[x]->client->ps))
						{
							vec3_t throwVec;
							VectorScale(pushDir, 10.0f, throwVec);
							WP_SaberLose(push_list[x], throwVec);
							NPC_SetAnim(push_list[x], SETANIM_BOTH, BOTH_LOSE_SABER,
								SETANIM_FLAG_OVERRIDE | SETANIM_FLAG_HOLD);
							push_list[x]->client->ps.torsoAnimTimer += 500;
							push_list[x]->client->ps.pm_time = push_list[x]->client->ps.weaponTime = push_list[x]->
								client->ps.torsoAnimTimer;
							push_list[x]->client->ps.pm_flags |= PMF_TIME_KNOCKBACK;
							push_list[x]->client->ps.saberMove = LS_NONE;
							push_list[x]->aimDebounceTime = level.time + push_list[x]->client->ps.torsoAnimTimer;
							VectorClear(push_list[x]->client->ps.velocity);
							VectorClear(push_list[x]->client->ps.moveDir);
							//Kyle will stand around for a bit, too...
							self->client->ps.pm_time = self->client->ps.weaponTime = 2000;
							self->client->ps.pm_flags |= PMF_TIME_KNOCKBACK;
							self->painDebounceTime = level.time + self->client->ps.weaponTime;
							TIMER_Set(self, "kyleTakesSaber", Q_irand(60000, 180000)); //don't do this again for a while
							G_AddVoiceEvent(self, Q_irand(EV_TAUNT1, EV_TAUNT3), Q_irand(4000, 6000));
							VectorClear(self->client->ps.velocity);
							VectorClear(self->client->ps.moveDir);
							continue;
						}
						if (push_list[x]->NPC
							&& push_list[x]->NPC->scriptFlags & SCF_DONT_FLEE)
						{
							//*SIGH*... if an NPC can't flee, they can't run after and pick up their weapon, do don't drop it
						}
						else if (self->client->ps.forcePowerLevel[FP_PULL] > FORCE_LEVEL_1
							&& push_list[x]->client->NPC_class != CLASS_ROCKETTROOPER
							//rockettroopers never drop their weapon
							&& push_list[x]->client->NPC_class != CLASS_VEHICLE
							&& push_list[x]->client->NPC_class != CLASS_BOBAFETT
							&& push_list[x]->client->NPC_class != CLASS_TUSKEN
							&& push_list[x]->client->NPC_class != CLASS_HAZARD_TROOPER
							&& push_list[x]->client->NPC_class != CLASS_ASSASSIN_DROID
							&& push_list[x]->client->NPC_class != CLASS_DROIDEKA
							&& push_list[x]->client->NPC_class != CLASS_SBD
							&& push_list[x]->s.weapon != WP_SABER
							&& push_list[x]->s.weapon != WP_MELEE
							&& push_list[x]->s.weapon != WP_THERMAL
							&& push_list[x]->s.weapon != WP_CONCUSSION) // so rax can't drop his
						{
							//yank the weapon - NOTE: level 1 just knocks them down, not take weapon
							if (in_front(self->currentOrigin, push_list[x]->currentOrigin,
								push_list[x]->client->ps.viewangles, 0.0f))
							{
								//enemy has to be facing me, too...
								WP_DropWeapon(push_list[x], pushDir);
							}
						}
						knockback += VectorNormalize(pushDir);
						if (knockback > 300)
						{
							knockback = 300;
						}
						if (self->client->ps.forcePowerLevel[FP_PULL] < FORCE_LEVEL_3)
						{
							//maybe just knock them down
							knockback /= 3;
						}
					}
					else
					{
						VectorSubtract(push_list[x]->currentOrigin, self->currentOrigin, pushDir);
						knockback -= VectorNormalize(pushDir);
						if (knockback < 200)
						{
							knockback = 200;
						}
						//scale for push level
						if (self->client->ps.forcePowerLevel[FP_PUSH] < FORCE_LEVEL_2)
						{
							//maybe just knock them down
							knockback /= 3;
						}
						else if (self->client->ps.forcePowerLevel[FP_PUSH] > FORCE_LEVEL_2)
						{
							//super-hard push
							soundIndex = G_SoundIndex("sound/weapons/force/pushed.mp3");
						}
						else if (self->s.weapon == WP_MELEE ||
							self->s.weapon == WP_NONE ||
							self->s.weapon == WP_SABER &&
							!self->client->ps.SaberActive() &&
							!PM_InKnockDown(&self->client->ps) &&
							self->client->ps.forcePowerLevel[FP_PUSH] > FORCE_LEVEL_2)
						{
							//super-hard push
							if (self->client->ps.groundEntityNum == ENTITYNUM_NONE)
							{
								knockback *= 6; //superpush
							}
							else
							{
								knockback *= 3; //superpush
							}
							soundIndex = G_SoundIndex("sound/weapons/force/pushed.mp3");
						}
					}

					if (modPowerLevel != -1)
					{
						if (!modPowerLevel)
						{
							knockback /= 10.0f;
						}
						else if (modPowerLevel == 1)
						{
							knockback /= 6.0f;
						}
						else
						{
							knockback /= 2.0f;
						}
					}
					//actually push/pull the enemy
					g_throw(push_list[x], pushDir, knockback);
					//make it so they don't actually hurt me when pulled at me...
					push_list[x]->forcePuller = self->s.number;

					if (push_list[x]->client->ps.groundEntityNum != ENTITYNUM_NONE)
					{
						//if on the ground, make sure they get shoved up some
						if (push_list[x]->client->ps.velocity[2] < knockback)
						{
							push_list[x]->client->ps.velocity[2] = knockback;
						}
					}

					if (push_list[x]->health > 0)
					{
						//target is still alive
						if ((push_list[x]->s.number || cg.renderingThirdPerson && !cg.zoomMode)
							//NPC or 3rd person player
							&& (!pull && self->client->ps.forcePowerLevel[FP_PUSH] < FORCE_LEVEL_2 && push_list[x]->
								client->ps.forcePowerLevel[FP_PUSH] < FORCE_LEVEL_1 //level 1 push
								|| pull && self->client->ps.forcePowerLevel[FP_PULL] < FORCE_LEVEL_2 && push_list[x]->
								client->ps.forcePowerLevel[FP_PULL] < FORCE_LEVEL_1)) //level 1 pull
						{
							//NPC or third person player (without force push/pull skill), and force push/pull level is at 1
							WP_ForceKnockdown(push_list[x], self, pull, static_cast<qboolean>(!pull && knockback > 150),
								qfalse);
							soundIndex = G_SoundIndex("sound/weapons/force/pushed.mp3");

							if (self->s.weapon == WP_MELEE || self->s.weapon == WP_NONE
								|| self->s.weapon == WP_SABER && !self->client->ps.SaberActive()
								&& self->client->ps.groundEntityNum == ENTITYNUM_NONE)
							{
								RepulseDamage(self, push_list[x], tr.endpos, damageLevel);
							}
							else
							{
								if (pull)
								{
									//
								}
								else
								{
									PushDamage(self, push_list[x], tr.endpos, damageLevel);
								}
							}
						}
						else if (!push_list[x]->s.number)
						{
							//player, have to force an anim on him
							WP_ForceKnockdown(push_list[x], self, pull, static_cast<qboolean>(!pull && knockback > 150),
								qfalse);
							soundIndex = G_SoundIndex("sound/weapons/force/pushed.mp3");

							if (self->s.weapon == WP_MELEE || self->s.weapon == WP_NONE && self->client->ps.
								groundEntityNum == ENTITYNUM_NONE
								|| self->s.weapon == WP_SABER && !self->client->ps.SaberActive() && self->client->ps.
								groundEntityNum == ENTITYNUM_NONE)
							{
								RepulseDamage(self, push_list[x], tr.endpos, damageLevel);
							}
							else
							{
								if (pull)
								{
									//
								}
								else
								{
									PushDamage(self, push_list[x], tr.endpos, damageLevel);
								}
							}
						}
						else
						{
							//NPC and force-push/pull at level 2 or higher
							WP_ForceKnockdown(push_list[x], self, pull, static_cast<qboolean>(!pull && knockback > 100),
								qfalse);
							soundIndex = G_SoundIndex("sound/weapons/force/pushed.mp3");

							if (self->s.weapon == WP_MELEE || self->s.weapon == WP_NONE && self->client->ps.
								groundEntityNum == ENTITYNUM_NONE
								|| self->s.weapon == WP_SABER && !self->client->ps.SaberActive() && self->client->ps.
								groundEntityNum == ENTITYNUM_NONE)
							{
								RepulseDamage(self, push_list[x], tr.endpos, damageLevel);
							}
							else
							{
								if (pull)
								{
									//
								}
								else
								{
									PushDamage(self, push_list[x], tr.endpos, damageLevel);
								}
							}
						}
						soundIndex = G_SoundIndex("sound/weapons/force/pushed.mp3");
					}
					push_list[x]->forcePushTime = level.time + 600; // let the push effect last for 600 ms
				}
			}
			else if (!fake)
			{
				//not a fake push/pull
				if (push_list[x]->s.weapon == WP_SABER && push_list[x]->contents & CONTENTS_LIGHTSABER)
				{
					//a thrown saber, just send it back
					if (push_list[x]->owner && push_list[x]->owner->client && push_list[x]->owner->client->ps.
						SaberActive() && push_list[x]->s.pos.trType == TR_LINEAR && push_list[x]->owner->client->ps.
						saberEntityState != SES_RETURNING)
					{
						//it's on and being controlled
						if (self->s.number == 0 || Q_irand(0, 2))
						{
							//certain chance of throwing it aside and turning it off?
							//give it some velocity away from me
							if (Q_irand(0, 1))
							{
								VectorScale(right, -1, right);
							}

							if (self->s.number >= MAX_CLIENTS && !G_ControlledByPlayer(self))
							{
								g_reflect_missile_npc(self, push_list[x], right);
							}
							else
							{
								g_reflect_missile_auto(self, push_list[x], right);
							}
							WP_SaberDrop(push_list[x]->owner, push_list[x]);
						}
						else
						{
							WP_SaberDrop(push_list[x]->owner, push_list[x]);
						}
					}
				}
				else if (push_list[x]->s.eType == ET_MISSILE
					&& push_list[x]->s.pos.trType != TR_STATIONARY
					&& (push_list[x]->s.pos.trType != TR_INTERPOLATE || push_list[x]->s.weapon != WP_THERMAL))
					//rolling and stationary thermal detonators are dealt with below
				{
					vec3_t dir2Me;
					VectorSubtract(self->currentOrigin, push_list[x]->currentOrigin, dir2Me);
					float dot = DotProduct(push_list[x]->s.pos.trDelta, dir2Me);
					if (pull)
					{
						//deflect rather than reflect?
					}
					else
					{
						if (push_list[x]->s.eFlags & EF_MISSILE_STICK)
						{
							//caught a sticky in-air
							push_list[x]->s.eType = ET_MISSILE;
							push_list[x]->s.eFlags &= ~EF_MISSILE_STICK;
							push_list[x]->s.eFlags |= EF_BOUNCE_HALF;
							push_list[x]->splashDamage /= 3;
							push_list[x]->splashRadius /= 3;
							push_list[x]->e_ThinkFunc = thinkF_WP_Explode;
							push_list[x]->nextthink = level.time + Q_irand(500, 3000);
						}
						if (dot >= 0)
						{
							//it's heading towards me
							if (self->s.number >= MAX_CLIENTS && !G_ControlledByPlayer(self))
							{
								g_reflect_missile_npc(self, push_list[x], forward);
							}
							else
							{
								g_reflect_missile_auto(self, push_list[x], forward);
							}
						}
						else
						{
							VectorScale(push_list[x]->s.pos.trDelta, 1.25f, push_list[x]->s.pos.trDelta);
						}
					}
					if (push_list[x]->s.eType == ET_MISSILE
						&& push_list[x]->s.weapon == WP_ROCKET_LAUNCHER
						&& push_list[x]->damage < 60)
					{
						//pushing away a rocket raises it's damage to the max for NPCs
						push_list[x]->damage = 60;
					}
				}
				else if (push_list[x]->svFlags & SVF_GLASS_BRUSH)
				{
					//break the glass
					trace_t trace;
					vec3_t pushDir;
					float damage = 800;

					AngleVectors(self->client->ps.viewangles, forward, nullptr, nullptr);
					VectorNormalize(forward);
					VectorMA(self->client->renderInfo.eyePoint, radius, forward, end);
					gi.trace(&trace, self->client->renderInfo.eyePoint, vec3_origin, vec3_origin, end, self->s.number,
						MASK_SHOT, static_cast<EG2_Collision>(0), 0);
					if (trace.entityNum != push_list[x]->s.number || trace.fraction == 1.0 || trace.allsolid || trace.
						startsolid)
					{
						//must be pointing right at it
						continue;
					}

					if (pull)
					{
						VectorSubtract(self->client->renderInfo.eyePoint, trace.endpos, pushDir);
					}
					else
					{
						VectorSubtract(trace.endpos, self->client->renderInfo.eyePoint, pushDir);
					}
					damage -= VectorNormalize(pushDir);
					if (damage < 200)
					{
						damage = 200;
					}
					VectorScale(pushDir, damage, pushDir);

					G_Damage(push_list[x], self, self, pushDir, trace.endpos, damage, 0, MOD_UNKNOWN);
				}
				else if (!Q_stricmp("func_static", push_list[x]->classname))
				{
					//force-usable func_static
					if (!pull && push_list[x]->spawnflags & 1/*F_PUSH*/)
					{
						if (push_list[x]->NPC_targetname == nullptr
							|| self->targetname && Q_stricmp(push_list[x]->NPC_targetname, self->targetname) == 0)
						{
							//anyone can pull it or only 1 person can push it and it's me
							GEntity_UseFunc(push_list[x], self, self);
						}
					}
					else if (pull && push_list[x]->spawnflags & 2/*F_PULL*/)
					{
						if (push_list[x]->NPC_targetname == nullptr
							|| self->targetname && Q_stricmp(push_list[x]->NPC_targetname, self->NPC_targetname) == 0)
						{
							//anyone can push it or only 1 person can push it and it's me
							GEntity_UseFunc(push_list[x], self, self);
						}
					}
				}
				else if (!Q_stricmp("func_door", push_list[x]->classname) && push_list[x]->spawnflags & 2)
				{
					//push/pull the door
					vec3_t pos1, pos2;

					AngleVectors(self->client->ps.viewangles, forward, nullptr, nullptr);
					VectorNormalize(forward);
					VectorMA(self->client->renderInfo.eyePoint, radius, forward, end);
					gi.trace(&tr, self->client->renderInfo.eyePoint, vec3_origin, vec3_origin, end, self->s.number,
						MASK_SHOT, static_cast<EG2_Collision>(0), 0);
					if (tr.entityNum != push_list[x]->s.number || tr.fraction == 1.0 || tr.allsolid || tr.startsolid)
					{
						//must be pointing right at it
						continue;
					}

					if (VectorCompare(vec3_origin, push_list[x]->s.origin))
					{
						//does not have an origin brush, so pos1 & pos2 are relative to world origin, need to calc center
						VectorSubtract(push_list[x]->absmax, push_list[x]->absmin, size);
						VectorMA(push_list[x]->absmin, 0.5, size, center);
						if (push_list[x]->spawnflags & 1 && push_list[x]->moverState == MOVER_POS1)
						{
							//if at pos1 and started open, make sure we get the center where it *started* because we're going to add back in the relative values pos1 and pos2
							VectorSubtract(center, push_list[x]->pos1, center);
						}
						else if (!(push_list[x]->spawnflags & 1) && push_list[x]->moverState == MOVER_POS2)
						{
							//if at pos2, make sure we get the center where it *started* because we're going to add back in the relative values pos1 and pos2
							VectorSubtract(center, push_list[x]->pos2, center);
						}
						VectorAdd(center, push_list[x]->pos1, pos1);
						VectorAdd(center, push_list[x]->pos2, pos2);
					}
					else
					{
						//actually has an origin, pos1 and pos2 are absolute
						VectorCopy(push_list[x]->currentOrigin, center);
						VectorCopy(push_list[x]->pos1, pos1);
						VectorCopy(push_list[x]->pos2, pos2);
					}

					if (Distance(pos1, self->client->renderInfo.eyePoint) < Distance(
						pos2, self->client->renderInfo.eyePoint))
					{
						//pos1 is closer
						if (push_list[x]->moverState == MOVER_POS1)
						{
							//at the closest pos
							if (pull)
							{
								//trying to pull, but already at closest point, so screw it
								continue;
							}
						}
						else if (push_list[x]->moverState == MOVER_POS2)
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
						if (push_list[x]->moverState == MOVER_POS1)
						{
							//at the farthest pos
							if (!pull)
							{
								//trying to push, but already at farthest point, so screw it
								continue;
							}
						}
						else if (push_list[x]->moverState == MOVER_POS2)
						{
							//at closest pos
							if (pull)
							{
								//trying to pull, but already at closest point, so screw it
								continue;
							}
						}
					}
					GEntity_UseFunc(push_list[x], self, self);
				}
				else if (push_list[x]->s.eType == ET_MISSILE /*thermal resting on ground*/
					|| push_list[x]->s.eType == ET_ITEM
					|| push_list[x]->e_ThinkFunc == thinkF_G_RunObject || Q_stricmp("limb", push_list[x]->classname) ==
					0)
				{
					//general object, toss it
					vec3_t pushDir, kvel;
					float knockback = pull ? 0 : 200;
					float mass = 200;

					if (pull)
					{
						if (push_list[x]->s.eType == ET_ITEM)
						{
							//pull it to a little higher point
							vec3_t adjustedOrg;
							VectorCopy(self->currentOrigin, adjustedOrg);
							adjustedOrg[2] += self->maxs[2] / 3;
							VectorSubtract(adjustedOrg, push_list[x]->currentOrigin, pushDir);
						}
						else if (self->enemy //I have an enemy
							//&& push_list[x]->s.eType != ET_ITEM //not an item
							&& self->client->ps.forcePowerLevel[FP_PUSH] > FORCE_LEVEL_2 //have push 3 or greater
							&& in_front(push_list[x]->currentOrigin, self->currentOrigin, self->currentAngles, 0.25f)
							//object is generally in front of me
							&& in_front(self->enemy->currentOrigin, self->currentOrigin, self->currentAngles, 0.75f)
							//enemy is pretty much right in front of me
							&& !in_front(push_list[x]->currentOrigin, self->enemy->currentOrigin,
								self->enemy->currentAngles, -0.25f) //object is generally behind enemy
							//FIXME: check dist to enemy and clear LOS to enemy and clear Path between object and enemy?
							&& (self->NPC && (noResist || Q_irand(0, RANK_CAPTAIN) < self->NPC->rank)
								//NPC with enough skill
								|| self->s.number < MAX_CLIENTS))
						{
							//if I have an auto-enemy & he's in front of me, push it toward him!
							VectorSubtract(self->enemy->currentOrigin, push_list[x]->currentOrigin, pushDir);
						}
						else
						{
							VectorSubtract(self->currentOrigin, push_list[x]->currentOrigin, pushDir);
						}
						knockback += VectorNormalize(pushDir);
						if (knockback > 200)
						{
							knockback = 200;
						}
						if (push_list[x]->s.eType == ET_ITEM
							&& push_list[x]->item
							&& push_list[x]->item->giType == IT_HOLDABLE
							&& push_list[x]->item->giTag == INV_SECURITY_KEY)
						{
							//security keys are pulled with less enthusiasm
							if (knockback > 100)
							{
								knockback = 100;
							}
						}
						else if (knockback > 200)
						{
							knockback = 200;
						}
					}
					else
					{
						if (self->enemy //I have an enemy
							&& push_list[x]->s.eType != ET_ITEM //not an item
							&& self->client->ps.forcePowerLevel[FP_PUSH] > FORCE_LEVEL_2 //have push 3 or greater
							&& in_front(push_list[x]->currentOrigin, self->currentOrigin, self->currentAngles, 0.25f)
							//object is generally in front of me
							&& in_front(self->enemy->currentOrigin, self->currentOrigin, self->currentAngles, 0.75f)
							//enemy is pretty much right in front of me
							&& in_front(push_list[x]->currentOrigin, self->enemy->currentOrigin,
								self->enemy->currentAngles, 0.25f) //object is generally in front of enemy
							//FIXME: check dist to enemy and clear LOS to enemy and clear Path between object and enemy?
							&& (self->NPC && (noResist || Q_irand(0, RANK_CAPTAIN) < self->NPC->rank)
								//NPC with enough skill
								|| self->s.number < MAX_CLIENTS))
						{
							//if I have an auto-enemy & he's in front of me, push it toward him!
							VectorSubtract(self->enemy->currentOrigin, push_list[x]->currentOrigin, pushDir);
						}
						else
						{
							VectorSubtract(push_list[x]->currentOrigin, self->currentOrigin, pushDir);
						}
						knockback -= VectorNormalize(pushDir);
						if (knockback < 100)
						{
							knockback = 100;
						}
					}
					//FIXME: if pull a FL_FORCE_PULLABLE_ONLY, clear the flag, assuming it's no longer in solid?  or check?
					VectorCopy(push_list[x]->currentOrigin, push_list[x]->s.pos.trBase);
					push_list[x]->s.pos.trTime = level.time; // move a bit on the very first frame
					if (push_list[x]->s.pos.trType != TR_INTERPOLATE)
					{
						//don't do this to rolling missiles
						push_list[x]->s.pos.trType = TR_GRAVITY;
					}

					if (push_list[x]->e_ThinkFunc == thinkF_G_RunObject && push_list[x]->physicsBounce)
					{
						//it's a pushable misc_model_breakable, use it's mass instead of our one-size-fits-all mass
						mass = push_list[x]->physicsBounce; //same as push_list[x]->mass, right?
					}
					if (mass < 50)
					{
						//???
						mass = 50;
					}
					if (g_gravity->value > 0)
					{
						VectorScale(pushDir, g_knockback->value * knockback / mass * 0.8, kvel);
						kvel[2] = pushDir[2] * g_knockback->value * knockback / mass * 1.5;
					}
					else
					{
						VectorScale(pushDir, g_knockback->value * knockback / mass, kvel);
					}
					VectorAdd(push_list[x]->s.pos.trDelta, kvel, push_list[x]->s.pos.trDelta);
					if (g_gravity->value > 0)
					{
						if (push_list[x]->s.pos.trDelta[2] < knockback)
						{
							push_list[x]->s.pos.trDelta[2] = knockback;
						}
					}
					//no trDuration?
					if (push_list[x]->e_ThinkFunc != thinkF_G_RunObject)
					{
						//objects spin themselves?
						//spin it
						//FIXME: messing with roll ruins the rotational center???
						push_list[x]->s.apos.trTime = level.time;
						push_list[x]->s.apos.trType = TR_LINEAR;
						VectorClear(push_list[x]->s.apos.trDelta);
						push_list[x]->s.apos.trDelta[1] = Q_irand(-800, 800);
					}

					if (Q_stricmp("limb", push_list[x]->classname) == 0)
					{
						//make sure it runs it's physics
						push_list[x]->e_ThinkFunc = thinkF_LimbThink;
						push_list[x]->nextthink = level.time + FRAMETIME;
					}
					push_list[x]->forcePushTime = level.time + 600; // let the push effect last for 600 ms
					push_list[x]->forcePuller = self->s.number; //remember this regardless
					if (push_list[x]->item && push_list[x]->item->giTag == INV_SECURITY_KEY)
					{
						AddSightEvent(player, push_list[x]->currentOrigin, 128, AEL_DISCOVERED);
						//security keys are more important
					}
					else
					{
						AddSightEvent(player, push_list[x]->currentOrigin, 128, AEL_SUSPICIOUS);
						//hmm... or should this always be discovered?
					}
				}
				else if (push_list[x]->s.weapon == WP_TURRET
					&& !Q_stricmp("PAS", push_list[x]->classname)
					&& push_list[x]->s.apos.trType == TR_STATIONARY)
				{
					//a portable turret
					WP_KnockdownTurret(self, push_list[x]);
				}
			}
		}
		if (pull)
		{
			if (self->client->ps.forcePowerLevel[FP_PULL] > FORCE_LEVEL_2)
			{
				//at level 3, can pull multiple, so it costs more
				actualCost = force_power_needed[FP_PULL] * ent_count;
				if (actualCost > 50)
				{
					actualCost = 50;
				}
				else if (actualCost < cost)
				{
					actualCost = cost;
				}
			}
			else
			{
				actualCost = cost;
			}
			WP_ForcePowerStart(self, FP_PULL, actualCost);
		}
		else
		{
			if (self->client->ps.forcePowerLevel[FP_PUSH] > FORCE_LEVEL_2)
			{
				//at level 3, can push multiple, so costs more
				actualCost = force_power_needed[FP_PUSH] * ent_count;
				if (actualCost > 50)
				{
					actualCost = 50;
				}
				else if (actualCost < cost)
				{
					actualCost = cost;
				}
			}
			else if (self->client->ps.forcePowerLevel[FP_PUSH] > FORCE_LEVEL_1)
			{
				//at level 2, can push multiple, so costs more
				actualCost = floor(force_power_needed[FP_PUSH] * ent_count / 1.5f);
				if (actualCost > 50)
				{
					actualCost = 50;
				}
				else if (actualCost < cost)
				{
					actualCost = cost;
				}
			}
			else
			{
				actualCost = cost;
			}
			WP_ForcePowerStart(self, FP_PUSH, actualCost);
		}
	}
	else
	{
		//didn't push or pull anything?  don't penalize them too much
		if (pull)
		{
			WP_ForcePowerStart(self, FP_PULL, 5);
		}
		else
		{
			WP_ForcePowerStart(self, FP_PUSH, 5);
		}
	}
	if (pull)
	{
		if (self->NPC)
		{
			//NPCs can push more often
			self->client->ps.forcePowerDebounce[FP_PULL] = level.time + 200;
		}
		else
		{
			self->client->ps.forcePowerDebounce[FP_PULL] = level.time + self->client->ps.torsoAnimTimer + 500;
		}
	}
	else
	{
		if (self->NPC)
		{
			//NPCs can push more often
			self->client->ps.forcePowerDebounce[FP_PUSH] = level.time + 200;
		}
		else
		{
			self->client->ps.forcePowerDebounce[FP_PUSH] = level.time + self->client->ps.torsoAnimTimer + 500;
		}
	}
}

void WP_DebounceForceDeactivateTime(const gentity_t* self)
{
	if (self && self->client)
	{
		if (self->client->ps.forcePowersActive & 1 << FP_SPEED
			|| self->client->ps.forcePowersActive & 1 << FP_PROTECT
			|| self->client->ps.forcePowersActive & 1 << FP_ABSORB
			|| self->client->ps.forcePowersActive & 1 << FP_RAGE
			|| self->client->ps.forcePowersActive & 1 << FP_SEE
			|| self->client->ps.forcePowersActive & 1 << FP_DEADLYSIGHT)
		{
			//already running another power that can be manually, stopped don't debounce so long
			self->client->ps.forceAllowDeactivateTime = level.time + 500;
		}
		else
		{
			//not running one of the interrupt able powers
			self->client->ps.forceAllowDeactivateTime = level.time + 1500;
		}
	}
}

void ForceDeadlySight(gentity_t* self)
{
	if (self->health <= 0)
	{
		return;
	}

	if (self->client->ps.forcePowerDebounce[FP_DEADLYSIGHT] > level.time)
	{
		//already pushing- now you can't haul someone across the room, sorry
		return;
	}

	if (self->client->ps.forceAllowDeactivateTime < level.time &&
		self->client->ps.forcePowersActive & 1 << FP_DEADLYSIGHT)
	{
		WP_ForcePowerStop(self, FP_DEADLYSIGHT);
		return;
	}

	if (!WP_ForcePowerUsable(self, FP_DEADLYSIGHT, 0))
	{
		return;
	}

	WP_DebounceForceDeactivateTime(self);

	WP_ForcePowerStart(self, FP_DEADLYSIGHT, 0);

	if (self->client->ps.saberLockTime < level.time && !PM_InKnockDown(&self->client->ps))
	{
		if (self->client->ps.forcePowerLevel[FP_DEADLYSIGHT] < FORCE_LEVEL_3)
		{
			//animate
			int parts = SETANIM_BOTH;
			if (self->client->ps.forcePowerLevel[FP_DEADLYSIGHT] > FORCE_LEVEL_1)
			{
				//level 2 only does it on torso (can keep running)
				parts = SETANIM_TORSO;
			}
			else
			{
				if (self->client->ps.groundEntityNum != ENTITYNUM_NONE)
				{
					VectorClear(self->client->ps.velocity);
				}
				if (self->NPC)
				{
					VectorClear(self->client->ps.moveDir);
					self->client->ps.speed = 0;
				}
			}
			NPC_SetAnim(self, parts, BOTH_FORCE_RAGE, SETANIM_FLAG_OVERRIDE | SETANIM_FLAG_HOLD);
		}
		else
		{
			NPC_SetAnim(self, SETANIM_TORSO, BOTH_FORCE_RAGE, SETANIM_FLAG_OVERRIDE | SETANIM_FLAG_HOLD);
			self->client->ps.weaponTime = self->client->ps.torsoAnimTimer;
		}
	}
	CG_PlayEffectBolted("misc/breath.efx", self->playerModel, self->headBolt, self->s.number, self->currentOrigin);

	self->client->ps.forcePowerDebounce[FP_DEADLYSIGHT] = level.time + self->client->ps.torsoAnimTimer + 5000;
}

void ForceRepulseDamage(gentity_t* self, gentity_t* enemy, vec3_t location, int damageLevel)
{
	switch (damageLevel)
	{
	case FORCE_LEVEL_1:
		G_Damage(enemy, self, self, nullptr, location, 10, DAMAGE_NO_KNOCKBACK, MOD_UNKNOWN);
		break;
	case FORCE_LEVEL_2:
		G_Damage(enemy, self, self, nullptr, location, 25, DAMAGE_NO_KNOCKBACK, MOD_UNKNOWN);
		break;
	case FORCE_LEVEL_3:
		G_Damage(enemy, self, self, nullptr, location, 50, DAMAGE_NO_KNOCKBACK, MOD_UNKNOWN);
		break;
	default:
		break;
	}
}

void ForceRepulseThrow(gentity_t* self, int chargeTime)
{
	//shove things around you away
	qboolean fake = qfalse;
	gentity_t* push_list[MAX_GENTITIES];
	int numListedEntities = 0;
	int ent_count = 0;
	int radius;
	vec3_t center, forward, right, fwdangles = { 0 };
	trace_t tr;
	int anim, hold, soundIndex, cost;
	int damageLevel = FORCE_LEVEL_0;

	if (self->health <= 0)
	{
		return;
	}
	if (self->client->ps.leanofs)
	{
		//can't force-throw while leaning
		return;
	}
	if (self->client->ps.forcePowerDebounce[FP_REPULSE] > level.time)
	{
		//already pushing- now you can't haul someone across the room, sorry
		return;
	}
	if (!self->s.number && (cg.zoomMode || in_camera))
	{
		//can't force throw/pull when zoomed in or in cinematic
		return;
	}
	if (self->client->ps.saberLockTime > level.time)
	{
		if (self->client->ps.forcePowerLevel[FP_REPULSE] < FORCE_LEVEL_3)
		{
			//this can be a way to break out
			return;
		}
		//else, I'm breaking my half of the saberlock
		self->client->ps.saberLockTime = 0;
		self->client->ps.saberLockEnemy = ENTITYNUM_NONE;
	}

	if (self->client->ps.legsAnim == BOTH_KNOCKDOWN3
		|| self->client->ps.torsoAnim == BOTH_FORCE_GETUP_F1 && self->client->ps.torsoAnimTimer > 400
		|| self->client->ps.torsoAnim == BOTH_FORCE_GETUP_F2 && self->client->ps.torsoAnimTimer > 900
		|| self->client->ps.torsoAnim == BOTH_GETUP3 && self->client->ps.torsoAnimTimer > 500
		|| self->client->ps.torsoAnim == BOTH_GETUP4 && self->client->ps.torsoAnimTimer > 300
		|| self->client->ps.torsoAnim == BOTH_GETUP5 && self->client->ps.torsoAnimTimer > 500)
	{
		//we're face-down, so we'd only be force-push/pulling the floor
		return;
	}

	radius = forcePushPullRadius[self->client->ps.forcePowerLevel[FP_REPULSE]];

	if (!radius)
	{
		//no ability to do this yet
		return;
	}

	if (chargeTime > 2500.0f)
	{
		damageLevel = FORCE_LEVEL_3;
	}
	else if (chargeTime > 1250.0f)
	{
		damageLevel = FORCE_LEVEL_2;
	}
	else if (chargeTime > 500.0f)
	{
		damageLevel = FORCE_LEVEL_1;
	}

	cost = force_power_needed[FP_REPULSE];
	if (!WP_ForcePowerUsable(self, FP_REPULSE, cost))
	{
		return;
	}
	//make sure this plays and that you cannot press fire for about 1 second after this
	anim = BOTH_SUPERPUSH;
	soundIndex = G_SoundIndex("sound/weapons/force/repulsepush.wav");
	hold = 650;

	int parts = SETANIM_TORSO;
	if (!PM_InKnockDown(&self->client->ps))
	{
		if (self->client->ps.saberLockTime > level.time)
		{
			self->client->ps.saberLockTime = 0;
			self->painDebounceTime = level.time + 2000;
			hold += 1000;
			parts = SETANIM_BOTH;
		}
	}
	NPC_SetAnim(self, parts, anim, SETANIM_FLAG_OVERRIDE | SETANIM_FLAG_HOLD | SETANIM_FLAG_RESTART);
	self->client->ps.saberMove = self->client->ps.saberBounceMove = LS_READY;
	//don't finish whatever saber anim you may have been in
	self->client->ps.saberBlocked = BLOCKED_NONE;
	if (self->client->ps.forcePowersActive & 1 << FP_SPEED)
	{
		hold = floor(hold * g_timescale->value);
	}
	self->client->ps.weaponTime = hold; //was 1000, but want to swing sooner
	//do effect... FIXME: build-up or delay this until in proper part of anim
	self->client->ps.powerups[PW_FORCE_REPULSE] = level.time + self->client->ps.torsoAnimTimer + 500;
	//reset to 0 in case it's still > 0 from a previous push
	self->client->pushEffectFadeTime = 0;

	G_Sound(self, soundIndex);

	VectorCopy(self->client->ps.viewangles, fwdangles);
	//fwdangles[1] = self->client->ps.viewangles[1];
	AngleVectors(fwdangles, forward, right, nullptr);
	VectorCopy(self->currentOrigin, center);

	if (!numListedEntities)
	{
		vec3_t mins;
		vec3_t maxs;
		vec3_t v;
		int i;
		int e;
		gentity_t* entityList[MAX_GENTITIES];
		gentity_t* ent;
		float dist;
		for (i = 0; i < 3; i++)
		{
			mins[i] = center[i] - radius;
			maxs[i] = center[i] + radius;
		}

		numListedEntities = gi.EntitiesInBox(mins, maxs, entityList, MAX_GENTITIES);

		for (e = 0; e < numListedEntities; e++)
		{
			vec3_t ent_org;
			vec3_t size;
			vec3_t dir;
			gentity_t* forwardEnt = nullptr;
			ent = entityList[e];

			if (!WP_ForceThrowable(ent, forwardEnt, self, qfalse, 0.0f, radius, forward))
			{
				continue;
			}

			//this is all to see if we need to start a saber attack, if it's in flight, this doesn't matter
			// find the distance from the edge of the bounding box
			for (i = 0; i < 3; i++)
			{
				if (center[i] < ent->absmin[i])
				{
					v[i] = ent->absmin[i] - center[i];
				}
				else if (center[i] > ent->absmax[i])
				{
					v[i] = center[i] - ent->absmax[i];
				}
				else
				{
					v[i] = 0;
				}
			}

			VectorSubtract(ent->absmax, ent->absmin, size);
			VectorMA(ent->absmin, 0.5, size, ent_org);

			//see if they're in front of me
			VectorSubtract(ent_org, center, dir);
			VectorNormalize(dir);

			dist = VectorLength(v);

			if (dist >= radius)
			{
				continue;
			}

			//in PVS?
			if (!ent->bmodel && !gi.inPVS(ent_org, self->client->renderInfo.eyePoint))
			{
				//must be in PVS
				continue;
			}

			if (ent != forwardEnt)
			{
				//don't need to trace against forwardEnt again
				//really should have a clear LOS to this thing...
				gi.trace(&tr, self->client->renderInfo.eyePoint, vec3_origin, vec3_origin, ent_org, self->s.number,
					MASK_FORCE_PUSH, static_cast<EG2_Collision>(0), 0);
				//was MASK_SHOT, but changed to match above trace and crosshair trace
				if (tr.fraction < 1.0f && tr.entityNum != ent->s.number)
				{
					//must have clear LOS
					continue;
				}
			}

			// ok, we are within the radius, add us to the incoming list
			push_list[ent_count] = ent;
			ent_count++;
		}
	}

	for (int x = 0; x < ent_count; x++)
	{
		if (push_list[x]->client)
		{
			//SIGH band-aid...
			if (push_list[x]->s.number >= MAX_CLIENTS
				&& self->s.number < MAX_CLIENTS)
			{
				if (push_list[x]->client->ps.forcePowersActive & 1 << FP_GRIP
					//&& push_list[x]->client->ps.forcePowerDebounce[FP_GRIP] < level.time
					&& push_list[x]->client->ps.forceGripEntityNum == self->s.number)
				{
					WP_ForcePowerStop(push_list[x], FP_GRIP);
				}
				if (push_list[x]->client->ps.forcePowersActive & 1 << FP_DRAIN
					//&& push_list[x]->client->ps.forcePowerDebounce[FP_DRAIN] < level.time
					&& push_list[x]->client->ps.forceDrainEntityNum == self->s.number)
				{
					WP_ForcePowerStop(push_list[x], FP_DRAIN);
				}
			}

			if (Rosh_BeingHealed(push_list[x]))
			{
				continue;
			}
			if (push_list[x]->client->NPC_class == CLASS_HAZARD_TROOPER
				&& push_list[x]->health > 0)
			{
				//living hazard troopers resist push/pull
				WP_ForceThrowHazardTrooper(self, push_list[x], qfalse);
				continue;
			}
			if (fake)
			{
				//always resist
				WP_ResistForcePush(push_list[x], self, qfalse);
				continue;
			}

			int powerLevel, powerUse;
			powerLevel = self->client->ps.forcePowerLevel[FP_REPULSE];
			powerUse = FP_REPULSE;

			int modPowerLevel = wp_absorb_conversion(push_list[x], push_list[x]->client->ps.forcePowerLevel[FP_ABSORB],
				self, powerUse, powerLevel,
				force_power_needed[self->client->ps.forcePowerLevel[powerUse]]);
			if (push_list[x]->client->NPC_class == CLASS_ASSASSIN_DROID ||
				push_list[x]->client->NPC_class == CLASS_HAZARD_TROOPER)
			{
				modPowerLevel = 0; // divides throw by 10
			}

			//First, if this is the player we're push/pulling, see if he can counter it
			if (modPowerLevel != -1
				&& in_front(self->currentOrigin, push_list[x]->client->renderInfo.eyePoint,
					push_list[x]->client->ps.viewangles, 0.3f))
			{
				//absorbed and I'm in front of them
				//counter it
				if (push_list[x]->client->ps.forcePowerLevel[FP_ABSORB] > FORCE_LEVEL_2)
				{
					//no reaction at all
				}
				else
				{
					WP_ResistForcePush(push_list[x], self, qfalse);
					push_list[x]->client->ps.saberMove = push_list[x]->client->ps.saberBounceMove = LS_READY;
					//don't finish whatever saber anim you may have been in
					push_list[x]->client->ps.saberBlocked = BLOCKED_NONE;
				}
				continue;
			}
			if (!push_list[x]->s.number)
			{
				//player
				if (ShouldPlayerResistForceThrow(push_list[x], self, qfalse))
				{
					WP_ResistForcePush(push_list[x], self, qfalse);
					push_list[x]->client->ps.saberMove = push_list[x]->client->ps.saberBounceMove = LS_READY;
					//don't finish whatever saber anim you may have been in
					push_list[x]->client->ps.saberBlocked = BLOCKED_NONE;
					continue;
				}
			}
			else if (push_list[x]->client && Jedi_WaitingAmbush(push_list[x]))
			{
				WP_ForceKnockdown(push_list[x], self, qfalse, qtrue, qfalse);
				ForceRepulseDamage(self, push_list[x], tr.endpos, damageLevel);
				continue;
			}

			G_KnockOffVehicle(push_list[x], self, qfalse);

			if (push_list[x]->client->ps.forceDrainEntityNum == self->s.number
				&& self->s.eFlags & EF_FORCE_DRAINED)
			{
				//stop them from draining me now, dammit!
				WP_ForcePowerStop(push_list[x], FP_DRAIN);
			}

			//okay, everyone else (or player who couldn't resist it)...
			if ((self->s.number == 0 && Q_irand(0, 2) || Q_irand(0, 2)) && push_list[x]->client && push_list[x]->health
				> 0 //a living client
				&& push_list[x]->client->ps.weapon == WP_SABER //Jedi
				&& push_list[x]->health > 0 //alive
				&& push_list[x]->client->ps.forceRageRecoveryTime < level.time //not recovering from rage
				&& (self->client->NPC_class != CLASS_DESANN && Q_stricmp("Yoda", self->NPC_type) || !Q_irand(0, 2))
				//only 30% chance of resisting a Desann push
				&& push_list[x]->client->ps.groundEntityNum != ENTITYNUM_NONE //on the ground
				&& in_front(self->currentOrigin, push_list[x]->currentOrigin, push_list[x]->client->ps.viewangles,
					0.3f) //I'm in front of him
				&& (push_list[x]->client->ps.powerups[PW_FORCE_PUSH] > level.time || //he's pushing too
					push_list[x]->s.number != 0 && push_list[x]->client->ps.weaponTime < level.time
					//not the player and not attacking (NPC jedi auto-defend against pushes)
					)
				)
			{
				//Jedi don't get pushed, they resist as long as they aren't already attacking and are on the ground
				if (push_list[x]->client->ps.saberLockTime > level.time)
				{
					//they're in a lock
					if (push_list[x]->client->ps.saberLockEnemy != self->s.number)
					{
						//they're not in a lock with me
						continue;
					}
					if (self->client->ps.forcePowerLevel[FP_REPULSE] < FORCE_LEVEL_3 ||
						push_list[x]->client->ps.forcePowerLevel[FP_PUSH] > FORCE_LEVEL_2)
					{
						//they're in a lock with me, but my push is too weak
						continue;
					}
					//we will knock them down
					self->painDebounceTime = 0;
					self->client->ps.weaponTime = 500;
					if (self->client->ps.forcePowersActive & 1 << FP_SPEED)
					{
						self->client->ps.weaponTime = floor(self->client->ps.weaponTime * g_timescale->value);
					}
				}
				int resistChance = Q_irand(0, 2);
				if (push_list[x]->s.number >= MAX_CLIENTS)
				{
					//NPC
					if (g_spskill->integer == 1)
					{
						//stupid tweak for graham
						resistChance = Q_irand(0, 3);
					}
				}
				if (modPowerLevel == -1
					&& self->client->ps.forcePowerLevel[FP_REPULSE] > FORCE_LEVEL_2
					&& !resistChance
					&& push_list[x]->client->ps.forcePowerLevel[FP_PUSH] < FORCE_LEVEL_3)
				{
					//a level 3 push can even knock down a jedi
					if (PM_InKnockDown(&push_list[x]->client->ps))
					{
						//can't knock them down again
						continue;
					}
					WP_ForceKnockdown(push_list[x], self, qfalse, qfalse, qtrue);
					ForceRepulseDamage(self, push_list[x], tr.endpos, damageLevel);
				}
				else
				{
					WP_ResistForcePush(push_list[x], self, qfalse);
				}
			}
			else
			{
				float knockback = 200;
				vec3_t pushDir;
				//UGH: FIXME: for enemy jedi, they should probably always do force pull 3, and not your weapon (if player?)!
				//shove them
				if (push_list[x]->NPC
					&& push_list[x]->NPC->jumpState == JS_JUMPING)
				{
					//don't interrupt a scripted jump
					//WP_ResistForcePush( push_list[x], self, qfalse );
					push_list[x]->forcePushTime = level.time + 600; // let the push effect last for 600 ms
					continue;
				}

				if (push_list[x]->s.number
					&& (push_list[x]->message || push_list[x]->flags & FL_NO_KNOCKBACK))
				{
					//an NPC who has a key
					//don't push me... FIXME: maybe can pull the key off me?
					WP_ForceKnockdown(push_list[x], self, qfalse, qfalse, qfalse);
					ForceRepulseDamage(self, push_list[x], tr.endpos, damageLevel);
					continue;
				}
				{
					VectorSubtract(push_list[x]->currentOrigin, self->currentOrigin, pushDir);
					knockback -= VectorNormalize(pushDir);
					if (knockback < 100)
					{
						knockback = 100;
					}
					//scale for push level
					if (self->client->ps.forcePowerLevel[FP_REPULSE] < FORCE_LEVEL_2)
					{
						//maybe just knock them down
						knockback /= 3;
					}
					else if (self->client->ps.forcePowerLevel[FP_REPULSE] > FORCE_LEVEL_2)
					{
						//super-hard push
						knockback *= 2;
					}
				}

				if (modPowerLevel != -1)
				{
					if (!modPowerLevel)
					{
						knockback /= 10.0f;
					}
					else if (modPowerLevel == 1)
					{
						knockback /= 6.0f;
					}
					else // if ( modPowerLevel == 2 )
					{
						knockback /= 2.0f;
					}
				}
				//actually push/pull the enemy
				g_throw(push_list[x], pushDir, knockback);
				//make it so they don't actually hurt me when pulled at me...
				push_list[x]->forcePuller = self->s.number;

				if (push_list[x]->client->ps.groundEntityNum != ENTITYNUM_NONE)
				{
					//if on the ground, make sure they get shoved up some
					if (push_list[x]->client->ps.velocity[2] < knockback)
					{
						push_list[x]->client->ps.velocity[2] = knockback;
					}
				}

				if (push_list[x]->health > 0)
				{
					//target is still alive
					if ((push_list[x]->s.number || cg.renderingThirdPerson && !cg.zoomMode) //NPC or 3rd person player
						&& (self->client->ps.forcePowerLevel[FP_REPULSE] < FORCE_LEVEL_2 && push_list[x]->client->ps.
							forcePowerLevel[FP_PUSH] < FORCE_LEVEL_1))
					{
						//NPC or third person player (without force push/pull skill), and force push/pull level is at 1
						WP_ForceKnockdown(push_list[x], self, qfalse, static_cast<qboolean>(knockback > 150), qfalse);
						ForceRepulseDamage(self, push_list[x], tr.endpos, damageLevel);
					}
					else if (!push_list[x]->s.number)
					{
						//player, have to force an anim on him
						WP_ForceKnockdown(push_list[x], self, qfalse, static_cast<qboolean>(knockback > 150), qfalse);
						ForceRepulseDamage(self, push_list[x], tr.endpos, damageLevel);
					}
					else
					{
						//NPC and force-push/pull at level 2 or higher
						WP_ForceKnockdown(push_list[x], self, qfalse, static_cast<qboolean>(knockback > 100), qfalse);
						ForceRepulseDamage(self, push_list[x], tr.endpos, damageLevel);
					}
				}
				push_list[x]->forcePushTime = level.time + 600; // let the push effect last for 600 ms
			}
		}
		else if (!fake)
		{
			//not a fake push/pull
			if (push_list[x]->s.weapon == WP_SABER && push_list[x]->contents & CONTENTS_LIGHTSABER)
			{
				//a thrown saber, just send it back
				/*
				 if ( pull )
				 {//steal it?
				 }
				 else */
				if (push_list[x]->owner && push_list[x]->owner->client && push_list[x]->owner->client->ps.SaberActive()
					&& push_list[x]->s.pos.trType == TR_LINEAR && push_list[x]->owner->client->ps.saberEntityState !=
					SES_RETURNING)
				{
					//it's on and being controlled
					//FIXME: prevent it from damaging me?
					if (self->s.number == 0 || Q_irand(0, 2))
					{
						//certain chance of throwing it aside and turning it off?
						//give it some velocity away from me
						if (Q_irand(0, 1))
						{
							VectorScale(right, -1, right);
						}

						if (self->s.number >= MAX_CLIENTS && !G_ControlledByPlayer(self))
						{
							g_reflect_missile_npc(self, push_list[x], right);
						}
						else
						{
							g_reflect_missile_auto(self, push_list[x], right);
						}
						WP_SaberDrop(push_list[x]->owner, push_list[x]);
					}
					else
					{
						WP_SaberDrop(push_list[x]->owner, push_list[x]);
					}
				}
			}
			else if (push_list[x]->s.eType == ET_MISSILE
				&& push_list[x]->s.pos.trType != TR_STATIONARY
				&& (push_list[x]->s.pos.trType != TR_INTERPOLATE || push_list[x]->s.weapon != WP_THERMAL))
				//rolling and stationary thermal detonators are dealt with below
			{
				vec3_t dir2Me;
				VectorSubtract(self->currentOrigin, push_list[x]->currentOrigin, dir2Me);
				float dot = DotProduct(push_list[x]->s.pos.trDelta, dir2Me);

				if (push_list[x]->s.eFlags & EF_MISSILE_STICK)
				{
					//caught a sticky in-air
					push_list[x]->s.eType = ET_MISSILE;
					push_list[x]->s.eFlags &= ~EF_MISSILE_STICK;
					push_list[x]->s.eFlags |= EF_BOUNCE_HALF;
					push_list[x]->splashDamage /= 3;
					push_list[x]->splashRadius /= 3;
					push_list[x]->e_ThinkFunc = thinkF_WP_Explode;
					push_list[x]->nextthink = level.time + Q_irand(500, 3000);
				}
				if (dot >= 0)
				{
					//it's heading towards me
					if (self->s.number >= MAX_CLIENTS && !G_ControlledByPlayer(self))
					{
						g_reflect_missile_npc(self, push_list[x], forward);
					}
					else
					{
						g_reflect_missile_auto(self, push_list[x], forward);
					}
				}
				else
				{
					VectorScale(push_list[x]->s.pos.trDelta, 1.25f, push_list[x]->s.pos.trDelta);
				}

				if (push_list[x]->s.eType == ET_MISSILE
					&& push_list[x]->s.weapon == WP_ROCKET_LAUNCHER
					&& push_list[x]->damage < 60)
				{
					//pushing away a rocket raises it's damage to the max for NPCs
					push_list[x]->damage = 60;
				}
			}
			else if (push_list[x]->svFlags & SVF_GLASS_BRUSH)
			{
				vec3_t end;
				//break the glass
				trace_t trace;
				vec3_t pushDir;
				float damage = 800;

				AngleVectors(self->client->ps.viewangles, forward, nullptr, nullptr);
				VectorNormalize(forward);
				VectorMA(self->client->renderInfo.eyePoint, radius, forward, end);
				gi.trace(&trace, self->client->renderInfo.eyePoint, vec3_origin, vec3_origin, end, self->s.number,
					MASK_SHOT, static_cast<EG2_Collision>(0), 0);
				if (trace.entityNum != push_list[x]->s.number || trace.fraction == 1.0 || trace.allsolid || trace.
					startsolid)
				{
					//must be pointing right at it
					continue;
				}

				VectorSubtract(trace.endpos, self->client->renderInfo.eyePoint, pushDir);

				damage -= VectorNormalize(pushDir);
				if (damage < 200)
				{
					damage = 200;
				}
				VectorScale(pushDir, damage, pushDir);

				G_Damage(push_list[x], self, self, pushDir, trace.endpos, damage, 0, MOD_UNKNOWN);
			}
			else if (push_list[x]->s.eType == ET_MISSILE /*thermal resting on ground*/
				|| push_list[x]->s.eType == ET_ITEM
				|| push_list[x]->e_ThinkFunc == thinkF_G_RunObject || Q_stricmp("limb", push_list[x]->classname) == 0)
			{
				//general object, toss it
				vec3_t pushDir, kvel;
				float knockback = 200;
				float mass = 200;

				if (self->enemy //I have an enemy
					&& push_list[x]->s.eType != ET_ITEM //not an item
					&& self->client->ps.forcePowerLevel[FP_REPULSE] > FORCE_LEVEL_2 //have push 3 or greater
					&& in_front(push_list[x]->currentOrigin, self->currentOrigin, self->currentAngles, 0.25f)
					//object is generally in front of me
					&& in_front(self->enemy->currentOrigin, self->currentOrigin, self->currentAngles, 0.75f)
					//enemy is pretty much right in front of me
					&& in_front(push_list[x]->currentOrigin, self->enemy->currentOrigin, self->enemy->currentAngles,
						0.25f) //object is generally in front of enemy
					//FIXME: check dist to enemy and clear LOS to enemy and clear Path between object and enemy?
					&& (self->NPC && Q_irand(0, RANK_CAPTAIN) < self->NPC->rank //NPC with enough skill
						|| self->s.number < MAX_CLIENTS)
					)
				{
					//if I have an auto-enemy & he's in front of me, push it toward him!
					/*
					 if ( targetedObjectMassTotal + push_list[x]->mass > TARGETED_OBJECT_PUSH_MASS_MAX )
					 {//already pushed too many things
						//FIXME: pick closest?
						continue;
					 }
					 targetedObjectMassTotal += push_list[x]->mass;
					 */
					VectorSubtract(self->enemy->currentOrigin, push_list[x]->currentOrigin, pushDir);
				}
				else
				{
					VectorSubtract(push_list[x]->currentOrigin, self->currentOrigin, pushDir);
				}
				knockback -= VectorNormalize(pushDir);
				if (knockback < 100)
				{
					knockback = 100;
				}

				//FIXME: if pull a FL_FORCE_PULLABLE_ONLY, clear the flag, assuming it's no longer in solid?  or check?
				VectorCopy(push_list[x]->currentOrigin, push_list[x]->s.pos.trBase);
				push_list[x]->s.pos.trTime = level.time; // move a bit on the very first frame
				if (push_list[x]->s.pos.trType != TR_INTERPOLATE)
				{
					//don't do this to rolling missiles
					push_list[x]->s.pos.trType = TR_GRAVITY;
				}

				if (push_list[x]->e_ThinkFunc == thinkF_G_RunObject && push_list[x]->physicsBounce)
				{
					//it's a pushable misc_model_breakable, use it's mass instead of our one-size-fits-all mass
					mass = push_list[x]->physicsBounce; //same as push_list[x]->mass, right?
				}
				if (mass < 50)
				{
					//???
					mass = 50;
				}
				if (g_gravity->value > 0)
				{
					VectorScale(pushDir, g_knockback->value * knockback / mass * 0.8, kvel);
					kvel[2] = pushDir[2] * g_knockback->value * knockback / mass * 1.5;
				}
				else
				{
					VectorScale(pushDir, g_knockback->value * knockback / mass, kvel);
				}
				VectorAdd(push_list[x]->s.pos.trDelta, kvel, push_list[x]->s.pos.trDelta);
				if (g_gravity->value > 0)
				{
					if (push_list[x]->s.pos.trDelta[2] < knockback)
					{
						push_list[x]->s.pos.trDelta[2] = knockback;
					}
				}
				//no trDuration?
				if (push_list[x]->e_ThinkFunc != thinkF_G_RunObject)
				{
					//objects spin themselves?
					//spin it
					//FIXME: messing with roll ruins the rotational center???
					push_list[x]->s.apos.trTime = level.time;
					push_list[x]->s.apos.trType = TR_LINEAR;
					VectorClear(push_list[x]->s.apos.trDelta);
					push_list[x]->s.apos.trDelta[1] = Q_irand(-800, 800);
				}

				if (Q_stricmp("limb", push_list[x]->classname) == 0)
				{
					//make sure it runs it's physics
					push_list[x]->e_ThinkFunc = thinkF_LimbThink;
					push_list[x]->nextthink = level.time + FRAMETIME;
				}
				push_list[x]->forcePushTime = level.time + 600; // let the push effect last for 600 ms
				push_list[x]->forcePuller = self->s.number; //remember this regardless
				if (push_list[x]->item && push_list[x]->item->giTag == INV_SECURITY_KEY)
				{
					AddSightEvent(player, push_list[x]->currentOrigin, 128, AEL_DISCOVERED);
					//security keys are more important
				}
				else
				{
					AddSightEvent(player, push_list[x]->currentOrigin, 128, AEL_SUSPICIOUS);
					//hmm... or should this always be discovered?
				}
			}
			else if (push_list[x]->s.weapon == WP_TURRET
				&& !Q_stricmp("PAS", push_list[x]->classname)
				&& push_list[x]->s.apos.trType == TR_STATIONARY)
			{
				//a portable turret
				WP_KnockdownTurret(self, push_list[x]);
			}
		}
	}

	WP_ForcePowerDrain(self, FP_REPULSE, cost);

	if (self->NPC)
	{
		//NPCs can push more often
		//FIXME: vary by rank and game skill?
		self->client->ps.forcePowerDebounce[FP_REPULSE] = level.time + 200;
	}
	else
	{
		self->client->ps.forcePowerDebounce[FP_REPULSE] = level.time + self->client->ps.torsoAnimTimer + 500;
	}
}

int IsPressingDashButton(const gentity_t* self)
{
	if (PM_RunningAnim(self->client->ps.legsAnim)
		&& !PM_SaberInAttack(self->client->ps.saberMove)
		&& !(self->client->buttons & BUTTON_KICK)
		&& self->client->buttons & BUTTON_DASH)
	{
		return qtrue;
	}
	return qfalse;
}

int IsPressingKickButton(const gentity_t* self)
{
	if (!(self->client->buttons & BUTTON_DASH)
		&& (self->client->buttons & BUTTON_KICK && self->client->ps.pm_flags & PMF_KICK_HELD))
	{
		return qtrue;
	}
	return qfalse;
}

void ForceSpeed(gentity_t* self, int duration)
{
	if (self->health <= 0)
	{
		return;
	}

	if (BG_InKnockDown(self->client->ps.legsAnim) || PM_InKnockDown(&self->client->ps))
	{
		return;
	}

	if (PM_InLedgeMove(self->client->ps.legsAnim))
	{
		return;
	}

	if (self->client->ps.forceAllowDeactivateTime < level.time &&
		self->client->ps.forcePowersActive & 1 << FP_SPEED)
	{
		WP_ForcePowerStop(self, FP_SPEED);
		return;
	}

	if (!WP_ForcePowerUsable(self, FP_SPEED, 0))
	{
		return;
	}

	if (self->client->ps.saberLockTime > level.time)
	{
		return;
	}

	if (self->client->ps.forcePowersActive & 1 << FP_SPEED)
	{
		//it's already turned on.  turn it off.
		WP_ForcePowerStop(self, FP_SPEED);
		return;
	}

	WP_DebounceForceDeactivateTime(self);
	WP_ForcePowerStart(self, FP_SPEED, 0);

	if (duration)
	{
		self->client->ps.forcePowerDuration[FP_SPEED] = level.time + duration;
	}

	if (self->client->ps.forcePowerLevel[FP_SPEED] < FORCE_LEVEL_3)
	{
		//short burst
		G_Sound(self, G_SoundIndex("sound/weapons/force/speed.mp3"));
	}
	else
	{
		//holding it
		G_Sound(self, G_SoundIndex("sound/weapons/force/speedloop.wav"));
	}
	CG_PlayEffectBolted("misc/breath.efx", self->playerModel, self->headBolt, self->s.number, self->currentOrigin);
}

void ForceDashAnim(gentity_t* self)
{
	constexpr int setAnimOverride = SETANIM_AFLAG_PACE;

	if (self->client->pers.cmd.rightmove > 0)
	{
		NPC_SetAnim(self, SETANIM_BOTH, BOTH_HOP_R, setAnimOverride);
	}
	else if (self->client->pers.cmd.rightmove < 0)
	{
		NPC_SetAnim(self, SETANIM_BOTH, BOTH_HOP_L, setAnimOverride);
	}
	else if (self->client->pers.cmd.forwardmove < 0)
	{
		NPC_SetAnim(self, SETANIM_BOTH, BOTH_HOP_B, setAnimOverride);
	}
	else
	{
		NPC_SetAnim(self, SETANIM_BOTH, BOTH_HOP_F, setAnimOverride);
	}
}

void ForceDashAnimDash(gentity_t* self)
{
	constexpr int setAnimOverride = SETANIM_AFLAG_PACE;

	if (self->client->pers.cmd.rightmove > 0)
	{
		NPC_SetAnim(self, SETANIM_BOTH, BOTH_DASH_R, setAnimOverride);
	}
	else if (self->client->pers.cmd.rightmove < 0)
	{
		NPC_SetAnim(self, SETANIM_BOTH, BOTH_DASH_L, setAnimOverride);
	}
	else if (self->client->pers.cmd.forwardmove < 0)
	{
		NPC_SetAnim(self, SETANIM_BOTH, BOTH_DASH_B, setAnimOverride);
	}
	else
	{
		NPC_SetAnim(self, SETANIM_BOTH, BOTH_DASH_F, setAnimOverride);
	}
}

void ForceSpeedDash(gentity_t* self)
{
	if (self->health <= 0)
	{
		return;
	}

	if (PM_InLedgeMove(self->client->ps.legsAnim))
	{
		return;
	}

	if (PM_SaberInAttack(self->client->ps.saberMove))
	{
		return;
	}

	if (PM_KickMove(self->client->ps.saberMove))
	{
		return;
	}

	if (self->client->ps.forcePowerDebounce[FP_SPEED] > level.time)
	{
		//stops it while using it and also after using it, up to 3 second delay
		return;
	}

	if (self->client->ps.forceSpeedRecoveryTime >= level.time)
	{
		return;
	}

	if (BG_InKnockDown(self->client->ps.legsAnim) || PM_InKnockDown(&self->client->ps))
	{
		return;
	}

	if (self->client->NPC_class == CLASS_DROIDEKA ||
		self->client->NPC_class == CLASS_VEHICLE ||
		self->client->NPC_class == CLASS_SBD ||
		self->client->NPC_class == CLASS_OBJECT ||
		self->client->NPC_class == CLASS_ASSASSIN_DROID)
	{
		return;
	}

	if (self->client->ps.forceAllowDeactivateTime < level.time && self->client->ps.forcePowersActive & 1 << FP_SPEED)
	{
		WP_ForcePowerStop(self, FP_SPEED);
		return;
	}

	if (self->client->ps.forcePowersActive & 1 << FP_SPEED) //If using speed at same time just in case
	{
		if (PM_RunningAnim(self->client->ps.legsAnim))
		{
			ForceDashAnim(self);
		}
		else
		{
			return;
		}
	}

	if (self->client->ps.saberLockTime > level.time)
	{
		return;
	}

	if (self->client->ps.groundEntityNum != ENTITYNUM_NONE)
	{
		vec3_t dir;
		if (self->client->ps.forcePowerLevel[FP_SPEED] > FORCE_LEVEL_2)
		{
			AngleVectors(self->client->ps.viewangles, dir, nullptr, nullptr);
			self->client->ps.velocity[0] = self->client->ps.velocity[0] * 7;
			self->client->ps.velocity[1] = self->client->ps.velocity[1] * 7;

			ForceDashAnimDash(self);
		}
		else if (self->client->ps.forcePowerLevel[FP_SPEED] > FORCE_LEVEL_1)
		{
			AngleVectors(self->client->ps.viewangles, dir, nullptr, nullptr);
			self->client->ps.velocity[0] = self->client->ps.velocity[0] * 5;
			self->client->ps.velocity[1] = self->client->ps.velocity[1] * 5;

			ForceDashAnimDash(self);
		}
		else
		{
			AngleVectors(self->client->ps.viewangles, dir, nullptr, nullptr);
			self->client->ps.velocity[0] = self->client->ps.velocity[0] * 3;
			self->client->ps.velocity[1] = self->client->ps.velocity[1] * 3;

			ForceDashAnimDash(self);
		}
	}
	else if (self->client->ps.groundEntityNum == ENTITYNUM_NONE)
	{
		NPC_SetAnim(self, SETANIM_BOTH, BOTH_FORCEINAIR1, SETANIM_AFLAG_PACE);
	}

	G_Sound(self, G_SoundIndex("sound/weapons/force/dash.mp3"));
	CG_PlayEffectBolted("misc/breath.efx", self->playerModel, self->headBolt, self->s.number, self->currentOrigin);
}

void player_Burn(const gentity_t* self);

void player_StopBurn(const gentity_t* self)
{
	if (self && self->client)
	{
		if (self->client->ps.PlayerEffectFlags & 1 << PEF_BURNING)
		{
			//Unburn
			self->client->ps.PlayerEffectFlags &= ~(1 << PEF_BURNING);
		}
		else
		{
			//Unburn
			self->client->ps.PlayerEffectFlags &= ~(1 << PEF_BURNING);
		}
	}
}

void Player_CheckBurn(const gentity_t* self)
{
	if (self && self->client)
	{
		if (self->health <= 0 || self->painDebounceTime < level.time)
		{
			player_StopBurn(self);
		}
		else if (PM_InRoll(&self->client->ps))
		{
			player_StopBurn(self);
		}
		else if (TIMER_Done(self, "BurnDebounce"))
		{
			player_StopBurn(self);
		}
		else if (self->client->ps.saberEventFlags & SEF_INWATER)
		{
			player_StopBurn(self);
		}
		else
		{
			player_StopBurn(self);
		}
	}
}

void player_Burn(const gentity_t* self)
{
	if ((self->s.number < MAX_CLIENTS || G_ControlledByPlayer(self)) && self && self->client)
	{
		if (!(self->client->ps.PlayerEffectFlags & 1 << PEF_BURNING))
		{
			self->client->ps.PlayerEffectFlags |= 1 << PEF_BURNING;
		}
		/*else
		{
			player_StopBurn(self);
		}*/
	}

	TIMER_Set(self, "BurnDebounce", 1000);
}

void WP_StartForceHealEffects(const gentity_t* self)
{
	if (self->ghoul2.size())
	{
		if (self->chestBolt != -1)
		{
			G_PlayEffect(G_EffectIndex("force/heal2"), self->playerModel, self->chestBolt, self->s.number,
				self->currentOrigin, 3000, qtrue);
		}
	}
}

void WP_StopForceHealEffects(const gentity_t* self)
{
	if (self->ghoul2.size())
	{
		if (self->chestBolt != -1)
		{
			G_StopEffect(G_EffectIndex("force/heal2"), self->playerModel, self->chestBolt, self->s.number);
		}
	}
}

int FP_MaxForceHeal(const gentity_t* self)
{
	if (self->s.number >= MAX_CLIENTS)
	{
		return MAX_FORCE_HEAL_MEDIUM;
	}
	switch (g_spskill->integer)
	{
	case 0: //easy
		return MAX_FORCE_HEAL_EASY;
	case 1: //medium
		return MAX_FORCE_HEAL_MEDIUM;
	case 2: //hard
	default:
		return MAX_FORCE_HEAL_HARD;
	}
}

int FP_ForceHealInterval(const gentity_t* self)
{
	if (self->s.number < MAX_CLIENTS || G_ControlledByPlayer(self))
	{
		return self->client->ps.forcePowerLevel[FP_HEAL] > FORCE_LEVEL_2 ? 50 : FORCE_HEAL_INTERVAL_PLAYER;
		//X2 as fast for the player
	}
	return self->client->ps.forcePowerLevel[FP_HEAL] > FORCE_LEVEL_2 ? 50 : FORCE_HEAL_INTERVAL;
}

void ForceHeal(gentity_t* self)
{
	if (self->health <= 0 || self->client->ps.stats[STAT_MAX_HEALTH] <= self->health)
	{
		return;
	}

	if (!WP_ForcePowerUsable(self, FP_HEAL, 20))
	{
		//must have enough force power for at least 5 points of health
		return;
	}

	if (PM_InLedgeMove(self->client->ps.legsAnim))
	{
		return;
	}

	if (self->painDebounceTime > level.time || self->client->ps.weaponTime && self->client->ps.weapon != WP_NONE)
	{
		//can't initiate a heal while taking pain or attacking
		return;
	}

	if (self->client->ps.saberLockTime > level.time)
	{
		//FIXME: can this be a way to break out?
		return;
	}
	//start health going up
	WP_ForcePowerStart(self, FP_HEAL, 0);

	if (self->client->ps.forcePowerLevel[FP_HEAL] < FORCE_LEVEL_2)
	{
		//must meditate
		NPC_SetAnim(self, SETANIM_BOTH, BOTH_FORCEHEAL_START, SETANIM_FLAG_OVERRIDE | SETANIM_FLAG_HOLD);
		self->client->ps.saberMove = self->client->ps.saberBounceMove = LS_READY;
		//don't finish whatever saber anim you may have been in
		self->client->ps.saberBlocked = BLOCKED_NONE;
		self->client->ps.torsoAnimTimer = self->client->ps.legsAnimTimer = FP_ForceHealInterval(self) *
			FP_MaxForceHeal(self) + 2000; //???
		WP_DeactivateSaber(self); //turn off saber when meditating
	}
	else
	{
		//just a quick gesture
		NPC_SetAnim(self, SETANIM_TORSO, BOTH_FORCEHEAL_QUICK, SETANIM_FLAG_OVERRIDE | SETANIM_FLAG_HOLD);
		self->client->ps.saberMove = self->client->ps.saberBounceMove = LS_READY;
		//don't finish whatever saber anim you may have been in
		self->client->ps.saberBlocked = BLOCKED_NONE;
	}

	if (self->client->ps.forcePowerLevel[FP_HEAL] < FORCE_LEVEL_2)
	{
		G_SoundOnEnt(self, CHAN_ITEM, "sound/weapons/force/heal.mp3");
	}
	else
	{
		G_SoundOnEnt(self, CHAN_ITEM, "sound/player/injecthealth.mp3");
	}
}

extern void NPC_PlayConfusionSound(gentity_t* self);
extern void NPC_Jedi_PlayConfusionSound(const gentity_t* self);

qboolean WP_CheckBreakControl(gentity_t* self)
{
	if (!self)
	{
		return qfalse;
	}
	if (!self->s.number)
	{
		//player
		if (self->client && self->client->ps.forcePowerLevel[FP_TELEPATHY] > FORCE_LEVEL_3)
		{
			//control-level
			if (self->client->ps.viewEntity > 0 && self->client->ps.viewEntity < ENTITYNUM_WORLD)
			{
				//we are in a viewentity
				const gentity_t* controlled = &g_entities[self->client->ps.viewEntity];
				if (controlled->NPC && controlled->NPC->controlledTime > level.time)
				{
					//it is an NPC we controlled
					//clear it and return
					G_ClearViewEntity(self);
					return qtrue;
				}
			}
		}
	}
	else
	{
		//NPC
		if (self->NPC && self->NPC->controlledTime > level.time)
		{
			//being controlled
			gentity_t* controller = &g_entities[0];
			if (controller->client && controller->client->ps.viewEntity == self->s.number)
			{
				//we are being controlled by player
				if (controller->client->ps.forcePowerLevel[FP_TELEPATHY] > FORCE_LEVEL_3)
				{
					//control-level mind trick
					//clear the control and return
					G_ClearViewEntity(controller);
					return qtrue;
				}
			}
		}
	}
	return qfalse;
}

extern bool Pilot_AnyVehiclesRegistered();

void ForceTelepathy(gentity_t* self)
{
	trace_t tr;
	vec3_t end, forward;
	qboolean targetLive = qfalse;

	if (WP_CheckBreakControl(self))
	{
		return;
	}
	if (self->health <= 0)
	{
		return;
	}
	//FIXME: if mind trick 3 and aiming at an enemy need more force power
	if (!WP_ForcePowerUsable(self, FP_TELEPATHY, 0))
	{
		return;
	}

	if (PM_InLedgeMove(self->client->ps.legsAnim))
	{
		return;
	}

	if (self->client->ps.weaponTime >= 800)
	{
		//just did one!
		return;
	}
	if (self->client->ps.saberLockTime > level.time)
	{
		//FIXME: can this be a way to break out?
		return;
	}

	AngleVectors(self->client->ps.viewangles, forward, nullptr, nullptr);
	VectorNormalize(forward);
	VectorMA(self->client->renderInfo.eyePoint, 2048, forward, end);

	//Cause a distraction if enemy is not fighting
	gi.trace(&tr, self->client->renderInfo.eyePoint, vec3_origin, vec3_origin, end, self->s.number,
		MASK_OPAQUE | CONTENTS_BODY, static_cast<EG2_Collision>(0), 0);
	if (tr.entityNum == ENTITYNUM_NONE || tr.fraction == 1.0 || tr.allsolid || tr.startsolid)
	{
		return;
	}

	gentity_t* traceEnt = &g_entities[tr.entityNum];

	if (traceEnt->NPC && traceEnt->NPC->scriptFlags & SCF_NO_FORCE)
	{
		return;
	}

	if (traceEnt && traceEnt->client)
	{
		switch (traceEnt->client->NPC_class)
		{
		case CLASS_GALAKMECH: //cant grip him, he's in armor
			G_AddVoiceEvent(traceEnt, Q_irand(EV_PUSHED1, EV_PUSHED3), Q_irand(3000, 5000));
			return;
			//case CLASS_ATST://much too big to grip!
			//no droids either
		case CLASS_PROBE:
		case CLASS_GONK:
		case CLASS_R2D2:
		case CLASS_R5D2:
		case CLASS_MARK1:
		case CLASS_MARK2:
		case CLASS_MOUSE:
		case CLASS_SEEKER:
		case CLASS_REMOTE:
		case CLASS_PROTOCOL:
		case CLASS_ASSASSIN_DROID:
		case CLASS_SABER_DROID:
		case CLASS_DROIDEKA:
		case CLASS_SBD:
		case CLASS_BOBAFETT:
			break;
		case CLASS_RANCOR:
			if (!(traceEnt->spawnflags & 1))
			{
				targetLive = qtrue;
			}
			break;
		default:
			targetLive = qtrue;
			break;
		}
	}
	if (targetLive
		&& traceEnt->NPC
		&& traceEnt->health > 0
		&& traceEnt->NPC->darkCharmedTime < level.time
		&& traceEnt->NPC->insanityTime < level.time)
	{
		//hit an organic non-player
		if (G_ActivateBehavior(traceEnt, BSET_MINDTRICK))
		{
			//activated a script on him
			//FIXME: do the visual sparkles effect on their heads, still?
			WP_ForcePowerStart(self, FP_TELEPATHY, 0);
		}
		else if (traceEnt->client->playerTeam != self->client->playerTeam)
		{
			//an enemy
			int override = 0;
			if (traceEnt->NPC->scriptFlags & SCF_NO_MIND_TRICK)
			{
				if (traceEnt->client->NPC_class == CLASS_GALAKMECH)
				{
					G_AddVoiceEvent(traceEnt, Q_irand(EV_CONFUSE1, EV_CONFUSE3), Q_irand(3000, 5000));
				}
			}
			else if (self->client->ps.forcePowerLevel[FP_TELEPATHY] > FORCE_LEVEL_2)
			{
				//control them, even jedi
				G_SetViewEntity(self, traceEnt);
				traceEnt->NPC->controlledTime = level.time + 30000;
			}
			else if (traceEnt->s.weapon != WP_SABER && traceEnt->client->NPC_class != CLASS_REBORN)
			{
				//haha!  Jedi aren't easily confused!
				if (self->client->ps.forcePowerLevel[FP_TELEPATHY] > FORCE_LEVEL_1
					&& traceEnt->s.weapon != WP_NONE
					//don't charm people who aren't capable of fighting... like ugnaughts and droids, just confuse them
					&& traceEnt->client->NPC_class != CLASS_TUSKEN //don't charm them, just confuse them
					&& traceEnt->client->NPC_class != CLASS_NOGHRI //don't charm them, just confuse them
					&& !Pilot_AnyVehiclesRegistered() //also, don't charm guys when bikes are near
					)
				{
					//turn them to our side
					//if mind trick 3 and aiming at an enemy need more force power
					override = 50;
					if (self->client->ps.forcePower < 50)
					{
						return;
					}
					if (traceEnt->enemy)
					{
						G_ClearEnemy(traceEnt);
					}
					if (traceEnt->NPC)
					{
						traceEnt->client->leader = self;
					}
					const team_t saveTeam = traceEnt->client->enemyTeam;
					traceEnt->client->enemyTeam = traceEnt->client->playerTeam;
					traceEnt->client->playerTeam = saveTeam;
					//FIXME: need a *charmed* timer on this...?  Or do TEAM_PLAYERS assume that "confusion" means they should switch to team_enemy when done?
					traceEnt->NPC->charmedTime = level.time + mindTrickTime[self->client->ps.forcePowerLevel[
						FP_TELEPATHY]];

					NPC_SetAnim(traceEnt, SETANIM_TORSO, BOTH_SONICPAIN_HOLD,
						SETANIM_FLAG_OVERRIDE | SETANIM_FLAG_RESTART | SETANIM_FLAG_HOLD);

					if (traceEnt->ghoul2.size() && traceEnt->headBolt != -1)
					{
						//FIXME: what if already playing effect?
						G_PlayEffect(G_EffectIndex("force/confusion"), traceEnt->playerModel, traceEnt->headBolt,
							traceEnt->s.number, traceEnt->currentOrigin,
							mindTrickTime[self->client->ps.forcePowerLevel[FP_TELEPATHY]], qtrue);
					}
				}
				else
				{
					//just confuse them
					//somehow confuse them?  Set don't fire to true for a while?  Drop their aggression?  Maybe just take their enemy away and don't let them pick one up for a while unless shot?
					traceEnt->NPC->confusionTime = level.time + mindTrickTime[self->client->ps.forcePowerLevel[
						FP_TELEPATHY]]; //confused for about 10 seconds

					NPC_SetAnim(traceEnt, SETANIM_TORSO, BOTH_SONICPAIN_HOLD,
						SETANIM_FLAG_OVERRIDE | SETANIM_FLAG_RESTART | SETANIM_FLAG_HOLD);

					if (traceEnt->ghoul2.size() && traceEnt->headBolt != -1)
					{
						//FIXME: what if already playing effect?
						G_PlayEffect(G_EffectIndex("force/confusion"), traceEnt->playerModel, traceEnt->headBolt,
							traceEnt->s.number, traceEnt->currentOrigin,
							mindTrickTime[self->client->ps.forcePowerLevel[FP_TELEPATHY]], qtrue);
					}
					NPC_PlayConfusionSound(traceEnt);
					if (traceEnt->enemy)
					{
						G_ClearEnemy(traceEnt);
					}
				}
			}
			else
			{
				NPC_Jedi_PlayConfusionSound(traceEnt);
			}
			WP_ForcePowerStart(self, FP_TELEPATHY, override);
		}
		else if (traceEnt->client->playerTeam == self->client->playerTeam)
		{
			//an ally
			if (traceEnt->client->ps.pm_type < PM_DEAD && traceEnt->NPC != nullptr && !(traceEnt->NPC->scriptFlags &
				SCF_NO_RESPONSE))
			{
				NPC_UseResponse(traceEnt, self, qfalse);
				WP_ForcePowerStart(self, FP_TELEPATHY, 1);
			}
		}
		vec3_t eyeDir;
		AngleVectors(traceEnt->client->renderInfo.eyeAngles, eyeDir, nullptr, nullptr);
		VectorNormalize(eyeDir);
		G_PlayEffect("force/force_touch", traceEnt->client->renderInfo.eyePoint, eyeDir);

		//make sure this plays and that you cannot press fire for about 1 second after this
		NPC_SetAnim(self, SETANIM_TORSO, BOTH_MINDTRICK1,
			SETANIM_FLAG_OVERRIDE | SETANIM_FLAG_RESTART | SETANIM_FLAG_HOLD);
	}
	else
	{
		if (self->client->ps.forcePowerLevel[FP_TELEPATHY] > FORCE_LEVEL_1 && tr.fraction * 2048 > 64)
		{
			//don't create a diversion less than 64 from you of if at power level 1
			//use distraction anim instead
			G_PlayEffect(G_EffectIndex("force/force_touch"), tr.endpos, tr.plane.normal);
			AddSoundEvent(self, tr.endpos, 512, AEL_SUSPICIOUS, qtrue, qtrue);
			AddSightEvent(self, tr.endpos, 512, AEL_SUSPICIOUS, 50);
			WP_ForcePowerStart(self, FP_TELEPATHY, 0);
		}
		NPC_SetAnim(self, SETANIM_TORSO, BOTH_MINDTRICK2,
			SETANIM_FLAG_OVERRIDE | SETANIM_FLAG_RESTART | SETANIM_FLAG_HOLD);
	}
	self->client->ps.saberMove = self->client->ps.saberBounceMove = LS_READY;
	//don't finish whatever saber anim you may have been in
	self->client->ps.saberBlocked = BLOCKED_NONE;
	self->client->ps.weaponTime = 1000;
	if (self->client->ps.forcePowersActive & 1 << FP_SPEED)
	{
		self->client->ps.weaponTime = floor(self->client->ps.weaponTime * g_timescale->value);
	}
}

void ForceGripWide(gentity_t* self, gentity_t* traceEnt, vec3_t dir, float dist, float dot, vec3_t impactPoint)
{
#ifdef JK2_RAGDOLL_GRIPNOHEALTH
	if (!traceEnt || traceEnt == self || traceEnt->bmodel || traceEnt->NPC && traceEnt->NPC->scriptFlags & SCF_NO_FORCE)
	{
		return;
	}
#else
	if (!traceEnt || traceEnt == self || traceEnt->bmodel || (traceEnt->health <= 0 && traceEnt->takedamage) || (traceEnt->NPC && traceEnt->NPC->scriptFlags & SCF_NO_FORCE))
	{
		return;
		traceEnt->client->ps.powerups[PW_MEDITATE] = level.time + traceEnt->client->ps.torsoAnimTimer + 5000;
		NPC_SetAnim(traceEnt, SETANIM_TORSO, BOTH_WIND, SETANIM_AFLAG_PACE);
	}
#endif

	if (traceEnt->m_pVehicle != nullptr)
	{
		//is it a vehicle
		//grab pilot if there is one
		if (traceEnt->m_pVehicle->m_pPilot != nullptr
			&& traceEnt->m_pVehicle->m_pPilot->client != nullptr)
		{
			//grip the pilot
			traceEnt = traceEnt->m_pVehicle->m_pPilot;
		}
		else
		{
			//can't grip a vehicle
			return;
		}
	}
	if (traceEnt->client)
	{
		if (traceEnt->client->ps.forceJumpZStart)
		{
			//can't catch them in mid force jump - FIXME: maybe base it on velocity?
			return;
		}
		if (traceEnt->client->ps.pullAttackTime > level.time)
		{
			//can't grip someone who is being pull-attacked or is pull-attacking
			return;
		}
		if (!Q_stricmp("Yoda", traceEnt->NPC_type))
		{
			Jedi_PlayDeflectSound(traceEnt);
			ForceThrow(traceEnt, qfalse);
			return;
		}
		if (!Q_stricmp("T_Yoda", traceEnt->NPC_type))
		{
			Jedi_PlayDeflectSound(traceEnt);
			ForceThrow(traceEnt, qfalse);
			return;
		}
		if (!Q_stricmp("T_Palpatine_sith", traceEnt->NPC_type))
		{
			Jedi_PlayDeflectSound(traceEnt);
			ForceThrow(traceEnt, qfalse);
			return;
		}

		if (G_IsRidingVehicle(traceEnt)
			&& traceEnt->s.eFlags & EF_NODRAW)
		{
			//riding *inside* vehicle
			return;
		}

		switch (traceEnt->client->NPC_class)
		{
		case CLASS_GALAKMECH: //cant grip him, he's in armor
			G_AddVoiceEvent(traceEnt, Q_irand(EV_PUSHED1, EV_PUSHED3), Q_irand(3000, 5000));
			return;
		case CLASS_HAZARD_TROOPER: //cant grip him, he's in armor
			return;
		case CLASS_ATST: //much too big to grip!
		case CLASS_RANCOR: //much too big to grip!
		case CLASS_WAMPA: //much too big to grip!
		case CLASS_SAND_CREATURE: //much too big to grip!
			return;
			//no droids either...?
		case CLASS_GONK:
		case CLASS_R2D2:
		case CLASS_R5D2:
		case CLASS_MARK1:
		case CLASS_MARK2:
		case CLASS_MOUSE: //?
		case CLASS_PROTOCOL:
			//*sigh*... in JK3, you'll be able to grab and move *anything*...
			return;
			//not even combat droids?  (No animation for being gripped...)
		case CLASS_SABER_DROID:
		case CLASS_ASSASSIN_DROID:
		case CLASS_SBD:
		case CLASS_DROIDEKA:
			//*sigh*... in JK3, you'll be able to grab and move *anything*...
			return;
		case CLASS_PROBE:
		case CLASS_SEEKER:
		case CLASS_REMOTE:
		case CLASS_SENTRY:
		case CLASS_INTERROGATOR:
			//*sigh*... in JK3, you'll be able to grab and move *anything*...
			return;
		case CLASS_SITHLORD:
		case CLASS_DESANN: //Desann cannot be gripped, he just pushes you back instantly
		case CLASS_VADER:
		case CLASS_KYLE:
		case CLASS_TAVION:
		case CLASS_LUKE:
		case CLASS_YODA:
			Jedi_PlayDeflectSound(traceEnt);
			ForceThrow(traceEnt, qfalse);
			return;
		case CLASS_REBORN:
		case CLASS_SHADOWTROOPER:
		case CLASS_ALORA:
		case CLASS_JEDI:
			if (traceEnt->NPC && traceEnt->NPC->rank > RANK_CIVILIAN && self->client->ps.forcePowerLevel[FP_GRIP] <
				FORCE_LEVEL_2)
			{
				Jedi_PlayDeflectSound(traceEnt);
				ForceThrow(traceEnt, qfalse);
				return;
			}
			break;
		default:
			break;
		}
		if (traceEnt->s.weapon == WP_EMPLACED_GUN)
		{
			//FIXME: maybe can pull them out?
			return;
		}
		if (self->enemy && traceEnt != self->enemy && traceEnt->client->playerTeam == self->client->playerTeam)
		{
			//can't accidently grip your teammate in combat
			return;
		}
		if (-1 != wp_absorb_conversion(traceEnt, traceEnt->client->ps.forcePowerLevel[FP_ABSORB], self, FP_GRIP,
			self->client->ps.forcePowerLevel[FP_GRIP],
			force_power_needed[self->client->ps.forcePowerLevel[FP_GRIP]]))
		{
			return;
		}
	}
	else
	{
		//can't grip non-clients... right?
		if (g_gripitems->integer && (traceEnt->s.eType == ET_ITEM || traceEnt->s.eType == ET_MISSILE || traceEnt->s.
			eType == ET_GENERAL))
		{
			/* WRONG! */
		}
		else
		{
			return;
		}
	}

	// Make sure to turn off Force Protection and Force Absorb.
	if (self->client->ps.forcePowersActive & 1 << FP_PROTECT)
	{
		WP_ForcePowerStop(self, FP_PROTECT);
	}
	if (self->client->ps.forcePowersActive & 1 << FP_ABSORB)
	{
		WP_ForcePowerStop(self, FP_ABSORB);
	}

	WP_ForcePowerStart(self, FP_GRIP, 10);
	self->client->ps.forceGripEntityNum = traceEnt->s.number;
	if (traceEnt->client)
	{
		Vehicle_t* pVeh;
		if ((pVeh = G_IsRidingVehicle(traceEnt)) != nullptr)
		{
			//riding vehicle? pull him off!
			pVeh->m_pVehicleInfo->Eject(pVeh, traceEnt, qtrue);
		}
		G_AddVoiceEvent(traceEnt, Q_irand(EV_PUSHED1, EV_PUSHED3), 2000);
		if (self->client->ps.forcePowerLevel[FP_GRIP] > FORCE_LEVEL_2 || traceEnt->s.weapon == WP_SABER)
		{
			//if we pick up & carry, drop their weap
			if (traceEnt->s.weapon
				&& traceEnt->client->NPC_class != CLASS_ROCKETTROOPER
				&& traceEnt->client->NPC_class != CLASS_VEHICLE
				&& traceEnt->client->NPC_class != CLASS_HAZARD_TROOPER
				&& traceEnt->client->NPC_class != CLASS_TUSKEN
				&& traceEnt->client->NPC_class != CLASS_BOBAFETT
				&& traceEnt->client->NPC_class != CLASS_MANDO
				&& traceEnt->client->NPC_class != CLASS_ASSASSIN_DROID
				&& traceEnt->client->NPC_class != CLASS_DROIDEKA
				&& traceEnt->client->NPC_class != CLASS_SBD
				&& traceEnt->s.weapon != WP_CONCUSSION) // so rax can't drop his
			{
				if (traceEnt->client->NPC_class == CLASS_BOBAFETT || traceEnt->client->NPC_class == CLASS_MANDO)
				{
					//he doesn't drop them, just puts it away
					ChangeWeapon(traceEnt, WP_MELEE);
				}
				else if (traceEnt->s.weapon == WP_MELEE)
				{
					//they can't take that away from me, oh no...
				}
				else if (traceEnt->s.weapon == WP_SABER && traceEnt->client->ps.ManualBlockingFlags & 1 << HOLDINGBLOCK)
				{
					//they can't take that away from me, oh no...
				}
				else if (traceEnt->NPC
					&& traceEnt->NPC->scriptFlags & SCF_DONT_FLEE)
				{
					//*SIGH*... if an NPC can't flee, they can't run after and pick up their weapon, do don't drop it
				}
				else if (traceEnt->s.weapon != WP_SABER)
				{
					WP_DropWeapon(traceEnt, nullptr);
				}
				else
				{
					if (traceEnt->client->ps.SaberActive())
					{
						traceEnt->client->ps.SaberDeactivate();
						G_SoundOnEnt(traceEnt, CHAN_WEAPON, "sound/weapons/saber/saberoffquick.mp3");
					}
				}
			}
		}
		VectorCopy(traceEnt->client->renderInfo.headPoint, self->client->ps.forceGripOrg);
	}
	else
	{
		VectorCopy(traceEnt->currentOrigin, self->client->ps.forceGripOrg);
	}
	self->client->ps.forceGripOrg[2] += 48;

	self->s.loopSound = G_SoundIndex("sound/weapons/force/griploop.wav");

	if (self->client->ps.forcePowerLevel[FP_GRIP] < FORCE_LEVEL_2)
	{
		//just a duration
		self->client->ps.forcePowerDebounce[FP_GRIP] = level.time + 250;
		self->client->ps.forcePowerDuration[FP_GRIP] = level.time + 5000;

		if (self->m_pVehicle && self->m_pVehicle->m_pVehicleInfo->Inhabited(self->m_pVehicle))
		{
			//empty vehicles don't make gripped noise
			if (traceEnt
				&& traceEnt->client
				&& traceEnt->client->NPC_class == CLASS_OBJECT)
			{
				//
			}
			else
			{
			}
		}
	}
	else
	{
		self->client->ps.forcePowerDebounce[FP_GRIP] = level.time + 1000;

		if (traceEnt
			&& traceEnt->client
			&& traceEnt->client->NPC_class == CLASS_OBJECT)
		{
			//
		}
		else
		{
			G_SoundOnEnt(self, CHAN_BODY, "sound/weapons/force/grip.mp3");
		}
	}
}

void ForceGripAdvanced(gentity_t* self)
{
	//FIXME: make enemy Jedi able to use this
	trace_t tr;
	vec3_t end;
	gentity_t* traceEnt = nullptr;

	if (self->health <= 0)
	{
		return;
	}

	if (PM_InLedgeMove(self->client->ps.legsAnim))
	{
		return;
	}
	if (!self->s.number && (cg.zoomMode || in_camera))
	{
		//can't force grip when zoomed in or in cinematic
		return;
	}
	if (self->client->ps.leanofs)
	{
		//can't force-grip while leaning
		return;
	}

	if (self->s.number >= MAX_CLIENTS && !G_ControlledByPlayer(self) && self->client->ps.weapon == WP_SABER)
		//npc force use limit
	{
		if (self->client->ps.blockPoints < 50 || self->client->ps.forcePower < 50)
		{
			return;
		}
	}

	if (self->client->ps.forceGripEntityNum <= ENTITYNUM_WORLD)
	{
		//already gripping
		if (self->client->ps.forcePowerLevel[FP_GRIP] > FORCE_LEVEL_1)
		{
			self->client->ps.forcePowerDuration[FP_GRIP] = level.time + 100;
			self->client->ps.weaponTime = 1000;
			if (self->client->ps.forcePowersActive & 1 << FP_SPEED)
			{
				self->client->ps.weaponTime = floor(self->client->ps.weaponTime * g_timescale->value);
			}
		}
		return;
	}

	if (self->client->ps.userInt3 & 1 << FLAG_PREBLOCK)
	{
		return;
	}

	if (self->client->ps.weaponTime > 0 && (!PM_SaberInParry(self->client->ps.saberMove) || !(self->client->ps.userInt3
		& 1 << FLAG_PREBLOCK)))
	{
		return;
	}

	if (!WP_ForcePowerUsable(self, FP_GRIP, 0))
	{
		//can't use it right now
		return;
	}

	if (self->client->ps.forcePower < 26)
	{
		//need 20 to start, 6 to hold it for any decent amount of time...
		return;
	}

	if (self->client->ps.saberLockTime > level.time)
	{
		return;
	}
	//Cause choking anim + health drain in ent in front of me
	if (self->s.weapon == WP_NONE
		|| self->s.weapon == WP_MELEE
		|| self->s.weapon == WP_SABER && !self->client->ps.SaberActive())
	{
		NPC_SetAnim(self, SETANIM_TORSO, BOTH_FORCEGRIP_HOLD, SETANIM_FLAG_OVERRIDE | SETANIM_FLAG_HOLD);
	}
	else
	{
		NPC_SetAnim(self, SETANIM_TORSO, BOTH_FORCEGRIP_OLD, SETANIM_FLAG_OVERRIDE | SETANIM_FLAG_HOLD);
	}
	self->client->ps.saberMove = self->client->ps.saberBounceMove = LS_READY;
	//don't finish whatever saber anim you may have been in
	self->client->ps.saberBlocked = BLOCKED_NONE;

	self->client->ps.weaponTime = 2000;
	if (self->client->ps.forcePowersActive & 1 << FP_SPEED)
	{
		self->client->ps.weaponTime = floor(self->client->ps.weaponTime * g_timescale->value);
	}

	if (self->enemy)
	{
		vec3_t forward;
		//I have an enemy
		if (self->client->ps.forcePowerLevel[FP_GRIP] > FORCE_LEVEL_2 && self->enemy->health > 0)
		{
			//grip them all!
			AngleVectors(self->client->ps.viewangles, forward, nullptr, nullptr);
			VectorNormalize(forward);
			// ReSharper disable once CppObjectMemberMightNotBeInitialized
			traceEnt = &g_entities[tr.entityNum];

			vec3_t center, mins, maxs, v;
			constexpr float radius = 512;
			float dot;
			gentity_t* entityList[MAX_GENTITIES];
			int i;

			VectorCopy(self->currentOrigin, center);
			for (i = 0; i < 3; i++)
			{
				mins[i] = center[i] - radius;
				maxs[i] = center[i] + radius;
			}
			const int numListedEntities = gi.EntitiesInBox(mins, maxs, entityList, MAX_GENTITIES);

			for (int e = 0; e < numListedEntities; e++)
			{
				vec3_t size;
				vec3_t ent_org;
				vec3_t dir;
				traceEnt = entityList[e];

				if (!traceEnt)
					continue;
				if (traceEnt == self)
					continue;
				if (!traceEnt->inuse)
					continue;
				if (!traceEnt->takedamage)
					continue;
				if (traceEnt->health <= 0) //no torturing corpses
					continue;
				// find the distance from the edge of the bounding box
				for (i = 0; i < 3; i++)
				{
					if (center[i] < traceEnt->absmin[i])
					{
						v[i] = traceEnt->absmin[i] - center[i];
					}
					else if (center[i] > traceEnt->absmax[i])
					{
						v[i] = center[i] - traceEnt->absmax[i];
					}
					else
					{
						v[i] = 0;
					}
				}

				VectorSubtract(traceEnt->absmax, traceEnt->absmin, size);
				VectorMA(traceEnt->absmin, 0.5, size, ent_org);

				//see if they're in front of me
				//must be within the forward cone
				VectorSubtract(ent_org, center, dir);
				VectorNormalize(dir);
				if ((dot = DotProduct(dir, forward)) < 0.5)
					continue;

				//must be close enough
				const float dist = VectorLength(v);
				if (dist >= radius)
				{
					continue;
				}

				//in PVS?
				if (!traceEnt->bmodel && !gi.inPVS(ent_org, self->client->renderInfo.eyePoint))
				{
					//must be in PVS
					continue;
				}

				//Now check and see if we can actually hit it
				gi.trace(&tr, self->client->renderInfo.eyePoint, vec3_origin, vec3_origin, ent_org, self->s.number,
					MASK_SHOT, static_cast<EG2_Collision>(0), 0);
				if (tr.fraction < 1.0f && tr.entityNum != traceEnt->s.number)
				{
					//must have clear LOS
					continue;
				}

				// ok, we are within the radius, add us to the incoming list
				G_AddVoiceEvent(traceEnt, Q_irand(EV_CHOKE1, EV_CHOKE3), 2000);
				ForceGripWide(self, traceEnt, dir, dist, dot, ent_org);
			}
		}
		else
		{
			//grip the enemy!
			AngleVectors(self->client->ps.viewangles, forward, nullptr, nullptr);
			VectorNormalize(forward);
			VectorMA(self->client->renderInfo.eyePoint, FORCE_GRIP_DIST, forward, end);

			if (self->client->ps.forcePowerLevel[FP_GRIP] > FORCE_LEVEL_1 && self->enemy->health > 0)
			{
				//arc
				if (!self->enemy->message && !(self->flags & FL_NO_KNOCKBACK))
				{
					//don't auto-pickup guys with keys
					if (DistanceSquared(self->enemy->currentOrigin, self->currentOrigin) <
						FORCE_STASIS_DIST_SQUARED_LOW)
					{
						//close enough to grab
						float minDot = 0.5f;
						if (self->s.number < MAX_CLIENTS)
						{
							//player needs to be facing more directly
							minDot = 0.2f;
						}
						if (in_front(self->enemy->currentOrigin, self->client->renderInfo.eyePoint,
							self->client->ps.viewangles, minDot))
						{
							//need to be facing the enemy
							if (gi.inPVS(self->enemy->currentOrigin, self->client->renderInfo.eyePoint))
							{
								//must be in PVS
								gi.trace(&tr, self->client->renderInfo.eyePoint, vec3_origin, vec3_origin,
									self->enemy->currentOrigin, self->s.number, MASK_SHOT,
									static_cast<EG2_Collision>(0), 0);
								if (tr.fraction == 1.0f || tr.entityNum == self->enemy->s.number)
								{
									//must have clear LOS
									traceEnt = self->enemy;
								}
							}
						}
					}
				}
			}
			else
			{
				if (!self->enemy->message && !(self->flags & FL_NO_KNOCKBACK) && self->enemy->health > 0)
				{
					//don't auto-pickup guys with keys
					if (DistanceSquared(self->enemy->currentOrigin, self->currentOrigin) < FORCE_GRIP_DIST_SQUARED)
					{
						//close enough to grab
						float minDot = 0.5f;
						if (self->s.number < MAX_CLIENTS)
						{
							//player needs to be facing more directly
							minDot = 0.2f;
						}
						if (in_front(self->enemy->currentOrigin, self->client->renderInfo.eyePoint,
							self->client->ps.viewangles, minDot))
						{
							//need to be facing the enemy
							if (gi.inPVS(self->enemy->currentOrigin, self->client->renderInfo.eyePoint))
							{
								//must be in PVS
								gi.trace(&tr, self->client->renderInfo.eyePoint, vec3_origin, vec3_origin,
									self->enemy->currentOrigin, self->s.number, MASK_SHOT,
									static_cast<EG2_Collision>(0), 0);
								if (tr.fraction == 1.0f || tr.entityNum == self->enemy->s.number)
								{
									//must have clear LOS
									traceEnt = self->enemy;
								}
							}
						}
					}
				}
			}
		}
	}
	if (!traceEnt)
	{
		//okay, trace straight ahead and see what's there
		gi.trace(&tr, self->client->renderInfo.handLPoint, vec3_origin, vec3_origin, end, self->s.number, MASK_SHOT,
			static_cast<EG2_Collision>(0), 0);
		if (tr.entityNum >= ENTITYNUM_WORLD || tr.fraction == 1.0 || tr.allsolid || tr.startsolid)
		{
			if (g_gripitems->integer)
			{
				// One more try...try and physically check for entities in a box, similar to push
				vec3_t mins, maxs;

				for (int i = 0; i < 3; i++)
				{
					mins[i] = self->currentOrigin[i] - 512;
					maxs[i] = self->currentOrigin[i] + 512;
				}

				gentity_t* entlist[MAX_GENTITIES];
				const int numListedEntities = gi.EntitiesInBox(mins, maxs, entlist, MAX_GENTITIES);
				vec3_t vec2, vwangles, traceend;

				VectorCopy(self->currentAngles, vwangles);
				AngleVectors(vwangles, vec2, nullptr, nullptr);
				VectorMA(self->client->renderInfo.eyePoint, 512.0f, vec2, traceend);

				trace_t tr2;
				gi.trace(&tr2, self->client->renderInfo.eyePoint, vec3_origin, vec3_origin, traceend, self->s.number,
					MASK_OPAQUE | CONTENTS_SOLID | CONTENTS_BODY | CONTENTS_ITEM | CONTENTS_CORPSE | MASK_SHOT,
					static_cast<EG2_Collision>(0), 0);
				const gentity_t* fwdEnt = &g_entities[tr2.entityNum];
				qboolean fwdEntIsCorrect = qfalse;
				for (int i = 0; i < numListedEntities; i++)
				{
					const gentity_t* targ_ent = entlist[i];
					if (targ_ent->s.eType == ET_ITEM || targ_ent->s.eType == ET_MISSILE || targ_ent->s.eType ==
						ET_GENERAL)
					{
						if (targ_ent != fwdEnt)
						{
							continue;
						}
						fwdEntIsCorrect = qtrue;
					}
				}

				if (fwdEntIsCorrect)
				{
					tr.entityNum = fwdEnt->s.number;
				}
				else
				{
					return;
				}
			}
			else
			{
				return;
			}
		}
		traceEnt = &g_entities[tr.entityNum];
		G_AddVoiceEvent(traceEnt, Q_irand(EV_CHOKE1, EV_CHOKE3), 2000);
	}
#ifdef JK2_RAGDOLL_GRIPNOHEALTH
	if (!traceEnt || traceEnt == self || traceEnt->bmodel || traceEnt->NPC && traceEnt->NPC->scriptFlags & SCF_NO_FORCE)
	{
		return;
	}
#else
	if (!traceEnt || traceEnt == self || traceEnt->bmodel || (traceEnt->health <= 0 && traceEnt->takedamage) || (traceEnt->NPC && traceEnt->NPC->scriptFlags & SCF_NO_FORCE))
	{
		return;
		traceEnt->client->ps.powerups[PW_MEDITATE] = level.time + traceEnt->client->ps.torsoAnimTimer + 5000;
		NPC_SetAnim(traceEnt, SETANIM_TORSO, BOTH_WIND, SETANIM_AFLAG_PACE);
	}
#endif
	if (traceEnt->m_pVehicle != nullptr)
	{
		//is it a vehicle
		//grab pilot if there is one
		if (traceEnt->m_pVehicle->m_pPilot != nullptr
			&& traceEnt->m_pVehicle->m_pPilot->client != nullptr)
		{
			//grip the pilot
			traceEnt = traceEnt->m_pVehicle->m_pPilot;
		}
		else
		{
			//can't grip a vehicle
			return;
		}
	}
	if (traceEnt->client)
	{
		if (traceEnt->client->ps.forceJumpZStart)
		{
			//can't catch them in mid force jump - FIXME: maybe base it on velocity?
			return;
		}
		if (traceEnt->client->ps.pullAttackTime > level.time)
		{
			//can't grip someone who is being pull-attacked or is pull-attacking
			return;
		}
		if (!Q_stricmp("Yoda", traceEnt->NPC_type))
		{
			Jedi_PlayDeflectSound(traceEnt);
			ForceThrow(traceEnt, qfalse);
			return;
		}
		if (!Q_stricmp("T_Yoda", traceEnt->NPC_type))
		{
			Jedi_PlayDeflectSound(traceEnt);
			ForceThrow(traceEnt, qfalse);
			return;
		}
		if (!Q_stricmp("T_Palpatine_sith", traceEnt->NPC_type))
		{
			Jedi_PlayDeflectSound(traceEnt);
			ForceThrow(traceEnt, qfalse);
			return;
		}

		if (G_IsRidingVehicle(traceEnt)
			&& traceEnt->s.eFlags & EF_NODRAW)
		{
			//riding *inside* vehicle
			return;
		}

		switch (traceEnt->client->NPC_class)
		{
		case CLASS_GALAKMECH: //cant grip him, he's in armor
			G_AddVoiceEvent(traceEnt, Q_irand(EV_PUSHED1, EV_PUSHED3), Q_irand(3000, 5000));
			return;
		case CLASS_HAZARD_TROOPER: //cant grip him, he's in armor
			return;
		case CLASS_ATST: //much too big to grip!
		case CLASS_RANCOR: //much too big to grip!
		case CLASS_WAMPA: //much too big to grip!
		case CLASS_SAND_CREATURE: //much too big to grip!
			return;
			//no droids either...?
		case CLASS_GONK:
		case CLASS_R2D2:
		case CLASS_R5D2:
		case CLASS_MARK1:
		case CLASS_MARK2:
		case CLASS_MOUSE: //?
		case CLASS_PROTOCOL:
			//*sigh*... in JK3, you'll be able to grab and move *anything*...
			return;
			//not even combat droids?  (No animation for being gripped...)
		case CLASS_SABER_DROID:
		case CLASS_ASSASSIN_DROID:
		case CLASS_SBD:
		case CLASS_DROIDEKA:
			//*sigh*... in JK3, you'll be able to grab and move *anything*...
			return;
		case CLASS_PROBE:
		case CLASS_SEEKER:
		case CLASS_REMOTE:
		case CLASS_SENTRY:
		case CLASS_INTERROGATOR:
			//*sigh*... in JK3, you'll be able to grab and move *anything*...
			return;
		case CLASS_SITHLORD:
		case CLASS_DESANN: //Desann cannot be gripped, he just pushes you back instantly
		case CLASS_VADER:
		case CLASS_KYLE:
		case CLASS_TAVION:
		case CLASS_LUKE:
		case CLASS_YODA:
			Jedi_PlayDeflectSound(traceEnt);
			ForceThrow(traceEnt, qfalse);
			return;
		case CLASS_REBORN:
		case CLASS_SHADOWTROOPER:
		case CLASS_ALORA:
		case CLASS_JEDI:
			if (traceEnt->NPC && traceEnt->NPC->rank > RANK_CIVILIAN && self->client->ps.forcePowerLevel[FP_GRIP] <
				FORCE_LEVEL_2)
			{
				Jedi_PlayDeflectSound(traceEnt);
				ForceThrow(traceEnt, qfalse);
				return;
			}
			break;
		default:
			break;
		}
		if (traceEnt->s.weapon == WP_EMPLACED_GUN)
		{
			//FIXME: maybe can pull them out?
			return;
		}
		if (self->enemy && traceEnt != self->enemy && traceEnt->client->playerTeam == self->client->playerTeam)
		{
			//can't accidently grip your teammate in combat
			return;
		}
		if (-1 != wp_absorb_conversion(traceEnt, traceEnt->client->ps.forcePowerLevel[FP_ABSORB], self, FP_GRIP,
			self->client->ps.forcePowerLevel[FP_GRIP],
			force_power_needed[self->client->ps.forcePowerLevel[FP_GRIP]]))
		{
			return;
		}
	}
	else
	{
		//can't grip non-clients... right?
		if (g_gripitems->integer && (traceEnt->s.eType == ET_ITEM || traceEnt->s.eType == ET_MISSILE || traceEnt->s.
			eType == ET_GENERAL))
		{
			/* WRONG! */
		}
		else
		{
			return;
		}
	}

	// Make sure to turn off Force Protection and Force Absorb.
	if (self->client->ps.forcePowersActive & 1 << FP_PROTECT)
	{
		WP_ForcePowerStop(self, FP_PROTECT);
	}
	if (self->client->ps.forcePowersActive & 1 << FP_ABSORB)
	{
		WP_ForcePowerStop(self, FP_ABSORB);
	}

	WP_ForcePowerStart(self, FP_GRIP, 20);
	self->client->ps.forceGripEntityNum = traceEnt->s.number;
	if (traceEnt->client)
	{
		Vehicle_t* pVeh;
		if ((pVeh = G_IsRidingVehicle(traceEnt)) != nullptr)
		{
			//riding vehicle? pull him off!
			pVeh->m_pVehicleInfo->Eject(pVeh, traceEnt, qtrue);
		}
		G_AddVoiceEvent(traceEnt, Q_irand(EV_PUSHED1, EV_PUSHED3), 2000);
		if (self->client->ps.forcePowerLevel[FP_GRIP] > FORCE_LEVEL_2 || traceEnt->s.weapon == WP_SABER)
		{
			//if we pick up & carry, drop their weap
			if (traceEnt->s.weapon
				&& traceEnt->client->NPC_class != CLASS_ROCKETTROOPER
				&& traceEnt->client->NPC_class != CLASS_VEHICLE
				&& traceEnt->client->NPC_class != CLASS_HAZARD_TROOPER
				&& traceEnt->client->NPC_class != CLASS_TUSKEN
				&& traceEnt->client->NPC_class != CLASS_BOBAFETT
				&& traceEnt->client->NPC_class != CLASS_MANDO
				&& traceEnt->client->NPC_class != CLASS_ASSASSIN_DROID
				&& traceEnt->client->NPC_class != CLASS_DROIDEKA
				&& traceEnt->client->NPC_class != CLASS_SBD
				&& traceEnt->s.weapon != WP_CONCUSSION) // so rax can't drop his
			{
				if (traceEnt->client->NPC_class == CLASS_BOBAFETT || traceEnt->client->NPC_class == CLASS_MANDO)
				{
					//he doesn't drop them, just puts it away
					ChangeWeapon(traceEnt, WP_MELEE);
				}
				else if (traceEnt->s.weapon == WP_MELEE)
				{
					//they can't take that away from me, oh no...
				}
				else if (traceEnt->s.weapon == WP_SABER && traceEnt->client->ps.ManualBlockingFlags & 1 << HOLDINGBLOCK)
				{
					//they can't take that away from me, oh no...
				}
				else if (traceEnt->NPC
					&& traceEnt->NPC->scriptFlags & SCF_DONT_FLEE)
				{
					//*SIGH*... if an NPC can't flee, they can't run after and pick up their weapon, do don't drop it
				}
				else if (traceEnt->s.weapon != WP_SABER)
				{
					WP_DropWeapon(traceEnt, nullptr);
				}
				else
				{
					//turn it off?
					if (traceEnt->client->ps.SaberActive())
					{
						traceEnt->client->ps.SaberDeactivate();
						G_SoundOnEnt(traceEnt, CHAN_WEAPON, "sound/weapons/saber/saberoffquick.mp3");
					}
				}
			}
		}
		VectorCopy(traceEnt->client->renderInfo.headPoint, self->client->ps.forceGripOrg);
	}
	else
	{
		VectorCopy(traceEnt->currentOrigin, self->client->ps.forceGripOrg);
	}

	self->s.loopSound = G_SoundIndex("sound/weapons/force/griploop.wav");

	if (self->client->ps.forcePowerLevel[FP_GRIP] < FORCE_LEVEL_2)
	{
		//just a duration
		self->client->ps.forcePowerDebounce[FP_GRIP] = level.time + 250;
		self->client->ps.forcePowerDuration[FP_GRIP] = level.time + 5000;

		if (self->m_pVehicle && self->m_pVehicle->m_pVehicleInfo->Inhabited(self->m_pVehicle))
		{
			//empty vehicles don't make gripped noise
			if (traceEnt
				&& traceEnt->client
				&& traceEnt->client->NPC_class == CLASS_OBJECT)
			{
				//
			}
			else
			{
				G_SoundOnEnt(self, CHAN_BODY, "sound/weapons/force/grip.mp3");
			}
		}
	}
	else
	{
		self->client->ps.forcePowerDebounce[FP_GRIP] = level.time + 1000;

		if (traceEnt
			&& traceEnt->client
			&& traceEnt->client->NPC_class == CLASS_OBJECT)
		{
			//
		}
		else
		{
			G_SoundOnEnt(self, CHAN_BODY, "sound/weapons/force/grip.mp3");
		}
	}
}

void ForceGripBasic(gentity_t* self)
{
	//FIXME: make enemy Jedi able to use this
	trace_t tr;
	vec3_t end, forward;
	gentity_t* traceEnt = nullptr;

	if (self->health <= 0)
	{
		return;
	}

	if (PM_InLedgeMove(self->client->ps.legsAnim))
	{
		return;
	}
	if (!self->s.number && (cg.zoomMode || in_camera))
	{
		//can't force grip when zoomed in or in cinematic
		return;
	}
	if (self->client->ps.leanofs)
	{
		//can't force-grip while leaning
		return;
	}

	if (self->client->ps.forceGripEntityNum <= ENTITYNUM_WORLD)
	{
		//already gripping
		if (self->client->ps.forcePowerLevel[FP_GRIP] > FORCE_LEVEL_1)
		{
			self->client->ps.forcePowerDuration[FP_GRIP] = level.time + 100;
			self->client->ps.weaponTime = 1000;
			if (self->client->ps.forcePowersActive & 1 << FP_SPEED)
			{
				self->client->ps.weaponTime = floor(self->client->ps.weaponTime * g_timescale->value);
			}
		}
		return;
	}

	if (self->s.number >= MAX_CLIENTS && !G_ControlledByPlayer(self) && self->client->ps.weapon == WP_SABER)
		//npc force use limit
	{
		if (self->client->ps.blockPoints < 75 || self->client->ps.forcePower < 75)
		{
			return;
		}
	}

	if (self->client->ps.userInt3 & 1 << FLAG_PREBLOCK)
	{
		return;
	}

	if (self->client->ps.weaponTime > 0 && (!PM_SaberInParry(self->client->ps.saberMove) || !(self->client->ps.userInt3
		& 1 << FLAG_PREBLOCK)))
	{
		return;
	}

	if (!WP_ForcePowerUsable(self, FP_GRIP, 0))
	{
		//can't use it right now
		return;
	}

	if (self->client->ps.forcePower < 26)
	{
		//need 20 to start, 6 to hold it for any decent amount of time...
		return;
	}

	if (self->client->ps.saberLockTime > level.time)
	{
		return;
	}

	if (self->s.weapon == WP_NONE
		|| self->s.weapon == WP_MELEE
		|| self->s.weapon == WP_SABER && !self->client->ps.SaberActive())
	{
		NPC_SetAnim(self, SETANIM_TORSO, BOTH_FORCEGRIP_HOLD, SETANIM_FLAG_OVERRIDE | SETANIM_FLAG_HOLD);
	}
	else
	{
		NPC_SetAnim(self, SETANIM_TORSO, BOTH_FORCEGRIP_OLD, SETANIM_FLAG_OVERRIDE | SETANIM_FLAG_HOLD);
	}
	self->client->ps.saberMove = self->client->ps.saberBounceMove = LS_READY;
	self->client->ps.saberBlocked = BLOCKED_NONE;

	self->client->ps.weaponTime = 2000;
	if (self->client->ps.forcePowersActive & 1 << FP_SPEED)
	{
		self->client->ps.weaponTime = floor(self->client->ps.weaponTime * g_timescale->value);
	}

	AngleVectors(self->client->ps.viewangles, forward, nullptr, nullptr);
	VectorNormalize(forward);
	VectorMA(self->client->renderInfo.handLPoint, FORCE_GRIP_DIST, forward, end);

	if (self->enemy)
	{
		//I have an enemy
		if (self->client->ps.forcePowerLevel[FP_GRIP] > FORCE_LEVEL_1 && self->enemy->health > 0)
		{
			//arc
			if (!self->enemy->message && !(self->flags & FL_NO_KNOCKBACK))
			{
				//don't auto-pickup guys with keys
				if (DistanceSquared(self->enemy->currentOrigin, self->currentOrigin) < FORCE_STASIS_DIST_SQUARED_LOW)
				{
					//close enough to grab
					float minDot = 0.5f;
					if (self->s.number < MAX_CLIENTS)
					{
						//player needs to be facing more directly
						minDot = 0.2f;
					}
					if (in_front(self->enemy->currentOrigin, self->client->renderInfo.eyePoint,
						self->client->ps.viewangles, minDot))
					{
						//need to be facing the enemy
						if (gi.inPVS(self->enemy->currentOrigin, self->client->renderInfo.eyePoint))
						{
							//must be in PVS
							gi.trace(&tr, self->client->renderInfo.eyePoint, vec3_origin, vec3_origin,
								self->enemy->currentOrigin, self->s.number, MASK_SHOT,
								static_cast<EG2_Collision>(0), 0);
							if (tr.fraction == 1.0f || tr.entityNum == self->enemy->s.number)
							{
								//must have clear LOS
								traceEnt = self->enemy;
							}
						}
					}
				}
			}
		}
		else
		{
			if (!self->enemy->message && !(self->flags & FL_NO_KNOCKBACK) && self->enemy->health > 0)
			{
				//don't auto-pickup guys with keys
				if (DistanceSquared(self->enemy->currentOrigin, self->currentOrigin) < FORCE_GRIP_DIST_SQUARED)
				{
					//close enough to grab
					float minDot = 0.5f;
					if (self->s.number < MAX_CLIENTS)
					{
						//player needs to be facing more directly
						minDot = 0.2f;
					}
					if (in_front(self->enemy->currentOrigin, self->client->renderInfo.eyePoint,
						self->client->ps.viewangles, minDot))
					{
						//need to be facing the enemy
						if (gi.inPVS(self->enemy->currentOrigin, self->client->renderInfo.eyePoint))
						{
							//must be in PVS
							gi.trace(&tr, self->client->renderInfo.eyePoint, vec3_origin, vec3_origin,
								self->enemy->currentOrigin, self->s.number, MASK_SHOT,
								static_cast<EG2_Collision>(0), 0);
							if (tr.fraction == 1.0f || tr.entityNum == self->enemy->s.number)
							{
								//must have clear LOS
								traceEnt = self->enemy;
							}
						}
					}
				}
			}
		}
	}
	if (!traceEnt)
	{
		//okay, trace straight ahead and see what's there
		gi.trace(&tr, self->client->renderInfo.handLPoint, vec3_origin, vec3_origin, end, self->s.number, MASK_SHOT,
			static_cast<EG2_Collision>(0), 0);
		if (tr.entityNum >= ENTITYNUM_WORLD || tr.fraction == 1.0 || tr.allsolid || tr.startsolid)
		{
			return;
		}

		traceEnt = &g_entities[tr.entityNum];
	}
#ifdef JK2_RAGDOLL_GRIPNOHEALTH
	if (!traceEnt || traceEnt == self || traceEnt->bmodel || traceEnt->NPC && traceEnt->NPC->scriptFlags & SCF_NO_FORCE)
	{
		return;
	}
#else
	if (!traceEnt || traceEnt == self || traceEnt->bmodel || (traceEnt->health <= 0 && traceEnt->takedamage) || (traceEnt->NPC && traceEnt->NPC->scriptFlags & SCF_NO_FORCE))
	{
		return;
		traceEnt->client->ps.powerups[PW_MEDITATE] = level.time + traceEnt->client->ps.torsoAnimTimer + 5000;
		NPC_SetAnim(traceEnt, SETANIM_TORSO, BOTH_WIND, SETANIM_AFLAG_PACE);
	}
#endif

	if (traceEnt->m_pVehicle != nullptr)
	{
		//is it a vehicle
		//grab pilot if there is one
		if (traceEnt->m_pVehicle->m_pPilot != nullptr
			&& traceEnt->m_pVehicle->m_pPilot->client != nullptr)
		{
			//grip the pilot
			traceEnt = traceEnt->m_pVehicle->m_pPilot;
		}
		else
		{
			//can't grip a vehicle
			return;
		}
	}
	if (traceEnt->client)
	{
		if (traceEnt->client->ps.forceJumpZStart)
		{
			//can't catch them in mid force jump - FIXME: maybe base it on velocity?
			return;
		}
		if (traceEnt->client->ps.pullAttackTime > level.time)
		{
			//can't grip someone who is being pull-attacked or is pull-attacking
			return;
		}
		if (!Q_stricmp("Yoda", traceEnt->NPC_type))
		{
			Jedi_PlayDeflectSound(traceEnt);
			ForceThrow(traceEnt, qfalse);
			return;
		}
		if (!Q_stricmp("T_Yoda", traceEnt->NPC_type))
		{
			Jedi_PlayDeflectSound(traceEnt);
			ForceThrow(traceEnt, qfalse);
			return;
		}
		if (!Q_stricmp("T_Palpatine_sith", traceEnt->NPC_type))
		{
			Jedi_PlayDeflectSound(traceEnt);
			ForceThrow(traceEnt, qfalse);
			return;
		}

		if (G_IsRidingVehicle(traceEnt)
			&& traceEnt->s.eFlags & EF_NODRAW)
		{
			//riding *inside* vehicle
			return;
		}

		switch (traceEnt->client->NPC_class)
		{
		case CLASS_GALAKMECH: //cant grip him, he's in armor
			G_AddVoiceEvent(traceEnt, Q_irand(EV_PUSHED1, EV_PUSHED3), Q_irand(3000, 5000));
			return;
		case CLASS_HAZARD_TROOPER: //cant grip him, he's in armor
			return;
		case CLASS_ATST: //much too big to grip!
		case CLASS_RANCOR: //much too big to grip!
		case CLASS_WAMPA: //much too big to grip!
		case CLASS_SAND_CREATURE: //much too big to grip!
			return;
			//no droids either...?
		case CLASS_GONK:
		case CLASS_R2D2:
		case CLASS_R5D2:
		case CLASS_MARK1:
		case CLASS_MARK2:
		case CLASS_MOUSE: //?
		case CLASS_PROTOCOL:
			//*sigh*... in JK3, you'll be able to grab and move *anything*...
			return;
			//not even combat droids?  (No animation for being gripped...)
		case CLASS_SABER_DROID:
		case CLASS_ASSASSIN_DROID:
		case CLASS_SBD:
		case CLASS_DROIDEKA:
			//*sigh*... in JK3, you'll be able to grab and move *anything*...
			return;
		case CLASS_PROBE:
		case CLASS_SEEKER:
		case CLASS_REMOTE:
		case CLASS_SENTRY:
		case CLASS_INTERROGATOR:
			//*sigh*... in JK3, you'll be able to grab and move *anything*...
			return;
		case CLASS_SITHLORD:
		case CLASS_DESANN: //Desann cannot be gripped, he just pushes you back instantly
		case CLASS_VADER:
		case CLASS_KYLE:
		case CLASS_TAVION:
		case CLASS_LUKE:
		case CLASS_YODA:
			Jedi_PlayDeflectSound(traceEnt);
			ForceThrow(traceEnt, qfalse);
			return;
		case CLASS_REBORN:
		case CLASS_SHADOWTROOPER:
		case CLASS_ALORA:
		case CLASS_JEDI:
			if (traceEnt->NPC && traceEnt->NPC->rank > RANK_CIVILIAN && self->client->ps.forcePowerLevel[FP_GRIP] <
				FORCE_LEVEL_2)
			{
				Jedi_PlayDeflectSound(traceEnt);
				ForceThrow(traceEnt, qfalse);
				return;
			}
			break;
		default:
			break;
		}
		if (traceEnt->s.weapon == WP_EMPLACED_GUN)
		{
			//FIXME: maybe can pull them out?
			return;
		}
		if (self->enemy && traceEnt != self->enemy && traceEnt->client->playerTeam == self->client->playerTeam)
		{
			//can't accidently grip your teammate in combat
			return;
		}
		if (-1 != wp_absorb_conversion(traceEnt, traceEnt->client->ps.forcePowerLevel[FP_ABSORB], self, FP_GRIP,
			self->client->ps.forcePowerLevel[FP_GRIP],
			force_power_needed[self->client->ps.forcePowerLevel[FP_GRIP]]))
		{
			return;
		}
		//===============
	}
	else
	{
		//can't grip non-clients... right?
		return;
	}

	// Make sure to turn off Force Protection and Force Absorb.
	if (self->client->ps.forcePowersActive & 1 << FP_PROTECT)
	{
		WP_ForcePowerStop(self, FP_PROTECT);
	}
	if (self->client->ps.forcePowersActive & 1 << FP_ABSORB)
	{
		WP_ForcePowerStop(self, FP_ABSORB);
	}

	WP_ForcePowerStart(self, FP_GRIP, 20);
	self->client->ps.forceGripEntityNum = traceEnt->s.number;
	if (traceEnt->client)
	{
		Vehicle_t* pVeh;
		if ((pVeh = G_IsRidingVehicle(traceEnt)) != nullptr)
		{
			//riding vehicle? pull him off!
			pVeh->m_pVehicleInfo->Eject(pVeh, traceEnt, qtrue);
		}
		G_AddVoiceEvent(traceEnt, Q_irand(EV_PUSHED1, EV_PUSHED3), 2000);
		if (self->client->ps.forcePowerLevel[FP_GRIP] > FORCE_LEVEL_2 || traceEnt->s.weapon == WP_SABER)
		{
			//if we pick up & carry, drop their weap
			if (traceEnt->s.weapon
				&& traceEnt->client->NPC_class != CLASS_ROCKETTROOPER
				&& traceEnt->client->NPC_class != CLASS_VEHICLE
				&& traceEnt->client->NPC_class != CLASS_HAZARD_TROOPER
				&& traceEnt->client->NPC_class != CLASS_TUSKEN
				&& traceEnt->client->NPC_class != CLASS_BOBAFETT
				&& traceEnt->client->NPC_class != CLASS_MANDO
				&& traceEnt->client->NPC_class != CLASS_ASSASSIN_DROID
				&& traceEnt->client->NPC_class != CLASS_DROIDEKA
				&& traceEnt->client->NPC_class != CLASS_SBD
				&& traceEnt->s.weapon != WP_CONCUSSION) // so rax can't drop his
			{
				if (traceEnt->client->NPC_class == CLASS_BOBAFETT || traceEnt->client->NPC_class == CLASS_MANDO)
				{
					//he doesn't drop them, just puts it away
					ChangeWeapon(traceEnt, WP_MELEE);
				}
				else if (traceEnt->s.weapon == WP_MELEE)
				{
					//they can't take that away from me, oh no...
				}
				else if (traceEnt->s.weapon == WP_SABER && traceEnt->client->ps.ManualBlockingFlags & 1 << HOLDINGBLOCK)
				{
					//they can't take that away from me, oh no...
				}
				else if (traceEnt->NPC
					&& traceEnt->NPC->scriptFlags & SCF_DONT_FLEE)
				{
					//*SIGH*... if an NPC can't flee, they can't run after and pick up their weapon, do don't drop it
				}
				else if (traceEnt->s.weapon != WP_SABER)
				{
					WP_DropWeapon(traceEnt, nullptr);
				}
				else
				{
					//turn it off?
					if (traceEnt->client->ps.SaberActive())
					{
						traceEnt->client->ps.SaberDeactivate();
						G_SoundOnEnt(traceEnt, CHAN_WEAPON, "sound/weapons/saber/saberoffquick.mp3");
					}
				}
			}
		}
		VectorCopy(traceEnt->client->renderInfo.headPoint, self->client->ps.forceGripOrg);
	}
	else
	{
		VectorCopy(traceEnt->currentOrigin, self->client->ps.forceGripOrg);
	}
	self->client->ps.forceGripOrg[2] += 48;

	self->s.loopSound = G_SoundIndex("sound/weapons/force/griploop.wav");

	if (self->client->ps.forcePowerLevel[FP_GRIP] < FORCE_LEVEL_2)
	{
		//just a duration
		self->client->ps.forcePowerDebounce[FP_GRIP] = level.time + 250;
		self->client->ps.forcePowerDuration[FP_GRIP] = level.time + 5000;

		if (self->m_pVehicle && self->m_pVehicle->m_pVehicleInfo->Inhabited(self->m_pVehicle))
		{
			//empty vehicles don't make gripped noise
			if (traceEnt
				&& traceEnt->client
				&& traceEnt->client->NPC_class == CLASS_OBJECT)
			{
				//
			}
			else
			{
				G_SoundOnEnt(self, CHAN_BODY, "sound/weapons/force/grip.mp3");
			}
		}
	}
	else
	{
		self->client->ps.forcePowerDebounce[FP_GRIP] = level.time + 1000;

		if (traceEnt
			&& traceEnt->client
			&& traceEnt->client->NPC_class == CLASS_OBJECT)
		{
			//
		}
		else
		{
			G_SoundOnEnt(self, CHAN_BODY, "sound/weapons/force/grip.mp3");
		}
	}
}

qboolean ForceLightningCheckattack(const gentity_t* self)
{
	if (self->client->ps.torsoAnim == BOTH_FORCE_2HANDEDLIGHTNING
		|| self->client->ps.torsoAnim == BOTH_FORCE_2HANDEDLIGHTNING_OLD
		|| self->client->ps.torsoAnim == BOTH_FORCE_2HANDEDLIGHTNING_NEW
		|| self->client->ps.torsoAnim == BOTH_FORCE_2HANDEDLIGHTNING_START
		|| self->client->ps.torsoAnim == BOTH_FORCE_2HANDEDLIGHTNING_HOLD
		|| self->client->ps.torsoAnim == BOTH_FORCELIGHTNING_START
		|| self->client->ps.torsoAnim == BOTH_FORCELIGHTNING_HOLD
		|| self->client->ps.torsoAnim == BOTH_FORCELIGHTNING)
	{
		return qtrue;
	}
	return qfalse;
}

qboolean ForceLightningCheck2Handed(const gentity_t* self)
{
	if (!in_camera && self && self->client)
	{
		if (self->s.weapon == WP_NONE
			|| self->s.weapon == WP_MELEE
			|| self->s.weapon == WP_SABER && !self->client->ps.SaberActive())
		{
			return qtrue;
		}
	}
	return qfalse;
}

void ForceLightningAnim(gentity_t* self)
{
	if (!self || !self->client)
	{
		return;
	}

	//one-handed lightning 2 and above
	int startAnim = BOTH_FORCELIGHTNING_START;
	int holdAnim = BOTH_FORCELIGHTNING_HOLD;
	constexpr int bobahold_anim = BOTH_FLAMETHROWER;

	if (ForceLightningCheck2Handed(self))
	{
		//empty handed lightning 3
		startAnim = BOTH_FORCE_2HANDEDLIGHTNING_START;
		holdAnim = BOTH_FORCE_2HANDEDLIGHTNING_HOLD;
	}

	//FIXME: if standing still, play on whole body?  Especially 2-handed version
	if (self->client->ps.torsoAnim == startAnim)
	{
		if (!self->client->ps.torsoAnimTimer)
		{
			if (self->client->NPC_class == CLASS_BOBAFETT || self->client->NPC_class == CLASS_MANDO)
			{
				NPC_SetAnim(self, SETANIM_TORSO, bobahold_anim,
					SETANIM_FLAG_OVERRIDE | SETANIM_FLAG_HOLD | SETANIM_FLAG_RESTART);
			}
			else
			{
				NPC_SetAnim(self, SETANIM_TORSO, holdAnim,
					SETANIM_FLAG_OVERRIDE | SETANIM_FLAG_HOLD | SETANIM_FLAG_RESTART);
			}
		}
		else
		{
			NPC_SetAnim(self, SETANIM_TORSO, startAnim,
				SETANIM_FLAG_OVERRIDE | SETANIM_FLAG_HOLD | SETANIM_FLAG_RESTART);
		}
	}
	else
	{
		if (self->client->NPC_class == CLASS_BOBAFETT || self->client->NPC_class == CLASS_MANDO)
		{
			NPC_SetAnim(self, SETANIM_TORSO, bobahold_anim, SETANIM_FLAG_OVERRIDE | SETANIM_FLAG_HOLD);
		}
		else
		{
			NPC_SetAnim(self, SETANIM_TORSO, holdAnim, SETANIM_FLAG_OVERRIDE | SETANIM_FLAG_HOLD);
		}
	}
}

qboolean CanBeFeared(const gentity_t* self, const gentity_t* traceEnt);

void ForceFear(gentity_t* self)
{
	trace_t tr;
	vec3_t end, forward;
	qboolean targetLive = qfalse;

	if (WP_CheckBreakControl(self))
	{
		return;
	}
	if (self->health <= 0)
	{
		return;
	}
	//FIXME: if mind trick 3 and aiming at an enemy need more force power
	if (!WP_ForcePowerUsable(self, FP_FEAR, 0))
	{
		return;
	}

	if (self->client->ps.weaponTime >= 800)
	{
		//just did one!
		return;
	}
	if (self->client->ps.saberLockTime > level.time)
	{
		//FIXME: can this be a way to break out?
		return;
	}

	if (PM_InLedgeMove(self->client->ps.legsAnim))
	{
		return;
	}

	AngleVectors(self->client->ps.viewangles, forward, nullptr, nullptr);
	VectorNormalize(forward);
	VectorMA(self->client->renderInfo.eyePoint, 2048, forward, end);

	//Cause a distraction if enemy is not fighting
	gi.trace(&tr, self->client->renderInfo.eyePoint, vec3_origin, vec3_origin, end, self->s.number,
		MASK_OPAQUE | CONTENTS_BODY, static_cast<EG2_Collision>(0), 0);
	if (tr.entityNum == ENTITYNUM_NONE || tr.fraction == 1.0 || tr.allsolid || tr.startsolid)
	{
		return;
	}

	gentity_t* traceEnt = &g_entities[tr.entityNum];

	if (traceEnt->NPC && traceEnt->NPC->scriptFlags & SCF_NO_FORCE)
	{
		return;
	}

	if (traceEnt && traceEnt->client)
	{
		switch (traceEnt->client->NPC_class)
		{
		case CLASS_GALAKMECH: //cant grip him, he's in armor
		case CLASS_ATST: //much too big to grip!
			//no droids either
		case CLASS_PROBE:
		case CLASS_GONK:
		case CLASS_R2D2:
		case CLASS_R5D2:
		case CLASS_MARK1:
		case CLASS_MARK2:
		case CLASS_MOUSE:
		case CLASS_SEEKER:
		case CLASS_REMOTE:
		case CLASS_PROTOCOL:
		case CLASS_ASSASSIN_DROID:
		case CLASS_SABER_DROID:
		case CLASS_BOBAFETT:
		case CLASS_MANDO:
			break;
		case CLASS_RANCOR:
			if (!(traceEnt->spawnflags & 1))
			{
				targetLive = qtrue;
			}
			break;
		default:
			targetLive = qtrue;
			break;
		}
	}
	if (targetLive
		&& traceEnt->NPC
		&& traceEnt->health > 0)
	{
		//hit an organic non-player
		if (traceEnt->client->playerTeam != self->client->playerTeam || traceEnt->client->playerTeam == self->client->
			playerTeam)
		{
			//an enemy
			int override = 0;
			if (traceEnt->NPC->scriptFlags & SCF_NO_MIND_TRICK)
			{
				if (traceEnt->client->NPC_class == CLASS_GALAKMECH)
				{
					G_AddVoiceEvent(traceEnt, Q_irand(EV_CONFUSE1, EV_CONFUSE3), Q_irand(3000, 5000));
				}
			}
			else if (CanBeFeared(self, traceEnt))
			{
				// Jedi can be feared, but it's quite rare
				if (!Pilot_AnyVehiclesRegistered()) //don't charm guys when bikes are near
				{
					//turn them crazy
					override = 50;
					if (self->client->ps.forcePower < 50)
					{
						return;
					}
					if (traceEnt->enemy)
					{
						G_ClearEnemy(traceEnt);
					}
					if (traceEnt->NPC)
					{
						if (traceEnt->s.weapon == WP_NONE)
						{
							CG_ChangeWeapon(WP_MELEE);
						}
					}

					if (PM_HasAnimation(traceEnt, BOTH_SONICPAIN_HOLD))
					{
						NPC_SetAnim(traceEnt, SETANIM_LEGS, BOTH_SONICPAIN_HOLD, SETANIM_FLAG_NORMAL);
						NPC_SetAnim(traceEnt, SETANIM_TORSO, BOTH_SONICPAIN_HOLD,
							SETANIM_FLAG_OVERRIDE | SETANIM_FLAG_HOLD);
						traceEnt->client->ps.torsoAnimTimer += 200;
						traceEnt->client->ps.weaponTime = traceEnt->client->ps.torsoAnimTimer;
					}

					if (self->client->ps.forcePowerLevel[FP_FEAR] > FORCE_LEVEL_1)
					{
						traceEnt->client->enemyTeam = TEAM_SOLO;
						traceEnt->client->playerTeam = TEAM_SOLO;
					}
					else
					{
						traceEnt->client->enemyTeam = TEAM_FREE;
						traceEnt->client->playerTeam = TEAM_FREE;
					}

					G_Sound(self, G_SoundIndex("sound/weapons/force/fear.mp3"));
					G_ClearEnemy(traceEnt);

					if (traceEnt->s.weapon == WP_SABER)
					{
						G_AddVoiceEvent(traceEnt, EV_FALL_MEDIUM, 1);
					}
					else
					{
						G_AddVoiceEvent(traceEnt, Q_irand(EV_GIVEUP1, EV_GIVEUP4), 2000);
					}

					traceEnt->NPC->charmedTime = level.time + mindTrickTime[self->client->ps.forcePowerLevel[FP_FEAR]];
					if (traceEnt->ghoul2.size() && traceEnt->headBolt != -1)
					{
						G_PlayEffect(G_EffectIndex("force/drain_hand"), traceEnt->playerModel, traceEnt->headBolt,
							traceEnt->s.number, traceEnt->currentOrigin,
							mindTrickTime[self->client->ps.forcePowerLevel[FP_FEAR]], qtrue);
					}
				}
			}
			else
			{
				NPC_Jedi_PlayConfusionSound(traceEnt);
			}
			WP_ForcePowerStart(self, FP_FEAR, override);
		}
		vec3_t eyeDir;
		AngleVectors(traceEnt->client->renderInfo.eyeAngles, eyeDir, nullptr, nullptr);
		VectorNormalize(eyeDir);
		G_PlayEffect("force/drain_hand", traceEnt->client->renderInfo.eyePoint, eyeDir);

		//FIXME: build-up or delay this until in proper part of anim
	}
	NPC_SetAnim(self, SETANIM_TORSO, BOTH_MINDTRICK1, SETANIM_FLAG_OVERRIDE | SETANIM_FLAG_RESTART | SETANIM_FLAG_HOLD);

	if (!targetLive
		&& !traceEnt->NPC)
	{
		G_Sound(self, G_SoundIndex("sound/weapons/force/fearfail.mp3"));
	}
	self->client->ps.saberMove = self->client->ps.saberBounceMove = LS_READY;
	//don't finish whatever saber anim you may have been in
	self->client->ps.saberBlocked = BLOCKED_NONE;
	self->client->ps.weaponTime = 1000;
	if (self->client->ps.forcePowersActive & 1 << FP_SPEED)
	{
		self->client->ps.weaponTime = floor(self->client->ps.weaponTime * g_timescale->value);
	}
}

void ForceInsanity(gentity_t* self)
{
	trace_t tr;
	vec3_t end, forward;
	qboolean targetLive = qfalse;

	if (self->health <= 0)
	{
		return;
	}
	if (!WP_ForcePowerUsable(self, FP_INSANITY, 0))
	{
		return;
	}

	if (self->client->ps.weaponTime >= 800)
	{
		//just did one!
		return;
	}
	if (self->client->ps.saberLockTime > level.time)
	{
		//FIXME: can this be a way to break out?
		return;
	}

	if (PM_InLedgeMove(self->client->ps.legsAnim))
	{
		return;
	}

	AngleVectors(self->client->ps.viewangles, forward, nullptr, nullptr);
	VectorNormalize(forward);
	VectorMA(self->client->renderInfo.eyePoint, 2048, forward, end);

	//Cause a distraction if enemy is not fighting
	gi.trace(&tr, self->client->renderInfo.eyePoint, vec3_origin, vec3_origin, end, self->s.number,
		MASK_OPAQUE | CONTENTS_BODY, static_cast<EG2_Collision>(0), 0);
	if (tr.entityNum == ENTITYNUM_NONE || tr.fraction == 1.0 || tr.allsolid || tr.startsolid)
	{
		return;
	}

	gentity_t* traceEnt = &g_entities[tr.entityNum];

	if (traceEnt->NPC && traceEnt->NPC->scriptFlags & SCF_NO_FORCE)
	{
		return;
	}

	if (traceEnt && traceEnt->client)
	{
		switch (traceEnt->client->NPC_class)
		{
		case CLASS_GALAKMECH: //cant grip him, he's in armor
		case CLASS_ATST: //much too big to grip!
			//no droids either
		case CLASS_PROBE:
		case CLASS_GONK:
		case CLASS_R2D2:
		case CLASS_R5D2:
		case CLASS_MARK1:
		case CLASS_MARK2:
		case CLASS_MOUSE:
		case CLASS_SEEKER:
		case CLASS_REMOTE:
		case CLASS_PROTOCOL:
		case CLASS_ASSASSIN_DROID:
		case CLASS_SABER_DROID:
		case CLASS_BOBAFETT:
			break;
		case CLASS_RANCOR:
			if (!(traceEnt->spawnflags & 1))
			{
				targetLive = qtrue;
			}
			break;
		default:
			targetLive = qtrue;
			break;
		}
	}
	if (targetLive
		&& traceEnt->NPC
		&& traceEnt->health > 0
		&& traceEnt->NPC->charmedTime < level.time
		&& traceEnt->NPC->confusionTime < level.time)
	{
		//hit an organic non-player
		int override = 0;
		if (traceEnt->NPC->scriptFlags & SCF_NO_MIND_TRICK)
		{
			if (traceEnt->client->NPC_class == CLASS_GALAKMECH)
			{
				G_AddVoiceEvent(traceEnt, Q_irand(EV_CONFUSE1, EV_CONFUSE3), Q_irand(3000, 5000));
			}
		}
		else if (traceEnt->s.weapon != WP_SABER && traceEnt->client->NPC_class != CLASS_REBORN)
		{
			//haha!  Jedi aren't easily confused!
			if (self->client->ps.forcePowerLevel[FP_INSANITY] > FORCE_LEVEL_2
				&& traceEnt->s.weapon != WP_NONE
				//don't charm people who aren't capable of fighting... like ugnaughts and droids, just confuse them
				&& traceEnt->client->NPC_class != CLASS_TUSKEN //don't charm them, just confuse them
				&& traceEnt->client->NPC_class != CLASS_NOGHRI //don't charm them, just confuse them
				&& !Pilot_AnyVehiclesRegistered()) //also, don't charm guys when bikes are near
			{
				//turn them to our side
				//if insanity 3 and aiming at an enemy need more force power
				override = 50;
				if (self->client->ps.forcePower < 50)
				{
					return;
				}
				if (traceEnt->enemy)
				{
					G_ClearEnemy(traceEnt);
				}
				if (traceEnt->NPC)
				{
					traceEnt->client->leader = self;
				}
				const team_t saveTeam = traceEnt->client->enemyTeam;
				traceEnt->client->enemyTeam = traceEnt->client->playerTeam;
				traceEnt->client->playerTeam = saveTeam;
				traceEnt->NPC->darkCharmedTime = level.time + insanityTime[self->client->ps.forcePowerLevel[
					FP_INSANITY]];

				if (traceEnt->ghoul2.size() && traceEnt->headBolt != -1)
				{
					G_PlayEffect(G_EffectIndex("force/drain_hand"), traceEnt->playerModel, traceEnt->headBolt,
						traceEnt->s.number, traceEnt->currentOrigin,
						insanityTime[self->client->ps.forcePowerLevel[FP_INSANITY]], qtrue);
				}
				if (PM_HasAnimation(traceEnt, BOTH_SONICPAIN_HOLD))
				{
					NPC_SetAnim(traceEnt, SETANIM_LEGS, BOTH_SONICPAIN_HOLD, SETANIM_FLAG_NORMAL);
					NPC_SetAnim(traceEnt, SETANIM_TORSO, BOTH_SONICPAIN_HOLD,
						SETANIM_FLAG_OVERRIDE | SETANIM_FLAG_HOLD);
					traceEnt->client->ps.torsoAnimTimer += 200;
					traceEnt->client->ps.weaponTime = traceEnt->client->ps.torsoAnimTimer;
				}
			}
			else
			{
				//just insanity them
				if (PM_HasAnimation(traceEnt, BOTH_SONICPAIN_HOLD))
				{
					NPC_SetAnim(traceEnt, SETANIM_LEGS, BOTH_SONICPAIN_HOLD, SETANIM_FLAG_NORMAL);
					NPC_SetAnim(traceEnt, SETANIM_TORSO, BOTH_SONICPAIN_HOLD,
						SETANIM_FLAG_OVERRIDE | SETANIM_FLAG_HOLD);
					traceEnt->client->ps.torsoAnimTimer += insanityTime[self->client->ps.forcePowerLevel[FP_INSANITY]];
					traceEnt->client->ps.weaponTime = traceEnt->client->ps.torsoAnimTimer;
				}
				traceEnt->NPC->insanityTime = level.time + insanityTime[self->client->ps.forcePowerLevel[FP_INSANITY]];
				//confused for 5-10 seconds
				if (traceEnt->enemy)
				{
					G_ClearEnemy(traceEnt);
				}
				if (traceEnt->ghoul2.size() && traceEnt->headBolt != -1)
				{
					G_PlayEffect(G_EffectIndex("force/drain_hand"), traceEnt->playerModel, traceEnt->headBolt,
						traceEnt->s.number, traceEnt->currentOrigin,
						insanityTime[self->client->ps.forcePowerLevel[FP_INSANITY]], qtrue);
				}
			}
		}
		else
		{
			NPC_Jedi_PlayConfusionSound(traceEnt);
		}

		G_Sound(self, G_SoundIndex("sound/weapons/force/pushyoda.mp3"));
		WP_ForcePowerStart(self, FP_INSANITY, override);
		vec3_t eyeDir;
		AngleVectors(traceEnt->client->renderInfo.eyeAngles, eyeDir, nullptr, nullptr);
		VectorNormalize(eyeDir);
		G_PlayEffect("force/drain_hand", traceEnt->client->renderInfo.eyePoint, eyeDir);

		NPC_SetAnim(self, SETANIM_TORSO, BOTH_MINDTRICK1,
			SETANIM_FLAG_OVERRIDE | SETANIM_FLAG_RESTART | SETANIM_FLAG_HOLD);
	}

	WP_ForcePowerStart(self, FP_INSANITY, 0);

	self->client->ps.saberMove = self->client->ps.saberBounceMove = LS_READY;
	//don't finish whatever saber anim you may have been in
	self->client->ps.saberBlocked = BLOCKED_NONE;
	self->client->ps.weaponTime = 1000;
	if (self->client->ps.forcePowersActive & 1 << FP_SPEED)
	{
		self->client->ps.weaponTime = floor(self->client->ps.weaponTime * g_timescale->value);
	}
}

void ForceBlinding(gentity_t* self)
{
	trace_t tr;
	vec3_t end, forward;
	qboolean targetLive = qfalse;

	if (self->health <= 0)
	{
		return;
	}
	if (!WP_ForcePowerUsable(self, FP_BLINDING, 0))
	{
		return;
	}

	if (self->client->ps.weaponTime >= 800)
	{
		//just did one!
		return;
	}
	if (self->client->ps.saberLockTime > level.time)
	{
		//FIXME: can this be a way to break out?
		return;
	}

	if (PM_InLedgeMove(self->client->ps.legsAnim))
	{
		return;
	}

	AngleVectors(self->client->ps.viewangles, forward, nullptr, nullptr);
	VectorNormalize(forward);
	VectorMA(self->client->renderInfo.eyePoint, 2048, forward, end);

	//Cause a distraction if enemy is not fighting
	gi.trace(&tr, self->client->renderInfo.eyePoint, vec3_origin, vec3_origin, end, self->s.number,
		MASK_OPAQUE | CONTENTS_BODY, static_cast<EG2_Collision>(0), 0);
	if (tr.entityNum == ENTITYNUM_NONE || tr.fraction == 1.0 || tr.allsolid || tr.startsolid)
	{
		return;
	}

	gentity_t* traceEnt = &g_entities[tr.entityNum];

	if (traceEnt->NPC && traceEnt->NPC->scriptFlags & SCF_NO_FORCE)
	{
		return;
	}

	if (traceEnt && traceEnt->client)
	{
		switch (traceEnt->client->NPC_class)
		{
		case CLASS_GALAKMECH: //cant grip him, he's in armor
		case CLASS_ATST: //much too big to grip!
			//no droids either
		case CLASS_PROBE:
		case CLASS_GONK:
		case CLASS_R2D2:
		case CLASS_R5D2:
		case CLASS_MARK1:
		case CLASS_MARK2:
		case CLASS_MOUSE:
		case CLASS_SEEKER:
		case CLASS_REMOTE:
		case CLASS_PROTOCOL:
		case CLASS_ASSASSIN_DROID:
		case CLASS_SABER_DROID:
		case CLASS_BOBAFETT:
			break;
		case CLASS_RANCOR:
			if (!(traceEnt->spawnflags & 1))
			{
				targetLive = qtrue;
			}
			break;
		default:
			targetLive = qtrue;
			break;
		}
	}
	if (targetLive
		&& traceEnt->NPC
		&& traceEnt->health > 0
		&& traceEnt->NPC->charmedTime < level.time
		&& traceEnt->NPC->confusionTime < level.time)
	{
		//hit an organic non-player
		int override = 0;
		if (traceEnt->NPC->scriptFlags & SCF_NO_MIND_TRICK)
		{
			if (traceEnt->client->NPC_class == CLASS_GALAKMECH)
			{
				G_AddVoiceEvent(traceEnt, Q_irand(EV_CONFUSE1, EV_CONFUSE3), Q_irand(3000, 5000));
			}
		}
		else if (traceEnt->s.weapon != WP_SABER && traceEnt->client->NPC_class != CLASS_REBORN)
		{
			//haha!  Jedi aren't easily confused!
			if (self->client->ps.forcePowerLevel[FP_BLINDING] > FORCE_LEVEL_2
				&& traceEnt->s.weapon != WP_NONE
				//don't charm people who aren't capable of fighting... like ugnaughts and droids, just confuse them
				&& traceEnt->client->NPC_class != CLASS_TUSKEN //don't charm them, just confuse them
				&& traceEnt->client->NPC_class != CLASS_NOGHRI //don't charm them, just confuse them
				&& !Pilot_AnyVehiclesRegistered()) //also, don't charm guys when bikes are near
			{
				//turn them to our side
				//if insanity 3 and aiming at an enemy need more force power
				override = 50;
				if (self->client->ps.forcePower < 50)
				{
					return;
				}
				if (traceEnt->enemy)
				{
					G_ClearEnemy(traceEnt);
				}
				if (traceEnt->NPC)
				{
					traceEnt->client->leader = self;
				}
				const team_t saveTeam = traceEnt->client->enemyTeam;
				traceEnt->client->enemyTeam = traceEnt->client->playerTeam;
				traceEnt->client->playerTeam = saveTeam;
				traceEnt->NPC->darkCharmedTime = level.time + insanityTime[self->client->ps.forcePowerLevel[
					FP_BLINDING]];

				if (traceEnt->ghoul2.size() && traceEnt->headBolt != -1)
				{
					G_PlayEffect(G_EffectIndex("force/invin"), traceEnt->playerModel, traceEnt->headBolt,
						traceEnt->s.number, traceEnt->currentOrigin,
						insanityTime[self->client->ps.forcePowerLevel[FP_BLINDING]], qtrue);
				}
				if (PM_HasAnimation(traceEnt, BOTH_SONICPAIN_HOLD))
				{
					NPC_SetAnim(traceEnt, SETANIM_LEGS, BOTH_SONICPAIN_HOLD, SETANIM_FLAG_NORMAL);
					NPC_SetAnim(traceEnt, SETANIM_TORSO, BOTH_SONICPAIN_HOLD,
						SETANIM_FLAG_OVERRIDE | SETANIM_FLAG_HOLD);
					traceEnt->client->ps.torsoAnimTimer += 200;
					traceEnt->client->ps.weaponTime = traceEnt->client->ps.torsoAnimTimer;
				}
			}
			else
			{
				//just insanity them
				if (PM_HasAnimation(traceEnt, BOTH_SONICPAIN_HOLD))
				{
					NPC_SetAnim(traceEnt, SETANIM_LEGS, BOTH_SONICPAIN_HOLD, SETANIM_FLAG_NORMAL);
					NPC_SetAnim(traceEnt, SETANIM_TORSO, BOTH_SONICPAIN_HOLD,
						SETANIM_FLAG_OVERRIDE | SETANIM_FLAG_HOLD);
					traceEnt->client->ps.torsoAnimTimer += insanityTime[self->client->ps.forcePowerLevel[FP_BLINDING]];
					traceEnt->client->ps.weaponTime = traceEnt->client->ps.torsoAnimTimer;
				}
				traceEnt->NPC->insanityTime = level.time + insanityTime[self->client->ps.forcePowerLevel[FP_BLINDING]];
				//confused for 5-10 seconds
				if (traceEnt->enemy)
				{
					G_ClearEnemy(traceEnt);
				}
				if (traceEnt->ghoul2.size() && traceEnt->headBolt != -1)
				{
					G_PlayEffect(G_EffectIndex("force/invin"), traceEnt->playerModel, traceEnt->headBolt,
						traceEnt->s.number, traceEnt->currentOrigin,
						insanityTime[self->client->ps.forcePowerLevel[FP_BLINDING]], qtrue);
				}
			}
		}
		else
		{
			NPC_Jedi_PlayConfusionSound(traceEnt);
		}

		G_Sound(self, G_SoundIndex("sound/weapons/force/fear.mp3"));
		WP_ForcePowerStart(self, FP_BLINDING, override);
		vec3_t eyeDir;
		AngleVectors(traceEnt->client->renderInfo.eyeAngles, eyeDir, nullptr, nullptr);
		VectorNormalize(eyeDir);
		G_PlayEffect("force/invin", traceEnt->client->renderInfo.eyePoint, eyeDir);

		NPC_SetAnim(self, SETANIM_TORSO, BOTH_MINDTRICK1,
			SETANIM_FLAG_OVERRIDE | SETANIM_FLAG_RESTART | SETANIM_FLAG_HOLD);
	}

	WP_ForcePowerStart(self, FP_BLINDING, 0);

	self->client->ps.saberMove = self->client->ps.saberBounceMove = LS_READY;
	//don't finish whatever saber anim you may have been in
	self->client->ps.saberBlocked = BLOCKED_NONE;
	self->client->ps.weaponTime = 1000;
	if (self->client->ps.forcePowersActive & 1 << FP_SPEED)
	{
		self->client->ps.weaponTime = floor(self->client->ps.weaponTime * g_timescale->value);
	}
}

qboolean CanBeFeared(const gentity_t* self, const gentity_t* traceEnt)
{
	int commonFSChance = 0;
	int powerfulFSChance = 0;

	// Very weak force fear should pose almost zero threat against any jedi and sith
	if (self->client->ps.forcePowerLevel[FP_FEAR] < FORCE_LEVEL_2)
	{
		commonFSChance = 1000;
		powerfulFSChance = 1000;
	}
	// An intermediate Force Fear might pose some threat against lesser Jedi and Sith, but powerful ones should have next to no chance to be affected (but still a chance)
	else if (self->client->ps.forcePowerLevel[FP_FEAR] == FORCE_LEVEL_2)
	{
		commonFSChance = 20;
		powerfulFSChance = 100;
	}
	// A master of Force Fear should be able to scare a common Jedi or Sith with minimal effort, and still heavily struggle against the powerful ones.
	else if (self->client->ps.forcePowerLevel[FP_FEAR] > FORCE_LEVEL_2)
	{
		commonFSChance = 1;
		powerfulFSChance = 20;
	}

	if (Q_irand(0, commonFSChance) && (traceEnt->client->NPC_class == CLASS_JEDI
		|| traceEnt->client->NPC_class == CLASS_REBORN
		|| traceEnt->client->NPC_class == CLASS_SHADOWTROOPER))
	{
		return qfalse;
	}
	// Boss NPCs should put up significant resistance, you'd have to be very extremely lucky to affect one
	if (Q_irand(0, powerfulFSChance) &&
		(traceEnt->client->NPC_class == CLASS_KYLE
			|| traceEnt->client->NPC_class == CLASS_LUKE
			|| traceEnt->client->NPC_class == CLASS_TAVION
			|| traceEnt->client->NPC_class == CLASS_DESANN))
	{
		return qfalse;
	}

	if (self->client->ps.forcePowerLevel[FP_FEAR] == FORCE_LEVEL_1 && !Q_irand(0, 1))
	{
		return qfalse;
	}

	// All other NPCs should just be affected
	return qtrue;
}

constexpr auto STRIKE_DAMAGELOW = 5;
constexpr auto STRIKE_DAMAGEMEDIUM = 10;
constexpr auto STRIKE_DAMAGEHIGH = 15;

extern bool WP_MissileTargetHint(gentity_t* shooter, vec3_t start, vec3_t out);
extern qboolean LogAccuracyHit(const gentity_t* target, const gentity_t* attacker);
extern int G_GetHitLocFromTrace(trace_t* trace, int mod);

void ForceShootstrike(gentity_t* self)
{
	trace_t tr;
	vec3_t end, forward, right, up, dir;
	gentity_t* trace_ent;
	constexpr int damagelow = STRIKE_DAMAGELOW;
	constexpr int damageMedium = STRIKE_DAMAGEMEDIUM;
	constexpr int damageHigh = STRIKE_DAMAGEHIGH;
	qboolean render_impact = qtrue;
	vec3_t center, mins, maxs, ent_org, size, v;
	const float chanceOfFizz = Q_flrand(0.0f, 1.0f);
	constexpr int npcSaberNum = 0;
	constexpr int npcBladeNum = 0;
	// ReSharper disable once CppEntityAssignedButNoRead
	float dot;
	float dist;
	gentity_t* entityList[MAX_GENTITIES];
	int e, numListedEntities, i;

	const int hitLoc = G_GetHitLocFromTrace(&tr, MOD_LIGHTNING_STRIKE);

	if (self->health <= 0)
	{
		return;
	}
	if (!self->s.number && cg.zoomMode)
	{
		//can't force lightning when zoomed in
		return;
	}

	if (PM_InLedgeMove(self->client->ps.legsAnim))
	{
		return;
	}

	AngleVectors(self->client->ps.viewangles, forward, nullptr, nullptr);
	VectorNormalize(forward);

	// always render a shot beam, doing this the old way because I don't much feel like overriding the effect.

	G_PlayEffect("env/yellow_lightning", self->client->renderInfo.handLPoint, forward);
	gentity_t* tent = G_TempEntity(tr.endpos, EV_LIGHTNING_STRIKE);
	tent->svFlags |= SVF_BROADCAST;

	if (self->client->ps.forcePowerLevel[FP_LIGHTNING_STRIKE] == FORCE_LEVEL_3)
	{
		constexpr float radiusHigh = 512;
		//arc
		VectorCopy(self->currentOrigin, center);

		for (i = 0; i < 3; i++)
		{
			mins[i] = center[i] - radiusHigh;
			maxs[i] = center[i] + radiusHigh;
		}
		numListedEntities = gi.EntitiesInBox(mins, maxs, entityList, MAX_GENTITIES);

		for (e = 0; e < numListedEntities; e++)
		{
			trace_ent = entityList[e];

			if (!trace_ent)
				continue;
			if (trace_ent == self)
				continue;
			if (trace_ent->owner == self && trace_ent->s.weapon != WP_THERMAL) //can push your own thermals
				continue;
			if (!trace_ent->inuse)
				continue;
			if (!trace_ent->takedamage)
				continue;
			for (i = 0; i < 3; i++)
			{
				if (center[i] < trace_ent->absmin[i])
				{
					v[i] = trace_ent->absmin[i] - center[i];
				}
				else if (center[i] > trace_ent->absmax[i])
				{
					v[i] = center[i] - trace_ent->absmax[i];
				}
				else
				{
					v[i] = 0;
				}
			}

			VectorSubtract(trace_ent->absmax, trace_ent->absmin, size);
			VectorMA(trace_ent->absmin, 0.5, size, ent_org);

			//see if they're in front of me
			//must be within the forward cone
			VectorSubtract(ent_org, center, dir);
			VectorNormalize(dir);
			if ((dot = DotProduct(dir, forward)) < 0.5)
				continue;

			//must be close enough
			dist = VectorLength(v);
			if (dist >= radiusHigh)
			{
				continue;
			}

			//in PVS?
			if (!trace_ent->bmodel && !gi.inPVS(ent_org, self->client->renderInfo.handLPoint))
			{
				//must be in PVS
				continue;
			}

			//Now check and see if we can actually hit it
			gi.trace(&tr, self->client->renderInfo.handLPoint, vec3_origin, vec3_origin, ent_org, self->s.number,
				MASK_SHOT, static_cast<EG2_Collision>(0), 0);

			if (tr.surfaceFlags & SURF_NOIMPACT)
			{
				render_impact = qfalse;
			}

			if (tr.fraction < 1.0f && tr.entityNum != trace_ent->s.number)
			{
				//must have clear LOS
				continue;
			}

			if (render_impact)
			{
				if (tr.entityNum < ENTITYNUM_WORLD && trace_ent->takedamage)
				{
					// Create a simple impact type mark that doesn't last long in the world
					G_PlayEffect(G_EffectIndex("tusken/hit"), tr.endpos, tr.plane.normal);

					if (trace_ent->client && LogAccuracyHit(trace_ent, self))
					{
						self->client->ps.persistant[PERS_ACCURACY_HITS]++;
					}

					if (trace_ent->client && trace_ent->client->ps.powerups[PW_CLOAKED])
					{
						//disable cloak temporarily
						player_Decloak(trace_ent);
						G_AddVoiceEvent(trace_ent, Q_irand(EV_ANGER1, EV_ANGER3), 1000);
					}

					if (trace_ent && trace_ent->client && trace_ent->client->NPC_class == CLASS_GALAKMECH)
					{
						//hehe
						if (trace_ent->client->ps.stats[STAT_ARMOR] > 1)
						{
							trace_ent->client->ps.stats[STAT_ARMOR] = 0;
						}
						trace_ent->client->ps.powerups[PW_STUNNED] = level.time + 4000;
						G_Damage(trace_ent, self, self, dir, tr.endpos, damageHigh, DAMAGE_DEATH_KNOCKBACK,
							MOD_LIGHTNING_STRIKE, hitLoc);
					}
					else if (trace_ent && trace_ent->client && trace_ent->client->NPC_class == CLASS_OBJECT)
					{
						trace_ent->client->ps.powerups[PW_STUNNED] = level.time + 4000;
					}
					else
					{
						if (trace_ent && trace_ent->client &&
							(trace_ent->client->NPC_class == CLASS_ATST
								|| trace_ent->client->NPC_class == CLASS_GONK
								|| trace_ent->client->NPC_class == CLASS_INTERROGATOR
								|| trace_ent->client->NPC_class == CLASS_MARK1
								|| trace_ent->client->NPC_class == CLASS_MARK2
								|| trace_ent->client->NPC_class == CLASS_MOUSE
								|| trace_ent->client->NPC_class == CLASS_PROBE
								|| trace_ent->client->NPC_class == CLASS_PROTOCOL
								|| trace_ent->client->NPC_class == CLASS_R2D2
								|| trace_ent->client->NPC_class == CLASS_R5D2
								|| trace_ent->client->NPC_class == CLASS_SEEKER
								|| trace_ent->client->NPC_class == CLASS_SENTRY
								|| trace_ent->client->NPC_class == CLASS_SBD
								|| trace_ent->client->NPC_class == CLASS_BATTLEDROID
								|| trace_ent->client->NPC_class == CLASS_DROIDEKA
								|| trace_ent->client->NPC_class == CLASS_OBJECT
								|| trace_ent->client->NPC_class == CLASS_ASSASSIN_DROID
								|| trace_ent->client->NPC_class == CLASS_SABER_DROID))
						{
							// special droid only behaviors
							trace_ent->client->ps.powerups[PW_STUNNED] = level.time + 4000;
							G_Damage(trace_ent, self, self, dir, tr.endpos, damageHigh, DAMAGE_DEATH_KNOCKBACK,
								MOD_LIGHTNING_STRIKE, hitLoc);
						}
						else if (trace_ent && trace_ent->client && trace_ent->client->NPC_class == CLASS_BOBAFETT ||
							trace_ent && trace_ent->client && trace_ent->client->NPC_class == CLASS_MANDO)
						{
							//he doesn't drop them, just puts it away
							if (trace_ent->health <= 50)
							{
								ChangeWeapon(trace_ent, WP_MELEE);
								G_Knockdown(trace_ent, self, dir, 80, qtrue);
							}
							else
							{
								G_Stagger(trace_ent);
							}
							Boba_FlyStop(trace_ent);
							if (trace_ent->client->jetPackOn)
							{
								//disable jetpack temporarily
								Jetpack_Off(trace_ent);
								trace_ent->client->jetPackToggleTime = level.time + Q_irand(3000, 10000);
							}
							trace_ent->client->ps.powerups[PW_STUNNED] = level.time + 4000;
							G_Damage(trace_ent, self, self, dir, tr.endpos, damageHigh, DAMAGE_DEATH_KNOCKBACK,
								MOD_LIGHTNING_STRIKE, hitLoc);
						}
						else if (trace_ent && trace_ent->client && trace_ent->s.weapon == WP_MELEE)
						{
							//they can't take that away from me, oh no...
							trace_ent->client->ps.powerups[PW_STUNNED] = level.time + 4000;
							G_Damage(trace_ent, self, self, dir, tr.endpos, damageHigh, DAMAGE_DEATH_KNOCKBACK,
								MOD_LIGHTNING_STRIKE, hitLoc);
							if (trace_ent->health <= 50)
							{
								G_Knockdown(trace_ent, self, dir, 80, qtrue);
							}
							else
							{
								G_Stagger(trace_ent);
							}
						}
						else if (trace_ent && trace_ent->client
							&& (trace_ent->s.weapon == WP_BLASTER_PISTOL
								|| trace_ent->s.weapon == WP_BLASTER
								|| trace_ent->s.weapon == WP_DISRUPTOR
								|| trace_ent->s.weapon == WP_BOWCASTER
								|| trace_ent->s.weapon == WP_REPEATER
								|| trace_ent->s.weapon == WP_DEMP2
								|| trace_ent->s.weapon == WP_FLECHETTE
								|| trace_ent->s.weapon == WP_ROCKET_LAUNCHER
								|| trace_ent->s.weapon == WP_THERMAL
								|| trace_ent->s.weapon == WP_TRIP_MINE
								|| trace_ent->s.weapon == WP_DET_PACK
								|| trace_ent->s.weapon == WP_CONCUSSION
								|| trace_ent->s.weapon == WP_BRYAR_PISTOL
								|| trace_ent->s.weapon == WP_SBD_PISTOL
								|| trace_ent->s.weapon == WP_WRIST_BLASTER
								|| trace_ent->s.weapon == WP_JAWA
								|| trace_ent->s.weapon == WP_TUSKEN_RIFLE
								|| trace_ent->s.weapon == WP_TUSKEN_STAFF
								|| trace_ent->s.weapon == WP_SCEPTER
								|| trace_ent->s.weapon == WP_NOGHRI_STICK))
						{
							if (trace_ent->health <= 50)
							{
								WP_DropWeapon(trace_ent, nullptr);
								G_Knockdown(trace_ent, self, dir, 80, qtrue);
							}
							else
							{
								G_Stagger(trace_ent);
							}
							trace_ent->client->ps.powerups[PW_STUNNED] = level.time + 4000;
							G_Damage(trace_ent, self, self, dir, tr.endpos, damageHigh, DAMAGE_DEATH_KNOCKBACK,
								MOD_LIGHTNING_STRIKE, hitLoc);
						}
						else
						{
							if (trace_ent->s.weapon == WP_SABER
								&& trace_ent->client->ps.SaberActive()
								&& !trace_ent->client->ps.saberInFlight
								&& InFOV(self->currentOrigin, trace_ent->currentOrigin,
									trace_ent->client->ps.viewangles, 20, 35)
								&& !PM_InKnockDown(&trace_ent->client->ps)
								&& !PM_SuperBreakLoseAnim(trace_ent->client->ps.torsoAnim)
								&& !PM_SuperBreakWinAnim(trace_ent->client->ps.torsoAnim)
								&& !pm_saber_in_special_attack(trace_ent->client->ps.torsoAnim)
								&& !PM_InSpecialJump(trace_ent->client->ps.torsoAnim)
								&& manual_saberblocking(trace_ent))
							{
								//saber can block lightning make them do a parry
								VectorNegate(dir, forward);

								//randomise direction a bit
								MakeNormalVectors(forward, right, up);
								VectorMA(forward, Q_irand(0, 360), right, forward);
								VectorMA(forward, Q_irand(0, 360), up, forward);
								VectorNormalize(forward);

								if (chanceOfFizz > 0)
								{
									VectorMA(trace_ent->client->ps.saber[npcSaberNum].blade[npcBladeNum].muzzlePoint,
										trace_ent->client->ps.saber[npcSaberNum].blade[npcBladeNum].length *
										Q_flrand(0, 1),
										trace_ent->client->ps.saber[npcSaberNum].blade[npcBladeNum].muzzleDir,
										end);
									G_PlayEffect(G_EffectIndex("saber/fizz.efx"), end, forward);
								}

								switch (trace_ent->client->ps.saberAnimLevel)
								{
								case SS_DUAL:
									NPC_SetAnim(trace_ent, SETANIM_TORSO, BOTH_P6_S6_T_, SETANIM_AFLAG_PACE);
									break;
								case SS_STAFF:
									NPC_SetAnim(trace_ent, SETANIM_TORSO, BOTH_P7_S7_T_, SETANIM_AFLAG_PACE);
									break;
								case SS_FAST:
									NPC_SetAnim(trace_ent, SETANIM_TORSO, BOTH_P1_S1_T_, SETANIM_AFLAG_PACE);
									break;
								case SS_TAVION:
									NPC_SetAnim(trace_ent, SETANIM_TORSO, BOTH_P1_S1_T_, SETANIM_AFLAG_PACE);
									break;
								case SS_STRONG:
									NPC_SetAnim(trace_ent, SETANIM_TORSO, BOTH_P1_S1_T_, SETANIM_AFLAG_PACE);
									break;
								case SS_DESANN:
									NPC_SetAnim(trace_ent, SETANIM_TORSO, BOTH_P1_S1_T_, SETANIM_AFLAG_PACE);
									break;
								case SS_MEDIUM:
									NPC_SetAnim(trace_ent, SETANIM_TORSO, BOTH_P1_S1_T_, SETANIM_AFLAG_PACE);
									break;
								case SS_NONE:
								default:
									NPC_SetAnim(trace_ent, SETANIM_TORSO, BOTH_P1_S1_T_, SETANIM_AFLAG_PACE);
									break;
								}
								trace_ent->client->ps.weaponTime = Q_irand(300, 600);
							}
							else
							{
								if (trace_ent && trace_ent->client)
								{
									trace_ent->client->ps.powerups[PW_STUNNED] = level.time + 4000;

									G_Damage(trace_ent, self, self, dir, tr.endpos, damageHigh, DAMAGE_DEATH_KNOCKBACK,
										MOD_LIGHTNING_STRIKE, hitLoc);
									if (trace_ent->health <= 50 && trace_ent->s.weapon == WP_SABER)
									{
										//turn it off?
										if (trace_ent->client->ps.SaberActive())
										{
											trace_ent->client->ps.SaberDeactivate();
											G_SoundOnEnt(trace_ent, CHAN_WEAPON, "sound/weapons/saber/saberoffquick.mp3");
										}
										ChangeWeapon(trace_ent, WP_MELEE);
										G_Knockdown(trace_ent, self, dir, 80, qtrue);
									}
									else
									{
										G_Stagger(trace_ent);
									}
								}
							}
						}
					}
				}
				else
				{
					G_PlayEffect(G_EffectIndex("tusken/hitwall"), tr.endpos, tr.plane.normal);
				}
			}
		}
	}
	else if (self->client->ps.forcePowerLevel[FP_LIGHTNING_STRIKE] == FORCE_LEVEL_2)
	{
		constexpr float radiusmedium = 256;
		//arc
		VectorCopy(self->currentOrigin, center);

		for (i = 0; i < 3; i++)
		{
			mins[i] = center[i] - radiusmedium;
			maxs[i] = center[i] + radiusmedium;
		}
		numListedEntities = gi.EntitiesInBox(mins, maxs, entityList, MAX_GENTITIES);

		for (e = 0; e < numListedEntities; e++)
		{
			trace_ent = entityList[e];

			if (!trace_ent)
				continue;
			if (trace_ent == self)
				continue;
			if (trace_ent->owner == self && trace_ent->s.weapon != WP_THERMAL) //can push your own thermals
				continue;
			if (!trace_ent->inuse)
				continue;
			if (!trace_ent->takedamage)
				continue;
			for (i = 0; i < 3; i++)
			{
				if (center[i] < trace_ent->absmin[i])
				{
					v[i] = trace_ent->absmin[i] - center[i];
				}
				else if (center[i] > trace_ent->absmax[i])
				{
					v[i] = center[i] - trace_ent->absmax[i];
				}
				else
				{
					v[i] = 0;
				}
			}

			VectorSubtract(trace_ent->absmax, trace_ent->absmin, size);
			VectorMA(trace_ent->absmin, 0.5, size, ent_org);

			//see if they're in front of me
			//must be within the forward cone
			VectorSubtract(ent_org, center, dir);
			VectorNormalize(dir);
			if ((dot = DotProduct(dir, forward)) < 0.5)
				continue;

			//must be close enough
			dist = VectorLength(v);
			if (dist >= radiusmedium)
			{
				continue;
			}

			//in PVS?
			if (!trace_ent->bmodel && !gi.inPVS(ent_org, self->client->renderInfo.handLPoint))
			{
				//must be in PVS
				continue;
			}

			//Now check and see if we can actually hit it
			gi.trace(&tr, self->client->renderInfo.handLPoint, vec3_origin, vec3_origin, ent_org, self->s.number,
				MASK_SHOT, static_cast<EG2_Collision>(0), 0);

			if (tr.surfaceFlags & SURF_NOIMPACT)
			{
				render_impact = qfalse;
			}

			if (tr.fraction < 1.0f && tr.entityNum != trace_ent->s.number)
			{
				//must have clear LOS
				continue;
			}

			if (render_impact)
			{
				if (tr.entityNum < ENTITYNUM_WORLD && trace_ent->takedamage)
				{
					// Create a simple impact type mark that doesn't last long in the world
					G_PlayEffect(G_EffectIndex("tusken/hit"), tr.endpos, tr.plane.normal);

					if (trace_ent->client && LogAccuracyHit(trace_ent, self))
					{
						self->client->ps.persistant[PERS_ACCURACY_HITS]++;
					}

					if (trace_ent->client && trace_ent->client->ps.powerups[PW_CLOAKED])
					{
						//disable cloak temporarily
						player_Decloak(trace_ent);
						G_AddVoiceEvent(trace_ent, Q_irand(EV_ANGER1, EV_ANGER3), 1000);
					}

					if (trace_ent && trace_ent->client && trace_ent->client->NPC_class == CLASS_GALAKMECH)
					{
						//hehe
						if (trace_ent->client->ps.stats[STAT_ARMOR] > 1)
						{
							trace_ent->client->ps.stats[STAT_ARMOR] = 0;
						}
						trace_ent->client->ps.powerups[PW_STUNNED] = level.time + 2000;
						G_Damage(trace_ent, self, self, dir, tr.endpos, damageMedium, DAMAGE_DEATH_KNOCKBACK,
							MOD_LIGHTNING_STRIKE, hitLoc);
					}
					else if (trace_ent && trace_ent->client && trace_ent->client->NPC_class == CLASS_OBJECT)
					{
						trace_ent->client->ps.powerups[PW_STUNNED] = level.time + 2000;
					}
					else
					{
						if (trace_ent && trace_ent->client &&
							(trace_ent->client->NPC_class == CLASS_ATST
								|| trace_ent->client->NPC_class == CLASS_GONK
								|| trace_ent->client->NPC_class == CLASS_INTERROGATOR
								|| trace_ent->client->NPC_class == CLASS_MARK1
								|| trace_ent->client->NPC_class == CLASS_MARK2
								|| trace_ent->client->NPC_class == CLASS_MOUSE
								|| trace_ent->client->NPC_class == CLASS_PROBE
								|| trace_ent->client->NPC_class == CLASS_PROTOCOL
								|| trace_ent->client->NPC_class == CLASS_R2D2
								|| trace_ent->client->NPC_class == CLASS_R5D2
								|| trace_ent->client->NPC_class == CLASS_SEEKER
								|| trace_ent->client->NPC_class == CLASS_SENTRY
								|| trace_ent->client->NPC_class == CLASS_SBD
								|| trace_ent->client->NPC_class == CLASS_BATTLEDROID
								|| trace_ent->client->NPC_class == CLASS_DROIDEKA
								|| trace_ent->client->NPC_class == CLASS_OBJECT
								|| trace_ent->client->NPC_class == CLASS_ASSASSIN_DROID
								|| trace_ent->client->NPC_class == CLASS_SABER_DROID))
						{
							// special droid only behaviors
							trace_ent->client->ps.powerups[PW_STUNNED] = level.time + 2000;
							G_Damage(trace_ent, self, self, dir, tr.endpos, damageMedium, DAMAGE_DEATH_KNOCKBACK,
								MOD_LIGHTNING_STRIKE, hitLoc);
						}
						else if (trace_ent && trace_ent->client && trace_ent->client->NPC_class == CLASS_BOBAFETT ||
							trace_ent && trace_ent->client && trace_ent->client->NPC_class == CLASS_MANDO)
						{
							//he doesn't drop them, just puts it away
							if (trace_ent->health <= 50)
							{
								ChangeWeapon(trace_ent, WP_MELEE);
								G_Knockdown(trace_ent, self, dir, 80, qtrue);
							}
							else
							{
								G_Stagger(trace_ent);
							}
							Boba_FlyStop(trace_ent);
							if (trace_ent->client->jetPackOn)
							{
								//disable jetpack temporarily
								Jetpack_Off(trace_ent);
								trace_ent->client->jetPackToggleTime = level.time + Q_irand(3000, 10000);
							}
							trace_ent->client->ps.powerups[PW_STUNNED] = level.time + 2000;
							G_Damage(trace_ent, self, self, dir, tr.endpos, damageMedium, DAMAGE_DEATH_KNOCKBACK,
								MOD_LIGHTNING_STRIKE, hitLoc);
						}
						else if (trace_ent->s.weapon == WP_MELEE)
						{
							//they can't take that away from me, oh no...
							trace_ent->client->ps.powerups[PW_STUNNED] = level.time + 2000;
							G_Damage(trace_ent, self, self, dir, tr.endpos, damageMedium, DAMAGE_DEATH_KNOCKBACK,
								MOD_LIGHTNING_STRIKE, hitLoc);
							if (trace_ent->health <= 50)
							{
								G_Knockdown(trace_ent, self, dir, 80, qtrue);
							}
							else
							{
								G_Stagger(trace_ent);
							}
						}
						else if (trace_ent && trace_ent->client
							&& (trace_ent->s.weapon == WP_BLASTER_PISTOL
								|| trace_ent->s.weapon == WP_BLASTER
								|| trace_ent->s.weapon == WP_DISRUPTOR
								|| trace_ent->s.weapon == WP_BOWCASTER
								|| trace_ent->s.weapon == WP_REPEATER
								|| trace_ent->s.weapon == WP_DEMP2
								|| trace_ent->s.weapon == WP_FLECHETTE
								|| trace_ent->s.weapon == WP_ROCKET_LAUNCHER
								|| trace_ent->s.weapon == WP_THERMAL
								|| trace_ent->s.weapon == WP_TRIP_MINE
								|| trace_ent->s.weapon == WP_DET_PACK
								|| trace_ent->s.weapon == WP_CONCUSSION
								|| trace_ent->s.weapon == WP_BRYAR_PISTOL
								|| trace_ent->s.weapon == WP_SBD_PISTOL
								|| trace_ent->s.weapon == WP_WRIST_BLASTER
								|| trace_ent->s.weapon == WP_JAWA
								|| trace_ent->s.weapon == WP_TUSKEN_RIFLE
								|| trace_ent->s.weapon == WP_TUSKEN_STAFF
								|| trace_ent->s.weapon == WP_SCEPTER
								|| trace_ent->s.weapon == WP_NOGHRI_STICK))
						{
							if (trace_ent->health <= 50)
							{
								WP_DropWeapon(trace_ent, nullptr);
								G_Knockdown(trace_ent, self, dir, 80, qtrue);
							}
							else
							{
								G_Stagger(trace_ent);
							}
							trace_ent->client->ps.powerups[PW_STUNNED] = level.time + 2000;
							G_Damage(trace_ent, self, self, dir, tr.endpos, damageMedium, DAMAGE_DEATH_KNOCKBACK,
								MOD_LIGHTNING_STRIKE, hitLoc);
						}
						else
						{
							if (trace_ent->s.weapon == WP_SABER
								&& trace_ent->client->ps.SaberActive()
								&& !trace_ent->client->ps.saberInFlight
								&& InFOV(self->currentOrigin, trace_ent->currentOrigin,
									trace_ent->client->ps.viewangles, 20, 35)
								&& !PM_InKnockDown(&trace_ent->client->ps)
								&& !PM_SuperBreakLoseAnim(trace_ent->client->ps.torsoAnim)
								&& !PM_SuperBreakWinAnim(trace_ent->client->ps.torsoAnim)
								&& !pm_saber_in_special_attack(trace_ent->client->ps.torsoAnim)
								&& !PM_InSpecialJump(trace_ent->client->ps.torsoAnim)
								&& manual_saberblocking(trace_ent))
							{
								//saber can block lightning make them do a parry
								VectorNegate(dir, forward);

								//randomise direction a bit
								MakeNormalVectors(forward, right, up);
								VectorMA(forward, Q_irand(0, 360), right, forward);
								VectorMA(forward, Q_irand(0, 360), up, forward);
								VectorNormalize(forward);

								if (chanceOfFizz > 0)
								{
									VectorMA(trace_ent->client->ps.saber[npcSaberNum].blade[npcBladeNum].muzzlePoint,
										trace_ent->client->ps.saber[npcSaberNum].blade[npcBladeNum].length *
										Q_flrand(0, 1),
										trace_ent->client->ps.saber[npcSaberNum].blade[npcBladeNum].muzzleDir,
										end);
									G_PlayEffect(G_EffectIndex("saber/fizz.efx"), end, forward);
								}

								switch (trace_ent->client->ps.saberAnimLevel)
								{
								case SS_DUAL:
									NPC_SetAnim(trace_ent, SETANIM_TORSO, BOTH_P6_S6_T_, SETANIM_AFLAG_PACE);
									break;
								case SS_STAFF:
									NPC_SetAnim(trace_ent, SETANIM_TORSO, BOTH_P7_S7_T_, SETANIM_AFLAG_PACE);
									break;
								case SS_FAST:
									NPC_SetAnim(trace_ent, SETANIM_TORSO, BOTH_P1_S1_T_, SETANIM_AFLAG_PACE);
									break;
								case SS_TAVION:
									NPC_SetAnim(trace_ent, SETANIM_TORSO, BOTH_P1_S1_T_, SETANIM_AFLAG_PACE);
									break;
								case SS_STRONG:
									NPC_SetAnim(trace_ent, SETANIM_TORSO, BOTH_P1_S1_T_, SETANIM_AFLAG_PACE);
									break;
								case SS_DESANN:
									NPC_SetAnim(trace_ent, SETANIM_TORSO, BOTH_P1_S1_T_, SETANIM_AFLAG_PACE);
									break;
								case SS_MEDIUM:
									NPC_SetAnim(trace_ent, SETANIM_TORSO, BOTH_P1_S1_T_, SETANIM_AFLAG_PACE);
									break;
								case SS_NONE:
								default:
									NPC_SetAnim(trace_ent, SETANIM_TORSO, BOTH_P1_S1_T_, SETANIM_AFLAG_PACE);
									break;
								}
								trace_ent->client->ps.weaponTime = Q_irand(300, 600);
							}
							else
							{
								if (trace_ent && trace_ent->client)
								{
									trace_ent->client->ps.powerups[PW_STUNNED] = level.time + 4000;

									G_Damage(trace_ent, self, self, dir, tr.endpos, damageMedium, DAMAGE_DEATH_KNOCKBACK,
										MOD_LIGHTNING_STRIKE, hitLoc);
									if (trace_ent->health <= 50 && trace_ent->s.weapon == WP_SABER)
									{
										//turn it off?
										if (trace_ent->client->ps.SaberActive())
										{
											trace_ent->client->ps.SaberDeactivate();
											G_SoundOnEnt(trace_ent, CHAN_WEAPON, "sound/weapons/saber/saberoffquick.mp3");
										}
										ChangeWeapon(trace_ent, WP_MELEE);
										G_Knockdown(trace_ent, self, dir, 80, qtrue);
									}
									else
									{
										G_Stagger(trace_ent);
									}
								}
							}
						}
					}
				}
				else
				{
					G_PlayEffect(G_EffectIndex("tusken/hitwall"), tr.endpos, tr.plane.normal);
				}
			}
		}
	}
	else
	{
		constexpr float radiuslow = 128;
		//arc
		VectorCopy(self->currentOrigin, center);

		for (i = 0; i < 3; i++)
		{
			mins[i] = center[i] - radiuslow;
			maxs[i] = center[i] + radiuslow;
		}
		numListedEntities = gi.EntitiesInBox(mins, maxs, entityList, MAX_GENTITIES);

		for (e = 0; e < numListedEntities; e++)
		{
			trace_ent = entityList[e];

			if (!trace_ent)
				continue;
			if (trace_ent == self)
				continue;
			if (trace_ent->owner == self && trace_ent->s.weapon != WP_THERMAL) //can push your own thermals
				continue;
			if (!trace_ent->inuse)
				continue;
			if (!trace_ent->takedamage)
				continue;
			for (i = 0; i < 3; i++)
			{
				if (center[i] < trace_ent->absmin[i])
				{
					v[i] = trace_ent->absmin[i] - center[i];
				}
				else if (center[i] > trace_ent->absmax[i])
				{
					v[i] = center[i] - trace_ent->absmax[i];
				}
				else
				{
					v[i] = 0;
				}
			}

			VectorSubtract(trace_ent->absmax, trace_ent->absmin, size);
			VectorMA(trace_ent->absmin, 0.5, size, ent_org);

			//see if they're in front of me
			//must be within the forward cone
			VectorSubtract(ent_org, center, dir);
			VectorNormalize(dir);
			if ((dot = DotProduct(dir, forward)) < 0.5)
				continue;

			//must be close enough
			dist = VectorLength(v);
			if (dist >= radiuslow)
			{
				continue;
			}

			//in PVS?
			if (!trace_ent->bmodel && !gi.inPVS(ent_org, self->client->renderInfo.handLPoint))
			{
				//must be in PVS
				continue;
			}

			//Now check and see if we can actually hit it
			gi.trace(&tr, self->client->renderInfo.handLPoint, vec3_origin, vec3_origin, ent_org, self->s.number,
				MASK_SHOT, static_cast<EG2_Collision>(0), 0);

			if (tr.surfaceFlags & SURF_NOIMPACT)
			{
				render_impact = qfalse;
			}

			if (tr.fraction < 1.0f && tr.entityNum != trace_ent->s.number)
			{
				//must have clear LOS
				continue;
			}

			if (render_impact)
			{
				if (tr.entityNum < ENTITYNUM_WORLD && trace_ent->takedamage)
				{
					// Create a simple impact type mark that doesn't last long in the world
					G_PlayEffect(G_EffectIndex("tusken/hit"), tr.endpos, tr.plane.normal);

					if (trace_ent->client && LogAccuracyHit(trace_ent, self))
					{
						self->client->ps.persistant[PERS_ACCURACY_HITS]++;
					}

					if (trace_ent->client && trace_ent->client->ps.powerups[PW_CLOAKED])
					{
						//disable cloak temporarily
						player_Decloak(trace_ent);
						G_AddVoiceEvent(trace_ent, Q_irand(EV_ANGER1, EV_ANGER3), 1000);
					}

					if (trace_ent && trace_ent->client && trace_ent->client->NPC_class == CLASS_GALAKMECH)
					{
						//hehe
						if (trace_ent->client->ps.stats[STAT_ARMOR] > 1)
						{
							trace_ent->client->ps.stats[STAT_ARMOR] = 0;
						}
						trace_ent->client->ps.powerups[PW_STUNNED] = level.time + 1000;
						G_Damage(trace_ent, self, self, dir, tr.endpos, damagelow, DAMAGE_DEATH_KNOCKBACK,
							MOD_LIGHTNING_STRIKE, hitLoc);
					}
					else if (trace_ent && trace_ent->client && trace_ent->client->NPC_class == CLASS_OBJECT)
					{
						trace_ent->client->ps.powerups[PW_STUNNED] = level.time + 1000;
					}
					else
					{
						if (trace_ent && trace_ent->client &&
							(trace_ent->client->NPC_class == CLASS_ATST
								|| trace_ent->client->NPC_class == CLASS_GONK
								|| trace_ent->client->NPC_class == CLASS_INTERROGATOR
								|| trace_ent->client->NPC_class == CLASS_MARK1
								|| trace_ent->client->NPC_class == CLASS_MARK2
								|| trace_ent->client->NPC_class == CLASS_MOUSE
								|| trace_ent->client->NPC_class == CLASS_PROBE
								|| trace_ent->client->NPC_class == CLASS_PROTOCOL
								|| trace_ent->client->NPC_class == CLASS_R2D2
								|| trace_ent->client->NPC_class == CLASS_R5D2
								|| trace_ent->client->NPC_class == CLASS_SEEKER
								|| trace_ent->client->NPC_class == CLASS_SENTRY
								|| trace_ent->client->NPC_class == CLASS_SBD
								|| trace_ent->client->NPC_class == CLASS_BATTLEDROID
								|| trace_ent->client->NPC_class == CLASS_DROIDEKA
								|| trace_ent->client->NPC_class == CLASS_OBJECT
								|| trace_ent->client->NPC_class == CLASS_ASSASSIN_DROID
								|| trace_ent->client->NPC_class == CLASS_SABER_DROID))
						{
							// special droid only behaviors
							trace_ent->client->ps.powerups[PW_STUNNED] = level.time + 1000;
							G_Damage(trace_ent, self, self, dir, tr.endpos, damagelow, DAMAGE_DEATH_KNOCKBACK,
								MOD_LIGHTNING_STRIKE, hitLoc);
						}
						else if (trace_ent && trace_ent->client && trace_ent->client->NPC_class == CLASS_BOBAFETT ||
							trace_ent && trace_ent->client && trace_ent->client->NPC_class == CLASS_MANDO)
						{
							//he doesn't drop them, just puts it away
							if (trace_ent->health <= 50)
							{
								ChangeWeapon(trace_ent, WP_MELEE);
								G_Knockdown(trace_ent, self, dir, 80, qtrue);
							}
							else
							{
								G_Stagger(trace_ent);
							}
							Boba_FlyStop(trace_ent);
							if (trace_ent->client->jetPackOn)
							{
								//disable jetpack temporarily
								Jetpack_Off(trace_ent);
								trace_ent->client->jetPackToggleTime = level.time + Q_irand(3000, 10000);
							}
							trace_ent->client->ps.powerups[PW_STUNNED] = level.time + 1000;
							G_Damage(trace_ent, self, self, dir, tr.endpos, damagelow, DAMAGE_DEATH_KNOCKBACK,
								MOD_LIGHTNING_STRIKE, hitLoc);
						}
						else if (trace_ent->s.weapon == WP_MELEE)
						{
							//they can't take that away from me, oh no...
							trace_ent->client->ps.powerups[PW_STUNNED] = level.time + 1000;
							G_Damage(trace_ent, self, self, dir, tr.endpos, damagelow, DAMAGE_DEATH_KNOCKBACK,
								MOD_LIGHTNING_STRIKE, hitLoc);
							if (trace_ent->health <= 50)
							{
								G_Knockdown(trace_ent, self, dir, 80, qtrue);
							}
							else
							{
								G_Stagger(trace_ent);
							}
						}
						else if (trace_ent && trace_ent->client
							&& (trace_ent->s.weapon == WP_BLASTER_PISTOL
								|| trace_ent->s.weapon == WP_BLASTER
								|| trace_ent->s.weapon == WP_DISRUPTOR
								|| trace_ent->s.weapon == WP_BOWCASTER
								|| trace_ent->s.weapon == WP_REPEATER
								|| trace_ent->s.weapon == WP_DEMP2
								|| trace_ent->s.weapon == WP_FLECHETTE
								|| trace_ent->s.weapon == WP_ROCKET_LAUNCHER
								|| trace_ent->s.weapon == WP_THERMAL
								|| trace_ent->s.weapon == WP_TRIP_MINE
								|| trace_ent->s.weapon == WP_DET_PACK
								|| trace_ent->s.weapon == WP_CONCUSSION
								|| trace_ent->s.weapon == WP_BRYAR_PISTOL
								|| trace_ent->s.weapon == WP_SBD_PISTOL
								|| trace_ent->s.weapon == WP_WRIST_BLASTER
								|| trace_ent->s.weapon == WP_JAWA
								|| trace_ent->s.weapon == WP_TUSKEN_RIFLE
								|| trace_ent->s.weapon == WP_TUSKEN_STAFF
								|| trace_ent->s.weapon == WP_SCEPTER
								|| trace_ent->s.weapon == WP_NOGHRI_STICK))
						{
							if (trace_ent->health <= 50)
							{
								WP_DropWeapon(trace_ent, nullptr);
								G_Knockdown(trace_ent, self, dir, 80, qtrue);
							}
							else
							{
								G_Stagger(trace_ent);
							}
							trace_ent->client->ps.powerups[PW_STUNNED] = level.time + 1000;
							G_Damage(trace_ent, self, self, dir, tr.endpos, damagelow, DAMAGE_DEATH_KNOCKBACK,
								MOD_LIGHTNING_STRIKE, hitLoc);
						}
						else
						{
							if (trace_ent->s.weapon == WP_SABER
								&& trace_ent->client->ps.SaberActive()
								&& !trace_ent->client->ps.saberInFlight
								&& InFOV(self->currentOrigin, trace_ent->currentOrigin,
									trace_ent->client->ps.viewangles, 20, 35)
								&& !PM_InKnockDown(&trace_ent->client->ps)
								&& !PM_SuperBreakLoseAnim(trace_ent->client->ps.torsoAnim)
								&& !PM_SuperBreakWinAnim(trace_ent->client->ps.torsoAnim)
								&& !pm_saber_in_special_attack(trace_ent->client->ps.torsoAnim)
								&& !PM_InSpecialJump(trace_ent->client->ps.torsoAnim)
								&& manual_saberblocking(trace_ent))
							{
								//saber can block lightning make them do a parry
								VectorNegate(dir, forward);

								//randomise direction a bit
								MakeNormalVectors(forward, right, up);
								VectorMA(forward, Q_irand(0, 360), right, forward);
								VectorMA(forward, Q_irand(0, 360), up, forward);
								VectorNormalize(forward);

								if (chanceOfFizz > 0)
								{
									VectorMA(trace_ent->client->ps.saber[npcSaberNum].blade[npcBladeNum].muzzlePoint,
										trace_ent->client->ps.saber[npcSaberNum].blade[npcBladeNum].length *
										Q_flrand(0, 1),
										trace_ent->client->ps.saber[npcSaberNum].blade[npcBladeNum].muzzleDir,
										end);
									G_PlayEffect(G_EffectIndex("saber/fizz.efx"), end, forward);
								}

								switch (trace_ent->client->ps.saberAnimLevel)
								{
								case SS_DUAL:
									NPC_SetAnim(trace_ent, SETANIM_TORSO, BOTH_P6_S6_T_, SETANIM_AFLAG_PACE);
									break;
								case SS_STAFF:
									NPC_SetAnim(trace_ent, SETANIM_TORSO, BOTH_P7_S7_T_, SETANIM_AFLAG_PACE);
									break;
								case SS_FAST:
									NPC_SetAnim(trace_ent, SETANIM_TORSO, BOTH_P1_S1_T_, SETANIM_AFLAG_PACE);
									break;
								case SS_TAVION:
									NPC_SetAnim(trace_ent, SETANIM_TORSO, BOTH_P1_S1_T_, SETANIM_AFLAG_PACE);
									break;
								case SS_STRONG:
									NPC_SetAnim(trace_ent, SETANIM_TORSO, BOTH_P1_S1_T_, SETANIM_AFLAG_PACE);
									break;
								case SS_DESANN:
									NPC_SetAnim(trace_ent, SETANIM_TORSO, BOTH_P1_S1_T_, SETANIM_AFLAG_PACE);
									break;
								case SS_MEDIUM:
									NPC_SetAnim(trace_ent, SETANIM_TORSO, BOTH_P1_S1_T_, SETANIM_AFLAG_PACE);
									break;
								case SS_NONE:
								default:
									NPC_SetAnim(trace_ent, SETANIM_TORSO, BOTH_P1_S1_T_, SETANIM_AFLAG_PACE);
									break;
								}
								trace_ent->client->ps.weaponTime = Q_irand(300, 600);
							}
							else
							{
								if (trace_ent && trace_ent->client)
								{
									trace_ent->client->ps.powerups[PW_STUNNED] = level.time + 4000;

									G_Damage(trace_ent, self, self, dir, tr.endpos, damagelow, DAMAGE_DEATH_KNOCKBACK,
										MOD_LIGHTNING_STRIKE, hitLoc);
									if (trace_ent->health <= 50 && trace_ent->s.weapon == WP_SABER)
									{
										//turn it off?
										if (trace_ent->client->ps.SaberActive())
										{
											trace_ent->client->ps.SaberDeactivate();
											G_SoundOnEnt(trace_ent, CHAN_WEAPON, "sound/weapons/saber/saberoffquick.mp3");
										}
										ChangeWeapon(trace_ent, WP_MELEE);
										G_Knockdown(trace_ent, self, dir, 80, qtrue);
									}
									else
									{
										G_Stagger(trace_ent);
									}
								}
							}
						}
					}
				}
				else
				{
					G_PlayEffect(G_EffectIndex("tusken/hitwall"), tr.endpos, tr.plane.normal);
				}
			}
		}
	}
}

void ForceLightningStrike(gentity_t* self)
{
	if (self->health <= 0)
	{
		return;
	}
	if (!WP_ForcePowerUsable(self, FP_LIGHTNING_STRIKE, 0))
	{
		return;
	}
	if (self->client->ps.forcePowerDebounce[FP_LIGHTNING_STRIKE] > level.time)
	{
		//already using lightning strike
		return;
	}
	if (!self->s.number && (cg.zoomMode || in_camera))
	{
		//can't destruction when zoomed in or in cinematic
		return;
	}
	if (self->client->ps.saberLockTime > level.time)
	{
		//FIXME: can this be a way to break out?
		return;
	}

	if (PM_InLedgeMove(self->client->ps.legsAnim))
	{
		return;
	}

	constexpr int anim = BOTH_FORCELIGHTNING;
	const int soundIndex = G_SoundIndex("sound/weapons/force/strike.wav");

	int parts = SETANIM_TORSO;
	if (!PM_InKnockDown(&self->client->ps))
	{
		if (!VectorLengthSquared(self->client->ps.velocity) && !(self->client->ps.pm_flags & PMF_DUCKED))
		{
			parts = SETANIM_BOTH;
		}
	}
	NPC_SetAnim(self, parts, anim, SETANIM_FLAG_OVERRIDE | SETANIM_FLAG_HOLD | SETANIM_FLAG_RESTART);
	self->client->ps.saberMove = self->client->ps.saberBounceMove = LS_READY;
	//don't finish whatever saber anim you may have been in
	self->client->ps.saberBlocked = BLOCKED_NONE;

	if (self->handLBolt != -1)
	{
		G_PlayEffect(G_EffectIndex("force/strike_flare"), self->playerModel, self->handLBolt, self->s.number,
			self->currentOrigin, 200, qtrue);
	}

	G_Sound(self, soundIndex);

	ForceShootstrike(self);

	WP_ForcePowerStart(self, FP_LIGHTNING_STRIKE, 0);

	self->client->ps.weaponTime = 1000;
	if (self->client->ps.forcePowersActive & 1 << FP_SPEED)
	{
		self->client->ps.weaponTime = floor(self->client->ps.weaponTime * g_timescale->value);
	}
	self->client->ps.forcePowerDebounce[FP_LIGHTNING_STRIKE] = level.time + self->client->ps.torsoAnimTimer + 500;
}

void ForceLightning(gentity_t* self)
{
	if (self->health <= 0)
	{
		return;
	}

	if (PM_InLedgeMove(self->client->ps.legsAnim))
	{
		return;
	}

	if (BG_InKnockDown(self->client->ps.legsAnim) || self->client->ps.leanofs || PM_InKnockDown(&self->client->ps))
	{
		return;
	}

	if (!self->s.number && (cg.zoomMode || in_camera))
	{
		//can't force lightning when zoomed in or in cinematic
		return;
	}

	if (self->s.number >= MAX_CLIENTS && !G_ControlledByPlayer(self) && self->client->ps.weapon == WP_SABER)
		//npc force use limit
	{
		if (self->client->ps.blockPoints < 75 || self->client->ps.forcePower < 75 || !WP_ForcePowerUsable(
			self, FP_LIGHTNING, 0))
		{
			return;
		}
	}
	else
	{
		if (self->client->ps.forcePower < 25 || !WP_ForcePowerUsable(self, FP_LIGHTNING, 0))
		{
			return;
		}
	}

	if (self->client->ps.forcePowerDebounce[FP_LIGHTNING] > level.time)
	{
		//stops it while using it and also after using it, up to 3 second delay
		return;
	}
	if (self->client->ps.saberLockTime > level.time)
	{
		//FIXME: can this be a way to break out?
		return;
	}
	if (self->client->ps.userInt3 & 1 << FLAG_PREBLOCK)
	{
		return;
	}
	if (self->client->ps.weaponTime > 0 && (!PM_SaberInParry(self->client->ps.saberMove) || !(self->client->ps.userInt3
		& 1 << FLAG_PREBLOCK)))
	{
		return;
	}
	// Make sure to turn off Force Protection and Force Absorb.
	if (self->client->ps.forcePowersActive & 1 << FP_PROTECT)
	{
		WP_ForcePowerStop(self, FP_PROTECT);
	}
	if (self->client->ps.forcePowersActive & 1 << FP_ABSORB)
	{
		WP_ForcePowerStop(self, FP_ABSORB);
	}
	if (self->client->ps.forcePowersActive & 1 << FP_SEE)
	{
		WP_ForcePowerStop(self, FP_SEE);
	}

	if (in_camera)
	{
		NPC_SetAnim(self, SETANIM_TORSO, BOTH_FORCELIGHTNING, SETANIM_FLAG_OVERRIDE | SETANIM_FLAG_HOLD);
	}
	else
	{
		ForceLightningAnim(self);
	}

	self->client->ps.saberMove = self->client->ps.saberBounceMove = LS_READY;
	//don't finish whatever saber anim you may have been in
	self->client->ps.saberBlocked = BLOCKED_NONE;

	G_SoundOnEnt(self, CHAN_BODY, "sound/weapons/force/lightning2.wav");

	if (self->client->ps.forcePowerLevel[FP_LIGHTNING] < FORCE_LEVEL_2)
	{
		//short burst
		//G_SoundOnEnt(self, CHAN_BODY, "sound/weapons/force/lightning3.mp3");
	}
	else
	{
		//holding it
		self->s.loopSound = G_SoundIndex("sound/weapons/force/lightning.mp3");
	}

	self->client->ps.weaponTime = self->client->ps.torsoAnimTimer;
	WP_ForcePowerStart(self, FP_LIGHTNING, self->client->ps.torsoAnimTimer);
}

void force_lightning_damage(gentity_t* self, gentity_t* trace_ent, vec3_t dir, const float dist, const float dot,
	vec3_t impact_point)
{
	int lightning_blocked = qfalse;

	if (trace_ent->NPC && trace_ent->NPC->scriptFlags & SCF_NO_FORCE)
	{
		if (!in_camera
			&& trace_ent->health > 0)
		{
			if (trace_ent->s.weapon == WP_SABER
				&& trace_ent->client->ps.SaberActive()
				&& !trace_ent->client->ps.saberInFlight
				&& (trace_ent->client->ps.saberMove == LS_READY || PM_SaberInParry(trace_ent->client->ps.saberMove) ||
					PM_SaberInReturn(trace_ent->client->ps.saberMove))
				&& InFOV(self->currentOrigin, trace_ent->currentOrigin, trace_ent->client->ps.viewangles, 20, 35)
				&& !PM_InKnockDown(&trace_ent->client->ps)
				&& !PM_SuperBreakLoseAnim(trace_ent->client->ps.torsoAnim)
				&& !PM_SuperBreakWinAnim(trace_ent->client->ps.torsoAnim)
				&& !pm_saber_in_special_attack(trace_ent->client->ps.torsoAnim)
				&& !PM_InSpecialJump(trace_ent->client->ps.torsoAnim))
			{
				//saber can block lightning
				//make them do a parry
				const float chanceOfFizz = Q_flrand(0.0f, 1.0f);
				vec3_t fwd;
				vec3_t right;
				vec3_t up;
				lightning_blocked = qtrue;
				VectorNegate(dir, fwd);

				//randomise direction a bit
				MakeNormalVectors(fwd, right, up);
				VectorMA(fwd, Q_irand(0, 360), right, fwd);
				VectorMA(fwd, Q_irand(0, 360), up, fwd);
				VectorNormalize(fwd);

				if (chanceOfFizz > 0)
				{
					vec3_t end;
					constexpr int npcBladeNum = 0;
					constexpr int npcSaberNum = 0;
					VectorMA(trace_ent->client->ps.saber[npcSaberNum].blade[npcBladeNum].muzzlePoint,
						trace_ent->client->ps.saber[npcSaberNum].blade[npcBladeNum].length * Q_flrand(0, 1),
						trace_ent->client->ps.saber[npcSaberNum].blade[npcBladeNum].muzzleDir, end);
					G_PlayEffect(G_EffectIndex("saber/fizz.efx"), end, fwd);
				}

				switch (trace_ent->client->ps.saberAnimLevel)
				{
				case SS_DUAL:
					NPC_SetAnim(trace_ent, SETANIM_TORSO, BOTH_P6_S6_T_, SETANIM_AFLAG_PACE);
					break;
				case SS_STAFF:
					NPC_SetAnim(trace_ent, SETANIM_TORSO, BOTH_P7_S7_T_, SETANIM_AFLAG_PACE);
					break;
				case SS_FAST:
				case SS_TAVION:
				case SS_STRONG:
				case SS_DESANN:
				case SS_MEDIUM:
					NPC_SetAnim(trace_ent, SETANIM_TORSO, BOTH_P1_S1_T_, SETANIM_AFLAG_PACE);
					break;
				case SS_NONE:
				default:
					NPC_SetAnim(trace_ent, SETANIM_TORSO, BOTH_P1_S1_T_, SETANIM_AFLAG_PACE);
					break;
				}
				trace_ent->client->ps.weaponTime = Q_irand(300, 600);
			}
			else if (trace_ent->s.weapon == WP_MELEE || trace_ent->s.weapon == WP_NONE || trace_ent->client->ps.weapon
				==
				WP_SABER && !trace_ent->client->ps.SaberActive()
				&& !in_camera
				&& (manual_forceblocking(trace_ent) || trace_ent->client->ps.forcePowersActive & 1 << FP_ABSORB &&
					trace_ent
					->client->ps.forcePowerLevel[FP_ABSORB] > FORCE_LEVEL_2)
				&& !PM_InKnockDown(&trace_ent->client->ps)
				&& !PM_InRoll(&trace_ent->client->ps)
				&& trace_ent->client->ps.groundEntityNum != ENTITYNUM_NONE
				&& !PM_RunningAnim(trace_ent->client->ps.legsAnim)
				&& InFOV(self->currentOrigin, trace_ent->currentOrigin, trace_ent->client->ps.viewangles, 20, 35))
			{
				NPC_SetAnim(trace_ent, SETANIM_TORSO, BOTH_FORCE_2HANDEDLIGHTNING_HOLD, SETANIM_AFLAG_PACE);
				trace_ent->client->ps.weaponTime = Q_irand(300, 600);
				lightning_blocked = qtrue;

				if (trace_ent->ghoul2.size())
				{
					if (trace_ent->handRBolt != -1)
					{
						G_PlayEffect(G_EffectIndex("force/HBlockLightning.efx"), trace_ent->playerModel,
							trace_ent->handRBolt, trace_ent->s.number, trace_ent->currentOrigin, 1000, qtrue);
					}
					if (trace_ent->handLBolt != -1)
					{
						G_PlayEffect(G_EffectIndex("force/HBlockLightning.efx"), trace_ent->playerModel,
							trace_ent->handLBolt, trace_ent->s.number, trace_ent->currentOrigin, 1000, qtrue);
					}
				}
				trace_ent->client->ps.powerups[PW_MEDITATE] = level.time + trace_ent->client->ps.torsoAnimTimer + 5000;
			}
			else
			{
				NPC_SetAnim(trace_ent, SETANIM_TORSO, BOTH_WIND, SETANIM_AFLAG_PACE);
				trace_ent->client->ps.weaponTime = Q_irand(300, 600);
				lightning_blocked = qtrue;
				trace_ent->client->ps.powerups[PW_MEDITATE] = level.time + trace_ent->client->ps.torsoAnimTimer + 5000;
			}
		}
		return;
	}

	if (trace_ent && trace_ent->takedamage)
	{
		if (!trace_ent->client || trace_ent->client->playerTeam != self->client->playerTeam || self->enemy == trace_ent
			||
			trace_ent->enemy == self)
		{
			int NPCLfpBlockCost;
			//an enemy or object
			int dmg;

			if (self->client->ps.forcePowerLevel[FP_LIGHTNING] > FORCE_LEVEL_2)
			{
				//more damage if closer and more in front
				dmg = 1;

				if (self->client->NPC_class == CLASS_REBORN && self->client->ps.weapon == WP_NONE)
				{
					//Cultist: looks fancy, but does less damage
				}
				else
				{
					if (dist < 100)
					{
						dmg += 2;
					}
					else if (dist < 200)
					{
						dmg += 1;
					}
					if (dot > 0.9f)
					{
						dmg += 2;
					}
					else if (dot > 0.7f)
					{
						dmg += 1;
					}
				}
				if (self->client->ps.torsoAnim == BOTH_FORCE_2HANDEDLIGHTNING
					|| self->client->ps.torsoAnim == BOTH_FORCE_2HANDEDLIGHTNING_OLD
					|| self->client->ps.torsoAnim == BOTH_FORCE_2HANDEDLIGHTNING_NEW
					|| self->client->ps.torsoAnim == BOTH_FORCE_2HANDEDLIGHTNING_START
					|| self->client->ps.torsoAnim == BOTH_FORCE_2HANDEDLIGHTNING_HOLD
					|| self->client->ps.torsoAnim == BOTH_FORCE_2HANDEDLIGHTNING_RELEASE)
				{
					//jackin' 'em up, Palpatine-style
					dmg *= 2;
				}
			}
			else
			{
				dmg = 1;
			}

			if (trace_ent->client && trace_ent->NPC)
			{
				NPCLfpBlockCost = Q_irand(0, 1);
			}
			else
			{
				NPCLfpBlockCost = 1;
			}

			const int np_cfp_block_cost = Q_irand(2, 4);

			if (trace_ent->client && trace_ent->health > 0)
			{
				constexpr int fpBlockCost = 1;

				if (trace_ent->client
					&& trace_ent->health > 0
					&& trace_ent->NPC
					&& !in_camera)
				{
					if (trace_ent->s.weapon == WP_SABER
						&& trace_ent->client->ps.SaberActive()
						&& !trace_ent->client->ps.saberInFlight
						&& (trace_ent->client->ps.saberMove == LS_READY ||
							PM_SaberInParry(trace_ent->client->ps.saberMove) || PM_SaberInReturn(
								trace_ent->client->ps.saberMove))
						&& InFOV(self->currentOrigin, trace_ent->currentOrigin, trace_ent->client->ps.viewangles, 20,
							35)
						&& !PM_InKnockDown(&trace_ent->client->ps)
						&& !PM_SuperBreakLoseAnim(trace_ent->client->ps.torsoAnim)
						&& !PM_SuperBreakWinAnim(trace_ent->client->ps.torsoAnim)
						&& !pm_saber_in_special_attack(trace_ent->client->ps.torsoAnim)
						&& !PM_InSpecialJump(trace_ent->client->ps.torsoAnim)
						&& trace_ent->client->ps.blockPoints > 5)
					{
						//saber can block lightning
						//make them do a parry
						const float chanceOfFizz = Q_flrand(0.0f, 1.0f);
						vec3_t fwd;
						vec3_t right;
						vec3_t up;
						lightning_blocked = qtrue;
						VectorNegate(dir, fwd);

						//randomise direction a bit
						MakeNormalVectors(fwd, right, up);
						VectorMA(fwd, Q_irand(0, 360), right, fwd);
						VectorMA(fwd, Q_irand(0, 360), up, fwd);
						VectorNormalize(fwd);

						if (chanceOfFizz > 0)
						{
							vec3_t end;
							constexpr int npcBladeNum = 0;
							constexpr int npcSaberNum = 0;
							VectorMA(trace_ent->client->ps.saber[npcSaberNum].blade[npcBladeNum].muzzlePoint,
								trace_ent->client->ps.saber[npcSaberNum].blade[npcBladeNum].length *
								Q_flrand(0, 1),
								trace_ent->client->ps.saber[npcSaberNum].blade[npcBladeNum].muzzleDir, end);
							G_PlayEffect(G_EffectIndex("saber/fizz.efx"), end, fwd);
						}

						switch (trace_ent->client->ps.saberAnimLevel)
						{
						case SS_DUAL:
							NPC_SetAnim(trace_ent, SETANIM_TORSO, BOTH_P6_S6_T_, SETANIM_AFLAG_PACE);
							break;
						case SS_STAFF:
							NPC_SetAnim(trace_ent, SETANIM_TORSO, BOTH_P7_S7_T_, SETANIM_AFLAG_PACE);
							break;
						case SS_FAST:
						case SS_TAVION:
						case SS_STRONG:
						case SS_DESANN:
						case SS_MEDIUM:
							NPC_SetAnim(trace_ent, SETANIM_TORSO, BOTH_P1_S1_T_, SETANIM_AFLAG_PACE);
							break;
						case SS_NONE:
						default:
							NPC_SetAnim(trace_ent, SETANIM_TORSO, BOTH_P1_S1_T_, SETANIM_AFLAG_PACE);
							break;
						}
						trace_ent->client->ps.weaponTime = Q_irand(300, 600);

						if (trace_ent->client->ps.blockPoints < BLOCKPOINTS_HALF)
						{
							WP_ForcePowerDrain(trace_ent, FP_ABSORB, fpBlockCost);
						}
						WP_BlockPointsDrain(trace_ent, NPCLfpBlockCost);
						dmg = 0;
					}
					else if (trace_ent->s.weapon == WP_MELEE
						&& !in_camera
						&& manual_forceblocking(trace_ent)
						&& !PM_InKnockDown(&trace_ent->client->ps)
						&& !PM_InRoll(&trace_ent->client->ps)
						&& trace_ent->client->ps.groundEntityNum != ENTITYNUM_NONE
						&& !PM_RunningAnim(trace_ent->client->ps.legsAnim)
						&& InFOV(self->currentOrigin, trace_ent->currentOrigin, trace_ent->client->ps.viewangles, 20,
							35)
						&& trace_ent->client->ps.forcePower > 20)
					{
						NPC_SetAnim(trace_ent, SETANIM_TORSO, BOTH_FORCE_2HANDEDLIGHTNING_HOLD, SETANIM_AFLAG_PACE);
						trace_ent->client->ps.weaponTime = Q_irand(300, 600);
						lightning_blocked = qtrue;

						if (trace_ent->ghoul2.size())
						{
							if (trace_ent->handRBolt != -1)
							{
								G_PlayEffect(G_EffectIndex("force/HBlockLightning.efx"), trace_ent->playerModel,
									trace_ent->handRBolt, trace_ent->s.number, trace_ent->currentOrigin, 1000,
									qtrue);
							}
							if (trace_ent->handLBolt != -1)
							{
								G_PlayEffect(G_EffectIndex("force/HBlockLightning.efx"), trace_ent->playerModel,
									trace_ent->handLBolt, trace_ent->s.number, trace_ent->currentOrigin, 1000,
									qtrue);
							}
						}
						trace_ent->client->ps.powerups[PW_MEDITATE] = level.time + trace_ent->client->ps.torsoAnimTimer
							+
							5000;
						WP_ForcePowerDrain(trace_ent, FP_ABSORB, np_cfp_block_cost);
						dmg = Q_irand(0, 1);
					}
				}
				else if (trace_ent->s.weapon == WP_SABER && trace_ent->client->ps.SaberActive())
				{
					if (!in_camera && manual_saberblocking(trace_ent) && InFOV(
						self->currentOrigin, trace_ent->currentOrigin, trace_ent->client->ps.viewangles, 20, 35)
						&& trace_ent->client->ps.blockPoints > 5)
					{
						//saber can block lightning
						const qboolean ActiveBlocking = trace_ent->client->ps.ManualBlockingFlags & 1 << HOLDINGBLOCKANDATTACK ? qtrue : qfalse;//Active Blocking
						//make them do a parry
						const float chanceOfFizz = Q_flrand(0.0f, 1.0f);
						vec3_t fwd;
						vec3_t right;
						vec3_t up;
						lightning_blocked = qtrue;
						VectorNegate(dir, fwd);
						//randomise direction a bit
						MakeNormalVectors(fwd, right, up);
						VectorMA(fwd, Q_irand(0, 360), right, fwd);
						VectorMA(fwd, Q_irand(0, 360), up, fwd);
						VectorNormalize(fwd);

						if (chanceOfFizz > 0)
						{
							vec3_t end;
							constexpr int npcBladeNum = 0;
							constexpr int npcSaberNum = 0;
							VectorMA(trace_ent->client->ps.saber[npcSaberNum].blade[npcBladeNum].muzzlePoint,
								trace_ent->client->ps.saber[npcSaberNum].blade[npcBladeNum].length *
								Q_flrand(0, 1),
								trace_ent->client->ps.saber[npcSaberNum].blade[npcBladeNum].muzzleDir, end);
							if (trace_ent->client->ps.blockPoints > 50)
							{
								G_PlayEffect(G_EffectIndex("saber/saber_Lightninghit.efx"), end, fwd);
							}
							else
							{
								G_PlayEffect(G_EffectIndex("saber/fizz.efx"), end, fwd);
							}
						}

						switch (trace_ent->client->ps.saberAnimLevel)
						{
						case SS_DUAL:
							NPC_SetAnim(trace_ent, SETANIM_TORSO, BOTH_P6_S6_T_, SETANIM_AFLAG_PACE);
							break;
						case SS_STAFF:
							NPC_SetAnim(trace_ent, SETANIM_TORSO, BOTH_P7_S7_T_, SETANIM_AFLAG_PACE);
							break;
						case SS_FAST:
						case SS_TAVION:
						case SS_STRONG:
						case SS_DESANN:
						case SS_MEDIUM:
							NPC_SetAnim(trace_ent, SETANIM_TORSO, BOTH_P1_S1_T_, SETANIM_AFLAG_PACE);
							break;
						case SS_NONE:
						default:
							NPC_SetAnim(trace_ent, SETANIM_TORSO, BOTH_P1_S1_T_, SETANIM_AFLAG_PACE);
							break;
						}
						trace_ent->client->ps.weaponTime = Q_irand(300, 600);
						dmg = 0;

						if (ActiveBlocking)
						{
							PM_AddBlockFatigue(&trace_ent->client->ps, fpBlockCost);
						}
						else
						{
							WP_ForcePowerDrain(trace_ent, FP_ABSORB, fpBlockCost);
						}
					}
					else if (in_camera
						&& trace_ent->NPC
						&& trace_ent->client->ps.SaberActive()
						&& !trace_ent->client->ps.saberInFlight
						&& !PM_InKnockDown(&trace_ent->client->ps)
						&& InFOV(self->currentOrigin, trace_ent->currentOrigin, trace_ent->client->ps.viewangles, 20,
							35))
					{
						const float chanceOfFizz = Q_flrand(0.0f, 1.0f);
						vec3_t fwd;
						vec3_t right;
						vec3_t up;
						lightning_blocked = qtrue;
						VectorNegate(dir, fwd);
						//randomise direction a bit
						MakeNormalVectors(fwd, right, up);
						VectorMA(fwd, Q_irand(0, 360), right, fwd);
						VectorMA(fwd, Q_irand(0, 360), up, fwd);
						VectorNormalize(fwd);

						if (chanceOfFizz > 0)
						{
							vec3_t end;
							constexpr int npc_blade_num = 0;
							constexpr int npc_saber_num = 0;
							VectorMA(trace_ent->client->ps.saber[npc_saber_num].blade[npc_blade_num].muzzlePoint,
								trace_ent->client->ps.saber[npc_saber_num].blade[npc_blade_num].length * Q_flrand(
									0, 1),
								trace_ent->client->ps.saber[npc_saber_num].blade[npc_blade_num].muzzleDir, end);
							G_PlayEffect(G_EffectIndex("saber/fizz.efx"), end, fwd);
						}

						switch (trace_ent->client->ps.saberAnimLevel)
						{
						case SS_DUAL:
							NPC_SetAnim(trace_ent, SETANIM_TORSO, BOTH_P6_S6_TL, SETANIM_AFLAG_PACE);
							break;
						case SS_STAFF:
							NPC_SetAnim(trace_ent, SETANIM_TORSO, BOTH_P7_S7_TL, SETANIM_AFLAG_PACE);
							break;
						case SS_FAST:
						case SS_TAVION:
						case SS_STRONG:
						case SS_DESANN:
						case SS_MEDIUM:
							NPC_SetAnim(trace_ent, SETANIM_TORSO, BOTH_P1_S1_TL, SETANIM_AFLAG_PACE);
							break;
						case SS_NONE:
						default:
							NPC_SetAnim(trace_ent, SETANIM_TORSO, BOTH_P1_S1_T_, SETANIM_AFLAG_PACE);
							break;
						}
						trace_ent->client->ps.weaponTime = Q_irand(300, 600);
						dmg = 0;
					}
					else if (!in_camera && (trace_ent->client->ps.forcePowersActive & 1 << FP_ABSORB && trace_ent->
						client
						->ps.forcePowerLevel[FP_ABSORB] > FORCE_LEVEL_2) && InFOV(
							self->currentOrigin, trace_ent->currentOrigin, trace_ent->client->ps.viewangles, 20, 35) &&
						trace_ent->client->ps.forcePower > 20)
					{
						//make them do a parry
						const float chanceOfFizz = Q_flrand(0.0f, 1.0f);
						vec3_t fwd;
						vec3_t right;
						vec3_t up;
						lightning_blocked = qtrue;
						VectorNegate(dir, fwd);
						//randomise direction a bit
						MakeNormalVectors(fwd, right, up);
						VectorMA(fwd, Q_irand(0, 360), right, fwd);
						VectorMA(fwd, Q_irand(0, 360), up, fwd);
						VectorNormalize(fwd);

						if (chanceOfFizz > 0)
						{
							vec3_t end;
							constexpr int npc_blade_num = 0;
							constexpr int npc_saber_num = 0;
							VectorMA(trace_ent->client->ps.saber[npc_saber_num].blade[npc_blade_num].muzzlePoint,
								trace_ent->client->ps.saber[npc_saber_num].blade[npc_blade_num].length * Q_flrand(
									0, 1),
								trace_ent->client->ps.saber[npc_saber_num].blade[npc_blade_num].muzzleDir, end);
							if (trace_ent->client->ps.blockPoints > 50)
							{
								G_PlayEffect(G_EffectIndex("saber/saber_Lightninghit.efx"), end, fwd);
							}
							else
							{
								G_PlayEffect(G_EffectIndex("saber/fizz.efx"), end, fwd);
							}
						}

						switch (trace_ent->client->ps.saberAnimLevel)
						{
						case SS_DUAL:
							NPC_SetAnim(trace_ent, SETANIM_TORSO, BOTH_P6_S6_T_, SETANIM_AFLAG_PACE);
							break;
						case SS_STAFF:
							NPC_SetAnim(trace_ent, SETANIM_TORSO, BOTH_P7_S7_T_, SETANIM_AFLAG_PACE);
							break;
						case SS_FAST:
						case SS_TAVION:
						case SS_STRONG:
						case SS_DESANN:
						case SS_MEDIUM:
							NPC_SetAnim(trace_ent, SETANIM_TORSO, BOTH_P1_S1_T_, SETANIM_AFLAG_PACE);
							break;
						case SS_NONE:
						default:
							NPC_SetAnim(trace_ent, SETANIM_TORSO, BOTH_P1_S1_T_, SETANIM_AFLAG_PACE);
							break;
						}
						trace_ent->client->ps.weaponTime = Q_irand(300, 600);
						dmg = 0;
						WP_ForcePowerDrain(trace_ent, FP_ABSORB, fpBlockCost);
					}
					else
					{
						dmg = 1;
					}
				}
				else if (trace_ent->s.weapon == WP_MELEE || trace_ent->s.weapon == WP_NONE || trace_ent->client->ps.
					weapon
					== WP_SABER && !trace_ent->client->ps.SaberActive())
				{
					if (!in_camera
						&& (manual_forceblocking(trace_ent) || trace_ent->client->ps.forcePowersActive & 1 << FP_ABSORB
							&&
							trace_ent->client->ps.forcePowerLevel[FP_ABSORB] > FORCE_LEVEL_2)
						&& !PM_InKnockDown(&trace_ent->client->ps)
						&& !PM_InRoll(&trace_ent->client->ps)
						&& trace_ent->client->ps.groundEntityNum != ENTITYNUM_NONE
						&& !PM_RunningAnim(trace_ent->client->ps.legsAnim)
						&& InFOV(self->currentOrigin, trace_ent->currentOrigin, trace_ent->client->ps.viewangles, 20,
							35)
						&& trace_ent->client->ps.forcePower > 20)
					{
						NPC_SetAnim(trace_ent, SETANIM_TORSO, BOTH_FORCE_2HANDEDLIGHTNING_HOLD, SETANIM_AFLAG_PACE);
						trace_ent->client->ps.weaponTime = Q_irand(300, 600);
						lightning_blocked = qtrue;

						if (trace_ent->ghoul2.size())
						{
							if (trace_ent->handRBolt != -1)
							{
								G_PlayEffect(G_EffectIndex("force/HBlockLightning.efx"), trace_ent->playerModel,
									trace_ent->handRBolt, trace_ent->s.number, trace_ent->currentOrigin, 1000,
									qtrue);
							}
							if (trace_ent->handLBolt != -1)
							{
								G_PlayEffect(G_EffectIndex("force/HBlockLightning.efx"), trace_ent->playerModel,
									trace_ent->handLBolt, trace_ent->s.number, trace_ent->currentOrigin, 1000,
									qtrue);
							}
						}
						trace_ent->client->ps.powerups[PW_MEDITATE] = level.time + trace_ent->client->ps.torsoAnimTimer
							+
							5000;
						WP_ForcePowerDrain(trace_ent, FP_ABSORB, fpBlockCost);
					}
					else
					{
						dmg = 1;
					}
				}
			}

			if (trace_ent && trace_ent->client && trace_ent->client->ps.powerups[PW_GALAK_SHIELD])
			{
				//has shield up
				dmg = 0;
				lightning_blocked = qtrue;
			}

			if (trace_ent && trace_ent->client && trace_ent->client->NPC_class == CLASS_OBJECT)
			{
				dmg = 0;
				lightning_blocked = qtrue;
			}

			int modPowerLevel = -1;

			if (trace_ent->client)
			{
				modPowerLevel = wp_absorb_conversion(trace_ent, trace_ent->client->ps.forcePowerLevel[FP_ABSORB], self,
					FP_LIGHTNING, self->client->ps.forcePowerLevel[FP_LIGHTNING], 1);
			}

			if (modPowerLevel != -1)
			{
				if (!modPowerLevel)
				{
					dmg = 0;
				}
				else if (modPowerLevel == 1)
				{
					dmg = floor(static_cast<float>(dmg) / 4.0f);
				}
				else if (modPowerLevel == 2)
				{
					dmg = floor(static_cast<float>(dmg) / 2.0f);
				}
			}

			if (dmg && !lightning_blocked)
			{
				if (trace_ent->s.weapon == WP_SABER)
				{
					G_Damage(trace_ent, self, self, dir, impact_point, dmg, DAMAGE_LIGHNING_KNOCKBACK,
						MOD_FORCE_LIGHTNING);
				}
				else
				{
					G_Damage(trace_ent, self, self, dir, impact_point, dmg, DAMAGE_NO_KNOCKBACK, MOD_FORCE_LIGHTNING);
				}

				if (!in_camera && (trace_ent->NPC || trace_ent->s.eType == ET_PLAYER)
					&& !(trace_ent->s.weapon == WP_EMPLACED_GUN))
				{
					if (trace_ent
						&& trace_ent->health <= 35 && (trace_ent->s.number < MAX_CLIENTS ||
							G_ControlledByPlayer(trace_ent)))
					{
						trace_ent->client->stunDamage = 9;
						trace_ent->client->stunTime = level.time + 1000;

						gentity_t* tent = G_TempEntity(trace_ent->currentOrigin, EV_STUNNED);
						tent->owner = trace_ent;
					}

					if (trace_ent->client->ps.stats[STAT_HEALTH] <= 0 && class_is_gunner(trace_ent)) // if we are dead
					{
						vec3_t spot;

						VectorCopy(trace_ent->currentOrigin, spot);

						trace_ent->flags |= FL_DISINTEGRATED;
						trace_ent->svFlags |= SVF_BROADCAST;
						gentity_t* tent = G_TempEntity(spot, EV_DISINTEGRATION);
						tent->s.eventParm = PW_DISRUPTION;
						tent->svFlags |= SVF_BROADCAST;
						tent->owner = trace_ent;

						if (trace_ent->playerModel >= 0)
						{
							// don't let 'em animate
							gi.G2API_PauseBoneAnimIndex(&trace_ent->ghoul2[self->playerModel], trace_ent->rootBone,
								cg.time);
							gi.G2API_PauseBoneAnimIndex(&trace_ent->ghoul2[self->playerModel], trace_ent->motionBone,
								cg.time);
							gi.G2API_PauseBoneAnimIndex(&trace_ent->ghoul2[self->playerModel],
								trace_ent->lowerLumbarBone,
								cg.time);
						}

						//not solid anymore
						trace_ent->contents = 0;
						trace_ent->maxs[2] = -8;

						//need to pad deathtime some to stick around long enough for death effect to play
						trace_ent->NPC->timeOfDeath = level.time + 2000;
					}

					if (PM_RunningAnim(trace_ent->client->ps.legsAnim) && trace_ent->health > 1)
					{
						G_KnockOver(trace_ent, self, dir, 25, qtrue);
					}
					else if (trace_ent->client->ps.groundEntityNum == ENTITYNUM_NONE && trace_ent->health > 1)
					{
						g_throw(trace_ent, dir, 2);
						NPC_SetAnim(trace_ent, SETANIM_BOTH, Q_irand(BOTH_SLAPDOWNRIGHT, BOTH_SLAPDOWNLEFT), SETANIM_AFLAG_PACE);
					}
					else
					{
						if (!PM_RunningAnim(trace_ent->client->ps.legsAnim)
							&& !PM_InKnockDown(&trace_ent->client->ps)
							&& trace_ent->health > 1)
						{
							if (trace_ent->health < 75)
							{
								NPC_SetAnim(trace_ent, SETANIM_TORSO, BOTH_COWER1, SETANIM_AFLAG_PACE);
							}
							else if (trace_ent->health < 50)
							{
								NPC_SetAnim(trace_ent, SETANIM_TORSO, BOTH_SONICPAIN_HOLD, SETANIM_AFLAG_PACE);
							}
							else
							{
								NPC_SetAnim(trace_ent, SETANIM_TORSO, BOTH_FACEPROTECT, SETANIM_AFLAG_PACE);
							}
						}
						else
						{
							if ((trace_ent->NPC || trace_ent->s.eType == ET_PLAYER)
								&& trace_ent->client->ps.stats[STAT_HEALTH] < 2 && class_is_gunner(trace_ent))
							{
								vec3_t defaultDir;
								VectorSet(defaultDir, 0, 0, 1);
								G_PlayEffect(G_EffectIndex("force/Lightningkill.efx"), trace_ent->currentOrigin, defaultDir);
							}
						}
					}
				}
			}

			if (trace_ent->client)
			{
				if (!Q_irand(0, 2))
				{
					G_Sound(trace_ent, G_SoundIndex(va("sound/weapons/force/lightninghit%d.mp3", Q_irand(1, 3))));
				}

				if (trace_ent->client && trace_ent->client->ps.powerups[PW_CLOAKED])
				{
					//disable cloak temporarily
					player_Decloak(trace_ent);
					G_AddVoiceEvent(trace_ent, Q_irand(EV_ANGER1, EV_ANGER3), 1000);
				}
				trace_ent->s.powerups |= 1 << PW_SHOCKED;

				if (trace_ent && trace_ent->client &&
					(trace_ent->health <= 0 || trace_ent->client->NPC_class == CLASS_ATST
						|| trace_ent->client->NPC_class == CLASS_GONK
						|| trace_ent->client->NPC_class == CLASS_INTERROGATOR
						|| trace_ent->client->NPC_class == CLASS_MARK1
						|| trace_ent->client->NPC_class == CLASS_MARK2
						|| trace_ent->client->NPC_class == CLASS_MOUSE
						|| trace_ent->client->NPC_class == CLASS_PROBE
						|| trace_ent->client->NPC_class == CLASS_PROTOCOL
						|| trace_ent->client->NPC_class == CLASS_R2D2
						|| trace_ent->client->NPC_class == CLASS_R5D2
						|| trace_ent->client->NPC_class == CLASS_SEEKER
						|| trace_ent->client->NPC_class == CLASS_SENTRY
						|| trace_ent->client->NPC_class == CLASS_SBD
						|| trace_ent->client->NPC_class == CLASS_BATTLEDROID
						|| trace_ent->client->NPC_class == CLASS_DROIDEKA
						|| trace_ent->client->NPC_class == CLASS_OBJECT
						|| trace_ent->client->NPC_class == CLASS_ASSASSIN_DROID
						|| trace_ent->client->NPC_class == CLASS_SABER_DROID))
				{
					// special droid only behaviors
					trace_ent->client->ps.powerups[PW_SHOCKED] = level.time + 4000;
				}
				else if (lightning_blocked)
				{
					trace_ent->client->ps.powerups[PW_SHOCKED] = 0;
				}
				else
				{
					trace_ent->client->ps.powerups[PW_SHOCKED] = level.time + 2000;
				}
			}
		}
	}
}

void force_shoot_lightning(gentity_t* self)
{
	trace_t tr;
	vec3_t forward;
	gentity_t* traceEnt;

	if (self->health <= 0)
	{
		return;
	}

	if (PM_InLedgeMove(self->client->ps.legsAnim))
	{
		return;
	}
	if (!self->s.number && cg.zoomMode)
	{
		//can't force lightning when zoomed in
		return;
	}

	AngleVectors(self->client->ps.viewangles, forward, nullptr, nullptr);
	VectorNormalize(forward);
	//Execute our bState

	if (self->client->ps.forcePowerLevel[FP_LIGHTNING] > FORCE_LEVEL_2)
	{
		vec3_t center;
		vec3_t mins;
		vec3_t maxs;
		vec3_t v;
		constexpr float radius = FORCE_LIGHTNING_RADIUS_WIDE;
		float dot;
		gentity_t* entityList[MAX_GENTITIES];
		int i;

		VectorCopy(self->currentOrigin, center);
		for (i = 0; i < 3; i++)
		{
			mins[i] = center[i] - radius;
			maxs[i] = center[i] + radius;
		}
		const int num_listed_entities = gi.EntitiesInBox(mins, maxs, entityList, MAX_GENTITIES);

		for (int e = 0; e < num_listed_entities; e++)
		{
			vec3_t size;
			vec3_t ent_org;
			vec3_t dir;
			traceEnt = entityList[e];

			if (!traceEnt)
				continue;
			if (traceEnt == self)
				continue;
			if (traceEnt->owner == self && traceEnt->s.weapon != WP_THERMAL) //can push your own thermals
				continue;
			if (!traceEnt->inuse)
				continue;
			if (!traceEnt->takedamage)
				continue;
			//if (traceEnt->health <= 0)//no torturing corpses
			//	continue;
			//this is all to see if we need to start a saber attack, if it's in flight, this doesn't matter
			// find the distance from the edge of the bounding box
			for (i = 0; i < 3; i++)
			{
				if (center[i] < traceEnt->absmin[i])
				{
					v[i] = traceEnt->absmin[i] - center[i];
				}
				else if (center[i] > traceEnt->absmax[i])
				{
					v[i] = center[i] - traceEnt->absmax[i];
				}
				else
				{
					v[i] = 0;
				}
			}

			VectorSubtract(traceEnt->absmax, traceEnt->absmin, size);
			VectorMA(traceEnt->absmin, 0.5, size, ent_org);

			//see if they're in front of me
			//must be within the forward cone
			VectorSubtract(ent_org, center, dir);
			VectorNormalize(dir);
			if ((dot = DotProduct(dir, forward)) < 0.5)
				continue;

			//must be close enough
			const float dist = VectorLength(v);
			if (dist >= radius)
			{
				continue;
			}

			if (!traceEnt->bmodel && !gi.inPVS(ent_org, self->client->renderInfo.handLPoint))
			{
				//must be in PVS
				continue;
			}

			//Now check and see if we can actually hit it
			gi.trace(&tr, self->client->renderInfo.handLPoint, vec3_origin, vec3_origin, ent_org, self->s.number,
				MASK_SHOT, static_cast<EG2_Collision>(0), 0);

			if (tr.fraction < 1.0f && tr.entityNum != traceEnt->s.number)
			{
				//must have clear LOS
				continue;
			}
			force_lightning_damage(self, traceEnt, dir, dist, dot, ent_org);
		}
	}
	else
	{
		vec3_t end;
		int ignore = self->s.number;
		int traces = 0;
		vec3_t start;

		VectorCopy(self->client->renderInfo.handLPoint, start);
		VectorMA(self->client->renderInfo.handLPoint, 2048, forward, end);

		while (traces < 10)
		{
			//need to loop this in case we hit a Jedi who dodges the shot
			gi.trace(&tr, start, vec3_origin, vec3_origin, end, ignore, MASK_SHOT, G2_RETURNONHIT, 10);
			if (tr.entityNum == ENTITYNUM_NONE || tr.fraction == 1.0 || tr.allsolid || tr.startsolid)
			{
				return;
			}

			traceEnt = &g_entities[tr.entityNum];
			//NOTE: only NPCs do this auto-dodge
			if (!in_camera && traceEnt
				&& traceEnt->s.weapon != WP_SABER
				&& traceEnt->s.number >= MAX_CLIENTS
				&& traceEnt->client
				&& traceEnt->client->ps.forcePowerLevel[FP_LEVITATION] > FORCE_LEVEL_0)
			{
				//FIXME: need a more reliable way to know we hit a jedi?
				if (!jedi_dodge_evasion(traceEnt, self, &tr, HL_NONE))
				{
					//act like we didn't even hit him
					VectorCopy(tr.endpos, start);
					ignore = tr.entityNum;
					traces++;
					continue;
				}
			}
			//a Jedi is not dodging this shot
			break;
		}

		traceEnt = &g_entities[tr.entityNum];

		force_lightning_damage(self, traceEnt, forward, 0, 0, tr.endpos);
	}
}

void WP_DeactivateSaber(const gentity_t* self, qboolean clearLength)
{
	const qboolean Blocking = self->client->ps.ManualBlockingFlags & 1 << HOLDINGBLOCK ? qtrue : qfalse;
	//Normal Blocking
	const qboolean ActiveBlocking = self->client->ps.ManualBlockingFlags & 1 << HOLDINGBLOCKANDATTACK ? qtrue : qfalse;
	//Active Blocking

	if (!self || !self->client)
	{
		return;
	}

	if (Blocking || ActiveBlocking)
	{
		return;
	}
	self->client->ps.ManualBlockingFlags &= ~(1 << HOLDINGBLOCK);
	self->client->ps.ManualBlockingFlags &= ~(1 << HOLDINGBLOCKANDATTACK);
	self->client->ps.ManualBlockingFlags &= ~(1 << PERFECTBLOCKING);
	self->client->ps.ManualBlockingFlags &= ~(1 << MBF_NPCBLOCKING);
	//keep my saber off!
	if (self->client->ps.SaberActive())
	{
		self->client->ps.SaberDeactivate();
		if (clearLength)
		{
			self->client->ps.SetSaberLength(0);
		}
		G_SoundIndexOnEnt(self, CHAN_WEAPON, self->client->ps.saber[0].soundOff);
	}
	self->client->ps.ManualBlockingFlags &= ~(1 << HOLDINGBLOCK);
	self->client->ps.ManualBlockingFlags &= ~(1 << HOLDINGBLOCKANDATTACK);
	self->client->ps.ManualBlockingFlags &= ~(1 << PERFECTBLOCKING);
	self->client->ps.ManualBlockingFlags &= ~(1 << MBF_NPCBLOCKING);
}

void WP_DeactivateLightSaber(const gentity_t* self, qboolean clearLength)
{
	const qboolean Blocking = self->client->ps.ManualBlockingFlags & 1 << HOLDINGBLOCK ? qtrue : qfalse;
	//Normal Blocking
	const qboolean ActiveBlocking = self->client->ps.ManualBlockingFlags & 1 << HOLDINGBLOCKANDATTACK ? qtrue : qfalse;
	//Active Blocking

	if (!self || !self->client)
	{
		return;
	}

	if (Blocking || ActiveBlocking)
	{
		return;
	}
	self->client->ps.ManualBlockingFlags &= ~(1 << HOLDINGBLOCK);
	self->client->ps.ManualBlockingFlags &= ~(1 << HOLDINGBLOCKANDATTACK);
	self->client->ps.ManualBlockingFlags &= ~(1 << PERFECTBLOCKING);
	self->client->ps.ManualBlockingFlags &= ~(1 << MBF_NPCBLOCKING);
	//keep my saber off!
	if (self->client->ps.SaberActive())
	{
		self->client->ps.SaberDeactivate();
		if (clearLength)
		{
			self->client->ps.SetSaberLength(0);
			PM_SetSaberMove(LS_DRAW);
		}
		G_SoundIndexOnEnt(self, CHAN_WEAPON, self->client->ps.saber[0].soundOff);
	}
	self->client->ps.ManualBlockingFlags &= ~(1 << HOLDINGBLOCK);
	self->client->ps.ManualBlockingFlags &= ~(1 << HOLDINGBLOCKANDATTACK);
	self->client->ps.ManualBlockingFlags &= ~(1 << PERFECTBLOCKING);
	self->client->ps.ManualBlockingFlags &= ~(1 << MBF_NPCBLOCKING);
}

void ForceShootDrain(gentity_t* self);

void ForceDrainGrabStart(gentity_t* self)
{
	NPC_SetAnim(self, SETANIM_BOTH, BOTH_FORCE_DRAIN_GRAB_START, SETANIM_FLAG_OVERRIDE | SETANIM_FLAG_HOLD);
	self->client->ps.saberMove = self->client->ps.saberBounceMove = LS_READY;
	//don't finish whatever saber anim you may have been in
	self->client->ps.saberBlocked = BLOCKED_NONE;

	self->client->ps.weaponTime = 1000;
	if (self->client->ps.forcePowersActive & 1 << FP_SPEED)
	{
		self->client->ps.weaponTime = floor(self->client->ps.weaponTime * g_timescale->value);
	}
	//actually grabbing someone, so turn off the saber!
	WP_DeactivateSaber(self, qtrue);
}

qboolean ForceDrain2(gentity_t* self)
{
	//FIXME: make enemy Jedi able to use this
	trace_t tr;
	vec3_t end, forward;

	if (self->health <= 0)
	{
		return qtrue;
	}

	if (!self->s.number && (cg.zoomMode || in_camera))
	{
		//can't force grip when zoomed in or in cinematic
		return qtrue;
	}

	if (self->client->ps.leanofs)
	{
		//can't force-drain while leaning
		return qtrue;
	}

	if (self->client->ps.forceDrainEntityNum <= ENTITYNUM_WORLD)
	{
		//already draining
		//keep my saber off!
		WP_DeactivateSaber(self, qtrue);
		if (self->client->ps.forcePowerLevel[FP_DRAIN] > FORCE_LEVEL_1)
		{
			self->client->ps.forcePowerDuration[FP_DRAIN] = level.time + 100;
			self->client->ps.weaponTime = 1000;
			if (self->client->ps.forcePowersActive & 1 << FP_SPEED)
			{
				self->client->ps.weaponTime = floor(self->client->ps.weaponTime * g_timescale->value);
			}
		}
		return qtrue;
	}

	if (self->client->ps.forcePowerDebounce[FP_DRAIN] > level.time)
	{
		//stops it while using it and also after using it, up to 3 second delay
		return qtrue;
	}

	if (self->client->ps.weaponTime > 0)
	{
		//busy
		return qtrue;
	}

	if (self->s.number >= MAX_CLIENTS && !G_ControlledByPlayer(self) && self->client->ps.weapon == WP_SABER)
		//npc force use limit
	{
		if (self->client->ps.blockPoints < 75 || self->client->ps.forcePower < 75)
		{
			return qtrue;
		}
	}
	else
	{
		if (self->client->ps.forcePower < 25 || !WP_ForcePowerUsable(self, FP_DRAIN, 0))
		{
			return qtrue;
		}
	}

	if (self->client->ps.saberLockTime > level.time)
	{
		//in saberlock
		return qtrue;
	}

	//NOTE: from here on, if it fails, it's okay to try a normal drain, so return qfalse
	if (self->client->ps.groundEntityNum == ENTITYNUM_NONE)
	{
		//in air
		return qfalse;
	}

	//Cause choking anim + health drain in ent in front of me
	AngleVectors(self->client->ps.viewangles, forward, nullptr, nullptr);
	VectorNormalize(forward);
	VectorMA(self->client->renderInfo.eyePoint, FORCE_DRAIN_DIST, forward, end);

	//okay, trace straight ahead and see what's there
	gi.trace(&tr, self->client->renderInfo.eyePoint, vec3_origin, vec3_origin, end, self->s.number, MASK_SHOT,
		static_cast<EG2_Collision>(0), 0);
	if (tr.entityNum >= ENTITYNUM_WORLD || tr.fraction == 1.0 || tr.allsolid || tr.startsolid)
	{
		return qfalse;
	}
	gentity_t* traceEnt = &g_entities[tr.entityNum];
	if (!traceEnt || traceEnt == self/*???*/ || traceEnt->bmodel || traceEnt->health <= 0 && traceEnt->takedamage ||
		traceEnt->NPC && traceEnt->NPC->scriptFlags & SCF_NO_FORCE)
	{
		return qfalse;
	}

	if (traceEnt->client)
	{
		if (traceEnt->client->ps.forceJumpZStart)
		{
			//can't catch them in mid force jump - FIXME: maybe base it on velocity?
			return qfalse;
		}
		if (traceEnt->client->ps.groundEntityNum == ENTITYNUM_NONE)
		{
			//can't catch them in mid air
			return qfalse;
		}
		if (!Q_stricmp("Yoda", traceEnt->NPC_type))
		{
			Jedi_PlayDeflectSound(traceEnt);
			ForceThrow(traceEnt, qfalse);
			return qtrue;
		}
		if (!Q_stricmp("T_Yoda", traceEnt->NPC_type))
		{
			Jedi_PlayDeflectSound(traceEnt);
			ForceThrow(traceEnt, qfalse);
			return qtrue;
		}
		if (!Q_stricmp("T_Palpatine_sith", traceEnt->NPC_type))
		{
			Jedi_PlayDeflectSound(traceEnt);
			ForceThrow(traceEnt, qfalse);
			return qtrue;
		}
		switch (traceEnt->client->NPC_class)
		{
		case CLASS_GALAKMECH: //cant grab him, he's in armor
			G_AddVoiceEvent(traceEnt, Q_irand(EV_PUSHED1, EV_PUSHED3), Q_irand(3000, 5000));
			return qfalse;
		case CLASS_SBD: //cant grab him, he's in armor
			G_AddVoiceEvent(traceEnt, Q_irand(EV_PUSHED1, EV_PUSHED3), Q_irand(3000, 5000));
			return qfalse;
		case CLASS_ROCKETTROOPER: //cant grab him, he's in armor
		case CLASS_HAZARD_TROOPER: //cant grab him, he's in armor
			return qfalse;
		case CLASS_ATST: //much too big to grab!
			return qfalse;
			//no droids either
		case CLASS_GONK:
		case CLASS_R2D2:
		case CLASS_R5D2:
		case CLASS_MARK1:
		case CLASS_MARK2:
		case CLASS_MOUSE:
		case CLASS_PROTOCOL:
		case CLASS_SABER_DROID:
		case CLASS_ASSASSIN_DROID:
		case CLASS_DROIDEKA:
			return qfalse;
		case CLASS_PROBE:
		case CLASS_SEEKER:
		case CLASS_REMOTE:
		case CLASS_SENTRY:
		case CLASS_INTERROGATOR:
			return qfalse;
		case CLASS_SITHLORD:
		case CLASS_DESANN: //Desann cannot be gripped, he just pushes you back instantly
		case CLASS_VADER:
		case CLASS_KYLE:
		case CLASS_TAVION:
		case CLASS_LUKE:
		case CLASS_YODA:
			Jedi_PlayDeflectSound(traceEnt);
			ForceThrow(traceEnt, qfalse);
			return qtrue;
		case CLASS_REBORN:
		case CLASS_SHADOWTROOPER:
			//case CLASS_ALORA:
		case CLASS_JEDI:
			if (traceEnt->NPC
				&& traceEnt->NPC->rank > RANK_CIVILIAN
				&& self->client->ps.forcePowerLevel[FP_DRAIN] < FORCE_LEVEL_2
				&& traceEnt->client->ps.weaponTime <= 0)
			{
				ForceDrainGrabStart(self);
				Jedi_PlayDeflectSound(traceEnt);
				ForceThrow(traceEnt, qfalse);
				return qtrue;
			}
			break;
		default:
			break;
		}
		if (traceEnt->s.weapon == WP_EMPLACED_GUN)
		{
			//FIXME: maybe can pull them out?
			return qfalse;
		}
		if (traceEnt != self->enemy && OnSameTeam(self, traceEnt))
		{
			//can't accidently grip-drain your teammate
			return qfalse;
		}

		if (!FP_ForceDrainGrippableEnt(traceEnt))
		{
			return qfalse;
		}
	}
	else
	{
		//can't drain non-clients
		return qfalse;
	}

	ForceDrainGrabStart(self);

	WP_ForcePowerStart(self, FP_DRAIN, 10);
	self->client->ps.forceDrainEntityNum = traceEnt->s.number;

	G_AddVoiceEvent(traceEnt, Q_irand(EV_CHOKE1, EV_CHOKE3), 2000);
	if (traceEnt->s.weapon == WP_SABER)
	{
		//if we pick up, turn off their weapon
		WP_DeactivateSaber(traceEnt, qtrue);
	}

	G_SoundOnEnt(self, CHAN_BODY, "sound/weapons/force/drain.mp3");

	NPC_SetAnim(traceEnt, SETANIM_BOTH, BOTH_FORCE_DRAIN_GRABBED, SETANIM_FLAG_OVERRIDE | SETANIM_FLAG_HOLD);

	WP_SabersCheckLock2(self, traceEnt, LOCK_FORCE_DRAIN);

	return qtrue;
}

void ForceDrain(gentity_t* self, qboolean triedDrain2)
{
	if (self->health <= 0)
	{
		return;
	}

	if (!triedDrain2 && self->client->ps.weaponTime > 0)
	{
		return;
	}

	if (PM_InLedgeMove(self->client->ps.legsAnim))
	{
		return;
	}

	if (self->client->ps.forcePower < 25 || !WP_ForcePowerUsable(self, FP_DRAIN, 0))
	{
		return;
	}
	if (self->client->ps.forcePowerDebounce[FP_DRAIN] > level.time)
	{
		//stops it while using it and also after using it, up to 3 second delay
		return;
	}

	if (self->client->ps.saberLockTime > level.time)
	{
		//FIXME: can this be a way to break out?
		return;
	}

	// Make sure to turn off Force Protection and Force Absorb.
	if (self->client->ps.forcePowersActive & 1 << FP_PROTECT)
	{
		WP_ForcePowerStop(self, FP_PROTECT);
	}
	if (self->client->ps.forcePowersActive & 1 << FP_ABSORB)
	{
		WP_ForcePowerStop(self, FP_ABSORB);
	}

	G_SoundOnEnt(self, CHAN_BODY, "sound/weapons/force/drain.mp3");

	WP_ForcePowerStart(self, FP_DRAIN, 0);
}

qboolean FP_ForceDrainableEnt(const gentity_t* victim)
{
	if (!victim || !victim->client)
	{
		return qfalse;
	}
	switch (victim->client->NPC_class)
	{
	case CLASS_SAND_CREATURE: //??
	case CLASS_ATST: // technically droid...
	case CLASS_GONK: // droid
	case CLASS_INTERROGATOR: // droid
	case CLASS_MARK1: // droid
	case CLASS_MARK2: // droid
	case CLASS_GALAKMECH: // droid
	case CLASS_MINEMONSTER:
	case CLASS_MOUSE: // droid
	case CLASS_PROBE: // droid
	case CLASS_PROTOCOL: // droid
	case CLASS_R2D2: // droid
	case CLASS_R5D2: // droid
	case CLASS_REMOTE:
	case CLASS_SEEKER: // droid
	case CLASS_SENTRY:
	case CLASS_SABER_DROID:
	case CLASS_SBD:
	case CLASS_ASSASSIN_DROID:
	case CLASS_DROIDEKA:
	case CLASS_VEHICLE:
		return qfalse;
	default:
		break;
	}
	return qtrue;
}

qboolean FP_ForceDrainGrippableEnt(const gentity_t* victim)
{
	if (!victim || !victim->client)
	{
		return qfalse;
	}
	if (!FP_ForceDrainableEnt(victim))
	{
		return qfalse;
	}
	switch (victim->client->NPC_class)
	{
	case CLASS_RANCOR:
	case CLASS_SAND_CREATURE:
	case CLASS_WAMPA:
	case CLASS_LIZARD:
	case CLASS_MINEMONSTER:
	case CLASS_MURJJ:
	case CLASS_SWAMP:
	case CLASS_ROCKETTROOPER:
	case CLASS_HAZARD_TROOPER:
		return qfalse;
	default:
		break;
	}
	return qtrue;
}

qboolean Jedi_DrainReaction(gentity_t* self, gentity_t* shooter)
{
	if (self->health > 0 && !PM_InKnockDown(&self->client->ps)
		&& self->client->ps.torsoAnim != BOTH_FORCE_DRAIN_GRABBED)
	{
		//still alive
		if (self->client->ps.stats[STAT_HEALTH] < 50)
		{
			NPC_SetAnim(self, SETANIM_TORSO, BOTH_SONICPAIN_HOLD, SETANIM_AFLAG_PACE);
		}
		else
		{
			NPC_SetAnim(self, SETANIM_TORSO, BOTH_WIND, SETANIM_AFLAG_PACE);
		}

		self->client->ps.ManualBlockingFlags &= ~(1 << HOLDINGBLOCK);
		self->client->ps.ManualBlockingFlags &= ~(1 << HOLDINGBLOCKANDATTACK);
		self->client->ps.ManualBlockingFlags &= ~(1 << PERFECTBLOCKING);
		self->client->ps.ManualBlockingFlags &= ~(1 << MBF_NPCBLOCKING);
	}
	return qfalse;
}

void ForceDrainDamage(gentity_t* self, gentity_t* traceEnt, vec3_t dir, vec3_t impactPoint)
{
	if (traceEnt
		&& traceEnt->health > 0
		&& traceEnt->takedamage
		&& FP_ForceDrainableEnt(traceEnt))
	{
		if (traceEnt->client
			&& (!OnSameTeam(self, traceEnt) || self->enemy == traceEnt)
			//don't drain an ally unless that is actually my current enemy
			&& self->client->ps.forceDrainTime < level.time)
		{
			//an enemy or object
			int modPowerLevel = -1;
			int dmg = self->client->ps.forcePowerLevel[FP_DRAIN] + 1;
			int dflags = DAMAGE_NO_ARMOR | DAMAGE_NO_KNOCKBACK | DAMAGE_NO_HIT_LOC;
			if (traceEnt->s.number == self->client->ps.forceDrainEntityNum)
			{
				//grabbing hold of them does more damage/drains more, and can actually kill them
				dmg += 3;
				dflags |= DAMAGE_IGNORE_TEAM;
			}

			if (traceEnt->client)
			{
				//check for client using FP_ABSORB
				modPowerLevel = wp_absorb_conversion(traceEnt, traceEnt->client->ps.forcePowerLevel[FP_ABSORB], self,
					FP_DRAIN, self->client->ps.forcePowerLevel[FP_DRAIN], 0);
				//Since this is drain, don't absorb any power, but nullify the affect it has
			}

			if (modPowerLevel != -1)
			{
				if (!modPowerLevel)
				{
					dmg = 0;
				}
				else if (modPowerLevel == 1)
				{
					dmg = 1;
				}
				else if (modPowerLevel == 2)
				{
					dmg = 2;
				}
			}

			if (dmg)
			{
				int drain = 0;
				if (traceEnt->client->ps.forcePower)
				{
					if (dmg > traceEnt->client->ps.forcePower)
					{
						drain = traceEnt->client->ps.forcePower;
						dmg -= drain;
						traceEnt->client->ps.forcePower = 0;
					}
					else
					{
						drain = dmg;
						traceEnt->client->ps.forcePower -= dmg;
						dmg = 0;
					}
					Jedi_DrainReaction(traceEnt, self);
				}

				int maxHealth = self->client->ps.stats[STAT_MAX_HEALTH];
				if (self->client->ps.forcePowerLevel[FP_DRAIN] > FORCE_LEVEL_2)
				{
					//overcharge health
					maxHealth = floor(static_cast<float>(self->client->ps.stats[STAT_MAX_HEALTH]) * 1.25f);
				}
				if (self->client->ps.stats[STAT_HEALTH] < maxHealth &&
					self->health > 0 && self->client->ps.stats[STAT_HEALTH] > 0)
				{
					self->health += drain + dmg;
					if (self->health > maxHealth)
					{
						self->health = maxHealth;
					}
					self->client->ps.stats[STAT_HEALTH] = self->health;
					if (self->health > self->client->ps.stats[STAT_MAX_HEALTH])
					{
						self->flags |= FL_OVERCHARGED_HEALTH;
					}
				}

				if (dmg)
				{
					//do damage, too
					G_Damage(traceEnt, self, self, dir, impactPoint, dmg, dflags, MOD_FORCE_DRAIN);
					Jedi_DrainReaction(traceEnt, self);
				}
				else if (drain)
				{
					NPC_SetPainEvent(traceEnt);
				}

				if (!Q_irand(0, 2))
				{
					G_Sound(traceEnt, G_SoundIndex("sound/weapons/force/drained.mp3"));
				}

				traceEnt->client->ps.forcePowerRegenDebounceTime = level.time + 800;
				//don't let the client being drained get force power back right away
			}
		}
	}
}

qboolean WP_CheckForceDraineeStopMe(gentity_t* self, gentity_t* drainee)
{
	if (drainee->NPC
		&& drainee->client
		&& drainee->client->ps.forcePowersKnown & 1 << FP_PUSH
		&& level.time - (self->client->ps.forcePowerDebounce[FP_DRAIN] > self->client->ps.forcePowerLevel[FP_DRAIN] *
			500) //at level 1, I always get at least 500ms of drain, at level 3 I get 1500ms
		&& !Q_irand(0, 100 - drainee->NPC->stats.evasion * 10 - g_spskill->integer * 12))
	{
		//a jedi who broke free
		ForceThrow(drainee, qfalse);
		//FIXME: I need to go into some pushed back anim...
		WP_ForcePowerStop(self, FP_DRAIN);
		//can't drain again for 2 seconds
		self->client->ps.forcePowerDebounce[FP_DRAIN] = level.time + 4000;
		return qtrue;
	}
	return qfalse;
}

void ForceShootDrain(gentity_t* self)
{
	trace_t tr;
	int numDrained = 0;

	if (self->health <= 0)
	{
		return;
	}

	if (PM_InLedgeMove(self->client->ps.legsAnim))
	{
		return;
	}

	if (self->client->ps.forcePowerDebounce[FP_DRAIN] <= level.time)
	{
		gentity_t* traceEnt;
		vec3_t forward;
		AngleVectors(self->client->ps.viewangles, forward, nullptr, nullptr);
		VectorNormalize(forward);

		if (self->client->ps.forcePowerLevel[FP_DRAIN] > FORCE_LEVEL_2)
		{
			//arc
			vec3_t center, mins, maxs, v;
			constexpr float radius = MAX_DRAIN_DISTANCE;
			gentity_t* entityList[MAX_GENTITIES];
			int i;

			VectorCopy(self->client->ps.origin, center);
			for (i = 0; i < 3; i++)
			{
				mins[i] = center[i] - radius;
				maxs[i] = center[i] + radius;
			}
			const int numListedEntities = gi.EntitiesInBox(mins, maxs, entityList, MAX_GENTITIES);

			for (int e = 0; e < numListedEntities; e++)
			{
				vec3_t size;
				vec3_t ent_org;
				vec3_t dir;
				// ReSharper disable once CppEntityAssignedButNoRead
				float dot;
				traceEnt = entityList[e];

				if (!traceEnt)
					continue;
				if (traceEnt == self)
					continue;
				if (!traceEnt->inuse)
					continue;
				if (!traceEnt->takedamage)
					continue;
				if (traceEnt->health <= 0) //no torturing corpses
					continue;
				if (!traceEnt->client)
					continue;
				if (self->enemy != traceEnt //not my enemy
					&& OnSameTeam(self, traceEnt)) //on my team
					continue;
				//this is all to see if we need to start a saber attack, if it's in flight, this doesn't matter
				// find the distance from the edge of the bounding box
				for (i = 0; i < 3; i++)
				{
					if (center[i] < traceEnt->absmin[i])
					{
						v[i] = traceEnt->absmin[i] - center[i];
					}
					else if (center[i] > traceEnt->absmax[i])
					{
						v[i] = center[i] - traceEnt->absmax[i];
					}
					else
					{
						v[i] = 0;
					}
				}

				VectorSubtract(traceEnt->absmax, traceEnt->absmin, size);
				VectorMA(traceEnt->absmin, 0.5, size, ent_org);

				//see if they're in front of me
				//must be within the forward cone
				VectorSubtract(ent_org, center, dir);
				VectorNormalize(dir);
				if ((dot = DotProduct(dir, forward)) < 0.5)
					continue;

				//must be close enough
				const float dist = VectorLength(v);
				if (dist >= radius)
				{
					continue;
				}

				//in PVS?
				if (!traceEnt->bmodel && !gi.inPVS(ent_org, self->client->renderInfo.handLPoint))
				{
					//must be in PVS
					continue;
				}

				//Now check and see if we can actually hit it
				gi.trace(&tr, self->client->ps.origin, vec3_origin, vec3_origin, ent_org, self->s.number, MASK_SHOT,
					G2_RETURNONHIT, 10);
				if (tr.fraction < 1.0f && tr.entityNum != traceEnt->s.number)
				{
					//must have clear LOS
					continue;
				}

				if (traceEnt
					&& traceEnt->s.number >= MAX_CLIENTS
					&& traceEnt->client
					&& traceEnt->client->ps.forcePowerLevel[FP_LEVITATION] > FORCE_LEVEL_0 &&
					traceEnt->client->ps.weapon != WP_SABER)
				{
					if (!Q_irand(0, 4) && !jedi_dodge_evasion(traceEnt, self, &tr, HL_NONE))
					{
						//act like we didn't even hit him
						continue;
					}
				}

				// ok, we are within the radius, add us to the incoming list
				if (WP_CheckForceDraineeStopMe(self, traceEnt))
				{
					continue;
				}
				ForceDrainDamage(self, traceEnt, dir, ent_org);
				numDrained++;
			}
		}
		else
		{
			vec3_t end;
			//trace-line
			int ignore = self->s.number;
			int traces = 0;
			vec3_t start;

			VectorCopy(self->client->renderInfo.handLPoint, start);
			VectorMA(start, MAX_DRAIN_DISTANCE, forward, end);

			while (traces < 10)
			{
				//need to loop this in case we hit a Jedi who dodges the shot
				gi.trace(&tr, start, vec3_origin, vec3_origin, end, ignore, MASK_SHOT, G2_RETURNONHIT, 10);
				if (tr.entityNum == ENTITYNUM_NONE || tr.fraction == 1.0 || tr.allsolid || tr.startsolid)
				{
					//always take 1 force point per frame that we're shooting this
					WP_ForcePowerDrain(self, FP_DRAIN, 1);
					return;
				}

				traceEnt = &g_entities[tr.entityNum];

				if (traceEnt
					&& traceEnt->s.number >= MAX_CLIENTS
					&& traceEnt->client
					&& traceEnt->client->ps.forcePowerLevel[FP_LEVITATION] > FORCE_LEVEL_0 &&
					traceEnt->client->ps.weapon != WP_SABER)
				{
					if (!Q_irand(0, 2) && !jedi_dodge_evasion(traceEnt, self, &tr, HL_NONE))
					{
						//act like we didn't even hit him
						VectorCopy(tr.endpos, start);
						ignore = tr.entityNum;
						traces++;
						continue;
					}
				}
				//a Jedi is not dodging this shot
				break;
			}
			traceEnt = &g_entities[tr.entityNum];
			if (!WP_CheckForceDraineeStopMe(self, traceEnt))
			{
				ForceDrainDamage(self, traceEnt, forward, tr.endpos);
			}
			numDrained = 1;
		}

		self->client->ps.forcePowerDebounce[FP_DRAIN] = level.time + 200; //so we don't drain so damn fast!
	}
	self->client->ps.forcePowerRegenDebounceTime = level.time + 500;

	if (!numDrained)
	{
		//always take 1 force point per frame that we're shooting this
		WP_ForcePowerDrain(self, FP_DRAIN, 1);
	}
	else
	{
		WP_ForcePowerDrain(self, FP_DRAIN, numDrained); //was 2, but...
	}
}

void ForceDrainEnt(gentity_t* self, gentity_t* drainEnt)
{
	if (self->health <= 0)
	{
		return;
	}

	if (self->client->ps.forcePowerDebounce[FP_DRAIN] <= level.time)
	{
		if (!drainEnt)
			return;
		if (drainEnt == self)
			return;
		if (!drainEnt->inuse)
			return;
		if (!drainEnt->takedamage)
			return;
		if (drainEnt->health <= 0) //no torturing corpses
			return;
		if (!drainEnt->client)
			return;
		if (OnSameTeam(self, drainEnt))
			return;

		vec3_t fwd;
		AngleVectors(self->client->ps.viewangles, fwd, nullptr, nullptr);

		drainEnt->painDebounceTime = 0;
		ForceDrainDamage(self, drainEnt, fwd, drainEnt->currentOrigin);
		drainEnt->painDebounceTime = level.time + 2000;

		if (drainEnt->s.number)
		{
			if (self->client->ps.forcePowerLevel[FP_DRAIN] > FORCE_LEVEL_2)
			{
				//do damage faster at level 3
				self->client->ps.forcePowerDebounce[FP_DRAIN] = level.time + Q_irand(100, 500);
			}
			else
			{
				self->client->ps.forcePowerDebounce[FP_DRAIN] = level.time + Q_irand(200, 800);
			}
		}
		else
		{
			//player takes damage faster
			self->client->ps.forcePowerDebounce[FP_DRAIN] = level.time + Q_irand(100, 500);
		}
	}

	self->client->ps.forcePowerRegenDebounceTime = level.time + 500;
}

void ForceSeeing(gentity_t* self)
{
	if (self->health <= 0)
	{
		return;
	}

	if (PM_InLedgeMove(self->client->ps.legsAnim))
	{
		return;
	}

	if (self->client->ps.forceAllowDeactivateTime < level.time &&
		self->client->ps.forcePowersActive & 1 << FP_SEE)
	{
		WP_ForcePowerStop(self, FP_SEE);
		return;
	}

	if (!WP_ForcePowerUsable(self, FP_SEE, 0))
	{
		return;
	}

	WP_DebounceForceDeactivateTime(self);

	WP_ForcePowerStart(self, FP_SEE, 0);

	if (self->client->ps.saberLockTime < level.time && !PM_InKnockDown(&self->client->ps))
	{
		if (self->client->ps.forcePowerLevel[FP_SEE] < FORCE_LEVEL_3)
		{
			//animate
			int parts = SETANIM_BOTH;
			if (self->client->ps.forcePowerLevel[FP_SEE] > FORCE_LEVEL_1)
			{
				//level 2 only does it on torso (can keep running)
				parts = SETANIM_TORSO;
			}
			else
			{
				if (self->client->ps.groundEntityNum != ENTITYNUM_NONE)
				{
					VectorClear(self->client->ps.velocity);
				}
				if (self->NPC)
				{
					VectorClear(self->client->ps.moveDir);
					self->client->ps.speed = 0;
				}
			}
			NPC_SetAnim(self, parts, BOTH_FORCE_DRAIN_HOLD, SETANIM_FLAG_OVERRIDE | SETANIM_FLAG_HOLD);

			//don't move or attack during this anim
			if (self->client->ps.forcePowerLevel[FP_SEE] < FORCE_LEVEL_2)
			{
				self->client->ps.pm_flags |= PMF_TIME_KNOCKBACK;
				self->client->ps.pm_time = self->client->ps.weaponTime = self->client->ps.torsoAnimTimer;

				if (self->s.number)
				{
					//NPC
					self->painDebounceTime = level.time + self->client->ps.torsoAnimTimer;
				}
				else
				{
					//player
					self->aimDebounceTime = level.time + self->client->ps.torsoAnimTimer;
				}
			}
			else
			{
				self->client->ps.weaponTime = self->client->ps.torsoAnimTimer;
			}
		}
		else
		{
			NPC_SetAnim(self, SETANIM_TORSO, BOTH_FORCE_DRAIN_HOLD, SETANIM_FLAG_OVERRIDE | SETANIM_FLAG_HOLD);
			self->client->ps.weaponTime = self->client->ps.torsoAnimTimer;
		}
	}

	G_SoundOnEnt(self, CHAN_ITEM, "sound/weapons/force/see.wav");
}

void ForceProtect(gentity_t* self)
{
	if (self->health <= 0)
	{
		return;
	}

	if (self->client->ps.forceAllowDeactivateTime < level.time &&
		self->client->ps.forcePowersActive & 1 << FP_PROTECT)
	{
		WP_ForcePowerStop(self, FP_PROTECT);
		return;
	}

	if (PM_InLedgeMove(self->client->ps.legsAnim))
	{
		return;
	}

	if (!WP_ForcePowerUsable(self, FP_PROTECT, 0))
	{
		return;
	}

	// Make sure to turn off Force Rage.
	if (self->client->ps.forcePowersActive & 1 << FP_RAGE)
	{
		WP_ForcePowerStop(self, FP_RAGE);
	}

	WP_DebounceForceDeactivateTime(self);

	WP_ForcePowerStart(self, FP_PROTECT, 0);

	if (self->client->ps.saberLockTime < level.time && !PM_InKnockDown(&self->client->ps))
	{
		if (self->client->ps.forcePowerLevel[FP_PROTECT] < FORCE_LEVEL_3)
		{
			//animate
			int parts = SETANIM_BOTH;
			if (self->client->ps.forcePowerLevel[FP_PROTECT] > FORCE_LEVEL_1)
			{
				//level 2 only does it on torso (can keep running)
				parts = SETANIM_TORSO;
			}
			else
			{
				if (self->client->ps.groundEntityNum != ENTITYNUM_NONE)
				{
					VectorClear(self->client->ps.velocity);
				}
				if (self->NPC)
				{
					VectorClear(self->client->ps.moveDir);
					self->client->ps.speed = 0;
				}
			}
			NPC_SetAnim(self, parts, BOTH_FORCE_PROTECT, SETANIM_FLAG_OVERRIDE | SETANIM_FLAG_HOLD);

			//don't move or attack during this anim
			if (self->client->ps.forcePowerLevel[FP_PROTECT] < FORCE_LEVEL_2)
			{
				self->client->ps.pm_flags |= PMF_TIME_KNOCKBACK;
				self->client->ps.pm_time = self->client->ps.weaponTime = self->client->ps.torsoAnimTimer;

				if (self->s.number)
				{
					//NPC
					self->painDebounceTime = level.time + self->client->ps.torsoAnimTimer;
				}
				else
				{
					//player
					self->aimDebounceTime = level.time + self->client->ps.torsoAnimTimer;
				}
			}
			else
			{
				self->client->ps.weaponTime = self->client->ps.torsoAnimTimer;
			}
		}
		else
		{
			NPC_SetAnim(self, SETANIM_TORSO, BOTH_FORCE_PROTECT_FAST, SETANIM_FLAG_OVERRIDE | SETANIM_FLAG_HOLD);
			self->client->ps.weaponTime = self->client->ps.torsoAnimTimer;
		}
	}
}

void ForceAbsorb(gentity_t* self)
{
	if (self->health <= 0)
	{
		return;
	}

	if (self->client->ps.forceAllowDeactivateTime < level.time &&
		self->client->ps.forcePowersActive & 1 << FP_ABSORB)
	{
		WP_ForcePowerStop(self, FP_ABSORB);
		return;
	}

	if (PM_InLedgeMove(self->client->ps.legsAnim))
	{
		return;
	}

	if (!WP_ForcePowerUsable(self, FP_ABSORB, 0))
	{
		return;
	}

	// Make sure to turn off Force Rage and Force Protection.
	if (self->client->ps.forcePowersActive & 1 << FP_RAGE)
	{
		WP_ForcePowerStop(self, FP_RAGE);
	}

	WP_DebounceForceDeactivateTime(self);

	WP_ForcePowerStart(self, FP_ABSORB, 0);

	if (self->client->ps.saberLockTime < level.time && !PM_InKnockDown(&self->client->ps))
	{
		if (self->client->ps.forcePowerLevel[FP_ABSORB] < FORCE_LEVEL_3)
		{
			//animate
			int parts = SETANIM_BOTH;
			if (self->client->ps.forcePowerLevel[FP_ABSORB] > FORCE_LEVEL_1)
			{
				//level 2 only does it on torso (can keep running)
				parts = SETANIM_TORSO;
			}
			else
			{
				if (self->client->ps.groundEntityNum != ENTITYNUM_NONE)
				{
					VectorClear(self->client->ps.velocity);
				}
				if (self->NPC)
				{
					VectorClear(self->client->ps.moveDir);
					self->client->ps.speed = 0;
				}
			}
			NPC_SetAnim(self, parts, BOTH_FORCE_ABSORB, SETANIM_FLAG_OVERRIDE | SETANIM_FLAG_HOLD);

			//don't move or attack during this anim
			if (self->client->ps.forcePowerLevel[FP_ABSORB] < FORCE_LEVEL_2)
			{
				self->client->ps.pm_flags |= PMF_TIME_KNOCKBACK;
				self->client->ps.pm_time = self->client->ps.weaponTime = self->client->ps.torsoAnimTimer;

				if (self->s.number)
				{
					//NPC
					self->painDebounceTime = level.time + self->client->ps.torsoAnimTimer;
				}
				else
				{
					//player
					self->aimDebounceTime = level.time + self->client->ps.torsoAnimTimer;
				}
			}
			else
			{
				self->client->ps.weaponTime = self->client->ps.torsoAnimTimer;
			}
		}
		else
		{
			NPC_SetAnim(self, SETANIM_TORSO, BOTH_FORCE_ABSORB, SETANIM_FLAG_OVERRIDE | SETANIM_FLAG_HOLD);
			self->client->ps.weaponTime = self->client->ps.torsoAnimTimer;
		}
	}
}

void ForceRage(gentity_t* self)
{
	if (self->health <= 0)
	{
		return;
	}

	if (self->client->ps.forceAllowDeactivateTime < level.time &&
		self->client->ps.forcePowersActive & 1 << FP_RAGE)
	{
		WP_ForcePowerStop(self, FP_RAGE);
		return;
	}

	if (PM_InLedgeMove(self->client->ps.legsAnim))
	{
		return;
	}

	if (!WP_ForcePowerUsable(self, FP_RAGE, 0))
	{
		return;
	}

	if (self->client->ps.forceRageRecoveryTime >= level.time)
	{
		return;
	}

	if (self->s.number < MAX_CLIENTS
		&& self->health < 25)
	{
		//have to have at least 25 health to start it
		return;
	}

	if (self->health < 10)
	{
		return;
	}

	// Make sure to turn off Force Protection and Force Absorb.
	if (self->client->ps.forcePowersActive & 1 << FP_PROTECT)
	{
		WP_ForcePowerStop(self, FP_PROTECT);
	}

	if (self->client->ps.forcePowersActive & 1 << FP_ABSORB)
	{
		WP_ForcePowerStop(self, FP_ABSORB);
	}

	WP_DebounceForceDeactivateTime(self);

	WP_ForcePowerStart(self, FP_RAGE, 0);

	if (self->client->ps.saberLockTime < level.time && !PM_InKnockDown(&self->client->ps))
	{
		if (self->client->ps.forcePowerLevel[FP_RAGE] < FORCE_LEVEL_3)
		{
			//animate
			int parts = SETANIM_BOTH;
			if (self->client->ps.forcePowerLevel[FP_RAGE] > FORCE_LEVEL_1)
			{
				//level 2 only does it on torso (can keep running)
				parts = SETANIM_TORSO;
			}
			else
			{
				if (self->client->ps.groundEntityNum != ENTITYNUM_NONE)
				{
					VectorClear(self->client->ps.velocity);
				}
				if (self->NPC)
				{
					VectorClear(self->client->ps.moveDir);
					self->client->ps.speed = 0;
				}
			}
			NPC_SetAnim(self, parts, BOTH_FORCE_RAGE, SETANIM_FLAG_OVERRIDE | SETANIM_FLAG_HOLD);

			//don't move or attack during this anim
			if (self->client->ps.forcePowerLevel[FP_RAGE] < FORCE_LEVEL_2)
			{
				self->client->ps.pm_flags |= PMF_TIME_KNOCKBACK;
				self->client->ps.pm_time = self->client->ps.weaponTime = self->client->ps.torsoAnimTimer;

				if (self->s.number)
				{
					//NPC
					self->painDebounceTime = level.time + self->client->ps.torsoAnimTimer;
				}
				else
				{
					//player
					self->aimDebounceTime = level.time + self->client->ps.torsoAnimTimer;
				}
			}
			else
			{
				self->client->ps.weaponTime = self->client->ps.torsoAnimTimer;
			}
		}
		else
		{
			NPC_SetAnim(self, SETANIM_TORSO, BOTH_FORCE_RAGE, SETANIM_FLAG_OVERRIDE | SETANIM_FLAG_HOLD);
			self->client->ps.weaponTime = self->client->ps.torsoAnimTimer;
		}
	}

	if (self->client->NPC_class == CLASS_WOOKIE)
	{
		G_SoundOnEnt(self, CHAN_VOICE, "sound/chars/chewbacca/misc/death2.mp3");
	}
	CG_PlayEffectBolted("misc/breath.efx", self->playerModel, self->headBolt, self->s.number, self->currentOrigin);
}

void ForceJumpCharge(const gentity_t* self, usercmd_t* ucmd)
{
	const float forceJumpChargeInterval = forceJumpStrength[0] / (FORCE_JUMP_CHARGE_TIME / FRAMETIME);

	if (self->health <= 0)
	{
		return;
	}
	if (!self->s.number && cg.zoomMode)
	{
		//can't force jump when zoomed in
		return;
	}

	if (PM_InLedgeMove(self->client->ps.legsAnim))
	{
		return;
	}

	//need to play sound
	if (!self->client->ps.forceJumpCharge)
	{
		//FIXME: this should last only as long as the actual charge-up
		G_SoundOnEnt(self, CHAN_BODY, "sound/weapons/force/jumpbuild.wav");
	}
	//Increment
	self->client->ps.forceJumpCharge += forceJumpChargeInterval;

	//clamp to max strength for current level
	if (self->client->ps.forceJumpCharge > forceJumpStrength[self->client->ps.forcePowerLevel[FP_LEVITATION]])
	{
		self->client->ps.forceJumpCharge = forceJumpStrength[self->client->ps.forcePowerLevel[FP_LEVITATION]];
	}

	//clamp to max available force power
	if (self->client->ps.forceJumpCharge / forceJumpChargeInterval / (FORCE_JUMP_CHARGE_TIME / FRAMETIME) *
		force_power_needed[FP_LEVITATION] > self->client->ps.forcePower)
	{
		//can't use more than you have
		self->client->ps.forceJumpCharge = self->client->ps.forcePower * forceJumpChargeInterval / (
			FORCE_JUMP_CHARGE_TIME / FRAMETIME);
	}
	//FIXME: a simple tap should always do at least a normal height's jump?
}

int WP_GetVelocityForForceJump(const gentity_t* self, vec3_t jumpVel, const usercmd_t* ucmd)
{
	float pushFwd = 0, pushRt = 0;
	vec3_t view, forward, right;
	VectorCopy(self->client->ps.viewangles, view);
	view[0] = 0;
	AngleVectors(view, forward, right, nullptr);
	if (ucmd->forwardmove && ucmd->rightmove)
	{
		if (ucmd->forwardmove > 0)
		{
			pushFwd = 50;
		}
		else
		{
			pushFwd = -50;
		}
		if (ucmd->rightmove > 0)
		{
			pushRt = 50;
		}
		else
		{
			pushRt = -50;
		}
	}
	else if (ucmd->forwardmove || ucmd->rightmove)
	{
		if (ucmd->forwardmove > 0)
		{
			pushFwd = 100;
		}
		else if (ucmd->forwardmove < 0)
		{
			pushFwd = -100;
		}
		else if (ucmd->rightmove > 0)
		{
			pushRt = 100;
		}
		else if (ucmd->rightmove < 0)
		{
			pushRt = -100;
		}
	}
	VectorMA(self->client->ps.velocity, pushFwd, forward, jumpVel);
	VectorMA(self->client->ps.velocity, pushRt, right, jumpVel);
	jumpVel[2] += self->client->ps.forceJumpCharge; //forceJumpStrength;
	if (pushFwd > 0 && self->client->ps.forceJumpCharge > 200)
	{
		return FJ_FORWARD;
	}
	if (pushFwd < 0 && self->client->ps.forceJumpCharge > 200)
	{
		return FJ_BACKWARD;
	}
	if (pushRt > 0 && self->client->ps.forceJumpCharge > 200)
	{
		return FJ_RIGHT;
	}
	if (pushRt < 0 && self->client->ps.forceJumpCharge > 200)
	{
		return FJ_LEFT;
	}
	//FIXME: jump straight up anim
	return FJ_UP;
}

void ForceJump(gentity_t* self, const usercmd_t* ucmd)
{
	if (self->client->ps.forcePowerDuration[FP_LEVITATION] > level.time)
	{
		return;
	}
	if (!WP_ForcePowerUsable(self, FP_LEVITATION, 0))
	{
		return;
	}

	if (self->s.groundEntityNum == ENTITYNUM_NONE)
	{
		return;
	}

	if (PM_InLedgeMove(self->client->ps.legsAnim))
	{
		return;
	}
	if (self->client->ps.pm_flags & PMF_JUMP_HELD)
	{
		return;
	}
	if (self->health <= 0)
	{
		return;
	}
	if (!self->s.number && (cg.zoomMode || in_camera))
	{
		//can't force jump when zoomed in or in cinematic
		return;
	}
	if (self->client->ps.saberLockTime > level.time)
	{
		//FIXME: can this be a way to break out?
		return;
	}

	if (self->client->NPC_class == CLASS_BOBAFETT
		|| self->client->NPC_class == CLASS_MANDO
		|| self->client->NPC_class == CLASS_ROCKETTROOPER)
	{
		if (self->client->ps.forceJumpCharge > 300)
		{
			JET_FlyStart(NPC);
		}
		else
		{
			G_AddEvent(self, EV_JUMP, 0);
		}
	}
	else
	{
		if (self->client->ps.forcePowerLevel[FP_LEVITATION] < FORCE_LEVEL_3)
		{
			//short burst
			G_SoundOnEnt(self, CHAN_BODY, "sound/weapons/force/jumpsmall.mp3");
		}
		else
		{
			//holding it
			G_SoundOnEnt(self, CHAN_BODY, "sound/weapons/force/jump.mp3");
		}
	}

	const float forceJumpChargeInterval = forceJumpStrength[self->client->ps.forcePowerLevel[FP_LEVITATION]] / (
		FORCE_JUMP_CHARGE_TIME / FRAMETIME);

	int anim;
	vec3_t jumpVel;

	switch (WP_GetVelocityForForceJump(self, jumpVel, ucmd))
	{
	case FJ_FORWARD:
		if ((self->client->NPC_class == CLASS_BOBAFETT ||
			self->client->NPC_class == CLASS_MANDO || self->client->NPC_class == CLASS_ROCKETTROOPER) && self->
			client->ps.forceJumpCharge > 300
			|| self->client->ps.saber[0].saberFlags & SFL_NO_FLIPS
			|| self->client->ps.dualSabers && self->client->ps.saber[1].saberFlags & SFL_NO_FLIPS
			|| self->NPC &&
			self->NPC->rank != RANK_CREWMAN &&
			self->NPC->rank <= RANK_LT_JG)
		{
			//can't do acrobatics
			if (self->s.weapon == WP_SABER)
			{
				anim = BOTH_FORCEJUMP2;
			}
			else
			{
				anim = BOTH_FORCEJUMP1;
			}
		}
		else
		{
			if (self->client->NPC_class == CLASS_ALORA && Q_irand(0, 3))
			{
				anim = Q_irand(BOTH_ALORA_FLIP_1, BOTH_ALORA_FLIP_3);
			}
			else
			{
				anim = BOTH_FLIP_F;
			}
		}
		break;
	case FJ_BACKWARD:
		if ((self->client->NPC_class == CLASS_BOBAFETT ||
			self->client->NPC_class == CLASS_MANDO || self->client->NPC_class == CLASS_ROCKETTROOPER)
			&& self->client->ps.forceJumpCharge > 300
			|| self->client->ps.saber[0].saberFlags & SFL_NO_FLIPS
			|| self->client->ps.dualSabers && self->client->ps.saber[1].saberFlags & SFL_NO_FLIPS
			|| self->NPC
			&& self->NPC->rank != RANK_CREWMAN
			&& self->NPC->rank <= RANK_LT_JG)
		{
			//can't do acrobatics
			anim = BOTH_FORCEJUMPBACK1;
		}
		else
		{
			anim = BOTH_FLIP_B;
		}
		break;
	case FJ_RIGHT:
		if ((self->client->NPC_class == CLASS_BOBAFETT ||
			self->client->NPC_class == CLASS_MANDO || self->client->NPC_class == CLASS_ROCKETTROOPER) && self->
			client->ps.forceJumpCharge > 300
			|| self->client->ps.saber[0].saberFlags & SFL_NO_FLIPS
			|| self->client->ps.dualSabers && self->client->ps.saber[1].saberFlags & SFL_NO_FLIPS
			|| self->NPC &&
			self->NPC->rank != RANK_CREWMAN &&
			self->NPC->rank <= RANK_LT_JG)
		{
			//can't do acrobatics
			anim = BOTH_FORCEJUMPRIGHT1;
		}
		else
		{
			anim = BOTH_FLIP_R;
		}
		break;
	case FJ_LEFT:
		if ((self->client->NPC_class == CLASS_BOBAFETT ||
			self->client->NPC_class == CLASS_MANDO || self->client->NPC_class == CLASS_ROCKETTROOPER) && self->
			client->ps.forceJumpCharge > 300
			|| self->client->ps.saber[0].saberFlags & SFL_NO_FLIPS
			|| self->client->ps.dualSabers && self->client->ps.saber[1].saberFlags & SFL_NO_FLIPS
			|| self->NPC &&
			self->NPC->rank != RANK_CREWMAN &&
			self->NPC->rank <= RANK_LT_JG)
		{
			//can't do acrobatics
			anim = BOTH_FORCEJUMPLEFT1;
		}
		else
		{
			anim = BOTH_FLIP_L;
		}
		break;
	default:
	case FJ_UP:
		if (self->s.weapon == WP_SABER)
		{
			anim = BOTH_JUMP2;
		}
		else
		{
			anim = BOTH_JUMP1;
		}
		break;
	}

	int parts = SETANIM_BOTH;
	if (self->client->ps.weaponTime)
	{
		//FIXME: really only care if we're in a saber attack anim.. maybe trail length?
		parts = SETANIM_LEGS;
	}

	NPC_SetAnim(self, parts, anim, SETANIM_FLAG_OVERRIDE | SETANIM_FLAG_HOLD);

	//FIXME: sound effect
	self->client->ps.forceJumpZStart = self->currentOrigin[2]; //remember this for when we land
	VectorCopy(jumpVel, self->client->ps.velocity);

	WP_ForcePowerStart(self, FP_LEVITATION,
		self->client->ps.forceJumpCharge / forceJumpChargeInterval / (FORCE_JUMP_CHARGE_TIME / FRAMETIME)
		* force_power_needed[FP_LEVITATION]);
	self->client->ps.forceJumpCharge = 0;
}

constexpr auto DESTRUCTION_VELOCITY = 1200;
constexpr auto DESTRUCTION_DAMAGE = 40;
constexpr auto DESTRUCTION_SPLASH_DAMAGE = 30;
constexpr auto DESTRUCTION_SPLASH_RADIUS = 160;
constexpr auto DESTRUCTION_NPC_DAMAGE_EASY = 10;
constexpr auto DESTRUCTION_NPC_DAMAGE_NORMAL = 20;
constexpr auto DESTRUCTION_NPC_DAMAGE_HARD = 30;
constexpr auto DESTRUCTION_SIZE = 3;

gentity_t* create_missile(vec3_t org, vec3_t dir, float vel, int life, gentity_t* owner, qboolean altFire = qfalse);
//---------------------------------------------------------
void WP_FireDestruction(gentity_t* ent, int forceLevel)
//---------------------------------------------------------
{
	vec3_t start, forward;
	int damage = DESTRUCTION_DAMAGE;
	float vel = DESTRUCTION_VELOCITY;

	if (forceLevel == FORCE_LEVEL_2)
	{
		vel *= 1.5f;
	}
	else if (forceLevel == FORCE_LEVEL_3)
	{
		vel *= 2.0f;
	}

	AngleVectors(ent->client->ps.viewangles, forward, nullptr, nullptr);
	VectorNormalize(forward);

	VectorCopy(ent->client->renderInfo.eyePoint, start);

	gentity_t* missile = create_missile(start, forward, vel, 10000, ent, qfalse);

	missile->classname = "rocket_proj";
	missile->s.weapon = WP_CONCUSSION;
	missile->s.powerups |= 1 << PW_FORCE_PROJECTILE;
	missile->mass = 10;

	// Do the damages
	if (ent->s.number != 0)
	{
		if (g_spskill->integer == 0)
		{
			damage = DESTRUCTION_NPC_DAMAGE_EASY;
		}
		else if (g_spskill->integer == 1)
		{
			damage = DESTRUCTION_NPC_DAMAGE_NORMAL;
		}
		else
		{
			damage = DESTRUCTION_NPC_DAMAGE_HARD;
		}
	}

	// Make it easier to hit things
	VectorSet(missile->maxs, DESTRUCTION_SIZE, DESTRUCTION_SIZE, DESTRUCTION_SIZE);
	VectorScale(missile->maxs, -1, missile->mins);

	missile->damage = damage * (1.0f + forceLevel) / 2.0f;
	missile->dflags = DAMAGE_DEATH_KNOCKBACK | DAMAGE_EXTRA_KNOCKBACK;

	missile->methodOfDeath = MOD_DESTRUCTION;
	missile->splashMethodOfDeath = MOD_DESTRUCTION; // ?SPLASH;

	missile->clipmask = MASK_SHOT;
	missile->splashDamage = DESTRUCTION_SPLASH_DAMAGE * (1.0f + forceLevel) / 2.0f;
	missile->splashRadius = DESTRUCTION_SPLASH_RADIUS * (1.0f + forceLevel) / 2.0f;

	// we don't want it to ever bounce
	missile->bounceCount = 0;
}

void ForceDestruction(gentity_t* self)
{
	int anim;
	if (self->health <= 0)
	{
		return;
	}
	if (!WP_ForcePowerUsable(self, FP_DESTRUCTION, 0))
	{
		return;
	}
	if (self->client->ps.forcePowerDebounce[FP_DESTRUCTION] > level.time)
	{
		//already using destruction
		return;
	}

	if (PM_InLedgeMove(self->client->ps.legsAnim))
	{
		return;
	}
	if (!self->s.number && (cg.zoomMode || in_camera))
	{
		//can't destruction when zoomed in or in cinematic
		return;
	}
	if (self->client->ps.saberLockTime > level.time)
	{
		//FIXME: can this be a way to break out?
		return;
	}

	if (self->s.number >= MAX_CLIENTS && !G_ControlledByPlayer(self) && self->client->ps.weapon == WP_SABER)
		//npc force use limit
	{
		if (self->client->ps.blockPoints < 75 || self->client->ps.forcePower < 75)
		{
			return;
		}
	}

	if (self->s.weapon == WP_MELEE ||
		self->s.weapon == WP_NONE ||
		self->s.weapon == WP_SABER && !self->client->ps.SaberActive())
	{
		//2-handed PUSH
		if (self->client->ps.groundEntityNum == ENTITYNUM_NONE)
		{
			anim = BOTH_2HANDPUSH;

			if (self->handLBolt != -1)
			{
				G_PlayEffect(G_EffectIndex("force/drain_hand"), self->playerModel, self->handLBolt, self->s.number,
					self->currentOrigin, 200, qtrue);
			}

			if (self->handRBolt != -1)
			{
				G_PlayEffect(G_EffectIndex("force/drain_hand"), self->playerModel, self->handRBolt, self->s.number,
					self->currentOrigin, 200, qtrue);
			}
		}
		else
		{
			if (self->s.eFlags & EF_FORCE_DRAINED || self->s.eFlags & EF_FORCE_GRIPPED || self->s.eFlags &
				EF_FORCE_GRABBED)
			{
				anim = BOTH_FORCEPUSH;

				if (self->handLBolt != -1)
				{
					G_PlayEffect(G_EffectIndex("force/drain_hand"), self->playerModel, self->handLBolt, self->s.number,
						self->currentOrigin, 200, qtrue);
				}
			}
			else
			{
				anim = BOTH_2HANDPUSH;

				if (self->handLBolt != -1)
				{
					G_PlayEffect(G_EffectIndex("force/drain_hand"), self->playerModel, self->handLBolt, self->s.number,
						self->currentOrigin, 200, qtrue);
				}

				if (self->handRBolt != -1)
				{
					G_PlayEffect(G_EffectIndex("force/drain_hand"), self->playerModel, self->handRBolt, self->s.number,
						self->currentOrigin, 200, qtrue);
				}
			}
		}
	}
	else
	{
		anim = BOTH_FORCEPUSH;

		if (self->handLBolt != -1)
		{
			G_PlayEffect(G_EffectIndex("force/drain_hand"), self->playerModel, self->handLBolt, self->s.number,
				self->currentOrigin, 200, qtrue);
		}
	}
	const int soundIndex = G_SoundIndex("sound/weapons/force/destruction.mp3");

	int parts = SETANIM_TORSO;
	if (!PM_InKnockDown(&self->client->ps))
	{
		if (!VectorLengthSquared(self->client->ps.velocity) && !(self->client->ps.pm_flags & PMF_DUCKED))
		{
			parts = SETANIM_BOTH;
		}
	}
	NPC_SetAnim(self, parts, anim, SETANIM_FLAG_OVERRIDE | SETANIM_FLAG_HOLD | SETANIM_FLAG_RESTART);
	self->client->ps.saberMove = self->client->ps.saberBounceMove = LS_READY;
	//don't finish whatever saber anim you may have been in
	self->client->ps.saberBlocked = BLOCKED_NONE;

	G_Sound(self, soundIndex);

	WP_FireDestruction(self, self->client->ps.forcePowerLevel[FP_DESTRUCTION]);

	WP_ForcePowerStart(self, FP_DESTRUCTION, 0);

	self->client->ps.weaponTime = 1000;
	if (self->client->ps.forcePowersActive & 1 << FP_SPEED)
	{
		self->client->ps.weaponTime = floor(self->client->ps.weaponTime * g_timescale->value);
	}
	self->client->ps.forcePowerDebounce[FP_DESTRUCTION] = level.time + self->client->ps.torsoAnimTimer + 3500;
}

qboolean PlayerAffectedByStasis()
{
	const gentity_t* ent = &g_entities[0];
	if (ent && ent->client && ent->client->ps.stasisTime > (cg.time ? cg.time : level.time))
	{
		return qtrue;
	}
	if (ent && ent->client && ent->client->ps.stasisJediTime > (cg.time ? cg.time : level.time))
	{
		return qtrue;
	}
	return qfalse;
}

extern void PM_SetTorsoAnimTimer(gentity_t* ent, int* torsoAnimTimer, int time);

void ForceStasisWide(const gentity_t* self, gentity_t* traceEnt, vec3_t dir, float dist, float dot, vec3_t impactPoint)
{
	float currentFrame, animSpeed;
	int junk;
	trace_t tr;

	if (!traceEnt)
	{
		vec3_t end;
		//okay, trace straight ahead and see what's there
		gi.trace(&tr, self->client->renderInfo.handLPoint, vec3_origin, vec3_origin, end, self->s.number,
			MASK_OPAQUE | CONTENTS_SOLID | CONTENTS_BODY | CONTENTS_ITEM | CONTENTS_CORPSE | MASK_SHOT,
			static_cast<EG2_Collision>(0), 0);
		if (tr.entityNum >= ENTITYNUM_WORLD || tr.fraction == 1.0 || tr.allsolid || tr.startsolid)
		{
			if (g_stasistems->integer)
			{
				// One more try...try and physically check for entities in a box, similar to push
				vec3_t mins, maxs;

				for (int i = 0; i < 3; i++)
				{
					mins[i] = self->currentOrigin[i] - 512;
					maxs[i] = self->currentOrigin[i] + 512;
				}

				gentity_t* entlist[MAX_GENTITIES];
				const int numListedEntities = gi.EntitiesInBox(mins, maxs, entlist, MAX_GENTITIES);
				vec3_t forward, vwangles, traceend;

				VectorCopy(self->currentAngles, vwangles);
				AngleVectors(vwangles, forward, nullptr, nullptr);
				VectorMA(self->client->renderInfo.eyePoint, 512.0f, forward, traceend);

				trace_t tr2;
				gi.trace(&tr2, self->client->renderInfo.eyePoint, vec3_origin, vec3_origin, traceend, self->s.number,
					MASK_OPAQUE | CONTENTS_SOLID | CONTENTS_BODY | CONTENTS_ITEM | CONTENTS_CORPSE | MASK_SHOT,
					static_cast<EG2_Collision>(0), 0);
				const gentity_t* fwdEnt = &g_entities[tr2.entityNum];
				qboolean fwdEntIsCorrect = qfalse;
				for (int i = 0; i < numListedEntities; i++)
				{
					const gentity_t* targEnt = entlist[i];
					if (targEnt->s.eType == ET_ITEM || targEnt->s.eType == ET_MISSILE || targEnt->s.eType == ET_GENERAL)
					{
						if (targEnt != fwdEnt)
						{
							continue;
						}
						fwdEntIsCorrect = qtrue;
					}
				}

				if (fwdEntIsCorrect)
				{
					tr.entityNum = fwdEnt->s.number;
				}
				else
				{
					return;
				}
			}
			else
			{
				return;
			}
		}
		traceEnt = &g_entities[tr.entityNum];
		G_AddVoiceEvent(traceEnt, Q_irand(EV_PUSHED1, EV_PUSHED3), 2000);
	}

	if (traceEnt->NPC && traceEnt->NPC->scriptFlags & SCF_NO_FORCE)
	{
		return;
	}

	if (traceEnt && traceEnt->takedamage)
	{
		if (!traceEnt->client || traceEnt->client->playerTeam != self->client->playerTeam || self->enemy == traceEnt ||
			traceEnt->enemy == self)
		{
			// ReSharper disable once CppEntityAssignedButNoRead
			int actual_time;
			//an enemy or object
			if (traceEnt->health > 0 &&
				traceEnt->s.weapon != WP_SABER && traceEnt->client && self->client->NPC_class != CLASS_REBORN)
			{
				//doesn't affect jedi .but affects everything else??
				if (traceEnt->client)
				{
					traceEnt->client->ps.stasisTime = level.time + stasisTime[self->client->ps.forcePowerLevel[
						FP_STASIS]];
					VectorClear(traceEnt->client->ps.velocity);
					player_Freeze(traceEnt);

					if (traceEnt->client->NPC_class == CLASS_BOBAFETT ||
						traceEnt->client->NPC_class == CLASS_MANDO)
					{
						// also disables npc jetpack
						Boba_FlyStop(traceEnt);
						if (traceEnt->client->jetPackOn)
						{
							//disable jetpack temporarily
							Jetpack_Off(traceEnt);
							traceEnt->client->jetPackToggleTime = level.time + Q_irand(3000, 10000);
						}
					}
				}

				if (gi.G2API_HaveWeGhoul2Models(traceEnt->ghoul2))
				{
					actual_time = cg.time ? cg.time : level.time;
					gi.G2API_GetBoneAnimIndex(&traceEnt->ghoul2[traceEnt->playerModel], traceEnt->rootBone,
						level.time, &currentFrame, &junk, &junk, &junk, &animSpeed, nullptr);

					gi.G2API_SetBoneAnimIndex(&traceEnt->ghoul2[traceEnt->playerModel], traceEnt->rootBone,
						currentFrame, currentFrame + 1,
						BONE_ANIM_OVERRIDE_FREEZE, animSpeed, level.time, currentFrame, 100);
					if (traceEnt->headModel > 0)
					{
						gi.G2API_SetBoneAnimIndex(&traceEnt->ghoul2[traceEnt->headModel], traceEnt->headRootBone,
							currentFrame, currentFrame + 1,
							BONE_ANIM_OVERRIDE_FREEZE, animSpeed, level.time, currentFrame, 100);
					}
				}
			}
			else if (traceEnt->health > 0 &&
				traceEnt->s.weapon == WP_SABER && traceEnt->client && traceEnt->client->ps.forcePower <= 75)
			{
				//affect jedi.
				if (traceEnt->client)
				{
					traceEnt->client->ps.stasisJediTime = level.time + stasisJediTime[self->client->ps.forcePowerLevel[
						FP_STASIS]];
					VectorClear(traceEnt->client->ps.velocity);
					player_Freeze(traceEnt);

					if (traceEnt->client->NPC_class == CLASS_BOBAFETT ||
						traceEnt->client->NPC_class == CLASS_MANDO)
					{
						// also disables npc jetpack
						Boba_FlyStop(traceEnt);
						if (traceEnt->client->jetPackOn)
						{
							//disable jetpack temporarily
							Jetpack_Off(traceEnt);
							traceEnt->client->jetPackToggleTime = level.time + Q_irand(3000, 10000);
						}
					}
				}

				if (gi.G2API_HaveWeGhoul2Models(traceEnt->ghoul2))
				{
					actual_time = cg.time ? cg.time : level.time;
					gi.G2API_GetBoneAnimIndex(&traceEnt->ghoul2[traceEnt->playerModel], traceEnt->rootBone,
						level.time, &currentFrame, &junk, &junk, &junk, &animSpeed, nullptr);

					gi.G2API_SetBoneAnimIndex(&traceEnt->ghoul2[traceEnt->playerModel], traceEnt->rootBone,
						currentFrame, currentFrame + 1,
						BONE_ANIM_OVERRIDE_FREEZE, animSpeed, level.time, currentFrame, 100);
					if (traceEnt->headModel > 0)
					{
						gi.G2API_SetBoneAnimIndex(&traceEnt->ghoul2[traceEnt->headModel], traceEnt->headRootBone,
							currentFrame, currentFrame + 1,
							BONE_ANIM_OVERRIDE_FREEZE, animSpeed, level.time, currentFrame, 100);
					}
				}
				if (d_slowmoaction->integer && (self->s.number < MAX_CLIENTS || G_ControlledByPlayer(self)))
				{
					G_StartStasisEffect(self);
				}
			}
		}
	}
	else
	{
		if (g_stasistems->integer && (traceEnt->s.eType == ET_ITEM || traceEnt->s.eType == ET_MISSILE || traceEnt->s.
			eType == ET_GENERAL))
		{
			/* WRONG! */
		}
		else
		{
			Player_CheckFreeze(traceEnt);
		}
	}
}

void force_stasis(gentity_t* self)
{
	trace_t tr;
	vec3_t forward;
	gentity_t* traceEnt = nullptr;
	int anim, soundIndex;
	float currentFrame, animSpeed;
	int radius;
	int junk;
	int actual_time;

	if (self->health <= 0)
	{
		return;
	}

	if (PM_InLedgeMove(self->client->ps.legsAnim))
	{
		return;
	}

	if (PM_InGetUp(&self->client->ps) || PM_InForceGetUp(&self->client->ps))
	{
		return;
	}

	if (self->s.number >= MAX_CLIENTS && !G_ControlledByPlayer(self) && self->client->ps.weapon == WP_SABER)
		//npc force use limit
	{
		if (self->client->ps.blockPoints < 50 || self->client->ps.forcePower < 50 || !WP_ForcePowerUsable(
			self, FP_STASIS, 0))
		{
			return;
		}
	}
	else
	{
		if (self->client->ps.forcePower < 25 || !WP_ForcePowerUsable(self, FP_STASIS, 0))
		{
			return;
		}
	}

	if (!self->s.number && (cg.zoomMode || in_camera))
	{
		//can't stasis when zoomed in or in cinematic
		return;
	}

	if (self->client->ps.saberLockTime > level.time)
	{
		return;
	}

	radius = forceStasisRadius[self->client->ps.forcePowerLevel[FP_STASIS]];

	if (self->client->ps.forcePowerLevel[FP_STASIS] > FORCE_LEVEL_2)
	{
		//Stasis the enemy!
		AngleVectors(self->client->ps.viewangles, forward, nullptr, nullptr);
		VectorNormalize(forward);
		traceEnt = &g_entities[tr.entityNum];

		vec3_t center, mins, maxs, v;
		float reach = radius, dot, dist;
		gentity_t* entityList[MAX_GENTITIES];
		int e, numListedEntities, i;

		VectorCopy(self->currentOrigin, center);
		for (i = 0; i < 3; i++)
		{
			mins[i] = center[i] - reach;
			maxs[i] = center[i] + reach;
		}
		numListedEntities = gi.EntitiesInBox(mins, maxs, entityList, MAX_GENTITIES);

		for (e = 0; e < numListedEntities; e++)
		{
			vec3_t size;
			vec3_t ent_org;
			vec3_t dir;
			traceEnt = entityList[e];

			if (!traceEnt)
				continue;
			if (traceEnt == self)
				continue;
			if (!traceEnt->inuse)
				continue;
			if (!traceEnt->takedamage)
				continue;
			if (traceEnt->health <= 0) //no torturing corpses
				continue;
			// find the distance from the edge of the bounding box
			for (i = 0; i < 3; i++)
			{
				if (center[i] < traceEnt->absmin[i])
				{
					v[i] = traceEnt->absmin[i] - center[i];
				}
				else if (center[i] > traceEnt->absmax[i])
				{
					v[i] = center[i] - traceEnt->absmax[i];
				}
				else
				{
					v[i] = 0;
				}
			}

			VectorSubtract(traceEnt->absmax, traceEnt->absmin, size);
			VectorMA(traceEnt->absmin, 0.5, size, ent_org);

			//see if they're in front of me
			//must be within the forward cone
			VectorSubtract(ent_org, center, dir);
			VectorNormalize(dir);
			if ((dot = DotProduct(dir, forward)) < 0.5)
				continue;

			//must be close enough
			dist = VectorLength(v);
			if (dist >= reach)
			{
				continue;
			}

			//in PVS?
			if (!traceEnt->bmodel && !gi.inPVS(ent_org, self->client->renderInfo.handLPoint))
			{
				//must be in PVS
				continue;
			}

			//Now check and see if we can actually hit it
			gi.trace(&tr, self->client->renderInfo.handLPoint, vec3_origin, vec3_origin, ent_org, self->s.number,
				MASK_SHOT, G2_NOCOLLIDE, 0);
			if (tr.fraction < 1.0f && tr.entityNum != traceEnt->s.number)
			{
				//must have clear LOS
				continue;
			}

			// ok, we are within the radius, add us to the incoming list
			G_AddVoiceEvent(traceEnt, Q_irand(EV_PUSHED1, EV_PUSHED3), 2000);
			ForceStasisWide(self, traceEnt, dir, dist, dot, ent_org);
		}
	}
	else
	{
		vec3_t end;
		//Stasis the enemy!
		AngleVectors(self->client->ps.viewangles, forward, nullptr, nullptr);
		VectorNormalize(forward);
		VectorMA(self->client->renderInfo.eyePoint, radius, forward, end);

		if (self->enemy)
		{
			//I have an enemy
			if (self->client->ps.forcePowerLevel[FP_STASIS] > FORCE_LEVEL_1)
			{
				//arc
				if (DistanceSquared(self->enemy->currentOrigin, self->currentOrigin) < FORCE_STASIS_DIST_SQUARED_HIGH)
				{
					//close enough to grab
					float minDot = 0.5f;
					if (self->s.number < MAX_CLIENTS)
					{
						//player needs to be facing more directly
						minDot = 0.2f;
					}
					if (in_front(self->enemy->currentOrigin, self->client->renderInfo.eyePoint,
						self->client->ps.viewangles, minDot))
					{
						//need to be facing the enemy
						if (gi.inPVS(self->enemy->currentOrigin, self->client->renderInfo.eyePoint))
						{
							//must be in PVS
							gi.trace(&tr, self->client->renderInfo.eyePoint, vec3_origin, vec3_origin,
								self->enemy->currentOrigin, self->s.number, MASK_SHOT,
								static_cast<EG2_Collision>(0), 0);
							if (tr.fraction == 1.0f || tr.entityNum == self->enemy->s.number)
							{
								//must have clear LOS
								traceEnt = self->enemy;
							}
						}
					}
				}
			}
			else
			{
				//Stasis the enemy!
				gi.trace(&tr, self->client->renderInfo.eyePoint, vec3_origin, vec3_origin, end, self->s.number,
					MASK_OPAQUE | CONTENTS_BODY, static_cast<EG2_Collision>(0), 0);
				if (tr.entityNum == ENTITYNUM_NONE || tr.fraction == 1.0 || tr.allsolid || tr.startsolid)
				{
					return;
				}

				traceEnt = &g_entities[tr.entityNum];
			}
		}
		if (!traceEnt)
		{
			//okay, trace straight ahead and see what's there
			gi.trace(&tr, self->client->renderInfo.handLPoint, vec3_origin, vec3_origin, end, self->s.number, MASK_SHOT,
				static_cast<EG2_Collision>(0), 0);
			if (tr.entityNum >= ENTITYNUM_WORLD || tr.fraction == 1.0 || tr.allsolid || tr.startsolid)
			{
				if (g_stasistems->integer)
				{
					// One more try...try and physically check for entities in a box, similar to push
					vec3_t mins, maxs;

					for (int i = 0; i < 3; i++)
					{
						mins[i] = self->currentOrigin[i] - 512;
						maxs[i] = self->currentOrigin[i] + 512;
					}

					gentity_t* entlist[MAX_GENTITIES];
					int num_listed_entities = gi.EntitiesInBox(mins, maxs, entlist, MAX_GENTITIES);
					vec3_t vec2, vwangles, traceend;

					VectorCopy(self->currentAngles, vwangles);
					AngleVectors(vwangles, vec2, nullptr, nullptr);
					VectorMA(self->client->renderInfo.eyePoint, 512.0f, vec2, traceend);

					trace_t tr2;
					gi.trace(&tr2, self->client->renderInfo.eyePoint, vec3_origin, vec3_origin, traceend,
						self->s.number,
						MASK_OPAQUE | CONTENTS_SOLID | CONTENTS_BODY | CONTENTS_ITEM | CONTENTS_CORPSE | MASK_SHOT,
						static_cast<EG2_Collision>(0), 0);
					gentity_t* fwdEnt = &g_entities[tr2.entityNum];
					qboolean fwdEntIsCorrect = qfalse;
					for (int i = 0; i < num_listed_entities; i++)
					{
						gentity_t* targEnt = entlist[i];
						if (targEnt->s.eType == ET_ITEM || targEnt->s.eType == ET_MISSILE || targEnt->s.eType ==
							ET_GENERAL)
						{
							if (targEnt != fwdEnt)
							{
								continue;
							}
							fwdEntIsCorrect = qtrue;
						}
					}

					if (fwdEntIsCorrect)
					{
						tr.entityNum = fwdEnt->s.number;
					}
					else
					{
						return;
					}
				}
				else
				{
					return;
				}
			}
			traceEnt = &g_entities[tr.entityNum];
			G_AddVoiceEvent(traceEnt, Q_irand(EV_PUSHED1, EV_PUSHED3), 2000);
		}
	}

	if (traceEnt->NPC && traceEnt->NPC->scriptFlags & SCF_NO_FORCE)
	{
		return;
	}

	if (traceEnt->health > 0 &&
		traceEnt->s.weapon != WP_SABER && traceEnt->client && self->client->NPC_class != CLASS_REBORN)
	{
		//doesn't affect jedi .but affects everything else??
		int modPowerLevel = wp_absorb_conversion(traceEnt, traceEnt->client->ps.forcePowerLevel[FP_ABSORB], self,
			FP_STASIS, self->client->ps.forcePowerLevel[FP_STASIS],
			force_power_needed[FP_STASIS]);
		int actualPowerLevel;
		if (modPowerLevel == -1)
		{
			actualPowerLevel = self->client->ps.forcePowerLevel[FP_STASIS];
		}
		else
		{
			actualPowerLevel = modPowerLevel;
		}

		if (actualPowerLevel > 0)
		{
			if (traceEnt->client)
			{
				traceEnt->client->ps.stasisTime = level.time + stasisTime[actualPowerLevel]; //stuck for 5-10 seconds
				VectorClear(traceEnt->client->ps.velocity);
				player_Freeze(traceEnt);

				if (traceEnt->client->NPC_class == CLASS_BOBAFETT ||
					traceEnt->client->NPC_class == CLASS_MANDO)
				{
					// also disables npc jetpack
					Boba_FlyStop(traceEnt);
					if (traceEnt->client->jetPackOn)
					{
						//disable jetpack temporarily
						Jetpack_Off(traceEnt);
						traceEnt->client->jetPackToggleTime = level.time + Q_irand(3000, 10000);
					}
				}
			}

			if (gi.G2API_HaveWeGhoul2Models(traceEnt->ghoul2))
			{
				actual_time = cg.time ? cg.time : level.time;
				gi.G2API_GetBoneAnimIndex(&traceEnt->ghoul2[traceEnt->playerModel], traceEnt->rootBone,
					level.time, &currentFrame, &junk, &junk, &junk, &animSpeed, nullptr);

				gi.G2API_SetBoneAnimIndex(&traceEnt->ghoul2[traceEnt->playerModel], traceEnt->rootBone,
					currentFrame, currentFrame + 1,
					BONE_ANIM_OVERRIDE_FREEZE, animSpeed, level.time, currentFrame, 100);
				if (traceEnt->headModel > 0)
				{
					gi.G2API_SetBoneAnimIndex(&traceEnt->ghoul2[traceEnt->headModel], traceEnt->headRootBone,
						currentFrame, currentFrame + 1,
						BONE_ANIM_OVERRIDE_FREEZE, animSpeed, level.time, currentFrame, 100);
				}
			}
			if (d_slowmoaction->integer && (self->s.number < MAX_CLIENTS || G_ControlledByPlayer(self)))
			{
				G_StartStasisEffect(self);
			}
		}
	}
	else if (traceEnt->health > 0 && traceEnt->s.weapon == WP_SABER && traceEnt->client && traceEnt->client->ps.
		forcePower <= 75)
	{
		//affect jedi.
		int modPowerLevel = wp_absorb_conversion(traceEnt, traceEnt->client->ps.forcePowerLevel[FP_ABSORB], self,
			FP_STASIS, self->client->ps.forcePowerLevel[FP_STASIS],
			force_power_needed[FP_STASIS]);
		int actualPowerLevel;
		if (modPowerLevel == -1)
		{
			actualPowerLevel = self->client->ps.forcePowerLevel[FP_STASIS];
		}
		else
		{
			actualPowerLevel = modPowerLevel;
		}

		if (actualPowerLevel > 0)
		{
			if (traceEnt->client)
			{
				traceEnt->client->ps.stasisJediTime = level.time + stasisJediTime[actualPowerLevel];
				//stuck for 2-5 seconds
				VectorClear(traceEnt->client->ps.velocity);
				player_Freeze(traceEnt);

				if (traceEnt->client->NPC_class == CLASS_BOBAFETT ||
					traceEnt->client->NPC_class == CLASS_MANDO)
				{
					// also disables npc jetpack
					Boba_FlyStop(traceEnt);
					if (traceEnt->client->jetPackOn)
					{
						//disable jetpack temporarily
						Jetpack_Off(traceEnt);
						traceEnt->client->jetPackToggleTime = level.time + Q_irand(3000, 10000);
					}
				}
			}

			if (gi.G2API_HaveWeGhoul2Models(traceEnt->ghoul2))
			{
				actual_time = cg.time ? cg.time : level.time;
				gi.G2API_GetBoneAnimIndex(&traceEnt->ghoul2[traceEnt->playerModel], traceEnt->rootBone,
					level.time, &currentFrame, &junk, &junk, &junk, &animSpeed, nullptr);

				gi.G2API_SetBoneAnimIndex(&traceEnt->ghoul2[traceEnt->playerModel], traceEnt->rootBone,
					currentFrame, currentFrame + 1,
					BONE_ANIM_OVERRIDE_FREEZE, animSpeed, level.time, currentFrame, 100);
				if (traceEnt->headModel > 0)
				{
					gi.G2API_SetBoneAnimIndex(&traceEnt->ghoul2[traceEnt->headModel], traceEnt->headRootBone,
						currentFrame, currentFrame + 1,
						BONE_ANIM_OVERRIDE_FREEZE, animSpeed, level.time, currentFrame, 100);
				}
			}
			if (d_slowmoaction->integer && (self->s.number < MAX_CLIENTS || G_ControlledByPlayer(self)))
			{
				G_StartStasisEffect(self);
			}
		}
	}
	else
	{
		if (g_stasistems->integer && (traceEnt->s.eType == ET_ITEM || traceEnt->s.eType == ET_MISSILE || traceEnt->s.
			eType == ET_GENERAL))
		{
			/* WRONG! */
		}
		else
		{
			Player_CheckFreeze(traceEnt);
		}
	}

	if (self->s.weapon == WP_MELEE ||
		self->s.weapon == WP_NONE ||
		self->s.weapon == WP_SABER && !self->client->ps.SaberActive())
	{
		//2-handed PUSH
		if (self->client->ps.groundEntityNum == ENTITYNUM_NONE)
		{
			anim = BOTH_SUPERPUSH;

			if (self->handLBolt != -1)
			{
				G_PlayEffect(G_EffectIndex("force/pushblur"), self->playerModel, self->handLBolt, self->s.number,
					self->currentOrigin, 200, qtrue);
			}

			if (self->handRBolt != -1)
			{
				G_PlayEffect(G_EffectIndex("force/pushblur"), self->playerModel, self->handRBolt, self->s.number,
					self->currentOrigin, 200, qtrue);
			}
		}
		else
		{
			if (self->s.eFlags & EF_FORCE_DRAINED || self->s.eFlags & EF_FORCE_GRIPPED || self->s.eFlags &
				EF_FORCE_GRABBED)
			{
				anim = BOTH_FORCEPUSH;

				if (self->handLBolt != -1)
				{
					G_PlayEffect(G_EffectIndex("force/pushblur"), self->playerModel, self->handLBolt, self->s.number,
						self->currentOrigin, 200, qtrue);
				}
			}
			else
			{
				anim = BOTH_2HANDPUSH;

				if (self->handLBolt != -1)
				{
					G_PlayEffect(G_EffectIndex("force/pushblur"), self->playerModel, self->handLBolt, self->s.number,
						self->currentOrigin, 200, qtrue);
				}

				if (self->handRBolt != -1)
				{
					G_PlayEffect(G_EffectIndex("force/pushblur"), self->playerModel, self->handRBolt, self->s.number,
						self->currentOrigin, 200, qtrue);
				}
			}
		}
	}
	else
	{
		anim = BOTH_FORCEPUSH;

		if (self->handLBolt != -1)
		{
			G_PlayEffect(G_EffectIndex("force/pushblur"), self->playerModel, self->handLBolt, self->s.number,
				self->currentOrigin, 200, qtrue);
		}
	}
	soundIndex = G_SoundIndex("sound/weapons/force/force_stasis.mp3");

	int parts = SETANIM_TORSO;
	if (!PM_InKnockDown(&self->client->ps))
	{
		if (!VectorLengthSquared(self->client->ps.velocity) && !(self->client->ps.pm_flags & PMF_DUCKED))
		{
			parts = SETANIM_BOTH;
		}
	}
	NPC_SetAnim(self, parts, anim, SETANIM_FLAG_OVERRIDE | SETANIM_FLAG_HOLD | SETANIM_FLAG_RESTART);
	self->client->ps.saberMove = self->client->ps.saberBounceMove = LS_READY;
	//don't finish whatever saber anim you may have been in
	self->client->ps.saberBlocked = BLOCKED_NONE;

	G_Sound(self, soundIndex);

	WP_ForcePowerStart(self, FP_STASIS, 0);

	self->client->ps.weaponTime = 1000;
	if (self->client->ps.forcePowersActive & 1 << FP_SPEED)
	{
		self->client->ps.weaponTime = floor(self->client->ps.weaponTime * g_timescale->value);
	}
	self->client->ps.forcePowerDebounce[FP_STASIS] = level.time + self->client->ps.torsoAnimTimer + 500;
}

void ForceGrasp(gentity_t* self)
{
	//FIXME: make enemy Jedi able to use this
	trace_t tr;
	vec3_t end, forward;
	gentity_t* traceEnt = nullptr;

	if (self->health <= 0)
	{
		return;
	}
	if (!self->s.number && (cg.zoomMode || in_camera))
	{
		//can't force grip when zoomed in or in cinematic
		return;
	}
	if (self->client->ps.leanofs)
	{
		//can't force-grip while leaning
		return;
	}
	if (self->client->ps.stasisTime > level.time)
	{
		return;
	}

	if (PM_InLedgeMove(self->client->ps.legsAnim))
	{
		return;
	}

	if (self->client->ps.forceGripEntityNum <= ENTITYNUM_WORLD)
	{
		//already gripping
		if (self->client->ps.forcePowerLevel[FP_GRASP] > FORCE_LEVEL_1)
		{
			self->client->ps.forcePowerDuration[FP_GRASP] = level.time + 100;
			self->client->ps.weaponTime = 1000;
			if (self->client->ps.forcePowersActive & 1 << FP_SPEED)
			{
				self->client->ps.weaponTime = floor(self->client->ps.weaponTime * g_timescale->value);
			}
		}
		return;
	}

	if (!WP_ForcePowerUsable(self, FP_GRASP, 0))
	{
		//can't use it right now
		return;
	}

	if (self->client->ps.forcePower < 1)
	{
		//need 20 to start, 6 to hold it for any decent amount of time...
		return;
	}

	if (self->client->ps.weaponTime)
	{
		//busy
		return;
	}

	if (self->client->ps.saberLockTime > level.time)
	{
		//FIXME: can this be a way to break out?
		return;
	}
	//Cause choking anim + health drain in ent in front of me

	if (self->s.weapon == WP_NONE
		|| self->s.weapon == WP_MELEE
		|| self->s.weapon == WP_SABER && !self->client->ps.SaberActive())
	{
		NPC_SetAnim(self, SETANIM_TORSO, BOTH_FORCEGRIP_HOLD, SETANIM_FLAG_OVERRIDE | SETANIM_FLAG_HOLD);
	}
	else
	{
		NPC_SetAnim(self, SETANIM_TORSO, BOTH_FORCEGRIP_OLD, SETANIM_FLAG_OVERRIDE | SETANIM_FLAG_HOLD);
	}

	self->client->ps.saberMove = self->client->ps.saberBounceMove = LS_READY;
	//don't finish whatever saber anim you may have been in
	self->client->ps.saberBlocked = BLOCKED_NONE;

	self->client->ps.weaponTime = 1000;
	if (self->client->ps.forcePowersActive & 1 << FP_SPEED)
	{
		self->client->ps.weaponTime = floor(self->client->ps.weaponTime * g_timescale->value);
	}

	AngleVectors(self->client->ps.viewangles, forward, nullptr, nullptr);
	VectorNormalize(forward);
	VectorMA(self->client->renderInfo.handLPoint, FORCE_GRIP_DIST, forward, end);

	if (self->enemy)
	{
		//I have an enemy
		if (!self->enemy->message
			&& !(self->flags & FL_NO_KNOCKBACK))
		{
			//don't auto-pickup guys with keys
			if (DistanceSquared(self->enemy->currentOrigin, self->currentOrigin) < FORCE_GRIP_DIST_SQUARED)
			{
				//close enough to grab
				float minDot = 0.5f;
				if (self->s.number < MAX_CLIENTS)
				{
					//player needs to be facing more directly
					minDot = 0.2f;
				}
				if (in_front(self->enemy->currentOrigin, self->client->renderInfo.eyePoint, self->client->ps.viewangles,
					minDot))
					//self->s.number || //NPCs can always lift enemy since we assume they're looking at them...?
				{
					//need to be facing the enemy
					if (gi.inPVS(self->enemy->currentOrigin, self->client->renderInfo.eyePoint))
					{
						//must be in PVS
						gi.trace(&tr, self->client->renderInfo.eyePoint, vec3_origin, vec3_origin,
							self->enemy->currentOrigin, self->s.number, MASK_SHOT, static_cast<EG2_Collision>(0),
							0);
						if (tr.fraction == 1.0f || tr.entityNum == self->enemy->s.number)
						{
							//must have clear LOS
							traceEnt = self->enemy;
						}
					}
				}
			}
		}
	}
	if (!traceEnt)
	{
		//okay, trace straight ahead and see what's there
		gi.trace(&tr, self->client->renderInfo.handLPoint, vec3_origin, vec3_origin, end, self->s.number, MASK_SHOT,
			static_cast<EG2_Collision>(0), 0);
		if (tr.entityNum >= ENTITYNUM_WORLD || tr.fraction == 1.0 || tr.allsolid || tr.startsolid)
		{
			return;
		}

		traceEnt = &g_entities[tr.entityNum];
	}
	//rww - RAGDOLL_BEGIN
#ifdef JK2_RAGDOLL_GRIPNOHEALTH
	if (!traceEnt || traceEnt == self/*???*/ || traceEnt->bmodel || traceEnt->NPC && traceEnt->NPC->scriptFlags &
		SCF_NO_FORCE)
	{
		return;
	}
	/*if (traceEnt == player && player->flags & FL_NOFORCE)
		return;*/
#else
//rww - RAGDOLL_END
	if (!traceEnt || traceEnt == self/*???*/ || traceEnt->bmodel || (traceEnt->health <= 0 && traceEnt->takedamage) || (traceEnt->NPC && traceEnt->NPC->scriptFlags & SCF_NO_FORCE))
	{
		return;
	}
	//rww - RAGDOLL_BEGIN
#endif
	//rww - RAGDOLL_END

	if (traceEnt->m_pVehicle != nullptr)
	{
		//is it a vehicle
		//grab pilot if there is one
		if (traceEnt->m_pVehicle->m_pPilot != nullptr
			&& traceEnt->m_pVehicle->m_pPilot->client != nullptr)
		{
			//grip the pilot
			traceEnt = traceEnt->m_pVehicle->m_pPilot;
		}
		else
		{
			//can't grip a vehicle
			return;
		}
	}
	if (traceEnt->client)
	{
		if (traceEnt->client->ps.forceJumpZStart)
		{
			//can't catch them in mid force jump - FIXME: maybe base it on velocity?
			return;
		}
		if (traceEnt->client->ps.pullAttackTime > level.time)
		{
			//can't grip someone who is being pull-attacked or is pull-attacking
			return;
		}
		if (!Q_stricmp("Yoda", traceEnt->NPC_type))
		{
			Jedi_PlayDeflectSound(traceEnt);
			ForceThrow(traceEnt, qfalse);
			return;
		}

		if (G_IsRidingVehicle(traceEnt)
			&& traceEnt->s.eFlags & EF_NODRAW)
		{
			//riding *inside* vehicle
			return;
		}

		switch (traceEnt->client->NPC_class)
		{
		case CLASS_GALAKMECH: //cant grip him, he's in armor
			G_AddVoiceEvent(traceEnt, Q_irand(EV_PUSHED1, EV_PUSHED3), Q_irand(3000, 5000));
			return;
		case CLASS_HAZARD_TROOPER: //cant grip him, he's in armor
			return;
		case CLASS_ATST: //much too big to grip!
		case CLASS_RANCOR: //much too big to grip!
		case CLASS_WAMPA: //much too big to grip!
		case CLASS_SAND_CREATURE: //much too big to grip!
			return;
			//no droids either...?
		case CLASS_GONK:
		case CLASS_R2D2:
		case CLASS_R5D2:
		case CLASS_MARK1:
		case CLASS_MARK2:
		case CLASS_MOUSE: //?
		case CLASS_PROTOCOL:
			//*sigh*... in JK3, you'll be able to grab and move *anything*...
			return;
			//not even combat droids?  (No animation for being gripped...)
		case CLASS_SABER_DROID:
		case CLASS_ASSASSIN_DROID:
			//*sigh*... in JK3, you'll be able to grab and move *anything*...
			return;
		case CLASS_PROBE:
		case CLASS_SEEKER:
		case CLASS_REMOTE:
		case CLASS_SENTRY:
		case CLASS_INTERROGATOR:
			//*sigh*... in JK3, you'll be able to grab and move *anything*...
			return;
		case CLASS_SITHLORD:
		case CLASS_DESANN: //Desann cannot be gripped, he just pushes you back instantly
		case CLASS_VADER:
		case CLASS_KYLE:
		case CLASS_TAVION:
		case CLASS_LUKE:
		case CLASS_YODA:
			Jedi_PlayDeflectSound(traceEnt);
			ForceThrow(traceEnt, qfalse);
			return;
		case CLASS_REBORN:
		case CLASS_SHADOWTROOPER:
		case CLASS_ALORA:
		case CLASS_JEDI:
			if (traceEnt->NPC && traceEnt->NPC->rank > RANK_CIVILIAN && self->client->ps.forcePowerLevel[FP_GRASP] <
				FORCE_LEVEL_2)
			{
				Jedi_PlayDeflectSound(traceEnt);
				ForceThrow(traceEnt, qfalse);
				return;
			}
			break;
		default:
			break;
		}
		if (traceEnt->s.weapon == WP_EMPLACED_GUN)
		{
			//FIXME: maybe can pull them out?
			return;
		}
		if (self->enemy && traceEnt != self->enemy && traceEnt->client->playerTeam == self->client->playerTeam)
		{
			//can't accidently grip your teammate in combat
			return;
		}
		//=CHECKABSORB===
		if (-1 != wp_absorb_conversion(traceEnt, traceEnt->client->ps.forcePowerLevel[FP_ABSORB], self, FP_GRASP,
			self->client->ps.forcePowerLevel[FP_GRASP],
			force_power_needed[self->client->ps.forcePowerLevel[FP_GRASP]]))
		{
			//WP_ForcePowerStop( self, FP_GRIP );
			return;
		}
		//===============
	}
	else
	{
		//can't grip non-clients... right?
		//FIXME: Make it so objects flagged as "grabbable" are let through
		//if ( Q_stricmp( "misc_model_breakable", traceEnt->classname ) || !(traceEnt->s.eFlags&EF_BOUNCE_HALF) || !traceEnt->physicsBounce )
		{
			return;
		}
	}

	WP_ForcePowerStart(self, FP_GRASP, 1);
	//FIXME: rule out other things?
	//FIXME: Jedi resist, like the push and pull?
	self->client->ps.forceGripEntityNum = traceEnt->s.number;
	if (traceEnt->client)
	{
		Vehicle_t* pVeh;
		if ((pVeh = G_IsRidingVehicle(traceEnt)) != nullptr)
		{
			//riding vehicle? pull him off!
			//FIXME: if in an AT-ST or X-Wing, shouldn't do this... :)
			//pull him off of it
			//((CVehicleNPC *)traceEnt->NPC)->Eject( traceEnt );
			pVeh->m_pVehicleInfo->Eject(pVeh, traceEnt, qtrue);
			//G_DriveVehicle( traceEnt, NULL, NULL );
		}
		G_AddVoiceEvent(traceEnt, Q_irand(EV_PUSHED1, EV_PUSHED3), 2000);
		if (self->client->ps.forcePowerLevel[FP_GRASP] > FORCE_LEVEL_2 || traceEnt->s.weapon == WP_SABER)
		{
			//if we pick up & carry, drop their weap
			if (traceEnt->s.weapon
				&& traceEnt->client->NPC_class != CLASS_ROCKETTROOPER
				&& traceEnt->client->NPC_class != CLASS_VEHICLE
				&& traceEnt->client->NPC_class != CLASS_HAZARD_TROOPER
				&& traceEnt->client->NPC_class != CLASS_TUSKEN
				&& traceEnt->client->NPC_class != CLASS_BOBAFETT
				&& traceEnt->client->NPC_class != CLASS_MANDO
				&& traceEnt->client->NPC_class != CLASS_ASSASSIN_DROID
				&& traceEnt->client->NPC_class != CLASS_DROIDEKA
				&& traceEnt->client->NPC_class != CLASS_SBD
				&& traceEnt->s.weapon != WP_CONCUSSION) // so rax can't drop his
			{
				if (traceEnt->client->NPC_class == CLASS_BOBAFETT || traceEnt->client->NPC_class == CLASS_MANDO)
				{
					//he doesn't drop them, just puts it away
					ChangeWeapon(traceEnt, WP_MELEE);
				}
				else if (traceEnt->s.weapon == WP_MELEE)
				{
					//they can't take that away from me, oh no...
				}
				else if (traceEnt->NPC
					&& traceEnt->NPC->scriptFlags & SCF_DONT_FLEE)
				{
					//*SIGH*... if an NPC can't flee, they can't run after and pick up their weapon, do don't drop it
				}
				else if (traceEnt->s.weapon != WP_SABER)
				{
					WP_DropWeapon(traceEnt, nullptr);
				}
				else
				{
					if (traceEnt->client->ps.SaberActive())
					{
						traceEnt->client->ps.SaberDeactivate();
						G_SoundOnEnt(traceEnt, CHAN_WEAPON, "sound/weapons/saber/saberoffquick.mp3");
					}
				}
			}
		}
		//else FIXME: need a one-armed choke if we're not on a high enough level to make them drop their gun
		VectorCopy(traceEnt->client->renderInfo.headPoint, self->client->ps.forceGripOrg);
	}
	else
	{
		VectorCopy(traceEnt->currentOrigin, self->client->ps.forceGripOrg);
	}
	self->client->ps.forceGripOrg[2] += 48; //FIXME: define?

	if (self->client->ps.forcePowerLevel[FP_GRASP] < FORCE_LEVEL_2)
	{
		//just a duration
		self->client->ps.forcePowerDebounce[FP_GRASP] = level.time + 250;
		self->client->ps.forcePowerDuration[FP_GRASP] = level.time + 5000;

		if (self->m_pVehicle && self->m_pVehicle->m_pVehicleInfo->Inhabited(self->m_pVehicle))
		{
			//empty vehicles don't make gripped noise
			if (traceEnt
				&& traceEnt->client
				&& traceEnt->client->NPC_class == CLASS_OBJECT)
			{
				//
			}
			else
			{
				G_SoundOnEnt(self, CHAN_BODY, "sound/weapons/force/grab.mp3");
			}
		}
	}
	else
	{
		if (self->client->ps.forcePowerLevel[FP_GRASP] == FORCE_LEVEL_2)
		{
			//lifting sound?  or always?
		}
		//if ( traceEnt->s.number )
		{
			//picks them up for a second first
			self->client->ps.forcePowerDebounce[FP_GRASP] = level.time + 1000;
		}

		if (traceEnt
			&& traceEnt->client
			&& traceEnt->client->NPC_class == CLASS_OBJECT)
		{
			//
		}
		else
		{
			G_SoundOnEnt(self, CHAN_BODY, "sound/weapons/force/grab.mp3");
		}
	}
}

extern void WP_FireBlast(gentity_t* ent, int forceLevel);

void ForceBlast(gentity_t* self)
{
	int anim;
	if (self->health <= 0)
	{
		return;
	}
	if (!WP_ForcePowerUsable(self, FP_BLAST, 0))
	{
		return;
	}
	if (self->client->ps.forcePowerDebounce[FP_BLAST] > level.time)
	{
		//already using destruction
		return;
	}
	if (!self->s.number && (cg.zoomMode || in_camera))
	{
		//can't destruction when zoomed in or in cinematic
		return;
	}
	if (self->client->ps.saberLockTime > level.time)
	{
		//FIXME: can this be a way to break out?
		return;
	}

	if (PM_InLedgeMove(self->client->ps.legsAnim))
	{
		return;
	}

	if (self->s.number >= MAX_CLIENTS && !G_ControlledByPlayer(self) && self->client->ps.weapon == WP_SABER)
		//npc force use limit
	{
		if (self->client->ps.blockPoints < 75 || self->client->ps.forcePower < 75)
		{
			return;
		}
	}

	if (self->s.weapon == WP_MELEE ||
		self->s.weapon == WP_NONE ||
		self->s.weapon == WP_SABER && !self->client->ps.SaberActive())
	{
		//2-handed PUSH
		if (self->client->ps.groundEntityNum == ENTITYNUM_NONE)
		{
			anim = BOTH_2HANDPUSH;

			if (self->handLBolt != -1)
			{
				G_PlayEffect(G_EffectIndex("force/invin"), self->playerModel, self->handLBolt, self->s.number,
					self->currentOrigin, 200, qtrue);
			}

			if (self->handRBolt != -1)
			{
				G_PlayEffect(G_EffectIndex("force/invin"), self->playerModel, self->handRBolt, self->s.number,
					self->currentOrigin, 200, qtrue);
			}
		}
		else
		{
			if (self->s.eFlags & EF_FORCE_DRAINED || self->s.eFlags & EF_FORCE_GRIPPED || self->s.eFlags &
				EF_FORCE_GRABBED)
			{
				anim = BOTH_FORCEPUSH;

				if (self->handLBolt != -1)
				{
					G_PlayEffect(G_EffectIndex("force/invin"), self->playerModel, self->handLBolt, self->s.number,
						self->currentOrigin, 200, qtrue);
				}
			}
			else
			{
				anim = BOTH_2HANDPUSH;

				if (self->handLBolt != -1)
				{
					G_PlayEffect(G_EffectIndex("force/invin"), self->playerModel, self->handLBolt, self->s.number,
						self->currentOrigin, 200, qtrue);
				}

				if (self->handRBolt != -1)
				{
					G_PlayEffect(G_EffectIndex("force/invin"), self->playerModel, self->handRBolt, self->s.number,
						self->currentOrigin, 200, qtrue);
				}
			}
		}
	}
	else
	{
		anim = BOTH_FORCEPUSH;

		if (self->handLBolt != -1)
		{
			G_PlayEffect(G_EffectIndex("force/invin"), self->playerModel, self->handLBolt, self->s.number,
				self->currentOrigin, 200, qtrue);
		}
	}
	const int soundIndex = G_SoundIndex("sound/weapons/force/blast.wav");

	int parts = SETANIM_TORSO;
	if (!PM_InKnockDown(&self->client->ps))
	{
		if (!VectorLengthSquared(self->client->ps.velocity) && !(self->client->ps.pm_flags & PMF_DUCKED))
		{
			parts = SETANIM_BOTH;
		}
	}
	NPC_SetAnim(self, parts, anim, SETANIM_FLAG_OVERRIDE | SETANIM_FLAG_HOLD | SETANIM_FLAG_RESTART);
	self->client->ps.saberMove = self->client->ps.saberBounceMove = LS_READY;
	//don't finish whatever saber anim you may have been in
	self->client->ps.saberBlocked = BLOCKED_NONE;

	G_Sound(self, soundIndex);

	WP_FireBlast(self, self->client->ps.forcePowerLevel[FP_BLAST]);

	WP_ForcePowerStart(self, FP_BLAST, 0);

	self->client->ps.weaponTime = 1000;
	if (self->client->ps.forcePowersActive & 1 << FP_SPEED)
	{
		self->client->ps.weaponTime = floor(self->client->ps.weaponTime * g_timescale->value);
	}
	self->client->ps.forcePowerDebounce[FP_BLAST] = level.time + self->client->ps.torsoAnimTimer + 1500;
}

constexpr auto BLAST_VELOCITY = 1200;
constexpr auto BLAST_DAMAGE = 20;
constexpr auto BLAST_SPLASH_DAMAGE = 35;
constexpr auto BLAST_SPLASH_RADIUS = 80;
constexpr auto BLAST_NPC_DAMAGE_EASY = 10;
constexpr auto BLAST_NPC_DAMAGE_NORMAL = 20;
constexpr auto BLAST_NPC_DAMAGE_HARD = 30;
constexpr auto BLAST_SIZE = 1;

//---------------------------------------------------------
void WP_FireBlast(gentity_t* ent, int forceLevel)
//---------------------------------------------------------
{
	vec3_t start, forward;
	int damage = BLAST_DAMAGE;
	float vel = BLAST_VELOCITY;

	if (ent == player)
	{
		damage *= forceLevel;
	}

	vel *= forceLevel;

	AngleVectors(ent->client->ps.viewangles, forward, nullptr, nullptr);
	VectorNormalize(forward);

	VectorCopy(ent->client->renderInfo.eyePoint, start);

	gentity_t* missile = create_missile(start, forward, vel, 10000, ent, qfalse);

	missile->classname = "rocket_proj";
	missile->s.weapon = WP_ROCKET_LAUNCHER;
	missile->s.powerups |= 1 << PW_FORCE_PROJECTILE;
	missile->mass = 20;

	// Do the damages
	if (ent->s.number != 0)
	{
		if (g_spskill->integer == 0)
		{
			damage = BLAST_NPC_DAMAGE_EASY;
		}
		else if (g_spskill->integer == 1)
		{
			damage = BLAST_NPC_DAMAGE_NORMAL;
		}
		else
		{
			damage = BLAST_NPC_DAMAGE_HARD;
		}
	}

	// Make it easier to hit things
	VectorSet(missile->maxs, BLAST_SIZE, BLAST_SIZE, BLAST_SIZE);
	VectorScale(missile->maxs, -1, missile->mins);

	missile->damage = damage;

	missile->dflags = DAMAGE_EXTRA_KNOCKBACK;

	missile->methodOfDeath = MOD_DESTRUCTION;
	missile->splashMethodOfDeath = MOD_DESTRUCTION; // ?SPLASH;

	missile->clipmask = MASK_SHOT | CONTENTS_LIGHTSABER;
	missile->splashDamage = damage;
	missile->splashRadius = BLAST_SPLASH_RADIUS * (1.0f + forceLevel) / 2.0f;

	// we don't want it to ever bounce
	missile->bounceCount = 0;
}

void ForceRepulse(gentity_t* self)
{
	if (self->health <= 0)
	{
		return;
	}
	if (!self->s.number && (cg.zoomMode || in_camera))
	{
		//can't repulse when zoomed in or in cinematic
		return;
	}
	if (self->client->ps.leanofs)
	{
		//can't repulse while leaning
		return;
	}
	if (!WP_ForcePowerUsable(self, FP_REPULSE, 40))
	{
		return;
	}
	if (self->client->ps.repulseChargeStart)
	{
		return;
	}
	if (self->client->ps.saberLockTime > level.time)
	{
		//FIXME: can this be a way to break out?
		return;
	}
	// Make sure to turn off Force Protection and Force Absorb.
	if (self->client->ps.forcePowersActive & 1 << FP_PROTECT)
	{
		WP_ForcePowerStop(self, FP_PROTECT);
	}
	if (self->client->ps.forcePowersActive & 1 << FP_ABSORB)
	{
		WP_ForcePowerStop(self, FP_ABSORB);
	}

	self->client->ps.repulseChargeStart = level.time;

	NPC_SetAnim(self, SETANIM_TORSO, BOTH_FORCE_PROTECT_FAST, SETANIM_FLAG_OVERRIDE | SETANIM_FLAG_HOLD);

	self->client->ps.saberMove = self->client->ps.saberBounceMove = LS_READY;
	//don't finish whatever saber anim you may have been in
	self->client->ps.saberBlocked = BLOCKED_NONE;

	G_SoundOnEnt(self, CHAN_BODY, "sound/weapons/force/repulsecharge.mp3");

	if (self->s.number < MAX_CLIENTS || G_ControlledByPlayer(self))
	{
		CGCam_BlockShakeSP(0.50f, 1500);
	}

	self->client->ps.weaponTime = self->client->ps.torsoAnimTimer;
	WP_ForcePowerStart(self, FP_REPULSE, 1);
}

int wp_absorb_conversion(const gentity_t* attacked, const int atd_abs_level, gentity_t* attacker, const int at_power,
	const int at_power_level, const
	int at_force_spent)
{
	if (at_power != FP_DRAIN &&
		at_power != FP_GRIP &&
		at_power != FP_PUSH &&
		at_power != FP_PULL &&
		at_power != FP_REPULSE &&
		at_power != FP_GRASP &&
		at_power != FP_STASIS)
	{
		//Only these powers can be absorbed
		return -1;
	}

	if (!atd_abs_level)
	{
		//looks like attacker doesn't have any absorb power
		return -1;
	}

	if (!(attacked->client->ps.forcePowersActive & 1 << FP_ABSORB))
	{
		//absorb is not active
		return -1;
	}

	//Subtract absorb power level from the offensive force power
	int get_level = at_power_level;
	get_level -= atd_abs_level;

	if (get_level < 0)
	{
		get_level = 0;
	}

	//let the attacker absorb an amount of force used in this attack based on his level of absorb
	int add_tot = at_force_spent / 3 * attacked->client->ps.forcePowerLevel[FP_ABSORB];

	if (add_tot < 1 && at_force_spent >= 1)
	{
		add_tot = 1;
	}
	attacked->client->ps.forcePower += add_tot;
	if (attacked->client->ps.forcePower > attacked->client->ps.forcePowerMax)
	{
		attacked->client->ps.forcePower = attacked->client->ps.forcePowerMax;
	}

	G_SoundOnEnt(attacked, CHAN_ITEM, "sound/weapons/force/absorbhit.wav");

	return get_level;
}

void wp_force_power_regenerate(const gentity_t* self, const int override_amt)
{
	if (self->client->ps.forcePower < self->client->ps.forcePowerMax)
	{
		if (override_amt)
		{
			self->client->ps.forcePower += override_amt;
		}
		else
		{
			self->client->ps.forcePower++;
		}
		if (self->client->ps.forcePower > self->client->ps.forcePowerMax)
		{
			self->client->ps.forcePower = self->client->ps.forcePowerMax;
		}
	}
}

void wp_block_points_regenerate(const gentity_t* self, const int override_amt)
{
	const qboolean holding_block = self->client->ps.ManualBlockingFlags & 1 << HOLDINGBLOCK ? qtrue : qfalse;
	//Normal Blocking

	if (!holding_block)
	{
		if (self->client->ps.blockPoints < BLOCK_POINTS_MAX)
		{
			if (override_amt)
			{
				self->client->ps.blockPoints += override_amt;
			}
			else
			{
				self->client->ps.blockPoints++;
			}
			if (self->client->ps.blockPoints > BLOCK_POINTS_MAX)
			{
				self->client->ps.blockPoints = BLOCK_POINTS_MAX;
			}
		}
	}
}

void wp_block_points_regenerate_over_ride(const gentity_t* self, const int override_amt)
{
	if (self->client->ps.blockPoints < BLOCK_POINTS_MAX)
	{
		if (override_amt)
		{
			self->client->ps.blockPoints += override_amt;
		}
		else
		{
			self->client->ps.blockPoints++;
		}
		if (self->client->ps.blockPoints > BLOCK_POINTS_MAX)
		{
			self->client->ps.blockPoints = BLOCK_POINTS_MAX;
		}
	}
}

void WP_ForcePowerDrain(const gentity_t* self, const forcePowers_t forcePower, const int overrideAmt)
{
	//take away the power
	int drain = overrideAmt;

	if (!drain)
	{
		if (self->client->ps.forcePowerLevel[FP_SPEED] == FORCE_LEVEL_1 &&
			self->client->ps.forcePowersActive & 1 << FP_SPEED)
		{
			drain = force_power_neededlevel1[forcePower];
		}
		else if (self->client->ps.forcePowerLevel[FP_SPEED] == FORCE_LEVEL_2 &&
			self->client->ps.forcePowersActive & 1 << FP_SPEED)
		{
			drain = force_power_neededlevel2[forcePower];
		}
		else if (self->client->ps.forcePowerLevel[FP_SPEED] == FORCE_LEVEL_3 &&
			self->client->ps.forcePowersActive & 1 << FP_SPEED)
		{
			drain = force_power_needed[forcePower];
		}
		else
		{
			drain = force_power_needed[forcePower];
		}
	}

	if (!drain)
	{
		return;
	}

	self->client->ps.forcePower -= drain;

	if (self->client->ps.forcePower < 0)
	{
		self->client->ps.forcePower = 0;
	}
	//check for fatigued state.
	if (self->client->ps.forcePower <= self->client->ps.forcePowerMax * FATIGUEDTHRESHHOLD)
	{
		//Pop the Fatigued flag
		self->client->ps.userInt3 |= 1 << FLAG_FATIGUED;
	}
}

void WP_BlockPointsDrain(const gentity_t* self, int Fatigue)
{
	if (self->client->ps.blockPoints > Fatigue)
	{
		self->client->ps.blockPoints -= Fatigue;
	}
	else
	{
		self->client->ps.blockPoints = 0;
	}

	if (self->client->ps.blockPoints < 0)
	{
		self->client->ps.blockPoints = 0;
	}

	if (self->client->ps.blockPoints <= BLOCKPOINTS_HALF)
	{
		//Pop the Fatigued flag
		self->client->ps.userInt3 |= 1 << FLAG_BLOCKDRAINED;
	}
}

void WP_ForcePowerStart(gentity_t* self, forcePowers_t forcePower, int overrideAmt)
{
	int duration = 0;

	self->client->ps.forcePowerDebounce[forcePower] = 0;

	//and it in
	//set up duration time
	switch (static_cast<int>(forcePower))
	{
	case FP_HEAL:
		self->client->ps.forcePowersActive |= 1 << forcePower;
		self->client->ps.forceHealCount = 0;
		WP_StartForceHealEffects(self);
		break;
	case FP_LEVITATION:
		self->client->ps.forcePowersActive |= 1 << forcePower;
		break;
	case FP_SPEED:
		//duration is always 5 seconds, player time
		if (self->client->ps.forcePowerLevel[forcePower] == FORCE_LEVEL_1)
		{
			duration = ceil(
				FORCE_SPEED_DURATION_FORCE_LEVEL_1 * forceSpeedValue[self->client->ps.forcePowerLevel[forcePower]]);
		}
		else if (self->client->ps.forcePowerLevel[forcePower] == FORCE_LEVEL_2)
		{
			duration = ceil(
				FORCE_SPEED_DURATION_FORCE_LEVEL_2 * forceSpeedValue[self->client->ps.forcePowerLevel[forcePower]]);
		}
		else if (self->client->ps.forcePowerLevel[forcePower] == FORCE_LEVEL_3)
		{
			duration = ceil(
				FORCE_SPEED_DURATION_FORCE_LEVEL_3 * forceSpeedValue[self->client->ps.forcePowerLevel[forcePower]]);
		}
		self->client->ps.forcePowersActive |= 1 << forcePower;

		if (IsPressingDashButton(self))
		{
			//
		}
		else
		{
			self->s.loopSound = G_SoundIndex("sound/weapons/force/speedloop.wav");
		}

		if (self->client->ps.forcePowerLevel[forcePower] > FORCE_LEVEL_2)
		{
			if (IsPressingDashButton(self))
			{
				//
			}
			else
			{
				self->client->ps.forcePowerDebounce[forcePower] = level.time;
			}
		}
		break;
	case FP_PUSH:
		break;
	case FP_PULL:
		self->client->ps.forcePowersActive |= 1 << forcePower;
		break;
	case FP_TELEPATHY:
		break;
	case FP_GRIP:
		duration = 1000;
		self->client->ps.forcePowersActive |= 1 << forcePower;
		break;
	case FP_LIGHTNING:
		duration = overrideAmt;
		overrideAmt = 0;
		self->client->ps.forcePowersActive |= 1 << forcePower;
		break;
	case FP_RAGE:
		if (self->client->ps.forcePowerLevel[forcePower] == FORCE_LEVEL_1)
		{
			duration = 8000;
		}
		else if (self->client->ps.forcePowerLevel[forcePower] == FORCE_LEVEL_2)
		{
			duration = 14000;
		}
		else if (self->client->ps.forcePowerLevel[forcePower] == FORCE_LEVEL_3)
		{
			duration = 20000;
		}
		else //shouldn't get here
		{
			break;
		}
		self->client->ps.forcePowersActive |= 1 << forcePower;
		G_SoundOnEnt(self, CHAN_ITEM, "sound/weapons/force/rage.mp3");
		self->s.loopSound = G_SoundIndex("sound/weapons/force/rageloop.wav");
		if (self->chestBolt != -1)
		{
			G_PlayEffect(G_EffectIndex("force/rage2"), self->playerModel, self->chestBolt, self->s.number,
				self->currentOrigin, duration, qtrue);
		}
		break;
	case FP_DRAIN:
		if (self->client->ps.forcePowerLevel[forcePower] > FORCE_LEVEL_1
			&& self->client->ps.forceDrainEntityNum >= ENTITYNUM_WORLD)
		{
			duration = overrideAmt;
			overrideAmt = 0;
			self->client->ps.forcePowerDebounce[forcePower] = level.time;
		}
		else
		{
			duration = 1000;
		}
		self->client->ps.forcePowersActive |= 1 << forcePower;
		break;
	case FP_PROTECT:
		switch (self->client->ps.forcePowerLevel[forcePower])
		{
		case FORCE_LEVEL_5:
		case FORCE_LEVEL_4:
		case FORCE_LEVEL_3:
			duration = 20000;
			break;
		case FORCE_LEVEL_2:
			duration = 15000;
			break;
		case FORCE_LEVEL_1:
		default:
			duration = 10000;
			break;
		}
		self->client->ps.forcePowersActive |= 1 << forcePower;
		G_SoundOnEnt(self, CHAN_ITEM, "sound/weapons/force/protect.mp3");
		self->s.loopSound = G_SoundIndex("sound/weapons/force/protectloop.wav");
		break;
	case FP_ABSORB:
		duration = 20000;
		self->client->ps.forcePowersActive |= 1 << forcePower;
		G_SoundOnEnt(self, CHAN_ITEM, "sound/weapons/force/absorb.mp3");
		self->s.loopSound = G_SoundIndex("sound/weapons/force/absorbloop.wav");
		break;
	case FP_SEE:
		if (self->client->ps.forcePowerLevel[forcePower] == FORCE_LEVEL_1)
		{
			duration = 5000;
		}
		else if (self->client->ps.forcePowerLevel[forcePower] == FORCE_LEVEL_2)
		{
			duration = 10000;
		}
		else
		{
			duration = 20000;
		}

		self->client->ps.forcePowersActive |= 1 << forcePower;
		G_SoundOnEnt(self, CHAN_ITEM, "sound/weapons/force/see.mp3");
		self->s.loopSound = G_SoundIndex("sound/weapons/force/seeloop.wav");
		break;
	case FP_STASIS:
		break;
	case FP_DESTRUCTION:
		break;
	case FP_GRASP:
		duration = 1000;
		self->client->ps.forcePowersActive |= 1 << forcePower;
		break;
	case FP_REPULSE:
		self->client->ps.forcePowersActive |= 1 << forcePower;
		self->client->ps.powerups[PW_FORCE_REPULSE] = Q3_INFINITE;
		self->client->pushEffectFadeTime = 0;
		break;
	case FP_DEADLYSIGHT:
		if (self->client->ps.forcePowerLevel[forcePower] == FORCE_LEVEL_1)
		{
			duration = 700;
		}
		else if (self->client->ps.forcePowerLevel[forcePower] == FORCE_LEVEL_2)
		{
			duration = 1500;
		}
		else
		{
			duration = 3000;
		}
		self->client->ps.forcePowersActive |= 1 << forcePower;
		G_SoundOnEnt(self, CHAN_ITEM, "sound/weapons/force/absorbhit.mp3");
		self->s.loopSound = G_SoundIndex("sound/weapons/force/protectloop.mp3");
		break;
	case FP_BLAST:
		break;
	case FP_INSANITY:
		break;
	case FP_BLINDING:
		break;
	case FP_LIGHTNING_STRIKE:
		break;
	default:
		break;
	}
	if (duration)
	{
		self->client->ps.forcePowerDuration[forcePower] = level.time + duration;
	}
	else
	{
		self->client->ps.forcePowerDuration[forcePower] = 0;
	}

	WP_ForcePowerDrain(self, forcePower, overrideAmt);

	if (!self->s.number)
	{
		self->client->sess.missionStats.forceUsed[static_cast<int>(forcePower)]++;
	}
}

qboolean WP_ForcePowerAvailable(const gentity_t* self, forcePowers_t forcePower, int overrideAmt)
{
	if (forcePower == FP_LEVITATION)
	{
		return qtrue;
	}
	const int drain = overrideAmt ? overrideAmt : force_power_needed[forcePower];
	if (!drain)
	{
		return qtrue;
	}
	if (self->client->ps.forcePower < drain)
	{
		return qfalse;
	}
	return qtrue;
}

extern void CG_PlayerLockedWeaponSpeech(int jumping);
extern qboolean Rosh_TwinNearBy(const gentity_t* self);

qboolean WP_ForcePowerUsable(const gentity_t* self, forcePowers_t forcePower, int overrideAmt)
{
	if (!(self->client->ps.forcePowersKnown & 1 << forcePower))
	{
		//don't know this power
		return qfalse;
	}
	if (self->NPC && self->NPC->aiFlags & NPCAI_ROSH)
	{
		if (1 << forcePower & FORCE_POWERS_ROSH_FROM_TWINS)
		{
			//this is a force power we can only use when a twin is near us
			if (!Rosh_TwinNearBy(self))
			{
				return qfalse;
			}
		}
	}

	if (self->client->ps.forcePowerLevel[forcePower] <= 0)
	{
		//can't use this power
		return qfalse;
	}

	if (self->flags & FL_LOCK_PLAYER_WEAPONS)
		// yes this locked weapons check also includes force powers, if we need a separate check later I'll make one
	{
		if (self->s.number < MAX_CLIENTS)
		{
			CG_PlayerLockedWeaponSpeech(qfalse);
		}
		return qfalse;
	}

	if (self->client->ps.pm_flags & PMF_STUCK_TO_WALL)
	{
		//no offensive force powers when stuck to wall
		switch (forcePower)
		{
		case FP_GRIP:
		case FP_LIGHTNING:
		case FP_DRAIN:
		case FP_SABER_OFFENSE:
		case FP_SABER_DEFENSE:
		case FP_SABERTHROW:
		case FP_LIGHTNING_STRIKE:
		case FP_BLAST:
			return qfalse;
		default:
			break;
		}
	}

	if (in_camera && self->s.number < MAX_CLIENTS)
	{
		//player can't turn on force powers during cinematics
		return qfalse;
	}

	if (PM_LockedAnim(self->client->ps.torsoAnim) && self->client->ps.torsoAnimTimer)
	{
		//no force powers during these special anims
		return qfalse;
	}
	if (PM_SuperBreakLoseAnim(self->client->ps.torsoAnim)
		|| PM_SuperBreakWinAnim(self->client->ps.torsoAnim))
	{
		return qfalse;
	}

	if (self->client->ps.forcePowersActive & 1 << forcePower)
	{
		//already using this power
		return qfalse;
	}
	if (self->client->NPC_class == CLASS_ATST)
	{
		//Doh!  No force powers in an AT-ST!
		return qfalse;
	}
	const Vehicle_t* p_veh;
	if ((p_veh = G_IsRidingVehicle(self)) != nullptr)
	{
		//Doh!  No force powers when flying a vehicle!
		if (p_veh->m_pVehicleInfo->numHands > 1)
		{
			//if in a two-handed vehicle
			return qfalse;
		}
	}
	if (self->client->ps.viewEntity > 0 && self->client->ps.viewEntity < ENTITYNUM_WORLD)
	{
		//Doh!  No force powers when controlling an NPC
		return qfalse;
	}
	if (self->client->ps.eFlags & EF_LOCKED_TO_WEAPON)
	{
		//Doh!  No force powers when in an emplaced gun!
		return qfalse;
	}

	if (self->client->ps.saber[0].saberFlags & SFL_SINGLE_BLADE_THROWABLE //SaberStaff() //using staff
		&& !self->client->ps.dualSabers //only 1, in right hand
		&& !self->client->ps.saber[0].blade[1].active) //only first blade is on
	{
		//allow power
	}
	else
	{
		if (forcePower == FP_SABERTHROW && self->client->ps.saber[0].saberFlags & SFL_NOT_THROWABLE)
		{
			//cannot throw this kind of saber
			return qfalse;
		}

		if (self->client->ps.saber[0].Active())
		{
			if (self->client->ps.saber[0].saberFlags & SFL_TWO_HANDED)
			{
				if (g_saberRestrictForce->integer)
				{
					switch (forcePower)
					{
					case FP_PUSH:
					case FP_PULL:
					case FP_TELEPATHY:
					case FP_GRIP:
					case FP_GRASP:
					case FP_LIGHTNING:
					case FP_DRAIN:
					case FP_LIGHTNING_STRIKE:
					case FP_BLAST:
						return qfalse;
					default:
						break;
					}
				}
			}
			if (self->client->ps.saber[0].saberFlags & SFL_TWO_HANDED
				|| self->client->ps.dualSabers && self->client->ps.saber[1].Active())
			{
				//this saber requires the use of two hands OR our other hand is using an active saber too
				if (self->client->ps.saber[0].forceRestrictions & 1 << forcePower)
				{
					//this power is verboten when using this saber
					return qfalse;
				}
			}
		}
		if (self->client->ps.dualSabers && self->client->ps.saber[1].Active())
		{
			if (g_saberRestrictForce->integer)
			{
				switch (forcePower)
				{
				case FP_PUSH:
				case FP_PULL:
				case FP_TELEPATHY:
				case FP_GRIP:
				case FP_GRASP:
				case FP_LIGHTNING:
				case FP_DRAIN:
				case FP_LIGHTNING_STRIKE:
				case FP_BLAST:
					return qfalse;
				default:
					break;
				}
			}
			if (self->client->ps.saber[1].forceRestrictions & 1 << forcePower)
			{
				//this power is verboten when using this saber
				return qfalse;
			}
		}
	}

	return WP_ForcePowerAvailable(self, forcePower, overrideAmt);
}

void WP_ForcePowerStop(gentity_t* self, forcePowers_t forcePower)
{
	gentity_t* gripEnt;

	if (!(self->client->ps.forcePowersActive & 1 << forcePower))
	{
		//umm, wasn't doing it, so...
		return;
	}

	self->client->ps.forcePowersActive &= ~(1 << forcePower);

	switch (static_cast<int>(forcePower))
	{
	case FP_HEAL:
		if (self->client->ps.forcePowerLevel[forcePower] < FORCE_LEVEL_2)
		{
			//if in meditation pose, must come out of it
			if (self->client->ps.legsAnim == BOTH_FORCEHEAL_START)
			{
				NPC_SetAnim(self, SETANIM_LEGS, BOTH_FORCEHEAL_STOP, SETANIM_FLAG_OVERRIDE | SETANIM_FLAG_HOLD);
			}
			if (self->client->ps.torsoAnim == BOTH_FORCEHEAL_START)
			{
				NPC_SetAnim(self, SETANIM_TORSO, BOTH_FORCEHEAL_STOP, SETANIM_FLAG_OVERRIDE | SETANIM_FLAG_HOLD);
			}
			self->client->ps.saberMove = self->client->ps.saberBounceMove = LS_READY;
			//don't finish whatever saber anim you may have been in
			self->client->ps.saberBlocked = BLOCKED_NONE;
		}
		WP_StopForceHealEffects(self);
		if (self->health >= self->client->ps.stats[STAT_MAX_HEALTH] / 3)
		{
			gi.G2API_ClearSkinGore(self->ghoul2);
		}
		break;
	case FP_LEVITATION:
		self->client->ps.forcePowerDebounce[forcePower] = 0;
		break;
	case FP_SPEED:
		if (!self->s.number)
		{
			//player using force speed
			if (g_timescale->value != 1.0)
			{
				//if (!(self->client->ps.forcePowersActive& (1 << FP_RAGE)) || self->client->ps.forcePowerLevel[FP_RAGE] < FORCE_LEVEL_2)
				{
					//not slowed down because of force rage
					gi.cvar_set("timescale", "1");
				}
			}
		}
		if (self->client->ps.forcePowerLevel[forcePower] < FORCE_LEVEL_2)
		{
			self->client->ps.forceSpeedRecoveryTime = level.time + 1500; //recover for 1.5 seconds
		}
		else
		{
			self->client->ps.forceSpeedRecoveryTime = level.time + 1000; //recover for 1 seconds
		}
		self->s.loopSound = 0;
		break;
	case FP_PUSH:
		break;
	case FP_PULL:
		break;
	case FP_TELEPATHY:
		break;
	case FP_GRIP:
		if (self->NPC)
		{
			TIMER_Set(self, "gripping", -level.time);
		}
		if (self->client->ps.forceGripEntityNum < ENTITYNUM_WORLD)
		{
			gripEnt = &g_entities[self->client->ps.forceGripEntityNum];
			if (gripEnt)
			{
				gripEnt->s.loopSound = 0;
				if (gripEnt->client)
				{
					gripEnt->client->ps.eFlags &= ~EF_FORCE_GRIPPED;
					if (self->client->ps.forcePowerLevel[forcePower] > FORCE_LEVEL_1)
					{
						//sanity-cap the velocity
						float gripVel = VectorNormalize(gripEnt->client->ps.velocity);
						if (gripVel > 500.0f)
						{
							gripVel = 500.0f;
						}
						VectorScale(gripEnt->client->ps.velocity, gripVel, gripEnt->client->ps.velocity);
					}

					//FIXME: they probably dropped their weapon, should we make them flee?  Or should AI handle no-weapon behavior?
					//rww - RAGDOLL_BEGIN
#ifndef JK2_RAGDOLL_GRIPNOHEALTH
//rww - RAGDOLL_END
					if (gripEnt->health > 0)
						//rww - RAGDOLL_BEGIN
#endif
					//rww - RAGDOLL_END
					{
						int hold_time;
						if (gripEnt->health > 0)
						{
							G_AddEvent(gripEnt, EV_WATER_CLEAR, 0);
						}
						if (gripEnt->client->ps.forcePowerDebounce[FP_PUSH] > level.time)
						{
							//they probably pushed out of it
							hold_time = 0;
						}
						else if (gripEnt->s.weapon == WP_SABER)
						{
							//jedi recover faster
							hold_time = self->client->ps.forcePowerLevel[forcePower] * 200;
						}
						else
						{
							hold_time = self->client->ps.forcePowerLevel[forcePower] * 500;
						}
						//stop the anims soon, keep them locked in place for a bit
						if (gripEnt->client->ps.torsoAnim == BOTH_CHOKE1 || gripEnt->client->ps.torsoAnim == BOTH_CHOKE3
							|| gripEnt->client->ps.torsoAnim == BOTH_CHOKE4)
						{
							//stop choking anim on torso
							if (gripEnt->client->ps.torsoAnimTimer > hold_time)
							{
								gripEnt->client->ps.torsoAnimTimer = hold_time;
							}
						}
						if (gripEnt->client->ps.legsAnim == BOTH_CHOKE1 || gripEnt->client->ps.legsAnim == BOTH_CHOKE3
							|| gripEnt->client->ps.legsAnim == BOTH_CHOKE4)
						{
							//stop choking anim on legs
							gripEnt->client->ps.legsAnimTimer = 0;
							if (hold_time)
							{
								//lock them in place for a bit
								gripEnt->client->ps.pm_time = gripEnt->client->ps.torsoAnimTimer;
								gripEnt->client->ps.pm_flags |= PMF_TIME_KNOCKBACK;
								if (gripEnt->s.number)
								{
									//NPC
									gripEnt->painDebounceTime = level.time + gripEnt->client->ps.torsoAnimTimer;
								}
								else
								{
									//player
									gripEnt->aimDebounceTime = level.time + gripEnt->client->ps.torsoAnimTimer;
								}
							}
						}
						if (gripEnt->NPC)
						{
							if (!(gripEnt->NPC->aiFlags & NPCAI_DIE_ON_IMPACT))
							{
								//not falling to their death
								gripEnt->NPC->nextBStateThink = level.time + hold_time;
							}
							//if still alive after stopped gripping, let them wake others up
							if (gripEnt->health > 0)
							{
								G_AngerAlert(gripEnt);
							}
						}
					}
				}
				else
				{
					gripEnt->s.eFlags &= ~EF_FORCE_GRIPPED;
					if (gripEnt->s.eType == ET_MISSILE)
					{
						//continue normal movement
						if (gripEnt->s.weapon == WP_THERMAL)
						{
							gripEnt->s.pos.trType = TR_INTERPOLATE;
						}
						else
						{
							gripEnt->s.pos.trType = TR_LINEAR; //FIXME: what about gravity-effected projectiles?
						}
						VectorCopy(gripEnt->currentOrigin, gripEnt->s.pos.trBase);
						gripEnt->s.pos.trTime = level.time;
					}
					else
					{
						//drop it
						gripEnt->e_ThinkFunc = thinkF_G_RunObject;
						gripEnt->nextthink = level.time + FRAMETIME;
						gripEnt->s.pos.trType = TR_GRAVITY;
						VectorCopy(gripEnt->currentOrigin, gripEnt->s.pos.trBase);
						gripEnt->s.pos.trTime = level.time;
					}
				}
			}
			self->s.loopSound = 0;
			self->client->ps.forceGripEntityNum = ENTITYNUM_NONE;
		}
		if (self->client->ps.torsoAnim == BOTH_FORCEGRIP_HOLD
			|| self->client->ps.torsoAnim == BOTH_FORCEGRIP_OLD)
		{
			NPC_SetAnim(self, SETANIM_BOTH, BOTH_FORCEGRIP_RELEASE, SETANIM_FLAG_OVERRIDE | SETANIM_FLAG_HOLD);
		}
		break;
	case FP_LIGHTNING:
		if (self->NPC)
		{
			TIMER_Set(self, "holdLightning", -level.time);
		}
		if (self->client->ps.torsoAnim == BOTH_FORCELIGHTNING_HOLD || self->client->ps.torsoAnim == BOTH_FLAMETHROWER ||
			self->client->ps.torsoAnim == BOTH_FORCELIGHTNING_START)
		{
			NPC_SetAnim(self, SETANIM_TORSO, BOTH_FORCELIGHTNING_RELEASE, SETANIM_FLAG_OVERRIDE | SETANIM_FLAG_HOLD);
		}
		else if (self->client->ps.torsoAnim == BOTH_FORCE_2HANDEDLIGHTNING_HOLD || self->client->ps.torsoAnim ==
			BOTH_FORCE_2HANDEDLIGHTNING_START)
		{
			NPC_SetAnim(self, SETANIM_TORSO, BOTH_FORCE_2HANDEDLIGHTNING_RELEASE,
				SETANIM_FLAG_OVERRIDE | SETANIM_FLAG_HOLD);
		}
		if (self->client->ps.forcePowerLevel[forcePower] < FORCE_LEVEL_2)
		{
			//don't do it again for 3 seconds, minimum... FIXME: this should be automatic once regeneration is slower (normal)
			self->client->ps.forcePowerDebounce[forcePower] = level.time + 3000; //FIXME: define?
		}
		else
		{
			//stop the looping sound
			self->client->ps.forcePowerDebounce[forcePower] = level.time + 1000; //FIXME: define?
			self->s.loopSound = 0;
		}
		break;
	case FP_RAGE:
		if (self->client->ps.forcePowerLevel[forcePower] < FORCE_LEVEL_2)
		{
			self->client->ps.forceRageRecoveryTime = level.time + 10000; //recover for 10 seconds
		}
		else
		{
			self->client->ps.forceRageRecoveryTime = level.time + 5000; //recover for 5 seconds
		}

		if (self->client->ps.forcePowerDuration[forcePower] > level.time)
		{
			//still had time left, we cut it short
			self->client->ps.forceRageRecoveryTime -= self->client->ps.forcePowerDuration[forcePower] - level.time;
			//minus however much time you had left when you cut it short
		}
		self->s.loopSound = 0;
		if (self->NPC)
		{
			Jedi_RageStop(self);
		}
		if (self->chestBolt != -1)
		{
			G_StopEffect("force/rage2", self->playerModel, self->chestBolt, self->s.number);
		}
		break;
	case FP_DRAIN:
		if (self->NPC)
		{
			TIMER_Set(self, "draining", -level.time);
		}
		if (self->client->ps.forcePowerLevel[forcePower] < FORCE_LEVEL_2)
		{
			//don't do it again for 3 seconds, minimum... FIXME: this should be automatic once regeneration is slower (normal)
			self->client->ps.forcePowerDebounce[forcePower] = level.time + 3000; //FIXME: define?
		}
		else
		{
			//stop the looping sound
			self->client->ps.forcePowerDebounce[forcePower] = level.time + 1000; //FIXME: define?
			self->s.loopSound = 0;
		}
		//drop them
		if (self->client->ps.forceDrainEntityNum < ENTITYNUM_WORLD)
		{
			gentity_t* drainEnt = &g_entities[self->client->ps.forceDrainEntityNum];
			if (drainEnt)
			{
				if (drainEnt->client)
				{
					drainEnt->client->ps.eFlags &= ~EF_FORCE_DRAINED;
					//VectorClear( drainEnt->client->ps.velocity );
					if (drainEnt->health > 0)
					{
						if (drainEnt->client->ps.forcePowerDebounce[FP_PUSH] > level.time)
						{
							//they probably pushed out of it
						}
						else
						{
							if (drainEnt->client->ps.torsoAnim != BOTH_FORCEPUSH)
							{
								//don't stop the push
								drainEnt->client->ps.torsoAnimTimer = 0;
							}
							drainEnt->client->ps.legsAnimTimer = 0;
						}
						if (drainEnt->NPC)
						{
							//if still alive after stopped draining, let them wake others up
							G_AngerAlert(drainEnt);
						}
					}
					else
					{
						//leave the effect playing on them for a few seconds
						//drainEnt->client->ps.eFlags |= EF_FORCE_DRAINED;
						drainEnt->s.powerups |= 1 << PW_DRAINED;
						drainEnt->client->ps.powerups[PW_DRAINED] = level.time + Q_irand(1000, 4000);
					}
				}
			}
			self->client->ps.forceDrainEntityNum = ENTITYNUM_NONE;
		}
		if (self->client->ps.torsoAnim == BOTH_HUGGER1)
		{
			//old anim
			NPC_SetAnim(self, SETANIM_BOTH, BOTH_HUGGERSTOP1, SETANIM_FLAG_OVERRIDE | SETANIM_FLAG_HOLD);
		}
		else if (self->client->ps.torsoAnim == BOTH_FORCE_DRAIN_GRAB_START
			|| self->client->ps.torsoAnim == BOTH_FORCE_DRAIN_GRAB_HOLD)
		{
			//new anim
			NPC_SetAnim(self, SETANIM_BOTH, BOTH_FORCE_DRAIN_GRAB_END, SETANIM_FLAG_OVERRIDE | SETANIM_FLAG_HOLD);
		}
		else if (self->client->ps.torsoAnim == BOTH_FORCE_DRAIN_HOLD
			|| self->client->ps.torsoAnim == BOTH_FORCE_DRAIN_START)
		{
			NPC_SetAnim(self, SETANIM_TORSO, BOTH_FORCE_DRAIN_RELEASE, SETANIM_FLAG_OVERRIDE | SETANIM_FLAG_HOLD);
		}
		break;
	case FP_PROTECT:
		self->s.loopSound = 0;
		break;
	case FP_ABSORB:
		self->s.loopSound = 0;
		if (self->client->ps.legsAnim == BOTH_FORCE_ABSORB_START)
		{
			NPC_SetAnim(self, SETANIM_LEGS, BOTH_FORCE_ABSORB_END, SETANIM_FLAG_OVERRIDE | SETANIM_FLAG_HOLD);
		}
		if (self->client->ps.torsoAnim == BOTH_FORCE_ABSORB_START)
		{
			NPC_SetAnim(self, SETANIM_TORSO, BOTH_FORCE_ABSORB_END, SETANIM_FLAG_OVERRIDE | SETANIM_FLAG_HOLD);
		}
		if (self->client->ps.forcePowerLevel[forcePower] < FORCE_LEVEL_2)
		{
			//was stuck, free us in case we interrupted it or something
			self->client->ps.weaponTime = 0;
			self->client->ps.pm_flags &= ~PMF_TIME_KNOCKBACK;
			self->client->ps.pm_time = 0;
			if (self->s.number)
			{
				//NPC
				self->painDebounceTime = 0;
			}
			else
			{
				//player
				self->aimDebounceTime = 0;
			}
		}
		break;
	case FP_SEE:
		self->s.loopSound = 0;
		break;
	case FP_STASIS:
		break;
	case FP_DESTRUCTION:
		break;
	case FP_GRASP:
		if (self->NPC)
		{
			TIMER_Set(self, "grasping", -level.time);
		}
		if (self->client->ps.forceGripEntityNum < ENTITYNUM_WORLD)
		{
			gripEnt = &g_entities[self->client->ps.forceGripEntityNum];
			if (gripEnt)
			{
				gripEnt->s.loopSound = 0;
				if (gripEnt->client)
				{
					gripEnt->client->ps.eFlags &= ~EF_FORCE_GRABBED;
					if (self->client->ps.forcePowerLevel[forcePower] > FORCE_LEVEL_1)
					{
						//sanity-cap the velocity
						float gripVel = VectorNormalize(gripEnt->client->ps.velocity);
						if (gripVel > 500.0f)
						{
							gripVel = 500.0f;
						}
						VectorScale(gripEnt->client->ps.velocity, gripVel, gripEnt->client->ps.velocity);
					}

					//FIXME: they probably dropped their weapon, should we make them flee?  Or should AI handle no-weapon behavior?
					//rww - RAGDOLL_BEGIN
#ifndef JK2_RAGDOLL_GRIPNOHEALTH
//rww - RAGDOLL_END
					if (gripEnt->health > 0)
						//rww - RAGDOLL_BEGIN
#endif
					//rww - RAGDOLL_END
					{
						int hold_time;
						if (gripEnt->health > 0)
						{
							G_AddEvent(gripEnt, EV_WATER_CLEAR, 0);
						}
						if (gripEnt->client->ps.forcePowerDebounce[FP_PUSH] > level.time)
						{
							//they probably pushed out of it
							hold_time = 0;
						}
						else if (gripEnt->s.weapon == WP_SABER)
						{
							//jedi recover faster
							hold_time = self->client->ps.forcePowerLevel[forcePower] * 200;
						}
						else
						{
							hold_time = self->client->ps.forcePowerLevel[forcePower] * 500;
						}
						//stop the anims soon, keep them locked in place for a bit
						if (gripEnt->client->ps.torsoAnim == BOTH_CHOKE1 || gripEnt->client->ps.torsoAnim == BOTH_CHOKE3
							|| gripEnt->client->ps.torsoAnim == BOTH_CHOKE4)
						{
							//stop choking anim on torso
							if (gripEnt->client->ps.torsoAnimTimer > hold_time)
							{
								gripEnt->client->ps.torsoAnimTimer = hold_time;
							}
						}
						if (gripEnt->client->ps.legsAnim == BOTH_PULLED_INAIR_F || gripEnt->client->ps.legsAnim ==
							BOTH_SWIM_IDLE1)
						{
							//stop choking anim on legs
							gripEnt->client->ps.legsAnimTimer = 0;
							if (hold_time)
							{
								//lock them in place for a bit
								gripEnt->client->ps.pm_time = gripEnt->client->ps.torsoAnimTimer;
								gripEnt->client->ps.pm_flags |= PMF_TIME_KNOCKBACK;
								if (gripEnt->s.number)
								{
									//NPC
									gripEnt->painDebounceTime = level.time + gripEnt->client->ps.torsoAnimTimer;
								}
								else
								{
									//player
									gripEnt->aimDebounceTime = level.time + gripEnt->client->ps.torsoAnimTimer;
								}
							}
						}
						if (gripEnt->NPC)
						{
							if (!(gripEnt->NPC->aiFlags & NPCAI_DIE_ON_IMPACT))
							{
								//not falling to their death
								gripEnt->NPC->nextBStateThink = level.time + hold_time;
							}
							//if still alive after stopped gripping, let them wake others up
							if (gripEnt->health > 0)
							{
								G_AngerAlert(gripEnt);
							}
						}
					}
				}
				else
				{
					gripEnt->s.eFlags &= ~EF_FORCE_GRABBED;
					if (gripEnt->s.eType == ET_MISSILE)
					{
						//continue normal movement
						if (gripEnt->s.weapon == WP_THERMAL)
						{
							gripEnt->s.pos.trType = TR_INTERPOLATE;
						}
						else
						{
							gripEnt->s.pos.trType = TR_LINEAR; //FIXME: what about gravity-effected projectiles?
						}
						VectorCopy(gripEnt->currentOrigin, gripEnt->s.pos.trBase);
						gripEnt->s.pos.trTime = level.time;
					}
					else
					{
						//drop it
						gripEnt->e_ThinkFunc = thinkF_G_RunObject;
						gripEnt->nextthink = level.time + FRAMETIME;
						gripEnt->s.pos.trType = TR_GRAVITY;
						VectorCopy(gripEnt->currentOrigin, gripEnt->s.pos.trBase);
						gripEnt->s.pos.trTime = level.time;
					}
				}
			}
			self->s.loopSound = 0;
			self->client->ps.forceGripEntityNum = ENTITYNUM_NONE;
		}
		if (self->client->ps.torsoAnim == BOTH_FORCEGRIP_HOLD)
		{
			NPC_SetAnim(self, SETANIM_BOTH, BOTH_FORCEGRIP_RELEASE, SETANIM_FLAG_OVERRIDE | SETANIM_FLAG_HOLD);
		}
		break;
	case FP_REPULSE:
		self->client->ps.powerups[PW_FORCE_REPULSE] = 0;

		if (self->client->ps.repulseChargeStart)
		{
			self->s.loopSound = 0;
			ForceRepulseThrow(self, level.time - self->client->ps.repulseChargeStart);
			self->client->ps.repulseChargeStart = 0;
		}
		break;
	case FP_DEADLYSIGHT:
		self->s.loopSound = 0;
		break;
	case FP_BLAST:
		break;
	case FP_INSANITY:
		break;
	case FP_BLINDING:
		break;
	default:
		break;
	}
}

void WP_ForceForceThrow(gentity_t* thrower)
{
	if (!thrower || !thrower->client)
	{
		return;
	}
	qboolean relock = qfalse;
	if (!(thrower->client->ps.forcePowersKnown & 1 << FP_PUSH))
	{
		//give them push just for this specific purpose
		thrower->client->ps.forcePowersKnown |= 1 << FP_PUSH;
		thrower->client->ps.forcePowerLevel[FP_PUSH] = FORCE_LEVEL_1;
	}

	if (thrower->NPC
		&& thrower->NPC->aiFlags & NPCAI_HEAL_ROSH
		&& thrower->flags & FL_LOCK_PLAYER_WEAPONS)
	{
		thrower->flags &= ~FL_LOCK_PLAYER_WEAPONS;
		relock = qtrue;
	}

	ForceThrow(thrower, qfalse);

	if (relock)
	{
		thrower->flags |= FL_LOCK_PLAYER_WEAPONS;
	}

	if (thrower)
	{
		//take it back off
		thrower->client->ps.forcePowersKnown &= ~(1 << FP_PUSH);
		thrower->client->ps.forcePowerLevel[FP_PUSH] = FORCE_LEVEL_0;
	}
}

extern qboolean PM_ForceJumpingUp(const gentity_t* gent);

static void wp_force_power_run(gentity_t* self, forcePowers_t forcePower, usercmd_t* cmd)
{
	float speed;
	gentity_t* gripEnt;
	vec3_t angles, dir, gripOrg, gripEntOrg;
	float dist;
	extern usercmd_t ucmd;

	switch (static_cast<int>(forcePower))
	{
	case FP_HEAL:
		if (self->client->ps.forceHealCount >= FP_MaxForceHeal(self) || self->health >= self->client->ps.stats[
			STAT_MAX_HEALTH])
		{
			//fully healed or used up all 25
			if (!Q3_TaskIDPending(self, TID_CHAN_VOICE))
			{
				int index = Q_irand(1, 4);
				if (self->s.number < MAX_CLIENTS)
				{
					if (!Q_stricmp("kyle", self->client->clientInfo.customBasicSoundDir)
						|| !Q_stricmp("kyle2", self->client->clientInfo.customBasicSoundDir)
						|| !Q_stricmp("Kyle_boss", self->client->clientInfo.customBasicSoundDir)
						|| !Q_stricmp("KyleJK2", self->client->clientInfo.customBasicSoundDir)
						|| !Q_stricmp("KyleJK2noforce", self->client->clientInfo.customBasicSoundDir)
						|| !Q_stricmp("Kyle_JKA", self->client->clientInfo.customBasicSoundDir)
						|| !Q_stricmp("Kyle_MOD", self->client->clientInfo.customBasicSoundDir)
						|| !Q_stricmp("KylePlayer", self->client->clientInfo.customBasicSoundDir)
						|| !Q_stricmp("md_kyle", self->client->clientInfo.customBasicSoundDir)
						|| !Q_stricmp("md_kyle_df2_lvl1", self->client->clientInfo.customBasicSoundDir)
						|| !Q_stricmp("md_kyle_df2_lvl2", self->client->clientInfo.customBasicSoundDir)
						|| !Q_stricmp("md_kyle_df2_lvl3", self->client->clientInfo.customBasicSoundDir)
						|| !Q_stricmp("md_kyle_df2_lvl4", self->client->clientInfo.customBasicSoundDir)
						|| !Q_stricmp("md_kyle_df2_lvl5", self->client->clientInfo.customBasicSoundDir)
						|| !Q_stricmp("md_kyle_df2_lvl6", self->client->clientInfo.customBasicSoundDir)
						|| !Q_stricmp("md_kyle_alt", self->client->clientInfo.customBasicSoundDir)
						|| !Q_stricmp("md_kyle_sj", self->client->clientInfo.customBasicSoundDir)
						|| !Q_stricmp("md_kyle_jm", self->client->clientInfo.customBasicSoundDir)
						|| !Q_stricmp("md_kyle_old", self->client->clientInfo.customBasicSoundDir)
						|| !Q_stricmp("md_kyle_df2_coat", self->client->clientInfo.customBasicSoundDir)
						|| !Q_stricmp("md_kyle_df2", self->client->clientInfo.customBasicSoundDir)
						|| !Q_stricmp("md_kyle_df2ig", self->client->clientInfo.customBasicSoundDir)
						|| !Q_stricmp("md_kyle_df2lowpoly", self->client->clientInfo.customBasicSoundDir)
						|| !Q_stricmp("md_kyle_mots", self->client->clientInfo.customBasicSoundDir)
						|| !Q_stricmp("md_kyle_emp", self->client->clientInfo.customBasicSoundDir)
						|| !Q_stricmp("md_kyle_df1", self->client->clientInfo.customBasicSoundDir)
						|| !Q_stricmp("md_kyle_officer", self->client->clientInfo.customBasicSoundDir)
						|| !Q_stricmp("jedi_kyle", self->client->clientInfo.customBasicSoundDir))
					{
						G_SoundOnEnt(self, CHAN_VOICE, va("sound/weapons/force/heal%d.mp3", index));
					}
					else
					{
						G_SoundOnEnt(self, CHAN_VOICE, va("sound/weapons/force/heal%d_%c.mp3", index, g_sex->string[0]));
					}
				}
				else if (self->NPC)
				{
					if (self->NPC->blockedSpeechDebounceTime <= level.time)
					{
						//enough time has passed since our last speech
						if (Q3_TaskIDPending(self, TID_CHAN_VOICE))
						{
							//not playing a scripted line
							//say "Ahhh...."
							if (self->NPC->stats.sex == SEX_MALE
								|| self->NPC->stats.sex == SEX_NEUTRAL)
							{
								if (!Q_stricmp("kyle", self->soundSet)
									|| !Q_stricmp("kyle2", self->soundSet)
									|| !Q_stricmp("Kyle_boss", self->soundSet)
									|| !Q_stricmp("KyleJK2", self->soundSet)
									|| !Q_stricmp("KyleJK2noforce", self->soundSet)
									|| !Q_stricmp("Kyle_JKA", self->soundSet)
									|| !Q_stricmp("Kyle_MOD", self->soundSet)
									|| !Q_stricmp("KylePlayer", self->soundSet)
									|| !Q_stricmp("md_kyle", self->soundSet)
									|| !Q_stricmp("md_kyle_df2_lvl1", self->soundSet)
									|| !Q_stricmp("md_kyle_df2_lvl2", self->soundSet)
									|| !Q_stricmp("md_kyle_df2_lvl3", self->soundSet)
									|| !Q_stricmp("md_kyle_df2_lvl4", self->soundSet)
									|| !Q_stricmp("md_kyle_df2_lvl5", self->soundSet)
									|| !Q_stricmp("md_kyle_df2_lvl6", self->soundSet)
									|| !Q_stricmp("md_kyle_alt", self->soundSet)
									|| !Q_stricmp("md_kyle_sj", self->soundSet)
									|| !Q_stricmp("md_kyle_jm", self->soundSet)
									|| !Q_stricmp("md_kyle_old", self->soundSet)
									|| !Q_stricmp("md_kyle_df2_coat", self->soundSet)
									|| !Q_stricmp("md_kyle_df2", self->soundSet)
									|| !Q_stricmp("md_kyle_df2ig", self->soundSet)
									|| !Q_stricmp("md_kyle_df2lowpoly", self->soundSet)
									|| !Q_stricmp("md_kyle_mots", self->soundSet)
									|| !Q_stricmp("md_kyle_emp", self->soundSet)
									|| !Q_stricmp("md_kyle_df1", self->soundSet)
									|| !Q_stricmp("md_kyle_officer", self->soundSet)
									|| !Q_stricmp("jedi_kyle", self->soundSet))
								{
									G_SoundOnEnt(self, CHAN_VOICE, va("sound/weapons/force/heal%d.mp3", index));
								}
								else
								{
									G_SoundOnEnt(self, CHAN_VOICE, va("sound/weapons/force/heal%d_m.mp3", index));
								}
							}
							else //all other sexes use female sounds
							{
								G_SoundOnEnt(self, CHAN_VOICE, va("sound/weapons/force/heal%d_f.mp3", index));
							}
						}
					}
				}
			}
			WP_ForcePowerStop(self, forcePower);
		}
		else if (self->client->ps.forcePowerLevel[FP_HEAL] < FORCE_LEVEL_3 && (cmd->buttons & BUTTON_ATTACK || cmd->
			buttons & BUTTON_ALT_ATTACK || self->painDebounceTime > level.time || self->client->ps.weaponTime && self->
			client->ps.weapon != WP_NONE))
		{
			//attacked or was hit while healing...
			//stop healing
			WP_ForcePowerStop(self, forcePower);
		}
		else if (self->client->ps.forcePowerLevel[FP_HEAL] < FORCE_LEVEL_2 && (cmd->rightmove || cmd->forwardmove || cmd
			->upmove > 0))
		{
			//moved while healing... FIXME: also, in WP_ForcePowerStart, stop healing if any other force power is used
			//stop healing
			WP_ForcePowerStop(self, forcePower);
		}
		else if (self->client->ps.forcePowerDebounce[FP_HEAL] < level.time)
		{
			//time to heal again
			if (WP_ForcePowerAvailable(self, forcePower, 4))
			{
				//have available power
				int healInterval = FP_ForceHealInterval(self);
				int healAmount = 1; //hard, normal healing rate
				if (self->s.number < MAX_CLIENTS)
				{
					if (g_spskill->integer == 1)
					{
						//medium, heal twice as fast
						healAmount *= 2;
					}
					else if (g_spskill->integer == 0)
					{
						//easy, heal 3 times as fast...
						healAmount *= 3;
					}
				}
				if (self->health + healAmount > self->client->ps.stats[STAT_MAX_HEALTH])
				{
					healAmount = self->client->ps.stats[STAT_MAX_HEALTH] - self->health;
				}
				if (self->health >= self->client->ps.stats[STAT_MAX_HEALTH])
				{
					// Shield Heal skill. Done when player has full HP
					if (self->client->ps.stats[STAT_ARMOR] < 100)
					{
						self->client->ps.stats[STAT_ARMOR] += 25;

						if (self->client->ps.stats[STAT_ARMOR] > 100)
						{
							self->client->ps.stats[STAT_ARMOR] = 100;
						}

						G_SoundOnEnt(self, CHAN_ITEM, "sound/player/pickupshield.wav");
					}
				}
				self->health += healAmount;
				self->client->ps.forceHealCount += healAmount;
				self->client->ps.forcePowerDebounce[FP_HEAL] = level.time + healInterval;
				WP_ForcePowerDrain(self, forcePower, 4);
			}
			else
			{
				//stop
				WP_ForcePowerStop(self, forcePower);
			}
		}
		break;
	case FP_LEVITATION:
		if (self->client->ps.groundEntityNum != ENTITYNUM_NONE && !self->client->ps.forceJumpZStart ||
			PM_InLedgeMove(self->client->ps.legsAnim) || PM_InWallHoldMove(self->client->ps.legsAnim))
		{
			//done with jump
			WP_ForcePowerStop(self, forcePower);
		}
		else
		{
			if (PM_ForceJumpingUp(self))
			{
				//holding jump in air
				if (cmd->upmove > 10)
				{
					//still trying to go up
					if (WP_ForcePowerAvailable(self, FP_LEVITATION, 1))
					{
						if (self->client->ps.forcePowerDebounce[FP_LEVITATION] < level.time)
						{
							WP_ForcePowerDrain(self, FP_LEVITATION, 5);
							self->client->ps.forcePowerDebounce[FP_LEVITATION] = level.time + 100;
						}
						self->client->ps.forcePowersActive |= 1 << FP_LEVITATION;
						self->client->ps.forceJumpCharge = 1;
						//just used as a flag for the player, cleared when he lands
					}
					else
					{
						//cut the jump short
						WP_ForcePowerStop(self, forcePower);
					}
				}
				else
				{
					//cut the jump short
					WP_ForcePowerStop(self, forcePower);
				}
			}
			else
			{
				WP_ForcePowerStop(self, forcePower);
			}
		}
		break;
	case FP_SPEED:

		speed = forceSpeedValue[self->client->ps.forcePowerLevel[FP_SPEED]];

		if (!self->s.number)
		{
			//player using force speed
			//if (!(self->client->ps.forcePowersActive& (1 << FP_RAGE))
			//|| self->client->ps.forcePowerLevel[FP_SPEED] >= self->client->ps.forcePowerLevel[FP_RAGE])
			{
				//either not using rage or rage is at a lower level than speed
				gi.cvar_set("timescale", va("%4.2f", speed));
				if (g_timescale->value > speed)
				{
					float newSpeed;
					newSpeed = g_timescale->value - 0.05;
					if (newSpeed < speed)
					{
						newSpeed = speed;
					}
					gi.cvar_set("timescale", va("%4.2f", newSpeed));
				}
			}
		}
		break;
	case FP_PUSH:
		break;
	case FP_PULL:
		break;
	case FP_TELEPATHY:
		break;
	case FP_GRIP:
	{
		if (!WP_ForcePowerAvailable(self, FP_GRIP, 0)
			|| self->client->ps.forcePowerLevel[FP_GRIP] > FORCE_LEVEL_1 && !self->s.number && !(cmd->buttons &
				BUTTON_FORCEGRIP))
		{
			WP_ForcePowerStop(self, FP_GRIP);
			return;
		}
		if (self->client->ps.forceGripEntityNum >= 0 && self->client->ps.forceGripEntityNum < ENTITYNUM_WORLD)
		{
			gripEnt = &g_entities[self->client->ps.forceGripEntityNum];

			if (!gripEnt || !gripEnt->inuse)
			{
				//invalid or freed ent
				WP_ForcePowerStop(self, FP_GRIP);
				return;
			}
#ifndef JK2_RAGDOLL_GRIPNOHEALTH
			if (gripEnt->health <= 0 && gripEnt->takedamage)
			{//either invalid ent, or dead ent
				WP_ForcePowerStop(self, FP_GRIP);
				return;
			}
			else
#endif
			{
				if (self->client->ps.forcePowerLevel[FP_GRIP] == FORCE_LEVEL_1
					&& gripEnt->client
					&& gripEnt->client->ps.groundEntityNum == ENTITYNUM_NONE
					&& gripEnt->client->moveType != MT_FLYSWIM)
				{
					WP_ForcePowerStop(self, FP_GRIP);
					return;
				}
				if (gripEnt->client && gripEnt->client->moveType == MT_FLYSWIM && VectorLengthSquared(
					gripEnt->client->ps.velocity) > 300 * 300)
				{
					//flying creature broke free
					WP_ForcePowerStop(self, FP_GRIP);
					return;
				}
				if (gripEnt->client
					&& gripEnt->health > 0 //dead dudes don't fly
					&& (gripEnt->client->NPC_class == CLASS_BOBAFETT || gripEnt->client->NPC_class == CLASS_MANDO ||
						gripEnt->client->NPC_class == CLASS_ROCKETTROOPER)
					&& self->client->ps.forcePowerDebounce[FP_GRIP] < level.time
					&& !Q_irand(0, 3))
				{
					//boba fett - fly away!
					gripEnt->client->ps.forceJumpCharge = 0; //so we don't play the force flip anim
					gripEnt->client->ps.velocity[2] = 250;
					gripEnt->client->ps.forceJumpZStart = gripEnt->currentOrigin[2];
					//so we don't take damage if we land at same height
					gripEnt->client->ps.pm_flags |= PMF_JUMPING;
					G_AddEvent(gripEnt, EV_JUMP, 0);
					JET_FlyStart(gripEnt);
					WP_ForcePowerStop(self, FP_GRIP);
					return;
				}
				if (gripEnt->NPC
					&& gripEnt->client
					&& gripEnt->client->ps.forcePowersKnown
					&& (gripEnt->client->NPC_class == CLASS_REBORN || gripEnt->client->ps.weapon == WP_SABER)
					&& !Jedi_CultistDestroyer(gripEnt)
					&& !Q_irand(0, 100 - gripEnt->NPC->stats.evasion * 8 - g_spskill->integer * 20))
				{
					//a jedi who broke free FIXME: maybe have some minimum grip length- a reaction time?
					WP_ForceForceThrow(gripEnt);
					WP_ForcePowerStop(self, FP_GRIP);
					return;
				}
				if (PM_SaberInAttack(self->client->ps.saberMove) || PM_SaberInStart(self->client->ps.saberMove))
				{
					//started an attack
					WP_ForcePowerStop(self, FP_GRIP);
					return;
				}
				int gripLevel = self->client->ps.forcePowerLevel[FP_GRIP];
				if (gripEnt->client)
				{
					gripLevel = wp_absorb_conversion(gripEnt, gripEnt->client->ps.forcePowerLevel[FP_ABSORB], self,
						FP_GRIP, self->client->ps.forcePowerLevel[FP_GRIP],
						force_power_needed[gripLevel]);
				}
				if (!gripLevel)
				{
					WP_ForcePowerStop(self, forcePower);
					return;
				}
				if (self->s.weapon == WP_NONE
					|| self->s.weapon == WP_MELEE
					|| self->s.weapon == WP_SABER && !self->client->ps.SaberActive())
				{
					NPC_SetAnim(self, SETANIM_TORSO, BOTH_FORCEGRIP_HOLD,
						SETANIM_FLAG_OVERRIDE | SETANIM_FLAG_HOLD);
				}
				else
				{
					NPC_SetAnim(self, SETANIM_TORSO, BOTH_FORCEGRIP_OLD, SETANIM_FLAG_OVERRIDE | SETANIM_FLAG_HOLD);
				}
				if (self->client->ps.torsoAnimTimer < 100)
				{
					//we were already playing this anim, we didn't want to restart it, but we want to hold it for at least 100ms.
					self->client->ps.torsoAnimTimer = 200;
				}
				//}
				//get their org
				VectorCopy(self->client->ps.viewangles, angles);
				angles[0] -= 10;
				AngleVectors(angles, dir, nullptr, nullptr);
				if (gripEnt->client)
				{
					//move
					VectorCopy(gripEnt->client->renderInfo.headPoint, gripEntOrg);
				}
				else
				{
					VectorCopy(gripEnt->currentOrigin, gripEntOrg);
				}

				//how far are they
				dist = Distance(self->client->renderInfo.handLPoint, gripEntOrg);
				if (self->client->ps.forcePowerLevel[FP_GRIP] == FORCE_LEVEL_2 &&
					(!in_front(gripEntOrg, self->client->renderInfo.handLPoint, self->client->ps.viewangles,
						0.3f) ||
						DistanceSquared(gripEntOrg, self->client->renderInfo.handLPoint) > FORCE_GRIP_DIST_SQUARED))
				{
					//must face them
					WP_ForcePowerStop(self, FP_GRIP);
					return;
				}

				//check for lift or carry
				if (self->client->ps.forcePowerLevel[FP_GRIP] > FORCE_LEVEL_2
					&& (!gripEnt->client || !gripEnt->message && !(gripEnt->flags & FL_NO_KNOCKBACK)))
				{
					//carry
					//cap dist
					if (dist > FORCE_GRIP_3_MAX_DIST)
					{
						dist = FORCE_GRIP_3_MAX_DIST;
					}
					else if (dist < FORCE_GRIP_3_MIN_DIST)
					{
						dist = FORCE_GRIP_3_MIN_DIST;
					}
					VectorMA(self->client->renderInfo.handLPoint, dist, dir, gripOrg);
				}
				else if (self->client->ps.forcePowerLevel[FP_GRIP] > FORCE_LEVEL_1)
				{
					//just lift
					VectorCopy(self->client->ps.forceGripOrg, gripOrg);
				}
				else
				{
					VectorCopy(gripEnt->currentOrigin, gripOrg);
				}
				if (self->client->ps.forcePowerLevel[FP_GRIP] > FORCE_LEVEL_1)
				{
					//if holding him, make sure there's a clear LOS between my hand and him
					trace_t gripTrace;
					gi.trace(&gripTrace, self->client->renderInfo.handLPoint, nullptr, nullptr, gripEntOrg,
						ENTITYNUM_NONE, MASK_FORCE_PUSH, static_cast<EG2_Collision>(0), 0);
					if (gripTrace.startsolid
						|| gripTrace.allsolid
						|| gripTrace.fraction < 1.0f)
					{
						//no clear trace, drop them
						WP_ForcePowerStop(self, FP_GRIP);
						return;
					}
				}
				//now move them
				if (gripEnt->client)
				{
					if (self->client->ps.forcePowerLevel[FP_GRIP] > FORCE_LEVEL_1)
					{
						//level 1 just holds them
						VectorSubtract(gripOrg, gripEntOrg, gripEnt->client->ps.velocity);
						if (self->client->ps.forcePowerLevel[FP_GRIP] > FORCE_LEVEL_2
							&& (!gripEnt->client || !gripEnt->message && !(gripEnt->flags & FL_NO_KNOCKBACK)))
						{
							//level 2 just lifts them
							float gripDist = VectorNormalize(gripEnt->client->ps.velocity) / 3.0f;
							if (gripDist < 20.0f)
							{
								if (gripDist < 2.0f)
								{
									VectorClear(gripEnt->client->ps.velocity);
								}
								else
								{
									VectorScale(gripEnt->client->ps.velocity, gripDist * gripDist,
										gripEnt->client->ps.velocity);
								}
							}
							else
							{
								VectorScale(gripEnt->client->ps.velocity, gripDist * gripDist,
									gripEnt->client->ps.velocity);
							}
						}
					}
					//stop them from thinking
					gripEnt->client->ps.pm_time = 2000;
					gripEnt->client->ps.pm_flags |= PMF_TIME_KNOCKBACK;
					if (gripEnt->NPC)
					{
						if (!(gripEnt->NPC->aiFlags & NPCAI_DIE_ON_IMPACT))
						{
							//not falling to their death
							gripEnt->NPC->nextBStateThink = level.time + 2000;
						}
						if (self->client->ps.forcePowerLevel[FP_GRIP] > FORCE_LEVEL_1)
						{
							//level 1 just holds them
							vectoangles(dir, angles);
							gripEnt->NPC->desiredYaw = AngleNormalize180(angles[YAW] + 180);
							gripEnt->NPC->desiredPitch = -angles[PITCH];
							SaveNPCGlobals();
							SetNPCGlobals(gripEnt);
							NPC_UpdateAngles(qtrue, qtrue);
							gripEnt->NPC->last_ucmd.angles[0] = ucmd.angles[0];
							gripEnt->NPC->last_ucmd.angles[1] = ucmd.angles[1];
							gripEnt->NPC->last_ucmd.angles[2] = ucmd.angles[2];
							RestoreNPCGlobals();
						}
					}
					else if (!gripEnt->s.number)
					{
						gripEnt->enemy = self;
						NPC_SetLookTarget(gripEnt, self->s.number, level.time + 1000);
					}

					gripEnt->client->ps.eFlags |= EF_FORCE_GRIPPED;
					//dammit!  Make sure that saber stays off!

					if (!(gripEnt->client->ps.ManualBlockingFlags & 1 << HOLDINGBLOCK))
					{
						WP_DeactivateSaber(gripEnt);
					}
					else
					{
						gripEnt->client->ps.ManualBlockingFlags &= ~(1 << HOLDINGBLOCK);
						gripEnt->client->ps.ManualBlockingFlags &= ~(1 << HOLDINGBLOCKANDATTACK);
						gripEnt->client->ps.ManualBlockingFlags &= ~(1 << PERFECTBLOCKING);
						gripEnt->client->ps.ManualBlockingFlags &= ~(1 << MBF_NPCBLOCKING);
						WP_DeactivateSaber(gripEnt);
					}
				}
				else
				{
					//move
					if (self->client->ps.forcePowerLevel[FP_GRIP] > FORCE_LEVEL_1)
					{
						//level 1 just holds them
						VectorCopy(gripEnt->currentOrigin, gripEnt->s.pos.trBase);
						VectorSubtract(gripOrg, gripEntOrg, gripEnt->s.pos.trDelta);
						if (self->client->ps.forcePowerLevel[FP_GRIP] > FORCE_LEVEL_2
							&& (!gripEnt->client || !gripEnt->message && !(gripEnt->flags & FL_NO_KNOCKBACK)))
						{
							//level 2 just lifts them
							VectorScale(gripEnt->s.pos.trDelta, 10, gripEnt->s.pos.trDelta);
						}
						gripEnt->s.pos.trType = TR_LINEAR;
						gripEnt->s.pos.trTime = level.time;
					}

					gripEnt->s.eFlags |= EF_FORCE_GRIPPED;
				}
				AddSightEvent(self, gripOrg, 128, AEL_DISCOVERED, 20);

				if (self->client->ps.forcePowerDebounce[FP_GRIP] < level.time)
				{
					if (!gripEnt->client
						|| gripEnt->client->NPC_class != CLASS_VEHICLE
						|| gripEnt->m_pVehicle
						&& gripEnt->m_pVehicle->m_pVehicleInfo
						&& gripEnt->m_pVehicle->m_pVehicleInfo->type == VH_ANIMAL)
					{
						//we don't damage the empty vehicle
						gripEnt->painDebounceTime = 0;
						int grip_dmg = forceGripDamage[self->client->ps.forcePowerLevel[FP_GRIP]];

						if (gripLevel != -1)
						{
							if (gripLevel == 1)
							{
								grip_dmg = floor(static_cast<float>(grip_dmg) / 3.0f);
							}
							// Grip does extra damage in newgameplus mode
							else if (g_newgameplusJKA->integer || g_newgameplusJKO->integer)
							{
								grip_dmg = floor(static_cast<float>(grip_dmg) / 0.5f);
							}
							else
							{
								grip_dmg = floor(static_cast<float>(grip_dmg) / 1.5f);
							}
						}
						G_Damage(gripEnt, self, self, dir, gripOrg, grip_dmg, DAMAGE_NO_ARMOR, MOD_CRUSH);
					}
					if (gripEnt->s.number)
					{
						if (self->client->ps.forcePowerLevel[FP_GRIP] > FORCE_LEVEL_2)
						{
							//do damage faster at level 3
							self->client->ps.forcePowerDebounce[FP_GRIP] = level.time + Q_irand(150, 750);
						}
						else
						{
							self->client->ps.forcePowerDebounce[FP_GRIP] = level.time + Q_irand(250, 1000);
						}
					}
					else
					{
						//player takes damage faster
						self->client->ps.forcePowerDebounce[FP_GRIP] = level.time + Q_irand(100, 600);
					}
					if (forceGripDamage[self->client->ps.forcePowerLevel[FP_GRIP]] > 0)
					{
						//no damage at level 1
						WP_ForcePowerDrain(self, FP_GRIP, 3);
					}
					if (self->client->NPC_class == CLASS_KYLE
						&& self->spawnflags & 1)
					{
						//"Boss" Kyle
						if (gripEnt->client)
						{
							if (!Q_irand(0, 2))
							{
								//toss him aside!
								vec3_t vRt;
								AngleVectors(self->currentAngles, nullptr, vRt, nullptr);
								//stop gripping
								TIMER_Set(self, "gripping", -level.time);
								WP_ForcePowerStop(self, FP_GRIP);
								//now toss him
								if (Q_irand(0, 1))
								{
									//throw him to my left
									NPC_SetAnim(self, SETANIM_BOTH, BOTH_TOSS1,
										SETANIM_FLAG_OVERRIDE | SETANIM_FLAG_HOLD);
									VectorScale(vRt, -1500.0f, gripEnt->client->ps.velocity);
									G_Knockdown(gripEnt, self, vRt, 500, qfalse);
								}
								else
								{
									//throw him to my right
									NPC_SetAnim(self, SETANIM_BOTH, BOTH_TOSS2,
										SETANIM_FLAG_OVERRIDE | SETANIM_FLAG_HOLD);
									VectorScale(vRt, 1500.0f, gripEnt->client->ps.velocity);
									G_Knockdown(gripEnt, self, vRt, 500, qfalse);
								}
								//don't do anything for a couple seconds
								self->client->ps.weaponTime = self->client->ps.torsoAnimTimer + 2000;
								self->painDebounceTime = level.time + self->client->ps.weaponTime;
								//stop moving
								VectorClear(self->client->ps.velocity);
								VectorClear(self->client->ps.moveDir);
								return;
							}
						}
					}
				}
				else
				{
					if (!gripEnt->enemy)
					{
						if (gripEnt->client
							&& gripEnt->client->playerTeam == TEAM_PLAYER
							&& self->s.number < MAX_CLIENTS
							&& self->client
							&& self->client->playerTeam == TEAM_PLAYER)
						{
							//this shouldn't make allies instantly turn on you, let the damage->pain routine determine how allies should react to this
						}
						else
						{
							G_SetEnemy(gripEnt, self);
						}
					}
				}
				if (gripEnt->client && gripEnt->health > 0)
				{
					// ReSharper disable once CppEntityAssignedButNoRead
					int anim = BOTH_CHOKE3; //left-handed choke
					if (gripEnt->client->ps.weapon == WP_NONE || gripEnt->client->ps.weapon == WP_MELEE)
					{
						if (gripEnt->health > 50)
						{
							anim = BOTH_CHOKE4; //two-handed choke
						}
						else if (gripEnt->health > 30)
						{
							anim = BOTH_CHOKE3; //two-handed choke
						}
						else
						{
							anim = BOTH_CHOKE1; //two-handed choke
						}
					}
					if (self->client->ps.forcePowerLevel[FP_GRIP] < FORCE_LEVEL_2)
					{
						//still on ground, only set anim on torso
						if (gripEnt->health > 50)
						{
							NPC_SetAnim(gripEnt, SETANIM_TORSO, BOTH_CHOKE3,
								SETANIM_FLAG_OVERRIDE | SETANIM_FLAG_HOLD);
						}
						else
						{
							NPC_SetAnim(gripEnt, SETANIM_TORSO, BOTH_CHOKE1,
								SETANIM_FLAG_OVERRIDE | SETANIM_FLAG_HOLD);
						}
					}
					else
					{
						//in air, set on whole body
						if (gripEnt->health > 50)
						{
							NPC_SetAnim(gripEnt, SETANIM_BOTH, BOTH_CHOKE4,
								SETANIM_FLAG_OVERRIDE | SETANIM_FLAG_HOLD);
						}
						else if (gripEnt->health > 30)
						{
							NPC_SetAnim(gripEnt, SETANIM_BOTH, BOTH_CHOKE3,
								SETANIM_FLAG_OVERRIDE | SETANIM_FLAG_HOLD);
						}
						else
						{
							NPC_SetAnim(gripEnt, SETANIM_BOTH, BOTH_CHOKE1,
								SETANIM_FLAG_OVERRIDE | SETANIM_FLAG_HOLD);
						}
					}
					gripEnt->painDebounceTime = level.time + 2000;
				}
			}
		}
	}
	break;
	case FP_LIGHTNING:
		if (self->client->ps.forcePowerLevel[FP_LIGHTNING] > FORCE_LEVEL_1)
		{
			//higher than level 1
			if (cmd->buttons & BUTTON_FORCE_LIGHTNING)
			{
				//holding it keeps it going
				self->client->ps.forcePowerDuration[FP_LIGHTNING] = level.time + 500;
				ForceLightningAnim(self);
			}
		}
		if (self->client->ps.forcePower < 25 || !WP_ForcePowerAvailable(self, forcePower, 0))
		{
			WP_ForcePowerStop(self, forcePower);
		}
		else
		{
			force_shoot_lightning(self);
			WP_ForcePowerDrain(self, forcePower, 0);
		}
		break;
		//new Jedi Academy force powers
	case FP_RAGE:
		if (self->health < 1)
		{
			WP_ForcePowerStop(self, forcePower);
			break;
		}
		if (self->client->ps.forceRageDrainTime < level.time)
		{
			int addTime = 400;

			self->health -= 2;

			if (self->client->ps.forcePowerLevel[FP_RAGE] == FORCE_LEVEL_1)
			{
				addTime = 100;
			}
			else if (self->client->ps.forcePowerLevel[FP_RAGE] == FORCE_LEVEL_2)
			{
				addTime = 250;
			}
			else if (self->client->ps.forcePowerLevel[FP_RAGE] == FORCE_LEVEL_3)
			{
				addTime = 500;
			}
			self->client->ps.forceRageDrainTime = level.time + addTime;
		}

		if (self->health < 1)
		{
			self->health = 1;
		}
		else
		{
			int soundduration = 0;
			self->client->ps.stats[STAT_HEALTH] = self->health;

			speed = forceSpeedValue[self->client->ps.forcePowerLevel[FP_RAGE] - 1];

			soundduration = ceil(FORCE_RAGE_DURATION * forceSpeedValue[self->client->ps.forcePowerLevel[FP_RAGE] - 1]);
			if (soundduration)
			{
				self->s.loopSound = G_SoundIndex("sound/weapons/force/rageloop.wav");
			}
		}
		break;
	case FP_DRAIN:
		if (cmd->buttons & BUTTON_FORCE_DRAIN)
		{
			//holding it keeps it going
			self->client->ps.forcePowerDuration[FP_DRAIN] = level.time + 500;
		}
		if (!WP_ForcePowerAvailable(self, forcePower, 0))
		{
			//no more force power, stop
			WP_ForcePowerStop(self, forcePower);
		}
		else if (self->client->ps.forceDrainEntityNum >= 0 && self->client->ps.forceDrainEntityNum < ENTITYNUM_WORLD)
		{
			gentity_t* drainEnt;
			//holding someone
			if (!WP_ForcePowerAvailable(self, FP_DRAIN, 0)
				|| self->client->ps.forcePowerLevel[FP_DRAIN] > FORCE_LEVEL_1
				&& !self->s.number
				&& !(cmd->buttons & BUTTON_FORCE_DRAIN)
				&& self->client->ps.forcePowerDuration[FP_DRAIN] < level.time)
			{
				WP_ForcePowerStop(self, FP_DRAIN);
				return;
			}
			drainEnt = &g_entities[self->client->ps.forceDrainEntityNum];

			if (!drainEnt)
			{
				//invalid ent
				WP_ForcePowerStop(self, FP_DRAIN);
				return;
			}
			if (drainEnt->health <= 0 && drainEnt->takedamage)
				//FIXME: what about things that never had health or lose takedamage when they die?
			{
				//dead ent
				WP_ForcePowerStop(self, FP_DRAIN);
				return;
			}
			if (drainEnt->client && drainEnt->client->moveType == MT_FLYSWIM && VectorLengthSquared(
				NPC->client->ps.velocity) > 300 * 300)
			{
				//flying creature broke free
				WP_ForcePowerStop(self, FP_DRAIN);
				return;
			}
			if (drainEnt->client
				&& drainEnt->health > 0 //dead dudes don't fly
				&& (drainEnt->client->NPC_class == CLASS_BOBAFETT || drainEnt->client->NPC_class == CLASS_MANDO ||
					drainEnt->client->NPC_class == CLASS_ROCKETTROOPER)
				&& self->client->ps.forcePowerDebounce[FP_DRAIN] < level.time
				&& !Q_irand(0, 10))
			{
				//boba fett - fly away!
				drainEnt->client->ps.forceJumpCharge = 0; //so we don't play the force flip anim
				drainEnt->client->ps.velocity[2] = 250;
				drainEnt->client->ps.forceJumpZStart = drainEnt->currentOrigin[2];
				//so we don't take damage if we land at same height
				drainEnt->client->ps.pm_flags |= PMF_JUMPING;
				G_AddEvent(drainEnt, EV_JUMP, 0);
				JET_FlyStart(drainEnt);
				WP_ForcePowerStop(self, FP_DRAIN);
				return;
			}
			if (drainEnt->NPC
				&& drainEnt->client
				&& drainEnt->client->ps.forcePowersKnown
				&& (drainEnt->client->NPC_class == CLASS_REBORN || drainEnt->client->ps.weapon == WP_SABER)
				&& !Jedi_CultistDestroyer(drainEnt)
				&& level.time - (self->client->ps.forcePowerDebounce[FP_DRAIN] > self->client->ps.forcePowerLevel[
					FP_DRAIN] * 500) //at level 1, I always get at least 500ms of drain, at level 3 I get 1500ms
				&& !Q_irand(0, 100 - drainEnt->NPC->stats.evasion * 8 - g_spskill->integer * 15))
			{
				//a jedi who broke free FIXME: maybe have some minimum grip length- a reaction time?
				WP_ForceForceThrow(drainEnt);
				//FIXME: I need to go into some pushed back anim...
				WP_ForcePowerStop(self, FP_DRAIN);
				//can't drain again for 2 seconds
				self->client->ps.forcePowerDebounce[FP_DRAIN] = level.time + 4000;
				return;
			}
					if (self->client->ps.torsoAnim != BOTH_FORCE_DRAIN_GRAB_START
						|| !self->client->ps.torsoAnimTimer)
					{
						NPC_SetAnim(self, SETANIM_BOTH, BOTH_FORCE_DRAIN_GRAB_HOLD, SETANIM_FLAG_OVERRIDE | SETANIM_FLAG_HOLD);
					}
					if (self->handLBolt != -1)
					{
						G_PlayEffect(G_EffectIndex("force/drain_hand"), self->playerModel, self->handLBolt, self->s.number,
							self->currentOrigin, 200, qtrue);
					}
					if (self->handRBolt != -1)
					{
						G_PlayEffect(G_EffectIndex("force/drain_hand"), self->playerModel, self->handRBolt, self->s.number,
							self->currentOrigin, 200, qtrue);
					}

					//how far are they
					dist = Distance(self->client->renderInfo.eyePoint, drainEnt->currentOrigin);
					if (DistanceSquared(drainEnt->currentOrigin, self->currentOrigin) > FORCE_DRAIN_DIST_SQUARED)
					{
						//must be close, got away somehow!
						WP_ForcePowerStop(self, FP_DRAIN);
						return;
					}

					//keep my saber off!
					WP_DeactivateSaber(self, qtrue);
					if (drainEnt->client)
					{
						//now move them
						VectorCopy(self->client->ps.viewangles, angles);
						angles[0] = 0;
						AngleVectors(angles, dir, nullptr, nullptr);
						//stop them from thinking
						drainEnt->client->ps.pm_time = 2000;
						drainEnt->client->ps.pm_flags |= PMF_TIME_KNOCKBACK;
						if (drainEnt->NPC)
						{
							if (!(drainEnt->NPC->aiFlags & NPCAI_DIE_ON_IMPACT))
							{
								//not falling to their death
								drainEnt->NPC->nextBStateThink = level.time + 2000;
							}
							vectoangles(dir, angles);
							drainEnt->NPC->desiredYaw = AngleNormalize180(angles[YAW] + 180);
							drainEnt->NPC->desiredPitch = -angles[PITCH];
							SaveNPCGlobals();
							SetNPCGlobals(drainEnt);
							NPC_UpdateAngles(qtrue, qtrue);
							drainEnt->NPC->last_ucmd.angles[0] = ucmd.angles[0];
							drainEnt->NPC->last_ucmd.angles[1] = ucmd.angles[1];
							drainEnt->NPC->last_ucmd.angles[2] = ucmd.angles[2];
							RestoreNPCGlobals();
							//FIXME: why does he turn back to his original angles once he dies or is let go?
						}
						else if (!drainEnt->s.number)
						{
							drainEnt->enemy = self;
							NPC_SetLookTarget(drainEnt, self->s.number, level.time + 1000);
						}

						drainEnt->client->ps.eFlags |= EF_FORCE_DRAINED;
						//dammit!  Make sure that saber stays off!

						if (!(drainEnt->client->ps.ManualBlockingFlags & 1 << HOLDINGBLOCK))
						{
							WP_DeactivateSaber(drainEnt, qtrue);
						}
						else
						{
							drainEnt->client->ps.ManualBlockingFlags &= ~(1 << HOLDINGBLOCK);
							drainEnt->client->ps.ManualBlockingFlags &= ~(1 << HOLDINGBLOCKANDATTACK);
							drainEnt->client->ps.ManualBlockingFlags &= ~(1 << PERFECTBLOCKING);
							drainEnt->client->ps.ManualBlockingFlags &= ~(1 << MBF_NPCBLOCKING);
							WP_DeactivateSaber(drainEnt, qtrue);
						}
					}
					//Shouldn't this be discovered?
					AddSightEvent(self, drainEnt->currentOrigin, 128, AEL_DISCOVERED, 20);

					if (self->client->ps.forcePowerDebounce[FP_DRAIN] < level.time)
					{
						int drainLevel = wp_absorb_conversion(drainEnt, drainEnt->client->ps.forcePowerLevel[FP_ABSORB], self,
							FP_DRAIN, self->client->ps.forcePowerLevel[FP_DRAIN],
							force_power_needed[self->client->ps.forcePowerLevel[FP_DRAIN]]);
						if (drainLevel && drainLevel == -1
							|| Q_irand(drainLevel, 3) < 3)
						{
							//the drain is being absorbed
							ForceDrainEnt(self, drainEnt);
						}
						WP_ForcePowerDrain(self, FP_DRAIN, 3);
					}
					else
					{
						if (!Q_irand(0, 4))
						{
							WP_ForcePowerDrain(self, FP_DRAIN, 1);
						}
						if (!drainEnt->enemy)
						{
							G_SetEnemy(drainEnt, self);
						}
					}
					if (drainEnt->health > 0)
					{
						//still alive
						NPC_SetAnim(drainEnt, SETANIM_BOTH, BOTH_FORCE_DRAIN_GRABBED,
							SETANIM_FLAG_OVERRIDE | SETANIM_FLAG_HOLD);
					}
		}
		else if (self->client->ps.forcePowerLevel[forcePower] > FORCE_LEVEL_1)
		{
			//regular distance-drain
			if (cmd->buttons & BUTTON_FORCE_DRAIN)
			{
				//holding it keeps it going
				self->client->ps.forcePowerDuration[FP_DRAIN] = level.time + 500;
				if (self->client->ps.torsoAnim == BOTH_FORCE_DRAIN_START)
				{
					if (!self->client->ps.torsoAnimTimer)
					{
						NPC_SetAnim(self, SETANIM_TORSO, BOTH_FORCE_DRAIN_HOLD,
							SETANIM_FLAG_OVERRIDE | SETANIM_FLAG_HOLD);
					}
					else
					{
						NPC_SetAnim(self, SETANIM_TORSO, BOTH_FORCE_DRAIN_START,
							SETANIM_FLAG_OVERRIDE | SETANIM_FLAG_HOLD);
					}
				}
				else
				{
					NPC_SetAnim(self, SETANIM_TORSO, BOTH_FORCE_DRAIN_HOLD, SETANIM_FLAG_OVERRIDE | SETANIM_FLAG_HOLD);
				}
			}
			if (!WP_ForcePowerAvailable(self, forcePower, 0))
			{
				WP_ForcePowerStop(self, forcePower);
			}
			else
			{
				ForceShootDrain(self);
			}
		}
		break;
	case FP_PROTECT:
		break;
	case FP_ABSORB:
		break;
	case FP_SEE:
		break;
	case FP_STASIS:
		break;
	case FP_DESTRUCTION:
		break;
	case FP_GRASP:
	{
		if (!WP_ForcePowerAvailable(self, FP_GRASP, 0)
			|| self->client->ps.forcePowerLevel[FP_GRASP] > FORCE_LEVEL_1 && !self->s.number && !(cmd->buttons &
				BUTTON_FORCEGRASP))
		{
			WP_ForcePowerStop(self, FP_GRASP);
			return;
		}
		if (self->client->ps.forceGripEntityNum >= 0 && self->client->ps.forceGripEntityNum < ENTITYNUM_WORLD)
		{
			gripEnt = &g_entities[self->client->ps.forceGripEntityNum];

			if (!gripEnt || !gripEnt->inuse)
			{
				//invalid or freed ent
				WP_ForcePowerStop(self, FP_GRASP);
				return;
			}
			//rww - RAGDOLL_BEGIN
#ifndef JK2_RAGDOLL_GRIPNOHEALTH
//rww - RAGDOLL_END
			if (gripEnt->health <= 0 && gripEnt->takedamage)//FIXME: what about things that never had health or lose takedamage when they die?
			{//either invalid ent, or dead ent
				WP_ForcePowerStop(self, FP_GRASP);
				return;
			}
			else
				//rww - RAGDOLL_BEGIN
#endif
				//rww - RAGDOLL_END
				if (self->client->ps.forcePowerLevel[FP_GRASP] == FORCE_LEVEL_1
					&& gripEnt->client
					&& gripEnt->client->ps.groundEntityNum == ENTITYNUM_NONE
					&& gripEnt->client->moveType != MT_FLYSWIM)
				{
					WP_ForcePowerStop(self, FP_GRASP);
					return;
				}
			if (gripEnt->client && gripEnt->client->moveType == MT_FLYSWIM && VectorLengthSquared(
				gripEnt->client->ps.velocity) > 300 * 300)
			{
				//flying creature broke free
				WP_ForcePowerStop(self, FP_GRASP);
				return;
			}
			if (gripEnt->client
				&& gripEnt->health > 0 //dead dudes don't fly
				&& (gripEnt->client->NPC_class == CLASS_BOBAFETT || gripEnt->client->NPC_class == CLASS_MANDO ||
					gripEnt->client->NPC_class == CLASS_ROCKETTROOPER)
				&& self->client->ps.forcePowerDebounce[FP_GRASP] < level.time
				&& !Q_irand(0, 3)
				)
			{
				//boba fett - fly away!
				gripEnt->client->ps.forceJumpCharge = 0; //so we don't play the force flip anim
				gripEnt->client->ps.velocity[2] = 250;
				gripEnt->client->ps.forceJumpZStart = gripEnt->currentOrigin[2];
				//so we don't take damage if we land at same height
				gripEnt->client->ps.pm_flags |= PMF_JUMPING;
				G_AddEvent(gripEnt, EV_JUMP, 0);
				JET_FlyStart(gripEnt);
				WP_ForcePowerStop(self, FP_GRASP);
				return;
			}
			if (gripEnt->NPC
				&& gripEnt->client
				&& gripEnt->client->ps.forcePowersKnown
				&& (gripEnt->client->NPC_class == CLASS_REBORN || gripEnt->client->ps.weapon == WP_SABER)
				&& !Jedi_CultistDestroyer(gripEnt)
				&& !Q_irand(0, 100 - gripEnt->NPC->stats.evasion * 8 - g_spskill->integer * 20))
			{
				//a jedi who broke free FIXME: maybe have some minimum grip length- a reaction time?
				WP_ForceForceThrow(gripEnt);
				//FIXME: I need to go into some pushed back anim...
				WP_ForcePowerStop(self, FP_GRASP);
				return;
			}
			if (PM_SaberInAttack(self->client->ps.saberMove)
				|| PM_SaberInStart(self->client->ps.saberMove))
			{
				//started an attack
				WP_ForcePowerStop(self, FP_GRASP);
				return;
			}
			int gripLevel = self->client->ps.forcePowerLevel[FP_GRASP];
			if (gripEnt->client)
			{
				gripLevel = wp_absorb_conversion(gripEnt, gripEnt->client->ps.forcePowerLevel[FP_ABSORB], self,
					FP_GRASP, self->client->ps.forcePowerLevel[FP_GRASP],
					force_power_needed[gripLevel]);
			}
			if (!gripLevel)
			{
				WP_ForcePowerStop(self, forcePower);
				return;
			}

			if (self->client->ps.forcePowerLevel[FP_GRASP] > FORCE_LEVEL_1)
			{
				//holding it
				NPC_SetAnim(self, SETANIM_TORSO, BOTH_FORCEGRIP_HOLD, SETANIM_FLAG_OVERRIDE | SETANIM_FLAG_HOLD);
				if (self->client->ps.torsoAnimTimer < 100)
				{
					//we were already playing this anim, we didn't want to restart it, but we want to hold it for at least 100ms.
					self->client->ps.torsoAnimTimer = 100;
				}
			}
			//get their org
			VectorCopy(self->client->ps.viewangles, angles);
			angles[0] -= 10;
			AngleVectors(angles, dir, nullptr, nullptr);
			if (gripEnt->client)
			{
				//move
				VectorCopy(gripEnt->client->renderInfo.headPoint, gripEntOrg);
			}
			else
			{
				VectorCopy(gripEnt->currentOrigin, gripEntOrg);
			}

			//how far are they
			dist = Distance(self->client->renderInfo.handLPoint, gripEntOrg);
			if (self->client->ps.forcePowerLevel[FP_GRASP] == FORCE_LEVEL_2 &&
				(!in_front(gripEntOrg, self->client->renderInfo.handLPoint, self->client->ps.viewangles, 0.3f) ||
					DistanceSquared(gripEntOrg, self->client->renderInfo.handLPoint) > FORCE_GRIP_DIST_SQUARED))
			{
				//must face them
				WP_ForcePowerStop(self, FP_GRASP);
				return;
			}

			//check for lift or carry
			if (self->client->ps.forcePowerLevel[FP_GRASP] > FORCE_LEVEL_2
				&& (!gripEnt->client || !gripEnt->message && !(gripEnt->flags & FL_NO_KNOCKBACK)))
			{
				//carry
				//cap dist
				if (dist > FORCE_GRIP_3_MAX_DIST)
				{
					dist = FORCE_GRIP_3_MAX_DIST;
				}
				else if (dist < FORCE_GRIP_3_MIN_DIST)
				{
					dist = FORCE_GRIP_3_MIN_DIST;
				}
				VectorMA(self->client->renderInfo.handLPoint, dist, dir, gripOrg);
			}
			else if (self->client->ps.forcePowerLevel[FP_GRASP] > FORCE_LEVEL_1)
			{
				//just lift
				VectorCopy(self->client->ps.forceGripOrg, gripOrg);
			}
			else
			{
				VectorCopy(gripEnt->currentOrigin, gripOrg);
			}
			if (self->client->ps.forcePowerLevel[FP_GRASP] > FORCE_LEVEL_1)
			{
				//if holding him, make sure there's a clear LOS between my hand and him
				trace_t gripTrace;
				gi.trace(&gripTrace, self->client->renderInfo.handLPoint, nullptr, nullptr, gripEntOrg,
					ENTITYNUM_NONE, MASK_FORCE_PUSH, static_cast<EG2_Collision>(0), 0);
				if (gripTrace.startsolid
					|| gripTrace.allsolid
					|| gripTrace.fraction < 1.0f)
				{
					//no clear trace, drop them
					WP_ForcePowerStop(self, FP_GRASP);
					return;
				}
			}
			//now move them
			if (gripEnt->client)
			{
				if (self->client->ps.forcePowerLevel[FP_GRASP] > FORCE_LEVEL_1)
				{
					//level 1 just holds them
					VectorSubtract(gripOrg, gripEntOrg, gripEnt->client->ps.velocity);
					if (self->client->ps.forcePowerLevel[FP_GRASP] > FORCE_LEVEL_2
						&& (!gripEnt->client || !gripEnt->message && !(gripEnt->flags & FL_NO_KNOCKBACK)))
					{
						//level 2 just lifts them
						float gripDist = VectorNormalize(gripEnt->client->ps.velocity) / 3.0f;
						if (gripDist < 20.0f)
						{
							if (gripDist < 2.0f)
							{
								VectorClear(gripEnt->client->ps.velocity);
							}
							else
							{
								VectorScale(gripEnt->client->ps.velocity, gripDist * gripDist,
									gripEnt->client->ps.velocity);
							}
						}
						else
						{
							VectorScale(gripEnt->client->ps.velocity, gripDist * gripDist,
								gripEnt->client->ps.velocity);
						}
					}
				}
				//stop them from thinking
				gripEnt->client->ps.pm_time = 2000;
				gripEnt->client->ps.pm_flags |= PMF_TIME_KNOCKBACK;
				if (gripEnt->NPC)
				{
					if (!(gripEnt->NPC->aiFlags & NPCAI_DIE_ON_IMPACT))
					{
						//not falling to their death
						gripEnt->NPC->nextBStateThink = level.time + 2000;
					}
					if (self->client->ps.forcePowerLevel[FP_GRASP] > FORCE_LEVEL_1)
					{
						//level 1 just holds them
						vectoangles(dir, angles);
						gripEnt->NPC->desiredYaw = AngleNormalize180(angles[YAW] + 180);
						gripEnt->NPC->desiredPitch = -angles[PITCH];
						SaveNPCGlobals();
						SetNPCGlobals(gripEnt);
						NPC_UpdateAngles(qtrue, qtrue);
						gripEnt->NPC->last_ucmd.angles[0] = ucmd.angles[0];
						gripEnt->NPC->last_ucmd.angles[1] = ucmd.angles[1];
						gripEnt->NPC->last_ucmd.angles[2] = ucmd.angles[2];
						RestoreNPCGlobals();
						//FIXME: why does he turn back to his original angles once he dies or is let go?
					}
				}
				else if (!gripEnt->s.number)
				{
					gripEnt->enemy = self;
					NPC_SetLookTarget(gripEnt, self->s.number, level.time + 1000);
				}

				gripEnt->client->ps.eFlags |= EF_FORCE_GRABBED;
				//dammit!  Make sure that saber stays off!
				WP_DeactivateSaber(gripEnt);
			}
			else
			{
				//move
				if (self->client->ps.forcePowerLevel[FP_GRASP] > FORCE_LEVEL_1)
				{
					//level 1 just holds them
					VectorCopy(gripEnt->currentOrigin, gripEnt->s.pos.trBase);
					VectorSubtract(gripOrg, gripEntOrg, gripEnt->s.pos.trDelta);
					if (self->client->ps.forcePowerLevel[FP_GRASP] > FORCE_LEVEL_2
						&& (!gripEnt->client || !gripEnt->message && !(gripEnt->flags & FL_NO_KNOCKBACK)))
					{
						//level 2 just lifts them
						VectorScale(gripEnt->s.pos.trDelta, 10, gripEnt->s.pos.trDelta);
					}
					gripEnt->s.pos.trType = TR_LINEAR;
					gripEnt->s.pos.trTime = level.time;
				}

				gripEnt->s.eFlags |= EF_FORCE_GRABBED;
			}

			//Shouldn't this be discovered?
			//AddSightEvent( self, gripOrg, 128, AEL_DANGER, 20 );
			AddSightEvent(self, gripOrg, 128, AEL_DISCOVERED, 20);

			if (self->client->ps.forcePowerDebounce[FP_GRASP] < level.time)
			{
				//GEntity_PainFunc( gripEnt, self, self, gripOrg, 0, MOD_CRUSH );
				if (!gripEnt->client
					|| gripEnt->client->NPC_class != CLASS_VEHICLE
					|| gripEnt->m_pVehicle
					&& gripEnt->m_pVehicle->m_pVehicleInfo
					&& gripEnt->m_pVehicle->m_pVehicleInfo->type == VH_ANIMAL)
				{
					//we don't damage the empty vehicle
					gripEnt->painDebounceTime = 0;
					int gripDmg = forceGraspDamage[self->client->ps.forcePowerLevel[FP_GRASP]];
					if (gripLevel != -1)
					{
						if (gripLevel == 1)
						{
							gripDmg = floor(static_cast<float>(gripDmg) / 3.0f);
						}
						// Grip does extra damage in newgameplus mode
						else if (g_newgameplusJKA->integer || g_newgameplusJKO->integer)
						{
							gripDmg = floor(static_cast<float>(gripDmg) / 0.5f);
						}
						else
						{
							gripDmg = floor(static_cast<float>(gripDmg) / 1.5f);
						}
					}
					G_Damage(gripEnt, self, self, dir, gripOrg, gripDmg, DAMAGE_NO_ARMOR, MOD_CRUSH); //MOD_???
				}
				if (gripEnt->s.number)
				{
					if (self->client->ps.forcePowerLevel[FP_GRASP] > FORCE_LEVEL_2)
					{
						//do damage faster at level 3
						self->client->ps.forcePowerDebounce[FP_GRASP] = level.time + Q_irand(150, 750);
					}
					else
					{
						self->client->ps.forcePowerDebounce[FP_GRASP] = level.time + Q_irand(250, 1000);
					}
				}
				else
				{
					//player takes damage faster
					self->client->ps.forcePowerDebounce[FP_GRASP] = level.time + Q_irand(100, 600);
				}
				if (forceGripDamage[self->client->ps.forcePowerLevel[FP_GRASP]] > 0)
				{
					//no damage at level 1
					WP_ForcePowerDrain(self, FP_GRASP, 3);
				}
				if (self->client->NPC_class == CLASS_KYLE
					&& self->spawnflags & 1)
				{
					//"Boss" Kyle
					if (gripEnt->client)
					{
						if (!Q_irand(0, 2))
						{
							//toss him aside!
							vec3_t vRt;
							AngleVectors(self->currentAngles, nullptr, vRt, nullptr);
							//stop gripping
							TIMER_Set(self, "grasping", -level.time);
							WP_ForcePowerStop(self, FP_GRASP);
							//now toss him
							if (Q_irand(0, 1))
							{
								//throw him to my left
								NPC_SetAnim(self, SETANIM_BOTH, BOTH_TOSS1,
									SETANIM_FLAG_OVERRIDE | SETANIM_FLAG_HOLD);
								VectorScale(vRt, -1500.0f, gripEnt->client->ps.velocity);
								G_Knockdown(gripEnt, self, vRt, 500, qfalse);
							}
							else
							{
								//throw him to my right
								NPC_SetAnim(self, SETANIM_BOTH, BOTH_TOSS2,
									SETANIM_FLAG_OVERRIDE | SETANIM_FLAG_HOLD);
								VectorScale(vRt, 1500.0f, gripEnt->client->ps.velocity);
								G_Knockdown(gripEnt, self, vRt, 500, qfalse);
							}
							//don't do anything for a couple seconds
							self->client->ps.weaponTime = self->client->ps.torsoAnimTimer + 2000;
							self->painDebounceTime = level.time + self->client->ps.weaponTime;
							//stop moving
							VectorClear(self->client->ps.velocity);
							VectorClear(self->client->ps.moveDir);
							return;
						}
					}
				}
			}
			else
			{
				//WP_ForcePowerDrain( self, FP_GRASP, 0 );
				if (!gripEnt->enemy)
				{
					if (gripEnt->client
						&& gripEnt->client->playerTeam == TEAM_PLAYER
						&& self->s.number < MAX_CLIENTS
						&& self->client
						&& self->client->playerTeam == TEAM_PLAYER)
					{
						//this shouldn't make allies instantly turn on you, let the damage->pain routine determine how allies should react to this
					}
					else
					{
						G_SetEnemy(gripEnt, self);
					}
				}
			}
			if (gripEnt->client && gripEnt->health > 0)
			{
				int anim = BOTH_SWIM_IDLE1; //left-handed choke
				if (gripEnt->client->ps.weapon == WP_NONE || gripEnt->client->ps.weapon == WP_MELEE)
				{
					anim = BOTH_PULLED_INAIR_F; //two-handed choke
				}
				if (self->client->ps.forcePowerLevel[FP_GRASP] < FORCE_LEVEL_2)
				{
					//still on ground, only set anim on torso
					NPC_SetAnim(gripEnt, SETANIM_TORSO, anim, SETANIM_FLAG_OVERRIDE | SETANIM_FLAG_HOLD);
				}
				else
				{
					//in air, set on whole body
					NPC_SetAnim(gripEnt, SETANIM_BOTH, anim, SETANIM_FLAG_OVERRIDE | SETANIM_FLAG_HOLD);
				}
				gripEnt->painDebounceTime = level.time + 2000;
			}
		}
	}
	break;
	case FP_REPULSE:
	{
		if (!self->s.number && !(cmd->buttons & BUTTON_REPULSE))
		{
			WP_ForcePowerStop(self, FP_REPULSE);
			return;
		}
		if (self->client->ps.repulseChargeStart && WP_ForcePowerAvailable(
			self, FP_REPULSE, force_power_needed[FP_REPULSE] + 30))
		{
			static qboolean registered = qfalse;

			if (self->client->ps.groundEntityNum == ENTITYNUM_NONE)
			{
				//still on ground, only set anim on torso
				NPC_SetAnim(self, SETANIM_TORSO, BOTH_FORCE_PROTECT_FAST,
					SETANIM_FLAG_OVERRIDE | SETANIM_FLAG_HOLD);
			}
			else
			{
				//in air, set on whole body
				NPC_SetAnim(self, SETANIM_BOTH, BOTH_FORCE_PROTECT_FAST, SETANIM_FLAG_OVERRIDE | SETANIM_FLAG_HOLD);
			}

			if (self->client->ps.torsoAnimTimer < 100)
			{
				//we were already playing this anim, we didn't want to restart it, but we want to hold it for at least 100ms.
				self->client->ps.torsoAnimTimer = 100;
			}
			if (self->client->ps.legsAnimTimer < 100)
			{
				//we were already playing this anim, we didn't want to restart it, but we want to hold it for at least 100ms.
				self->client->ps.legsAnimTimer = 100;
			}
			if (!registered)
			{
				repulseLoopSound = G_SoundIndex("sound/weapons/force/repulseloop.wav");
				registered = qtrue;
			}
			if (!Q_irand(0, 4))
			{
				WP_ForcePowerDrain(self, FP_REPULSE, 1);
			}

			self->s.loopSound = repulseLoopSound;
			VectorClear(self->client->ps.velocity);
			cmd->forwardmove = 0;
			cmd->rightmove = 0;
			cmd->upmove = 0;
			VectorClear(self->client->ps.moveDir);
		}
		else
		{
			WP_ForcePowerStop(self, FP_REPULSE);
		}
	}
	break;
	case FP_DEADLYSIGHT:
		if (self->client->ps.deadlySightLastChecked < level.time)
		{
			vec3_t forward, mins, maxs;
			int e, numListedEntities;
			gentity_t* entityList[MAX_GENTITIES];
			gentity_t* check = nullptr;
			trace_t tr;

			int addTime = 400;
			int radius = 1024;

			if (self->client->ps.forcePowerLevel[FP_DEADLYSIGHT] == FORCE_LEVEL_1)
			{
				addTime = 250;
				radius = 1024;
			}
			else if (self->client->ps.forcePowerLevel[FP_DEADLYSIGHT] == FORCE_LEVEL_2)
			{
				addTime = 150;
				radius = 1536;
			}
			else if (self->client->ps.forcePowerLevel[FP_DEADLYSIGHT] == FORCE_LEVEL_3)
			{
				addTime = 50;
				radius = 2048;
			}
			self->client->ps.deadlySightLastChecked = level.time + addTime;

			AngleVectors(self->client->ps.viewangles, forward, nullptr, nullptr);

			for (e = 0; e < 3; e++)
			{
				mins[e] = self->currentOrigin[e] - radius;
				maxs[e] = self->currentOrigin[e] + radius;
			}
			numListedEntities = gi.EntitiesInBox(mins, maxs, entityList, MAX_GENTITIES);

			for (e = 0; e < numListedEntities; e++)
			{
				float minDot = 0.5f;
				// ReSharper disable once CppEntityAssignedButNoRead
				float dist1;
				check = entityList[e];
				if (check == self)
				{
					//me
					continue;
				}
				if (!check->inuse)
				{
					//freed
					continue;
				}
				if (!check->client)
				{
					//not a client - FIXME: what about turrets?
					continue;
				}

				if (check->health <= 0)
				{
					//dead
					continue;
				}

				if (!gi.inPVS(check->currentOrigin, self->currentOrigin))
				{
					//can't potentially see them
					continue;
				}

				VectorSubtract(check->currentOrigin, self->currentOrigin, dir);
				dist1 = VectorNormalize(dir);

				if (DotProduct(dir, forward) < minDot)
				{
					//not in front
					continue;
				}

				//really should have a clear LOS to this thing...
				gi.trace(&tr, self->currentOrigin, vec3_origin, vec3_origin, check->currentOrigin, self->s.number,
					MASK_SHOT, static_cast<EG2_Collision>(0), 0);
				if (tr.fraction < 1.0f && tr.entityNum != check->s.number)
				{
					//must have clear shot
					continue;
				}

				if (self->client->ps.forcePowerLevel[FP_DEADLYSIGHT] == FORCE_LEVEL_1)
				{
					G_Damage(check, self, self, nullptr, check->client->renderInfo.headPoint, 2, DAMAGE_EXTRA_KNOCKBACK,
						MOD_BLASTER);
				}
				else if (self->client->ps.forcePowerLevel[FP_DEADLYSIGHT] == FORCE_LEVEL_2)
				{
					G_Damage(check, self, self, nullptr, check->client->renderInfo.headPoint, 5, DAMAGE_EXTRA_KNOCKBACK,
						MOD_BLASTER);
				}
				else if (self->client->ps.forcePowerLevel[FP_DEADLYSIGHT] == FORCE_LEVEL_3)
				{
					G_Damage(check, self, self, nullptr, check->client->renderInfo.headPoint, 10, DAMAGE_NO_KNOCKBACK,
						MOD_SNIPER);
				}

				if (check->ghoul2.size() && check->headBolt != -1)
				{
					G_PlayEffect(G_EffectIndex("volumetric/black_smoke"), check->playerModel, check->headBolt,
						check->s.number, check->currentOrigin, addTime, qtrue);
				}
			}
		}
		break;
	case FP_BLAST:
		break;
	case FP_INSANITY:
		break;
	case FP_BLINDING:
		break;
	default:
		break;
	}
}

void WP_CheckForcedPowers(gentity_t* self, usercmd_t* ucmd)
{
	for (int forcePower = FP_FIRST; forcePower < NUM_FORCE_POWERS; forcePower++)
	{
		if (self->client->ps.forcePowersForced & 1 << forcePower)
		{
			switch (forcePower)
			{
			case FP_HEAL:
				ForceHeal(self);
				//do only once
				self->client->ps.forcePowersForced &= ~(1 << forcePower);
				break;
			case FP_LEVITATION:
				//nothing
				break;
			case FP_SPEED:
				if (self->client->ps.communicatingflags & 1 << DASHING)
				{
					ForceSpeedDash(self);
				}
				else
				{
					ForceSpeed(self);
				}
				//do only once
				self->client->ps.forcePowersForced &= ~(1 << forcePower);
				break;
			case FP_PUSH:
				ForceThrow(self, qfalse);
				//do only once
				self->client->ps.forcePowersForced &= ~(1 << forcePower);
				break;
			case FP_PULL:
				ForceThrow(self, qtrue);
				//do only once
				self->client->ps.forcePowersForced &= ~(1 << forcePower);
				break;
			case FP_TELEPATHY:
				//FIXME: target at enemy?
				ForceTelepathy(self);
				//do only once
				self->client->ps.forcePowersForced &= ~(1 << forcePower);
				break;
			case FP_GRIP:
				ucmd->buttons &= ~(BUTTON_ATTACK | BUTTON_ALT_ATTACK | BUTTON_FORCE_FOCUS | BUTTON_FORCE_DRAIN |
					BUTTON_FORCE_LIGHTNING | BUTTON_REPULSE | BUTTON_FORCEGRASP);
				ucmd->buttons |= BUTTON_FORCEGRIP;
				//holds until cleared
				break;
			case FP_LIGHTNING:
				ucmd->buttons &= ~(BUTTON_ATTACK | BUTTON_ALT_ATTACK | BUTTON_FORCE_FOCUS | BUTTON_FORCEGRIP |
					BUTTON_FORCE_DRAIN | BUTTON_REPULSE | BUTTON_FORCEGRASP);
				ucmd->buttons |= BUTTON_FORCE_LIGHTNING;
				//holds until cleared
				break;
			case FP_SABERTHROW:
				ucmd->buttons &= ~(BUTTON_ATTACK | BUTTON_FORCE_FOCUS | BUTTON_FORCEGRIP | BUTTON_FORCE_DRAIN |
					BUTTON_FORCE_LIGHTNING | BUTTON_REPULSE | BUTTON_FORCEGRASP);
				ucmd->buttons |= BUTTON_ALT_ATTACK;
				//holds until cleared?
				break;
			case FP_SABER_DEFENSE:
				//nothing
				break;
			case FP_SABER_OFFENSE:
				//nothing
				break;
			case FP_RAGE:
				ForceRage(self);
				//do only once
				self->client->ps.forcePowersForced &= ~(1 << forcePower);
				break;
			case FP_PROTECT:
				ForceProtect(self);
				//do only once
				self->client->ps.forcePowersForced &= ~(1 << forcePower);
				break;
			case FP_ABSORB:
				ForceAbsorb(self);
				//do only once
				self->client->ps.forcePowersForced &= ~(1 << forcePower);
				break;
			case FP_DRAIN:
				ucmd->buttons &= ~(BUTTON_ATTACK | BUTTON_ALT_ATTACK | BUTTON_FORCE_FOCUS | BUTTON_FORCEGRIP |
					BUTTON_FORCE_LIGHTNING | BUTTON_FORCEGRASP | BUTTON_REPULSE);
				ucmd->buttons |= BUTTON_FORCE_DRAIN;
				//holds until cleared
				break;
			case FP_SEE:
				//nothing
				break;
			case FP_STASIS:
				force_stasis(self);
				//do only once
				self->client->ps.forcePowersForced &= ~(1 << forcePower);
				break;
			case FP_DESTRUCTION:
				ForceDestruction(self);
				//do only once
				self->client->ps.forcePowersForced &= ~(1 << forcePower);
				break;
			case FP_GRASP:
				ucmd->buttons &= ~(BUTTON_ATTACK | BUTTON_ALT_ATTACK | BUTTON_FORCE_FOCUS | BUTTON_FORCE_DRAIN |
					BUTTON_FORCE_LIGHTNING | BUTTON_FORCEGRIP | BUTTON_REPULSE);
				ucmd->buttons |= BUTTON_FORCEGRASP;
				//holds until cleared
				break;
			case FP_REPULSE:
				ucmd->buttons &= ~(BUTTON_ATTACK | BUTTON_ALT_ATTACK | BUTTON_FORCE_FOCUS | BUTTON_FORCEGRIP |
					BUTTON_FORCE_LIGHTNING | BUTTON_FORCEGRASP | BUTTON_FORCE_DRAIN);
				ucmd->buttons |= BUTTON_REPULSE;
				break;
			case FP_DEADLYSIGHT:
				ForceDeadlySight(self);
				self->client->ps.forcePowersForced &= ~(1 << forcePower);
				break;
			case FP_BLAST:
				ForceBlast(self);
				//do only once
				self->client->ps.forcePowersForced &= ~(1 << forcePower);
				break;
			case FP_INSANITY:
				ForceInsanity(self);
				//do only once
				self->client->ps.forcePowersForced &= ~(1 << forcePower);
				break;
			case FP_BLINDING:
				ForceBlinding(self);
				//do only once
				self->client->ps.forcePowersForced &= ~(1 << forcePower);
				break;
			default:;
			}
		}
	}
}

void WP_ForcePowersUpdate(gentity_t* self, usercmd_t* ucmd)
{
	qboolean usingForce = qfalse;
	int i;
	//see if any force powers are running
	if (!self)
	{
		return;
	}
	if (!self->client)
	{
		return;
	}

	if (self->health <= 0)
	{
		//if dead, deactivate any active force powers
		for (i = 0; i < NUM_FORCE_POWERS; i++)
		{
			if (self->client->ps.forcePowerDuration[i] || self->client->ps.forcePowersActive & 1 << i)
			{
				WP_ForcePowerStop(self, static_cast<forcePowers_t>(i));
				self->client->ps.forcePowerDuration[i] = 0;
			}
		}
		return;
	}

	WP_CheckForcedPowers(self, ucmd);

	if (self->client->ps.forcePower <= self->client->ps.forcePowerMax * FATIGUEDTHRESHHOLD)
	{
		//Pop the Fatigued flag
		self->client->ps.userInt3 |= 1 << FLAG_FATIGUED;
	}

	if (!self->s.number)
	{
		//player uses different kind of force-jump
	}
	else
	{
		if (self->client->ps.forceJumpCharge)
		{
			//let go of charge button, have charge
			//if leave the ground by some other means, cancel the force jump so we don't suddenly jump when we land.
			if (self->client->ps.groundEntityNum == ENTITYNUM_NONE
				&& !PM_SwimmingAnim(self->client->ps.legsAnim))
			{
				//FIXME: stop sound?
			}
			else
			{
				//still on ground, so jump
				ForceJump(self, ucmd);
				return;
			}
		}
	}

	if (ucmd->buttons & BUTTON_REPULSE)
	{
		ForceRepulse(self);
		self->client->ps.powerups[PW_INVINCIBLE] = level.time + self->client->ps.torsoAnimTimer + 2000;
	}

	if (ucmd->buttons & BUTTON_FORCEGRASP)
	{
		ForceGrasp(self);
	}

	if (!self->s.number
		&& self->client->NPC_class == CLASS_BOBAFETT || self->client->NPC_class == CLASS_MANDO)
	{
		//Boba Fett
		if (self->client->ps.weapon == WP_MELEE && ucmd->buttons & BUTTON_WALKING && ucmd->buttons & BUTTON_BLOCK)
		{
			//start wrist laser
			Boba_FireWristMissile(self, BOBA_MISSILE_LASER);
			return;
		}
		if (self->client->ps.forcePowerDuration[FP_GRIP])
		{
			Boba_EndWristMissile(self, BOBA_MISSILE_LASER);
			return;
		}
	}
	else if (ucmd->buttons & BUTTON_FORCEGRIP)
	{
		if (self->client->ps.forcePower < 90)
		{
			ForceGripBasic(self);
		}
		else
		{
			ForceGripAdvanced(self);
		}
	}

	if (!self->s.number
		&& self->client->NPC_class == CLASS_BOBAFETT || self->client->NPC_class == CLASS_MANDO)
	{
		//Boba Fett
		if (ucmd->buttons & BUTTON_FORCE_LIGHTNING)
		{
			//start flamethrower
			Mando_DoFlameThrower(self);
			return;
		}
		if (self->client->ps.forcePowerDuration[FP_LIGHTNING])
		{
			self->client->ps.forcePowerDuration[FP_LIGHTNING] = 0;
			Boba_StopFlameThrower(self);
			return;
		}
	}
	else if (ucmd->buttons & BUTTON_FORCE_LIGHTNING)
	{
		ForceLightning(self);
	}

	if (self->client->ps.communicatingflags & 1 << DASHING)
	{
		ForceSpeedDash(self);
	}

	if (!self->s.number
		&& self->client->NPC_class == CLASS_BOBAFETT || self->client->NPC_class == CLASS_MANDO)
	{
		//Boba Fett
		if (self->client->ps.weapon != WP_MELEE && ucmd->buttons & BUTTON_WALKING && ucmd->buttons & BUTTON_BLOCK)
		{
			//start wrist rocket
			Boba_FireWristMissile(self, BOBA_MISSILE_VIBROBLADE);
			return;
		}
		if (self->client->ps.forcePowerDuration[FP_DRAIN])
		{
			Boba_EndWristMissile(self, BOBA_MISSILE_VIBROBLADE);

			return;
		}
	}
	else if (ucmd->buttons & BUTTON_FORCE_DRAIN)
	{
		if (!ForceDrain2(self))
		{
			//can't drain-grip someone right in front
			if (self->client->ps.forcePowerLevel[FP_DRAIN] > FORCE_LEVEL_1)
			{
				//try ranged
				ForceDrain(self, qtrue);
			}
		}
	}

	for (i = 0; i < NUM_FORCE_POWERS; i++)
	{
		if (self->client->ps.forcePowerDuration[i])
		{
			if (self->client->ps.forcePowerDuration[i] < level.time)
			{
				if (self->client->ps.forcePowersActive & 1 << i)
				{
					//turn it off
					WP_ForcePowerStop(self, static_cast<forcePowers_t>(i));
				}
				self->client->ps.forcePowerDuration[i] = 0;
			}
		}
		if (self->client->ps.forcePowersActive & 1 << i)
		{
			usingForce = qtrue;
			wp_force_power_run(self, static_cast<forcePowers_t>(i), ucmd);
		}
	}
	if (self->client->ps.saberInFlight)
	{
		//don't regen force power while throwing saber
		if (self->client->ps.saberEntityNum < ENTITYNUM_NONE && self->client->ps.saberEntityNum > 0) //player is 0
		{
			//
			if (&g_entities[self->client->ps.saberEntityNum] != nullptr && g_entities[self->client->ps.saberEntityNum].s
				.pos.trType == TR_LINEAR)
			{
				//fell to the ground and we're trying to pull it back
				usingForce = qtrue;
			}
		}
	}
	if (PM_ForceUsingSaberAnim(self->client->ps.torsoAnim))
	{
		usingForce = qtrue;
	}

	const qboolean ActiveBlocking = self->client->ps.ManualBlockingFlags & 1 << HOLDINGBLOCKANDATTACK ? qtrue : qfalse;
	const qboolean HoldingBlock = self->client->ps.ManualBlockingFlags & 1 << HOLDINGBLOCK ? qtrue : qfalse;
	//Normal Blocking

	if (!usingForce
		&& !PM_InKnockDown(&self->client->ps)
		&& walk_check(self)
		&& self->client->ps.weaponTime <= 0
		&& self->client->ps.groundEntityNum != ENTITYNUM_NONE)
	{
		//when not using the force, regenerate at 10 points per second
		if (self->client->ps.forcePowerRegenDebounceTime < level.time)
		{
			wp_force_power_regenerate(self, self->client->ps.forcePowerRegenAmount);

			self->client->ps.forcePowerRegenDebounceTime = level.time + self->client->ps.forcePowerRegenRate;

			if (self->client->ps.forceRageRecoveryTime >= level.time)
			{
				//regen half as fast
				self->client->ps.forcePowerRegenDebounceTime += self->client->ps.forcePowerRegenRate;
			}
			else if (self->client->ps.saberInFlight)
			{
				//regen half as fast
				self->client->ps.forcePowerRegenDebounceTime += 2000; //1 point per 1 seconds.. super slow
			}
			else if (PM_SaberInAttack(self->client->ps.saberMove)
				|| pm_saber_in_special_attack(self->client->ps.torsoAnim)
				|| PM_SpinningSaberAnim(self->client->ps.torsoAnim)
				|| PM_SaberInParry(self->client->ps.saberMove)
				|| PM_SaberInReturn(self->client->ps.saberMove))
			{
				//regen half as fast
				self->client->ps.forcePowerRegenDebounceTime += 4000; //1 point per 1 seconds.. super slow
			}

			if (!(PM_StabAnim(self->client->ps.legsAnim) || PM_StabAnim(self->client->ps.torsoAnim)))
			{
				if (PM_RestAnim(self->client->ps.legsAnim))
				{
					wp_force_power_regenerate(self, 4);
					bg_reduce_saber_mishap_level(&self->client->ps);
					bg_reduce_blaster_mishap_level(&self->client->ps);
					self->client->ps.powerups[PW_MEDITATE] = level.time + self->client->ps.torsoAnimTimer + 3000;
					self->client->ps.eFlags |= EF_MEDITATING;
				}
				else if (PM_CrouchAnim(self->client->ps.legsAnim))
				{
					wp_force_power_regenerate(self, 2);
					bg_reduce_saber_mishap_level(&self->client->ps);
					bg_reduce_blaster_mishap_level(&self->client->ps);
				}
				else if (HoldingBlock || ActiveBlocking)
				{
					//regen half as fast
					self->client->ps.forcePowerRegenDebounceTime += 2000; //1 point per 1 seconds.. super slow
				}
				else if (self->client->ps.powerups[PW_CLOAKED])
				{
					//regen half as fast
					self->client->ps.forcePowerRegenDebounceTime += self->client->ps.forcePowerRegenRate;
				}
				else
				{
					self->client->ps.eFlags &= ~EF_MEDITATING;
				}
			}
			if (self->client->ps.forcePower > self->client->ps.forcePowerMax * FATIGUEDTHRESHHOLD)
			{
				//You gained some FP back.  Cancel the Fatigue status.
				self->client->ps.userInt3 &= ~(1 << FLAG_FATIGUED);
			}
		}
	}
}

void WP_BlockPointsUpdate(const gentity_t* self)
{
	const qboolean HoldingBlock = self->client->ps.ManualBlockingFlags & 1 << HOLDINGBLOCK ? qtrue : qfalse;
	//Normal Blocking

	if (self->s.number < MAX_CLIENTS || G_ControlledByPlayer(self))
	{
		if (!PM_InKnockDown(&self->client->ps)
			&& self->client->ps.groundEntityNum != ENTITYNUM_NONE
			&& walk_check(self)
			&& !PM_SaberInAttack(self->client->ps.saberMove)
			&& !pm_saber_in_special_attack(self->client->ps.torsoAnim)
			&& !PM_SpinningSaberAnim(self->client->ps.torsoAnim)
			&& !PM_SaberInParry(self->client->ps.saberMove)
			&& !PM_SaberInReturn(self->client->ps.saberMove)
			&& !PM_Saberinstab(self->client->ps.saberMove)
			&& !HoldingBlock
			&& !(self->client->buttons & BUTTON_BLOCK))
		{
			//when not using the block, regenerate at 10 points per second
			if (self->client->ps.BlockPointsRegenDebounceTime < level.time)
			{
				wp_block_points_regenerate(self, self->client->ps.BlockPointRegenAmount);

				self->client->ps.BlockPointsRegenDebounceTime = level.time + self->client->ps.BlockPointRegenRate;

				if (PM_RestAnim(self->client->ps.legsAnim))
				{
					wp_block_points_regenerate(self, 4);
					self->client->ps.powerups[PW_MEDITATE] = level.time + self->client->ps.torsoAnimTimer + 3000;
					self->client->ps.eFlags |= EF_MEDITATING;
				}
				else if (PM_CrouchAnim(self->client->ps.legsAnim))
				{
					wp_block_points_regenerate(self, 2);
				}
				else
				{
					if (self->client->ps.forceRageRecoveryTime >= level.time)
					{
						//regen half as fast
						self->client->ps.BlockPointsRegenDebounceTime += self->client->ps.BlockPointRegenRate;
					}
					else if (self->client->ps.blockPoints > BLOCKPOINTS_MISSILE) //slows down for the last 25 points
					{
						//regen half as fast
						self->client->ps.BlockPointsRegenDebounceTime += 2000;
					}
					else if (self->client->ps.saberInFlight) //slows down
					{
						//regen half as fast
						self->client->ps.BlockPointsRegenDebounceTime += 2000;
					}
					else if (self->client->ps.weaponTime <= 0) //slows down
					{
						//regen half as fast
						self->client->ps.BlockPointsRegenDebounceTime += 2000;
					}
					self->client->ps.eFlags &= ~EF_MEDITATING;
				}
			}
		}
	}
	else
	{
		//when not using the block, regenerate at 10 points per second
		if (!PM_InKnockDown(&self->client->ps)
			&& self->client->ps.groundEntityNum != ENTITYNUM_NONE
			&& walk_check(self)
			&& !PM_SaberInAttack(self->client->ps.saberMove)
			&& !pm_saber_in_special_attack(self->client->ps.torsoAnim)
			&& !PM_SpinningSaberAnim(self->client->ps.torsoAnim)
			&& !PM_SaberInParry(self->client->ps.saberMove)
			&& !PM_SaberInReturn(self->client->ps.saberMove)
			&& !PM_Saberinstab(self->client->ps.saberMove))
		{
			if (self->client->ps.BlockPointsRegenDebounceTime < level.time)
			{
				wp_block_points_regenerate(self, self->client->ps.BlockPointRegenAmount);

				self->client->ps.BlockPointsRegenDebounceTime = level.time + self->client->ps.BlockPointRegenRate;

				if (self->client->ps.forceRageRecoveryTime >= level.time)
				{
					//regen half as fast
					self->client->ps.BlockPointsRegenDebounceTime += self->client->ps.BlockPointRegenRate;
				}
				else if (self->client->ps.blockPoints > BLOCKPOINTS_MISSILE) //slows down for the last 25 points
				{
					//regen half as fast
					self->client->ps.BlockPointsRegenDebounceTime += 2000;
				}
				else if (self->client->ps.saberInFlight) //slows down
				{
					//regen half as fast
					self->client->ps.BlockPointsRegenDebounceTime += 2000;
				}
				else if (self->client->ps.weaponTime <= 0) //slows down
				{
					//regen half as fast
					self->client->ps.BlockPointsRegenDebounceTime += 2000;
				}
			}
		}
	}
}

void WP_InitForcePowers(const gentity_t* ent)
{
	if (!ent || !ent->client)
	{
		return;
	}

	if (!ent->client->ps.blockPointsMax)
	{
		ent->client->ps.blockPointsMax = BLOCK_POINTS_MAX;
	}

	if (!ent->client->ps.forcePowerMax)
	{
		ent->client->ps.forcePowerMax = FORCE_POWER_MAX;
	}
	if (!ent->client->ps.forcePowerRegenRate)
	{
		ent->client->ps.forcePowerRegenRate = 100;
	}
	if (!ent->client->ps.BlockPointRegenRate)
	{
		ent->client->ps.BlockPointRegenRate = 100;
	}
	ent->client->ps.blockPoints = ent->client->ps.blockPointsMax;
	ent->client->ps.forcePower = ent->client->ps.forcePowerMax;
	ent->client->ps.forcePowerRegenDebounceTime = level.time;
	ent->client->ps.BlockPointsRegenDebounceTime = level.time;

	ent->client->ps.forceGripEntityNum = ent->client->ps.forceDrainEntityNum = ent->client->ps.pullAttackEntNum =
		ENTITYNUM_NONE;
	ent->client->ps.forceRageRecoveryTime = 0;
	ent->client->ps.forceSpeedRecoveryTime = 0;
	ent->client->ps.forceDrainTime = 0;
	ent->client->ps.pullAttackTime = 0;

	if (ent->s.number < MAX_CLIENTS)
	{
		//player
		if (!g_cheats->integer) //devmaps give you all the FP
		{
			ent->client->ps.forcePowerLevel[FP_SABER_DEFENSE] = FORCE_LEVEL_3;
			ent->client->ps.forcePowerLevel[FP_SABER_OFFENSE] = FORCE_LEVEL_3;
		}
		else
		{
			ent->client->ps.forcePowersKnown = 1 << FP_HEAL | 1 << FP_LEVITATION | 1 << FP_SPEED | 1 << FP_PUSH | 1 <<
				FP_PULL | 1 << FP_TELEPATHY | 1 << FP_GRIP | 1 << FP_LIGHTNING | 1 << FP_SABERTHROW
				| 1 << FP_SABER_DEFENSE | 1 << FP_SABER_OFFENSE | 1 << FP_RAGE | 1 << FP_DRAIN | 1 << FP_PROTECT | 1 <<
				FP_ABSORB | 1 << FP_SEE | 1 << FP_STASIS | 1 << FP_DESTRUCTION | 1 << FP_GRASP
				| 1 << FP_REPULSE | 1 << FP_LIGHTNING_STRIKE | 1 << FP_FEAR | 1 << FP_DEADLYSIGHT | 1 << FP_BLAST | 1 <<
				FP_INSANITY | 1 << FP_BLINDING;

			ent->client->ps.forcePowerLevel[FP_HEAL] = FORCE_LEVEL_2;
			ent->client->ps.forcePowerLevel[FP_LEVITATION] = FORCE_LEVEL_2;
			ent->client->ps.forcePowerLevel[FP_PUSH] = FORCE_LEVEL_1;
			ent->client->ps.forcePowerLevel[FP_PULL] = FORCE_LEVEL_1;
			ent->client->ps.forcePowerLevel[FP_SABERTHROW] = FORCE_LEVEL_2;
			ent->client->ps.forcePowerLevel[FP_SPEED] = FORCE_LEVEL_1;
			ent->client->ps.forcePowerLevel[FP_LIGHTNING] = FORCE_LEVEL_1;
			ent->client->ps.forcePowerLevel[FP_TELEPATHY] = FORCE_LEVEL_2;

			ent->client->ps.forcePowerLevel[FP_RAGE] = FORCE_LEVEL_1;
			ent->client->ps.forcePowerLevel[FP_PROTECT] = FORCE_LEVEL_1;
			ent->client->ps.forcePowerLevel[FP_ABSORB] = FORCE_LEVEL_1;
			ent->client->ps.forcePowerLevel[FP_DRAIN] = FORCE_LEVEL_1;
			ent->client->ps.forcePowerLevel[FP_SEE] = FORCE_LEVEL_1;

			ent->client->ps.forcePowerLevel[FP_SABER_DEFENSE] = FORCE_LEVEL_3;
			ent->client->ps.forcePowerLevel[FP_SABER_OFFENSE] = FORCE_LEVEL_3;
			ent->client->ps.forcePowerLevel[FP_GRIP] = FORCE_LEVEL_2;

			ent->client->ps.forcePowerLevel[FP_STASIS] = FORCE_LEVEL_2;
			ent->client->ps.forcePowerLevel[FP_DESTRUCTION] = FORCE_LEVEL_2;

			ent->client->ps.forcePowerLevel[FP_GRASP] = FORCE_LEVEL_2;
			ent->client->ps.forcePowerLevel[FP_REPULSE] = FORCE_LEVEL_2;
			ent->client->ps.forcePowerLevel[FP_LIGHTNING_STRIKE] = FORCE_LEVEL_1;
			ent->client->ps.forcePowerLevel[FP_FEAR] = FORCE_LEVEL_1;
			ent->client->ps.forcePowerLevel[FP_DEADLYSIGHT] = FORCE_LEVEL_1;
			ent->client->ps.forcePowerLevel[FP_BLAST] = FORCE_LEVEL_1;
			ent->client->ps.forcePowerLevel[FP_INSANITY] = FORCE_LEVEL_1;
			ent->client->ps.forcePowerLevel[FP_BLINDING] = FORCE_LEVEL_1;
		}
	}
}

qboolean WP_DoingForcedAnimationForForcePowers(const gentity_t* self)
{
	if (!self->client)
	{
		return qfalse;
	}
	if (self->client->ps.legsAnim == BOTH_FORCE_ABSORB_START ||
		self->client->ps.legsAnim == BOTH_FORCE_ABSORB_END ||
		self->client->ps.legsAnim == BOTH_FORCE_ABSORB ||
		self->client->ps.torsoAnim == BOTH_FORCE_RAGE ||
		self->client->ps.legsAnim == BOTH_FORCE_PROTECT)
	{
		return qtrue;
	}
	return qfalse;
}

extern qboolean G_StandardHumanoid(gentity_t* self);

void G_SaberBounce(const gentity_t* attacker, gentity_t* victim)
{
	if (victim->health <= 20)
	{
		return;
	}

	if (PM_SaberInnonblockableAttack(attacker->client->ps.torsoAnim))
	{
		return;
	}

	if (attacker->client->ps.saberFatigueChainCount < MISHAPLEVEL_HUDFLASH)
	{
		return;
	}

	if (!G_StandardHumanoid(victim))
	{
		return;
	}

	if (attacker->s.number >= MAX_CLIENTS && !G_ControlledByPlayer(attacker))
	{
		return;
	}

	if (attacker->client->ps.saberBlocked == BLOCKED_NONE)
	{
		if (!pm_saber_in_special_attack(attacker->client->ps.torsoAnim))
		{
			if (SaberAttacking(attacker))
			{
				// Saber is in attack, use bounce for this attack.
				attacker->client->ps.saberMove = PM_SaberBounceForAttack(attacker->client->ps.saberMove);
				attacker->client->ps.saberBlocked = BLOCKED_BOUNCE_MOVE;
			}
			else
			{
				// Saber is in defense, use defensive bounce.
				attacker->client->ps.saberBlocked = BLOCKED_ATK_BOUNCE;
			}
		}
	}
}

void AnimateStun(gentity_t* self, gentity_t* inflictor, const vec3_t impact)
{
	//place self into a stunned state.
	if (self->client->ps.weapon != WP_SABER)
	{
		//knock them down instead
		G_Knockdown(self, inflictor, vec3_origin, 50, qtrue);
	}
	else if (!PM_SaberInBrokenParry(self->client->ps.saberMove) && !PM_InKnockDown(&self->client->ps))
	{
		if (!PM_SaberInParry(G_GetParryForBlock(self->client->ps.saberBlocked)))
		{
			//not already in a parry position, get one
			WP_SaberParryNonRandom(self, saberHitLocation, qfalse);
		}

		self->client->ps.saberMove = PM_BrokenParryForParry(G_GetParryForBlock(self->client->ps.saberBlocked));
		self->client->ps.saberBlocked = BLOCKED_PARRY_BROKEN;

		//make pain noise
		if (npc_is_dark_jedi(self))
		{
			// Do taunt/anger...
			const int CallOut = Q_irand(0, 3);

			switch (CallOut)
			{
			case 0:
			default:
				G_AddVoiceEvent(self, Q_irand(EV_TAUNT1, EV_TAUNT3), 3000);
				break;
			case 1:
				G_AddVoiceEvent(self, Q_irand(EV_ANGER1, EV_ANGER1), 3000);
				break;
			case 2:
				G_AddVoiceEvent(self, Q_irand(EV_COMBAT1, EV_COMBAT3), 3000);
				break;
			case 3:
				G_AddVoiceEvent(self, Q_irand(EV_JCHASE1, EV_JCHASE3), 3000);
				break;
			}
		}
		else if (npc_is_light_jedi(self))
		{
			// Do taunt...
			const int CallOut = Q_irand(0, 3);

			switch (CallOut)
			{
			case 0:
			default:
				G_AddVoiceEvent(self, Q_irand(EV_TAUNT1, EV_TAUNT3), 3000);
				break;
			case 1:
				G_AddVoiceEvent(self, Q_irand(EV_ANGER1, EV_ANGER1), 3000);
				break;
			case 2:
				G_AddVoiceEvent(self, Q_irand(EV_COMBAT1, EV_COMBAT3), 3000);
				break;
			case 3:
				G_AddVoiceEvent(self, Q_irand(EV_JCHASE1, EV_JCHASE3), 3000);
				break;
			}
		}
		else
		{
			// Do taunt/anger...
			const int CallOut = Q_irand(0, 3);

			switch (CallOut)
			{
			case 0:
			default:
				G_AddVoiceEvent(self, Q_irand(EV_TAUNT1, EV_TAUNT3), 3000);
				break;
			case 1:
				G_AddVoiceEvent(self, Q_irand(EV_ANGER1, EV_ANGER1), 3000);
				break;
			case 2:
				G_AddVoiceEvent(self, Q_irand(EV_COMBAT1, EV_COMBAT3), 3000);
				break;
			case 3:
				G_AddVoiceEvent(self, Q_irand(EV_JCHASE1, EV_JCHASE3), 3000);
				break;
			}
		}
	}
}

void player_StopFreeze(const gentity_t* self)
{
	if (self && self->client)
	{
		if (self->client->ps.PlayerEffectFlags & 1 << PEF_FREEZING)
		{
			//UnFreeze
			self->client->ps.PlayerEffectFlags &= ~(1 << PEF_FREEZING);
		}
		else
		{
			//UnFreeze
			self->client->ps.PlayerEffectFlags &= ~(1 << PEF_FREEZING);
		}
	}
}

void Player_CheckFreeze(const gentity_t* self)
{
	if (self && self->client)
	{
		if (self->health <= 0 || self->painDebounceTime < level.time)
		{
			player_StopFreeze(self);
		}
		else if (PM_InRoll(&self->client->ps))
		{
			player_StopFreeze(self);
		}
		else if (TIMER_Done(self, "FreezeDebounce"))
		{
			player_StopFreeze(self);
		}
		else
		{
			player_StopFreeze(self);
		}
	}
}

void player_Freeze(const gentity_t* self)
{
	if ((self->s.number < MAX_CLIENTS || G_ControlledByPlayer(self)) && self && self->client)
	{
		if (!(self->client->ps.PlayerEffectFlags & 1 << PEF_FREEZING))
		{
			self->client->ps.PlayerEffectFlags |= 1 << PEF_FREEZING;
		}
		else
		{
			player_StopFreeze(self);
		}
	}

	TIMER_Set(self, "FreezeDebounce", 1000);
}

qboolean BG_SaberInPartialDamageMove(gentity_t* self)
{
	//The player is attacking with a saber attack that does NO damage AT THIS POINT
	if (self->client->ps.torsoAnim == BOTH_JUMPFLIPSTABDOWN ||
		self->client->ps.torsoAnim == BOTH_JUMPFLIPSLASHDOWN1 ||
		self->client->ps.torsoAnim == BOTH_ROLL_STAB ||
		self->client->ps.torsoAnim == BOTH_STABDOWN ||
		self->client->ps.torsoAnim == BOTH_STABDOWN_STAFF ||
		self->client->ps.torsoAnim == BOTH_STABDOWN_DUAL)
	{
		float current = 0.0f;
		int end = 0;
		int start = 0;

		if (!!gi.G2API_GetBoneAnimIndex(&self->ghoul2[self->playerModel],
			self->lowerLumbarBone,
			level.time,
			&current,
			&start,
			&end,
			nullptr,
			nullptr,
			nullptr))
		{
			const float percentComplete = (current - start) / (end - start);

			if (g_IsSaberDoingAttackDamage->integer || g_DebugSaberCombat->integer)
			{
				gi.Printf("%f\n", percentComplete);
			}

			switch (self->client->ps.torsoAnim)
			{
			case BOTH_JUMPFLIPSTABDOWN: return static_cast<qboolean>(percentComplete > 0.0f && percentComplete < 0.30f);
			case BOTH_JUMPFLIPSLASHDOWN1: return static_cast<qboolean>(percentComplete > 0.0f && percentComplete <
				0.30f);
			case BOTH_ROLL_STAB: return static_cast<qboolean>(percentComplete > 0.0f && percentComplete < 0.30f);
			case BOTH_STABDOWN: return static_cast<qboolean>(percentComplete > 0.0f && percentComplete < 0.35f);
			case BOTH_STABDOWN_STAFF: return static_cast<qboolean>(percentComplete > 0.0f && percentComplete < 0.35f);
			case BOTH_STABDOWN_DUAL: return static_cast<qboolean>(percentComplete > 0.0f && percentComplete < 0.35f);
			default:;
			}
		}
	}

	return qfalse;
}