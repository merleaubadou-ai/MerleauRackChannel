#include "PluginProcessor.h"
#include "PluginEditor.h"

namespace
{
    using Attr = juce::AudioParameterFloatAttributes;

    juce::NormalisableRange<float> range (float lo, float hi, float step, float centre = -1.0f)
    {
        juce::NormalisableRange<float> r (lo, hi, step);
        if (centre > lo && centre < hi) r.setSkewForCentre (centre);
        return r;
    }

    juce::String hz (float v)
    {
        return v >= 1000.0f ? juce::String (v / 1000.0f, v >= 10000.0f ? 1 : 2) + " kHz"
                            : juce::String (juce::roundToInt (v)) + " Hz";
    }

    std::unique_ptr<juce::AudioParameterFloat> fparam (const char* id, const char* name, juce::NormalisableRange<float> r,
                                                       float def, Attr attr)
    {
        return std::make_unique<juce::AudioParameterFloat> (juce::ParameterID { id, 1 }, name, r, def, attr);
    }

    Attr dB()  { return Attr().withLabel ("dB").withStringFromValueFunction ([] (float v, int) { return (v > 0 ? "+" : "") + juce::String (v, 1) + " dB"; }); }
    Attr ms()  { return Attr().withLabel ("ms").withStringFromValueFunction ([] (float v, int) { return v < 10 ? juce::String (v, 1) + " ms" : juce::String (juce::roundToInt (v)) + " ms"; }); }
    Attr freq() { return Attr().withLabel ("Hz").withStringFromValueFunction ([] (float v, int) { return hz (v); }); }
    Attr pct() { return Attr().withLabel ("%").withStringFromValueFunction ([] (float v, int) { return juce::String (juce::roundToInt (v)) + " %"; }); }

    std::unique_ptr<juce::AudioParameterBool> bparam (const char* id, const char* name, bool def)
    {
        return std::make_unique<juce::AudioParameterBool> (juce::ParameterID { id, 1 }, name, def);
    }
}

