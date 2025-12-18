#include "../include/systems/mystery_system.h"
#include "render_utils.h"
#include "linenoise.h"
#include "ansi_colors.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>

// --- Data Structures ---

#define MAX_KEYWORDS 10
#define MAX_COMBINATIONS 20
#define MAX_KEYWORD_LEN 32
#define MAX_TEXT_LEN 256

typedef struct {
    char id[MAX_KEYWORD_LEN];
    char display_name[MAX_KEYWORD_LEN];
    bool is_visible;
} MysteryKeyword;

typedef struct {
    char keyword1_id[MAX_KEYWORD_LEN];
    char keyword2_id[MAX_KEYWORD_LEN]; // Can be empty for single-keyword queries
    char question[MAX_TEXT_LEN];
    char answer[MAX_TEXT_LEN]; // "YES", "NO", "IRRELEVANT", or custom text
} MysteryCombination;

typedef struct {
    char question[MAX_TEXT_LEN];
    char options[3][MAX_TEXT_LEN];
    int correct_option_index; // 0, 1, or 2
} QuizQuestion;

typedef struct {
    char title[64];
    char story_text[1024];
    MysteryKeyword keywords[MAX_KEYWORDS];
    int keyword_count;
    MysteryCombination combinations[MAX_COMBINATIONS];
    int combination_count;
    QuizQuestion final_quiz[3]; // Fixed 3 questions for now
    int quiz_count;
} MysteryCase;

// --- State ---

static char selected_slot_1[MAX_KEYWORD_LEN] = {0};
static char selected_slot_2[MAX_KEYWORD_LEN] = {0};
static char last_response[MAX_TEXT_LEN] = {0};
static char last_question[MAX_TEXT_LEN] = {0};

// --- Content (Hardcoded for Prototype) ---

static MysteryCase g_case_01 = {
    .title = "The Barman's Gun",
    .story_text = "A man walks into a bar and asks for a glass of [water].\n"
                  "The [bartender] pulls out a [gun] and points it at the [man].\n"
                  "The man says '[thank_you]' and leaves.\n"
                  "Why?",
    .keywords = {
        {"man", "Man", true},
        {"water", "Water", true},
        {"bartender", "Bartender", true},
        {"gun", "Gun", true},
        {"thank_you", "Thank You", true},
        {"hiccups", "Hiccups", false} // Hidden keyword, revealed later?
    },
    .keyword_count = 6,
    .combinations = {
        // Simple 1-word queries (optional, but good for "What is this?")
        {"water", "", "Did he want water to drink?", "NO"},
        
        // Combinations
        {"man", "water", "Was the man thirsty?", "NO"},
        {"man", "gun", "Was the man afraid of the gun?", "YES (Initially)"},
        {"bartender", "gun", "Did the bartender want to kill the man?", "NO"},
        {"bartender", "water", "Did the bartender refuse to give water?", "NO (He helped in another way)"},
        {"gun", "water", "Was it a water gun?", "NO"},
        {"man", "thank_you", "Was the man grateful for the gun?", "YES"},
        {"gun", "thank_you", "Did the gun help the man?", "YES"},
        
        // The "Solution" combinations
        {"gun", "shock", "Did the gun shock the man?", "YES (It cured him)"}, // "shock" isn't a keyword yet, assume implicit logic or add it
    },
    .combination_count = 8,
    .final_quiz = {
        {
            "Why did the man ask for water?",
            {"He was thirsty", "He had hiccups", "He wanted to clean a stain"},
            1
        },
        {
            "Why did the bartender pull a gun?",
            {"To rob the man", "To scare the man", "To clean the gun"},
            1
        },
        {
            "Why did the man say thank you?",
            {"He likes guns", "He was suicidal", "His hiccups were cured"},
            2
        }
    },
    .quiz_count = 3
};

// --- Helper Functions ---

static void clear_slots() {
    memset(selected_slot_1, 0, sizeof(selected_slot_1));
    memset(selected_slot_2, 0, sizeof(selected_slot_2));
}

