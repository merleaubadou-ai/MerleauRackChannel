// MerleauRack Channel - look "console analogique" : boutons bakelite a jupe moletee,
// poussoirs a LED, typographie gravee.
#pragma once

#include <juce_gui_basics/juce_gui_basics.h>

namespace mrui
{
using namespace juce;

namespace Palette
{
    const Colour gold      { 0xffd4a64a };
    const Colour goldDim   { 0xff8a6a2c };
    const Colour rackBlack { 0xff0e0e10 };
    const Colour ledRed    { 0xffff3b2f };
    const Colour ledGreen  { 0xff42f56c };
    const Colour ledAmber  { 0xffffb020 };
    const Colour ledBlue   { 0xff4fb3ff };
}

inline Font font (float h, bool bold = true, float kerning = 0.0f)
{
    Font f { FontOptions (h, bold ? Font::bold : Font::plain) };
    return kerning != 0.0f ? f.withExtraKerningFactor (kerning) : f;
}

inline Font serif (float h, bool bold = true, bool italic = false)
{
    static const String name = []
    {
        const auto names = Font::findAllTypefaceNames();
        for (auto* n : { "Didot", "Bodoni 72", "Georgia", "Times New Roman", "DejaVu Serif", "Liberation Serif" })
            if (names.contains (n)) return String (n);
        return Font::getDefaultSerifFontName();
    }();
    int style = (bold ? Font::bold : 0) | (italic ? Font::italic : 0);
    return Font { FontOptions (name, h, style) };
}

inline float textWidth (const Font& f, const String& t)
{
    GlyphArrangement ga;
    ga.addLineOfText (f, t, 0.0f, 0.0f);
    return ga.getBoundingBox (0, -1, true).getWidth();
}

// Texte grave dans le metal : ombre sombre decalee + texte
inline void engraved (Graphics& g, const String& t, Rectangle<float> r, Colour ink, Font f,
                      Justification j = Justification::centred)
{
    g.setFont (f);
    g.setColour (Colours::black.withAlpha (0.45f));
    g.drawText (t, r.translated (0.0f, 1.0f), j, false);
    g.setColour (ink);
    g.drawText (t, r, j, false);
}

inline void drawScrew (Graphics& g, Point<float> c, float r, float angle)
{
    g.setColour (Colours::black.withAlpha (0.55f));
    g.fillEllipse (Rectangle<float> (r * 2.0f + 2.0f, r * 2.0f + 2.0f).withCentre (c.translated (0.0f, 1.0f)));
    ColourGradient cg (Colour (0xfff2f2f2), c.x - r, c.y - r, Colour (0xff4a4a4a), c.x + r, c.y + r, false);
    cg.addColour (0.5, Colour (0xffa8a8a8));
    g.setGradientFill (cg);
    g.fillEllipse (Rectangle<float> (r * 2.0f, r * 2.0f).withCentre (c));
    Path slot;
    slot.addRoundedRectangle (-r * 0.75f, -r * 0.14f, r * 1.5f, r * 0.28f, 0.6f);
    slot.addRoundedRectangle (-r * 0.14f, -r * 0.75f, r * 0.28f, r * 1.5f, 0.6f);
    g.setColour (Colour (0xff2a2a2a));
    g.fillPath (slot, AffineTransform::rotation (angle).translated (c));
}

inline void drawLed (Graphics& g, Rectangle<float> r, Colour col, bool on)
{
    auto c = r.getCentre();
    if (on)
    {
        ColourGradient glow (col.withAlpha (0.55f), c.x, c.y, col.withAlpha (0.0f), c.x + r.getWidth() * 1.4f, c.y, true);
        g.setGradientFill (glow);
        g.fillEllipse (r.expanded (r.getWidth() * 0.9f));
    }
    g.setColour (Colours::black.withAlpha (0.7f));
    g.fillEllipse (r.expanded (1.5f));
    ColourGradient body (on ? col.brighter (0.6f) : col.darker (2.2f), c.x - r.getWidth() * 0.2f, c.y - r.getHeight() * 0.25f,
                         on ? col : col.darker (3.0f).withMultipliedSaturation (0.5f), c.x, c.y + r.getHeight() * 0.5f, true);
    g.setGradientFill (body);
    g.fillEllipse (r);
    g.setColour (Colours::white.withAlpha (on ? 0.75f : 0.18f));
    g.fillEllipse (r.withSizeKeepingCentre (r.getWidth() * 0.35f, r.getHeight() * 0.25f).translated (-r.getWidth() * 0.12f, -r.getHeight() * 0.2f));
}

//==============================================================================
class RackLookAndFeel : public LookAndFeel_V4
{
public:
    RackLookAndFeel()
    {
        setColour (Slider::rotarySliderFillColourId, Colour (0xfff4ecd8));
        setColour (Slider::trackColourId, Colour (0xffe8e0cc));
        setColour (BubbleComponent::backgroundColourId, Colour (0xf0101012));
        setColour (BubbleComponent::outlineColourId, Palette::gold);
        setColour (TooltipWindow::textColourId, Palette::gold);
        setColour (ComboBox::backgroundColourId, Colour (0xff141416));
        setColour (ComboBox::outlineColourId, Palette::goldDim);
        setColour (ComboBox::textColourId, Colour (0xffe9dcc0));
        setColour (ComboBox::arrowColourId, Palette::gold);
        setColour (PopupMenu::backgroundColourId, Colour (0xff141416));
        setColour (PopupMenu::textColourId, Colour (0xffe9dcc0));
        setColour (PopupMenu::highlightedBackgroundColourId, Palette::goldDim);
        setColour (PopupMenu::highlightedTextColourId, Colours::white);
    }

