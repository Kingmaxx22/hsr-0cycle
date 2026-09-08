#!/usr/bin/env python3
"""One-off generator: character_eidolons.csv -> character_eidolons_rules.json.

The source CSV is scraped messily:
  - One slug has 7 rows (E1..E7). Row E1 holds the concatenated blob of all
    eidolon texts; rows E2..E7 hold the TRUE E1..E6 effect texts (off by one).
  - The `name` column holds a fragment of the effect text, NOT the eidolon
    title. True titles ("Silenced Sky Spake Sooth") are parsed from the E1
    blob via the "E <n> <title> Eidolon <n>" pattern.
  - Descriptions repeat their tail ("X. tail-fragment"); trailing sentences
    that are substrings of the kept text are dropped.

Output schema (character_eidolons_rules.json):
  {schema_version, source_file, count, eidolons: [
    {slug, name, levels: [
      {eidolon, title, description, skillLevels: ["Ultimate", ...]}]}]}

`skillLevels` lists the abilities an Eidolon raises ("Skill Lv. +2" style),
used data-driven by the sim to pick boosted manual scaling values.
Run:  python build_eidolon_rules.py   (from engine/hsr_engine/)
"""
import csv
import json
import os
import re
import sys

DATA_DIR = os.path.join(os.path.dirname(os.path.abspath(__file__)), "data")
SRC = os.path.join(DATA_DIR, "character_eidolons.csv")
DST = os.path.join(DATA_DIR, "character_eidolons_rules.json")

ABILITY = r"(?:Basic ATK|Skill|Ultimate|Talent|Memosprite Skill|Memosprite Talent)"
SKILL_RE = re.compile(ABILITY + r" Lv\. \+\s*(\d)")
# The E1 blob concatenates all texts: "<t1> 1 <t1> Eidolon 1 <fx1>
# E 2 <t2> Eidolon 2 <fx2> ...". Split on the "E <n>" boundaries; the
# first chunk holds E1 (no "E 1" prefix in the scrape).
BLOB_SPLIT_RE = re.compile(r"\s*E\s+(\d)\s+")
E1_TITLE_RE = re.compile(r"^(.+?)\s+1\s+\1\s+Eidolon\s+1\b")
EN_TITLE_RE = re.compile(r"^(.+?)\s+Eidolon\s+(\d)\b")


def parse_blob_titles(blob):
    """True eidolon number -> title (only entries that parse cleanly)."""
    titles = {}
    chunks = BLOB_SPLIT_RE.split(blob)
    head = chunks[0]
    m = E1_TITLE_RE.match(head)
    if m:
        titles[1] = clean_title(m.group(1))
    for i in range(1, len(chunks) - 1, 2):
        try:
            num = int(chunks[i])
        except ValueError:
            continue
        m = EN_TITLE_RE.match(chunks[i + 1])
        if m and int(m.group(2)) == num:
            titles[num] = clean_title(m.group(1))
    return titles


def anchor_titles(blob, descs):
    """Fallback for blobs without E/Eidolon markers: each true effect text
    (rows E2..E7) is anchored in the blob by its opening words; the title
    is the text after the previous sentence-end (effects end with periods,
    titles contain none)."""
    titles = {}
    cursor = 0
    for i, desc in enumerate(descs):
        sentences = desc.split(". ")
        first = sentences[0][:80].strip()
        if len(first) < 20 and len(sentences) > 1:
            # Skill-level texts open with a short "Skill Lv. +2" sentence;
            # extend the anchor with the next sentence.
            first = (sentences[0] + ". " + sentences[1])[:80].strip()
        idx = blob.find(first, cursor)
        if idx < 0:
            continue
        seg = blob[cursor:idx]
        # Title follows the previous effect's final period.
        cut = seg.rfind(".")
        title = seg[cut + 1:] if cut >= 0 else seg
        title = re.sub(r"^[\s\"']+|[\s\"']+$", "", title).strip()
        title = re.sub(r"\s+", " ", title)
        if 0 < len(title) <= 64:
            titles[i + 1] = title
        cursor = idx + len(first)
    return titles


def dedupe_sentences(text):
    # Protect abbreviations ("Lv.", "1 .") from the sentence splitter.
    protected = text.replace("Lv.", "Lv<dot>")
    parts = [p.strip() for p in re.split(r"(?<=\.)\s+", protected) if p.strip()]
    kept = []
    seen = ""
    for p in parts:
        raw = p.replace("Lv<dot>", "Lv.")
        candidate = raw if raw.endswith(".") else raw + " ."
        if candidate in seen or raw in seen:
            continue
        kept.append(candidate)
        seen += " " + candidate
    out = " ".join(kept)
    return re.sub(r"\s+", " ", out).strip()


def clean_title(raw):
    title = re.sub(r"\s+\d+$", "", raw.strip())
    return re.sub(r"\s+", " ", title)


def main():
    with open(SRC, encoding="utf-8-sig") as f:
        rows = list(csv.DictReader(f))
    by_slug = {}
    for r in rows:
        by_slug.setdefault(r["slug"], {})[r["eidolon"]] = r

    eidolons = []
    warnings = []
    for slug, group in sorted(by_slug.items()):
        blob = (group.get("E1") or {}).get("description", "")
        titles = parse_blob_titles(blob)
        # Marker-less blobs (no "E N ... Eidolon N"): anchor fallback.
        if not titles:
            blob = (group.get("E1") or {}).get("description", "")
            # Variant stubs (e.g. blade-mortenax): the E1 cell is just the
            # E1 title with no effect text attached.
            if "." not in blob and 0 < len(blob.strip()) <= 80:
                titles[1] = clean_title(blob)
            descs = []
            for csv_n in range(2, 8):
                row = group.get("E" + str(csv_n))
                if row is None:
                    break
                descs.append(dedupe_sentences(row.get("description", "")))
            for k, v in anchor_titles(blob, descs).items():
                titles.setdefault(k, v)
        # True E1..E6 live in CSV rows E2..E7 (off by one).
        levels = []
        name = (group.get("E2") or {}).get("character", slug)
        for csv_n in range(2, 8):
            true_n = csv_n - 1
            row = group.get("E" + str(csv_n))
            if row is None:
                warnings.append(slug + ": missing row E" + str(csv_n))
                continue
            desc = dedupe_sentences(row.get("description", ""))
            # Every "X Lv. +N" raise is recorded (both +1 and +2 matter for
            # boosted manual scaling tables), deduped, in mention order.
            skills = []
            for m in SKILL_RE.finditer(desc):
                ability = re.match(ABILITY, m.group(0)).group(0)
                if ability not in skills:
                    skills.append(ability)
            title = titles.get(true_n, "")
            if not title:
                # Variant/collab stubs with no recoverable title: the
                # description and skillLevels (the valuable parts) are
                # intact; the display title falls back to "Eidolon N".
                title = "Eidolon " + str(true_n)
            levels.append({
                "eidolon": true_n,
                "title": title,
                "description": desc,
                "skillLevels": skills,
            })
        eidolons.append({"slug": slug, "name": name, "levels": levels})

    doc = {
        "schema_version": "1.0",
        "source_file": "character_eidolons.csv",
        "count": len(eidolons),
        "eidolons": eidolons,
    }
    with open(DST, "w", encoding="utf-8") as f:
        json.dump(doc, f, ensure_ascii=False, indent=1)
        f.write("\n")
    print("wrote " + DST + " (" + str(len(eidolons)) + " characters)")
    for w in warnings:
        print("WARN: " + w, file=sys.stderr)


if __name__ == "__main__":
    main()
