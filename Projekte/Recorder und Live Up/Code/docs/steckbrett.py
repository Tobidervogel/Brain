"""Draws the breadboard build plan for the recorder (XIAO ESP32S3 Sense + Adafruit bq24074) as a PNG.

  python steckbrett.py out.png
"""

import sys

from PIL import Image, ImageDraw, ImageFont

W, H = 1500, 2400
P = 40                                   # hole pitch in pixels
COLS = {"a": 560, "b": 600, "c": 640, "d": 680, "e": 720, "f": 840, "g": 880, "h": 920, "i": 960, "j": 1000}
RAIL = {"L+": 440, "L-": 480, "R+": 1080, "R-": 1120}
TOP_ROWS, LOW_ROWS = range(1, 15), range(31, 57)
LOW_Y = 810                              # y of row 31

BLACK, RED, GREEN, WHITE, GREY = (40, 40, 40), (205, 40, 40), (30, 140, 50), (255, 255, 255), (120, 120, 120)
BLUE = (40, 90, 200)
BOARD, HOLE, EDGE = (247, 244, 235), (120, 116, 105), (150, 150, 150)
XIAO, CHARGER, GOLD = (52, 50, 78), (28, 62, 70), (214, 170, 60)


def font(size, bold=False):
    return ImageFont.truetype("C:/Windows/Fonts/arialbd.ttf" if bold else "C:/Windows/Fonts/arial.ttf", size)


F16, F18, F20, F22 = font(16), font(18), font(20), font(22)
B18, B20, B24, B30 = font(18, True), font(20, True), font(24, True), font(30, True)


def y_of(row):
    return 120 + (row - 1) * P if row <= 14 else LOW_Y + (row - 31) * P


def xy(hole):
    """'c11' -> pixel position; 'L-11' -> the hole of the left minus rail next to row 11."""
    if hole[0] in "LR":
        return RAIL[hole[:2]], y_of(int(hole[2:]))
    return COLS[hole[0]], y_of(int(hole[1:]))


img = Image.new("RGB", (W, H), WHITE)
d = ImageDraw.Draw(img)


def text(pos, s, f=F22, fill=BLACK, anchor="lm"):
    d.text(pos, s, font=f, fill=fill, anchor=anchor)


def board_section(rows):
    top, bottom = y_of(rows[0]) - 34, y_of(rows[-1]) + 34
    d.rectangle((400, top, 1160, bottom), fill=BOARD, outline=EDGE, width=2)
    for x in (420, 1060):
        d.line((x, top + 8, x, bottom - 8), fill=RED, width=4)
    for x in (500, 1140):
        d.line((x, top + 8, x, bottom - 8), fill=BLUE, width=4)
    d.rectangle((760, top, 800, bottom), fill=(232, 228, 216))       # centre channel
    for r in rows:
        y = y_of(r)
        for x in list(COLS.values()) + list(RAIL.values()):
            d.rectangle((x - 4, y - 4, x + 4, y + 4), fill=HOLE)
        f = B18 if r % 5 == 0 or r == 1 else F16
        text((530, y), str(r), f, fill=(90, 90, 90), anchor="rm")
        text((1030, y), str(r), f, fill=(90, 90, 90), anchor="lm")
    for name, x in COLS.items():
        text((x, top - 18), name, B20, fill=(90, 90, 90), anchor="mm")
    for key, sign in (("L+", "+"), ("L-", "−"), ("R+", "+"), ("R-", "−")):
        text((RAIL[key], top - 18), sign, B24, fill=RED if sign == "+" else BLUE, anchor="mm")


def marker(hole, number, colour, later=False, r=15):
    x, y = xy(hole)
    if later:
        d.ellipse((x - r, y - r, x + r, y + r), fill=WHITE, outline=colour, width=5)
        text((x, y), str(number), B18, fill=colour, anchor="mm")
    else:
        d.ellipse((x - r, y - r, x + r, y + r), fill=colour, outline=WHITE, width=2)
        text((x, y), str(number), B18, fill=WHITE, anchor="mm")


def label(target, s, side, ly, colour=BLACK):
    """Text in the left or right margin with a thin leader to a hole name or a pixel position."""
    x, y = xy(target) if isinstance(target, str) else target
    if side == "left":
        d.line((388, ly, x - 18, y), fill=colour, width=2)
        text((382, ly), s, F22, fill=colour, anchor="rm")
    else:
        d.line((1172, ly, x + 18, y), fill=colour, width=2)
        text((1178, ly), s, F22, fill=colour, anchor="lm")


