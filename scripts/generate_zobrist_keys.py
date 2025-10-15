import random

print('#include "zobrist.h"\n')
print('#include <stdlib.h>\n')

# Use a fixed seed for deterministic keys
random.seed(0)

zobrist_keys = [[[0] * 9 for _ in range(10)] for _ in range(14)]
zobrist_player = 0

for i in range(14):
    for r in range(10):
        for c in range(9):
            zobrist_keys[i][r][c] = random.getrandbits(64)

zobrist_player = random.getrandbits(64)

print("uint64_t zobrist_keys[14][10][9] = {")
for i in range(14):
    print("    {")
    for r in range(10):
        print("        {", end="")
        for c in range(9):
            print(f"0x{zobrist_keys[i][r][c]:016x}ULL", end=", ")
        print("}, ")
    print("    }, ")
print("};")

print(f"\nuint64_t zobrist_player = 0x{zobrist_player:016x}ULL;")

print("\nvoid init_zobrist_keys() {")
print("    // Keys are pre-generated and hardcoded")
print("}")
