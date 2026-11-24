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
        case 0: presFreq = 3200.0
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

void StudioRoomModelerAudioProcessor::processRoomAndDeccaStage (juce::AudioBuffer<float>& buffer)
{
    juce::dsp::AudioBlock<float> block (buffer);
    juce::dsp::ProcessContextReplacing<float> ctx (block);
    roomReverb.process (ctx);

    // Matriz Mid/Side para controlar el balance Center / Sides del Decca Tree
    float width = *apvts.getRawParameterValue ("deccaWidth");
    if (buffer.getNumChannels() >= 2)
    {
        auto* left  = buffer.getWritePointer (0);
        auto* right = buffer.getWritePointer (1);
        for (int i = 0; i < buffer.getNumSamples(); ++i)
        {
            float mid  = 0.5f * (left[i] + right[i]);
            float side = 0.5f * (left[i] - right[i]) * width;
            left[i]  = mid + side;
            right[i] = mid - side;
        }
    }
}

void StudioRoomModelerAudioProcessor::processChamberStage (juce::AudioBuffer<float>& buffer)
{
    float mix = *apvts.getRawParameterValue ("chamberMix");
    if (mix <= 0.001f) return;

    juce::AudioBuffer<float> chamberBuf;
    chamberBuf.makeCopyOf (buffer);

    float preDelayMs = *apvts.getRawParameterValue ("chamberPreDelay");
    float delaySamples = static_cast<float> (preDelayMs * 0.001 * currentSampleRate);
    preDelay.setDelay (juce::jlimit (0.0f, 47999.0f, delaySamples));

    for (int ch = 0; ch < chamberBuf.getNumChannels(); ++ch)
    {
        auto* data = chamberBuf.getWritePointer (ch);
        for (int i = 0; i < chamberBuf.getNumSamples(); ++i)
        {
            preDelay.pushSample (ch, data[i]);
            data[i] = preDelay.popSample (ch);
        }
    }

    juce::dsp::AudioBlock<float> block (chamberBuf);
    juce::dsp::ProcessContextReplacing<float> ctx (block);
    chamberHPF.process (ctx);
    chamberLPF.process (ctx);
    chamberReverb.process (ctx);

    for (int ch = 0; ch < buffer.getNumChannels(); ++ch)
        buffer.addFrom (ch, 0, chamberBuf, ch, 0, buffer.getNumSamples(), mix);
}

void StudioRoomModelerAudioProcessor::processConsoleEQStage (juce::AudioBuffer<float>& buffer)
{
    juce::dsp::AudioBlock<float> block (buffer);
    juce::dsp::ProcessContextReplacing<float> ctx (block);
    consoleLow.process (ctx);
    consoleMid.process (ctx);
    consoleHigh.process (ctx);
}

void StudioRoomModelerAudioProcessor::processCompressorStage (juce::AudioBuffer<float>& buffer)
{
    int compType = static_cast<int> (*apvts.getRawParameterValue ("compType"));
    if (compType == 0) return;

    juce::dsp::AudioBlock<float> block (buffer);
    juce::dsp::ProcessContextReplacing<float> ctx (block);
    compressor.process (ctx);
}

void StudioRoomModelerAudioProcessor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer&)
{
    juce::ScopedNoDenormals noDenormals;
    updateDSPParameters();

    // 1. Entrada física: Fuente -> Sala -> Micrófono
    processSourceStage (buffer);
    processRoomAndDeccaStage (buffer);
    processMicStage (buffer);

    // 2. Cadena modular reordenable (Consola EQ, Compresor, Cámara)
    int order = static_cast<int> (*apvts.getRawParameterValue ("chainOrder"));
    if (order == 0) // EQ -> Comp -> Chamber
    {
        processConsoleEQStage (buffer);
        processCompressorStage (buffer);
        processChamberStage (buffer);
    }
    else if (order == 1) // Comp -> EQ -> Chamber
    {
        processCompressorStage (buffer);
        processConsoleEQStage (buffer);
        processChamberStage (buffer);
    }
    else // Chamber -> Comp -> EQ
    {
        processChamberStage (buffer);
        processCompressorStage (buffer);
        processConsoleEQStage (buffer);
    }

    // 3. Ganancia de salida
    float outGain = juce::Decibels::decibelsToGain (*apvts.getRawParameterValue ("outputGain"));
    buffer.applyGain (outGain);
}

juce::AudioProcessorEditor* StudioRoomModelerAudioProcessor::createEditor()
{
    return new StudioRoomModelerAudioProcessorEditor (*this);
}

void StudioRoomModelerAudioProcessor::getStateInformation (juce::MemoryBlock& destData)
{
    auto state = apvts.copyState();
    std::unique_ptr<juce::XmlElement> xml (state.createXml());
    copyXmlToBinary (*xml, destData);
}

void StudioRoomModelerAudioProcessor::setStateInformation (const void* data, int sizeInBytes)
{
    std::unique_ptr<juce::XmlElement> xmlState (getXmlFromBinary (data, sizeInBytes));
    if (xmlState && xmlState->hasTagName (apvts.state.getType()))
        apvts.replaceState (juce::ValueTree::fromXml (*xmlState));
}

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new StudioRoomModelerAudioProcessor();
}