def part(a, b, name, colour, vertical=False):
    (x1, y1), (x2, y2) = xy(a), xy(b)
    d.line((x1, y1, x2, y2), fill=EDGE, width=4)
    cx, cy = (x1 + x2) // 2, (y1 + y2) // 2
    w, h = (13, 36) if vertical else (30, 13)
    d.rounded_rectangle((cx - w, cy - h, cx + w, cy + h), radius=7, fill=colour, outline=(90, 90, 90), width=2)
    text((cx, cy), name, B18, anchor="mm")
    for x, y in ((x1, y1), (x2, y2)):
        d.ellipse((x - 7, y - 7, x + 7, y + 7), fill=EDGE, outline=(70, 70, 70))
    return cx, cy


text((W // 2, 34), "Recorder: Aufbau auf dem Steckbrett", B30, anchor="mm")
board_section(TOP_ROWS)
board_section(LOW_ROWS)
text((780, (y_of(14) + 34 + y_of(31) - 52) // 2), "Reihen 15 bis 30 bleiben frei", F20, fill=GREY, anchor="mm")

# ---- XIAO: pins in d6..d12 and h6..h12, USB end at row 12 ----
d.rounded_rectangle((662, y_of(6) - 46, 938, y_of(12) + 46), radius=12, fill=XIAO)
d.rounded_rectangle((752, y_of(12) + 30, 848, y_of(12) + 74), radius=6, fill=(170, 170, 178))
text((800, y_of(12) + 52), "USB", B18, anchor="mm")
text((800, y_of(6) - 24), "Antenne", F18, fill=(200, 200, 215), anchor="mm")
text((800, y_of(8) + 20), "XIAO", B24, fill=WHITE, anchor="mm")
text((800, y_of(9) + 20), "SD-Karte", F18, fill=(200, 200, 215), anchor="mm")
text((800, y_of(10) + 12), "am USB-Ende", F18, fill=(200, 200, 215), anchor="mm")
for i, (left, right) in enumerate(zip(("D7", "D8", "D9", "D10", "3V3", "GND", "5V"), ("D6", "D5", "D4", "D3", "D2", "D1", "D0"))):
    y = y_of(6 + i)
    for x in (COLS["d"], COLS["h"]):
        d.ellipse((x - 7, y - 7, x + 7, y + 7), fill=GOLD)
    used_l, used_r = left in ("GND", "5V"), right in ("D4", "D3", "D0")
    text((694, y), left, B20 if used_l else F18, fill=WHITE if used_l else (170, 170, 190))
    text((906, y), right, B20 if used_r else F18, fill=WHITE if used_r else (170, 170, 190), anchor="rm")

# ---- charger: header in i33..i43, USB-C at the top, the two white sockets at the bottom ----
d.rounded_rectangle((940, y_of(32) - 30, 1420, y_of(44) + 30), radius=12, fill=CHARGER)
d.rounded_rectangle((1040, y_of(32) - 46, 1150, y_of(32) + 6), radius=6, fill=(170, 170, 178))
text((1095, y_of(32) - 20), "USB-C", B18, anchor="mm")
for x, name in ((1180, "LiPo Batt"), (1300, "Load Out")):
    d.rounded_rectangle((x, y_of(44) - 10, x + 96, y_of(44) + 46), radius=4, fill=(236, 230, 214), outline=(150, 145, 130))
    text((x + 48, y_of(44) + 18), name, F16, anchor="mm")
text((1250, y_of(34)), "Lader", B24, fill=WHITE, anchor="mm")
text((1250, y_of(35)), "Adafruit bq24074", F20, fill=(190, 215, 220), anchor="mm")
for i, s in enumerate(("PGOOD ist der", "mittlere Stift:", "der 6. von oben", "und der 6. von unten")):
    text((1250, y_of(37) + 10 + i * 30), s, F18, fill=(190, 215, 220), anchor="mm")
for i, name in enumerate(("VBUS", "GND", "THERM", "ISET", "CE", "PGOOD", "CHG", "OUT", "GND", "LIPO", "GND")):
    y = y_of(33 + i)
    d.ellipse((COLS["i"] - 7, y - 7, COLS["i"] + 7, y + 7), fill=GOLD)
    used = 33 + i in (38, 39, 40, 41, 42)
    text((1012, y), name, B20 if used else F18, fill=WHITE if used else (150, 180, 186))

# ---- voltage divider at rows 49..54 ----
r1 = part("b49", "b52", "R1", (130, 190, 235), vertical=True)
r2 = part("a52", "L-52", "R2", (130, 190, 235))
c1 = part("c52", "L-54", "C", (235, 150, 70))

# ---- wires that stay inside one section are drawn, the long ones only get their number at both ends ----
d.line((*xy("a11"), *xy("L-11")), fill=BLACK, width=5)
gx, gy = xy("g42")
ax, ay = xy("a49")
d.line((gx, gy, gx - 70, gy + 90, ax + 60, ay - 60, ax, ay), fill=RED, width=5, joint="curve")

WIRES = [  # number, colour, from, to, later
    (1, BLACK, "a11", "L-11", False),
    (2, BLACK, "b11", "g41", False),
    (3, RED, "g42", "a49", False),
    (4, BLACK, "j12", "d52", False),
    (5, GREEN, "j9", "g38", True),
    (6, RED, "j8", "g39", True),
    (7, RED, "c12", "g40", True),
]
for n, colour, a, b, later in WIRES:
    marker(a, n, colour, later)
    marker(b, n, colour, later)

# ---- margin labels ----
label("L-11", "1  schwarz: a11 zur Minus-Schiene", "left", y_of(9))
label("b11", "2  schwarz: b11 nach g41", "left", y_of(10) + 10)
label("c12", "7  rot: c12 nach g40 (Stufe 2)", "left", y_of(13), RED)
label("j8", "6  rot: j8 nach g39 (Stufe 2)", "right", y_of(7), RED)
label("j9", "5  grün: j9 nach g38 (Stufe 2)", "right", y_of(9), GREEN)
label("j12", "4  schwarz: j12 nach d52", "right", y_of(12))
label("g38", "5  grün: g38 nach j9 (Stufe 2)", "left", y_of(36), GREEN)
label("g39", "6  rot: g39 nach j8 (Stufe 2)", "left", y_of(37) + 14, RED)
label("g40", "7  rot: g40 nach c12 (Stufe 2)", "left", y_of(39), RED)
label("g41", "2  schwarz: g41 nach b11", "left", y_of(41))
label("g42", "3  rot: g42 nach a49", "left", y_of(43) + 6, RED)
label("a49", "3  rot: a49, kommt von g42", "left", y_of(47) + 10, RED)
label((r1[0] + 4, r1[1]), "R1 100 kΩ: b49 nach b52", "left", y_of(50) + 10)
label((r2[0] - 14, r2[1]), "R2 100 kΩ: a52 zur Minus-Schiene", "left", y_of(52))
label((c1[0] - 14, c1[1]), "C 104: c52 zur Minus-Schiene", "left", y_of(54) + 14)
label("d52", "4  schwarz: d52, kommt von j12", "right", y_of(52))

# ---- legend ----
def legend(y, number, colour, later, name, holes, purpose):
    if later:
        d.ellipse((60, y - 15, 90, y + 15), fill=WHITE, outline=colour, width=5)
        text((75, y), str(number), B18, fill=colour, anchor="mm")
    else:
        d.ellipse((60, y - 15, 90, y + 15), fill=colour)
        text((75, y), str(number), B18, fill=WHITE, anchor="mm")
    text((108, y), name, F22)
    text((230, y), holes, F22)
    text((700, y), purpose, F22)


ly = y_of(56) + 84
text((60, ly), "Stufe 1: jetzt stecken (gefüllte Punkte)", B24)
legend(ly + 46, 1, BLACK, False, "schwarz", "a11 → Minus-Schiene links (blaue Linie)", "Minus vom XIAO")
legend(ly + 86, 2, BLACK, False, "schwarz", "b11 → g41", "Minus zum Lader")
legend(ly + 126, 3, RED, False, "rot", "g42 → a49", "LIPO zum Spannungsteiler")
legend(ly + 166, 4, BLACK, False, "schwarz", "j12 → d52", "Messpunkt zu D0")
text((60, ly + 210), "R1 und R2: 100 kΩ.  C: Keramikkondensator mit Aufdruck 104.  R2 und C enden in der blauen Minus-Reihe.", F22)

ly2 = ly + 274
text((60, ly2), "Stufe 2: erst nach dem Nachmessen (hohle Punkte)", B24)
legend(ly2 + 46, 5, GREEN, True, "grün", "g38 → j9", "PGOOD an D3")
legend(ly2 + 86, 6, RED, True, "rot", "g39 → j8", "CHG an D4")
legend(ly2 + 126, 7, RED, True, "rot", "g40 → c12", "OUT an 5V. Dann darf der XIAO nicht am PC hängen.")
text((60, ly2 + 176), "Es zählt die Reihe. Am XIAO sind links a, b, c frei und rechts i, j. Am Lader sind f, g, h frei.", F22, fill=GREY)

img.save(sys.argv[1])
print(img.size, "legend ends at", ly2 + 190)
