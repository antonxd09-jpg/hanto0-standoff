#!/usr/bin/env python3
# astra_recon_step1.py
# Read-only recon: aad enum values, gby camera/render functions,
# VA->file-offset per ELF segment. No patching. No args.

import json
import os
import re
import struct
import sys
import traceback

DUMP_DIR = "/storage/emulated/0/Download/AyuGram/LemmingRMT/LemmingRMT/reference/dumps/1.0.0"
OUT_DIR  = "/storage/emulated/0/Download"
OUT_FILE = os.path.join(OUT_DIR, "astra_recon_step1.txt")

DUMP_CS   = os.path.join(DUMP_DIR, "dump.cs")
IL2CPP_H  = os.path.join(DUMP_DIR, "il2cpp.h")
SCRIPT_JSON = os.path.join(DUMP_DIR, "script.json")
LITERALS  = os.path.join(DUMP_DIR, "stringliteral.json")
LIBUNITY  = os.path.join(DUMP_DIR, "libunity.so")

# Symbols we care about. Class::method where known, bare name otherwise.
TARGET_METHODS = [
    "aad",
    "gby.rqzy", "gby.iwqc", "gby.hnyp", "gby.ihpw", "gby.vlih",
    "xmt.nriq", "xmt.mxrj", "xmt.syzn",
    "djo.ztyy", "djo.anqf",
    "nrl.LateTick",
    "PlayerAnimationData.IsFirstPerson",
]

TARGET_CLASSES = ["aad", "gby", "xmt", "djo", "nrl", "gly", "PlayerAnimationData"]

ENUM_HINTS = ["ThirdPerson", "FirstPerson"]


def log(lines, s=""):
    lines.append(s)


def read_text(path):
    with open(path, "r", encoding="utf-8", errors="replace") as f:
        return f.read()


def find_enum_block(text, enum_name):
    """Return the body of `enum <name> ... { ... }` if present."""
    # dump.cs usually: public enum aad : int { ... }
    pat = re.compile(
        r"(?:public|internal|private|protected)?\s*enum\s+" + re.escape(enum_name) +
        r"\s*(?::\s*[A-Za-z0-9_\.]+)?\s*\{",
        re.MULTILINE,
    )
    m = pat.search(text)
    if not m:
        return None
    start = m.end()
    depth = 1
    i = start
    while i < len(text) and depth > 0:
        c = text[i]
        if c == "{":
            depth += 1
        elif c == "}":
            depth -= 1
            if depth == 0:
                return text[start:i]
        i += 1
    return None


def find_class_block(text, class_name):
    """Return body of `class <name> ... { ... }` for the first match."""
    pat = re.compile(
        r"(?:public|internal|private|protected|sealed|abstract|static|\s)*"
        r"\bclass\s+" + re.escape(class_name) + r"\b[^\n{]*\{",
        re.MULTILINE,
    )
    m = pat.search(text)
    if not m:
        return None
    start = m.end()
    depth = 1
    i = start
    while i < len(text) and depth > 0:
        c = text[i]
        if c == "{":
            depth += 1
        elif c == "}":
            depth -= 1
            if depth == 0:
                return text[start:i]
        i += 1
    return None


def find_method_in_class(class_body, method_name):
    """Return the full method declaration (up to its matching closing brace or ;)."""
    if class_body is None:
        return None
    pat = re.compile(
        r"(?m)^[ \t]*(?:public|private|protected|internal|static|virtual|override|sealed|"
        r"readonly|extern|unsafe|\s)*[\w\<\>\[\]\.,\s\?]+?\b" +
        re.escape(method_name) + r"\s*\(",
    )
    m = pat.search(class_body)
    if not m:
        return None
    start = m.start()
    # Walk to matching close brace (for bodies) or semicolon (for extern).
    i = class_body.find("(", m.end() - 1)
    if i < 0:
        return class_body[start:start + 400]
    depth = 0
    while i < len(class_body):
        c = class_body[i]
        if c == "(":
            depth += 1
        elif c == ")":
            depth -= 1
            if depth == 0:
                break
        i += 1
    # After params, find { or ;
    j = i + 1
    while j < len(class_body) and class_body[j] in " \t\r\n":
        j += 1
    if j < len(class_body) and class_body[j] == ";":
        return class_body[start:j + 1]
    if j < len(class_body) and class_body[j] == "{":
        depth = 1
        k = j + 1
        while k < len(class_body) and depth > 0:
            c = class_body[k]
            if c == "{":
                depth += 1
            elif c == "}":
                depth -= 1
            k += 1
        return class_body[start:k]
    return class_body[start:j + 200]


