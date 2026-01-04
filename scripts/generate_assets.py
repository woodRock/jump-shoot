import os
import sys
import random
import math

try:
    from PIL import Image, ImageDraw, ImageColor
except ImportError:
    print("Pillow (PIL) is not installed. Please install it using: pip install Pillow")
    sys.exit(1)

ASSET_DIR = "assets"

def ensure_dir(directory):
    if not os.path.exists(directory):
        os.makedirs(directory)

def generate_wall_texture(filename, color_base, color_detail):
    size = 64
    img = Image.new('RGBA', (size, size), color_base)
    draw = ImageDraw.Draw(img)
    
    # Brick pattern
    brick_height = 16
    brick_width = 32
    
    for y in range(0, size, brick_height):
        offset = 0 if (y // brick_height) % 2 == 0 else brick_width // 2
        for x in range(-brick_width // 2, size, brick_width):
            draw.rectangle(
                [x + offset + 1, y + 1, x + offset + brick_width - 1, y + brick_height - 1],
                fill=color_detail
            )
            
    img.save(os.path.join(ASSET_DIR, filename))
    print(f"Generated {filename}")

def generate_floor_texture(filename, color1, color2):
    size = 64
    img = Image.new('RGBA', (size, size), color1)
    draw = ImageDraw.Draw(img)
    
    # Checkerboard / Noise
    for i in range(100):
        x = random.randint(0, size-1)
        y = random.randint(0, size-1)
        draw.point((x, y), fill=color2)
        
    img.save(os.path.join(ASSET_DIR, filename))
    print(f"Generated {filename}")

def generate_bow_sprite(filename, state):
    # state: 0=idle, 1=draw
    size = 300 # Large for UI overlay
    img = Image.new('RGBA', (size, size), (0,0,0,0))
    draw = ImageDraw.Draw(img)
    
    center_x = size // 2
    center_y = size # Bottom center
    
    # Bow color
    wood = (139, 69, 19)
    string_color = (200, 200, 200)
    
    # Bow curve
    # Simple arc
    
    offset_y = 50 if state == 1 else 0
    width_mod = 20 if state == 1 else 0
    
    # Draw bow limbs
    draw.arc([center_x - 100 - width_mod, center_y - 250 + offset_y, center_x + 100 + width_mod, center_y + 50], 
             start=0, end=180, fill=wood, width=15)
             
    # Draw String
    string_pull_y = center_y - 50 if state == 0 else center_y - 20 # Pulled back
    
    left_tip = (center_x - 100 - width_mod, center_y - 100 + offset_y) # Approx
    right_tip = (center_x + 100 + width_mod, center_y - 100 + offset_y)
    
    draw.line([left_tip, (center_x, string_pull_y), right_tip], fill=string_color, width=3)
    
    # Arrow (only if drawing)
    if state >= 0: # Always show arrow for now
        arrow_y_tip = center_y - 300 if state == 0 else center_y - 280
        arrow_y_end = string_pull_y
        
        # Shaft
        draw.line([(center_x, arrow_y_tip), (center_x, arrow_y_end)], fill=(210, 180, 140), width=5)
        # Fletching
        draw.polygon([(center_x, arrow_y_end), (center_x - 10, arrow_y_end + 20), (center_x + 10, arrow_y_end + 20)], fill=(255, 0, 0))
        # Tip
        draw.polygon([(center_x, arrow_y_tip), (center_x - 5, arrow_y_tip + 10), (center_x + 5, arrow_y_tip + 10)], fill=(100, 100, 100))

    img.save(os.path.join(ASSET_DIR, filename))
    print(f"Generated {filename}")

def generate_target_sprite(filename):
    size = 64
    img = Image.new('RGBA', (size, size), (0,0,0,0))
    draw = ImageDraw.Draw(img)
    
    # Stand
    draw.rectangle([28, 40, 36, 64], fill=(100, 50, 0))
    
    # Target circles
    colors = [(255, 0, 0), (255, 255, 255), (255, 0, 0), (255, 255, 255)]
    radii = [20, 15, 10, 5]
    
    center = (32, 32)
    
    for r, col in zip(radii, colors):
        draw.ellipse([center[0]-r, center[1]-r, center[0]+r, center[1]+r], fill=col, outline=(0,0,0))
        
    img.save(os.path.join(ASSET_DIR, filename))
    print(f"Generated {filename}")

def generate_target_destroyed_sprite(filename):
    size = 64
    img = Image.new('RGBA', (size, size), (0,0,0,0))
    draw = ImageDraw.Draw(img)
    
    # Broken bits
    draw.rectangle([28, 50, 36, 64], fill=(100, 50, 0)) # Stump
    
    # Debris
    for i in range(10):
        x = random.randint(10, 54)
        y = random.randint(40, 60)
        draw.rectangle([x, y, x+4, y+4], fill=(255, 0, 0))

    img.save(os.path.join(ASSET_DIR, filename))
    print(f"Generated {filename}")
    
def main():
    ensure_dir(ASSET_DIR)
    
    generate_wall_texture("wall_brick.png", (100, 100, 100, 255), (150, 100, 100, 255))
    generate_wall_texture("wall_mossy.png", (80, 100, 80, 255), (50, 120, 50, 255))
    generate_floor_texture("floor_grass.png", (34, 139, 34, 255), (0, 100, 0, 255))
    
    generate_bow_sprite("bow_idle.png", 0)
    generate_bow_sprite("bow_draw.png", 1)
    
    generate_target_sprite("target.png")
    generate_target_destroyed_sprite("target_broken.png")
    
    # UI Crosshair
    img = Image.new('RGBA', (32, 32), (0,0,0,0))
    draw = ImageDraw.Draw(img)
    draw.line([16, 8, 16, 24], fill=(255, 255, 255, 200), width=2)
    draw.line([8, 16, 24, 16], fill=(255, 255, 255, 200), width=2)
    img.save(os.path.join(ASSET_DIR, "crosshair.png"))
    print("Generated crosshair.png")

if __name__ == "__main__":
    main()