//==============================================================================
juce::AudioProcessorValueTreeState::ParameterLayout MerleauRackProcessor::createLayout()
{
    juce::AudioProcessorValueTreeState::ParameterLayout l;

    // MASTER
    l.add (fparam ("in_gain",  "Master Input",  range (-24, 24, 0.1f), 0, dB()));
    l.add (fparam ("out_gain", "Master Output", range (-24, 24, 0.1f), 0, dB()));

    // PREAMP
    l.add (fparam ("pre_gain",  "Preamp Gain",  range (-20, 30, 0.1f), 0, dB()));
    l.add (fparam ("pre_drive", "Preamp Drive", range (0, 100, 0.1f), 0, pct()));
    l.add (fparam ("pre_hpf",   "Preamp HPF",   range (20, 400, 1, 90),  20,
                   Attr().withLabel ("Hz").withStringFromValueFunction ([] (float v, int) { return v <= 20.5f ? juce::String ("OFF") : hz (v); })));
    l.add (fparam ("pre_trim",  "Preamp Trim",  range (-20, 20, 0.1f), 0, dB()));
    l.add (bparam ("pre_phase", "Preamp Phase", false));
    l.add (bparam ("pre_on",    "Preamp In",    true));

    // GATE
    l.add (fparam ("gate_thresh",  "Gate Threshold", range (-80, 0, 0.1f), -80, dB()));
    l.add (fparam ("gate_range",   "Gate Range",     range (0, 80, 0.1f), 40, dB()));
    l.add (fparam ("gate_attack",  "Gate Attack",    range (0.1f, 50, 0.01f, 3), 1, ms()));
    l.add (fparam ("gate_hold",    "Gate Hold",      range (0, 500, 1, 80), 50, ms()));
    l.add (fparam ("gate_release", "Gate Release",   range (5, 2000, 1, 200), 150, ms()));
    l.add (bparam ("gate_on",      "Gate In",        true));

    // EQ
    l.add (fparam ("eq_hf_gain",  "EQ HF Gain",  range (-15, 15, 0.1f), 0, dB()));
    l.add (fparam ("eq_hf_freq",  "EQ HF Freq",  range (1500, 16000, 1, 6000), 8000, freq()));
    l.add (fparam ("eq_hmf_gain", "EQ HMF Gain", range (-15, 15, 0.1f), 0, dB()));
    l.add (fparam ("eq_hmf_freq", "EQ HMF Freq", range (600, 8000, 1, 2500), 3000, freq()));
    l.add (fparam ("eq_hmf_q",    "EQ HMF Q",    range (0.3f, 4, 0.01f, 1), 1, Attr().withStringFromValueFunction ([] (float v, int) { return "Q " + juce::String (v, 2); })));
    l.add (fparam ("eq_lmf_gain", "EQ LMF Gain", range (-15, 15, 0.1f), 0, dB()));
    l.add (fparam ("eq_lmf_freq", "EQ LMF Freq", range (200, 2500, 1, 700), 600, freq()));
    l.add (fparam ("eq_lmf_q",    "EQ LMF Q",    range (0.3f, 4, 0.01f, 1), 1, Attr().withStringFromValueFunction ([] (float v, int) { return "Q " + juce::String (v, 2); })));
    l.add (fparam ("eq_lf_gain",  "EQ LF Gain",  range (-15, 15, 0.1f), 0, dB()));
    l.add (fparam ("eq_lf_freq",  "EQ LF Freq",  range (30, 450, 1, 120), 100, freq()));
    l.add (fparam ("eq_lpf",      "EQ LPF",      range (3000, 22000, 1, 9000), 22000,
                   Attr().withLabel ("Hz").withStringFromValueFunction ([] (float v, int) { return v >= 20000.0f ? juce::String ("OFF") : hz (v); })));
    l.add (bparam ("eq_on",       "EQ In",       true));

    // COMPRESSEUR
    l.add (fparam ("comp_thresh",  "Comp Threshold", range (-60, 0, 0.1f), 0, dB()));
    l.add (fparam ("comp_ratio",   "Comp Ratio",     range (1, 20, 0.01f, 4), 4,
                   Attr().withStringFromValueFunction ([] (float v, int) { return juce::String (v, v < 10 ? 1 : 0) + ":1"; })));
    l.add (fparam ("comp_attack",  "Comp Attack",    range (0.1f, 100, 0.01f, 10), 10, ms()));
    l.add (fparam ("comp_release", "Comp Release",   range (10, 2000, 1, 200), 150, ms()));
    l.add (fparam ("comp_knee",    "Comp Knee",      range (0, 12, 0.1f), 6, dB()));
    l.add (fparam ("comp_makeup",  "Comp Makeup",    range (0, 24, 0.1f), 0, dB()));
    l.add (fparam ("comp_mix",     "Comp Mix",       range (0, 100, 0.1f), 100, pct()));
    l.add (bparam ("comp_on",      "Comp In",        true));

    // DE-ESSER
    l.add (fparam ("des_freq",   "DeEsser Freq",      range (2000, 12000, 1, 6000), 6500, freq()));
    l.add (fparam ("des_thresh", "DeEsser Threshold", range (-60, 0, 0.1f), 0, dB()));
    l.add (fparam ("des_range",  "DeEsser Range",     range (0, 24, 0.1f), 10, dB()));
    l.add (bparam ("des_listen", "DeEsser Listen",    false));
    l.add (bparam ("des_on",     "DeEsser In",        true));

    return l;
}

//==============================================================================
MerleauRackProcessor::MerleauRackProcessor()
    : AudioProcessor (BusesProperties()
                          .withInput  ("Input",  juce::AudioChannelSet::stereo(), true)
                          .withOutput ("Output", juce::AudioChannelSet::stereo(), true)),
      apvts (*this, nullptr, "MerleauRack", createLayout()),
      packedOrder (pack (defaultOrder()))
{
    auto g = [this] (const char* id) { auto* v = apvts.getRawParameterValue (id); jassert (v != nullptr); return v; };

    p.inGain = g ("in_gain");       p.outGain = g ("out_gain");
    p.preGain = g ("pre_gain");     p.preDrive = g ("pre_drive");   p.preHpf = g ("pre_hpf");
    p.prePhase = g ("pre_phase");   p.preTrim = g ("pre_trim");     p.preOn = g ("pre_on");
    p.gateThresh = g ("gate_thresh"); p.gateRange = g ("gate_range"); p.gateAttack = g ("gate_attack");
    p.gateHold = g ("gate_hold");   p.gateRelease = g ("gate_release"); p.gateOn = g ("gate_on");
    p.lfGain = g ("eq_lf_gain");    p.lfFreq = g ("eq_lf_freq");
    p.lmfGain = g ("eq_lmf_gain");  p.lmfFreq = g ("eq_lmf_freq");  p.lmfQ = g ("eq_lmf_q");
    p.hmfGain = g ("eq_hmf_gain");  p.hmfFreq = g ("eq_hmf_freq");  p.hmfQ = g ("eq_hmf_q");
    p.hfGain = g ("eq_hf_gain");    p.hfFreq = g ("eq_hf_freq");    p.lpf = g ("eq_lpf");  p.eqOn = g ("eq_on");
    p.compThresh = g ("comp_thresh"); p.compRatio = g ("comp_ratio"); p.compAttack = g ("comp_attack");
    p.compRelease = g ("comp_release"); p.compKnee = g ("comp_knee"); p.compMakeup = g ("comp_makeup");
    p.compMix = g ("comp_mix");     p.compOn = g ("comp_on");
    p.desFreq = g ("des_freq");     p.desThresh = g ("des_thresh"); p.desRange = g ("des_range");
    p.desListen = g ("des_listen"); p.desOn = g ("des_on");

    apvts.state.setProperty ("order", orderToString (defaultOrder()), nullptr);
}

