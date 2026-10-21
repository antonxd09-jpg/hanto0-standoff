#!/usr/bin/env bash
set -euo pipefail

ADB="${ADB:-adb}"
ACTION="${1:-follow}"
PACKAGE="${2:-${PACKAGE:-com.axlebolt.standoff2}}"
TAG="LemmingRMT"

usage() {
  cat <<EOF
Usage: $0 [follow|dump|clear|diag|toggle|snapshot] [package]

  follow    Follow only LemmingRMT logcat messages (default)
  dump      Print current buffered LemmingRMT messages
  clear     Clear logcat
  diag      Send SIGUSR2 to request a diagnostic dump
  toggle    Send SIGUSR1 to toggle the menu
  snapshot  Clear logs, request diagnostics, then print them

Environment:
  ADB=/path/to/adb
  PACKAGE=com.axlebolt.standoff2
EOF
}

pid_for_package() {
  local pid
  pid="$($ADB shell pidof "$PACKAGE" 2>/dev/null | tr -d '\r\n' || true)"
  if [[ -z "$pid" ]]; then
    echo "Process not found: $PACKAGE" >&2
    exit 2
  fi
  printf '%s' "$pid"
}

send_signal() {
  local signal="$1"
  local pid
  pid="$(pid_for_package)"
  echo "Sending $signal to $PACKAGE (pid $pid)"
  $ADB shell su -c "kill -$signal $pid"
}

case "$ACTION" in
  follow)
    exec "$ADB" logcat -v threadtime "$TAG:D" '*:S'
    ;;
  dump)
    exec "$ADB" logcat -d -v threadtime "$TAG:D" '*:S'
    ;;
  clear)
    exec "$ADB" logcat -c
    ;;
  diag)
    send_signal USR2
    ;;
  toggle)
    send_signal USR1
    ;;
  snapshot)
    "$ADB" logcat -c
    send_signal USR2
    sleep 1
    exec "$ADB" logcat -d -v threadtime "$TAG:D" '*:S'
    ;;
  -h|--help|help)
    usage
    ;;
  *)
    usage >&2
    exit 1
    ;;
esac
