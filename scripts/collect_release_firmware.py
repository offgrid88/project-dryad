#!/usr/bin/env python3
"""Copy one PlatformIO firmware image into a release folder.

Reads `.pio/build/<env>/flasher_args.json` so bootloader offsets stay correct
per chip. The application binary is named `dryad-<env>.bin`. Bootloader and
partition-table images get the same chip prefix so a GitHub Release can hold
all four targets without filename collisions. The ELF is copied only when it
is at most 8 MiB.
"""

from __future__ import annotations

import json
import shutil
import sys
from pathlib import Path

ELF_LIMIT = 8 * 1024 * 1024


def dest_name(env: str, source: Path) -> str:
    base = source.name
    if base in ("firmware.bin", "dryad.bin"):
        return f"dryad-{env}.bin"
    if "bootloader" in base:
        return f"dryad-{env}-bootloader.bin"
    if "partition" in base:
        return f"dryad-{env}-partitions.bin"
    if "ota" in base:
        return f"dryad-{env}-otadata.bin"
    return f"dryad-{env}-{base}"


def resolve_flash_file(build: Path, rel: str) -> Path:
    """PlatformIO keeps flasher_args paths from ESP-IDF, then flattens the bins."""
    candidates = [
        build / rel,
        build / Path(rel).name,
    ]
    name = Path(rel).name
    if name == "dryad.bin":
        candidates.append(build / "firmware.bin")
    if "bootloader" in rel:
        candidates.append(build / "bootloader.bin")
    if "partition" in name:
        candidates.append(build / "partitions.bin")
    for candidate in candidates:
        if candidate.is_file():
            return candidate
    raise FileNotFoundError(rel)


def main() -> int:
    if len(sys.argv) != 3:
        print("usage: collect_release_firmware.py <pio-env> <out-dir>", file=sys.stderr)
        return 2
    env, out_arg = sys.argv[1], sys.argv[2]
    build = Path(".pio/build") / env
    args_path = build / "flasher_args.json"
    if not args_path.is_file():
        print(f"missing {args_path}", file=sys.stderr)
        return 1
    args = json.loads(args_path.read_text())
    flash_files = args.get("flash_files") or {}
    if not flash_files:
        print(f"{args_path} has no flash_files", file=sys.stderr)
        return 1

    out = Path(out_arg)
    out.mkdir(parents=True, exist_ok=True)

    copied: list[tuple[str, str]] = []
    ordered = sorted(flash_files.items(), key=lambda item: int(str(item[0]), 16))
    for offset, rel in ordered:
        try:
            src = resolve_flash_file(build, rel)
        except FileNotFoundError:
            print(f"missing flash file {build / rel}", file=sys.stderr)
            return 1
        name = dest_name(env, src)
        shutil.copy2(src, out / name)
        copied.append((str(offset), name))
        print(f"copied {src} -> {out / name}")

    notes = [
        f"Dryad {env}",
        "",
        "MQTT is compiled in and left off.",
        "Replace PORT with the board serial device.",
        "",
    ]
    elf = build / "firmware.elf"
    if elf.is_file():
        size = elf.stat().st_size
        if size <= ELF_LIMIT:
            elf_name = f"dryad-{env}.elf"
            shutil.copy2(elf, out / elf_name)
            print(f"copied {elf} -> {out / elf_name} ({size} bytes)")
        else:
            notes.append(
                f"firmware.elf is {size} bytes, over the {ELF_LIMIT} byte release limit, so it is not attached."
            )
            notes.append("")
            print(f"skipped {elf} ({size} bytes)")

    extra = " ".join(args.get("write_flash_args") or [])
    pairs = " ".join(f"{offset} {name}" for offset, name in copied)
    prefix = f"esptool.py --chip {env} -p PORT -b 460800"
    if extra:
        prefix = f"{prefix} {extra}"
    notes.append(f"{prefix} write_flash {pairs}")
    notes.append("")
    note_path = out / f"dryad-{env}-flash.txt"
    note_path.write_text("\n".join(notes))
    print(f"wrote {note_path}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
