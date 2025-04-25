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
    // prepare filters before we use them by passing a process spec object to the chain, which will pass it to each link in the chain
    juce::dsp::ProcessSpec spec;
    //needs to know the max number of samples that will pass through
    spec.maximumBlockSize = samplesPerBlock;
    //mono chains can only handle one channel
    spec.numChannels = 1;
    //needs to know the sample rate
    spec.sampleRate = sampleRate;
    leftChain.prepare(spec);
    rightChain.prepare(spec);
    
    updateFilters();
    
    

};
    


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

// the processor chain requires a processing context to be passed to it in order to run the audio through the links in the chain
// in order to make a processing context, we need to supply it with an audio block instance (juce::AudioBuffer<>)
// need to extract the left and right channel from this bufer (typically channels 0 and 1)

void SimpleEQAudioProcessor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midiMessages)
{
    //std::cout << "entering process block" << std::endl;
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
    
    //helper functions using IIR class
   // auto chainSettings = getChainSettings(apvts);
    //IIR:Coefficients object is a reference counted wrapped around an array
    //we want to copy values over so we want to dereference it
    
    //made the following refactoring functions
    updateFilters();
//    updatePeakFilter(chainSettings);
//    
//    
//    
//    ///auto peakCoefficients = juce::dsp::IIR::Coefficients<float>::makePeakFilter(getSampleRate(),
//    ///                                                                            chainSettings.peakFreq,
//    ///                                                                            chainSettings.peakQuality,
//    ///                                                                           juce::Decibels::decibelsToGain(chainSettings.peakGainInDecibels));
//    
//    ///access coefficients using .coefficients and assign what we wrote above
//    ///dereference them using * on both sides
//    ///at this point the peak has been set up and will make audible changes to audio running through it if the gain parameter is not 0
//    ///whenever the slider changes we need to update the filter with new coefficients whenever the slider changes (do it next in process block)
//    ///*leftChain.get<ChainPositions::Peak>().coefficients = *peakCoefficients;
//    ///*rightChain.get<ChainPositions::Peak>().coefficients = *peakCoefficients;
//    
////    //cut coefficients using helper function
////    //for order we have to do lowCutSlope + 1 * 2, since adding 1 and doubling will give us an order of 2,4,6, or 8
////    //std::cout << juce::String("sampleRate :") << juce::String(getSampleRate()) << std::endl;
//     auto lowCutCoefficients = juce::dsp::FilterDesign<float>::designIIRHighpassHighOrderButterworthMethod(chainSettings.lowCutFreq,
//                                                                                                       getSampleRate(),
//                                                                                                       2 * (chainSettings.lowCutSlope + 1));
//     //initialize left chain
//     auto& leftLowCut = leftChain.get<ChainPositions::LowCut>();
//     updateCutFilter(leftLowCut, lowCutCoefficients, chainSettings.lowCutSlope);
////
////    // (bypass all links in left chain) there are 4 positions
////    leftLowCut.setBypassed<0>(true);
////    leftLowCut.setBypassed<1>(true);
////    leftLowCut.setBypassed<2>(true);
////    leftLowCut.setBypassed<3>(true);
////    
////    //use enum to display slope setting since enums decay to integers (which is what our choice parameter is expressed in)
////    switch (chainSettings.lowCutSlope){
////            
////            //if order is 2 = 12bc/oct slope, the helper function will return an array with 1 coefficient object only
////            //assign coefficients to first filter in cut filter chain and also stop bypassing that filter chain
////        case Slope_12:
////        {
////            //dereference left
////            *leftLowCut.get<0>().coefficients = *cutCoefficients[0];
////            //stop bypassing particular left chain
////            leftLowCut.setBypassed<0>(false);
////            break;
////        }
////            //now will assign to the first 2 links in the filter chain and stop by passing them
////        case Slope_24:
////        {
////            *leftLowCut.get<0>().coefficients = *cutCoefficients[0];
////            leftLowCut.setBypassed<0>(false);
////            *leftLowCut.get<1>().coefficients = *cutCoefficients[1];
////            leftLowCut.setBypassed<1>(false);
////            break;
////        }
////            //now will assign to the first 3 links in the filter chain and stop by passing them
////        case Slope_36:
////        {
////            *leftLowCut.get<0>().coefficients = *cutCoefficients[0];
////            leftLowCut.setBypassed<0>(false);
////            *leftLowCut.get<1>().coefficients = *cutCoefficients[1];
////            leftLowCut.setBypassed<1>(false);
////            *leftLowCut.get<2>().coefficients = *cutCoefficients[2];
////            leftLowCut.setBypassed<2>(false);
////            break;
////        }
////            //now will assign to the first 4 links in the filter chain and stop by passing them
////        case Slope_48:
////        {
////            *leftLowCut.get<0>().coefficients = *cutCoefficients[0];
////            leftLowCut.setBypassed<0>(false);
////            *leftLowCut.get<1>().coefficients = *cutCoefficients[1];
////            leftLowCut.setBypassed<1>(false);
////            *leftLowCut.get<2>().coefficients = *cutCoefficients[2];
////            leftLowCut.setBypassed<2>(false);
////            *leftLowCut.get<3>().coefficients = *cutCoefficients[3];
////            leftLowCut.setBypassed<3>(false);
////            break;
////        }
////    }
////    
//    //initialize right chain
//    auto& rightLowCut = rightChain.get<ChainPositions::LowCut>();
//    updateCutFilter(rightLowCut, lowCutCoefficients, chainSettings.lowCutSlope);
//    
//    // (bypass all links in right chain) there are 4 positions
////    rightLowCut.setBypassed<0>(true);
////    rightLowCut.setBypassed<1>(true);
////    rightLowCut.setBypassed<2>(true);
////    rightLowCut.setBypassed<3>(true);
////    
////    //use enum to display slope setting since enums decay to integers (which is what our choice parameter is expressed in)
////    switch (chainSettings.lowCutSlope){
////            
////            //if order is 2 = 12bc/oct slope, the helper function will return an array with 1 coefficient object only
////            //assign coefficients to first filter in cut filter chain and also stop bypassing that filter chain
////        case Slope_12:
////        {
////            //dereference left
////            *rightLowCut.get<0>().coefficients = *cutCoefficients[0];
////            //stop bypassing particular left chain
////            rightLowCut.setBypassed<0>(false);
////            break;
////        }
////            //now will assign to the first 2 links in the filter chain and stop by passing them
////        case Slope_24:
////        {
////            *rightLowCut.get<0>().coefficients = *cutCoefficients[0];
////            rightLowCut.setBypassed<0>(false);
////            *rightLowCut.get<1>().coefficients = *cutCoefficients[1];
////            rightLowCut.setBypassed<1>(false);
////            break;
////        }
////            //now will assign to the first 3 links in the filter chain and stop by passing them
////        case Slope_36:
////        {
////            *rightLowCut.get<0>().coefficients = *cutCoefficients[0];
////            rightLowCut.setBypassed<0>(false);
////            *rightLowCut.get<1>().coefficients = *cutCoefficients[1];
////            rightLowCut.setBypassed<1>(false);
////            *rightLowCut.get<2>().coefficients = *cutCoefficients[2];
////            rightLowCut.setBypassed<2>(false);
////            break;
////        }
////            //now will assign to the first 4 links in the filter chain and stop by passing them
////        case Slope_48:
////        {
////            *rightLowCut.get<0>().coefficients = *cutCoefficients[0];
////            rightLowCut.setBypassed<0>(false);
////            *rightLowCut.get<1>().coefficients = *cutCoefficients[1];
////            rightLowCut.setBypassed<1>(false);
////            *rightLowCut.get<2>().coefficients = *cutCoefficients[2];
////            rightLowCut.setBypassed<2>(false);
////            *rightLowCut.get<3>().coefficients = *cutCoefficients[3];
////            rightLowCut.setBypassed<3>(false);
////            break;
////        }
////    }
//    
//    auto highCutCoefficients = juce::dsp::FilterDesign<float>::designIIRLowpassHighOrderButterworthMethod(chainSettings.highCutFreq,
//                                                                                                       getSampleRate(),
//                                                                                                       2 * (chainSettings.highCutSlope + 1));
//    
//    
//    //initialize left chain
//    auto& leftHighCut = leftChain.get<ChainPositions::HighCut>();
//    //call new function
//    updateCutFilter(leftHighCut, highCutCoefficients, chainSettings.highCutSlope);
//    //initialize right chain
//    auto& rightHighCut = rightChain.get<ChainPositions::HighCut>();
//    //call new functionlo
//    updateCutFilter(rightHighCut, highCutCoefficients, chainSettings.highCutSlope);

    juce::dsp::AudioBlock<float> block(buffer);
    
    //extract individual channels using helper
    auto leftBlock = block.getSingleChannelBlock(0);
    auto rightBlock = block.getSingleChannelBlock(1);
    
    //create processing contexts that wrap each individual audio block
    juce::dsp::ProcessContextReplacing<float> leftContext(leftBlock);
    juce::dsp::ProcessContextReplacing<float> rightContext(rightBlock);
    
    //pass contexts to mono filter chains
    leftChain.process(leftContext);
    rightChain.process(rightContext);

    
}