    // ---- Bouton rotatif : graduation + jupe metal moletee + capuchon bakelite
    void drawRotarySlider (Graphics& g, int x, int y, int w, int h, float pos,
                           float startA, float endA, Slider& s) override
    {
        const auto bounds = Rectangle<float> ((float) x, (float) y, (float) w, (float) h);
        const float size  = jmin (bounds.getWidth(), bounds.getHeight());
        const auto  c     = bounds.getCentre();
        const float rOut  = size * 0.5f - 1.0f;
        const float rKnob = rOut - jmax (5.0f, size * 0.13f);
        const float rCap  = rKnob * 0.70f;

        // graduation
        const auto tickCol = s.findColour (Slider::trackColourId);
        const int ticks = 11;
        for (int i = 0; i < ticks; ++i)
        {
            const float a  = startA + (float) i / (float) (ticks - 1) * (endA - startA);
            const bool big = (i == 0 || i == ticks - 1 || i == ticks / 2);
            const float r1 = rOut, r2 = rOut - (big ? size * 0.075f : size * 0.045f);
            g.setColour (tickCol.withAlpha (big ? 0.95f : 0.6f));
            g.drawLine (c.x + r1 * std::sin (a), c.y - r1 * std::cos (a),
                        c.x + r2 * std::sin (a), c.y - r2 * std::cos (a), big ? 1.6f : 1.0f);
        }

        // ombre portee
        g.setColour (Colours::black.withAlpha (0.55f));
        g.fillEllipse (Rectangle<float> (rKnob * 2.0f + 3.0f, rKnob * 2.0f + 3.0f).withCentre (c.translated (0.0f, size * 0.035f)));

        // jupe metallique
        auto knob = Rectangle<float> (rKnob * 2.0f, rKnob * 2.0f).withCentre (c);
        ColourGradient skirt (Colour (0xffeeeeee), c.x - rKnob * 0.6f, c.y - rKnob, Colour (0xff4b4b4d), c.x + rKnob * 0.4f, c.y + rKnob, false);
        skirt.addColour (0.45, Colour (0xffa9a9ab));
        g.setGradientFill (skirt);
        g.fillEllipse (knob);

        // moletage
        g.setColour (Colours::black.withAlpha (0.28f));
        const int knurls = jlimit (18, 48, (int) (rKnob * 1.6f));
        for (int i = 0; i < knurls; ++i)
        {
            const float a = MathConstants<float>::twoPi * (float) i / (float) knurls;
            g.drawLine (c.x + rKnob * 0.80f * std::sin (a), c.y - rKnob * 0.80f * std::cos (a),
                        c.x + rKnob * 0.98f * std::sin (a), c.y - rKnob * 0.98f * std::cos (a), 0.8f);
        }

        // capuchon bakelite
        auto cap = Rectangle<float> (rCap * 2.0f, rCap * 2.0f).withCentre (c);
        g.setColour (Colours::black.withAlpha (0.6f));
        g.fillEllipse (cap.expanded (1.2f));
        ColourGradient capG (Colour (0xff3d3b3a), c.x, c.y - rCap, Colour (0xff0b0b0b), c.x, c.y + rCap, false);
        g.setGradientFill (capG);
        g.fillEllipse (cap);
        ColourGradient shine (Colours::white.withAlpha (0.20f), c.x - rCap * 0.3f, c.y - rCap * 0.6f,
                              Colours::white.withAlpha (0.0f), c.x, c.y, true);
        g.setGradientFill (shine);
        g.fillEllipse (cap.reduced (rCap * 0.08f));

        // index
        const float a = startA + pos * (endA - startA);
        Path ptr;
        const float pw = jmax (2.0f, rCap * 0.13f);
        ptr.addRoundedRectangle (-pw * 0.5f, -rKnob * 0.96f, pw, rKnob * 0.62f, pw * 0.5f);
        g.setColour (s.findColour (Slider::rotarySliderFillColourId));
        g.fillPath (ptr, AffineTransform::rotation (a).translated (c));

        if (! s.isEnabled())
        {
            g.setColour (Colours::black.withAlpha (0.35f));
            g.fillEllipse (knob);
        }
    }

