#pragma once
#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_dsp/juce_dsp.h>

class StudioRoomModelerAudioProcessor : public juce::AudioProcessor
{
public:
    StudioRoomModelerAudioProcessor();
    ~StudioRoomModelerAudioProcessor() override = default;

    void prepareToPlay (double sampleRate, int samplesPerBlock) override;
    void releaseResources() override;
    bool isBusesLayoutSupported (const BusesLayout& layouts) const override;
    void processBlock (juce::AudioBuffer<float>&, juce::MidiBuffer&) override;

    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override { return true; }

    const juce::String getName() const override { return JucePlugin_Name; }
    bool acceptsMidi() const override { return false; }
    bool producesMidi() const override { return false; }
    bool isMidiEffect() const override { return false; }
    double getTailLengthSeconds() const override { return 4.0; }

    int getNumPrograms() override { return 1; }
    int getCurrentProgram() override { return 0; }
    void setCurrentProgram (int) override {}
    const juce::String getProgramName (int) override { return {}; }
    void changeProgramName (int, const juce::String&) override {}

    void getStateInformation (juce::MemoryBlock& destData) override;
    void setStateInformation (const void* data, int sizeInBytes) override;

    juce::AudioProcessorValueTreeState apvts;

private:
    static juce::AudioProcessorValueTreeState::ParameterLayout createParameterLayout();

    void updateDSPParameters();
    void processSourceStage (juce::AudioBuffer<float>& buffer);
    void processMicStage (juce::AudioBuffer<float>& buffer);
    void processRoomAndDeccaStage (juce::AudioBuffer<float>& buffer);
    void processChamberStage (juce::AudioBuffer<float>& buffer);
    void processConsoleEQStage (juce::AudioBuffer<float>& buffer);
    void processCompressorStage (juce::AudioBuffer<float>& buffer);

    using StereoFilter = juce::dsp::ProcessorDuplicator<
        juce::dsp::IIR::Filter<float>,
        juce::dsp::IIR::Coefficients<float>>;

    double currentSampleRate = 44100.0;

    // 1. Source Stage (Monitors / Vocals / Drums / Ensemble)
    StereoFilter sourceLowCut, sourceHighCut, sourceMidBump;

    // 2. Mic Locker Stage (U48, C12, U67, M50, C800G)
    StereoFilter micLowShelf, micPresence, micAir;

    // 3. Room & Decca Tree Stage
    juce::dsp::Reverb roomReverb;
    juce::dsp::DelayLine<float, juce::dsp::DelayLineInterpolationTypes::Linear> preDelay { 48000 };

    // 4. Underground Echo Chamber Stage
    juce::dsp::Reverb chamberReverb;
    StereoFilter chamberHPF, chamberLPF;
    juce::dsp::Convolution irConvolver; // Listo para cargar IRs WAV externos

    // 5. British Console EQ & Outboard Compressor
    StereoFilter consoleLow, consoleMid, consoleHigh;
    juce::dsp::Compressor<float> compressor;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (StudioRoomModelerAudioProcessor)
};
