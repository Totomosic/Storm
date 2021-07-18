import os
import sys
import argparse

WIDTH = 50
LINE_CHUNKS = 300

def format_byte(value):
    result = "{}".format(value)
    while len(result) < 3:
        result = " " + result
    return result

completed = 0

def save_progress(data, filename):
    with open(filename, "a") as f:
        f.write(data)

if __name__ == "__main__":
    parser = argparse.ArgumentParser()
    parser.add_argument("filename", type=str, help="Filename")
    parser.add_argument("--output_filename", type=str, default="EmbeddedNetwork.h")

    args = parser.parse_args()

    result = "const char EmbeddedNNUEData[] = {"

    open(args.output_filename, "w").close()

    with open(args.filename, "rb") as f:
        while True:
            chunk = f.read(WIDTH * LINE_CHUNKS)
            if chunk:
                for i in range(LINE_CHUNKS):
                    if i * WIDTH < len(chunk):
                        result += "\n\t"
                        for b in chunk[i * WIDTH : (i + 1) * WIDTH]:
                            result += "{},".format(b)
                completed += WIDTH * LINE_CHUNKS
                if completed > 5 * 1024 * 1024:
                    completed = 0
                    save_progress(result, args.output_filename)
                    result = ""
                print(completed / 1024, "kb")
            else:
                break

    result += "\n};\n"

    with open(args.output_filename, "a") as f:
        f.write(result)
