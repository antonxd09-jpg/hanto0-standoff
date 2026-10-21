# LemmingRMT

ARM64-only internal GLES3 overlay for Standoff 2 1.0.0.

## Build

```bash
cd /home/leftcode/Projects/LemmingRMT
./build.sh clean
./build.sh
```

Current output:

```text
out/arm64-v8a/liblemmingrmt.so
```

`armeabi-v7a` is intentionally disabled. The active profile is `Standoff2-1.0.0-arm64-il2cppdump-v8-native-camera-cache`.

## v8 player collection

The old sample RVAs `0x912241C` and `0x91270BC` belong to an older build and are not used.
For the matching 1.0.0 ARM64 binary:

- player object: `ake` (`mfc` NetObject base);
- update callback: `ake::opmn()` at RVA `0x949E8A0`;
- local-owned byte: `mfc + 0x44`;
- position: `ake::xdzx()` at RVA `0x9489018`;
- player update interception: validated `ake` IL2CPP vtable slot, not an inline trampoline;
- fallback for already-existing players: `njh -> gat -> gzx -> yjk<krm>` NetObject registry.

The renderer uses a mutex-protected snapshot of hooked `ake` pointers. Stale pointers are pruned with guarded reads before HP/team/weapon/position processing. Until at least two valid hook entities are present, the NetObject registry remains the bootstrap fallback.

## Runtime log markers

- `ESP ake update hook ready` — the current `ake::opmn` vtable slot was found and replaced;
- `ESP ake update hook entity` — a player pointer was collected;
- `ESP ake update hook snapshot` — hook calls and valid tracked players;
- `ESP player frame: source=ake-update-hook` — ESP is consuming the hook list;
- `source=netobject-registry` — registry fallback is still active.

Gameplay-state writes, normal aim, and silent-aim writes remain disabled. The hook changes only the `ake` class method pointer and always forwards to the original method.

See `docs/ADB_DEBUG.md` for runtime checks.

## v8 ESP runtime fix

- `rct+0x10` is `List<eur>`; the backing array is at `List+0x10` and size at `List+0x18`.
- Enemy team prefers `WeaponContext.Team` at `nrk+0x38`, with `ake+0x90` retained as a fallback.
- Unity 6000.3 virtual dispatch uses `Il2CppClass+0x1A0`; the hook scans bounded entries for the exact `ake::opmn` pointer.
- Menu status labels are ASCII so the default ImGui font does not render `??????`.


### Native camera cache fix (2026-09-30)

This build removes the incorrect GC-handle-as-object camera unwrap. Native
Camera::GetMain (0x54C1338) is used only on UnityMain; safe matrix copies and
main-thread player position snapshots are consumed by the render path.
Direct camera matrix getters, W2S, and render-thread position getters are off.
Only ARM64 is built. Live runtime stability and visible ESP still require testing.


### Presentation sync (2026-09-30)

The reported working native-camera-cache build had a 33ms screen-box throttle.
The new present-sync build recomputes projection before every EGL presentation.
A leased native camera identity is published by UnityMain; its cached matrix
values are double-copied safely at presentation time without calling Unity.
No old screen boxes are drawn on a failed update. The overlay is still Dear
ImGui in the game's own swap hook, NOT Unity scene geometry or an external app.
Exact GPU frame/view-projection matching is not claimed: if render queues still
cause residual drift, capture the rendering pass's own camera matrices next.


## Premium UI / ARM64 (2026-09-30)
- Main rectangle is screen-centered; category and subcategory rails are independent panels to its left.
- Centered top watermark is the only menu open/close control. No title bar, X, close button, screen diagnostics, or SIGUSR1 menu toggle. ADB diagnostics/SIGUSR2 are retained.
- Exponential menu/page/switch animations; frame-independent and cosmetic only.
- Visuals > Players: master enemy ESP, box, skeleton, HP, armor, weapon ID, ammo, soft fill; full/corner frame.
- Visuals > Appearance and Settings > Theme: live RGBA pickers; line weight. Colors are held for this process only.
- Aim/Chams/World/Misc are clearly marked reserved; no aim, chams, movement or world features implemented.
- Lucide SVG assets fetched from the official project, compiled as vector lines, no game-time networking. Licensing: assets/lucide/LICENSE.
- Skeleton is opt-in (off initially): actual jgi+0x30 > CharacterBindings+0x20 > BonesDictionary > m_dict/_entries, with metadata-discovered field offsets, serialized-array fallback. ARM64 Dictionary<int,Transform> Entry stride 0x18. Invalid/cyclic/stale hierarchies are skipped, not replaced by mannequin data.
- Bone native TransformHierarchy: managed cached ptr +0x10, native hierarchy +0x28, index +0x30; hierarchy TRS array +0x18 (stride 0x30), parent indices +0x20 (stride 4). Confirmed from native GetPositionAndRotation at 0x554A610/0x554A634. Reader uses safe kernel copies, no new Unity getter calls. Capture only UnityMain after original callback, 25ms cadence, 150ms expiry, world identity check, 128-parent traversal bound and cycle/finite/quaternion/proximity checks.
- Fresh present-camera projection is preserved; no pixel smoothing or 30 FPS screen cache.
- Restart the game before loading the uniquely named SO; do not stack libraries in one process. New skeleton path still requires live device validation.
