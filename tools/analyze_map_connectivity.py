import os
import re
import json
import collections
import sys

# Configuration
PROJECT_ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
SEQUENCES_DIR = os.path.join(PROJECT_ROOT, 'sequences')
STATION_COORDINATES_FILE = os.path.join(SEQUENCES_DIR, 'station_coordinates.json')

# Regex to extract connection information
# (source_loc, action_id, target_loc, target_scene)
# This will be tricky because add_connection and add_connection_to_location have different signatures
# and target_scene is not always relevant for graph building.

# Regex to extract the ID of the current location being defined in a scene.c file
# Group 1: var_name (e.g., front_yard)
# Group 2: loc_id (e.g., iwakura_front_yard)
CURRENT_LOC_ID_REGEX = re.compile(
    r'(?:strcpy|init_location)\s*\(\s*(\w+)(?:->id)?,\s*"([^"]+)"\)'
)