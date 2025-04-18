/*
  ==============================================================================

    This file contains the basic framework code for a JUCE plugin processor.

  ==============================================================================
*/

#pragma once

#include <JuceHeader.h>

// extract parameters from audio processor value tree state (create data structure to represent all values)

struct ChainSettings {
    float peakFreq {0}, peakGainInDecibels {0}, peakQuality {1.f};
    float lowCutFreq {0}, highCutFreq {0};
    int lowCutSlope {0}, highCutSlope {0};
};

// helper function that will give all parameter values in data struct
ChainSettings getChainSettings(juce::AudioProcessorValueTreeState& apvts);

//==============================================================================
/**
*/
class SimpleEQAudioProcessor  : public juce::AudioProcessor
{
public:
    //==============================================================================
    SimpleEQAudioProcessor();
    ~SimpleEQAudioProcessor() override;

    //==============================================================================
    // gets called bny the host when its about to start playback
    void prepareToPlay (double sampleRate, int samplesPerBlock) override;
    void releaseResources() override;

   #ifndef JucePlugin_PreferredChannelConfigurations
    bool isBusesLayoutSupported (const BusesLayout& layouts) const override;
   #endif
    // what happens whenever you hit the play button in the transport control -> host sends buffers at regular rate to plug in.
    // plug in's job is to give back any finished audio that is done processing (don't interrupt chain of events)
    void processBlock (juce::AudioBuffer<float>&, juce::MidiBuffer&) override;

    //==============================================================================
    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override;

    //==============================================================================
    const juce::String getName() const override;

    bool acceptsMidi() const override;
    bool producesMidi() const override;
    bool isMidiEffect() const override;
    double getTailLengthSeconds() const override;

    //==============================================================================
    int getNumPrograms() override;
    int getCurrentProgram() override;
    void setCurrentProgram (int index) override;
    const juce::String getProgramName (int index) override;
    void changeProgramName (int index, const juce::String& newName) override;

    //==============================================================================
    void getStateInformation (juce::MemoryBlock& destData) override;
    void setStateInformation (const void* data, int sizeInBytes) override;
    
    // need an APVTS and need it public so we can add knobs and everyrhing
    // create parameter layout gets called by apvts in type parameterLayout
    static juce::AudioProcessorValueTreeState::ParameterLayout createParameterLayout();
    
    juce::AudioProcessorValueTreeState apvts {*this, nullptr, "Parameters", createParameterLayout()};
    // since juce dsp library is built to process mono audio, we need to duplicate everything we do for stereo
private:
    // create a filer alias (peakfilter)
    using Filter = juce::dsp::IIR::Filter<float>;
    // since we are going between [12,24,36,48] db/Oct, we need 4 of these filters
    // for dsp in JUCE:
    // 1. define a Chain 2. pass in a Processing Context, which will run through each element of the chian automatically
    // we can put 4 of these filters in a single processor chain, which will allow us to pass a single context and allow us to process all of the audio automatically
    using CutFilter = juce::dsp::ProcessorChain<Filter, Filter, Filter, Filter>;
    // if we configure as LOWPASS/HIGHPASS filter, it will have a default 12db slope
    
    //can represent parametric filter too
    //need 2 instances of MonoChain if we want to do stereo processing
    using MonoChain = juce::dsp::ProcessorChain<CutFilter, Filter, CutFilter>;
    
    MonoChain leftChain, rightChain;
    
    //define an enum to return an index
    enum ChainPositions {
        LowCut,
        Peak,
        HighCut
    };
    
    
    //==============================================================================
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (SimpleEQAudioProcessor)
};
