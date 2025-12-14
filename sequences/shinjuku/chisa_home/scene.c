#include "scene.h"
#include <string.h>
#include <stdio.h>
#include "conditions.h" // For potential future accessibility functions
#include "flag_system.h" // For potential future flag checks
#include "game_types.h" // For MAX_NAME_LENGTH, MAX_DESC_LENGTH etc.
#include "map_loader.h" // For global helper functions

int create_chisa_home_layout(Location* all_locations, int starting_index) {
    if (all_locations == NULL || starting_index < 0) {
        return 0;
    }

    Location* chisa_home = &all_locations[starting_index];
    *chisa_home = (Location){0}; // Zero out the struct

    strcpy(chisa_home->id, "chisa_home");
    strcpy(chisa_home->name, "千砂家");
    strcpy(chisa_home->description, "一个整洁 没有门锁的房间 没有阳台 只有一个窗户 门前挂载备战东大 背后挂着 她小时候和奶奶一起在关岛沙滩上的照片。");

    // Connections (from old_map/chisa_home/connections.json - which was empty)
    // Add a default connection to Shinjuku Station for now
    add_connection_to_location(chisa_home, "exit_home", "shinjuku_station", NULL, NULL, NULL);

    // POIs from old_map/chisa_home/poi.json
    add_poi_to_location(chisa_home, "photo_on_door", "门上的照片", "一张褪色的旧照片，照片上是一个小女孩和一位慈祥的老人，背景是明媚的沙滩和大海。翻到照片背后，能看到用几乎看不清的、另一种语言写着一行字：“我不能输”。", NULL);
    add_poi_to_location(chisa_home, "stool_by_window", "窗边的小板凳", "一个看起来很普通的木板凳，放在窗边似乎有些碍事。如果把它翻过来，会发现底部用小刀刻着一行字：“観星専用”（观星专用）。", NULL);
    
    // Combined description for "书柜" and its sub_items
    add_poi_to_location(chisa_home, "bookshelf", "书柜", "书柜上塞满了各种书籍，大部分是学校的参考书和一些文学名著。在书本的缝隙间，放着几颗被标记过的石头和一个试剂瓶。\n参考书与名著：大部分是学校的参考书和一些文学名著，看起来很新，似乎很少被翻动。\n被标记的石头：三颗不起眼的石头，被当作书挡使用。每颗石头下面都压着一张小纸条，上面分别写着：‘翠石’、‘沉积岩’、‘血石’。\n装着黄沙的试剂瓶：一个化学实验用的试剂瓶，被擦拭得非常干净，里面装着大约半瓶黄色的颗粒物。轻轻摇晃，可以看到那是很细的、干净的海沙，没有普通河沙的粗糙和粘稠感。沙子中，还混杂着一些微小的、破碎的贝壳。- [拿走瓶子](action:take_sand_bottle)", NULL);
    
    // Combined description for "床" and its sub_items
    add_poi_to_location(chisa_home, "bed", "床", "一张整理得非常整洁的单人床，被褥叠得一丝不苟。床下似乎塞了一些书。\n床下的SF小说：几本SF小说，被随意地塞在床下。看起来经常被翻阅，但似乎都没有读到最后一页。", NULL);
    
    add_poi_to_location(chisa_home, "empty_tripod", "空着的三脚架", "一个专业的相机或望远镜三脚架，稳固地立在窗边。但上面空空如也，没有安装任何设备。", NULL);
    
    return CHISA_HOME_ROOM_COUNT;
}
