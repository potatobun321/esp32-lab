"""
Image to C Bitmap Converter for 128x64 Monochrome OLED (SSD1306)
Usage:
    python image_to_c.py input_image.png [output_name] [--width W] [--height H] [--invert]

Example:
    python image_to_c.py logo.png my_logo --width 48 --height 48
"""

import sys
import os
import argparse
from PIL import Image, ImageOps

def convert_image(input_path, output_name, max_w, max_h, invert, threshold):
    orig = Image.open(input_path).convert("RGBA")
    
    # Calculate aspect-ratio preserving dimensions
    orig_w, orig_h = orig.size
    ratio = min(max_w / orig_w, max_h / orig_h)
    new_w = max(1, int(orig_w * ratio))
    new_h = max(1, int(orig_h * ratio))
    
    # Resize
    resized = orig.resize((new_w, new_h), Image.Resampling.LANCZOS)
    
    # Create white or black background for transparency
    bg_color = (0, 0, 0, 255) if not invert else (255, 255, 255, 255)
    canvas = Image.new("RGBA", resized.size, bg_color)
    canvas.paste(resized, mask=resized.split()[3])
    
    # Grayscale
    gray = ImageOps.grayscale(canvas)
    if invert:
        gray = ImageOps.invert(gray)
        
    # Convert to 1-bit using Floyd-Steinberg dithering or threshold
    if threshold is not None:
        bw = gray.point(lambda p: 255 if p > threshold else 0, mode='1')
    else:
        bw = gray.convert("1", dither=Image.Dither.FLOYDSTEINBERG)
        
    byte_width = (new_w + 7) // 8
    bytes_list = []
    
    for y in range(new_h):
        for b in range(byte_width):
            val = 0
            for bit in range(8):
                x = b * 8 + bit
                if x < new_w and bw.getpixel((x, y)) > 0:
                    val |= (0x80 >> bit)
            bytes_list.append(val)
            
    header_filename = f"{output_name}_bitmap.h"
    with open(header_filename, "w") as f:
        f.write(f"#ifndef {output_name.upper()}_BITMAP_H\n")
        f.write(f"#define {output_name.upper()}_BITMAP_H\n\n")
        f.write("#include <stdint.h>\n\n")
        f.write(f"#define {output_name.upper()}_WIDTH  {new_w}\n")
        f.write(f"#define {output_name.upper()}_HEIGHT {new_h}\n\n")
        f.write(f"static const uint8_t {output_name}_bitmap[{len(bytes_list)}] = {{\n")
        for i in range(0, len(bytes_list), byte_width):
            chunk = bytes_list[i:i+byte_width]
            f.write("    " + ", ".join(f"0x{x:02X}" for x in chunk) + ",\n")
        f.write("};\n\n")
        f.write(f"#endif // {output_name.upper()}_BITMAP_H\n")
        
    print(f"Success! Generated '{header_filename}' ({new_w}x{new_h}, {len(bytes_list)} bytes)")
    
    # Print small ASCII preview
    print("\nPreview:")
    for y in range(0, new_h, 2):
        print("".join("#" if bw.getpixel((x, y)) > 0 else " " for x in range(new_w)))

if __name__ == "__main__":
    parser = argparse.ArgumentParser(description="Convert any image to a C bitmap header for SSD1306 OLED.")
    parser.add_argument("input", help="Input image file path")
    parser.add_argument("name", nargs="?", default="image", help="Base name for C array/macro (default: image)")
    parser.add_argument("--width", type=int, default=128, help="Max width (default: 128)")
    parser.add_argument("--height", type=int, default=64, help="Max height (default: 64)")
    parser.add_argument("--invert", action="store_true", help="Invert black/white pixels")
    parser.add_argument("--threshold", type=int, default=None, help="Fixed threshold 0-255 instead of dithering")
    
    args = parser.parse_args()
    if not os.path.exists(args.input):
        print(f"Error: file '{args.input}' not found.")
        sys.exit(1)
        
    convert_image(args.input, args.name, args.width, args.height, args.invert, args.threshold)

