#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "characters/mika.h"

// Mock GameTime utils if needed, but since we are using pure logic function,
// we just need to pass raw integers.

const char* sanity_to_string(MikaSanityLevel level) {
    switch(level) {
        case MIKA_SANITY_NORMAL: return "NORMAL (0)";
        case MIKA_SANITY_IRRITATED: return "IRRITATED (1)";
        case MIKA_SANITY_PARANOID: return "PARANOID (2)";
        case MIKA_SANITY_BROKEN: return "BROKEN (3)";
        default: return "UNKNOWN";
    }
}

void print_schedule_for_day(MikaSanityLevel sanity) {
    printf("\n=== Schedule for Sanity Level: %s ===\n", sanity_to_string(sanity));
    printf("Time  | Location ID\n");
    printf("------+-----------------------------------\n");
    
    const char* last_loc = "";
    
    for (int hour = 0; hour < 24; hour++) {
        // Calculate units: Hour * 60 min * 60 sec * 16 units
        uint32_t units = hour * 3600 * 16; 
        
        const char* loc = mika_calculate_scheduled_location(units, sanity);
        
        if (loc == NULL) loc = "NULL";
        
        // Highlight changes
        char change_marker = ' ';
        if (strcmp(loc, last_loc) != 0 && hour != 0) {
            change_marker = '*';
        }
        
        printf("%02d:00 | %s %c\n", hour, loc, change_marker);
        last_loc = loc;
    }
}

int main(int argc, char* argv[]) {
    printf("Mika Schedule Logic Debugger\n");
    printf("----------------------------\n");
    
    if (argc > 1) {
        int sanity_input = atoi(argv[1]);
        if (sanity_input < 0 || sanity_input > 3) {
            printf("Invalid sanity level. Use 0-3.\n");
            return 1;
        }
        print_schedule_for_day((MikaSanityLevel)sanity_input);
    } else {
        // Print all schedules if no argument
        print_schedule_for_day(MIKA_SANITY_NORMAL);
        print_schedule_for_day(MIKA_SANITY_PARANOID);
        print_schedule_for_day(MIKA_SANITY_BROKEN);
    }
    
    return 0;
}
