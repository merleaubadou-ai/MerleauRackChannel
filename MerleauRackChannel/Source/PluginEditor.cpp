#include "PluginEditor.h"

using namespace juce;

namespace
{
    constexpr int woodW = 26, pad = 12, headerH = 66, rackH = 522, masterW = 150;
    constexpr int rackW = RackArea::moduleW * ModuleId::Count + RackArea::gap * (ModuleId::Count - 1);
    constexpr int editorW = woodW + pad + rackW + pad + masterW + pad + woodW;
    constexpr int editorH = headerH + rackH + 14;

    String utf8 (const char* s) { return String (CharPointer_UTF8 (s)); }

    void drawWood (Graphics& g, Rectangle<float> r, int seed)
    {
        ColourGradient base (Colour (0xff5a3420), r.getX(), r.getY(), Colour (0xff2e190e), r.getRight(), r.getY(), false);
        base.addColour (0.45, Colour (0xff6e4128));
        g.setGradientFill (base);
        g.fillRect (r);

        Random rnd (seed);
        for (int i = 0; i < 26; ++i)
        {
            Path grain;
            float x = r.getX() + rnd.nextFloat() * r.getWidth();
            grain.startNewSubPath (x, r.getY());
            for (float y = r.getY(); y < r.getBottom(); y += 24.0f)
            {
                x += (rnd.nextFloat() - 0.5f) * 1.6f;
                x = jlimit (r.getX(), r.getRight(), x);
                grain.lineTo (x, y + 24.0f);
            }
            g.setColour ((rnd.nextBool() ? Colour (0xff1e0f07) : Colour (0xff8a5634)).withAlpha (0.18f + rnd.nextFloat() * 0.25f));
            g.strokePath (grain, PathStrokeType (0.6f + rnd.nextFloat() * 1.2f));
        }
        ColourGradient sheen (Colours::white.withAlpha (0.10f), r.getX(), r.getY(), Colours::black.withAlpha (0.35f), r.getX(), r.getBottom(), false);
        g.setGradientFill (sheen);
        g.fillRect (r);
        g.setColour (Colours::black.withAlpha (0.6f));
        g.drawRect (r, 1.0f);
    }
}

const std::array<MerleauRackEditor::ChainPreset, 5> MerleauRackEditor::chainPresets {{
    { "Classique :  PRE > GATE > EQ > COMP > DE-ESS",  { 0, 1, 2, 3, 4 } },
    { "Voix :  PRE > GATE > COMP > EQ > DE-ESS",       { 0, 1, 3, 2, 4 } },
    { "De-ess d'abord :  PRE > DE-ESS > GATE > COMP > EQ", { 0, 4, 1, 3, 2 } },
    { "Batterie :  GATE > PRE > EQ > COMP > DE-ESS",   { 1, 0, 2, 3, 4 } },
    { "Bus / Mix :  EQ > COMP > PRE > DE-ESS > GATE",  { 2, 3, 0, 4, 1 } },
}};

//==============================================================================
RackArea::RackArea (MerleauRackProcessor& p) : proc (p)
{
    panels[ModuleId::Preamp]  = std::make_unique<mrui::PreampPanel>  (proc);
    panels[ModuleId::Gate]    = std::make_unique<mrui::GatePanel>    (proc);
    panels[ModuleId::EQ]      = std::make_unique<mrui::EqPanel>      (proc);
    panels[ModuleId::Comp]    = std::make_unique<mrui::CompPanel>    (proc);
    panels[ModuleId::DeEsser] = std::make_unique<mrui::DeEsserPanel> (proc);
    for (auto& panel : panels) addAndMakeVisible (*panel);
}

Rectangle<int> RackArea::slotBounds (int slot) const
{
    return { slot * (moduleW + gap), 0, moduleW, getHeight() };
}

int RackArea::slotForX (int x) const
{
    return jlimit (0, ModuleId::Count - 1, x / (moduleW + gap));
}

void RackArea::layoutModules (bool animate)
{
    shownOrder = proc.getOrder();
    auto& animator = Desktop::getInstance().getAnimator();

    for (int slot = 0; slot < ModuleId::Count; ++slot)
    {
        auto& panel = *panels[(size_t) shownOrder[(size_t) slot]];
        panel.setSlotNumber (slot + 1);
        const auto target = slotBounds (slot);

        if (animate && panel.getBounds() != target && ! panel.getBounds().isEmpty())
            animator.animateComponent (&panel, target, 1.0f, 240, false, 1.0, 0.0);
        else
            panel.setBounds (target);
    }
}

