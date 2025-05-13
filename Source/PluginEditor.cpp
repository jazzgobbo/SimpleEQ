/*
  ==============================================================================

    This file contains the basic framework code for a JUCE plugin editor.

  ==============================================================================
*/

#include "PluginProcessor.h"
#include "PluginEditor.h"

//==============================================================================
SimpleEQAudioProcessorEditor::SimpleEQAudioProcessorEditor (SimpleEQAudioProcessor& p)
    : AudioProcessorEditor (&p), audioProcessor (p),
peakFreqSliderAttachment(audioProcessor.apvts, "Peak Freq", peakFreqSlider),
peakGainSliderAttachment(audioProcessor.apvts, "Peak Gain", peakGainSlider),
peakQualitySliderAttachment(audioProcessor.apvts, "Peak Quality", peakQualitySlider),
lowCutFreqSliderAttachment(audioProcessor.apvts, "LowCut Freq", lowCutFreqSlider),
highCutFreqSliderAttachment(audioProcessor.apvts, "HighCut Freq", highCutFreqSlider),
lowCutSlopeSliderAttachment(audioProcessor.apvts, "LowCut Slope", lowCutSlopeSlider),
highCutSlopeSliderAttachment(audioProcessor.apvts, "HighCut Slope", highCutSlopeSlider)
{
    // Make sure that before the constructor has finished, you've set the
    // editor's size to whatever you need it to be.
    for (auto* comp : getComps()) {
        addAndMakeVisible(comp);
    }
    
    //now that we can update any peak filter link with the chain settings, we need to listen for when the parameters actually change
    // grab all parameters from audio processor and add ourself as a listener
    const auto& params = audioProcessor.getParameters();
    //loop through them
    for ( auto param : params )
    {
        //getParameters function returns an array of pointers
        param->addListener(this);
    }
    
    //start with a 60hz refresh rate
    startTimerHz(60);

    setSize (600, 400);
}

SimpleEQAudioProcessorEditor::~SimpleEQAudioProcessorEditor()
{
    //if we register as a listener, we need to deregister as a listener
    const auto& params = audioProcessor.getParameters();
    for ( auto param : params )
    {
        param->removeListener(this);
    }
}

//==============================================================================
void SimpleEQAudioProcessorEditor::paint (juce::Graphics& g)
{
    //so we don't have to keep typing juce::..
    using namespace juce;
    
    // PREVIOUS CODE
    // (Our component is opaque, so we must completely fill the background with a solid colour)
    // g.fillAll (getLookAndFeel().findColour (juce::ResizableWindow::backgroundColourId));
    // g.setColour (juce::Colours::white);
    // g.setFont (juce::FontOptions (15.0f));
    // g.drawFittedText ("Hello World!", getLocalBounds(), juce::Justification::centred, 1);
    
    g.fillAll (Colours::black);
    auto bounds = getLocalBounds();
    // need response area
    auto responseArea = bounds.removeFromTop(bounds.getHeight() * 0.33);
    // and width
    auto w = responseArea.getWidth();
    
    // need chain elements
    auto& lowcut = monoChain.get<ChainPositions::LowCut>();
    auto& highcut = monoChain.get<ChainPositions::HighCut>();
    auto& peak = monoChain.get<ChainPositions::Peak>();
    
    //call get magnitude for frequency function for each element in the chain
    auto sampleRate = audioProcessor.getSampleRate();
    // need a place to store all the magnitudes (doubles)
    std::vector<double> mags;
    // 1 magnitude per pixel so create space we need
    mags.resize(w);
    // iterate thorugh each pixel and compute magnitude at that frequency
    for (int i = 0; i < w; ++i )
    {
        // magnitude expressed as gain units (multiplicatibe) so start with 1
        double mag = 1.f;
        // call magnitude function for a particular pixed mapped from pixel space to frequency space
        auto freq = mapToLog10(double(i) / double(w), 20.0, 20000.0);
        
        // check if a band is bypassed, if it is then we won't do this function
        if (! monoChain.isBypassed<ChainPositions::Peak>())
            mag *= peak.coefficients-> getMagnitudeForFrequency(freq, sampleRate);
        // do the same thing for every other fiklter in the low and highcut chains
        
        //lowcut is defined as its own processor chain, so we need to get each individual element of it
        if (! lowcut.isBypassed<0>())
            mag *= lowcut.get<0>().coefficients-> getMagnitudeForFrequency(freq, sampleRate);
        if (! lowcut.isBypassed<1>())
            mag *= lowcut.get<1>().coefficients-> getMagnitudeForFrequency(freq, sampleRate);
        if (! lowcut.isBypassed<2>())
            mag *= lowcut.get<2>().coefficients-> getMagnitudeForFrequency(freq, sampleRate);
        if (! lowcut.isBypassed<3>())
            mag *= lowcut.get<3>().coefficients-> getMagnitudeForFrequency(freq, sampleRate);
        
        if (! highcut.isBypassed<0>())
            mag *= highcut.get<0>().coefficients-> getMagnitudeForFrequency(freq, sampleRate);
        if (! highcut.isBypassed<1>())
            mag *= highcut.get<1>().coefficients-> getMagnitudeForFrequency(freq, sampleRate);
        if (! highcut.isBypassed<2>())
            mag *= highcut.get<2>().coefficients-> getMagnitudeForFrequency(freq, sampleRate);
        if (! highcut.isBypassed<3>())
            mag *= highcut.get<3>().coefficients-> getMagnitudeForFrequency(freq, sampleRate);
        
        //convert magnitude into decibels and store it
        mags[i] = Decibels::gainToDecibels(mag);
    }
    //build path from the vecot of decibels
    Path responseCurve;
    // map decibel value to response area
    // define max and minimum positions in the window
    const double outputMin = responseArea.getBottom();
    const double outputMax = responseArea.getY();
    auto map = [outputMin, outputMax](double input)
    {
        // 24s because becasue peak control goes from -24 to +24, so the window should be this range
        return jmap(input, -24.0, 24.0, outputMin, outputMax);
    };
    
    //left edge of the component, first value will be map(mags.front())
    responseCurve.startNewSubPath(responseArea.getX(), map(mags.front()));
    
    // create line
    for ( size_t i = 1; i < mags.size(); ++i )
    {
        responseCurve.lineTo(responseArea.getX() + i, map(mags[i]));
    }
    
    // draw background
    g.setColour(Colours::orange);
    g.drawRoundedRectangle(responseArea.toFloat(), 4.f, 1.f); //line thickness is 1
    // draw path
    g.setColour(Colours::white);
    g.strokePath(responseCurve, PathStrokeType(2.f));
    
    
    
    

    
}

