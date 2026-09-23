#include "PluginProcessor.h"
#include "PluginEditor.h"

namespace
{
    juce::NormalisableRange<float> range(float lo, float hi, float step = 0.01f)
    {
        return { lo, hi, step };
    }

    juce::AudioParameterFloatAttributes percentAttributes()
    {
        return juce::AudioParameterFloatAttributes().withStringFromValueFunction(
            [] (float value, int) { return juce::String(value * 100.0f, 0) + "%"; });
    }

    float clampFinite(float value, float lo, float hi, float fallback)
    {
        return std::isfinite(value) ? juce::jlimit(lo, hi, value) : fallback;
    }

    struct Biquad
    {
        float b0 = 1.0f, b1 = 0.0f, b2 = 0.0f, a1 = 0.0f, a2 = 0.0f;
        float z1 = 0.0f, z2 = 0.0f;

        void reset() noexcept { z1 = z2 = 0.0f; }
        float process(float x) noexcept
        {
            const float y = b0 * x + z1;
            z1 = b1 * x - a1 * y + z2;
            z2 = b2 * x - a2 * y;
            return y;
        }

        void set(float nb0, float nb1, float nb2, float na0, float na1, float na2) noexcept
        {
            const float inv = 1.0f / na0;
            b0 = nb0 * inv; b1 = nb1 * inv; b2 = nb2 * inv;
            a1 = na1 * inv; a2 = na2 * inv;
        }

        void highPass(double sr, float frequency, float q = 0.7071f) noexcept
        {
            const float w = juce::MathConstants<float>::twoPi * frequency / static_cast<float>(sr);
            const float c = std::cos(w), s = std::sin(w), alpha = s / (2.0f * q);
            set((1.0f + c) * 0.5f, -(1.0f + c), (1.0f + c) * 0.5f, 1.0f + alpha, -2.0f * c, 1.0f - alpha);
        }

        void peak(double sr, float frequency, float q, float db) noexcept
        {
            const float w = juce::MathConstants<float>::twoPi * frequency / static_cast<float>(sr);
            const float c = std::cos(w), s = std::sin(w), alpha = s / (2.0f * q);
            const float A = std::pow(10.0f, db / 40.0f);
            set(1.0f + alpha * A, -2.0f * c, 1.0f - alpha * A, 1.0f + alpha / A, -2.0f * c, 1.0f - alpha / A);
        }

        void highShelf(double sr, float frequency, float db) noexcept
        {
            const float w = juce::MathConstants<float>::twoPi * frequency / static_cast<float>(sr);
            const float c = std::cos(w), s = std::sin(w), A = std::pow(10.0f, db / 40.0f);
            const float alpha = s * 0.5f * std::sqrt((A + 1.0f / A) * (1.0f / 0.65f - 1.0f) + 2.0f);
            const float beta = 2.0f * std::sqrt(A) * alpha;
            set(A * ((A + 1) + (A - 1) * c + beta), -2 * A * ((A - 1) + (A + 1) * c), A * ((A + 1) + (A - 1) * c - beta),
                (A + 1) - (A - 1) * c + beta, 2 * ((A - 1) - (A + 1) * c), (A + 1) - (A - 1) * c - beta);
        }
    };
}

struct AfroVocalPresetsAudioProcessor::ChannelDSP
{
    static constexpr int analysisSize = 2048;
    static constexpr int hopSize = 256;
    static constexpr int ringSize = 16384;

    struct PitchEstimate { float frequency = 0.0f; float confidence = 0.0f; bool voiced = false; };

    Biquad highPass[2], lowMid[2], presence[2], air[2];
    juce::dsp::Compressor<float> compressor, parallelCompressor;
    juce::Reverb reverb;
    juce::dsp::DelayLine<float, juce::dsp::DelayLineInterpolationTypes::Linear> delay { 192000 };
    juce::AudioBuffer<float> parallelBuffer, reverbBuffer;
    std::array<float, ringSize> monoRing{};
    std::array<float, analysisSize> analysis{};
    std::array<float, ringSize> shiftRingL{}, shiftRingR{};
    juce::SmoothedValue<float> pitchRatio, correctionAuthority, saturation;
    double sampleRate = 48000.0;
    int preparedChannels = 2;
    int preparedMaximum = 512;
    int ringWrite = 0;
    int analysisCounter = 0;
    float previousPitch = 0.0f;
    float targetMidi = 69.0f;
    float pitchConfidence = 0.0f;
    bool voiced = false;
    float deEssEnvelope = 0.0f;
    float delayFeedback = 0.18f;

