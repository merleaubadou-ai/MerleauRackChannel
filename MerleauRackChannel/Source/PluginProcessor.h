#pragma once

#include <juce_audio_processors/juce_audio_processors.h>
#include <array>
#include <atomic>
#include "DSP/MerleauDSP.h"

namespace ModuleId
{
    enum : int { Preamp = 0, Gate, EQ, Comp, DeEsser, Count };
}

class MerleauRackProcessor : public juce::AudioProcessor
{
public:
    using Order = std::array<int, ModuleId::Count>;

    MerleauRackProcessor();
    ~MerleauRackProcessor() override = default;

    void prepareToPlay (double sampleRate, int samplesPerBlock) override;
    void releaseResources() override {}
    bool isBusesLayoutSupported (const BusesLayout&) const override;
    void processBlock (juce::AudioBuffer<float>&, juce::MidiBuffer&) override;
    using AudioProcessor::processBlock;

    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override { return true; }

    const juce::String getName() const override { return JucePlugin_Name; }
    bool acceptsMidi() const override  { return false; }
    bool producesMidi() const override { return false; }
    bool isMidiEffect() const override { return false; }
    double getTailLengthSeconds() const override { return 0.0; }

    int getNumPrograms() override { return 1; }
    int getCurrentProgram() override { return 0; }
    void setCurrentProgram (int) override {}
    const juce::String getProgramName (int) override { return {}; }
    void changeProgramName (int, const juce::String&) override {}

    void getStateInformation (juce::MemoryBlock&) override;
    void setStateInformation (const void*, int) override;

    static juce::AudioProcessorValueTreeState::ParameterLayout createLayout();
    juce::AudioProcessorValueTreeState apvts;

    // ---- ordre des modules (slot -> module) ------------------------------
    Order getOrder() const { return unpack (packedOrder.load()); }
    void setOrder (const Order&);
    void moveModule (int fromSlot, int toSlot);
    int slotOf (int moduleId) const;
    static Order defaultOrder() { return { 0, 1, 2, 3, 4 }; }

    // ---- vu-metres (ecrits par l'audio, lus/remis a zero par l'UI) ------
    std::atomic<float> inPeak { 0.0f }, outPeak { 0.0f }, compGrDb { 0.0f }, deessGrDb { 0.0f }, gateGain { 1.0f };

private:
    static uint32_t pack (const Order&);
    static Order unpack (uint32_t);
    static bool isValid (const Order&);
    static juce::String orderToString (const Order&);
    static Order orderFromString (const juce::String&);

    std::atomic<uint32_t> packedOrder;

    mrdsp::Preamp     preamp;
    mrdsp::Gate       gate;
    mrdsp::Equalizer  eq;
    mrdsp::Compressor comp;
    mrdsp::DeEsser    deesser;

    juce::LinearSmoothedValue<float> inGain, outGain;

    struct P
    {
        std::atomic<float> *inGain, *outGain,
            *preGain, *preDrive, *preHpf, *prePhase, *preTrim, *preOn,
            *gateThresh, *gateRange, *gateAttack, *gateHold, *gateRelease, *gateOn,
            *lfGain, *lfFreq, *lmfGain, *lmfFreq, *lmfQ, *hmfGain, *hmfFreq, *hmfQ, *hfGain, *hfFreq, *lpf, *eqOn,
            *compThresh, *compRatio, *compAttack, *compRelease, *compKnee, *compMakeup, *compMix, *compOn,
            *desFreq, *desThresh, *desRange, *desListen, *desOn;
    } p {};

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (MerleauRackProcessor)
};
