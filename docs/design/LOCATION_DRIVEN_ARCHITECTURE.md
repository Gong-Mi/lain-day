# 地点驱动叙事架构（Location-Driven Narrative Architecture）

> 状态：设计文档，待实现  
> 目标：将场景（Scene）降级为地点（Location）的事件，地点成为第一公民

---

## 一、核心原则

```
旧模型：场景驱动
  玩家进入场景 → 场景描述 + 对话 + 选项 → 选择 → 跳转到下一个场景

新模型：地点驱动
  玩家在地点A → arls 查询地点A的当前状态 → 移动/互动 → 进入地点B
  "场景"只是地点在特定时间/条件下的一个事件触发器
```

**地点 = 容器。场景 = 容器里的液体。时间是温度——不同温度下液体形态不同。**

---

## 二、数据架构

### 2.1 地点注册表（Location Registry）

```rust
/// 地点注册表 —— 世界的唯一真实来源
#[derive(Debug, Clone)]
pub struct LocationRegistry {
    pub locations: HashMap<String, Location>,
}

/// 地点 = 可探索的空间单位
#[derive(Debug, Clone)]
pub struct Location {
    pub id: String,
    /// 分层描述（物理层/网络层/认知层）
    pub descriptions: LayeredDescriptions,
    /// 兴趣点
    pub pois: Vec<PointOfInterest>,
    /// 空间连接（可移动的目标）
    pub connections: Vec<Connection>,
    /// 绑定到该地点的事件列表
    pub events: Vec<LocationEvent>,
    /// 关联的符号（可选，用于符号图共振）
    pub symbol_id: Option<String>,
    /// 访问计数（用于轮回中的信息差）
    pub visit_count: u32,
}

/// 分层描述 —— 直接替代单一 description.txt
#[derive(Debug, Clone, Default)]
pub struct LayeredDescriptions {
    pub physical: String,      // 默认层（wired_level = 0）
    pub wired: Option<String>, // 网络层（wired_level >= 3）
    pub cognitive: Option<String>, // 认知层（精神不稳定时）
}

/// 兴趣点
#[derive(Debug, Clone)]
pub struct PointOfInterest {
    pub id: String,
    pub name_text_id: String,
    pub description_text_id: String,
    pub action_id: String, // 交互时触发的 action
    pub requires_wired_level: u8,
}

/// 空间连接
#[derive(Debug, Clone)]
pub struct Connection {
    pub action_id: String,   // 玩家输入的命令（如 "downstairs"）
    pub target_location: String,
    pub time_cost_minutes: u32,
    pub conditions: Vec<Condition>, // 条件锁（如需要钥匙、特定时间）
}

/// 地点事件 —— 替代独立 .ssl 场景文件的核心结构
#[derive(Debug, Clone)]
pub struct LocationEvent {
    pub event_id: String,
    pub trigger: EventTrigger,
    pub scene_id: String, // 触发时加载的场景内容（复用现有 .ssl）
    pub priority: i32,    // 事件优先级，高优先级覆盖低优先级
    pub consume: bool,    // true = 触发后从列表移除（一次性事件）
}

/// 事件触发器 —— 时间 + 状态 + 访问次数的三重索引
#[derive(Debug, Clone)]
pub struct EventTrigger {
    /// ECC 时间范围（起始，结束），None 表示任意时间
    pub time_range: Option<(u32, u32)>,
    /// 必须满足的 flag
    pub requires_flags: Vec<(String, String)>,
    /// 访问次数要求（如 "第2次访问该地点时触发"）
    pub visit_count: Option<u32>,
    /// 触发概率（0.0~1.0，用于不确定事件）
    pub probability: f32,
}
```

### 2.2 与现有系统的映射

| 旧系统 | 新系统中的角色 |
|--------|--------------|
| `.ssl` 场景文件 | `LocationEvent.scene_id` 的引用目标 |
| `executor.rs` 的跳转表 | `Location.connections` + `Location.events` |
| `arls` 命令输出 | `Location.descriptions` + `Location.pois` + `Location.connections` |
| `GameState.current_scene` | `GameState.current_location` + 当前激活的 `LocationEvent` |
| `sequences/` 目录 | 数据来源之一，用于初始化 `LocationRegistry` |