    void prepare(double sr, int block, int channels)
    {
        sampleRate = sr; preparedMaximum = juce::jmax(1, block); preparedChannels = juce::jlimit(1, 2, channels);
        const juce::dsp::ProcessSpec spec { sr, static_cast<juce::uint32>(preparedMaximum), static_cast<juce::uint32>(preparedChannels) };
        compressor.prepare(spec); parallelCompressor.prepare(spec); delay.prepare(spec);
        parallelBuffer.setSize(preparedChannels, preparedMaximum, false, false, true);
        reverbBuffer.setSize(preparedChannels, preparedMaximum, false, false, true);
        reverb.setSampleRate(sr);
        pitchRatio.reset(sr, 0.025); pitchRatio.setCurrentAndTargetValue(1.0f);
        correctionAuthority.reset(sr, 0.012); correctionAuthority.setCurrentAndTargetValue(0.0f);
        saturation.reset(sr, 0.03); saturation.setCurrentAndTargetValue(1.0f);
        reset();
    }

    void reset()
    {
        for (auto& f : highPass) f.reset(); for (auto& f : lowMid) f.reset(); for (auto& f : presence) f.reset(); for (auto& f : air) f.reset();
        compressor.reset(); parallelCompressor.reset(); reverb.reset(); delay.reset();
        parallelBuffer.clear(); reverbBuffer.clear(); monoRing.fill(0.0f); analysis.fill(0.0f); shiftRingL.fill(0.0f); shiftRingR.fill(0.0f);
        ringWrite = 0; analysisCounter = 0; previousPitch = 0.0f; targetMidi = 69.0f; pitchConfidence = 0.0f; voiced = false; deEssEnvelope = 0.0f;
        pitchRatio.setCurrentAndTargetValue(1.0f); correctionAuthority.setCurrentAndTargetValue(0.0f); saturation.setCurrentAndTargetValue(1.0f);
    }

    void updateFilters(float hp, float lowMidDb, float presenceDb, float airDb) noexcept
    {
        for (int ch = 0; ch < preparedChannels; ++ch)
        {
            highPass[ch].highPass(sampleRate, hp);
            lowMid[ch].peak(sampleRate, 280.0f, 0.9f, lowMidDb);
            presence[ch].peak(sampleRate, 4200.0f, 0.8f, presenceDb);
            air[ch].highShelf(sampleRate, 10500.0f, airDb);
        }
    }

    PitchEstimate detectPitch() noexcept
    {
        float energy = 0.0f;
        for (int i = 0; i < analysisSize; ++i) energy += analysis[static_cast<size_t>(i)] * analysis[static_cast<size_t>(i)];
        const float rms = std::sqrt(energy / static_cast<float>(analysisSize));
        if (rms < 0.0025f) return {};

        const int minLag = juce::jmax(20, static_cast<int>(sampleRate / 1200.0));
        const int maxLag = juce::jmin(analysisSize / 2 - 2, static_cast<int>(sampleRate / 65.0));
        float bestScore = 0.0f, secondScore = 0.0f; int bestLag = 0;
        for (int lag = minLag; lag <= maxLag; ++lag)
        {
            float diff = 0.0f, corr = 0.0f, a = 0.0f, b = 0.0f;
            for (int i = 0; i < analysisSize - lag; i += 4)
            {
                const float x = analysis[static_cast<size_t>(i)], y = analysis[static_cast<size_t>(i + lag)];
                const float d = x - y; diff += d * d; corr += x * y; a += x * x; b += y * y;
            }
            const float normCorr = corr / (std::sqrt(a * b) + 1.0e-9f);
            const float score = juce::jmax(0.0f, normCorr - diff / (a + b + 1.0e-9f) * 0.35f);
            if (score > bestScore) { secondScore = bestScore; bestScore = score; bestLag = lag; }
            else if (score > secondScore) secondScore = score;
        }
        if (bestLag == 0 || bestScore < 0.23f) return {};
        const float frequency = static_cast<float>(sampleRate) / static_cast<float>(bestLag);
        const float continuity = previousPitch > 0.0f ? std::exp(-std::abs(std::log2(frequency / previousPitch)) * 4.0f) : 0.65f;
        const float confidence = juce::jlimit(0.0f, 1.0f, bestScore * 0.9f + (bestScore - secondScore) * 1.7f + continuity * 0.2f);
        return { frequency, confidence, confidence > 0.34f };
    }

