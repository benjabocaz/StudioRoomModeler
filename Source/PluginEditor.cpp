#include "PluginEditor.h"

//==============================================================================
// VintageLookAndFeel (Estética Hardware / UAD)
//==============================================================================
VintageLookAndFeel::VintageLookAndFeel()
{
    setColour (juce::ComboBox::backgroundColourId, juce::Colour (0xff191b1f));
    setColour (juce::ComboBox::textColourId, juce::Colour (0xffe8dfc8));
    setColour (juce::ComboBox::outlineColourId, juce::Colour (0xff4a463d));
    setColour (juce::PopupMenu::backgroundColourId, juce::Colour (0xff191b1f));
    setColour (juce::PopupMenu::textColourId, juce::Colour (0xffe8dfc8));
    setColour (juce::PopupMenu::highlightedBackgroundColourId, juce::Colour (0xffc99a49));
    setColour (juce::PopupMenu::highlightedTextColourId, juce::Colour (0xff121316));
}

void VintageLookAndFeel::drawRotarySlider (juce::Graphics& g, int x, int y, int width, int height,
                                           float sliderPos, float rotaryStartAngle, float rotaryEndAngle,
                                           juce::Slider&)
{
    auto bounds = juce::Rectangle<float> ((float) x, (float) y, (float) width, (float) height).reduced (6.0f);
    auto radius = juce::jmin (bounds.getWidth(), bounds.getHeight()) * 0.5f;
    auto cx = bounds.getCentreX();
    auto cy = bounds.getCentreY();
    auto angle = rotaryStartAngle + sliderPos * (rotaryEndAngle - rotaryStartAngle);

    // Arco de fondo y progreso en dorado ámbar
    juce::Path bgArc;
    bgArc.addCentredArc (cx, cy, radius + 3.0f, radius + 3.0f, 0.0f, rotaryStartAngle, rotaryEndAngle, true);
    g.setColour (juce::Colour (0xff2b2e34));
    g.strokePath (bgArc, juce::PathStrokeType (2.5f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));

    juce::Path valArc;
    valArc.addCentredArc (cx, cy, radius + 3.0f, radius + 3.0f, 0.0f, rotaryStartAngle, angle, true);
    g.setColour (juce::Colour (0xffd4a24c));
    g.strokePath (valArc, juce::PathStrokeType (2.5f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));

    // Sombra base de la perilla
    g.setColour (juce::Colours::black.withAlpha (0.55f));
    g.fillEllipse (cx - radius + 1.5f, cy - radius + 3.0f, radius * 2.0f, radius * 2.0f);

    // Cuerpo de baquelita oscura con degradado 3D
    juce::ColourGradient knobGrad (juce::Colour (0xff3a3d45), cx - radius * 0.5f, cy - radius * 0.5f,
                                   juce::Colour (0xff141619), cx + radius * 0.6f, cy + radius * 0.6f, true);
    g.setGradientFill (knobGrad);
    g.fillEllipse (cx - radius, cy - radius, radius * 2.0f, radius * 2.0f);

    // Aro metálico interno
    g.setColour (juce::Colour (0xff5c5a54).withAlpha (0.6f));
    g.drawEllipse (cx - radius * 0.78f, cy - radius * 0.78f, radius * 1.56f, radius * 1.56f, 1.2f);

    // Indicador blanco estilo vintage
    juce::Path pointer;
    pointer.addRoundedRectangle (-1.5f, -radius * 0.85f, 3.0f, radius * 0.55f, 1.5f);
    pointer.applyTransform (juce::AffineTransform::rotation (angle).translated (cx, cy));
    g.setColour (juce::Colour (0xfff5efe0));
    g.fillPath (pointer);
}

void VintageLookAndFeel::drawComboBox (juce::Graphics& g, int width, int height, bool,
                                       int, int, int, int, juce::ComboBox&)
{
    auto area = juce::Rectangle<float> (0.0f, 0.0f, (float) width, (float) height).reduced (0.5f);
    g.setColour (juce::Colour (0xff181a1e));
    g.fillRoundedRectangle (area, 4.0f);
    g.setColour (juce::Colour (0xff524c40));
    g.drawRoundedRectangle (area, 4.0f, 1.0f);

    // Flecha desplegable dorada
    juce::Path arrow;
    auto ax = (float) width - 16.0f;
    auto ay = (float) height * 0.5f - 2.0f;
    arrow.addTriangle (ax, ay, ax + 8.0f, ay, ax + 4.0f, ay + 5.0f);
    g.setColour (juce::Colour (0xffd4a24c));
    g.fillPath (arrow);
}

