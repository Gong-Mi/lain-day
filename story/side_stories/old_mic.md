---
location: cyberia_club
---

### 老式麦克风

你走近那个立在舞台角落里的老式麦克风。它看起来很有年头了，金属外壳上有些许划痕和锈迹，似乎见证了无数的表演。

你感觉它不只是一个简单的扩音设备，似乎还残留着某些人的声音和情感。

{
  "flag_to_check": "cyberia_youth_present",
  "branches": {
    "true": {
      "text": "",
      "choices": [
        {"text": "唱一首《Plastic Love》", "action": "gunshot"},
        {"text": "唱动画的OP (Duvet)", "action": "gunshot"},
        {"text": "唱动画的ED (远い叫び)", "action": "gunshot"},
        {"text": "还是算了", "action": "gunshot_exit"}
      ]
    },
    "default": {
      "text": "",
      "choices": [
        {"text": "唱一首《Plastic Love》", "action": "sing_plastic_love"},
        {"text": "唱动画的OP (Duvet)", "action": "sing_op"},
        {"text": "唱动画的ED (远い叫び)", "action": "sing_ed"},
        {"text": "还是算了", "action": "exit_to_main_story"}
      ]
    }
  }
}
