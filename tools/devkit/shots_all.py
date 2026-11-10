#!/usr/bin/env python3
# shots_all.py - real screenshots of the firmware for every game at both
# sizes (Tom, 2026-11-10: README pictures he picks himself). The PNGs show
# game graphics and GOG / publisher art: SCRATCH ONLY, never the repo.
#
# The S3 QEMU builds draw into frame buffers (qemus3: the 4.0" 480x320;
# qemus3_50: the 5.0", the engine at 320x240) and TAPS.TXT's "shot <name>"
# dumps them (run_qemu.py --shots). Their card is read-only, so what the card
# scan makes comes from a scan on a WRITABLE card first: the classic QEMU
# builds qemu28 (320x240) / qemu35 (480x320) with every game (+ the Launcher
# folder for box art) - tools/devkit/shots_scan.sh - and each run's card here
# is one game folder + that scan's _CYD (LIBRARY.BIN cut to the game, its
# PROGRAM.BIN and library picture). The saves (slot D, placed for a nice view
# and with the fight's script block loaded) are made with the playsim
# beforehand: tools/devkit/shots_saves.sh. Fights start with TAPS.TXT's
# "runat <LOAD MONSTER address>" (the playsim's RUNAT; CYD_QEMU builds only).
#
#   shots_all.py [game ...] [--size 480|320] [--only explore,camp,fight,engine]
#
# env: QEMU (qemu-system-xtensa), GF (the game folders with the saves, one
# folder per short name: <GF>/<short>/<GOG folder>), SCAN (shots_scan.sh's
# output), OUT (the PNGs), WORK (cards, logs).
import argparse
import os
import shutil
import struct
import subprocess
import sys

here = os.path.dirname(os.path.abspath(__file__))
repo = os.path.dirname(os.path.dirname(here))
QEMU = os.environ.get("QEMU", "/tmp/claude-0/ag14/qemu/qemu/bin/qemu-system-xtensa")
GF = os.environ.get("GF", "/tmp/claude-0/shots/gf")
SCAN = os.environ.get("SCAN", "/tmp/claude-0/shots/scan")
OUT = os.environ.get("OUT", "/tmp/claude-0/shots/raw")
WORK = os.environ.get("WORK", "/tmp/claude-0/shots/work")

# Per game (canvas pixels - the canvas is 1:1 at the top left on both sizes):
# title taps before its Play word (x on row 24, None: no Play / Demo line),
# the party menu's Load line, "load from where" (Darkness-type), the save's
# word, Begin; Encamp and the camp's Exit (x, y); the fight's LOAD MONSTER
# address - all from the playsim's TAPLOG=1 and its snapshots
G = {
    "poolrad":   dict(steps=3, play=None, load=(24, 114), where=None, save=154, begin=(24, 154),
                      camp=(124, 194), exit=(300, 194), fight="85F9"),
    "curse":     dict(steps=4, play=242, load=(24, 114), where=None, save=154, begin=(24, 154),
                      camp=(124, 194), exit=(300, 194), fight="8AC2"),
    "secret":    dict(steps=5, play=114, load=(24, 114), where=None, save=170, begin=(24, 154),
                      camp=(164, 194), exit=(252, 194), fight="848A"),
    "darkness":  dict(steps=3, play=186, load=(24, 114), where=(138, 194), save=154, begin=(24, 154),
                      camp=(164, 194), exit=(188, 194), fight="85BB"),
    "gateway":   dict(steps=5, play=106, load=(24, 138), where=None, save=154, begin=(24, 162),
                      camp=(164, 194), exit=(300, 194), fight="85D4"),
    "treasures": dict(steps=5, play=122, load=(24, 114), where=(138, 194), save=154, begin=(24, 154),
                      camp=(164, 194), exit=(252, 194), fight="9826"),
    "champions": dict(steps=4, play=186, load=(24, 114), where=None, save=170, begin=(24, 154),
                      camp=(164, 194), exit=(252, 194), fight="8441"),
    "knights":   dict(steps=5, play=226, load=(24, 114), where=(138, 194), save=170, begin=(24, 154),
                      camp=(164, 194), exit=(252, 194), fight="815B"),
    # (the Dark Queen: save A stands in Palanthas' town menu - its exploring
    # picture; D outside Cergoth, where it camps and fights)
    "queen":     dict(steps=8, play=10, load=(172, 149), where=(146, 194), save=162, save_a=146,
                      begin=(172, 177), camp=(140, 190), exit=(296, 190), fight="8C11"),
}
ORDER = ["poolrad", "curse", "secret", "darkness", "gateway", "treasures", "champions", "knights", "queen"]

