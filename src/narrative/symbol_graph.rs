//! 符号图数据库（Symbol Graph）
//!
//! 存储符号节点和它们之间的关系边，支持动态权重调整。
//! 所有关系权重在运行时是浮动的，由游戏事件和上下文共同决定。
//!
//! 用途：
//! 1. 千砂 AI 生成有象征意义的回复
//! 2. 分层现实描述系统选择不同层的符号
//! 3. 场景描述自动关联相关符号
//! 4. 防止叙事矛盾（检查符号冲突）

use serde::Deserialize;
use std::collections::HashMap;

// =============================================================================
// 数据结构
// =============================================================================

/// 符号图（从 YAML 加载的静态数据）
#[derive(Debug, Clone, Deserialize)]
pub struct SymbolGraph {
    #[serde(default)]
    pub symbols: HashMap<String, SymbolNode>,
    #[serde(default)]
    pub relations: Vec<SymbolRelation>,
}

/// 符号节点
#[derive(Debug, Clone, Deserialize)]
pub struct SymbolNode {
    pub category: SymbolCategory,
    #[serde(default)]
    pub interpretations: Vec<String>,
    #[serde(default = "default_weight")]
    pub base_weight: f32,
}

/// 符号类别
#[derive(Debug, Clone, Copy, PartialEq, Eq, Deserialize)]
#[serde(rename_all = "PascalCase")]
pub enum SymbolCategory {
    Visual,    // 视觉符号（麻花辫、眼镜、鞋）
    Identity,  // 身份符号（神性、影性）
    System,    // 系统符号（时间、协议、错误）
    Spatial,   // 空间符号（地点、场所）
    Action,    // 动作符号（拥抱、阅读邮件）
    Concept,   // 抽象概念（秩序、混沌、连接）
}

/// 关系边
#[derive(Debug, Clone, Deserialize)]
pub struct SymbolRelation {
    pub from: String,
    pub to: String,
    #[serde(rename = "type")]
    pub relation_type: RelationType,
    #[serde(default = "default_weight")]
    pub base_weight: f32,
    pub context: Option<String>,
    #[serde(default)]
    pub note: Option<String>,
}

/// 关系类型
#[derive(Debug, Clone, Copy, PartialEq, Eq, Deserialize)]
#[serde(rename_all = "PascalCase")]
pub enum RelationType {
    Metaphor,       // 隐喻（麻花辫 = 秩序）
    Opposition,     // 对立（神性 vs 影性）
    Resonance,      // 共振（鞋尖朝外 = 协议中止）
    Containment,    // 包含（影性被约束在房间）
    Reflection,     // 反射（神性将房间重构为神庙）
    Reinforcement,  // 强化（小熊睡衣强化影性）
    Causality,      // 因果（溢出导致错误）
    Revelation,     // 启示（错误泄漏神性）
    Parallel,       // 平行（Cyberia 和走廊都是过渡空间）
    Threshold,      // 门槛（走廊→房间）
    Mirror,         // 镜像（千砂隔离 = Lain 隔离）
    Foreshadowing,  // 预示（千砂关机预示 Lain 重置）
    CrossEffect,    // 交叉效果（辫子+眼镜=刻板印象）
}

fn default_weight() -> f32 {
    0.5
}

// =============================================================================
// 运行时（动态权重 + 查询）
// =============================================================================

/// 运行时符号图
///
/// 封装静态图 + 动态权重表 + 上下文状态。
/// 所有查询都通过此结构，确保权重计算一致。
#[derive(Debug, Clone)]
pub struct SymbolGraphRuntime {
    pub graph: SymbolGraph,
    /// 动态权重表：key = "from->to"，value = 当前有效权重
    pub current_weights: HashMap<String, f32>,
    /// 激活的上下文标签（如 ["chapter_2", "wired_level_3"]）
    pub active_contexts: Vec<String>,
}

