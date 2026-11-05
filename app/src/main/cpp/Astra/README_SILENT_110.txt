ASTRA — SILENT 1.1.0 ARM64

Added Silent tab beside Visuals / Config. Normal Aim, Anti-Aim and automation remain hidden.
Silent uses separate features/aim/silent_offsets.hpp; normal Aim offsets are unchanged.
Shot emission: kaa.tfqa, BL inside kaa.edjj.
Command: eyc.uduz / eyc.dfee, ubz.syha; camera mzox call to syha.
Hitbox collection: bis / mtc. WeaponContext HitTransform +0x18, ActiveWeapon +0x50.
Collider bounds / raycast use verified managed injected wrappers, preserving native pointer ABI.
PhysicsScene: 8 bytes; RaycastHit: 44 bytes; EntityId: 4 bytes.
Silent validation no longer depends on unported normal camera functions.
Command look must be installed before the command backend can report ready.
Normal automation is bypassed on the Silent-only path.
Recoil compensation and no-recoil/spread controls are deliberately unavailable pending their separate port.
Config exposes shot, command and look hooks, visibility, last result, callbacks, rays,
commands, targets, hitboxes, validation failures and Save silent diagnostics.
Counters describe local execution; they are not server damage confirmation.

Verification completed:
- host g++ C++17 syntax check of the actual runtime with ARM64 code paths enabled;
- host syntax check of actual Silent / Config / routing functions;
- verify_silent_110.py: input hash, method identities, exact guards, four BL targets, layout;
- verify_offsets_110.py: existing Visuals profile checks;
- verify_gui.py: actual Dear ImGui watermark interaction and three-tab route checks.
No Android build or on-device test was performed; no new libastra.so is supplied.
The input's obj binary is omitted so an old build cannot be mistaken for this revision.
The original build.sh references absent scripts/build.sh; use Android NDK directly:
    ndk-build NDK_PROJECT_PATH=. APP_BUILD_SCRIPT=jni/Android.mk NDK_APPLICATION_MK=jni/Application.mk
Checks:
    python checks/verify_silent_110.py /path/to/1.1.0/dump
    python checks/verify_offsets_110.py /path/to/1.1.0/dump
    python checks/verify_gui.py

Dumps are unchanged and not duplicated in this source-only archive.
