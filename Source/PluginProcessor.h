/*
  ==============================================================================

    This file contains the basic framework code for a JUCE plugin processor.

  ==============================================================================
*/

#pragma once

#include <JuceHeader.h>

//cant use numbers to begin identifiers in c++ so have to put Slope before that
enum Slope {
    Slope_12,
    Slope_24,
    Slope_36,
    Slope_48
};

// extract parameters from audio processor value tree state (create data structure to represent all values)
struct ChainSettings {
    float peakFreq {0}, peakGainInDecibels {0}, peakQuality {1.f};
    float lowCutFreq {0}, highCutFreq {0};
    //change what slope is expressed as
    //int lowCutSlope {0}, highCutSlope {0};
    Slope lowCutSlope{Slope::Slope_12}, highCutSlope{Slope::Slope_12};
};

// helper function that will give all parameter values in data struct
ChainSettings getChainSettings(juce::AudioProcessorValueTreeState& apvts);
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

//define an enum to return an index
enum ChainPositions {
    LowCut,
    Peak,
    HighCut
};

using Coefficients = Filter::CoefficientsPtr;
//& because it allows you to modify the original object, const because you cant make changes to the replacement
void updateCoefficients(Coefficients& old, const Coefficients& replacements);

//makes a peak filter from chain settings and sample rate
Coefficients makePeakFilter(const ChainSettings& chainSettings, double sampleRate);

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
    //moved enum to public
    MonoChain leftChain, rightChain;
    
    //cleaning up stuff that configures peak filter
    void updatePeakFilter(const ChainSettings& chainSettings);
    
    
    template <int Index, typename ChainType, typename CoefficientType>
    void update(ChainType& chain, const CoefficientType& coefficients) {
        //update the coefficients
        updateCoefficients(chain.template get<Index>().coefficients, coefficients[Index]);
        //set bypass to false
        chain.template setBypassed<Index>(false);
        
        
        
    }
    
    template<typename ChainType, typename CoefficientType>
    void updateCutFilter(ChainType& chain,
                         const CoefficientType& coefficients,
                         const Slope& slope)
                         //const ChainSettings& chainSettings)
    {
        //cut coefficients using helper function
        //for order we have to do lowCutSlope + 1 * 2, since adding 1 and doubling will give us an order of 2,4,6, or 8
        //std::cout << juce::String("sampleRate :") << juce::String(getSampleRate()) << std::endl;
        //auto cutCoefficients = juce::dsp::FilterDesign<float>::designIIRHighpassHighOrderButterworthMethod(chainSettings.lowCutFreq,
        //                                                                                                   getSampleRate(),
        //                                                                                                   2 * (chainSettings.lowCutSlope + 1));
        
        
        //initialize left chain
        //auto& leftLowCut = leftChain.get<ChainPositions::LowCut>();
        
        // (bypass all links in left chain) there are 4 positions
        chain.template setBypassed<0>(true);
        chain.template setBypassed<1>(true);
        chain.template setBypassed<2>(true);
        chain.template setBypassed<3>(true);
        
        //cleaning up switch statement below
        
        //use enum to display slope setting since enums decay to integers (which is what our choice parameter is expressed in)
        switch (slope){
            case Slope_48: {
                update<3>(chain, coefficients);
                //chain.template setBypassed<3>(false);
                //leftLowCut.template setBypassed<3>(false);
            }
            case Slope_36: {
                update<2>(chain, coefficients);
                //chain.template setBypassed<2>(false);
                //leftLowCut.template setBypassed<2>(false);
            }
            case Slope_24: {
                update<1>(chain, coefficients);
                //chain.template setBypassed<1>(false);
                //leftLowCut.template setBypassed<1>(false);
            }
            case Slope_12: {
                update<0>(chain, coefficients);
                //chain.template setBypassed<0>(false);
                //leftLowCut.template setBypassed<0>(false);
            }
                
                
                /*
                
                //if order is 2 = 12bc/oct slope, the helper function will return an array with 1 coefficient object only
                //assign coefficients to first filter in cut filter chain and also stop bypassing that filter chain
            case Slope_12:
            {
                //dereference left
                *leftLowCut.template get<0>().coefficients = *cutCoefficients[0];
                //stop bypassing particular left chain
                leftLowCut.template setBypassed<0>(false);
                break;
            }
                //now will assign to the first 2 links in the filter chain and stop by passing them
            case Slope_24:
            {
                *leftLowCut.template get<0>().coefficients = *cutCoefficients[0];
                leftLowCut.template setBypassed<0>(false);
                *leftLowCut.template get<1>().coefficients = *cutCoefficients[1];
                leftLowCut.template setBypassed<1>(false);
                break;
            }
                //now will assign to the first 3 links in the filter chain and stop by passing them
            case Slope_36:
            {
                *leftLowCut.template get<0>().coefficients = *cutCoefficients[0];
                leftLowCut.template setBypassed<0>(false);
                *leftLowCut.template get<1>().coefficients = *cutCoefficients[1];
                leftLowCut.template setBypassed<1>(false);
                *leftLowCut.template get<2>().coefficients = *cutCoefficients[2];
                leftLowCut.template setBypassed<2>(false);
                break;
            }
                //now will assign to the first 4 links in the filter chain and stop by passing them
            case Slope_48:
            {
                *leftLowCut.template get<0>().coefficients = *cutCoefficients[0];
                leftLowCut.template setBypassed<0>(false);
                *leftLowCut.template get<1>().coefficients = *cutCoefficients[1];
                leftLowCut.template setBypassed<1>(false);
                *leftLowCut.template get<2>().coefficients = *cutCoefficients[2];
                leftLowCut.template setBypassed<2>(false);
                *leftLowCut.template get<3>().coefficients = *cutCoefficients[3];
                leftLowCut.template setBypassed<3>(false);
                break;
            }
                 */
        }
                 
        
        
    }
    void updateLowCutFilters (const ChainSettings& chainSettings);
    void updateHighCutFilters (const ChainSettings& chainSettings);
    void updateFilters();
    
    //==============================================================================
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (SimpleEQAudioProcessor)
};
