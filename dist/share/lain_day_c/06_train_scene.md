# 摇晃的电车

电车上人熙熙攘攘，空气闷热而粘稠。你被挤在人群中，周围充满了各种各样的声音。

> “——你认识那个明星吗？”
> “——什么？今天外勤安排的是我？”
> “——哦哦哦，我SSR！手机快没电那我先挂了——”

这些声音和你无关，它们像白噪音一样包裹着你。
{
  "flag_to_check": "sister_mood",
  "branches": {
    "curious": {
      "text": "但千砂的邮件，依然在你脑海里挥之不去。你感觉头痛欲裂。\n\n你必须做点什么，或者什么都不做。",
      "choices": [
        { "text": "（回应邮件）尝试连接到“连接世界”", "action": "active_overload" },
        { "text": "（回应邮件）向Navi询问关于“四方田千砂”的事", "action": "active_overload" },
        { "text": "（回应邮件）把这封邮件当成恶作剧，删除它", "action": "active_overload" },
        { "text": "........ (静静忍耐)", "action": "passive_overload" }
      ]
    },
    "default": {
      "text": "突然，一个声音在你脑海中响起，它不属于周围任何人：\n\n> “你真的认识lain吗？”\n\n这个声音让你感觉头痛欲裂。\n\n你必须做点什么，或者什么都不做。",
      "choices": [
        { "text": "尝试在脑海中回应“你是谁？”", "action": "active_overload" },
        { "text": "试图忽略这个声音", "action": "passive_overload" }
      ]
    }
  }
}