//==============================================================================
bool SimpleEQAudioProcessor::hasEditor() const
{
    return true; // (change this to false if you choose to not supply an editor)
}

juce::AudioProcessorEditor* SimpleEQAudioProcessor::createEditor()
{
    return new SimpleEQAudioProcessorEditor (*this);
    //no more generic one
    //return new juce::GenericAudioProcessorEditor(*this);
}

//==============================================================================
void SimpleEQAudioProcessor::getStateInformation (juce::MemoryBlock& destData)
{
    // You should use this method to store your parameters in the memory block.
    // You could do that either as raw data, or use the XML or ValueTree classes
    // as intermediaries to make it easy to save and load complex data.
    
    //use a memory output stream to write APVTS oto memory block
    juce::MemoryOutputStream mos(destData, true);
    apvts.state.writeToStream(mos);
}

void SimpleEQAudioProcessor::setStateInformation (const void* data, int sizeInBytes)
{
    // You should use this method to restore your parameters from this memory block,
    // whose contents will have been created by the getStateInformation() call.
    
    //check tree you pull is valid before writing it
    auto tree = juce::ValueTree::readFromData(data, sizeInBytes);
    if (tree.isValid()) {
        apvts.replaceState(tree);
        updateFilters();
    }
}

ChainSettings getChainSettings (juce::AudioProcessorValueTreeState& apvts) {
    ChainSettings settings;
    //get parameter values from apvts
    //since the parameter is a normalized value you cant use the function below cause it expects real world values
    //apvts.getParameter("lowCutFreq")->getValue();
    //set up settings (initialize all variables
    settings.lowCutFreq = apvts.getRawParameterValue("LowCut Freq")->load();
    settings.highCutFreq = apvts.getRawParameterValue("HighCut Freq")->load();
    settings.peakFreq = apvts.getRawParameterValue("Peak Freq")->load();
    settings.peakGainInDecibels = apvts.getRawParameterValue("Peak Gain")->load();
    settings.peakQuality = apvts.getRawParameterValue("Peak Quality")->load();
    settings.lowCutSlope = static_cast<Slope>(apvts.getRawParameterValue("LowCut Slope")->load());
    settings.highCutSlope = static_cast<Slope>(apvts.getRawParameterValue("HighCut Slope")->load());
    return settings;
}

