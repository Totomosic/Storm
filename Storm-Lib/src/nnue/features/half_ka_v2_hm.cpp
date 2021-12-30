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

//Definition of input features HalfKAv2_hm of NNUE evaluation function

#include "half_ka_v2_hm.h"

#include "../../position.h"

namespace Storm::NNUE
{

    // Orient a square according to perspective (rotates by 180 for black)
    inline SquareIndex HalfKAv2_hm::orient(Color perspective, SquareIndex s, SquareIndex ksq)
    {
        return SquareIndex(int(s) ^ (bool(perspective) * a8) ^ ((FileOf(ksq) < FILE_E) * h1));
    }

    // Index of a feature for a given king position and another piece on some square
    inline IndexType HalfKAv2_hm::make_index(Color perspective, SquareIndex s, ColorPiece pc, SquareIndex ksq)
    {
        SquareIndex o_ksq = orient(perspective, ksq, ksq);
        return IndexType(
          IndexType(orient(perspective, s, ksq)) + PieceSquareIndex[perspective][pc] + PS_NB * KingBuckets[o_ksq]);
    }

    // Get a list of indices for active features
    void HalfKAv2_hm::append_active_indices(const Position& pos, Color perspective, IndexList& active)
    {
        SquareIndex ksq = pos.GetKingSquare(perspective);
        BitBoard bb = pos.GetPieces();
        while (bb)
        {
            SquareIndex s = PopLeastSignificantBit(bb);
            active.push_back(make_index(perspective, s, pos.GetPieceOnSquare(s), ksq));
        }
    }

    // append_changed_indices() : get a list of indices for recently changed features

    void HalfKAv2_hm::append_changed_indices(
      SquareIndex ksq, const DirtyPiece& dp, Color perspective, IndexList& removed, IndexList& added)
    {
        for (int i = 0; i < dp.dirty_num; ++i)
        {
            if (dp.from[i] != SQUARE_INVALID)
                removed.push_back(make_index(perspective, dp.from[i], dp.piece[i], ksq));
            if (dp.to[i] != SQUARE_INVALID)
                added.push_back(make_index(perspective, dp.to[i], dp.piece[i], ksq));
        }
    }

    int HalfKAv2_hm::update_cost(const StateInfo* st)
    {
        return st->DirtyPiece.dirty_num;
    }

    int HalfKAv2_hm::refresh_cost(const Position& pos)
    {
        return Popcount(pos.GetPieces());
    }

    bool HalfKAv2_hm::requires_refresh(const StateInfo* st, Color perspective)
    {
        return st->DirtyPiece.piece[0] == CreatePiece(PIECE_KING, perspective);
    }

}   // namespace Stockfish::Eval::NNUE::Features