def parse_script_json(path):
    """Return list of (name, va) from script.json. Tolerant of shape."""
    try:
        data = json.loads(read_text(path))
    except Exception as e:
        return [], "script.json parse error: %s" % e
    out = []
    # Common il2cpp script.json: {"ScriptMethod":[{"Name":..,"Address":..}]}
    def walk(obj):
        if isinstance(obj, dict):
            name = obj.get("Name") or obj.get("name") or obj.get("Symbol")
            addr = obj.get("Address") or obj.get("address") or obj.get("VA") or obj.get("RVA")
            if isinstance(name, str) and addr is not None:
                try:
                    out.append((name, int(addr, 0) if isinstance(addr, str) else int(addr)))
                except Exception:
                    pass
            for v in obj.values():
                walk(v)
        elif isinstance(obj, list):
            for v in obj:
                walk(v)
    walk(data)
    return out, None


def parse_elf_segments(path):
    """Minimal ELF64 parser. Returns list of PT_LOAD dicts with vaddr, filesz, offset, flags."""
    with open(path, "rb") as f:
        data = f.read()
    if len(data) < 64:
        raise RuntimeError("file too small for ELF64")
    if data[:4] != b"\x7fELF":
        raise RuntimeError("not ELF")
    ei_class = data[4]
    ei_data = data[5]
    if ei_class != 2 or ei_data != 1:
        raise RuntimeError("only ELF64 little-endian supported here")
    e_phoff = struct.unpack_from("<Q", data, 0x20)[0]
    e_phentsize = struct.unpack_from("<H", data, 0x36)[0]
    e_phnum = struct.unpack_from("<H", data, 0x38)[0]
    segs = []
    for i in range(e_phnum):
        off = e_phoff + i * e_phentsize
        if off + 56 > len(data):
            break
        p_type = struct.unpack_from("<I", data, off + 0x00)[0]
        if p_type != 1:  # PT_LOAD
            continue
        p_flags = struct.unpack_from("<I", data, off + 0x04)[0]
        p_offset = struct.unpack_from("<Q", data, off + 0x08)[0]
        p_vaddr = struct.unpack_from("<Q", data, off + 0x10)[0]
        p_filesz = struct.unpack_from("<Q", data, off + 0x20)[0]
        p_memsz = struct.unpack_from("<Q", data, off + 0x28)[0]
        segs.append({
            "offset": p_offset,
            "vaddr": p_vaddr,
            "filesz": p_filesz,
            "memsz": p_memsz,
            "flags": p_flags,
        })
    return segs


def va_to_file_offset(segs, va):
    """Map VA to file offset using the PT_LOAD segment that contains it.
    Returns (offset, segment) or (None, None)."""
    for s in segs:
        if s["vaddr"] <= va < s["vaddr"] + s["filesz"]:
            return s["offset"] + (va - s["vaddr"]), s
    return None, None


