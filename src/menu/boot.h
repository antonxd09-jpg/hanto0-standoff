// Menu source watermark: @use_sberbank
#pragma once

// What the user sees between the injection and the menu: a small Neverlose window with a
// progress bar while the heavy assets load, then the corner notification once the menu is up.
// Both draw on the foreground list, so they work whether the menu is open or not.
namespace nl::boot {

// Draws the loading window. `progress` is 0..1; once `done` is set the bar fills, the window
// holds for a moment and fades out. Returns true when it has fully disappeared.
bool loading(float progress, const char* status, bool done);

// Queues a corner notification with the NL logo ("Successfully injected!").
void notify(const char* text);
// Draws the queued notifications; call every frame.
void notifications();

} // namespace nl::boot
