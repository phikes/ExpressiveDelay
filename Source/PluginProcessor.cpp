/*
  ==============================================================================

    This file contains the basic framework code for a JUCE plugin processor.

  ==============================================================================
*/

#include "PluginProcessor.h"
#include "PluginEditor.h"

#define MAX_DELAY_TIME 5

//==============================================================================
ExpressiveDelayAudioProcessor::ExpressiveDelayAudioProcessor()
#ifndef JucePlugin_PreferredChannelConfigurations
     : AudioProcessor (BusesProperties()
                     #if ! JucePlugin_IsMidiEffect
                      #if ! JucePlugin_IsSynth
                       .withInput  ("Input",  juce::AudioChannelSet::stereo(), true)
                      #endif
                       .withOutput ("Output", juce::AudioChannelSet::stereo(), true)
                     #endif
                       )
#endif
{
    delayLine = juce::dsp::DelayLine<float>();
    auto attributes = juce::AudioParameterFloatAttributes()
        .withStringFromValueFunction ([] (auto x, auto) { return juce::String (x * 100); })
        .withLabel ("%");
    addParameter(feedbackParameter = new juce::AudioParameterFloat (juce::ParameterID("feedback", 1), "Feedback", juce::NormalisableRange<float>(), 0.5f, attributes));
    keyboardState = new juce::MidiKeyboardState();
}

ExpressiveDelayAudioProcessor::~ExpressiveDelayAudioProcessor()
{
}

//==============================================================================
const juce::String ExpressiveDelayAudioProcessor::getName() const
{
    return JucePlugin_Name;
}

bool ExpressiveDelayAudioProcessor::acceptsMidi() const
{
   #if JucePlugin_WantsMidiInput
    return true;
   #else
    return false;
   #endif
}

bool ExpressiveDelayAudioProcessor::producesMidi() const
{
   #if JucePlugin_ProducesMidiOutput
    return true;
   #else
    return false;
   #endif
}

bool ExpressiveDelayAudioProcessor::isMidiEffect() const
{
   #if JucePlugin_IsMidiEffect
    return true;
   #else
    return false;
   #endif
}

double ExpressiveDelayAudioProcessor::getTailLengthSeconds() const
{
    return 0.0;
}

int ExpressiveDelayAudioProcessor::getNumPrograms()
{
    return 1;   // NB: some hosts don't cope very well if you tell them there are 0 programs,
                // so this should be at least 1, even if you're not really implementing programs.
}

int ExpressiveDelayAudioProcessor::getCurrentProgram()
{
    return 0;
}

void ExpressiveDelayAudioProcessor::setCurrentProgram (int index)
{
}

const juce::String ExpressiveDelayAudioProcessor::getProgramName (int index)
{
    return {};
}

void ExpressiveDelayAudioProcessor::changeProgramName (int index, const juce::String& newName)
{
}

//==============================================================================
void ExpressiveDelayAudioProcessor::prepareToPlay (double sampleRate, int samplesPerBlock)
{
    delayLine.prepare({ .sampleRate = sampleRate, .numChannels = 2, .maximumBlockSize = (juce::uint32)samplesPerBlock});
    delayLine.setMaximumDelayInSamples(MAX_DELAY_TIME * sampleRate);
    delayLine.setDelay(0.5 * sampleRate);
}

void ExpressiveDelayAudioProcessor::releaseResources()
{
    // When playback stops, you can use this as an opportunity to free up any
    // spare memory, etc.
}

#ifndef JucePlugin_PreferredChannelConfigurations
bool ExpressiveDelayAudioProcessor::isBusesLayoutSupported (const BusesLayout& layouts) const
{
  #if JucePlugin_IsMidiEffect
    juce::ignoreUnused (layouts);
    return true;
  #else
    // This is the place where you check if the layout is supported.
    // In this template code we only support mono or stereo.
    // Some plugin hosts, such as certain GarageBand versions, will only
    // load plugins that support stereo bus layouts.
    if (layouts.getMainOutputChannelSet() != juce::AudioChannelSet::mono()
     && layouts.getMainOutputChannelSet() != juce::AudioChannelSet::stereo())
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

void ExpressiveDelayAudioProcessor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midiMessages)
{
    juce::ScopedNoDenormals noDenormals;
    auto totalNumInputChannels  = getTotalNumInputChannels();
    auto totalNumOutputChannels = getTotalNumOutputChannels();
    
    keyboardState->processNextMidiBuffer(midiMessages, midiMessages.getFirstEventTime(), midiMessages.getNumEvents(), false);
    
    if (getPlayHead() != nullptr) {
        auto bpm = getPlayHead()->getPosition()->getBpm();
        if (bpm) DBG(<double>bpm);
    }

    for (auto i = totalNumInputChannels; i < totalNumOutputChannels; ++i)
        buffer.clear (i, 0, buffer.getNumSamples());
    
    if (keyboardState->isNoteOn(1, 60)) {
        for (auto channelIndex = 0; channelIndex < buffer.getNumChannels(); channelIndex++) {
            for (auto sampleIndex = 0; sampleIndex < buffer.getNumSamples(); sampleIndex++) {
                auto outSample = delayLine.popSample(channelIndex);
                delayLine.pushSample(channelIndex, buffer.getSample(channelIndex, sampleIndex) + outSample * *feedbackParameter);
                
                buffer.addSample(channelIndex, sampleIndex, outSample);
            }
        }
    }
}

//==============================================================================
bool ExpressiveDelayAudioProcessor::hasEditor() const
{
    return true; // (change this to false if you choose to not supply an editor)
}

juce::AudioProcessorEditor* ExpressiveDelayAudioProcessor::createEditor()
{
    return new ExpressiveDelayAudioProcessorEditor (*this);
}

//==============================================================================
void ExpressiveDelayAudioProcessor::getStateInformation (juce::MemoryBlock& destData)
{
    // You should use this method to store your parameters in the memory block.
    // You could do that either as raw data, or use the XML or ValueTree classes
    // as intermediaries to make it easy to save and load complex data.
}

void ExpressiveDelayAudioProcessor::setStateInformation (const void* data, int sizeInBytes)
{
    // You should use this method to restore your parameters from this memory block,
    // whose contents will have been created by the getStateInformation() call.
}

//==============================================================================
// This creates new instances of the plugin..
juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new ExpressiveDelayAudioProcessor();
}
