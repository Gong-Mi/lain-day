#include "scene.h"
#include <string.h>
#include <stdio.h>
#include "conditions.h"
#include "scenes.h" // For SCENE_MIKA_ROOM_LOCKED etc.
#include "flag_system.h" // For hash_table_get
#include "characters/mika.h"

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
static void add_poi(Location* loc, const char* poi_id, const char* poi_name, const char* poi_desc, const char* examine_scene_id) {
    if (loc == NULL || poi_id == NULL) return;
    if (loc->pois_count < MAX_POIS) {
        POI* new_poi = &loc->pois[loc->pois_count];
        strncpy(new_poi->id, poi_id, MAX_NAME_LENGTH - 1);
        new_poi->id[MAX_NAME_LENGTH - 1] = '\0';
        strncpy(new_poi->name, poi_name, MAX_NAME_LENGTH - 1);
        new_poi->name[MAX_NAME_LENGTH - 1] = '\0';
        strncpy(new_poi->description, poi_desc, (MAX_DESC_LENGTH * 2) - 1); // MAX_DESC_LENGTH * 2 for POIs
        new_poi->description[(MAX_DESC_LENGTH * 2) - 1] = '\0';
        new_poi->examine_scene_id = examine_scene_id;
        loc->pois_count++;
    } else {
        fprintf(stderr, "Warning: Max POIs reached for location %s. Cannot add %s.\n", loc->id, poi_id);
    }
}

