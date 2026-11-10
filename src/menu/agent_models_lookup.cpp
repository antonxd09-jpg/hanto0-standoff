// Menu source watermark: @use_sberbank
// Lookups over the generated agent model table (tools/build_agents.py).
#include <cstring>

#include "menu/agent_models.h"

namespace nl::agents {

namespace {

// The schema's display names are not uniformly spaced ("Name  | Group"), so runs of
// spaces compare as one.
bool same_name(const char* a, const char* b)
{
    while (*a && *b) {
        if (*a == ' ' && *b == ' ') {
            while (*a == ' ')
                ++a;
            while (*b == ' ')
                ++b;
            continue;
        }
        if (*a != *b)
            return false;
        ++a;
        ++b;
    }
    return *a == *b;
}

} // namespace

const char* model_for(int def_index)
{
    for (int index = 0; index < model_count; ++index)
        if (models[index].def_index == def_index)
            return models[index].model;
    return nullptr;
}

const char* model_for_name(const char* name)
{
    if (!name || !*name)
        return nullptr;
    for (int index = 0; index < model_count; ++index)
        if (models[index].name && same_name(models[index].name, name))
            return models[index].model;
    return nullptr;
}

const char* default_model(Team team)
{
    for (int index = 0; index < model_count; ++index)
        if (models[index].team == static_cast<int>(team))
            return models[index].model;
    return nullptr;
}

} // namespace nl::agents