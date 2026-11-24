// MerleauRack Channel - facades des modules (format "500 series" sur rack)
#pragma once

#include <juce_audio_processors/juce_audio_processors.h>
#include "../PluginProcessor.h"
#include "Meters.h"

namespace mrui
{
using namespace juce;
using SliderAttachment = AudioProcessorValueTreeState::SliderAttachment;
using ButtonAttachment = AudioProcessorValueTreeState::ButtonAttachment;

struct Faceplate
{
    Colour face, ink, pointer;
    String title, model, onParam;
};

//==============================================================================
class ModulePanel : public Component
{
public:
    static constexpr int headerH = 50;

    ModulePanel (MerleauRackProcessor& p, int id, Faceplate f, bool draggableIn = true)
        : proc (p), moduleId (id), fp (std::move (f)), draggable (draggableIn)
    {
        setOpaque (true);
    }

    int  getModuleId() const     { return moduleId; }
    void setSlotNumber (int n)   { if (n != slot) { slot = n; repaint(); } }

    // appele ~30 fois/s par l'editeur
    virtual void updateMeters()
    {
        if (fp.onParam.isNotEmpty())
            if (auto* v = proc.apvts.getRawParameterValue (fp.onParam))
            {
                const bool on = v->load() > 0.5f;
                if (on != isOn) { isOn = on; repaint(); }
            }
    }

    void resized() override
    {
        if (texture.getWidth() != getWidth() || texture.getHeight() != getHeight())
            rebuildTexture();
        layoutControls();
    }

    void paint (Graphics& g) override
    {
        const auto r = getLocalBounds().toFloat();
        g.drawImageAt (texture, 0, 0);

        // biseau
        g.setColour (Colours::white.withAlpha (0.16f));
        g.drawLine (0.5f, 0.5f, r.getRight() - 0.5f, 0.5f, 1.0f);
        g.drawLine (0.5f, 0.5f, 0.5f, r.getBottom() - 0.5f, 1.0f);
        g.setColour (Colours::black.withAlpha (0.55f));
        g.drawLine (0.5f, r.getBottom() - 0.5f, r.getRight() - 0.5f, r.getBottom() - 0.5f, 1.5f);
        g.drawLine (r.getRight() - 0.5f, 0.5f, r.getRight() - 0.5f, r.getBottom() - 0.5f, 1.5f);

        // bandeau titre
        auto head = r.withHeight ((float) headerH);
        g.setColour (Colours::black.withAlpha (0.22f));
        g.fillRect (head);
        g.setColour (Colours::black.withAlpha (0.45f));
        g.drawLine (6.0f, head.getBottom(), r.getRight() - 6.0f, head.getBottom(), 1.0f);
        g.setColour (Colours::white.withAlpha (0.10f));
        g.drawLine (6.0f, head.getBottom() + 1.0f, r.getRight() - 6.0f, head.getBottom() + 1.0f, 1.0f);

        {
            auto tf = font (15.0f, true, 0.22f);
            if (textWidth (tf, fp.title) > r.getWidth() - 48.0f) tf = font (14.0f, true, 0.12f);
            engraved (g, fp.title, Rectangle<float> (0.0f, 7.0f, r.getWidth(), 20.0f), fp.ink, tf);
        }

        if (draggable)
        {
            // poignee de deplacement (picots)
            g.setColour (fp.ink.withAlpha (hoverHeader ? 0.9f : 0.45f));
            for (int row = 0; row < 2; ++row)
                for (int i = 0; i < 7; ++i)
                    g.fillEllipse (r.getCentreX() - 21.0f + (float) i * 7.0f - 1.3f, 33.0f + (float) row * 5.0f, 2.6f, 2.6f);
        }
        else
        {
            engraved (g, fp.model, Rectangle<float> (0.0f, 28.0f, r.getWidth(), 14.0f), fp.ink.withAlpha (0.7f), font (9.0f, false, 0.25f));
        }

        // vis de fixation
        Random rnd (moduleId * 31 + 7);
        drawScrew (g, { 12.0f, 12.0f }, 5.0f, rnd.nextFloat() * 3.0f);
        drawScrew (g, { r.getRight() - 12.0f, 12.0f }, 5.0f, rnd.nextFloat() * 3.0f);
        drawScrew (g, { 12.0f, r.getBottom() - 12.0f }, 5.0f, rnd.nextFloat() * 3.0f);
        drawScrew (g, { r.getRight() - 12.0f, r.getBottom() - 12.0f }, 5.0f, rnd.nextFloat() * 3.0f);

        // pied de module : reference + numero de slot
        if (draggable)
        {
            engraved (g, fp.model, Rectangle<float> (0.0f, r.getBottom() - 30.0f, r.getWidth(), 12.0f), fp.ink.withAlpha (0.65f), font (8.5f, false, 0.25f));
            engraved (g, "SLOT " + String (slot), Rectangle<float> (0.0f, r.getBottom() - 18.0f, r.getWidth(), 12.0f), fp.ink.withAlpha (0.85f), font (9.0f, true, 0.3f));
        }

        // legendes des boutons
        for (auto& k : knobs)
            if (! k.captionArea.isEmpty())
                engraved (g, k.caption, k.captionArea, fp.ink, font (9.5f, true, 0.14f));

        for (auto& s : sections)
        {
            auto tr = Rectangle<float> (14.0f, s.y, r.getWidth() - 28.0f, 12.0f);
            g.setFont (font (9.5f, true, 0.25f));
            const float tw = textWidth (g.getCurrentFont(), s.text) + 12.0f;
            g.setColour (fp.ink.withAlpha (0.35f));
            g.drawLine (tr.getX(), tr.getCentreY(), tr.getCentreX() - tw * 0.5f, tr.getCentreY(), 1.0f);
            g.drawLine (tr.getCentreX() + tw * 0.5f, tr.getCentreY(), tr.getRight(), tr.getCentreY(), 1.0f);
            engraved (g, s.text, tr, s.colour.isTransparent() ? fp.ink : s.colour, font (9.5f, true, 0.25f));
        }

        paintExtras (g);
    }