    float targetFor(float midi, int key, int scale) const noexcept
    {
        if (key == 0) return std::round(midi);
        static constexpr int major[] = { 0, 2, 4, 5, 7, 9, 11 };
        static constexpr int minor[] = { 0, 2, 3, 5, 7, 8, 10 };
        static constexpr int dorian[] = { 0, 2, 3, 5, 7, 9, 10 };
        static constexpr int pent[] = { 0, 2, 4, 7, 9 };
        const int* intervals = major; int count = 7;
        if (scale == 1) intervals = minor; else if (scale == 2) intervals = dorian; else if (scale == 3) { intervals = pent; count = 5; }
        const float octave = std::floor((midi - 60.0f) / 12.0f);
        const float local = midi - (60.0f + octave * 12.0f) - static_cast<float>(key);
        float best = static_cast<float>(intervals[0]); float distance = 100.0f;
        for (int i = 0; i < count; ++i) { const float d = std::abs(local - static_cast<float>(intervals[i])); if (d < distance) { distance = d; best = static_cast<float>(intervals[i]); } }
        return 60.0f + octave * 12.0f + static_cast<float>(key) + best;
    }

    float readRing(const std::array<float, ringSize>& ring, float position) const noexcept
    {
        while (position < 0.0f) position += static_cast<float>(ringSize);
        const int i0 = static_cast<int>(position) % ringSize, i1 = (i0 + 1) % ringSize;
        const float frac = position - std::floor(position);
        return ring[static_cast<size_t>(i0)] * (1.0f - frac) + ring[static_cast<size_t>(i1)] * frac;
    }

    void process(juce::AudioBuffer<float>& buffer, float tune, float retuneMs, int key, int scale, float hp, float lowMidDb,
                 float presenceDb, float airDb, float compThreshold, float compRatio, float compAttack, float compRelease,
                 float deEss, float warmth, float parallel, float plate, float ambient, float delayMix, float delayMs)
    {
        juce::ignoreUnused(retuneMs);
        const int channels = juce::jmin(preparedChannels, buffer.getNumChannels());
        const int samples = juce::jmin(preparedMaximum, buffer.getNumSamples());
        if (samples <= 0 || channels <= 0 || samples != buffer.getNumSamples()) { buffer.clear(); return; }
        updateFilters(hp, lowMidDb, presenceDb, airDb);
        compressor.setThreshold(compThreshold); compressor.setRatio(compRatio); compressor.setAttack(compAttack); compressor.setRelease(compRelease);
        parallelCompressor.setThreshold(-24.0f); parallelCompressor.setRatio(8.0f); parallelCompressor.setAttack(2.0f); parallelCompressor.setRelease(80.0f);
        juce::dsp::AudioBlock<float> block(buffer);

        for (int s = 0; s < samples; ++s)
        {
            const float inL = buffer.getSample(0, s), inR = channels > 1 ? buffer.getSample(1, s) : inL;
            const float mono = 0.5f * (inL + inR);
            monoRing[static_cast<size_t>(ringWrite)] = mono;
            analysis[static_cast<size_t>(analysisCounter)] = mono;
            if (++analysisCounter >= analysisSize) analysisCounter = 0;
            if (++ringWrite >= ringSize) ringWrite = 0;

            const int historyIndex = (ringWrite - analysisSize + ringSize) % ringSize;
            if (analysisCounter % hopSize == 0)
            {
                for (int i = 0; i < analysisSize; ++i) analysis[static_cast<size_t>(i)] = monoRing[static_cast<size_t>((historyIndex + i) % ringSize)];
                const auto estimate = detectPitch();
                voiced = estimate.voiced;
                pitchConfidence = estimate.confidence;
                if (voiced)
                {
                    previousPitch = estimate.frequency;
                    const float midi = 69.0f + 12.0f * std::log2(estimate.frequency / 440.0f);
                    targetMidi = targetFor(midi, key, scale);
                    const float cents = 100.0f * (targetMidi - midi);
                    const float correction = cents * tune;
                    const float ratio = juce::jlimit(0.5f, 2.0f, std::pow(2.0f, correction / 1200.0f));
                    const float authority = juce::jlimit(0.0f, 1.0f, tune * estimate.confidence);
                    pitchRatio.setTargetValue(ratio); correctionAuthority.setTargetValue(authority);
                }
                else { pitchRatio.setTargetValue(1.0f); correctionAuthority.setTargetValue(0.0f); }
            }

            const float ratio = pitchRatio.getNextValue();
            const float authority = correctionAuthority.getNextValue();
            const int baseDelay = 1280;
            const float readPos = static_cast<float>((ringWrite - baseDelay + ringSize) % ringSize);
            shiftRingL[static_cast<size_t>(ringWrite)] = inL; shiftRingR[static_cast<size_t>(ringWrite)] = inR;
            const float shiftedL = readRing(shiftRingL, readPos + (ratio - 1.0f) * 420.0f);
            const float shiftedR = readRing(shiftRingR, readPos + (ratio - 1.0f) * 420.0f);
            buffer.setSample(0, s, inL * (1.0f - authority) + shiftedL * authority);
            if (channels > 1) buffer.setSample(1, s, inR * (1.0f - authority) + shiftedR * authority);
        }

        auto context = juce::dsp::ProcessContextReplacing<float>(block);
        compressor.process(context);
        parallelBuffer.makeCopyOf(buffer, true);
        juce::dsp::AudioBlock<float> parallelBlock(parallelBuffer);
        parallelCompressor.process(juce::dsp::ProcessContextReplacing<float>(parallelBlock));
        const float satDrive = 1.0f + warmth * 5.0f; saturation.setTargetValue(satDrive);
        const float deEssCoeff = juce::jlimit(0.0f, 0.9f, deEss * 0.82f);
        for (int s = 0; s < samples; ++s)
        {
            const float drive = saturation.getNextValue();
            for (int ch = 0; ch < channels; ++ch)
            {
                float x = buffer.getSample(ch, s);
                const float bright = x - (s > 0 ? buffer.getSample(ch, s - 1) * 0.985f : 0.0f);
                deEssEnvelope = 0.999f * deEssEnvelope + 0.001f * std::abs(bright);
                x *= 1.0f - deEssCoeff * juce::jlimit(0.0f, 1.0f, deEssEnvelope * 16.0f);
                x = std::tanh(x * drive) / juce::jmax(0.01f, std::tanh(drive));
                buffer.setSample(ch, s, x * (1.0f - parallel) + parallelBuffer.getSample(ch, s) * parallel);
            }
        }

        juce::Reverb::Parameters rp;
        rp.roomSize = 0.15f + plate * 0.35f + ambient * 0.45f; rp.damping = 0.45f;
        rp.wetLevel = juce::jlimit(0.0f, 0.7f, plate * 0.22f + ambient * 0.30f); rp.dryLevel = 1.0f; rp.width = 0.85f;
        reverb.setParameters(rp); reverbBuffer.makeCopyOf(buffer, true);
        if (channels > 1) reverb.processStereo(reverbBuffer.getWritePointer(0), reverbBuffer.getWritePointer(1), samples);
        else reverb.processMono(reverbBuffer.getWritePointer(0), samples);
        const float wet = juce::jlimit(0.0f, 0.75f, plate * 0.22f + ambient * 0.30f);
        for (int ch = 0; ch < channels; ++ch) buffer.addFrom(ch, 0, reverbBuffer, ch, 0, samples, wet);

        const float delaySamples = static_cast<float>(juce::jlimit(1.0, 190000.0, delayMs * sampleRate / 1000.0));
        const float dMix = juce::jlimit(0.0f, 0.55f, delayMix);
        for (int s = 0; s < samples; ++s)
        {
            const float delayedL = delay.popSample(0, delaySamples), delayedR = channels > 1 ? delay.popSample(1, delaySamples) : delayedL;
            for (int ch = 0; ch < channels; ++ch)
            {
                const float input = buffer.getSample(ch, s), delayed = ch == 0 ? delayedL : delayedR;
                delay.pushSample(ch, input + delayed * delayFeedback); buffer.setSample(ch, s, input + delayed * dMix);
            }
        }
    }
};

