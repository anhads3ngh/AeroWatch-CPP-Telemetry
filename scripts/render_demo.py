#!/usr/bin/env python3
"""Render real C++ test and synthetic telemetry output into an MP4 and README GIF.

This visualization is a replay with a visual overlay, not a project GUI.
Dependencies: Pillow and ffmpeg; C++ project must be built before running.
"""
from pathlib import Path
import os
import re
import subprocess
import tempfile
from PIL import Image, ImageDraw, ImageFont

ROOT = Path(__file__).resolve().parents[1]
OUT = ROOT / "media"
W, H, FPS, SECONDS = 1200, 676, 12, 16
BG, BOX, EDGE = "#09111f", "#111f31", "#293d52"
WHITE, MUTED = "#e6f0f9", "#89a1b7"
CYAN, GREEN, AMBER, RED = "#45d3e0", "#4bdea6", "#fac157", "#ff707c"

def run(args):
    proc = subprocess.run(args, cwd=ROOT, capture_output=True, text=True)
    if proc.returncode != 0:
        raise RuntimeError("Command failed: " + str(args) + "\n" + proc.stdout + "\n" + proc.stderr)
    return proc

def font(size, mono=False, bold=False):
    options = (
        ["/usr/share/fonts/truetype/dejavu/DejaVuSansMono.ttf"] if mono else
        ["/usr/share/fonts/truetype/dejavu/DejaVuSans-Bold.ttf" if bold else
         "/usr/share/fonts/truetype/dejavu/DejaVuSans.ttf"]
    )
    for p in options:
        if os.path.exists(p):
            return ImageFont.truetype(p, size)
    return ImageFont.load_default()

F16, F18, F21, F24, F30, F44 = (
    font(16, True), font(18), font(21), font(24, bold=True),
    font(30, bold=True), font(44, bold=True)
)

def label(draw, pos, value, color=WHITE, face=F21):
    draw.text(pos, value, font=face, fill=color)

def card(draw, coords, color=BOX, radius=16, border=None):
    draw.rounded_rectangle(coords, radius=radius, fill=color, outline=border, width=2)

def pill(draw, name, x, y, color=CYAN):
    width = draw.textbbox((0,0), name, font=F18)[2] + 28
    card(draw, (x,y,x+width,y+31), "#194051", 13)
    label(draw, (x+14,y+4), name, color, F18)

