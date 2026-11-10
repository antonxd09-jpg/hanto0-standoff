// Menu source watermark: @use_sberbank
#pragma once

// Agent models for the inventory (see tools/build_agents.py).
//
// CS2 loads an agent as a model, so the inventory can show the real animated character
// instead of a flat picture: the definition index of the agent item maps to the path the
// game itself would request from pak01.
namespace nl::agents {

enum class Team : int { ct = 0, t = 1, other = 2 };

struct Entry {
    int def_index;
    const char* name;
    int team;          // Team as an int so the table stays plain data.
    const char* model; // "agents/models/tm_professional/tm_professional_varf5.vmdl_c"
};

extern const Entry models[];
extern const int model_count;

// Model path for an agent definition index, or nullptr when the table has no entry.
const char* model_for(int def_index);
// Model path for an agent name, which is how the inventory catalogue identifies items
// (it does not carry definition indices). Both tables come from the same API, so the
// names match exactly.
const char* model_for_name(const char* name);
// First model of a team, used when a slot has no explicit agent.
const char* default_model(Team team);

} // namespace nl::agents
