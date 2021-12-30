#pragma once
#include "Attacks.h"
#include "EvalConstants.h"
#include "Evaluation.h"
#include "Format.h"
#include "Move.h"
#include "MoveGeneration.h"
#include "Position.h"
#include "SearchThread.h"
#include "Book.h"

namespace Storm
{

    void Init(const std::string& evalFilename = "");

}
