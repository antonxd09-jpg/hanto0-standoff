#!/system/bin/sh

# ================================================
#   STANDOFF 2 AUTO INJECTOR v3.1
#   by [R]
#   - Задержка инжекта: 3 сек после запуска игры
#   - Проверка версии, логов, ретрай
#   - Анти-автоповорот
# ================================================

PACKAGE="com.axlebolt.standoff2"
DIR="/data/local/tmp"
INJECTOR="$DIR/vpnService"
LIB="$DIR/libastra.so"

INJECT_DELAY=3

# ================================================
#   [1/7] ПРОВЕРКА ФАЙЛОВ
# ================================================
echo ""
echo "================================================"
echo "   [1/7] ПРОВЕРКА ФАЙЛОВ"
echo "================================================"
echo ""

if [ ! -f "$INJECTOR" ]; then
    echo "   [!] ИНЖЕКТОР НЕ НАЙДЕН: $INJECTOR"
    exit 1
fi
echo "   [+] vpnService ............. OK"

if [ ! -f "$LIB" ]; then
    echo "   [!] БИБЛИОТЕКА НЕ НАЙДЕНА: $LIB"
    exit 1
fi
echo "   [+] liblemmingrmt.so ....... OK"

chmod 777 $INJECTOR
chmod 777 $LIB
echo "   [+] Права 777 установлены"
echo ""

# ================================================
#   [2/7] ПРОВЕРКА ПАКЕТА
# ================================================
echo "================================================"
echo "   [2/7] ПРОВЕРКА ПАКЕТА"
echo "================================================"
echo ""

if ! pm list packages | grep -q "$PACKAGE"; then
    echo "   [!] Standoff 2 НЕ УСТАНОВЛЕН"
    exit 1
fi

VER=$(dumpsys package $PACKAGE | grep versionName | head -1 | cut -d= -f2 | tr -d ' ')
echo "   [+] Пакет: $PACKAGE"
echo "   [+] Версия: $VER"
echo ""

# ================================================
#   [3/7] ОСТАНОВКА СТАРОГО ПРОЦЕССА
# ================================================
echo "================================================"
echo "   [3/7] ОСТАНОВКА СТАРОГО ПРОЦЕССА"
echo "================================================"
echo ""

OLD_PID=$(pidof $PACKAGE)
if [ -z "$OLD_PID" ]; then
    echo "   [*] Игра не запущена"
else
    echo "   [*] Найден PID: $OLD_PID — убиваю"
    am force-stop $PACKAGE
    sleep 2
    if [ -z "$(pidof $PACKAGE)" ]; then
        echo "   [+] Процесс убит"
    fi
fi
echo ""

# ================================================
#   [4/7] ЗАПУСК ИГРЫ
# ================================================
echo "================================================"
echo "   [4/7] ЗАПУСК STANDOFF 2"
echo "================================================"
echo ""

echo "   [*] Отправляю LAUNCH intent..."
monkey -p $PACKAGE -c android.intent.category.LAUNCHER 1 >/dev/null 2>&1

if [ $? -ne 0 ]; then
    echo "   [!] Ошибка intent"
    exit 1
fi
echo "   [+] Intent отправлен"
echo ""

# ================================================
#   [5/7] ОЖИДАНИЕ ${INJECT_DELAY} СЕК
# ================================================
echo "================================================"
echo "   [5/7] ОЖИДАНИЕ (${INJECT_DELAY} сек)"
echo "================================================"
echo ""

i=1
while [ $i -le $INJECT_DELAY ]; do
    sleep 1
    echo "   ... $i/$INJECT_DELAY"
    i=$((i+1))
done
echo ""

NEW_PID=$(pidof $PACKAGE)
if [ -z "$NEW_PID" ]; then
    echo "   [!] ПРОЦЕСС НЕ ПОЯВИЛСЯ"
    exit 1
fi
echo "   [+] PID: $NEW_PID"
echo ""

# ================================================
#   [6/7] ИНЖЕКТ
# ================================================
echo "================================================"
echo "   [6/7] ИНЖЕКТ"
echo "================================================"
echo ""

echo "   [*] Запускаю vpnService..."
echo "       --package $PACKAGE"
echo "       --libs $LIB"
echo "       --memfd"
echo "       --delay 2000000"
echo ""

START=$(date +%s)
$INJECTOR --package $PACKAGE --libs $LIB --memfd --delay 2000000
RC=$?
END=$(date +%s)
DURATION=$((END - START))

echo ""
echo "   [*] Код: $RC | Время: ${DURATION}s"
echo ""

# Проверка через 3 сек
sleep 3
CHECK_PID=$(pidof $PACKAGE)

if [ -z "$CHECK_PID" ]; then
    echo "   [!] ИГРА КРАШНУЛАСЬ"
    echo ""
    echo "   [*] Логи:"
    logcat -d -t 100 2>/dev/null | grep -iE "standoff|axlebolt|lemming|fatal|signal" | tail -15 | sed 's/^/       /'
    echo ""
    exit 1
fi

if cat /proc/$CHECK_PID/maps 2>/dev/null | grep -q "lemming"; then
    echo "   [+] Игра жива. PID: $CHECK_PID"
    echo "   [+] liblemmingrmt.so ЗАГРУЖЕНА"
else
    echo "   [!] Игра жива, но либа НЕ в maps"
fi
echo ""

# ================================================
#   [7/7] АНТИ-АВТОПОВОРОТ
# ================================================
echo "================================================"
echo "   [7/7] АНТИ-АВТОПОВОРОТ"
echo "================================================"
echo ""

settings put system accelerometer_rotation 0
settings put secure accelerometer_rotation 0
settings put global accelerometer_rotation 0
settings put system user_rotation 0

(
    while true; do
        settings put system accelerometer_rotation 0
        settings put secure accelerometer_rotation 0
        settings put global accelerometer_rotation 0
        settings put system user_rotation 0
        sleep 2
    done
) &
echo "   [+] Watchdog PID: $!"
echo ""

echo "================================================"
echo "   ЗАВЕРШЕНО"
echo "================================================"
echo ""