bool MerleauRackProcessor::isBusesLayoutSupported (const BusesLayout& layouts) const
{
    const auto out = layouts.getMainOutputChannelSet();
    if (out != juce::AudioChannelSet::mono() && out != juce::AudioChannelSet::stereo())
        return false;
    return layouts.getMainInputChannelSet() == out;
}

void MerleauRackProcessor::prepareToPlay (double sr, int)
{
    preamp.prepare (sr);
    gate.prepare (sr);
    eq.prepare (sr);
    comp.prepare (sr);
    deesser.prepare (sr);
    inGain.reset (sr, 0.03);
    outGain.reset (sr, 0.03);
    inGain.setCurrentAndTargetValue (mrdsp::dbToGain (p.inGain->load()));
    outGain.setCurrentAndTargetValue (mrdsp::dbToGain (p.outGain->load()));
}

static void storeMax (std::atomic<float>& a, float v)
{
    if (v > a.load (std::memory_order_relaxed)) a.store (v, std::memory_order_relaxed);
}

void MerleauRackProcessor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer&)
{
    juce::ScopedNoDenormals noDenormals;

    const int numIn  = getTotalNumInputChannels();
    const int numOut = getTotalNumOutputChannels();
    const int n      = buffer.getNumSamples();
    for (int c = numIn; c < numOut; ++c) buffer.clear (c, 0, n);
    if (n == 0) return;

    const int nc = juce::jmin (mrdsp::kMaxCh, numOut, buffer.getNumChannels());
    float* const* d = buffer.getArrayOfWritePointers();

    inGain.setTargetValue (mrdsp::dbToGain (p.inGain->load()));
    inGain.applyGain (buffer, n);
    storeMax (inPeak, buffer.getMagnitude (0, n));

    const auto order = getOrder();
    for (int moduleId : order)
    {
        switch (moduleId)
        {
            case ModuleId::Preamp:
            {
                mrdsp::Preamp::Params q;
                q.gainDb = p.preGain->load(); q.drive = p.preDrive->load() * 0.01f; q.hpfHz = p.preHpf->load();
                q.trimDb = p.preTrim->load(); q.phase = p.prePhase->load() > 0.5f; q.on = p.preOn->load() > 0.5f;
                preamp.process (d, nc, n, q);
                break;
            }
            case ModuleId::Gate:
            {
                mrdsp::Gate::Params q;
                q.threshDb = p.gateThresh->load(); q.rangeDb = p.gateRange->load(); q.attackMs = p.gateAttack->load();
                q.holdMs = p.gateHold->load(); q.releaseMs = p.gateRelease->load(); q.on = p.gateOn->load() > 0.5f;
                gate.process (d, nc, n, q);
                gateGain.store (gate.getGain(), std::memory_order_relaxed);
                break;
            }
            case ModuleId::EQ:
            {
                mrdsp::Equalizer::Params q;
                q.lfGain = p.lfGain->load();   q.lfFreq = p.lfFreq->load();
                q.lmfGain = p.lmfGain->load(); q.lmfFreq = p.lmfFreq->load(); q.lmfQ = p.lmfQ->load();
                q.hmfGain = p.hmfGain->load(); q.hmfFreq = p.hmfFreq->load(); q.hmfQ = p.hmfQ->load();
                q.hfGain = p.hfGain->load();   q.hfFreq = p.hfFreq->load();   q.lpfHz = p.lpf->load();
                q.on = p.eqOn->load() > 0.5f;
                eq.process (d, nc, n, q);
                break;
            }
            case ModuleId::Comp:
            {
                mrdsp::Compressor::Params q;
                q.threshDb = p.compThresh->load(); q.ratio = p.compRatio->load(); q.attackMs = p.compAttack->load();
                q.releaseMs = p.compRelease->load(); q.kneeDb = p.compKnee->load(); q.makeupDb = p.compMakeup->load();
                q.mix = p.compMix->load() * 0.01f; q.on = p.compOn->load() > 0.5f;
                comp.process (d, nc, n, q);
                storeMax (compGrDb, comp.getGrDb());
                break;
            }
            case ModuleId::DeEsser:
            {
                mrdsp::DeEsser::Params q;
                q.freqHz = p.desFreq->load(); q.threshDb = p.desThresh->load(); q.rangeDb = p.desRange->load();
                q.listen = p.desListen->load() > 0.5f; q.on = p.desOn->load() > 0.5f;
                deesser.process (d, nc, n, q);
                storeMax (deessGrDb, deesser.getGrDb());
                break;
            }
            default: break;
        }
    }

    outGain.setTargetValue (mrdsp::dbToGain (p.outGain->load()));
    outGain.applyGain (buffer, n);
    storeMax (outPeak, buffer.getMagnitude (0, n));
}

