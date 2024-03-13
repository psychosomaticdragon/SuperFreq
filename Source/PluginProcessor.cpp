/*
  ==============================================================================

    This file contains the basic framework code for a JUCE plugin processor.

  ==============================================================================
*/

#include "PluginProcessor.h"
#include "PluginEditor.h"

//==============================================================================
SimpleEQAudioProcessor::SimpleEQAudioProcessor()
#ifndef JucePlugin_PreferredChannelConfigurations
    : AudioProcessor(BusesProperties()
#if ! JucePlugin_IsMidiEffect
#if ! JucePlugin_IsSynth
        .withInput("Input", juce::AudioChannelSet::stereo(), true)
#endif
        .withOutput("Output", juce::AudioChannelSet::stereo(), true)
#endif
    )
#endif
{
}

SimpleEQAudioProcessor::~SimpleEQAudioProcessor()
{
}

//==============================================================================
const juce::String SimpleEQAudioProcessor::getName() const
{
    return JucePlugin_Name;
}

bool SimpleEQAudioProcessor::acceptsMidi() const
{
#if JucePlugin_WantsMidiInput
    return true;
#else
    return false;
#endif
}

bool SimpleEQAudioProcessor::producesMidi() const
{
#if JucePlugin_ProducesMidiOutput
    return true;
#else
    return false;
#endif
}

bool SimpleEQAudioProcessor::isMidiEffect() const
{
#if JucePlugin_IsMidiEffect
    return true;
#else
    return false;
#endif
}

double SimpleEQAudioProcessor::getTailLengthSeconds() const
{
    return 0.0;
}

int SimpleEQAudioProcessor::getNumPrograms()
{
    return 1;   // NB: some hosts don't cope very well if you tell them there are 0 programs,
                // so this should be at least 1, even if you're not really implementing programs.
}

int SimpleEQAudioProcessor::getCurrentProgram()
{
    return 0;
}

void SimpleEQAudioProcessor::setCurrentProgram(int index)
{
}

const juce::String SimpleEQAudioProcessor::getProgramName(int index)
{
    return {};
}

void SimpleEQAudioProcessor::changeProgramName(int index, const juce::String& newName)
{
}

//==============================================================================
void SimpleEQAudioProcessor::prepareToPlay(double sampleRate, int samplesPerBlock)
{
    // Use this method as the place to do any pre-playback
    // initialisation that you need..

    juce::dsp::ProcessSpec spec;

    spec.maximumBlockSize = samplesPerBlock;

    spec.numChannels = 1;

    spec.sampleRate = sampleRate;

    leftChain.prepare(spec);
    rightChain.prepare(spec);
    leftEQ.prepare(spec);
    rightEQ.prepare(spec);

    e1.prepare(spec);
    e2.prepare(spec);
    e3.prepare(spec);
    e4.prepare(spec);
    e5.prepare(spec);
    e6.prepare(spec);
    e7.prepare(spec);
    e8.prepare(spec);
    e9.prepare(spec);
    e10.prepare(spec);
    e11.prepare(spec);

    arm.prepare(spec);
    ar1.prepare(spec);
    ar2.prepare(spec);
    ar3.prepare(spec);
    ar4.prepare(spec);
    ar5.prepare(spec);
    ar6.prepare(spec);
    ar7.prepare(spec);
    ar8.prepare(spec);
    ar9.prepare(spec);
    ar10.prepare(spec);
    ar11.prepare(spec);

    b1.setSize(2, samplesPerBlock, false, true, true);
    b2.setSize(2, samplesPerBlock, false, true, true);
    b3.setSize(2, samplesPerBlock, false, true, true);
    b4.setSize(2, samplesPerBlock, false, true, true);
    b5.setSize(2, samplesPerBlock, false, true, true);
    b6.setSize(2, samplesPerBlock, false, true, true);
    b7.setSize(2, samplesPerBlock, false, true, true);
    b8.setSize(2, samplesPerBlock, false, true, true);
    b9.setSize(2, samplesPerBlock, false, true, true);
    b10.setSize(2, samplesPerBlock, false, true, true);
    b11.setSize(2, samplesPerBlock, false, true, true);

    updateFilters();

    leftChannelFifo.prepare(samplesPerBlock);
    rightChannelFifo.prepare(samplesPerBlock);

    osc.initialise([](float x) { return std::sin(x); });

    spec.numChannels = getTotalNumOutputChannels();
    osc.prepare(spec);
    osc.setFrequency(440);
}

