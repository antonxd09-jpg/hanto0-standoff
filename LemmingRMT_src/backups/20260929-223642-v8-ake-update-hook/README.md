# LemmingRMT

Минимальный internal-overlay каркас для ARMv7 и ARM64.

## Что уже есть

- две нативные сборки `liblemmingrmt.so`: `armeabi-v7a` и `arm64-v8a`;
- GLES3-оверлей с меню и ватермаркой **LemmingRMT**;
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

## ARM64 dump profile v8-netobject

Активный профиль: `Standoff2-1.0.0-arm64-il2cppdump-v8-netobject`.
Он сверён с `/home/leftcode/Projects/dump 1.0.0`, matching ARM64 `libunity.so` и APK 1.0.0.

### Что изменено в ESP

- старый постоянный поиск каждого `ake` по всему heap удалён;
- выполняется только безопасный bootstrap-поиск одного `ClientWorld` (`njh`) или registry (`gat`);
- после нахождения root данные берутся через цепочку `njh+0x40 -> gat+0x30 -> gzx+0x18 -> yjk<krm>`;
- player-объекты фильтруются из общего NetObject массива по точному TypeInfo `ake`;
- позиция читается через dump-confirmed `ake::xdzx()` (`RVA 0x9489018`), а не через guessed transform walk;
- все pointer/layout reads используют `process_vm_readv` с fallback на `/proc/self/mem`;
- меню показывает `Enemies`, `Player NetObjects` и общее число world objects;
- ADB-логи содержат root scan, registry/store pointers, общее число NetObjects, число `ake`, snapshots, enemies и projected.

Runtime остаётся read-only: игровые memory writes, normal aim и silent aim отключены. Проверка на телефоне должна выполняться именно по маркеру `v8-netobject`; старые записи logcat нельзя использовать для диагностики этой сборки.

## Сборка и ADB

```bash
cd /home/leftcode/Projects/LemmingRMT
./build.sh
./adb_logs.sh clear
./adb_logs.sh follow
```

Для подключённого ARM64 устройства используется:

```text
out/arm64-v8a/liblemmingrmt.so
```

См. `docs/ADB_DEBUG.md` для интерпретации записей `ESP NetObject ...` и crash buffer.