    void paintOverChildren (Graphics& g) override
    {
        if (! isOn)
        {
            g.setColour (Colours::black.withAlpha (0.30f));
            g.fillRect (getLocalBounds().withTrimmedTop (headerH + 2).withTrimmedBottom (34));
        }
    }

    // ---- glisser-deposer par le bandeau titre ---------------------------------
    void mouseMove (const MouseEvent& e) override
    {
        const bool h = draggable && e.y < headerH;
        if (h != hoverHeader) { hoverHeader = h; repaint (0, 0, getWidth(), headerH); }
        setMouseCursor (h ? MouseCursor::DraggingHandCursor : MouseCursor::NormalCursor);
    }
    void mouseExit (const MouseEvent&) override
    {
        if (hoverHeader) { hoverHeader = false; repaint (0, 0, getWidth(), headerH); }
    }
    void mouseDown (const MouseEvent& e) override { dragArmed = draggable && e.y < headerH; }
    void mouseDrag (const MouseEvent& e) override
    {
        if (dragArmed && e.getDistanceFromDragStart() > 4)
        {
            dragArmed = false;
            if (auto* dc = DragAndDropContainer::findParentDragContainerFor (this))
                dc->startDragging (var (moduleId), this);
        }
    }

protected:
    virtual void layoutControls() = 0;
    virtual void paintExtras (Graphics&) {}

