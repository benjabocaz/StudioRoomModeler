#pragma once
#include "PluginProcessor.h"

// Estilo visual tipo hardware vintage / Universal Audio
class VintageLookAndFeel : public juce::LookAndFeel_V4
{
public:
    VintageLookAndFeel();

    void drawRotarySlider (juce::Graphics& g, int x, int y, int width, int height,
                           float sliderPos, float rotaryStartAngle, float rotaryEndAngle,
                           juce::Slider& slider) override;

    void drawComboBox (juce::Graphics& g, int width, int height, bool isButtonDown,
                       int buttonX, int buttonY, int buttonW, int buttonH,
                       juce::ComboBox& box) override;
};

// Plano interactivo 2D de la sala (arrastrar el micrófono cambia la distancia en tiempo real)
class RoomPadComponent : public juce::Component, private juce::Timer
{
public:
    explicit RoomPadComponent (juce::AudioProcessorValueTreeState& apvtsRef);
    void paint (juce::Graphics& g) override;
    void mouseDown (const juce::MouseEvent& e) override;
    void mouseDrag (const juce::MouseEvent& e) override;

private:
    void timerCallback() override { repaint(); }
    void updateMicFromMouse (const juce::MouseEvent& e);

    juce::AudioProcessorValueTreeState& apvts;
};

class StudioRoomModelerAudioProcessorEditor : public juce::AudioProcessorEditor
{
public:
    explicit StudioRoomModelerAudioProcessorEditor (StudioRoomModelerAudioProcessor&);
    ~StudioRoomModelerAudioProcessorEditor() override;

    void paint (juce::Graphics&) override;
    void resized() override;

private:
    void setupKnob (juce::Slider& slider, juce::Label& label, const juce::String& text);
    void setupCombo (juce::ComboBox& box, const juce::StringArray& items, juce::Label& label, const juce::String& text);

    StudioRoomModelerAudioProcessor& audioProcessor;
    VintageLookAndFeel vintageLAF;
    RoomPadComponent roomPad;

    // Selectores
    juce::ComboBox modeBox, sourceBox, monitorTypeBox, roomBox, micBox, compTypeBox, chainOrderBox;
    juce::Label modeLbl, sourceLbl, monitorTypeLbl, roomLbl, micLbl, compTypeLbl, chainOrderLbl;

    // Perillas
    juce::Slider micDistSlider, deccaWidthSlider;
    juce::Slider chamberMixSlider, chamberDecaySlider, chamberPreDelaySlider;
    juce::Slider eqLowSlider, eqMidSlider, eqHighSlider, compThreshSlider, outputGainSlider;

    juce::Label micDistLbl, deccaWidthLbl;
    juce::Label chamberMixLbl, chamberDecayLbl, chamberPreDelayLbl;
    juce::Label eqLowLbl, eqMidLbl, eqHighLbl, compThreshLbl, outputGainLbl;

    // Enlaces con APVTS
    using ComboAttach  = juce::AudioProcessorValueTreeState::ComboBoxAttachment;
    using SliderAttach = juce::AudioProcessorValueTreeState::SliderAttachment;

    std::unique_ptr<ComboAttach> modeAtt, sourceAtt, monitorTypeAtt, roomAtt, micAtt, compTypeAtt, chainOrderAtt;
    std::unique_ptr<SliderAttach> micDistAtt, deccaWidthAtt, chamberMixAtt, chamberDecayAtt, chamberPreDelayAtt;
    std::unique_ptr<SliderAttach> eqLowAtt, eqMidAtt, eqHighAtt, compThreshAtt, outputGainAtt;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (StudioRoomModelerAudioProcessorEditor)
};