void SimpleEQAudioProcessor::releaseResources()
{
    // When playback stops, you can use this as an opportunity to free up any
    // spare memory, etc.
}

#ifndef JucePlugin_PreferredChannelConfigurations
bool SimpleEQAudioProcessor::isBusesLayoutSupported(const BusesLayout& layouts) const
{
#if JucePlugin_IsMidiEffect
    juce::ignoreUnused(layouts);
    return true;
#else
    // This is the place where you check if the layout is supported.
    // In this template code we only support mono or stereo.
    // Some plugin hosts, such as certain GarageBand versions, will only
    // load plugins that support stereo bus layouts.
    if (//layouts.getMainOutputChannelSet() != juce::AudioChannelSet::stereo()
        layouts.getMainOutputChannelSet() != juce::AudioChannelSet::stereo())
        return false;

    // This checks if the input layout matches the output layout
#if ! JucePlugin_IsSynth
    if (layouts.getMainOutputChannelSet() != layouts.getMainInputChannelSet())
        return false;
#endif

    return true;
#endif
}
#endif

void SimpleEQAudioProcessor::processBlock(juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midiMessages)
{
    juce::ScopedNoDenormals noDenormals;
    auto totalNumInputChannels = getTotalNumInputChannels();
    auto totalNumOutputChannels = getTotalNumOutputChannels();

    // In case we have more outputs than inputs, this code clears any output
    // channels that didn't contain input data, (because these aren't
    // guaranteed to be empty - they may contain garbage).
    // This is here to avoid people getting screaming feedback
    // when they first compile a plugin, but obviously you don't need to keep
    // this code if your algorithm always overwrites all the output channels.
    for (auto i = totalNumInputChannels; i < totalNumOutputChannels; ++i)
    {
        buffer.clear(i, 0, buffer.getNumSamples());
        /*
        * this is how we do it.
        b1.clear();
        b1.setSize(2, buffer.getNumSamples(), false, true, true);
        b1.copyFrom(i, 0, buffer, i, 0, buffer.getNumSamples());
        */
    }

    monobuffer.setSize(2, buffer.getNumSamples(), false, true, true);
    monobuffer.clear();
    monobuffer.copyFrom(0, 0, buffer, 0, 0, buffer.getNumSamples());
    monobuffer.addFrom(1, 0, buffer, 1, 0, buffer.getNumSamples());

    b1.copyFrom(0, 0, monobuffer, 0, 0, buffer.getNumSamples());
    b2.copyFrom(0, 0, monobuffer, 0, 0, buffer.getNumSamples());
    b3.copyFrom(0, 0, monobuffer, 0, 0, buffer.getNumSamples());
    b4.copyFrom(0, 0, monobuffer, 0, 0, buffer.getNumSamples());
    b5.copyFrom(0, 0, monobuffer, 0, 0, buffer.getNumSamples());
    b6.copyFrom(0, 0, monobuffer, 0, 0, buffer.getNumSamples());
    b7.copyFrom(0, 0, monobuffer, 0, 0, buffer.getNumSamples());
    b8.copyFrom(0, 0, monobuffer, 0, 0, buffer.getNumSamples());
    b9.copyFrom(0, 0, monobuffer, 0, 0, buffer.getNumSamples());
    b10.copyFrom(0, 0, monobuffer, 0, 0, buffer.getNumSamples());
    b11.copyFrom(0, 0, monobuffer, 0, 0, buffer.getNumSamples());

    updateFilters();

    juce::dsp::AudioBlock<float> eqblock(monobuffer);

    juce::dsp::AudioBlock<float> block1(b1);
    juce::dsp::AudioBlock<float> block2(b2);
    juce::dsp::AudioBlock<float> block3(b3);
    juce::dsp::AudioBlock<float> block4(b4);
    juce::dsp::AudioBlock<float> block5(b5);
    juce::dsp::AudioBlock<float> block6(b6);
    juce::dsp::AudioBlock<float> block7(b7);
    juce::dsp::AudioBlock<float> block8(b8);
    juce::dsp::AudioBlock<float> block9(b9);
    juce::dsp::AudioBlock<float> block10(b10);
    juce::dsp::AudioBlock<float> block11(b11);

    //    buffer.clear();
    //
    //    for( int i = 0; i < buffer.getNumSamples(); ++i )
    //    {
    //        buffer.setSample(0, i, osc.processSample(0));
    //    }
    //
    //    juce::dsp::ProcessContextReplacing<float> stereoContext(block);
    //    osc.process(stereoContext);



    auto band1 = block1.getSingleChannelBlock(0);
    auto band2 = block2.getSingleChannelBlock(0);
    auto band3 = block3.getSingleChannelBlock(0);
    auto band4 = block4.getSingleChannelBlock(0);
    auto band5 = block5.getSingleChannelBlock(0);
    auto band6 = block6.getSingleChannelBlock(0);
    auto band7 = block7.getSingleChannelBlock(0);
    auto band8 = block8.getSingleChannelBlock(0);
    auto band9 = block9.getSingleChannelBlock(0);
    auto band10 = block10.getSingleChannelBlock(0);
    auto band11 = block11.getSingleChannelBlock(0);

    auto leftBlock = eqblock.getSingleChannelBlock(0);
    auto rightBlock = eqblock.getSingleChannelBlock(1);

    juce::dsp::ProcessContextReplacing<float> leftContext(leftBlock);
    juce::dsp::ProcessContextReplacing<float> rightContext(rightBlock);

    leftEQ.process(leftContext);
    rightEQ.process(rightContext);

    juce::dsp::ProcessContextReplacing<float> c1(band1);
    juce::dsp::ProcessContextReplacing<float> c2(band2);
    juce::dsp::ProcessContextReplacing<float> c3(band3);
    juce::dsp::ProcessContextReplacing<float> c4(band4);
    juce::dsp::ProcessContextReplacing<float> c5(band5);
    juce::dsp::ProcessContextReplacing<float> c6(band6);
    juce::dsp::ProcessContextReplacing<float> c7(band7);
    juce::dsp::ProcessContextReplacing<float> c8(band8);
    juce::dsp::ProcessContextReplacing<float> c9(band9);
    juce::dsp::ProcessContextReplacing<float> c10(band10);
    juce::dsp::ProcessContextReplacing<float> c11(band11);
    //juce::dsp::ProcessContextReplacing<float> rightContext(rightBlock);

    e1.process(c1);
    e1.process(c2);
    e1.process(c3);
    e1.process(c4);
    e1.process(c5);
    e1.process(c6);
    e1.process(c7);
    e1.process(c8);
    e1.process(c9);
    e1.process(c10);
    e1.process(c11);
    //e5.process(rightContext);

    /*
    float* monobufferL = monobuffer.getWritePointer(0);
    float* monobufferR = monobuffer.getWritePointer(1);

    for (int i = 0; i < buffer.getNumSamples(); ++i)
    {
        monobufferL[i] = sqrt(pow(monobufferL[i], 2) + pow(monobufferR[i], 2));
    }
    */
    buffer.copyFrom(0, 0, monobuffer, 0, 0, buffer.getNumSamples());
    buffer.copyFrom(1, 0, monobuffer, 0, 0, buffer.getNumSamples());

    leftChannelFifo.update(buffer);
    rightChannelFifo.update(buffer);

}