class AfroVocalPresetsAudioProcessor::AiWorker final : private juce::Thread
{
public:
    explicit AiWorker(AfroVocalPresetsAudioProcessor& owner) : Thread("AfroVocal AI"), processor(owner) {}
    ~AiWorker() override { signalThreadShouldExit(); notify(); stopThread(3000); }
    void startRequest(const juce::String& prompt, const juce::String& key)
    {
        const juce::ScopedLock lock(requestLock); requestPrompt = prompt; apiKey = key;
        if (!isThreadRunning()) startThread(); notify();
    }
private:
    AfroVocalPresetsAudioProcessor& processor; juce::CriticalSection requestLock; juce::String requestPrompt, apiKey;
    void run() override
    {
        while (!threadShouldExit())
        {
            wait(-1); if (threadShouldExit()) break;
            juce::String prompt, key; { const juce::ScopedLock lock(requestLock); prompt = requestPrompt; key = apiKey; requestPrompt.clear(); }
            if (prompt.isEmpty()) continue;
            if (key.trim().isEmpty()) { processor.setAiStatus("AI disabled: enter an API key to use online assistance."); processor.aiBusy.store(false); continue; }
            processor.setAiStatus("AI is creating a vocal preset…");
            juce::Array<juce::var> messageArray;
            messageArray.add(juce::JSON::parse("{\"role\":\"system\",\"content\":\"Return only JSON with presetName,tuneAmount,air,space,warmth,outputGain,highPass,lowMidCut,presence,compThreshold,compRatio,deEss,parallelComp,plate,ambient,delayMix,delayMs. Values must be numbers in sensible ranges.\"}"));
            messageArray.add(juce::JSON::parse("{\"role\":\"user\",\"content\":\"" + prompt.replace("\\", "\\\\").replace("\"", "\\\"") + "\"}"));
            juce::DynamicObject body; body.setProperty("model", "gpt-4o-mini"); body.setProperty("temperature", 0.3); body.setProperty("messages", juce::var(messageArray));
            body.setProperty("response_format", juce::JSON::parse("{\"type\":\"json_object\"}"));
            juce::URL url("https://api.openai.com/v1/chat/completions");
            auto options = juce::URL::InputStreamOptions(juce::URL::ParameterHandling::inPostData).withExtraHeaders("Authorization: Bearer " + key + "\r\nContent-Type: application/json").withConnectionTimeoutMs(12000);
            std::unique_ptr<juce::InputStream> stream(url.withPOSTData(juce::JSON::toString(juce::var(&body))).createInputStream(options));
            if (stream == nullptr) { processor.setAiStatus("AI unavailable: could not connect. Offline presets remain active."); processor.aiBusy.store(false); continue; }
            auto parsed = juce::JSON::parse(stream->readEntireStreamAsString());
            auto* root = parsed.getDynamicObject();
            if (root == nullptr) { processor.setAiStatus("AI response was not valid JSON."); processor.aiBusy.store(false); continue; }
            auto choices = root->getProperty("choices");
            if (!choices.isArray() || choices.getArray()->isEmpty()) { processor.setAiStatus("AI did not return a preset. Offline presets remain active."); processor.aiBusy.store(false); continue; }
            auto* choice = (*choices.getArray())[0].getDynamicObject();
            auto* message = choice != nullptr ? choice->getProperty("message").getDynamicObject() : nullptr;
            const auto content = message != nullptr ? message->getProperty("content").toString() : juce::String();
            auto preset = juce::JSON::parse(content);
            if (preset.isVoid()) { processor.setAiStatus("AI preset JSON could not be parsed."); processor.aiBusy.store(false); continue; }
            processor.queueAiPreset(preset); processor.setAiStatus("AI preset ready — loading on the message thread…");
        }
    }
};

