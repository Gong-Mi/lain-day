#ifndef STORY_PARSER_H
#define STORY_PARSER_H

#include "game_types.h"

// Loads a story scene from a Markdown file.
// The caller is responsible for ensuring the StoryScene struct is properly managed (e.g., initialized to zeros).
// Returns 1 on success, 0 on failure.
int load_story_scene(const char* story_file_path, StoryScene* scene);

#endif // STORY_PARSER_H