void RackArea::updateMeters()
{
    for (auto& panel : panels) panel->updateMeters();
}

void RackArea::paint (Graphics& g)
{
    // interieur du rack : rails perces
    g.fillAll (Colour (0xff09090a));
    for (int slot = 0; slot < ModuleId::Count; ++slot)
    {
        auto b = slotBounds (slot).toFloat().reduced (1.0f);
        g.setColour (Colour (0xff151517));
        g.fillRect (b);
        g.setColour (Colours::white.withAlpha (0.05f));
        g.drawRect (b, 1.0f);
    }
}

void RackArea::paintOverChildren (Graphics& g)
{
    if (dropSlot < 0) return;
    auto b = slotBounds (dropSlot).toFloat().reduced (2.0f);
    g.setColour (mrui::Palette::gold.withAlpha (0.14f));
    g.fillRoundedRectangle (b, 4.0f);
    g.setColour (mrui::Palette::gold);
    g.drawRoundedRectangle (b, 4.0f, 2.5f);

    // fleche d'insertion
    Path arrow;
    const float cx = b.getCentreX();
    arrow.addTriangle (cx - 10.0f, b.getY() + 4.0f, cx + 10.0f, b.getY() + 4.0f, cx, b.getY() + 16.0f);
    g.fillPath (arrow);
}

bool RackArea::isInterestedInDragSource (const SourceDetails& d)
{
    return d.description.isInt() && (int) d.description >= 0 && (int) d.description < ModuleId::Count;
}

void RackArea::itemDragEnter (const SourceDetails& d)
{
    dragFromSlot = proc.slotOf ((int) d.description);
    itemDragMove (d);
}

void RackArea::itemDragMove (const SourceDetails& d)
{
    const int s = slotForX (d.localPosition.x);
    if (s != dropSlot) { dropSlot = s; repaint(); }
}

void RackArea::itemDragExit (const SourceDetails&)
{
    dropSlot = -1;
    repaint();
}

void RackArea::itemDropped (const SourceDetails& d)
{
    const int from = proc.slotOf ((int) d.description);
    const int to   = slotForX (d.localPosition.x);
    dropSlot = -1;
    proc.moveModule (from, to);
    layoutModules (true);
    repaint();
    if (onOrderChanged) onOrderChanged();
}

//==============================================================================
MerleauRackEditor::MerleauRackEditor (MerleauRackProcessor& p)
    : AudioProcessorEditor (p), proc (p), rack (p), master (p)
{
    setLookAndFeel (&lnf);

    addAndMakeVisible (rack);
    addAndMakeVisible (master);

    for (int i = 0; i < (int) chainPresets.size(); ++i)
        chainBox.addItem (utf8 (chainPresets[(size_t) i].name), i + 1);
    chainBox.setTextWhenNothingSelected (utf8 ("Ordre personnalisé"));
    chainBox.setJustificationType (Justification::centredLeft);
    chainBox.onChange = [this]
    {
        const int id = chainBox.getSelectedId();
        if (id > 0)
        {
            proc.setOrder (chainPresets[(size_t) id - 1].order);
            rack.layoutModules (true);
        }
    };
    addAndMakeVisible (chainBox);

    rack.onOrderChanged = [this] { syncChainMenu(); };
    syncChainMenu();

    setSize (editorW, editorH);
    startTimerHz (30);
}

MerleauRackEditor::~MerleauRackEditor()
{
    stopTimer();
    setLookAndFeel (nullptr);
}

void MerleauRackEditor::syncChainMenu()
{
    const auto o = proc.getOrder();
    int match = 0;
    for (int i = 0; i < (int) chainPresets.size(); ++i)
        if (chainPresets[(size_t) i].order == o) match = i + 1;
    chainBox.setSelectedId (match, dontSendNotification);
}

void MerleauRackEditor::timerCallback()
{
    if (rack.orderChangedExternally())
    {
        rack.layoutModules (false);
        syncChainMenu();
    }
    rack.updateMeters();
    master.updateMeters();
}

