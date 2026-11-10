// Menu source watermark: @use_sberbank
// Lookups over the generated map table (see tools/build_maps.py, which writes the data
// into map_catalog_data.cpp; this file only reads it).
#include <cstring>

#include "menu/map_catalog.h"

namespace nl::maps {

int image_of(const char* id)
{
    if (!id || !*id)
        return -1;
    for (int index = 0; index < entry_count; ++index)
        if (std::strcmp(entries[index].id, id) == 0)
            return entries[index].image;
    return -1;
}

const char* display_name(const char* id)
{
    if (!id || !*id)
        return "";
    for (int index = 0; index < entry_count; ++index)
        if (std::strcmp(entries[index].id, id) == 0)
            return entries[index].name;
    return id;
}

} // namespace nl::maps
