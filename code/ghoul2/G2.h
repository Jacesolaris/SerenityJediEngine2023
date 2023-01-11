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

#pragma once
#if !defined(G2_H_INC)
#define G2_H_INC

class CMiniHeap;

// defines to setup the
constexpr auto ENTITY_WIDTH = 12;
constexpr auto MODEL_WIDTH = 10;
constexpr auto BOLT_WIDTH = 10;

#define		MODEL_AND	((1<<MODEL_WIDTH)-1)
#define		BOLT_AND	((1<<BOLT_WIDTH)-1)
#define		ENTITY_AND	((1<<ENTITY_WIDTH)-1)

constexpr auto BOLT_SHIFT = 0;
#define		MODEL_SHIFT	(BOLT_SHIFT + BOLT_WIDTH)
#define		ENTITY_SHIFT (MODEL_SHIFT + MODEL_WIDTH)

//rww - RAGDOLL_BEGIN
class CRagDollUpdateParams;
//rww - RAGDOLL_END

struct model_s;
// internal surface calls  G2_surfaces.cpp
qboolean G2_SetSurfaceOnOff(CGhoul2Info* ghl_info, const char* surfaceName, int offFlags);
qboolean G2_SetRootSurface(CGhoul2Info_v& ghoul2, int model_index, const char* surfaceName);
int G2_AddSurface(CGhoul2Info* ghoul2, int surfaceNumber, int polyNumber, float BarycentricI, float BarycentricJ,
                  int lod);
qboolean G2_RemoveSurface(surfaceInfo_v& slist, int index);
const surfaceInfo_t* G2_FindOverrideSurface(int surfaceNum, const surfaceInfo_v& surfaceList);
int G2_IsSurfaceLegal(const model_s*, const char* surfaceName, uint32_t* flags);
int G2_GetParentSurface(CGhoul2Info* ghl_info, int index);
int G2_GetSurfaceIndex(CGhoul2Info* ghl_info, const char* surfaceName);
int G2_IsSurfaceRendered(CGhoul2Info* ghl_info, const char* surfaceName, surfaceInfo_v& slist);

// internal bone calls - G2_Bones.cpp
qboolean G2_Set_Bone_Angles(const CGhoul2Info* ghl_info, boneInfo_v& blist, const char* bone_name, const float* angles,
                            int flags,
                            Eorientations up, Eorientations left, Eorientations forward,
                            int blend_time, int current_time, const vec3_t offset);
qboolean G2_Remove_Bone(const CGhoul2Info* ghl_info, boneInfo_v& blist, const char* bone_name);
qboolean G2_Remove_Bone_Index(boneInfo_v& blist, int index);
qboolean G2_Set_Bone_Anim(const CGhoul2Info* ghl_info, boneInfo_v& blist, const char* bone_name, int start_frame,
                          int end_frame, int flags, float anim_speed, int current_time, float set_frame, int blend_time);
qboolean G2_Get_Bone_Anim(const CGhoul2Info* ghl_info, boneInfo_v& blist, const char* bone_name, int current_time,
                          float* current_frame, int* start_frame, int* end_frame, int* flags, float* ret_anim_speed);
qboolean G2_Get_Bone_Anim_Range(const CGhoul2Info* ghl_info, const boneInfo_v& blist, const char* boneName,
                                int* start_frame,
                                int* end_frame);
qboolean G2_Get_Bone_Anim_Range_Index(const boneInfo_v& blist, int bone_index, int* start_frame, int* end_frame);
qboolean G2_Pause_Bone_Anim(const CGhoul2Info* ghl_info, boneInfo_v& blist, const char* bone_name, int current_time);
qboolean G2_Pause_Bone_Anim_Index(boneInfo_v& blist, int bone_index, int current_time, int num_frames);
qboolean G2_IsPaused(const CGhoul2Info* ghl_info, const boneInfo_v& blist, const char* bone_name);
qboolean G2_Stop_Bone_Anim(const CGhoul2Info* ghl_info, boneInfo_v& blist, const char* bone_name);
qboolean G2_Stop_Bone_Angles(const CGhoul2Info* ghl_info, boneInfo_v& blist, const char* bone_name);
//rww - RAGDOLL_BEGIN
void G2_Animate_Bone_List(CGhoul2Info_v& ghoul2, int current_time, int index, CRagDollUpdateParams* params);
//rww - RAGDOLL_END

void G2_Init_Bone_List(boneInfo_v& blist, int numBones);
int G2_Find_Bone_In_List(const boneInfo_v& blist, int bone_num);
qboolean G2_Set_Bone_Angles_Matrix(const CGhoul2Info* ghl_info, boneInfo_v& blist, const char* bone_name,
                                   const mdxaBone_t& matrix,
                                   int flags, int blend_time, int current_time);
