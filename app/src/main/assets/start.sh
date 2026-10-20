#!/system/bin/sh
set -eu
fail() { printf '%s\n' "$*" >&2; exit 1; }
[ "$(id -u)" = 0 ] || fail 'Запусти этот скрипт через root.'
SOURCE=$(CDPATH= cd -- "$(dirname -- "$0")" && pwd)
PACKAGE=com.axlebolt.standoff2

PIDS=$(pidof "$PACKAGE" 2>/dev/null || true)
if [ -z "$PIDS" ]; then
  printf '[HantoO] Запускаю игру...\n'
  monkey -p "$PACKAGE" -c android.intent.category.LAUNCHER 1 >/dev/null 2>&1 || am start -n "$PACKAGE/.UnityPlayerActivity" >/dev/null 2>&1 || fail 'Не удалось запустить игру'
  i=0
  while [ $i -lt 30 ]; do
    PIDS=$(pidof "$PACKAGE" 2>/dev/null || true)
    [ -n "$PIDS" ] && break
    i=$((i+1)); sleep 1
  done
  [ -n "$PIDS" ] || fail 'Игра не запустилась'
  printf '[HantoO] Игра запущена, жду 50 секунд загрузки...\n'
  sleep 50
fi

set -- $PIDS
[ "$#" = 1 ] || fail 'Найдено несколько процессов игры.'
GAME_PID=$1

cd "$SOURCE"
chmod 700 "$SOURCE/manualmap_injector"
chmod 600 "$SOURCE/libmain.so"

printf '[HantoO] Инжектирую чит в PID %s...\n' "$GAME_PID"
"$SOURCE/manualmap_injector" "$GAME_PID" "$SOURCE/libmain.so"
printf '[HantoO] Инжект завершен успешно! Ищи меню в игре.\n'
