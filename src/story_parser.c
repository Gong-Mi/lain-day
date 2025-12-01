#include "story_parser.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#define USE_DEBUG_LOGGING // FORCING DEBUG FOR DIAGNOSIS

// Helper function to trim whitespace from a string in-place
static void trim_whitespace(char *str) {
    if (str == NULL) return;

    char *start = str;
    // Find the first non-whitespace character
    while (isspace((unsigned char)*start)) {
        start++;
    }

    // Move the non-whitespace part to the beginning of the string
    if (str != start) {
        memmove(str, start, strlen(start) + 1);
    }

    // If string is all whitespace, it is now empty
    if (*str == '\0') {
        return;
    }
    
    // Trim trailing space
    char *end = str + strlen(str) - 1;
    while (end > str && isspace((unsigned char)*end)) {
        end--;
    }
    *(end + 1) = '\0';
}

int load_story_scene(const char* story_file_path, StoryScene* scene) {
    FILE *file = fopen(story_file_path, "r");
    if (!file) {
        return 0;
    }

    strncpy(scene->file_path, story_file_path, MAX_PATH_LENGTH - 1);
    scene->text_line_count = 0;
    scene->choice_count = 0;
    memset(&scene->front_matter, 0, sizeof(FrontMatter));

    char line[MAX_LINE_LENGTH];
    int in_front_matter = 0;
    int front_matter_delimiter_count = 0;

    while (fgets(line, sizeof(line), file)) {
        
        char *line_start = line;
        while(isspace((unsigned char)*line_start)) line_start++;

        // Remove trailing newline
        line_start[strcspn(line_start, "\n\r")] = 0;

#ifdef USE_DEBUG_LOGGING
        printf("DEBUG: Parser: Processing line: '%s'\n", line_start);
#endif

        if (strcmp(line_start, "---") == 0) {
            front_matter_delimiter_count++;
            if (front_matter_delimiter_count == 1) {
                in_front_matter = 1;
                continue;
            } else if (front_matter_delimiter_count == 2) {
                in_front_matter = 0;
                continue;
            }
        }

        if (in_front_matter) {
            if (strncmp(line_start, "location:", 9) == 0) {
                char *loc_id = line_start + 9;
                trim_whitespace(loc_id);
                strncpy(scene->front_matter.location_id, loc_id, MAX_NAME_LENGTH - 1);
            }
        } else {
            if (strncmp(line_start, "- [", 3) == 0 && scene->choice_count < MAX_CHOICES_PER_SCENE) {
                char *text_start = line_start + 3;
                char *text_end = strstr(text_start, "](");
                char *action_start = strstr(text_end ? text_end + 2 : NULL, "action:");
                char *action_id_end = strstr(action_start ? action_start + 7 : NULL, ")");

                if (text_end && action_start && action_id_end) {
#ifdef USE_DEBUG_LOGGING
                    fprintf(stderr, "DEBUG: Parser: Successfully identified choice line: '%s'\n", line_start);
                    fprintf(stderr, "DEBUG: Parser: text_end found, action_start found, action_id_end found.\n");
#endif
                    StoryChoice *choice = &scene->choices[scene->choice_count];

                    *text_end = '\0';
                    strncpy(choice->text, text_start, MAX_LINE_LENGTH - 1);

                    *action_id_end = '\0';
                    strncpy(choice->action_id, action_start + 7, MAX_NAME_LENGTH - 1);

                    scene->choice_count++;
                } else {
                    if (scene->text_line_count < MAX_TEXT_LINES_PER_SCENE) {
                        strncpy(scene->text_lines[scene->text_line_count], line_start, MAX_LINE_LENGTH - 1);
                        scene->text_line_count++;
                    }
                }
            } else {
                if (scene->text_line_count < MAX_TEXT_LINES_PER_SCENE) {
                    strncpy(scene->text_lines[scene->text_line_count], line_start, MAX_LINE_LENGTH - 1);
                    scene->text_line_count++;
                }
            }
        }
    }

    fclose(file);
    return 1;
}