//==============================================================================
bool SimpleEQAudioProcessor::hasEditor() const
{
    return true; // (change this to false if you choose to not supply an editor)
}

juce::AudioProcessorEditor* SimpleEQAudioProcessor::createEditor()
{
    return new SimpleEQAudioProcessorEditor(*this);
    //    return new juce::GenericAudioProcessorEditor(*this);
}

//==============================================================================
void SimpleEQAudioProcessor::getStateInformation(juce::MemoryBlock& destData)
{
    // You should use this method to store your parameters in the memory block.
    // You could do that either as raw data, or use the XML or ValueTree classes
    // as intermediaries to make it easy to save and load complex data.

    juce::MemoryOutputStream mos(destData, true);
    apvts.state.writeToStream(mos);
}

void SimpleEQAudioProcessor::setStateInformation(const void* data, int sizeInBytes)
{
    // You should use this method to restore your parameters from this memory block,
    // whose contents will have been created by the getStateInformation() call.
    auto tree = juce::ValueTree::readFromData(data, sizeInBytes);
    if (tree.isValid())
    {
        apvts.replaceState(tree);
        updateFilters();
    }
}

ChainSettings getChainSettings(juce::AudioProcessorValueTreeState& apvts)
{
    ChainSettings settings;

    settings.lowCutFreq = apvts.getRawParameterValue("LowCut Freq")->load();
    settings.highCutFreq = apvts.getRawParameterValue("HighCut Freq")->load();
    settings.peakFreq = apvts.getRawParameterValue("Peak Freq")->load();
    settings.peakGainInDecibels = apvts.getRawParameterValue("Peak Gain")->load();
    settings.peakQuality = apvts.getRawParameterValue("Peak Quality")->load();
    settings.lowCutSlope = static_cast<Slope>(apvts.getRawParameterValue("LowCut Slope")->load());
    settings.highCutSlope = static_cast<Slope>(apvts.getRawParameterValue("HighCut Slope")->load());

    settings.lowCutBypassed = apvts.getRawParameterValue("LowCut Bypassed")->load() > 0.5f;
    settings.peakBypassed = apvts.getRawParameterValue("Peak Bypassed")->load() > 0.5f;
    settings.highCutBypassed = apvts.getRawParameterValue("HighCut Bypassed")->load() > 0.5f;

    return settings;
}

