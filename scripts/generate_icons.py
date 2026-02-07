import os
from PIL import Image, ImageDraw

def create_icon(name, draw_func, size=(32, 32)):
    img = Image.new('RGBA', size, (0, 0, 0, 0))
    draw = ImageDraw.Draw(img)
    draw_func(draw, size)
    path = os.path.join('assets/icons', f'{name}.png')
    img.save(path)
    print(f'Generated {path}')

def draw_play(draw, size):
    # Triangle
    points = [(8, 8), (24, 16), (8, 24)]
    draw.polygon(points, fill=(0, 255, 200, 255)) # Teal accent

def draw_pause(draw, size):
    # Two bars
    draw.rectangle([8, 8, 12, 24], fill=(0, 255, 200, 255))
    draw.rectangle([20, 8, 24, 24], fill=(0, 255, 200, 255))

def draw_next(draw, size):
    # Triangle + line
    draw.polygon([(8, 8), (20, 16), (8, 24)], fill=(0, 255, 200, 255))
    draw.rectangle([22, 8, 26, 24], fill=(0, 255, 200, 255))

def draw_prev(draw, size):
    # Line + triangle
    draw.rectangle([6, 8, 10, 24], fill=(0, 255, 200, 255))
    draw.polygon([(24, 8), (12, 16), (24, 24)], fill=(0, 255, 200, 255))

def draw_heart_filled(draw, size):
    # Simple pixel heart
    points = [
        (16, 26), (6, 16), (6, 10), (10, 6), (16, 10), (22, 6), (26, 10), (26, 16)
    ]
    draw.polygon(points, fill=(255, 100, 100, 255))

def draw_heart_outline(draw, size):
    # Simple pixel heart outline
    points = [
        (16, 26), (6, 16), (6, 10), (10, 6), (16, 10), (22, 6), (26, 10), (26, 16)
    ]
    draw.polygon(points, outline=(200, 200, 200, 255), width=2)

def draw_shuffle(draw, size):
    # Two crossing arrows (simplified)
    draw.line([6, 8, 26, 24], fill=(0, 255, 200, 255), width=2)
    draw.line([6, 24, 26, 8], fill=(0, 255, 200, 255), width=2)

def draw_repeat(draw, size):
    # Square arrow loop
    draw.rectangle([8, 8, 24, 24], outline=(0, 255, 200, 255), width=2)
    draw.polygon([(20, 4), (28, 8), (20, 12)], fill=(0, 255, 200, 255))

if __name__ == '__main__':
    os.makedirs('assets/icons', exist_ok=True)
    create_icon('play', draw_play)
    create_icon('pause', draw_pause)
    create_icon('next', draw_next)
    create_icon('prev', draw_prev)
    create_icon('heart_filled', draw_heart_filled)
    create_icon('heart_outline', draw_heart_outline)
    create_icon('shuffle', draw_shuffle)
    create_icon('repeat', draw_repeat)
