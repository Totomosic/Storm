#pragma once
#include "Types.h"
#include <array>
#include <vector>

namespace Storm
{

	constexpr size_t INPUT_NEURONS = COLOR_PIECE_COUNT * SQUARE_MAX;
	constexpr size_t HIDDEN_NEURONS = 256;

	constexpr int16_t MAX_VALUE = 128;
	constexpr int16_t PRECISION = (size_t(std::numeric_limits<int16_t>::max()) + 1) / MAX_VALUE;
	constexpr int32_t PRECISION_SQUARED = int32_t(PRECISION) * int32_t(PRECISION);
	constexpr int32_t HALF_PRECISION_SQUARED = PRECISION_SQUARED / 2;
	constexpr int16_t HALF_PRECISION = PRECISION / 2;

	struct STORM_API DeltaArray
	{
	public:
		struct DeltaPoint
		{
		public:
			size_t Index;
			int16_t Delta;
		};

		size_t Size;
		DeltaPoint Deltas[4];
	};

	class STORM_API Network
	{
	private:
		std::vector<std::array<int16_t, HIDDEN_NEURONS>> m_Zeta;

		static std::array<std::array<int16_t, HIDDEN_NEURONS>, INPUT_NEURONS> s_HiddenWeights;
		static std::array<int16_t, HIDDEN_NEURONS> s_HiddenBias;
		static std::array<int16_t, HIDDEN_NEURONS> s_OutputWeights;
		static int16_t s_OutputBias;

	public:
		Network() = default;

		void RecalculateIncremental(const std::array<int16_t, INPUT_NEURONS>& inputs);
		void ApplyDelta(const DeltaArray& delta);
		void ApplyInverseDelta();
		ValueType Evaluate() const;

	public:
		static void Init();
	};

}