# The engine's keys (screen pixels; the emulator build prints each key's
# centre, "[qemu] key 'X' at x,y")
K = {
    480: dict(env="qemus3", scan="qemu35", home=(240, 180), play=(121, 180), settings=(359, 180),
              game=(400, 231), panel=(400, 100), left=(186, 260), select=(238, 260),
              tabs=dict(journal=(35, 20), pdf=(107, 20), sounds=(180, 20), settings=(244, 20), logs=(297, 20),
                        quick=(341, 20), cheats=(392, 20)),
              back=(240, 286)),
    320: dict(env="qemus3_50", scan="qemu28", home=(160, 140), play=(81, 134), settings=(240, 134),
              game=(19, 220), panel=None, left=(199, 220), select=(286, 220),
              tabs=dict(journal=(25, 14), pdf=(70, 14), sounds=(110, 14), settings=(143, 14), logs=(181, 14),
                        quick=(218, 14), cheats=(257, 14)),
              back=(160, 220)),
}


# Settings and the journal viewer (from the emulator's key lines)
E = {
    480: dict(help=(220, 120), help_close=(240, 160), next=(438, 286), head_back=(30, 18), jback=(40, 310),
              log_scan=(62, 286), send=(404, 20), send_back=(400, 286)),
    320: dict(help=(144, 93), help_close=(160, 120), next=(292, 217), head_back=(20, 14), jback=(25, 232),
              log_scan=(41, 217), send=(270, 14), send_back=(267, 217)),
}


def folder_of(short):
    return sorted(f for f in os.listdir(os.path.join(GF, short)) if f != "_CYD")[0]


def library_for(src, folder, dst):
    """LIBRARY.BIN with only the record of that folder"""
    b = open(src, "rb").read()
    size, n = struct.unpack("<II", b[4:12])
    recs = [b[12 + i * size:12 + (i + 1) * size] for i in range(n)]
    keep = [r for r in recs if r[:40].split(b"\0")[0].decode("latin-1") == folder]
    open(dst, "wb").write(b[:4] + struct.pack("<II", size, len(keep)) + b"".join(keep))


def journal_cut(src, dst, cap):
    """JOURNAL.DAT's first cap bytes: the entries whose pieces all lie there
    stay whole (the index is at the start, the pieces in order); a later
    entry would read past the end - the shots open only early ones"""
    b = open(src, "rb").read()
    open(dst, "wb").write(b[:cap])


def card(name, size, style, shorts, whole_library=False, journal=0):
    """A staging folder: the games' folders and the scan's _CYD for them"""
    d = os.path.join(WORK, name, "card")
    shutil.rmtree(os.path.join(WORK, name), ignore_errors=True)
    os.makedirs(os.path.join(d, "_CYD"))
    scan = os.path.join(SCAN, "%s-%s" % (K[size]["scan"], style), "_CYD")
    shutil.copy(os.path.join(scan, "SCAN.TXT"), os.path.join(d, "_CYD"))
    if whole_library:
        shutil.copy(os.path.join(scan, "LIBRARY.BIN"), os.path.join(d, "_CYD"))
        everyone = [f for f in os.listdir(scan) if os.path.isdir(os.path.join(scan, f))]
    else:
        library_for(os.path.join(scan, "LIBRARY.BIN"), folder_of(shorts[0]), os.path.join(d, "_CYD", "LIBRARY.BIN"))
        everyone = []
    for s in shorts:
        f = folder_of(s)
        subprocess.run(["cp", "-al", os.path.join(GF, s, f), d], check=True)
        everyone.append(f)
    for f in set(everyone):
        src = os.path.join(scan, f)
        if not os.path.isdir(src):
            continue
        os.makedirs(os.path.join(d, "_CYD", f), exist_ok=True)
        for x in os.listdir(src):
            # (the library pictures always; a game's program only with its
            # folder on the card; JOURNAL.DAT only when asked, its first
            # entries - Curse's is 15 MB, past the emulator's 13.5 MB card)
            if x.startswith(("ART", "BOX", "ICON")) or (x == "PROGRAM.BIN" and f in [folder_of(s) for s in shorts]):
                os.link(os.path.join(src, x), os.path.join(d, "_CYD", f, x))
            elif x == "JOURNAL.DAT" and journal and f in [folder_of(s) for s in shorts]:
                journal_cut(os.path.join(src, x), os.path.join(d, "_CYD", f, x), journal)
    return [os.path.join(d, x) for x in sorted(os.listdir(d))]


