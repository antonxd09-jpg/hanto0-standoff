#!/system/bin/sh
set -eu
fail() { printf '%s\n' "$*" >&2; exit 1; }
[ "$(id -u)" = 0 ] || fail 'root required'

SOURCE=$(CDPATH= cd -- "$(dirname -- "$0")" && pwd)
RUNTIME=/data/local/tmp/hantoo-local
PACKAGE=com.axlebolt.standoff2

PIDS=$(pidof "$PACKAGE" 2>/dev/null || true)
if [ -z "$PIDS" ]; then
  printf '[HantoO] start game\n'
  am start -n "$PACKAGE/.UnityPlayerActivity" >/dev/null 2>&1 || monkey -p "$PACKAGE" -c android.intent.category.LAUNCHER 1 >/dev/null 2>&1 || fail 'cannot start game'
  i=0
  while [ $i -lt 25 ]; do
    PIDS=$(pidof "$PACKAGE" 2>/dev/null || true)
    [ -n "$PIDS" ] && break
    i=$((i+1)); sleep 1
  done
  [ -n "$PIDS" ] || fail 'game not running'
  printf '[HantoO] wait menu 8s\n'
  sleep 8
fi

set -- $PIDS
[ "$#" = 1 ] || fail 'multiple game processes'
GAME_PID=$1
GAME_START=$(awk '{print $22}' "/proc/$GAME_PID/stat")
GAME_UID=$(awk '/^Uid:/{print $2}' "/proc/$GAME_PID/status")
ANDROID_USER=$((GAME_UID / 100000))
GAME_FILES=/data/user/$ANDROID_USER/$PACKAGE/files
[ -d "$GAME_FILES" ] || fail "no game files: $GAME_FILES"
GAME_LABEL=$(ls -Zd "$GAME_FILES" | awk '{print $1}')

cd "$SOURCE"
mkdir -p "$RUNTIME"
chmod 700 "$RUNTIME"

for C in $(pidof rest.sh 2>/dev/null || true); do
  E=$(readlink "/proc/$C/exe" 2>/dev/null || true)
  [ "$E" != "$RUNTIME/rest.sh" ] || fail 'menu already running, stop first'
done

cp -f "$SOURCE/manualmap_injector" "$RUNTIME/manualmap_injector"
cp -f "$SOURCE/libmain.so" "$RUNTIME/libmain.so"
cp -f "$SOURCE/rest.sh" "$RUNTIME/rest.sh"
chmod 700 "$RUNTIME/manualmap_injector" "$RUNTIME/rest.sh"
chmod 600 "$RUNTIME/libmain.so"

mkdir -p "$GAME_FILES/exodium"
chown "$GAME_UID:$GAME_UID" "$GAME_FILES/exodium"
chcon "$GAME_LABEL" "$GAME_FILES/exodium" 2>/dev/null || true
cp -f "$SOURCE/cache" "$GAME_FILES/exodium/cache"
chown "$GAME_UID:$GAME_UID" "$GAME_FILES/exodium/cache"
chmod 600 "$GAME_FILES/exodium/cache"
chcon "$GAME_LABEL" "$GAME_FILES/exodium/cache" 2>/dev/null || true

for NAME in sssss3 sssss4 sssss5 sssss sssss2 sssss7 sssss8 sssss9 sssss1 sssss10; do
  F=$GAME_FILES/$NAME.txt
  [ -e "$F" ] || touch "$F"
  chown "$GAME_UID:$GAME_UID" "$F"
  chmod 600 "$F"
  chcon "$GAME_LABEL" "$F" 2>/dev/null || true
done

printf 'Remaster' > /data/local/tmp/version.txt
chmod 644 /data/local/tmp/version.txt

MARKER=$RUNTIME/loaded.$GAME_PID.$GAME_START
if [ ! -f "$MARKER" ]; then
  printf '[HantoO] inject %s\n' "$GAME_PID"
  if "$RUNTIME/manualmap_injector" "$GAME_PID" "$RUNTIME/libmain.so" > "$RUNTIME/injector.log" 2>&1; then
    printf 'loaded\n' > "$MARKER"
  else
    cat "$RUNTIME/injector.log"
    fail 'inject failed'
  fi
fi
[ -d "/proc/$GAME_PID" ] || fail 'game died'

rm -f "$RUNTIME/rest.ready" "$RUNTIME/rest.exit"
unset EXODIUM_KEY EXODIUM_AUTH_FILE
nohup /system/bin/sh -c '
  EXODIUM_READY_FILE="$1/rest.ready" "$1/rest.sh" > "$1/rest.log" 2>&1
  printf "%s\n" $? > "$1/rest.exit"
' hantoo-worker "$RUNTIME" >/dev/null 2>&1 </dev/null &
echo $! > "$RUNTIME/worker.pid"

i=0
while [ $i -lt 30 ]; do
  if [ -s "$RUNTIME/rest.ready" ]; then
    printf '[HantoO] menu ready\n'
    exit 0
  fi
  if [ -f "$RUNTIME/rest.exit" ]; then
    cat "$RUNTIME/rest.log"
    fail "rest exit $(cat $RUNTIME/rest.exit)"
  fi
  for C in $(pidof rest.sh 2>/dev/null || true); do
    E=$(readlink "/proc/$C/exe" 2>/dev/null || true)
    [ "$E" = "$RUNTIME/rest.sh" ] && echo $C > "$RUNTIME/rest.pid"
  done
  i=$((i+1)); sleep 1
done
tail -n 40 "$RUNTIME/rest.log" 2>/dev/null || true
fail 'menu timeout'