    Slider& knob (const String& paramId, const String& caption, Colour pointer = {})
    {
        KnobEntry k;
        k.slider = std::make_unique<Slider> (Slider::RotaryHorizontalVerticalDrag, Slider::NoTextBox);
        auto& s = *k.slider;
        s.setRotaryParameters (MathConstants<float>::pi * 1.22f, MathConstants<float>::pi * 2.78f, true);
        s.setColour (Slider::rotarySliderFillColourId, pointer.isTransparent() ? fp.pointer : pointer);
        s.setColour (Slider::trackColourId, fp.ink);
        s.setMouseDragSensitivity (220);
        s.setPopupDisplayEnabled (true, true, this, 1500);
        s.setScrollWheelEnabled (true);
        k.attachment = std::make_unique<SliderAttachment> (proc.apvts, paramId, s);
        if (auto* prm = proc.apvts.getParameter (paramId))
            s.setDoubleClickReturnValue (true, (double) prm->convertFrom0to1 (prm->getDefaultValue()));
        k.caption = caption;
        addAndMakeVisible (s);
        knobs.push_back (std::move (k));
        return s;
    }

    ToggleButton& button (const String& paramId, const String& text, Colour led)
    {
        ButtonEntry b;
        b.button = std::make_unique<ToggleButton> (text);
        b.button->setColour (ToggleButton::tickColourId, led);
        b.button->setColour (ToggleButton::textColourId, fp.ink);
        b.button->setClickingTogglesState (true);
        b.attachment = std::make_unique<ButtonAttachment> (proc.apvts, paramId, *b.button);
        addAndMakeVisible (*b.button);
        buttons.push_back (std::move (b));
        return *buttons.back().button;
    }

    void place (Slider& s, float cx, float cy, float d)
    {
        s.setBounds (Rectangle<float> (d, d).withCentre ({ cx, cy }).toNearestInt());
        for (auto& k : knobs)
            if (k.slider.get() == &s)
                k.captionArea = Rectangle<float> (cx - 40.0f, cy + d * 0.5f + 1.0f, 80.0f, 12.0f);
    }

    void place (ToggleButton& b, float cx, float top, float size = 30.0f)
    {
        b.setBounds (Rectangle<float> (cx - size * 0.5f - 8.0f, top, size + 16.0f, size + 18.0f).toNearestInt());
    }

    void section (const String& text, float y, Colour c = Colours::transparentBlack)
    {
        for (auto& s : sections) if (s.text == text) { s.y = y; return; }
        sections.push_back ({ text, y, c });
    }

    MerleauRackProcessor& proc;
    const int moduleId;
    Faceplate fp;

private:
    void rebuildTexture()
    {
        const int w = jmax (1, getWidth()), h = jmax (1, getHeight());
        texture = Image (Image::RGB, w, h, false);
        Graphics tg (texture);
        tg.fillAll (fp.face);

        // metal brosse
        Random rnd (1234 + moduleId);
        for (int y = 0; y < h; ++y)
        {
            const float v = rnd.nextFloat();
            tg.setColour ((v > 0.5f ? Colours::white : Colours::black).withAlpha (0.015f + 0.035f * std::abs (v - 0.5f)));
            tg.drawHorizontalLine (y, 0.0f, (float) w);
        }
        for (int i = 0; i < w * h / 40; ++i)
        {
            tg.setColour (Colours::white.withAlpha (rnd.nextFloat() * 0.05f));
            const float x = rnd.nextFloat() * (float) w, y = rnd.nextFloat() * (float) h;
            tg.drawLine (x, y, x + 4.0f + rnd.nextFloat() * 18.0f, y, 0.5f);
        }
        // eclairage
        ColourGradient light (Colours::white.withAlpha (0.10f), 0.0f, 0.0f, Colours::black.withAlpha (0.18f), 0.0f, (float) h, false);
        tg.setGradientFill (light);
        tg.fillAll();
        ColourGradient side (Colours::black.withAlpha (0.12f), 0.0f, 0.0f, Colours::transparentBlack, (float) w * 0.18f, 0.0f, false);
        tg.setGradientFill (side);
        tg.fillAll();
    }

