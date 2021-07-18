#pragma once
#include "Attacks.h"
#include "EvalConstants.h"
#include "Evaluation.h"
#include "Format.h"
#include "Move.h"
#include "MoveGeneration.h"
#include "Position.h"
#include "Search.h"
#include "Book.h"

namespace Storm
{

	void Init(const std::string& evalFilename = "nn-9e3c6298299a.nnue");

}
