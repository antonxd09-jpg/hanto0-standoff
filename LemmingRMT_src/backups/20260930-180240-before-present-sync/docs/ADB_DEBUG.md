# LemmingRMT — ADB debugging (v8 ake update hook)

Active profile: `Standoff2-1.0.0-arm64-il2cppdump-v8-native-camera-cache`.

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

1. `bootstrap started: build=...v8-native-camera-cache`
2. `ESP resolver ready`
3. `ESP ake update hook ready: reason=exact-methodPtr RVA=0x949e8a0`
4. In a live match: one or more `ESP ake update hook entity`
5. `ESP ake update hook snapshot: calls=N tracked=M`
6. `ESP player frame: source=ake-update-hook ... projected=P`

The NetObject path remains a bootstrap fallback, so `ESP NetObject root scan`, `registry ready`, and `snapshot` may appear before the hook has at least two players.

## Triage

- No `hook ready`: capture the `invalid vtable count`, `absent in ... slots`, or `slot is not writable` line.
- `hook ready`, but `calls=0`: `ake::opmn()` was not invoked in the tested scene; enter a live match and respawn once.
- `calls>0`, but `tracked=0`: capture the first crash/log window; the callback ABI needs re-checking.
- `tracked>0`, but `localTeam=0`: verify that one collected object has local-owned byte at `mfc+0x44` and team at `ake+0x90`.
- `enemies>0`, but `projected=0`: inspect the native-camera-cache rejection reasons, position snapshots and matrix projection diagnostics.
- Crash: use `./adb_logs.sh crash` and attribute it to this build only if the exact `v8-native-camera-cache` marker appears in the same process lifetime.

On this non-root/non-debuggable device, shell signals can be denied. Use the menu diagnostics button when needed.


## 2026-09-30 native-camera-cache correction

- The previous build installed the hook successfully on UnityMain. Crash was in
  camera matrix capture, not hook installation: 0x536520C -> 0x5A7C2FC -> 0x55493B0.
- 0x53653C4 returns a GC handle (native Camera+0x18), not a managed Camera*.
  Its managed wrapper at 0x6D28F70 explicitly converts the handle through 0x67EC810.
- Active camera source is native Camera::GetMain, RVA 0x54C1338.
- Matrix getters and direct W2S are disabled on every thread. Matrix cache at
  native Camera+0x70 (view) and +0xB0 (projection) is copied through the safe reader.
- Position getter runs only on UnityMain after the original ake::opmn callback,
  after validating the parts list and dwc part. Render consumes cached positions.
- Expected logs: `ESP ake update hook ready`, `ESP UnityMain position snapshot`,
  `ESP UnityMain camera snapshot: source=native-cache`, `ESP player frame ... projected=N`.
- Rejected camera samples include a reason and pointer. No sample => no drawing.
- Build success and offline tests do NOT prove runtime stability/visible ESP.
  A new live ADB match test is required.