---

## 三、运行时流程

### 3.1 初始化（游戏启动）

```rust
// 1. 从 locations_*.json 加载基础地点拓扑
let registry = LocationRegistry::load_from_json("data/locations/");

// 2. 扫描所有 .ssl 文件，按 location_id 注册为 LocationEvent
registry.register_scenes_from_ssl("data/scenes/story/");

// 3. 扫描符号图，将 symbol_id 绑定到对应地点
registry.bind_symbols("data/symbols/symbol_graph.yaml");
```

### 3.2 主循环（每帧/每次输入）

```rust
// 玩家输入命令 "arls"
fn cmd_arls(state: &GameState, registry: &LocationRegistry) -> String {
    let loc = registry.get(&state.player.location).unwrap();

    // 1. 选择描述层
    let desc = loc.descriptions.select(state.wired_level);

    // 2. 列出 POI（根据 wired_level 过滤）
    let visible_pois: Vec<_> = loc.pois.iter()
        .filter(|p| p.requires_wired_level <= state.wired_level)
        .collect();

    // 3. 列出连接（根据条件过滤）
    let visible_connections: Vec<_> = loc.connections.iter()
        .filter(|c| check_all(&c.conditions, state))
        .collect();

    // 4. 查询符号图，注入象征性描述片段（可选）
    let symbol_fragments = if let Some(ref sym) = loc.symbol_id {
        state.symbol_graph.generate_resonance(sym, 2)
    } else { String::new() };

    format!("{}\n{}\n{}", desc, render_pois(visible_pois), render_connections(visible_connections))
}

// 玩家输入命令 "move downstairs"
fn cmd_move(action_id: &str, state: &mut GameState, registry: &LocationRegistry) -> Vec<Command> {
    let loc = registry.get(&state.player.location).unwrap();

    // 找到连接
    let conn = loc.connections.iter().find(|c| c.action_id == action_id).unwrap();

    // 消耗时间
    let mut cmds = vec![AdvanceTime(conn.time_cost_minutes)];

    // 移动
    cmds.push(MoveTo(conn.target_location.clone()));

    // 增加访问计数
    cmds.push(IncrementVisitCount(conn.target_location.clone()));

    // 检查目标地点是否有事件触发
    let target_loc = registry.get(&conn.target_location).unwrap();
    if let Some(event) = target_loc.find_triggerable_event(state) {
        cmds.push(TriggerEvent(event.event_id.clone()));
    }

    cmds
}
```

### 3.3 事件触发判定

```rust
impl Location {
    /// 找到当前状态下应该触发的事件
    pub fn find_triggerable_event(&self, state: &GameState) -> Option<&LocationEvent> {
        self.events.iter()
            .filter(|e| !e.is_consumed(state)) // 未被消耗
            .filter(|e| e.trigger.is_satisfied(state, self.visit_count))
            .max_by_key(|e| e.priority) // 高优先级优先
    }
}

impl EventTrigger {
    pub fn is_satisfied(&self, state: &GameState, visit_count: u32) -> bool {
        // 时间检查
        if let Some((start, end)) = self.time_range {
            let t = state.time_of_day;
            if t < start || t > end { return false; }
        }

        // Flag 检查
        for (k, v) in &self.requires_flags {
            if state.get_flag(k) != Some(v) { return false; }
        }

        // 访问次数检查
        if let Some(req) = self.visit_count {
            if visit_count != req { return false; }
        }

        // 概率检查（用于不确定事件）
        if self.probability < 1.0 {
            let roll = fastrand::f32(); // 项目已有 fastrand 依赖
            if roll > self.probability { return false; }
        }

        true
    }
}
```

---

## 四、与现有代码的集成点

### 4.1 GameState 修改（最小化）

```rust
pub struct GameState {
    pub player: PlayerState,
    // 旧：pub current_scene: String,
    // 新：地点成为主键，场景降级为"当前地点的激活事件"
    pub current_location: String,
    pub active_event: Option<String>, // 当前触发的事件ID

    pub time_of_day: u32,
    pub flags: HashMap<String, String>,
    pub typewriter_delay: f32,
    // ... 保留其余字段

    // 新增：符号图运行时（已有模块）
    pub symbol_graph: SymbolGraphRuntime,
}
```