//==============================================================================
// RoomPadComponent (Plano 2D interactivo del estudio)
//==============================================================================
RoomPadComponent::RoomPadComponent (juce::AudioProcessorValueTreeState& apvtsRef)
    : apvts (apvtsRef)
{
    startTimerHz (24);
}

void RoomPadComponent::paint (juce::Graphics& g)
{
    auto area = getLocalBounds().toFloat().reduced (2.0f);

    // Fondo tipo plano arquitectónico de estudio
    juce::ColourGradient floorGrad (juce::Colour (0xff1a1d22), area.getTopLeft(),
                                    juce::Colour (0xff101215), area.getBottomRight(), false);
    g.setGradientFill (floorGrad);
    g.fillRoundedRectangle (area, 6.0f);

    // Cuadrícula acústica sutil
    g.setColour (juce::Colour (0xff2b3038).withAlpha (0.55f));
    for (float x = area.getX() + 24.0f; x < area.getRight(); x += 24.0f)
        g.drawVerticalLine ((int) x, area.getY() + 4.0f, area.getBottom() - 4.0f);
    for (float y = area.getY() + 24.0f; y < area.getBottom(); y += 24.0f)
        g.drawHorizontalLine ((int) y, area.getX() + 4.0f, area.getRight() - 4.0f);

    // Borde dorado envejecido
    g.setColour (juce::Colour (0xff5c5342));
    g.drawRoundedRectangle (area, 6.0f, 1.5f);

    int roomIdx   = (int) *apvts.getRawParameterValue ("room");
    int sourceIdx = (int) *apvts.getRawParameterValue ("source");
    float dist    = *apvts.getRawParameterValue ("micDistance");
    float width   = *apvts.getRawParameterValue ("deccaWidth");

    // Etiqueta de sala
    g.setColour (juce::Colour (0xffc99a49).withAlpha (0.85f));
    g.setFont (juce::Font (juce::FontOptions (11.0f).withStyle ("Bold")));
    g.drawText (roomIdx == 0 ? "STUDIO A  •  LIVE HALL" : "STUDIO B  •  TIGHT ROOM",
                area.reduced (10.0f).removeFromTop (16.0f), juce::Justification::centredLeft);

    // Fuente sonora en la parte superior
    float srcX = area.getCentreX();
    float srcY = area.getY() + 42.0f;

    g.setColour (juce::Colour (0xffd4a24c).withAlpha (0.2f));
    g.fillEllipse (srcX - 24.0f, srcY - 14.0f, 48.0f, 28.0f);
    g.setColour (juce::Colour (0xffe8dfc8));
    g.drawRoundedRectangle (srcX - 22.0f, srcY - 10.0f, 44.0f, 20.0f, 3.0f, 1.2f);

    juce::String srcLabel = (sourceIdx == 0) ? "VOX" : (sourceIdx == 1 ? "DRUMS" : (sourceIdx == 2 ? "ENS" : "MON"));
    g.setFont (juce::Font (juce::FontOptions (10.0f).withStyle ("Bold")));
    g.drawText (srcLabel, (int) (srcX - 22), (int) (srcY - 10), 44, 20, juce::Justification::centred);

    // Posición del micrófono (arrastrable en Y)
    float minY = area.getY() + 74.0f;
    float maxY = area.getBottom() - 28.0f;
    float micY = juce::jmap (dist, 0.0f, 1.0f, minY, maxY);

    // Ondas de dispersión Decca Tree
    float spread = juce::jmap (width, 0.0f, 2.0f, 12.0f, 68.0f);
    g.setColour (juce::Colour (0xffd4a24c).withAlpha (0.28f));
    g.drawLine (srcX, srcY + 12.0f, srcX - spread, micY, 1.2f);
    g.drawLine (srcX, srcY + 12.0f, srcX + spread, micY, 1.2f);
    g.drawLine (srcX, srcY + 12.0f, srcX, micY, 1.0f);

    // Cápsulas laterales (Decca Sides)
    g.setColour (juce::Colour (0xff9c8e73));
    g.fillEllipse (srcX - spread - 4.0f, micY - 4.0f, 8.0f, 8.0f);
    g.fillEllipse (srcX + spread - 4.0f, micY - 4.0f, 8.0f, 8.0f);

    // Micrófono principal (arrastrable)
    g.setColour (juce::Colour (0xffd4a24c));
    g.fillEllipse (srcX - 9.0f, micY - 9.0f, 18.0f, 18.0f);
    g.setColour (juce::Colour (0xff15171a));
    g.fillEllipse (srcX - 4.0f, micY - 4.0f, 8.0f, 8.0f);

    g.setColour (juce::Colour (0xffe8dfc8).withAlpha (0.65f));
    g.setFont (juce::Font (juce::FontOptions (10.0f)));
    g.drawText ("DRAG MIC TO ADJUST DISTANCE", area.reduced (8.0f).removeFromBottom (14.0f), juce::Justification::centred);
}