AfroVocalPresetsAudioProcessor::AfroVocalPresetsAudioProcessor()
    : AudioProcessor(BusesProperties().withInput("Input", juce::AudioChannelSet::stereo(), true).withOutput("Output", juce::AudioChannelSet::stereo(), true)),
      parameters(*this, nullptr, "PARAMETERS", createParameterLayout()), aiWorker(std::make_unique<AiWorker>(*this)), channelDSP(std::make_unique<ChannelDSP>())
{
    cacheParameterPointers();
}

AfroVocalPresetsAudioProcessor::~AfroVocalPresetsAudioProcessor() = default;

void AfroVocalPresetsAudioProcessor::cacheParameterPointers()
{
    auto get = [this](const char* id) { return parameters.getRawParameterValue(id); };
    bypassParam = get("bypass"); tuneAmountParam = get("tuneAmount"); retuneSpeedParam = get("retuneSpeed"); keyParam = get("key"); scaleParam = get("scale");
    highPassParam = get("highPass"); lowMidParam = get("lowMidCut"); presenceParam = get("presence"); airParam = get("air"); compThresholdParam = get("compThreshold");
    compRatioParam = get("compRatio"); compAttackParam = get("compAttack"); compReleaseParam = get("compRelease"); deEssParam = get("deEss"); warmthParam = get("warmth");
    parallelParam = get("parallelComp"); plateParam = get("plate"); ambientParam = get("ambient"); delayMixParam = get("delayMix"); delayMsParam = get("delayMs"); outputGainParam = get("outputGain");
}

