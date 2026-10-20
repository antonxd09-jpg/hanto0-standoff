import time, hashlib, sys, random

SECRET = "HantoO_Super_Secret_Salt_2024"
BASE_TIME = 1704067200 # 1 января 2024
CHARS = "0123456789abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ"

def to_base62_2(num):
    if num == 0: return "00"
    res = ""
    while num > 0:
        res = CHARS[num % 62] + res
        num //= 62
    return res.zfill(2)

def to_base62_4(num):
    if num == 0: return "0000"
    res = ""
    while num > 0:
        res = CHARS[num % 62] + res
        num //= 62
    return res.zfill(4)

if __name__ == "__main__":
    try:
        days = int(sys.argv[1]) if len(sys.argv) > 1 else 1
        key_type = sys.argv[2].upper() if len(sys.argv) > 2 else "G"
        count = int(sys.argv[3]) if len(sys.argv) > 3 else 1

        if count > 500: count = 500

        target_time = int(time.time()) + (days * 86400)
        days_from_base = int((target_time - BASE_TIME) / 86400)
        time_str = to_base62_2(days_from_base)
        type_str = 'G' if key_type == "G" else 'S'

        for _ in range(count):
            rand_str = random.choice(CHARS)
            part1 = time_str + type_str + rand_str

            sig_raw = hashlib.sha256((part1 + SECRET).encode()).hexdigest()
            sig_num = int(sig_raw[:8], 16) % (62**4)
            part2 = to_base62_4(sig_num)

            print(f"KEY-{part1}-{part2}")

    except Exception as e:
        print("Ошибка:", e)
