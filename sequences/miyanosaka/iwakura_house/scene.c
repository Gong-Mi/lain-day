#include "scene.h"
#include <string.h>
#include <stdio.h>

// Helper function to safely add a connection to a location
static void add_connection(Location* loc, const char* connection_id) {
    if (loc == NULL || connection_id == NULL) return;
    if (loc->connection_count < MAX_CONNECTIONS) {
        strncpy(loc->connections[loc->connection_count], connection_id, MAX_NAME_LENGTH - 1);
        loc->connections[loc->connection_count][MAX_NAME_LENGTH - 1] = '\0'; // Ensure null-termination
        loc->connection_count++;
    } else {
        // In a real game, you might log an error here
        fprintf(stderr, "Warning: Max connections reached for location %s. Cannot add %s.\n", loc->id, connection_id);
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
    strcpy(front_yard->description, "岩仓家的前院。很普通，没有什么特别的东西。");
    add_connection(front_yard, "iwakura_lower_hallway:enter_house");
    add_poi(front_yard, "mailbox", "邮箱", "一个老旧的邮箱，里面可能有什么东西。");
    add_poi(front_yard, "doorbell", "门铃", "一个铜制的门铃，上面有些生锈。");

    // --- 2. Lower Hallway (下走廊) ---
    *lower_hallway = (Location){0};
    strcpy(lower_hallway->id, "iwakura_lower_hallway");
    strcpy(lower_hallway->name, "下走廊");
    strcpy(lower_hallway->description, "连接着房子一楼的各个房间。");
    add_connection(lower_hallway, "iwakura_front_yard:go_outside");
    add_connection(lower_hallway, "iwakura_living_dining_kitchen:enter_living_area");
    add_connection(lower_hallway, "iwakura_bathroom:enter_bathroom");
    add_connection(lower_hallway, "iwakura_upper_hallway:go_upstairs");
    add_connection(lower_hallway, "iwakura_study:enter_study"); // Added connection to study
    add_poi(lower_hallway, "shoe_rack", "鞋柜", "一个放着家人鞋子的鞋柜，有些凌乱。");
    add_poi(lower_hallway, "telephone", "电话", "墙上挂着一部老式电话。");

    // --- 3. Living-Dining-Kitchen (客厅-餐厅-厨房) ---
    *living_dining_kitchen = (Location){0};
    strcpy(living_dining_kitchen->id, "iwakura_living_dining_kitchen");
    strcpy(living_dining_kitchen->name, "客厅-餐厅-厨房");
    strcpy(living_dining_kitchen->description, "一个开放式的起居空间，包含了客厅、餐厅和厨房的功能。");
    add_connection(living_dining_kitchen, "iwakura_lower_hallway:go_to_hallway");
    add_poi(living_dining_kitchen, "sofa", "沙发", "一套舒适的布艺沙发。");
    add_poi(living_dining_kitchen, "tv", "电视", "一台播放着新闻节目的电视机。");
    add_poi(living_dining_kitchen, "dining_table", "餐桌", "摆放着水果和杯子的餐桌。");
    add_poi(living_dining_kitchen, "refrigerator", "冰箱", "发出嗡嗡声的冰箱，里面可能有些吃的。");

    // --- 4. Bathroom (浴室) ---
    *bathroom = (Location){0};
    strcpy(bathroom->id, "iwakura_bathroom");
    strcpy(bathroom->name, "浴室");
    strcpy(bathroom->description, "一个普通的浴室。");
    add_connection(bathroom, "iwakura_lower_hallway:go_to_hallway");
    add_poi(bathroom, "sink", "洗手台", "干净的洗手台，上面放着牙刷。");
    add_poi(bathroom, "bathtub", "浴缸", "一个白色的浴缸，看起来很干净。");

    // --- 5. Upper Hallway (上走廊) ---
    *upper_hallway = (Location){0};
    strcpy(upper_hallway->id, "iwakura_upper_hallway");
    strcpy(upper_hallway->name, "上走廊");
    strcpy(upper_hallway->description, "连接着房子二楼的房间。");
    add_connection(upper_hallway, "iwakura_lower_hallway:go_downstairs");
    add_connection(upper_hallway, "iwakura_lains_room:enter_lains_room");
    add_connection(upper_hallway, "iwakura_mikas_room:enter_mikas_room"); // Added connection to Mika's room
    add_poi(upper_hallway, "painting", "装饰画", "墙上挂着一幅抽象画。");

    // --- 6. Lain's Room (Lain的房间) ---
    *lains_room = (Location){0};
    strcpy(lains_room->id, "iwakura_lains_room");
    strcpy(lains_room->name, "Lain的房间");
    strcpy(lains_room->description, "你的房间。很暗，充满了电子设备运作的嗡嗡声。");
    add_connection(lains_room, "iwakura_upper_hallway:exit_room");
    // POIs from original lain_room
    add_poi(lains_room, "navi_computer", "NAVI电脑", "一台巨大的Navi电脑，屏幕上闪烁着绿色的光标。");
    add_poi(lains_room, "bed", "床", "一张单人床，被子有些凌乱。");
    add_poi(lains_room, "window", "窗户", "一扇看向外面世界的窗户，外面是夜色。");
    add_poi(lains_room, "toy_dog", "玩具狗", "一个陈旧的玩具狗，坐在书桌一角。");
    add_poi(lains_room, "bookshelf", "书架", "一个堆满了各种书籍和杂志的书架。");

    // --- 7. Mika's Room (美香的房间) ---
    *mikas_room = (Location){0};
    strcpy(mikas_room->id, "iwakura_mikas_room");
    strcpy(mikas_room->name, "美香的房间");
    strcpy(mikas_room->description, "姐姐美香的房间。与你的房间相比，这里整洁得有些不真实。");
    add_connection(mikas_room, "iwakura_upper_hallway:exit_room");
    add_poi(mikas_room, "desk", "书桌", "一张整洁的书桌，上面摆放着化妆品和教科书。");
    add_poi(mikas_room, "wardrobe", "衣柜", "一个高大的衣柜，里面是美香的衣服。");
    
    // --- 8. Study (书房) ---
    *study = (Location){0};
    strcpy(study->id, "iwakura_study");
    strcpy(study->name, "书房");
    strcpy(study->description, "父亲的书房。空气中弥漫着旧书和淡淡的烟草味。");
    add_connection(study, "iwakura_lower_hallway:go_to_hallway");
    add_poi(study, "bookshelf", "书架", "一个巨大的书架，塞满了各种专业书籍。");
    add_poi(study, "desk", "办公桌", "一张堆满了文件和电脑的办公桌。");
    
    return IWAKURA_HOUSE_ROOM_COUNT;
}