juce::AudioProcessorValueTreeState::ParameterLayout AfroVocalPresetsAudioProcessor::createParameterLayout()
{
    std::vector<std::unique_ptr<juce::RangedAudioParameter>> p;
    p.push_back(std::make_unique<juce::AudioParameterBool>("bypass", "Bypass", false));
    p.push_back(std::make_unique<juce::AudioParameterFloat>("tuneAmount", "Tune Amount", range(0, 1), 0.35f, percentAttributes()));
    p.push_back(std::make_unique<juce::AudioParameterFloat>("retuneSpeed", "Retune Speed", range(0, 100), 35.0f, "ms"));
    p.push_back(std::make_unique<juce::AudioParameterChoice>("key", "Key", juce::StringArray { "Chromatic", "C", "C#", "D", "D#", "E", "F", "F#", "G", "G#", "A", "A#", "B" }, 0));
    p.push_back(std::make_unique<juce::AudioParameterChoice>("scale", "Scale", juce::StringArray { "Major", "Minor", "Dorian", "Pentatonic" }, 1));
    p.push_back(std::make_unique<juce::AudioParameterFloat>("highPass", "High Pass", range(40, 220, 1), 75.0f, "Hz"));
    p.push_back(std::make_unique<juce::AudioParameterFloat>("lowMidCut", "Low-Mid Cleanup", range(-9, 3), -2.5f, "dB"));
    p.push_back(std::make_unique<juce::AudioParameterFloat>("presence", "Presence", range(-3, 8), 2.5f, "dB"));
    p.push_back(std::make_unique<juce::AudioParameterFloat>("air", "Air", range(0, 1), 0.45f, percentAttributes()));
    p.push_back(std::make_unique<juce::AudioParameterFloat>("compThreshold", "Comp Threshold", range(-36, -6), -18.0f, "dB"));
    p.push_back(std::make_unique<juce::AudioParameterFloat>("compRatio", "Comp Ratio", range(1, 12), 3.0f, ":1"));
    p.push_back(std::make_unique<juce::AudioParameterFloat>("compAttack", "Comp Attack", range(0.5, 40), 8.0f, "ms"));
    p.push_back(std::make_unique<juce::AudioParameterFloat>("compRelease", "Comp Release", range(30, 300), 100.0f, "ms"));
    p.push_back(std::make_unique<juce::AudioParameterFloat>("deEss", "De-Esser", range(0, 1), 0.35f, percentAttributes()));
    p.push_back(std::make_unique<juce::AudioParameterFloat>("warmth", "Warmth", range(0, 1), 0.30f, percentAttributes()));
    p.push_back(std::make_unique<juce::AudioParameterFloat>("parallelComp", "Parallel Comp", range(0, 1), 0.10f, percentAttributes()));
    p.push_back(std::make_unique<juce::AudioParameterFloat>("plate", "Short Plate", range(0, 1), 0.25f, percentAttributes()));
    p.push_back(std::make_unique<juce::AudioParameterFloat>("ambient", "Ambient Reverb", range(0, 1), 0.15f, percentAttributes()));
    p.push_back(std::make_unique<juce::AudioParameterFloat>("delayMix", "Delay", range(0, 1), 0.08f, percentAttributes()));
    p.push_back(std::make_unique<juce::AudioParameterFloat>("delayMs", "Delay Time", range(20, 800, 1), 115.0f, "ms"));
    p.push_back(std::make_unique<juce::AudioParameterFloat>("outputGain", "Output", range(-24, 12, 0.01f), 0.0f, "dB"));
    return { p.begin(), p.end() };
}

