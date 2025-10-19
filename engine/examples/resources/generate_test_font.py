#!/usr/bin/env python3
"""
Generate a simple 8x8 test font with perfect grid alignment.
Creates a 128x128 PNG (16x16 grid of 8x8 glyphs).
"""

from PIL import Image, ImageDraw

# Create 128x128 image (16x16 grid of 8x8 glyphs)
# This gives us 256 possible glyphs (full extended ASCII)
img_size = 128
chars_per_row = 16
char_size = 8

img = Image.new('RGB', (img_size, img_size), color=(0, 0, 0))
draw = ImageDraw.Draw(img)

# Define some simple test patterns for key characters
def draw_glyph(draw, x, y, char_code):
    """Draw a simple glyph at grid position (x, y)"""
    base_x = x * char_size
    base_y = y * char_size

    # Define patterns for specific characters
    if char_code == ord(' '):  # Space
        pass  # Leave blank

    elif char_code == ord('H'):  # H
        # Two vertical lines with horizontal crossbar
        for dy in range(8):
            draw.point((base_x + 1, base_y + dy), fill=(255, 255, 255))
            draw.point((base_x + 6, base_y + dy), fill=(255, 255, 255))
        for dx in range(2, 6):
            draw.point((base_x + dx, base_y + 4), fill=(255, 255, 255))

    elif char_code == ord('E'):  # E
        for dy in range(8):
            draw.point((base_x + 1, base_y + dy), fill=(255, 255, 255))
        for dx in range(1, 7):
            draw.point((base_x + dx, base_y + 0), fill=(255, 255, 255))
            draw.point((base_x + dx, base_y + 4), fill=(255, 255, 255))
            draw.point((base_x + dx, base_y + 7), fill=(255, 255, 255))

    elif char_code == ord('L'):  # L
        for dy in range(8):
            draw.point((base_x + 1, base_y + dy), fill=(255, 255, 255))
        for dx in range(1, 7):
            draw.point((base_x + dx, base_y + 7), fill=(255, 255, 255))

    elif char_code == ord('O'):  # O
        # Draw a rectangle
        for dy in range(1, 7):
            draw.point((base_x + 1, base_y + dy), fill=(255, 255, 255))
            draw.point((base_x + 6, base_y + dy), fill=(255, 255, 255))
        for dx in range(2, 6):
            draw.point((base_x + dx, base_y + 1), fill=(255, 255, 255))
            draw.point((base_x + dx, base_y + 6), fill=(255, 255, 255))

    elif char_code == ord('W'):  # W
        for dy in range(7):
            draw.point((base_x + 1, base_y + dy), fill=(255, 255, 255))
            draw.point((base_x + 6, base_y + dy), fill=(255, 255, 255))
        for dy in range(4, 8):
            draw.point((base_x + 3, base_y + dy), fill=(255, 255, 255))
            draw.point((base_x + 4, base_y + dy), fill=(255, 255, 255))

    elif char_code == ord('R'):  # R
        # Left vertical line
        for dy in range(8):
            draw.point((base_x + 1, base_y + dy), fill=(255, 255, 255))
        # Top horizontal
        for dx in range(1, 6):
            draw.point((base_x + dx, base_y + 0), fill=(255, 255, 255))
        # Middle horizontal
        for dx in range(1, 6):
            draw.point((base_x + dx, base_y + 4), fill=(255, 255, 255))
        # Right side top
        for dy in range(1, 4):
            draw.point((base_x + 6, base_y + dy), fill=(255, 255, 255))
        # Diagonal leg
        for i in range(3):
            draw.point((base_x + 4 + i, base_y + 5 + i), fill=(255, 255, 255))

    elif char_code == ord('D'):  # D
        # Left vertical line
        for dy in range(8):
            draw.point((base_x + 1, base_y + dy), fill=(255, 255, 255))
        # Top and bottom
        for dx in range(1, 6):
            draw.point((base_x + dx, base_y + 0), fill=(255, 255, 255))
            draw.point((base_x + dx, base_y + 7), fill=(255, 255, 255))
        # Right side
        for dy in range(1, 7):
            draw.point((base_x + 6, base_y + dy), fill=(255, 255, 255))

    elif char_code == ord('0'):  # 0
        # Similar to O
        for dy in range(1, 7):
            draw.point((base_x + 1, base_y + dy), fill=(255, 255, 255))
            draw.point((base_x + 6, base_y + dy), fill=(255, 255, 255))
        for dx in range(2, 6):
            draw.point((base_x + dx, base_y + 1), fill=(255, 255, 255))
            draw.point((base_x + dx, base_y + 6), fill=(255, 255, 255))

    elif char_code >= ord('1') and char_code <= ord('9'):  # Numbers 1-9
        # Simple vertical line for all numbers (simplified)
        for dy in range(8):
            draw.point((base_x + 3, base_y + dy), fill=(255, 255, 255))
            draw.point((base_x + 4, base_y + dy), fill=(255, 255, 255))

    elif char_code == ord('!'):  # !
        for dy in range(5):
            draw.point((base_x + 3, base_y + dy), fill=(255, 255, 255))
            draw.point((base_x + 4, base_y + dy), fill=(255, 255, 255))
        draw.point((base_x + 3, base_y + 7), fill=(255, 255, 255))
        draw.point((base_x + 4, base_y + 7), fill=(255, 255, 255))

    elif char_code == ord('.'):  # .
        draw.point((base_x + 3, base_y + 7), fill=(255, 255, 255))
        draw.point((base_x + 4, base_y + 7), fill=(255, 255, 255))

    else:
        # For all other characters, draw a simple test pattern
        # A small square in the center
        for dy in range(3, 6):
            for dx in range(3, 6):
                draw.point((base_x + dx, base_y + dy), fill=(255, 255, 255))

# Generate glyphs for ASCII 0-255
for i in range(256):
    grid_x = i % chars_per_row
    grid_y = i // chars_per_row
    draw_glyph(draw, grid_x, grid_y, i)

# Save the image
output_path = 'test_font_8x8.png'
img.save(output_path)
print(f"Generated {output_path}: {img_size}x{img_size} pixels, {chars_per_row}x{chars_per_row} grid of {char_size}x{char_size} glyphs")
print(f"Total glyphs: {chars_per_row * chars_per_row}")