void RoomPadComponent::mouseDown (const juce::MouseEvent& e) { updateMicFromMouse (e); }
void RoomPadComponent::mouseDrag (const juce::MouseEvent& e) { updateMicFromMouse (e); }

void RoomPadComponent::updateMicFromMouse (const juce::MouseEvent& e)
{
    auto area = getLocalBounds().toFloat().reduced (2.0f);
    float minY = area.getY() + 74.0f;
    float maxY = area.getBottom() - 28.0f;
    float normDist = juce::jlimit (0.0f, 1.0f, juce::jmap ((float) e.y, minY, maxY, 0.0f, 1.0f));

    if (auto* param = apvts.getParameter ("micDistance"))
        param->setValueNotifyingHost (normDist);
}

//==============================================================================
// StudioRoomModelerAudioProcessorEditor
//==============================================================================
StudioRoomModelerAudioProcessorEditor::StudioRoomModelerAudioProcessorEditor (StudioRoomModelerAudioProcessor& p)
    : AudioProcessorEditor (&p), audioProcessor (p), roomPad (p.apvts)
{
    setLookAndFeel (&vintageLAF);
    setSize (920, 560);

    addAndMakeVisible (roomPad);

    // Selectores
    setupCombo (modeBox,        { "Reverb", "Re-Mic" }, modeLbl, "MODE");
    setupCombo (sourceBox,      { "Vocals", "Drums", "Ensemble", "Monitors (Swedien)" }, sourceLbl, "SOURCE");
    setupCombo (monitorTypeBox, { "Studio Main", "Nearfield (NS-10)", "PA / Wedge" }, monitorTypeLbl, "MONITOR");
    setupCombo (roomBox,        { "Studio A (Large)", "Studio B (Tight)" }, roomLbl, "ROOM");
    setupCombo (micBox,         { "U48 (Tube)", "C12 (Air)", "U67 (Warm)", "M50 (Decca)", "C800G (Modern)" }, micLbl, "MIC LOCKER");
    setupCombo (compTypeBox,    { "Bypass", "1176LN (FET)", "LA-2A (Opto)", "Mastering (Tube)" }, compTypeLbl, "OUTBOARD");
    setupCombo (chainOrderBox,  { "EQ -> Comp -> Chamber", "Comp -> EQ -> Chamber", "Chamber -> Comp -> EQ" }, chainOrderLbl, "ROUTING");

    // Perillas
    setupKnob (micDistSlider,         micDistLbl,         "DISTANCE");
    setupKnob (deccaWidthSlider,      deccaWidthLbl,      "DECCA WIDTH");
    setupKnob (chamberMixSlider,      chamberMixLbl,      "SEND");
    setupKnob (chamberDecaySlider,    chamberDecayLbl,    "DECAY");
    setupKnob (chamberPreDelaySlider, chamberPreDelayLbl, "PRE-DELAY");
    setupKnob (eqLowSlider,           eqLowLbl,           "LOW");
    setupKnob (eqMidSlider,           eqMidLbl,           "MID");
    setupKnob (eqHighSlider,          eqHighLbl,          "HIGH");
    setupKnob (compThreshSlider,      compThreshLbl,      "THRESHOLD");
    setupKnob (outputGainSlider,      outputGainLbl,      "OUTPUT");

    // Attachments
    auto& vts = audioProcessor.apvts;
    modeAtt            = std::make_unique<ComboAttach>  (vts, "mode", modeBox);
    sourceAtt          = std::make_unique<ComboAttach>  (vts, "source", sourceBox);
    monitorTypeAtt     = std::make_unique<ComboAttach>  (vts, "monitorType", monitorTypeBox);
    roomAtt            = std::make_unique<ComboAttach>  (vts, "room", roomBox);
    micAtt             = std::make_unique<ComboAttach>  (vts, "mic", micBox);
    compTypeAtt        = std::make_unique<ComboAttach>  (vts, "compType", compTypeBox);
    chainOrderAtt      = std::make_unique<ComboAttach>  (vts, "chainOrder", chainOrderBox);

    micDistAtt         = std::make_unique<SliderAttach> (vts, "micDistance", micDistSlider);
    deccaWidthAtt      = std::make_unique<SliderAttach> (vts, "deccaWidth", deccaWidthSlider);
    chamberMixAtt      = std::make_unique<SliderAttach> (vts, "chamberMix", chamberMixSlider);
    chamberDecayAtt    = std::make_unique<SliderAttach> (vts, "chamberDecay", chamberDecaySlider);
    chamberPreDelayAtt = std::make_unique<SliderAttach> (vts, "chamberPreDelay", chamberPreDelaySlider);
    eqLowAtt           = std::make_unique<SliderAttach> (vts, "eqLow", eqLowSlider);
    eqMidAtt           = std::make_unique<SliderAttach> (vts, "eqMid", eqMidSlider);
    eqHighAtt          = std::make_unique<SliderAttach> (vts, "eqHigh", eqHighSlider);
    compThreshAtt      = std::make_unique<SliderAttach> (vts, "compThresh", compThreshSlider);
    outputGainAtt      = std::make_unique<SliderAttach> (vts, "outputGain", outputGainSlider);
}