    struct KnobEntry
    {
        std::unique_ptr<Slider> slider;
        std::unique_ptr<SliderAttachment> attachment;
        String caption;
        Rectangle<float> captionArea;
    };
    struct ButtonEntry
    {
        std::unique_ptr<ToggleButton> button;
        std::unique_ptr<ButtonAttachment> attachment;
    };
    struct Section { String text; float y; Colour colour; };

    // les attachments sont detruits avant les composants (ordre dans la struct)
    std::vector<KnobEntry> knobs;
    std::vector<ButtonEntry> buttons;
    std::vector<Section> sections;

    Image texture;
    int slot = 0;
    bool draggable, dragArmed = false, hoverHeader = false, isOn = true;
};

//==============================================================================
class PreampPanel : public ModulePanel
{
public:
    explicit PreampPanel (MerleauRackProcessor& p)
        : ModulePanel (p, ModuleId::Preamp, { Colour (0xff5e1b1c), Colour (0xfff0e3c8), Colour (0xfff4ecd8), "PREAMP", "MR-73 - CLASS A", "pre_on" }),
          gain   (knob ("pre_gain",  "GAIN")),
          drive  (knob ("pre_drive", "DRIVE", Colour (0xffffb347))),
          hpf    (knob ("pre_hpf",   "HPF")),
          trim   (knob ("pre_trim",  "TRIM")),
          phase  (button ("pre_phase", "PHASE", mrui::Palette::ledAmber)),
          inBtn  (button ("pre_on",    "IN",    mrui::Palette::ledRed)) {}

    void layoutControls() override
    {
        const float cx = getWidth() * 0.5f, l = getWidth() * 0.28f, r = getWidth() * 0.72f;
        place (gain, cx, 132, 100);
        place (drive, l, 262, 62);
        place (hpf,   r, 262, 62);
        place (trim, cx, 362, 62);
        place (phase, l, 420);
        place (inBtn, r, 420);
    }

    void paintExtras (Graphics& g) override
    {
        // plaque "+30 dB" autour du gros bouton
        engraved (g, "-20", Rectangle<float> (14.0f, 176.0f, 30.0f, 12.0f), fp.ink.withAlpha (0.7f), font (8.5f, false));
        engraved (g, "+30", Rectangle<float> ((float) getWidth() - 44.0f, 176.0f, 30.0f, 12.0f), fp.ink.withAlpha (0.7f), font (8.5f, false));
    }

private:
    Slider &gain, &drive, &hpf, &trim;
    ToggleButton &phase, &inBtn;
};

//==============================================================================
class GatePanel : public ModulePanel
{
public:
    explicit GatePanel (MerleauRackProcessor& p)
        : ModulePanel (p, ModuleId::Gate, { Colour (0xff1c3a2b), Colour (0xffe7e3d2), Colour (0xfff4ecd8), "GATE", "MR-G2 - EXPANDER", "gate_on" }),
          thresh  (knob ("gate_thresh",  "THRESHOLD")),
          range   (knob ("gate_range",   "RANGE")),
          attack  (knob ("gate_attack",  "ATTACK")),
          hold    (knob ("gate_hold",    "HOLD")),
          release (knob ("gate_release", "RELEASE")),
          inBtn   (button ("gate_on", "IN", mrui::Palette::ledRed)),
          led (mrui::Palette::ledGreen)
    {
        addAndMakeVisible (led);
    }

    void layoutControls() override
    {
        const float cx = getWidth() * 0.5f, l = getWidth() * 0.28f, r = getWidth() * 0.72f;
        place (thresh, cx, 128, 92);
        place (range,  l, 244, 58);
        place (attack, r, 244, 58);
        place (hold,   l, 334, 58);
        place (release, r, 334, 58);
        led.setBounds (Rectangle<int> (26, 26).withCentre ({ (int) l, 436 }));
        place (inBtn, r, 420);
    }

