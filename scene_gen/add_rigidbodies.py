
import os

file_path = r"L:\C++\GameEngine\scenes\game.scene"
rigidbody_line = "RIGIDBODY CAPSULE 1.0 1.0 1.8 OFFSET 0.0 2.0 0.0 DYNAMIC\n"

print(f"Processing {file_path}...")

if not os.path.exists(file_path):
    print("File not found!")
    exit(1)

with open(file_path, 'r') as f:
    lines = f.readlines()

new_lines = []
is_dummy = False
added_count = 0

for i, line in enumerate(lines):
    new_lines.append(line)
    
    stripped = line.strip()
    if stripped.startswith("NEW_ENTITY Dummy") and not stripped.startswith("NEW_ENTITY VideoDummy"):
        is_dummy = True
    elif stripped.startswith("NEW_ENTITY"):
        is_dummy = False
        
    if is_dummy and stripped.startswith("MATERIAL PHONG"):
        # Check if next line is already a RIGIDBODY (of any kind)
        already_has_rb = False
        if i + 1 < len(lines):
             if lines[i+1].strip().startswith("RIGIDBODY"):
                 already_has_rb = True
        
        if not already_has_rb:
             new_lines.append(rigidbody_line)
             added_count += 1

print(f"Writing back to {file_path}...")
with open(file_path, 'w') as f:
    f.writelines(new_lines)

print(f"Done. Added {added_count} Rigidbody lines.")
