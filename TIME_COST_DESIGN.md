# Time Cost Design Document (Pending Implementation)

This document outlines proposed time costs for various game actions to enhance realism and pacing. This is a design document for a pending feature and should be consulted if/when the `get_action_time_cost` function in `src/executor.c` is updated.

The `action_id`s listed here are based on the refactoring for the `move` command. If these `action_id`s are changed again, this document will need to be updated accordingly.

## 1. Movement Actions

### Short-Distance (1-2 minutes)
- `downstairs`
- `upstairs`
- `lains_room`
- `mikas_room`
- `bathroom`
- `study`
- `living_area`
- `hallway`
- `outside`
- `house`

### Long-Distance (15-30 minutes)
- `shibuya` (e.g., 25 mins)
- `home` (e.g., 25 mins)
- `shinjuku_site` (e.g., 30 mins)
- `cyberia` (e.g., 15 mins)

## 2. Interaction & Dialogue Actions (2-10 minutes)
- `examine_navi` (e.g., 2 mins)
- `talk_to_dad` (e.g., 5-10 mins)
- `talk_to_mom` (e.g., 5-10 mins)
- `talk_to_sister` (e.g., 5 mins)
- `read_email_from_chisa` (e.g., 2 mins)
- All `ask_*` actions (e.g., 3-5 mins)

## 3. Item & Scene-Specific Actions (1-3 minutes)
- `get_milk` (e.g., 2 mins)
- `take_milk_from_fridge` (e.g., 1 min)
- `wait_one_minute` (e.g., 1 min)
- `navi_reboot` (e.g., 2 mins)
- `navi_shutdown` (e.g., 1 min)

**Total Estimated Actions with Time Cost:** ~30
