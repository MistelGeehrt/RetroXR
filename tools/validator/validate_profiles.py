#!/usr/bin/env python3
"""Valide tous les profiles/*/profile.json contre profiles/schema.json.

Usage :
    python tools/validator/validate_profiles.py            # tout le dossier profiles/
    python tools/validator/validate_profiles.py chemin.json  # un fichier précis

Sort avec le code 1 si au moins un profil est invalide (utilisé par la CI).
"""
from __future__ import annotations

import json
import sys
from pathlib import Path

try:
    from jsonschema import Draft202012Validator, FormatChecker
except ImportError:  # pragma: no cover
    print("pip install jsonschema", file=sys.stderr)
    sys.exit(2)

ROOT = Path(__file__).resolve().parents[2]
SCHEMA_PATH = ROOT / "profiles" / "schema.json"


def iter_profiles(args: list[str]):
    if args:
        for a in args:
            yield Path(a).resolve()
        return
    for p in sorted((ROOT / "profiles").glob("*/profile.json")):
        yield p


def main(argv: list[str]) -> int:
    schema = json.loads(SCHEMA_PATH.read_text(encoding="utf-8"))
    validator = Draft202012Validator(schema, format_checker=FormatChecker())

    failures = 0
    count = 0
    for path in iter_profiles(argv):
        count += 1
        try:
            data = json.loads(path.read_text(encoding="utf-8"))
        except json.JSONDecodeError as e:
            failures += 1
            print(f"✗ {path.relative_to(ROOT)} : JSON invalide ({e})")
            continue

        errors = sorted(validator.iter_errors(data), key=lambda e: list(e.path))
        if errors:
            failures += 1
            print(f"✗ {path.relative_to(ROOT)}")
            for err in errors:
                loc = "/".join(str(x) for x in err.path) or "<racine>"
                print(f"    {loc}: {err.message}")
        else:
            status = data.get("status", "?")
            print(f"✓ {path.relative_to(ROOT)}  [{status}]")

    print(f"\n{count - failures}/{count} profils valides")
    return 1 if failures else 0


if __name__ == "__main__":
    sys.exit(main(sys.argv[1:]))