    void paintExtras (Graphics& g) override
    {
        const float l = getWidth() * 0.28f;
        engraved (g, "OPEN", Rectangle<float> (l - 30.0f, 452.0f, 60.0f, 12.0f), fp.ink, font (9.5f, true, 0.14f));
    }

    void updateMeters() override
    {
        ModulePanel::updateMeters();
        const bool active = proc.apvts.getRawParameterValue ("gate_on")->load() > 0.5f;
        const float g = proc.gateGain.load();
        led.setLedColour (g > 0.89f ? mrui::Palette::ledGreen : mrui::Palette::ledRed);
        led.setOn (active);
    }

private:
    Slider &thresh, &range, &attack, &hold, &release;
    ToggleButton& inBtn;
    StatusLed led;
};

//==============================================================================
class EqPanel : public ModulePanel
{
public:
    explicit EqPanel (MerleauRackProcessor& p)
        : ModulePanel (p, ModuleId::EQ, { Colour (0xff22374e), Colour (0xffe9e6dd), Colour (0xfff4ecd8), "EQUALIZER", "MR-4K - 4 BANDS", "eq_on" }),
          hfG  (knob ("eq_hf_gain",  "GAIN", hfCol)),  hfF  (knob ("eq_hf_freq",  "FREQ")),
          hmfG (knob ("eq_hmf_gain", "GAIN", hmfCol)), hmfF (knob ("eq_hmf_freq", "FREQ")), hmfQ (knob ("eq_hmf_q", "Q")),
          lmfG (knob ("eq_lmf_gain", "GAIN", lmfCol)), lmfF (knob ("eq_lmf_freq", "FREQ")), lmfQ (knob ("eq_lmf_q", "Q")),
          lfG  (knob ("eq_lf_gain",  "GAIN", lfCol)),  lfF  (knob ("eq_lf_freq",  "FREQ")),
          lpf  (knob ("eq_lpf", "LPF")),
          inBtn (button ("eq_on", "IN", mrui::Palette::ledRed)) {}

    void layoutControls() override
    {
        const float w = (float) getWidth();
        const float c1 = w * 0.19f, c2 = w * 0.5f, c3 = w * 0.81f, d = 46.0f;
        float top = 58.0f;
        const float rowH = 80.0f;

        section ("HF", top, hfCol);   place (hfG, c1, top + 38, d);  place (hfF, c2, top + 38, d);  top += rowH;
        section ("HMF", top, hmfCol); place (hmfG, c1, top + 38, d); place (hmfF, c2, top + 38, d); place (hmfQ, c3, top + 38, d); top += rowH;
        section ("LMF", top, lmfCol); place (lmfG, c1, top + 38, d); place (lmfF, c2, top + 38, d); place (lmfQ, c3, top + 38, d); top += rowH;
        section ("LF", top, lfCol);   place (lfG, c1, top + 38, d);  place (lfF, c2, top + 38, d);  top += rowH;
        section ("FILTER", top);      place (lpf, c1, top + 38, d);
        place (inBtn, c3 - 4.0f, top + 16.0f);
    }

private:
    const Colour hfCol { 0xffe0503f }, hmfCol { 0xff62c46e }, lmfCol { 0xff5aa0e6 }, lfCol { 0xffe3c25a };
    Slider &hfG, &hfF, &hmfG, &hmfF, &hmfQ, &lmfG, &lmfF, &lmfQ, &lfG, &lfF, &lpf;
    ToggleButton& inBtn;
};

//==============================================================================
class CompPanel : public ModulePanel
{
public:
    explicit CompPanel (MerleauRackProcessor& p)
        : ModulePanel (p, ModuleId::Comp, { Colour (0xff19191b), Colour (0xffdedad0), Colour (0xffd4a64a), "COMPRESSOR", "MR-76 - VCA", "comp_on" }),
          thresh  (knob ("comp_thresh",  "THRESHOLD")),
          ratio   (knob ("comp_ratio",   "RATIO")),
          attack  (knob ("comp_attack",  "ATTACK")),
          release (knob ("comp_release", "RELEASE")),
          knee    (knob ("comp_knee",    "KNEE")),
          makeup  (knob ("comp_makeup",  "MAKE-UP")),
          mix     (knob ("comp_mix",     "MIX")),
          inBtn   (button ("comp_on", "IN", mrui::Palette::ledRed)),
          vu ("GR", { { 0.0f, "0" }, { 1.0f, "1" }, { 3.0f, "3" }, { 6.0f, "6" }, { 10.0f, "10" }, { 20.0f, "20" } },
              [] (float gr) { return 1.0f - std::log1p (jmax (0.0f, gr)) / std::log1p (20.0f); }, 1.0f, 0.0f)
    {
        addAndMakeVisible (vu);
    }