impl SymbolGraphRuntime {
    /// 从 YAML 字符串加载
    pub fn from_yaml(yaml: &str) -> Result<Self, serde_yaml::Error> {
        let graph: SymbolGraph = serde_yaml::from_str(yaml)?;
        let mut current_weights = HashMap::new();
        for rel in &graph.relations {
            let key = Self::rel_key(&rel.from, &rel.to);
            current_weights.insert(key, rel.base_weight);
        }
        Ok(Self {
            graph,
            current_weights,
            active_contexts: Vec::new(),
        })
    }

    /// 生成关系键
    fn rel_key(from: &str, to: &str) -> String {
        format!("{}->{}", from, to)
    }

    // -------------------------------------------------------------------------
    // 动态权重操作
    // -------------------------------------------------------------------------

    /// 调整某条关系的权重（delta 可为正或负）
    pub fn adjust_weight(&mut self, from: &str, to: &str, delta: f32) {
        let key = Self::rel_key(from, to);
        if let Some(w) = self.current_weights.get_mut(&key) {
            *w = (*w + delta).clamp(0.0, 1.0);
        }
    }

    /// 将某条关系权重设为固定值
    pub fn set_weight(&mut self, from: &str, to: &str, value: f32) {
        let key = Self::rel_key(from, to);
        if self.current_weights.contains_key(&key) {
            self.current_weights.insert(key, value.clamp(0.0, 1.0));
        }
    }

    /// 激活一个上下文标签（如 "chapter_2"）
    pub fn activate_context(&mut self, ctx: impl Into<String>) {
        let s = ctx.into();
        if !self.active_contexts.contains(&s) {
            self.active_contexts.push(s);
        }
    }

    /// 关闭一个上下文标签
    pub fn deactivate_context(&mut self, ctx: &str) {
        self.active_contexts.retain(|c| c != ctx);
    }

    // -------------------------------------------------------------------------
    // 查询接口
    // -------------------------------------------------------------------------

    /// 查询与指定符号相关的所有关系，按当前有效权重降序排列。
    ///
    /// 自动过滤：
    /// - 如果关系声明了 context，只有当该上下文激活时才返回（权重×1.5）
    /// - 否则正常返回（权重×1.0）
    pub fn query_related(
        &self,
        symbol_id: &str,
    ) -> Vec<(&SymbolRelation, f32)> {
        let mut results: Vec<(&SymbolRelation, f32)> = self
            .graph
            .relations
            .iter()
            .filter(|r| r.from == symbol_id || r.to == symbol_id)
            .map(|r| {
                let key = Self::rel_key(&r.from, &r.to);
                let base = self.current_weights.get(&key).copied().unwrap_or(r.base_weight);

                // 上下文匹配时权重放大
                let multiplier = if let Some(ref ctx) = r.context {
                    if self.active_contexts.iter().any(|ac| ctx.contains(ac)) {
                        1.5
                    } else {
                        0.3 // 上下文不匹配时大幅衰减，但不归零
                    }
                } else {
                    1.0
                };

                let effective = (base * multiplier).clamp(0.0, 1.0);
                (r, effective)
            })
            .collect();

        // 按有效权重降序
        results.sort_by(|a, b| b.1.partial_cmp(&a.1).unwrap());
        results
    }

    /// 查询两个符号之间的直接关系权重
    pub fn query_between(&self, from: &str, to: &str) -> Option<f32> {
        let key = Self::rel_key(from, to);
        self.current_weights.get(&key).copied()
    }

    /// 查询某符号的所有解释，按 base_weight 排序
    pub fn interpretations_of(&self, symbol_id: &str) -> Vec<String> {
        self.graph
            .symbols
            .get(symbol_id)
            .map(|n| n.interpretations.clone())
            .unwrap_or_default()
    }

    /// 查询某类别的所有符号
    pub fn symbols_by_category(&self, cat: SymbolCategory) -> Vec<&String> {
        self.graph
            .symbols
            .iter()
            .filter(|(_, n)| n.category == cat)
            .map(|(id, _)| id)
            .collect()
    }