void SimpleEQAudioProcessorEditor::resized()
{
    // This is generally where you'll want to lay out the positions of any
    // subcomponents in your editor..
    // top 1/3 reserved for frequency response
    // bottom 2/3 reserved for sliders
    auto bounds = getLocalBounds();
    //chopping off 33% of height for response area
    auto responseArea = bounds.removeFromTop(bounds.getHeight() * 0.33);
    
    // putting lowcut area on the left (chopping of 33% of the bounding box)
    auto lowCutArea = bounds.removeFromLeft(bounds.getWidth() * 0.33);
    //since we chopped off 33%, we are left with 66% of bounds. Take (50%) of bounds for high cut area (right)
    auto highCutArea = bounds.removeFromRight(bounds.getWidth() * 0.5);
    
    lowCutFreqSlider.setBounds(lowCutArea.removeFromTop(lowCutArea.getHeight()* 0.5));
    lowCutSlopeSlider.setBounds(lowCutArea);
    highCutFreqSlider.setBounds(highCutArea.removeFromTop(highCutArea.getHeight() * 0.5));
    highCutSlopeSlider.setBounds(highCutArea);
    
    //set peak slider to middle
    peakFreqSlider.setBounds(bounds.removeFromTop(bounds.getHeight() * 0.33));
    peakGainSlider.setBounds(bounds.removeFromTop(bounds.getHeight()* 0.5));
    peakQualitySlider.setBounds(bounds);
    
}

//set up parameterValue changed callback
void SimpleEQAudioProcessorEditor::parameterValueChanged(int parameterIndex, float newValue) {
    //set atomic flag to true
    parametersChanged.set(true);
}

//set up timer callback
void SimpleEQAudioProcessorEditor::timerCallback()
{
    //see if it is true, if it is we want to set it back to false
    if (parametersChanged.compareAndSetBool(false, true))
    {
        //update monochain
        //grab chain settings
        auto chainSettings = getChainSettings(audioProcessor.apvts);
        //make coefficients for peak band
        auto peakCoefficients = makePeakFilter(chainSettings, audioProcessor.getSampleRate());
        //update our old (peak from monochain) coefficients with new replacements (peakCoefficients)
        updateCoefficients(monoChain.get<ChainPositions::Peak>().coefficients, peakCoefficients);
        
        auto lowCutCoefficients = makeLowCutFilter(chainSettings,audioProcessor.getSampleRate());
        auto highCutCoefficients = makeHighCutFilter(chainSettings, audioProcessor.getSampleRate());
        // chain is the monochain. replace with low cut coefficients, and low cut slope
        updateCutFilter(monoChain.get<ChainPositions::LowCut>(), lowCutCoefficients, chainSettings.lowCutSlope);
        updateCutFilter(monoChain.get<ChainPositions::HighCut>(), highCutCoefficients, chainSettings.highCutSlope);
        
        
        //trigger a repaint
        repaint();
    };
}

std::vector<juce::Component*> SimpleEQAudioProcessorEditor::getComps()
{
    return
    {
        &peakFreqSlider,
        &peakGainSlider,
        &peakQualitySlider,
        &lowCutFreqSlider,
        &highCutFreqSlider,
        &lowCutSlopeSlider,
        &highCutSlopeSlider
        
    };
    
};