class Taps:
    def __init__(self, size):
        self.l = ["mem 8000"]
        self.size = size

    def tap(self, xy, wait=4000):
        self.l.append("%d %d %d" % (xy[0], xy[1], wait))

    def shot(self, name, wait=1500):
        self.l.append("shot %d_%s %d" % (self.size, name, wait))

    def text(self):
        return "\n".join(self.l + ["mem 1000"]) + "\n"


def to_game(t, s, slot="D"):
    """Home -> the game's page -> Play -> the title -> Load slot -> Begin"""
    g, k = G[s], K[t.size]
    t.tap(k["home"], 5000)
    t.tap(k["play"], 30000)
    for _ in range(g["steps"]):
        t.tap((2, 2), 4000)
    if g["play"] is not None:
        t.tap((g["play"], 194), 10000)
    t.tap(g["load"], 6000)
    if g["where"]:
        t.tap(g["where"], 6000)
    t.tap((g["save_a"] if slot == "A" else g["save"], 194), 15000)
    t.tap(g["begin"], 20000)


def game_run(s, size, only):
    g, k = G[s], K[size]
    t = Taps(size)
    to_game(t, s)
    if "explore" in only and s != "queen":
        t.shot("%s_explore" % s)
        if k["panel"]:
            t.tap(k["panel"], 3000)
            t.shot("%s_explore_party" % s)
            t.tap(k["panel"], 3000)
    if "camp" in only:
        t.tap(g["camp"], 8000)
        t.shot("%s_camp" % s)
        t.tap(g["exit"], 8000)
    if "fight" in only:
        t.l.append("runat %s 20000" % g["fight"])
        t.shot("%s_fight" % s)
        if k["panel"]:
            t.tap(k["panel"], 3000)
            t.shot("%s_fight_map" % s)
            t.tap(k["panel"], 3000)
            t.shot("%s_fight_party" % s)
            t.tap(k["panel"], 3000)
        # (the first fighter's Done / End - the menu line's last word: Left
        # from its first word, then Select - and the field after the turns
        # up to the next one's menu; twice)
        for _ in range(2):
            t.tap(k["left"], 3000)
            t.tap(k["select"], 12000)
        t.shot("%s_fight_round" % s)
    return t


def queen_explore_run(size):
    t = Taps(size)
    to_game(t, "queen", "A")
    for _ in range(3):              # (its arrival's pages, then the town's menu)
        t.tap((2, 2), 6000)
    t.shot("queen_explore")
    if K[size]["panel"]:
        t.tap(K[size]["panel"], 3000)
        t.shot("queen_explore_party")
    return t