int G2_Get_Bone_Index(CGhoul2Info* ghoul2, const char* boneName, qboolean bAddIfNotFound);
qboolean G2_Set_Bone_Angles_Index(CGhoul2Info* ghl_info, boneInfo_v& blist, int index,
                                  const float* angles, int flags, Eorientations yaw,
                                  Eorientations pitch, Eorientations roll,
                                  int blend_time, int current_time, const vec3_t offset);
qboolean G2_Set_Bone_Angles_Matrix_Index(boneInfo_v& blist, int index,
                                         const mdxaBone_t& matrix, int flags,
                                         int blend_time, int current_time);
qboolean G2_Stop_Bone_Anim_Index(boneInfo_v& blist, int index);
qboolean G2_Stop_Bone_Angles_Index(boneInfo_v& blist, int index);
qboolean G2_Set_Bone_Anim_Index(boneInfo_v& blist, int index, int start_frame,
                                int end_frame, int flags, float anim_speed, int current_time, float set_frame,
                                int ablend_time, int num_frames);
qboolean G2_Get_Bone_Anim_Index(boneInfo_v& blist, int index, int current_time,
                                float* retcurrent_frame, int* start_frame, int* end_frame, int* flags, float* ret_anim_speed,
                                int num_frames);

// misc functions G2_misc.cpp
void G2_List_Model_Surfaces(const char* fileName);
void G2_List_Model_Bones(const char* fileName, int frame);
qboolean G2_GetAnimFileName(const char* fileName, char** filename);
#ifdef _G2_GORE
void G2_TraceModels(CGhoul2Info_v& ghoul2, vec3_t rayStart, vec3_t rayEnd, CCollisionRecord* collRecMap, int entNum,
                    EG2_Collision e_g2_trace_type, int use_lod, float fRadius, float ssize, float tsize, float theta,
                    int shader, SSkinGoreData* gore, qboolean skipIfLODNotMatch);
#else
void		G2_TraceModels(CGhoul2Info_v& ghoul2, vec3_t rayStart, vec3_t rayEnd, CCollisionRecord* collRecMap, int entNum, EG2_Collision e_g2_trace_type, int use_lod, float fRadius);
#endif
void TransformAndTranslatePoint(const vec3_t in, vec3_t out, const mdxaBone_t* mat);
#ifdef _G2_GORE
void G2_TransformModel(CGhoul2Info_v& ghoul2, int frameNum, vec3_t scale, CMiniHeap* G2VertSpace, int use_lod,
                       bool ApplyGore, const SSkinGoreData* gore = nullptr);
#else
void		G2_TransformModel(CGhoul2Info_v& ghoul2, const int frameNum, vec3_t scale, CMiniHeap* G2VertSpace, int use_lod);
#endif
void G2_GenerateWorldMatrix(const vec3_t angles, const vec3_t origin);
void TransformPoint(const vec3_t in, vec3_t out, const mdxaBone_t* mat);
void Inverse_Matrix(const mdxaBone_t* src, mdxaBone_t* dest);
void* G2_FindSurface(const model_s*, int index, int lod);
void G2_SaveGhoul2Models(CGhoul2Info_v& ghoul2);
void G2_LoadGhoul2Model(CGhoul2Info_v& ghoul2, const char* buffer);

// internal bolt calls. G2_bolts.cpp
int G2_Add_Bolt(const CGhoul2Info* ghl_info, boltInfo_v& bltlist, surfaceInfo_v& slist, const char* boneName);
qboolean G2_Remove_Bolt(boltInfo_v& bltlist, int index);
void G2_Init_Bolt_List(boltInfo_v& bltlist);
int G2_Find_Bolt_Bone_Num(const boltInfo_v& bltlist, int boneNum);
int G2_Find_Bolt_Surface_Num(const boltInfo_v& bltlist, int surfaceNum, int flags);
int G2_Add_Bolt_Surf_Num(const CGhoul2Info* ghl_info, boltInfo_v& bltlist, const surfaceInfo_v& slist, int surfNum);

// API calls - G2_API.cpp
void RestoreGhoul2InfoArray();
void SaveGhoul2InfoArray();

qhandle_t G2API_PrecacheGhoul2Model(const char* fileName);

int G2API_InitGhoul2Model(CGhoul2Info_v& ghoul2, const char* fileName, int model_index,
                          qhandle_t customSkin = NULL_HANDLE,
                          qhandle_t customShader = NULL_HANDLE, int modelFlags = 0, int lodBias = 0);
