# LemmingRMT — ADB debugging (v8 ake update hook)

Active profile: `Standoff2-1.0.0-arm64-il2cppdump-v8-method-slot-camera-snapshot`.

## Quick start

```bash
cd /home/leftcode/Projects/LemmingRMT
./adb_logs.sh clear
./adb_logs.sh follow
```

Use only:

```text
out/arm64-v8a/liblemmingrmt.so
```

## Expected sequence

1. `bootstrap started: build=...v8-ake-update-hook`
2. `ESP resolver ready`
3. `ESP ake update hook ready: method=ake::opmn RVA=0x949e8a0`
4. In a live match: one or more `ESP ake update hook entity`
5. `ESP ake update hook snapshot: calls=N tracked=M`
6. `ESP player frame: source=ake-update-hook ... projected=P`

The NetObject path remains a bootstrap fallback, so `ESP NetObject root scan`, `registry ready`, and `snapshot` may appear before the hook has at least two players.

## Triage

- No `hook ready`: capture the `invalid vtable count`, `absent in ... slots`, or `slot is not writable` line.
- `hook ready`, but `calls=0`: `ake::opmn()` was not invoked in the tested scene; enter a live match and respawn once.
- `calls>0`, but `tracked=0`: capture the first crash/log window; the callback ABI needs re-checking.
- `tracked>0`, but `localTeam=0`: verify that one collected object has local-owned byte at `mfc+0x44` and team at `ake+0x90`.
- `enemies>0`, but `projected=0`: inspect `ake::xdzx()` and Camera/WorldToScreen diagnostics.
- Crash: use `./adb_logs.sh crash` and attribute it to this build only if the exact `v8-ake-update-hook` marker appears in the same process lifetime.

On this non-root/non-debuggable device, shell signals can be denied. Use the menu diagnostics button when needed.
