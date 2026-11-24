// MerleauRack Channel - moteur DSP (C++ pur, sans dependance JUCE)
// Chaque module traite jusqu'a 2 canaux "in place".
#pragma once

#include <algorithm>
#include <array>
#include <cmath>

namespace mrdsp
{
constexpr int kMaxCh = 2;
constexpr double kPi = 3.14159265358979323846;

inline float dbToGain (float db)   { return std::pow (10.0f, db * 0.05f); }
inline float gainToDb (float g)    { return 20.0f * std::log10 (std::max (g, 1.0e-9f)); }
inline float msToCoef (float ms, double sr)
{
    return (float) (1.0 - std::exp (-1.0 / (std::max (ms, 0.01f) * 0.001 * sr)));
}

//==============================================================================
// Biquad RBJ, forme directe II transposee
struct Biquad
{
    double b0 = 1, b1 = 0, b2 = 0, a1 = 0, a2 = 0;
    double z1[kMaxCh] {}, z2[kMaxCh] {};

    void reset() { for (int c = 0; c < kMaxCh; ++c) z1[c] = z2[c] = 0.0; }

    inline float process (float x, int c)
    {
        const double y = b0 * x + z1[c];
        z1[c] = b1 * x - a1 * y + z2[c];
        z2[c] = b2 * x - a2 * y;
        return (float) y;
    }

    static double clampF (double sr, double f) { return std::clamp (f, 10.0, sr * 0.49); }

    void set (double B0, double B1, double B2, double A0, double A1, double A2)
    {
        b0 = B0 / A0; b1 = B1 / A0; b2 = B2 / A0; a1 = A1 / A0; a2 = A2 / A0;
    }

    void lowPass (double sr, double f, double q)
    {
        const double w = 2 * kPi * clampF (sr, f) / sr, cw = std::cos (w), al = std::sin (w) / (2 * q);
        set ((1 - cw) / 2, 1 - cw, (1 - cw) / 2, 1 + al, -2 * cw, 1 - al);
    }
    void highPass (double sr, double f, double q)
    {
        const double w = 2 * kPi * clampF (sr, f) / sr, cw = std::cos (w), al = std::sin (w) / (2 * q);
        set ((1 + cw) / 2, -(1 + cw), (1 + cw) / 2, 1 + al, -2 * cw, 1 - al);
    }
    void bandPass (double sr, double f, double q)
    {
        const double w = 2 * kPi * clampF (sr, f) / sr, cw = std::cos (w), al = std::sin (w) / (2 * q);
        set (al, 0, -al, 1 + al, -2 * cw, 1 - al);
    }
    void peak (double sr, double f, double q, double gainDb)
    {
        const double A = std::pow (10.0, gainDb / 40.0);
        const double w = 2 * kPi * clampF (sr, f) / sr, cw = std::cos (w), al = std::sin (w) / (2 * q);
        set (1 + al * A, -2 * cw, 1 - al * A, 1 + al / A, -2 * cw, 1 - al / A);
    }
    void lowShelf (double sr, double f, double gainDb)
    {
        const double A = std::pow (10.0, gainDb / 40.0), sA = std::sqrt (A);
        const double w = 2 * kPi * clampF (sr, f) / sr, cw = std::cos (w), al = std::sin (w) / 2 * std::sqrt (2.0);
        set (A * ((A + 1) - (A - 1) * cw + 2 * sA * al),
             2 * A * ((A - 1) - (A + 1) * cw),
             A * ((A + 1) - (A - 1) * cw - 2 * sA * al),
             (A + 1) + (A - 1) * cw + 2 * sA * al,
             -2 * ((A - 1) + (A + 1) * cw),
             (A + 1) + (A - 1) * cw - 2 * sA * al);
    }
    void highShelf (double sr, double f, double gainDb)
    {
        const double A = std::pow (10.0, gainDb / 40.0), sA = std::sqrt (A);
        const double w = 2 * kPi * clampF (sr, f) / sr, cw = std::cos (w), al = std::sin (w) / 2 * std::sqrt (2.0);
        set (A * ((A + 1) + (A - 1) * cw + 2 * sA * al),
             -2 * A * ((A - 1) + (A + 1) * cw),
             A * ((A + 1) + (A - 1) * cw - 2 * sA * al),
             (A + 1) - (A - 1) * cw + 2 * sA * al,
             2 * ((A - 1) - (A + 1) * cw),
             (A + 1) - (A - 1) * cw - 2 * sA * al);
    }
};

//==============================================================================
// PREAMP : gain, saturation analogique asymetrique, filtre passe-haut, phase, trim
class Preamp
{
public:
    struct Params { float gainDb = 0, drive = 0, hpfHz = 20, trimDb = 0; bool phase = false, on = true; };

    void prepare (double sr) { fs = sr; dc.highPass (fs, 8.0, 0.707); lastHpf = -1; reset(); }
    void reset() { hpf.reset(); dc.reset(); }

