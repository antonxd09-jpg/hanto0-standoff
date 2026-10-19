import time, hashlib, sys

SECRET = "HantoO_Super_Secret_Salt_2024"

def generate_key(days, device_id="none"):
    exp_time = int(time.time()) + (days * 86400)
    exp_hex = hex(exp_time)[2:] # Время в HEX
    raw = f"{exp_hex}:{device_id}"
    h = hashlib.sha256((raw + SECRET).encode()).hexdigest()[:8]
    return f"HANT-{exp_hex}-{device_id}-{h}"

if __name__ == "__main__":
    try:
        days = int(sys.argv[1]) if len(sys.argv) > 1 else 1
        dev_id = sys.argv[2] if len(sys.argv) > 2 else "none"
        print(generate_key(days, dev_id))
    except Exception as e:
        print("Ошибка:", e)
