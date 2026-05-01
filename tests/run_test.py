import subprocess
import argparse
import sys

def run():
    parser = argparse.ArgumentParser()
    parser.add_argument("--compiler", required=True)
    parser.add_argument("--stdlib", required=True)
    parser.add_argument("--as", dest="assembler", required=True)
    parser.add_argument("--ld", required=True)
    parser.add_argument("--src", required=True)
    parser.add_argument("--expected", required=True)
    args = parser.parse_args()

    subprocess.run([args.compiler, args.src, "-o", "tmp.s"], check=True)

    subprocess.run([args.assembler, "-g", "tmp.s", "-o", "tmp.o"], check=True)

    subprocess.run([args.ld, "tmp.o", args.stdlib, "-o", "tmp.bin"], check=True)

    result = subprocess.run(["./tmp.bin"], capture_output=True, text=True)
    
    if result.stdout.strip() == args.expected.strip():
        print("PASS")
        sys.exit(0)
    else:
        print(f"FAIL:\nExpected:\n{args.expected}\nGot:\n{result.stdout}")
        sys.exit(1)

if __name__ == "__main__":
    run()