Coefficients makePeakFilter2(float peakFreq, float peakGain, float peakQ, double sampleRate)
{
    return juce::dsp::IIR::Coefficients<float>::makePeakFilter(sampleRate,
        peakFreq,
        peakQ,
        peakGain);
}

Coefficients makePeakFilter(const ChainSettings& chainSettings, double sampleRate)
{
    return juce::dsp::IIR::Coefficients<float>::makePeakFilter(sampleRate,
        chainSettings.peakFreq,
        chainSettings.peakQuality,
        juce::Decibels::decibelsToGain(chainSettings.peakGainInDecibels));
}

void SimpleEQAudioProcessor::setEQ(const ChainSettings& chainSettings)
{
    // we slam the peaks in the logarithmic center of each band, i.e.
    // exp( (ln(lowcut)+ln(highcut))/2 )
    auto sr = getSampleRate();
    auto q = chainSettings.peakQuality; // set to 9.090909
    auto gain = juce::Decibels::decibelsToGain(chainSettings.peakGainInDecibels);

    auto c1 = makePeakFilter2(58.f, gain, 0.444, sr);
    auto c2 = makePeakFilter2(206.f, gain, 1.170, sr);
    auto c3 = makePeakFilter2(423.f, gain, 1.608, sr);
    auto c4 = makePeakFilter2(746.f, gain, 1.912, sr);
    auto c5 = makePeakFilter2(1230.f, gain, 2.079, sr);
    auto c6 = makePeakFilter2(1955.f, gain, 2.203, sr);
    auto c7 = makePeakFilter2(3042.f, gain, 2.283, sr);
    auto c8 = makePeakFilter2(4672.f, gain, 2.342, sr);
    auto c9 = makePeakFilter2(7117.f, gain, 2.375, sr);
    auto c10 = makePeakFilter2(10785.f, gain, 2.398, sr);
    auto c11 = makePeakFilter2(16286.f, gain, 2.415, sr);

    updateCoefficients(leftEQ.get<0>().coefficients, c1);
    updateCoefficients(rightEQ.get<0>().coefficients, c1);

    updateCoefficients(leftEQ.get<1>().coefficients, c2);
    updateCoefficients(rightEQ.get<1>().coefficients, c2);

    updateCoefficients(leftEQ.get<2>().coefficients, c3);
    updateCoefficients(rightEQ.get<2>().coefficients, c3);

    updateCoefficients(leftEQ.get<3>().coefficients, c4);
    updateCoefficients(rightEQ.get<3>().coefficients, c4);

    updateCoefficients(leftEQ.get<4>().coefficients, c5);
    updateCoefficients(rightEQ.get<4>().coefficients, c5);

    updateCoefficients(leftEQ.get<5>().coefficients, c6);
    updateCoefficients(rightEQ.get<5>().coefficients, c6);

    updateCoefficients(leftEQ.get<6>().coefficients, c7);
    updateCoefficients(rightEQ.get<6>().coefficients, c7);

    updateCoefficients(leftEQ.get<7>().coefficients, c8);
    updateCoefficients(rightEQ.get<7>().coefficients, c8);

    updateCoefficients(leftEQ.get<8>().coefficients, c9);
    updateCoefficients(rightEQ.get<8>().coefficients, c9);

    updateCoefficients(leftEQ.get<9>().coefficients, c10);
    updateCoefficients(rightEQ.get<9>().coefficients, c10);

    updateCoefficients(leftEQ.get<10>().coefficients, c11);
    updateCoefficients(rightEQ.get<10>().coefficients, c11);
}