    void layoutControls() override
    {
        const float l = getWidth() * 0.28f, r = getWidth() * 0.72f;
        vu.setBounds (12, headerH + 8, getWidth() - 24, 84);
        place (thresh,  l, 190, 56);
        place (ratio,   r, 190, 56);
        place (attack,  l, 270, 52);
        place (release, r, 270, 52);
        place (knee,    l, 350, 52);
        place (makeup,  r, 350, 52);
        place (mix,     l, 428, 48);
        place (inBtn,   r, 408);
    }

    void updateMeters() override
    {
        ModulePanel::updateMeters();
        vu.setValue (proc.compGrDb.exchange (0.0f));
        vu.tick();
    }

private:
    Slider &thresh, &ratio, &attack, &release, &knee, &makeup, &mix;
    ToggleButton& inBtn;
    NeedleMeter vu;
};

//==============================================================================
class DeEsserPanel : public ModulePanel
{
public:
    explicit DeEsserPanel (MerleauRackProcessor& p)
        : ModulePanel (p, ModuleId::DeEsser, { Colour (0xffd6ccb2), Colour (0xff2b2621), Colour (0xffb3261e), "DE-ESSER", "MR-DS - SPLIT BAND", "des_on" }),
          freq   (knob ("des_freq",   "FREQUENCY")),
          thresh (knob ("des_thresh", "THRESHOLD")),
          range  (knob ("des_range",  "RANGE")),
          listen (button ("des_listen", "LISTEN", mrui::Palette::ledBlue)),
          inBtn  (button ("des_on",     "IN",     mrui::Palette::ledRed)),
          ladder (12, false, LedLadder::Mode::Reduction)
    {
        addAndMakeVisible (ladder);
    }

    void layoutControls() override
    {
        const float cx = getWidth() * 0.5f, l = getWidth() * 0.28f, r = getWidth() * 0.72f;
        place (freq,   cx, 128, 92);
        place (thresh, l, 246, 58);
        place (range,  r, 246, 58);
        ladder.setBounds (18, 318, getWidth() - 36, 14);
        place (listen, l, 380);
        place (inBtn,  r, 380);
    }

    void paintExtras (Graphics& g) override
    {
        engraved (g, "REDUCTION", Rectangle<float> (0.0f, 336.0f, (float) getWidth(), 12.0f), fp.ink, font (9.0f, true, 0.2f));
        engraved (g, "0", Rectangle<float> (14.0f, 302.0f, 20.0f, 12.0f), fp.ink.withAlpha (0.7f), font (8.0f, false));
        engraved (g, "-12", Rectangle<float> ((float) getWidth() - 36.0f, 302.0f, 24.0f, 12.0f), fp.ink.withAlpha (0.7f), font (8.0f, false));
    }