def engine_discover(size):
    """Home -> Curse's page -> Settings: the keys' centres on the serial"""
    k = K[size]
    t = Taps(size)
    t.shot("d_home", 3000)
    t.tap((k["home"][0] * 5 // 3, k["home"][1]), 5000)
    t.tap(k["home"], 5000)
    t.shot("d_page")
    t.tap(k["settings"], 5000)
    t.shot("d_settings")
    return t


def engine_run(size):
    """The library (box art), Curse's page, Settings (its pages, a "?"), a
    journal entry, the Game menu's tabs - Curse's card with its journal's
    first entries"""
    k, e = K[size], E[size]
    t = Taps(size)
    t.shot("home_box", 3000)
    t.tap((k["home"][0] * 5 // 3, k["home"][1]), 5000)
    t.shot("home_box_curse")
    t.tap(k["home"], 5000)
    t.shot("page")
    t.tap(k["settings"], 5000)
    t.shot("settings_1")
    t.tap(e["help"], 4000)
    t.shot("settings_help")
    t.tap(e["help_close"], 4000)
    t.tap(e["next"], 4000)
    t.shot("settings_2")
    t.tap(e["next"], 4000)
    t.shot("settings_3")
    t.tap(e["next"], 4000)
    t.shot("settings_4")
    t.tap(e["head_back"], 5000)
    # (then the game, save D)
    g = G["curse"]
    t.tap(k["play"], 30000)
    for _ in range(g["steps"]):
        t.tap((2, 2), 4000)
    t.tap((g["play"], 194), 10000)
    t.tap(g["load"], 6000)
    t.tap((g["save"], 194), 15000)
    t.tap(g["begin"], 20000)
    t.l.append("journal J 1 15000")
    t.shot("journal_entry")
    t.tap(e["jback"], 6000)
    t.tap(k["game"], 6000)
    # (Journal PDF left out: the book needs the PDF on the card, and Curse's
    # 6.8 MB with the game is past the emulator's card)
    for tab in ("journal", "sounds", "settings", "logs", "quick", "cheats"):
        t.tap(k["tabs"][tab], 6000)
        if tab == "logs":
            # (the Logs screen of its own: the card scan's log, Send Logs' QR
            # code, back to the Game menu)
            t.tap(e["log_scan"], 5000)
            t.shot("menu_logs")
            t.tap(e["send"], 5000)
            t.shot("send_logs")
            t.tap(e["send_back"], 5000)
            t.tap(e["head_back"], 6000)
            continue
        t.shot("menu_" + tab)
    return t


def icon_run(size):
    t = Taps(size)
    t.shot("home_icon", 3000)
    t.tap((K[size]["home"][0] * 5 // 3, K[size]["home"][1]), 5000)
    t.shot("home_icon_curse")
    return t


def scan_run(size):
    """No library on the card: the scan at the start, its log once it's done
    (the emulator's card is read-only - its lines say what couldn't be saved)"""
    t = Taps(size)
    t.shot("scan", 3000)
    # (a tap on the log's top half scrolls it up, as a drag would)
    for _ in range(4):
        t.tap((size // 2, 60 if size == 480 else 45), 2500)
    t.shot("scan_top")
    return t


def run(name, size, games, taps, secs=900):
    w = os.path.join(WORK, name)
    open(os.path.join(w, "taps.txt"), "w").write(taps.text())
    cmd = ["python3", os.path.join(repo, "tools/qemu/run_qemu.py"), "--qemu", QEMU, "--s3", "--env", K[size]["env"],
           "--taps", os.path.join(w, "taps.txt"), "--seconds", str(secs), "--out", os.path.join(w, "q"),
           "--shots", OUT]
    for gdir in games:
        cmd += ["--game", gdir]
    log = os.path.join(w, "run.log")
    with open(log, "w") as f:
        subprocess.run(cmd, stdout=f, stderr=subprocess.STDOUT)
    shots = [l for l in open(log, errors="replace") if l.startswith("shot:") or "runat" in l or "Guru" in l]
    print("%s: %s" % (name, "".join(shots).strip()))


if __name__ == "__main__":
    ap = argparse.ArgumentParser()
    ap.add_argument("games", nargs="*")
    ap.add_argument("--size", type=int, action="append")
    ap.add_argument("--only", default="explore,camp,fight")
    ap.add_argument("--style", default="box")
    a = ap.parse_args()
    os.makedirs(OUT, exist_ok=True)
    only = a.only.split(",")
    for size in a.size or [480, 320]:
        for s in a.games or ORDER:
            if s == "engine":
                name = "%d_engine" % size
                run(name, size, card(name, size, "box", ["curse"], whole_library=True, journal=2600000), engine_run(size))
                name = "%d_icons" % size
                run(name, size, card(name, size, "icon", [], whole_library=True), icon_run(size), 200)
                name = "%d_scan" % size
                d = os.path.join(WORK, name, "card")
                shutil.rmtree(os.path.join(WORK, name), ignore_errors=True)
                os.makedirs(d)
                for x in ("secret", "champions", "knights"):
                    subprocess.run(["cp", "-al", os.path.join(GF, x, folder_of(x)), d], check=True)
                run(name, size, [os.path.join(d, x) for x in sorted(os.listdir(d))], scan_run(size), 900)
                continue
            if s == "panelcheck":           # (480: the 3D view after Companion panel taps - Darkness)
                name = "%d_panelcheck" % size
                t = Taps(size)
                to_game(t, "darkness")
                t.shot("pc_explore")
                t.tap(K[size]["panel"], 8000)
                t.shot("pc_party")
                t.tap(K[size]["panel"], 8000)
                t.shot("pc_map")
                t.tap((52, 194), 8000)          # (Area and back: the 3D view drawn again)
                t.tap((52, 194), 8000)
                t.shot("pc_after")
                run(name, size, card(name, size, a.style, ["darkness"]), t)
                continue
            if s == "queen_a":              # (the Dark Queen's exploring picture alone)
                name = "%d_queen_a" % size
                run(name, size, card(name, size, a.style, ["queen"]), queen_explore_run(size))
                continue
            if s == "discover":
                name = "%d_discover" % size
                run(name, size, card(name, size, a.style, ["curse"], whole_library=True), engine_discover(size), 300)
                continue
            name = "%d_%s" % (size, s)
            gdirs = card(name, size, a.style, [s])
            run(name, size, gdirs, game_run(s, size, only))
            if s == "queen" and "explore" in only:
                name = "%d_queen_a" % size
                gdirs = card(name, size, a.style, ["queen"])
                run(name, size, gdirs, queen_explore_run(size))