qboolean G2API_SetLodBias(CGhoul2Info* ghl_info, int lodBias);
qboolean G2API_SetSkin(CGhoul2Info* ghl_info, qhandle_t customSkin, qhandle_t renderSkin = 0);
qboolean G2API_SetShader(CGhoul2Info* ghl_info, qhandle_t customShader);
qboolean G2API_RemoveGhoul2Model(CGhoul2Info_v& ghl_info, int model_index);
qboolean G2API_SetSurfaceOnOff(CGhoul2Info* ghl_info, const char* surfaceName, int flags);
qboolean G2API_SetRootSurface(CGhoul2Info_v& ghl_info, int model_index, const char* surfaceName);
qboolean G2API_RemoveSurface(CGhoul2Info* ghl_info, int index);
int G2API_AddSurface(CGhoul2Info* ghl_info, int surfaceNumber, int polyNumber, float BarycentricI, float BarycentricJ,
                     int lod);
qboolean G2API_SetBoneAnim(CGhoul2Info* ghl_info, const char* boneName, int start_frame, int end_frame,
                           int flags, float animSpeed, int current_time, float setFrame = -1, int blend_time = -1);
qboolean G2API_GetBoneAnim(CGhoul2Info* ghl_info, const char* boneName, int current_time, float* current_frame,
                           int* start_frame, int* end_frame, int* flags, float* animSpeed, qhandle_t* modelList);
qboolean G2API_GetBoneAnimIndex(CGhoul2Info* ghl_info, int iBoneIndex, int current_time, float* current_frame,
                                int* start_frame, int* end_frame, int* flags, float* animSpeed, qhandle_t* modelList);
qboolean G2API_GetAnimRange(CGhoul2Info* ghl_info, const char* boneName, int* start_frame, int* end_frame);
qboolean G2API_GetAnimRangeIndex(CGhoul2Info* ghl_info, int bone_index, int* start_frame, int* end_frame);
qboolean G2API_PauseBoneAnim(CGhoul2Info* ghl_info, const char* boneName, int current_time);
qboolean G2API_PauseBoneAnimIndex(CGhoul2Info* ghl_info, int bone_index, int current_time);
qboolean G2API_IsPaused(CGhoul2Info* ghl_info, const char* boneName);
qboolean G2API_StopBoneAnim(CGhoul2Info* ghl_info, const char* boneName);
qboolean G2API_SetBoneAngles(CGhoul2Info* ghl_info, const char* boneName, const vec3_t angles, int flags,
                             Eorientations up, Eorientations right, Eorientations forward, qhandle_t* modelList,
                             int blend_time = 0, int current_time = 0);
qboolean G2API_SetBoneAnglesOffset(CGhoul2Info* ghl_info, const char* boneName, const vec3_t angles, int flags,
                                   Eorientations up, Eorientations right, Eorientations forward, qhandle_t* modelList,
                                   int blend_time = 0, int current_time = 0, const vec3_t offset = nullptr);
qboolean G2API_StopBoneAngles(CGhoul2Info* ghl_info, const char* boneName);
qboolean G2API_RemoveBone(CGhoul2Info* ghl_info, const char* boneName);
qboolean G2API_RemoveBolt(CGhoul2Info* ghl_info, int index);
int G2API_AddBolt(CGhoul2Info* ghl_info, const char* boneName);
int G2API_AddBoltSurfNum(CGhoul2Info* ghl_info, int surfIndex);
qboolean G2API_AttachG2Model(CGhoul2Info* ghl_info, CGhoul2Info* ghlInfoTo, int toBoltIndex, int toModel);
qboolean G2API_DetachG2Model(CGhoul2Info* ghl_info);
qboolean G2API_AttachEnt(int* boltInfo, CGhoul2Info* ghlInfoTo, int toBoltIndex, int entNum, int toModelNum);
void G2API_DetachEnt(int* boltInfo);

qboolean G2API_GetBoltMatrix(CGhoul2Info_v& ghoul2, int model_index, int bolt_index, mdxaBone_t* matrix,
                             const vec3_t angles, const vec3_t position, int frameNum, qhandle_t* modelList,
                             const vec3_t scale);

void G2API_ListSurfaces(CGhoul2Info* ghl_info);
void G2API_ListBones(CGhoul2Info* ghl_info, int frame);
qboolean G2API_HaveWeGhoul2Models(CGhoul2Info_v& ghoul2);
void G2API_SetGhoul2ModelIndexes(CGhoul2Info_v& ghoul2, qhandle_t* modelList, qhandle_t* skinList);
qboolean G2API_SetGhoul2ModelFlags(CGhoul2Info* ghl_info, int flags);
int G2API_GetGhoul2ModelFlags(CGhoul2Info* ghl_info);

