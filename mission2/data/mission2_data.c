#include "mission2_data.h"

const Agent original_agents[] = {
    {4497, "DOOFENSHMIRTZ", "INVENTOR", 1, 1},
    {2864, "EVIL_MORTY", "STRATEGIST", 5, 1},
    {6291, "PLANKTON", "CYBER_OPERATOR", 3, 1},
    {4439, "GRETA_THUNBERG", "MARITIME_SUPPORT", 1, 0},
    {1475, "MRS_NORA_BEADY", "CAMPUS_SURVEILLANCE", 1, 0},
    {8775, "BOMBARDIRO_CROCODILO", "IRANIAN_AIR_SUPPORT", 4, 0},
    {2082, "SECURITY_OFFICER", "SECURITY", 5, 1},
    {3722, "TECH_SUPPORT", "TECHNICAL_SUPPORT", 4, 1},
    {2420, "LOGISTICS_STAFF", "LOGISTICS", 2, 1},
    {3319, "MAINTENANCE_STAFF", "MAINTENANCE", 3, 1}
};
const size_t original_agent_count =
    sizeof original_agents / sizeof original_agents[0];

const AgentPatch verified_patches[] = {
    {4497, 4, 1},
    {4439, 1, 1},
    {1475, 2, 1},
    {8775, 4, 1}
};
const size_t verified_patch_count =
    sizeof verified_patches / sizeof verified_patches[0];

const AccessEvent access_events[] = {
    {6291, CONTROL_SYSTEM, 2, 0, 5, 7},
    {2864, EVIL_MORTY_PRIVATE_COMMS, 5, 1, -1, -1},
    {67, EVIL_MORTY_PRIVATE_COMMS, 3, 1, 6, 5},
    {8775, AIR_SUPPORT, 4, 1, -1, -1},
    {4439, PORT_8080, 5, 0, -1, -1},
    {67, BGU_CAMERAS, 3, 1, 3, 8},
    {67, EVIL_MORTY_PRIVATE_COMMS, 5, 1, 2, 3},
    {4439, PORT_8080, 5, 0, -1, -1},
    {4439, PORT_8080, 5, 0, -1, -1},
    {67, AIR_SUPPORT, 4, 1, 4, 15},
    {67, BGU_CAMERAS, 3, 1, 0, 24},
    {4439, PORT_8080, 5, 0, -1, -1},
    {67, CONTROL_SYSTEM, 5, 1, 5, 24},
    {67, SERVER_BACKUP, 4, 1, 7, 25},
    {90001, PORT_8080, 1, 0, 1, 4},
    {4497, CONTROL_SYSTEM, 4, 1, 3, 12},
    {4439, PORT_8080, 5, 0, -1, -1},
    {4439, PORT_8080, 5, 0, -1, -1},
    {4439, PORT_8080, 5, 0, -1, -1},
    {67, CONTROL_SYSTEM, 4, 1, 1, 19},
    {1475, BGU_CAMERAS, 3, 0, -1, -1}
};
const size_t access_event_count =
    sizeof access_events / sizeof access_events[0];
