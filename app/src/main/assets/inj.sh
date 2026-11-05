#!/system/bin/sh
PACKAGE="com.axlebolt.standoff2"
DIR="/data/local/tmp"
INJECTOR="$DIR/vpnService"
LIB="$DIR/libastra.so"
INJECT_DELAY=40
LOG="/sdcard/HantoO_log.txt"

echo "=== HantoO INJECT START ===" > $LOG
echo "[*] Остановка старого процесса..." >> $LOG
am force-stop $PACKAGE
sleep 2

echo "[*] Запуск Standoff 2..." >> $LOG
am start -a android.intent.action.MAIN -c android.intent.category.LAUNCHER -n $PACKAGE/com.axlebolt.unity.MainActivity >> $LOG 2>&1

if [ $? -ne 0 ]; then
    echo "[!] Ошибка am start, пробую monkey..." >> $LOG
    monkey -p $PACKAGE -c android.intent.category.LAUNCHER 1 >> $LOG 2>&1
fi

echo "[*] Ожидание ${INJECT_DELAY} сек..." >> $LOG
sleep $INJECT_DELAY

NEW_PID=$(pidof $PACKAGE)
if [ -z "$NEW_PID" ]; then
    echo "[!] Игра не запустилась (PID не найден)" >> $LOG
    exit 1
fi
echo "[+] Игра жива, PID: $NEW_PID" >> $LOG

echo "[*] Инжект libastra.so..." >> $LOG
$INJECTOR --package $PACKAGE --libs $LIB --memfd --delay 2000000 >> $LOG 2>&1

echo "[*] Готово. Проверяем карту памяти..." >> $LOG
if cat /proc/$NEW_PID/maps 2>/dev/null | grep -q "lemming"; then
    echo "[+] liblemmingrmt.so ЗАГРУЖЕНА!" >> $LOG
else
    echo "[!] Либа не в maps" >> $LOG
fi
echo "=== HantoO INJECT END ===" >> $LOG