void SimpleEQAudioProcessor::updatePeakFilter(const ChainSettings& chainSettings)
{
    auto peakCoefficients = makePeakFilter(chainSettings, getSampleRate());

    leftChain.setBypassed<ChainPositions::Peak>(chainSettings.peakBypassed);
    rightChain.setBypassed<ChainPositions::Peak>(chainSettings.peakBypassed);

    updateCoefficients(leftChain.get<ChainPositions::Peak>().coefficients, peakCoefficients);
    updateCoefficients(rightChain.get<ChainPositions::Peak>().coefficients, peakCoefficients);
}

void updateCoefficients(Coefficients& old, const Coefficients& replacements)
{
    *old = *replacements;
}


void SimpleEQAudioProcessor::setFilterBallistics(const ChainSettings& chainSettings)
{
    auto rms = juce::dsp::BallisticsFilterLevelCalculationType(1);
    auto bal1 = ar1.get<0>();
    bal1.setLevelCalculationType(rms);
    bal1.setAttackTime(35.f);
    bal1.setReleaseTime(35.f);

    auto bal2 = ar2.get<0>();
    bal2.setLevelCalculationType(rms);
    bal2.setAttackTime(35.f);
    bal2.setReleaseTime(35.f);

    auto bal3 = ar3.get<0>();
    bal3.setLevelCalculationType(rms);
    bal3.setAttackTime(35.f);
    bal3.setReleaseTime(35.f);

    auto bal4 = ar4.get<0>();
    bal4.setLevelCalculationType(rms);
    bal4.setAttackTime(35.f);
    bal4.setReleaseTime(35.f);

    auto bal5 = ar5.get<0>();
    bal5.setLevelCalculationType(rms);
    bal5.setAttackTime(35.f);
    bal5.setReleaseTime(35.f);

    auto bal6 = ar6.get<0>();
    bal6.setLevelCalculationType(rms);
    bal6.setAttackTime(35.f);
    bal6.setReleaseTime(35.f);

    auto bal7 = ar7.get<0>();
    bal7.setLevelCalculationType(rms);
    bal7.setAttackTime(35.f);
    bal7.setReleaseTime(35.f);

    auto bal8 = ar8.get<0>();
    bal8.setLevelCalculationType(rms);
    bal8.setAttackTime(35.f);
    bal8.setReleaseTime(35.f);

    auto bal9 = ar9.get<0>();
    bal9.setLevelCalculationType(rms);
    bal9.setAttackTime(35.f);
    bal9.setReleaseTime(35.f);

    auto bal10 = ar10.get<0>();
    bal10.setLevelCalculationType(rms);
    bal10.setAttackTime(35.f);
    bal10.setReleaseTime(35.f);

    auto bal11 = ar11.get<0>();
    bal11.setLevelCalculationType(rms);
    bal11.setAttackTime(35.f);
    bal11.setReleaseTime(35.f);
}

