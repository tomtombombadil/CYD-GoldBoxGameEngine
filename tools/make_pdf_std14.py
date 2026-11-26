"""Writes src/engine/pdf_std14.h: the widths of PDF's standard fonts (Times, Helvetica; Courier is 600 throughout)
for WinAnsi's codes 32 - 255, from Adobe's AFM files of the 14 core fonts.

A PDF may name one of these fonts without its own /Widths (the Steam journals' OCR text layer: Times-Roman); the
text reader then needs the font's widths to tell a word's letters from the gap between words.

Usage: python tools/make_pdf_std14.py <folder of the core fonts' .afm files>
(matplotlib ships them: mpl-data/fonts/pdfcorefonts.)
"""
import os
import sys

from fontTools import agl

FONTS = ["Times-Roman", "Times-Bold", "Times-Italic", "Times-BoldItalic", "Helvetica", "Helvetica-Bold"]


def widths(path):
    w = {}
    for line in open(path, encoding="latin-1"):
        if not line.startswith("C "):
            continue
        parts = [p.strip() for p in line.split(";")]
        wx = name = None
        for p in parts:
            if p.startswith("WX "):
                wx = int(p[3:])
            elif p.startswith("N "):
                name = p[2:]
        if name is not None and wx is not None:
            w[name] = wx
    return w


def main():
    folder = sys.argv[1]
    out = ["// Made by tools/make_pdf_std14.py from Adobe's AFM files of the PDF core fonts - do not edit.",
           "// The widths are Adobe's: Copyright (c) 1985 - 1997 Adobe Systems",
           "// Incorporated. All Rights Reserved. Modified: the widths only, as tables by WinAnsi code (their terms:",
           "// THIRD_PARTY_NOTICES.md). Times is a trademark of",
           "// Linotype-Hell AG and/or its subsidiaries; Helvetica of Linotype-Hell AG.",
           "// Each table: WinAnsi's codes 32 - 255 in order, in 1/1000 of the size (0: no such letter).",
           "#pragma once", "#include <stdint.h>", "", "namespace pdf {", "namespace std14 {", ""]
    for name in FONTS:
        w = widths(os.path.join(folder, name + ".afm"))
        row = []
        for code in range(32, 256):
            ch = bytes([code]).decode("cp1252", errors="ignore")
            g = agl.UV2AGL.get(ord(ch)) if ch else None
            if code == 0xA0:
                g = "space"
            if code == 0xAD:
                g = "hyphen"
            row.append(w.get(g, 0) if g else 0)
        ident = name.replace("-", "_")
        out.append("const uint16_t k%s[224] = {" % ident)
        for i in range(0, 224, 16):
            out.append("    " + ", ".join(str(v) for v in row[i:i + 16]) + ",")
        out.append("};")
        out.append("")
    out += ["} // namespace std14", "} // namespace pdf", ""]
    open(os.path.join(os.path.dirname(__file__), "..", "src", "engine", "pdf_std14.h"), "w", newline="\n").write(
        "\n".join(out))


if __name__ == "__main__":
    main()