    // ---- Poussoir metal a LED, legende gravee en dessous
    void drawToggleButton (Graphics& g, ToggleButton& b, bool hover, bool down) override
    {
        auto r = b.getLocalBounds().toFloat();
        const float bs = jmin (r.getWidth() - 4.0f, r.getHeight() - 16.0f);
        auto btn = Rectangle<float> (bs, bs).withCentre ({ r.getCentreX(), r.getY() + bs * 0.5f + 2.0f });
        const bool on = b.getToggleState();

        g.setColour (Colours::black.withAlpha (0.6f));
        g.fillRoundedRectangle (btn.expanded (2.0f).translated (0.0f, 1.5f), 4.0f);

        ColourGradient bezel (Colour (0xffd8d8d8), btn.getX(), btn.getY(), Colour (0xff3a3a3a), btn.getRight(), btn.getBottom(), false);
        g.setGradientFill (bezel);
        g.fillRoundedRectangle (btn, 3.5f);

        auto face = btn.reduced (bs * 0.12f).translated (0.0f, down ? 1.0f : 0.0f);
        ColourGradient faceG (Colour (down ? 0xff1b1b1b : 0xff2c2c2e), face.getX(), face.getY(),
                              Colour (0xff0c0c0c), face.getX(), face.getBottom(), false);
        g.setGradientFill (faceG);
        g.fillRoundedRectangle (face, 2.5f);
        if (hover)
        {
            g.setColour (Colours::white.withAlpha (0.06f));
            g.fillRoundedRectangle (face, 2.5f);
        }

        const float ls = bs * 0.30f;
        drawLed (g, Rectangle<float> (ls, ls).withCentre (face.getCentre()), b.findColour (ToggleButton::tickColourId), on);

        auto txt = Rectangle<float> (r.getX() - 6.0f, btn.getBottom() + 2.0f, r.getWidth() + 12.0f, 13.0f);
        engraved (g, b.getButtonText(), txt, b.findColour (ToggleButton::textColourId), font (9.5f, true, 0.12f));
    }

    Font getComboBoxFont (ComboBox&) override        { return font (12.0f, false); }
    Font getPopupMenuFont() override                 { return font (13.0f, false); }
    Font getSliderPopupFont (Slider&) override       { return font (12.5f, true); }
    int  getSliderPopupPlacement (Slider&) override  { return BubbleComponent::above; }
};

} // namespace mrui
