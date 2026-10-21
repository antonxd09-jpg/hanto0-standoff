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

`armeabi-v7a` is intentionally disabled. The active profile is `Standoff2-1.0.0-arm64-il2cppdump-v8-method-slot-camera-snapshot`.

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
