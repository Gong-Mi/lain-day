#ifndef SCENE_MOM_DIALOGUE_H
#define SCENE_MOM_DIALOGUE_H

#include "game_types.h"

// Declaration for the initial conversation scene
void init_scene_02d_talk_to_mom_normal(StoryScene* scene);

// Declarations for the end-prologue scenes resulting from the mom conversation
void init_scene_02f_mom_reply_fine_endprologue(StoryScene* scene);
void init_scene_02g_mom_reply_silent_endprologue(StoryScene* scene);

// NOTE: Other related scene functions like init_scene_02e_talk_to_mom_vision
// can be added here later.

#endif // SCENE_MOM_DIALOGUE_H