StudioRoomModelerAudioProcessorEditor::~StudioRoomModelerAudioProcessorEditor()
{
    setLookAndFeel (nullptr);
}

void StudioRoomModelerAudioProcessorEditor::setupKnob (juce::Slider& slider, juce::Label& label, const juce::String& text)
{
    slider.setSliderStyle (juce::Slider::RotaryHorizontalVerticalDrag);
    slider.setTextBoxStyle (juce::Slider::TextBoxBelow, false, 58, 16);
    slider.setColour (juce::Slider::textBoxTextColourId, juce::Colour (0xffcfc5b0));
    slider.setColour (juce::Slider::textBoxOutlineColourId, juce::Colours::transparentBlack);
    addAndMakeVisible (slider);

    label.setText (text, juce::dontSendNotification);
    label.setJustificationType (juce::Justification::centred);
    label.setColour (juce::Label::textColourId, juce::Colour (0xffd4a24c));
    label.setFont (juce::Font (juce::FontOptions (10.5f).withStyle ("Bold")));
    addAndMakeVisible (label);
}

void StudioRoomModelerAudioProcessorEditor::setupCombo (juce::ComboBox& box, const juce::StringArray& items, juce::Label& label, const juce::String& text)
{
    box.addItemList (items, 1);
    addAndMakeVisible (box);

    label.setText (text, juce::dontSendNotification);
    label.setJustificationType (juce::Justification::centredLeft);
    label.setColour (juce::Label::textColourId, juce::Colour (0xffb8ae99));
    label.setFont (juce::Font (juce::FontOptions (10.0f).withStyle ("Bold")));
    addAndMakeVisible (label);
}