    void updateMeters() override
    {
        ModulePanel::updateMeters();
        const float gr = proc.deessGrDb.exchange (0.0f);
        shown = jmax (gr, shown - 0.8f);
        ladder.setValue (shown / 12.0f);
    }

private:
    Slider &freq, &thresh, &range;
    ToggleButton &listen, &inBtn;
    LedLadder ladder;
    float shown = 0.0f;
};

//==============================================================================
// Section MASTER (fixe, non deplacable)
class MasterPanel : public ModulePanel
{
public:
    explicit MasterPanel (MerleauRackProcessor& p)
        : ModulePanel (p, 9, { Colour (0xff2a2826), Colour (0xffd4a64a), Colour (0xfff4ecd8), "MASTER", "MERLEAU AUDIO", {} }, false),
          inK  (knob ("in_gain",  "INPUT")),
          outK (knob ("out_gain", "OUTPUT")),
          vu ("VU", { { -20.0f, "20" }, { -10.0f, "10" }, { -7.0f, "7" }, { -5.0f, "5" }, { -3.0f, "3" }, { 0.0f, "0" }, { 3.0f, "+3" } },
              [] (float v) { return vuPos (v); }, vuPos (0.0f), -40.0f),
          inMeter (16, true, LedLadder::Mode::Level),
          outMeter (16, true, LedLadder::Mode::Level)
    {
        addAndMakeVisible (vu);
        addAndMakeVisible (inMeter);
        addAndMakeVisible (outMeter);
    }

    static float vuPos (float vuDb)
    {
        auto lin = [] (float db) { return std::pow (10.0f, db / 20.0f); };
        return (lin (vuDb) - lin (-20.0f)) / (lin (3.0f) - lin (-20.0f));
    }

    void layoutControls() override
    {
        const float cx = getWidth() * 0.5f;
        vu.setBounds (10, headerH + 8, getWidth() - 20, 80);
        place (inK,  cx, 198, 66);
        place (outK, cx, 300, 66);
        inMeter.setBounds  ((int) (cx - 34), meterTop, 16, meterH);
        outMeter.setBounds ((int) (cx + 18), meterTop, 16, meterH);
    }

    void paintExtras (Graphics& g) override
    {
        const float cx = getWidth() * 0.5f;
        const float lblY = (float) (meterTop + meterH + 4);
        engraved (g, "IN",  Rectangle<float> (cx - 46.0f, lblY, 40.0f, 12.0f), fp.ink, font (9.0f, true, 0.2f));
        engraved (g, "OUT", Rectangle<float> (cx + 6.0f, lblY, 40.0f, 12.0f), fp.ink, font (9.0f, true, 0.2f));
        g.setColour (fp.ink.withAlpha (0.55f));
        g.setFont (font (7.5f, false));
        for (int db : { 0, -6, -12, -24, -36, -48 })
        {
            const float y = (float) meterTop + 3.0f + (1.0f - (float) (db + 48) / 48.0f) * (float) (meterH - 6);
            g.drawText (String (db), Rectangle<float> (cx - 13.0f, y - 5.0f, 26.0f, 10.0f), Justification::centred, false);
        }
        engraved (g, "0 VU = -18 dBFS", Rectangle<float> (0.0f, (float) getHeight() - 28.0f, (float) getWidth(), 12.0f), fp.ink.withAlpha (0.6f), font (8.0f, false, 0.1f));
    }

    void updateMeters() override
    {
        const float inDb  = Decibels::gainToDecibels (proc.inPeak.exchange (0.0f), -100.0f);
        const float outDb = Decibels::gainToDecibels (proc.outPeak.exchange (0.0f), -100.0f);
        inShown  = jmax (inDb,  inShown  - 1.2f);
        outShown = jmax (outDb, outShown - 1.2f);
        inMeter.setValue  ((inShown + 48.0f) / 48.0f);
        outMeter.setValue ((outShown + 48.0f) / 48.0f);
        vu.setValue (outDb + 18.0f);
        vu.tick();
    }

private:
    Slider &inK, &outK;
    NeedleMeter vu;
    LedLadder inMeter, outMeter;
    float inShown = -100.0f, outShown = -100.0f;
    static constexpr int meterTop = 350, meterH = 112;
};

} // namespace mrui
