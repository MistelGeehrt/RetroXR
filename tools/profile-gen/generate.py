#!/usr/bin/env python3
"""Générateur de profil assisté par LLM — version 0 (Claude Code en mode headless).

Principe (voir docs/ROADMAP.md, semaines 8-10) :
  1. fv_dump.lua produit flatverse_dump.json dans le jeu.
  2. summarize.py le condense en brief.
  3. Ce script envoie le brief + le schéma + le prompt à `claude -p` et récupère
     un profile.json PROPOSÉ (status forcé à "unverified", generated_by_ai=true).
  4. validate_profiles.py le vérifie ; l'humain teste en jeu et passe human_validated=true.

Le LLM ne touche jamais au jeu en temps réel : il produit un artefact, c'est tout.

Usage :
    python tools/profile-gen/generate.py dump.json --title "Nom du jeu" --exe "Jeu-Win64-Shipping.exe" \
        --out profiles/nom-du-jeu/profile.json

Prérequis : Claude Code installé et authentifié (`claude --version`).
"""
from __future__ import annotations

import argparse
import json
import re
import subprocess
import sys
from pathlib import Path

HERE = Path(__file__).resolve().parent
ROOT = HERE.parents[1]
sys.path.insert(0, str(ROOT / "tools" / "uobject-dump"))
from summarize import render_text, summarize  # noqa: E402


def build_prompt(brief: str, title: str, exe: str) -> str:
    system = (HERE / "prompt.md").read_text(encoding="utf-8")
    schema = (ROOT / "profiles" / "schema.json").read_text(encoding="utf-8")
    template = (ROOT / "profiles" / "_template" / "profile.json").read_text(encoding="utf-8")
    return (
        f"{system}\n\n"
        f"## Jeu\ntitre: {title}\nexécutable: {exe}\n\n"
        f"## Brief du dump UObject\n{brief}\n\n"
        f"## Schéma JSON (à respecter strictement)\n```json\n{schema}\n```\n\n"
        f"## Gabarit\n```json\n{template}\n```\n\n"
        "Réponds UNIQUEMENT avec le JSON du profil, sans commentaire."
    )


def call_claude(prompt: str, model: str | None) -> str:
    cmd = ["claude", "-p", "--output-format", "text"]
    if model:
        cmd += ["--model", model]
    res = subprocess.run(cmd, input=prompt, text=True, capture_output=True, check=False)
    if res.returncode != 0:
        print(res.stderr, file=sys.stderr)
        raise SystemExit("claude -p a échoué")
    return res.stdout


def extract_json(text: str) -> dict:
    m = re.search(r"\{.*\}", text, re.S)
    if not m:
        raise SystemExit("aucun JSON trouvé dans la réponse du modèle")
    return json.loads(m.group(0))


def main() -> None:
    ap = argparse.ArgumentParser()
    ap.add_argument("dump", type=Path)
    ap.add_argument("--title", required=True)
    ap.add_argument("--exe", required=True)
    ap.add_argument("--out", type=Path, required=True)
    ap.add_argument("--model", default=None)
    args = ap.parse_args()

    dump = json.loads(args.dump.read_text(encoding="utf-8"))
    brief = render_text(summarize(dump))
    prompt = build_prompt(brief, args.title, args.exe)

    profile = extract_json(call_claude(prompt, args.model))

    # Garde-fous : l'IA propose, elle ne valide pas.
    profile["status"] = "unverified"
    profile.setdefault("provenance", {})
    profile["provenance"]["license"] = "CC-BY-4.0"
    profile["provenance"]["generated_by_ai"] = True
    profile["provenance"]["human_validated"] = False

    args.out.parent.mkdir(parents=True, exist_ok=True)
    args.out.write_text(json.dumps(profile, ensure_ascii=False, indent=2) + "\n", encoding="utf-8")
    print(f"profil proposé -> {args.out}")
    print("étape suivante : python tools/validator/validate_profiles.py", args.out)


if __name__ == "__main__":
    main()