void MerleauRackEditor::resized()
{
    rack.setBounds (woodW + pad, headerH, rackW, rackH);
    master.setBounds (woodW + pad + rackW + pad, headerH, masterW, rackH);
    chainBox.setBounds (getWidth() - woodW - 44 - 320, 28, 320, 26);
}

void MerleauRackEditor::paint (Graphics& g)
{
    const auto r = getLocalBounds().toFloat();
    g.fillAll (Colour (0xff050506));

    // chassis interieur
    auto inner = r.withTrimmedLeft ((float) woodW).withTrimmedRight ((float) woodW);
    ColourGradient chassis (Colour (0xff1b1b1d), 0.0f, 0.0f, Colour (0xff0b0b0c), 0.0f, r.getBottom(), false);
    g.setGradientFill (chassis);
    g.fillRect (inner);

    // bandeau superieur anodise
    auto head = inner.withHeight ((float) headerH - 8.0f).reduced (6.0f, 6.0f);
    ColourGradient hg (Colour (0xff2a2a2d), head.getX(), head.getY(), Colour (0xff121214), head.getX(), head.getBottom(), false);
    g.setGradientFill (hg);
    g.fillRoundedRectangle (head, 4.0f);
    g.setColour (Colours::white.withAlpha (0.08f));
    g.drawRoundedRectangle (head.reduced (0.5f), 4.0f, 1.0f);
    {
        Random rnd (99);
        for (float y = head.getY() + 2.0f; y < head.getBottom() - 2.0f; y += 1.0f)
        {
            g.setColour (Colours::white.withAlpha (rnd.nextFloat() * 0.025f));
            g.drawHorizontalLine ((int) y, head.getX() + 3.0f, head.getRight() - 3.0f);
        }
    }

    mrui::drawScrew (g, { head.getX() + 14.0f, head.getCentreY() }, 6.0f, 0.6f);
    mrui::drawScrew (g, { head.getRight() - 14.0f, head.getCentreY() }, 6.0f, 2.1f);

    // logo
    const float lx = head.getX() + 34.0f;
    mrui::engraved (g, "MerleauRack", { lx, head.getY() + 4.0f, 260.0f, 36.0f }, mrui::Palette::gold,
                    mrui::serif (32.0f, true, true), Justification::centredLeft);
    const float logoW = mrui::textWidth (mrui::serif (32.0f, true, true), "MerleauRack");
    mrui::engraved (g, "C H A N N E L", { lx + logoW + 14.0f, head.getY() + 15.0f, 160.0f, 18.0f }, Colour (0xffe9dcc0),
                    mrui::font (13.0f, true, 0.18f), Justification::centredLeft);
    mrui::engraved (g, "ANALOG CONSOLE STRIP  -  MERLEAU AUDIO", { lx + 2.0f, head.getY() + 38.0f, 400.0f, 12.0f },
                    Colour (0xff8f8a80), mrui::font (8.5f, false, 0.25f), Justification::centredLeft);

    // badge LED d'alimentation
    mrui::drawLed (g, Rectangle<float> (9.0f, 9.0f).withCentre ({ lx + 415.0f, head.getCentreY() - 2.0f }), mrui::Palette::ledRed, true);
    mrui::engraved (g, "POWER", { lx + 395.0f, head.getCentreY() + 6.0f, 40.0f, 10.0f }, Colour (0xff8f8a80), mrui::font (7.5f, true, 0.2f));

    // legende du menu d'ordre
    auto cb = chainBox.getBounds().toFloat();
    mrui::engraved (g, utf8 ("ORDRE DE LA CHAÎNE"), { cb.getX(), cb.getY() - 15.0f, cb.getWidth(), 12.0f },
                    mrui::Palette::gold, mrui::font (9.0f, true, 0.25f), Justification::centredLeft);
    mrui::engraved (g, utf8 ("Glisse un module par son en-tête pour changer l'ordre  -  double-clic sur un bouton = valeur par défaut"),
                    { inner.getX(), (float) (headerH + rackH) + 1.0f, inner.getWidth(), 12.0f }, Colour (0xff77726a),
                    mrui::font (9.5f, false, 0.05f), Justification::centred);

    // flasques en bois
    drawWood (g, r.withWidth ((float) woodW), 3);
    drawWood (g, r.withLeft (r.getRight() - (float) woodW), 8);
}
