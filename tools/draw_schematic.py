# Draws the Fuzz14 schematic as SVG.
#   python tools/draw_schematic.py images/schematic.svg           (README)
#   python tools/draw_schematic.py Source/skin.svg --skin         (the plugin's background: no
#                                                                 footer, the plugin draws its own)
import sys, math
skin = '--skin' in sys.argv
out = []
S = '#1d1d1f'; ACC = '#c0392b'; GREY = '#666'

def L(*p, w=2, c=S):
    d = ' '.join(f'{x},{y}' for x, y in zip(p[::2], p[1::2]))
    out.append(f'<polyline points="{d}" fill="none" stroke="{c}" stroke-width="{w}" stroke-linejoin="round" stroke-linecap="round"/>')
def T(x, y, s, size=14, anchor='middle', c=S, weight='normal'):
    s = s.replace('&', '&amp;')
    out.append(f'<text xml:space="preserve" x="{x}" y="{y}" font-size="{size}" text-anchor="{anchor}" fill="{c}" font-weight="{weight}">{s}</text>')
def dot(x, y): out.append(f'<circle cx="{x}" cy="{y}" r="4" fill="{S}"/>')
def gnd(x, y):
    L(x, y, x, y + 10); L(x - 14, y + 10, x + 14, y + 10); L(x - 9, y + 16, x + 9, y + 16); L(x - 4, y + 22, x + 4, y + 22)
def res_v(x, y1, y2):
    L(x, y1, x, y1 + 10); n = 6; h = (y2 - y1 - 20) / n; pts = [x, y1 + 10]
    for i in range(n): pts += [x + (9 if i % 2 == 0 else -9), y1 + 10 + h * (i + 0.5)]
    pts += [x, y2 - 10]; L(*pts); L(x, y2 - 10, x, y2)
def cap_v(x, y):
    L(x - 15, y - 5, x + 15, y - 5, w=3); L(x - 15, y + 5, x + 15, y + 5, w=3)
def cap_v_pol(x, y):   # straight plate (+) on top, curved (-) below
    L(x - 15, y - 5, x + 15, y - 5, w=3)
    out.append(f'<path d="M{x-15},{y+11} Q{x},{y+3} {x+15},{y+11}" fill="none" stroke="{S}" stroke-width="3"/>')
def cap_h(x, y, curved_left=False):
    if curved_left:
        out.append(f'<path d="M{x-10},{y-15} Q{x-3},{y} {x-10},{y+15}" fill="none" stroke="{S}" stroke-width="3"/>')
    else:
        L(x - 5, y - 15, x - 5, y + 15, w=3)
    L(x + 5, y - 15, x + 5, y + 15, w=3)
def diode_left(cx, y):  # anode right, stripe left
    out.append(f'<polygon points="{cx+9},{y-10} {cx+9},{y+10} {cx-8},{y}" fill="{S}"/>'); L(cx - 9, y - 10, cx - 9, y + 10, w=3)
def arrow(x1, y1, x2, y2):
    a = math.atan2(y2 - y1, x2 - x1); s = 10
    p = [(x2, y2), (x2 - s * math.cos(a - 0.4), y2 - s * math.sin(a - 0.4)), (x2 - s * math.cos(a + 0.4), y2 - s * math.sin(a + 0.4))]
    out.append('<polygon points="' + ' '.join(f'{a:.1f},{b:.1f}' for a, b in p) + f'" fill="{S}"/>')
def hop(x, y):  # a wire along y jumping over a vertical wire at x
    out.append(f'<path d="M{x-8},{y} A8,8 0 0 1 {x+8},{y}" fill="none" stroke="{S}" stroke-width="2"/>')
def jack(x, y):
    out.append(f'<circle cx="{x}" cy="{y}" r="9" fill="#fff" stroke="{S}" stroke-width="2.5"/>')

W, H = 860, 520
CX, BX = 480, 300          # collector line, base line

# battery + / VCC and the 10k
dot(CX, 40); T(CX + 14, 36, '+9 V', 15, 'start', weight='bold'); T(CX + 58, 36, '(battery +, VCC)', 12, 'start', c=GREY)
res_v(CX, 40, 110); T(CX - 22, 80, '10k', 14, 'end')
L(CX, 110, CX, 300)