void SimpleEQAudioProcessor::setFilterBank(const ChainSettings& chainSettings)
{
    // bands are calculated by integrating (20 + x*1.5^n0... x*1.5^n-1) = 20,000
    // and solving for x. Desmos is your friend, because this is a curve between linear and
    // logarithmic
    auto& loc1 = e1.get<0>();
    auto& hic1 = e1.get<1>();
    loc1.setType(juce::dsp::LinkwitzRileyFilterType(1));
    loc1.setCutoffFrequency(20.f);
    hic1.setType(juce::dsp::LinkwitzRileyFilterType(0));
    hic1.setCutoffFrequency(137.f);

    auto& loc2 = e2.get<0>();
    auto& hic2 = e2.get<1>();
    loc2.setType(juce::dsp::LinkwitzRileyFilterType(1));
    loc2.setCutoffFrequency(137.f);
    hic2.setType(juce::dsp::LinkwitzRileyFilterType(0));
    hic2.setCutoffFrequency(312.f);

    auto& loc3 = e3.get<0>();
    auto& hic3 = e3.get<1>();
    loc3.setType(juce::dsp::LinkwitzRileyFilterType(1));
    loc3.setCutoffFrequency(312.f);
    hic3.setType(juce::dsp::LinkwitzRileyFilterType(0));
    hic3.setCutoffFrequency(575.f);

    auto& loc4 = e4.get<0>();
    auto& hic4 = e4.get<1>();
    loc4.setType(juce::dsp::LinkwitzRileyFilterType(1));
    loc4.setCutoffFrequency(575.f);
    hic4.setType(juce::dsp::LinkwitzRileyFilterType(0));
    hic4.setCutoffFrequency(969.f);

    auto& loc5 = e5.get<0>();
    auto& hic5 = e5.get<1>();
    loc5.setType(juce::dsp::LinkwitzRileyFilterType(1));
    loc5.setCutoffFrequency(969.f);
    hic5.setType(juce::dsp::LinkwitzRileyFilterType(0));
    hic5.setCutoffFrequency(1561.f);

    auto& loc6 = e6.get<0>();
    auto& hic6 = e6.get<1>();
    loc6.setType(juce::dsp::LinkwitzRileyFilterType(1));
    loc6.setCutoffFrequency(1561.f);
    hic6.setType(juce::dsp::LinkwitzRileyFilterType(0));
    hic6.setCutoffFrequency(2448.f);

    auto& loc7 = e7.get<0>();
    auto& hic7 = e7.get<1>();
    loc7.setType(juce::dsp::LinkwitzRileyFilterType(1));
    loc7.setCutoffFrequency(2448.f);
    hic7.setType(juce::dsp::LinkwitzRileyFilterType(0));
    hic7.setCutoffFrequency(3779.f);

    auto& loc8 = e8.get<0>();
    auto& hic8 = e8.get<1>();
    loc8.setType(juce::dsp::LinkwitzRileyFilterType(1));
    loc8.setCutoffFrequency(3779.f);
    hic8.setType(juce::dsp::LinkwitzRileyFilterType(0));
    hic8.setCutoffFrequency(5776.f);

    auto& loc9 = e9.get<0>();
    auto& hic9 = e9.get<1>();
    loc9.setType(juce::dsp::LinkwitzRileyFilterType(1));
    loc9.setCutoffFrequency(5776.f);
    hic9.setType(juce::dsp::LinkwitzRileyFilterType(0));
    hic9.setCutoffFrequency(8770.f);

    auto& loc10 = e10.get<0>();
    auto& hic10 = e10.get<1>();
    loc10.setType(juce::dsp::LinkwitzRileyFilterType(1));
    loc10.setCutoffFrequency(8770.f);
    hic10.setType(juce::dsp::LinkwitzRileyFilterType(0));
    hic10.setCutoffFrequency(13262.f);

    auto& loc11 = e11.get<0>();
    auto& hic11 = e11.get<1>();
    loc11.setType(juce::dsp::LinkwitzRileyFilterType(1));
    loc11.setCutoffFrequency(13262.f);
    hic11.setType(juce::dsp::LinkwitzRileyFilterType(0));
    hic11.setCutoffFrequency(20000.f);
}

