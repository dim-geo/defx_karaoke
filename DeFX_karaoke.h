/*******************************************************************************
 The block below describes the properties of this PIP. A PIP is a short snippet
 of code that can be read by the Projucer and used to generate a JUCE project.

 BEGIN_JUCE_PIP_METADATA

 name:             DeFX_Karaoke
 version:          1.0.0
 vendor:           JUCE
 website:          http://juce.com
 description:      Noise gate audio plugin.

 dependencies:     juce_audio_basics, juce_audio_devices, juce_audio_formats,
                   juce_audio_plugin_client, juce_audio_processors, juce_dsp,
                   juce_audio_utils, juce_core, juce_data_structures,
                   juce_events, juce_graphics, juce_gui_basics, juce_gui_extra
 exporters:        xcode_mac, vs2019, linux_make

 type:             AudioProcessor
 mainClass:        DeFXKaraoke

 useLocalCopy:     1

 END_JUCE_PIP_METADATA

*******************************************************************************/


#pragma once

//==============================================================================
class DeFXKaraoke  : public juce::AudioProcessor
{
public:
    //==============================================================================
    DeFXKaraoke()
        : AudioProcessor (BusesProperties().withInput  ("Input",     juce::AudioChannelSet::stereo())           // [1]
                                           .withOutput ("Output",    juce::AudioChannelSet::stereo()))
                                           //.withInput  ("Sidechain", juce::AudioChannelSet::stereo()))
    {
        //addParameter (threshold = new juce::AudioParameterFloat ("threshold", "Threshold", 0.0f, 1.0f, 0.5f));  // [2]
        //addParameter (alpha     = new juce::AudioParameterFloat ("alpha",     "Alpha",     0.0f, 1.0f, 0.8f));
        addParameter (level     = new juce::AudioParameterFloat ("level",     "Dry/Wet Mix",     0.0f, 1.0f, 1.0f));
        addParameter (MonoLevel = new juce::AudioParameterFloat ("MonoLevel", "Filtered Mono level", 0.0f, 1.0f, 0.4f));
        addParameter (Band      = new juce::AudioParameterFloat ("Band", "Lowpass Frequency",    10.0f, 20000.0f, 100.0f));
        //addParameter (Width     = new juce::AudioParameterFloat ("Width", "Width",  10.0f, 20000.0f, 250.0f));
        addParameter (Bandup      = new juce::AudioParameterFloat ("BandUp", "Highpass Frequency",    10.0f, 20000.0f, 7000.0f));
        addParameter (Width     = new juce::AudioParameterFloat ("Width", "Lowpass Q",  0.01f, 4.0f, 2.0f));
        addParameter (WidthUp     = new juce::AudioParameterFloat ("WidthUp", "Highpas Q",  0.01f, 4.0f, 2.0f));
        addParameter (bypassKaraoke = new juce::AudioParameterBool ("bypassKaraoke", "bypass Karaoke",  false));
        addParameter (listenKaraoke = new juce::AudioParameterBool ("ListenKaraoke", "Solo Filtered Mono",  false));
        
        notchfilter = new dsp::IIR::Filter<double>();
        notchfilter2 = new dsp::IIR::Filter<double>();
    }

    //==============================================================================
    bool isBusesLayoutSupported (const BusesLayout& layouts) const override
    {
        // the sidechain can take any layout, the main bus needs to be the same on the input and output
        return layouts.getMainInputChannelSet() == layouts.getMainOutputChannelSet()
                 && ! layouts.getMainInputChannelSet().isDisabled();
    }

    //==============================================================================
    void prepareToPlay (double sampleRate, int samplesPerBlock) override
    {
      //  lowPassCoeff = 0.0f;    // [3]
      //  sampleCountDown = 0;    // [4]
      dsp::ProcessSpec spec;
      spec.sampleRate = sampleRate;
      spec.maximumBlockSize = samplesPerBlock;
      spec.numChannels = getTotalNumOutputChannels()-1;

      //notchfilter = new dsp::IIR::Filter<float>(dsp::IIR::Coefficients<float>::makeNotch (sampleRate, 400.0f, 0.01));
      //notchfilter-> coefficients = dsp::IIR::Coefficients<float>::makeNotch (sampleRate, 1000.0f, 0.15);
      notchfilter-> coefficients = dsp::IIR::Coefficients<double>::makeLowPass (sampleRate, 100.0f,2.0f);
      notchfilter->prepare(spec);
      notchfilter->reset();

      //notchfilter2-> coefficients = dsp::IIR::Coefficients<float>::makeNotch (sampleRate, 1000.0f, 0.15);
      notchfilter2-> coefficients = dsp::IIR::Coefficients<double>::makeHighPass (sampleRate, 6000.0f,2.0f);
      notchfilter2->prepare(spec);
      notchfilter2->reset();

    }

    void releaseResources() override          {}

