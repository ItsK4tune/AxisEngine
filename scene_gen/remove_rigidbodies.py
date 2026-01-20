
import os

file_path = r"L:\C++\GameEngine\scenes\game.scene"
target_line = "RIGIDBODY CAPSULE 1.0 1.0 1.8 OFFSET 0.0 2.0 0.0 DYNAMIC"

print(f"Processing {file_path}...")

if not os.path.exists(file_path):
    print("File not found!")
    exit(1)

with open(file_path, 'r') as f:
    lines = f.readlines()

new_lines = []
removed_count = 0

for line in lines:
    if target_line in line:
        removed_count += 1
    else:
        new_lines.append(line)

with open(file_path, 'w') as f:
    f.writelines(new_lines)

print(f"Done. Removed {removed_count} lines.")