    void process (float* const* d, int nc, int n, const Params& p)
    {
        if (! p.on || n <= 0) return;

        if (p.hpfHz != lastHpf) { hpf.highPass (fs, p.hpfHz, 0.707); lastHpf = p.hpfHz; }
        const bool hpOn = p.hpfHz > 20.5f;

        const float drive = std::clamp (p.drive, 0.0f, 1.0f);
        const float k     = 1.0f + 3.0f * drive;
        const float bias  = 0.15f * drive;
        const float tb    = std::tanh (bias);
        const float norm  = 1.0f / (k * (1.0f - tb * tb));
        const float pol   = p.phase ? -1.0f : 1.0f;

        const float tgtIn = dbToGain (p.gainDb), tgtOut = dbToGain (p.trimDb);
        const float stIn  = (tgtIn - gIn) / (float) n, stOut = (tgtOut - gOut) / (float) n;

        for (int s = 0; s < n; ++s)
        {
            gIn += stIn; gOut += stOut;
            for (int c = 0; c < nc; ++c)
            {
                float x = d[c][s] * gIn * pol;
                if (hpOn) x = hpf.process (x, c);
                if (drive > 0.0f)
                {
                    const float sat = (std::tanh (k * x + bias) - tb) * norm;
                    x += drive * (sat - x);
                }
                x = dc.process (x, c);
                d[c][s] = x * gOut;
            }
        }
        gIn = tgtIn; gOut = tgtOut;
    }

private:
    double fs = 44100;
    Biquad hpf, dc;
    float lastHpf = -1, gIn = 1, gOut = 1;
};

//==============================================================================
// GATE / EXPANDER : seuil avec hysteresis, range, attack, hold, release
class Gate
{
public:
    struct Params { float threshDb = -80, rangeDb = 40, attackMs = 1, holdMs = 50, releaseMs = 150; bool on = true; };

    void prepare (double sr) { fs = sr; reset(); }
    void reset() { gain = 1; gainDb = 0; env = 0; holdCount = 0; open = true; }
    float getGain() const { return gain; }

    void process (float* const* d, int nc, int n, const Params& p)
    {
        if (! p.on) { gain = 1; gainDb = 0; open = true; return; }

        const float floorDb  = -p.rangeDb;
        const float thrOpen  = dbToGain (p.threshDb);
        const float thrClose = dbToGain (p.threshDb - 4.0f);
        const float aC = msToCoef (p.attackMs, fs), rC = msToCoef (p.releaseMs, fs);
        const float detRel = msToCoef (5.0f, fs);
        const int holdS = (int) (p.holdMs * 0.001 * fs);

        for (int s = 0; s < n; ++s)
        {
            float pk = 0;
            for (int c = 0; c < nc; ++c) pk = std::max (pk, std::abs (d[c][s]));
            env = pk > env ? pk : env + (pk - env) * detRel;

            if (env > (open ? thrClose : thrOpen)) { open = true; holdCount = holdS; }
            else if (holdCount > 0)                --holdCount;
            else                                    open = false;

            // lissage dans le domaine dB : fermeture naturelle jusqu'au plancher
            const float targetDb = open ? 0.0f : floorDb;
            gainDb += (targetDb - gainDb) * (targetDb > gainDb ? aC : rC);
            gain = dbToGain (gainDb);
            for (int c = 0; c < nc; ++c) d[c][s] *= gain;
        }
    }

private:
    double fs = 44100;
    float gain = 1, gainDb = 0, env = 0;
    int holdCount = 0;
    bool open = true;
};

//==============================================================================
// EQ 4 bandes (LF shelf, LMF cloche, HMF cloche, HF shelf) + passe-bas
class Equalizer
{
public:
    struct Params
    {
        float lfGain = 0, lfFreq = 100, lmfGain = 0, lmfFreq = 600, lmfQ = 1,
              hmfGain = 0, hmfFreq = 3000, hmfQ = 1, hfGain = 0, hfFreq = 8000, lpfHz = 22000;
        bool on = true;
    };

    void prepare (double sr) { fs = sr; dirty = true; reset(); }
    void reset() { for (auto& b : bands) b.reset(); lpf.reset(); }