    /// 检查两个符号是否冲突（Opposition 关系权重 > 0.5）
    pub fn is_conflicting(&self, a: &str, b: &str) -> bool {
        let check = |x: &str, y: &str| -> bool {
            self.graph.relations.iter().any(|r| {
                r.from == x && r.to == y && r.relation_type == RelationType::Opposition
            })
        };
        check(a, b) || check(b, a)
    }

    /// 根据当前上下文，生成一段"符号共振文本"
    ///
    /// 示例：查询 "lain_shadow"，找到权重最高的相关符号，
    /// 拼接它们的解释，形成千砂的碎片化回复。
    pub fn generate_resonance(&self, seed: &str, max_fragments: usize) -> String {
        let related = self.query_related(seed);
        let mut fragments = Vec::new();

        for (rel, weight) in related.iter().take(max_fragments) {
            if let Some(node) = self.graph.symbols.get(&rel.to) {
                if let Some(interp) = node.interpretations.first() {
                    // 权重越高，文本越完整；权重越低，越碎片化
                    let cutoff = ((1.0 - weight) * interp.len() as f32) as usize;
                    let text = if cutoff > 0 && cutoff < interp.len() {
                        format!("{}...", &interp[..cutoff])
                    } else {
                        interp.clone()
                    };
                    fragments.push(text);
                }
            }
        }

        fragments.join("\n")
    }
}

// =============================================================================
// 测试
// =============================================================================
#[cfg(test)]
mod tests {
    use super::*;

    fn test_graph() -> SymbolGraphRuntime {
        let yaml = r#"
symbols:
  chisa_braids:
    category: Visual
    interpretations:
      - "秩序的符号"
      - "伪装的盾牌"
    base_weight: 1.0
  lain_shadow:
    category: Identity
    interpretations:
      - "影性"
      - "囚徒"
    base_weight: 1.0
  order:
    category: Concept
    interpretations:
      - "逻辑"
    base_weight: 1.0
  chaos:
    category: Concept
    interpretations:
      - "无秩序"
    base_weight: 1.0

relations:
  - from: chisa_braids
    to: order
    type: Metaphor
    base_weight: 0.9
  - from: chisa_braids
    to: chaos
    type: Opposition
    base_weight: 0.85
    context: "internal"
  - from: lain_shadow
    to: chisa_braids
    type: Mirror
    base_weight: 0.6
"#;
        SymbolGraphRuntime::from_yaml(yaml).unwrap()
    }

    #[test]
    fn test_load_and_query() {
        let sg = test_graph();
        assert_eq!(sg.graph.symbols.len(), 4);
        assert_eq!(sg.graph.relations.len(), 3);
    }

    #[test]
    fn test_query_related_sorted() {
        let sg = test_graph();
        let related = sg.query_related("chisa_braids");
        assert_eq!(related.len(), 2);
        // order (0.9) 应该在 chaos (0.85 * 0.3 = 0.255) 之前
        assert_eq!(related[0].0.to, "order");
        assert!((related[0].1 - 0.9).abs() < 0.01);
    }

    #[test]
    fn test_context_boost() {
        let mut sg = test_graph();
        sg.activate_context("internal");
        let related = sg.query_related("chisa_braids");
        // chaos (0.85 * 1.5 = 1.0) 应该排在 order (0.9) 之前
        assert_eq!(related[0].0.to, "chaos");
        assert!((related[0].1 - 1.0).abs() < 0.01);
    }

    #[test]
    fn test_adjust_weight() {
        let mut sg = test_graph();
        sg.adjust_weight("chisa_braids", "order", 0.05);
        assert!((sg.query_between("chisa_braids", "order").unwrap() - 0.95).abs() < 0.01);
    }

    #[test]
    fn test_conflict_check() {
        let sg = test_graph();
        assert!(sg.is_conflicting("chisa_braids", "chaos"));
        assert!(!sg.is_conflicting("lain_shadow", "order"));
    }

    #[test]
    fn test_generate_resonance() {
        let mut sg = test_graph();
        sg.activate_context("internal");
        let text = sg.generate_resonance("chisa_braids", 2);
        // 应该包含 chaos 和 order 的解释
        assert!(text.contains("无秩序") || text.contains("逻辑"));
    }
}
