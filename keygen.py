import base64, time, hashlib, sys

SECRET = "HantoO_Super_Secret_Salt_2024"

def generate_key(days):
    exp_time = int(time.time()) + (days * 86400)
    raw = str(exp_time)
    # Делаем хэш от времени + секрет, чтобы нельзя было просто перебрать время
    h = hashlib.sha256((raw + SECRET).encode()).hexdigest()[:8]
    payload = f"{raw}|{h}"
    # Кодируем в base64 и убираем лишние символы
    key = base64.b64encode(payload.encode()).decode().replace("=", "")
    return f"HANT-{key}"

if __name__ == "__main__":
    try:
        days = int(sys.argv[1]) if len(sys.argv) > 1 else 1
        print(generate_key(days))
    except Exception as e:
        print("Использование: python keygen.py <дни>")
        print("Пример: python keygen.py 30")