def frame(seconds, test_count, telemetry_rows, failures, summary, dest):
    im = Image.new("RGB", (W,H), BG)
    d = ImageDraw.Draw(im)
    d.rectangle((0,0,W,6), fill=CYAN)
    label(d, (58,35), "AEROWATCH", CYAN,F30)
    label(d, (294,40), "/ C++17 TELEMETRY MONITOR", WHITE,F21)
    pill(d, "OFFLINE SIMULATION",903,37)
    d.line((58,105,1142,105),fill=EDGE,width=2)

    card(d,(58,128,850,582),"#0c1828",17,EDGE)
    card(d,(58,128,850,177),"#182a3e",15)
    d.rectangle((59,158,849,177),fill="#182a3e")
    for x,color in zip((84,107,130),(RED,AMBER,GREEN)):
        d.ellipse((x,147,x+11,158),fill=color)
    label(d,(185,142),"aerowatch / terminal playback",MUTED,F18)

    if seconds < 3:
        lines = [
            ("$ cmake -S . -B build -DCMAKE_BUILD_TYPE=Release",CYAN),
            ("$ cmake --build build --config Release",CYAN),
            ("[100%] Built target aerowatch",GREEN),
            ("$ ctest --test-dir build --output-on-failure",CYAN),
            (f"100% tests passed, 0 tests failed out of {test_count}",GREEN),
            ("",WHITE),
            ("# Run deterministic synthetic telemetry",MUTED),
            ("$ ./build/aerowatch --samples 24 --faults",CYAN)
        ]
        for idx,(line,color) in enumerate(lines[:max(1,min(len(lines),int((seconds+.5)*3.2)))]):
            label(d,(80,198+39*idx),line,color,F16)
        progress = seconds/3
    else:
        progress = max(0,min(1,(seconds-3)/8.2))
        count = max(1,min(len(telemetry_rows),int(progress*len(telemetry_rows))))
        for idx,line in enumerate(telemetry_rows[max(0,count-10):count]):
            color = RED if "CRITICAL" in line else AMBER if "WARNING" in line else GREEN
            label(d,(80,192+idx*35),line[:75],color,F16)
        if seconds > 11.3:
            card(d,(74,510,832,564),"#273740",10)
            label(d,(88,524),summary[:82],CYAN,F16)
    d.rectangle((75,590,833,595),fill=EDGE)
    d.rectangle((75,590,75+int(758*progress),595),fill=CYAN)

    card(d,(878,128,1142,323),BOX,17,EDGE)
    label(d,(899,148),"RUN STATUS",MUTED,F18)
    if seconds < 3:
        label(d,(898,187),f"{test_count} / {test_count}",GREEN,F44)
        label(d,(899,252),"AUTOMATED TESTS",WHITE,F18)
        pill(d,"PASS",899,279,GREEN)
    else:
        count = max(1,min(len(telemetry_rows),int(progress*len(telemetry_rows))))
        label(d,(899,187),f"{count:02d} / {len(telemetry_rows):02d}",CYAN,F44)
        label(d,(899,252),"TELEMETRY ROWS",WHITE,F18)
        pill(d,"DETERMINISTIC",899,279,CYAN)

    card(d,(878,337,1142,582),BOX,17,EDGE)
    label(d,(899,358),"FAULT DETECTION",MUTED,F18)
    if seconds < 7.7:
        label(d,(900,402),"STANDBY",WHITE,F24)
        label(d,(900,447),"Validating messages",MUTED,F18)
        label(d,(900,473),"and state transitions",MUTED,F18)
    elif seconds < 10.1:
        label(d,(900,402),"REJECTED",AMBER,F24)
        label(d,(900,447),"Malformed CSV",WHITE,F18)
        label(d,(900,473),"packet detected",MUTED,F18)
    else:
        label(d,(900,402),f"{len(failures)} FAULTS",RED,F24)
        label(d,(900,447),"Malformed packet",WHITE,F18)
        label(d,(900,474),"Out-of-order packet",WHITE,F18)
        pill(d,"HANDLED SAFELY",899,522,GREEN)
    label(d,(58,612),"REPLAYED FROM REAL EXECUTABLE OUTPUT / VISUAL OVERLAY",MUTED,F18)
    label(d,(949,614),f"{int(seconds):02d}s / {SECONDS:02d}s",CYAN,F18)
    if seconds > 14.2:
        card(d,(170,238,1010,439),"#152c3f",25,CYAN)
        label(d,(236,272),"AEROWATCH - DEMO COMPLETE",WHITE,F30)
        label(d,(250,335),f"{test_count} tests passed | {len(failures)} invalid packets rejected",GREEN,F24)
        label(d,(244,381),"Synthetic data only - no vehicle control",MUTED,F21)
    im.save(dest)

def main():
    OUT.mkdir(parents=True,exist_ok=True)
    ctest = run(["ctest","--test-dir",str(ROOT/"build"),"-C","Release","--output-on-failure"]).stdout
    found = re.search(r"100% tests passed, 0 tests failed out of (\d+)",ctest)
    if not found: raise RuntimeError("Tests did not pass; refusing to publish demo")
    sample = run([str(ROOT/"build"/"aerowatch"),"--samples","24","--faults"])
    rows = [line for line in sample.stdout.splitlines()
            if re.match(r"^\s*(ALPHA01|BRAVO02)\s+\d+",line)]
    faults = [line for line in sample.stderr.splitlines() if "[REJECTED]" in line]
    summary = re.search(r"Telemetry received:[^\n]+",sample.stdout)
    if len(rows)!=22 or len(faults)!=2 or not summary:
        raise RuntimeError(f"Unexpected output: {len(rows)} rows and {len(faults)} rejected")
    with tempfile.TemporaryDirectory() as folder:
        p = Path(folder)
        for i in range(FPS*SECONDS):
            frame(i/FPS,int(found.group(1)),rows,faults,summary.group(0),p/f"frame-{i:04d}.png")
        run(["ffmpeg","-hide_banner","-loglevel","error","-y","-framerate",str(FPS),
             "-i",str(p/"frame-%04d.png"),"-c:v","libx264","-crf","23",
             "-preset","medium","-pix_fmt","yuv420p","-movflags","+faststart",str(OUT/"demo.mp4")])
        run(["ffmpeg","-hide_banner","-loglevel","error","-y","-i",str(OUT/"demo.mp4"),
             "-vf","fps=7,scale=720:-1:flags=lanczos,split[s0][s1];[s0]palettegen=max_colors=64:stats_mode=diff[p];[s1][p]paletteuse=dither=bayer:bayer_scale=3",
             "-loop","0",str(OUT/"demo.gif")])
    print(f"Published video and GIF based on real CLI: {found.group(1)} tests, {len(rows)} accepted, {len(faults)} rejected")

if __name__ == "__main__":
    main()
