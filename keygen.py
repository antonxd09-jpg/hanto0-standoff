import time, hashlib, sys, random

SECRET = "HantoO_Super_Secret_Salt_2024"
BASE_TIME = 1704067200 # 1 января 2024
CHARS = "0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZ"

def to_base36(num):
    res = ""
    while num > 0:
        res = CHARS[num % 36] + res
        num //= 36
    return res.zfill(4)

if __name__ == "__main__":
    try:
        days = int(sys.argv[1]) if len(sys.argv) > 1 else 1
        key_type = sys.argv[2].upper() if len(sys.argv) > 2 else "G"
        count = int(sys.argv[3]) if len(sys.argv) > 3 else 1

        # Ограничение до 500
        if count > 500:
            print("[!] Максимум 500 ключей за раз. Генерирую 500.")
            count = 500

        target_time = int(time.time()) + (days * 86400)
        hours = int((target_time - BASE_TIME) / 3600)
        time_str = to_base36(hours)

        for _ in range(count):
            nonce = "".join(random.choices(CHARS, k=2))
            sig_raw = int(hashlib.sha256((time_str + nonce + key_type + SECRET).encode()).hexdigest()[:8], 16)
            sig_str = to_base36(sig_raw % 1679616)
            print(f"KEY-{time_str}-{nonce}-{sig_str}-{key_type}")

    except Exception as e:
        print("Ошибка:", e)
