#!/usr/bin/env python3
"""Generate the 1-bit LVGL artwork and exact portrait previews. Requires Pillow.

The supplied PNG stays untouched. Derived images use its alpha and luminance,
with a solid dark core and dithered light facets for the monochrome nice!view.
The controller is drawn from the vector geometry below, without external art.
"""
from pathlib import Path
from PIL import Image, ImageChops, ImageDraw, ImageFilter

ROOT = Path(__file__).resolve().parents[1]
ASSETS = ROOT / 'assets/display'
SIZE = (68, 124)
FONT = {
    'A': [14, 17, 17, 31, 17, 17, 17],
    'E': [31, 16, 16, 30, 16, 16, 31],
    'F': [31, 16, 16, 30, 16, 16, 16],
    'G': [14, 17, 16, 23, 17, 17, 14],
    'L': [16, 16, 16, 16, 16, 16, 31],
    'M': [17, 27, 21, 21, 17, 17, 17],
    'N': [17, 25, 25, 21, 19, 19, 17],
    'S': [15, 16, 16, 14, 1, 1, 30],
    'Y': [17, 17, 10, 4, 4, 4, 4],
}


def caption(image, text, y):
    draw = ImageDraw.Draw(image)
    x = (image.width - (len(text) * 6 - 1)) // 2
    for char in text:
        for row, bits in enumerate(FONT[char]):
            for col in range(5):
                if bits & (1 << (4 - col)):
                    draw.point((x + col, y + row), fill=0)
        x += 6


def make_logo():
    source = Image.open(ASSETS / 'nyffels-logo.png').convert('RGBA')
    source = source.crop(source.getchannel('A').getbbox())
    source.thumbnail((58, 76), Image.Resampling.LANCZOS)
    alpha = source.getchannel('A')
    gray = source.convert('L').point(lambda v: max(0, min(255, (v - 51) * 255 // 204)))
    flat = Image.new('L', source.size, 255)
    flat.paste(gray, mask=alpha)
    mono = flat.convert('1')
    silhouette = alpha.point(lambda v: 255 if v >= 128 else 0)
    edge = ImageChops.subtract(silhouette, silhouette.filter(ImageFilter.MinFilter(3)))
    mono.paste(0, mask=edge)
    image = Image.new('1', SIZE, 1)
    image.paste(mono, ((SIZE[0] - mono.width) // 2, 21))
    caption(image, 'NYFFELS', 106)
    return image


def make_controller():
    # The rounded body and grips are sampled from the same quadratic segments
    # emitted to controller.svg, which is the editable vector source.
    commands = [('M', 17, 10), ('L', 47, 10), ('Q', 55, 10, 58, 21),
                ('L', 63, 38), ('Q', 64, 44, 57, 42), ('L', 44, 32),
                ('L', 20, 32), ('L', 7, 42), ('Q', 0, 44, 1, 38),
                ('L', 6, 21), ('Q', 9, 10, 17, 10)]
    points = []
    for command in commands:
        if command[0] in ('M', 'L'):
            points.append(tuple(command[1:]))
        else:
            start = points[-1]
            cx, cy, ex, ey = command[1:]
            for step in range(1, 17):
                t = step / 16
                points.append(((1-t)**2*start[0]+2*(1-t)*t*cx+t*t*ex,
                               (1-t)**2*start[1]+2*(1-t)*t*cy+t*t*ey))
    scale = 4
    art = Image.new('L', (64*scale, 48*scale), 255)
    draw = ImageDraw.Draw(art)
    draw.polygon([(round(x*scale), round(y*scale)) for x, y in points], fill=0)
    cross = [(14, 17), (19, 17), (19, 21), (23, 21), (23, 26),
             (19, 26), (19, 30), (14, 30), (14, 26), (10, 26), (10, 21), (14, 21)]
    draw.polygon([(x*scale, y*scale) for x, y in cross], fill=255)
    for cx, cy in [(46, 18), (52, 24), (40, 24), (46, 30)]:
        draw.ellipse(((cx-2)*scale, (cy-2)*scale, (cx+2)*scale, (cy+2)*scale), fill=255)
    draw.rounded_rectangle((28*scale, 22*scale, 35*scale, 24*scale), radius=scale, fill=255)
    art = art.resize((64, 48), Image.Resampling.LANCZOS).point(lambda v: 255 if v >= 128 else 0).convert('1')
    image = Image.new('1', SIZE, 1)
    image.paste(art, (2, 27))
    caption(image, 'GAME', 91)
    path = ' '.join(command[0]+' '+' '.join(map(str, command[1:])) for command in commands) + ' Z'
    cross_path = 'M '+' L '.join(f'{x} {y}' for x,y in cross)+' Z'
    svg = ('<svg xmlns="http://www.w3.org/2000/svg" viewBox="0 0 64 48">\n'
           f'  <path fill="black" d="{path}"/>\n'
           f'  <path fill="white" d="{cross_path}"/>\n'+
           ''.join(f'  <circle fill="white" cx="{x}" cy="{y}" r="2"/>\n' for x,y in [(46,18),(52,24),(40,24),(46,30)])+
           '  <rect fill="white" x="28" y="22" width="7" height="2" rx="1"/>\n</svg>\n')
    (ASSETS / 'controller.svg').write_text(svg, encoding='utf-8')
    return image


def c_image(name, portrait):
    # Hardware's native 160x68 coordinates are rotated relative to portrait.
    native = portrait.transpose(Image.Transpose.ROTATE_270)
    data = []
    for y in range(native.height):
        for start in range(0, native.width, 8):
            byte = 0
            for bit in range(8):
                x = start + bit
                if x < native.width and native.getpixel((x, y)) == 0:
                    byte |= 1 << (7-bit)
            data.append(byte)
    rows = '\n'.join('    '+', '.join(f'0x{v:02x}' for v in data[i:i+16])+',' for i in range(0, len(data), 16))
    return f'''static const uint8_t {name}_map[] = {{
#if CONFIG_NICE_VIEW_WIDGET_INVERTED
    0x00, 0x00, 0x00, 0xff, 0xff, 0xff, 0xff, 0xff,
#else
    0xff, 0xff, 0xff, 0xff, 0x00, 0x00, 0x00, 0xff,
#endif
{rows}
}};

const lv_img_dsc_t {name} = {{
    .header.cf = LV_IMG_CF_INDEXED_1BIT,
    .header.always_zero = 0,
    .header.reserved = 0,
    .header.w = {native.width},
    .header.h = {native.height},
    .data_size = sizeof({name}_map),
    .data = {name}_map,
}};
'''


def main():
    ASSETS.mkdir(parents=True, exist_ok=True)
    logo, controller = make_logo(), make_controller()
    logo.save(ASSETS / 'right-normal.png')
    controller.save(ASSETS / 'right-game.png')
    code = '/* SPDX-License-Identifier: MIT\n * Generated by scripts/generate_display_art.py; edit the source assets instead.\n */\n#include <lvgl.h>\n\n'
    code += c_image('nyffels_logo', logo) + '\n' + c_image('nyffels_controller', controller)
    (ROOT / 'src/display_art_images.c').write_text(code, encoding='utf-8')
    print('Generated two 68x124 portrait images and matching 124x68 LVGL images.')


if __name__ == '__main__':
    main()
