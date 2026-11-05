#!/system/bin/sh
PACKAGE="com.axlebolt.standoff2"
DIR="/data/local/tmp"
INJECTOR="$DIR/vpnService"
LIB="$DIR/libastra.so"
INJECT_DELAY=4
LOG="/sdcard/HantoO_log.txt"

echo "=== HantoO INJECT START ===" > $LOG
echo "[*] Остановка старого процесса..." >> $LOG
am force-stop $PACKAGE
sleep 1

echo "[*] Запуск Standoff 2..." >> $LOG
monkey -p $PACKAGE -c android.intent.category.LAUNCHER 1 >> $LOG 2>&1

echo "[*] Ожидание ${INJECT_DELAY} сек..." >> $LOG
sleep $INJECT_DELAY

NEW_PID=$(pidof $PACKAGE)
if [ -z "$NEW_PID" ]; then
    echo "[!] Игра не запустилась (PID не найден)" >> $LOG
    exit 1
fi
echo "[+] Игра жива, PID: $NEW_PID" >> $LOG

echo "[*] Инжект libastra.so (скрытый)..." >> $LOG
$INJECTOR --package $PACKAGE --libs $LIB --memfd --hide --free --delay 2000000 >> $LOG 2>&1

echo "[*] Уничтожаем улики в /data/local/tmp..." >> $LOG
rm -f /data/local/tmp/libastra.so
rm -f /data/local/tmp/vpnService
rm -f /data/local/tmp/inj.sh

echo "[*] Проверяем карту памяти..." >> $LOG
if cat /proc/$NEW_PID/maps 2>/dev/null | grep -q "lemming"; then
    echo "[+] liblemmingrmt.so ЗАГРУЖЕНА!" >> $LOG
else
    echo "[!] Либа не в maps. Возможен краш или скрыта." >> $LOG
fi
echo "=== HantoO INJECT END ===" >> $LOG
