#!/usr/bin/env python3
"""Résume un flatverse_dump.json (produit par lua/lib/fv_dump.lua) en un brief
lisible par un humain… et par un LLM (c'est l'entrée de tools/profile-gen).

Usage :
    python tools/uobject-dump/summarize.py "<config UEVR>/<Jeu>/scripts/data/flatverse_dump.json"
    python tools/uobject-dump/summarize.py dump.json --json   # sortie JSON compacte
"""
from __future__ import annotations

import argparse
import json
from collections import Counter
from pathlib import Path


def short(name: str) -> str:
    # "BlueprintGeneratedClass /Game/UI/HUD/WBP_Minimap.WBP_Minimap_C" -> "WBP_Minimap_C"
    return name.rsplit(".", 1)[-1] if "." in name else name


def summarize(dump: dict) -> dict:
    out: dict = {
        "schema": dump.get("schema"),
        "runtime": dump.get("runtime"),
        "pawn": None,
        "camera_manager": None,
        "camera_components": [],
        "spring_arms": [],
        "widgets": [],
        "hud_classes": [],
    }

    if dump.get("local_pawn"):
        p = dump["local_pawn"]
        out["pawn"] = {"name": p["name"], "chain": [short(c) for c in p.get("chain", [])]}

    if dump.get("camera_manager"):
        cm = dump["camera_manager"]
        out["camera_manager"] = {"name": cm["name"], "chain": [short(c) for c in cm.get("chain", [])]}

    for entry in dump.get("classes", []):
        if not entry.get("found"):
            continue
        cls = entry["class"]
        names = [short(i["name"]) for i in entry.get("instances", [])]
        if cls.endswith("CameraComponent"):
            out["camera_components"] = names
        elif cls.endswith("SpringArmComponent"):
            out["spring_arms"] = names
        elif cls.endswith("UserWidget"):
            # regroupe par classe Blueprint : WBP_Minimap_C x3, etc.
            counter = Counter(n.split("_C")[0] + "_C" if "_C" in n else n for n in names)
            out["widgets"] = [{"class": k, "count": v} for k, v in counter.most_common(40)]
        elif cls.endswith("Engine.HUD"):
            out["hud_classes"] = [i["chain"][0] if i.get("chain") else i["name"] for i in entry.get("instances", [])]

    return out


def render_text(s: dict) -> str:
    lines = ["# flatverse — résumé du dump UObject", ""]
    lines.append(f"Runtime : {s.get('runtime')}")
    if s["pawn"]:
        lines.append(f"Pawn : {s['pawn']['name']}")
        lines.append("  héritage : " + " → ".join(s["pawn"]["chain"]))
    if s["camera_manager"]:
        lines.append(f"CameraManager : {s['camera_manager']['name']}")
    lines.append(f"CameraComponents ({len(s['camera_components'])}) : " + ", ".join(s["camera_components"][:10]))
    lines.append(f"SpringArms ({len(s['spring_arms'])}) : " + ", ".join(s["spring_arms"][:10]))
    lines.append("")
    lines.append("Widgets UMG visibles (candidats wrist-HUD) :")
    for w in s["widgets"]:
        lines.append(f"  - {w['class']} ×{w['count']}")
    if s["hud_classes"]:
        lines.append("")
        lines.append("Classes HUD : " + ", ".join(short(h) for h in s["hud_classes"]))
    return "\n".join(lines)


def main() -> None:
    ap = argparse.ArgumentParser()
    ap.add_argument("dump", type=Path)
    ap.add_argument("--json", action="store_true", help="sortie JSON au lieu du texte")
    args = ap.parse_args()

    dump = json.loads(args.dump.read_text(encoding="utf-8"))
    s = summarize(dump)
    print(json.dumps(s, ensure_ascii=False, indent=2) if args.json else render_text(s))


if __name__ == "__main__":
    main()