void AfroVocalPresetsAudioProcessor::prepareToPlay(double sampleRate, int samplesPerBlock)
{
    const auto channels = juce::jmax(1, getTotalNumOutputChannels()); channelDSP->prepare(sampleRate, samplesPerBlock, channels);
    outputGain.reset(sampleRate, 0.05); outputGain.setCurrentAndTargetValue(1.0f); setLatencySamples(0);
}
void AfroVocalPresetsAudioProcessor::releaseResources() { channelDSP->reset(); }
void AfroVocalPresetsAudioProcessor::reset() { channelDSP->reset(); outputGain.setCurrentAndTargetValue(juce::Decibels::decibelsToGain(outputGainParam->load(std::memory_order_relaxed))); }
bool AfroVocalPresetsAudioProcessor::isBusesLayoutSupported(const BusesLayout& layouts) const
{
    const auto out = layouts.getMainOutputChannelSet(); return (out == juce::AudioChannelSet::mono() || out == juce::AudioChannelSet::stereo()) && layouts.getMainInputChannelSet() == out;
}
void AfroVocalPresetsAudioProcessor::processBlockBypassed(juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midi)
{
    juce::ignoreUnused(midi); juce::ScopedNoDenormals noDenormals; for (int ch = getTotalNumInputChannels(); ch < getTotalNumOutputChannels(); ++ch) buffer.clear(ch, 0, buffer.getNumSamples());
}
void AfroVocalPresetsAudioProcessor::processBlock(juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midi)
{
    juce::ignoreUnused(midi); juce::ScopedNoDenormals noDenormals;
    if (buffer.getNumSamples() > channelDSP->preparedMaximum || buffer.getNumChannels() > channelDSP->preparedChannels) { buffer.clear(); return; }
    if (bypassParam->load(std::memory_order_relaxed) > 0.5f) { processBlockBypassed(buffer, midi); return; }
    channelDSP->process(buffer, tuneAmountParam->load(std::memory_order_relaxed), retuneSpeedParam->load(std::memory_order_relaxed),
                        static_cast<int>(keyParam->load(std::memory_order_relaxed)), static_cast<int>(scaleParam->load(std::memory_order_relaxed)),
                        highPassParam->load(std::memory_order_relaxed), lowMidParam->load(std::memory_order_relaxed), presenceParam->load(std::memory_order_relaxed),
                        airParam->load(std::memory_order_relaxed) * 8.0f, compThresholdParam->load(std::memory_order_relaxed), compRatioParam->load(std::memory_order_relaxed),
                        compAttackParam->load(std::memory_order_relaxed), compReleaseParam->load(std::memory_order_relaxed), deEssParam->load(std::memory_order_relaxed),
                        warmthParam->load(std::memory_order_relaxed), parallelParam->load(std::memory_order_relaxed), plateParam->load(std::memory_order_relaxed),
                        ambientParam->load(std::memory_order_relaxed), delayMixParam->load(std::memory_order_relaxed), delayMsParam->load(std::memory_order_relaxed));
    outputGain.setTargetValue(juce::Decibels::decibelsToGain(outputGainParam->load(std::memory_order_relaxed)));
    float peak = 0.0f;
    for (int s = 0; s < buffer.getNumSamples(); ++s) { const float g = outputGain.getNextValue(); for (int ch = 0; ch < buffer.getNumChannels(); ++ch) { const float y = buffer.getSample(ch, s) * g; buffer.setSample(ch, s, y); peak = juce::jmax(peak, std::abs(y)); } }
    meterPeak.store(juce::jlimit(0.0f, 1.0f, peak), std::memory_order_relaxed);
    gainReduction.store(juce::jlimit(0.0f, 1.0f, compThresholdParam->load(std::memory_order_relaxed) < -20.0f ? 0.18f : 0.07f), std::memory_order_relaxed);
}
juce::AudioProcessorParameter* AfroVocalPresetsAudioProcessor::getBypassParameter() const { return parameters.getParameter("bypass"); }
juce::AudioProcessorEditor* AfroVocalPresetsAudioProcessor::createEditor() { return new AfroVocalPresetsAudioProcessorEditor(*this); }

void AfroVocalPresetsAudioProcessor::applyFactoryPreset(const juce::String& name)
{
    struct V { float tune, hp, low, pres, air, thr, ratio, deess, warm, par, plate, amb, delay, delayMs, out; } v { .35f, 75, -2.5f, 2.5f, .45f, -18, 3, .35f, .30f, .10f, .25f, .15f, .08f, 115, 0 };
    if (name == "Afrobeat Lead - Clear Bounce") v = { .42f, 80, -2, 3.5f, .55f, -18, 3.2f, .38f, .28f, .12f, .24f, .12f, .12f, 118, -.5f };
    else if (name == "Afrobeat Lead - Warm Pocket") v = { .28f, 70, -3, 2, .35f, -20, 2.8f, .30f, .50f, .18f, .18f, .10f, .10f, 145, -.5f };
    else if (name == "Amapiano Lead - Gloss") v = { .58f, 95, -2.5f, 4.5f, .65f, -16, 3.5f, .45f, .20f, .10f, .30f, .18f, .15f, 132, -1 };
    else if (name == "Amapiano Lead - Soft Air") v = { .35f, 85, -3, 2.5f, .75f, -19, 3, .32f, .25f, .08f, .25f, .24f, .08f, 180, -1 };
    else if (name == "Emotional Ballad - Intimate") v = { .22f, 65, -2, 1.5f, .30f, -20, 2.5f, .25f, .35f, .08f, .32f, .42f, .10f, 260, -1.5f };
    else if (name == "Emotional Ballad - Wide") v = { .30f, 75, -2.5f, 2, .45f, -21, 2.3f, .25f, .28f, .06f, .28f, .62f, .16f, 420, -2 };
    else if (name == "Background Harmony - Tucked") v = { .72f, 130, -4, 1, .20f, -17, 4.5f, .55f, .18f, .25f, .30f, .45f, .20f, 230, -3 };
    else if (name == "Background Harmony - Airy Stack") v = { .65f, 150, -3, 3, .65f, -18, 4, .45f, .12f, .18f, .38f, .50f, .22f, 310, -3 };
    else if (name == "Hard-Tuned Lead - Modern") v = { 1, 90, -2, 3, .45f, -15, 4, .42f, .22f, .12f, .18f, .08f, .11f, 105, -.5f };
    else if (name == "Hard-Tuned Lead - Dry") v = { 1, 110, -3, 2, .25f, -14, 5, .50f, .18f, .08f, .08f, .04f, .04f, 90, -.5f };
    auto set = [this](const char* id, float value) { if (auto* p = parameters.getParameter(id)) p->setValueNotifyingHost(p->convertTo0to1(value)); };
    set("tuneAmount", v.tune); set("highPass", v.hp); set("lowMidCut", v.low); set("presence", v.pres); set("air", v.air); set("compThreshold", v.thr); set("compRatio", v.ratio); set("deEss", v.deess); set("warmth", v.warm); set("parallelComp", v.par); set("plate", v.plate); set("ambient", v.amb); set("delayMix", v.delay); set("delayMs", v.delayMs); set("outputGain", v.out);
    const juce::ScopedLock lock(metadataLock); lastPreset = name;
}