static MysteryKeyword* find_keyword(const char* name) {
    for (int i = 0; i < g_case_01.keyword_count; i++) {
        // Check both ID and Display Name (case insensitive)
        if (strcasecmp(g_case_01.keywords[i].id, name) == 0 || 
            strcasecmp(g_case_01.keywords[i].display_name, name) == 0) {
            return &g_case_01.keywords[i];
        }
    }
    return NULL;
}

static void check_combination() {
    if (strlen(selected_slot_1) == 0) return;

    // Normalize order: Alphabetical sort to ensure "man + water" == "water + man"
    char* k1 = selected_slot_1;
    char* k2 = selected_slot_2;
    if (strlen(k2) > 0 && strcmp(k1, k2) > 0) {
        char* temp = k1;
        k1 = k2;
        k2 = temp;
    }

    // Search combinations
    for (int i = 0; i < g_case_01.combination_count; i++) {
        MysteryCombination* comb = &g_case_01.combinations[i];
        
        bool match = false;
        if (strlen(k2) == 0) {
            // Single keyword match
            if (strcmp(comb->keyword1_id, k1) == 0 && strlen(comb->keyword2_id) == 0) match = true;
        } else {
            // Double keyword match
            // The database must store them in a consistent order or we check both ways.
            // Since we sorted k1/k2, we assume database is unordered or we check both.
            // Let's check both directions to be safe.
            if ((strcmp(comb->keyword1_id, k1) == 0 && strcmp(comb->keyword2_id, k2) == 0) ||
                (strcmp(comb->keyword1_id, k2) == 0 && strcmp(comb->keyword2_id, k1) == 0)) {
                match = true;
            }
        }

        if (match) {
            strncpy(last_question, comb->question, MAX_TEXT_LEN - 1);
            strncpy(last_response, comb->answer, MAX_TEXT_LEN - 1);
            
            // Special logic for revealing info (prototype hacking)
            if (strcmp(comb->answer, "YES (It cured him)") == 0) {
                // Unlock hint?
            }
            return;
        }
    }

    // No match found
    if (strlen(k2) > 0) {
        snprintf(last_question, MAX_TEXT_LEN, "Is there a connection between %s and %s?", k1, k2);
    } else {
        snprintf(last_question, MAX_TEXT_LEN, "Is %s relevant?", k1);
    }
    strncpy(last_response, "The connection is unclear... (Try a different pair)", MAX_TEXT_LEN - 1);
}

static void render_mystery_screen() {
    clear_screen();
    printf(ANSI_COLOR_CYAN "=== MYSTERY APP: CASE 001 ===\n" ANSI_COLOR_RESET);
    printf(ANSI_COLOR_YELLOW "%s\n\n" ANSI_COLOR_RESET, g_case_01.title);
    
    // Render Story with highlighted keywords
    // For prototype, just print raw text. A real version would parse [] and colorize.
    printf("%s\n", g_case_01.story_text);
    printf("----------------------------------------\n");
    
    // Render Keywords "Toolbar"
    printf("KEYWORDS: ");
    for (int i = 0; i < g_case_01.keyword_count; i++) {
        if (g_case_01.keywords[i].is_visible) {
            printf("[%s] ", g_case_01.keywords[i].display_name);
        }
    }
    printf("\n----------------------------------------\n");

    // Render "Thinking Slot"
    printf("THINKING: ");
    if (strlen(selected_slot_1) > 0) printf("[%s] ", selected_slot_1); 
    else printf("[   ] ");
    
    printf("+ ");
    
    if (strlen(selected_slot_2) > 0) printf("[%s] ", selected_slot_2); 
    else printf("[   ] ");
    
    printf("\n");
    printf("----------------------------------------\n");
    
    // Render Last Interaction
    if (strlen(last_question) > 0) {
        printf("Q: %s\n", last_question);
        printf("A: %s%s%s\n", ANSI_COLOR_GREEN, last_response, ANSI_COLOR_RESET);
    }
    
    printf("\nCommands: 'touch <word>', 'clear', 'solve', 'exit'\n");
}