qboolean G2API_GetAnimFileName(CGhoul2Info* ghl_info, char** filename);
void G2API_CollisionDetect(CCollisionRecord* collRecMap, CGhoul2Info_v& ghoul2, const vec3_t angles,
                           const vec3_t position,
                           int frameNumber, int entNum, vec3_t rayStart, vec3_t rayEnd, vec3_t scale,
                           CMiniHeap* G2VertSpace,
                           EG2_Collision e_g2_trace_type, int use_lod, float fRadius);
void G2API_GiveMeVectorFromMatrix(mdxaBone_t& bolt_matrix, Eorientations flags, vec3_t& vec);
void G2API_CopyGhoul2Instance(CGhoul2Info_v& Ghoul2From, CGhoul2Info_v& Ghoul2To, int model_index = -1);
void G2API_CleanGhoul2Models(CGhoul2Info_v& ghoul2);
int G2API_GetParentSurface(CGhoul2Info* ghl_info, int index);
int G2API_GetSurfaceIndex(CGhoul2Info* ghl_info, const char* surfaceName);
char* G2API_GetSurfaceName(CGhoul2Info* ghl_info, int surfNumber);
char* G2API_GetGLAName(CGhoul2Info* ghl_info);
qboolean G2API_SetBoneAnglesMatrix(CGhoul2Info* ghl_info, const char* boneName, const mdxaBone_t& matrix, int flags,
                                   qhandle_t* modelList, int blend_time = 0, int current_time = 0);
qboolean G2API_SetNewOrigin(CGhoul2Info* ghl_info, int bolt_index);
int G2API_GetBoneIndex(CGhoul2Info* ghl_info, const char* boneName, qboolean bAddIfNotFound);
qboolean G2API_StopBoneAnglesIndex(CGhoul2Info* ghl_info, int index);
qboolean G2API_StopBoneAnimIndex(CGhoul2Info* ghl_info, int index);
qboolean G2API_SetBoneAnglesIndex(CGhoul2Info* ghl_info, int index, const vec3_t angles, int flags,
                                  Eorientations yaw, Eorientations pitch, Eorientations roll,
                                  qhandle_t* modelList, int blend_time, int current_time);
qboolean G2API_SetBoneAnglesMatrixIndex(CGhoul2Info* ghl_info, int index, const mdxaBone_t& matrix,
                                        int flags, qhandle_t* modelList, int blend_time, int current_time);
qboolean G2API_SetBoneAnimIndex(CGhoul2Info* ghl_info, int index, int start_frame, int end_frame, int flags,
                                float animSpeed, int current_time, float setFrame, int blend_time);
qboolean G2API_SetAnimIndex(CGhoul2Info* ghl_info, int index);
int G2API_GetAnimIndex(CGhoul2Info* ghl_info);
void G2API_SaveGhoul2Models(CGhoul2Info_v& ghoul2);
void G2API_LoadGhoul2Models(CGhoul2Info_v& ghoul2, char* buffer);
void G2API_LoadSaveCodeDestructGhoul2Info(CGhoul2Info_v& ghoul2);
char* G2API_GetAnimFileNameIndex(qhandle_t model_index);
char* G2API_GetAnimFileInternalNameIndex(qhandle_t model_index);
int G2API_GetSurfaceRenderStatus(CGhoul2Info* ghl_info, const char* surfaceName);

// From tr_ghoul2.cpp
void G2_ConstructGhoulSkeleton(CGhoul2Info_v& ghoul2, int frameNum, bool checkForNewOrigin, const vec3_t scale);
void G2_GetBoltMatrixLow(CGhoul2Info& ghoul2, int boltNum, const vec3_t scale, mdxaBone_t& retMatrix);
void G2_TimingModel(boneInfo_t& bone, int time, int numFramesInFile, int& current_frame, int& newFrame, float& lerp);

bool G2_SetupModelPointers(CGhoul2Info_v& ghoul2); // returns true if any model is properly set up
bool G2_SetupModelPointers(CGhoul2Info* ghl_info); // returns true if the model is properly set up

//#ifdef _G2_GORE	// These exist regardless, non-gore versions are empty
void G2API_AddSkinGore(CGhoul2Info_v & ghoul2, SSkinGoreData & gore);
void G2API_ClearSkinGore(CGhoul2Info_v& ghoul2);
//#endif

#endif // G2_H_INC
