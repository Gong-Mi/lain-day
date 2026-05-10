//! 事件系统 —— 替代 C 版本的 event_system.c
//!
//! 简单的循环队列，单线程版本中不需要 Mutex。

use std::collections::VecDeque;

const MAX_EVENTS: usize = 64;

/// 事件类型
#[derive(Debug, Clone, Copy, PartialEq, Eq)]
pub enum EventType {
    SceneTransition,
    FlagChanged,
    TimeAdvanced,
    ItemAcquired,
    SystemMessage,
}

/// 事件
#[derive(Debug, Clone, PartialEq, Eq)]
pub struct Event {
    pub event_type: EventType,
    pub data: String,
}

/// 线程安全的事件队列
#[derive(Debug, Clone)]
pub struct EventQueue {
    queue: VecDeque<Event>,
}

impl Default for EventQueue {
    fn default() -> Self {
        Self {
            queue: VecDeque::with_capacity(MAX_EVENTS),
        }
    }
}

impl EventQueue {
    pub fn new() -> Self {
        Self::default()
    }

    /// 推送事件，队列满时返回 false
    pub fn push(&mut self, event: Event) -> bool {
        if self.queue.len() >= MAX_EVENTS {
            return false;
        }
        self.queue.push_back(event);
        true
    }

    /// 轮询事件
    pub fn poll(&mut self) -> Option<Event> {
        self.queue.pop_front()
    }

    pub fn len(&self) -> usize {
        self.queue.len()
    }

    pub fn is_empty(&self) -> bool {
        self.queue.is_empty()
    }

    pub fn clear(&mut self) {
        self.queue.clear();
    }
}

// =============================================================================
// 测试
// =============================================================================
#[cfg(test)]
mod tests {
    use super::*;

    #[test]
    fn test_push_and_poll() {
        let mut q = EventQueue::new();
        assert!(q.is_empty());

        let e = Event {
            event_type: EventType::SceneTransition,
            data: "SCENE_01".into(),
        };
        assert!(q.push(e.clone()));
        assert_eq!(q.len(), 1);

        let polled = q.poll().unwrap();
        assert_eq!(polled, e);
        assert!(q.is_empty());
    }

    #[test]
    fn test_queue_full() {
        let mut q = EventQueue::new();
        for i in 0..MAX_EVENTS {
            assert!(q.push(Event {
                event_type: EventType::SystemMessage,
                data: format!("msg{}", i),
            }));
        }
        assert!(!q.push(Event {
            event_type: EventType::SystemMessage,
            data: "overflow".into(),
        }));
    }
}