//implement free function
Coefficients makePeakFilter(const ChainSettings& chainSettings, double sampleRate)
{
    //this was originally in updatepeakfilter
    return juce::dsp::IIR::Coefficients<float>::makePeakFilter(sampleRate,
                                                               chainSettings.peakFreq,
                                                               chainSettings.peakQuality,
                                                               juce::Decibels::decibelsToGain(chainSettings.peakGainInDecibels));
}

//implement refactoring function beneath where we are getting the chain settings
//copy the implementation from the process block (paste here), repaste in process block & do the same thing in prepare to play
void SimpleEQAudioProcessor::updatePeakFilter(const ChainSettings &chainSettings) {
    
    //call the makePeakFilter function
    auto peakCoefficients = makePeakFilter(chainSettings, getSampleRate());
    //access coefficients using .coefficients and assign what we wrote above
    //dereference them using * on both sides
    //at this point the peak has been set up and will make audible changes to audio running through it if the gain parameter is not 0
    //whenever the slider changes we need to update the filter with new coefficients whenever the slider changes (do it next in process block)
    //use update coefficients function
    //*leftChain.get<ChainPositions::Peak>().coefficients = *peakCoefficients;
    //*rightChain.get<ChainPositions::Peak>().coefficients = *peakCoefficients;
    updateCoefficients(leftChain.get<ChainPositions::Peak>().coefficients, peakCoefficients);
    updateCoefficients(rightChain.get<ChainPositions::Peak>().coefficients, peakCoefficients);
}

