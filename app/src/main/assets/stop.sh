#!/system/bin/sh
set -eu
[ "$(id -u)" = 0 ] || exit 1
RUNTIME=/data/local/tmp/hantoo-local
for PID in $(pidof rest.sh 2>/dev/null || true); do
  EXE=$(readlink "/proc/$PID/exe" 2>/dev/null || true)
  if [ "$EXE" = "$RUNTIME/rest.sh" ]; then
    kill -TERM "$PID" 2>/dev/null || true
    echo "[HantoO] stop $PID"
  fi
done
rm -f "$RUNTIME/rest.pid" "$RUNTIME/worker.pid"
echo "[HantoO] close game fully to unload"