    void processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer&) override
    {
        
        auto mainInputOutput = getBusBuffer (buffer, true, 0);                                  // [5]
        //auto sideChainInput  = getBusBuffer (buffer, true, 1);

        auto bypass = bypassKaraoke->get();
        auto listen = listenKaraoke->get();
        auto levelCopy = level->get();                                                          // [6]
        auto MonoLevelCopy = MonoLevel->get();
        auto BandCopy = Band->get();                                                          // [6]
        auto BandupCopy = Bandup->get();                                                          // [6]
        auto WidthCopy = Width->get();
        auto WidthUpCopy = WidthUp->get();
        auto samplerate = (int) getSampleRate();
        
        if ((BandCopy != oldband) || ( WidthCopy != oldwidth) ){
           //notchfilter-> coefficients = dsp::IIR::Coefficients<float>::makeNotch (samplerate, BandCopy, WidthCopy);
           notchfilter-> coefficients = dsp::IIR::Coefficients<double>::makeLowPass (samplerate, BandCopy,WidthCopy);
           notchfilter->reset();
           oldband=BandCopy;
           oldwidth=WidthCopy;
        }

        if ((WidthUpCopy != oldwidthup) || ( BandupCopy != oldbandup ) ){
           //notchfilter2-> coefficients = dsp::IIR::Coefficients<float>::makeNotch (samplerate, BandCopy, WidthCopy);
           notchfilter2-> coefficients = dsp::IIR::Coefficients<double>::makeHighPass (samplerate, BandupCopy,WidthUpCopy);
           notchfilter2->reset();
           oldbandup=BandupCopy;
           oldwidthup=WidthUpCopy;
        }
        
        for (auto j = 0; j < buffer.getNumSamples(); ++j)                                       // [7]
        {
            
            float data[2],data_out[2];
            for (auto i = 0; i < mainInputOutput.getNumChannels(); ++i){                          // [8]
                data[i]= mainInputOutput.getReadPointer (i) [j];
                if (bypass){
                    *mainInputOutput.getWritePointer (i, j) = data[i];
                    return;
                }
            }
            auto l=data[0];
            auto r=data[1];
            
            auto x= l*.5 + r*.5;
            
            //y=notchfilter2->processSample(notchfilter->processSample(x));
            y=notchfilter->processSample(x)+notchfilter2->processSample(x);
            notchfilter->snapToZero();
            notchfilter2->snapToZero();
            
            auto o=2.0f*y*MonoLevelCopy;
            if (o > 1.0f){
                o=1.0f;
            }else if(o<-1.0f){
                o=-1.0f;
            }
            o=levelCopy*o;
            if(listen){
                x=o;
                r=o;
            }else{
                x=l-r*levelCopy + o;
                r=r-l*levelCopy + o;
            }
            if (x > 1.0f){
                x=1.0f;
            }else if(x<-1.0f){
                x=-1.0f;
            }
            if (r > 1.0f){
                r=1.0f;
            }else if(r<-1.0f){
                r=-1.0f;
            }
            data_out[0]=x;
            data_out[1]=r;
            //data_out[0]=data[0];
            //data_out[1]=data[1];
            
            for (auto i = 0; i < mainInputOutput.getNumChannels(); ++i)                         // [11]
                *mainInputOutput.getWritePointer (i, j) = data_out[i];
        
        }
    }

    //==============================================================================
    juce::AudioProcessorEditor* createEditor() override            { return new juce::GenericAudioProcessorEditor (*this); }
    bool hasEditor() const override                                { return true; }
    const juce::String getName() const override                    { return "DeFX Karaoke"; }
    bool acceptsMidi() const override                              { return false; }
    bool producesMidi() const override                             { return false; }
    double getTailLengthSeconds() const override                   { return 0.0; }
    int getNumPrograms() override                                  { return 1; }
    int getCurrentProgram() override                               { return 0; }
    void setCurrentProgram (int) override                          {}
    const juce::String getProgramName (int) override               { return {}; }
    void changeProgramName (int, const juce::String&) override     {}

    bool isVST2() const noexcept                                   { return (wrapperType == wrapperType_VST); }

    //==============================================================================
    void getStateInformation (juce::MemoryBlock& destData) override
    {
        juce::MemoryOutputStream stream (destData, true);

        /*stream.writeFloat (*level);
        stream.writeFloat (*Band);
        stream.writeFloat (*Bandup);
        stream.writeFloat (*Width);
        stream.writeFloat (*WidthUp);
        stream.writeFloat (*MonoLevel);
        stream.writeBool (*bypassKaraoke);
        stream.writeBool (*listenKaraoke);*/
        
    }

    void setStateInformation (const void* data, int sizeInBytes) override
    {
        juce::MemoryInputStream stream (data, static_cast<size_t> (sizeInBytes), false);

        /*level->setValueNotifyingHost (stream.readFloat());
        Band->setValueNotifyingHost (stream.readFloat());
        Bandup->setValueNotifyingHost (stream.readFloat());
        Width->setValueNotifyingHost (stream.readFloat());
        WidthUp->setValueNotifyingHost (stream.readFloat());
        MonoLevel->setValueNotifyingHost (stream.readFloat());
        bypassKaraoke->setValueNotifyingHost (stream.readBool());
        listenKaraoke->setValueNotifyingHost (stream.readBool());*/
    }

private:
    //==============================================================================
    //juce::AudioParameterFloat* threshold;
    //juce::AudioParameterFloat* alpha;
    //int sampleCountDown;

    /* effect level */
    juce::AudioParameterFloat* level;

    /* center band and bandwith of filter */
    juce::AudioParameterFloat* Band;
    juce::AudioParameterFloat* Bandup;
    juce::AudioParameterFloat* Width;
    juce::AudioParameterFloat* WidthUp;
    
    double y;//,y1=0,y2=0;
    float oldband,oldwidth=0.0f;
    float oldbandup,oldwidthup=0.0f;
    //double x1=0,x2=0;

    /* filtered mono signal level */
    juce::AudioParameterFloat* MonoLevel;
    
    dsp::IIR::Filter<double>* notchfilter;
    dsp::IIR::Filter<double>* notchfilter2;
    
    
    juce::AudioParameterBool* bypassKaraoke;
    juce::AudioParameterBool* listenKaraoke;
    
    //float lowPassCoeff;

    //==============================================================================
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (DeFXKaraoke)
};
