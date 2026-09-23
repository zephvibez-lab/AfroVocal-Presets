#pragma once

#include <JuceHeader.h>

class AfroVocalPresetsAudioProcessor final : public juce::AudioProcessor
{
public:
    AfroVocalPresetsAudioProcessor();
    ~AfroVocalPresetsAudioProcessor() override;

    void prepareToPlay(double sampleRate, int samplesPerBlock) override;
    void releaseResources() override;
    void reset() override;
    bool isBusesLayoutSupported(const BusesLayout& layouts) const override;
    void processBlock(juce::AudioBuffer<float>&, juce::MidiBuffer&) override;
    void processBlockBypassed(juce::AudioBuffer<float>&, juce::MidiBuffer&) override;

    juce::AudioProcessorParameter* getBypassParameter() const override;
    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override { return true; }
    const juce::String getName() const override { return JucePlugin_Name; }
    bool acceptsMidi() const override { return false; }
    bool producesMidi() const override { return false; }
    bool isMidiEffect() const override { return false; }
    double getTailLengthSeconds() const override { return 2.6; }
    int getNumPrograms() override { return 1; }
    int getCurrentProgram() override { return 0; }
    void setCurrentProgram(int) override {}
    const juce::String getProgramName(int) override { return {}; }
    void changeProgramName(int, const juce::String&) override {}
    void getStateInformation(juce::MemoryBlock&) override;
    void setStateInformation(const void*, int) override;

    juce::AudioProcessorValueTreeState parameters;
    static juce::AudioProcessorValueTreeState::ParameterLayout createParameterLayout();
    void applyFactoryPreset(const juce::String& presetName);
    juce::String getLastPreset() const;

    void requestAiPreset(const juce::String& prompt, const juce::String& apiKey);
    bool isAiBusy() const { return aiBusy.load(std::memory_order_relaxed); }
    juce::String getAiStatus() const;
    bool applyPendingAiPreset();
    float getMeterPeak() const { return meterPeak.load(std::memory_order_relaxed); }
    float getGainReduction() const { return gainReduction.load(std::memory_order_relaxed); }

private:
    class AiWorker;
    std::unique_ptr<AiWorker> aiWorker;

    struct ChannelDSP;
    std::unique_ptr<ChannelDSP> channelDSP;
    juce::SmoothedValue<float> outputGain;

    std::atomic<float>* bypassParam = nullptr;
    std::atomic<float>* tuneAmountParam = nullptr;
    std::atomic<float>* retuneSpeedParam = nullptr;
    std::atomic<float>* keyParam = nullptr;
    std::atomic<float>* scaleParam = nullptr;
    std::atomic<float>* highPassParam = nullptr;
    std::atomic<float>* lowMidParam = nullptr;
    std::atomic<float>* presenceParam = nullptr;
    std::atomic<float>* airParam = nullptr;
    std::atomic<float>* compThresholdParam = nullptr;
    std::atomic<float>* compRatioParam = nullptr;
    std::atomic<float>* compAttackParam = nullptr;
    std::atomic<float>* compReleaseParam = nullptr;
    std::atomic<float>* deEssParam = nullptr;
    std::atomic<float>* warmthParam = nullptr;
    std::atomic<float>* parallelParam = nullptr;
    std::atomic<float>* plateParam = nullptr;
    std::atomic<float>* ambientParam = nullptr;
    std::atomic<float>* delayMixParam = nullptr;
    std::atomic<float>* delayMsParam = nullptr;
    std::atomic<float>* outputGainParam = nullptr;

    juce::String lastPreset { "Afrobeat Lead - Clear Bounce" };
    mutable juce::CriticalSection metadataLock;
    std::atomic<bool> aiBusy { false };
    mutable juce::CriticalSection aiStatusLock;
    juce::String aiStatus { "Offline-ready. AI requires an API key and internet connection." };
    mutable juce::CriticalSection pendingAiLock;
    juce::var pendingAiJson;
    bool pendingAiReady = false;
    std::atomic<float> meterPeak { 0.0f };
    std::atomic<float> gainReduction { 0.0f };

    void cacheParameterPointers();
    void setAiStatus(const juce::String& status);
    void queueAiPreset(const juce::var& json);
    void applyAiJson(const juce::var& json);

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(AfroVocalPresetsAudioProcessor)
};