# D1, D2 side by side, collector -> base
for y in (130, 165):
    dot(CX, y); L(CX, y, BX, y); diode_left(395, y)
dot(BX, 165)
T(352, 124, 'D1', 12, c=GREY); T(352, 159, 'D2', 12, c=GREY)
if not skin:   # the plugin puts its diode pickers here instead
    T(BX - 12, 150, '1N4148 ×2', 13, 'end'); T(BX - 12, 166, 'side by side', 12, 'end', c=GREY)

# 102K63 to the output jack
dot(CX, 235); L(CX, 235, 555, 235); cap_h(560, 235); L(565, 235, 660, 235); dot(660, 235)
T(560, 212, '1 nF', 13); T(560, 270, '102K63', 12, c=GREY)
L(660, 235, 728, 235); jack(740, 235); T(756, 231, 'OUT', 15, 'start', weight='bold'); T(756, 249, 'to amp', 12, 'start', c=GREY)

# D3 from the output back to the base, over the collector line
L(660, 235, 660, 200, CX + 8, 200); hop(CX, 200); L(CX - 8, 200, BX, 200); dot(BX, 200)
diode_left(395, 200); T(395, 222, 'D3' if skin else 'D3  1N4148', 12, c=GREY)

# base line
L(BX, 130, BX, 282)

# the MPSA13 Darlington
out.append('<circle cx="455" cy="320" r="78" fill="none" stroke="#999" stroke-width="1.5" stroke-dasharray="5 4"/>')
L(400, 262, 400, 302, w=4); L(400, 272, 425, 252, CX, 252); dot(CX, 252)
L(400, 292, 425, 312); arrow(400, 292, 425, 312)
L(445, 322, 445, 362, w=4); L(425, 312, 425, 342, 445, 342)
L(445, 332, CX, 312, CX, 300); L(445, 352, CX, 372); arrow(445, 352, CX, 372)
T(CX + 8, 284, '1 C', 12, 'start', c=ACC); T(392, 272, '2 B', 12, 'end', c=ACC); T(CX + 12, 408, '3 E', 12, 'start', c=ACC)
T(395, 412, 'MPSA13', 13, 'end', c=GREY); T(395, 428, '(Darlington)', 12, 'end', c=GREY)

# input
L(BX, 282, 400, 282); dot(BX, 282)
L(BX, 282, 236, 282); cap_h(226, 282, curved_left=True); L(221, 282, 120, 282)
T(234, 262, '+', 16, 'start', weight='bold'); T(212, 262, '–', 16, 'end', weight='bold')
T(200, 318, '2.2 µF  (m 2u2 50v)', 13); T(200, 334, '+ should face the base', 12, c=GREY)
jack(110, 282); T(94, 278, 'IN', 15, 'end', weight='bold'); T(94, 296, 'guitar / violin', 12, 'end', c=GREY)

# 334 from base to pin 3 / ground
L(BX, 282, BX, 340); cap_v(BX, 345); L(BX, 350, BX, 440); gnd(BX, 440)
T(BX + 24, 350, '330 nF', 13, 'start'); T(BX + 24, 366, '(334)', 12, 'start', c=GREY)

# pin 3 to ground
L(CX, 372, CX, 440); gnd(CX, 440)

notes = ['Diode stripes face the base. MPSA13 pins from the round side: 1 collector, 2 base, 3 emitter.',
         'Battery – is the ground rail; jack sleeves to ground. The 1 nF is the output cap, so only the buzzy top end gets through.']
for i, n in enumerate(notes): T(30, 482 + i * 16, n, 12, 'start', c='#555')
if not skin:
    T(W - 20, H - 6, 'Fuzz14  ·  Farica☆Kimora © Cursed Entertainment', 11, 'end', c='#888')

svg = f'''<svg xmlns="http://www.w3.org/2000/svg" viewBox="0 0 {W} {H}" width="{W}" height="{H}" font-family="Segoe UI, Helvetica, Arial, sans-serif">
<rect width="{W}" height="{H}" rx="10" fill="#ffffff"/>
''' + '\n'.join(out) + '\n</svg>\n'
open(sys.argv[1], 'w', encoding='utf-8').write(svg)
