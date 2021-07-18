/*
  Stockfish, a UCI chess playing engine derived from Glaurung 2.1
  Copyright (C) 2004-2021 The Stockfish developers (see AUTHORS file)

  Stockfish is free software: you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation, either version 3 of the License, or
  (at your option) any later version.

  Stockfish is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

//Definition of input features HalfKAv2 of NNUE evaluation function

#include "half_ka_v2.h"

#include "../../Position.h"

namespace Storm::NNUE {

  // Orient a square according to perspective (rotates by 180 for black)
  inline SquareIndex HalfKAv2::orient(Color perspective, SquareIndex s) {
    return SquareIndex(int(s) ^ (bool(perspective) * 56));
  }

  // Index of a feature for a given king position and another piece on some square
  inline IndexType HalfKAv2::make_index(Color perspective, SquareIndex s, ColorPiece pc, SquareIndex ksq) {
    return IndexType(int(orient(perspective, s)) + PieceSquareIndex[perspective][pc] + PS_NB * ksq);
  }

  // Get a list of indices for active features
  void HalfKAv2::append_active_indices(
    const Position& pos,
    Color perspective,
    ValueListInserter<IndexType> active
  ) {
    SquareIndex ksq = orient(perspective, pos.GetKingSquare(perspective));
    BitBoard bb = pos.GetPieces();
    while (bb)
    {
        SquareIndex s = PopLeastSignificantBit(bb);
        active.push_back(make_index(perspective, s, pos.GetPieceOnSquare(s), ksq));
    }
  }


  // append_changed_indices() : get a list of indices for recently changed features

  void HalfKAv2::append_changed_indices(
    SquareIndex ksq,
    StateInfo* st,
    Color perspective,
    ValueListInserter<IndexType> removed,
    ValueListInserter<IndexType> added
  ) {
    const auto& dp = st->DirtyPiece;
    SquareIndex oriented_ksq = orient(perspective, ksq);
    for (int i = 0; i < dp.dirty_num; ++i) {
      ColorPiece pc = dp.piece[i];
      if (dp.from[i] != SQUARE_INVALID)
        removed.push_back(make_index(perspective, dp.from[i], pc, oriented_ksq));
      if (dp.to[i] != SQUARE_INVALID)
        added.push_back(make_index(perspective, dp.to[i], pc, oriented_ksq));
    }
  }

  int HalfKAv2::update_cost(StateInfo* st) {
    return st->DirtyPiece.dirty_num;
  }

  int HalfKAv2::refresh_cost(const Position& pos) {
      return Popcount(pos.GetPieces());
  }

  bool HalfKAv2::requires_refresh(StateInfo* st, Color perspective) {
    return st->DirtyPiece.piece[0] == CreatePiece(PIECE_KING, perspective);
  }

}  // namespace Storm::NNUE::Features
