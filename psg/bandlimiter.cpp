// bandlimiter.cpp -- see bandlimiter.h.

#include "bandlimiter.h"
#include <algorithm>
#include <cmath>

namespace psg {

namespace {
constexpr double kPi = 3.14159265358979323846;
}

BandLimiter::BandLimiter(double input_rate)
    : m_input_rate(input_rate)
    , m_ring(2 * kRingSize, 0.0f)
{
    SetOutputRate(44100.0);
}

void BandLimiter::Reset()
{
    std::fill(m_ring.begin(), m_ring.end(), 0.0f);
    m_count = 0;
}

void BandLimiter::SetOutputRate(double output_rate)
{
    output_rate = std::clamp(output_rate, 8000.0, 192000.0);
    m_output_rate = output_rate;

    // Cutoff at 0.43 x the output rate; the Blackman transition band
    // (~5.5/N cycles per input sample) is sized to fit between the cutoff
    // and 0.5 x the output rate, so everything that would alias is in the
    // stopband (about -74dB).
    const double fc = 0.43 * output_rate / m_input_rate;          // cycles/sample
    const double transition = 0.07 * output_rate / m_input_rate;
    m_half = std::clamp(int(std::ceil(5.5 / transition / 2.0)), 4, int(kRingSize / 2 - 64));

    const int taps = 2 * m_half;
    m_kernel.assign(size_t(kPhases + 1) * taps, 0.0f);
    for (int p = 0; p <= kPhases; p++) {
        const double frac = double(p) / kPhases;
        double sum = 0.0;
        std::vector<double> row(taps);
        for (int k = 0; k < taps; k++) {
            // tap k is input sample base - m_half + 1 + k; distance from t
            const double d = frac + double(m_half - 1 - k);
            const double u = d / m_half;
            double w = 0.0;
            if (std::fabs(u) < 1.0)
                w = 0.42 + 0.5 * std::cos(kPi * u) + 0.08 * std::cos(2.0 * kPi * u);
            const double x = 2.0 * fc * d;
            const double sinc = (std::fabs(x) < 1e-12) ? 1.0 : std::sin(kPi * x) / (kPi * x);
            row[k] = 2.0 * fc * sinc * w;
            sum += row[k];
        }
        for (int k = 0; k < taps; k++)
            m_kernel[size_t(p) * taps + k] = float(row[k] / sum);   // unity DC gain
    }
}

double BandLimiter::At(double t) const
{
    const double fl = std::floor(t);
    const int64_t base = int64_t(fl);
    const int p = int((t - fl) * kPhases + 0.5);
    const int taps = 2 * m_half;
    const float* h = &m_kernel[size_t(p) * taps];
    const float* x = &m_ring[size_t(base - m_half + 1) & kMask];
    // Four partial sums so the compiler can vectorise the dot product.
    float a0 = 0, a1 = 0, a2 = 0, a3 = 0;
    int k = 0;
    for (; k + 4 <= taps; k += 4) {
        a0 += h[k] * x[k];
        a1 += h[k + 1] * x[k + 1];
        a2 += h[k + 2] * x[k + 2];
        a3 += h[k + 3] * x[k + 3];
    }
    for (; k < taps; k++)
        a0 += h[k] * x[k];
    return double((a0 + a1) + (a2 + a3));
}

} // namespace psg
