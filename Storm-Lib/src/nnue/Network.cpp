#include "Network.h"
#include "Epoch250.net"
#include <cmath>

namespace Storm
{

    std::array<std::array<int16_t, HIDDEN_NEURONS>, INPUT_NEURONS> Network::s_HiddenWeights = {};
    std::array<int16_t, HIDDEN_NEURONS> Network::s_HiddenBias = {};
    std::array<int16_t, HIDDEN_NEURONS> Network::s_OutputWeights = {};
    int16_t Network::s_OutputBias = 0;

    constexpr int16_t Multiplier = 1;

    template<typename T, size_t S>
    std::array<T, S> ReLU(const std::array<T, S>& src)
    {
        std::array<T, S> result;
        for (size_t i = 0; i < S; i++)
            result[i] = std::max(T(0), src[i]);
        return result;
    }

    template<typename T_In, typename T_Out, size_t S>
    void DotProduct(const std::array<T_In, S>& a, const std::array<T_In, S>& b, T_Out& output)
    {
        for (size_t i = 0; i < S; i++)
            output += a[i] * b[i];
    }

    void Network::RecalculateIncremental(const std::array<int16_t, INPUT_NEURONS>& inputs)
    {
        m_Zeta = { s_HiddenBias };
        for (size_t i = 0; i < HIDDEN_NEURONS; i++)
        {
            for (size_t j = 0; j < INPUT_NEURONS; j++)
                m_Zeta[0][i] += inputs[j] * s_HiddenWeights[j][i];
        }
    }

    void Network::ApplyDelta(const DeltaArray& delta)
    {
        m_Zeta.push_back(m_Zeta.back());
        auto& current = m_Zeta.back();
        for (size_t i = 0; i < delta.Size; i++)
        {
            if (delta.Deltas[i].Delta == 1)
            {
                for (size_t j = 0; j < HIDDEN_NEURONS; j++)
                    current[j] += s_HiddenWeights[delta.Deltas[i].Index][j];
            }
            else
            {
                for (size_t j = 0; j < HIDDEN_NEURONS; j++)
                    current[j] -= s_HiddenWeights[delta.Deltas[i].Index][j];
            }
        }
    }

    void Network::ApplyInverseDelta()
    {
        m_Zeta.pop_back();
    }

    ValueType Network::Evaluate() const
    {
        int32_t output = s_OutputBias * PRECISION;
        DotProduct(ReLU(m_Zeta.back()), s_OutputWeights, output);
        return output * Multiplier / PRECISION_SQUARED;
    }

    void Network::Init()
    {
        float* data = (float*)label;

        for (size_t i = 0; i < HIDDEN_NEURONS; i++)
        {
            s_HiddenBias[i] = int16_t(std::round(*data++ * PRECISION));
        }

        for (size_t i = 0; i < INPUT_NEURONS; i++)
        {
            for (size_t j = 0; j < HIDDEN_NEURONS; j++)
            {
                s_HiddenWeights[i][j] = int16_t(std::round(*data++ * PRECISION));
            }
        }

        s_OutputBias = int16_t(std::round(*data++ * PRECISION));

        for (size_t i = 0; i < HIDDEN_NEURONS; i++)
        {
            s_OutputWeights[i] = int16_t(std::round(*data++ * PRECISION));
        }
    }

}
