
import re
import random

file_path = r"L:\C++\GameEngine\scenes\game.scene"

print(f"Reading {file_path}...")

with open(file_path, 'r') as f:
    lines = f.readlines()

new_lines = []
dummy_counter = 1

i = 0
while i < len(lines):
    line = lines[i]
    stripped = line.strip()
    
    # Identify NEW_ENTITY Dummy... (covers Dummy, Dummy1, Dummy_1, etc.)
    # We look for "NEW_ENTITY Dummy" at the start of the line
    if stripped.startswith("NEW_ENTITY Dummy"):
        # Rename to sequential Dummy1, Dummy2...
        new_lines.append(f"NEW_ENTITY Dummy{dummy_counter}\n")
        dummy_counter += 1
        
        # Look ahead for TRANSFORM line
        if i + 1 < len(lines) and lines[i+1].strip().startswith("TRANSFORM"):
            # Generate random transform
            x = random.uniform(-10.0, 10.0)
            y = random.uniform(10.0, 50.0)
            z = random.uniform(-10.0, 10.0)
            
            rx = random.uniform(0.0, 360.0)
            ry = random.uniform(0.0, 360.0)
            rz = random.uniform(0.0, 360.0)
            
            # Keep scale 0.01
            new_lines.append(f"TRANSFORM {x:.2f} {y:.2f} {z:.2f} {rx:.2f} {ry:.2f} {rz:.2f} 0.01 0.01 0.01\n")
            i += 1 # Skip the original TRANSFORM line
        else:
            # If next line isn't TRANSFORM, just proceed (or insert one? ignoring for now as per "modify existing")
            pass
    else:
        new_lines.append(line)
        
    i += 1

print(f"Writing back to {file_path}...")
with open(file_path, 'w') as f:
    f.writelines(new_lines)

print(f"Done. Processed {dummy_counter-1} Dummy entities.")
