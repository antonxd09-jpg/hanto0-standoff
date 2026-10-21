# LemmingRMT — ADB debugging (v8-netobject)

Активный профиль: `Standoff2-1.0.0-arm64-il2cppdump-v8-netobject`. Библиотека пишет сообщения с tag `LemmingRMT`.

## Quick start

```bash
cd /home/leftcode/Projects/LemmingRMT
./adb_logs.sh clear
./adb_logs.sh follow
```

По умолчанию используется package `com.axlebolt.standoff2`. Путь к ADB можно переопределить:

```bash
ADB=/path/to/adb ./adb_logs.sh follow
```

## Важные v8 записи

- `bootstrap started`: constructor библиотеки выполнился;
- `profile=...v8-netobject`: подтверждает, что реально загружена текущая сборка;
- `renderer ready` / `render heartbeat`: swap hook и меню исполняются;
- `ESP resolver ready`: TypeInfo `njh/gat/gzx/mfc/ake` разрешены;
- `ESP NetObject root scan`: безопасный одноразовый bootstrap-поиск ClientWorld/registry;
- `ESP NetObject registry ready`: найден валидный `gat`, его `gzx` store и ClientWorld;
- `ESP NetObject snapshot`: `total` — все `krm`, `ake` — player NetObjects;
- `ESP NetObject frame`: snapshots, local team, enemies, projected, HP/weapon reads;
- `ESP NetObject registry invalidated`: store/list стал stale, после чего root ищется заново;
- `ESP diagnostics[...]`: полный snapshot renderer/input/NetObject состояния.

## Что проверять

### Краш

```bash
./adb_logs.sh crash
```

Сначала убедитесь, что перед падением есть точный маркер `v8-netobject`. Backtrace без этого маркера нельзя приписывать текущей сборке.

### Меню есть, но `World objects: 0`

Смотрите `ESP NetObject root scan`. Если `registry ready` не появляется после полного цикла, сохраните logcat от `bootstrap started` до `root scan complete`.

### `World objects > 0`, но `Player NetObjects: 0`

Registry найден, но в текущем world snapshot нет объектов exact class `ake`. Снимите лог внутри живого матча, а не в главном меню.

### `Player NetObjects > 0`, но `Enemies: 0`

Проверяйте `localTeam`, `snapshots`, `stats` и `weapon`. Рендер намеренно ждёт local-owned игрока и его team byte, затем исключает союзников и мёртвых игроков.

### `Enemies > 0`, но `projected=0`

NetObject traversal уже работает; проблема дальше по цепочке — `ake::xdzx()`, camera или WorldToScreen. Нужна строка `ESP NetObject frame` с camera pointer.

## Signals

`SIGUSR1` переключает меню, `SIGUSR2` печатает диагностику. На non-root/non-debuggable устройстве ADB shell не может послать signal процессу приложения — это нормально. Используйте кнопку диагностики в меню и обычный `follow`/`dump`; `adb_logs.sh diag` сообщит, если permission отсутствует.

## Scope

ESP read-only. Источник объектов: `ClientWorld/NetObject registry`, путь `njh+0x40 -> gat+0x30 -> gzx+0x18 -> yjk<krm>`. Игроки фильтруются по `ake`, позиция берётся из `ake::xdzx()`, HP/team/weapon/ammo читаются с guarded offsets. Normal aim и silent aim writes отключены.
