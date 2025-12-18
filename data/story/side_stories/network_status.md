---
location: lain_room
---

**NAVI - 网络状态**

使用 `network status` 命令查看当前连接状态。

---

**协议栈:**

{
  "flag_to_check": "network_status.protocols.ipv4",
  "branches": {
    "on": {
      "choices": [ { "text": "- [x] IPv4", "action": "toggle_ipv4" } ]
    },
    "default": {
      "choices": [ { "text": "- [ ] IPv4", "action": "toggle_ipv4" } ]
    }
  }
}

{
  "flag_to_check": "network_status.protocols.ipv6",
  "branches": {
    "on": {
      "choices": [ { "text": "- [x] IPv6", "action": "toggle_ipv6" } ]
    },
    "default": {
      "choices": [ { "text": "- [ ] IPv6", "action": "toggle_ipv6" } ]
    }
  }
}

{
  "flag_to_check": "network_status.protocols.ip7",
  "branches": {
    "on": {
      "choices": [ { "text": "- [x] IP7 (实验性)", "action": "toggle_ip7" } ]
    },
    "default": {
      "choices": [ { "text": "- [ ] IP7 (实验性)", "action": "toggle_ip7" } ]
    }
  }
}


**范围切换:**

{
  "flag_to_check": "network_status.scope",
  "branches": {
    "地区局域网": {
      "choices": [
        { "text": "- <地区局域网> | 全国互联网", "action": "connect_to_national" }
      ]
    },
    "全国互联网": {
      "choices": [
        { "text": "- 地区局域网 | <全国互联网>", "action": "connect_to_regional" }
      ]
    },
    "default": {
      "choices": [
        { "text": "- <地区局域网> | 全国互联网", "action": "connect_to_national" }
      ]
    }
  }
}

- [返回系统设置](action:system_settings)