int create_iwakura_house_layout(Location* all_locations, int starting_index) {
    if (all_locations == NULL || starting_index < 0) {
        return 0;
    }

    // Define the 8 rooms
    Location* front_yard = &all_locations[starting_index + 0];
    Location* lower_hallway = &all_locations[starting_index + 1];
    Location* living_dining_kitchen = &all_locations[starting_index + 2];
    Location* bathroom = &all_locations[starting_index + 3];
    Location* upper_hallway = &all_locations[starting_index + 4];
    Location* lains_room = &all_locations[starting_index + 5];
    Location* mikas_room = &all_locations[starting_index + 6]; // New room
    Location* study = &all_locations[starting_index + 7];      // New room

    // --- 1. Front Yard (前院) ---
    *front_yard = (Location){0}; // Zero out the struct
    strcpy(front_yard->id, "iwakura_front_yard");
    strcpy(front_yard->name, "前院");
    strcpy(front_yard->description, "岩仓家的前院。这是一栋需要走3次楼梯才能到达主楼的小独栋。从外面看，似乎没有自己的停车位，也许是为了更好的采光设计。没有什么特别的东西。");
    add_connection(front_yard, "enter_house", "iwakura_lower_hallway", NULL, NULL);
    add_poi(front_yard, "mailbox", "邮箱", "一个老旧的邮箱，里面可能有什么东西。", NULL);
    add_poi(front_yard, "doorbell", "门铃", "一个铜制的门铃，上面有些生锈。", NULL);

    // --- 2. Lower Hallway (下走廊) ---
    *lower_hallway = (Location){0};
    strcpy(lower_hallway->id, "iwakura_lower_hallway");
    strcpy(lower_hallway->name, "下走廊");
    strcpy(lower_hallway->description, "连接着房子一楼的各个房间。");
    add_connection(lower_hallway, "go_outside", "iwakura_front_yard", NULL, NULL);
    add_connection(lower_hallway, "enter_living_area", "iwakura_living_dining_kitchen", NULL, NULL);
    add_connection(lower_hallway, "enter_bathroom", "iwakura_bathroom", NULL, NULL);
    add_connection(lower_hallway, "go_upstairs", "iwakura_upper_hallway", NULL, NULL);
    add_connection(lower_hallway, "enter_study", "iwakura_study", NULL, NULL);
    add_poi(lower_hallway, "shoe_rack", "鞋柜", "一个放着家人鞋子的鞋柜，有些凌乱。", NULL);
    add_poi(lower_hallway, "telephone", "电话", "墙上挂着一部老式电话。", NULL);
    add_poi(lower_hallway, "umbrella_stand", "雨伞桶", "一个放着各式雨伞的桶，看上去家里的每个人都有一把。", NULL);

    // --- 3. Living-Dining-Kitchen (客厅-餐厅-厨房) ---
    *living_dining_kitchen = (Location){0};
    strcpy(living_dining_kitchen->id, "iwakura_living_dining_kitchen");
    strcpy(living_dining_kitchen->name, "客厅-餐厅-厨房");
    strcpy(living_dining_kitchen->description, "一个开放式的起居空间，包含了客厅、餐厅和厨房的功能。");
    add_connection(living_dining_kitchen, "go_to_hallway", "iwakura_lower_hallway", NULL, NULL);
    add_poi(living_dining_kitchen, "sofa", "沙发", "一套舒适的布艺沙发。", NULL);
    add_poi(living_dining_kitchen, "tv", "电视", "一台播放着新闻节目的电视机。", NULL);
    add_poi(living_dining_kitchen, "dining_table", "餐桌", "摆放着水果和杯子的餐桌。", NULL);
    add_poi(living_dining_kitchen, "refrigerator", "冰箱", "发出嗡嗡声的冰箱，里面有4盒未开封的1L牛奶，一些面包和罐头等食物。", "SCENE_EXAMINE_FRIDGE");

    // --- 4. Bathroom (浴室) ---
    *bathroom = (Location){0};
    strcpy(bathroom->id, "iwakura_bathroom");
    strcpy(bathroom->name, "浴室");
    strcpy(bathroom->description, "一个普通的浴室。");
    add_connection(bathroom, "go_to_hallway", "iwakura_lower_hallway", NULL, NULL);
    add_poi(bathroom, "sink", "洗手台", "干净的洗手台，上面放着牙刷。", NULL);
    add_poi(bathroom, "bathtub", "浴缸", "一个白色的浴缸，看起来很干净。", NULL);

    // --- 5. Upper Hallway (上走廊) ---
    *upper_hallway = (Location){0};
    strcpy(upper_hallway->id, "iwakura_upper_hallway");
    strcpy(upper_hallway->name, "上走廊");
    strcpy(upper_hallway->description, "连接着房子二楼的房间。");
    add_connection(upper_hallway, "go_downstairs", "iwakura_lower_hallway", NULL, NULL);
    add_connection(upper_hallway, "enter_lains_room", "iwakura_lains_room", NULL, NULL);
    add_connection(upper_hallway, "enter_mikas_room", "iwakura_mikas_room", get_mika_module()->is_room_accessible, "SCENE_MIKA_ROOM_LOCKED");
    add_poi(upper_hallway, "painting", "装饰画", "墙上挂着一幅抽象画。", NULL);

    // --- 6. Lain's Room (Lain的房间) ---
    *lains_room = (Location){0};
    strcpy(lains_room->id, "iwakura_lains_room");
    strcpy(lains_room->name, "Lain的房间");
    strcpy(lains_room->description, "你的房间。很暗，充满了电子设备运作的嗡嗡声。");
    add_connection(lains_room, "exit_room", "iwakura_upper_hallway", NULL, NULL);
    // POIs from original lain_room
    add_poi(lains_room, "navi_computer", "NAVI电脑", "一台巨大的Navi电脑，屏幕上闪烁着绿色的光标。", NULL);
    add_poi(lains_room, "bed", "床", "一张单人床，被子有些凌乱。", NULL);
    add_poi(lains_room, "window", "窗户", "一扇看向外面世界的窗户，外面是夜色。", NULL);
    add_poi(lains_room, "toy_dog", "玩具狗", "一个陈旧的玩具狗，坐在书桌一角。", NULL);
    add_poi(lains_room, "bookshelf", "书架", "一个堆满了各种书籍和杂志的书架。", NULL);

    // --- 7. Mika's Room (美香的房间) ---
    *mikas_room = (Location){0};
    strcpy(mikas_room->id, "iwakura_mikas_room");
    strcpy(mikas_room->name, "美香的房间");
    strcpy(mikas_room->description, "姐姐美香的房间。与你的房间相比，这里整洁得有些不真实。");
    add_connection(mikas_room, "exit_room", "iwakura_upper_hallway", NULL, NULL);
    add_poi(mikas_room, "desk", "书桌", "一张整洁的书桌，上面摆放着化妆品和教科书。", NULL);
    add_poi(mikas_room, "wardrobe", "衣柜", "一个高大的衣柜，里面是美香的衣服。", NULL);
    
    // --- 8. Study (书房) ---
    *study = (Location){0};
    strcpy(study->id, "iwakura_study");
    strcpy(study->name, "书房");
    strcpy(study->description, "父亲的书房。空气中弥漫着旧书和淡淡的烟草味。");
    add_connection(study, "go_to_hallway", "iwakura_lower_hallway", NULL, NULL);
    add_poi(study, "bookshelf", "书架", "一个巨大的书架，塞满了各种专业书籍。", NULL);
    add_poi(study, "desk", "办公桌", "一张堆满了文件和电脑的办公桌。", NULL);
    
    return IWAKURA_HOUSE_ROOM_COUNT;
}
