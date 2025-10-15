import json
import struct

'''
从 opening_book.json 文件生成 opening_book.bin 文件
opening_book.json 文件从以下 git 项目中获取:
https://github.com/quantumknight/minixiangqi_c
'''


def create_binary_opening_book(json_path, bin_path):
    try:
        with open(json_path, 'r') as f:
            book_data = json.load(f)
    except FileNotFoundError:
        print(f"Error: {json_path} not found.")
        return

    with open(bin_path, 'wb') as f:
        for hash_key_str, moves in book_data.items():
            hash_key = int(hash_key_str)
            for move in moves:
                # Assuming move is in the format [[r1, c1], [r2, c2]]
                from_sq = move[0][0] * 9 + move[0][1]
                to_sq = move[1][0] * 9 + move[1][1]
                # Pack hash_key (uint64), from_sq (int), and to_sq (int)
                # Using 'Qii' for unsigned long long, int, int
                packed_data = struct.pack('Qii', hash_key, from_sq, to_sq)
                f.write(packed_data)

    print(f"Successfully created binary opening book at {bin_path}")


if __name__ == "__main__":
    # The JSON file is in the root, but this script is in scripts/
    # So we need to go up one level.
    create_binary_opening_book('opening_book.json', 'opening_book.bin')