### 4.2 executor.rs 修改

`resolve_action` 的核心逻辑从 **"查找场景ID并跳转"** 改为 **"查找地点连接并移动"**。

旧逻辑：
```rust
"downstairs" => Some(("SCENE_02_DOWNSTAIRS", None))
```

新逻辑：
```rust
"downstairs" => {
    // 查询 LocationRegistry
    vec![MoveTo("iwakura_living_dining_kitchen".into())]
}
```

**保留 `.ssl` 的兼容性**：`.ssl` 文件仍然定义场景的具体文本和选项，但它们的加载方式从 **"主动跳转目标"** 变为 **"地点事件的被动触发内容"**。

### 4.3 符号图的联动

```rust
// 地点注册时自动绑定符号
for (loc_id, loc) in &mut registry.locations {
    // 符号ID默认等于 location_id，但可覆盖
    loc.symbol_id = Some(format!("location_{}", loc_id));
}

// arls 渲染时自动注入符号共振
let related_symbols = symbol_graph.query_related(&loc.symbol_id);
for (rel, weight) in related_symbols {
    if weight > 0.7 {
        // 高权重符号会微妙地改变地点描述
        desc = desc.replace("普通房间", "不再普通的房间");
    }
}
```

---

## 五、迁移路线图

| 阶段 | 任务 | 风险 | 预计时间 |
|------|------|------|---------|
| P0 | 新建 `LocationRegistry` + `Location` 结构 | 低 | 2天 |
| P0 | 从 `locations_*.json` 加载基础地点 | 低 | 1天 |
| P1 | 重写 `arls` 命令：从场景渲染改为地点查询 | 中 | 2天 |
| P1 | 重写 `move` 命令：从跳转表改为地点连接 | 中 | 2天 |
| P2 | 给所有 `.ssl` 补充 `location_id` 和 `EventTrigger` | 低 | 1天 |
| P2 | 将 `.ssl` 注册为 `LocationEvent` | 低 | 1天 |
| P3 | 接入符号图：`arls` 输出随 wired_level + 符号权重变化 | 中 | 3天 |
| P3 | 时间锁：同一地点不同时间触发不同事件 | 低 | 1天 |
| P4 | 访问计数：轮回中第N次访问同一地点触发不同事件 | 中 | 2天 |

**总预计：约2周完成核心迁移。**

---

## 六、验证标准

重构完成后，以下测试应该通过：

```rust
#[test]
fn test_location_driven_flow() {
    let mut state = GameState::default();
    let registry = LocationRegistry::load_test();

    // 玩家在二楼走廊
    assert_eq!(state.current_location, "iwakura_upper_hallway");

    // arls 应该显示走廊的描述 + 门 + 楼梯口
    let output = cmd_arls(&state, &registry);
    assert!(output.contains("房门"));
    assert!(output.contains("下楼"));

    // move downstairs
    let cmds = cmd_move("downstairs", &mut state, &registry);
    assert!(cmds.iter().any(|c| matches!(c, MoveTo(loc) if loc == "iwakura_living_dining_kitchen")));

    // 移动到厨房后，时间应该前进
    assert!(cmds.iter().any(|c| matches!(c, AdvanceTime(2)))); // 下楼耗时2分钟
}
```

---

## 七、为什么这样搭是对的

| 设计决策 | 解决的问题 |
|---------|-----------|
| 地点是第一公民 | 场景不再碎片化，空间拓扑成为叙事的骨骼 |
| EventTrigger 三重索引（时间/flag/访问次数） | 时间轮回终于有地方落地——同一地点第1次和第3次访问可以完全不同 |
| 分层描述（物理/网络/认知） | `arls` 不再需要 hacks，描述切换是地点自身的属性 |
| 符号图绑定 | 地点描述可以自动携带象征意义，不需要写手硬塞 |
| 保留 `.ssl` 作为事件内容 | 已有的剧本资产不被废弃，只是重新挂载到地点上 |

---

要我先把 `LocationRegistry` 的 Rust 骨架写出来吗？还是你更想先确认这个架构设计有没有遗漏的边界情况？
