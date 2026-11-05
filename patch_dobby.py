import os, re
for root, dirs, files in os.walk('app/src/main/cpp/Dobby'):
    for file in files:
        if file.endswith('.S'):
            path = os.path.join(root, file)
            with open(path, 'r') as f:
                c = f.read()
            # Заменяем symbol@PAGEOFF на :lo12:symbol
            c = re.sub(r'([a-zA-Z0-9_]+)@PAGEOFF', r':lo12:\1', c)
            # Убираем оставшийся @PAGE
            c = c.replace('@PAGE', '')
            with open(path, 'w') as f:
                f.write(c)
print("Dobby patched successfully!")
