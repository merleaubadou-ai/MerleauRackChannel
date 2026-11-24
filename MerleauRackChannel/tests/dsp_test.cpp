// Test rapide du moteur DSP (compile sans JUCE) : g++ -O2 -std=c++17 tests/dsp_test.cpp -o dsp_test
#include "../Source/DSP/MerleauDSP.h"
#include <cstdio>
#include <vector>
#include <random>

using namespace mrdsp;
static const double SR = 48000;
static const int N = 48000;

static void sine (std::vector<float>& L, std::vector<float>& R, float f, float amp)
{
    for (int i = 0; i < N; ++i) L[i] = R[i] = amp * (float) std::sin (2 * kPi * f * i / SR);
}
static float rmsDb (const std::vector<float>& x, int from)
{
    double s = 0; for (int i = from; i < N; ++i) s += x[i] * x[i];
    return gainToDb ((float) std::sqrt (s / (N - from)));
}
static bool finite (const std::vector<float>& x) { for (float v : x) if (! std::isfinite (v)) return false; return true; }

template <typename M, typename P>
static float run (M& m, const P& p, std::vector<float>& L, std::vector<float>& R)
{
    m.prepare (SR);
    float* ch[2];
    for (int pos = 0; pos < N; pos += 512)
    {
        ch[0] = L.data() + pos; ch[1] = R.data() + pos;
        m.process (ch, 2, std::min (512, N - pos), p);
    }
    return rmsDb (L, N / 2);
}

int main()
{
    std::vector<float> L (N), R (N);
    int fails = 0;
    auto check = [&] (const char* name, bool ok, float v) { std::printf ("%-48s %8.2f dB  %s\n", name, v, ok ? "OK" : "ECHEC"); if (! ok) ++fails; };

    float ref; sine (L, R, 1000, 0.5f); ref = rmsDb (L, N / 2);

    { Preamp m; Preamp::Params p; sine (L, R, 1000, 0.5f); float v = run (m, p, L, R); check ("Preamp neutre ~ transparent", std::abs (v - ref) < 0.2f && finite (L), v - ref); }
    { Preamp m; Preamp::Params p; p.gainDb = 6; sine (L, R, 1000, 0.25f); float v = run (m, p, L, R); check ("Preamp +6 dB", std::abs (v - (ref)) < 0.3f, v - ref); }
    { Preamp m; Preamp::Params p; p.drive = 1; p.gainDb = 12; sine (L, R, 100, 0.5f); float v = run (m, p, L, R); check ("Preamp drive max reste fini", finite (L), v); }
    { Preamp m; Preamp::Params p; p.hpfHz = 200; sine (L, R, 50, 0.5f); float v = run (m, p, L, R); check ("Preamp HPF 200 Hz coupe 50 Hz", v - ref < -15, v - ref); }

    { Gate m; Gate::Params p; p.threshDb = -20; p.rangeDb = 40; sine (L, R, 1000, 0.01f); float in = rmsDb (L, N/2); float v = run (m, p, L, R); check ("Gate ferme sous le seuil (-40 dB)", v - in < -35, v - in); }
    { Gate m; Gate::Params p; p.threshDb = -20; sine (L, R, 1000, 0.5f); float v = run (m, p, L, R); check ("Gate ouvert au-dessus du seuil", std::abs (v - ref) < 0.1f, v - ref); }

    { Equalizer m; Equalizer::Params p; sine (L, R, 1000, 0.5f); float v = run (m, p, L, R); check ("EQ a plat transparent", std::abs (v - ref) < 0.05f, v - ref); }
    { Equalizer m; Equalizer::Params p; p.lmfFreq = 1000; p.lmfGain = 6; sine (L, R, 1000, 0.5f); float v = run (m, p, L, R); check ("EQ LMF +6 dB a 1 kHz", std::abs (v - ref - 6) < 0.3f, v - ref); }
    { Equalizer m; Equalizer::Params p; p.hfGain = -10; p.hfFreq = 2000; sine (L, R, 15000, 0.5f); float v = run (m, p, L, R); check ("EQ HF shelf -10 dB", std::abs (v - ref + 10) < 0.6f, v - ref); }
    { Equalizer m; Equalizer::Params p; p.lpfHz = 3000; sine (L, R, 15000, 0.5f); float v = run (m, p, L, R); check ("EQ LPF 3 kHz coupe 15 kHz", v - ref < -20, v - ref); }

    { Compressor m; Compressor::Params p; p.threshDb = -20; p.ratio = 4; p.kneeDb = 0; sine (L, R, 1000, 0.5f);
      float v = run (m, p, L, R); check ("Comp -20/4:1 reduit (crete -6 dBFS => ~-10.5)", v - ref < -8 && v - ref > -13, v - ref); }
    { Compressor m; Compressor::Params p; p.threshDb = -20; p.ratio = 4; p.mix = 0; sine (L, R, 1000, 0.5f); float v = run (m, p, L, R); check ("Comp mix 0% = sec", std::abs (v - ref) < 0.01f, v - ref); }

    { DeEsser m; DeEsser::Params p; p.threshDb = -40; p.rangeDb = 12; p.freqHz = 6500; sine (L, R, 7000, 0.5f); float v = run (m, p, L, R); check ("De-esser reduit 7 kHz", v - ref < -8, v - ref); }
    { DeEsser m; DeEsser::Params p; p.threshDb = -40; p.rangeDb = 12; p.freqHz = 6500; sine (L, R, 200, 0.5f); float v = run (m, p, L, R); check ("De-esser laisse passer 200 Hz", std::abs (v - ref) < 0.5f, v - ref); }
    { DeEsser m; DeEsser::Params p; sine (L, R, 7000, 0.5f); float v = run (m, p, L, R); check ("De-esser seuil 0 dB transparent", std::abs (v - ref) < 0.05f, v - ref); }

    // bruit fort a travers toute la chaine : pas de NaN
    { std::mt19937 rng (1); std::uniform_real_distribution<float> u (-1, 1); for (int i = 0; i < N; ++i) { L[i] = u (rng); R[i] = u (rng); }
      Preamp a; Gate b; Equalizer c; Compressor e; DeEsser f;
      Preamp::Params pa; pa.drive = 1; pa.gainDb = 30; Gate::Params pb; pb.threshDb = -10; Equalizer::Params pc; pc.lfGain = 15; pc.hfGain = 15; pc.lpfHz = 5000;
      Compressor::Params pe; pe.threshDb = -60; pe.ratio = 20; pe.makeupDb = 24; DeEsser::Params pf; pf.threshDb = -60;
      run (a, pa, L, R); run (b, pb, L, R); run (c, pc, L, R); run (e, pe, L, R); float v = run (f, pf, L, R);
      check ("Chaine extreme : aucune valeur NaN/inf", finite (L) && finite (R), v); }

    std::printf ("\n%s (%d echec(s))\n", fails ? "ECHECS" : "TOUS LES TESTS PASSENT", fails);
    return fails;
}
