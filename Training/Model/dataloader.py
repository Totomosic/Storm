from torch.utils.data import Dataset, DataLoader
import torch

from constants import SQUARE_COUNT, FILE_COUNT, RANK_COUNT, INPUT_NEURONS, SCORE_MULTIPLIER

PIECE_INDEX_MAP = {
    "P": 0,
    "N": 1,
    "B": 2,
    "R": 3,
    "Q": 4,
    "K": 5,
    "p": 6,
    "n": 7,
    "b": 8,
    "r": 9,
    "q": 10,
    "k": 11,
}

def parse_line(line: str):
    first_space = line.index(" ")
    score_string = line[0 : first_space]
    fen_string = line[first_space + 1:]
    score_multiplier = 1
    fen_space = fen_string.index(" ")
    if fen_space >= 0:
        next_fen_space = fen_string.index(" ", fen_space + 1)
        if next_fen_space >= 0:
            color_to_move = fen_string[fen_space + 1 : next_fen_space]
            if color_to_move == "b":
                score_multiplier = -1
    return int(score_string) * int(score_multiplier), fen_string.strip()

def fen_to_tensor(fen: str, device):
    def is_piece(c: str):
        return c in PIECE_INDEX_MAP

    def get_piece_index(c: str, file: int, rank: int):
        return PIECE_INDEX_MAP[c] * SQUARE_COUNT + (file + rank * FILE_COUNT)

    data = [0 for i in range(INPUT_NEURONS)]
    first_space = fen.index(" ")
    if first_space >= 0:
        fen = fen[0 : first_space]
    rank_parts = fen.split("/")
    assert len(rank_parts) == RANK_COUNT
    for i in range(len(rank_parts)):
        part = rank_parts[i]
        current_file = 0
        for c in part:
            if is_piece(c):
                idx = get_piece_index(c, current_file, RANK_COUNT - i - 1)
                assert data[idx] == 0
                data[idx] = 1
                current_file += 1
            else:
                current_file += int(c)
            assert current_file <= FILE_COUNT
    return torch.tensor(data, dtype=torch.float32, device=device)

class PositionsDataset(Dataset):
    def __init__(self, filename: str, device):
        with open(filename, "r") as f:
            self.lines = list(map(parse_line, f.readlines()))
        self.device = device

    def __len__(self):
        return len(self.lines)

    def __getitem__(self, idx):
        score, fen = self.lines[idx]
        return fen_to_tensor(fen, self.device), float(score) / SCORE_MULTIPLIER
