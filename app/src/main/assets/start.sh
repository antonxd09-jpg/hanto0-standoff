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
[ "$#" = 1 ] || fail 'Найдено несколько процессов игры. Перезапусти игру.'
GAME_PID=$1
GAME_START=$(awk '{print $22}' "/proc/$GAME_PID/stat")
GAME_UID=$(awk '/^Uid:/{print $2}' "/proc/$GAME_PID/status")
ANDROID_USER=$((GAME_UID / 100000))

GAME_FILES="/data/data/$PACKAGE/files"
if [ ! -d "$GAME_FILES" ]; then
    GAME_FILES="/data/user/$ANDROID_USER/$PACKAGE/files"
fi

if [ ! -d "$GAME_FILES" ]; then
    mkdir -p "$GAME_FILES"
    chown "$GAME_UID:$GAME_UID" "$GAME_FILES"
    chmod 771 "$GAME_FILES"
fi

PACKAGE_DIR=$(dirname "$GAME_FILES")
GAME_LABEL=$(ls -Zd "$PACKAGE_DIR" | awk '{print $1}')
[ -n "$GAME_LABEL" ] || fail 'Не прочитан SELinux-контекст.'
chcon "$GAME_LABEL" "$GAME_FILES" 2>/dev/null || true

cd "$SOURCE"
chmod 600 "$SOURCE/libmain.so" 2>/dev/null || true

mkdir -p "$GAME_FILES/exodium"
chown "$GAME_UID:$GAME_UID" "$GAME_FILES/exodium"
chmod 700 "$GAME_FILES/exodium"
chcon "$GAME_LABEL" "$GAME_FILES/exodium" 2>/dev/null || true

cp "$SOURCE/cache" "$GAME_FILES/exodium/cache"
chown "$GAME_UID:$GAME_UID" "$GAME_FILES/exodium/cache"
chmod 600 "$GAME_FILES/exodium/cache"
chcon "$GAME_LABEL" "$GAME_FILES/exodium/cache" 2>/dev/null || true

for NAME in sssss3 sssss4 sssss5 sssss sssss2 sssss7 sssss8 sssss9 sssss1 sssss10; do
    FILE=$GAME_FILES/$NAME.txt
    [ -e "$FILE" ] || touch "$FILE"
    chown "$GAME_UID:$GAME_UID" "$FILE"
    chmod 600 "$FILE"
    chcon "$GAME_LABEL" "$FILE" 2>/dev/null || true
done

printf 'Remaster' > /data/local/tmp/version.txt
chmod 644 /data/local/tmp/version.txt

MARKER=$SOURCE/loaded.$GAME_PID.$GAME_START
if [ ! -f "$MARKER" ]; then
    printf '[HantoO] Инжектирую чит в PID %s...\n' "$GAME_PID"
    if "$SOURCE/manualmap_injector" "$GAME_PID" "$SOURCE/libmain.so" > "$SOURCE/injector.log" 2>&1; then
        printf 'loaded\n' > "$MARKER"
    else
        cat "$SOURCE/injector.log"
        fail 'Инжектор завершился с ошибкой.'
    fi
fi
[ -d "/proc/$GAME_PID" ] || fail 'Игра завершилась после загрузки библиотеки.'

rm -f "$SOURCE/rest.ready" "$SOURCE/rest.exit"
nohup /system/bin/sh -c '
    RC=$?
    printf "%s\n" "$RC" > "$1/rest.exit"
' exodium-worker "$SOURCE" > /dev/null 2>&1 < /dev/null &
WORKER_PID=$!
printf '%s\n' "$WORKER_PID" > "$SOURCE/worker.pid"

ATTEMPT=0
while [ "$ATTEMPT" -lt 30 ]; do
    if [ -s "$SOURCE/rest.ready" ]; then
        printf '[HantoO] Меню готово.\n'
        exit 0
    fi
    if [ -f "$SOURCE/rest.exit" ]; then
        cat "$SOURCE/rest.log"
        fail "Меню завершилось с кодом $(cat "$SOURCE/rest.exit")."
    fi
    ATTEMPT=$((ATTEMPT + 1))
    sleep 1
done
tail -n 30 "$SOURCE/rest.log"
fail 'Меню не подтвердило готовность за 30 секунд.'
