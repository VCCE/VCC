// bandlimiter.h
//
// Resamples the YM2149's internal-rate output (one value per chip tick,
// 250kHz) to the host audio rate with a windowed-sinc low-pass, so the
// square-wave harmonics above the host Nyquist frequency are removed
// instead of folding back into the audible band as inharmonic tones.
//
// Push() appends one input sample per chip tick. At(t) returns the
// filtered signal at fractional input position t; it needs input up to
// t + HalfWidth(), so the output runs HalfWidth() ticks (under 1ms)
// behind the newest input.

#pragma once
#include <cstdint>
#include <vector>

namespace psg {

class BandLimiter {
public:
    explicit BandLimiter(double input_rate);

    // Rebuild the filter for this output rate (cutoff 0.43 x rate).
    void SetOutputRate(double output_rate);
    double OutputRate() const { return m_output_rate; }

    void Reset();
    // The ring is stored twice over so any filter window is contiguous.
    void Push(float x)
    {
        const size_t i = size_t(m_count++) & kMask;
        m_ring[i] = x;
        m_ring[i + kRingSize] = x;
    }
    int64_t Count() const { return m_count; }
    int HalfWidth() const { return m_half; }

    double At(double t) const;

private:
    static constexpr size_t kRingSize = 8192;   // > 2 x max half-width + slack
    static constexpr size_t kMask = kRingSize - 1;
    static constexpr int kPhases = 64;

    double m_input_rate;
    double m_output_rate = 0.0;
    int m_half = 1;                 // kernel half-width in input samples
    std::vector<float> m_kernel;    // kPhases+1 rows of 2*m_half taps
    std::vector<float> m_ring;
    int64_t m_count = 0;
};

} // namespace psg
