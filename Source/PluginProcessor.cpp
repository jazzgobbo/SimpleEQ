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

void SimpleEQAudioProcessor::setCurrentProgram (int index)
{
}

const juce::String SimpleEQAudioProcessor::getProgramName (int index)
{
    return {};
}

void SimpleEQAudioProcessor::changeProgramName (int index, const juce::String& newName)
{
}

//==============================================================================
void SimpleEQAudioProcessor::prepareToPlay (double sampleRate, int samplesPerBlock)
{
    // Use this method as the place to do any pre-playback
    // initialisation that you need..
}

void SimpleEQAudioProcessor::releaseResources()
{
    // When playback stops, you can use this as an opportunity to free up any
    // spare memory, etc.
}

#ifndef JucePlugin_PreferredChannelConfigurations
bool SimpleEQAudioProcessor::isBusesLayoutSupported (const BusesLayout& layouts) const
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

void SimpleEQAudioProcessor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midiMessages)
{
    juce::ScopedNoDenormals noDenormals;
    auto totalNumInputChannels  = getTotalNumInputChannels();
    auto totalNumOutputChannels = getTotalNumOutputChannels();

    // In case we have more outputs than inputs, this code clears any output
    // channels that didn't contain input data, (because these aren't
    // guaranteed to be empty - they may contain garbage).
    // This is here to avoid people getting screaming feedback
    // when they first compile a plugin, but obviously you don't need to keep
    // this code if your algorithm always overwrites all the output channels.
    for (auto i = totalNumInputChannels; i < totalNumOutputChannels; ++i)
        buffer.clear (i, 0, buffer.getNumSamples());

    // This is the place where you'd normally do the guts of your plugin's
    // audio processing...
    // Make sure to reset the state if your inner loop is processing
    // the samples and the outer loop is handling the channels.
    // Alternatively, you can process the samples with the channels
    // interleaved by keeping the same state.
    // this is where we do all the work and make sure it does it in time
    for (int channel = 0; channel < totalNumInputChannels; ++channel)
    {
        auto* channelData = buffer.getWritePointer (channel);

        // ..do something to the data...
    }
}

//==============================================================================
bool SimpleEQAudioProcessor::hasEditor() const
{
    return true; // (change this to false if you choose to not supply an editor)
}

juce::AudioProcessorEditor* SimpleEQAudioProcessor::createEditor()
{
    //return new SimpleEQAudioProcessorEditor (*this);
    return new juce::GenericAudioProcessorEditor(*this);
}

//==============================================================================
void SimpleEQAudioProcessor::getStateInformation (juce::MemoryBlock& destData)
{
    // You should use this method to store your parameters in the memory block.
    // You could do that either as raw data, or use the XML or ValueTree classes
    // as intermediaries to make it easy to save and load complex data.
}

void SimpleEQAudioProcessor::setStateInformation (const void* data, int sizeInBytes)
{
    // You should use this method to restore your parameters from this memory block,
    // whose contents will have been created by the getStateInformation() call.
}

//declaring createParameterLayout
// SPEC: 3 BANDS: LOW, HIGH, PARAMETRIC/PEAK
// Cut Bands: Controllable Frequency/ Shape
// Parametric Band: Controllable Frequency, Gain, Quality (how narrow or wide peak is)
juce::AudioProcessorValueTreeState::ParameterLayout SimpleEQAudioProcessor::createParameterLayout()
{
    // declaring a parameter layout (we want it to be a float if its a slider)
    juce::AudioProcessorValueTreeState::ParameterLayout layout;
    
    //human hearing: 20hz - 20,000hz
    //slider will change parameter value in steps of 1
    //skew factor is slider response (ie. you can skew mapping logarithmically (factor <1.0 = lower end of range will fill more of slider's length [ie more of the slider will be lower hz], >1.0 = upper end of range will be expended)
    //default value is the lowest (20hz) because we dont want to hear anything unless we move it
    
    //LOWCUT FREQUENCY (default value 20hz)
    layout.add(std::make_unique<juce::AudioParameterFloat>(//paramID
                                                           juce::ParameterID("LowCut Freq", 1),
                                                           //parameter name
                                                           "LowCut Freq",
                                                           //normalisable range
                                                           juce::NormalisableRange<float>(20.f, 20000.f, 1.f, 1.f),
                                                           // default value
                                                           20.f));
    
    //HIGHCUT FREQUENCY (default value 20000hz)
    layout.add(std::make_unique<juce::AudioParameterFloat>(//paramID
                                                           juce::ParameterID("HighCut Freq", 1),
                                                           //parameter name
                                                           "HighCut Freq",
                                                           //normalisablerange type
                                                           juce::NormalisableRange<float>(20.f, 20000.f, 1.f, 1.f),
                                                           //default value
                                                           20000.f));
    
    //PEAK FREQUENCY (default value 750z)
    layout.add(std::make_unique<juce::AudioParameterFloat>(//paramID
                                                           juce::ParameterID("Peak Freq", 1),
                                                           //parameter name
                                                           "Peak Freq",
                                                           //normalisablerange type
                                                           juce::NormalisableRange<float>(20.f, 20000.f, 1.f, 1.f),
                                                           //default value
                                                           750.f));
    
    //PEAK GAIN (expressed in decibels)
    //range [-24, 24]
    //step slider: 0.5 db
    //linear behavior so skew by 1
    //don't want to add any gain or cut so default value of 0
    layout.add(std::make_unique<juce::AudioParameterFloat>(//paramID
                                                           juce::ParameterID("Peak Gain", 1),
                                                           //parameter name
                                                           "Peak Gain",
                                                           //normalisablerange type
                                                           juce::NormalisableRange<float>(-24.f, 24.f, 1.f, 0.5f),
                                                           //default value
                                                           0.0f));
    
    //QUALITY CONTROL (how tight or how wide the peak band is)
    //Narrow Q = high Q value
    //Wide Q = low Q value
    layout.add(std::make_unique<juce::AudioParameterFloat>(//paramID
                                                           juce::ParameterID("Quality", 1),
                                                           //parameter name
                                                           "Quality",
                                                           //normalisablerange type
                                                           juce::NormalisableRange<float>(0.1f, 10.f, 0.05f, 1.f),
                                                           //default value
                                                           1.f));
    
    
    //For lowcut and highcut filters, want the ability to change the steepness of the filter cut
    //Cut filters are usually expressed in multiples of 6. For this project we are using (12, 24, 36, 48)
    //since we are expressing these in terms of choices and not a range, we can use the AudioParameterChoice object
    
    // making a string of choices
    juce::StringArray stringArray;
    for (int i=0; i<4; ++i) {
        juce::String str;
        str << (12 + 12*i);
        str << "db/Oct";
        stringArray.add(str);
    }
    
    //create audioparameter that intakes string of choices
    //uses default value 0 so the filter will have a slope of 12db/Oct
    //LOWCUT SLOPE
    layout.add(std::make_unique<juce::AudioParameterChoice>(juce::ParameterID("LowCut Slope", 1), "LowCut Slope", stringArray, 0));
    
    //HIGHCUT SLOPE
    layout.add(std::make_unique<juce::AudioParameterChoice>(juce::ParameterID("HighCut Slope", 1), "HighCut Slope", stringArray, 0));
     
    return layout;
}

//==============================================================================
// This creates new instances of the plugin..
juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new SimpleEQAudioProcessor();
}
