// Menu source watermark: @use_sberbank
#include "menu/search.h"

#include <algorithm>
#include <cctype>
#include <cstring>
#include <string>

#include "imgui_internal.h"
#include "menu/localization.h"

namespace nl::search {
namespace {

bool g_collecting = false;
std::vector<Entry> g_entries;
std::vector<Path> g_paths;
std::vector<std::string> g_stack;
int g_tab = 0, g_subtab = 0;
int g_current = -1; // path of the current stack, -1 when it changed

int current_path()
{
    if (g_current >= 0)
        return g_current;
    std::string text;
    for (const std::string& s : g_stack) {
        if (!text.empty())
            text += '\x1f'; // separator, drawn as a chevron
        for (char ch : s)
            text += static_cast<char>(std::toupper(static_cast<unsigned char>(ch)));
    }
    for (size_t i = 0; i < g_paths.size(); ++i)
        if (g_paths[i].text == text) {
            g_current = static_cast<int>(i);
            return g_current;
        }
    g_paths.push_back({ text, g_tab, g_subtab });
    g_current = static_cast<int>(g_paths.size()) - 1;
    return g_current;
}

} // namespace

bool collecting() { return g_collecting; }

void begin()
{
    g_collecting = true;
    g_entries.clear();
    g_paths.clear();
    g_stack.clear();
    g_current = -1;
}

void end()
{
    g_collecting = false;
    g_stack.clear();
}

void set_tab(int tab, int subtab)
{
    g_tab = tab;
    g_subtab = subtab;
    g_current = -1;
}

void push(const char* segment)
{
    g_stack.emplace_back(segment, ImGui::FindRenderedTextEnd(segment));
    g_current = -1;
}

void pop()
{
    if (!g_stack.empty())
        g_stack.pop_back();
    g_current = -1;
}

void add(Entry e)
{
    if (!g_collecting || !e.label)
        return;
    e.path = current_path();
    g_entries.push_back(e);
}

const std::vector<Entry>& entries() { return g_entries; }
const std::vector<Path>& paths() { return g_paths; }

namespace {

// Position of a Cyrillic letter on the ЙЦУКЕН layout, as the Latin character the
// same key produces under the US layout. Every menu label is written in English,
// so a query typed with the wrong layout has to be folded back before comparing.
char cyrillic_to_latin(const unsigned code_point)
{
    if (code_point == 0x0401 || code_point == 0x0451)
        return '`'; // ё
    // ЙЦУКЕН is laid out as a contiguous pair of blocks: U+0410..U+042F for the
    // capitals and U+0430..U+044F for the lower case, 0x20 apart. Only the capital
    // block is shifted down -- the lower case block is not "bit 5 always set"
    // (ф is U+0444, which has bit 5 clear), so an unconditional `| 0x20` would
    // push half the alphabet out of range.
    unsigned lower = code_point;
    if (lower >= 0x0410u && lower <= 0x042Fu)
        lower += 0x20u;
    if (lower < 0x0430u || lower > 0x044Fu)
        return 0;
    constexpr char row[] = "f,dult;pbqrkvyjghcnea[wxio]sm'.z";
    const auto index = static_cast<std::size_t>(lower - 0x0430u);
    return index < sizeof(row) - 1 ? row[index] : 0;
}

void append_utf8(std::string& out, const unsigned code_point)
{
    if (code_point < 0x80u) {
        out += static_cast<char>(code_point);
    } else if (code_point < 0x800u) {
        out += static_cast<char>(0xC0u | (code_point >> 6));
        out += static_cast<char>(0x80u | (code_point & 0x3Fu));
    } else {
        out += static_cast<char>(0xE0u | (code_point >> 12));
        out += static_cast<char>(0x80u | ((code_point >> 6) & 0x3Fu));
        out += static_cast<char>(0x80u | (code_point & 0x3Fu));
    }
}

std::string fold(const char* text)
{
    std::string out;
    out.reserve(std::strlen(text));
    for (const char* p = text; *p;) {
        const auto lead = static_cast<unsigned char>(*p);
        if (lead < 0x80u) {
            out += static_cast<char>(std::tolower(lead));
            ++p;
            continue;
        }
        unsigned code_point = 0;
        int continuation = 0;
        if ((lead & 0xE0u) == 0xC0u) {
            code_point = lead & 0x1Fu;
            continuation = 1;
        } else if ((lead & 0xF0u) == 0xE0u) {
            code_point = lead & 0x0Fu;
            continuation = 2;
        } else if ((lead & 0xF8u) == 0xF0u) {
            code_point = lead & 0x07u;
            continuation = 3;
        } else {
            ++p;
            continue;
        }
        ++p;
        for (int i = 0; i < continuation && (static_cast<unsigned char>(*p) & 0xC0u) == 0x80u; ++i, ++p)
            code_point = code_point << 6 | (static_cast<unsigned char>(*p) & 0x3Fu);

        if (const char latin = cyrillic_to_latin(code_point)) {
            out += latin;
        } else {
            append_utf8(out, code_point); // unknown script stays as it is typed
        }
    }
    return out;
}

bool ascii_only(const char* text)
{
    for (const char* p = text; *p; ++p)
        if (static_cast<unsigned char>(*p) >= 0x80u)
            return false;
    return true;
}

} // namespace

void fold_query(const char* query, char* out, const size_t size)
{
    if (!query || !out || size == 0)
        return;
    const std::string folded = ascii_only(query) ? std::string(query) : fold(query);
    const size_t count = std::min(size - 1, folded.size());
    std::memcpy(out, folded.data(), count);
    out[count] = '\0';
}

bool matches(const char* text, const char* query)
{
    if (!query || !*query)
        return false;
    // Only pay for the fold when the query actually contains non-ASCII.
    char folded[192]{};
    const char* needle = query;
    if (!ascii_only(query)) {
        fold_query(query, folded, sizeof(folded));
        needle = folded;
    }
    const char* end = ImGui::FindRenderedTextEnd(text);
    const char* localized = l10n::text(text, end);
    if (localized != text && fold(localized).find(fold(query)) != std::string::npos)
        return true;
    for (const char* t = text; t < end; ++t) {
        const char* a = t;
        const char* b = needle;
        while (a < end && *b &&
               std::tolower(static_cast<unsigned char>(*a)) == std::tolower(static_cast<unsigned char>(*b))) {
            ++a;
            ++b;
        }
        if (!*b)
            return true;
    }
    return false;
}

} // namespace nl::search