static bool run_quiz() {
    clear_screen();
    printf(ANSI_COLOR_CYAN "=== FINAL DEDUCTION ===\n\n" ANSI_COLOR_RESET);
    
    for (int i = 0; i < g_case_01.quiz_count; i++) {
        QuizQuestion* q = &g_case_01.final_quiz[i];
        printf("Q%d: %s\n", i + 1, q->question);
        for (int j = 0; j < 3; j++) {
            printf("  %d) %s\n", j + 1, q->options[j]);
        }
        
        char buf[16];
        int choice = 0;
        do {
            printf("Choice (1-3): ");
            char* line = linenoise("");
            if (line) {
                choice = atoi(line);
                free(line);
            }
        } while (choice < 1 || choice > 3);
        
        if (choice - 1 != q->correct_option_index) {
            printf(ANSI_COLOR_RED "\nINCORRECT! The truth is still lost in the Wired...\n" ANSI_COLOR_RESET);
            usleep(2000000);
            return false;
        }
        printf(ANSI_COLOR_GREEN "CORRECT!\n\n" ANSI_COLOR_RESET);
    }
    
    return true;
}

// --- Main Loop ---

void enter_mystery_app(GameState* game_state) {
    char input[MAX_LINE_LENGTH];
    bool running = true;
    
    // Reset state
    clear_slots();
    memset(last_response, 0, sizeof(last_response));
    memset(last_question, 0, sizeof(last_question));
    
    while (running) {
        render_mystery_screen();
        
        char* line = linenoise("MYSTERY> ");
        if (line == NULL) break;
        strncpy(input, line, sizeof(input) - 1);
        free(line);
        
        if (strcmp(input, "exit") == 0 || strcmp(input, "quit") == 0) {
            running = false;
        } 
        else if (strcmp(input, "clear") == 0) {
            clear_slots();
            memset(last_response, 0, sizeof(last_response));
            memset(last_question, 0, sizeof(last_question));
        }
        else if (strcmp(input, "solve") == 0) {
            if (run_quiz()) {
                clear_screen();
                printf(ANSI_COLOR_GREEN "\n=== CASE SOLVED ===\n" ANSI_COLOR_RESET);
                printf("Congratulations, Lain. You have uncovered the truth.\n");
                printf("(Press ENTER to return)");
                (void)getchar();
                running = false;
            }
        }
        else if (strncmp(input, "touch ", 6) == 0) {
            char* word = input + 6;
            MysteryKeyword* kw = find_keyword(word);
            if (kw) {
                if (strlen(selected_slot_1) == 0) {
                    strncpy(selected_slot_1, kw->id, MAX_KEYWORD_LEN - 1);
                } else if (strlen(selected_slot_2) == 0) {
                    // Prevent adding same word twice? No, maybe useful for self-reference?
                    // Let's prevent for now to avoid confusion
                    if (strcmp(selected_slot_1, kw->id) != 0) {
                        strncpy(selected_slot_2, kw->id, MAX_KEYWORD_LEN - 1);
                        check_combination();
                        // Auto-clear after checking? Or let user clear?
                        // Let's auto-clear so they can try next pair immediately
                        // But wait, they need to see the result.
                        // We render result in next frame.
                        // We should probably NOT clear automatically so they see what they asked.
                        // But then they have to type 'clear'.
                        // Better UX: Wait for a keypress or "touch" again clears it?
                        // Let's keep it simple: It stays filled. If they 'touch' again, we shift?
                        // "FIFO": If full, clear slot 1, move slot 2 to 1, add new to 2.
                    } else {
                         snprintf(last_response, MAX_TEXT_LEN, "(You already selected that)");
                    }
                } else {
                    // Slots full, FIFO shift
                    strncpy(selected_slot_1, selected_slot_2, MAX_KEYWORD_LEN - 1);
                    strncpy(selected_slot_2, kw->id, MAX_KEYWORD_LEN - 1);
                    check_combination();
                }
            } else {
                snprintf(last_response, MAX_TEXT_LEN, "Keyword '%s' not found.", word);
            }
        }
        else {
             snprintf(last_response, MAX_TEXT_LEN, "Unknown command. Try 'touch <word>'");
        }
    }
}