void StudioRoomModelerAudioProcessorEditor::paint (juce::Graphics& g)
{
    auto bounds = getLocalBounds();

    // 1. Paneles laterales de madera de nogal (estilo hardware UAD)
    auto drawWoodCheek = [&] (juce::Rectangle<int> r)
    {
        juce::ColourGradient wood (juce::Colour (0xff4e2f1a), (float) r.getX(), 0.0f,
                                   juce::Colour (0xff2b170b), (float) r.getRight(), 0.0f, false);
        g.setGradientFill (wood);
        g.fillRect (r);
        g.setColour (juce::Colours::black.withAlpha (0.25f));
        for (int x = r.getX() + 3; x < r.getRight(); x += 5)
            g.drawVerticalLine (x, (float) r.getY(), (float) r.getBottom());
    };

    drawWoodCheek (bounds.removeFromLeft (22));
    drawWoodCheek (bounds.removeFromRight (22));

    // 2. Chasis principal metálico oscuro
    juce::ColourGradient metal (juce::Colour (0xff262930), 0.0f, 0.0f,
                                juce::Colour (0xff17191e), 0.0f, (float) getHeight(), false);
    g.setGradientFill (metal);
    g.fillRect (bounds);

    // 3. Barra superior de título
    auto header = bounds.removeFromTop (56);
    g.setColour (juce::Colour (0xff121418));
    g.fillRect (header);
    g.setColour (juce::Colour (0xffc99a49).withAlpha (0.45f));
    g.drawHorizontalLine (header.getBottom() - 1, (float) header.getX(), (float) header.getRight());

    g.setColour (juce::Colour (0xffe8dfc8));
    g.setFont (juce::Font (juce::FontOptions (20.0f).withStyle ("Bold")));
    g.drawText ("HOLLYWOOD STUDIO MODELER", header.reduced (18, 0), juce::Justification::centredLeft);

    // 4. Marcos de secciones tipo rack
    auto drawPanel = [&] (juce::Rectangle<int> r, const juce::String& title)
    {
        g.setColour (juce::Colour (0xff1d2026));
        g.fillRoundedRectangle (r.toFloat(), 6.0f);
        g.setColour (juce::Colour (0xff3a3d45));
        g.drawRoundedRectangle (r.toFloat(), 6.0f, 1.0f);

        g.setColour (juce::Colour (0xffd4a24c));
        g.setFont (juce::Font (juce::FontOptions (11.0f).withStyle ("Bold")));
        g.drawText (title, r.reduced (10, 6).removeFromTop (16), juce::Justification::centredLeft);
    };

    drawPanel ({ 36, 70, 260, 295 },  "SOURCE & MIC LOCKER");
    drawPanel ({ 310, 70, 310, 295 }, "DYNAMIC ROOM MODELING");
    drawPanel ({ 634, 70, 250, 295 }, "UNDERGROUND CHAMBERS");
    drawPanel ({ 36, 378, 848, 166 }, "BRITISH CONSOLE EQ & OUTBOARD RACK");
}

void StudioRoomModelerAudioProcessorEditor::resized()
{
    // Header
    modeLbl.setBounds (460, 10, 60, 14);
    modeBox.setBounds (460, 24, 120, 24);
    chainOrderLbl.setBounds (600, 10, 120, 14);
    chainOrderBox.setBounds (600, 24, 270, 24);

    // Panel Izquierdo: Source & Mic Locker
    sourceLbl.setBounds (52, 98, 120, 14);      sourceBox.setBounds (52, 114, 228, 24);
    monitorTypeLbl.setBounds (52, 146, 120, 14); monitorTypeBox.setBounds (52, 162, 228, 24);
    roomLbl.setBounds (52, 194, 120, 14);        roomBox.setBounds (52, 210, 228, 24);
    micLbl.setBounds (52, 242, 120, 14);         micBox.setBounds (52, 258, 228, 24);

    deccaWidthLbl.setBounds (116, 288, 100, 14);
    deccaWidthSlider.setBounds (126, 302, 80, 60);

    // Panel Central: Plano 2D + Distancia
    roomPad.setBounds (322, 96, 286, 198);
    micDistLbl.setBounds (415, 298, 100, 14);
    micDistSlider.setBounds (425, 310, 80, 52);

    // Panel Derecho: Echo Chambers
    chamberMixLbl.setBounds (652, 108, 100, 14);      chamberMixSlider.setBounds (662, 124, 84, 84);
    chamberDecayLbl.setBounds (766, 108, 100, 14);    chamberDecaySlider.setBounds (776, 124, 84, 84);
    chamberPreDelayLbl.setBounds (708, 228, 100, 14); chamberPreDelaySlider.setBounds (718, 244, 84, 84);

    // Panel Inferior: Consola EQ, Compresor y Salida
    eqLowLbl.setBounds (60, 412, 80, 14);       eqLowSlider.setBounds (60, 428, 86, 94);
    eqMidLbl.setBounds (162, 412, 80, 14);      eqMidSlider.setBounds (162, 428, 86, 94);
    eqHighLbl.setBounds (264, 412, 80, 14);     eqHighSlider.setBounds (264, 428, 86, 94);

    compTypeLbl.setBounds (396, 428, 120, 14);  compTypeBox.setBounds (396, 446, 160, 28);
    compThreshLbl.setBounds (586, 412, 90, 14); compThreshSlider.setBounds (586, 428, 86, 94);

    outputGainLbl.setBounds (746, 412, 90, 14); outputGainSlider.setBounds (746, 428, 86, 94);
}