def main():
    out = []
    log(out, "Astra recon step 1 report")
    log(out, "dump_dir=%s" % DUMP_DIR)
    log(out, "")

    # 1. Files
    for p in [DUMP_CS, IL2CPP_H, SCRIPT_JSON, LITERALS, LIBUNITY]:
        log(out, "file %s exists=%s size=%s" % (
            p, os.path.exists(p),
            os.path.getsize(p) if os.path.exists(p) else "-"))
    log(out, "")

    if not os.path.exists(DUMP_CS):
        log(out, "dump.cs missing, aborting")
        write_out(out)
        return

    dump = read_text(DUMP_CS)
    il2cpp = read_text(IL2CPP_H) if os.path.exists(IL2CPP_H) else ""

    # 2. aad enum
    log(out, "=" * 70)
    log(out, "SECTION: enum aad")
    log(out, "=" * 70)
    body = find_enum_block(dump, "aad")
    if body is None:
        log(out, "enum aad: NOT FOUND in dump.cs")
    else:
        log(out, "enum aad body:")
        log(out, body.strip())
    # Any other enum containing ThirdPerson/FirstPerson
    for m in re.finditer(r"enum\s+([A-Za-z_][A-Za-z0-9_]*)\s*[:{]?", dump):
        name = m.group(1)
        if name == "aad":
            continue
        b = find_enum_block(dump, name)
        if b and any(h in b for h in ENUM_HINTS):
            log(out, "")
            log(out, "enum %s also contains FP/TP hints:" % name)
            log(out, b.strip())
    log(out, "")

    # 3. Classes and methods
    for cls in TARGET_CLASSES:
        log(out, "=" * 70)
        log(out, "SECTION: class %s" % cls)
        log(out, "=" * 70)
        cb = find_class_block(dump, cls)
        if cb is None:
            log(out, "class %s: NOT FOUND" % cls)
            log(out, "")
            continue
        # Print short header (first 5 lines of the class decl)
        head = "\n".join(cb.splitlines()[:6])
        log(out, "class header (first lines):")
        log(out, head)
        # If this is gby/xmt/djo/nrl/gly, dump every method declaration containing target method names.
        wanted = [t.split(".", 1)[1] for t in TARGET_METHODS if t.startswith(cls + ".")]
        if wanted:
            for w in wanted:
                m = find_method_in_class(cb, w)
                log(out, "")
                log(out, "method %s.%s:" % (cls, w))
                if m is None:
                    log(out, "  NOT FOUND")
                else:
                    log(out, m)
        log(out, "")

    # 4. VA table from script.json
    log(out, "=" * 70)
    log(out, "SECTION: script.json symbol VAs")
    log(out, "=" * 70)
    syms, err = parse_script_json(SCRIPT_JSON) if os.path.exists(SCRIPT_JSON) else ([], "missing")
    if err:
        log(out, "script.json: %s" % err)
    else:
        log(out, "script.json entries: %d" % len(syms))
        want_names = set()
        for t in TARGET_METHODS:
            want_names.add(t)
            want_names.add(t.split(".", 1)[-1])
        for cls in TARGET_CLASSES:
            want_names.add(cls)
        seen = set()
        for name, va in syms:
            base = name.split("(")[0].split("::")[-1]
            if base in want_names or any(base.endswith("." + w) for w in want_names):
                key = (name, va)
                if key in seen:
                    continue
                seen.add(key)
                log(out, "  %-60s VA=0x%x" % (name, va))
    log(out, "")

    # 5. ELF segments and VA->offset for each target VA
    log(out, "=" * 70)
    log(out, "SECTION: ELF segments and VA->file offset")
    log(out, "=" * 70)
    if not os.path.exists(LIBUNITY):
        log(out, "libunity.so missing")
    else:
        try:
            segs = parse_elf_segments(LIBUNITY)
        except Exception as e:
            log(out, "ELF parse failed: %s" % e)
            segs = []
        for s in segs:
            log(out, "PT_LOAD off=0x%x vaddr=0x%x filesz=0x%x memsz=0x%x flags=%d" % (
                s["offset"], s["vaddr"], s["filesz"], s["memsz"], s["flags"]))
        log(out, "")
        # For each target VA we found, compute file offset in the segment that contains it.
        for name, va in syms:
            base = name.split("(")[0].split("::")[-1]
            if not (base in want_names or any(base.endswith("." + w) for w in want_names)):
                continue
            off, seg = va_to_file_offset(segs, va)
            if off is None:
                log(out, "  %-60s VA=0x%x  -> no PT_LOAD contains this VA" % (name, va))
            else:
                log(out, "  %-60s VA=0x%x  -> file_off=0x%x  (seg vaddr=0x%x off=0x%x)" % (
                    name, va, off, seg["vaddr"], seg["offset"]))
    log(out, "")

    # 6. stringliteral.json: find ThirdPerson / FirstPerson literals
    log(out, "=" * 70)
    log(out, "SECTION: stringliterals for FP/TP")
    log(out, "=" * 70)
    if os.path.exists(LITERALS):
        try:
            data = json.loads(read_text(LITERALS))
        except Exception as e:
            data = None
            log(out, "stringliteral.json parse error: %s" % e)
        if data is not None:
            found = []
            def walk(o, path=""):
                if isinstance(o, dict):
                    for k, v in o.items():
                        walk(v, path + "/" + str(k))
                elif isinstance(o, list):
                    for i, v in enumerate(o):
                        walk(v, path + "[%d]" % i)
                elif isinstance(o, str):
                    if any(h in o for h in ENUM_HINTS):
                        found.append((path, o))
            walk(data)
            if not found:
                log(out, "no FP/TP literals found")
            for p, s in found[:200]:
                log(out, "  %s = %r" % (p, s))
    else:
        log(out, "stringliteral.json missing")
    log(out, "")

    write_out(out)


def write_out(lines):
    os.makedirs(OUT_DIR, exist_ok=True)
    with open(OUT_FILE, "w", encoding="utf-8") as f:
        f.write("\n".join(lines))
    print("wrote %s (%d lines)" % (OUT_FILE, len(lines)))


if __name__ == "__main__":
    try:
        main()
    except Exception:
        traceback.print_exc()
        sys.exit(1)