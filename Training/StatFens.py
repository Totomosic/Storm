import os
import argparse

MIN_SCORE = -3000
MAX_SCORE = 3000
SCORE_RANGE = max(abs(MIN_SCORE), abs(MAX_SCORE))
BUCKET_SIZE = 50
NBUCKETS = int(SCORE_RANGE // BUCKET_SIZE)

class FileLine:
    def __init__(self, score, fen):
        self.fen = fen
        self.score = score

def parse_line(line: str):
    first_space = line.index(" ")
    score = int(line[0 : first_space])
    fen = line[first_space + 1:].rstrip().lstrip()

    return FileLine(score, fen)

def create_score_buckets():
    return [0 for i in range(NBUCKETS)]

def update_bucket(score, buckets):
    index = int(abs(score) / BUCKET_SIZE)
    buckets[min(index, len(buckets) - 1)] += 1

def get_bucket_label(index):
    min_score = BUCKET_SIZE * index
    max_score = min_score + BUCKET_SIZE - (1 if index != NBUCKETS - 1 else 0)
    return "{:4} - {:4}".format(min_score, max_score)

if __name__ == "__main__":
    parser = argparse.ArgumentParser()
    parser.add_argument("filename", type=str, help="Fen filename")

    args = parser.parse_args()

    unique_fens = set()
    total_fens = 0
    min_score = 0
    max_score = 0
    avg_score = 0

    buckets = create_score_buckets()

    with open(args.filename, "r") as f:
        lines = f.readlines()
        total_lines = len(lines)
        for line in lines:
            data = parse_line(line)
            total_fens += 1
            is_unique = not (data.fen in unique_fens)
            if is_unique:
                unique_fens.add(data.fen)
                min_score = min(min_score, data.score)
                max_score = max(max_score, data.score)
                avg_score += data.score / total_lines
                update_bucket(data.score, buckets)

    print("Total Fens", total_fens)
    print("Unique Fens", len(unique_fens))
    print("Min Score", min_score)
    print("Max Score", max_score)
    print("Average Score", int(avg_score))

    print("Score Histogram:")
    for i in range(len(buckets)):
        print("{}: {}".format(get_bucket_label(i), buckets[i]))
