// MerleauRack Channel - vu-metre a aiguille, echelle de LED, voyant
#pragma once

#include "RackLookAndFeel.h"
#include <functional>
#include <vector>

namespace mrui
{
using namespace juce;

//==============================================================================
// Vu-metre analogique a aiguille (cadran creme retro-eclaire)
class NeedleMeter : public Component
{
public:
    struct Mark { float value; String text; };

    // toPos : valeur -> position 0..1 sur l'arc ; redFrom : debut de la zone rouge (0..1)
    NeedleMeter (String legendIn, std::vector<Mark> marksIn, std::function<float (float)> toPosIn, float redFromIn, float restValue)
        : legend (std::move (legendIn)), marks (std::move (marksIn)), toPos (std::move (toPosIn)),
          redFrom (redFromIn), shown (toPos (restValue)), target (shown) {}

    void setValue (float v)      { target = jlimit (-0.04f, 1.04f, toPos (v)); }

    // balistique type VU (~300 ms)
    void tick()
    {
        const float before = shown;
        shown += (target - shown) * 0.28f;
        if (std::abs (shown - before) > 0.0005f) repaint();
    }

    void paint (Graphics& g) override
    {
        auto r = getLocalBounds().toFloat();

        // boitier
        g.setColour (Colours::black);
        g.fillRoundedRectangle (r, 5.0f);
        auto bezel = r.reduced (2.0f);
        ColourGradient bz (Colour (0xff6b6b6b), bezel.getX(), bezel.getY(), Colour (0xff1c1c1c), bezel.getX(), bezel.getBottom(), false);
        g.setGradientFill (bz);
        g.fillRoundedRectangle (bezel, 4.0f);

        // cadran creme retro-eclaire
        auto face = bezel.reduced (3.0f);
        ColourGradient fg (Colour (0xfffff1c8), face.getCentreX(), face.getY() + face.getHeight() * 0.35f,
                           Colour (0xffd9b870), face.getX(), face.getBottom(), true);
        g.setGradientFill (fg);
        g.fillRoundedRectangle (face, 3.0f);

        const Point<float> pivot { face.getCentreX(), face.getBottom() + face.getHeight() * 0.42f };
        const float radius = face.getHeight() * 1.05f;
        const float a0 = -0.80f, a1 = 0.80f;
        auto angleOf = [&] (float pos) { return a0 + pos * (a1 - a0); };
        auto pt = [&] (float a, float rr) { return Point<float> (pivot.x + rr * std::sin (a), pivot.y - rr * std::cos (a)); };

        g.saveState();
        g.reduceClipRegion (face.toNearestInt());

        // arc noir + zone rouge
        Path arc;
        arc.addCentredArc (pivot.x, pivot.y, radius * 0.80f, radius * 0.80f, 0.0f, angleOf (0.0f), angleOf (jmin (1.0f, redFrom)), true);
        g.setColour (Colour (0xff1a1410));
        g.strokePath (arc, PathStrokeType (1.4f));
        const bool hasRed = redFrom < 1.0f;
        if (hasRed)
        {
            Path red;
            red.addCentredArc (pivot.x, pivot.y, radius * 0.80f + 1.5f, radius * 0.80f + 1.5f, 0.0f, angleOf (redFrom), angleOf (1.0f), true);
            g.setColour (Colour (0xffc0281e));
            g.strokePath (red, PathStrokeType (4.0f));
        }

        g.setFont (font (jmax (7.5f, face.getHeight() * 0.14f), true));
        for (auto& m : marks)
        {
            const float p = toPos (m.value), a = angleOf (p);
            g.setColour (hasRed && p >= redFrom ? Colour (0xffb01f16) : Colour (0xff1a1410));
            g.drawLine (Line<float> (pt (a, radius * 0.80f), pt (a, radius * 0.88f)), 1.3f);
            auto tp = pt (a, radius * 0.97f);
            g.drawText (m.text, Rectangle<float> (26.0f, 11.0f).withCentre (tp), Justification::centred, false);
        }

        engraved (g, legend, Rectangle<float> (face.getX(), face.getBottom() - face.getHeight() * 0.36f, face.getWidth(), face.getHeight() * 0.2f),
                  Colour (0xff1a1410), serif (jmax (8.0f, face.getHeight() * 0.17f), true, true));

        // aiguille
        const float a = angleOf (jlimit (-0.04f, 1.04f, shown));
        g.setColour (Colours::black.withAlpha (0.18f));
        g.drawLine (Line<float> (pivot.translated (2.0f, 1.0f), pt (a, radius * 0.92f).translated (2.0f, 1.0f)), 1.6f);
        g.setColour (Colour (0xff141010));
        g.drawLine (Line<float> (pivot, pt (a, radius * 0.92f)), 1.3f);
        g.restoreState();

        // reflet verre
        ColourGradient glass (Colours::white.withAlpha (0.22f), face.getX(), face.getY(), Colours::white.withAlpha (0.0f), face.getX(), face.getCentreY(), false);
        g.setGradientFill (glass);
        g.fillRoundedRectangle (face.withHeight (face.getHeight() * 0.45f), 3.0f);
    }

private:
    String legend;
    std::vector<Mark> marks;
    std::function<float (float)> toPos;
    float redFrom, shown, target;
};

//==============================================================================
// Echelle de LED (vertical ou horizontal). value : 0..1
class LedLadder : public Component
{
public:
    enum class Mode { Level, Reduction };

