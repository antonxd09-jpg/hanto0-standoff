#!/system/bin/sh
set -eu
fail() { printf '%s\n' "$*" >&2; exit 1; }
[ "$(id -u)" = 0 ] || fail 'Запусти этот скрипт через root.'
SOURCE=$(CDPATH= cd -- "$(dirname -- "$0")" && pwd)
RUNTIME=/data/local/tmp/hantoo-local
PACKAGE=com.axlebolt.standoff2
PIDS=$(pidof "$PACKAGE" 2>/dev/null || true)
[ -n "$PIDS" ] || fail 'Сначала запусти Standoff 2.'
set -- $PIDS
[ "$#" = 1 ] || fail 'Найдено несколько процессов игры. Перезапусти игру.'
GAME_PID=$1
GAME_START=$(awk '{print $22}' "/proc/$GAME_PID/stat")
GAME_UID=$(awk '/^Uid:/{print $2}' "/proc/$GAME_PID/status")
ANDROID_USER=$((GAME_UID / 100000))
GAME_FILES=/data/user/$ANDROID_USER/$PACKAGE/files
[ -d "$GAME_FILES" ] || fail "Не найдена папка игры: $GAME_FILES"
GAME_LABEL=$(ls -Zd "$GAME_FILES" | awk '{print $1}')
[ -n "$GAME_LABEL" ] || fail 'Не прочитан SELinux-контекст папки игры.'
cd "$SOURCE"
sha256sum -c SHA256SUMS || fail 'Не совпали контрольные суммы комплекта.'
mkdir -p "$RUNTIME"
chmod 700 "$RUNTIME"
for CANDIDATE in $(pidof rest.sh 2>/dev/null || true); do
    EXE=$(readlink "/proc/$CANDIDATE/exe" 2>/dev/null || true)
    [ "$EXE" != "$RUNTIME/rest.sh" ] || fail 'Меню уже запущено. Сначала выполни stop.sh.'
done
if [ -s "$RUNTIME/rest.pid" ]; then
    OLD_PID=$(cat "$RUNTIME/rest.pid")
    case "$OLD_PID" in ''|*[!0-9]*) fail 'Некорректный rest.pid.';; esac
    OLD_EXE=$(readlink "/proc/$OLD_PID/exe" 2>/dev/null || true)
    [ "$OLD_EXE" != "$RUNTIME/rest.sh" ] || fail 'Меню уже запущено. Сначала выполни stop.sh.'
fi
for NAME in manualmap_injector libmain.so rest.sh; do
    if [ ! "$SOURCE/$NAME" -ef "$RUNTIME/$NAME" ]; then
        cp "$SOURCE/$NAME" "$RUNTIME/$NAME"
    fi
done
chmod 700 "$RUNTIME/manualmap_injector" "$RUNTIME/rest.sh"
chmod 600 "$RUNTIME/libmain.so"
mkdir -p "$GAME_FILES/exodium"
chown "$GAME_UID:$GAME_UID" "$GAME_FILES/exodium"
chmod 700 "$GAME_FILES/exodium"
chcon "$GAME_LABEL" "$GAME_FILES/exodium"
if [ -f "$GAME_FILES/exodium/cache" ] && [ ! -e "$RUNTIME/cache.before" ]; then
    cp "$GAME_FILES/exodium/cache" "$RUNTIME/cache.before"
fi
cp "$SOURCE/cache" "$GAME_FILES/exodium/cache"
chown "$GAME_UID:$GAME_UID" "$GAME_FILES/exodium/cache"
chmod 600 "$GAME_FILES/exodium/cache"
chcon "$GAME_LABEL" "$GAME_FILES/exodium/cache"
for NAME in sssss3 sssss4 sssss5 sssss sssss2 sssss7 sssss8 sssss9 sssss1 sssss10; do
    FILE=$GAME_FILES/$NAME.txt
    [ -e "$FILE" ] || touch "$FILE"
    chown "$GAME_UID:$GAME_UID" "$FILE"
    chmod 600 "$FILE"
    chcon "$GAME_LABEL" "$FILE"
done
if [ -f /data/local/tmp/version.txt ] && [ ! -e "$RUNTIME/version.before" ]; then
    cp /data/local/tmp/version.txt "$RUNTIME/version.before"
fi
printf 'Remaster' > /data/local/tmp/version.txt
chmod 644 /data/local/tmp/version.txt
MARKER=$RUNTIME/loaded.$GAME_PID.$GAME_START
if [ ! -f "$MARKER" ]; then
    printf 'Загрузка локальной библиотеки в PID %s\n' "$GAME_PID"
    if "$RUNTIME/manualmap_injector" "$GAME_PID" "$RUNTIME/libmain.so" > "$RUNTIME/injector.log" 2>&1; then
        printf 'loaded\n' > "$MARKER"
    else
        cat "$RUNTIME/injector.log"
        fail 'Инжектор завершился с ошибкой. Лог сохранён в hantoo-local/injector.log.'
    fi
fi
[ -d "/proc/$GAME_PID" ] || fail 'Игра завершилась после загрузки библиотеки.'
rm -f "$RUNTIME/rest.ready" "$RUNTIME/rest.exit"
unset EXODIUM_KEY EXODIUM_AUTH_FILE
nohup /system/bin/sh -c '
    EXODIUM_READY_FILE="$1/rest.ready" "$1/rest.sh" > "$1/rest.log" 2>&1
    RC=$?
    printf "%s\n" "$RC" > "$1/rest.exit"
' exodium-worker "$RUNTIME" > /dev/null 2>&1 < /dev/null &
WORKER_PID=$!
printf '%s\n' "$WORKER_PID" > "$RUNTIME/worker.pid"
ATTEMPT=0
while [ "$ATTEMPT" -lt 30 ]; do
    if [ -s "$RUNTIME/rest.ready" ]; then
        printf 'Меню подтвердило готовность. Логи: %s\n' "$RUNTIME"
        exit 0
    fi
    if [ -f "$RUNTIME/rest.exit" ]; then
        cat "$RUNTIME/rest.log"
        fail "rest.sh завершился, код $(cat "$RUNTIME/rest.exit")."
    fi
    REST_PIDS=$(pidof rest.sh 2>/dev/null || true)
    for CANDIDATE in $REST_PIDS; do
        EXE=$(readlink "/proc/$CANDIDATE/exe" 2>/dev/null || true)
        if [ "$EXE" = "$RUNTIME/rest.sh" ]; then
            printf '%s\n' "$CANDIDATE" > "$RUNTIME/rest.pid"
        fi
    done
    ATTEMPT=$((ATTEMPT + 1))
    sleep 1
done
tail -n 30 "$RUNTIME/rest.log"
fail 'За 30 секунд меню не подтвердило готовность. Пришли rest.log и injector.log.'
