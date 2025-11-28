---
location: lain_room
---

{
  "flag_to_check": "level",
  "branches": {
    "1": { "text": "DATA ASSIMILATION: [=>....................] 5%" },
    "2": { "text": "DATA ASSIMILATION: [=====>................] 28%" },
    "3": { "text": "DATA ASSIMILATION: [========>.............] 42%" },
    "4": { "text": "DATA ASSIMILATION: [===========>..........] 59%" },
    "5": { "text": "DATA ASSIMilation: [==============>.......] 77%" },
    "default": { "text": "DATA ASSIMILATION: [=>....................] 5%" }
  }
}

----------------------------------------

欢迎使用 NAVI。

- [查看邮件](action:show_emails)
- [系统设置](action:system_settings)
- [聊天室](action:enter_chatroom)
- [暂时离开](action:exit_to_main_story)

{
  "flag_to_check": "has_talked_to_sister",
  "branches": {
    "true": {
      "choices": [
        { "text": "- [退出NAVI]", "action": "exit_to_main_story" }
      ]
    },
    "default": {
      "choices": [
        { "text": "- [退出NAVI]", "action": "trigger_shutdown_story" }
      ]
    }
  }
}
