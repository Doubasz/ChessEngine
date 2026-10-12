import os
import subprocess
import sys

def run_engine_perft(engine_path, fen, depth):
    """Runs the chess engine's perft command and parses the output."""
    try:
        process = subprocess.run([engine_path, str(depth), fen], capture_output=True, text=True)
        output = process.stdout.strip()
        return parse_perft_output(output)
    except Exception as e:
        print(f"Error running engine: {e}")
        return None

def run_stockfish_perft(fen, depth):
    """Runs Stockfish's perft command and parses the output."""
    stockfish_cmd = "stockfish"  # Change this if stockfish is installed elsewhere
    try:
        process = subprocess.Popen(stockfish_cmd, stdin=subprocess.PIPE, stdout=subprocess.PIPE, text=True)
        process.stdin.write(f"position fen {fen}\n")
        process.stdin.write(f"go perft {depth}\n")
        process.stdin.write("quit\n")
        process.stdin.flush()
        output = process.communicate()[0]
        return parse_perft_output(output)
    except Exception as e:
        print(f"Error running Stockfish: {e}")
        return None

def parse_perft_output(output):
    """Parses perft output and returns a dictionary of move counts."""
    perft_data = {}
    for line in output.split("\n"):
        parts = line.split(":")
        if len(parts) == 2:
            move = parts[0].strip()
            count = parts[1].strip()
            if count.isdigit():
                perft_data[move] = int(count)
    return perft_data

def compare_perft(engine_perft, stockfish_perft):
    """Compares perft results and highlights discrepancies."""
    all_moves = set(engine_perft.keys()).union(set(stockfish_perft.keys()))
    differences = {}

    for move in all_moves:
        engine_count = engine_perft.get(move, 0)
        stockfish_count = stockfish_perft.get(move, 0)
        if engine_count != stockfish_count:
            differences[move] = (engine_count, stockfish_count)

    return differences

if __name__ == "__main__":
    # Usage: python tools/perftDiff.py <path to perft executable> <depth> [fen]
    if len(sys.argv) < 3:
        print("Usage: python tools/perftDiff.py <perft executable> <depth> [fen]")
        sys.exit(1)

    engine_path = os.path.abspath(sys.argv[1])  # Windows can't launch relative paths written with '/'
    depth = int(sys.argv[2])
    fen = " ".join(sys.argv[3:]) or "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1"

    print("Running your engine's perft...")
    engine_perft = run_engine_perft(engine_path, fen, depth)
    
    print("Running Stockfish's perft...")
    stockfish_perft = run_stockfish_perft(fen, depth)

    if engine_perft and stockfish_perft:
        differences = compare_perft(engine_perft, stockfish_perft)
        if differences:
            print("\nDiscrepancies found:")
            for move, (engine_count, stockfish_count) in differences.items():
                print(f"Move {move}: Your engine = {engine_count}, Stockfish = {stockfish_count}")
        else:
            print("\nNo differences found! Your perft matches Stockfish.")
    else:
        print("\nError: Could not retrieve perft data.")
