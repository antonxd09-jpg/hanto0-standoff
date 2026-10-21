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

- Текущий UI-каркас не использует IL2CPP offsets, поэтому он собирается для обеих архитектур независимо от архитектуры предоставленного дампа.
- Для будущей IL2CPP-функциональности дамп должен совпадать с архитектурой и версией установленной игры; текущий дамп имеет пометку x32.
- Автоматическое открытие/закрытие касанием зависит от того, проходит ли ввод конкретной сборки через `AInputQueue_getEvent`. Если нет, загрузчик может вызвать экспорт `LemmingRMT_ToggleMenu()`.
- Способ доставки/загрузки библиотеки в процесс в проект не включён.


## ARM64 dump profile

The project now embeds `jni/game_offsets.hpp`, generated from `/home/leftcode/Downloads/dump (1).cs` (SHA-256 `ecb17199f3219949dd0884ab0977dd96894ba81daa719bfd3d3de9a3dde7296a`). The overlay resolves the `libunity.so` base and reports whether the known ARM64 RVA profile falls inside the loaded image. The detailed profile is in `docs/Standoff2_1.0.0_ARM64_first_offsets_from_dump.txt`.

No direct player or weapon memory writes are enabled yet: the dump marks most relevant methods as `RVA: -1`, and the active runtime object chain still needs independent validation.
