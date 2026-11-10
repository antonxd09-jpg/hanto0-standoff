// Menu source watermark: @use_sberbank
#pragma once

#include "imgui.h"

namespace nl {

struct Fonts {
    ImFont* medium = nullptr;
    ImFont* semibold = nullptr;
    ImFont* black = nullptr;
    ImFont* icons = nullptr;
};

// Font Awesome 6 Free (Solid) code points used by the menu.
namespace icon {
inline constexpr const char* crosshairs = "\xef\x98\x81"; // U+F601 location-crosshairs
inline constexpr const char* mouse      = "\xef\xa3\x8c"; // U+F8CC computer-mouse
inline constexpr const char* image      = "\xef\x80\xbe"; // U+F03E image
inline constexpr const char* layers     = "\xef\x97\xbd"; // U+F5FD layer-group
inline constexpr const char* floppy     = "\xef\x83\x87"; // U+F0C7 floppy-disk
inline constexpr const char* search     = "\xef\x80\x82"; // U+F002 magnifying-glass
inline constexpr const char* check      = "\xef\x80\x8c"; // U+F00C check
inline constexpr const char* user       = "\xef\x80\x87"; // U+F007 user
inline constexpr const char* user_minus = "\xef\x94\x83"; // U+F503 user-minus
inline constexpr const char* earth      = "\xef\x95\xbd"; // U+F57D earth-americas
inline constexpr const char* globe      = "\xef\x82\xac"; // U+F0AC globe
inline constexpr const char* xmark      = "\xef\x80\x8d"; // U+F00D xmark
inline constexpr const char* zoom_in    = "\xef\x80\x8e"; // U+F00E magnifying-glass-plus
inline constexpr const char* expand     = "\xef\x81\xa5"; // U+F065 expand
inline constexpr const char* check_double = "\xef\x95\xa0"; // U+F560 check-double
inline constexpr const char* bomb       = "\xef\x87\xa2"; // U+F1E2 bomb
inline constexpr const char* scissors   = "\xef\x83\x84"; // U+F0C4 scissors
inline constexpr const char* cloud      = "\xef\x83\x82"; // U+F0C2 cloud
inline constexpr const char* flame      = "\xef\x91\xaa"; // U+F46A fire-flame-simple
inline constexpr const char* droplet    = "\xef\x81\x83"; // U+F043 droplet
inline constexpr const char* burst      = "\xee\x93\x9c"; // U+E4DC burst
inline constexpr const char* scale      = "\xef\xa2\x85"; // U+F885 arrow-up-short-wide
inline constexpr const char* text_height = "\xef\x80\xb4"; // U+F034 text-height
inline constexpr const char* windows    = "\xef\x8b\x92"; // U+F2D2 window-restore
inline constexpr const char* ruler      = "\xef\x95\x86"; // U+F546 ruler-combined
inline constexpr const char* fill_drip  = "\xef\x95\xb6"; // U+F576 fill-drip
inline constexpr const char* gear       = "\xef\x80\x93"; // U+F013 gear
inline constexpr const char* shield     = "\xef\x8f\xad"; // U+F3ED shield-halved
inline constexpr const char* display    = "\xee\x85\xa3"; // U+E163 display
inline constexpr const char* copyright  = "\xef\x87\xb9"; // U+F1F9 copyright
inline constexpr const char* comments   = "\xef\x82\x86"; // U+F086 comments
inline constexpr const char* list       = "\xef\x80\xa2"; // U+F022 rectangle-list
inline constexpr const char* poll       = "\xef\x9a\x82"; // U+F682 square-poll-horizontal
inline constexpr const char* sticker    = "\xef\x89\x89"; // U+F249 note-sticky
inline constexpr const char* certificate = "\xef\x82\xa3"; // U+F0A3 certificate
inline constexpr const char* hand       = "\xef\x89\x96"; // U+F256 hand
inline constexpr const char* sun        = "\xef\x86\x85"; // U+F185 sun
inline constexpr const char* keyboard   = "\xef\x84\x9c"; // U+F11C keyboard
inline constexpr const char* trash      = "\xef\x8b\xad"; // U+F2ED trash-can
inline constexpr const char* eye        = "\xef\x81\xae"; // U+F06E eye
inline constexpr const char* link       = "\xef\x83\x81"; // U+F0C1 link
inline constexpr const char* list_ul    = "\xef\x83\x8a"; // U+F0CA list-ul
inline constexpr const char* circle_plus = "\xef\x81\x95"; // U+F055 circle-plus
inline constexpr const char* rotate_left = "\xef\x8b\xaa"; // U+F2EA rotate-left
inline constexpr const char* rotate      = "\xef\x8b\xb1"; // U+F2F1 arrows-rotate
inline constexpr const char* folder_open = "\xef\x81\xbc"; // U+F07C folder-open
inline constexpr const char* toggle_on  = "\xef\x88\x85"; // U+F205 toggle-on
inline constexpr const char* snowflake  = "\xef\x8b\x9c"; // U+F2DC snowflake
inline constexpr const char* arrow_down = "\xef\x81\xa3"; // U+F063 arrow-down
inline constexpr const char* infinity   = "\xef\x94\xb4"; // U+F534 infinity
inline constexpr const char* bolt       = "\xef\x83\xa7"; // U+F0E7 bolt
inline constexpr const char* bullseye   = "\xef\x85\x80"; // U+F140 bullseye
inline constexpr const char* stopwatch  = "\xef\x8b\xb2"; // U+F2F2 stopwatch
inline constexpr const char* person_running = "\xef\x9c\x8c"; // U+F70C person-running
inline constexpr const char* clock      = "\xef\x80\x97"; // U+F017 clock
inline constexpr const char* gauge_high = "\xef\x98\xa5"; // U+F625 gauge-high
inline constexpr const char* signal     = "\xef\x80\x92"; // U+F012 signal
inline constexpr const char* chart_simple = "\xee\x91\xb3"; // U+E473 chart-simple
inline constexpr const char* skull      = "\xef\x95\x8c"; // U+F54C skull
inline constexpr const char* gamepad    = "\xef\x84\x9b"; // U+F11B gamepad
inline constexpr const char* microchip  = "\xef\x8b\x9b"; // U+F2DB microchip
inline constexpr const char* wave_square = "\xef\xa0\xbe"; // U+F83E wave-square
inline constexpr const char* memory     = "\xef\x94\xb8"; // U+F538 memory
inline constexpr const char* chart_line = "\xef\x88\x81"; // U+F201 chart-line
inline constexpr const char* wifi       = "\xef\x87\xab"; // U+F1EB wifi
inline constexpr const char* link_slash = "\xef\x84\xa7"; // U+F127 link-slash
inline constexpr const char* server     = "\xef\x88\xb3"; // U+F233 server
inline constexpr const char* upload     = "\xef\x82\x93"; // U+F093 upload
inline constexpr const char* file_import = "\xef\x95\xaf"; // U+F56F file-import
inline constexpr const char* plus       = "\xef\x81\xa7"; // U+F067 plus
inline constexpr const char* sort       = "\xef\x85\xa0"; // U+F160 arrow-down-wide-short
inline constexpr const char* cloud_down = "\xef\x83\xad"; // U+F0ED cloud-arrow-down
inline constexpr const char* pen        = "\xef\x8c\x84"; // U+F304 pen
inline constexpr const char* copy       = "\xef\x83\x85"; // U+F0C5 copy
inline constexpr const char* gun        = "\xee\x86\x9b"; // U+E19B gun
inline constexpr const char* grip       = "\xef\x96\x8e"; // U+F58E grip-vertical
inline constexpr const char* arrows     = "\xef\x82\xb2"; // U+F0B2 up-down-left-right
inline constexpr const char* heart      = "\xef\x80\x84"; // U+F004 heart
inline constexpr const char* lock       = "\xef\x80\xa3"; // U+F023 lock
inline constexpr const char* eye_slash  = "\xef\x81\xb0"; // U+F070 eye-slash
inline constexpr const char* minus      = "\xef\x81\xa8"; // U+F068 minus
inline constexpr const char* play       = "\xef\x81\x8b"; // U+F04B play
inline constexpr const char* circle_check = "\xef\x81\x98"; // U+F058 circle-check

// Not in Font Awesome Free: drawn with shapes by draw::any_icon().
inline constexpr const char* sparkles      = "\x01";
inline constexpr const char* dashed_circle = "\x02";
inline constexpr const char* rings         = "\x03";
inline constexpr const char* key_bind      = "\x04"; // sun rising over a line: hotkey
inline constexpr const char* list_tree     = "\x05";
inline constexpr const char* keyboard_down = "\x06"; // keyboard with a caret under it
inline constexpr const char* grid_plus     = "\x07"; // three squares and a plus: saved colors
} // namespace icon

// Loads the embedded fonts into io.Fonts. Call once after ImGui::CreateContext().
void load_fonts();
void adopt_fonts(ImFont*,ImFont*,ImFont*,ImFont*);
const Fonts& fonts();

} // namespace nl
