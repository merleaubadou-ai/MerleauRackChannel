#pragma once

#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_gui_extra/juce_gui_extra.h>
#include "PluginProcessor.h"
#include "UI/RackLookAndFeel.h"
#include "UI/ModulePanels.h"

//==============================================================================
// Zone du rack : contient les 5 modules, gere le glisser-deposer
class RackArea : public juce::Component,
                 public juce::DragAndDropTarget
{
public:
    static constexpr int moduleW = 170, gap = 6;

    explicit RackArea (MerleauRackProcessor&);

    void layoutModules (bool animate);
    void updateMeters();
    bool orderChangedExternally() const { return shownOrder != proc.getOrder(); }

    void paint (juce::Graphics&) override;
    void paintOverChildren (juce::Graphics&) override;
    void resized() override { layoutModules (false); }

    bool isInterestedInDragSource (const SourceDetails&) override;
    void itemDragEnter (const SourceDetails&) override;
    void itemDragMove (const SourceDetails&) override;
    void itemDragExit (const SourceDetails&) override;
    void itemDropped (const SourceDetails&) override;

    std::function<void()> onOrderChanged;

private:
    int slotForX (int x) const;
    juce::Rectangle<int> slotBounds (int slot) const;

    MerleauRackProcessor& proc;
    std::array<std::unique_ptr<mrui::ModulePanel>, ModuleId::Count> panels;
    MerleauRackProcessor::Order shownOrder {};
    int dropSlot = -1, dragFromSlot = -1;
};

//==============================================================================
class MerleauRackEditor : public juce::AudioProcessorEditor,
                          public juce::DragAndDropContainer,
                          private juce::Timer
{
public:
    explicit MerleauRackEditor (MerleauRackProcessor&);
    ~MerleauRackEditor() override;

    void paint (juce::Graphics&) override;
    void resized() override;

private:
    void timerCallback() override;
    void syncChainMenu();

    MerleauRackProcessor& proc;
    mrui::RackLookAndFeel lnf;          // declare en premier : detruit en dernier

    RackArea rack;
    mrui::MasterPanel master;
    juce::ComboBox chainBox;

    struct ChainPreset { const char* name; MerleauRackProcessor::Order order; };
    static const std::array<ChainPreset, 5> chainPresets;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (MerleauRackEditor)
};