void SimpleEQAudioProcessor::updateLowCutFilters(const ChainSettings& chainSettings)
{
    auto cutCoefficients = makeLowCutFilter(chainSettings, getSampleRate());
    auto& leftLowCut = leftChain.get<ChainPositions::LowCut>();
    auto& rightLowCut = rightChain.get<ChainPositions::LowCut>();

    leftChain.setBypassed<ChainPositions::LowCut>(chainSettings.lowCutBypassed);
    rightChain.setBypassed<ChainPositions::LowCut>(chainSettings.lowCutBypassed);

    updateCutFilter(rightLowCut, cutCoefficients, chainSettings.lowCutSlope);
    updateCutFilter(leftLowCut, cutCoefficients, chainSettings.lowCutSlope);
}

void SimpleEQAudioProcessor::updateHighCutFilters(const ChainSettings& chainSettings)
{
    auto highCutCoefficients = makeHighCutFilter(chainSettings, getSampleRate());

    auto& leftHighCut = leftChain.get<ChainPositions::HighCut>();
    auto& rightHighCut = rightChain.get<ChainPositions::HighCut>();

    leftChain.setBypassed<ChainPositions::HighCut>(chainSettings.highCutBypassed);
    rightChain.setBypassed<ChainPositions::HighCut>(chainSettings.highCutBypassed);

    updateCutFilter(leftHighCut, highCutCoefficients, chainSettings.highCutSlope);
    updateCutFilter(rightHighCut, highCutCoefficients, chainSettings.highCutSlope);
}

void SimpleEQAudioProcessor::updateFilters()
{
    auto chainSettings = getChainSettings(apvts);

    updateLowCutFilters(chainSettings);
    updatePeakFilter(chainSettings);
    updateHighCutFilters(chainSettings);
    setFilterBank(chainSettings);
    setFilterBallistics(chainSettings);
    setEQ(chainSettings);
}

juce::AudioProcessorValueTreeState::ParameterLayout SimpleEQAudioProcessor::createParameterLayout()
{
    juce::AudioProcessorValueTreeState::ParameterLayout layout;

    layout.add(std::make_unique<juce::AudioParameterFloat>("LowCut Freq",
        "LowCut Freq",
        juce::NormalisableRange<float>(20.f, 20000.f, 1.f, 0.25f),
        20.f));

    layout.add(std::make_unique<juce::AudioParameterFloat>("HighCut Freq",
        "HighCut Freq",
        juce::NormalisableRange<float>(20.f, 20000.f, 1.f, 0.25f),
        20000.f));

    layout.add(std::make_unique<juce::AudioParameterFloat>("Peak Freq",
        "Peak Freq",
        juce::NormalisableRange<float>(20.f, 20000.f, 1.f, 0.25f),
        750.f));

    layout.add(std::make_unique<juce::AudioParameterFloat>("Peak Gain",
        "Peak Gain",
        juce::NormalisableRange<float>(-24.f, 24.f, 0.5f, 1.f),
        0.0f));

    layout.add(std::make_unique<juce::AudioParameterFloat>("Peak Quality",
        "Peak Quality",
        juce::NormalisableRange<float>(0.1f, 10.f, 0.05f, 1.f),
        1.f));

    juce::StringArray stringArray;
    for (int i = 0; i < 4; ++i)
    {
        juce::String str;
        str << (12 + i * 12);
        str << " db/Oct";
        stringArray.add(str);
    }

    layout.add(std::make_unique<juce::AudioParameterChoice>("LowCut Slope", "LowCut Slope", stringArray, 0));
    layout.add(std::make_unique<juce::AudioParameterChoice>("HighCut Slope", "HighCut Slope", stringArray, 0));

    layout.add(std::make_unique<juce::AudioParameterBool>("LowCut Bypassed", "LowCut Bypassed", false));
    layout.add(std::make_unique<juce::AudioParameterBool>("Peak Bypassed", "Peak Bypassed", false));
    layout.add(std::make_unique<juce::AudioParameterBool>("HighCut Bypassed", "HighCut Bypassed", false));
    layout.add(std::make_unique<juce::AudioParameterBool>("Analyzer Enabled", "Analyzer Enabled", true));

    return layout;
}

//==============================================================================
// This creates new instances of the plugin..
juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new SimpleEQAudioProcessor();
}