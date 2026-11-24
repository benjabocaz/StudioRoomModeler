#include "PluginProcessor.h"
#include "PluginEditor.h"

StudioRoomModelerAudioProcessor::StudioRoomModelerAudioProcessor()
    : AudioProcessor (BusesProperties()
        .withInput  ("Input",  juce::AudioChannelSet::stereo(), true)
        .withOutput ("Output", juce::AudioChannelSet::stereo(), true)),
      apvts (*this, nullptr, "Parameters", createParameterLayout())
{
}

juce::AudioProcessorValueTreeState::ParameterLayout StudioRoomModelerAudioProcessor::createParameterLayout()
{
    std::vector<std::unique_ptr<juce::RangedAudioParameter>> params;

    // Modo y Fuente
    params.push_back (std::make_unique<juce::AudioParameterChoice> (
        "mode", "Mode", juce::StringArray { "Reverb", "Re-Mic" }, 1));

    params.push_back (std::make_unique<juce::AudioParameterChoice> (
        "source", "Source", juce::StringArray { "Vocals", "Drums", "Ensemble", "Monitors (Swedien)" }, 3));

    params.push_back (std::make_unique<juce::AudioParameterChoice> (
        "monitorType", "Monitor Type", juce::StringArray { "Studio Main", "Nearfield (NS-10)", "PA / Wedge" }, 0));

    // Sala y Mic Locker
    params.push_back (std::make_unique<juce::AudioParameterChoice> (
        "room", "Studio Room", juce::StringArray { "Studio A (Large)", "Studio B (Tight)" }, 0));

    params.push_back (std::make_unique<juce::AudioParameterChoice> (
        "mic", "Mic Locker", juce::StringArray { "U48 (Tube)", "C12 (Air)", "U67 (Warm)", "M50 (Decca)", "C800G (Modern)" }, 0));

    params.push_back (std::make_unique<juce::AudioParameterFloat> (
        "micDistance", "Mic Distance", 0.0f, 1.0f, 0.35f));

    params.push_back (std::make_unique<juce::AudioParameterFloat> (
        "deccaWidth", "Decca Sides Width", 0.0f, 2.0f, 1.0f));

    // Cámara de Eco Subterránea
    params.push_back (std::make_unique<juce::AudioParameterFloat> (
        "chamberMix", "Chamber Send", 0.0f, 1.0f, 0.25f));

    params.push_back (std::make_unique<juce::AudioParameterFloat> (
        "chamberDecay", "Chamber Decay", 0.1f, 1.0f, 0.65f));

    params.push_back (std::make_unique<juce::AudioParameterFloat> (
        "chamberPreDelay", "Chamber Pre-Delay (ms)", 0.0f, 150.0f, 25.0f));

    // Consola EQ + Compresor + Orden de Cadena
    params.push_back (std::make_unique<juce::AudioParameterFloat> (
        "eqLow", "Console Low (dB)", -12.0f, 12.0f, 0.0f));

    params.push_back (std::make_unique<juce::AudioParameterFloat> (
        "eqMid", "Console Mid (dB)", -12.0f, 12.0f, 0.0f));

    params.push_back (std::make_unique<juce::AudioParameterFloat> (
        "eqHigh", "Console High (dB)", -12.0f, 12.0f, 0.0f));

    params.push_back (std::make_unique<juce::AudioParameterChoice> (
        "compType", "Compressor", juce::StringArray { "Bypass", "1176LN (FET)", "LA-2A (Opto)", "Mastering (Tube)" }, 1));

    params.push_back (std::make_unique<juce::AudioParameterFloat> (
        "compThresh", "Comp Threshold (dB)", -40.0f, 0.0f, -18.0f));

    params.push_back (std::make_unique<juce::AudioParameterChoice> (
        "chainOrder", "Chain Order", juce::StringArray { "EQ -> Comp -> Chamber", "Comp -> EQ -> Chamber", "Chamber -> Comp -> EQ" }, 0));

    params.push_back (std::make_unique<juce::AudioParameterFloat> (
        "outputGain", "Output Gain (dB)", -18.0f, 12.0f, 0.0f));

    return { params.begin(), params.end() };
}

