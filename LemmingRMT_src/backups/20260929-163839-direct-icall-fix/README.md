# LemmingRMT

Минимальный internal-overlay каркас для ARMv7 и ARM64.

## Что уже есть

- две нативные сборки `liblemmingrmt.so`: `armeabi-v7a` и `arm64-v8a`;
- GLES2-оверлей с меню и ватермаркой **LemmingRMT**;
- кнопка `X` закрывает меню;
- нажатие на ватермарку снова открывает меню;
- PLT-hook `eglSwapBuffers` с поддержкой ELF32/REL и ELF64/RELA;
- обработка касаний через hook `AInputQueue_getEvent`;
- резервные экспорты управления меню:
  - `LemmingRMT_ToggleMenu()`;
  - `LemmingRMT_SetMenuOpen(int)`;
  - `LemmingRMT_IsMenuOpen()`.

## Сборка

```bash
cd /home/leftcode/Projects/LemmingRMT
./build.sh
```

Результаты:

```text
out/armeabi-v7a/liblemmingrmt.so
out/arm64-v8a/liblemmingrmt.so
```

Для 64-битного процесса и 64-битного AndKittyInjector нужен файл:

```text
out/arm64-v8a/liblemmingrmt.so
```

Очистка:

```bash
./build.sh clean
```

## Примечания

- UI собирается для обеих ABI, но профиль чтения игровых объектов активируется только в ARM64-процессе.
- Активный dump должен точно совпадать с архитектурой и версией игры; текущий профиль проверен для Standoff 2 1.0.0 ARM64.
- Автоматическое открытие/закрытие касанием зависит от того, проходит ли ввод конкретной сборки через `AInputQueue_getEvent`. Если нет, загрузчик может вызвать экспорт `LemmingRMT_ToggleMenu()`.
- Способ доставки/загрузки библиотеки в процесс в проект не включён.


## ARM64 dump profile

The project embeds `jni/game_offsets.hpp` using the matching `/home/leftcode/Projects/dump 1.0.0` artifacts and ARM64 `libunity.so`. The active profile is `Standoff2-1.0.0-arm64-il2cppdump-v4-threadfix`; source hashes, TypeInfo slots, layouts and validation anchors are recorded in `Standoff2_1.0.0_ARM64_internal_offsets.txt`.

The runtime remains read-only: direct player, input-command, camera and weapon writes are disabled. Device-side ADB validation is still required even for dump-confirmed reads.


## 1.0.0 ARM64 renderer / ESP debug update

The active profile is `Standoff2-1.0.0-arm64-il2cppdump-v4-threadfix`, cross-checked against `dump.cs`, `script.json` and ARM64 instructions from the matching `libunity.so`. Every TypeInfo slot from the older `apk-ida-v2` profile is revoked.

Changes:

- rebuilt Dear ImGui GLES3 renderer with context/surface recreation handling;
- hooks `eglSwapBuffers` plus KHR/EXT damage-present variants;
- responsive menu with renderer counters and a logcat diagnostic button;
- read-only enemy ESP using `ake`, local-owned/team filtering, HP/armor, raw weapon ID and magazine/reserve ammo;
- `SIGUSR1` toggles the menu and `SIGUSR2` emits a full diagnostic snapshot;
- unverified Java `RegisterNatives` input hook is disabled by default; native input and signal controls remain available.

Build:

```bash
cd /home/leftcode/Projects/LemmingRMT
./build.sh
```

ADB diagnostics:

```bash
./adb_logs.sh follow
./adb_logs.sh snapshot
```

See `docs/ADB_DEBUG.md` for log interpretation. The renderer is EGL/GLES3; a Vulkan game session will report no intercepted present calls. HP/team/weapon/ammo reads are enabled with strict pointer/range guards, while normal-aim and silent-aim writes remain disabled pending runtime serializer/tick validation.
