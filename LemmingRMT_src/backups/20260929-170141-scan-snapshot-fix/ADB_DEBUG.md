# LemmingRMT — ADB debugging

The native library writes structured messages with the single logcat tag `LemmingRMT`.

## Quick start

```bash
cd /home/leftcode/Projects/LemmingRMT
./adb_logs.sh clear
./adb_logs.sh follow
```

The default Android package is `com.axlebolt.standoff2`. Pass another package as the second argument when needed:

```bash
./adb_logs.sh follow your.package.name
```

## On-demand diagnostics

The library installs two signal handlers:

- `SIGUSR1`: toggle the menu;
- `SIGUSR2`: print renderer, hooks, profile and ESP diagnostics.

Helpers:

```bash
./adb_logs.sh toggle
./adb_logs.sh diag
./adb_logs.sh snapshot
```

Equivalent direct commands:

```bash
adb shell su -c 'kill -USR1 $(pidof com.axlebolt.standoff2)'
adb shell su -c 'kill -USR2 $(pidof com.axlebolt.standoff2)'
adb logcat -v threadtime LemmingRMT:D '*:S'
```

## Important log records

- `bootstrap started`: the injected library constructor ran;
- `resolved originals`: EGL and native-input symbols were found;
- `hook status`: number of patched swap/damage/input slots;
- `ARM64 profile`: `libunity.so` base and profile range check;
- `renderer ready`: active EGL context, GLES version, GLSL version and GPU;
- `render heartbeat`: the swap hook is executing and frames are being submitted;
- `ESP status`: resolver state transitions;
- `ESP heap scan complete`: one bounded scan cycle finished;
- `ESP frame`: cached objects, snapshots, local team, projected enemies, valid stats and weapon reads;
- `diagnostics[...]`: complete one-shot state requested by `SIGUSR2`.

## Triage

### No menu and no `bootstrap started`

The library was not loaded, was loaded into the wrong process, or failed before its constructor.

### `bootstrap started`, but `swap=0` and `present=0`

The injection happened before/after an unexpected renderer path, or the game is using Vulkan. The current menu renderer is GLES3/EGL only.

### Swap hook exists, but `GLES client version` is below 3

This binary is compiled with the Dear ImGui GLES3 backend. Use the game's GLES3 renderer or build a dedicated GLES2 backend.

### `profile ready=0`

The running game/library does not match the Standoff 2 1.0.0 ARM64 profile, or the process is not ARM64.

### `TypeInfo игрока ещё не готов`

Wait until IL2CPP metadata initialization completes. If it persists in a match, collect a `snapshot`; the build or TypeInfo slot likely differs.

### ESP scans but projects zero players

Collect `SIGUSR2` diagnostics while inside a live match. Check `localTeam`, the `ake/ylw/nrk/qjf` class pointers, stats/weapon read counts, `ESP heap scan complete`, camera state and the active profile. The enemy renderer intentionally draws nothing until the local-owned player team is resolved.

## Current ESP scope

The current ESP is read-only. It uses the validated ARM64 `ake` player TypeInfo, a bounded incremental heap scan, the `mfc+0x44` local-owned flag, `ake+0x90` team byte, Unity transform helpers and WorldToScreen. It draws enemy boxes plus guarded HP/armor and raw weapon ID/magazine/reserve labels. Dead players with validated zero HP are skipped.

Weapon labels intentionally use `W#<raw-id>` until the static `WeaponIds` value mapping is independently confirmed. Normal-aim and silent-aim writes are not enabled.