void StudioRoomModelerAudioProcessor::prepareToPlay (double sampleRate, int samplesPerBlock)
{
    currentSampleRate = sampleRate;

    juce::dsp::ProcessSpec spec;
    spec.sampleRate = sampleRate;
    spec.maximumBlockSize = static_cast<juce::uint32> (samplesPerBlock);
    spec.numChannels = static_cast<juce::uint32> (getTotalNumOutputChannels());

    sourceLowCut.prepare (spec);
    sourceHighCut.prepare (spec);
    sourceMidBump.prepare (spec);

    micLowShelf.prepare (spec);
    micPresence.prepare (spec);
    micAir.prepare (spec);

    roomReverb.prepare (spec);
    chamberReverb.prepare (spec);
    chamberHPF.prepare (spec);
    chamberLPF.prepare (spec);
    preDelay.prepare (spec);
    irConvolver.prepare (spec);

    consoleLow.prepare (spec);
    consoleMid.prepare (spec);
    consoleHigh.prepare (spec);
    compressor.prepare (spec);

    updateDSPParameters();
}

void StudioRoomModelerAudioProcessor::releaseResources() {}

bool StudioRoomModelerAudioProcessor::isBusesLayoutSupported (const BusesLayout& layouts) const
{
    return layouts.getMainOutputChannelSet() == juce::AudioChannelSet::stereo();
}