    void process (float* const* d, int nc, int n, const Params& p)
    {
        if (! p.on) return;
        update (p);
        const bool lpOn = p.lpfHz < 20000.0f;
        if (! lpOn) lpf.reset();

        for (int c = 0; c < nc; ++c)
        {
            float* x = d[c];
            for (int s = 0; s < n; ++s)
            {
                float v = x[s];
                for (auto& b : bands) v = b.process (v, c);
                if (lpOn) v = lpf.process (v, c);
                x[s] = v;
            }
        }
    }

private:
    void update (const Params& p)
    {
        const float key[] = { p.lfGain, p.lfFreq, p.lmfGain, p.lmfFreq, p.lmfQ,
                              p.hmfGain, p.hmfFreq, p.hmfQ, p.hfGain, p.hfFreq, p.lpfHz };
        bool changed = dirty;
        for (int i = 0; i < 11; ++i) if (key[i] != last[i]) { changed = true; last[i] = key[i]; }
        if (! changed) return;
        dirty = false;
        bands[0].lowShelf  (fs, p.lfFreq, p.lfGain);
        bands[1].peak      (fs, p.lmfFreq, p.lmfQ, p.lmfGain);
        bands[2].peak      (fs, p.hmfFreq, p.hmfQ, p.hmfGain);
        bands[3].highShelf (fs, p.hfFreq, p.hfGain);
        lpf.lowPass (fs, p.lpfHz, 0.707);
    }

    double fs = 44100;
    std::array<Biquad, 4> bands;
    Biquad lpf;
    float last[11] {};
    bool dirty = true;
};

//==============================================================================
// COMPRESSEUR feed-forward, domaine log, knee doux, mix parallele
class Compressor
{
public:
    struct Params { float threshDb = 0, ratio = 4, attackMs = 10, releaseMs = 150, kneeDb = 6, makeupDb = 0, mix = 1; bool on = true; };

    void prepare (double sr) { fs = sr; reset(); }
    void reset() { grDb = 0; meterGr = 0; }
    float getGrDb() const { return meterGr; }   // reduction max du dernier bloc (positif)

    static float computeGr (float levelDb, float thr, float ratio, float knee)
    {
        const float over = levelDb - thr;
        const float slope = 1.0f / std::max (ratio, 1.0f) - 1.0f; // negatif
        if (knee > 0.0f && 2.0f * std::abs (over) <= knee)
            return -slope * (over + knee * 0.5f) * (over + knee * 0.5f) / (2.0f * knee);
        if (over <= 0.0f) return 0.0f;
        return -slope * over;
    }

    void process (float* const* d, int nc, int n, const Params& p)
    {
        meterGr = 0;
        if (! p.on) { grDb = 0; return; }

        const float aC = msToCoef (p.attackMs, fs), rC = msToCoef (p.releaseMs, fs);
        const float mix = std::clamp (p.mix, 0.0f, 1.0f);

        for (int s = 0; s < n; ++s)
        {
            float pk = 0;
            for (int c = 0; c < nc; ++c) pk = std::max (pk, std::abs (d[c][s]));

            const float target = computeGr (gainToDb (pk), p.threshDb, p.ratio, p.kneeDb);
            grDb += (target - grDb) * (target > grDb ? aC : rC);
            meterGr = std::max (meterGr, grDb);

            const float g = dbToGain (p.makeupDb - grDb);
            for (int c = 0; c < nc; ++c)
            {
                const float x = d[c][s];
                d[c][s] = x + mix * (x * g - x);
            }
        }
    }

private:
    double fs = 44100;
    float grDb = 0, meterGr = 0;
};

//==============================================================================
// DE-ESSER split-band : detection passe-bande, reduction de la bande aigue uniquement
class DeEsser
{
public:
    struct Params { float freqHz = 6500, threshDb = 0, rangeDb = 10; bool listen = false, on = true; };

    void prepare (double sr) { fs = sr; lastF = -1; reset(); }
    void reset() { det.reset(); split.reset(); env = 0; meterGr = 0; }
    float getGrDb() const { return meterGr; }

    void process (float* const* d, int nc, int n, const Params& p)
    {
        meterGr = 0;
        if (! p.on) { env = 0; return; }

        if (p.freqHz != lastF)
        {
            det.bandPass (fs, p.freqHz, 1.4);
            split.bandPass (fs, p.freqHz, 0.8);   // x - k*BP = cloche de coupe exacte en phase
            lastF = p.freqHz;
        }

        const float att = msToCoef (0.3f, fs), rel = msToCoef (60.0f, fs);

        for (int s = 0; s < n; ++s)
        {
            float bp[kMaxCh] {};
            float pk = 0;
            for (int c = 0; c < nc; ++c)
            {
                bp[c] = det.process (d[c][s], c);
                pk = std::max (pk, std::abs (bp[c]));
            }
            env += (pk - env) * (pk > env ? att : rel);

            const float over = gainToDb (env) - p.threshDb;
            const float gr = std::clamp (over * 0.75f, 0.0f, p.rangeDb);
            meterGr = std::max (meterGr, gr);
            const float g = dbToGain (-gr);

            for (int c = 0; c < nc; ++c)
            {
                const float x = d[c][s];
                const float h = split.process (x, c);
                d[c][s] = p.listen ? bp[c] : x - h * (1.0f - g);
            }
        }
    }

private:
    double fs = 44100;
    Biquad det, split;
    float lastF = -1, env = 0, meterGr = 0;
};

} // namespace mrdsp