juce::String AfroVocalPresetsAudioProcessor::getLastPreset() const { const juce::ScopedLock lock(metadataLock); return lastPreset; }
void AfroVocalPresetsAudioProcessor::getStateInformation(juce::MemoryBlock& dest)
{
    auto state = parameters.copyState(); { const juce::ScopedLock lock(metadataLock); state.setProperty("lastPreset", lastPreset, nullptr); } state.setProperty("schemaVersion", 2, nullptr); if (auto xml = state.createXml()) copyXmlToBinary(*xml, dest);
}
void AfroVocalPresetsAudioProcessor::setStateInformation(const void* data, int size)
{
    if (size <= 0 || size > 4 * 1024 * 1024) return; if (auto xml = getXmlFromBinary(data, size)) if (xml->hasTagName(parameters.state.getType())) { parameters.replaceState(juce::ValueTree::fromXml(*xml)); const juce::ScopedLock lock(metadataLock); lastPreset = xml->getStringAttribute("lastPreset", lastPreset); }
}
void AfroVocalPresetsAudioProcessor::setAiStatus(const juce::String& status)
{
    { const juce::ScopedLock lock(aiStatusLock); aiStatus = status; } aiBusy.store(status.contains("creating"), std::memory_order_relaxed);
}
juce::String AfroVocalPresetsAudioProcessor::getAiStatus() const { const juce::ScopedLock lock(aiStatusLock); return aiStatus; }
void AfroVocalPresetsAudioProcessor::requestAiPreset(const juce::String& prompt, const juce::String& key) { if (prompt.trim().isEmpty()) { setAiStatus("AI needs a short vocal description first."); return; } aiBusy.store(true, std::memory_order_relaxed); aiWorker->startRequest(prompt, key); }
void AfroVocalPresetsAudioProcessor::queueAiPreset(const juce::var& json) { const juce::ScopedLock lock(pendingAiLock); pendingAiJson = json; pendingAiReady = true; }
bool AfroVocalPresetsAudioProcessor::applyPendingAiPreset()
{
    juce::var json; { const juce::ScopedLock lock(pendingAiLock); if (!pendingAiReady) return false; json = pendingAiJson; pendingAiJson = juce::var(); pendingAiReady = false; }
    applyAiJson(json); setAiStatus("AI preset loaded successfully."); return true;
}
void AfroVocalPresetsAudioProcessor::applyAiJson(const juce::var& json)
{
    auto* o = json.getDynamicObject(); if (o == nullptr) return;
    const std::pair<const char*, std::pair<float, float>> limits[] = { {"tuneAmount", {0, 1}}, {"air", {0, 1}}, {"outputGain", {-24, 12}}, {"highPass", {40, 220}}, {"lowMidCut", {-9, 3}}, {"presence", {-3, 8}}, {"compThreshold", {-36, -6}}, {"compRatio", {1, 12}}, {"deEss", {0, 1}}, {"parallelComp", {0, 1}}, {"plate", {0, 1}}, {"ambient", {0, 1}}, {"delayMix", {0, 1}}, {"delayMs", {20, 800}}, {"warmth", {0, 1}} };
    for (const auto& item : limits) if (o->hasProperty(item.first)) { const float v = clampFinite(static_cast<float>(o->getProperty(item.first)), item.second.first, item.second.second, item.second.first); if (auto* p = parameters.getParameter(item.first)) p->setValueNotifyingHost(p->convertTo0to1(v)); }
    if (o->hasProperty("presetName")) { const juce::ScopedLock lock(metadataLock); lastPreset = o->getProperty("presetName").toString().substring(0, 80); }
}

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter() { return new AfroVocalPresetsAudioProcessor(); }
