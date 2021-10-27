%module Storm

%include "typemaps.i"
%include "std_vector.i"
%include "std_string.i"
%include "stdint.i"

%{
#include "Types.h"
#include "Storm.h"
using namespace Storm;
%}

%include "../Storm-Lib/src/Storm.h"
%include "../Storm-Lib/src/Types.h"
%include "../Storm-Lib/src/Bitboard.h"
%include "../Storm-Lib/src/Move.h"
%include "../Storm-Lib/src/Position.h"
%include "../Storm-Lib/src/Evaluation.h"
%include "../Storm-Lib/src/Format.h"
%include "../Storm-Lib/src/SearchThread.h"

%template(MoveVector) std::vector<Storm::Move>;