void StudioRoomModelerAudioProcessor::updateDSPParameters()
{
    const auto sr = currentSampleRate;

    // 1. Source / Monitor Type (Bruce Swedien Worldizing)
    int sourceIdx = static_cast<int> (*apvts.getRawParameterValue ("source"));
    int monType   = static_cast<int> (*apvts.getRawParameterValue ("monitorType"));

    if (sourceIdx == 3) // Monitors (Swedien)
    {
        float hpFreq = (monType == 0) ? 55.0f : (monType == 1 ? 120.0f : 90.0f);
        float lpFreq = (monType == 0) ? 16000.0f : (monType == 1 ? 10500.0f : 13500.0f);
        float midFreq = (monType == 1) ? 1800.0f : 2500.0f;
        float midGain = (monType == 1) ? 3.5f : (monType == 2 ? 2.5f : 1.0f);

        *sourceLowCut.state  = *juce::dsp::IIR::Coefficients<float>::makeHighPass (sr, hpFreq);
        *sourceHighCut.state = *juce::dsp::IIR::Coefficients<float>::makeLowPass  (sr, lpFreq);
        *sourceMidBump.state = *juce::dsp::IIR::Coefficients<float>::makePeakFilter (sr, midFreq, 1.2f, juce::Decibels::decibelsToGain (midGain));
    }
    else
    {
        *sourceLowCut.state  = *juce::dsp::IIR::Coefficients<float>::makeHighPass (sr, 25.0f);
        *sourceHighCut.state = *juce::dsp::IIR::Coefficients<float>::makeLowPass  (sr, 20000.0f);
        *sourceMidBump.state = *juce::dsp::IIR::Coefficients<float>::makePeakFilter (sr, 1000.0f, 1.0f, 1.0f);
    }

    // 2. Mic Locker + Efecto de Proximidad
    int micIdx = static_cast<int> (*apvts.getRawParameterValue ("mic"));
    float dist = *apvts.getRawParameterValue ("micDistance");
    float proximityBoostDb = (1.0f - dist) * 3.0f;

    float presFreq = 4000.0f, presDb = 0.0f, airDb = 0.0f;
    switch (micIdx)
    {
        case 0: presFreq = 3200.0f; presDb = 2.0f; airDb = -0.5f; break; // U48
        case 1: presFreq = 5000.0f; presDb = 1.5f; airDb = 3.5f;  break; // C12
        case 2: presFreq = 2500.0f; presDb = 1.0f; airDb = -1.5f; break; // U67
        case 3: presFreq = 6000.0f; presDb = 2.5f; airDb = 2.0f;  break; // M50
        case 4: presFreq = 4500.0f; presDb = 2.0f; airDb = 4.5f;  break; // C800G
    }

    *micLowShelf.state = *juce::dsp::IIR::Coefficients<float>::makeLowShelf  (sr, 160.0f, 0.7f, juce::Decibels::decibelsToGain (proximityBoostDb));
    *micPresence.state = *juce::dsp::IIR::Coefficients<float>::makePeakFilter (sr, presFreq, 1.4f, juce::Decibels::decibelsToGain (presDb));
    *micAir.state      = *juce::dsp::IIR::Coefficients<float>::makeHighShelf (sr, 10000.0f, 0.7f, juce::Decibels::decibelsToGain (airDb));

    // 3. Room Parameters
    int roomIdx = static_cast<int> (*apvts.getRawParameterValue ("room"));
    int modeIdx = static_cast<int> (*apvts.getRawParameterValue ("mode"));

    juce::dsp::Reverb::Parameters roomParams;
    roomParams.roomSize = (roomIdx == 0) ? 0.72f : 0.42f;
    roomParams.damping  = (roomIdx == 0) ? 0.35f : 0.55f;
    roomParams.width    = 1.0f;
    roomParams.wetLevel = (modeIdx == 1) ? juce::jmap (dist, 0.25f, 0.85f) : juce::jmap (dist, 0.1f, 0.55f);
    roomParams.dryLevel = (modeIdx == 1) ? juce::jmap (dist, 0.85f, 0.20f) : 1.0f;
    roomReverb.setParameters (roomParams);

    // 4. Echo Chamber Parameters
    float chMix   = *apvts.getRawParameterValue ("chamberMix");
    float chDecay = *apvts.getRawParameterValue ("chamberDecay");

    juce::dsp::Reverb::Parameters chamberParams;
    chamberParams.roomSize = juce::jmap (chDecay, 0.35f, 0.96f);
    chamberParams.damping  = 0.45f;
    chamberParams.width    = 0.85f;
    chamberParams.wetLevel = chMix;
    chamberParams.dryLevel = 1.0f;
    chamberReverb.setParameters (chamberParams);

    *chamberHPF.state = *juce::dsp::IIR::Coefficients<float>::makeHighPass (sr, 180.0f);
    *chamberLPF.state = *juce::dsp::IIR::Coefficients<float>::makeLowPass  (sr, 6500.0f);

    // 5. Console EQ & Compressor
    float eqL = *apvts.getRawParameterValue ("eqLow");
    float eqM = *apvts.getRawParameterValue ("eqMid");
    float eqH = *apvts.getRawParameterValue ("eqHigh");

    *consoleLow.state  = *juce::dsp::IIR::Coefficients<float>::makeLowShelf   (sr, 100.0f, 0.7f, juce::Decibels::decibelsToGain (eqL));
    *consoleMid.state  = *juce::dsp::IIR::Coefficients<float>::makePeakFilter (sr, 1200.0f, 1.0f, juce::Decibels::decibelsToGain (eqM));
    *consoleHigh.state = *juce::dsp::IIR::Coefficients<float>::makeHighShelf  (sr, 8000.0f, 0.7f, juce::Decibels::decibelsToGain (eqH));

    int compType = static_cast<int> (*apvts.getRawParameterValue ("compType"));
    float thresh = *apvts.getRawParameterValue ("compThresh");
    compressor.setThreshold (thresh);

    if (compType == 1)      { compressor.setRatio (4.0f); compressor.setAttack (1.5f);  compressor.setRelease (60.0f);  } // 1176LN
    else if (compType == 2) { compressor.setRatio (3.0f); compressor.setAttack (12.0f); compressor.setRelease (250.0f); } // LA-2A
    else if (compType == 3) { compressor.setRatio (2.0f); compressor.setAttack (20.0f); compressor.setRelease (180.0f); } // Mastering
}

void StudioRoomModelerAudioProcessor::processSourceStage (juce::AudioBuffer<float>& buffer)
{
    juce::dsp::AudioBlock<float> block (buffer);
    juce::dsp::ProcessContextReplacing<float> ctx (block);

    sourceLowCut.process (ctx);
    sourceMidBump.process (ctx);
    sourceHighCut.process (ctx);

    // Saturación suave de cono en modo Monitors
    int sourceIdx = static_cast<int> (*apvts.getRawParameterValue ("source"));
    if (sourceIdx == 3)
    {
        for (int ch = 0; ch < buffer.getNumChannels(); ++ch)
        {
            auto* data = buffer.getWritePointer (ch);
            for (int i = 0; i < buffer.getNumSamples(); ++i)
                data[i] = std::tanh (data[i] * 1.25f) / 1.25f;
        }
    }
}

void StudioRoomModelerAudioProcessor::processMicStage (juce::AudioBuffer<float>& buffer)
{
    juce::dsp::AudioBlock<float> block (buffer);
    juce::dsp::ProcessContextReplacing<float> ctx (block);

    micLowShelf.process (ctx);
    micPresence.process (ctx);
    micAir.process (ctx);
}

void StudioRoomModelerAudioProcessor