void updateCoefficients(Coefficients &old, const Coefficients &replacements) {
    //reference objects allocated on the heap so we need to dereference them to get the underlying object
    *old = *replacements;
}

void SimpleEQAudioProcessor::updateLowCutFilters(const ChainSettings &chainSettings) {
    auto lowCutCoefficients = juce::dsp::FilterDesign<float>::designIIRHighpassHighOrderButterworthMethod(chainSettings.lowCutFreq,
                                                                                                      getSampleRate(),
                                                                                                      2 * (chainSettings.lowCutSlope + 1));
    //initialize left chain
    auto& leftLowCut = leftChain.get<ChainPositions::LowCut>();
    updateCutFilter(leftLowCut, lowCutCoefficients, chainSettings.lowCutSlope);
   //initialize right chain
    auto& rightLowCut = rightChain.get<ChainPositions::LowCut>();
    updateCutFilter(rightLowCut, lowCutCoefficients, chainSettings.lowCutSlope);
}

void SimpleEQAudioProcessor::updateHighCutFilters(const ChainSettings &chainSettings) {
    auto highCutCoefficients = juce::dsp::FilterDesign<float>::designIIRLowpassHighOrderButterworthMethod(chainSettings.highCutFreq,
                                                                                                       getSampleRate(),
                                                                                                       2 * (chainSettings.highCutSlope + 1));
    
    
    //initialize left chain
    auto& leftHighCut = leftChain.get<ChainPositions::HighCut>();
    //call new function
    updateCutFilter(leftHighCut, highCutCoefficients, chainSettings.highCutSlope);
    //initialize right chain
    auto& rightHighCut = rightChain.get<ChainPositions::HighCut>();
    //call new functionlo
    updateCutFilter(rightHighCut, highCutCoefficients, chainSettings.highCutSlope);
}

void SimpleEQAudioProcessor::updateFilters() {
    auto chainSettings = getChainSettings(apvts);
    
    updateLowCutFilters(chainSettings);
    updatePeakFilter(chainSettings);
    updateHighCutFilters(chainSettings);
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
                                                           juce::NormalisableRange<float>(20.f, 20000.f, 1.f, 0.25f),
                                                           // default value
                                                           20.f));
    
    //HIGHCUT FREQUENCY (default value 20000hz)
    layout.add(std::make_unique<juce::AudioParameterFloat>(//paramID
                                                           juce::ParameterID("HighCut Freq", 1),
                                                           //parameter name
                                                           "HighCut Freq",
                                                           //normalisablerange type
                                                           juce::NormalisableRange<float>(20.f, 20000.f, 1.f, 0.25f),
                                                           //default value
                                                           20000.f));
    
    //PEAK FREQUENCY (default value 750z)
    layout.add(std::make_unique<juce::AudioParameterFloat>(//paramID
                                                           juce::ParameterID("Peak Freq", 1),
                                                           //parameter name
                                                           "Peak Freq",
                                                           //normalisablerange type
                                                           juce::NormalisableRange<float>(20.f, 20000.f, 1.f, 0.25f),
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
                                                           juce::NormalisableRange<float>(-24.f, 24.f, 0.5f, 0.5f),
                                                           //default value
                                                           0.0f));
    
    //QUALITY CONTROL (how tight or how wide the peak band is)
    //Narrow Q = high Q value
    //Wide Q = low Q value
    layout.add(std::make_unique<juce::AudioParameterFloat>(//paramID
                                                           juce::ParameterID("Peak Quality", 1),
                                                           //parameter name
                                                           "Peak Quality",
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
