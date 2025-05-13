/*
  ==============================================================================

    This file contains the basic framework code for a JUCE plugin editor.

  ==============================================================================
*/

#pragma once

#include <JuceHeader.h>
#include "PluginProcessor.h"

struct CustomRotarySlider : juce::Slider
{
    CustomRotarySlider() : juce::Slider(juce::Slider::SliderStyle::RotaryHorizontalVerticalDrag,
                                          juce::Slider::TextEntryBoxPosition::NoTextBox)
    {
        
    }
};

// need to inherit from a bunch of stuff below
struct ResponseCurveComponent: juce::Component,
juce::AudioProcessorParameter::Listener,
juce::Timer
{
    ResponseCurveComponent(SimpleEQAudioProcessor&);
    ~ResponseCurveComponent();
    
    //migrate over callbacks
    void parameterValueChanged (int parameterIndex, float newValue) override;

    //dont care about this one so can give an empty implementation
    void parameterGestureChanged (int parameterIndex, bool gestureIsStarting) override {}
    
    //implement the Timer callback
    void timerCallback() override;
    
    //need a paint function so declare one of those
    void paint(juce::Graphics& g) override;
private:
    // This reference is provided as a quick way for your editor to
    // access the processor object that created it.
    SimpleEQAudioProcessor& audioProcessor;
    //add atomic flag below processor
    juce::Atomic<bool> parametersChanged { false };
    MonoChain monoChain;
};
//==============================================================================
/**
*/
// this is where all the visual elements happen
class SimpleEQAudioProcessorEditor  : public juce::AudioProcessorEditor
//inherit listener
//can't do any slow stuff like edit the GUI and trigger a repaint, but
//can set an atomic flag that the timer can check and update based on that flag
{
public:
    SimpleEQAudioProcessorEditor (SimpleEQAudioProcessor&);
    ~SimpleEQAudioProcessorEditor() override;

    //==============================================================================
    void paint (juce::Graphics&) override;
    void resized() override;
    

private:
    SimpleEQAudioProcessor& audioProcessor;
    
    
    //add some sliders
    CustomRotarySlider peakFreqSlider,
    peakGainSlider,
    peakQualitySlider,
    lowCutFreqSlider,
    highCutFreqSlider,
    lowCutSlopeSlider,
    highCutSlopeSlider;
    
    ResponseCurveComponent responseCurveComponent;
    
    //apvts has an attachment class that makes it easy to connect sliders to parameters (using typename to help with readability)
    using APVTS = juce::AudioProcessorValueTreeState;
    using Attachment = APVTS::SliderAttachment;
    
    //create 1 attachment for every one of the sliders
    Attachment peakFreqSliderAttachment,
                peakGainSliderAttachment,
                peakQualitySliderAttachment,
                lowCutFreqSliderAttachment,
                highCutFreqSliderAttachment,
                lowCutSlopeSliderAttachment,
                highCutSlopeSliderAttachment;
    
    //implementing a vector so you can iterate through them easily
    std::vector<juce::Component*> getComps();
    
    
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (SimpleEQAudioProcessorEditor)
};