//==============================================================================
uint32_t MerleauRackProcessor::pack (const Order& o)
{
    uint32_t v = 0;
    for (int i = 0; i < ModuleId::Count; ++i) v |= (uint32_t) (o[(size_t) i] & 7) << (3 * i);
    return v;
}

MerleauRackProcessor::Order MerleauRackProcessor::unpack (uint32_t v)
{
    Order o {};
    for (int i = 0; i < ModuleId::Count; ++i) o[(size_t) i] = (int) ((v >> (3 * i)) & 7);
    return o;
}

bool MerleauRackProcessor::isValid (const Order& o)
{
    bool seen[ModuleId::Count] {};
    for (int id : o)
    {
        if (id < 0 || id >= ModuleId::Count || seen[id]) return false;
        seen[id] = true;
    }
    return true;
}

juce::String MerleauRackProcessor::orderToString (const Order& o)
{
    juce::StringArray s;
    for (int id : o) s.add (juce::String (id));
    return s.joinIntoString (",");
}

MerleauRackProcessor::Order MerleauRackProcessor::orderFromString (const juce::String& str)
{
    auto t = juce::StringArray::fromTokens (str, ",", "");
    if (t.size() != ModuleId::Count) return defaultOrder();
    Order o {};
    for (int i = 0; i < ModuleId::Count; ++i) o[(size_t) i] = t[i].trim().getIntValue();
    return isValid (o) ? o : defaultOrder();
}

void MerleauRackProcessor::setOrder (const Order& o)
{
    if (! isValid (o)) return;
    packedOrder.store (pack (o));
    apvts.state.setProperty ("order", orderToString (o), nullptr);
}

int MerleauRackProcessor::slotOf (int moduleId) const
{
    const auto o = getOrder();
    for (int i = 0; i < ModuleId::Count; ++i) if (o[(size_t) i] == moduleId) return i;
    return -1;
}

void MerleauRackProcessor::moveModule (int fromSlot, int toSlot)
{
    if (fromSlot == toSlot || fromSlot < 0 || toSlot < 0 || fromSlot >= ModuleId::Count || toSlot >= ModuleId::Count)
        return;
    auto o = getOrder();
    const int id = o[(size_t) fromSlot];
    if (fromSlot < toSlot) for (int i = fromSlot; i < toSlot; ++i) o[(size_t) i] = o[(size_t) i + 1];
    else                   for (int i = fromSlot; i > toSlot; --i) o[(size_t) i] = o[(size_t) i - 1];
    o[(size_t) toSlot] = id;
    setOrder (o);
}

//==============================================================================
void MerleauRackProcessor::getStateInformation (juce::MemoryBlock& dest)
{
    auto state = apvts.copyState();
    state.setProperty ("order", orderToString (getOrder()), nullptr);
    if (auto xml = state.createXml())
        copyXmlToBinary (*xml, dest);
}

void MerleauRackProcessor::setStateInformation (const void* data, int size)
{
    if (auto xml = getXmlFromBinary (data, size))
    {
        if (xml->hasTagName (apvts.state.getType()))
        {
            auto state = juce::ValueTree::fromXml (*xml);
            const auto o = orderFromString (state.getProperty ("order").toString());
            apvts.replaceState (state);
            setOrder (o);
        }
    }
}

juce::AudioProcessorEditor* MerleauRackProcessor::createEditor()
{
    return new MerleauRackEditor (*this);
}

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new MerleauRackProcessor();
}
