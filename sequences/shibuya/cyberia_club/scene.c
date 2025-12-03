#include "scene.h"
#include <string.h>
#include <stdio.h>
#include "conditions.h" // For potential future accessibility functions
#include "flag_system.h" // For potential future flag checks
#include "game_types.h" // For MAX_NAME_LENGTH, MAX_DESC_LENGTH etc.

// Helper function to safely add a connection to a location
static void add_connection(Location* loc, const char* action_id, const char* target_id, is_accessible_func is_accessible, const char* access_denied_scene_id) {
    if (loc == NULL || target_id == NULL || action_id == NULL) return;
    if (loc->connection_count < MAX_CONNECTIONS) {
        Connection* conn = &loc->connections[loc->connection_count];
        conn->target_location_id = target_id;
        conn->action_id = action_id;
        conn->is_accessible = is_accessible;
        conn->access_denied_scene_id = access_denied_scene_id;
        loc->connection_count++;
    } else {
        fprintf(stderr, "Warning: Max connections reached for location %s. Cannot add %s.\n", loc->id, target_id);
    }
}

// Helper function to safely add a POI to a location
static void add_poi(Location* loc, const char* poi_id, const char* poi_name, const char* poi_desc) {
    if (loc == NULL || poi_id == NULL) return;
    if (loc->pois_count < MAX_POIS) {
        POI* new_poi = &loc->pois[loc->pois_count];
        strncpy(new_poi->id, poi_id, MAX_NAME_LENGTH - 1);
        new_poi->id[MAX_NAME_LENGTH - 1] = '\0';
        strncpy(new_poi->name, poi_name, MAX_NAME_LENGTH - 1);
        new_poi->name[MAX_NAME_LENGTH - 1] = '\0';
        strncpy(new_poi->description, poi_desc, (MAX_DESC_LENGTH * 2) - 1); // MAX_DESC_LENGTH * 2 for POIs
        new_poi->description[(MAX_DESC_LENGTH * 2) - 1] = '\0';
        loc->pois_count++;
    } else {
        fprintf(stderr, "Warning: Max POIs reached for location %s. Cannot add %s.\n", loc->id, poi_id);
    }
}

int create_cyberia_club_layout(Location* all_locations, int starting_index) {
    if (all_locations == NULL || starting_index < 0) {
        return 0;
    }

    Location* cyberia_club = &all_locations[starting_index];
    *cyberia_club = (Location){0}; // Zero out the struct

    strcpy(cyberia_club->id, "cyberia_club");
    strcpy(cyberia_club->name, "Cyberia 酒吧");
    strcpy(cyberia_club->description, "重低音和闪烁的灯光，空气中弥漫着汗水和电子的味道。孩子们在这里寻找归属。");

    // Connections (from old_map/cyberia_club/connections.json - which was empty)
    // Add a default connection to Shibuya Street for now
    add_connection(cyberia_club, "exit_club", "shibuya_street", NULL, NULL);

    // POIs from old_map/cyberia_club/poi.json
    add_poi(cyberia_club, "dance_floor", "舞池", "舞池里挤满了人，随着音乐疯狂舞动。");
    add_poi(cyberia_club, "bar", "吧台", "吧台后面，调酒师正忙着调制各种颜色的液体。");
    add_poi(cyberia_club, "dj", "DJ", "一个戴着奇怪帽子的DJ，正在操作台前打碟。");
    add_poi(cyberia_club, "old_mic", "老式麦克风", "一个老式的麦克风，看起来有些年头了。"); // Action: examine_old_mic
    add_poi(cyberia_club, "detective_kids", "侦探团", "三个小学生组成的侦探团，正在角落里窃窃私语。");
    add_poi(cyberia_club, "restroom", "洗手间", "洗手间，门上贴着奇怪的涂鸦。");
    add_poi(cyberia_club, "boss", "酒吧老板", "酒吧老板，一个看起来很疲惫的中年人。"); // Sells: milk, coffee, juice
    add_poi(cyberia_club, "business_card", "名片", "一张写着几行网址的名片，看起来有些可疑。");
    add_poi(cyberia_club, "weird_youth", "诡异青年", "一个神色诡异的青年，独自坐在角落里。");
    
    return CYBERIA_CLUB_ROOM_COUNT;
}