    LedLadder (int segmentsIn, bool verticalIn, Mode modeIn) : segments (segmentsIn), vertical (verticalIn), mode (modeIn) {}

    void setValue (float v01)
    {
        const int lit = jlimit (0, segments, roundToInt (v01 * (float) segments));
        if (lit != litCount) { litCount = lit; repaint(); }
    }

    void paint (Graphics& g) override
    {
        auto r = getLocalBounds().toFloat();
        g.setColour (Colours::black.withAlpha (0.85f));
        g.fillRoundedRectangle (r, 3.0f);
        auto in = r.reduced (3.0f);
        const float gap = 2.0f;
        const float len = ((vertical ? in.getHeight() : in.getWidth()) - gap * (float) (segments - 1)) / (float) segments;

        for (int i = 0; i < segments; ++i)
        {
            Rectangle<float> seg = vertical
                ? Rectangle<float> (in.getX(), in.getBottom() - (float) (i + 1) * len - (float) i * gap, in.getWidth(), len)
                : Rectangle<float> (in.getX() + (float) i * (len + gap), in.getY(), len, in.getHeight());

            const float f = (float) i / (float) jmax (1, segments - 1);
            Colour col = mode == Mode::Level
                ? (f > 0.86f ? Palette::ledRed : f > 0.66f ? Palette::ledAmber : Palette::ledGreen)
                : (f > 0.66f ? Palette::ledRed : Palette::ledAmber);
            const bool on = i < litCount;

            g.setColour (on ? col : col.darker (2.6f).withMultipliedSaturation (0.45f));
            g.fillRoundedRectangle (seg, 1.2f);
            if (on)
            {
                g.setColour (Colours::white.withAlpha (0.35f));
                g.fillRoundedRectangle (seg.withHeight (seg.getHeight() * 0.4f), 1.2f);
            }
        }
    }

private:
    int segments, litCount = 0;
    bool vertical;
    Mode mode;
};

//==============================================================================
class StatusLed : public Component
{
public:
    explicit StatusLed (Colour c) : colour (c) {}
    void setOn (bool b)          { if (b != on) { on = b; repaint(); } }
    void setLedColour (Colour c) { if (c != colour) { colour = c; repaint(); } }
    void paint (Graphics& g) override
    {
        const float s = (float) jmin (getWidth(), getHeight()) * 0.5f;
        drawLed (g, Rectangle<float> (s, s).withCentre (getLocalBounds().toFloat().getCentre()), colour, on);
    }
private:
    Colour colour;
    bool on = false;
};

} // namespace mrui
