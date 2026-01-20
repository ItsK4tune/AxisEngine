
import random
import os

file_path = r"L:\C++\GameEngine\scenes\game.scene"

# Determine start index
max_index = 0

if os.path.exists(file_path):
    with open(file_path, 'r') as f:
        for line in f:
            stripped = line.strip()
            if stripped.startswith("NEW_ENTITY Dummy"):
                try:
                    # Extract number part if any "NEW_ENTITY Dummy123"
                    suffix = stripped.replace("NEW_ENTITY Dummy", "")
                    if suffix.isdigit():
                        val = int(suffix)
                        if val > max_index:
                            max_index = val
                except:
                    pass
else:
    print("Scene file not found!")
    exit(1)

print(f"Found max Dummy index: {max_index}")
start_index = max_index + 1
count_to_add = 1000

print(f"Appending {count_to_add} dummies starting from {start_index}...")

with open(file_path, 'a') as f:
    f.write("\n")
    for i in range(count_to_add):
        idx = start_index + i
        
        # Increased range for 1000 objects
        x = random.uniform(-100.0, 100.0) 
        y = random.uniform(10.0, 150.0)
        z = random.uniform(-100.0, 100.0)
        
        rx = random.uniform(0.0, 360.0)
        ry = random.uniform(0.0, 360.0)
        rz = random.uniform(0.0, 360.0)
        
        f.write(f"NEW_ENTITY Dummy{idx}\n")
        f.write(f"TRANSFORM {x:.2f} {y:.2f} {z:.2f} {rx:.2f} {ry:.2f} {rz:.2f} 0.01 0.01 0.01\n")
        f.write("RENDERER dummyModel phongLitNoShadowShader\n")
        f.write("MATERIAL PHONG 32 0.5 0.5 0.5\n")
        f.write("RIGIDBODY CAPSULE 1.0 1.0 1.8 OFFSET 0.0 2.0 0.0 DYNAMIC\n\n")

print(f"Successfully appended {count_to_add} dummies.")
